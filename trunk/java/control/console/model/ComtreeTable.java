package forest.control.console.model;

public class ComtreeTable{
    private String comtree;
    private String inCore;
    private String pLink;
    private String link;
    public ComtreeTable(String comtree, String inCore, String pLink, String link){
        this.comtree = comtree; this.inCore = inCore;
        this.pLink = pLink; this.link = link;
    }
    public String getComtree() {return comtree;}
    public String getInCore() {return inCore;}
    public String getpLink() {return pLink;}
    public String getLink() {return link;}
}
