/***
*
* BNBT Beta 8.0 - A C++ BitTorrent Tracker
* Copyright (C) 2003-2004 Trevor Hogan
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*
***/

#if defined ( XBNBT_MYSQL ) || defined ( BNBT_MYSQL )

 #include "bnbt.h"
 #include "bnbt_mysql.h"
 #include "util.h"
 #include "config.h"
 
 #if defined ( XBNBT_MYSQL )
  #include "atom.h"
//   #include "config.h"
 #endif

#if defined ( BNBT_MYSQL )

MYSQL *gpMySQL = 0;
string gstrMySQLHost = string( );
string gstrMySQLDatabase = string( );
string gstrMySQLUser = string( );
string gstrMySQLPassword = string( );
string gstrMySQLPrefix = string( );
unsigned int guiMySQLPort = 0;

const string UTIL_StringToMySQL( const string &cstrString )
{
	char *szMySQL = new char[cstrString.size( ) * 2 + 1];

	if( gpMySQL )
		mysql_real_escape_string( gpMySQL, szMySQL, cstrString.c_str( ), ( unsigned long )cstrString.size( ) );

	const string cstrMySQL( szMySQL );

	delete [] szMySQL;

	return cstrMySQL;
}

void UTIL_MySQLCreateDatabase( )
{
		string strCreate = "CREATE DATABASE IF NOT EXISTS " + gstrMySQLDatabase ;
		
		mysql_real_query( gpMySQL, strCreate.c_str( ), ( unsigned long )strCreate.size( ) );
		mysql_store_result( gpMySQL );

		if( mysql_errno( gpMySQL ) )
			UTIL_LogPrint( "mysql error - %s\n", mysql_error( gpMySQL ) );
}

