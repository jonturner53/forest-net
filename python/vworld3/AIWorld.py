""" AIWorld - a virtual environment for each bot.

Author: Chao Wang and Jon Turner
World Model: Chao Wang
 
Adapted from "Roaming Ralph", a sample program included in Panda3D package.
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

#def addTitle(text):
#	""" Put title on the screen
#	"""
#	return OnscreenText(text=text, style=1, fg=(1,1,1,1), \
#		pos=(0,-0.95), align=TextNode.ACenter, scale = .07)
#
#def printText(pos):
#	""" Print text on the screen
#	"""
#	return OnscreenText( \
#		text=" ", \
#		style=2, fg=(1,0.8,0.7,1), pos=(-1.3, pos), \
#		align=TextNode.ALeft, scale = .06, mayChange = True)


class AIWorld(DirectObject):
	def __init__(self):

# uncomment these lines to show up the display 
#		base.windowType = 'onscreen' 
#		wp = WindowProperties.getDefault() 
#		base.openDefaultWindow(props = wp)
 
#		base.win.setClearColor(Vec4(0,0,0,1))

		# Add title and show map
#		self.title = addTitle("AI's viewpoint")
#		self.Dmap = OnscreenImage(image = 'models/2Dmap.png', \
#					  pos = (.8,0,.6), scale = .4)
#		self.Dmap.setTransparency(TransparencyAttrib.MAlpha)
#		self.dot = OnscreenImage(image = 'models/dot.png', \
#					 pos = (1,0,1), scale = .01)
	
		
		# Set up the environment

		self.environ = loader.loadModel("models/vworld24grid")
		self.environ.reparentTo(render)
		self.environ.setPos(0,0,0)
		
# 		base.setBackgroundColor(0.7,0.23,0.4,1)
		
		# Create the panda
		self.avatar = Actor("models/panda-model", \
				    {"run":"models/panda-walk4", \
				    "walk":"models/panda-walk"})
		self.avatar.reparentTo(render)
		self.avatar.setScale(.002)
		self.avatar.setPos(58,67,0)
		self.avatar.setHpr(random.randint(0,359),0,0) 

		self.rotateDir = -1
		self.rotateDuration = -1
		self.moveDir = -1
		self.moveDuration = -1
		self.isAvoidingCollision = False
		self.inBigRotate = False # if True, do not move forward; only rotate

		# Set the dot's position
#		self.dot.setPos(self.avatar.getX()/(120.0+self.Dmap.getX()), \
#				0,self.avatar.getY()/(120.0+self.Dmap.getZ()))
#		self.dotOrigin = self.dot.getPos()

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

		# Accept the control keys for movement and rotation
		self.accept("escape", sys.exit)

		taskMgr.add(self.move,"moveTask")

#		# Create some lighting
#		self.ambientLight = render.attachNewNode( AmbientLight( "ambientLight" ) )
#		self.ambientLight.node().setColor( Vec4( .8, .8, .8, 1 ) )
#		render.setLight(self.ambientLight)

		# Game state variables
		self.isMoving = False

		# Set up the camera
