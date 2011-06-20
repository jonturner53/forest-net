package remoteDisplay;

/** Class used to record the status of an Avatar.
 *  @author Jon Turner
 */
class MazeAvatarStatus {
	// note: all fields visible within package
	int id;		// identifier - based on Avatar's Forest address
	int when;	// timestamp in ms
	double x;	// Avatar's x position
	double y;	// Avatar's y position
	double dir;	// direction facing (in degrees)
	int numVisible;  // number of visible Avatars and includes the number of nearby avatars
	int numNear;	// number of nearby Avatars
	int comtree;	// comtree that report relates to
	int type;	// 1 - self, 2 - nearby to controller, 3 - visible to controller
	
	/** Copy contents of another AvatarStatus object to this one.
	 *  @param other is the object whose contents is to be copied
	 *  to this one.
	 */
	public void copyFrom(MazeAvatarStatus other) {
		id = other.id; when = other.when;
		x = other.x; y = other.y;
		dir = other.dir;
		numVisible = other.numVisible;
		numNear = other.numNear;
		comtree = other.comtree;
		type = other.type;
	}

	/** @return String representing this object. */
	public String toString() {
		return ((id >> 16) & 0xffff) + "." + (id & 0xffff) +
			" " + when + " (" + x + "," + y + ")/" + dir + " " + numVisible + 
			" " + numNear;
	}
}
