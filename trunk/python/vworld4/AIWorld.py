""" PandaWorld - simple virtual environment with wandering pandas.

This module is intended to be called by Avatar.py;
To run it independently, uncomment the end of this code:
	# w = PandaWorld()
	# (spawning NPCs...)
 	# run()
and type "python PandaWorld.py"
 
control:
	Move   -> Up, Left, Right, Down
	Rotate -> A, S
	Strafe -> Z, X

Last Updated: 2/12/2013
Author: Chao Wang and Jon Turner
World Model: Chao Wang
 
Adapted from "Roaming Ralph", a tutorial included in Panda3D package.
Author: Ryan Myers
Models: Jeff Styers, Reagan Heller
"""  

import direct.directbase.DirectStart
from panda3d.core import *
from direct.gui.OnscreenText import OnscreenText
from direct.gui.OnscreenImage import OnscreenImage
from direct.actor.Actor import Actor
from direct.showbase.DirectObject import DirectObject
from direct.interval.IntervalGlobal import Sequence
from panda3d.core import Point3
import random, sys, os, math, re

from Util import *

def addTitle(text):
	""" Put title on the screen
	"""
	return OnscreenText(text=text, style=1, fg=(1,1,1,1), \
		pos=(0,-0.95), align=TextNode.ACenter, scale = .07)

def printText(pos):
	""" Print text on the screen
	"""
	return OnscreenText( \
		text=" ", \
		style=2, fg=(1,0.8,0.7,1), pos=(-1.3, pos), \
		align=TextNode.ALeft, scale = .06, mayChange = True)


