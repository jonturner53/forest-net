#ifndef CLIENTMGR_H
#define CLIENTMGR_H

#include "CommonDefs.h"
#include <mysql++/mysql++.h>
#include <mysql++/ssqls.h>
class SqlProxy {
public:
	void run(uint32_t);		///< main run method, takes in finTime
	bool init(ipa_t,ipa_t); ///< initialize sockets
	ipa_t extIp;		///< address for tcp connections from "external" hosts
	ipa_t intIp;

	const static int LISTEN_PORT = 30190; ///< TCP port to listen for avatars on
	const static int UPDATE_PERIOD = 50;

	mysqlpp::Connection * sqlconn; //< pointer to mysql connection
	
	int tcpSockInt;		///< Listen for connections on internal ip
	int tcpSockExt;		///< Listen for connections on external ip
	int sqlSock;		///< Avatar connection socket
};
sql_create_3(user_pass,1,3,
  mysqlpp::sql_int, id,
  mysqlpp::sql_varchar, user,
  mysqlpp::sql_varchar, pass)

#endif
