package forest.control.console.model;

public class IfaceTable{
    private String iface;
    private String ip;
    private String rates;
    public IfaceTable(String iface, String ip, String rates){
        this.iface = iface; this.ip = ip; this.rates = rates;
    }
    public String getIface() {return iface;}
    public String getIp() {return ip;}
    public String getRates() {return rates;}
}
