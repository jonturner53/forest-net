package remoteDisplay;

import java.awt.*;

import princeton.StdDraw;

/** Class used to represent avatars in the graphic display.
 *  Includes methods to track Avatar status and to draw the
 *  avatar on the display.
 */
class AvatarGraphic {
	private AvatarStatus avaStatus;
	private static final double SIZE = .06;
	
	/** Constructor for AvatarGraphic class.
	 *  @param status is a AvatarStatus object that defines the avatar's
	 *  initial status.
	 */
	public AvatarGraphic(AvatarStatus status) {
		avaStatus = new AvatarStatus();
		update(status);
	}
	
	/** Return comtree for the AvatarGraphic. */
	public int getComtree() { return avaStatus.comtree; }
	
	/** Update the avatar's status. */
	public void update(AvatarStatus status) { 
		avaStatus.copyFrom(status);
	}
	
	/** Draw a graphic representation of the Avatar.
	 *  Avataris drawn as a circle with a radius that represents
	 *  its visibility range. A line is drawn from the center to
	 *  the circumference, to show the direction in which the 
	 *  Avatar is moving. The Avatar's Forest address is displayed
	 *  in the upper left quadrant of the circle and the number
	 *  of other Avatars it can "see" is displayed in the bottom
	 *  right quadrant.
	 */
	public void draw() {
		double scaleFactor = 2*Math.PI/360; // for converting degrees to radians
			
		StdDraw.setPenColor(Color.GRAY);
		//StdDraw.circle(avaStatus.x,avaStatus.y,SIZE);		
		StdDraw.line(avaStatus.x, avaStatus.y,
			     avaStatus.x + SIZE*Math.sin(avaStatus.dir*scaleFactor),
			     avaStatus.y + SIZE*Math.cos(avaStatus.dir*scaleFactor));
		
		StdDraw.setPenColor(Color.BLACK);
		StdDraw.filledCircle(avaStatus.x,avaStatus.y,.005);

		StdDraw.text(avaStatus.x-SIZE/3,avaStatus.y+SIZE/3,
			     ((avaStatus.id >> 16) & 0xffff) + "." +
			     (avaStatus.id & 0xffff));
		StdDraw.text(avaStatus.x+SIZE/3,avaStatus.y-SIZE/3,
			    ((Integer) avaStatus.numVisible).toString());
		StdDraw.text(avaStatus.x-SIZE/3,avaStatus.y-SIZE/3,
                            ((Integer) avaStatus.numNear).toString()); 
	}
}
