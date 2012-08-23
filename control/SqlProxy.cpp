#include "stdinc.h"
#include "SqlProxy.h"

int main(int argc, char *argv[]){
	uint32_t finTime;
	ipa_t intIp, extIp;
	if(!argc == 2 ||
	    (sscanf(argv[3],"%d",&finTime) != 1) ||
	    (intIp = Np4d::ipAddress(argv[1])) == 0 ||
	    (extIp = Np4d::ipAddress(argv[2])) == 0)
		fatal("usage: SqlProxy intIp extIp runTime");

	if (extIp == Np4d::ipAddress("127.0.0.1")) extIp = Np4d::myIpAddress(); 
	if (extIp == 0) fatal("can't retrieve default IP address");

	SqlProxy sp;
	if(!sp.init(intIp,extIp)) fatal("Failed to init client proxy sockets");
	sp.run(1000000*finTime);
}

bool SqlProxy::init(ipa_t intIp, ipa_t extIp) {
	tcpSockInt = Np4d::streamSocket();
	tcpSockExt = Np4d::streamSocket();
	sqlSock = -1;
	if(tcpSockInt < 0 || tcpSockExt < 0 ||
	   !Np4d::bind4d(tcpSockInt,intIp,30190) ||
	   !Np4d::bind4d(tcpSockExt,extIp,30191) ||
	   !Np4d::listen4d(tcpSockInt) || !Np4d::nonblock(tcpSockInt) ||
	   !Np4d::listen4d(tcpSockExt) || !Np4d::nonblock(tcpSockExt)) {
		cerr << "failed to initialize external socket" << endl;
		return false;
	}
	try {
		sqlconn = new mysqlpp::Connection("forest","/tmp/mysql.sock","root","");
	} catch(const mysqlpp::Exception& er) {
		fatal("cannot connect to mysql db");
	}
	return true;
}

void SqlProxy::run(uint32_t runTime) {
	uint32_t now,nextTime;
	now = nextTime = Util::getTime();
	while(now <= runTime) {
		ipa_t avIp; ipp_t avPort;
		sqlSock = Np4d::accept4d(tcpSockInt,avIp,avPort);
		if(sqlSock <= 0) sqlSock = Np4d::accept4d(tcpSockExt,avIp,avPort);
		if(sqlSock > 0) {
			char* sqlStr = new char[500];
			if(Np4d::recvBuf(sqlSock,sqlStr,500) <= 0) cerr << "couldn't receive sql string\n";
			vector<user_pass> results;
			try {
				mysqlpp::Query q = sqlconn->query();
				q << sqlStr;
				q.storein(results);
				if(results.size() > 0) {
					string buf = (string)results[0].pass;
					Np4d::sendBuf(sqlSock,(char*)buf.c_str(),buf.size()+1);
				}
			} catch(mysqlpp::Exception e) {
				cerr << "MySQL error\n";
			}
			//send sql result back
			close(sqlSock);
			sqlSock = -1;
		}
		nextTime += 1000*UPDATE_PERIOD;
                now = Misc::getTime();
                useconds_t delay = nextTime - now;
                if (delay < ((uint32_t) 1) << 31) usleep(delay);
                else nextTime = now + 1000*UPDATE_PERIOD;
	}
}
