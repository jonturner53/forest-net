package mygame;

import com.jme3.animation.AnimChannel;
import com.jme3.animation.AnimControl;
import com.jme3.asset.AssetManager;
import com.jme3.math.Vector3f;
import com.jme3.scene.Node;

/**
 *
 * @author codersarepeople
 */
public class AvatarGraphic {
    public double x;
    public double y;
    public int comtree;
    public int numVis;
    public int numNear;
    public int id;
    public String fAdr;
    public Vector3f dir;
    public Node user;
    public AnimControl control;
    public AnimChannel channel;
    //gets information and stores it; attached the node to the scene
    public AvatarGraphic(AvatarStatus as, AssetManager am, Node rootNode) {
        double degreeDir = as.dir;
        this.x = as.x;
        this.y = as.y;
        this.id = as.id;
        this.fAdr = ((id >> 16) & 0xffff) + "." + (id & 0xffff);
        this.numNear = as.numNear;
        this.numVis = as.numVisible;
        this.dir = new Vector3f((float) Math.sin(degreeDir*Math.PI/180),0f,(float)Math.cos(degreeDir*Math.PI/180));
        this.user = (Node) am.loadModel("Models/Oto.j3o");
        //now that we have data, update how the avatar is shown.
        user.lookAt(user.getLocalTranslation().add(dir), new Vector3f(0,1,0));
        user.setLocalTranslation((float) x,9.5f,(float) y);
        user.setLocalScale(2f);
        rootNode.attachChild(user);
        control = user.getControl(AnimControl.class);
        channel = control.createChannel();
        channel.setAnim("Walk");
    }
    public void remove(Node rootNode) {
        rootNode.detachChild(user);
    }
    public void update(AvatarStatus as) {
        double degreeDir = as.dir;
        this.x = as.x;
        this.y = as.y;
        this.numNear = as.numNear;
        this.numVis = as.numVisible;
        this.dir = new Vector3f((float) Math.sin(degreeDir*Math.PI/180),0f,(float)Math.cos(degreeDir*Math.PI/180));
        
        //now that we have data, update how the avatar is shown.
        //TODO: this is cheating, need to use dir instead
        user.lookAt(new Vector3f((float)this.x,9.5f,(float)this.y), new Vector3f(0,1,0));
        user.setLocalTranslation(new Vector3f((float) this.x,9.5f,(float) this.y));
        //user.lookAt(user.getWorldTranslation().add(dir), new Vector3f(0,1,0));
    }
}
