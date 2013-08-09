""" Miscellaneous utility functions.  """

AUTO = False	# flag for enabling automatic mode
SHOW = False	# flag for enabling display
AUDIO = False   # flag for enabling/disabling audio features
		# to use audio, you must have pyAudio and portAudio installed
DEBUG = 0	# controls amount of debugging output

def ip2string(ip) :
	""" Convert an integer IP address to a string.
	
	ip is an integer that represents an IP address
	return the string representing the value of ip in dotted decimal
	notation
	"""
	return	str((ip >> 24) & 0xff) + "." + str((ip >> 16) & 0xff) + "." + \
		str((ip >>  8) & 0xff) + "." + str(ip & 0xff)

def string2ip(ipstring) :
	""" Convert a dotted decimal IP address string to an integer IP address.

	ipstring is a dotted decimal string
	return the corresponding integer IP address
	"""
	parts = ipstring.split(".")
	if len(parts) != 4 : return 0
	return  (int(parts[0]) & 0xff) << 24 | (int(parts[1]) & 0xff) << 16 | \
		(int(parts[2]) & 0xff) << 8  | (int(parts[3]))

def fadr2string(fa) :
	""" Convert an integer forest address to a string.
	
	fa is an integer that represents an IP address
	return the string representing the value of fa in dotted decimal
	notation for unicast addresses and as an integer for multicast
	"""
	if fa < 0 : return str(fa)
	else : return str((fa >> 16) & 0xffff) + "." + str(fa & 0xffff)

def string2fadr(fas) :
	""" Convert a string representation of a forest address to an integer.

	fas is a string representation of a forest address
	return the corresponding integer forest address
	"""
	if fas[0] == '-' : return int(fas)
	parts = fas.split(".")
	if len(parts) != 2 : return 0
	return  (int(parts[0]) & 0xffff) << 16 | (int(parts[1]) & 0xffff)
