import sys
import struct
from time import sleep, time
from random import randint, random, uniform
from math import sin, cos
from threading import *

from Packet import *
from CtlPkt import *
from WorldMap import *

UPDATE_PERIOD = .05 	# number of seconds between status updates
STATUS_REPORT = 1 	# code for status report packets
GRID = 10000  		# xy extent of one grid square
MAXNEAR = 1000		# max # of nearby avatars

# speeds below are the amount avatar moves during an update period
# making them all equal to avoid funky animation
STOPPED =   0		# not moving
SLOW    = .01    	# slow avatar speed
MEDIUM  = .02    	# medium avatar speed
FAST    = .05    	# fast avatar speed

class Net(Thread) :
	""" Net support.

	"""
	def __init__(self, cliMgrIp, comtree, map, debug, auto):
		""" Initialize a new Net object.

		comtree is the number of the comtree to use
		map is a WorldMap object
		debug is an integer >=0 that determines amount of
		debugging output
		"""
		Thread.__init__(self)

		self.cliMgrIp = cliMgrIp
		self.comtree = comtree
		self.map = map
		self.debug = debug
		self.auto = auto

		# open and configure socket to be nonblocking
		self.sock = socket(AF_INET, SOCK_DGRAM);
		self.sock.setblocking(0)
		self.myAdr = self.sock.getsockname()

		# set initial position
		if self.auto :
			self.x = uniform(2,map.size-2)
			self.y = uniform(2,map.size-2)
			self.direction = randint(0,359)
			self.speed = SLOW; self.deltaDir = 0 
		else :
			self.x = 2; self.y = 2
                        self.direction = 180; self.deltaDir = 0
                        self.speed = STOPPED

		self.mySubs = set()	# multicast subscriptions
		self.visSet = set()	# set of visible squares
		loc = self.groupNum(self.x,self.y) - 1
		self.visSet = self.map.computeVisSet(loc)

		self.nearRemotes = {} # map fadr=>x,y,direction
		self.visRemotes = {}    # map fadr=>x,y,direction
		self.numNear = 0	# number of nearby remotes
		self.numVis = 0		# number of visible remotes

		self.seqNum = 1		# sequence # for control packets
		self.subSeqNum = 1	# sequence # for subscriptions

		self.quit = False

	def init(self, uname, pword) :
		if not self.login(uname, pword) : return False
		self.rtrAdr = (ip2string(self.rtrIp),self.rtrPort)
		self.t0 = time(); self.now = 0; self.nextTime = 0;
		self.connect()
		sleep(.1)
		if not self.joinComtree() :
			sys.stderr.write("Net.run: cannot join comtree\n")
			return False
		self.updateSubs()
		return True

	def readLine(self,sock,buf) :
		""" Return next line in a buffer, refilling buffer as necessary.
		sock is a blocking stream socket to read from
		buf is a buffer containing data read from socket but
		not yet returned as a distinct line.
		return the string up to (but not including) the next newline
		"""
		while True :
			pos = buf.find("\n") 
			if pos >= 0 :
				line = buf[0:pos]
				buf = buf[pos+1:]
				return line, buf
			nu = sock.recv(1000)
			buf += nu


	def login(self, uname, pword) :
		""" Login to connection manager and request session.

		uname is the user name to use when logging in
		pword is the password to use
		return True of the login dialog is successful, else False;
		the client manager returns several configuration parameters
		as part of the dialog
		"""
		print "connecting to client manager"
		cmSock = socket(AF_INET, SOCK_STREAM);
		cmSock.connect((self.cliMgrIp,30122))
		print "connected to client manager"
	
		cmSock.sendall("Forest-login-v1\nlogin: " + uname + \
                	       "\npassword: " + pword + "\nover\n")


		buf = ""
		print "reading line"
		line,buf = self.readLine(cmSock,buf)
		if line != "login successful" :
			return False
		line,buf = self.readLine(cmSock,buf)
		if line != "over" :
			return False

		cmSock.sendall("newSession\nover\n")

		line,buf = self.readLine(cmSock,buf)
		chunks = line.partition(":")
		if chunks[0].strip() != "yourAddress" or chunks[1] != ":" :
			return False
		self.myFadr = string2fadr(chunks[2].strip())

		line,buf = self.readLine(cmSock,buf)
		chunks = line.partition(":")
		if chunks[0].strip() != "yourRouter" or chunks[1] != ":" :
			return False
		triple = chunks[2].strip(); triple = triple[1:-1].strip()
		chunks = triple.split(",")
		if len(chunks) != 3 : return False
		self.rtrIp = string2ip(chunks[0].strip())
		self.rtrPort = int(chunks[1].strip())
		self.rtrFadr = string2fadr(chunks[2].strip())

		line,buf = self.readLine(cmSock,buf)
		chunks = line.partition(":")
		if chunks[0].strip() != "comtCtlAddress" or chunks[1] != ":" :
			return False
		self.comtCtlFadr = string2fadr(chunks[2].strip())

		line,buf = self.readLine(cmSock,buf)
		chunks = line.partition(":")
		if chunks[0].strip() != "connectNonce" or chunks[1] != ":" :
			return False
		self.nonce = int(chunks[2].strip())

		line,buf = self.readLine(cmSock,buf) 
		if line != "overAndOut" : return False

		print "login successful"

		cmSock.close()

		if self.debug >= 1 :
			print "avatar address =", fadr2string(self.myFadr)
			print "router info = (", ip2string(self.rtrIp), \
					     str(self.rtrPort), \
					     fadr2string(self.rtrFadr), ")"
			print "comtCtl address = ",fadr2string(self.comtCtlFadr)
			print "nonce = ", self.nonce

		return True

	def run(self) :
		""" This is the main method for the Net object.
		"""
		nextTime = time() + UPDATE_PERIOD
		while not self.quit :
			self.now = time() - self.t0

			self.numNear = len(self.nearRemotes)
			self.numVis = len(self.visRemotes)
			self.nearRemotes.clear()
			self.visRemotes.clear()

			p = self.receive()
			while p != None :
				# update maps of nearby/visible remotes
				# whenever we get a report packet
				self.updateNearby(p)
				p = self.receive()
	
			self.updateStatus()	# update Avatar status
			self.sendStatus()	# send status on comtree

			t = time()
			if t < nextTime - .002 : sleep(nextTime - t)
			nextTime += UPDATE_PERIOD

		self.leaveComtree()
		self.disconnect()

	def stop(self) : self.quit = True

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
		and returned. Otherwise, None is returned.
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
		p.payload = struct.pack("!Q",self.nonce)
		reply = self.sendCtlPkt(p)
		return reply != None and reply.flags == ACK_FLAG

	def disconnect(self) :
		p = Packet(); p.type = DISCONNECT
		p.comtree = CLIENT_CON_COMT
		p.srcAdr = self.myFadr; p.dstAdr = self.rtrFadr
		p.payload = struct.pack("!Q",self.nonce)
		reply = self.sendCtlPkt(p)
		return reply != None and reply.flags == ACK_FLAG

	def joinComtree(self) :
		return self.sendJoinLeave(CLIENT_JOIN_COMTREE)

	def leaveComtree(self) :
		return self.sendJoinLeave(CLIENT_LEAVE_COMTREE)

	def sendJoinLeave(self,which) :
		p = Packet()
		p.type = CLIENT_SIG; p.comtree = CLIENT_SIG_COMT
		p.srcAdr = self.myFadr; p.dstAdr = self.comtCtlFadr

		cp = CtlPkt(which,REQUEST,self.seqNum)
		cp.comtree = self.comtree
		p.payload = cp.pack()
		self.seqNum += 1

		reply = self.sendCtlPkt(p)
		if reply == None : return False
		cpReply = CtlPkt(0,0,0)
		cpReply.unpack(reply.payload)

		return	cpReply.seqNum == cp.seqNum and \
			cpReply.cpTyp  == cp.cpTyp and \
			cpReply.mode  == POS_REPLY

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
	
		if self.numNear > 5000 or self.numNear < 0 :
			print "numNear=", self.numNear
		p.payload = struct.pack('!IIIIIIII', \
					STATUS_REPORT, self.now, \
					int(self.x)*GRID, int(self.y)*GRID, \
					self.direction, int(1000*self.speed), \
					self.numNear, self.numVis)
		self.send(p)

	def updateStatus(self) :
		""" Update position and heading of avatar.

		When a user is controlling the avatar, just
		compute new xy location based on direction/speed.
		Otherwise, wander randomly, avoiding walls and
		non-walkable squares.
		"""
		PI = 3.141519625

		loc = self.groupNum(self.x,self.y)-1

		if not self.auto : # user in control of direction and speed
			# update position
			dist = self.speed + 0.0
			dirRad = self.direction * (2*PI/360)
	
			x = self.x + dist * sin(dirRad)
			y = self.y + dist * cos(dirRad)
	
			nuloc = self.groupNum(x,y)-1
	
			# stop if we hit a wall
			if x <= 0 or x >= self.map.size or \
			   y <= 0 or y >= self.map.size or \
			   self.map.blocked(nuloc) or \
			   (nuloc != loc and self.map.separated(nuloc,loc)) :
				self.speed = STOPPED

			self.x = x; self.y = y
			if nuloc != loc :
				self.visSet = self.map.computeVisSet(nuloc)
				self.updateSubs()
			return

		# code for auto operation
		atLeft  = (loc%self.map.size <= 1)
		atRight = (loc%self.map.size >= self.map.size-2)
		atBot   = (loc/self.map.size <= 1)
		atTop   = (loc/self.map.size >= self.map.size-2)
		xd = self.x - int(self.x); yd = self.y - int(self.y)
		if xd < .4 and \
		   (atLeft or self.map.separated(loc,loc-1) or \
		    self.map.blocked(loc-1) or \
		    self.map.blocked(loc-(self.map.size+1)) or \
		    self.map.blocked(loc+self.map.size-1)) :
			# near left edge of loc
			if 160 < self.direction and self.direction < 270 :
				 self.direction -= 1
			if 270 <= self.direction or self.direction < 20 :
				 self.direction += 1
			self.speed = SLOW; self.deltaDir = 0
		elif xd > .6 and \
		   (atRight or self.map.separated(loc,loc+1) or \
		    self.map.blocked(loc+1) or \
		    self.map.blocked(loc-self.map.size+1) or \
		    self.map.blocked(loc+self.map.size+1)) :
			# near right edge of loc
			if 340 < self.direction or self.direction <= 90 :
				 self.direction -= 1
			if 90 < self.direction and self.direction < 200 :
				 self.direction += 1
			self.speed = SLOW; self.deltaDir = 0
		elif yd < .4 and \
		   (atBot or self.map.separated(loc,loc-self.map.size) or \
		    self.map.blocked(loc-self.map.size) or \
		    self.map.blocked(loc-(self.map.size+1)) or \
		    self.map.blocked(loc-(self.map.size-1))) :
			# near bottom edge of loc
			if 70 < self.direction and self.direction <= 180 :
				 self.direction -= 1
			if 180 < self.direction and self.direction < 290 :
				 self.direction += 1
			self.speed = SLOW; self.deltaDir = 0
		elif yd > .6 and \
		   (atTop or self.map.separated(loc,loc+self.map.size) or \
		    self.map.blocked(loc+self.map.size) or \
		    self.map.blocked(loc+self.map.size-1) or \
		    self.map.blocked(loc+self.map.size+1)) :
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
		dirRad = self.direction * (2*PI/360)
		self.x += dist * sin(dirRad)
		self.y += dist * cos(dirRad)
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
		return 1 + int(x1) + int(y1)*self.map.size
	
	def subscribe(self,glist) :
		""" Subscribe to the multicast groups in a list.
		"""
		if self.comtree == 0 or len(glist) == 0 : return
		p = Packet()
		nsub = 0
		buf = bytearray(2048)
		for g in glist :
			nsub += 1
			if nsub > 300 :
				self.subSeqNum++;
				struct.pack_into('!I',buf,0,
						 (self.subSeqNum >> 32) & 0xffffffff)
				struct.pack_into('!I',buf,4*1,
						 self.subSeqNum & 0xffffffff)
				nsub -= 1;
				struct.pack_into('!I',buf,4*2,nsub)
				struct.pack_into('!I',buf,4*(nsub+3),0)
				p.payload = str(buf[0:4*(nsub+4)])
				p.length = OVERHEAD + 4*(nsub+4)
				p.type = SUB_UNSUB
				p.comtree = self.comtree
				p.srcAdr = self.myFadr; p.dstAdr = self.rtrFadr
				self.send(p)
				p = Packet()
				nsub = 1
			struct.pack_into('!i',buf,4*(nsub+2),-g)

		self.subSeqNum++;
		struct.pack_into('!I',buf,0,(self.subSeqNum >> 32) & 0xffffffff)
		struct.pack_into('!I',buf,4*1,self.subSeqNum & 0xffffffff)
		struct.pack_into('!I',buf,4*2,nsub)
		struct.pack_into('!I',buf,4*(nsub+3),0)
		p.payload = str(buf[0:4*(nsub+4)])
		p.length = OVERHEAD + 4*(nsub+4);
		p.type = SUB_UNSUB
		p.comtree = self.comtree;
		p.srcAdr = self.myFadr; p.dstAdr = self.rtrFadr
		self.send(p)
	
	def unsubscribe(self,glist) :
		""" Unsubscribe from the groups in a list.
		"""
		if self.comtree == 0 or len(glist) == 0 : return
		p = Packet()
		nunsub = 0
		buf = bytearray(2048)
		for g in glist :
			nunsub += 1
			if nunsub > 300 :
				self.subSeqNum++;
				struct.pack_into('!I',buf,0,
					 (self.subSeqNum >> 32) & 0xffffffff)
				struct.pack_into('!I',buf,4*1,
					 self.subSeqNum & 0xffffffff)
				nunsub -= 1
				struct.pack_into('!I',buf,4*2,0)
				struct.pack_into('!I',buf,4*3,nunsub)
				p.payload = str(buf[0:4*(nunsub+4)])
				p.length = OVERHEAD + 4*(nunsub+4)
				p.type = SUB_UNSUB
				p.comtree = self.comtree
				p.srcAdr = self.myFadr; p.dstAdr = self.rtrFadr
				self.send(p)
				p = Packet()
				nunsub = 1
			struct.pack_into('!i',buf,4*(nunsub+1),-g)

		self.subSeqNum++;
		struct.pack_into('!I',buf,0,(self.subSeqNum >> 32) & 0xffffffff)
		struct.pack_into('!I',buf,4*1,self.subSeqNum & 0xffffffff)
		struct.pack_into('!I',buf,4*2,0)
		struct.pack_into('!I',buf,4*3),nunsub)
		p.payload = str(buf[0:4*(nunsub+4)])
		p.length = OVERHEAD + 4*(nunsub+4);
		p.type = SUB_UNSUB
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

		This just updates the maps of remote and visible avatars.
		"""
		tuple = struct.unpack('!IIIII',p.payload[0:20])

		if tuple[0] != STATUS_REPORT : return
		x1 = float(tuple[2])/GRID; y1 = float(tuple[3])/GRID;
		dir1 = tuple[4]
		avId = p.srcAdr

		if avId in self.nearRemotes :
			# update map entry for this remote
			remote = self.nearRemotes[avId]
			remote[0] = x1; remote[1] = y1; remote[2] = dir1
		elif len(self.nearRemotes) < MAXNEAR :
			# fadr -> x,y,direction
			self.nearRemotes[avId] = [x1,y1,dir1]

		if avId in self.nearRemotes and avId not in self.visRemotes :
			if self.groupNum(x1,y1)-1 in self.visSet and \
			   self.map.canSee((x,y),(x1,y1)) :
				self.visRemotes[avId] = [x1,y1,dir]
