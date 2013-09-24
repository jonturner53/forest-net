""" Net.py

Date: 2013
Authors: Jon Turner, Chao Wang
Additions by Pan Feng, Vigo Wei

"""

import sys
import struct
from time import sleep, time
from random import randint, random
from threading import *
from collections import deque
import errno
import math
from math import sin, cos

import direct.directbase.DirectStart
from panda3d.core import *
from direct.task import Task

import Util
from Util import *
from Packet import *
from CtlPkt import *
from Mcast import *
from Avatar import *

if AUDIO : import pyaudio; import wave; import audioop

STATUS_REPORT = 1 	# code for status report packets
AUDIO_PKT = 2 		# code for audio packets
MESSAGE = 3 		# code for messages
CALL = 4 		# code for phone call requests

UPDATE_PERIOD = .05 	# number of seconds between status updates
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

# audio parameters
CHUNK = 512
BYTES = CHUNK*2
CHANNELS = 1
RATE = 10240
AUDIO_BUFFER_SIZE = 2
if AUDIO :
	FORMAT = pyaudio.paInt16

class Net :
	""" Net support.

	"""
	def __init__(self, cliMgrIp, comtree, userName, avaModel, myAvatar):
		""" Initialize a new Net object.

		comtree is the number of the comtree to use
		userName is the name of this avatar's user
		avaModel is the serial number of this avatar's model
		myAvatar is a reference to the Avatar object
		"""

		self.cliMgrIp = cliMgrIp
		self.comtree = comtree
		self.userName = userName
		self.avaModel = avaModel
		self.myAvatar = myAvatar

		self.count = 0


		# for chatting with others
		self.msg = "initial msg"

		# setup multicast object to maintain subscriptions
		self.mcg = Mcast(self, self.myAvatar)

		# open and configure socket to be nonblocking
		self.sock = socket(AF_INET, SOCK_DGRAM);
		self.sock.setblocking(0)
		self.myAdr = self.sock.getsockname()
		self.limit = myAvatar.getLimit()

		# set initial position
		self.x, self.y, self.z, self.direction = \
			self.myAvatar.getPosHeading()

		self.mySubs = set()	# multicast subscriptions
		self.nearRemotes = {}	# map: fadr -> [x,y,dir,dx,dy,count]

		self.seqNum = 1		# sequence # for control packets
		self.subSeqNum = 1	# sequence # for subscriptions

		self.setupAudio()

		return

	def setupAudio(self) :
		# initialize audio parameters
		self.rms = 0			# Avatar volume value
		self.isMute = 0			# if mute or not
		self.remoteAudioLevel = 0	# if Avatar is listening
		self.remoteSpeaker = None	# address of remote speaker
		self.audioBuffer = deque(maxlen=AUDIO_BUFFER_SIZE)
		self.needtoWait = True

		if not AUDIO : return
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

	def init(self, uname, pword) :
		""" More initialization, including logging into ClientMgr.
		"""
		if not self.login(uname, pword) : return False
		self.rtrAdr = (ip2string(self.rtrIp),self.rtrPort)

		self.t0 = time.time()

		self.now = 0; self.nextTime = 0

		if not self.connect() : return False
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
		line, buf = self.readLine(cmSock,buf)
		if line != "success" :
			print "unexpected response to login:", line
			return False
		line,buf = self.readLine(cmSock,buf)
		print line
		if line != "over" :
			print "unexpected response to login:", line
			return False

		cmSock.sendall("newSession\nover\n")

		line, buf = self.readLine(cmSock,buf)
		chunks = line.partition(":")
		if chunks[0].strip() != "yourAddress" or chunks[1] != ":" :
			print "unexpected response (expecting address):", line
			return False
		self.myFadr = string2fadr(chunks[2].strip())

		line, buf = self.readLine(cmSock,buf)
		chunks = line.partition(":")
		if chunks[0].strip() != "yourRouter" or chunks[1] != ":" :
			print "unexpected response (expecting router info):", line
			return False
		triple = chunks[2].strip(); triple = triple[1:-1].strip()
		chunks = triple.split(",")
		if len(chunks) != 3 :
			print "unexpected response (expecting router info):", line
			return False
		self.rtrIp = string2ip(chunks[0].strip())
		self.rtrPort = int(chunks[1].strip())
		self.rtrFadr = string2fadr(chunks[2].strip())

		line,buf = self.readLine(cmSock,buf)
		chunks = line.partition(":")
		if chunks[0].strip() != "comtCtlAddress" or chunks[1] != ":" :
			print "unexpected response (expecting comtCtl info):", line
			return False
		self.comtCtlFadr = string2fadr(chunks[2].strip())

		line, buf = self.readLine(cmSock,buf)
		chunks = line.partition(":")
		if chunks[0].strip() != "connectNonce" or chunks[1] != ":" :
			print "unexpected response (expecting nonce):", line
			return False
		self.nonce = int(chunks[2].strip())

		line,buf = self.readLine(cmSock,buf) 
		if line != "overAndOut" :
			print "unexpected response (expecting signoff):", line
			return False

		cmSock.close()

		print "DEBUG=", Util.DEBUG
		if Util.DEBUG >= 1 :
			print "avatar address =", fadr2string(self.myFadr)
			print "router info = (", ip2string(self.rtrIp), \
					     str(self.rtrPort), \
					     fadr2string(self.rtrFadr), ")"
			print "comtCtl address = ",fadr2string(self.comtCtlFadr)
			print "nonce = ", self.nonce

		return True

	# getPhoto communicates with the photo server and gets the 
	# picture specified by uname
	""" commented out for now
	def getPhoto(self, uname) :		
		psSock = socket(AF_INET, SOCK_STREAM)
		self.photoServerIp = gethostbyname("forest2.arl.wustl.edu")
		psSock.connect((self.photoServerIp, 30124))
		
		#send a getPhoto request
		request = "getPhoto:" + uname + '\n'

		sent = psSock.sendall(request)
		if sent == 0:
			raise RuntimeError("socket connection broken")
		if Util.DEBUG >= 1:
			print "photo server IP: ", self.photoServerIp
			print "bytes sent: ", sent

		# receiving the photo	
		echo_len = 15
		echo_recvd = 0
		while echo_recvd == 0:
			buffer = psSock.recv(echo_len)
			if Util.DEBUG >= 1:
				print "buf= ", buffer
			if buffer == '':
				RuntimeError("broken connection")
			echo_len = echo_len - len(buffer) 
			if echo_len == 0:
				echo_recvd = 1
		if struct.unpack("7s", buffer[0:7])[0] == "failure":
			# didn't get pic, give up, may do something more?
			psSock.close()
		else:	
			photo = ''
			photoLen = struct.unpack("!I", buffer[7:11])[0]
			gotPic = 0
			while (not gotPic):
				buffer = psSock.recv(4)
				if buffer == '':
					raise RuntimeError("socket connection "
							   "broken")
				pktlen = struct.unpack("!i", buffer)[0]
				if pktlen < 1024:
					gotPic = 1
				more2read = 1
				if Util.DEBUG >= 1:
					print "buffer is: ", buffer
					print "repr(buffer) is: ", repr(buffer)
				print pktlen
				while(more2read):
					block = psSock.recv(pktlen)
					# returns a string
					pktlen = pktlen - len(block)
					photo = photo + block
					if pktlen == 0:
						more2read = 0
						
			out = open(uname + ".jpg", "wb")
			out.write(photo)
			out.close()

			psSock.send("complete!")
			psSock.close()
	"""

	def run(self, task) :
		""" This is the main method for the Net object.
		"""

		self.now = time.time() - self.t0

		# process once per UPDATE_PERIOD
		if self.now < self.nextTime : return task.cont
		self.nextTime += UPDATE_PERIOD

		while True :
			# substrate has an incoming packet
			p = self.receive()
			if p == None : break
			self.processPkt(p) 

		self.pruneNearby()	# check for remotes that we're no longer
					# getting reports from
		self.updateStatus()	# update Avatar status
		self.sendReport()	# send status report on comtree

		if AUDIO : self.playAudio
		return task.cont

	def playAudio(self) :
		""" Retrieve audio samples from microphone and send them,
		also play back audio data received from network
		"""
		try :
			self.streamin.start_stream()
			data = self.streamin.read(CHUNK)
			rms = audioop.rms(data,2)
			if rms > 100 and not self.isMute:
				self.sendAudio(data)
				self.rms = rms
		except IOError:
			print 'warning: dropped frame'
		if self.needtoWait == False and \
		   len(self.audioBuffer) > 0 :
			data = self.audioBuffer.popleft()
			self.stream.write(data)
		elif len(self.audioBuffer) == 0:
			self.needtoWait = True		
		return

	def wrapup(self, task) :
		self.leaveComtree()
		self.disconnect()

	def send(self, p) :
		""" Send a packet to the router.
		"""
		if Util.DEBUG >= 3 or \
		   (Util.DEBUG >= 2 and p.type != CLIENT_DATA) or \
		   (Util.DEBUG >= 1 and p.type == CLIENT_SIG) :
			print self.now, self.myAdr, "sending to", self.rtrAdr
			print p.toString()
			if p.type == CLIENT_SIG :
				cp = CtlPkt(0,0,0)
				cp.unpack(p.payload)
				print cp.toString()
			sys.stdout.flush()
		if p.length == 26 :
			print self.now, self.myAdr, "sending bad length", self.rtrAdr
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
			if e.errno == 35 or e.errno == 11 or \
			   e.errno == 10035: return None
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
		if Util.DEBUG >= 3 or \
		   (Util.DEBUG >= 2 and p.type != CLIENT_DATA) or \
		   (Util.DEBUG >= 1 and p.type == CLIENT_SIG) :
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
		p.length = OVERHEAD + len(p.payload)
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
		now = sendTime = time.time()
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
			now = time.time()
	
	""" code for calling another avatar - not yet complete
	def getCallRequest(self):
		if self.myAvatar.getCallRequest != 0:
			#we have a request
			#send
			self.myAvatar.getCallRequest()
		#p.srcAdr = self.myFadr;
		else:
			print "no request to handle"		

	def sendCallRequest(self, id):
		if self.comtree == 0 : return
		p = Packet()
		msglen = len(self.msg)
		p.length = OVERHEAD + 4*6 + msglen;
		p.type = CLIENT_DATA; p.flags = 0
		p.comtree = self.comtree
	"""						

	def sendReport(self) :
		""" Send status report on multicast group for current location.
		"""
		if self.comtree == 0 : return
		p = Packet()
		namelen = len(self.userName)
		# msglen = len(self.msg)
		p.length = OVERHEAD + 4*8 + namelen;
		p.type = CLIENT_DATA; p.flags = 0
		p.comtree = self.comtree
		p.srcAdr = self.myFadr;
		p.dstAdr = -self.mcg.groupNum(self.x,self.y)

		now = int(1000000 * self.now)
	
		numNear = len(self.nearRemotes)

		p.payload = struct.pack('!8I' + str(namelen) + 'sIi', \
				STATUS_REPORT, now, \
				int(self.x*GRID), int(self.y*GRID),
				int(self.z*GRID), \
				int(self.direction), self.avaModel, \
				namelen,

				self.userName,

				0, #int(self.rms),


				0) #self.myFadr)
				#msglen, self.msg, self.rms, self.myFadr)
		self.send(p)

	def sendAudio(self,data):
		""" Send audio packet to the network """

		if self.comtree == 0: return
		p = Packet()
		p.length = OVERHEAD + BYTES+20;
		p.type = CLIENT_DATA; p.flags = 0
		p.comtree = self.comtree
		p.srcAdr = self.myFadr
		p.dstAdr = -self.myFadr		# unique multicast address
		numNear = len(self.nearRemotes)
		now = int(1000000 * self.now)
		p.payload = struct.pack('!5I' + str(BYTES) + 's', \
					AUDIO_PKT, now,
					int(self.x*GRID), int(self.y*GRID),
					data)
		self.send(p)

	# def sendCallRequest(id):
	# 	if self.comtree == 0: return
	# 	p = Packet()
	# 	msglen = len(msg)
	# 	namelen = len(self.userName)
	# 	p.length = OVERHEAD + msglen + namelen + 8; p.type = CLIENT_DATA; p.flags = 0
	# 	p.comtree = self.comtree
	# 	p.srcAdr = self.myFadr
	# 	p.dstAdr = -self.myFadr
	# 	numNear = len(self.nearRemotes)
	# 	if numNear > 5000 or numNear < 0:
	# 		print "numNear: ", numNear
	# 	p.payload = struct.pack('!II' + str(msglen) + 'sI' + str(namelen) + 's', \
	# 							MESSAGE, msglen, self.msg, namelen, self.userName)

