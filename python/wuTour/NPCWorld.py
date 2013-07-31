""" NPCWorld - a tour on Danforth Campus, WashU (for bots)

This module shall be called by wuTour.py

Author: Chao Wang, Jon Turner, Vigo Wei
World Model: Chao Wang, Vigo Wei
 
Adapted from "Roaming Ralph", a tutorial included in Panda3D package.
"""  



""" Set DEBUG=True to turn on the display
"""
DEBUG = False

import direct.directbase.DirectStart
from panda3d.core import *
from direct.gui.OnscreenText import OnscreenText
from direct.gui.OnscreenImage import OnscreenImage
from direct.actor.Actor import Actor
from direct.showbase.DirectObject import DirectObject
from direct.interval.IntervalGlobal import Sequence
from panda3d.core import Point3
import random, sys, os, math, re
from panda3d.core import loadPrcFileData
from Util import *
from direct.directbase.DirectStart import *
from pandac.PandaModules import *
from direct.gui.DirectGui import *

from collections import deque

import pyaudio
import wave

map_x = 1.3
map_y = 1


x_offset = float(574.5)
y_offset = float(740.0)

def addTitle(text):
	""" Put title on the screen
	"""
	return OnscreenText(text=text, style=1, fg=(1,1,1,1), \
		pos=(1.2,-0.95), align=TextNode.ARight, scale = .07)


class NPCWorld(DirectObject):

	def __init__(self, myAvaNum):
		if DEBUG:
			base.windowType = 'onscreen' 
			wp = WindowProperties.getDefault() 
			base.openDefaultWindow(props = wp)
 			#Change resolution 
			# loadPrcFileData("", "win-size 1300 1000")
			props = WindowProperties()
			props.setSize(800, 616)
			base.win.requestProperties(props)
			base.win.setClearColor(Vec4(0,0,0,1))
			self.title = addTitle("WashU Campus viewed from AI")


		self.currRegion = 2
		self.currViewRegion = 2

		self.fieldAngle = 40


		self.myAvaNum = myAvaNum
		
		# Setup the environment

		self.environ = loader.loadModel("models/campus/danforthBoxForAI")
		self.environ.reparentTo(render)
		self.environ.setPos(0,0,0)
#		size = max - min 
#		print size


 		base.setBackgroundColor(0.8,0.46,0.4,1)
		self.ambientLight = render.attachNewNode( AmbientLight( "ambientLight" ) )
		self.ambientLight.node().setColor( Vec4( 4, 4, 7, 1 ) )
		render.setLight(self.ambientLight)
		
		# Create the panda, our avatar
		self.avatarNP = NodePath("ourAvatarNP")
		self.avatarNP.reparentTo(render)
		self.avatar = Actor("models/panda-model", \
				    {"run":"models/panda-walk4", \
				    "walk":"models/panda-walk"})
		self.avatar.reparentTo(self.avatarNP)
		self.avatar.setScale(.002)
		self.avatar.setPos(0,0,0)
#		self.avatarNP.setPos(-231,-58,14)
		self.avatarNP.setPos(645,170,19)
		self.avatarNP.setH(230)
		self.dot = OnscreenImage(image = 'models/dot.png', \
			pos = (1,0,1), scale = 0)


		self.rotateDir = -1
		self.rotateDuration = -1
		self.moveDir = -1
		self.moveDuration = -1
		self.isAvoidingCollision = False
		self.inBigRotate = False # if True, do not move forward; only rotate


#		# Show avatar's position on the screen
#		self.avPos = printText(0.9)

		# Set the upper bound of # of remote avatars
		self.maxRemotes = 100

# 		# Show a list of visible NPCs
# 		self.showNumVisible = printText(0.8)
 		self.visList = []

		# Setup pool of remotes
		# do not attach to scene yet
		self.remoteMap = {}
		self.freeRemotes = []
		self.remotePics = {}
		for i in range(0,self.maxRemotes) : # allow up to 100 remotes
			self.freeRemotes.append(Actor("models/panda-model", \
						{"run":"models/panda-walk4"}))
			self.freeRemotes[i].setScale(0.002)
		self.msgs = {}


		# Setup the keyboard inputs
		self.accept("escape", sys.exit)

		taskMgr.add(self.move,"moveTask")



		# Game state variables
		self.isMoving = False
		self.isListening = False

