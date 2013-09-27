package forest.control.console;

import java.io.IOException;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.SocketAddress;
import java.nio.ByteBuffer;
import java.nio.CharBuffer;
import java.nio.channels.SocketChannel;
import java.nio.charset.Charset;
import java.nio.charset.CharsetEncoder;
import java.nio.charset.CodingErrorAction;
import java.util.ArrayList;

import javax.swing.table.AbstractTableModel;

//import forest.common.Forest;
import forest.common.NetBuffer;
//import forest.control.NetInfo;
import forest.control.console.model.AdminProfile;
import forest.control.console.model.Comtree;
import forest.control.console.model.ComtreeTableModel;
import forest.control.console.model.Iface;
import forest.control.console.model.IfaceTableModel;
import forest.control.console.model.Link;
import forest.control.console.model.LinkTableModel;
import forest.control.console.model.LogFilter;
import forest.control.console.model.Route;
import forest.control.console.model.RouteTableModel;

public class ConnectionNetMgr {
	private String nmAddr;
	private int nmPort = 30120;

	private Charset ascii;
	private CharsetEncoder enc;
	private CharBuffer cb;
	private ByteBuffer bb;
	private SocketChannel serverChan;
	private NetBuffer inBuf;

	private AdminProfile adminProfile;

	public ConnectionNetMgr() {
		adminProfile = new AdminProfile();
		setupIo();
	}

	/**
	 * Initialize a connection to Net Manager
	 */
	private void setupIo() {
		ascii = Charset.forName("US-ASCII");
		enc = ascii.newEncoder();
		enc.onMalformedInput(CodingErrorAction.IGNORE);
		enc.onUnmappableCharacter(CodingErrorAction.IGNORE);
		cb = CharBuffer.allocate(1024);
		bb = ByteBuffer.allocate(1024);
	}

	/**
	 * Connect to NetMgr
	 * 
	 * @return successful
	 */
	public boolean connectToNetMgr() {
		// open channel to server
		// on success, return true
		try {
			SocketAddress serverAdr = new InetSocketAddress(
					InetAddress.getByName(nmAddr), nmPort);
			// connect to server
			serverChan = SocketChannel.open(serverAdr);
		} catch (Exception e) {
			return false;
		}
		if (!serverChan.isConnected())
			return false;
		// add initial greeting message from NetMgr so that we can
		// detect when we're really talking to server and not just tunnel
		if (!sendString("Forest-Console-v1\n"))
			return false;
		inBuf = new NetBuffer(serverChan, 1000);
		return true;
	}

	/**
	 * Send a string msg to Net Manager
	 * 
	 * @param msg
	 *            message
	 * @return successful
	 */
	public boolean sendString(String msg) {
		bb.clear();
		cb.clear();
		cb.put(msg);
		cb.flip();
		enc.encode(cb, bb, false);
		bb.flip();
		try {
			System.out.println("SENDING: " + msg);
			serverChan.write(bb);
		} catch (Exception e) {
			e.printStackTrace();
			return false;
		}
		return true;
	}

	/**
	 * Login to a specific account.
	 * 
	 * @param user
	 *            is the name of the user whose account we're logging into
	 * @param pwd
	 *            is the password
	 * @return null if the operation succeeds, otherwise a string containing an
	 *         error message from the client manager
	 */
	public String login(String user, String pwd) {
		if (user.length() == 0 || pwd.length() == 0)
			return "missing user name or password";
		if (!sendString("login: " + user + "\n" + "password: " + pwd
				+ "\nover\n"))
			return "cannot send request to server";
		String s = inBuf.readLine();
		if (s == null || !s.equals("success")) {
			inBuf.nextLine();
			return s == null ? "unexpected response" : s;
		}
		s = inBuf.readLine();
		if (s == null || !s.equals("over"))
			return s == null ? "unexpected response" : s;

		s = getProfile(user);
		if (s != null)
			return s;
		System.out.println(adminProfile);
		return null;
	}

