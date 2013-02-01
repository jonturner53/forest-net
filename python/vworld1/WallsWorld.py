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
		except error as e :
			sys.stderr.write("WallsWorld: cannot open " + wallsFile)
			return False
		y = 0;
		lines = f.readlines()
		self.size = len(lines)
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

		# l1 is a line connecting the given pair of points
		l1 = (p1,p2)
		slope = (p2[1]-p1[1])/(p2[0]-p1[0])

		i = i1; j = j1;
		x = p1[0]; y = p1[1]
		xd = i1+1 - x; yd = xd*slope
		while i < i2 :
			while j+1 < y+yd :
				if self.walls[i+j*self.size]&2 :
					return False
				j += 1
			if self.walls[(i+1)+j*self.size]&1 :
				return False
			while j+1 < y+slope :
				c = (i+1)+j*self.size
				if c < 0 or c >= len(self.walls) :
					print "bad pair i=",i,"j=",j,\
						"size=",self.size,\
						"len=",len(self.walls)
				if self.walls[c]&2 :
					return False
				j += 1
			i += 1; x += 1.0; y += slope
		return True
	
	def linesIntersect(self,l1,l2) :
		epsilon = .001; # two lines are considered vertical if
				# x values differ by less than epsilon

		# unpack coordinates of line endpoints
		pa = l1[0]; pb = l1[1]
		ax = pa[0]; ay = pa[1]; bx = pb[0]; by = pb[1]

		pc = l2[0]; pd = l2[1]
		cx = pc[0]; cy = pc[1]; dx = pd[0]; dy = pd[1]

		# first handle special case of two vertical lines
		if abs(ax-bx) < epsilon and abs(cx-dx) < epsilon :
			return	abs(ax-cx) < epsilon and \
				max(ay,by) >= min(cy,dy) and \
			      	min(ay,by) <= max(cy,dy)
		# now handle cases of one vertical line
		if abs(ax-bx) < epsilon :
			s2 = (dy-cy)/(dx-cx)  	# slope of second line
			i2 = cy - s2*cx  	# y-intercept of second line
			y = s2*ax + i2	  	# lines intersect at (ax,y)
			return  (y >= min(ay,by) and y <= max(ay,by) and \
				 y >= min(cy,dy) and y <= max(cy,dy))
		if abs(cx-dx) < epsilon :
			s1 = (by-ay)/(bx-ax)	# slope of first line
			i1 = ay - s1*ax		# y-intercept of first line
			y = s1*cx + i1		# lines intersect at (cx,y)
			return  (y >= min(ay,by) and y <= max(ay,by) and \
				 y >= min(cy,dy) and y <= max(cy,dy))

		s1 = (by-ay)/(bx-ax)	# slope of first line
		i1 = ay - s1*ax 	# y-intercept of first line
		s2 = (dy-cy)/(dx-cx)	# slope of second line
		i2 = cy - s2*cx 	# y-intercept of second line
	
		# handle special case of lines with equal slope
		# treat the slopes as equal if both slopes have very small
		# magnitude, or if their relative difference is small
		if abs(s1)+abs(s2) <= epsilon or \
		   abs(s1-s2)/(abs(s1)+abs(s2)) < epsilon :
			return (abs(i1-i2) < epsilon and \
				min(ax,bx) <= max(cx,dx) and \
				max(ax,bx) >= min(cx,dx))
		# now, to the common case
		x = (i2-i1)/(s1-s2)	# x value where the lines intersect
		return	x >= min(ax,bx) and x <= max(ax,bx) and \
			x >= min(cx,dx) and x <= max(cx,dx)
