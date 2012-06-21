package mygame;

import com.jme3.app.SimpleApplication;
import com.jme3.bullet.BulletAppState;
import com.jme3.bullet.collision.shapes.CapsuleCollisionShape;
import com.jme3.bullet.collision.shapes.CollisionShape;
import com.jme3.bullet.control.CharacterControl;
import com.jme3.bullet.control.RigidBodyControl;
import com.jme3.bullet.util.CollisionShapeFactory;
import com.jme3.input.KeyInput;
import com.jme3.input.controls.ActionListener;
import com.jme3.input.controls.KeyTrigger;
import com.jme3.light.AmbientLight;
import com.jme3.light.DirectionalLight;
import com.jme3.material.Material;
import com.jme3.math.ColorRGBA;
import com.jme3.math.FastMath;
import com.jme3.math.Vector3f;
import com.jme3.scene.Geometry;
import com.jme3.scene.Node;
import com.jme3.scene.Spatial;
import com.jme3.scene.shape.Box;
import com.jme3.system.AppSettings;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.net.InetSocketAddress;
import java.nio.ByteBuffer;
import java.nio.channels.SocketChannel;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Scanner;
import java.util.logging.Level;
import java.util.logging.Logger;
import forest.common.*;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.Socket;
import java.util.ArrayList;

/**
 * Forest
 * @author Logan Stafman
 */
