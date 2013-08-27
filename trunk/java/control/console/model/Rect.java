package forest.control.console.model;

import java.awt.Color;

public class Rect{
    private int x, y, width, height = 0;
    private String name;
    private String nodeAddr;
    private Color color;
    public Rect(int x, int y, int width, int height, Color color, String name, String nodeAddr){
        this.x = x; this.y = y; 
        this.width = width; this.height = height;
        this.color = color;
        this.name = name; this.nodeAddr = nodeAddr;
    }
    public int getX(){      return x;}
    public int getY(){      return y;}
    public int getWidth(){  return width;}
    public int getHeight(){ return height;}
    public Color getColor(){return color;}
    public String getName(){return name;}
    public String getNodeAddr(){return nodeAddr;}
}
