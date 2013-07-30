""" Virtual Tour on Danforth Campus, Washington University in St. Louis

usage:
      python wuTour.py myIp cliMgrIp numg subLimit comtree name[ debug ] [ auto ]

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
from userWorld import *
from NPCWorld import *

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
if len(sys.argv) < 6 :
	sys.stderr.write("usage: python wuTour.py cliMgrIp numg " + \
			 "subLimit comtree name|-u name[ debug ] [ auto ] [autos]\n")
        sys.exit(1)

cliMgrIp = gethostbyname(sys.argv[1])
numg = int(sys.argv[2])
subLimit = int(sys.argv[3])
myComtree = int(sys.argv[4])

uName = None
myName = None
password = None

if sys.argv[5] == '-u':
	uName = sys.argv[6]
	myName = uName
	password = getpass.getpass()
else:
	print "use default username and password"
	uName = "user"
	password = "pass"
	myName = sys.argv[5]

auto = False
autos = False
debug = 0
for i in range(5,len(sys.argv)) :
	if sys.argv[i] == "debug" : debug = 1
	elif sys.argv[i] == "debugg" : debug = 2
	elif sys.argv[i] == "debuggg" : debug = 3
	elif sys.argv[i] == "auto" : auto = True
	elif sys.argv[i] == "autos" : autos = True

#pWorld = NPCWorld() if auto else userWorld()
#net = Net(cliMgrIp, myComtree, numg, subLimit, pWorld, uName, debug)

if auto == False and autos == False:
	pWorld = userWorld()
	net = Net(cliMgrIp, myComtree, numg, subLimit, pWorld, uName, debug)
	# setup tasks
	if not net.init(uName, password) :
		sys.stderr.write("cannot initialize net object\n")
		sys.exit(1)
	net.sendStatus()
else :
	if autos : botNum = 1
	else : botNum = 1
	vworld = {}; vnet = {}
	for i in range(0,botNum) :
		vworld[i] = NPCWorld()
		vnet[i] = Net(cliMgrIp, myComtree, numg, subLimit, vworld[i], uName, debug)
		if not vnet[i].init(uName, password) :
			sys.stderr.write("cannot initialize net object\n")
			sys.exit(1)
		vnet[i].sendStatus()
	print "type Ctrl-C to terminate"


loadPrcFileData("", "parallax-mapping-samples 3")
loadPrcFileData("", "parallax-mapping-scale 0.1")
loadPrcFileData("", "window-type none")

SPEED = 0.5
run()  # start the panda taskMgr
