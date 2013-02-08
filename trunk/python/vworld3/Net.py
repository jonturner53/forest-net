import sys
import struct
from time import sleep, time
from random import randint
from threading import *
from Queue import Queue
from collections import deque
from Packet import *
from CtlPkt import *
from WorldMap import *
from Substrate import *
from math import sin, cos
from direct.task import Task

UPDATE_PERIOD = .05 	# number of seconds between status updates
STATUS_REPORT = 1 	# code for status report packets
NUM_ITEMS = 10		# number of items in status report
GRID = 10000  		# xy extent of one grid square
MAXNEAR = 1000		# max # of nearby avatars

# speeds below are the amount avatar moves during an update period
STOPPED =   0		# not moving
SLOW    = 100    	# slow avatar speed
MEDIUM  = 250    	# medium avatar speed
FAST    = 600    	# fast avatar speed

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

		self.mySubs = set()
		self.nearRemotes = set()
		self.visibleRemotes = set()
		self.visSet = set()
		self.numNear = self.numVisible = 0

		# open and configure socket to be nonblocking
		self.sock = socket(AF_INET, SOCK_DGRAM);
		self.sock.bind((myIp,0))
		self.sock.setblocking(0)
		self.myAdr = self.sock.getsockname()

		self.limit = pWorld.getLimit()
		self.scale = (GRID*self.map.size + 0.0)/(self.limit + 0.0)

		x, y, self.direction = self.pWorld.getPosHeading()
		self.x = int(x*self.scale); self.y = int(y*self.scale)
		self.speed = SLOW  # only used for auto operation

		cell = (self.x/GRID) + (self.y/GRID)*self.map.size
		self.visSet = self.map.computeVisSet(cell)

		self.seqNum = 1

	def init(self, uname, pword) :
		if not self.login(uname, pword) : return False
		self.rtrAdr = (ip2string(self.rtrIp),ROUTER_PORT)
		self.t0 = time(); self.now = 0; 
		self.connect()
		sleep(.1)
		if not self.joinComtree() :
			sys.stderr.write("Net.run: cannot join comtree\n")
			return False
		taskMgr.add(self.run, "netTask",uponDeath=self.wrapup, \
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
	
		print	"avatar address:", fadr2string(self.myFadr), \
	       		" router address:", fadr2string(self.rtrFadr), \
	       		" comtree controller address:", \
				fadr2string(self.comtCtlFadr);
		return True

	def run(self, task) :
		""" This is the main method for the Net object.
		"""

		self.now = time() - self.t0
		if self.now < .5 :
			sys.stderr.write("Net.run at " + str(self.now) + "\n")

		# track nearby and visible avatars
		self.numNear = len(self.nearRemotes)
		self.nearRemotes.clear()
		self.numVisible = len(self.visibleRemotes)
		self.visibleRemotes.clear()

		while True :
			# substrate has an incoming packet
			p = self.receive()
			if p == None : break
			self.updateNearby(p)

		self.updateStatus()	# update Avatar status
		self.updateSubs() 	# update subscriptions

		# send status on comtree
		self.sendStatus()
		return task.cont

	def wrapup(self, task) :
		sys.stderr.write("running Net.wrapup()")
		self.leaveComtree()
		self.disconnect()



	def send(self, p) :
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
		if self.debug>=3 or \
		   (self.debug>=2 and p.type!=CLIENT_DATA) or \
		   (self.debug>=1 and p.type==CLIENT_SIG) :
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
			if reply != None : return reply
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
	
		p.payload = struct.pack('!IIIIIIII', \
					STATUS_REPORT, self.now, \
					self.x, self.y, \
					self.direction, self.speed, \
					self.numVisible, self.numNear)
		self.send(p)

	def updateStatus(self) :
		""" Update position and heading of avatar.

		"""
		prevRegion = self.groupNum(self.x,self.y)-1

		x, y, self.direction = self.pWorld.getPosHeading()
		self.x = int(x*self.scale); self.y = int(y*self.scale)

		postRegion = self.groupNum(self.x,self.y)-1
		if postRegion != prevRegion :
			self.visSet = self.map.updateVisSet( \
				prevRegion,postRegion,self.visSet)

		"""
		PI = 3.141519625
	
		# update position
		dist = self.speed + 0.0
		dirRad = self.direction * (2*PI/360)

		prevRegion = self.groupNum(self.x,self.y)-1

		x = self.x + int(dist * sin(dirRad))
		y = self.y + int(dist * cos(dirRad))
		x = max(x,0); x = min(x,GRID*self.map.size-1)
		y = max(y,0); y = min(y,GRID*self.map.size-1)

		postRegion = self.groupNum(x,y)-1

		# stop if we hit a wall
		if x == 0 or x == GRID*self.map.size-1 or \
		   y == 0 or y == GRID*self.map.size-1 or \
		   (prevRegion != postRegion and
		    self.map.separated(prevRegion,postRegion)) :
			self.speed = STOPPED
		else :
			self.x = x; self.y = y
			if postRegion != prevRegion :
				self.visSet = self.map.updateVisSet( \
					prevRegion,postRegion,self.visSet)
		"""
	
	def groupNum(self, x1, y1) :
		""" Return multicast group number for given position.
		 x1 is the x coordinate of the position of interest
		 y1 is the y coordinate of the position of interest
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
				p.srcAdr = self.myFadr; p.dstAdr = rtrFadr
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
				p.srcAdr = self.myFadr; p.dstAdr = rtrFadr
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
		""" Update the set of nearby Remotes.

		If the given packet is a status report, check to see if the
		sending avatar is visible. If it is visible, but not in our
		set of nearby avatars, then add it. If it is not visible
		but is in our set of nearby avatars, then delete it.
		Note: we assume that the visibility range and avatar speeds
		are such that we will get at least one report from a newly
		invisible avatar.
		"""
		tuple = struct.unpack('!IIIII',p.payload[0:20])

		if tuple[0] != STATUS_REPORT : return
		x1 = tuple[2]; y1 = tuple[3]; dir1 = tuple[4]
		avId = p.srcAdr

		if avId not in self.nearRemotes and \
		   len(self.nearRemotes) < MAXNEAR :
			self.nearRemotes.add(avId)
			self.pWorld.addRemote(x1/self.scale, y1/self.scale, \
					      dir1, avId)
		else :
			self.pWorld.updateRemote(x1/self.scale, y1/self.scale, \
					      	 dir1, avId)

		# determine if we can see the Avatar that sent this report
		g1 = self.groupNum(x1,y1)
		if g1-1 not in self.visSet :
			self.visibleRemotes.discard(avId)
			return
		
		if len(self.visibleRemotes) >= MAXNEAR : return
		p = ((self.x+0.0)/GRID,(self.y+0.0)/GRID)
		p1 = ((x1+0.0)/GRID,(y1+0.0)/GRID)
		if self.map.canSee(p,p1) : self.visibleRemotes.add(avId)
