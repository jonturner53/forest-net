package forest.control.console.model;

public class RouteTable{
    private String comtree;
    private String addr;
    public RouteTable(String comtree, String addr){
        this.comtree = comtree; this.addr = addr;
    }
    public String getComtree() {return comtree;}
    public String getAddr() {return addr;}
}