	/**
	 * Get a admin's profile. This method sends a request to the net manager and
	 * processes the response. The text fields in the displayed profile are
	 * updated based on the response.
	 * 
	 * @param userName
	 *            is the name of the admin whose profile is to be modified
	 * @return null if the operation succeeded, otherwise, a string containing
	 *         an error message from the net manager
	 */
	public String getProfile(String userName) {
		String s;
		boolean gotName, gotEmail;
		adminProfile.setUserName(userName);

		if (userName.length() == 0)
			return "empty user name";
		if (!sendString("getProfile: " + userName + "\nover\n"))
			return "cannot send request to server";

		gotName = gotEmail = false;

		while (true) {
			s = inBuf.readAlphas();
			if (s == null) {
				// skip
			} else if (s.equals("realName") && inBuf.verify(':')) {
				String realName = inBuf.readString();
				if (realName != null)
					adminProfile.setRealName(realName);
				else
					adminProfile.setRealName("noname");
				gotName = true;
			} else if (s.equals("email") && inBuf.verify(':')) {
				String email = inBuf.readWord();
				if (email != null)
					adminProfile.setEmail(email);
				else
					adminProfile.setEmail("nomail");
				gotEmail = true;
			} else if (s.equals("over")) {
				inBuf.nextLine();
				if (gotName && gotEmail)
					return null;
				else
					return "incomplete response";
			} else {
				return s == null ? "unexpected response " : s;
			}
			inBuf.nextLine();
		}
	}

	/**
	 * Create a new account.
	 * 
	 * @param user
	 *            is the user name for the account being created
	 * @param pwd
	 *            is the password to assign to the account
	 * @return null if the operation succeeds, otherwise a string containing an
	 *         error message from the net manager
	 */
	public String newAccount(String user, String pwd) {
		if (!sendString("newAccount: " + user + "\n" + "password: " + pwd
				+ "\nover\n"))
			return "cannot send request to server";
		String s = inBuf.readLine();
		if (s == null || !s.equals("success")) {
			inBuf.nextLine();
			return s == null ? "unexpected response" : s;
		}
		s = inBuf.readLine();
		if (s == null || !s.equals("over"))
			return s == null ? "unexpected response" : s;

		return null;
	}

	/**
	 * Update a admin's profile. This method sends a request to the net manager
	 * and checks the response.
	 * 
	 * @param userName
	 *            is the name of the admin whose profile is to be modified.
	 * @return null if the operation succeeded, otherwise, a string containing
	 *         an error message from the net manager
	 */
	public String updateProfile(String userName, String realName, String email) {
		if (userName.length() == 0)
			return "empty user name";
		if (!sendString("updateProfile: " + userName + "\n" + "realName: \""
				+ realName + "\"\n" + "email: " + email + "\n" + "over\n"))
			return "cannot send request";
		String s = inBuf.readLine();
		if (s == null || !s.equals("success")) {
			inBuf.nextLine();
			return s;
		}
		s = inBuf.readLine();
		if (s == null || !s.equals("over"))
			return s;
		return null;
	}

	/**
	 * Change an admin's password. This method sends a request to the net
	 * manager and checks the response.
	 * 
	 * @param userName
	 *            is the name of the admin whose profile is to be modified.
	 * @param pwd
	 *            is the new password to be assigned
	 * @return null if the operation succeeded, otherwise, a string containing
	 *         an error message from the net manager
	 */
	public String changePassword(String userName, String pwd) {
		if (userName.length() == 0 || pwd.length() == 0)
			return "empty user name or password";
		if (!sendString("changePassword: " + userName + " " + pwd + "\nover\n"))
			return "cannot send request to server";
		String s = inBuf.readLine();
		if (s == null || !s.equals("success")) {
			inBuf.nextLine();
			return s == null ? "unexpected response " : s;
		}
		s = inBuf.readLine();
		if (s == null || !s.equals("over"))
			return s == null ? "unexpected response " : s;
		return null;
	}

