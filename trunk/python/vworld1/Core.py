import sys
import struct
from time import sleep, time
from random import randint
from threading import *
from Queue import Queue
from collections import deque
from Packet import *
from CtlPkt import *
from WallsWorld import *
from Substrate import *
from math import sin, cos

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

class Core(Thread) :
	""" Core of a simple Avatar.

	This class implements the main thread of a simple Avatar.
	"""
	def __init__(self,sub,myIp,myFadr,rtrFadr,comtCtlFadr,comtree,finTime,world):
		""" Initialize a new Forwarder object.

		sub is a reference to a Substrate thread
		myFadr is the forest address of this Avatar
		ComtCtlFadr is the forest address of the comtree controller
		world is a WallsWorld object
		"""

		Thread.__init__(self)
		self.sub = sub
		self.myIp = myIp
		self.myFadr = myFadr
		self.rtrFadr = rtrFadr
		self.comtCtlFadr = comtCtlFadr
		self.comtree = comtree
		self.finTime = finTime
		self.world = world

		self.mySubs = set()
		self.nearAvatars = set()
		self.visibleAvatars = set()
		self.visSet = set()

		self.numNear = self.numVisible = 0

		self.seqNum = 1

	def run(self) :
		""" This is the main thread for the Core object.
		"""

		t0 = time(); self.now = 0; nextTime = 0

		sleep(.1)
		self.connect()
		sleep(.1)
		if not self.joinComtree() :
			sys.stderr.write("Core.run: cannot join comtree")
			return

		self.x = randint(0,GRID*self.world.size-1)
		self.y = randint(0,GRID*self.world.size-1)
		self.direction = randint(0,359)
		self.speed = STOPPED
		pos = (self.x/GRID) + (self.y/GRID)*self.world.size
		self.visSet = self.world.updateVisSet(pos,pos,self.visSet)
		print "visSet=", self.visSet

		self.quit = False

		while not self.quit :
			self.now = time() - t0

			# track nearby and visible avatars
			self.numNear = len(self.nearAvatars)
			self.nearAvatars.clear()
			self.numVisible = len(self.visibleAvatars)
			self.visibleAvatars.clear()

			while self.sub.incoming() :
				# substrate has an incoming packet
				p = self.sub.receive()
				self.updateNearby(p)

			self.updateStatus();	# update Avatar status
			self.sendStatus();	# send status on comtree
			self.updateSubs(); 	# update subscriptions

			nextTime += UPDATE_PERIOD
			delay = nextTime - (time()-t0)
			if delay > 0 : sleep(delay)
		self.leaveComtree()
		self.disconnect()

	def connect(self) :
		p = Packet(); p.type = CONNECT
		p.comtree = CLIENT_CON_COMT
		p.srcAdr = self.myFadr; p.dstAdr = self.rtrFadr
		self.sub.send(p)

	def disconnect(self) :
		p = Packet(); p.type = DISCONNECT
		p.comtree = CLIENT_CON_COMT
		p.srcAdr = self.myFadr; p.dstAdr = self.rtrFadr
		self.sub.send(p)

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
		cp.clientPort = self.sub.myAdr[1]
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
		If Substrate cannot accept packet, or
		no reply after three attempts, return None.
		"""
		sendTime = self.now
		numAttempts = 1
		if not self.sub.ready() : return None
		self.sub.send(p)

		while True :
			if self.sub.incoming() :
				p = self.sub.receive()
				return p
			elif self.now > sendTime + 1 :
				if numAttempts > 3 : return None
				sendTime = self.now
				numAttempts += 1
				if not self.sub.ready() : return None
				self.sub.send(p)
			else :
				sleep(.1)

	def stop(self) :
		""" Stop the run thread. """
		self.quit = True

	def sendStatus(self) :
		""" Send status packet on multicast group for current location.
		"""
		if self.comtree == 0 or not self.sub.ready() : return
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
		self.sub.send(p)

	def updateStatus(self) :
		""" Update status of avatar based on passage of time.
		"""
		PI = 3.141519625
	
		# update position
		dist = self.speed + 0.0
		dirRad = self.direction * (2*PI/360)

		prevRegion = self.groupNum(self.x,self.y)-1

		x = self.x + int(dist * sin(dirRad))
		y = self.y + int(dist * cos(dirRad))
		x = max(x,0); x = min(x,GRID*self.world.size-1)
		y = max(y,0); y = min(y,GRID*self.world.size-1)

		postRegion = self.groupNum(x,y)-1

		# stop if we hit a wall
		if x == 0 or x == GRID*self.world.size-1 or \
		   y == 0 or y == GRID*self.world.size-1 or \
		   self.world.separated(prevRegion,postRegion) :
			self.speed = STOPPED
		else :
			self.x = x; self.y = y
			if postRegion != prevRegion :
				self.visSet = self.world.updateVisSet( \
					prevRegion,postRegion,self.visSet)
	
	def groupNum(self, x1, y1) :
		""" Return multicast group number for given position.
		 x1 is the x coordinate of the position of interest
		 y1 is the y coordinate of the position of interest
		 """
		return 1 + (x1/GRID) + (y1/GRID)*self.world.size
	
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
				self.sub.send(p)
				p = Packet()
				nsub = 1
			struct.pack_into('!i',buf,4*nsub,-g)

		struct.pack_into('!I',buf,0,nsub)
		struct.pack_into('!I',buf,4*(nsub+1),0)
		p.length = OVERHEAD + 4*(2+nsub); p.type = SUB_UNSUB
		p.payload = str(buf[0:4*(2+nsub)])
		p.comtree = self.comtree;
		p.srcAdr = self.myFadr; p.dstAdr = self.rtrFadr
		self.sub.send(p)
	
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
				self.sub.send(p)
				p = Packet()
				nunsub = 1
			struct.pack_into('!i',buf,4*(nunsub+1),-g)

		struct.pack_into('!II',buf,0,0,nunsub)
		p.payload = str(buf)
		p.length = OVERHEAD + 4*(2+nunsub); p.type = SUB_UNSUB
		p.payload = str(buf[0:4*(2+nunsub)])
		p.comtree = self.comtree;
		p.srcAdr = self.myFadr; p.dstAdr = self.rtrFadr
		self.sub.send(p)
	
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
		""" Update the set of nearby Avatars.

		If the given packet is a status report, check to see if the
		sending avatar is visible. If it is visible, but not in our
		set of nearby avatars, then add it. If it is not visible
		but is in our set of nearby avatars, then delete it.
		Note: we assume that the visibility range and avatar speeds
		are such that we will get at least one report from a newly
		invisible avatar.
		"""
		tuple = struct.unpack('!IIII',p.payload[0:16])

		if tuple[0] != STATUS_REPORT : return
		x1 = tuple[2]; y1 = tuple[3]
		avId = p.srcAdr
		if len(self.nearAvatars) < MAXNEAR :
			self.nearAvatars.add(p.srcAdr)

		# determine if we can see the Avatar that sent this report
		g1 = self.groupNum(x1,y1)
		if g1-1 not in self.visSet :
			self.visibleAvatars.discard(avId)
			return
		
		if len(self.visibleAvatars) >= MAXNEAR : return
		p = ((self.x+0.0)/GRID,(self.y+0.0)/GRID)
		p1 = ((x1+0.0)/GRID,(y1+0.0)/GRID)
		if self.world.canSee(p,p1) : self.visibleAvatars.add(avId)
