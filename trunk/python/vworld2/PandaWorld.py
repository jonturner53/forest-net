""" PandaWorld - simple virtual environment with wandering pandas.

This module is intended to be called by Avatar.py;
To run it without network, uncomment the last six lines of this code:
	# w = PandaWorld()
	# (spawning NPCs...)
 	# run()
and type "python PandaWorld.py"
 
control:
	Move              -> Up, Left, Right, Down
	Look Up/Down      -> A / S
	Strafe Left/Right -> Z / X

Last Updated: 7/12/2013
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
		pos=(1.3,-0.95), align=TextNode.ARight, scale = .07)

def printText(pos):
	""" Print text on the screen
	"""
	return OnscreenText( \
		text=" ", \
		style=2, fg=(1,0.8,0.7,1), pos=(-1.3, pos), \
		align=TextNode.ALeft, scale = .06, mayChange = True)



class PandaWorld(DirectObject):
	def __init__(self):

		base.windowType = 'onscreen' 
		wp = WindowProperties.getDefault() 
		base.openDefaultWindow(props = wp)
 
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
		self.environ = loader.loadModel("models/vworld24grid")
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

		# Set up the camera
		base.disableMouse()
		base.camera.setPos(self.avatar.getPos())
		base.camera.setZ(self.avatar.getZ()+1)
		base.camera.setHpr(self.avatar.getHpr()[0],0,0)
		

		# Set the dot's position
		self.dot.setPos(
				base.camera.getX()/(self.getLimit()+0.0+self.Dmap.getX()), \
				0,
				base.camera.getY()/(self.getLimit()+0.0+self.Dmap.getZ()))
		self.dotOrigin = self.dot.getPos()

		# Show avatar's position on the screen
		self.avPos = printText(0.9)

		# Set the upper bound of # of remote avatars
		self.maxRemotes = 100

 		# Show a list of visible NPCs
 		self.showNumVisible = printText(0.8)
 		self.visList = []

		# Setup pool of remotes
		# do not attach to scene yet
		self.remoteMap = {}
		self.freeRemotes = []
		for i in range(0,self.maxRemotes) : # allow up to 100 remotes
			self.freeRemotes.append(Actor("models/panda-model", \
						{"run":"models/panda-walk4"}))
			self.freeRemotes[i].setScale(0.002)

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
						#	-1.0/self.avatar.getScale().getY(),
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


		"""
		Finally, let there be some light
		"""
		self.ambientLight = render.attachNewNode(AmbientLight("ambientLight"))
		self.ambientLight.node().setColor(Vec4(.8,.8,.8,1))
		render.setLight(self.ambientLight)

		
		dLight1 = DirectionalLight("dLight1")
		dLight1.setColor(Vec4(6,5,7,1))
		dLight1.setDirection(Vec3(1,1,1))
		dlnp1 = render.attachNewNode(dLight1)
		dlnp1.setHpr(30,-160,0)
		render.setLight(dlnp1)

		dLight2 = DirectionalLight("dLight2")
		dLight2.setColor(Vec4(.6,.7,1,1))
		dLight2.setDirection(Vec3(-1,-1,-1))
		self.dlnp2 = render.attachNewNode(dLight2)
		self.dlnp2.node().setScene(render)
		self.dlnp2.setHpr(-70,-60,0)
		render.setLight(self.dlnp2)
		


	def setKey(self, key, value):
		""" Records the state of the arrow keys
		"""
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
				OnscreenImage(image = 'models/dot1.png', \
						pos = (0,0,0), scale = .01)
				]
		# format above: [remote obj, isMoving, dot on the 2D map]

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
		else:
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
		if id in self.visList:
			self.visList.remove(id)

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
		It responds to the arrow keys to move the avatar
		and to the camera up/down keys.
		"""
		# If the camera-up key is held down, look up
		# If the camera-down key is pressed, look down
		if (self.keyMap["cam-up"]!=0): camAngle = 10
		elif (self.keyMap["cam-down"]!=0): camAngle = -10
		else: camAngle = 0

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

		# If a move-key is pressed, move avatar in the specified 
		# direction.
		newH = self.avatar.getH()
		newY = 0
		newX = 0
		
		if (self.keyMap["left"]!=0):
			newH += 50 * globalClock.getDt()
		if (self.keyMap["right"]!=0):
			newH -= 50 * globalClock.getDt()
		if (self.keyMap["forward"]!=0):
			newY -= 1000 * globalClock.getDt()
 		if (self.keyMap["backward"]!=0):
 			newY += 1000 * globalClock.getDt()
 		if (self.keyMap["strafe-left"]!=0):
 			newX += 1000 * globalClock.getDt()
 		if (self.keyMap["strafe-right"]!=0):
 			newX -= 1000 * globalClock.getDt()

		self.avatar.setH(newH)
		self.avatar.setFluidPos(self.avatar, newX,newY,self.avatar.getZ())

		# position the camera to where the avatar is
		pos = self.avatar.getPos()
		hpr = self.avatar.getHpr()
		hpr[0] += 180
		hpr[1] = camAngle
# to debug, uncomment the following line, and comment out the line next to it
#		base.camera.setPos(pos.getX(),pos.getY()+15,pos.getZ()+1)
		base.camera.setPos(pos.getX(),pos.getY(),pos.getZ()+1)
		base.camera.setHpr(hpr)
		
		# Check visibility and update visList
		for id in self.remoteMap:
			if self.canSee(self.avatar.getPos(),
					self.remoteMap[id][0].getPos()) == True:
				if id not in self.visList:
					self.visList.append(id)
			elif id in self.visList:
				self.visList.remove(id)
		s = ""
		for id in self.visList : s += fadr2string(id) + " "
 		self.showNumVisible.setText("Visible pandas: %s" % s)

		# Update the text showing panda's position on 
		# the screen
		self.avPos.setText("Panda's position: (x, y) = (%d, %d)" % \
			(self.avatar.getX(), self.avatar.getY()))

		# map the avatar's position to the 2D map on the top-right 
		# corner of the screen
		self.dot.setPos(
				(self.avatar.getX()/(self.getLimit()+0.0))*0.7+0.45, \
				0,
				(self.avatar.getY()/(self.getLimit()+0.0))*0.7+0.25)
		for id in self.remoteMap:
			self.remoteMap[id][2].setPos(
				(self.remoteMap[id][0].getX()/self.getLimit()+0.0)*0.7+0.45, \
				0,
				(self.remoteMap[id][0].getY()/self.getLimit()+0.0)*0.7+0.25)

		return task.cont

# uncomment the following lines to test this code without networking support
"""
w = PandaWorld()
w.addRemote(69, 67, 135, 111)
w.addRemote(20, 33, 135, 222)
w.addRemote(54, 46, 135, 333)
w.addRemote(90, 79, 135, 444)
run()
"""