#		p = pyaudio.PyAudio()
#		print p.get_device_info_by_index(0)['defaultSampleRate']

		if DEBUG:
			# Set up the camera
			base.disableMouse()
			base.camera.setPos(self.avatar.getX(),self.avatar.getY(),1)
			base.camera.setHpr(self.avatar.getHpr()[0],0,0)
		
 		"""
 		Setup collision systems for the two purposes:
 		1. prevent the avatar from passing through a wall
			(using collisionSphere + collisionHandlerPusher)
		2. keep the avatar on the ground
			(using collisionRay + collisionHandlerFloor)
 		"""
		# Setup collisionBitMasks
		FLOOR_MASK = BitMask32.bit(1)
		WALL_MASK = BitMask32.bit(2)
		self.floorCollider = self.environ.find("**/terrainCol")
		if DEBUG: self.floorCollider.show()
		self.floorCollider.node().setIntoCollideMask(FLOOR_MASK)
		self.buildingCollider = self.environ.find("**/buildingCol")
		if DEBUG: self.buildingCollider.show()
		self.buildingCollider.node().setIntoCollideMask(WALL_MASK)
		
		base.cTrav = CollisionTraverser()
		base.cTrav.setRespectPrevTransform(True)

		""" The difference between this code and below
			is whether we attach atavarSpNp to avatar or avatarNP.
			Because avatar is scaling down,
			we need to adjust the parameters accordingly
		self.avatarSpNp = \
			self.avatar.attachNewNode(CollisionNode('avatarSphere'))
		self.csSphere = self.avatarSpNp.node().addSolid(
			CollisionSphere(0.0/self.avatar.getScale().getX(),
							0.0/self.avatar.getScale().getY(),
							5.5/self.avatar.getScale().getZ(),
							0.8/self.avatar.getScale().getX()))
		"""
		self.avatarSpNp = \
			self.avatarNP.attachNewNode(CollisionNode('avatarSphere'))
		self.csSphere = self.avatarSpNp.node().addSolid(
			CollisionSphere(0,0,1.3,.8))
		# format above: (x,y,z,radius)
		self.avatarSpNp.node().setFromCollideMask(WALL_MASK)
		self.avatarSpNp.node().setIntoCollideMask(BitMask32.allOff())
		if DEBUG: self.avatarSpNp.show()
		self.avatarGroundHandler = CollisionHandlerPusher()
		self.avatarGroundHandler.setHorizontal(True)
		self.avatarGroundHandler.addCollider(self.avatarSpNp, self.avatarNP)
		base.cTrav.addCollider(self.avatarSpNp,	self.avatarGroundHandler)

		# setup collisionRay for the Z axis adjustment

		self.avatarRayNp = \
			self.avatarNP.attachNewNode(CollisionNode('avatarRay'))
		self.csRay = self.avatarRayNp.node().addSolid(
			CollisionRay(0,0,1,0,0,-1))
		self.avatarRayNp.node().setFromCollideMask(FLOOR_MASK)
		self.avatarRayNp.node().setIntoCollideMask(BitMask32.allOff())
		if DEBUG: self.avatarRayNp.show()
		self.avatarRayHandler = CollisionHandlerFloor()
		self.avatarRayHandler.setMaxVelocity(14)
		self.avatarRayHandler.addCollider(self.avatarRayNp, self.avatarNP)
		base.cTrav.addCollider(self.avatarRayNp, self.avatarRayHandler)
		if DEBUG: base.cTrav.showCollisions(render)


		# setup collisionSegment for the method canSee()
		self.csSeg = CollisionSegment()
		self.csSeg.setPointA(Point3(0,0,0))
		self.csSeg.setPointB(Point3(0,0,.5))
		self.csNode = self.environ.attachNewNode(CollisionNode('csSeg'))
		if DEBUG: self.csNode.show()
		self.csNode.node().addSolid(self.csSeg)
		self.csNode.node().setFromCollideMask(WALL_MASK)
		self.csNode.node().setIntoCollideMask(BitMask32.allOff())
		self.csHandler = CollisionHandlerQueue()
		self.csTrav = CollisionTraverser('CustomTraverser')
		self.csTrav.addCollider(self.csNode, self.csHandler)
		if DEBUG: self.csTrav.showCollisions(render)


	def addRemote(self, x, y, z, direction, id, name, avaNum) : 
		""" Add a remote panda.

		This method is used by the Net object when it starts
		getting packets from a panda that it did not previously
		know about.
		x,y gives the remote's position
		direction gives its heading
		id is an identifier used to distinguish this remote from others

		New remotes are included in the scene graph.
		"""
		if id in self.remoteMap:
			self.updateRemote(x,y,z,direction,id, name); return
		if len(self.freeRemotes) == 0 :
			sys.stderr.write("PandaWorld.addRemote: no unused " + \
					 "remotes\n")
			sys.exit(1)
		remote = self.freeRemotes.pop()
		self.remoteMap[id] = [remote, True ,
				OnscreenImage(image = 'models/dot1.png', \
						pos = (0,0,0), scale = 0)
				]
		"""
				OnscreenImage(image = 'models/dot1.png', \
						pos = (0,0,0), scale = 0),\
						None ### what's this for?
				]
		"""

		# set position and direction of remote and make it visible
		remote.reparentTo(render)
		remote.setPos(x, y, z) ### 
		remote.setHpr(direction, 0, 0) 

		remote.loop("run")

	def updateRemote(self, x, y, z, direction, id, name) :
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
		remote.setPos(x,y,z)
		remote.setHpr(direction,0,0) 


	def removeRemote(self, id) :
		""" Remove a remote when it goes out of range.
		
		id is the identifier for the remote
		"""
		if id not in self.remoteMap : return

		if id in self.remotePics.keys():
			if self.remotePics[id]:
				self.remotePics[id].destroy()
				self.remotePics[id] = None	

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
		"""
		print "PosNHeading"
		print self.avatarNP.getX()
		print self.avatarNP.getY()
		print (self.avatarNP.getHpr()[0])%360
		"""
		return (self.avatarNP.getX(), self.avatarNP.getY(), \
			self.avatarNP.getZ()+0.5, (self.avatarNP.getHpr()[0])%360)
