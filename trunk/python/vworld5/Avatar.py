""" Demonstration of simple virtual world using Forest overlay

usage:
      Avatar myIp cliMgrIp numg subLimit comtree name [ debug ] [ auto ]

- myIp is the IP address of the user's computer
- cliMgrIp is the IP address of the client manager's computer
- numg*numg is the number of multicast groups to use
- subLimit limits the number of multicast groups that we subscribe to;
  we'll subscribe to a group is it's L_0 distance is at most subLimit
- comtree is the number of a pre-configured Forest comtree
- the debug option, if present controls the amount of debugging output;
  use "debug" for a little debugging output, "debuggg" for lots
- the auto option, if present, starts an avatar that wanders aimlessly
"""

import sys
from socket import *
from Net import *
from Mcast import *
from Packet import *
from Util import *
from PandaWorld import *
from AIWorld import *

import direct.directbase.DirectStart
from panda3d.core import *
from direct.gui.OnscreenText import OnscreenText
from direct.gui.OnscreenImage import OnscreenImage
from direct.actor.Actor import Actor
from direct.showbase.DirectObject import DirectObject
from direct.interval.IntervalGlobal import Sequence
from panda3d.core import Point3
import random, sys, os, math, getpass
# process command line arguments
if len(sys.argv) < 7 :
	sys.stderr.write("usage: Avatar myIp cliMgrIp numg " + \
			 "subLimit comtree name|-u uname [ debug ] [ auto ]\n")
        sys.exit(1)

myIp = gethostbyname(sys.argv[1])
cliMgrIp = gethostbyname(sys.argv[2])
numg = int(sys.argv[3])
subLimit = int(sys.argv[4])
myComtree = int(sys.argv[5])
uName = None
myName = None
passWord = None
if sys.argv[6] == '-u':
	uName = sys.argv[7]
	myName = uName
	passWord = getpass.getpass()
else:
	print "use default username and password"
	uName = "user"
	passWord = "pass"
	myName = sys.argv[6]
auto = False; debug = 0
for i in range(6,len(sys.argv)) :
	if sys.argv[i] == "debug" : debug = 1
	elif sys.argv[i] == "debugg" : debug = 2
	elif sys.argv[i] == "debuggg" : debug = 3
	elif sys.argv[i] == "auto" :
		auto = True

pWorld = AIWorld() if auto else PandaWorld()
net = Net(myIp, cliMgrIp, myComtree, numg, subLimit, pWorld, myName, debug)

if auto : print "type Ctrl-C to terminate"

# setup tasks
if not net.init(uName, passWord) :
	sys.stderr.write("cannot initialize net object\n");
	sys.exit(1)
net.sendStatus();
loadPrcFileData("", "parallax-mapping-samples 3")
loadPrcFileData("", "parallax-mapping-scale 0.1")
loadPrcFileData("", "window-type none")

SPEED = 0.5
run()  # start the panda taskMgr