####	def sendMsg(self,msg):
####		if self.comtree == 0: return
####		p = Packet()
####		msglen = len(msg)
####		namelen = len(self.userName)
####		p.length = OVERHEAD + msglen + namelen + 8; 
####		p.type = CLIENT_DATA; p.flags = 0
####		p.comtree = self.comtree
####		p.srcAdr = self.myFadr
####		p.dstAdr = -self.myFadr
####		numNear = len(self.nearRemotes)
####		if numNear > 5000 or numNear < 0:
####			print "numNear: ", numNear
####		p.payload = struct.pack('!II' + str(msglen) + 'sI' + str(namelen) + 's', \
####			MESSAGE, msglen, self.msg, namelen, self.userName)	

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
				self.subSeqNum += 1
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

		self.subSeqNum += 1
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
				self.subSeqNum += 1;
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

		self.subSeqNum += 1
		struct.pack_into('!I',buf,0,(self.subSeqNum >> 32) & 0xffffffff)
		struct.pack_into('!I',buf,4*1,self.subSeqNum & 0xffffffff)
		struct.pack_into('!I',buf,4*2,0)
		struct.pack_into('!I',buf,4*3,nunsub)
		p.payload = str(buf[0:4*(nunsub+4)])
		p.length = OVERHEAD + 4*(nunsub+4);
		p.type = SUB_UNSUB
		p.comtree = self.comtree;
		p.srcAdr = self.myFadr; p.dstAdr = self.rtrFadr
		self.send(p)

	def updateStatus(self) :
		""" Update position and heading of avatar.

		Also, update multicast subscriptions.
		"""
		self.x, self.y, self.z, self.direction = \
					self.myAvatar.getPosHeading()
		self.mcg.updateSubs(self.x,self.y)

