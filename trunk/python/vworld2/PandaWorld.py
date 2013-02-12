""" PandaWorld - simple virtual environment with wandering pandas.

This module is intended to be called by Avatar.py;
To run it independently, uncomment the end of this code:
	# w = PandaWorld()
	# (what in bewteens are NPCs for test)
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

# Function to put title on the screen.
def addTitle(text):
	return OnscreenText(text=text, style=1, fg=(1,1,1,1), \
		pos=(1.3,-0.95), align=TextNode.ARight, scale = .07)

# Function to print text on the screen.
def printText(pos):
	return OnscreenText( \
		text=" ", \
		style=2, fg=(1,0.8,0.7,1), pos=(-1.3, pos), \
		align=TextNode.ALeft, scale = .06, mayChange = True)

# Function to test if two points is visible from each other
def canSee(self, p1, p2):
	traverser = CollisionTraverser('CustomTraverser')

	ray = CollisionSegment()
	# make the positions above the grouond;
	# otherwise, the ray might hit the edge of shallow terrain
	p1[2] += 0.5
	p2[2] += 0.5
	ray.setPointA(p1)
	ray.setPointB(p2)
	fromObj = self.environ.attachNewNode(CollisionNode('visRay'))
	fromObj.node().addSolid(ray)
	fromObj.node().setFromCollideMask(BitMask32.bit(0))
	fromObj.node().setIntoCollideMask(BitMask32.allOff())
	handler = CollisionHandlerQueue()

	traverser.addCollider(fromObj, handler)

	traverser.traverse(render)

	if handler.getNumEntries() != 0:
		return False
	else:
		return True


class PandaWorld(DirectObject):
	def __init__(self):
 		self.keyMap = {"left":0, "right":0, "forward":0, "backward":0, \
 				"cam-up":0, "cam-down":0, \
 				"strafe-left":0, "strafe-right":0}
		base.win.setClearColor(Vec4(0,0,0,1))

		# Add title and show map
		self.title = addTitle("wandering pandas")
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
		
 		base.setBackgroundColor(0.2,0.23,0.4,1)
		
		# Create the panda
		self.avatar = Actor("models/panda-model", \
				    {"run":"models/panda-walk4", \
				    "walk":"models/panda-walk"})
		self.avatar.reparentTo(render)
		self.avatar.setScale(.002)
		self.avatar.setPos(58,67,0)

		# set the dot's position
		self.dot.setPos(self.avatar.getX()/(120.0+self.Dmap.getX()), \
				0,self.avatar.getY()/(120.0+self.Dmap.getZ()))
		self.dotOrigin = self.dot.getPos()

		# Show avatar's position on the screen
		self.avPos = printText(0.9)

		# Set the upper bound of # of remote avatars
		self.maxRemotes = 100

 		# Show a list of visible NPCs
 		self.showNumVisible = printText(0.8)
 		self.visList = []

		# setup pool of remotes
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
		self.accept("arrow_left", self.setKey, ["left",1])
		self.accept("arrow_right", self.setKey, ["right",1])
		self.accept("arrow_up", self.setKey, ["forward",1])
 		self.accept("arrow_down", self.setKey, ["backward",1])
		self.accept("a", self.setKey, ["cam-up",1])
		self.accept("s", self.setKey, ["cam-down",1])
 		self.accept("z", self.setKey, ["strafe-left",1])
 		self.accept("x", self.setKey, ["strafe-right",1])
		self.accept("arrow_left-up", self.setKey, ["left",0])
		self.accept("arrow_right-up", self.setKey, ["right",0])
		self.accept("arrow_up-up", self.setKey, ["forward",0])
 		self.accept("arrow_down-up", self.setKey, ["backward",0])
		self.accept("a-up", self.setKey, ["cam-up",0])
		self.accept("s-up", self.setKey, ["cam-down",0])
 		self.accept("z-up", self.setKey, ["strafe-left",0])
 		self.accept("x-up", self.setKey, ["strafe-right",0])

		taskMgr.add(self.move,"moveTask")

		# Game state variables
		self.isMoving = False

		# Set up the camera
		base.disableMouse()
		base.camera.setPos(self.avatar.getX(),self.avatar.getY(),.2)
		base.camera.setHpr(self.avatar.getHpr()[0],0,0)
		
		self.cTrav = CollisionTraverser()

 		"""
 		CollisionRay that detects walkable region.
 		A move is illegal if the groundRay hits but the terrain.
 		"""
		# TODO: maybe we don't need avatarGroundRay; camGroundRay suffices.
		self.avatarGroundRay = CollisionRay()
		self.avatarGroundRay.setOrigin(0,0,1000)
		self.avatarGroundRay.setDirection(0,0,-1)
		self.avatarGroundCol = CollisionNode('avatarRay')
		self.avatarGroundCol.addSolid(self.avatarGroundRay)
		self.avatarGroundCol.setFromCollideMask(BitMask32.bit(0))
		self.avatarGroundCol.setIntoCollideMask(BitMask32.allOff())
		self.avatarGroundColNp = \
			self.avatar.attachNewNode(self.avatarGroundCol)
		self.avatarGroundHandler = CollisionHandlerQueue()
		self.cTrav.addCollider(self.avatarGroundColNp, \
			self.avatarGroundHandler)

		self.camGroundRay = CollisionRay()
		self.camGroundRay.setOrigin(0,0,1000)
		self.camGroundRay.setDirection(0,0,-1)
		self.camGroundCol = CollisionNode('camRay')
		self.camGroundCol.addSolid(self.camGroundRay)
		self.camGroundCol.setFromCollideMask(BitMask32.bit(0))
		self.camGroundCol.setIntoCollideMask(BitMask32.allOff())
		self.camGroundColNp = \
			base.camera.attachNewNode(self.camGroundCol)
		self.camGroundHandler = CollisionHandlerQueue()
		self.cTrav.addCollider(self.camGroundColNp, \
			self.camGroundHandler)

		# Create some lighting
		self.ambientLight = render.attachNewNode( AmbientLight( "ambientLight" ) )
		self.ambientLight.node().setColor( Vec4( .8, .8, .8, 1 ) )
		render.setLight(self.ambientLight)

		dLight1 = DirectionalLight("dLight1")
		dLight1.setColor(Vec4(1, .6, .7, 1))
		dLight1.setDirection(Vec3(1,1,1))
		dlnp1 = render.attachNewNode(dLight1)
		dlnp1.setHpr(30,-160,0)
		render.setLight(dlnp1)

		dLight2 = DirectionalLight("dLight2")
		dLight2.setColor(Vec4(.6, .7, 1, 1))
		dLight2.setDirection(Vec3(-1,-1,-1))
		self.dlnp2 = render.attachNewNode(dLight2)
		self.dlnp2.node().setScene(render)
		self.dlnp2.setHpr(-70,-60,0)
		render.setLight(self.dlnp2)



	# Records the state of the arrow keys
	def setKey(self, key, value):
		self.keyMap[key] = value

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
				OnscreenImage(image = 'models/dot1.png', pos = (0,0,0), scale = .01)
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
		self.remoteMap[id][4].destroy()
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

	def move(self, task):
		""" Update the position of this avatar.

		This method is called by the task scheduler on every frame.
		It responds to the arrow keys to move the avatar
		and to the camera up/down keys.
		"""

		# If the camera-up key is held down, look up
		# If the camera-down key is pressed, look down
		if (self.keyMap["cam-up"]!=0): camAngle = 10
		elif (self.keyMap["cam-down"]!=0): camAngle = -10
		else : camAngle = 0

		# save avatar's initial position so that we can restore it,
		# in case he falls off the map or runs into something.
		startpos = self.avatar.getPos()

		# If a move-key is pressed, move avatar in the specified 
		# direction.
		if (self.keyMap["left"]!=0):
			self.avatar.setH(self.avatar.getH() + \
					 50 * globalClock.getDt())
		if (self.keyMap["right"]!=0):
			self.avatar.setH(self.avatar.getH() - \
					 50 * globalClock.getDt())
		if (self.keyMap["forward"]!=0):
			self.avatar.setY(self.avatar, \
					 -3000 * globalClock.getDt())
 		if (self.keyMap["backward"]!=0):
 			self.avatar.setY(self.avatar, \
 					 +3000 * globalClock.getDt())
 		if (self.keyMap["strafe-left"]!=0):
 			self.avatar.setX(self.avatar, \
 					 +3000 * globalClock.getDt())
 		if (self.keyMap["strafe-right"]!=0):
 			self.avatar.setX(self.avatar, \
 					 -3000 * globalClock.getDt())

		# If avatar is moving, loop the run animation.
		# If he is standing still, stop the animation.
		if (self.keyMap["forward"]!=0) or (self.keyMap["left"]!=0) or \
		   (self.keyMap["right"]!=0):
			if self.isMoving is False:
				self.avatar.loop("run")
				self.isMoving = True
		else:
			if self.isMoving:
				self.avatar.stop()
				self.avatar.pose("run",5)
				self.isMoving = False

		# position the camera where the avatar is
		pos = self.avatar.getPos(); pos[2] += 1
		hpr = self.avatar.getHpr();
		hpr[0] += 180; hpr[1] = camAngle
		base.camera.setPos(pos); base.camera.setHpr(hpr)
		
		# Now check for collisions.
		self.cTrav.traverse(render)

		# Adjust avatar's Z coordinate.  If avatar's ray hit terrain,
		# update his Z. If it hit anything else, or didn't hit 
		# anything, put him back where he was last frame.
		entries = []
		for i in range(self.avatarGroundHandler.getNumEntries()):
			entries.append(self.avatarGroundHandler.getEntry(i))
		entries.sort(lambda x,y: cmp(y.getSurfacePoint(render).getZ(), \
				 x.getSurfacePoint(render).getZ()))
		if (len(entries)>0) and \
		   (entries[0].getIntoNode().getName() == "ID257") : # the tag of terrain
			self.avatar.setZ( \
				entries[0].getSurfacePoint(render).getZ())
		else:
			self.avatar.setPos(startpos)

		# Check visibility
		for id in self.remoteMap:
			if canSee(self, self.avatar.getPos(),
					self.remoteMap[id][0].getPos()) == True:
				if id not in self.visList:
					self.visList.append(id)
			elif id in self.visList:
				self.visList.remove(id)

		# Finally, update the text that shows avatar's position on 
		# the screen
		self.avPos.setText("Panda's pos: (%d, %d, %d)" % \
			(self.avatar.getX(), self.avatar.getY(),
			 (self.avatar.getHpr()[0])%360))

		# map the avatar's position to the 2D map on the top-right 
		# corner of the screen
		self.dot.setPos((self.avatar.getX()/120.0)*0.7+0.45, \
				0,(self.avatar.getY()/120.0)*0.7+0.25)
		for id in self.remoteMap:
			self.remoteMap[id][2].setPos((self.remoteMap[id][0].getX()/120.0)*0.7+0.45, \
				0,(self.remoteMap[id][0].getY()/120.0)*0.7+0.25)

		s = ""
		for id in self.visList : s += fadr2string(id) + " "
 		self.showNumVisible.setText("Visible pandas: %s" % s)

		return task.cont

#w = PandaWorld()
#w.addRemote(69, 67, 135, 111)
#w.addRemote(20, 33, 135, 222)
#w.addRemote(40, 60, 135, 333)
#w.addRemote(90, 79, 135, 444)
#w.addRemote(30, 79, 135, 555)
#w.addRemote(20, 39, 135, 666)
#run()