	/**
	 * Retrieve router table info(link, comtree, iface, route) and put them into
	 * AbstractTableModel class
	 * 
	 * @param tableModel
	 *            TableModel for each router info
	 * @param routerName
	 *            Router Name such as r1
	 * @return null if successful, otherwise, an error message
	 */
	public String getTable(AbstractTableModel tableModel, String routerName) {
		String type = null;
		if (tableModel instanceof ComtreeTableModel) {
			type = "Comtree";
			((ComtreeTableModel) tableModel).clear();
		} else if (tableModel instanceof LinkTableModel) {
			type = "Link";
			((LinkTableModel) tableModel).clear();
		} else if (tableModel instanceof IfaceTableModel) {
			type = "Iface";
			((IfaceTableModel) tableModel).clear();
		} else if (tableModel instanceof RouteTableModel) {
			type = "Route";
			((RouteTableModel) tableModel).clear();
		} else {
			return "no type defined";
		}
		String sendMsg = "get" + type + "Table: ";
		if (!sendString(sendMsg + routerName + "\n")) {
			return "connot send request to server";
		}
		ArrayList<String> lines = new ArrayList<String>();
		while (true) {
			String str = inBuf.readLine();
			System.out.println(str);
			if (str.equals("could not read comtree table")
					|| str.equals("could not read iface table")
					|| str.equals("could not read route table")
					|| str.equals("could not read link table")) {
				String tmp = inBuf.readLine();
				if (tmp.equals("over")) {
					return "could not read table";
				}
			} else if (str
					.equals("                                                                                                                                                                                                                                                                                                                                   over")) {
				return "could not read link table";
			}
			if (str.equals("over")) {
				if (tableModel instanceof ComtreeTableModel) {
					for (String s : lines) {
						s = s.replaceAll("\\s+", " ");
						String[] tokens = s.split(" ");
						if (tokens.length == 6) {
							((ComtreeTableModel) tableModel)
									.addComtreeTable(new Comtree(tokens[1],
											tokens[2], tokens[3], tokens[4],
											tokens[5]));
						} else if (tokens.length == 5) {
							((ComtreeTableModel) tableModel)
									.addComtreeTable(new Comtree(tokens[1],
											tokens[2], tokens[3], " ",
											tokens[4]));
						} else {
							return "tokens error";
						}
					}
				} else if (tableModel instanceof LinkTableModel) {
					for (String s : lines) {
						s = s.replaceAll("\\s+", " ");
						String[] tokens = s.split(" ");
						if (tokens.length == 9) {
							((LinkTableModel) tableModel)
									.addLinkTable(new Link(tokens[1],
											tokens[2], tokens[3], tokens[4],
											tokens[5], tokens[6]));
						} else {
							return "tokens error";
						}
					}
				} else if (tableModel instanceof IfaceTableModel) {
					for (String s : lines) {
						s = s.replaceAll("\\s+", " ");
						String[] tokens = s.split(" ");
						if (tokens.length == 4) {
							((IfaceTableModel) tableModel)
									.addIfaceTable(new Iface(tokens[1],
											tokens[2], tokens[3]));
						} else {
							return "tokens error";
						}
					}
				} else if (tableModel instanceof RouteTableModel) {
					for (String s : lines) {
						String[] tokens = s.split(" ");
						if (tokens.length == 3) {
							((RouteTableModel) tableModel)
									.addRouteTable(new Route(tokens[0],
											tokens[1], tokens[2]));
						} else {
							return "tokens error";
						}
					}
				}
				break;
			} else if (str.equals("success")) {
				continue;
			} else {
				lines.add(str);
			}
		}
		tableModel.fireTableDataChanged();
		return null;
	}

	public String addFilter(LogFilter filter) {
		String rtnName = filter.getRtn();
		String msg = "addFilter: " + rtnName + "\n";
		if (!sendString(msg))
			return "connot add log filter to server";

		String str = inBuf.readLine(); // success
		if (str.equals("could not add log filter"))
			return "could not add log filter";
		System.out.println(str);

		str = inBuf.readLine(); // id
		System.out.println(str);
		int id = Integer.valueOf(str);
		filter.setId(id);

		str = inBuf.readLine(); // over
		System.out.println(str);

		modFilter(filter);

		return null;
	}

