package mygame;

/**
 *
 * @author Logan Stafman
 */
public class AvatarStatus {

    int id;         // identifier - based on Avatar's Forest address
    int when;       // timestamp in ms
    double x;       // Avatar's x position
    double y;       // Avatar's y position
    double dir;     // direction facing (in degrees)
    int numVisible; // number of visible Avatars
    int numNear;    // number of nearby Avatars
    int comtree;    // comtree that report relates to

    /** Copy contents of another AvatarStatus object to this one.
     *  @param other is the object whose contents is to be copied
     *  to this one.
     */
    public void copyFrom(AvatarStatus other) {
        id = other.id;
        when = other.when;
        x = other.x;
        y = other.y;
        dir = other.dir;
        numVisible = other.numVisible;
        numNear = other.numNear;
        comtree = other.comtree;
    }

    /** @return String representing this object. */
    @Override
    public String toString() {
        return ((id >> 16) & 0xffff) + "." + (id & 0xffff)
                + " " + when + " (" + x + "," + y + ")/" + dir + " "
                + numVisible + " " + numNear;
    }
}