class AIWorld(DirectObject):
	def __init__(self):
 
		base.win.setClearColor(Vec4(0,0,0,1))

		# Add title and show map
		self.title = addTitle("AI's viewpoint")
		self.Dmap = OnscreenImage(image = 'models/2Dmap.png', \
					  pos = (.8,0,.6), scale = .4)
		self.Dmap.setTransparency(TransparencyAttrib.MAlpha)
		self.dot = OnscreenImage(image = 'models/dot.png', \
					 pos = (1,0,1), scale = .01)
	
		
		# Set up the environment
		#
		# This environment model contains collision meshes.  If you look
		# in the egg file, you will see the following:
		#
		#	<Collide> { Polyset keep descend }
		#
		# This tag causes the following mesh to be converted to a 
		# collision mesh -- a mesh which is optimized for collision,
		# not rendering. It also keeps the original mesh, so there
		# are now two copies --- one optimized for rendering,
		# one for collisions.  

		self.environ = loader.loadModel("models/vworld-grid-24")
		self.environ.reparentTo(render)
		self.environ.setPos(0,0,0)
		
 		base.setBackgroundColor(0.7,0.23,0.4,1)
		
		# Create the panda
		self.avatar = Actor("models/panda-model", \
				    {"run":"models/panda-walk4", \
				    "walk":"models/panda-walk"})
		self.avatar.reparentTo(render)
		self.avatar.setScale(.002)
		self.avatar.setPos(58,67,0)

		self.rotateDir = -1
		self.rotateDuration = -1
		self.moveDir = -1
		self.moveDuration = -1
		self.isAvoidingCollision = False
		self.inBigRotate = False # if True, do not move forward; only rotate

		# Set the dot's position
		self.dot.setPos(self.avatar.getX()/(120.0+self.Dmap.getX()), \
				0,self.avatar.getY()/(120.0+self.Dmap.getZ()))
		self.dotOrigin = self.dot.getPos()

		# Set the upper bound of # of remote avatars
		self.maxRemotes = 100

		# Setup pool of remotes
		# do not attach to scene yet
		self.remoteMap = {}
		self.freeRemotes = []
		for i in range(0,self.maxRemotes) : # allow up to 100 remotes
			self.freeRemotes.append(Actor("models/panda-model", \
						{"run":"models/panda-walk4"}))
			self.freeRemotes[i].setScale(.002)

		# Create a floater object.  We use the "floater" as a temporary
		# variable in a variety of calculations.
		self.floater = NodePath(PandaNode("floater"))
		self.floater.reparentTo(render)

		# Accept the control keys for movement and rotation
		self.accept("escape", sys.exit)

		taskMgr.add(self.move,"moveTask")

		# Create some lighting
		self.ambientLight = render.attachNewNode( AmbientLight( "ambientLight" ) )
		self.ambientLight.node().setColor( Vec4( .8, .8, .8, 1 ) )
		render.setLight(self.ambientLight)

		# Game state variables
		self.isMoving = False

		# Set up the camera
		base.disableMouse()
		base.camera.setPos(self.avatar.getX(),self.avatar.getY(),.2)
		base.camera.setHpr(self.avatar.getHpr()[0],0,0)
		
		self.cTrav = CollisionTraverser()

 		"""
 		CollisionSphere that detects walkable region.
 		A move is illegal if the groundRay hits but the terrain.
 		"""
		self.avatarCS = CollisionSphere(0,0,0,1)
		self.avatarGroundCol = CollisionNode('avatarSphere')
		self.avatarGroundCol.addSolid(self.avatarCS)
		self.avatarGroundCol.setFromCollideMask(BitMask32.bit(0))
		self.avatarGroundCol.setIntoCollideMask(BitMask32.allOff())
		self.avatarGroundColNp = \
			self.avatar.attachNewNode(self.avatarGroundCol)
		self.avatarCS.setRadius(500)
		self.avatarCS.setCenter(0,-80,300)
		self.avatarGroundHandler = CollisionHandlerQueue()
		self.cTrav.addCollider(self.avatarGroundColNp, \
			self.avatarGroundHandler)

		# setup collision detection objects used to implement
		# canSee method
		self.csSeg = CollisionSegment()
		self.csSeg.setPointA(Point3(0,0,0))
		self.csSeg.setPointB(Point3(0,0,.5))

		self.csNode = self.environ.attachNewNode(CollisionNode('csSeg'))
		self.csNode.node().addSolid(self.csSeg)
		self.csNode.node().setFromCollideMask(BitMask32.bit(0))
		self.csNode.node().setIntoCollideMask(BitMask32.allOff())

		self.csHandler = CollisionHandlerQueue()

		self.csTrav = CollisionTraverser('CustomTraverser')
		self.csTrav.addCollider(self.csNode, self.csHandler)

	def addRemote(self, x, y, direction, id) : 
		""" Add a remote panda.

		This method is used by the Net object when it starts
		getting packets from a panda that it did not previously
		know about.
		x,y gives the remote's position
		direction gives its heading
		id is an identifier used to distinguish this remote from others

		New remotes are included in the scene graph.
		"""
		if id in self.remoteMap :
			self.updateRemote(x,y,direction,id); return
		if len(self.freeRemotes) == 0 :
			sys.stderr.write("PandaWorld.addRemote: no unused " + \
					 "remotes\n")
			sys.exit(1)
		remote = self.freeRemotes.pop()
		self.remoteMap[id] = [remote, True ,
				OnscreenImage(image = 'models/dot1.png', \
						pos = (0,0,0), scale = .01)
				]

		# set position and direction of remote and make it visible
		remote.reparentTo(render)
		remote.setPos(x,y,0) 
		remote.setHpr(direction,0,0) 
		remote.loop("run")

	def updateRemote(self, x, y, direction, id) :
		""" Update location of a remote panda.

		This method is used by the Net object to update the location.
		x,y gives the remote's position
		direction gives its heading
		id is an identifier used to distinguish this remote from others
		"""
		if id not in self.remoteMap : return
		remote, isMoving, dot = self.remoteMap[id]
		if abs(x - remote.getX()) < .001 and \
		   abs(y - remote.getY()) < .001 and \
		   direction == remote.getHpr()[0] :
			remote.stop(); remote.pose("run",5)
			self.remoteMap[id][1] = False
			return
		elif not isMoving :
			remote.loop("run")
			self.remoteMap[id][1] = True
		
		# set position and direction of remote
		remote.setPos(x,y,0)
		remote.setHpr(direction,0,0) 

	def removeRemote(self, id) :
		""" Remove a remote when it goes out of range.
		
		id is the identifier for the remote
		"""
		if id not in self.remoteMap : return
		self.remoteMap[id][2].destroy()
		remote = self.remoteMap[id][0]
		remote.detachNode() # ??? check this
		self.freeRemotes.append(remote)
		del self.remoteMap[id]

	def getPosHeading(self) :
		""" Get the position and heading.
	
		Intended for use by a Net object that must track avatar's 
		position return a tuple containing the x-coordinate,
		y-coordinate, heading.
		"""
		return (self.avatar.getX(), self.avatar.getY(), \
			(self.avatar.getHpr()[0])%360)

	def getLimit(self) :
		""" Get the limit on the xy coordinates.
		"""
		return 120

	def canSee(self, p1, p2):
		""" Test if two points are visible from one another
	
		It works by setting up a collisionSegment between the
		two positions. Return True if the segment didn't hit
		any from/into object in between.
	
		An alternative is to create a pool of rays etc. in
		PandaWorld class, like what we've done with remotes, and pass
		them here, so that we don't create objects on the fly. But
		for canSee(), the current version demands less computation
		resources.
		"""

		# lift two points a bit above the grouond to prevent the
		# collision ray from hiting the edge of shallow terrain;
		# also, put them at different level so that the ray has
		# nonzero length (a requirement for collisionSegment()).
		p1[2] += 0.5
		p2[2] += 0.4
		self.csSeg.setPointA(p1)
		self.csSeg.setPointB(p2)
	
		self.csTrav.traverse(render)
	
		return (self.csHandler.getNumEntries() == 0)


	def move(self, task):
		""" Update the position of this avatar.

		This method is called by the task scheduler on every frame.
		It responds to the arrow keys to move the avatar
		and to the camera up/down keys.
		"""

		# save avatar's initial position so that we can restore it,
		# in case he falls off the map or runs into something.
		startpos = self.avatar.getPos()

		# randomly move the avatar
		if self.rotateDir == -1:
			self.rotateDir = random.randint(1,5) # 40% chance to rotate
		if self.rotateDuration == -1:
			self.rotateDuration = random.randint(30,80)