	/**
	 * Modify log filter
	 * 
	 * @return null on success, otherwise, error message
	 */
	public String modFilter(LogFilter filter) {
		String rtnName = filter.getRtn();
		int fIndex = filter.getId(); // id
		StringBuilder filterString = new StringBuilder();

		filterString.append(1); // on
		filterString.append(" ");

		filterString.append(filter.getLink()); // link
		filterString.append(" ");

		if (filter.getIn()) // in
			filterString.append(1);
		else
			filterString.append(0);
		filterString.append(" ");

		if (filter.getOut()) // out
			filterString.append(1);
		else
			filterString.append(0);
		filterString.append(" ");

		filterString.append(filter.getComtree()); // comtree
		filterString.append(" ");

		filterString.append(filter.getSrcAdr()); // src addr
		filterString.append(" ");
		filterString.append(filter.getDstAdr());// dest addr
		filterString.append(" ");

		filterString.append(filter.getType()); // type
		filterString.append(" ");
		filterString.append(filter.getCpType()); // cpType
		filterString.append(" ");

		String msg = "modFilter: " + rtnName + " " + fIndex + "\n"
				+ filterString.toString() + "\n";
		if (!sendString(msg))
			return "connot modify log filter";

		String str = inBuf.readLine(); // success
		if (str.equals("could not modify log filter"))
			return "could not modify log filter";
		System.out.println(str);

		str = inBuf.readLine(); // over
		System.out.println(str);

		// getFilter();
		return null;
	}

	/**
	 * Drop log filter
	 * 
	 * @return
	 */
	public String dropFilter(String rtnName, int fx) {
		String msg = "dropFilter: " + rtnName + " " + fx + "\n";
		if (!sendString(msg))
			return "connot drop log filter from server";

		String str = inBuf.readLine(); // success
		if (str.equals("could not drop log filter"))
			return "could not drop log filter";
		System.out.println(str);

		str = inBuf.readLine();
		System.out.println(str);

		return null;
	}

	/**
	 * get log filter
	 * 
	 * @return
	 */
	public String getFilter() {
		String rtnName = "r1";
		String msg = "getFilter: " + rtnName + " " + "1" + "\n";
		if (!sendString(msg))
			return "connot get log filter from server";

		String str = inBuf.readLine(); // success
		if (str.equals("could not get log filter"))
			return "could not get log filter";
		System.out.println(str);

		str = inBuf.readLine();
		System.out.println(str);

		str = inBuf.readLine();
		System.out.println(str);

		return null;
	}

	/**
	 * Get log filter set
	 * 
	 * @return
	 */
	public String getFilterSet(ArrayList<String> filters) {
		String rtnName = "r1";
		String msg = "getFilterSet: " + rtnName + "\n";
		if (!sendString(msg))
			return "connot get log filter set from server";

		while (true) {
			String str = inBuf.readLine();
			if (str.equals("over")) {
				break;
			} else if (str.equals("success")) {
				continue;
			} else {
				filters.add(str);
				System.out.println(str);
			}
		}

		return null;
	}

	/**
	 * Get logged packets
	 * 
	 * @return null if successful, otherwise error message
	 */
	public String getLoggedPackets(ArrayList<String> logs) {
		String msg = "getLoggedPackets: " + "r1" + " " + "\nover\n";
		if (!sendString(msg))
			return "connot retrieve logged packets";
		while (true) {
			String str = inBuf.readLine();
			if (str.equals("over")) {
				break;
			} else if (str.equals("success")) {
				continue;
			} else {
				logs.add(str);
				System.out.println(str);
			}
		}
		return null;
	}

	/**
	 * Close socket to Net Mgr
	 */
	public void closeSocket() {
		if (serverChan != null && serverChan.isConnected()) {
			try {
				serverChan.close();
			} catch (IOException e) {
				e.printStackTrace();
			}
		}
	}

	/**
	 * Get NetMgr address
	 * 
	 * @return
	 */
	public String getNmAddr() {
		return nmAddr;
	}

	/**
	 * Set NetMgr address
	 * 
	 * @param nmAddr
	 */
	public void setNmAddr(String nmAddr) {
		this.nmAddr = nmAddr;
	}

	/**
	 * Get NetMgr port number
	 * 
	 * @return
	 */
	public int getNmPort() {
		return nmPort;
	}

	/**
	 * Set NetMgr port number, default: 30120
	 * 
	 * @param nmPort
	 */
	public void setNmPort(int nmPort) {
		this.nmPort = nmPort;
	}

	/**
	 * Get AmdinProfile
	 * 
	 * @return
	 */
	public AdminProfile getAdminProfile() {
		return adminProfile;
	}

}