void UTIL_MySQLCreateTables( )
{
	string strCreate[26];

	strCreate[0] = "CREATE TABLE IF NOT EXISTS allowed ( bid MEDIUMINT UNSIGNED NOT NULL AUTO_INCREMENT, bhash BLOB, bfilename VARCHAR(255) NOT NULL, bname VARCHAR(255) NOT NULL, badded DATETIME NOT NULL, bsize BIGINT UNSIGNED NOT NULL DEFAULT 0, bfiles INT UNSIGNED NOT NULL DEFAULT 0, bcomment VARCHAR(255) NOT NULL, btag VARCHAR(255) NOT NULL, btitle VARCHAR(255) NOT NULL, bintr TEXT, bip VARCHAR(40) NOT NULL, buploader VARCHAR(16) NOT NULL, buploaderid MEDIUMINT UNSIGNED NOT NULL, bimdb VARCHAR(4) NOT NULL, bimdbid VARCHAR(16) NOT NULL, bimdbupdated DATETIME NOT NULL, bdefault_down SMALLINT UNSIGNED NOT NULL DEFAULT 100, bdefault_up SMALLINT UNSIGNED NOT NULL DEFAULT 100, bfree_down SMALLINT UNSIGNED NOT NULL DEFAULT 100, bfree_up SMALLINT UNSIGNED NOT NULL DEFAULT 100, bfree_time SMALLINT UNSIGNED NOT NULL DEFAULT 0, bfree_to DATETIME NOT NULL, btop TINYINT(1) UNSIGNED NOT NULL DEFAULT 0, bhl TINYINT(1) UNSIGNED NOT NULL DEFAULT 0, bclassic TINYINT(1) UNSIGNED NOT NULL DEFAULT 0, breq TINYINT(1) UNSIGNED NOT NULL DEFAULT 0, bnodownload TINYINT(1) UNSIGNED NOT NULL DEFAULT 0, bnocomment TINYINT(1) UNSIGNED NOT NULL DEFAULT 0, bseeders INT UNSIGNED NOT NULL, bseeders6 INT UNSIGNED NOT NULL, bleechers INT UNSIGNED NOT NULL, bleechers6 INT UNSIGNED NOT NULL, bcompleted INT UNSIGNED NOT NULL, bcomments MEDIUMINT UNSIGNED NOT NULL DEFAULT 0, bupdated DATETIME NOT NULL, bpost TINYINT(1) UNSIGNED NOT NULL DEFAULT 0, PRIMARY KEY( bid ), UNIQUE KEY( bhash(20) ), INDEX( bimdbid(9) ) )";
	strCreate[1] = "CREATE TABLE IF NOT EXISTS offer ( bid MEDIUMINT UNSIGNED NOT NULL AUTO_INCREMENT, bhash BLOB, bfilename VARCHAR(255) NOT NULL, bname VARCHAR(255) NOT NULL, badded DATETIME NOT NULL, bsize BIGINT UNSIGNED NOT NULL DEFAULT 0, bfiles INT UNSIGNED NOT NULL DEFAULT 0, bcomment VARCHAR(255) NOT NULL, btag VARCHAR(255) NOT NULL, btitle VARCHAR(255) NOT NULL, bintr TEXT, bip VARCHAR(40) NOT NULL, buploader VARCHAR(16) NOT NULL, buploaderid MEDIUMINT UNSIGNED NOT NULL, bimdb VARCHAR(4) NOT NULL, bimdbid VARCHAR(16) NOT NULL, bimdbupdated DATETIME NOT NULL, bseeded DATETIME NOT NULL, bcomments MEDIUMINT UNSIGNED NOT NULL DEFAULT 0, PRIMARY KEY( bid ), UNIQUE KEY( bhash(20) ), INDEX( bimdbid(9) ) )";
	strCreate[2] = "CREATE TABLE IF NOT EXISTS dstate ( bhash BLOB, bid MEDIUMINT UNSIGNED NOT NULL, bpeerid BLOB, buseragent VARCHAR(40) NOT NULL, busername VARCHAR(16) NOT NULL, buid MEDIUMINT UNSIGNED NOT NULL, bip VARCHAR(40) NOT NULL, bip6 VARCHAR(40) NOT NULL, bport SMALLINT UNSIGNED NOT NULL, bkey BLOB, buploaded BIGINT UNSIGNED NOT NULL DEFAULT 0, bdownloaded BIGINT UNSIGNED NOT NULL DEFAULT 0, bleft BIGINT UNSIGNED NOT NULL, bconnected DATETIME NOT NULL, bupdated DATETIME NOT NULL, bcompleted TINYINT(1) UNSIGNED NOT NULL DEFAULT 0, bupspeed BIGINT UNSIGNED NOT NULL DEFAULT 0, bdownspeed BIGINT UNSIGNED NOT NULL DEFAULT 0, PRIMARY KEY( bid, buid, bpeerid(20) ), INDEX( buid ) )";
	strCreate[3] = "CREATE TABLE IF NOT EXISTS peers ( bhash BLOB, bid MEDIUMINT UNSIGNED NOT NULL, busername VARCHAR(16) NOT NULL, buid MEDIUMINT UNSIGNED NOT NULL, bip VARCHAR(40) NOT NULL, buploaded BIGINT UNSIGNED NOT NULL DEFAULT 0, bdownloaded BIGINT UNSIGNED NOT NULL DEFAULT 0, bstarted DATETIME NOT NULL, bcompleted DATETIME NOT NULL, INDEX( bid, buid ), INDEX( buid ) )";
	strCreate[4] = "CREATE TABLE IF NOT EXISTS statistics ( buid MEDIUMINT UNSIGNED NOT NULL, bid MEDIUMINT UNSIGNED NOT NULL, busername VARCHAR(16) NOT NULL, bip VARCHAR(40) NOT NULL, buploaded BIGINT UNSIGNED NOT NULL DEFAULT 0, bdownloaded BIGINT UNSIGNED NOT NULL DEFAULT 0, bstarted DATETIME NOT NULL, bcompleted DATETIME NOT NULL, bdowntime INT UNSIGNED NOT NULL DEFAULT 0, bseedtime INT UNSIGNED NOT NULL DEFAULT 0, PRIMARY KEY( buid, bid ), INDEX( bid ) )";
	strCreate[5] = "CREATE TABLE IF NOT EXISTS users ( buid MEDIUMINT UNSIGNED NOT NULL AUTO_INCREMENT, busername VARCHAR(16) NOT NULL, bmd5 BLOB, bcreated DATETIME NOT NULL, bpasskey CHAR(32) NOT NULL, bemail VARCHAR(80) NOT NULL, baccess TINYINT UNSIGNED NOT NULL DEFAULT 1, bgroup TINYINT UNSIGNED NOT NULL DEFAULT 0, btitle VARCHAR(32) NOT NULL, buploaded BIGINT UNSIGNED NOT NULL DEFAULT 0, bdownloaded BIGINT UNSIGNED NOT NULL DEFAULT 0, bbonus BIGINT UNSIGNED NOT NULL DEFAULT 0, bip VARCHAR(40) NOT NULL, bseeding MEDIUMINT UNSIGNED NOT NULL DEFAULT 0, bleeching MEDIUMINT UNSIGNED NOT NULL DEFAULT 0, bseedbonus DECIMAL(8,2) NOT NULL DEFAULT 0.00, blast DATETIME NOT NULL, blast_index DATETIME NOT NULL, blast_offer DATETIME NOT NULL, blast_info DATETIME NOT NULL, bwarned DATETIME NOT NULL, bnote TEXT, binvites SMALLINT UNSIGNED NOT NULL DEFAULT 0, binviter VARCHAR(16) NOT NULL, binviterid MEDIUMINT UNSIGNED NOT NULL, btalk MEDIUMINT UNSIGNED NOT NULL DEFAULT 0, btalkref MEDIUMINT UNSIGNED NOT NULL DEFAULT 0, btalktorrent MEDIUMINT UNSIGNED NOT NULL DEFAULT 0, bperpage SMALLINT UNSIGNED NOT NULL, PRIMARY KEY( buid ), UNIQUE KEY( busername(16) ), INDEX( bpasskey(16) ), INDEX( bemail(16) ) )";
	strCreate[6] = "CREATE TABLE IF NOT EXISTS comments ( bid MEDIUMINT UNSIGNED NOT NULL AUTO_INCREMENT, bhash BLOB, btid MEDIUMINT UNSIGNED NOT NULL, boid MEDIUMINT UNSIGNED NOT NULL, busername VARCHAR(16) NOT NULL, buid MEDIUMINT UNSIGNED NOT NULL, bip VARCHAR(40) NOT NULL, bposted DATETIME NOT NULL, bcomment TEXT, PRIMARY KEY( bid ), INDEX( btid ), INDEX( boid ), INDEX( buid ) )";
	strCreate[7] = "CREATE TABLE IF NOT EXISTS messages ( bid MEDIUMINT UNSIGNED NOT NULL AUTO_INCREMENT, bsendto VARCHAR(16) NOT NULL, bsendtoid MEDIUMINT UNSIGNED NOT NULL, bfrom VARCHAR(16) NOT NULL, bfromid MEDIUMINT UNSIGNED NOT NULL, bip VARCHAR(40) NOT NULL, bsent DATETIME NOT NULL, btitle VARCHAR(255) NOT NULL, bmessage TEXT, bread TINYINT(1) UNSIGNED NOT NULL DEFAULT 0, PRIMARY KEY( bid ), INDEX( bsendtoid ) )";
	strCreate[8] = "CREATE TABLE IF NOT EXISTS messages_sent ( bid MEDIUMINT UNSIGNED NOT NULL AUTO_INCREMENT, bsendto VARCHAR(16) NOT NULL, bsendtoid MEDIUMINT UNSIGNED NOT NULL, bfrom VARCHAR(16) NOT NULL, bfromid MEDIUMINT UNSIGNED NOT NULL, bip VARCHAR(40) NOT NULL, bsent DATETIME NOT NULL, btitle VARCHAR(255) NOT NULL, bmessage TEXT, bread TINYINT(1) UNSIGNED NOT NULL DEFAULT 0, PRIMARY KEY( bid ), INDEX( bfromid ) )";
	strCreate[9] = "CREATE TABLE IF NOT EXISTS ips ( bip VARCHAR(40) NOT NULL, bcount MEDIUMINT UNSIGNED NOT NULL DEFAULT 0, PRIMARY KEY( bip(40) ) )";
	strCreate[10] = "CREATE TABLE IF NOT EXISTS announcements ( bid SMALLINT UNSIGNED NOT NULL AUTO_INCREMENT, bposted DATETIME NOT NULL, btitle VARCHAR(255) NOT NULL, bannouncement TEXT, baccess TINYINT UNSIGNED NOT NULL DEFAULT 0, PRIMARY KEY( bid ) )";
	strCreate[11] = "CREATE TABLE IF NOT EXISTS lily ( blilyid VARCHAR(16) NOT NULL, busername VARCHAR(16) NOT NULL, buid MEDIUMINT UNSIGNED NOT NULL, bpassword VARCHAR(8) NOT NULL, bdata VARCHAR(64) NOT NULL, PRIMARY KEY( blilyid(16) ) )";
	strCreate[12] = "CREATE TABLE IF NOT EXISTS bookmarks ( busername VARCHAR(16) NOT NULL, buid MEDIUMINT UNSIGNED NOT NULL, bhash BLOB, bid MEDIUMINT UNSIGNED NOT NULL, bshare TINYINT(1) UNSIGNED NOT NULL DEFAULT 0, PRIMARY KEY( buid, bid ), INDEX( bid ) )";
	strCreate[13] = "CREATE TABLE IF NOT EXISTS friends ( buid MEDIUMINT UNSIGNED NOT NULL, bfriendid MEDIUMINT UNSIGNED NOT NULL, bfriendname VARCHAR(16) NOT NULL, PRIMARY KEY( buid, bfriendid ), INDEX( bfriendid ) )";
	strCreate[14] = "CREATE TABLE IF NOT EXISTS invites ( bcode CHAR(32) NOT NULL, bownerid MEDIUMINT UNSIGNED NOT NULL, bcreated DATETIME NOT NULL, bused TINYINT(1) UNSIGNED NOT NULL DEFAULT 0, binvitee VARCHAR(16) NOT NULL, binviteeid MEDIUMINT UNSIGNED NOT NULL, PRIMARY KEY( bcode(16) ), INDEX( bownerid ) )";
	strCreate[15] = "CREATE TABLE IF NOT EXISTS thanks ( bid MEDIUMINT UNSIGNED NOT NULL, bthankerid MEDIUMINT UNSIGNED NOT NULL, bthanker VARCHAR(16) NOT NULL, bthanktime DATETIME NOT NULL, PRIMARY KEY( bid, bthankerid ) )";
	strCreate[16] = "CREATE TABLE IF NOT EXISTS subs ( bid MEDIUMINT UNSIGNED NOT NULL AUTO_INCREMENT, btid MEDIUMINT UNSIGNED NOT NULL, buid MEDIUMINT UNSIGNED NOT NULL, busername VARCHAR(16) NOT NULL, bsub VARCHAR(128) NOT NULL, bfilename VARCHAR(255) NOT NULL, bname VARCHAR(255) NOT NULL, buploadtime DATETIME NOT NULL, PRIMARY KEY( bid ), INDEX( btid ) )";
	strCreate[17] = "CREATE TABLE IF NOT EXISTS talk ( bid MEDIUMINT UNSIGNED NOT NULL AUTO_INCREMENT, busername VARCHAR(16) NOT NULL, buid MEDIUMINT UNSIGNED NOT NULL, bip VARCHAR(40) NOT NULL, bposted DATETIME NOT NULL, btalk TEXT, btalkstore TEXT, breply MEDIUMINT UNSIGNED NOT NULL, breplyto VARCHAR(16) NOT NULL, breplytoid MEDIUMINT UNSIGNED NOT NULL, brt MEDIUMINT UNSIGNED NOT NULL, brtto VARCHAR(16) NOT NULL, brttoid MEDIUMINT UNSIGNED NOT NULL, PRIMARY KEY( bid ), INDEX( buid ) )";
	strCreate[18] = "CREATE TABLE IF NOT EXISTS talkhome ( buid MEDIUMINT UNSIGNED NOT NULL, bfriendid MEDIUMINT UNSIGNED NOT NULL, btalkid MEDIUMINT UNSIGNED NOT NULL, bposted DATETIME NOT NULL, PRIMARY KEY( buid, btalkid ), INDEX( bfriendid ), INDEX( btalkid ) )";
	strCreate[19] = "CREATE TABLE IF NOT EXISTS talkref ( busername VARCHAR(16) NOT NULL, buid MEDIUMINT UNSIGNED NOT NULL, breferid MEDIUMINT UNSIGNED NOT NULL, brefid MEDIUMINT UNSIGNED NOT NULL, bposted DATETIME NOT NULL, PRIMARY KEY( buid, brefid ), INDEX( breferid ), INDEX( brefid ) )";
	strCreate[20] = "CREATE TABLE IF NOT EXISTS talktag ( btag VARCHAR(40) NOT NULL, bid MEDIUMINT UNSIGNED NOT NULL, bposted DATETIME NOT NULL, PRIMARY KEY( btag, bid ), INDEX( bid ) )";
	strCreate[21] = "CREATE TABLE IF NOT EXISTS talktorrent ( buid MEDIUMINT UNSIGNED NOT NULL, bfriendid MEDIUMINT UNSIGNED NOT NULL, btid MEDIUMINT UNSIGNED NOT NULL, bposted DATETIME NOT NULL, PRIMARY KEY( buid, btid ), INDEX( bfriendid ), INDEX( btid ) )";
	strCreate[22] = "CREATE TABLE IF NOT EXISTS users_prefs ( buid MEDIUMINT UNSIGNED NOT NULL, bdefaulttag VARCHAR(80) NOT NULL, baddedpassed TINYINT(1) UNSIGNED NOT NULL DEFAULT 0, bperpage SMALLINT UNSIGNED NOT NULL, bsavesent TINYINT(1) UNSIGNED NOT NULL DEFAULT 0, bmsgcomment TINYINT(1) UNSIGNED NOT NULL DEFAULT 0, PRIMARY KEY( buid ) )";
	strCreate[23] = "CREATE TABLE IF NOT EXISTS votes ( bid SMALLINT UNSIGNED NOT NULL AUTO_INCREMENT, btitle VARCHAR(255) NOT NULL, bcreated DATETIME NOT NULL, bopen DATETIME NOT NULL, bclosed DATETIME NOT NULL, bvotecount MEDIUMINT UNSIGNED NOT NULL DEFAULT 0, PRIMARY KEY( bid ) )";
	strCreate[24] = "CREATE TABLE IF NOT EXISTS votesoption ( bid SMALLINT UNSIGNED NOT NULL, boptionid TINYINT UNSIGNED NOT NULL, boption VARCHAR(255) NOT NULL, bvotecount MEDIUMINT UNSIGNED NOT NULL DEFAULT 0, PRIMARY KEY( bid, boptionid ) )";
	strCreate[25] = "CREATE TABLE IF NOT EXISTS votesticket ( bid SMALLINT UNSIGNED NOT NULL, buid MEDIUMINT UNSIGNED NOT NULL, boptionid TINYINT UNSIGNED NOT NULL, bvotetime DATETIME NOT NULL, PRIMARY KEY( bid, buid ) )";
//	strCreate[12] = "CREATE TABLE IF NOT EXISTS allowed_cache ( bid MEDIUMINT UNSIGNED NOT NULL AUTO_INCREMENT, bfilename VARCHAR(255) NOT NULL, bname VARCHAR(255) NOT NULL, badded DATETIME NOT NULL, bsize BIGINT UNSIGNED NOT NULL DEFAULT 0, bfiles INT UNSIGNED NOT NULL DEFAULT 0, bcomment VARCHAR(255) NOT NULL, btag VARCHAR(255) NOT NULL, btitle VARCHAR(255) NOT NULL, bip VARCHAR(40) NOT NULL, buploader VARCHAR(16) NOT NULL, buploaderid MEDIUMINT UNSIGNED NOT NULL, bimdb VARCHAR(4) NOT NULL, bimdbid VARCHAR(16) NOT NULL, bdefault_down SMALLINT UNSIGNED NOT NULL DEFAULT 100, bdefault_up SMALLINT UNSIGNED NOT NULL DEFAULT 100, bfree_down SMALLINT UNSIGNED NOT NULL DEFAULT 100, bfree_up SMALLINT UNSIGNED NOT NULL DEFAULT 100, bfree_to DATETIME NOT NULL, btop TINYINT(1) UNSIGNED NOT NULL DEFAULT 0, bhl TINYINT(1) UNSIGNED NOT NULL DEFAULT 0, bclassic TINYINT(1) UNSIGNED NOT NULL DEFAULT 0, breq TINYINT(1) UNSIGNED NOT NULL DEFAULT 0, bnodownload TINYINT(1) UNSIGNED NOT NULL DEFAULT 0, bseeders INT UNSIGNED NOT NULL, bleechers INT UNSIGNED NOT NULL, bcompleted INT UNSIGNED NOT NULL, bcomments MEDIUMINT UNSIGNED NOT NULL DEFAULT 0, PRIMARY KEY( bid ), INDEX( bimdbid(8) ) ) ENGINE=MEMORY";
//	strCreate[13] = "TRUNCATE TABLE allowed_cache";
//	strCreate[14] = "INSERT INTO allowed_cache SELECT bid,bfilename,bname,badded,bsize,bfiles,bcomment,btag,btitle,bip,buploader,buploaderid,bimdb,bimdbid,bdefault_down,bdefault_up,bfree_down,bfree_up,bfree_to,btop,bhl,bclassic,breq,bnodownload,bseeders,bleechers,bcompleted,bcomments FROM allowed";
	
	unsigned char ucQueries = 0;

// 	if( CFG_GetInt( "mysql_override_dstate", 0 ) == 0 ? false : true )
		ucQueries = sizeof( strCreate ) / sizeof( string );

	for( unsigned char ucLoop = 0; ucLoop < ucQueries; ucLoop++ )
	{
		mysql_real_query( gpMySQL, strCreate[ucLoop].c_str( ), ( unsigned long )strCreate[ucLoop].size( ) );
		
		mysql_store_result( gpMySQL );

		if( mysql_errno( gpMySQL ) )
			UTIL_LogPrint( "mysql error - %s\n", mysql_error( gpMySQL ) );
	}

}

