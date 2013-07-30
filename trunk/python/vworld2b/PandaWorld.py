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

Last Updated: 3/14/2013
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
# from threading import Thread, Lock
# from pandac.PandaModules import CardMaker
# import time

# import pyaudio
# import wave

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
	def __init__(self, myAvaNum):

		base.windowType = 'onscreen' 
		wp = WindowProperties.getDefault() 
		base.openDefaultWindow(props = wp)
 
		self.keyMap = {"left":0, "right":0, "forward":0, "backward":0, \
 				"cam-up":0, "cam-down":0, \
 				"strafe-left":0, "strafe-right":0, \
				"zoom-in":0, "zoom-out":0, \
				"reset-view":1, "voice":0}
		base.win.setClearColor(Vec4(0,0,0,1))

		self.myAvaNum = myAvaNum

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

		self.environ = loader.loadModel("models/vworld24grid")
		self.environ.reparentTo(render)
		self.environ.setPos(0,0,0)
		
 		base.setBackgroundColor(0.2,0.23,0.4,1)
		
		# Create cache of Actor objects for serial numbers 1..1000
		actorCache = []*1001;

		# Create my avatar - mainly for collision detection
		s = str(self.myAvaNum)
		self.avatar = Actor("models/ava" + s,{"walk":"models/walk" + s})
		if self.myAvaNum == 1 :
			self.avatar.setScale(.002)
			self.avatar.setPos(58,67,0)
		elif self.myAvaNum == 2 :
			self.avatar.setScale(.2)
			self.avatar.setPos(58,67,0)
		elif self.myAvaNum == 3 :
			self.avatar.setScale(.2)
			self.avatar.setPos(58,67,.5)
		elif self.myAvaNum == 4 :
			self.avatar.setScale(.2)
			self.avatar.setPos(58,67,.5)
		self.avatar.reparentTo(render)
			
		# Set the dot's position
		self.dot.setPos(self.avatar.getX()/(120.0+self.Dmap.getX()), \
				0, self.avatar.getY()/(120.0+self.Dmap.getZ()))
		self.dotOrigin = self.dot.getPos()

		# Show avatar's position on the screen
		self.avPos = printText(0.9)

		# Set the upper bound of # of remote avatars
		self.maxRemotes = 100

 		# Show a list of visible NPCs
 		self.showNumVisible = printText(0.8)
 		self.visList = []
         
	        # Set audioLevel and data
		self.audioLevel = 0
		self.audioData = ''	 
		
		# Show a list of instructions
		self.zoomCmd = printText(0.7)
		self.zoomCmd.setText("*I: zoom-in" + '\n' + "*O: zoom-out" \
					+ '\n' + "*R: reset view to Panda"\
					+ '\n' + "*T: toggle Zoom mode")

		# setup maps for remote avatars - key is remote's id
		# do not attach to scene yet
		self.remoteMap = {}
		self.remoteNames = {}
		self.remotePics = {}

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
		self.accept("i", self.setKey, ["zoom-in",1])
 		self.accept("o", self.setKey, ["zoom-out",1])
		self.accept("r", self.setKey, ["reset-view",1])
		self.accept("v", self.setKey, ["voice",1])
                
		self.accept("arrow_left-up", self.setKey, ["left",0])
		self.accept("arrow_right-up", self.setKey, ["right",0])
		self.accept("arrow_up-up", self.setKey, ["forward",0])
 		self.accept("arrow_down-up", self.setKey, ["backward",0])
		self.accept("a-up", self.setKey, ["cam-up",0])
		self.accept("s-up", self.setKey, ["cam-down",0])
 		self.accept("z-up", self.setKey, ["strafe-left",0])
 		self.accept("x-up", self.setKey, ["strafe-right",0])
		self.accept("i-up", self.setKey, ["zoom-in",0])
 		self.accept("o-up", self.setKey, ["zoom-out",0])
		self.accept("t", self.setKey, ["reset-view",0])
        	self.accept("v-up", self.setKey, ["voice",0])
		self.accept("mouse1", self.showPic)
        
		taskMgr.add(self.move,"moveTask")
        

		# Game state variables
		self.isMoving = False
        	self.isListening = False

		# Set up the audio
		#p = pyaudio.PyAudio()
		#print p.get_device_info_by_index(0)['defaultSampleRate']

		base.disableMouse()
		pos = self.avatar.getPos(); pos[2] += 1
		hpr = self.avatar.getHpr();
		hpr[0] += 180;
		base.camera.setPos(pos); base.camera.setHpr(hpr)		
			
		self.cTrav = CollisionTraverser()

 		"""
 		CollisionRay that detects walkable region.
 		A move is illegal if the groundRay hits but the terrain.
 		"""
		self.avatarCS = CollisionSphere(0,0,0,1)
		self.avatarGroundCol = CollisionNode('avatarSphere')
		self.avatarGroundCol.addSolid(self.avatarCS)
		self.avatarGroundCol.setFromCollideMask(BitMask32.bit(0))
		self.avatarGroundCol.setIntoCollideMask(BitMask32.allOff())
		self.avatarGroundColNp = \
			self.avatar.attachNewNode(self.avatarGroundCol)

		# model-specific adjustments - fix later
		if self.myAvaNum == 1 :
			self.avatarCS.setRadius(500)
			self.avatarCS.setCenter(0,-80,300)
		elif self.myAvaNum == 2 :
			self.avatarCS.setRadius(3)
			self.avatarCS.setCenter(0,0,2)
		elif self.myAvaNum == 3 or self.myAvaNum == 4 :
			self.avatarCS.setRadius(3)
			self.avatarCS.setCenter(0,0,0)

		self.avatarGroundHandler = CollisionHandlerQueue()
		self.cTrav.addCollider(self.avatarGroundColNp, \
			self.avatarGroundHandler)

		#self.avatarGroundColNp.show()

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

	#to display/hide pictures when the user clicks on the avatar/pic
	def showPic(self):
		x = y = 0
		if base.mouseWatcherNode.hasMouse():
			x=base.mouseWatcherNode.getMouseX()
			y=base.mouseWatcherNode.getMouseY()
			#print "has mouse @: " + str(x) + " " + str(y)
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
					card = OnscreenImage(image =  \
						'models/Display/'+ name + \
						'.jpg', pos = (x, 0, y), \
						scale = (0.2, 1, 0.2))
					self.remotePics[id] = card
					responded = 1
		if responded is not 1:
			for id in self.remotePics.keys():
				if self.remotePics[id]:
					card = self.remotePics[id]
					if abs(self.remoteNames[id].getX()-x)< 1 and abs(self.remoteNames[id].getY() < y) < 1:
					# the user clicked on the card
						if card:
						#if card is not None:
							card.destroy()
							self.remotePics[id] = None

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
		if len(actorCache[avaNum]) > 0 :
			actor = actorCache[avaNum].pop()
		else :
			s = str(avaNum)
			actor = Actor("models/ava" + s, \
				      {"walk":"models/walk" + s})
			if avaNum == 1 :
				self.avatar.setScale(.002)
				self.avatar.setPos(58,67,0)
			elif avaNum == 2 :
				self.avatar.setScale(.2)
				self.avatar.setPos(58,67,0)
			elif avaNum == 3 :
				self.avatar.setScale(.25)
				self.avatar.setPos(58,67,.5)
			elif avaNum == 4 :
				self.avatar.setScale(.25)
				self.avatar.setPos(58,67,.5)
		return actor

	def recycleActor(self, actor, avaNum) :
		""" Recycle an Actor object.

		Just put it back in the cache.
		actor is an Actor object that is not currently needed
		avaNum is the serial number for the actor's model
		"""
		actorCache[avaNum].append(actor)
		
	def addRemote(self, x, y, direction, id, name, avaNum) : 
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
			self.updateRemote(x,y,direction,id,name); return
		remote = self.getActor(avaNum)

		self.remoteMap[id] = [remote, avaNum, False,
				OnscreenImage(image = 'models/dot1.png', \
						pos = (0,0,0), scale = .01)
				]

		# set position and direction of remote and make it visible
		remote.reparentTo(render)
		remote.setPos(x,y,0) 
		remote.setHpr(direction,0,0) 
				
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
					fg = (1, 1, 1 ,1), \
					shadow = (0, 0, 0, 0.5) )
		
		# if off view
		else:
			if id in self.remoteNames.keys():
				if self.remoteNames[id]:
					self.remoteNames[id].destroy()	 				
	def updateRemote(self, x, y, direction, id, name) :
		""" Update location of a remote panda.

		This method is used by the Net object to update the location.
		x,y gives the remote's position
		direction gives its heading
		id is an identifier used to distinguish this remote from others
		"""
		if id not in self.remoteMap : return
		remote, avaNum, isMoving, dot = self.remoteMap[id]
		if isMoving and \
		   x == remote.getX() and y == remote.getY() and \
		   direction == remote.getHpr()[0] :
			remote.stop(); remote.pose("walk",5)
			self.remoteMap[id][2] = False
		elif not isMoving :
			remote.loop("walk")
			self.remoteMap[id][2] = True
		
		# set position and direction of remote
		remote.setPos(x,y,0)
		remote.setHpr(direction,0,0) 
		
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
						fg = (1, 1, 1, 1), \
						shadow = (0, 0, 0, 0.5))

			if id in self.remotePics.keys():
				if self.remotePics[id]:
					try:
						self.remotePics[id].setPos( \
							a2d[0], 0, a2d[1]) 
					# if pic happens to have been removed 
					# by the user
					except Exception as e:
						pass
					
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
		#stream.stop_stream()
		#stream.close()
		#p1.terminate()
		#print "input"
		#wf = wave.open('models/Panda.wav', 'rb')

		#data = wf.readframes(self.CHUNK)

		#while data != '':
		#	p = pyaudio.PyAudio()

		#	stream = p.open(format=p.get_format_from_width(wf.getsampwidth()),
		#		channels=wf.getnchannels(),
		#		rate=wf.getframerate(),
		#		output=True)


		#    	stream.write(data)
		#    	data = wf.readframes(self.CHUNK)

		#	stream.stop_stream()
		#	stream.close()

		#	p.terminate()

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
		self.recycleActor(remote,avaNum)
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
        
    	def getAudioLevel(self) :
		""" Get audioLevel to see if Avatar is talking
		"""
		return self.audioLevel

	def record(self) :
		try:
			self.streamin.read(CHUNK)
		except IOError:
			print 'warning:dropped frame'
	def getaudioData(self) :
		return self.audioData    

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

