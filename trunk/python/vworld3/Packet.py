import sys
import struct
from socket import *
from Util import *

# packet type codes
CLIENT_DATA=1		# normal data packet from a host
SUB_UNSUB=2		# subscribe to multicast groups
CLIENT_SIG=10		# client signalling packet
CONNECT=11		# establish connection for an access link
DISCONNECT=12		# disconnect an access link

ROUTER_PORT=30123
COMTCTL_PORT=30133
CLIMGR_PORT=30140

CLIENT_CON_COMT=1
CLIENT_SIG_COMT=2

HDRLENG=20		# number of bytes in Forest header
OVERHEAD=24		# total overhead bytes in Forest packet
XOVERHEAD=70		# extra overhead bytes including IP and Ethernet

class Packet:
	""" Class for working with Forest packets.  """

	def __init__(self) :
		""" Constructor for Packet objects.
		"""
		self.version = 1
		self.length = OVERHEAD
		self.type = 0
		self.flags = 0
		self.comtree = 0
		self.srcAdr = 0
		self.dstAdr = 0
		self.hdrCheck = 0
		self.payload = ""
		self.payCheck = 0

	def pack(self) :
		""" Pack attributes into a buffer and return it.
		"""
		
		plen = len(self.payload)
		self.length = (OVERHEAD + plen) & 0xfff
		self.type &= 0xff; self.flags &= 0xff
		firstWord = (1 << 28) | (self.length << 16) | \
			    self.type << 8 | self.flags
		buf = struct.pack('!IIIII' + str(plen) + 'sI', \
				  firstWord,
				  self.comtree & 0xffffffff, \
				  self.srcAdr & 0xffffffff, \
				  self.dstAdr & 0xffffffff, \
				  self.hdrCheck & 0xffffffff, \
				  self.payload, self.payCheck & 0xffffffff)
		return buf

	def unpack(self,buf) :
		""" Unpack attributes from a provided buffer.
		"""
		plen = len(buf)-OVERHEAD
		tuple = struct.unpack('!IIIiI' + str(plen) + 'sI', buf)
		if len(tuple) != 7 : return False
		self.version = (tuple[0] >> 28) & 0xf
		self.length = (tuple[0] >> 16) & 0xfff
		self.type = (tuple[0] >> 8) & 0xff
		self.flags = tuple[0] & 0xff
		self.comtree = tuple[1]
		self.srcAdr = tuple[2]
		self.dstAdr = tuple[3]
		self.hdrCheck = tuple[4]
		self.payload = tuple[5]
		self.payCheck = tuple[6]
		return	self.length == OVERHEAD + plen and self.type != 0 and \
			self.comtree != 0 and self.srcAdr != 0 and \
			self.dstAdr != 0

	def toString(self) :
		s = ""
		if self.type == CLIENT_DATA : s += "client_data"
		elif self.type == SUB_UNSUB : s += "sub_unsub"
		elif self.type == CLIENT_SIG : s += "client_sig"
		elif self.type == CONNECT : s += "connect"
		elif self.type == DISCONNECT : s += "disconnect"
		else : s += "undef"

		s += " flags=" + str(self.flags) 
		s += " comtree=" + str(self.comtree)
		s += " srcAdr=" + fadr2string(self.srcAdr) 
		s += " dstAdr=" + fadr2string(self.dstAdr)

		return s