CMySQLQuery :: CMySQLQuery( const string cstrQuery )
{
	mysql_real_query( gpMySQL, cstrQuery.c_str( ), ( unsigned long )cstrQuery.size( ) );

	m_pRes = mysql_store_result( gpMySQL );

	if( mysql_errno( gpMySQL ) )
	{
		m_pRes = 0;

//		if( gbDebug )
			UTIL_LogPrint( "mysql error - %s\n", cstrQuery.c_str( ) );
			UTIL_LogPrint( "mysql error - %s\n", mysql_error( gpMySQL ) );
	}
}

CMySQLQuery :: ~CMySQLQuery( )
{
	if( m_pRes )
		mysql_free_result( m_pRes );
}

const vector<string> CMySQLQuery :: nextRow( )
{
	vector<string> vecReturn;

	if( m_pRes )
	{
		const unsigned int num_fields( mysql_num_fields( m_pRes ) );

		vecReturn.reserve( num_fields );

		const MYSQL_ROW row( mysql_fetch_row( m_pRes ) );

		if( row != 0 )
		{
			const unsigned long *lengths( mysql_fetch_lengths( m_pRes ) );

			for( unsigned long ulCount = 0; ulCount < num_fields; ulCount++ )
				vecReturn.push_back( string( row[ulCount], lengths[ulCount] ) );
		}
	}

	return vecReturn;
}

