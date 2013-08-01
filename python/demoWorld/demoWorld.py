""" Demonstration of simple virtual world using Forest overlay

usage:
      demoWorld cliMgr numg subLimit comtree userName avaModel
		[ audio ] [ debug ] [ auto ]

- cliMgrIp is the host name or IP address of the client manager's server
- numg*numg is the number of multicast groups to use
- subLimit limits the number of multicast groups that we subscribe to;
  we'll subscribe to a group is it's L_0 distance is at most subLimit
- comtree is the number of a pre-configured Forest comtree
- userName is your forest user name; if you specify user, you will be
  connected using the demo account called "user"; for any other choice,
  you will be prompted for a password
- avaModel is the serial number of the 3d-model used for this avatar
- the audio option, if string "audio" is present, sending/receiving audio
  is enabled; to use this option, you must have pyaudio and portaudio installed
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
import random, sys, os, math

# process command line arguments
if len(sys.argv) < 5 :
	sys.stderr.write("usage: DemoWorld cliMgr numg subLimit comtree " + \
			 "userName avaModel [ audio ] [ debug ] [ auto ]")
        sys.exit(1)

cliMgrIp = gethostbyname(sys.argv[1])
numg = int(sys.argv[2])
subLimit = int(sys.argv[3])
myComtree = int(sys.argv[4])
userName = sys.argv[5]
avaModel = int(sys.argv[6])

AUDIO = False; auto = False; debug = 0
for i in range(7,len(sys.argv)) :
	if sys.argv[i] == "audio" : AUDIO = True
	elif sys.argv[i] == "debug"   : debug = 1
	elif sys.argv[i] == "debugg"  : debug = 2
	elif sys.argv[i] == "debuggg" : debug = 3
	elif sys.argv[i] == "auto" : auto = True

password = "pass"
if user != "user" :
	password = getpass.getpass()

myAvatar = Bot() if auto else Avatar(userName, avaModel)
net = Net(cliMgrIp, myComtree, numg, subLimit, userName, avaModel,
	  myAvatar, debug)

if auto : print "type Ctrl-C to terminate"

# setup tasks
if not net.init(user, password) :
	sys.stderr.write("cannot initialize net object\n");
	sys.exit(1)

loadPrcFileData("", "parallax-mapping-samples 3")
loadPrcFileData("", "parallax-mapping-scale 0.1")
loadPrcFileData("", "window-type none")

SPEED = 0.5
run()  # start the panda taskMgr