###			self.avatar.getZ())

	def getLimit(self) :
		""" Get the limit on the xy coordinates.
		"""
		return 1200


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
		It responds to keys to move the avatar/camera
		"""

		""" Updating the position of avatar
		"""
		newH = self.avatarNP.getH()

		if self.rotateDir == -1:
			self.rotateDir = random.randint(1,25) # chances to rotate
		if self.rotateDuration == -1:
			self.rotateDuration = random.randint(200,400)

		# guide the moving direction of the bot
		if self.rotateDir <= 4 : # turn left
			self.avatarNP.setH(self.avatarNP.getH() + \
							 8 * globalClock.getDt())
			self.rotateDuration -= 1
			if self.rotateDuration <= 0:
				self.rotateDuration = -1
				self.rotateDir = -1
		elif self.rotateDir <= 7 : # turn right
			self.avatarNP.setH(self.avatarNP.getH() - \
							 8 * globalClock.getDt())
			self.rotateDuration -= 1
			if self.rotateDuration <= 0:
				self.rotateDuration = -1
				self.rotateDir = -1
		elif self.rotateDir == 8 : # turn big left
			self.avatarNP.setH(self.avatarNP.getH() + \
							 20 * globalClock.getDt())
			self.rotateDuration -= 1
			if self.rotateDuration <= 0:
				self.rotateDuration = -1
				self.rotateDir = -1
		elif self.rotateDir == 9 : # turn big right
			self.avatarNP.setH(self.avatarNP.getH() - \
							 20 * globalClock.getDt())
			self.rotateDuration -= 1
			if self.rotateDuration <= 0:
				self.rotateDuration = -1
				self.rotateDir = -1
		else :
			self.rotateDuration -= 1
			if self.rotateDuration <= 0:
				self.rotateDuration = -1
				self.rotateDir = -1
			self.avatarNP.setFluidY(self.avatarNP,
									-0.3 * globalClock.getDt())
		# moving forward
		self.avatarNP.setFluidY(self.avatarNP,
								-1 * globalClock.getDt())


	#	self.avatarNP.setH(newH)
	#	self.avatarNP.setFluidY(self.avatarNP, newY)


		if DEBUG:
			""" Updating camera
			"""
			hpr = self.avatarNP.getHpr()
			pos = self.avatarNP.getPos()
			hpr[0] = hpr[0] + 180
			hpr[1] = 0
			if DEBUG:
				base.camera.setPos(self.avatarNP,0,15,2)
			else:
				base.camera.setPos(self.avatarNP,0,0,2)
			base.camera.setHpr(hpr)
			base.camLens.setFov(40)


		return task.cont