#		traverser = CollisionTraverser('CustomTraverser')
#	
#		ray = CollisionSegment()
#		# lift two points a bit above the grouond to prevent the
#		# collision ray from hiting the edge of shallow terrain;
#		# also, put them at different level so that the ray has
#		# nonzero length (a requirement for collisionSegment()).
#		p1[2] += 0.5
#		p2[2] += 0.4
#		ray.setPointA(p1)
#		ray.setPointB(p2)
#		fromObj = self.environ.attachNewNode(CollisionNode('visRay'))
#		fromObj.node().addSolid(ray)
#		fromObj.node().setFromCollideMask(BitMask32.bit(0))
#		fromObj.node().setIntoCollideMask(BitMask32.allOff())
#		handler = CollisionHandlerQueue()
#	
#		traverser.addCollider(fromObj, handler)
#	
#		traverser.traverse(render)
#	
#		if handler.getNumEntries() != 0:
#			return False
#		else:
#			return True

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
		# in case he falls off the map or walks into something.
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
			if self.myAvaNum == 1 :
				#self.avatar.setY(self.avatar, \
				#	 -1000 * globalClock.getDt())
				self.avatar.setY(self.avatar.getY() \
					 -1000 * globalClock.getDt())
			elif self.myAvaNum >= 2 and self.myAvaNum <= 4 :
				self.avatar.setY(self.avatar, \
					 -50 * globalClock.getDt())
 		if (self.keyMap["backward"]!=0):
			if self.myAvaNum == 1 :
				self.avatar.setY(self.avatar, \
					 +1000 * globalClock.getDt())
			elif self.myAvaNum >= 2 and self.myAvaNum <= 4 :
				self.avatar.setY(self.avatar, \
					 +50 * globalClock.getDt())
 		if (self.keyMap["strafe-left"]!=0):
 			self.avatar.setX(self.avatar, \
 					 +3000 * globalClock.getDt())
 		if (self.keyMap["strafe-right"]!=0):
 			self.avatar.setX(self.avatar, \
 					 -3000 * globalClock.getDt())
        	if (self.keyMap["voice"]!=0):
                	#mySound = base.loader.loadSfx("models/Panda.mp3")
            		#mySound.play()			
			self.audioLevel = 1
 		else:
			self.audioLevel = 0	

		# If avatar is moving, loop the walk animation.
		# If he is standing still, stop the animation.
		# Not clear there is any need for this, since we don't see
		# our own model. Only reason to keep it is if we allow a
		# different viewpoint
		if (self.keyMap["forward"]!=0) or \
		   (self.keyMap["backward"]!=0) or \
		   (self.keyMap["left"]!=0) or (self.keyMap["right"]!=0) :
			if self.isMoving is False:
				print "starting animation"
				self.avatar.loop("walk")
				self.isMoving = True
		else:
			if self.isMoving:
				print "stopping animation"
				self.avatar.stop()
				self.avatar.pose("walk",5)
				self.isMoving = False
		
		# if the zoom-in key 'i' is held down, move the camera forward
		# if the key 'o' is held down, zoom out	
		hpr = self.avatar.getHpr()
		pos = self.avatar.getPos()
		hpr[0] += 180; hpr[1] = camAngle
		
		if (self.keyMap["zoom-in"] != 0): 
			base.camera.setHpr(hpr)
			base.camera.setY(base.camera, 2 * globalClock.getDt())
		elif (self.keyMap["zoom-out"] != 0):	
			base.camera.setHpr(hpr)
			base.camera.setY(base.camera, -2 * globalClock.getDt())
		elif (self.keyMap["reset-view"] != 0):
			pos[2] += 1
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
				self.avatar.setPos(startpos)
