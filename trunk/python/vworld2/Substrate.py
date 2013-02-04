""" Substrate for dealing with sockets.

	This class implements a thread that handles socket IO.
	It provides send and receive methods that can be used by
	the Core thread to send and receive packets.
	It also maintains some statistics and prints a short report
	at the end of the run.
"""

import sys
from socket import *
from Queue import Queue
from threading import *
from time import time, sleep
from Util import *
from Packet import *
from CtlPkt import *

class Substrate(Thread) :
	""" Initialize a new Substrate object.

		myAdr is the IP address/port number pair to bind to the socket
		cliMgrAdr is the IP address/port pair for the client manager
		debug is an integer; if it is 1, each control packet sent
		and received is printed out; if it is 2, data packets are
		also printed
	"""
	def __init__(self, myIp, bitRate, pktRate, debug) :
		Thread.__init__(self)
		self.myIp = myIp
		self.byteTime = .008/bitRate; self.pktTime = 1/pktRate
		self.debug = debug

		# open and configure socket to be nonblocking
		self.sock = socket(AF_INET, SOCK_DGRAM);
		self.sock.bind((myIp,0))
		self.sock.setblocking(0)
		self.myAdr = self.sock.getsockname()

		# initialize queues for packets to/from upper-level thread
		self.sendQueue = Queue(1000)
		self.recvQueue = Queue(1000)

	def init(self,rtrIp) :
		self.rtrIp = rtrIp
		self.rtrAdr = (ip2string(rtrIp),ROUTER_PORT)
	
	""" This is the main method for the Substrate object.

		The method terminates after several seconds of inactivity.
		It generally keeps running for a short time after the Core
		object has stopped sending new payloads, ensuring that all
		submitted packets can be delivered and acknowledged.
	"""
	def run(self) :
		t0 = time(); now = 0;

		# initialize statistics
		sendCount = recvCount = 0

		self.quit = False
		nextSendTime = now
		while not self.quit :
			now = time() - t0; nothing2do = 1
			# attempt to receive a packet
			try :
				buf, senderAdr = self.sock.recvfrom(2000)
			except error as e :
				if e.errno != 35 and e.errno != 11 :
					# 11, 35 both imply no data to read
					# treat anything else as fatal error
					sys.stderr.write("Substrate: socket " \
							 + "error: " + str(e))
					sys.exit(1)
			else :
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
					print self.debug,now,self.myAdr,"received from", \
					      self.rtrAdr
					print p.toString()
					if p.type == CLIENT_SIG :
						cp = CtlPkt(0,0,0)
						cp.unpack(p.payload)
						print cp.toString()
					sys.stdout.flush()
				self.recvQueue.put(p)
				recvCount += 1; nothing2do = 0

			# send a packet if we have one and its time has come
			if not self.sendQueue.empty() and now >= nextSendTime :
				p = self.sendQueue.get()
				if self.debug>=3 or \
				   (self.debug>=2 and p.type!=CLIENT_DATA) or \
				   (self.debug>=1 and p.type==CLIENT_SIG) :
					print self.debug,now,self.myAdr,"sending to", \
					      self.rtrAdr
					print p.toString()
					if p.type == CLIENT_SIG :
						cp = CtlPkt(0,0,0)
						cp.unpack(p.payload)
						print cp.toString()
					sys.stdout.flush()
				self.sock.sendto(p.pack(),self.rtrAdr)
				plen = XOVERHEAD + len(p.payload)
				nextSendTime = now + \
					max(self.pktTime,self.byteTime*plen)
				sendCount += 1; nothing2do = 0

			# if nothing to do, snooze briefly
			if nothing2do :
				sleep(0.001)

		# print short report
		print "Substrate: sent", sendCount, "received", recvCount

	def stop(self) : self.quit = True
			
	""" Used by Core object to send a packet. """
	def send(self, p) : self.sendQueue.put(p)
		
	""" Return true if we are ready to accept a packet. """
	def ready(self) : return not self.sendQueue.full()

	""" Used by Core object to receive an incoming packet. """
	def receive(self) : return self.recvQueue.get()
	
	""" Return true if there is an incoming packet available. """
	def incoming(self) : return not self.recvQueue.empty()
