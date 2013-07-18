""" PandaWorld - simple virtual environment with wandering pandas.

This module is intended to be called by Avatar.py;
To run it independently, uncomment the end of this code:
	# w = PandaWorld()
	# (spawning NPCs...)
 	# run()
and type "python PandaWorld.py"
 
Author: Chao Wang and Jon Turner
World Model: Chao Wang
 
Adapted from "Roaming Ralph", a tutorial included in Panda3D package.
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

import pyaudio
import wave



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
 				"cam-up":0, "cam-down":0, "cam-left":0, "cam-right":0, \
				"zoom-in":0, "zoom-out":0, "reset-view":1, "dash":0}
		self.info = 0
		self.isMute = 0
		self.card = OnscreenImage(image = 'photo_cache/'+ 'unmute.jpg', pos = (0.67, 0, -0.94), scale=.04)
		self.card.setTransparency(TransparencyAttrib.MAlpha)
		base.win.setClearColor(Vec4(0,0,0,1))
		
		self.fieldAngle = 40

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
         
         

		
		# Setup pool of remotes
		# do not attach to scene yet
		self.remoteMap = {}
		self.freeRemotes = []
		self.remoteNames = {}
		self.remotePics = {}
		for i in range(0,self.maxRemotes) : # allow up to 100 remotes
			self.freeRemotes.append(Actor("models/panda-model", \
						{"run":"models/panda-walk4", \
				    "walk":"models/panda-walk"}))
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
		self.accept("mouse1", self.showPic)
		self.accept("f1", self.showInfo)
		self.accept("m", self.mute)
        
		taskMgr.add(self.move,"moveTask")
        

		# Game state variables
		self.isMoving = False
        	self.isListening = False
		# Panda is not moving at the beginning

		# Set up the camera
		p = pyaudio.PyAudio()
		print p.get_device_info_by_index(0)['defaultSampleRate']

		base.disableMouse()
		pos = self.avatar.getPos(); pos[2] += 1
		hpr = self.avatar.getHpr();
		hpr[0] += 180;
		base.camera.setPos(pos); base.camera.setHpr(hpr)		
			
		#base.camera.setPos(self.avatar.getX(),self.avatar.getY(),.2)
		#base.camera.setHpr(self.avatar.getHpr()[0],0,0)
		
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

		"""
		setup collision detection objects used to implement
		the canSee method
		"""
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


		"""
		Finally, let there be light
		"""
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


	def resetCamKeys(self):
		self.keyMap["cam-up"]=0
		self.keyMap["cam-down"]=0
		self.keyMap["cam-left"]=0
		self.keyMap["cam-right"]=0
		
	#to display/hide pictures when the user clicks on the avatar/pic
	def showPic(self):
		x = y = 0
		if base.mouseWatcherNode.hasMouse():
			x=base.mouseWatcherNode.getMouseX()
			y=base.mouseWatcherNode.getMouseY()
			#print "has mouse @: " + str(x) + " " + str(y)
		responded = 0	
		for id in self.remoteNames.keys():
			if abs(self.remoteNames[id].getPos()[0]- x) < 0.1 and abs(self.remoteNames[id].getPos()[1] - y) < 0.1:
				# click occurs close enough to the name of the avatar, try to create pic 
				if id not in self.remotePics.keys() or (id in self.remotePics.keys() and self.remotePics[id] is None):
					name = self.remoteNames[id].getText()
					card = OnscreenImage(image = 'photo_cache/'+ name +'.jpg', pos = (x, 0, y), scale = (0.2, 1, 0.2))
					self.remotePics[id] = card
					responded = 1
		if responded is not 1:
			for id in self.remotePics.keys():
				if self.remotePics[id]:
					card = self.remotePics[id]
					if abs(self.remoteNames[id].getX() - x) < 1 and abs(self.remoteNames[id].getY() < y) < 1:
					# the user clicked on the card
						if card:
						#if card is not None:
							card.destroy()
							self.remotePics[id] = None

	def setKey(self, key, value):
		""" Records the state of the arrow keys
		"""
		self.keyMap[key] = value

	def showInfo(self):
		# Show a list of instructions
		if self.info is 0:
			self.ctrlCmd = printText(0.7)
			self.ctrlCmd.setText("Z: zoom-in" + '\n' + "Ctrl+Z: zoom-out" \
								+ '\n' + "*R: reset view to Panda"\
								+ '\n' + "*T: toggle Zoom mode" \
								+ '\n' + "*M: mute/un-mute audio"\
								+ '\n' + "Arrow Keys: move avatar"\
								+ '\n' + "Shift+Arrows: look around")
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
					
	def addRemote(self, x, y, direction, id, name) : 
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
			self.updateRemote(x,y,direction,id,name); return
		if len(self.freeRemotes) == 0 :
			sys.stderr.write("PandaWorld.addRemote: no unused " + \
					 "remotes\n")
			sys.exit(1)
		remote = self.freeRemotes.pop()
		remote.reparentTo(render)

		self.remoteMap[id] = [remote, False,
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
				self.remoteNames[id] = OnscreenText( text = name, pos = a2d, scale = 0.04, \
												fg = (1, 1, 1 ,1), shadow = (0, 0, 0, 0.5) )
		
		# if off view
		else:
			if id in self.remoteNames.keys():
				if self.remoteNames[id]:
					self.remoteNames[id].destroy()	 				
					
		#remote.loop("run")

	def updateRemote(self, x, y, direction, id, name) :
		""" Update location of a remote panda.

		This method is used by the Net object to update the location.
		x,y gives the remote's position
		direction gives its heading
		id is an identifier used to distinguish this remote from others
		"""
		if id not in self.remoteMap : return
		remote, isMoving, dot = self.remoteMap[id]
		if x - remote.getX() == 0.0 and \
		   y - remote.getY() == 0.0 and \
		   direction == remote.getHpr()[0] :
			remote.stop(); remote.pose("run",5)
			self.remoteMap[id][1] = False
			#return
		elif not isMoving :
			remote.loop("run")
			self.remoteMap[id][1] = True
		
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
			self.remoteNames[id] = OnscreenText(text = name, pos = a2d, scale = 0.04, \
									fg = (1, 1, 1, 1), shadow = (0, 0, 0, 0.5))

			if id in self.remotePics.keys():
				if self.remotePics[id]:
					#try:
					self.remotePics[id].setPos(a2d[0], 0, a2d[1]) 
					# if pic happens to have been removed by the user
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
		self.remoteMap[id][2].destroy()
		remote = self.remoteMap[id][0]
		
		if id in self.remoteNames.keys():
			if self.remoteNames[id]:
				self.remoteNames[id].destroy()	
				
		if id in self.remotePics.keys():
			if self.remotePics[id]:
				self.remotePics[id].destroy()
				self.remotePics[id] = None
				
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
		It responds to the arrow keys to move the avatar
		and to the camera up/down keys.
		"""

		# If the camera-up key is held down, look up
		# If the camera-down key is pressed, look down
		if (self.keyMap["cam-up"]!=0): camAngle = 10
		elif (self.keyMap["cam-down"]!=0): camAngle = -10
		else : camAngle = 0 
		
		if (self.keyMap["cam-left"]!=0): camAngleX = 10
		elif (self.keyMap["cam-right"]!=0): camAngleX = -10
		else: camAngleX = 0

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
					 -1000 * globalClock.getDt())
 		if (self.keyMap["backward"]!=0):
 			self.avatar.setY(self.avatar, \
 					 +1000* globalClock.getDt())
 		if (self.keyMap["dash"]!=0):
			self.avatar.setY(self.avatar, \
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
		
		# if the zoom-in key 'i' is held down, increase fov of the lens
		# fov: field of view, 20--80 is acceptable range, others distorted
		# if the key 'o' is held down, zoom out	
		hpr = self.avatar.getHpr()
		pos = self.avatar.getPos()
		hpr[0] = hpr[0] + 180 + camAngleX; hpr[1] = camAngle
		
		if (self.keyMap["zoom-in"] != 0): 			
			if self.fieldAngle > 20:
				self.fieldAngle = self.fieldAngle - 5
			base.camLens.setFov(self.fieldAngle)
		elif (self.keyMap["zoom-out"] != 0):	
			if self.fieldAngle < 80:
				self.fieldAngle = self.fieldAngle + 5
			base.camLens.setFov(self.fieldAngle)		
		elif (self.keyMap["reset-view"] != 0):
			pos[2] = pos[2] + 1
			base.camera.setPos(pos); base.camera.setHpr(hpr)
			base.camLens.setFov(40)
			


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
			self.remoteMap[id][2].setPos((self.remoteMap[id][0].getX()/120.0)*0.7+0.45, \
				0,(self.remoteMap[id][0].getY()/120.0)*0.7+0.25)

		return task.cont

#"""
w = PandaWorld()
w.addRemote(69, 67, 135, 111, "Vigo")
w.addRemote(20, 33, 135, 222, "Chris")
w.addRemote(52, 46, 135, 333, "Jon")
w.addRemote(90, 79, 135, 444, "Chao")
run()
#"""
