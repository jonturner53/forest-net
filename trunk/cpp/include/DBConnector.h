/** @file DBConnector.h
 *
 *  @author Doowon Kim
 *  @date 2013
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef DBCONNECTOR_H
#define DBCONNECTOR_H

//#include <stdlib.h>
//#include <iostream>

#include "mysql_connection.h"

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>

using namespace std;

namespace forest {

class DBConnector {
private:
	string dbServerAdr;
	string dbId;
	string dbPasswd;
	string dbSchema;

	sql::Driver *driver;
 	sql::Connection *con;

public:
	 struct adminProfile {
 		string name;
 		string realName;
 		string email;
 	};
	DBConnector();
	~DBConnector();
	bool isAdmin(string adminName, string passwd);
	bool addAdmin(string newName, string passwd);
	bool getAdminProfile(string adminName, adminProfile *profile);
	bool updateAdminProfile(string adminName, adminProfile *profile);
	bool setPassword(string adminName, string newPasswd);
};

} // ends namespace


#endif