const map<string, string> CMySQLQuery :: nextRowMap( )
{
	map<string, string> mapReturn;

	if( m_pRes )
	{
		const unsigned int num_fields( mysql_num_fields( m_pRes ) );

		const MYSQL_ROW row( mysql_fetch_row( m_pRes ) );
		
		const MYSQL_FIELD *fields( mysql_fetch_fields( m_pRes ) );

		if( row != 0 )
		{
			const unsigned long *lengths( mysql_fetch_lengths( m_pRes ) );

			for( unsigned long ulCount = 0; ulCount < num_fields; ulCount++ )
				mapReturn[fields[ulCount].name] = string( row[ulCount], lengths[ulCount] );
		}
	}

	return mapReturn;
}

const unsigned long CMySQLQuery :: numRows( )
{

	if( m_pRes )
	{
		return ( unsigned long )mysql_num_rows( m_pRes );
	}

	return 0;
}

const unsigned long CMySQLQuery :: lastInsertID( )
{

	if( m_pRes == 0 && mysql_field_count( gpMySQL ) == 0 && mysql_insert_id( gpMySQL ) != 0 )
	{
		return ( unsigned long )mysql_insert_id( gpMySQL );
	}

	return 0;
}
#endif

/***
*
* XBNBT Beta 81b.3.5 - A C++ BitTorrent Tracker
* Copyright (C) 2003-2004 =Xotic=
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public

* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*
***/

