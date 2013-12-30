/** @file DBConnector.cpp 
 *
 *  @author Doowon Kim
 *  @date 2013
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "DBConnector.h"

namespace forest {

DBConnector::DBConnector() {
	dbServerAdr = "tcp://127.0.0.1:3306";
	dbId = "";
	dbPasswd = "";
	dbSchema = "";

	try {
		driver = get_driver_instance();
		con = driver->connect(dbServerAdr, dbId, dbPasswd);
		con->setSchema(dbSchema);
	} catch (sql::SQLException &e) {
		cerr << "# ERR: SQLException in " << __FILE__;
		cerr << "(" << __FUNCTION__ << ") on line " << __LINE__ << endl;
		cerr << "# ERR: " << e.what();
		cerr << " (MySQL error code: " << e.getErrorCode();
		cerr << ", SQLState: " << e.getSQLState() << " )" << endl;
	}
}

DBConnector::~DBConnector() {
	delete con; 
	// delete driver;
}

/**
 * Check if admin exists in DB.
 * @param  adminName admin name
 * @param  passwd    password
 * @return true if exists, otherwise false
 */
bool DBConnector::isAdmin(string adminName, string passwd) {
	sql::Statement *stmt;
	sql::ResultSet *res;
	
	string query = "SELECT * FROM people WHERE p_id = '" + adminName + 
					"' and p_pwd = '" + passwd + "'";
cerr << query << endl;
	try {
		stmt = con->createStatement();
		res = stmt->executeQuery(query);
		if (res->next()) {
			delete res;
			delete stmt;
			return true;
		}
	} catch (sql::SQLException &e) {
		cerr << "# ERR: SQLException in " << __FILE__;
		cerr << "(" << __FUNCTION__ << ") on line " << __LINE__ << endl;
		cerr << "# ERR: " << e.what();
		cerr << " (MySQL error code: " << e.getErrorCode();
		cerr << ", SQLState: " << e.getSQLState() << " )" << endl;
	}

	delete res;
	delete stmt;
	
	return false;
}

/**
 * Add an admin to DB. Other information such as real name and email are 
 * initially "tmp".
 * @param  newName	new admin name
 * @param  passwd 	new admin password
 * @return true on success, false on failure
 */
bool DBConnector::addAdmin(string newName, string passwd) {
	sql::Statement *stmt;
	sql::ResultSet *res;
	sql::PreparedStatement *pstmt;

	string query = "SELECT * FROM people WHERE p_id = '" + newName + "'";
cerr << query << endl;
	try {
		stmt = con->createStatement();
		res = stmt->executeQuery(query);
		if (res->next()) { // newName is already existed
			delete res;
			delete stmt;
			return false;
		} else {
			//insert people id and passwd
			query = "INSERT INTO people (p_id,p_pwd,p_name,p_email) VALUES (?,?,?,?)";
			cerr << query << endl;
			pstmt = con->prepareStatement(query);
			pstmt->setString(1, newName);
			pstmt->setString(2, passwd);
			pstmt->setString(3, "tmp");
			pstmt->setString(4, "tmp");
			pstmt->executeUpdate();
			
			//find the primary number of inserted row
			int p_no = 0;
			query = "SELECT * FROM people WHERE p_id = '" + newName + 
					"' and p_pwd = '" + passwd + "'";
			res = stmt->executeQuery(query);
			if (res->next()) {
				p_no = stoi(res->getString(1));
			}

			delete stmt; 
			delete pstmt;
			delete res;
			
			//grant roles
			query = "INSERT INTO roles_read_write (rw_people_no) VALUES (?)";
			cerr << query << endl;
			pstmt = con->prepareStatement(query);
			pstmt->setInt(1, p_no);
			pstmt->executeUpdate();
			
			delete pstmt;
			return true;
		}
	} catch (sql::SQLException &e) {
		cerr << "# ERR: SQLException in " << __FILE__;
		cerr << "(" << __FUNCTION__ << ") on line " << __LINE__ << endl;
		cerr << "# ERR: " << e.what();
		cerr << " (MySQL error code: " << e.getErrorCode();
		cerr << ", SQLState: " << e.getSQLState() << " )" << endl;
	}

	delete res;
	delete stmt;
	
	return false;
}

