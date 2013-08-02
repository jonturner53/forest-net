""" userWorld - a tour on the Danforth campus, WashU

This module is intended to be called by wuTour.py;
To run it independently, uncomment the end of this code
and type "python userWorld.py"
 
Author: Chao Wang, Jon Turner, Vigo Wei
World Model: Chao Wang, Vigo Wei
 
Adapted from "Roaming Ralph", a tutorial included in Panda3D package.
"""  



""" Set DEBUG = True to visualize the collision system
	and switch to third-person view
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

#FENGP import pyaudio
#FENGP import wave

map_x = 1.3
map_y = 1


x_offset = float(574.5)
y_offset = float(740.0)

def addTitle(text):
	""" Put title on the screen
	"""
	return OnscreenText(text=text, style=1, fg=(1,1,1,1), \
		pos=(1.2,-0.95), align=TextNode.ARight, scale = .07)

def printText(pos):
	""" Print text on the screen
	"""
	return OnscreenText( \
		text=" ", \
		style=2, fg=(1,0.8,0.7,1), pos=(-1.3, pos), \
		align=TextNode.ALeft, scale = .06, mayChange = True)



class userWorld(DirectObject):

	def __init__(self, myAvaNum):
		base.windowType = 'onscreen' 
		wp = WindowProperties.getDefault() 
		base.openDefaultWindow(props = wp)
 		#Change resolution 
		# loadPrcFileData("", "win-size 1300 1000")
		props = WindowProperties()
		props.setSize(800, 616)
		base.win.requestProperties(props)

		self.keyMap = {"left":0, "right":0, "forward":0, "backward":0, \
 				"cam-up":0, "cam-down":0, "cam-left":0, "cam-right":0, \
				"zoom-in":0, "zoom-out":0, "reset-view":1, "dash":0}
		self.info = 0
		self.isMute = 0
		# 3 means 3-d
		self.viewmode = 3

		self.msgQ  = deque()
		self.name = "you"

		#default map mode set to 2d. 
		# ctrl + s to switch between satellite and 2d
		self.mapView = "2d"
		self.DmapA = OnscreenImage(image = 'models/2dA.png', \
			pos = (0,0,0), scale = (0, 0, 0))
		# self.DmapAs = OnscreenImage(image = 'models/2dA_s.png', \
		#	pos = (0,0,0), scale = (0, 0, 0))
		self.terrainXa = -12046
		self.terrainYa = 100
		self.DmapB = OnscreenImage(image = 'models/2dB.png', \
			pos = (0,0,0), scale = (0, 0, 0))
		# self.DmapBs = OnscreenImage(image = 'models/2dB_s.png', \
		#	pos = (0,0,0), scale = (0, 0, 0))
		self.terrainXb = 11200
		self.terrainYb = 10000

		# initialize map view parameters
		# 1 is mapA (Simon to Olin) 
		# 2 is mapB (Olin to Skinker)
		# avatar is spawned in front of Brookings 

		self.currRegion = 2
		self.currViewRegion = 2

		self.fieldAngle = 40
#		self.card = OnscreenImage(image = 'photo_cache/'+ 'unmute.jpg', pos = (0.67, 0, -0.94), scale=.04)
#		self.card.setTransparency(TransparencyAttrib.MAlpha)
		base.win.setClearColor(Vec4(0,0,0,1))

		self.myAvaNum = myAvaNum

		self.title = addTitle("WashU Campus Tour")
		
		# Setup the environment

		self.environ = loader.loadModel("models/campus/danforth")
		self.environ.reparentTo(render)
		self.environ.setPos(0,0,0)
#		size = max - min 
#		print size


 		base.setBackgroundColor(0.4,0.46,0.8,1)
		self.ambientLight = render.attachNewNode( AmbientLight( "ambientLight" ) )
		self.ambientLight.node().setColor( Vec4( 4, 4, 7, 1 ) )
		render.setLight(self.ambientLight)
		
		# Create the panda, our avatar (mainly for collision detection)
		self.avatarNP = NodePath("ourAvatarNP")
		self.avatarNP.reparentTo(render)
		s = str(self.myAvaNum)
		self.avatar = Actor("models/ava" + s,{"walk":"models/walk" + s})
		self.avatar.reparentTo(self.avatarNP)
  		if self.myAvaNum == 1 :
  			self.avatar.setScale(.002)
  			self.avatar.setPos(0,0,0)
  		elif self.myAvaNum == 2 :
  			self.avatar.setScale(.2)
  			self.avatar.setPos(0,0,0)
  		elif self.myAvaNum == 3 :
#  			self.avatar.setScale(.4)
  			self.avatar.setPos(0,0,0)
  		elif self.myAvaNum == 4 :
#  			self.avatar.setScale(.4)
  			self.avatar.setPos(0,0,0)
#  		elif self.myAvaNum == 5 :
#  			self.avatar.setScale(.4)
#  			self.avatar.setPos(0,0,0)
		self.avatarNP.setPos(540,160,15)
		self.avatarNP.setH(230)
		self.dot = OnscreenImage(image = 'models/dot.png', \
			pos = (1,0,1), scale = 0)

		#chat window 
		self.lines = 0

		# Show avatar's position on the screen
		self.avPos = printText(0.9)

		# Set the upper bound of # of remote avatars
		self.maxRemotes = 100

 		# Show a list of visible NPCs
 		self.showNumVisible = printText(0.8)
 		self.visList = []

		# Create cache of Actor objects for serial numbers 1..1000
		self.actorCache = []
		for i in range(1,1001): self.actorCache.append([])

		# Setup pool of remotes
		# do not attach to scene yet
		self.remoteMap = {}
		self.remoteNames = {}
		self.remotePics = {}
		self.msgs = {}

		"""
		self.entry = DirectEntry(width = 15,
					initialText = "enter text here...",
                    numLines = 1,
                    scale = 0.07,
                    cursorKeys = 1,
                    frameSize = (0, 15, 0, 1),
                    pos = (-1, 0, -0.9),                 
                    command = self.newMsg,
                    focusInCommand = self.clearText)	
		"""

		# Setup the keyboard inputs
		self.accept("escape", sys.exit)
		self.accept("arrow_left", self.setKey, ["left",1])
		self.accept("arrow_right", self.setKey, ["right",1])
		self.accept("arrow_up", self.setKey, ["forward",1])
 		self.accept("arrow_down", self.setKey, ["backward",1])
		self.accept("shift-arrow_up", self.setKey, ["cam-up",1])
		self.accept("shift-arrow_down", self.setKey, ["cam-down",1])
		self.accept("shift-arrow_left", self.setKey, ["cam-left",1])
		self.accept("shift-arrow_right", self.setKey, ["cam-right",1])	
		self.accept("alt-arrow_up", self.setKey, ["dash", 1])
		self.accept("z", self.setKey, ["zoom-in",1])
 		self.accept("control-z", self.setKey, ["zoom-out",1])
		self.accept("r", self.setKey, ["reset-view",1])         
		self.accept("arrow_left-up", self.setKey, ["left",0])
		self.accept("arrow_right-up", self.setKey, ["right",0])
		self.accept("arrow_up-up", self.setKey, ["forward",0])
 		self.accept("arrow_down-up", self.setKey, ["backward",0])
 		self.accept("alt-up", self.setKey, ["dash", 0])
		self.accept("shift-up", self.resetCamKeys)
		# or "arrow_up-up", self.setKey, ["cam-up",0])
		# self.accept("shift-up" or "arrow_down-up", self.setKey, ["cam-down",0])
		# self.accept("shift-up" or "arrow_left-up", self.setKey, ["cam-left",0])
		# self.accept("shift-up" or "arrow_right-up", self.setKey, ["cam-right",0])
		self.accept("z-up", self.setKey, ["zoom-in",0])
 		self.accept("control-up", self.setKey, ["zoom-out",0])
		self.accept("t", self.setKey, ["reset-view",0])
#		self.accept("mouse1", self.calling)
#		self.accept("mouse2", self.showPic)
#		self.accept("mouse3", self.teleport)
		self.accept("f1", self.showInfo)
		self.accept("m", self.mute)
#		self.accept("v", self.view)
#		self.accept("s", self.switchRegion)
#		self.accept("control-s", self.switchView)

		taskMgr.add(self.move,"moveTask")



		# Game state variables
		self.isMoving = False
		self.isListening = False

#FENGP		p = pyaudio.PyAudio()
#FENGP		print p.get_device_info_by_index(0)['defaultSampleRate']

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
		self.floorCollider.node().setIntoCollideMask(FLOOR_MASK)
		self.buildingCollider = self.environ.find("**/buildingCol")
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
		self.avatarWallHandler = CollisionHandlerPusher()
		self.avatarWallHandler.setHorizontal(True)
		self.avatarWallHandler.addCollider(self.avatarSpNp, self.avatarNP)
		base.cTrav.addCollider(self.avatarSpNp,	self.avatarWallHandler)

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


	def displayMsg(self, msg, name):
		if self.lines >= 6:
			self.lines = 0
			for id in self.msgs.keys():
				self.msgs[id].destroy()

		self.msgs[self.lines] = OnscreenText(text= name + ": " + msg, pos = (-1, -self.lines*.1))
		self.lines += 1


	def newMsg(self, msg):
		if self.lines >= 5:
			self.lines = 0
			for id in self.msgs.keys():
				self.msgs[id].destroy()
		if len(self.msgQ) < 500:
			self.msgQ.append(msg)
			self.msgs[self.lines] = OnscreenText(text= "you: " + msg, pos = (-1, -self.lines*.1))
			self.lines += 1
		else:
			print "too many messages in the queue."


#	def clearText(self):
#		self.entry.enterText('')

	def getMsg(self):
		print "being called..."
		if len(self.msgQ) > 0:
			return self.msgQ.popleft()
		else:
			return "placeHolder"

	def switchView(self):
		if self.mapView == "2d":
			self.mapView = "sat"
			self.DmapA.setImage('models/2dA_s.png')
			self.DmapB.setImage('models/2dB_s.png')
		else:
			self.mapView = "2d"
			self.DmapA.setImage('models/2dA.png')
			self.DmapB.setImage('models/2dB.png')

		#reset image to show...	
		self.dot.setImage('models/dot.png')
		for id in self.remoteMap:
			self.remoteMap[id][2].setImage('models/dot1.png')

	"""
	def view(self):
		if self.viewmode != 3:
			self.viewmode = 3
			self.currViewRegion = 0
			self.DmapA.setScale(0,0,0)
			self.DmapB.setScale(0,0,0)
			self.dot.setScale(0)
			for id in self.remoteMap:
				self.remoteMap[id][2].setScale(0, 0, 0)

		else:
			self.viewmode = 2
			if self.avatar.getX() >= -13000:
				#we are in region 2
				self.currRegion = 2
				self.currViewRegion = 2
				self.DmapB.setScale(map_x, map_y, 1)
				self.DmapA.setScale(0)
				x = (self.avatar.getX() + x_offset)/self.terrainXb
				y = (self.avatar.getY())/self.terrainYb
				# print self.avatar.getY(),  y_offset, self.terrainYb, y
				self.dot.setImage('models/dot.png')
				self.dot.setScale(0.01)
				self.dot.setPos(x * map_x, 0, y)
				# for id in self.remoteMap:
				# 	#map view is on and we are looking at Region 2
				# 	if self.remoteMap[id][0].getX() > -13000:
				# 		self.remoteMap[id][2].setPos(((self.remoteMap[id][0].getX()) /self.terrainXb), 0, (self.remoteMap[id][0].getY() - y_offset)/self.terrainYb)
				# 		self.remoteMap[id][2].setScale(0.01)

			else:
				# we are in Region 1 (west of Olin)
				self.currRegion = 1
				self.currViewRegion = 1
				self.DmapB.setScale(0)
				self.DmapA.setScale(map_x, map_y, 1)
				x = -(self.avatar.getX() + 26000)/self.terrainXa
				y = self.avatar.getY()/self.terrainYa
				self.dot.setImage('models/dot.png')
				self.dot.setScale(0.01)
				self.dot.setPos(x*map_x, 0, y)
				# for id in self.remoteMap.keys():
				# 	if self.remoteMap[id][0].getX < -13000:
				# 		self.remoteMap[id][2].setPos( -(self.remoteMap[id][0].getX() + 26000 - x_offset) /self.terrainXa, 0, (self.remoteMap[id][0].getY() - y_offset)/self.terrainYa)
				# 		self.remoteMap[id][2].setScale(0.01)

			# old view
			# props = WindowProperties()
			# props.setSize(1300, 536)
			# base.win.requestProperties(props)
			# self.Dmap.setScale(2.6, 1, 1)
			# self.Dmap.setTransparency(TransparencyAttrib.MAlpha)
			# self.dot.setScale(0.01)
			# x = (self.avatar.getX() + 18000)/self.terrainX
			# y = (self.avatar.getY() - 1000)/self.terrainY
			# self.dot.setPos(x, y, 0)
			# print self.viewmode
	"""

	def calling(self):
		"""
		 Mouse1 even signals calling, we'll find the user pointed at,
		 then record the request
		"""
		if base.mouseWatcherNode.hasMouse():
			x = base.mouseWatcherNode.getMouseX()
			y = base.mouseWatcherNode.getMouseY()
		for id in self.remoteMap:
			if 1==1:
				x2d = self.remoteMap[id][2].getPos()[0]/map_x
				y2d = self.remoteMap[id][2].getPos()[2]
				if abs(x2d - x) < 0.02 and abs(y2d - y) < 0.02:
					print "calling ", id
					print x, x2d, y, y2d
					self.callRequest = id
					# print "dot@: ", self.remoteMap[id][2].getPos()[0], self.remoteMap[id][2].getPos()[2]
					# print "has mouse @: " + str(x) + " " + str(y)
			#TODO: ADD NETWORK SUPPORT		

	def getCallRequest():
		if self.callRequest != 0:
			return self.callRequest
		else:
			return 0		
	"""				
	def switchRegion(self):
		if self.currViewRegion == 2:
			self.currViewRegion = 1
			self.DmapB.setScale(0)
			self.DmapA.setScale(map_x, map_y, 1)
			for id in self.remoteMap:
				if self.remoteMap[id][0].getX > -12000:
					self.remoteMap[id][2].setScale(0)
				else:
					self.remoteMap[id][2].setScale(0.01)
		else:
			self.currViewRegion = 2
			self.DmapA.setScale(0)
			self.DmapB.setScale(map_x, map_y, 1)
			for id in self.remoteMap:
				if self.remoteMap[id][0].getX < -12000:
					self.remoteMap[id][2].setScale(0)
				else:
					self.remoteMap[id][2].setScale(0.01)		

		if self.currRegion != self.currViewRegion:
			self.dot.setScale(0)
	"""

	def resetCamKeys(self):
		self.keyMap["cam-up"]=0
		self.keyMap["cam-down"]=0
		self.keyMap["cam-left"]=0
		self.keyMap["cam-right"]=0
		
	#to display/hide pictures when the user clicks on the avatar/pic
	"""
	def showPic(self):
		x = y = 0
		if base.mouseWatcherNode.hasMouse():
			x=base.mouseWatcherNode.getMouseX()
			y=base.mouseWatcherNode.getMouseY()
			print "has mouse @: " + str(x) + " " + str(y)
		responded = 0	
		for id in self.remoteNames.keys():
			if abs(self.remoteNames[id].getPos()[0]- x) < 0.1 and \
				abs(self.remoteNames[id].getPos()[1] - y) < 0.1:
				# click occurs close enough to the name of the \
				# avatar, try to create pic 
				if id not in self.remotePics.keys() or \
					(id in self.remotePics.keys() and \
					 self.remotePics[id] is None):
					name = self.remoteNames[id].getText()
					card = OnscreenImage(image = '/'+ name +'.jpg', \
							pos = (x, 0, y), scale = (0.2, 1, 0.2))
					self.remotePics[id] = card
					responded = 1
		if responded is not 1:
			for id in self.remotePics.keys():
				if self.remotePics[id]:
					card = self.remotePics[id]
					if abs(self.remoteNames[id].getX() - x) < 1 and \
						abs(self.remoteNames[id].getY() < y) < 1:
					# the user clicked on the card
						if card:
						#if card is not None:
							card.destroy()
							self.remotePics[id] = None
	"""
	"""
	def teleport(self):
		print "teleporting...."
		x = y = 0
		if base.mouseWatcherNode.hasMouse():
			x = base.mouseWatcherNode.getMouseX()
			y = base.mouseWatcherNode.getMouseY()
		print x, y, self.currViewRegion, self.currRegion
		if self.currRegion == self.currViewRegion:
			if self.currRegion == 2:
				print "teleporting from Region 2 to 2"
				realX = x * self.terrainXb
				realY = y * self.terrainYb
			else:
				print "teleporting from Region 1 to 1"
				if x > 0:
					realX = x * self.terrainXa
				else:
					realX = -(x - 2) * self.terrainXa

				realY = y * self.terrainYa
		else:
			if self.currRegion == 2:
				print "teleporting from Region 2 to 1"
				if x > 0:
					realX = x * self.terrainXa
				else:
					realX = -(x - 2) * self.terrainXa
				realY = y * self.terrainYa
			else:
				print "teleporting from Region 1 to 2"
				realX = x * self.terrainXb
				realY = y * self.terrainYb			

		print realX, realY
		realX -= x_offset
		#realY += y_offset
		self.avatar.setPos(realX, realY, 550)
		self.DmapA.setScale(0)
		self.DmapB.setScale(0)

		#need some sort of loop for all visible dots
		self.dot.setScale(0)
		for id in self.remoteMap:
			self.remoteMap[id][2].setScale(0, 0, 0)
		# props = WindowProperties()
		# props.setSize(1300, 900)
		# base.win.requestProperties(props)
		self.viewmode = 3

		#cam moved over automatically?


		#convert 2d to 3d
		print "has mouse @: " + str(x) + " " + str(y)
	"""


	def showInfo(self):
		# Show a list of instructions
		if self.info is 0:
			self.ctrlCmd = printText(0.7)
			self.ctrlCmd.setText("Z: zoom-in" + '\n' + "Ctrl+Z: zoom-out" \
								+ '\n' + "*R: reset view to Panda"\
								+ '\n' + "*T: toggle Zoom mode" \
								+ '\n' + "*M: mute/un-mute audio"\
								+ '\n' + "Arrow Keys: move avatar"\
								+ '\n' + "Shift+Arrows: look around"\
								+ '\n' + "*V: map/3D view *S: switch regions"\
								+ '\n' + "Ctrl + S: switch 2D/satellite")
			self.info = 1
		else:
			self.ctrlCmd.setText("")
			self.info = 0
			
	def mute(self):
		#mute / un-mute
		if self.isMute is 1:
			self.isMute = 0
			self.card.destroy()
			self.card = OnscreenImage(image = 'photo_cache/'+ 'unmute.jpg', pos = (0.67, 0, -0.94), scale=.04)
			self.card.setTransparency(TransparencyAttrib.MAlpha)
		else:
			self.isMute = 1
			self.card.destroy()
			self.card = OnscreenImage(image = 'photo_cache/'+ 'mute.jpg', pos = (0.67, 0, -0.93), scale=.04)
			self.card.setTransparency(TransparencyAttrib.MAlpha)		

	def setKey(self, key, value):
		""" Records the state of the arrow keys
		"""
		self.keyMap[key] = value

  	def getActor(self, avaNum) :
  		""" Get an Actor object using a specfied model.
  
  		We first check the actorCache, and if no model is currently
  		available, we create one.
  		avaNum is the avatar serial number for the desired Actor
  		"""
  		actor = None
  		if len(self.actorCache[avaNum]) > 0 :
  			actor = self.actorCache[avaNum].pop()
  		else :
  			s = str(avaNum)
  			actor = Actor("models/ava" + s, \
  				      {"walk":"models/walk" + s})
  			if avaNum == 1 :
  				actor.setScale(.002)
  			elif avaNum == 2 :
  				actor.setScale(.2)
  		#	elif avaNum == 3 :
  		#	elif avaNum == 4 :
  		return actor
  
  	def recycleActor(self, actor, avaNum) :
  		""" Recycle an Actor object.
  
  		Just put it back in the cache.
  		actor is an Actor object that is not currently needed
  		avaNum is the serial number for the actor's model
  		"""
  		self.actorCache[avaNum].append(actor)

	def addRemote(self, x, y, z, direction, id, name, avaNum) : 
		""" Add a remote panda.

		This method is used by the Net object when it starts
		getting packets from a panda that it did not previously
		know about.
		x,y gives the remote's position
		direction gives its heading
		id is an identifier used to distinguish this remote from others
		avaNum is the serial number for this avatar's model

		New remotes are included in the scene graph.
		"""
		if id in self.remoteMap:
			self.updateRemote(x,y,z,direction,id, name); return
		remote = self.getActor(avaNum)
		self.remoteMap[id] = [remote, avaNum, False,
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

		# display name: convert 3D coordinates to 2D
		# point(x, y, 0) is the position of the Avatar in the 3D world
		# the 2D screen position is given by a2d
		
		p3 = base.cam.getRelativePoint(render, Point3(x,y,0))
		p2 = Point2()
		if base.camLens.project(p3, p2):
		    r2d = Point3(p2[0], 0, p2[1])
		    a2d = aspect2d.getRelativePoint(render2d, r2d) 
		    if id not in self.remoteNames.keys():
				self.remoteNames[id] = OnscreenText( \
					text = name, pos = a2d, scale = 0.04, \
					fg = (1, 1, 1 ,1), shadow = (0, 0, 0, 0.5) )
		
		# if off view
		else:
			if id in self.remoteNames.keys():
				if self.remoteNames[id]:
					self.remoteNames[id].destroy()	

		####remote.loop("run")

	def updateRemote(self, x, y, z, direction, id, name) :
		""" Update location of a remote panda.

		This method is used by the Net object to update the location.
		x,y gives the remote's position
		direction gives its heading
		id is an identifier used to distinguish this remote from others
		"""
		if id not in self.remoteMap : return
		remote, avaNum, isMoving, dot = self.remoteMap[id]


		""" jst start """
		if abs(x-remote.getX()) > .01 or abs(y-remote.getY()) > .01 or \
		   abs(direction-remote.getHpr()[0]) > .01 :
			if not isMoving :
				remote.loop("walk")
				self.remoteMap[id][2] = True
			remote.setHpr(direction,0,0) 
			remote.setPos(x,y,z)
		else :
			remote.stop(); remote.pose("walk",1)
			self.remoteMap[id][2] = False 
		""" jst end """


		# display name, similar to in addRemote()
		# (x, y, 0) is the 3D position of the target
		# the screen position is given by a2d
				
		p3 = base.cam.getRelativePoint(render, Point3(x,y,0))
		p2 = Point2()
		if base.camLens.project(p3, p2) and id in self.visList:
			r2d = Point3(p2[0], 0, p2[1])
			a2d = aspect2d.getRelativePoint(render2d, r2d) 
			if id in self.remoteNames.keys():
				if self.remoteNames[id]:
					self.remoteNames[id].destroy()
			self.remoteNames[id] = OnscreenText(text = name, \
						pos = a2d, scale = 0.04, \
						fg = (1, 1, 1, 1), shadow = (0, 0, 0, 0.5))

	#		if id in self.remotePics.keys():
	#			if self.remotePics[id]:
					#try:
					#self.remotePics[id].setPos(a2d[0], 0, a2d[1]) 
					## if pic happens to have been removed by the user
					# except Exception as e:
						# pass
					
		#off screen, delete text obj. and the picture
		else:
			if id in self.remoteNames.keys():
				if self.remoteNames[id]:
					self.remoteNames[id].destroy()	
			if id in self.remotePics.keys():
				if self.remotePics[id]:
					self.remotePics[id].destroy()
					self.remotePics[id] = None

	def playAudio(self,inputData) :
    	
		print len(inputData)
		self.stream.write(inputData)

	def removeRemote(self, id) :
		""" Remove a remote when it goes out of range.
		
		id is the identifier for the remote
		"""
		if id not in self.remoteMap : return
		self.remoteMap[id][3].destroy()
		remote = self.remoteMap[id][0]

		if id in self.remoteNames.keys():
			if self.remoteNames[id]:
				self.remoteNames[id].destroy()	
				
		if id in self.remotePics.keys():
			if self.remotePics[id]:
				self.remotePics[id].destroy()
				self.remotePics[id] = None	

		remote.detachNode()
		avaNum = self.remoteMap[id][1]
		self.recycleActor(remote, avaNum)
		del self.remoteMap[id]
		if id in self.visList:
			self.visList.remove(id)

	def getPosHeading(self) :
		""" Get the position and heading.
	
		Intended for use by a Net object that must track avatar's 
		position return a tuple containing the x-coordinate,
		y-coordinate, z-coordinate, and heading.
		"""
		return (self.avatarNP.getX(), self.avatarNP.getY(), \
			self.avatarNP.getZ(), \
			(self.avatarNP.getHpr()[0])%360)

	def getLimit(self) :
		""" Get the limit on the xy coordinates.
		"""
		return 1200

  	def is_Mute(self) :
		""" Get isMute to see if Avatar is mute
		"""
		return self.isMute		

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
		newY = 0
		newX = 0

		if (self.keyMap["left"]!=0):
			newH += 50 * globalClock.getDt()
		if (self.keyMap["right"]!=0):
			newH -= 50 * globalClock.getDt()
		if (self.keyMap["forward"]!=0):
			newY -= 1.3 * globalClock.getDt()
 		if (self.keyMap["backward"]!=0):
 			newY += 1.3 * globalClock.getDt()
 		if (self.keyMap["dash"]!=0):
 			newY -= 60 * globalClock.getDt()

		self.avatarNP.setH(newH)
		self.avatarNP.setFluidY(self.avatarNP, newY)


		""" Updating camera
		"""
		if (self.keyMap["cam-up"]!=0): camAngle = 10
		elif (self.keyMap["cam-down"]!=0): camAngle = -10
		else : camAngle = 0 
		if (self.keyMap["cam-left"]!=0): camAngleX = 10
		elif (self.keyMap["cam-right"]!=0): camAngleX = -10
		else: camAngleX = 0

		hpr = self.avatarNP.getHpr()
		pos = self.avatarNP.getPos()
		hpr[0] = hpr[0] + 180 + camAngleX
		hpr[1] = camAngle

		# if the zoom-in key 'i' is held down, increase fov of the lens
		# fov: field of view, 20--80 is acceptable range, others distorted
		# if the key 'o' is held down, zoom out	
		if (self.keyMap["zoom-in"] != 0): 			
			if self.fieldAngle > 20:
				self.fieldAngle = self.fieldAngle - 5
			base.camLens.setFov(self.fieldAngle)
		elif (self.keyMap["zoom-out"] != 0):	
			if self.fieldAngle < 80:
				self.fieldAngle = self.fieldAngle + 5
			base.camLens.setFov(self.fieldAngle)		
		elif (self.keyMap["reset-view"] != 0):
			if DEBUG:
				base.camera.setPos(self.avatarNP,0,15,2)
			else:
				base.camera.setPos(self.avatarNP,0,0,1.8)
			base.camera.setHpr(hpr)
			base.camLens.setFov(40)

		# If avatar is moving, loop the walk animation.
		# If he is standing still, stop the animation.
		""" irrelvant if we use the first-person viewpoint
		if (self.keyMap["forward"]!=0) or (self.keyMap["left"]!=0) or \
		   (self.keyMap["right"]!=0):
			if self.isMoving is False:
				self.avatar.loop("walk")
				self.isMoving = True
		else:
			if self.isMoving:
				self.avatar.stop()
				self.avatar.pose("walk",5)
				self.isMoving = False
		"""
		

		# Check visibility and update visList
		for id in self.remoteMap:
			if self.canSee(self.avatarNP.getPos(),
					self.remoteMap[id][0].getPos()) == True:
				if id not in self.visList:
					self.visList.append(id)
			elif id in self.visList:
				self.visList.remove(id)
		s = ""
		for id in self.visList : s += fadr2string(id) + " "
 		self.showNumVisible.setText("Visible people: %s" % s)

		# Update the text showing panda's position on 
		# the screen
		self.avPos.setText("Your location (x, y, z): (%d, %d, %d)"%\
			(self.avatarNP.getX(), self.avatarNP.getY(),
			 self.avatarNP.getZ()))

		"""
		# map the avatar's position to the 2D map on the top-right
		# corner of the screen
		# self.dot.setPos((self.avatar.getX()/120.0)*0.7+0.45, \
		# 		0,(self.avatar.getY()/120.0)*0.7+0.25)
		if self.viewmode == 2:
			for id in self.remoteMap:
			#map view is on and we are looking at Region 2
				if self.currViewRegion == 2 and self.remoteMap[id][0].getX() > -12000:
					self.remoteMap[id][2].setScale(0.01)
					self.remoteMap[id][2].setPos(((self.remoteMap[id][0].getX() + x_offset) /self.terrainXb) * map_x, 0, (self.remoteMap[id][0].getY() - y_offset)/self.terrainYb)
					# print "panda @: ",  self.remoteMap[id][0].getX(), self.remoteMap[id][0].getY()
					# print "calculated 2d pos: ",  (self.remoteMap[id][0].getX() + x_offset) /self.terrainXb, (self.remoteMap[id][0].getY() - y_offset)/self.terrainYb
					# print "real 2d pos: (", self.remoteMap[id][2].getPos()[0], self.remoteMap[id][2].getPos()[1], self.remoteMap[id][2].getPos()[2], ")"
					
				elif self.currViewRegion == 1 and self.remoteMap[id][0].getX() < -12000:
					self.remoteMap[id][2].setScale(0.01)
					self.remoteMap[id][2].setPos( -(self.remoteMap[id][0].getX() + 26000 - x_offset) * map_x /self.terrainXa, 0, (self.remoteMap[id][0].getY() - y_offset)/self.terrainYa)
		"""										
		return task.cont


"""
w = userWorld()
w.addRemote(0, 0, 135, 222)
w.addRemote(-217, -48, 5, 444)
w.addRemote(-266, -26, 135, 555)
w.addRemote(-353, 98, 135, 666)
w.addRemote(-202, -2, 135, 777)
run()
"""
