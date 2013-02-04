import sys
import struct
from socket import *
from Util import *

# request/return field types
RR_REQUEST=1		# signifies request packet
RR_POS_REPLY=2		# signifies positive reply
RR_NEG_REPLY=3		# signifies positive reply

# selected control packet types
CLIENT_JOIN_COMTREE=14
CLIENT_LEAVE_COMTREE=15

# selected attribute codes
COMTREE_NUM=4
CLIENT_IP=34
CLIENT_PORT=43

class CtlPkt:
	""" Class for working with selected Forest control packets.  """

	def __init__(self,cpTyp,rrTyp,seqNum) :
		""" Constructor for CtlPkt objects.
		
		"""
		self.cpTyp = cpTyp
		self.rrTyp = rrTyp
		self.seqNum = seqNum

		self.comtree = 0
		self.clientIp = 0
		self.clientPort = 0
		self.errMsg = None

	def pack(self) :
		""" Pack control packet into a buffer and return it.
		"""
		buf = struct.pack('!IIQ', \
				  self.cpTyp, self.rrTyp, self.seqNum)
		if (self.cpTyp == CLIENT_JOIN_COMTREE or \
		    self.cpTyp == CLIENT_LEAVE_COMTREE) and \
		    self.rrTyp == RR_REQUEST :
			if self.comtree == 0 or self.clientIp == 0 or \
			   self.clientPort == 0 :
				sys.stderr.write("CtlPkt.pack: missing " + \
					"required attribute(s)");
				return None
			buf += struct.pack('!IIIIII',
				  COMTREE_NUM,\
				  self.comtree & 0xffffffff, \
				  CLIENT_IP,\
				  self.clientIp & 0xffffffff, \
				  CLIENT_PORT, \
				  self.clientPort & 0xffff)
		else :
			sys.stderr.write("CtlPkt.pack: unimplemented " + \
				"control packet type\n" + self.toString() + \
				"\n")
			return None
		return buf

	def unpack(self,buf) :
		""" Unpack attributes from a provided buffer.

		This version just handles replies to control packets.
		"""
		tuple = struct.unpack('!IIQ', buf[0:16])
		if len(tuple) != 3 : return False
		self.cpTyp = tuple[0]
		self.rrTyp = tuple[1]
		self.seqNum = tuple[2]

		buf = buf[16:]
		if self.cpTyp == CLIENT_JOIN_COMTREE or \
		   self.cpTyp == CLIENT_LEAVE_COMTREE :
			if self.rrTyp != RR_NEG_REPLY : return True
			tuple = struct.unpack('!' + str(len(buf)) + 's',buf)
			if len(tuple) != 1 : return False
			self.errMsg = tuple[0]
			return True
		else :
			sys.stderr.write("CtlPkt.unpack: unimplemented " + \
				"control packet type\n" \
				+ self.toString() + "\n")
			return None

	def toString(self) :
		s = ""
		if self.cpTyp == CLIENT_JOIN_COMTREE :
			s += "client join comtree"
		elif self.cpTyp == CLIENT_LEAVE_COMTREE :
			s += "client join comtree"
		else :
			s += "unknown type"

		if self.rrTyp == RR_REQUEST : s += " (request)"
		elif self.rrTyp == RR_POS_REPLY : s += " (positive reply)"
		elif self.rrTyp == RR_NEG_REPLY : s += " (negative reply)"
		else : s += " (unknown type)"

		s += " seqNum=" + str(self.seqNum)
		if self.comtree != 0 :
			s += " comtree=" + str(self.comtree)
		if self.clientIp != 0 :
			s += " clientIp=" + ip2string(self.clientIp)
		if self.clientPort != 0 :
			s += " clientPort=" + str(self.clientPort)
		if self.errMsg != None :
			s += " errMsg=" + self.errMsg

		return 	s
