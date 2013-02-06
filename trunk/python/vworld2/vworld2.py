""" A front-end of Avatar

usage: 
      This module is intended to be called by Avatar.py.
	  To run it independently, uncomment the last two lines:
         #w = World()
         #run()
      and run 'python vworld2.py' in command line.

control:
      Use arrow keys (Up, Left, Right) to move the avatar;
      press "A" and "S" to rotate the camera.
"""

"""
Author: Chao Wang
Last Updated: 2/6/2013
 
This program is a front-end of Avatar, an application
that demonstrates the Forest overlay network.
 
The gameplay (intended) is to have two pandas walk around
in a 3D virtual world, each of which is controlled by a player.
Besides the two pandas, there are several little boys running
aimlessly in the world. Each player guides his panda
to get close to a boy, and casts a spell that turns the
boy into a panda. A player won by turning the maximum
number of boys into pandas.
 
The 2D map on the upper-right corner of the screen
shows the positions of both players and NPCs. The text
on the upper-left of the screen shows each player's
(x,y) position, the # of nearby NPCs, the # of visible NPCs,
and the # of boys that have been turned into pandas by him. 
""" 

""" 
This program is based on "Roaming Ralph", a tutorial
included in Panda3D package. Below is the original doc:
 
Author: Ryan Myers
Models: Jeff Styers, Reagan Heller

Last Updated: 6/13/2005
 
This tutorial provides an example of creating a character
and having it walk around on uneven terrain, as well
as implementing a fully rotatable camera.
"""  

import direct.directbase.DirectStart
from panda3d.core import *
#from panda3d.core import CollisionTraverser,CollisionNode
#from panda3d.core import CollisionHandlerQueue,CollisionRay
#from panda3d.core import Filename,AmbientLight,DirectionalLight
#from panda3d.core import PandaNode,NodePath,Camera,TextNode
#from panda3d.core import Vec3,Vec4,BitMask32
#from panda3d.core import TransparencyAttrib
from direct.gui.OnscreenText import OnscreenText
from direct.gui.OnscreenImage import OnscreenImage
from direct.actor.Actor import Actor
from direct.showbase.DirectObject import DirectObject
from direct.interval.IntervalGlobal import Sequence
from panda3d.core import Point3
import random, sys, os, math

from NPCsetup import spawnNPCs

loadPrcFileData("", "parallax-mapping-samples 3")
loadPrcFileData("", "parallax-mapping-scale 0.1")

SPEED = 0.5

# Function to put title on the screen.
def addTitle(text):
    return OnscreenText(text=text, style=1, fg=(1,1,1,1),
                        pos=(1.3,-0.95), align=TextNode.ARight, scale = .07)

# Function to print Avatar's position on the screen.
def addAvPos(pos, avatar):
    return OnscreenText(text="Avatar's pos: (%d, %d)" % (avatar.getX(), avatar.getY()),
                        style=2, fg=(1,0.8,0.7,1), pos=(-1.3, pos),
                        align=TextNode.ALeft, scale = .08, mayChange = True)

# Function to print Avatar's position on the screen.
def showMapPos(pos, avatar):
    return OnscreenText(text="Map's pos: (%f, %f)" % (avatar.getX(), avatar.getZ()),
                        style=2, fg=(1,0.8,0.7,1), pos=(-1.3, pos),
                        align=TextNode.ALeft, scale = .08, mayChange = True)