#			else:
#				self.avatar.setZ( \
#					entries[0].getSurfacePoint(render).getZ())

		# Check visibility and update visList
		for id in self.remoteMap:
			if self.canSee(self.avatar.getPos(),
					self.remoteMap[id][0].getPos()) == True:
				if id not in self.visList:
					self.visList.append(id)
			elif id in self.remoteNames.keys():
				if self.remoteNames[id]:
					self.remoteNames[id].destroy()
			elif id in self.visList:
				self.visList.remove(id)
		s = ""
		for id in self.visList : s += fadr2string(id) + " "
 		self.showNumVisible.setText("Visible pandas: %s" % s)

		# Update the text showing panda's position on 
		# the screen
		self.avPos.setText("Panda's pos: (%d, %d, %d)" % \
			(self.avatar.getX(), self.avatar.getY(),
			 (self.avatar.getHpr()[0])%360))

		# map the avatar's position to the 2D map on the top-right 
		# corner of the screen
		self.dot.setPos((self.avatar.getX()/120.0)*0.7+0.45, \
				0,(self.avatar.getY()/120.0)*0.7+0.25)
		for id in self.remoteMap:
			self.remoteMap[id][3].setPos((self.remoteMap[id][0].getX()/120.0)*0.7+0.45, \
				0,(self.remoteMap[id][0].getY()/120.0)*0.7+0.25)

		return task.cont

# w = PandaWorld()
# w.addRemote(69, 67, 135, 111, "Vigo")
# w.addRemote(20, 33, 135, 222, "Chris")
# w.addRemote(40, 60, 135, 333, "Jon")
# w.addRemote(90, 79, 135, 444, "Chao")
# w.addRemote(30, 79, 135, 555, "Andrew")
# w.addRemote(20, 39, 135, 666, "Sam")
# run()