#if defined ( XBNBT_MYSQL )

MYSQL *gpMySQLUsers = 0;
string gstrMySQLUsersHost = string( );
string gstrMySQLUsersDatabase = string( );
string gstrMySQLUsersUser = string( );
string gstrMySQLUsersPassword = string( );
string gstrMySQLUsersTable = string( );
string gstrMySQLUsersAdmin = string( );
unsigned int guiMySQLUsersPort = 0;
bool gbMySQLUsersOverrideUsers = false;
unsigned char gucMySQLUsersMemberAccess = 0;
string gstrMySQLUsersName = string( );

const bool UTIL_StartMySQLUsers( )
{
	bool bResult = false;
	unsigned char ucCount = 0;

	MILLISLEEP( 100 );

	while( !bResult && ( ucCount < 5 ) )
	{
		bResult = UTIL_OpenMySQLUsers( );

		ucCount++;

		MILLISLEEP( 1000 );
	}

	if( bResult )
	{
		if( UTIL_TestMySQLUsersColumn( ) )
			bResult = UTIL_SetMySQLUsersMemberDefaultAccess( );
		else
			bResult = UTIL_CreateMySQLUsersColumn( );

		if( bResult )
			bResult = UTIL_SetMySQLUsersMemberAccess( gstrMySQLUsersAdmin, 127 );
	}

	return bResult;
}

