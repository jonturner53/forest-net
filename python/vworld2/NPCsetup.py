# Author: Chao Wang
# Date: 1.26.2013

import direct.directbase.DirectStart
from panda3d.core import CollisionTraverser,CollisionNode
from panda3d.core import CollisionHandlerQueue,CollisionRay
from panda3d.core import Filename,AmbientLight,DirectionalLight
from panda3d.core import PandaNode,NodePath,Camera,TextNode
from panda3d.core import Vec3,Vec4,BitMask32
from direct.gui.OnscreenText import OnscreenText
from direct.actor.Actor import Actor
from direct.showbase.DirectObject import DirectObject
from direct.interval.IntervalGlobal import Sequence
from panda3d.core import Point3
import random, sys, os, math

# Function to create NPCs.
def spawnNPCs(self):
    self.npc1 = Actor("models/panda-model", {"run":"models/panda-walk4"})
    self.npc1.reparentTo(render)
    self.npc1.setScale(.002)
    #self.npc1.setPos(-4,22,0)
    self.npc1.loop("run")
    npc1i1 = self.npc1.posInterval(25,
                                   Point3(55, 12, 0),
                                   startPos=Point3(5, 12, 0))
    npc1i2 = self.npc1.posInterval(25,
                                   Point3(5, 12, 0),
                                   startPos=Point3(55, 12, 0))
    self.npc1Route = Sequence(npc1i1, npc1i2, name="npc1Route")
    self.npc1Route.loop()

    self.npc2 = Actor("models/panda-model", {"run":"models/panda-walk4"})
    self.npc2.reparentTo(render)
    self.npc2.setScale(.002)
    self.npc2.setPos(40,5,0)
    self.npc2.loop("run")

    self.npc3 = Actor("models/panda-model", {"run":"models/panda-walk4"})
    self.npc3.reparentTo(render)
    self.npc3.setScale(.002)
    self.npc3.setPos(20,35,0)
    self.npc3.loop("run")

    self.npc4 = Actor("models/panda-model", {"run":"models/panda-walk4"})
    self.npc4.reparentTo(render)
    self.npc4.setScale(.002)
    self.npc4.setPos(95,65,0)
    self.npc4.loop("run")

    self.npc5 = Actor("models/panda-model", {"run":"models/panda-walk4"})
    self.npc5.reparentTo(render)
    self.npc5.setScale(.002)
    self.npc5.setPos(95,75,0)
    self.npc5.loop("run")

    self.npc6 = Actor("models/panda-model", {"run":"models/panda-walk4"})
    self.npc6.reparentTo(render)
    self.npc6.setScale(.002)
    self.npc6.setPos(105,65,0)
    self.npc6.loop("run")

    self.npc7 = Actor("models/panda-model", {"run":"models/panda-walk4"})
    self.npc7.reparentTo(render)
    self.npc7.setScale(.002)
    self.npc7.setPos(105,75,0)
    self.npc7.loop("run")

    self.npc8 = Actor("models/panda-model", {"run":"models/panda-walk4"})
    self.npc8.reparentTo(render)
    self.npc8.setScale(.002)
    #self.npc8.setPos(-4,22,0)
    self.npc8.loop("run")
    npc8i1 = self.npc8.posInterval(25,
                                   Point3(75, 3, 0),
                                   startPos=Point3(50, 3, 0))
    npc8i2 = self.npc8.posInterval(25,
                                   Point3(50, 3, 0),
                                   startPos=Point3(75, 3, 0))
    self.npc8Route = Sequence(npc8i1, npc8i2, name="npc8Route")
    self.npc8Route.loop()

    self.npc9 = Actor("models/panda-model", {"run":"models/panda-walk4"})
    self.npc9.reparentTo(render)
    self.npc9.setScale(.002)
    #self.npc9.setPos(-4,22,0)
    self.npc9.loop("run")
    npc9i1 = self.npc9.posInterval(25,
                                   Point3(72, 40, 0),
                                   startPos=Point3(72, 7, 0))
    npc9i2 = self.npc9.posInterval(25,
                                   Point3(72, 7, 0),
                                   startPos=Point3(72, 40, 0))
    self.npc9Route = Sequence(npc9i1, npc9i2, name="npc9Route")
    self.npc9Route.loop()

    self.npc10 = Actor("models/panda-model", {"run":"models/panda-walk4"})
    self.npc10.reparentTo(render)
    self.npc10.setScale(.002)
    #self.npc10.setPos(-4,22,0)
    self.npc10.loop("run")
    npc10i1 = self.npc10.posInterval(25,
                                   Point3(20, 65, 0),
                                   startPos=Point3(20, 60, 0))
    npc10i2 = self.npc10.posInterval(25,
                                   Point3(20, 60, 0),
                                   startPos=Point3(20, 65, 0))
    self.npc10Route = Sequence(npc10i1, npc10i2, name="npc10Route")
    self.npc10Route.loop()


