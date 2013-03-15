import sys
import struct
from time import sleep, time
from random import randint, random
from threading import *
from Queue import Queue
from collections import deque
from Packet import *
from CtlPkt import *
from math import sin, cos
from direct.task import Task

UPDATE_PERIOD = .05 	# number of seconds between status updates
STATUS_REPORT = 1 	# code for status report packets
#NUM_ITEMS = 10		# number of items in status report #Chao: unused for now
GRID = 10000  		# xy extent of one grid square
MAXNEAR = 1000		# max # of nearby avatars
REGION_WIDTH = 40	# width of an area for region multicast (in meters)
VIS_RANGE = 20		# visible range (in meters)

# speeds below are the amount avatar moves during an update period
# making them all equal for now, to avoid funky animation
STOPPED =   0		# not moving
SLOW    = 50    	# slow avatar speed
MEDIUM  = 50    	# medium avatar speed
FAST    = 50    	# fast avatar speed

class Net :
	""" Net support.

	"""
	def __init__(self, myIp, cliMgrIp, comtree, pWorld, debug, auto):
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
		self.pWorld = pWorld
		self.debug = debug
		self.auto = auto

		# open and configure socket to be nonblocking
		self.sock = socket(AF_INET, SOCK_DGRAM);
		self.sock.bind((myIp,0))
		self.sock.setblocking(0)
		self.myAdr = self.sock.getsockname()

		# get the width of the world
		self.limit = pWorld.getLimit()
		# set initial position
		x, y, self.direction = self.pWorld.getPosHeading()
		#self.x = int(x); self.y = int(y)
		self.x = x; self.y = y
		if self.auto :	self.direction = randint(0,359)
		self.speed = SLOW

		self.nearRemotes = {}	# map: fadr -> [x,y,dir,dx,dy,count]
		self.regSet = set() # subscriptions for region multicasts
		self.avaSet = set() # subscriptions for avatar multicasts
							# (those who are in the visible range)
		self.mySubs = self.regSet.union(self.avaSet)# total subscriptions
		self.avaInRegions = set()   # bookkepping for avatar multicasts
									# (those who are in the four regions)

		# the total number of regions in current world partition
		self.numRegions = (self.limit/REGION_WIDTH)**2
		# A multicast address associated with our avatar.
		# set to self.myFadr + self.numRegions during login()
		self.myMulAdr = 0


		self.seqNum = 1		# sequence # for control packets


	def init(self, uname, pword) :
		if not self.login(uname, pword) : return False
		self.rtrAdr = (ip2string(self.rtrIp),ROUTER_PORT)
		self.t0 = time(); self.now = 0; self.nextTime = 0;
		self.regionTime = 0
		self.connect()
		sleep(.1)
		if not self.joinComtree() :
			sys.stderr.write("Net.run: cannot join comtree\n")
			return False

		currentRegion = self.regionNum(self.x, self.y)
		self.mySubs.add(currentRegion)

		glist = []
		glist += [currentRegion]
		self.subscribe(glist)

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
		if self.debug == 1: print "login: self.myFadr=", self.myFadr
		self.rtrIp = tuple[1]
		self.comtCtlFadr = tuple[2]

		# Set avatar's own multicast adr.
		# Defined by its forest address plus
		# the total number of regions in a map.
		self.myMulAdr = self.myFadr+self.numRegions
	
		return True


	def run(self, task) :
		""" This is the main method for the Net object.
		"""

		self.now = time() - self.t0

		# pacing the rate of sending status to a region multicast
		if self.now < self.regionTime : pass
		else :
			self.sendStatusRegion()
			self.regionTime += 1*(1+len(self.avaInRegions))
			if self.debug == 1 :
				print "# of peers in regions:", len(self.avaInRegions)
			self.avaInRegions.clear()

		# process once per UPDATE_PERIOD
		if self.now < self.nextTime : return task.cont
		self.nextTime += UPDATE_PERIOD

		avaSetCandi = set() # candidates to be added in avaSet

		while True :
			# substrate has an incoming packet
			p = self.receive()
			if p == None : break
			# update remote mappings and avaSetCandi
			peer = self.updateNearby(p)
			if peer != None :
				avaSetCandi.add(peer)

		self.pruneNearby()	# check for remotes that are MIA
							# (Chao: MIA == missing in action?)
		self.updateStatus(avaSetCandi)	# update Avatar status
		
		# finally, send status to avatar multicast
		self.sendStatusAva()

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


	def sendStatusAva(self) :
		""" Send status packet on avatar's multicast.
		"""
		if self.comtree == 0 : return
		p = Packet()
		p.leng = OVERHEAD + 4*8; p.type = CLIENT_DATA; p.flags = 0
		p.comtree = self.comtree
		p.srcAdr = self.myFadr
		p.dstAdr = -self.myMulAdr
	
		numNear = len(self.nearRemotes)
		p.payload = struct.pack('!8I', \
					STATUS_REPORT, self.now, \
					int(self.x*GRID), int(self.y*GRID), \
					self.direction, self.speed, \
					self.myMulAdr, numNear)
		self.send(p)


	def sendStatusRegion(self) :
		""" Send status packet on region multicast.
		"""
		if self.comtree == 0 : return
		p = Packet()
		p.leng = OVERHEAD + 4*8; p.type = CLIENT_DATA; p.flags = 0
		p.comtree = self.comtree
		p.srcAdr = self.myFadr
		p.dstAdr = -self.regionNum(self.x,self.y)
	
		numNear = len(self.nearRemotes)
		p.payload = struct.pack('!8I', \
					STATUS_REPORT, self.now, \
					int(self.x*GRID), int(self.y*GRID), \
					self.direction, self.speed, \
					self.myMulAdr, numNear)
		self.send(p)


	def updateStatus(self, avaSetCandi) :
		""" Update position and heading of avatar.

		Update local state from the panda model.
		"""

		x, y, self.direction = self.pWorld.getPosHeading()
		self.x = x; self.y = y

		# update region
		currentRegion = self.regionNum(self.x, self.y)
		newRegSet = set()
		# add current region in the set
		newRegSet.add(currentRegion)
		# add three adjacent regions in the set.
		# 	first, calculate the reference point of current region
		regNumPerAxis = self.limit/REGION_WIDTH

		# calculate a reference point;
		# use it to determine our relative location in a region;
		# then use that info to determine which three adjacent
		# regions to subscribe to
		refX = ((currentRegion-1)%regNumPerAxis)*REGION_WIDTH
		refY = ((currentRegion-1)/regNumPerAxis)*REGION_WIDTH

		# lower-left subregion
		if self.x-refX <= REGION_WIDTH/2 \
		   and self.y-refY <= REGION_WIDTH/2 :
			# check boundaries
			if refX == 0 and refY == 0 :
				pass
			elif refX == 0 :
				newRegSet.add(currentRegion-regNumPerAxis)
			elif refY == 0 :
				newRegSet.add(currentRegion-1)
			else :
				newRegSet.add(currentRegion-regNumPerAxis)
				newRegSet.add(currentRegion-1)
				newRegSet.add(currentRegion-regNumPerAxis-1)
		# lower-right subregion
		elif self.y-refY <= REGION_WIDTH/2 :
			# check boundaries
			if refX == (regNumPerAxis-1)*REGION_WIDTH and refY == 0 :
				pass
			elif refX == (regNumPerAxis-1)*REGION_WIDTH :
				newRegSet.add(currentRegion-regNumPerAxis)
			elif refY == 0 :
				newRegSet.add(currentRegion+1)
			else :
				newRegSet.add(currentRegion-regNumPerAxis)
				newRegSet.add(currentRegion+1)
				newRegSet.add(currentRegion-regNumPerAxis+1)
		# upper-left subregion
		elif self.x-refX <= REGION_WIDTH/2 :
			# check boundaries
			if refX == 0 and refY == (regNumPerAxis-1)*REGION_WIDTH :
				pass
			elif refX == 0 :
				newRegSet.add(currentRegion+regNumPerAxis)
			elif refY == (regNumPerAxis-1)*REGION_WIDTH :
				newRegSet.add(currentRegion-1)
			else :
				newRegSet.add(currentRegion+regNumPerAxis)
				newRegSet.add(currentRegion-1)
				newRegSet.add(currentRegion+regNumPerAxis-1)
		# upper-right subregion
		else :
			# check boundaries
			if refX == (regNumPerAxis-1)*REGION_WIDTH and \
			   refY == (regNumPerAxis-1)*REGION_WIDTH :
				pass
			elif refX == (regNumPerAxis-1)*REGION_WIDTH :
				newRegSet.add(currentRegion+regNumPerAxis)
			elif refY == (regNumPerAxis-1)*REGION_WIDTH :
				newRegSet.add(currentRegion+1)
			else :
				newRegSet.add(currentRegion+regNumPerAxis)
				newRegSet.add(currentRegion+1)
				newRegSet.add(currentRegion+regNumPerAxis+1)

		self.updateSubs(avaSetCandi, newRegSet)


	def regionNum(self, x1, y1) :
		""" Return multicast region number for given position.

		x1 is the x coordinate of the position of interest
		y1 is the y coordinate of the position of interest
		To convert region number to a region multicast address,
		just negate it.
		"""
		return 1 + (int(x1)/REGION_WIDTH) \
				+ (int(y1)/REGION_WIDTH)*(self.limit/REGION_WIDTH)


	def subscribe(self,glist) :
		""" Subscribe to a list of multicast groups.
		"""
		if self.comtree == 0 or len(glist) == 0 : return
		p = Packet()
		nsub = 0
		buf = bytearray(2048)

		if self.debug == 1:
			debugMsg = []
			for g in glist :
				if g <= self.numRegions : debugMsg += [g]
				else : debugMsg += [fadr2string(g-self.numRegions)]
			print "Multicasts to be subscribed : ", debugMsg

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

		if self.debug == 1:
			debugMsg = []
			for g in glist :
				if g <= self.numRegions : debugMsg += [g]
				else : debugMsg += [fadr2string(g-self.numRegions)]
			print "Multicasts to be unsubscribed : ", debugMsg

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
	
	
	def unsubscribeAll(self) :
		""" Unsubscribe from all multicasts. """
		self.unsubscribe(list(self.mySubs)); self.mySubs.clear()


	def updateSubs(self, avaSetCandi, newRegSet) :	
		""" Update subscriptions.
		
		Subscribe to
			1. multicasts for visible avatars;
			2. current region multicast
		Unsubscribe from
			1. previous region multicast

		We unsubscribe an avatar's multicast
		if we haven't heard from it in a while;
		implemented in method pruneNearby()
		"""

		glist = [] # a list of multicast addresses to be subscribed
		glist += list(newRegSet.difference(self.regSet))
		glist += list(avaSetCandi.difference(self.avaSet))

		self.subscribe(glist)

		glist1 = list(self.regSet.difference(newRegSet))
		self.unsubscribe(glist1)

		self.regSet = newRegSet
		self.avaSet = self.avaSet.union(avaSetCandi)
		self.mySubs = self.regSet.union(self.avaSet)

		if self.debug == 1 and (glist != [] or glist1 != []) :
			print "new regSet:", list(self.regSet)
			newAva = [e-self.numRegions for e in list(self.avaSet)]
			print "new avaSet:", [fadr2string(e) for e in newAva]
	

	def updateNearby(self, p) :
		""" Process a packet from a remote.

		1. return new peer's multicast address
		2. Currently, this just updates the map of remotes.
		Could be extended to change the state of this avatar
		(say, process a "kill packet")
		"""
		tuple = struct.unpack('!8I',p.payload[0:32])

		if tuple[0] != STATUS_REPORT : return
		# Chao: currently, only STATUS_REPORT

		x1 = (tuple[2]+0.0)/GRID; y1 = (tuple[3]+0.0)/GRID;
		dir1 = tuple[4]
		peerMulAdr = tuple[6]
		avId = p.srcAdr

		# Note: it is impossible to receive
		# our own multicast packet: router has prevented the case.
		# See multiSend() in RouterCore.cpp

		if avId in self.nearRemotes :

			# Since we've subscribed to its avatar multicast,
			# no need to update based on region multicase
			# (and if we do, it's likely to cause choppy animation)
			if peerMulAdr in self.avaSet and \
			   -p.dstAdr <= self.numRegions :
				if self.debug == 1 :
					avID = peerMulAdr-self.numRegions
					print "skip update from region multicast:" ,\
						 fadr2string(avID)
				return

			# update map entry for this remote
			remote = self.nearRemotes[avId]
			remote[3] = x1 - remote[0]; remote[4] = y1 - remote[1]
			remote[0] = x1; remote[1] = y1; remote[2] = dir1
			remote[5] = 0
			self.pWorld.updateRemote(x1,y1, dir1, avId)
			return
		elif len(self.nearRemotes) < MAXNEAR :
			self.avaInRegions.add(peerMulAdr)
			# if an avatar is visible, subscribe to its multicast
			if abs(self.x-x1)+abs(self.y-y1) <= VIS_RANGE :
				# fadr -> x,y,direction,dx,dy,count
				self.nearRemotes[avId] = [x1,y1,dir1,0,0,0]
				self.pWorld.addRemote(x1,y1, dir1, avId)
				return peerMulAdr
			else : return
		return
	

	def pruneNearby(self) :
		""" Remove old entries from nearRemotes

		If we haven't heard from a remote in a while, we remove
		it from the map of nearby remotes and unsubscribe from it.
		When a remote first "goes missing" we continue to
		update its position, by extrapolating from last update.
		"""
		dropList = []
		for avId, remote in self.nearRemotes.iteritems() :
			if remote[5] > 0 : # implies missing update
