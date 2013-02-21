import sys
import struct
from time import sleep, time
from random import randint, random
from threading import *
from Queue import Queue
from collections import deque
from Packet import *
from CtlPkt import *
from WorldMap import *
#from Substrate import *
from math import sin, cos
from direct.task import Task

UPDATE_PERIOD = .05 	# number of seconds between status updates
STATUS_REPORT = 1 	# code for status report packets
NUM_ITEMS = 10		# number of items in status report
GRID = 10000  		# xy extent of one grid square
MAXNEAR = 1000		# max # of nearby avatars

# speeds below are the amount avatar moves during an update period
# making them all equal to avoid funky animation
STOPPED =   0		# not moving
SLOW    = 50    	# slow avatar speed
MEDIUM  = 50    	# medium avatar speed
FAST    = 50    	# fast avatar speed

class Net :
	""" Net support.

	"""
	def __init__(self, myIp, cliMgrIp, comtree, map, pWorld, debug, auto):
		""" Initialize a new Net object.

		myIp address is IP address of this host
		comtree is the number of the comtree to use
		map is a WorldMap object
		debug is an integer >=0 that determines amount of
		debugging output
		"""

		self.myIp = myIp
		self.cliMgrIp = cliMgrIp
		self.comtree = comtree
		self.map = map
		self.pWorld = pWorld
		self.debug = debug
		self.auto = auto

		# open and configure socket to be nonblocking
		self.sock = socket(AF_INET, SOCK_DGRAM);
		self.sock.bind((myIp,0))
		self.sock.setblocking(0)
		self.myAdr = self.sock.getsockname()

		# set initial position
		if self.auto :
			self.x = 58*GRID/5; self.y = 67*GRID/5
			self.direction = randint(0,359)
		else :
			# compute scale factor for pWorld
			self.limit = pWorld.getLimit()
			self.scale = (GRID*self.map.size + 0.0) \
					/ (self.limit + 0.0)
			# set initial position
			x, y, self.direction = self.pWorld.getPosHeading()
			self.x = int(x*self.scale); self.y = int(y*self.scale)
			# correct panda headings to match mapWorld (yuck)
			self.direction = self.redirect(self.direction)
		self.speed = SLOW; self.deltaDir = 0 

		self.mySubs = set()	# multicast subscriptions
		self.nearRemotes = {}	# map: fadr -> [x,y,dir,dx,dy,count]
		self.visSet = set()	# set of visible squares
		loc = self.groupNum(self.x,self.y) - 1
		self.visSet = self.map.computeVisSet(loc)

		self.seqNum = 1		# sequence # for control packets

	def init(self, uname, pword) :
		if not self.login(uname, pword) : return False
		self.rtrAdr = (ip2string(self.rtrIp),ROUTER_PORT)
		self.t0 = time(); self.now = 0; self.nextTime = 0;
		self.connect()
		sleep(.1)
		if not self.joinComtree() :
			sys.stderr.write("Net.run: cannot join comtree\n")
			return False
		self.updateSubs()
		taskMgr.add(self.run, "netTask", uponDeath=self.wrapup, \
				appendTask=True)
		return True

	def login(self, uname, pword) :
		cmSock = socket(AF_INET, SOCK_STREAM);
		cmSock.connect((self.cliMgrIp,30140))
	
		s = uname + " " + pword + " " + str(self.myAdr[1]) + \
		    " 0 noproxy"
		blen = 4+len(s)
		buf = struct.pack("!I" + str(blen) + "s",blen,s);
		cmSock.sendall(buf)
		
		buf = cmSock.recv(4)
		while len(buf) < 4 : buf += cmSock.recv(4-len(buf))
		tuple = struct.unpack("!I",buf)
		if tuple[0] == -1 :
			sys.stderr.write("Avatar.login: negative reply from " \
					 "client manager");
			return False
	
		self.rtrFadr = tuple[0]
	
		buf = cmSock.recv(12)
		while len(buf) < 12 : buf += cmSock.recv(12-len(buf))
		tuple = struct.unpack("!III",buf)
	
		self.myFadr = tuple[0]
		self.rtrIp = tuple[1]
		self.comtCtlFadr = tuple[2]
	
		return True

	def redirect(self, direction) :
		""" Convert between panda headings and MapWorld directions.

		MapWorld uses compass headings, so positive y-axis points
		north (compass heading 0) and positive x-axis points east
		(compass heading 90). Panda's 0 heading is the negative
		y-axis and directions change in opposite directions (ugh).
		This method transforms between the two. Note, it symmetric,
		so you can use it to transform headings from panda to MapWorld
		or from MapWorld to panda.
		"""
		return (180 - direction)%360

	def run(self, task) :
		""" This is the main method for the Net object.
		"""

		self.now = time() - self.t0

		# process once per UPDATE_PERIOD
		if self.now < self.nextTime : return task.cont
		self.nextTime += UPDATE_PERIOD

		while True :
			# substrate has an incoming packet
			p = self.receive()
			if p == None : break
			self.updateNearby(p) # update map of nearby remotes

		self.pruneNearby()	# check for remotes that are MIA
		self.updateStatus()	# update Avatar status
		self.sendStatus()	# send status on comtree

		return task.cont

	def wrapup(self, task) :
		self.leaveComtree()
		self.disconnect()

	def send(self, p) :
		""" Send a packet to the router.
		"""
		if self.debug >= 3 or \
		   (self.debug >= 2 and p.type != CLIENT_DATA) or \
		   (self.debug >= 1 and p.type == CLIENT_SIG) :
			print self.now, self.myAdr, "sending to", self.rtrAdr
			print p.toString()
			if p.type == CLIENT_SIG :
				cp = CtlPkt(0,0,0)
				cp.unpack(p.payload)
				print cp.toString()
			sys.stdout.flush()
		self.sock.sendto(p.pack(),self.rtrAdr)

	def receive(self) :
		""" Receive a packet from the router.

		If the socket has a packet available, it is unpacked
		and returned. Othereise, None is returned.
		"""
		try :
			buf, senderAdr = self.sock.recvfrom(2000)
		except error as e :
			if e.errno == 35 or e.errno == 11 : return None
			# 11, 35 both imply no data to read
			# treat anything else as fatal error
			sys.stderr.write("Substrate: socket " \
					 + "error: " + str(e))
			sys.exit(1)
			
		# got a packet
		if senderAdr != self.rtrAdr :
			sys.stderr.write("Substrate: got an " \
				+ "unexpected packet from " \
				+ str(senderAdr))
			sys.exit(1)
		p = Packet()
		p.unpack(buf)
		if self.debug >= 3 or \
		   (self.debug >= 2 and p.type != CLIENT_DATA) or \
		   (self.debug >= 1 and p.type == CLIENT_SIG) :
			print self.now, self.myAdr, \
			      "received from", self.rtrAdr
			print p.toString()
			if p.type == CLIENT_SIG :
				cp = CtlPkt(0,0,0)
				cp.unpack(p.payload)
				print cp.toString()
			sys.stdout.flush()
		return p

	def connect(self) :
		p = Packet(); p.type = CONNECT
		p.comtree = CLIENT_CON_COMT
		p.srcAdr = self.myFadr; p.dstAdr = self.rtrFadr
		self.send(p)

	def disconnect(self) :
		p = Packet(); p.type = DISCONNECT
		p.comtree = CLIENT_CON_COMT
		p.srcAdr = self.myFadr; p.dstAdr = self.rtrFadr
		self.send(p)

	def joinComtree(self) :
		return self.sendJoinLeave(CLIENT_JOIN_COMTREE)

	def leaveComtree(self) :
		return self.sendJoinLeave(CLIENT_LEAVE_COMTREE)

	def sendJoinLeave(self,which) :
		p = Packet()
		p.type = CLIENT_SIG; p.comtree = CLIENT_SIG_COMT
		p.srcAdr = self.myFadr; p.dstAdr = self.comtCtlFadr

		cp = CtlPkt(which,RR_REQUEST,self.seqNum)
		cp.comtree = self.comtree
		cp.clientIp = string2ip(self.myIp)
		cp.clientPort = self.myAdr[1]
		p.payload = cp.pack()
		self.seqNum += 1

		reply = self.sendCtlPkt(p)
		if reply == None : return False
		cpReply = CtlPkt(0,0,0)
		cpReply.unpack(reply.payload)

		return	cpReply.seqNum == cp.seqNum and \
			cpReply.cpTyp  == cp.cpTyp and \
			cpReply.rrTyp  == RR_POS_REPLY

	def sendCtlPkt(self,p) :
		""" Send a control packet and wait for a response.
	
		If a reply is received, unpack the reply packet and return it.
		If no reply within one second, try again.
		If no reply after three attempts, return None.
		"""
		now = sendTime = time()
		numAttempts = 1
		self.send(p)

		while True :
			reply = self.receive()
			if reply != None :
				return reply
			elif now > sendTime + 1 :
				if numAttempts > 3 : return None
				sendTime = now
				numAttempts += 1
				self.send(p)
			else :
				sleep(.1)
			now = time()

	def sendStatus(self) :
		""" Send status packet on multicast group for current location.
		"""
		if self.comtree == 0 : return
		p = Packet()
		p.leng = OVERHEAD + 4*8; p.type = CLIENT_DATA; p.flags = 0
		p.comtree = self.comtree
		p.srcAdr = self.myFadr;
		p.dstAdr = -self.groupNum(self.x,self.y)
	
		numNear = len(self.nearRemotes)
		if numNear > 5000 or numNear < 0 :
			print "numNear=", numNear
		p.payload = struct.pack('!IIIIIIII', \
					STATUS_REPORT, self.now, \
					self.x, self.y, \
					self.direction, self.speed, \
					numNear, numNear)
		self.send(p)

	def updateStatus(self) :
		""" Update position and heading of avatar.

		When a player is controlling the avatar, just update
		local state from the panda model. Otherwise, wander
		randomly, avoiding walls and non-walkable squares.
		"""
		loc = self.groupNum(self.x,self.y)-1

		if not self.auto : # in this case, panda controls position
			x, y, self.direction = self.pWorld.getPosHeading()
			self.x = int(x*self.scale); self.y = int(y*self.scale)
			self.direction = self.redirect(self.direction)

			nuloc = self.groupNum(self.x,self.y)-1
			if nuloc != loc :
				self.visSet = self.map.computeVisSet(nuloc)
				self.updateSubs()
			return

		# code for auto operation
		atLeft  = (loc%self.map.size == 0)
		atRight = (loc%self.map.size == self.map.size-1)
		atBot   = (loc/self.map.size == 0)
		atTop   = (loc/self.map.size == self.map.size-1)
		xd = self.x%GRID; yd = self.y%GRID
		if xd < .4*GRID and \
		   (atLeft or self.map.separated(loc,loc-1) or \
		    self.map.blocked(loc-1)) :
			# near left edge of loc
			if 160 < self.direction and self.direction < 270 :
				 self.direction -= 1
			if 270 <= self.direction or self.direction < 20 :
				 self.direction += 1
			self.speed = SLOW; self.deltaDir = 0
		elif xd > .6*GRID and \
		   (atRight or self.map.separated(loc,loc+1) or \
		    self.map.blocked(loc+1)) :
			# near right edge of loc
			if 340 < self.direction or self.direction <= 90 :
				 self.direction -= 1
			if 90 < self.direction and self.direction < 200 :
				 self.direction += 1
			self.speed = SLOW; self.deltaDir = 0
		elif yd < .4*GRID and \
		   (atBot or self.map.separated(loc,loc-self.map.size) or \
		    self.map.blocked(loc-self.map.size)) :
			# near bottom edge of loc
			if 70 < self.direction and self.direction <= 180 :
				 self.direction -= 1
			if 180 < self.direction and self.direction < 290 :
				 self.direction += 1
			self.speed = SLOW; self.deltaDir = 0
		elif yd > .6*GRID and \
		   (atTop or self.map.separated(loc,loc+self.map.size) or \
		    self.map.blocked(loc+self.map.size)) :
			# near top edge of loc
			if 250 < self.direction and self.direction <= 359 :
				self.direction -= 1
			if 0 <= self.direction and self.direction < 110 :
				 self.direction += 1
			self.speed = SLOW; self.deltaDir = 0
		else :
			# no walls to avoid, so just make random
			# changes to direction and speed
			self.direction += self.deltaDir;
			r = random()
			if r < 0.1 :
				if r < .05 : self.deltaDir -= 0.2 * random()
				else :       self.deltaDir += 0.2 * random()
				self.deltaDir = min(1.0,self.deltaDir)
				self.deltaDir = max(-1.0,self.deltaDir)
			# update speed
			r = random()
			if r <= 0.1 :
				if self.speed == SLOW or self.speed == FAST :
					self.speed = MEDIUM
				elif r < 0.05 :
					self.speed = SLOW
				else :
					self.speed = FAST
		# update position using adjusted direction and speed
		self.direction %= 360
		dist = self.speed
		PI = 3.141519625
		dirRad = self.direction * (2*PI/360)
		self.x += (int) (dist * sin(dirRad))
		self.y += (int) (dist * cos(dirRad))
		nuloc = self.groupNum(self.x,self.y)-1
		if nuloc != loc :
			self.visSet = self.map.computeVisSet(nuloc)
			self.updateSubs()
	
	def groupNum(self, x1, y1) :
		""" Return multicast group number for given position.

		x1 is the x coordinate of the position of interest
		y1 is the y coordinate of the position of interest
		Note: multicast group numbers are 1 larger than the
		index of the MapWorld square that contains (x1,y1).
		To convert group number to a Forest multicast address,
		just negate it.
		"""
		return 1 + (int(x1)/GRID) + (int(y1)/GRID)*self.map.size
	
	def subscribe(self,glist) :
		""" Subscribe to a list of multicast groups.
		"""
		if self.comtree == 0 or len(glist) == 0 : return
		p = Packet()
		nsub = 0
		buf = bytearray(2048)
		for g in glist :
			nsub += 1
			if nsub > 350 :
				struct.pack_into('!I',buf,0,nsub-1)
				struct.pack_into('!I',buf,4*(nsub),0)
				p.length = OVERHEAD + 4*(1+nsub)
				p.type = SUB_UNSUB
				p.comtree = self.comtree
				p.srcAdr = self.myFadr; p.dstAdr = self.rtrFadr
				self.send(p)
				p = Packet()
				nsub = 1
			struct.pack_into('!i',buf,4*nsub,-g)

		struct.pack_into('!I',buf,0,nsub)
		struct.pack_into('!I',buf,4*(nsub+1),0)
		p.length = OVERHEAD + 4*(2+nsub); p.type = SUB_UNSUB
		p.payload = str(buf[0:4*(2+nsub)])
		p.comtree = self.comtree;
		p.srcAdr = self.myFadr; p.dstAdr = self.rtrFadr
		self.send(p)
	
	def unsubscribe(self,glist) :
		""" Unsubscribe from a list of multicast groups.
		"""
		if self.comtree == 0 or len(glist) == 0 : return
		p = Packet()
		nunsub = 0
		buf = bytearray(2048)
		for g in glist :
			nunsub += 1
			if nunsub > 350 :
				struct.pack_into('!II',buf,0,0,nunsub-1)
				p.length = OVERHEAD + 4*(1+nunsub)
				p.type = SUB_UNSUB
				p.comtree = self.comtree
				p.srcAdr = self.myFadr; p.dstAdr = self.rtrFadr
				self.send(p)
				p = Packet()
				nunsub = 1
			struct.pack_into('!i',buf,4*(nunsub+1),-g)

		struct.pack_into('!II',buf,0,0,nunsub)
		p.payload = str(buf)
		p.length = OVERHEAD + 4*(2+nunsub); p.type = SUB_UNSUB
		p.payload = str(buf[0:4*(2+nunsub)])
		p.comtree = self.comtree;
		p.srcAdr = self.myFadr; p.dstAdr = self.rtrFadr
		self.send(p)
	
	def subscribeAll(self) :
		""" Subscribe to all multicasts for regions that are visible.
		"""
		glist = []
		for c in self.visSet :
			g = c+1
			if g not in self.mySubs :
				self.mySubs.add(g); glist += [g]
		self.subscribe(glist)
	
	def unsubscribeAll(self) :
		""" Unsubscribe from all multicasts. """
		self.unsubscribe(list(self.mySubs)); self.mySubs.clear()

	def updateSubs(self) :	
		""" Update subscriptions.
		
		Unsubscribe from multicasts for cells that are no
		longer visible.
		Subscribe to multicasts for cells that are visible
		"""
		glist = []
		for g in self.mySubs :
			if g-1 not in self.visSet : glist += [g]
		self.mySubs -= set(glist); self.unsubscribe(glist)
		self.subscribeAll()
	
	def updateNearby(self, p) :
		""" Process a packet from a remote.

		Currently, this just updates the map of remotes.
		Could be extended to change the state of this avatar
		(say, process a "kill packet")
		"""
		tuple = struct.unpack('!IIIII',p.payload[0:20])

		if tuple[0] != STATUS_REPORT : return
		x1 = tuple[2]; y1 = tuple[3]; dir1 = tuple[4]
		avId = p.srcAdr

		if avId in self.nearRemotes :
			# update map entry for this remote
			remote = self.nearRemotes[avId]
			remote[3] = x1 - remote[0]; remote[4] = y1 - remote[1]
			remote[0] = x1; remote[1] = y1; remote[2] = dir1
			remote[5] = 0
			dir1 = self.redirect(dir1)
			if not self.auto :
				self.pWorld.updateRemote(x1/self.scale, \
					y1/self.scale, dir1, avId)
		elif len(self.nearRemotes) < MAXNEAR :
			# fadr -> x,y,direction,dx,dy,count
			self.nearRemotes[avId] = [x1,y1,dir1,0,0,0]
			dir1 = self.redirect(dir1)
			if not self.auto :
				self.pWorld.addRemote(x1/self.scale, \
					y1/self.scale, dir1, avId)
	
	def pruneNearby(self) :
		""" Remove old entries from nearRemotes

		If we haven't heard from a remote in a while, we remove
		it from the map of nearby remotes. When a remote first
		"goes missing" we continue to update its position,
		by extrapolating from last update.
		"""
		dropList = []
		for avId, remote in self.nearRemotes.iteritems() :
			if remote[0] > 0 : # implies missing update
				# update position based on dx, dy
				remote[0] += remote[3]; remote[1] += remote[4]
				remote[0] = max(0,remote[0])
				remote[0] = min(self.map.size*GRID-1,remote[0])
				remote[1] = max(0,remote[1])
				remote[1] = min(self.map.size*GRID-1,remote[1])
			if remote[5] >= 6 :
				dropList.append(avId)
			remote[5] += 1
		for avId in dropList :
			rem = self.nearRemotes[avId]
			x = rem[0]; y = rem[1]
			del self.nearRemotes[avId]
			if not self.auto : self.pWorld.removeRemote(avId)