####		self.msg = self.myAvatar.getMsg()
####		if self.msg != '':
####			self.sendMsg(self.msg)
####			self.msg = ''

		# self.callRequest = self.myAvatar.getCallRequest()
		# if self.callRequest != 0:
		# 	self.sendCallRequest(self.callRequest)
		# 	self.callRequest = 0

		return
	
	def processPkt(self, p) :
		""" Process a packet from a remote.

		Currently, this just updates the map of remotes.
		Could be extended to change the state of this avatar
		(say, process a "kill packet")
		"""
		# for now just ignore non-data packets
#		print self.now, self.myAdr, \
#		      "processPkt", self.rtrAdr
#		print p.toString()
#		if p.type == CLIENT_SIG :
#			cp = CtlPkt(0,0,0)
#			cp.unpack(p.payload)
#			print cp.toString()
#		sys.stdout.flush()

		if p.type != CLIENT_DATA : return

		tuple = struct.unpack('!I',p.payload[0:4])
		if tuple[0] == STATUS_REPORT : self.processReport(p)
		elif tuple[0] == AUDIO_PKT : self.processAudio(p)
		elif tuple[0] == MESSAGE : self.processTextMessage(p)

	def processReport(self, p) :
		""" Process a status report packet """

		# extract position, direction, avatar model number and id
		tuple = struct.unpack('!6I',p.payload[4:28])
		x1 = (tuple[1]+0.0)/GRID
		y1 = (tuple[2]+0.0)/GRID
		z1 = (tuple[3]+0.0)/GRID
		dir1 = tuple[4]
		avaModel = tuple[5]
		
		avId = p.srcAdr

		# extract remote user's name
		tuple = struct.unpack('!I', p.payload[28:32])
		namelen = tuple[0]
		tuple = struct.unpack(str(namelen) + 's', \
					p.payload[32: 32+namelen])
		name = ""
		if namelen != 0: name = tuple[0]

		# now, get audio energy level and update remote speaker
		tuple = struct.unpack('!Ii',p.payload[32+namelen:40+namelen])
		rms = tuple[0]
		remoteSpeaker = p.srcAdr
		self.updateRemoteSpeaker(x1,y1,rms,p.srcAdr)

		if avId in self.nearRemotes :
			# update map entry for this remote
			remote = self.nearRemotes[avId]
			remote[4] = x1 - remote[0]
			remote[5] = y1 - remote[1]
			remote[6] = z1 - remote[2]
			remote[0] = x1
			remote[1] = y1
			remote[2] = z1
			remote[3] = dir1
			remote[7] = 0
			self.myAvatar.updateRemote(x1,y1,z1, \
						    dir1, avId, name)
		elif len(self.nearRemotes) < MAXNEAR :
			# fadr -> x,y,z,direction,dx,dy,dz,count
			self.nearRemotes[avId] = [x1,y1,z1,dir1,0,0,0,0]
			self.myAvatar.addRemote(x1, y1, z1, dir1,
						 avId, name, avaModel)
	###		savedPath = os.getcwd()
	###		os.chdir("photo_cache")
	###		self.getPhoto(name)
	###		os.chdir(savedPath)

	def updateRemoteSpeaker(self, x, y, rms, remoteSpeaker) :
		""" Determine apparent sound energy level for remoteSpeaker
		    and if louder than that of current remote speaker
		    then make this the new remote speaker

		x,y gives position of remote speaker
		rms is reported energy level of remote speaker
		remoteSpeaker is identifier for remoteSpeaker (forest address)
		"""
		
		# determine if we should be listening to this speaker or not
		man_distance = math.sqrt(math.pow(x-self.x,2) + 
					 math.pow(y-self.y,2))+1 
		rms = rms/man_distance	
		#print "rms" + str(rms)
		if rms > 100 and rms <1000: 	audiolvl = 1
		elif rms > 1000 : 		audiolvl = 2
		else : 				audiolvl = 0
		#print "audiolvl:" + str(audiolvl)

		if self.remoteAudioLevel < audiolvl :
			self.remoteSpeaker = remoteSpeaker
			self.remoteAudioLevel = audiolvl
		elif audiolvl == 0 and self.remoteAudioLevel > 0 :
			if remoteSpeaker == self.remoteSpeaker:
				self.remoteSpeaker = None
				self.remoteAudioLevel = 0	

	def processAudio(self, p) :
		""" Extract audio from packet and play it out. """
		# extract position, direction, avatar model number and id

		tuple = struct.unpack('!3I',p.payload[4:16])
		sendTime = tuple[0]/1000000.0
		x1 = (tuple[1]+0.0)/GRID
		y1 = (tuple[2]+0.0)/GRID

		man_distance = math.sqrt(math.pow(x1-self.x,2) +
					 math.pow(y1-self.y,2))+1 
		#print "man_dis:" + str(man_distance)

		tuple = struct.unpack('!' + str(BYTES) + 's',
					p.payload[16:BYTES+16])
		if self.needtoWait == True:
			if len(self.audioBuffer) < AUDIO_BUFFER_SIZE :	
				data = audioop.mul(tuple[0],2,1/man_distance)
				self.audioBuffer.append(data)
			else:
				self.needtoWait = False
		else:	
			data = audioop.mul(tuple[0],2,2/man_distance)
			self.audioBuffer.append(data)
		return

	def processTextMessage(self,p) :
		tuple = struct.unpack('!I', p.payload[4:8])
		namelen = tuple[0]
		if namelen != 0:
			name = struct.unpack(str(namelen) + 's', 
					     p.payload[8: 8+namelen])[0]

		tuple = struct.unpack('!I', payload[8+namelen: 12+namelen])
		msglen = tuple[0]
		if msglen != 0:
			msg = struct.unpack(str(msglen) + 's',
				p.payload[12+namelen: 12+namelen+msglen])[0]
			self.myAvatar.displayMsg(msg, name)

	def pruneNearby(self) :
		""" Remove old entries from nearRemotes

		If we haven't heard from a remote in a while, we remove
		it from the map of nearby remotes. When a remote first
		"goes missing" we continue to update its position,
		by extrapolating from last update.
		"""
		dropList = []
		# nearRemote: fadr -> x,y,z,direction,dx,dy,dz,count
		for avId, remote in self.nearRemotes.iteritems() :
			if remote[7] > 0 : # implies missing update
				# update position based on dx, dy
				remote[0] += remote[4]
				remote[1] += remote[5]
				remote[2] += remote[6]
				remote[0] = max(0,remote[0])
				remote[0] = min(self.limit,remote[0])
				remote[1] = max(0,remote[1])
				remote[1] = min(self.limit,remote[1])
				remote[2] = max(0,remote[2])
				remote[2] = min(self.limit,remote[2])
			if remote[7] >= 6 :
				dropList.append(avId)
			remote[7] += 1
		for avId in dropList :
			if avId == self.remoteSpeaker:
				self.remoteSpeaker = None
				self.remoteAudioLevel = 0			
			rem = self.nearRemotes[avId]
			del self.nearRemotes[avId]
			self.myAvatar.removeRemote(avId)