#				print "missing update:", remote[5]
				# update position based on dx, dy
				remote[0] += remote[3]; remote[1] += remote[4]
				remote[0] = max(0,remote[0])
				remote[0] = min(self.limit,remote[0])
				remote[1] = max(0,remote[1])
				remote[1] = min(self.limit,remote[1])
			if remote[5] >= 6 or \
			   abs(self.x-remote[0])+abs(self.y-remote[1])>=VIS_RANGE:
				dropList.append(avId)
			remote[5] += 1
		for avId in dropList :
			rem = self.nearRemotes[avId]
			x = rem[0]; y = rem[1]
			del self.nearRemotes[avId]
			self.pWorld.removeRemote(avId)

		# Reminder: avId is defined to be forest adr,
		# and avatar multicast is defined to be forest adr plus
		# the total number of regions in our world partition.
		# That's why we do the following transformation when
		# determining the multicast adrs to be unsubscribed.
		unsubList = [e+self.numRegions for e in dropList]
		self.unsubscribe(unsubList)

		self.avaSet = self.avaSet.difference(set(unsubList))
		self.mySubs = self.regSet.difference(set(unsubList))

		if self.debug == 1 and dropList != [] :
			newAva = [e-self.numRegions for e in list(self.avaSet)]
			print "new avaSet:", [fadr2string(e) for e in newAva]
