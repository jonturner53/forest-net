""" Avatar for a simple virtual world

usage:
      Avatar myIp cliMgrIp wallsFile comtree userName passWord [ debug ]

Command line arguments include the ip address of the
avatar's machine, the client manager's IP address,
the walls file, the comtree to be used in the packet,
a user name and password and the number of
seconds to run before terminating.
"""

"""
Implementation notes

Multiple threads
- substrate handles all socket IO and multiplexes data from next level
- net thread generates/consumes reports and sends/receives subscriptions
  - just do a 20 ms loop, rather than bother with time
- control thread sends/receives control packets; one at a time;
  when core wants to join or leave a comtree, it sends a request to
  the control thread and then waits for response; control thread handles
  retransmissions;

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

if len(sys.argv) < 8 :
	sys.stderr.write("usage: Avatar myIp cliMgrIp walls " + \
			 "comtree uname pword finTime\n")
        sys.exit(1)

myIp = sys.argv[1]; cliMgrIp = sys.argv[2]
wallsFile = sys.argv[3]; myComtree = int(sys.argv[4])
uname = sys.argv[5]; pword = sys.argv[6]

debug = 0
if len(sys.argv) > 7 :
	if sys.argv[7] == "debug" : debug = 1
	elif sys.argv[7] == "debugg" : debug = 2
	elif sys.argv[7] == "debuggg" : debug = 3

world = WallsWorld() 
if not world.init(wallsFile) :
	sys.stderr.write("cannot initialize world from walls file\n");
	sys.exit(1)

pWorld = PandaWorld()
net = Net(myIp, cliMgrIp, myComtree, world, pWorld, debug)

# setup tasks
if not net.init(uname, pword) :
	sys.stderr.write("cannot initialize net object\n");
	sys.exit(1)

loadPrcFileData("", "parallax-mapping-samples 3")
loadPrcFileData("", "parallax-mapping-scale 0.1")

SPEED = 0.5
run()  # start the panda taskMgr
