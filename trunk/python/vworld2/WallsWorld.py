import sys

MAX_VIS = 20

class WallsWorld :
	""" Maintain information about walls.
	"""
	def __init__(self) :
		""" Initialize a new WallsWorld object.
		"""
		pass

	def init(self, wallsFile) :
		f = None
		try :
			f = open(wallsFile, 'r')
		except error,  e :
			sys.stderr.write("WallsWorld: cannot open " + wallsFile)
			return False
		y = 0;
		lines = f.readlines()
		self.size = len(lines)
		print "self.size=", self.size
		y = self.size - 1
		self.walls = [0] * (self.size*self.size)
		for line in lines :
			if len(line) != 1+len(lines) :
				sys.stderr.write(str(len(line)) + " " + \
						 str(len(lines)) + "\n")
				sys.stderr.write("Core.setupWorld: length of " \
					"line " + str(self.size-y) + \
					" does not match line count\n")
				return False
			for x in range(self.size) :
				if  line[x] == '+' :
					self.walls[y*self.size + x] = 3;
				elif line[x] == '-' :
					self.walls[y*self.size + x] = 2;
				elif line[x] == '|' :
					self.walls[y*self.size + x] = 1;
				elif line[x] == ' ' :
					self.walls[y*self.size + x] = 0;
				else :
					sys.write.stderr("Core.setupWorld: " + \
						"unrecognized symbol in " + \
						"map file!\n")
					return False
			y -= 1
		return True

	def separated(self,c0,c1) :
		""" Determine if two adjacent cells are separated by a wall.
		c0 is index of some cell
		c1 is index of an adjacent cell
		"""
		if c0 > c1 : c0, c1 = c1, c0
		if c0/self.size == c1/self.size : # same row
			return self.walls[c1] & 1
		elif c0%self.size == c1%self.size : # same column
			return self.walls[c0] & 2
		elif c0%self.size > c1%self.size : # se/nw diagonal
			if self.walls[c0] == 3 or \
			   (self.walls[c0]&1 and self.walls[c1+1]&1) or \
			   (self.walls[c0]&2 and self.walls[c0-1]&2) or \
			   (self.walls[c1+1]&1 and self.walls[c0-1]&2) :
				return True
			else :
				return False
		else : # sw/ne diagonal
			if (self.walls[c0]&2 and self.walls[c0+1]&1) or \
			   (self.walls[c0+1]&1 and self.walls[c1]&1) or \
			   (self.walls[c0]&2 and self.walls[c0+1]&2) or \
			   (self.walls[c0+1]&2 and self.walls[c1]&1) :
				return True
			else :
				return False

	def updateVisSet(self,c0,c1,vSet0) :
		""" Update a visibility set.
		
		For an avatar moving from cell c0 to c1, compute new
		visibility set to replace old one.
		For now, just compute from scratch.
		"""
		return self.computeVisSet(c1)

	def computeVisSet(self,c1) :
		""" Compute a visibility set for a square in the virtual world.

			c1 is cell number for a square in virtual world
	 	"""
		x1 = c1%self.size; y1 = c1/self.size;
		vSet = set();
		vSet.add(c1)
	
		visVec = [False] * self.size
		prevVisVec = [False] * self.size
	
		# start with upper right quadrant
		prevVisVec[0] = True
		dlimit = 1 + min(self.size,MAX_VIS);
		for d in range(1,dlimit) :
			done = True
			for x2 in range(x1,min(x1+d+1,self.size)) :
				dx = x2 - x1; y2 = d + y1 - dx 
				if y2 >= self.size : continue
				visVec[dx] = False
				if (x1==x2 and not prevVisVec[dx]) or \
				   (x1!=x2 and y1==y2 and \
						not prevVisVec[dx-1]) \
				   or (x1!=x2 and y1!=y2 and \
						not prevVisVec[dx-1] \
				   and not prevVisVec[dx]) :
					continue
				c2 = x2 + y2*self.size
	
				if self.isVis(c1,c2) :
					visVec[dx] = True
					vSet.add(c2)
					done = False
			if done : break
			for x2 in range(x1,min(x1+d+1,self.size)) :
				prevVisVec[x2-x1] = visVec[x2-x1]
		# now process upper left quadrant
		prevVisVec[0] = True;
		for d in range(1,dlimit) :
			done = True;
			for x2 in range(max(x1-d,0),x1+1) :
				dx = x1-x2; y2 = d + y1 - dx
				if y2 >= self.size : continue
				visVec[dx] = False;
				if (x1==x2 and not prevVisVec[dx]) or \
				   (x1!=x2 and y1==y2 and \
						not prevVisVec[dx-1]) or \
				   (x1!=x2 and y1!=y2 and \
						not prevVisVec[dx-1] and \
						not prevVisVec[dx]) :
					continue
				c2 = x2 + y2*self.size;
				if self.isVis(c1,c2) :
					visVec[dx] = True
					vSet.add(c2)
					done = False
			if done : break
			for x2 in range(max(x1-d,0),x1+1) :
				dx = x1-x2; prevVisVec[dx] = visVec[dx]
		# now process lower left quadrant
		prevVisVec[0] = True;
		for d in range(1,dlimit) :
			done = True;
			for x2 in range(max(x1-d,0),x1+1) :
				dx = x1-x2; y2 = (y1 - d) + dx
				if y2 < 0 : continue
				visVec[dx] = False
				if (x1==x2 and not prevVisVec[dx]) or \
				   (x1!=x2 and y1==y2 and \
						not prevVisVec[dx-1]) or \
				   (x1!=x2 and y1!=y2 and \
						not prevVisVec[dx-1] and \
						not prevVisVec[dx]) :
					continue
				c2 = x2 + y2*self.size;
				if self.isVis(c1,c2) :
					visVec[dx] = True
					vSet.add(c2)
					done = False
			if done : break
			for x2 in range(max(x1-d,0),x1+1) :
				dx = x1-x2; prevVisVec[dx] = visVec[dx]
		# finally, process lower right quadrant
		prevVisVec[0] = True;
		for d in range(1,dlimit) :
			done = True;
			for x2 in range(x1,min(x1+d+1,self.size)) :
				dx = x2-x1; y2 = (y1 - d) + dx
				if y2 < 0 : continue
				visVec[dx] = False
				if (x1==x2 and not prevVisVec[dx]) or \
				   (x1!=x2 and y1==y2 and \
						not prevVisVec[dx-1]) or \
				   (x1!=x2 and y1!=y2 and \
						not prevVisVec[dx-1] and \
						not prevVisVec[dx]) :
					continue
				c2 = x2 + y2*self.size
				if self.isVis(c1,c2) :
					visVec[dx] = True
					vSet.add(c2)
					done = False
			if done : break
			for x2 in range(x1,min(x1+d+1,self.size)) :
				dx = x1-x2; prevVisVec[dx] = visVec[dx]
		return vSet

	def isVis(self, c1, c2) :
		""" Determine if two squares are visible from each other.

	   	c1 is the cell number of the first square
	   	c2 is the cell number of the second square
		"""
		i1 = c1%self.size; j1 = c1/self.size;
		i2 = c2%self.size; j2 = c2/self.size;

		eps = .001
		points1 = ((i1+eps,  j1+eps),   (i1+eps,  j1+1-eps),
			   (i1+1-eps,j1+eps),   (i1+1-eps,j1+1-eps))
		points2 = ((i2+eps,  j2+eps),   (i2+eps,  j2+1-eps),
			   (i2+1-eps,j2+eps),   (i2+1-eps,j2+1-eps))
		for p1 in points1 :
			for p2 in points2 :
				if self.canSee(p1,p2) : return True
		return False

	def canSee(self,p1,p2) :
		""" Determine if two points are visible to each other.

		p1 and p2 are (x,y) pairs representing points in the world
		"""
		if p1[0] > p2[0] : p1,p2 = p2,p1

		i1 = int(p1[0]); j1 = int(p1[1])
		i2 = int(p2[0]); j2 = int(p2[1])
		minj = min(j1,j2); maxj = max(j1,j2)

		if i1 == i2 :
			for j in range(minj,maxj) :
				if self.walls[i1+j*self.size]&2 :
					return False
			return True

		slope = (p2[1]-p1[1])/(p2[0]-p1[0])

		i = i1; j = j1;
		x = p1[0]; y = p1[1]
		xd = i1+1 - x; yd = xd*slope
		c = i+j*self.size
		while i < i2 :
			if slope > 0 :
				while j+1 < y+yd :
					if self.walls[c]&2 : return False
					j += 1; c += self.size
			else :
				while j > y+yd :
					j -= 1; c -= self.size
					if self.walls[c]&2 : return False
			c += 1
			if self.walls[c]&1 : return False
			if slope > 0 :
				ylim = min(y+slope, p2[1])
				while j+1 < ylim :
					if self.walls[c]&2 : return False
					j += 1; c += self.size
			else :
				ylim = max(y+slope, p2[1])
				while j > ylim :
					j -= 1; c -= self.size
					if self.walls[c]&2 : return False
			i += 1; x += 1.0; y += slope
		return True