#		if self.moveDir == -1:
#			self.moveDir = random.randint(1,10)
#		if self.moveDuration == -1:
#			self.moveDuration = random.randint(100,400)

		if self.rotateDir == 1 : # turn left
			if self.rotateDuration > 50 : self.inBigRotate = True
			self.avatar.setH(self.avatar.getH() + \
							 10 * globalClock.getDt())
			self.rotateDuration -= 1
			if not self.rotateDuration:
				self.rotateDuration = -1
				self.rotateDir = -1
				self.inBigRotate = False
		elif self.rotateDir == 2 : # turn right
			if self.rotateDuration > 50 : self.inBigRotate = True
			self.avatar.setH(self.avatar.getH() - \
							 10 * globalClock.getDt())
			self.rotateDuration -= 1
			if not self.rotateDuration:
				self.rotateDuration = -1
				self.rotateDir = -1
				self.inBigRotate = False
		else : # do not rotate; keep moving forward
			self.rotateDuration -= 1
			if not self.rotateDuration:
				self.rotateDuration = -1
				self.rotateDir = -1

#		if self.moveDir > 2 and not self.isAvoidingCollision: # move forward
		if not self.inBigRotate and not self.isAvoidingCollision: # move forward
			self.avatar.setY(self.avatar, -200 * globalClock.getDt())
#			self.moveDuration -= 1
#			if not self.moveDuration:
#				self.moveDuration = -1
#				self.moveDir = -1
#		elif not self.isAvoidingCollision: # move backward
#			self.avatar.setY(self.avatar, +200 * globalClock.getDt())
#			self.moveDuration -= 1
#			if not self.moveDuration:
#				self.moveDuration = -1
#				self.moveDir = -1

		# If avatar is moving, loop the run animation.
		# If he is standing still, stop the animation.
		self.avatar.loop("run")
		self.isMoving = True

		# position the camera where the avatar is
		pos = self.avatar.getPos(); pos[2] += 1
		hpr = self.avatar.getHpr();
		hpr[0] += 180; hpr[1] = 0 #camAngle
		base.camera.setPos(pos); base.camera.setHpr(hpr)
		
		# Now check for collisions.
		self.cTrav.traverse(render)

		# Adjust avatar's Z coordinate.  If avatar's ray hit terrain,
		# update his Z. If it hit anything else, or didn't hit 
		# anything, put him back where he was last frame.
		entries = []
		for i in range(self.avatarGroundHandler.getNumEntries()):
			entries.append(self.avatarGroundHandler.getEntry(i))
		collide = False
		if (len(entries)>0) :
			for entry in entries :
				if entry.getIntoNode().getName() != "ID257" : # tag of terrain
					collide = True
			if collide == True :
				self.isAvoidingCollision = True
				self.avatar.setPos(startpos)
				# take a left turn
				self.avatar.setH(self.avatar.getH() + \
						 50 * globalClock.getDt())
			else:
				self.isAvoidingCollision = False
				self.avatar.setZ( \
					entries[0].getSurfacePoint(render).getZ())

		# map the avatar's position to the 2D map on the top-right 
		# corner of the screen
		self.dot.setPos((self.avatar.getX()/120.0)*0.7+0.45, \
				0,(self.avatar.getY()/120.0)*0.7+0.25)
		for id in self.remoteMap:
			self.remoteMap[id][2].setPos((self.remoteMap[id][0].getX()/120.0)*0.7+0.45, \
				0,(self.remoteMap[id][0].getY()/120.0)*0.7+0.25)

		return task.cont

#w = AIWorld()
#w.addRemote(69, 67, 135, 111)
#w.addRemote(20, 33, 135, 222)
#w.addRemote(40, 60, 135, 333)
#w.addRemote(90, 79, 135, 444)
#w.addRemote(30, 79, 135, 555)
#w.addRemote(20, 39, 135, 666)
#run()
