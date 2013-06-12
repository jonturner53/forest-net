from socket import *
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
#edit by feng
import errno
import pyaudio
import wave
#edit by feng end
import os

UPDATE_PERIOD = .05 	# number of seconds between status updates
STATUS_REPORT = 1 	# code for status report packets
AUDIO = 2 		# code for audio packets
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

#audio parameters, edit by feng
CHUNK = 512
BYTES = CHUNK*2
CHANNELS = 1
FORMAT = pyaudio.paInt16
RATE = 11025

class Net :
	""" Net support.

	"""
	def __init__(self, myIp, cliMgrIp, comtree, numg, subLimit, pWorld, name, \
		     debug):
		""" Initialize a new Net object.

		myIp address is IP address of this host
		comtree is the number of the comtree to use
		numg*numg is the number of multicast groups
		subLimit defines the maximum visibility distance for multicast
		groups
		debug is an integer >=0 that determines amount of
		debugging output
		"""

		self.myIp = myIp
		self.cliMgrIp = cliMgrIp
		self.comtree = comtree
		self.pWorld = pWorld
        	self.name = name
		self.debug = debug
		self.count = 0

		#edit by feng
		self.audioLevel = 0	#if Avatar is talking
		self.isListening = 0	#if Avatar is listening
		self.audioAdr = None	#audio multicast address
		#edit by feng end

		# setup multicast object to maintain subscriptions
		self.mcg = Mcast(numg, subLimit, self, self.pWorld)

		# open and configure socket to be nonblocking
		self.sock = socket(AF_INET, SOCK_DGRAM);
		self.sock.bind((myIp,0))
		self.sock.setblocking(0)
		self.myAdr = self.sock.getsockname()

		self.limit = pWorld.getLimit()

		# set initial position
		self.x, self.y, self.direction = \
			self.pWorld.getPosHeading()

		self.mySubs = set()	# multicast subscriptions
		self.nearRemotes = {}	# map: fadr -> [x,y,dir,dx,dy,count]

		self.seqNum = 1		# sequence # for control packets
		
		#initial the stream, edit by feng
		self.p1 = pyaudio.PyAudio()
                self.stream = self.p1.open(format=FORMAT,
                        channels=CHANNELS,
                        rate=RATE,
                        output=True)
                self.p = pyaudio.PyAudio()
		self.streamin = self.p.open(format=FORMAT,
                                channels=CHANNELS,
                                rate=RATE,
				start=False,
                                input=True,
                                frames_per_buffer=CHUNK)
		#edit end by feng

	def init(self, uname, pword) :
		if not self.login(uname, pword) : return False
		self.rtrAdr = (ip2string(self.rtrIp),self.rtrPort)
		self.t0 = time(); self.now = 0; self.nextTime = 0;
		if not self.connect() : return False
		sleep(.1)
		if not self.joinComtree() :
			sys.stderr.write("Net.run: cannot join comtree\n")
			return False
		self.mcg.updateSubs(self.x,self.y,self.audioAdr)
		taskMgr.add(self.run, "netTask", uponDeath=self.wrapup, \
				appendTask=True)
		return True

	def readLine(self,sock,buf) :
		""" Return next line in a buffer, refilling buffer as necessary.
		sock is a blocking stream socket to read from
		buf is a buffer containing data read from socket but
		not yet returned as a distinct linke
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
		cmSock = socket(AF_INET, SOCK_STREAM);
		cmSock.connect((self.cliMgrIp,30122))
	
		cmSock.sendall("Forest-login-v1\nlogin: " + uname + \
                	       "\npassword: " + pword + "\nover\n")


		buf = ""
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

		cmSock.close()

		if self.debug >= 1 :
			print "avatar address =", fadr2string(self.myFadr)
			print "router info = (", ip2string(self.rtrIp), \
					     str(self.rtrPort), \
					     fadr2string(self.rtrFadr), ")"
			print "comtCtl address = ",fadr2string(self.comtCtlFadr)
			print "nonce = ", self.nonce

		return True
	
	# getPhoto communicates with the photo server and get the picture specified by uname
	
	def getPhoto(self, uname) :		
		psSock = socket(AF_INET, SOCK_STREAM)
		self.photoServerIp = gethostbyname("shell.cec.wustl.edu")
		print self.photoServerIp
		psSock.connect((self.photoServerIp, 30124))
		
		#send a getPhoto request
		totalSent = 0
		msgLen = 10 + len(uname)
		while totalSent < msgLen:
			sent = psSock.send("getPhoto: " + uname)
			if sent == 0:
				raise RuntimeError("socket connection broken")
			totalSent = totalSent + sent
			
		# receiving the photo	
		buffer = ''
		photo = ''
		photoLen = 0
		gotPic = false
		while (not gotPic):
			buffer = psSock.recv(4)
			if buffer == '':
				raise RuntimeError("socket connection broken")			
			len = struct.unpack("!I", buffer)[0]
			photoLen = photoLen + len
			if len < 1024:
				gotPic = true
			more2read = true
			while(more2read):
				block = psSock.recv(len) #returns a string
				len = len - len(block)
				photo = photo + block
				if len == 0:
					more2read = false
					
		#tuple = struct.unpack(str(photoLen/4) + "I", photo) #endian don't care?
		out = open(uname + ".jpg", "wb")
		out.write(photo)
		out.close()
		
		
		psSock.send("complete!")
		psSock.close()
						
		
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

		#record every 50ms, edit by feng
		try:
			if self.audioLevel == 1:
				self.streamin.start_stream()
				data = self.streamin.read(CHUNK)
				self.sendAudio(data)
			else:
				self.streamin.stop_stream()	
		except IOError:
			print 'warning: dropped frame'
		#edit by feng end

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
		cp.ip1 = string2ip(self.myIp)
		cp.port1 = self.myAdr[1]
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
		namelen = len(self.name)
		p.leng = OVERHEAD + 4*8 + namelen ; p.type = CLIENT_DATA; p.flags = 0
		p.comtree = self.comtree
		p.srcAdr = self.myFadr;
		p.dstAdr = -self.mcg.groupNum(self.x,self.y)

		now = int(1000000 * self.now)
	
		numNear = len(self.nearRemotes)
		if numNear > 5000 or numNear < 0 :
			print "numNear=", numNear
		#add audioLevel and audioAddress in payload, edit by feng
		p.payload = struct.pack('!IIIIII' + str(namelen) + 'sIi', \
					STATUS_REPORT, self.now, \
					int(self.x*GRID), int(self.y*GRID), \
					self.direction, namelen, self.name, self.audioLevel, self.myFadr)
		#edit end by feng
		self.send(p)
	
	#edit by feng
	def sendAudio(self,data) :
                """ Send audio packet on audio multicast address.
                """
                if self.comtree == 0 : return
                p = Packet()
                p.leng = OVERHEAD + BYTES+20; p.type = CLIENT_DATA; p.flags = 0
                p.comtree = self.comtree
                p.srcAdr = self.myFadr;
                p.dstAdr = -self.myFadr

                numNear = len(self.nearRemotes)
                if numNear > 5000 or numNear < 0 :
                        print "numNear=", numNear
		p.payload = struct.pack('!5I'+str(BYTES)+'s', \
                                        AUDIO,0,0,0,0, data)
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
		self.audioLevel = self.pWorld.getAudioLevel()
		self.mcg.updateSubs(self.x,self.y,self.audioAdr)
		return
	
	def updateNearby(self, p) :
		""" Process a packet from a remote.

		Currently, this just updates the map of remotes.
		Could be extended to change the state of this avatar
		(say, process a "kill packet")
		"""
        	tuple = struct.unpack('!IIIII',p.payload[0:20])
		if tuple[0] == STATUS_REPORT :
    			x1 = (tuple[2]+0.0)/GRID; y1 = (tuple[3]+0.0)/GRID;
    			dir1 = tuple[4]
            
			tuple = struct.unpack('!I', p.payload[20:24])
			namelen = tuple[0]
			tuple = struct.unpack(str(namelen) + 's', p.payload[24:(24+namelen)])
			name = tuple[0]
		    
		    	# feng: to assign audio level and addr.
			tuple = struct.unpack('!Ii',p.payload[24+namelen:32+namelen])
			audiolvl = tuple[0]
			audioAdr = tuple[1]
			if audiolvl == 1 and self.isListening == 0:
				self.audioAdr = audioAdr
				self.isListening = 1
			elif audiolvl ==0 and self.isListening == 1:
				if audioAdr == self.audioAdr:
					self.audioAdr = None
					self.isListening = 0
				    		
		    	avId = p.srcAdr        
		    	if avId in self.nearRemotes :
				# update map entry for this remote
				remote = self.nearRemotes[avId]
				remote[3] = x1 - remote[0]; remote[4] = y1 - remote[1]
				remote[0] = x1; remote[1] = y1; remote[2] = dir1
				remote[5] = 0
				self.pWorld.updateRemote(x1,y1, dir1, avId,name)
			elif len(self.nearRemotes) < MAXNEAR :
				# fadr -> x,y,direction,dx,dy,count
				self.nearRemotes[avId] = [x1,y1,dir1,0,0,0]
				# download picture for a remote for the 1st time and save it in cache
				savedPath = os.getcwd()
				os.chdir("photo_cache")
				self.getPhoto(name)
				os.chdir(savedPath)
				self.pWorld.addRemote(x1, y1, dir1, avId, name)                           		
        	#play audio,edit by feng
		elif tuple[0] == AUDIO :
			tuple = struct.unpack('!'+str(BYTES)+'s',p.payload[20:BYTES+20])
			self.stream.write(tuple[0])
			return
		

	
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