/*
const string UTIL_StringToMySQLUsers( const string &cstrString )
{
	char *szMySQLUsers = new char[cstrString.size( ) * 2 + 1];

	if( gpMySQLUsers )
		mysql_real_escape_string( gpMySQLUsers, szMySQLUsers, cstrString.c_str( ), ( unsigned long )cstrString.size( ) );

	const string cstrMySQLUsers( szMySQLUsers );

	delete [] szMySQLUsers;

	return cstrMySQLUsers;
}
*/

const bool UTIL_OpenMySQLUsers(void)
{
	// Start MySQL Users mysql
	gpMySQLUsers = mysql_init( 0 );

	if( !gpMySQLUsers )
	{
		if( gbDebug )
			UTIL_LogPrint( string( gmapLANG_CFG["init_mysql_users"] + "\n" ).c_str( ), mysql_error( gpMySQLUsers ) );
		
		return false;
	}
	
	if( !( mysql_real_connect( gpMySQLUsers, gstrMySQLUsersHost.c_str( ), gstrMySQLUsersUser.c_str( ), gstrMySQLUsersPassword.c_str( ), gstrMySQLUsersDatabase.c_str( ), guiMySQLUsersPort, 0, 0 ) ) )
	{
		if( gbDebug )
			UTIL_LogPrint( string( gmapLANG_CFG["connect_mysql_users"] + "\n" ).c_str( ), mysql_error( gpMySQLUsers ) );
		
		return false;
	}

	if( gbDebug )
		UTIL_LogPrint( string( gmapLANG_CFG["success_mysql_users"] + "\n" ).c_str( ) );
	
	return true;
}

