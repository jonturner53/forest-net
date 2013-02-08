""" Demonstration of simple virtual world using Forest overlay

usage:
      Avatar myIp cliMgrIp mapFile comtree [ debug ] [ auto ]

- myIp is the IP address of the user's computer
- cliMgrIp is the IP address of the client manager's computer
- mapFile is a text file that defines which parts of the world are
  walkable and which are visible from each other
- comtree is the number of a pre-configured Forest comtree
- the debug option, if present controls the amount of debugging output;
  use "debug" for a little debugging output, "debuggg" for lots
- the auto option, if present, starts an avatar that wanders aimlessly
"""

import sys
from socket import *
from Net import *
from Packet import *
from Util import *
from PandaWorld import *

import direct.directbase.DirectStart
from panda3d.core import *
from direct.gui.OnscreenText import OnscreenText
from direct.gui.OnscreenImage import OnscreenImage
from direct.actor.Actor import Actor
from direct.showbase.DirectObject import DirectObject
from direct.interval.IntervalGlobal import Sequence
from panda3d.core import Point3
import random, sys, os, math

# process command line arguments
if len(sys.argv) < 5 :
	sys.stderr.write("usage: Avatar myIp cliMgrIp mapFile " + \
			 "comtree [ debug ] [ auto ]\n")
        sys.exit(1)

myIp = sys.argv[1]; cliMgrIp = sys.argv[2]
mapFile = sys.argv[3]; myComtree = int(sys.argv[4])

auto = False; debug = 1
for i in range(5,len(sys.argv)) :
	if sys.argv[i] == "debug" : debug = 1
	elif sys.argv[i] == "debugg" : debug = 2
	elif sys.argv[i] == "debuggg" : debug = 3
	elif sys.argv[i] == "auto" : debug = True

map = WorldMap() 
if not map.init(mapFile) :
	sys.stderr.write("cannot initialize map from mapFile\n");
	sys.exit(1)

pWorld = PandaWorld()
net = Net(myIp, cliMgrIp, myComtree, map, pWorld, debug, auto)

# setup tasks
if not net.init("user", "pass") :
	sys.stderr.write("cannot initialize net object\n");
	sys.exit(1)

loadPrcFileData("", "parallax-mapping-samples 3")
loadPrcFileData("", "parallax-mapping-scale 0.1")

SPEED = 0.5
run()  # start the panda taskMgr