public class Main extends SimpleApplication
        implements ActionListener {
    
    private final boolean LET_ME_FLY = false;
    
    private Spatial sceneModel;
    private BulletAppState bulletAppState;
    private RigidBodyControl landscape;
    private CharacterControl player;
    private Vector3f walkDirection = new Vector3f();
    private static int worldSize;
    private boolean left = false, right = false, up = false, down = false;
    private int comtree = 1001;
    private static String hostname;
    private static String mapfile;

    public static void main(String[] args) {
        Main app = new Main();
        //limit framerate to 60 fps
        AppSettings newSetting = new AppSettings(true);
        newSetting.setFrameRate(60);
        newSetting.setTitle("Forest Overlay Network");
        app.setSettings(newSetting);
        //hide jmonkey splash screen
        app.setShowSettings(false);
        if (args.length != 5) {
            System.out.println("usage: java -jar MyGame.jar hostname mapfile gridSize myIpAdr [tcp|udp]");
            System.exit(1);
        }
        hostname = args[0];
        mapfile = args[1];
        worldSize = Integer.parseInt(args[2]);
        String myIpAdrStr = args[3];
        myIpAdr = Forest.ipAddress(myIpAdrStr);
        needCliProxy = args[4].equals("tcp");
        java.util.logging.Logger.getLogger("").setLevel(Level.SEVERE);
        app.start();
    }

    public void simpleInitApp() {
        //hide the framerate
        guiNode.detachAllChildren();
        setupVisibility();
        //physics model for floor/camera
        bulletAppState = new BulletAppState();
        stateManager.attach(bulletAppState);
        // We re-use the flyby camera for rotation, while positioning is handled by physics
        viewPort.setBackgroundColor(new ColorRGBA(0.7f, 0.8f, 1f, 1f));
        flyCam.setMoveSpeed(100);
        setUpKeys();
        setUpLight();
        setUpWalls();
        login();
        //use the plain 512x512 floor I made in assets
        sceneModel = assetManager.loadModel("Scenes/newScene.j3o");
        sceneModel.setLocalScale(2f);

        // We set up collision detection for the scene by creating a
        // compound collision shape and a static RigidBodyControl with mass zero.
        CollisionShape sceneShape =
                CollisionShapeFactory.createMeshShape((Node) sceneModel);
        landscape = new RigidBodyControl(sceneShape, 0);
        sceneModel.addControl(landscape);

        // We set up collision detection for the camera
        // Some of this is probably unnecessary
        CapsuleCollisionShape capsuleShape = new CapsuleCollisionShape(13f, 6f, 1);
        player = new CharacterControl(capsuleShape, 0.05f);
        player.setJumpSpeed(20);
        player.setFallSpeed(60);
        if(LET_ME_FLY) player.setGravity(0);
        else player.setGravity(100);
        player.setPhysicsLocation(new Vector3f(0, 10, 0));


        // We attach the scene and the player to the rootNode and the physics space,
        // to make them appear in the game world.
        rootNode.attachChild(sceneModel);
        bulletAppState.getPhysicsSpace().add(landscape);
        bulletAppState.getPhysicsSpace().add(player);
        //wait to ssh to port
        if (needCliProxy) {
            System.out.println("Press enter once you've tunnelled...");
            (new Scanner(System.in)).nextLine();
        }
        try {
            chan = SocketChannel.open(new InetSocketAddress(hostname, CP_PORT));
            chan.configureBlocking(false);
        } catch (IOException ex) {
            System.out.println("Couldn't open channel to Client Proxy");
            System.out.println(ex);
            System.exit(1);
        }
        recentIds = new HashSet<Integer>();
        seqNum = 1;
        idCounter = 1;
        mySubs = new HashSet<Integer>();
        waiting4comtCtl = true;
        buf = ByteBuffer.allocate(1500);
        now = nextTime = (int) System.nanoTime() / 1000000;
        status = new HashMap<Integer, AvatarGraphic>();
        connect();
        sendCtlPkt2CC(true, comtree);
    }
    //setup all of the data structures needed in ShowWorld
    private HashMap<Integer, AvatarGraphic> status;
    private HashSet<Integer> recentIds;
    private int idCounter;
    private SocketChannel chan;
    private DatagramSocket udpSock;
    private final int CM_PORT = 30140;
    private final int CP_PORT = 30182;
    private ByteBuffer buf;
    private String uname = "user";
    private String pword = "pass";
    private int rtrAdr;
    private int myAdr;
    private int rtrIpAdr;
    private int ccAdr;
    private long seqNum;
    private int[] walls;
    private final int GRID = 10000;
    private final int MAXNEAR = 1000;
    private final int STATUS_REPORT = 1;
    private final int UPDATE_PERIOD = 50;
    private HashSet<Integer>[] visSet;
    private static int myIpAdr;
    private HashSet<Integer> mySubs;
    private HashMap<Integer, Integer> nearAvatars;
    private HashMap<Integer, Integer> visibleAvatars;
    private int numNear;
    private int numVisible;
    private boolean waiting4comtCtl;
    private int newcomt;
    private int now;
    private int nextTime;
    private static boolean needCliProxy;
    private int cpIpAdr;
    private int cpPort;
    private int cpForestPort;
    //logic taken from ShowWorld::main() before the while(true) loop

    private void login() {
        System.out.println("got to login");
        Socket cm_sock = null;
        try {
            cm_sock = new Socket(hostname, CM_PORT);
            String portString = String.valueOf(cm_sock.getPort());
            String strBuf = uname + " " + pword + " " + portString;
            if (needCliProxy) {
                strBuf += " proxy";
            } else {
                strBuf += " noproxy";
            }
            System.out.println(strBuf);
            DataOutputStream dos = new DataOutputStream(cm_sock.getOutputStream());
            dos.writeInt(strBuf.length());
            dos.writeBytes(strBuf);
            DataInputStream dis = new DataInputStream(cm_sock.getInputStream());
            rtrAdr = dis.readInt();
            if (rtrAdr == -1) {
                System.out.println("Couldn't connect, negative reply");
                System.exit(1);
            }
            if (needCliProxy) {
                myAdr = dis.readInt();
                cpIpAdr = dis.readInt();
                cpPort = dis.readInt();
                cpForestPort = dis.readInt();
                ccAdr = dis.readInt();
                System.out.println("assigned client proxy ip " +
                        Forest.ip2string(cpIpAdr));
                System.out.println("assigned forest address " +
                        Forest.fAdr2string(myAdr));
                System.out.println("assigned client proxy port " + cpPort);
                System.out.println("router address " + Forest.fAdr2string(rtrAdr));
                System.out.println("comtree controller address " + 
                        Forest.fAdr2string(ccAdr));
            } else {
                
                myAdr = dis.readInt();
                rtrIpAdr = dis.readInt();
                ccAdr = dis.readInt();
                System.out.println("assigned address " + Forest.fAdr2string(myAdr));
                System.out.println("router address " + Forest.fAdr2string(rtrAdr));
                System.out.println(" comtree controller address " + Forest.fAdr2string(ccAdr));
            }
            cm_sock.close();
        } catch (Exception e) {
            System.out.println("Couldn't open socket to client manager");
            System.out.println(e);
            System.exit(1);
        }
    }
    //make walls from the map file
    //then make walls around the edges
    //note: walls are weird in that giving them a width of x makes them 2x wide

    private void setUpWalls() {
        try {
            float wallHeight = 15f;
            float wallWidth = 256f / 15f;
            Node walls = new Node();
            Scanner in = new Scanner(new File(mapfile));
            int y = 0;
            Geometry g;
            Box b;
            Material mat = new Material(assetManager, "Common/MatDefs/Misc/Unshaded.j3md");
            mat.setTexture("ColorMap", assetManager.loadTexture("Textures/BrickWall.jpg"));
            Material mat2 = new Material(assetManager, "Common/MatDefs/Misc/Unshaded.j3md");
            mat2.setColor("Color", ColorRGBA.Red);
            RigidBodyControl wallControl;
            while (in.hasNextLine()) {
                String s = in.nextLine();
                for (int i = 0; i < s.length(); i++) {

                    char c = s.charAt(i);
                    int x = i;
                    float tx = x * 2 * wallWidth - 256f;//transformed x
                    float ty = y * 2 * wallWidth - 256f; //transformed y

                    switch (c) {
                        case '+':
                            b = new Box(Vector3f.ZERO, wallWidth, wallHeight, .25f);
                            g = new Geometry("Box", b);
                            g.setLocalTranslation((tx + b.xExtent), b.yExtent, ty);
                            g.setMaterial(mat);
                            walls.attachChild(g);
                            wallControl = new RigidBodyControl(0);
                            g.addControl(wallControl);
                            bulletAppState.getPhysicsSpace().add(wallControl);

                            b = new Box(Vector3f.ZERO, .25f, wallHeight, wallWidth);
                            g = new Geometry("Box", b);
                            g.setLocalTranslation(tx, b.yExtent, (ty + b.zExtent));
                            g.setMaterial(mat);
                            walls.attachChild(g);
                            wallControl = new RigidBodyControl(0);
                            g.addControl(wallControl);
                            bulletAppState.getPhysicsSpace().add(wallControl);
                            break;
                        case '-':
                            b = new Box(Vector3f.ZERO, wallWidth, wallHeight, .25f);
                            g = new Geometry("Box", b);
                            g.setLocalTranslation((tx + b.xExtent), b.yExtent, ty);
                            g.setMaterial(mat);
                            walls.attachChild(g);
                            wallControl = new RigidBodyControl(0);
                            g.addControl(wallControl);
                            bulletAppState.getPhysicsSpace().add(wallControl);
                            break;
                        case '|':
                            b = new Box(Vector3f.ZERO, .25f, wallHeight, wallWidth);
                            g = new Geometry("Box", b);
                            g.setLocalTranslation(tx, b.yExtent, (ty + b.zExtent));
                            g.setMaterial(mat);
                            walls.attachChild(g);
                            wallControl = new RigidBodyControl(0);
                            g.addControl(wallControl);
                            bulletAppState.getPhysicsSpace().add(wallControl);
                            break;
                        case ' ':
                            break;
                    }
                }
                y++;
            }
            //NOW DO EDGE walls
            b = new Box(Vector3f.ZERO, 1f, wallHeight, 256);
            g = new Geometry("Box", b);
            g.setLocalTranslation(-256, b.yExtent, 0);
            g.setMaterial(mat);
            walls.attachChild(g);
            wallControl = new RigidBodyControl(0);
            g.addControl(wallControl);
            bulletAppState.getPhysicsSpace().add(wallControl);

            b = new Box(Vector3f.ZERO, 1f, wallHeight, 256);
            g = new Geometry("Box", b);
            g.setLocalTranslation(256, b.yExtent, 0);
            g.setMaterial(mat);
            walls.attachChild(g);
            wallControl = new RigidBodyControl(0);
            g.addControl(wallControl);
            bulletAppState.getPhysicsSpace().add(wallControl);

            b = new Box(Vector3f.ZERO, 256, wallHeight, 1f);
            g = new Geometry("Box", b);
            g.setLocalTranslation(0, b.yExtent, -256);
            g.setMaterial(mat);
            walls.attachChild(g);
            wallControl = new RigidBodyControl(0);
            g.addControl(wallControl);
            bulletAppState.getPhysicsSpace().add(wallControl);

            b = new Box(Vector3f.ZERO, 256, wallHeight, 1f);
            g = new Geometry("Box", b);
            g.setLocalTranslation(0, b.yExtent, 256);
            g.setMaterial(mat);
            walls.attachChild(g);
            wallControl = new RigidBodyControl(0);
            g.addControl(wallControl);
            bulletAppState.getPhysicsSpace().add(wallControl);

            rootNode.attachChild(walls);
        } catch (FileNotFoundException ex) {
            Logger.getLogger(Main.class.getName()).log(Level.SEVERE, null, ex);
        }

    }
    //need two types of light; one for sky, one for mesh models

    private void setUpLight() {
        // We add light so we see the scene
        AmbientLight al = new AmbientLight();
        al.setColor(ColorRGBA.White.mult(1.3f));
        rootNode.addLight(al);

        DirectionalLight dl = new DirectionalLight();
        dl.setColor(ColorRGBA.White);
        dl.setDirection(new Vector3f(2.8f, -2.8f, -2.8f).normalizeLocal());
        rootNode.addLight(dl);
    }

    /** We over-write some navigational key mappings here, so we can
     * add physics-controlled flying: */
    private void setUpKeys() {
        inputManager.addMapping("Left", new KeyTrigger(KeyInput.KEY_A));
        inputManager.addMapping("Right", new KeyTrigger(KeyInput.KEY_D));
        inputManager.addMapping("Up", new KeyTrigger(KeyInput.KEY_W));
        inputManager.addMapping("Down", new KeyTrigger(KeyInput.KEY_S));
        inputManager.addListener(this, "Left");
        inputManager.addListener(this, "Right");
        inputManager.addListener(this, "Up");
        inputManager.addListener(this, "Down");
    }

    /** These are our custom actions triggered by key presses.
     * We do not walk yet, we just keep track of the direction the user pressed. */
    public void onAction(String binding, boolean value, float tpf) {
        if (binding.equals("Left")) {
            left = value;
        } else if (binding.equals("Right")) {
            right = value;
        } else if (binding.equals("Up")) {
            up = value;
        } else if (binding.equals("Down")) {
            down = value;
        }
    }

    /**
     * This is the main loop; Moving happens here, as does packet sending/receiving
     * The second half is logic from the while(true) loop of ShowWorld
     */
    @Override
    public void simpleUpdate(float tpf) {
        Vector3f camDir = cam.getDirection().clone().multLocal(1.5f);
        Vector3f camLeft = cam.getLeft().clone().multLocal(1.5f);
        walkDirection.set(0, 0, 0);
        if (left) {
            walkDirection.addLocal(camLeft);
        }
        if (right) {
            walkDirection.addLocal(camLeft.negate());
        }
        if (up) {
            walkDirection.addLocal(camDir);
        }
        if (down) {
            walkDirection.addLocal(camDir.negate());
        }
        if(!LET_ME_FLY) walkDirection.y = 0;
        player.setWalkDirection(walkDirection);

        cam.setLocation(player.getPhysicsLocation());

        now = (int) System.nanoTime() / 1000000;
        if (!waiting4comtCtl) {
            updateSubs();
        }
        
        idCounter = (++idCounter)%5;
        if(idCounter == 0) {
            HashSet<Integer> tmp = new HashSet<Integer>();
            for(Integer i : status.keySet())
                if(!recentIds.contains(i)) tmp.add(i);
            recentIds.clear();
            for(Integer i : tmp) {
                status.get(i).remove(rootNode);
                status.remove(i);
            }
        }
        
        Forest.PktBuffer b;
        while ((b = receive()) != null) {
            PktHeader h = new PktHeader();
            h.unpack(b);
            Forest.PktTyp ptyp = h.getPtype();
            if (!waiting4comtCtl && ptyp == Forest.PktTyp.CLIENT_DATA) {
                updateNearby(b);
                draw(b);
            } else if (waiting4comtCtl && ptyp == Forest.PktTyp.CLIENT_SIG) {
                CtlPkt cp = new CtlPkt();
                cp.unpack(b);
                if (cp.getCpType() == CpTyp.CLIENT_JOIN_COMTREE
                        && cp.getRrType() == CtlPkt.CpRrTyp.POS_REPLY) {
                    waiting4comtCtl = false;
                } else if (cp.getCpType() == CpTyp.CLIENT_LEAVE_COMTREE
                        && cp.getRrType() == CtlPkt.CpRrTyp.POS_REPLY) {
                    comtree = newcomt;
                    sendCtlPkt2CC(true, comtree);
                }
            }
        }
        if (!waiting4comtCtl) {
            sendStatus(now);
        }
        now = (int) System.nanoTime() / 1000000;
        int delay = nextTime - now;
        if (delay < (1 << 31)) {
            try {
                Thread.sleep(delay);
            } catch (InterruptedException ex) {
                Logger.getLogger(Main.class.getName()).log(Level.SEVERE, null, ex);
            }
        } else {
            nextTime = now + 1000 * UPDATE_PERIOD;
        }
    }
    /**
     * send the current status of this avatar to the Forest network
     * @param now is the current time
     */
    public void sendStatus(int now) {
        Forest.PktBuffer b = new Forest.PktBuffer();
        PktHeader h = new PktHeader();
        h.setLength(4 * (5 + 8));
        h.setPtype(Forest.PktTyp.CLIENT_DATA);
        h.setFlags((short) 0);
        h.setComtree(comtree);
        h.setSrcAdr(myAdr);
        h.setDstAdr(-groupNum(getX(), getY()));
        h.pack(b);
        
        b.set(Forest.HDR_LENG/4    , STATUS_REPORT);
        b.set(Forest.HDR_LENG/4 + 1, now);
        b.set(Forest.HDR_LENG/4 + 2, getX());
        b.set(Forest.HDR_LENG/4 + 3, getY());
        b.set(Forest.HDR_LENG/4 + 4, getDirection());
        b.set(Forest.HDR_LENG/4 + 5, 0);//TODO
        b.set(Forest.HDR_LENG/4 + 6, numNear);
        b.set(Forest.HDR_LENG/4 + 7, numVisible);
        send(b, h.getLength());
    }
    /**
     * receive packet from Forest network
     * @return the buffer of the new packet, or null if none is available
     */
    public Forest.PktBuffer receive() {
        if (needCliProxy) {
            Forest.PktBuffer b = new Forest.PktBuffer();
            buf.clear();
            buf.limit(4);
            try {
                int nbytes = chan.read(buf);
                if (nbytes <= 0) {
                    return null;
                }
                int length = buf.getInt(0);
                buf.clear();
                buf.limit(length);
                while (buf.position() != buf.limit()) {
                    chan.read(buf);
                }
                for (int i = 0; i < length / 4; i++) {
                    b.set(i, buf.getInt(i * 4));
                }
            } catch (IOException ex) {
                System.out.println("I/O exception when reading");
                System.out.println(ex);
                System.exit(1);
            }

            return b;
        } else {
            byte[] buff = new byte[1500];
            Forest.PktBuffer b = new Forest.PktBuffer();
            DatagramPacket p = new DatagramPacket(buff,1500);
            try {
                udpSock.receive(p);
                for(int i = 0; i < p.getLength(); i+=4) {
                    int k = (buff[i] << 24) | (buff[i+1] << 16) |
                            (buff[i+2] << 8) | (buff[i+3]);
                    b.set(i/4, k);
                }
            } catch (IOException ex) {
                System.out.println("I/O exception when reading");
                System.out.println(ex);
                System.exit(1);
            }
            return b;
        }
    }
    /**
     * send connect packet to Forest network
     */
    public void connect() {
        Forest.PktBuffer buff = new Forest.PktBuffer();
        PktHeader h = new PktHeader();
        h.setLength(4 * (5 + 1));
        h.setPtype(Forest.PktTyp.CONNECT);
        h.setFlags((short) 0);
        h.setComtree(Forest.CLIENT_CON_COMT);
        h.setSrcAdr(myAdr);
        h.setDstAdr(rtrAdr);
        h.pack(buff);
        send(buff, h.getLength());
    }
    /**
     * send disconnect packet to Forest router
     */
    public void disconnect() {
        Forest.PktBuffer buff = new Forest.PktBuffer();
        PktHeader h = new PktHeader();
        h.setLength(4 * (5 + 1));
        h.setPtype(Forest.PktTyp.DISCONNECT);
        h.setFlags((short) 0);
        h.setComtree(Forest.CLIENT_CON_COMT);
        h.setSrcAdr(myAdr);
        h.setDstAdr(rtrAdr);
        h.pack(buff);
        send(buff, h.getLength());
    }
    /**
     * Send packet to proxy or Forest network
     * @param b buffer to be sent
     * @param length length of packet to be sent
     */
    public void send(Forest.PktBuffer b, int length) {
        if (needCliProxy) {
            try {
                b.put2BytBuf(buf, length);
                buf.flip();
                chan.write(buf);
                chan.socket().getOutputStream().flush();
            } catch (IOException ex) {
                System.out.println("Could not send tcp packet");
                System.out.println(ex);
                System.exit(1);
            }
        } else {
            try {
                byte[] pkt = b.toByteArray(length);
                InetAddress ip = InetAddress.getByAddress(Forest.ip2byteArr(rtrIpAdr));
                udpSock.send(new DatagramPacket(pkt,pkt.length,ip,Forest.ROUTER_PORT));
            } catch (Exception ex) {
                System.out.println("Could not send datagram packet");
                System.out.println(ex);
                System.exit(1);
            }
        }
    }
    /**
     * Send packet to the comtree controller to leave/join a comtree
     * @param join whether to join or leave comtree
     * @param comt the comtree to join or leave
     */
    public void sendCtlPkt2CC(boolean join, int comt) {
        Forest.PktBuffer buff = new Forest.PktBuffer();
        CpTyp cpType = (join ? CpTyp.CLIENT_JOIN_COMTREE : CpTyp.CLIENT_LEAVE_COMTREE);
        CtlPkt cp = new CtlPkt(cpType, CtlPkt.CpRrTyp.REQUEST, seqNum++);
        cp.setAttr(CpAttr.COMTREE_NUM, comt);
        cp.setAttr(CpAttr.CLIENT_IP, needCliProxy ? cpIpAdr : myIpAdr);
        cp.setAttr(CpAttr.CLIENT_PORT, needCliProxy ? cpForestPort : udpSock.getLocalPort());
        int leng = cp.pack(buff);
        if (leng == 0) {
            System.out.println("Couldn't pack control packt");
            System.exit(1);
        }
        PktHeader h = new PktHeader();
        h.setLength(Forest.OVERHEAD + leng);
        h.setPtype(Forest.PktTyp.CLIENT_SIG);
        h.setComtree(Forest.CLIENT_SIG_COMT);
        h.setSrcAdr(myAdr);
        h.setDstAdr(ccAdr);
        h.pack(buff);
        send(buff, Forest.OVERHEAD + leng);
    }
    /**
     * 
     * @param g1 is groupNum of one group
     * @param g2 is groupNum of the other group
     * @return true if g1 can be seen from g2, false otherwise
     */
    public boolean isVis(int g1, int g2) {
        int region1 = g1 - 1; int region2 = g2 - 1;
        int[] region1xs = new int[4];
        int[] region1ys = new int[4];
        int[] region2xs = new int[4];
        int[] region2ys = new int[4];

        int row1 = region1 / worldSize;
        int col1 = region1 % worldSize;
        int row2 = region2 / worldSize;
        int col2 = region2 % worldSize;

        region1xs[0] = region1xs[2] = col1 * GRID + 1;
        region1ys[0] = region1ys[1] = (row1 + 1) * GRID - 1;
        region1xs[1] = region1xs[3] = (col1 + 1) * GRID - 1;
        region1ys[2] = region1ys[3] = row1 * GRID + 1;
        region2xs[0] = region2xs[2] = col2 * GRID + 1;
        region2ys[0] = region2ys[1] = (row2 + 1) * GRID - 1;
        region2xs[1] = region2xs[3] = (col2 + 1) * GRID - 1;
        region2ys[2] = region2ys[3] = row2 * GRID + 1;

        int minRow = Math.min(row1, row2);
        int maxRow = Math.max(row1, row2);
        int minCol = Math.min(col1, col2);
        int maxCol = Math.max(col1, col2);

        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                boolean canSee = true;
                double ax = (double) region1xs[i];
                double ay = (double) region1ys[i];
                double bx = (double) region2xs[j];
                double by = (double) region2ys[j];
                // line segment ab connects a corner of region1
                // to a corner of region2
                for (int y = minRow; y <= maxRow; y++) {
                    for (int z = minCol; z <= maxCol; z++) {
                        double cx = (double) z * GRID;
                        double cy = (double) (y + 1) * GRID;
                        // (cx,cy) is top left corner of region (i,j)
                        int k = y * worldSize + z; // k=region index
                        if (walls[k] == 1 || walls[k] == 3) {
                            // region k has a left side wall
                            // check if ab intersects it
                            double dx = cx;
                            double dy = cy - GRID;
                            if (linesIntersect(ax, ay, bx, by,
                                    cx, cy, dx, dy)) {
                                canSee = false;
                                break;
                            } else {
                            }
                        }
                        if (walls[k] == 2 || walls[k] == 3) {
                            // region k has a top wall
                            // check if ab intersects it
                            double dx = cx + GRID;
                            double dy = cy;
                            if (linesIntersect(ax, ay, bx, by,
                                    cx, cy, dx, dy)) {
                                canSee = false;
                                break;
                            } else {
                            }
                        }
                    }
                    if (!canSee) {
                        break;
                    }
                }
                if (canSee) {
                    return true;
                }
            }
        }
        return false;
    }
    /**
     * 
     * @param ax x of point a on line 1
     * @param ay y of point a on line 1
     * @param bx x of point b on line 1
     * @param by y of point b on line 1
     * @param cx x of point c on line 2
     * @param cy y of point c on line 2
     * @param dx x of point d on line 2
     * @param dy y of point d on line 2
     * @return true if the lines intersect, false otherwise
     */
    public boolean linesIntersect(double ax, double ay, double bx, double by,
            double cx, double cy, double dx, double dy) {
        double epsilon = .0001; // two lines are considered vertical if
        // x values differ by less than epsilon
        // first handle special case of two vertical lines
        if (Math.abs(ax - bx) < epsilon && Math.abs(cx - dx) < epsilon) {
            return Math.abs(ax - cx) < epsilon && Math.max(ay, by) >= Math.min(cy, dy)
                    && Math.min(ay, by) <= Math.max(cy, dy);
        }
        // now handle cases of one vertical line
        if (Math.abs(ax - bx) < epsilon) {
            double s2 = (dy - cy) / (dx - cx); // slope of second line
            double i2 = cy - s2 * cx; // y-intercept of second line
            double y = s2 * ax + i2;  // lines intersect at (ax,y)
            return (y >= Math.min(ay, by) && y <= Math.max(ay, by)
                    && y >= Math.min(cy, dy) && y <= Math.max(cy, dy));
        }
        if (Math.abs(cx - dx) < epsilon) {
            double s1 = (by - ay) / (bx - ax); // slope of first line
            double i1 = ay - s1 * ax; // y-intercept of first line
            double y = s1 * cx + i1;  // lines intersect at (cx,y)
            return (y >= Math.min(ay, by) && y <= Math.max(ay, by)
                    && y >= Math.min(cy, dy) && y <= Math.max(cy, dy));
        }
        double s1 = (by - ay) / (bx - ax);    // slope of first line
        double i1 = ay - s1 * ax;         // y-intercept of first line
        double s2 = (dy - cy) / (dx - cx);    // slope of second line
        double i2 = cy - s2 * cx;         // y-intercept of second line

        // handle special case of lines with equal slope
        // treat the slopes as equal if both slopes have very small
        // magnitude, or if their relative difference is small
        if (Math.abs(s1) + Math.abs(s2) <= epsilon
                || Math.abs(s1 - s2) / (Math.abs(s1) + Math.abs(s2)) < epsilon) {
            return (Math.abs(i1 - i2) < epsilon
                    && Math.min(ax, bx) <= Math.max(cx, dx)
                    && Math.max(ax, bx) >= Math.min(cx, dx));
        }
        // now, to the common case
        double x = (i2 - i1) / (s1 - s2);     // x value where the lines interesect

        return (x >= Math.min(ax, bx) && x <= Math.max(ax, bx)
                && x >= Math.min(cx, dx) && x <= Math.max(cx, dx));
    }
    /**
     * read into the walls array from the mapfile
     */
    public void readWalls() {
        walls = new int[worldSize * worldSize];
        Scanner in = null;
        try {
            in = new Scanner(new File(mapfile));
        } catch (FileNotFoundException ex) {
            System.out.println("couldn't read mapfile");
            System.exit(1);
        }
        int lineCnt = 1;
        while (true) {
            String line;
            line = in.nextLine();
            if (line.length() < worldSize) {
                System.out.println("format error, all lines must have same length");
                System.exit(1);
            }
            for (int i = 0; i < worldSize; i++) {
                if (line.charAt(i) == '+') {
                    walls[(worldSize - lineCnt) * worldSize + i] = 3;
                } else if (line.charAt(i) == '-') {
                    walls[(worldSize - lineCnt) * worldSize + i] = 2;
                } else if (line.charAt(i) == '|') {
                    walls[(worldSize - lineCnt) * worldSize + i] = 1;
                } else if (line.charAt(i) == ' ') {
                    walls[(worldSize - lineCnt) * worldSize + i] = 0;
                } else {
                    System.out.println("Unrecognized symbol in map file!");
                    System.exit(1);
                }
            }
            if (lineCnt == worldSize) {
                break;
            }
            lineCnt++;
        }
        in.close();
    }
    /**
     * read the map file and create visibility sets as necessary
     */
    public void setupVisibility() {
        readWalls();
        visSet = new HashSet[worldSize * worldSize + 1];
        for(int i = 0; i <= worldSize * worldSize; i++) {
            visSet[i] = new HashSet<Integer>();
        }
        for (int x1 = 0; x1 < worldSize; x1++) {
            for (int y1 = 0; y1 < worldSize; y1++) {
                int g1 = 1 + x1 + y1 * worldSize;
                //start with upper right quadrant
                for (int d = 1; d < worldSize; d++) {
                    boolean done = true;
                    for (int x2 = x1; x2 <= Math.min(x1 + d, worldSize - 1); x2++) {
                        int y2 = d + y1 - (x2 - x1);
                        if (y2 >= worldSize) continue;
                        int g2 = 1 + x2 + y2 * worldSize;
                        if (isVis(g1, g2)) {
                            visSet[g1].add(g2);
                            done = false;
                        }
                    }
                    if (done) break;
                }
                //now upper left quadrant
                for (int d = 1; d < worldSize; d++) {
                    boolean done = true;
                    for (int x2 = x1; x2 >= Math.max(0, x1 - d); x2--) {
                        int y2 = d + y1 - (x1 - x2);
                        if (y2 >= worldSize) continue;
                        int g2 = 1 + x2 + y2 * worldSize;
                        if (isVis(g1, g2)) {
                            visSet[g1].add(g2);
                            done = false;
                        }
                    }
                    if (done) break;
                }
                //lower left quadrant
                for (int d = 1; d < worldSize; d++) {
                    boolean done = true;
                    for (int x2 = x1; x2 >= Math.max(0, x1 - d); x2--) {
                        int y2 = (x1 - x2) + y1 - d;
                        if (y2 < 0) continue;
                        int g2 = 1 + x2 + y2 * worldSize;
                        if (isVis(g1, g2)) {
                            visSet[g1].add(g2);
                            done = false;
                        }
                    }
                    if (done) break;
                }
                //lower right quadrant
                for (int d = 1; d < worldSize; d++) {
                    boolean done = true;
                    for (int x2 = x1; x2 <= Math.min(x1 + d, worldSize - 1); x2++) {
                        int y2 = (x2 - x1) + y1 - d;
                        if (y2 < 0) continue;
                        int g2 = 1 + x2 + y2 * worldSize;
                        if (isVis(g1, g2)) {
                            visSet[g1].add(g2);
                            done = false;
                        }
                    }
                    if (done) break;
                }
            }
        }
        int maxVis = 0; int totVis = 0;
        for(int g = 1; g <= worldSize*worldSize; g++) {
            int vis = visSet[g].size();
            maxVis = Math.max(vis,maxVis); totVis += vis;
        }
        System.out.println("avg visible: " + totVis/(worldSize*worldSize) +
                            " max visible: " + maxVis);
    }
    /**
     * 
     * @param x1 current x in forest context
     * @param y1 current y in forest context
     * @return multicast group number
     */
    public int groupNum(int x1, int y1) {
        return (1 + x1 / GRID) + (y1 / GRID) * worldSize;
    }
    /**
     * Send unsubscribe packet for all group numbers in glist
     */
    public void unsubscribe(ArrayList<Integer> glist) {
        if(glist.isEmpty()) return;
        PktHeader h = new PktHeader();
        Forest.PktBuffer b = new Forest.PktBuffer();
        int nunsub = 0;
        for (Integer g : glist) {
            nunsub++;
            if (nunsub > 350) {
                b.set(Forest.HDR_LENG/4    , 0);
                b.set(Forest.HDR_LENG/4 + 1, nunsub - 1);
                h.setLength(Forest.OVERHEAD + 4 * (2 + nunsub));
                h.setPtype(Forest.PktTyp.SUB_UNSUB);
                h.setFlags((short) 0);
                h.setComtree(comtree);
                h.setSrcAdr(myAdr);
                h.setDstAdr(rtrAdr);
                h.pack(b);
                send(b, h.getLength());
            }
            b.set(Forest.HDR_LENG/4 + nunsub + 1, -g);
        }
        b.set(Forest.HDR_LENG/4    , 0);
        b.set(Forest.HDR_LENG/4 + 1, nunsub);

        h.setLength(Forest.OVERHEAD + 4 * (2 + nunsub));
        h.setPtype(Forest.PktTyp.SUB_UNSUB);
        h.setFlags((short) 0);
        h.setComtree(comtree);
        h.setSrcAdr(myAdr);
        h.setDstAdr(rtrAdr);
        h.pack(b);
        send(b, h.getLength());
    }
    /*
     * Send subscription packet for all groups in glist
     */
    public void subscribe(ArrayList<Integer> glist) {
        if(glist.isEmpty()) return;
        PktHeader h = new PktHeader();
        Forest.PktBuffer b = new Forest.PktBuffer();
        int nsub = 0;
        for (Integer g : glist) {
            nsub++;
            if (nsub > 350) {
                b.set(Forest.HDR_LENG/4, nsub - 1);
                b.set(Forest.HDR_LENG/4 + nsub, 0);
                h.setLength(Forest.OVERHEAD + 4 * (2 + nsub));
                h.setPtype(Forest.PktTyp.SUB_UNSUB);
                h.setFlags((short) 0);
                h.setComtree(comtree);
                h.setSrcAdr(myAdr);
                h.setDstAdr(rtrAdr);
                h.pack(b);
                send(b, h.getLength());
            }
            b.set(Forest.HDR_LENG/4 + nsub, -g);
        }
        b.set(Forest.HDR_LENG/4           , nsub);
        b.set(Forest.HDR_LENG/4 + nsub + 1, 0);
        h.setLength(Forest.OVERHEAD + 4 * (2 + nsub));
        h.setPtype(Forest.PktTyp.SUB_UNSUB);
        h.setFlags((short) 0);
        h.setComtree(comtree);
        h.setSrcAdr(myAdr);
        h.setDstAdr(rtrAdr);
        h.pack(b);
        send(b, h.getLength());
    }
    /*
     * Update subscriptions based on multicast group location
     */
    public void updateSubs() {
        int myGroup = groupNum(getX(), getY());
        ArrayList<Integer> glist = new ArrayList<Integer>();
        for (Integer g : mySubs) {
            if (!visSet[myGroup].contains(g)) {
                glist.add(g);
            }
        }
        mySubs.removeAll(glist);
        unsubscribe(glist);

        glist.clear();
        
        for (Integer g : visSet[myGroup]) {
            if (!mySubs.contains(g)) {
                mySubs.add(g);
                glist.add(g);
            }
        }
        subscribe(glist);
    }
    /*
     * Redraw avatars in their new locations as specified by the pkt
     */
    public void draw(Forest.PktBuffer b) {
        PktHeader h = new PktHeader();
        h.unpack(b);
        AvatarStatus as = new AvatarStatus();
        as.id   = h.getSrcAdr();
        as.when = b.get(Forest.HDR_LENG/4 + 1);
        as.x    = b.get(Forest.HDR_LENG/4 + 2);
        as.x    = (as.x / (GRID * worldSize) * 512 - 256);
        as.y    = b.get(Forest.HDR_LENG/4 + 3);
        as.y    = -1 * (as.y / (GRID * worldSize) * 512 - 256);
        as.dir  = b.get(Forest.HDR_LENG/4 + 4);
        as.numNear = b.get(Forest.HDR_LENG/4 + 6);
        as.numVisible = b.get(Forest.HDR_LENG/4 + 7);
        
        recentIds.add(as.id);
        AvatarGraphic ag = status.get(as.id);
        if(ag == null) {
            ag = new AvatarGraphic(as,assetManager,rootNode);
            status.put(as.id, ag);
        } else ag.update(as);
    }
    /*
     * Similar to the Avatar.cpp updateNearby, keeps track of how many avatars
     * visible/nearby
     */
    public void updateNearby(Forest.PktBuffer b) {
        PktHeader h = new PktHeader();
        h.unpack(b);
        if (b.get(0) != STATUS_REPORT) {
            return;
        }

        int x1 = b.get(Forest.HDR_LENG/4 + 2);
        int y1 = b.get(Forest.HDR_LENG/4 + 3);
        int key = h.getSrcAdr();
        if (!nearAvatars.containsKey(key)) {
            if (numNear <= MAXNEAR) {
                nearAvatars.put(key, ++numNear);
            }
        }
        boolean canSee = true;
        for (int i = 0; i < worldSize * worldSize; i++) {
            if (walls[i] == 1 || walls[i] == 3) {
                int l2x1 = (i % worldSize) * GRID;
                int l2x2 = l2x1;
                int l2y1 = (i / worldSize) * GRID;
                int l2y2 = l2y1 + GRID;
                if (linesIntersect(x1, y1, getX(), getY(), l2x1, l2y1, l2x2, l2y2)) {
                    canSee = false;
                    break;
                }
            }
            if (walls[i] == 2 || walls[i] == 3) {
                int l2x1 = (i % worldSize) * GRID;
                int l2x2 = l2x1 + GRID;
                int l2y1 = (i / worldSize) * GRID + GRID;
                int l2y2 = l2y1;
                if (linesIntersect(x1, y1, getX(), getY(), l2x1, l2y1, l2x2, l2y2)) {
                    canSee = false;
                    break;
                }
            }
        }
        if (canSee) {
            if (!visibleAvatars.containsKey(key)) {
                if (numVisible <= MAXNEAR) {
                    visibleAvatars.put(key, ++numVisible);
                }
            }
        }
    }
    /*
     * Computes x in the forest context from the local context
     */
    public int getX() {
        return (int) (((player.getPhysicsLocation().x + 256) / 512.0) * worldSize * GRID);
    }
    /*
     * Computes y in the forest context from the local context
     */
    public int getY() {
        return ((int) (((-1*player.getPhysicsLocation().z + 256) / 512.0) * worldSize * GRID));
    }
    /**
     * Computes direction based on physics directions
     * @return direction in degrees, 0 pointing upwards
     */
    public int getDirection() {
        float atan = FastMath.atan2(player.getWalkDirection().x, player.getWalkDirection().z);
        return (int) (atan * (180 / Math.PI));
    }
}