const bool UTIL_TestMySQLUsersColumn(void)
{
	const string cstrQuery( "SELECT xbnbt_perm FROM " + gstrMySQLUsersTable + " LIMIT 1" );

	mysql_real_query( gpMySQLUsers, cstrQuery.c_str( ), ( unsigned long )cstrQuery.size( ) );

	mysql_store_result( gpMySQLUsers );

	if( mysql_errno( gpMySQLUsers ) )
	{
		if( gbDebug )
			UTIL_LogPrint( string( gmapLANG_CFG["no_mysql_users_column"] + "\n" ).c_str( ), mysql_error( gpMySQLUsers ) );
		
		return false;
	}

	if( gbDebug )
		UTIL_LogPrint( string( gmapLANG_CFG["found_mysql_users_column"] + "\n" ).c_str( ) );
	
	return true;
}

const bool UTIL_SetMySQLUsersMemberDefaultAccess(void)
{
	const string cstrQuery( "ALTER TABLE " + gstrMySQLUsersTable + " ALTER COLUMN xbnbt_perm SET DEFAULT \'" + CAtomInt( gucMySQLUsersMemberAccess ).toString( ) + "\'" );
	
	mysql_real_query( gpMySQLUsers, cstrQuery.c_str( ), ( unsigned long )cstrQuery.size( ) );

	mysql_store_result( gpMySQLUsers );
	
	if( mysql_errno( gpMySQLUsers ) )
	{
		if( gbDebug )
			UTIL_LogPrint( string( gmapLANG_CFG["not_set_default_access"] + "\n" ).c_str( ), mysql_error( gpMySQLUsers ) );
		
		return false;
	}

	if( gbDebug )
		UTIL_LogPrint( string( gmapLANG_CFG["set_default_access"] + "\n" ).c_str( ) );
	
	return true;
}