#		base.disableMouse()
#		base.camera.setPos(self.avatar.getX(),self.avatar.getY(),.2)
#		base.camera.setHpr(self.avatar.getHpr()[0],0,0)
		
		self.cTrav = CollisionTraverser()

 		"""
 		Setup collision Solids for the two purpose:
 		1. prevent the avatar from passing through a wall
		2. keep the avatar on the ground (we skip it for now)
 		"""
		base.cTrav = CollisionTraverser()
		base.cTrav.setRespectPrevTransform(True)

		self.avatarSpNp = \
			self.avatar.attachNewNode(CollisionNode('avatarSphere'))
		
		self.csSphere = self.avatarSpNp.node().addSolid(
			CollisionSphere(0.0/self.avatar.getScale().getX(),
							-0.5/self.avatar.getScale().getY(),
							0.5/self.avatar.getScale().getZ(),
							0.8/self.avatar.getScale().getX()))
		# format above: (x,y,z,radius)

		self.avatarSpNp.node().setFromCollideMask(BitMask32.bit(0))
		self.avatarSpNp.node().setIntoCollideMask(BitMask32.allOff())

		# uncomment the following line to show the collisionSphere
		#self.avatarSpNp.show()

		self.avatarGroundHandler = CollisionHandlerPusher()
		self.avatarGroundHandler.setHorizontal(True)
		self.avatarGroundHandler.addCollider(self.avatarSpNp, self.avatar)
		base.cTrav.addCollider(self.avatarSpNp,	self.avatarGroundHandler)


		# setup a collision solid for the canSee method
		self.auxCSNp = self.environ.attachNewNode(CollisionNode('csForCanSee'))
		self.auxCSSolid = self.auxCSNp.node().addSolid(CollisionSegment())
		self.auxCSNp.node().setFromCollideMask(BitMask32.bit(0))
		self.auxCSNp.node().setIntoCollideMask(BitMask32.allOff())

		self.csHandler = CollisionHandlerQueue()

		self.csTrav = CollisionTraverser('CustomTraverser')
		self.csTrav.addCollider(self.auxCSNp, self.csHandler)

		# uncomment the following line to show where collision occurs
		#self.csTrav.showCollisions(render)


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
		self.remoteMap[id] = [remote, True
	#	self.remoteMap[id] = [remote, True ,
	#			OnscreenImage(image = 'models/dot1.png', \
	#					pos = (0,0,0), scale = .01)
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
#		remote, isMoving, dot = self.remoteMap[id]
		remote, isMoving = self.remoteMap[id]
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
#		self.remoteMap[id][2].destroy()
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
		self.auxCSNp.node().modifySolid(self.auxCSSolid).setPointA(p1)
		self.auxCSNp.node().modifySolid(self.auxCSSolid).setPointB(p2)
	
		self.csTrav.traverse(render)
	
		return (self.csHandler.getNumEntries() == 0)


	def move(self, task):
		""" Update the position of this avatar.

		This method is called by the task scheduler on every frame.
		It guides the AI.
		"""

		if self.rotateDir == -1:
			self.rotateDir = random.randint(1,25) # chances to rotate
		if self.rotateDuration == -1:
			self.rotateDuration = random.randint(200,400)

		# guide the moving direction of the bot
		if self.rotateDir <= 3 : # turn left
			self.avatar.setH(self.avatar.getH() + \
							 40 * globalClock.getDt())
			self.rotateDuration -= 1
			if self.rotateDuration <= 0:
				self.rotateDuration = -1
				self.rotateDir = -1
		elif self.rotateDir <= 6 : # turn right
			self.avatar.setH(self.avatar.getH() - \
							 50 * globalClock.getDt())
			self.rotateDuration -= 1
			if self.rotateDuration <= 0:
				self.rotateDuration = -1
				self.rotateDir = -1
		elif self.rotateDir == 7 : # turn big left
			self.avatar.setH(self.avatar.getH() + \
							 102 * globalClock.getDt())
			self.rotateDuration -= 1
			if self.rotateDuration <= 0:
				self.rotateDuration = -1
				self.rotateDir = -1
		elif self.rotateDir == 8 : # turn big right
			self.avatar.setH(self.avatar.getH() - \
							 102 * globalClock.getDt())
			self.rotateDuration -= 1
			if self.rotateDuration <= 0:
				self.rotateDuration = -1
				self.rotateDir = -1
		else :
			self.rotateDuration -= 1
			if self.rotateDuration <= 0:
				self.rotateDuration = -1
				self.rotateDir = -1
			self.avatar.setFluidPos(self.avatar,
									0,
									-60 * globalClock.getDt(),
									self.avatar.getZ() )
		# moving forward
		self.avatar.setFluidPos(self.avatar,
								0,
								-420 * globalClock.getDt(),
								self.avatar.getZ() )


		# position the camera where the avatar is
#		pos = self.avatar.getPos(); pos[2] += 1
#		hpr = self.avatar.getHpr();
#		hpr[0] += 180; hpr[1] = 0 #camAngle
#		base.camera.setPos(pos); base.camera.setHpr(hpr)


#		# map the avatar's position to the 2D map on the top-right 
#		# corner of the screen
#		self.dot.setPos((self.avatar.getX()/120.0)*0.7+0.45, \
#				0,(self.avatar.getY()/120.0)*0.7+0.25)
#		for id in self.remoteMap:
#			self.remoteMap[id][2].setPos((self.remoteMap[id][0].getX()/120.0)*0.7+0.45, \
#				0,(self.remoteMap[id][0].getY()/120.0)*0.7+0.25)

		return task.cont

