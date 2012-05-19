//
// Copyright (C) 2003-2004 Trevor Hogan
//

// Modified by =Xotic=

#ifndef BNBT_MYSQL_H
 #define BNBT_MYSQL_H

#if defined ( BNBT_MYSQL ) || defined ( XBNBT_MYSQL )

 #include <mysql.h>

#if defined ( BNBT_MYSQL )

 extern MYSQL *gpMySQL;
 extern string gstrMySQLHost;
 extern string gstrMySQLDatabase;
 extern string gstrMySQLUser;
 extern string gstrMySQLPassword;
 extern string gstrMySQLPrefix;
 extern unsigned int guiMySQLPort;

 extern map<pthread_t, MYSQL *> gmapMySQL;

 const string UTIL_StringToMySQL( const string &strString );
 void UTIL_MySQLCreateTables( );
 void UTIL_MySQLCreateDatabase( );

 class CMySQLQuery
 {
 public:
	CMySQLQuery( const string cstrQuery );
	virtual ~CMySQLQuery( );

	const vector<string> nextRow( );
	const map<string, string> nextRowMap( );
	const unsigned long numRows( );
	const unsigned long lastInsertID( );
 private:
	MYSQL *pMySQL;
	MYSQL_RES *m_pRes;
 };

 class CMySQLQueryLocal
 {
 public:
	CMySQLQueryLocal( );
	virtual ~CMySQLQueryLocal( );

	void query( const string cstrQuery );
	const vector<string> nextRow( );
	const map<string, string> nextRowMap( );
	const unsigned long numRows( );
	const unsigned long lastInsertID( );
 private:
	MYSQL *pMySQL;
	MYSQL_RES *m_pRes;
 };
#endif

#if defined ( XBNBT_MYSQL )

 extern MYSQL *gpMySQLUsers;
 extern string gstrMySQLUsersHost;
 extern string gstrMySQLUsersDatabase;
 extern string gstrMySQLUsersUser;
 extern string gstrMySQLUsersPassword;
 extern string gstrMySQLUsersTable;
 extern string gstrMySQLUsersAdmin;
 extern unsigned int guiMySQLUsersPort;
 extern bool gbMySQLUsersOverrideUsers;
 extern unsigned char gucMySQLUsersMemberAccess;
 extern string gstrMySQLUsersName;

const bool UTIL_StartMySQLUsers( );
//const string UTIL_StringToMySQLUsers( const string &cstrString );
const bool UTIL_OpenMySQLUsers(void);
const bool UTIL_TestMySQLUsersColumn(void);
const bool UTIL_SetMySQLUsersMemberDefaultAccess(void);
const bool UTIL_SetMySQLUsersMemberAccess( const string &cstrMySQLUsersMember, const unsigned char &cucMySQLUsersAccess );
const bool UTIL_CreateMySQLUsersColumn(void);
const bool UTIL_CloseMySQLUsers(void);

class CMySQLUsersQuery
{
public:
	CMySQLUsersQuery( const string cstrQuery );
	virtual ~CMySQLUsersQuery( );

	const vector<string> nextRow( );
private:
	MYSQL_RES *m_pRes;
};
#endif

#endif

#endif
