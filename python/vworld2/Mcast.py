import sys
import direct.directbase.DirectStart
from panda3d.core import *
from direct.task import Task

class Mcast :
	""" Multicast group management.

	"""
	#dwkim
	def __init__(self, actualRegionSizeX, actualRegionSizeY, subLimit, net, pWorld):
	#def __init__(self, numg, subLimit, net, pWorld):
		""" Initialize a new Mcast object.

		numg*numg is the number of multicast groups used to
		share status info
		subLimit limits number of multicast groups we subscribe to;
		we subscribe only if x-distance + y-distance is <= subLimit
		net is a reference to the Net object
		pWorld is a reference to the PandaWorld object
		"""

		# self.numg = numg
		self.subLimit = subLimit
		self.net = net
		self.pWorld = pWorld
		#self.limit = pWorld.getLimit()
		#dwkim
		self.actualRegionSizeX = actualRegionSizeX
		self.actualRegionSizeY = actualRegionSizeY
		#self.cellSize = (self.limit + 0.0)/self.numg

		# create array of visibility sets, one per cell
		#dwkim
		self.numOfRegionX = int(pWorld.modelSizeX / self.actualRegionSizeX)
		self.numOfRegionY = int(pWorld.modelSizeY / self.actualRegionSizeY)
		#self.vsets = [ None ] * (self.numg*self.numg)
		self.vsets = [ None ] *  self.numOfRegionX * self.numOfRegionY

		# list of vsets whose visibility range requires refinement
		# vsets are added as new cells are visited
		self.refineList = []

		self.mySubs = set()	# multicast subscriptions
		self.row = self.col = -1 # initial dummy values to force 
					 # first subscription

		taskMgr.add(self.refine, "mcastTask")

	def groupNum(self, x, y) :
		""" Return multicast group number for a given (x,y) location.

		(x,y) are PandaWorld coordinates
		return the number of the multicast group for an avatar at
		that location
		"""
		#dwkim
		return int(x / self.actualRegionSizeX) + self.numOfRegionX * int(y / self.actualRegionSizeY) + 1
		#return int(x/self.cellSize) + self.numg*int(y/self.cellSize) + 1

	def newVset(self, c) :
		""" Create and initialize a vset for a cell.

		A vset defines the cells that are visible from a given cell.
		Vset[c] includes two arrays, lo and hi, where lo[i]..hi[i]
		specifies the cells in column i that are potentially
		visible from cell c. The arrays have length 2*subLimit+1.
		The center entry in each array corresponds to the column
		of cell c, so lo[subLimit]..hi[subLimit] specifies the visible
		cells directly to the "south" and "north" of cell c.

		A vset also includes two auxiliary variables i and side.
		These are used to control the background refinement of
		the values of lo and hi, to reduce the number of cells
		that are considered visible. This is done over time,
		to avoid slowing the interactive graphics with a lot
		of time-consuming visibiity calculations.
		"""

		#dwkim
		# col = c % self.numg
		# row = c / self.numg
		col = c % self.numOfRegionX
		row = c / self.numOfRegionY

		lo = [row+1] * (2*self.subLimit+1) # low ends of column ranges
		hi = [row] * (2*self.subLimit+1)   # high ends of column ranges

		i0 = self.subLimit # entry in lo/hi corresponding to col

		# set initial visibility ranges using subLimit alone
		lo[i0] = max(0, row-self.subLimit) 
		hi[i0] = min(self.numOfRegionY-1, row+self.subLimit) 
		for i in range(1,self.subLimit+1) :
			if col-i >= 0 :
				lo[i0-i] = max(0,row-(self.subLimit-i))
				hi[i0-i] = min(self.numOfRegionY-1, row+self.subLimit-i)
			if col+i < self.numOfRegionX :
				lo[i0+i] = max(0, row-(self.subLimit-i))
				hi[i0+i] = min(self.numOfRegionY-1, row+self.subLimit-i)

		# add new vset to array of vsets, with i=side=0
		# and append it to the refineList, so we can start
		# working on better values for lo and hi, as time permits
		self.vsets[c] = [0,0,lo,hi]
		self.refineList.append(c)
		return self.vsets[c]

	def refine(self, task) :
		""" This is the main method for the Mcast object.
		
		Its job is simply to refine the visibility sets incrementally,
		so as to limit the computational load due to visibility
		calculations. It is called by the task manager each frame time.
		"""

		if len(self.refineList) == 0 : return task.cont

		c = self.refineList[-1]
		i,side,lo,hi = self.vsets[c]
			# c is index of current visibility set to refine
			# i is index of lo/hi entry in c to refine next
		    	# side=0 if we're refining lo[i]
		    	# side=1 if we're refining hi[i]

		col = c % self.numOfRegionX 
		row = c / self.numOfRegionY
		i0 = self.subLimit	# offset in lo/hi matching c's column

		ccol = col + (i-i0)	# current column to refine
		if ccol < 0:
			i = i0 - col
			ccol = 0
			self.vsets[c][0] = i
		elif ccol >= self.numOfRegionX :
			self.vsets[c][0] = 0; self.vsets[c][1] = 0
			self.refineList.pop();
			return task.cont

		if side == 0 :
			crow = lo[i]
			cc = ccol + crow * self.numOfRegionX + 1 #dwkim +1 is my guess
			if crow <= row and not self.isVis(c,cc):
				lo[i] += 1
				return task.cont
			self.vsets[c][1] = 1
		else :
			crow = hi[i]
			cc = ccol + crow * self.numOfRegionX + 1 #dwkim +1 is my guess
			if crow > row and not self.isVis(c,cc) :
				hi[i] -= 1; return task.cont
			if i < (2 * self.subLimit):
				self.vsets[c][0] = i + 1
				self.vsets[c][1] = 0
			else :
				self.vsets[c][0] = 0
				self.vsets[c][1] = 0
				self.refineList.pop();
		return task.cont

	def isVis(self, c1, c2) :
		""" Determine if two squares are visible from each other.

	   	c1 is the cell number of the first square
	   	c2 is the cell number of the second square
		"""
		# cellSize = self.cellSize

		x1 = float(self.actualRegionSizeX * (c1%self.numOfRegionX))
		y1 = float(self.actualRegionSizeY * (c1/self.numOfRegionY))
		x2 = float(self.actualRegionSizeX * (c2%self.numOfRegionX))
		y2 = float(self.actualRegionSizeY * (c2/self.numOfRegionY))

		# define points near cell corners in panda coordinates
		points1 = ( \
			Point3(x1+.001*self.actualRegionSizeX, y1+.001*self.actualRegionSizeY,0), \
			Point3(x1+.001*self.actualRegionSizeX, y1+.999*self.actualRegionSizeY,0), \
			Point3(x1+.999*self.actualRegionSizeX, y1+.999*self.actualRegionSizeY,0), \
			Point3(x1+.999*self.actualRegionSizeX, y1+.001*self.actualRegionSizeY,0))
		points2 = ( \
			Point3(x2+.001*self.actualRegionSizeX, y2+.001*self.actualRegionSizeY,0), \
			Point3(x2+.001*self.actualRegionSizeX, y2+.999*self.actualRegionSizeY,0), \
			Point3(x2+.999*self.actualRegionSizeX, y2+.999*self.actualRegionSizeY,0), \
			Point3(x2+.999*self.actualRegionSizeX, y2+.001*self.actualRegionSizeY,0))

		# Check sightlines between all pairs of "corners"
		for p1 in points1 :
			for p2 in points2 :
				if self.pWorld.canSee(p1,p2) :
					return True
		return False

	def updateSubs(self,x, y) :	
		""" Update multicast group subscriptions.

		x, y is the current coordinates of the avatar
		"""
		#dwkim
		# col = int(x/self.cellSize); row = int(y/self.cellSize)
		col = int(x / self.actualRegionSizeX)
		row = int(y / self.actualRegionSizeY)

		if col == self.col and row == self.row : return
		self.col = col; self.row = row

		# drop all current subscriptions - brute force, but effective
		self.net.unsubscribe(self.mySubs); self.mySubs.clear()

		c = col + row * self.numOfRegionX #weird-doublecheck if +1 
		vs = self.vsets[c]
		if vs == None : vs = self.newVset(c)

		lo = vs[2]; hi = vs[3]

		# identify all cells considered visible and subscribe to them
		left = max(0, col-self.subLimit)
		right = min(self.numOfRegionX-1, col+self.subLimit)
		for x in range(left,right+1):
			bot = lo[self.subLimit + (x-col)]
			top = hi[self.subLimit + (x-col)]
			for y in range(bot, top+1):
				self.mySubs.add(y * self.numOfRegionX + x + 1)

		self.net.subscribe(self.mySubs)
