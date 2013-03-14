""" Demonstration of simple virtual world using Forest overlay

usage:
      Avatar myIp cliMgrIp mapFile comtree [ debug ] [ auto tlim ]

- myIp is the IP address of the user's computer
- cliMgrIp is the IP address of the client manager's computer
- mapFile is a text file that defines which parts of the world are
  walkable and which are visible from each other
- comtree is the number of a pre-configured Forest comtree
- the debug option, if present controls the amount of debugging output;
  use "debug" for a little debugging output, "debuggg" for lots
- the auto option, if present, starts an avatar that wanders aimlessly
  it requires an integer time limit, which specifies how long the
  avatar should run (in seconds); if tlim=0, the avatar runs forever
"""

import sys
from Net import *
from WorldMap import *

# process command line arguments
if len(sys.argv) < 5 :
	sys.stderr.write("usage: Avatar myIp cliMgrIp mapFile " + \
			 "comtree [ debug ] [ auto tlim ]\n")
        sys.exit(1)

myIp = gethostbyname(sys.argv[1])
cliMgrIp = gethostbyname(sys.argv[2])
mapFile = sys.argv[3]
myComtree = int(sys.argv[4])

auto = False; debug = 0
for i in range(5,len(sys.argv)) :
	if sys.argv[i] == "debug" : debug = 1
	elif sys.argv[i] == "debugg" : debug = 2
	elif sys.argv[i] == "debuggg" : debug = 3
	elif sys.argv[i] == "auto" :
		auto = True
		tlim = int(sys.argv[i+1])

map = WorldMap() 
if not map.init(mapFile) :
	sys.stderr.write("cannot initialize map from mapFile\n");
	sys.exit(1)

net = Net(myIp, cliMgrIp, myComtree, map, debug, auto)

# setup net thread and start it running
if not net.init("user", "pass") :
	sys.stderr.write("cannot initialize net object\n");
	sys.exit(1)
try : net.start()
except : sys.exit(1)

t0 = time()
while True :
	sleep(.1)
	if auto :
		if time() < t0 + tlim : continue
		else : break

        sys.stdout.write(": ")
        line = sys.stdin.readline()
        if line[0] == "q" : break;
        elif line[0] == "a" :
                if net.speed == STOPPED : net.direction -= 30
                else : net.direction -= 10
		print net.x, net.y, net.direction, net.speed
        elif line[0] == "d" :
                if net.speed == STOPPED : net.direction += 30
                else : net.direction += 10
		print net.x, net.y, net.direction, net.speed
        elif line[0] == "s" :
                if net.speed == SLOW : net.speed = STOPPED
                elif net.speed == MEDIUM : net.speed = SLOW
                elif net.speed == FAST : net.speed = MEDIUM
		print net.x, net.y, net.direction, net.speed
        elif line[0] == "w" : 
                if net.speed == STOPPED : net.speed = SLOW
                elif net.speed == SLOW : net.speed = MEDIUM
                elif net.speed == MEDIUM : net.speed = FAST
		print net.x, net.y, net.direction, net.speed
        if net.direction < 0 : net.direction += 360

net.stop(); net.join()
sys.exit(0)
