import sys
import struct
from socket import *
from Util import *

# request/return field types
REQUEST=1		# signifies request packet
POS_REPLY=2		# signifies positive reply
NEG_REPLY=3		# signifies positive reply

# selected control packet types
CLIENT_JOIN_COMTREE=14
CLIENT_LEAVE_COMTREE=15

# selected attribute codes
COMTREE=50
IP1=5
PORT1=9

class CtlPkt:
	""" Class for working with selected Forest control packets.  """

	def __init__(self,cpTyp,mode,seqNum) :
		""" Constructor for CtlPkt objects.
		
		"""
		self.cpTyp = cpTyp
		self.mode = mode
		self.seqNum = seqNum

		self.comtree = 0
		self.ip1 = 0
		self.port1 = 0
		self.errMsg = None

	def pack(self) :
		""" Pack control packet into a buffer and return it.
		"""
		buf = struct.pack('!IIQ', \
				  self.cpTyp, self.mode, self.seqNum)
		if (self.cpTyp == CLIENT_JOIN_COMTREE or \
		    self.cpTyp == CLIENT_LEAVE_COMTREE) and \
		    self.mode == REQUEST :
			if self.comtree == 0 :
				sys.stderr.write("CtlPkt.pack: missing " + \
					"required attribute(s)");
				return None
			buf += struct.pack('!II',
				  COMTREE, self.comtree & 0xffffffff)
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
		self.mode = tuple[1]
		self.seqNum = tuple[2]

		buf = buf[16:]
		if self.cpTyp == CLIENT_JOIN_COMTREE or \
		   self.cpTyp == CLIENT_LEAVE_COMTREE :
			if self.mode == POS_REPLY : return True
			if self.mode == NEG_REPLY : 
				tuple = struct.unpack('!' + str(len(buf)) + \
						      's',buf)
				if len(tuple) != 1 : return False
				self.errMsg = tuple[0]
				return True
			# request type
			tuple = struct.unpack('!II',buf)
			if len(tuple) != 2 or tuple[0] != COMTREE :
				return False
			self.comtree = tuple[1]
			return True
		else :
			sys.stderr.write("CtlPkt.unpack: unimplemented " + \
				"control packet type (" + str(self.cpTyp) + \
				")\n" + self.toString() + "\n")
			return None

	def toString(self) :
		s = ""
		if self.cpTyp == CLIENT_JOIN_COMTREE :
			s += "client join comtree"
		elif self.cpTyp == CLIENT_LEAVE_COMTREE :
			s += "client leave comtree"
		else :
			s += "unknown type"

		if self.mode == REQUEST : s += " (request)"
		elif self.mode == POS_REPLY : s += " (positive reply)"
		elif self.mode == NEG_REPLY : s += " (negative reply)"
		else : s += " (unknown type)"

		s += " seqNum=" + str(self.seqNum)
		if self.comtree != 0 :
			s += " comtree=" + str(self.comtree)
		if self.ip1 != 0 :
			s += " ip1=" + ip2string(self.ip1)
		if self.port1 != 0 :
			s += " port1=" + str(self.port1)
		if self.errMsg != None :
			s += " errMsg=" + self.errMsg

		return 	s