/**
 * Retreive an admin profile from DB. Rather than returning an adminProfile 
 * structure as a return value, it assigns the admin infomation to the 
 * adminProfile structure which is passed by reference as a parameter.
 * @param  adminName admin name
 * @param  profile   admin profile (adminProfile stucture)
 * @return true on success, false if the given adminName is not in DB
 */	
 bool DBConnector::getAdminProfile(string adminName, adminProfile *profile) {
	sql::Statement *stmt;
	sql::ResultSet *res;

	string query = "SELECT * FROM people WHERE p_id = '" + adminName + "'";
cerr << query << endl;
	try {
		stmt = con->createStatement();
		res = stmt->executeQuery(query);
		if (res->next()) { 
			profile->name = res->getString("p_id");
			profile->realName = res->getString("p_name");
			profile->email = res->getString("p_email");

			delete res;
			delete stmt;
			return true;
		}
	} catch (sql::SQLException &e) {
		cerr << "# ERR: SQLException in " << __FILE__;
		cerr << "(" << __FUNCTION__ << ") on line " << __LINE__ << endl;
		cerr << "# ERR: " << e.what();
		cerr << " (MySQL error code: " << e.getErrorCode();
		cerr << ", SQLState: " << e.getSQLState() << " )" << endl;
	}

	delete res;
	delete stmt;
	return false;
}

/**
 * Handle an update admin profile request.
 * @param  adminName admin name
 * @param  profile   admin profile (adminProfile stucture)
 * @return true on success, false if the given admin name does not exist in DB
 */
bool DBConnector::updateAdminProfile(string adminName, DBConnector::adminProfile *profile) {
	sql::PreparedStatement *pstmt;

	string query = "UPDATE people SET p_name = ?, p_email = ? "
					"WHERE p_id = '" + adminName + "'";
cerr << query << endl;
	try {
		pstmt = con->prepareStatement(query);
		pstmt->setString(1, profile->realName);
		pstmt->setString(2, profile->email);
		if (pstmt->executeUpdate()) {
			delete pstmt;
			return true;	
		}
	} catch (sql::SQLException &e) {
		cerr << "# ERR: SQLException in " << __FILE__;
		cerr << "(" << __FUNCTION__ << ") on line " << __LINE__ << endl;
		cerr << "# ERR: " << e.what();
		cerr << " (MySQL error code: " << e.getErrorCode();
		cerr << ", SQLState: " << e.getSQLState() << " )" << endl;
	}
	delete pstmt;
	return false;
}

/**
 * Handle a change password request.
 * @param  adminName admin name
 * @param  newPasswd new password
 * @return true on success, false if the gvein admin name is not in DB
 */
bool DBConnector::setPassword(string adminName, string newPasswd) {
	sql::PreparedStatement *pstmt;

	string query = "UPDATE people SET p_pwd = ? WHERE p_id = '" + adminName + "'";
cerr << query << endl;
	try {
		pstmt = con->prepareStatement(query);
		pstmt->setString(1, newPasswd);
		if (pstmt->executeUpdate()) {
			delete pstmt;
			return true;	
		}
	} catch (sql::SQLException &e) {
		cerr << "# ERR: SQLException in " << __FILE__;
		cerr << "(" << __FUNCTION__ << ") on line " << __LINE__ << endl;
		cerr << "# ERR: " << e.what();
		cerr << " (MySQL error code: " << e.getErrorCode();
		cerr << ", SQLState: " << e.getSQLState() << " )" << endl;
	}

	delete pstmt;
	return false;
}


} // ends namespace

