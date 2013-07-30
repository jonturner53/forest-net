import sys
import struct
from time import sleep, time
from random import randint, random
from threading import *
from Queue import Queue
from collections import deque
from Mcast import *
from Packet import *
from CtlPkt import *
#from WorldMap import *
#from Substrate import *
from math import sin, cos
import direct.directbase.DirectStart
from panda3d.core import *
from direct.task import Task

UPDATE_PERIOD = .05 	# number of seconds between status updates
STATUS_REPORT = 1 	# code for status report packets
NUM_ITEMS = 10		# number of items in status report
GRID = 10000  		# xy extent of one grid square
MAXNEAR = 1000		# max # of nearby avatars
MAX_VIS = 20		# visibility range (measured in cells)

# speeds below are the amount avatar moves during an update period
# making them all equal to avoid funky animation
STOPPED =   0		# not moving
SLOW    = 50    	# slow avatar speed
MEDIUM  = 50    	# medium avatar speed
FAST    = 50    	# fast avatar speed

class Net :
	""" Net support.

	"""
	def __init__(self, cliMgrIp, comtree,  actualRegionSizeX, actualRegionSizeY, subLimit, pWorld, \
		     debug):
		""" Initialize a new Net object.

		comtree is the number of the comtree to use
		numg*numg is the number of multicast groups
		subLimit defines the maximum visibility distance for multicast
		groups
		debug is an integer >=0 that determines amount of
		debugging output
		"""

		self.cliMgrIp = cliMgrIp
		self.comtree = comtree
		self.pWorld = pWorld
		self.debug = debug
		self.count = 0

		# setup multicast object to maintain subscriptions
		self.mcg = Mcast(actualRegionSizeX, actualRegionSizeY, subLimit, self, self.pWorld)

		# open and configure socket to be nonblocking
		self.sock = socket(AF_INET, SOCK_DGRAM);
		self.sock.setblocking(0)
		self.myAdr = self.sock.getsockname()

		self.limit = pWorld.getLimit()

		# set initial position
		self.x, self.y, self.direction = \
			self.pWorld.getPosHeading()

		self.mySubs = set()	# multicast subscriptions
		self.nearRemotes = {}	# map: fadr -> [x,y,dir,dx,dy,count]

		self.seqNum = 1		# sequence # for control packets

	def init(self, uname, pword) :
		if not self.login(uname, pword) : return False
		self.rtrAdr = (ip2string(self.rtrIp),self.rtrPort)
		self.t0 = time(); self.now = 0; self.nextTime = 0;
		print "connect to router"
		if not self.connect() : return False
		print "connect succeeded"
		sleep(.1)
		if not self.joinComtree() :
			sys.stderr.write("Net.run: cannot join comtree\n")
			return False
		self.mcg.updateSubs(self.x,self.y)
		taskMgr.add(self.run, "netTask", uponDeath=self.wrapup, \
				appendTask=True)
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
		if line != "success" :
			print "oops", line
			return False
		line,buf = self.readLine(cmSock,buf)
		if line != "over" :
			print "oops", line
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
			if e.errno == 35 or e.errno == 11 or e.errno == 10035: return None
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
		p.leng = OVERHEAD + 4*5; p.type = CLIENT_DATA; p.flags = 0
		p.comtree = self.comtree
		p.srcAdr = self.myFadr;
		p.dstAdr = -self.mcg.groupNum(self.x,self.y)

		now = int(1000000 * self.now)
	
		numNear = len(self.nearRemotes)
		if numNear > 5000 or numNear < 0 :
			print "numNear=", numNear
		p.payload = struct.pack('!IIIII', \
					STATUS_REPORT, now, \
					int(self.x*GRID), int(self.y*GRID), \
					int(self.direction))
		self.send(p)

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
				struct.pack_into('!I',buf,0,nsub-1)
				struct.pack_into('!I',buf,4*(nsub),0)
				p.length = OVERHEAD + 4*(1+nsub)
				p.type = SUB_UNSUB
				p.comtree = self.comtree
				p.srcAdr = self.myFadr; p.dstAdr = self.rtrFadr
				p.payload = str(buf[0:4*(2+nsub)])
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
		""" Unsubscribe from the groups in a list.
		"""
		if self.comtree == 0 or len(glist) == 0 : return
		p = Packet()
		nunsub = 0
		buf = bytearray(2048)
		for g in glist :
			nunsub += 1
			if nunsub > 300 :
				struct.pack_into('!II',buf,0,0,nunsub-1)
				p.length = OVERHEAD + 4*(1+nunsub)
				p.type = SUB_UNSUB
				p.comtree = self.comtree
				p.srcAdr = self.myFadr; p.dstAdr = self.rtrFadr
				p.payload = str(buf[0:4*(2+nunsub)])
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

	def updateStatus(self) :
		""" Update position and heading of avatar.

		When a player is controlling the avatar, just update
		local state from the panda model. Otherwise, wander
		randomly, avoiding walls and non-walkable squares.
		"""
		self.x, self.y, self.direction = self.pWorld.getPosHeading()
		self.mcg.updateSubs(self.x,self.y)
		return
	
	def updateNearby(self, p) :
		""" Process a packet from a remote.

		Currently, this just updates the map of remotes.
		Could be extended to change the state of this avatar
		(say, process a "kill packet")
		"""
		tuple = struct.unpack('!IIIII',p.payload[0:20])

		if tuple[0] != STATUS_REPORT : return
		x1 = (tuple[2]+0.0)/GRID; y1 = (tuple[3]+0.0)/GRID;
		dir1 = tuple[4]
		avId = p.srcAdr

		if avId in self.nearRemotes :
			# update map entry for this remote
			remote = self.nearRemotes[avId]
			remote[3] = x1 - remote[0]; remote[4] = y1 - remote[1]
			remote[0] = x1; remote[1] = y1; remote[2] = dir1
			remote[5] = 0
			self.pWorld.updateRemote(x1,y1, dir1, avId)
		elif len(self.nearRemotes) < MAXNEAR :
			# fadr -> x,y,direction,dx,dy,count
			self.nearRemotes[avId] = [x1,y1,dir1,0,0,0]
			self.pWorld.addRemote(x1, y1, dir1, avId)
	
	def pruneNearby(self) :
		""" Remove old entries from nearRemotes

		If we haven't heard from a remote in a while, we remove
		it from the map of nearby remotes. When a remote first
		"goes missing" we continue to update its position,
		by extrapolating from last update.
		"""
		dropList = []
		for avId, remote in self.nearRemotes.iteritems() :
			if remote[5] > 0 : # implies missing update
				# update position based on dx, dy
				remote[0] += remote[3]; remote[1] += remote[4]
				remote[0] = max(0,remote[0])
				remote[0] = min(self.limit,remote[0])
				remote[1] = max(0,remote[1])
				remote[1] = min(self.limit,remote[1])
			if remote[5] >= 6 :
				dropList.append(avId)
			remote[5] += 1
		for avId in dropList :
			rem = self.nearRemotes[avId]
			x = rem[0]; y = rem[1]
			del self.nearRemotes[avId]
			self.pWorld.removeRemote(avId)
