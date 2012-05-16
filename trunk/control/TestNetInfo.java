package forest.control;

public class TestNetInfo {
	private static NetInfo net;

	public static void main(String[] args) {
		net = new NetInfo(1000, 1000, 100, 100, 1000);
		net.read(System.in);
		System.out.print(net.toString());
	}
}
