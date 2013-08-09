""" Demonstration of simple virtual world using Forest overlay

usage:
      demoWorld cliMgr comtree userName avaModel
		[ auto ] [ show ] [ audio ] [ debug ]

- cliMgrIp is the host name or IP address of the client manager's server
- comtree is the number of a pre-configured Forest comtree
- userName is your forest user name; if you specify user, you will be
  connected using the demo account called "user"; for any other choice,
  you will be prompted for a password
- avaModel is the serial number of the 3d-model used for this avatar
- the auto option, if present, starts an avatar that wanders aimlessly
- the show option, if present, opens a window showing the world from
  the avatar's viewpoint; for auto avatars, this is required to see what
  the avatar sees; for manually controlled avatars, a window is always opened
- the audio option, if present, sending/receiving audio
  is enabled; to use this option, you must have pyaudio and portaudio installed;
  this is always disabled for auto avatars
- the debug option, if present controls the amount of debugging output;
  use "debug" for a little debugging output, "debuggg" for lots
"""

import sys
from socket import *
import random, sys, os, math

import direct.directbase.DirectStart
from panda3d.core import *
from direct.gui.OnscreenText import OnscreenText
from direct.gui.OnscreenImage import OnscreenImage
from direct.actor.Actor import Actor
from direct.showbase.DirectObject import DirectObject
from direct.interval.IntervalGlobal import Sequence
from panda3d.core import Point3

import Util
from Util import *
from Net import *
from Avatar import *

# process command line arguments
if len(sys.argv) < 5 :
	sys.stderr.write("usage: DemoWorld cliMgr comtree userName avaModel" + \
			 " [ auto ] [ show ] [ audio ] [ debug ]")
        sys.exit(1)

cliMgrIp = gethostbyname(sys.argv[1])
myComtree = int(sys.argv[2])
userName = sys.argv[3]
avaModel = int(sys.argv[4])

Util.AUTO = False; Util.SHOW = False; Util.AUDIO = False; Util.DEBUG = 0
for i in range(5,len(sys.argv)) :
	if   sys.argv[i] == "auto" : Util.AUTO = True
	elif sys.argv[i] == "show" : Util.SHOW = True
	elif sys.argv[i] == "audio" : Util.AUDIO = True
	elif sys.argv[i] == "debug"   : Util.DEBUG = 1
	elif sys.argv[i] == "debugg"  : Util.DEBUG = 2
	elif sys.argv[i] == "debuggg" : Util.DEBUG = 3
if not Util.AUTO : Util.SHOW = True
if Util.AUTO : Util.AUDIO = False

password = "pass"
if userName != "user" :
	password = getpass.getpass()

print "DEBUG=", Util.DEBUG
myAvatar = Avatar(userName, avaModel)
net = Net(cliMgrIp, myComtree, userName, avaModel, myAvatar)

if Util.AUTO : print "type Ctrl-C to terminate"

# setup tasks
if not net.init(userName, password) :
	sys.stderr.write("cannot initialize net object\n");
	sys.exit(1)

loadPrcFileData("", "parallax-mapping-samples 3")
loadPrcFileData("", "parallax-mapping-scale 0.1")
loadPrcFileData("", "window-type none")

SPEED = 0.5
run()  # start the panda taskMgr