const bool UTIL_SetMySQLUsersMemberAccess( const string &cstrMySQLUsersMember, const unsigned char &cucMySQLUsersAccess )
{
	const string cstrQuery( "UPDATE " + gstrMySQLUsersTable + " SET xbnbt_perm=\'" + CAtomInt( cucMySQLUsersAccess ).toString( ) + "\' WHERE " + gstrMySQLUsersName + "=\'" + cstrMySQLUsersMember + "\'" );

	mysql_real_query( gpMySQLUsers, cstrQuery.c_str( ), ( unsigned long )cstrQuery.size( ) );

	mysql_store_result( gpMySQLUsers );
	
	if( mysql_errno( gpMySQLUsers ) )
	{
		if( gbDebug )
			UTIL_LogPrint( string( gmapLANG_CFG["not_set_member_access"] + "\n" ).c_str( ), mysql_error( gpMySQLUsers ) );
		
		return false;
	}

	if( gbDebug )
		UTIL_LogPrint( string( gmapLANG_CFG["set_member_access"] + "\n" ).c_str( ) );

	return true;
}

const bool UTIL_CreateMySQLUsersColumn(void)
{
	const string cstrQuery( "ALTER TABLE " + gstrMySQLUsersTable + " ADD COLUMN xbnbt_perm SMALLINT DEFAULT \'" + CAtomInt( gucMySQLUsersMemberAccess ).toString( ) + "\'" );

	mysql_real_query( gpMySQLUsers, cstrQuery.c_str( ), ( unsigned long )cstrQuery.size( ) );

	mysql_store_result( gpMySQLUsers );

	if( mysql_errno( gpMySQLUsers ) )
	{
		if( gbDebug )
			UTIL_LogPrint( string( gmapLANG_CFG["not_created_access_column"] + "\n" ).c_str( ), mysql_error( gpMySQLUsers ) );

		return false;
	}

	if( gbDebug )
		UTIL_LogPrint( string( gmapLANG_CFG["created_access_column"] + "\n" ).c_str( ) );
	return true;
}

const bool UTIL_CloseMySQLUsers(void)
{
	mysql_close( gpMySQLUsers );

	if( mysql_errno( gpMySQLUsers ) )
	{
		if( gbDebug )
			UTIL_LogPrint( string( gmapLANG_CFG["not_closed_mysql_users"] + "\n" ).c_str( ), mysql_error( gpMySQLUsers ) );

		return false;
	}

	if( gbDebug )
		UTIL_LogPrint( string( gmapLANG_CFG["closed_mysql_users"] + "\n" ).c_str( ) );

	return true;
}

CMySQLUsersQuery :: CMySQLUsersQuery( const string cstrQuery )
{
	if( gbDebug )
		UTIL_LogPrint( string( gmapLANG_CFG["mysql_users_query"] + "\n" ).c_str( ), cstrQuery.c_str( ) );

	if( !mysql_ping( gpMySQLUsers ) )
	{
		mysql_real_query( gpMySQLUsers, cstrQuery.c_str( ), ( unsigned long )cstrQuery.size( ) );

		m_pRes = mysql_store_result( gpMySQLUsers );

		if( mysql_errno( gpMySQLUsers ) )
		{
			m_pRes = 0;

			if( gbDebug )
				UTIL_LogPrint( string( gmapLANG_CFG["mysql_users_query_error"] + "\n" ).c_str( ), mysql_error( gpMySQLUsers ) );
		}
	}
	else
	{
		if( mysql_errno( gpMySQLUsers ) )
		{
			m_pRes = 0;
			
			if( gbDebug )
				UTIL_LogPrint( string( gmapLANG_CFG["ping_error"] + "\n" ).c_str( ), mysql_error( gpMySQLUsers ) );
		}
	}
}

CMySQLUsersQuery :: ~CMySQLUsersQuery( )
{
	if( m_pRes )
		mysql_free_result( m_pRes );
}

const vector<string> CMySQLUsersQuery :: nextRow( )
{
	vector<string> vecReturn;

	if( m_pRes )
	{
		const unsigned int num_fields( mysql_num_fields( m_pRes ) );

		vecReturn.reserve( num_fields );

		const MYSQL_ROW row( mysql_fetch_row( m_pRes ) );

		if( row )
		{
			const unsigned long *lengths( mysql_fetch_lengths( m_pRes ) );

			for( unsigned long ulCount = 0; ulCount < num_fields; ulCount++ )
				vecReturn.push_back( string( row[ulCount], lengths[ulCount] ) );
		}
	}

	return vecReturn;
}

 #endif

#endif