"""
    self.npc2 = Actor("models/panda-model", {"run":"models/panda-walk4"})
    self.npc2.reparentTo(render)
    self.npc2.setScale(.004)
    #self.npc1.setPos(-4,22,0)
    self.npc2.loop("run")
    npc2i1 = self.npc2.posInterval(25,
                                   Point3(-76, -24, 0),
                                   startPos=Point3(-76, 12, 0))
    npc2i2 = self.npc2.posInterval(25,
                                   Point3(-76, 12, 0),
                                   startPos=Point3(-76, -24, 0))
    npc2h1 = self.npc2.hprInterval(3,
                                   Point3(180, 0, 0),
                                   startHpr=Point3(0, 0, 0))
    npc2h2 = self.npc2.hprInterval(3,
                                   Point3(0, 0, 0),
                                   startHpr=Point3(180, 0, 0))
    self.npc2Route = Sequence(npc2i1, npc2h1, npc2i2, npc2h2, name="npc2Route")
    self.npc2Route.loop()

    # 3 way points
    self.npc3 = Actor("models/panda-model", {"run":"models/panda-walk4"})
    self.npc3.reparentTo(render)
    self.npc3.setScale(.002)
    #self.npc1.setPos(-4,22,0)
    self.npc3.loop("run")
    npc3i1 = self.npc3.posInterval(20,
                                   Point3(-72, 61, 0),
                                   startPos=Point3(-52, 61, 0))
    npc3i2 = self.npc3.posInterval(20,
                                   Point3(-72, 23, 0),
                                   startPos=Point3(-72, 61, 0))
    npc3i3 = self.npc3.posInterval(20,
                                   Point3(-72, 61, 0),
                                   startPos=Point3(-72, 23, 0))
    npc3i4 = self.npc3.posInterval(20,
                                   Point3(-52, 61, 0),
                                   startPos=Point3(-72, 61, 0))
    npc3h0 = self.npc3.hprInterval(3,
                                   Point3(-90, 0, 0),
                                   startHpr=Point3(-180, 0, 0))
    npc3h1 = self.npc3.hprInterval(3,
                                   Point3(0, 0, 0),
                                   startHpr=Point3(-90, 0, 0))
    npc3h2 = self.npc3.hprInterval(3,
                                   Point3(180, 0, 0),
                                   startHpr=Point3(0, 0, 0))
    npc3h3 = self.npc3.hprInterval(3,
                                   Point3(90, 0, 0),
                                   startHpr=Point3(180, 0, 0))
    npc3h4 = self.npc3.hprInterval(3,
                                   Point3(-90, 0, 0),
                                   startHpr=Point3(90, 0, 0))
    self.npc3Route = Sequence(npc3h0,
                              npc3i1, npc3h1, npc3i2, npc3h2,
                              npc3i3, npc3h3, npc3i4, npc3h4,
                              name="npc3Route")
    self.npc3Route.loop()

    # 4 way points
    self.npc4 = Actor("models/panda-model", {"run":"models/panda-walk4"})
    self.npc4.reparentTo(render)
    self.npc4.setScale(.003)
    #self.npc1.setPos(-4,22,0)
    self.npc4.loop("run")
    npc4i1 = self.npc4.posInterval(10,
                                   Point3(-58, -27, 0),
                                   startPos=Point3(-58, -23, 0))
    npc4i2 = self.npc4.posInterval(10,
                                   Point3(-68, -27, 0),
                                   startPos=Point3(-58, -27, 0))
    npc4i3 = self.npc4.posInterval(10,
                                   Point3(-68, -23, 0),
                                   startPos=Point3(-68, -27, 0))
    npc4i4 = self.npc4.posInterval(10,
                                   Point3(-52, 61, 0),
                                   startPos=Point3(-72, 61, 0))
    npc4h1 = self.npc4.hprInterval(3,
                                   Point3(-90, 0, 0),
                                   startHpr=Point3(0, 0, 0))
    npc4h2 = self.npc4.hprInterval(3,
                                   Point3(-180, 0, 0),
                                   startHpr=Point3(-90, 0, 0))
    npc4h3 = self.npc4.hprInterval(3,
                                   Point3(90, 0, 0),
                                   startHpr=Point3(-180, 0, 0))
    npc4h4 = self.npc4.hprInterval(3,
                                   Point3(0, 0, 0),
                                   startHpr=Point3(90, 0, 0))
    self.npc4Route = Sequence(
                              npc4i1, npc4h1, npc4i2, npc4h2,
                              npc4i3, npc4h3, npc4i4, npc4h4,
                              name="npc4Route")
    self.npc4Route.loop()
"""