class World(DirectObject):

    def __init__(self):
        
        self.keyMap = {"left":0, "right":0, "forward":0, "cam-left":0, "cam-right":0}
        base.win.setClearColor(Vec4(0,0,0,1))

        # Post the instructions
        self.title = addTitle("Finding Boys")

        self.Dmap = OnscreenImage(image = 'models/2Dmap.png', pos = (.8,0,.6),
                                   scale = .4)
        self.Dmap.setTransparency(TransparencyAttrib.MAlpha)
        self.dot = OnscreenImage(image = 'models/dot.png', pos = (1,0,1),
                                   scale = .02)
        
        # Set up the environment
        #
        # This environment model contains collision meshes.  If you look
        # in the egg file, you will see the following:
        #
        #    <Collide> { Polyset keep descend }
        #
        # This tag causes the following mesh to be converted to a collision
        # mesh -- a mesh which is optimized for collision, not rendering.
        # It also keeps the original mesh, so there are now two copies ---
        # one optimized for rendering, one for collisions.  

        self.environ = loader.loadModel("models/vworld-grid-24")
        self.environ.reparentTo(render)
        self.environ.setPos(0,0,0)
		
        base.setBackgroundColor(0.4,0.4,0.5,1)
        
        # Create the panda
        self.avatar = Actor("models/panda-model",
                                 {"run":"models/panda-walk4",
                                  "walk":"models/panda-walk"})
        self.avatar.reparentTo(render)
        self.avatar.setScale(.002)
        self.avatar.setPos(58,67,0)
        self.avatar.setHpr(135,0,0)

		# set the dot's position
        self.dot.setPos(self.avatar.getX()/(120.0+self.Dmap.getX()),
                        0,
                        self.avatar.getY()/(120.0+self.Dmap.getZ()))
        self.dotOrigin = self.dot.getPos()

        # Show avatar's position on the screen
        # self.avPos = addAvPos(0.55, self.avatar)

		# print Map's pos
        # self.mapPos = showMapPos(0.2, self.Dmap)
        # self.dotPos = showMapPos(0, self.dot)
		
            
        spawnNPCs(self)

        # Create a floater object.  We use the "floater" as a temporary
        # variable in a variety of calculations.
        self.floater = NodePath(PandaNode("floater"))
        self.floater.reparentTo(render)

        # Accept the control keys for movement and rotation
        self.accept("escape", sys.exit)
        self.accept("arrow_left", self.setKey, ["left",1])
        self.accept("arrow_right", self.setKey, ["right",1])
        self.accept("arrow_up", self.setKey, ["forward",1])
        self.accept("a", self.setKey, ["cam-left",1])
        self.accept("s", self.setKey, ["cam-right",1])
        self.accept("arrow_left-up", self.setKey, ["left",0])
        self.accept("arrow_right-up", self.setKey, ["right",0])
        self.accept("arrow_up-up", self.setKey, ["forward",0])
        self.accept("a-up", self.setKey, ["cam-left",0])
        self.accept("s-up", self.setKey, ["cam-right",0])

        taskMgr.add(self.move,"moveTask")

        # Game state variables
        self.isMoving = False

        # Set up the camera
        base.disableMouse()
        base.camera.setPos(self.avatar.getX(),self.avatar.getY()+12,9)
        base.camera.setHpr(135,0,0)
        
        # We will detect the height of the terrain by creating a collision
        # ray and casting it downward toward the terrain.  One ray will
        # start above avatar's head, and the other will start above the camera.
        # A ray may hit the terrain, or it may hit a rock or a tree.  If it
        # hits the terrain, we can detect the height.  If it hits anything
        # else, we rule that the move is illegal.

        self.cTrav = CollisionTraverser()

        self.avatarGroundRay = CollisionRay()
        self.avatarGroundRay.setOrigin(0,0,1000)
        self.avatarGroundRay.setDirection(0,0,-1)
        self.avatarGroundCol = CollisionNode('avatarRay')
        self.avatarGroundCol.addSolid(self.avatarGroundRay)
        self.avatarGroundCol.setFromCollideMask(BitMask32.bit(0))
        self.avatarGroundCol.setIntoCollideMask(BitMask32.allOff())
        self.avatarGroundColNp = self.avatar.attachNewNode(self.avatarGroundCol)
        self.avatarGroundHandler = CollisionHandlerQueue()
        self.cTrav.addCollider(self.avatarGroundColNp, self.avatarGroundHandler)

        self.camGroundRay = CollisionRay()
        self.camGroundRay.setOrigin(0,0,1000)
        self.camGroundRay.setDirection(0,0,-1)
        self.camGroundCol = CollisionNode('camRay')
        self.camGroundCol.addSolid(self.camGroundRay)
        self.camGroundCol.setFromCollideMask(BitMask32.bit(0))
        self.camGroundCol.setIntoCollideMask(BitMask32.allOff())
        self.camGroundColNp = base.camera.attachNewNode(self.camGroundCol)
        self.camGroundHandler = CollisionHandlerQueue()
        self.cTrav.addCollider(self.camGroundColNp, self.camGroundHandler)
        
        # Create some lighting
        directionalLight = DirectionalLight("directionalLight")
        directionalLight.setDirection(Vec3(-5, -5, -5))
        directionalLight.setColor(Vec4(.8, .8, .9, 1))
        directionalLight.setSpecularColor(Vec4(.3, .1, .1, 1))
        render.setLight(render.attachNewNode(directionalLight))

        dLight2 = DirectionalLight("directionalLight2")
        dLight2.setDirection(Vec3(5, 5, 5))
        dLight2.setColor(Vec4(.5, .5, .5, 1))
        dLight2.setSpecularColor(Vec4(.3, .3, .1, 1))
        render.setLight(render.attachNewNode(dLight2))

        self.light = render.attachNewNode(Spotlight("Spot"))
        self.light.node().setScene(render)
        self.light.node().setShadowCaster(True)
        self.light.node().showFrustum()
        self.light.node().getLens().setFov(40)
        self.light.node().getLens().setNearFar(10,100)
        self.light.node().setColor(Vec4(7, 7, 7, 1))
        render.setLight(self.light)

        self.light.setPos(-10,-50,100)
        self.light.lookAt(120,120,0)
        self.light.node().getLens().setNearFar(10,1000)
        
        self.environ.setShaderInput("light", self.light)

        # Important! Enable the shader generator.
        render.setShaderAuto()

        self.shaderenable = 1

    
    #Records the state of the arrow keys
    def setKey(self, key, value):
        self.keyMap[key] = value

    """
    def addAvatar(self, x, y, direction, identifier) : ...
    
    def updateAvatar(self, x, y, direction, identifier)
    
    """

    # Accepts arrow keys to move either the player or the menu cursor,
    # Also deals with grid checking and collision detection
    def move(self, task):

        # If the camera-left key is pressed, move camera left.
        # If the camera-right key is pressed, move camera right.
        # base.camera.lookAt(self.avatar)
        if (self.keyMap["cam-left"]!=0):
            base.camera.setX(base.camera, -20 * globalClock.getDt())
        if (self.keyMap["cam-right"]!=0):
            base.camera.setX(base.camera, +20 * globalClock.getDt())

        # save avatar's initial position so that we can restore it,
        # in case he falls off the map or runs into something.
        startpos = self.avatar.getPos()

        # If a move-key is pressed, move avatar in the specified direction.
        # Chao: I need to know more about globalClock to set the walking speed right
        if (self.keyMap["left"]!=0):
            self.avatar.setH(self.avatar.getH() + 300 * globalClock.getDt())
        if (self.keyMap["right"]!=0):
            self.avatar.setH(self.avatar.getH() - 300 * globalClock.getDt())
        if (self.keyMap["forward"]!=0):
            self.avatar.setY(self.avatar, -3000 * globalClock.getDt())

        # If avatar is moving, loop the run animation.
        # If he is standing still, stop the animation.
        if (self.keyMap["forward"]!=0) or (self.keyMap["left"]!=0) or (self.keyMap["right"]!=0):
            if self.isMoving is False:
                self.avatar.loop("run")
                self.isMoving = True
        else:
            if self.isMoving:
                self.avatar.stop()
                self.avatar.pose("run",5)
                self.isMoving = False

        # If the camera is too far from avatar, move it closer.
        # If the camera is too close to avatar, move it farther.
        camvec = self.avatar.getPos() - base.camera.getPos()
        camvec.setZ(0)
        camdist = camvec.length()
        camvec.normalize()
        if (camdist > 15.0):
            base.camera.setPos(base.camera.getPos() + camvec*(camdist-15))
            camdist = 15.0
        if (camdist < 5.0):
            base.camera.setPos(base.camera.getPos() - camvec*(5-camdist))
            camdist = 5.0

        # Now check for collisions.
        self.cTrav.traverse(render)

        # Adjust avatar's Z coordinate.  If avatar's ray hit terrain,
        # update his Z. If it hit anything else, or didn't hit anything, put
        # him back where he was last frame.

        entries = []
        for i in range(self.avatarGroundHandler.getNumEntries()):
            entry = self.avatarGroundHandler.getEntry(i)
            entries.append(entry)
        entries.sort(lambda x,y: cmp(y.getSurfacePoint(render).getZ(),
                                     x.getSurfacePoint(render).getZ()))
        if (len(entries)>0) and (entries[0].getIntoNode().getName() == "ID602"):
            self.avatar.setZ(entries[0].getSurfacePoint(render).getZ())
        else:
            self.avatar.setPos(startpos)

        # Keep the camera at one foot above the terrain,
        # or 8 feet above avatar, whichever is greater.
        entries = []
        for i in range(self.camGroundHandler.getNumEntries()):
            entry = self.camGroundHandler.getEntry(i)
            entries.append(entry)
        entries.sort(lambda x,y: cmp(y.getSurfacePoint(render).getZ(),
                                     x.getSurfacePoint(render).getZ()))
        if (len(entries)>0) and (entries[0].getIntoNode().getName() == "ID602"):
            base.camera.setZ(entries[0].getSurfacePoint(render).getZ()+1.0)
        if (base.camera.getZ() < self.avatar.getZ() + 8):
            base.camera.setZ(self.avatar.getZ() + 8)
            
        # The camera should look in avatar's direction,
        # but it should also try to stay horizontal, so look at
        # a floater which hovers above avatar's head.
        self.floater.setPos(self.avatar.getPos())
        self.floater.setZ(self.avatar.getZ() + 1.0)
        base.camera.lookAt(self.floater)

        # Finally, update the text that shows avatar's position on the screen
#        self.avPos.setText("Avatar's pos: (%d, %d)"
#                            % (self.avatar.getX(), self.avatar.getY()))
#        self.dotPos.setText("dot's pos: (%f, %f)"
#                            % (self.dot.getX(), self.dot.getZ()))

        # map the avatar's position to the 2D map on the top-right corner of the screen
        self.dot.setPos((self.avatar.getX()/120.0)*0.7+0.45,
                        0,
                        (self.avatar.getY()/120.0)*0.7+0.25)

        return task.cont

#w = World()
#run()

