package forest.control.console.model;

public class AdminProfile {
	private String userName;
	private String email;
	private String realName;
	
	public AdminProfile(){}
	public AdminProfile(String userName, String realName, String email){
		this.userName = userName;
		this.realName = realName;
		this.email = email;
	}

	public String getUserName() {
		return userName;
	}

	public void setUserName(String userName) {
		this.userName = userName;
	}

	public String getEmail() {
		return email;
	}

	public void setEmail(String email) {
		this.email = email;
	}

	public String getRealName() {
		return realName;
	}

	public void setRealName(String realName) {
		this.realName = realName;
	}
	
	public String toString(){
		return userName + " " + realName + " " + email;
	}
}
