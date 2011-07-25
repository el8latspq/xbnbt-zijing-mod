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

// =Xotic= Modified Source File

#if defined ( WIN32 )
 #include <io.h>
 #include <time.h>
#else
 #include <stdarg.h>
 #include <sys/time.h>
#endif

#include "bnbt.h"
#include "atom.h"
#include "bencode.h"
#include "config.h"
#include "sha1.h"
#include "tracker.h"
#include "util.h"

void UTIL_LogPrint( const char *format, ... )
{
	gmtxOutput.Claim( );

	time_t tNow = time( 0 );
	char *szTime = asctime( localtime( &tNow ) );
	szTime[strlen( szTime ) - 1] = TERM_CHAR;

	printf( "[%s] ", szTime );

	va_list printargs;
	va_start( printargs, format );
	vprintf( format, printargs );
	va_end( printargs );

	if( !gstrErrorLogDir.empty( ) && !gstrErrorLogFilePattern.empty( ) )
	{
		char pTime[256];
		memset( pTime, 0, sizeof( pTime ) / sizeof( char ) );
		strftime( pTime, sizeof( pTime ) / sizeof( char ), gstrErrorLogFilePattern.data( ), localtime( &tNow ) );

		const string cstrFile( gstrErrorLogDir + pTime );

		if( gstrErrorLogFile != cstrFile )
		{
			// start a new log

			gstrErrorLogFile = cstrFile;

			if( gpErrorLog )
				fclose( gpErrorLog );

			gpErrorLog = FILE_ERROR;

			gpErrorLog = fopen( cstrFile.c_str( ), "ab" );
		}

		if( gpErrorLog != FILE_ERROR )
		{
			fprintf( gpErrorLog, "[%s] ", szTime );

			va_list args;
			va_start( args, format );
			vfprintf( gpErrorLog, format, args );
			va_end( args );

			gulErrorLogCount++;

			if( gulErrorLogCount % guiFlushInterval == 0 )
				fflush( gpErrorLog );
		}
	}

	gmtxOutput.Release( );	  
}

void UTIL_LogFilePrint( const char *format, ... )
{
	gmtxOutput.Claim( );

	time_t tNow = time( 0 );
	char *szTime = asctime( localtime( &tNow ) );
	szTime[strlen( szTime ) - 1] = TERM_CHAR;

	printf( "[%s] ", szTime );

	va_list printargs;
	va_start( printargs, format );
	vprintf( format, printargs );
	va_end( printargs );

	if( !gstrLogDir.empty( ) && !gstrLogFilePattern.empty( ) )
	{
		char pTime[256];
		memset( pTime, 0, sizeof( pTime ) / sizeof( char ) );
		strftime( pTime, sizeof( pTime ) / sizeof( char ), gstrLogFilePattern.data( ), localtime( &tNow ) );

		const string cstrFile( gstrLogDir + pTime );

		if( gstrLogFile != cstrFile )
		{
			// start a new log

			gstrLogFile = cstrFile;

			if( gpLog )
				fclose( gpLog );

			gpLog = FILE_ERROR;

			gpLog = fopen( cstrFile.c_str( ), "ab" );
		}

		if( gpLog != FILE_ERROR )
		{
			fprintf( gpLog, "[%s] ", szTime );

			va_list args;
			va_start( args, format );
			vfprintf( gpLog, format, args );
			va_end( args );

			gulLogCount++;

			if( gulLogCount % guiFlushInterval == 0 )
				fflush( gpLog );
		}
	}

	gmtxOutput.Release( );	  
}

void UTIL_AccessLogPrint( const string &cstrIP, const string &cstrUser, const string &cstrRequest, const unsigned int &cuiStatus, const unsigned int &cuiBytes )
{
	gmtxOutput.Claim( );

	if( !gstrAccessLogDir.empty( ) && !gstrAccessLogFilePattern.empty( ) )
	{
		time_t tNow = time( 0 );

		char pTime[256];
		memset( pTime, 0, sizeof( pTime ) / sizeof( char ) );
		strftime( pTime, sizeof( pTime ) / sizeof( char ), gstrAccessLogFilePattern.data( ), localtime( &tNow ) );

		const string cstrFile( gstrAccessLogDir + pTime );

		if( gstrAccessLogFile != cstrFile )
		{
			// start a new log

			gstrAccessLogFile = cstrFile;

			if( gpAccessLog )
				fclose( gpAccessLog );

			gpAccessLog = FILE_ERROR;

			gpAccessLog = fopen( cstrFile.c_str( ), "ab" );

			if( gpAccessLog == FILE_ERROR)
				UTIL_LogPrint( string( gmapLANG_CFG["access_log"] + "\n" ).c_str( ), cstrFile.c_str( ) );
		}

		if( gpAccessLog )
		{
			fprintf( gpAccessLog, "%s - ", cstrIP.c_str( ) );

			if( cstrUser.empty( ) )
				fprintf( gpAccessLog, "- " );
			else
				fprintf( gpAccessLog, "%s ", cstrUser.c_str( ) );

			strftime( pTime, sizeof( pTime ) / sizeof( char ), "%d/%b/%Y:%H:%M:%S", localtime( &tNow ) );

#if defined( __APPLE__ ) || defined( __FREEBSD__ ) || defined( __OPENBSD__ ) || defined( __NETBSD__ )
			long timezone = -( localtime( &tNow )->tm_gmtoff );
#endif

			// timezone has the wrong sign, change it

			if( timezone > 0 )
				fprintf( gpAccessLog, "[%s -%02d%02d] ", pTime, abs( timezone / 3600 ) % 60, abs( timezone / 60 ) % 60 );
			else
				fprintf( gpAccessLog, "[%s +%02d%02d] ", pTime, abs( timezone / 3600 ) % 60, abs( timezone / 60 ) % 60 );

			fprintf( gpAccessLog, "\"%s\" %d %d\n", cstrRequest.c_str( ), cuiStatus, cuiBytes );

			gulAccessLogCount++;

			if( gulAccessLogCount % guiFlushInterval == 0 )
				fflush( gpAccessLog );
		}
	}

	gmtxOutput.Release( );	
}

// addition by labarks	

static inline const string addzero( const unsigned long &culLong )
{
	if( culLong < 10 )
		return string( "0" ) + CAtomLong( culLong ).toString( );
	else
		return CAtomLong( culLong ).toString( );
}

const string UTIL_AddedToDate( const string &cstrAdded )
{
	time_t tRawtime;
	struct tm * tDate;
	string strDate = string( );

	time ( &tRawtime );
	tDate = localtime ( &tRawtime );
	tDate->tm_year	= atoi( cstrAdded.substr( 0, 4 ).c_str( ) ) - 1900;
	tDate->tm_mon	= atoi( cstrAdded.substr( 5, 2 ).c_str( ) ) - 1;
	tDate->tm_mday  = atoi( cstrAdded.substr( 8, 2 ).c_str( ) );
	tDate->tm_hour	= atoi( cstrAdded.substr( 11, 2 ).c_str( ) );
	tDate->tm_min	= atoi( cstrAdded.substr( 14, 2 ).c_str( ) );
	tDate->tm_sec	= atoi( cstrAdded.substr( 17, 2 ).c_str( ) );
	mktime(tDate);

	char pTime[256];
	memset( pTime, 0, sizeof( pTime ) / sizeof( char ) );
	strftime( pTime, sizeof( pTime ) / sizeof( char ), "%a, %d %b %Y %H:%M:%S", tDate );

#if defined( __APPLE__ ) || defined( __FREEBSD__ ) || defined( __OPENBSD__ ) || defined( __NETBSD__ )
	time_t tNow = time( 0 );
	long timezone = -( localtime( &tNow )->tm_gmtoff );
#endif

	// timezone has the wrong sign, change it

	if( timezone > 0 )
		strDate = (string) pTime + " -" + addzero( abs( timezone / 3600 ) % 60 ) + addzero( abs( timezone / 60 ) % 60 );
	else
		strDate = (string) pTime + " +" + addzero( abs( timezone / 3600 ) % 60 ) + addzero( abs( timezone / 60 ) % 60 );

	return strDate;
}	

// end addition

//
// shamelessly stolen from wget source code
//

const struct bnbttv UTIL_CurrentTime( )
{
	struct bnbttv btv;

#ifdef WIN32
	FILETIME tFiletime;
	SYSTEMTIME tSystemtime;
	GetSystemTime( &tSystemtime );
	SystemTimeToFileTime( &tSystemtime, &tFiletime );
	btv.wintime.HighPart = tFiletime.dwHighDateTime;
	btv.wintime.LowPart = tFiletime.dwLowDateTime;
#else
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	gettimeofday( &tv, 0 );
	btv.sec = (unsigned long)tv.tv_sec;
	btv.usec = (unsigned long)tv.tv_usec;
#endif

	return btv;
}

static inline const uint64 UTIL_ElapsedTime( const struct bnbttv &btvStart, const struct bnbttv &btvEnd )
{
#ifdef WIN32
	return (uint64)( ( btvEnd.wintime.QuadPart - btvStart.wintime.QuadPart ) / 10000 );
#else
	return (uint64)( ( btvEnd.sec - btvStart.sec ) * 1000 + ( btvEnd.usec - btvStart.usec ) / 1000 );
#endif
}

const string UTIL_ElapsedTimeStr( const struct bnbttv &btvStart, const struct bnbttv &btvEnd )
{
	char szGen[8];
	memset( szGen, 0, sizeof( szGen ) / sizeof( char ) );

	snprintf( szGen, sizeof( szGen ) / sizeof( char ), "%0.3f", UTIL_ElapsedTime( btvStart, btvEnd ) / 1000.0 );

	return szGen;
}

const string UTIL_EscapedToString( const string &cstrEscape )
{
	// according to RFC 2396

	string strString = string( );
	unsigned int uiChar = 0;
	char pBuf[4];

	for( unsigned long ulCount = 0; ulCount < cstrEscape.size( ); )
	{
		if( cstrEscape[ulCount] == '%' )
		{
			if( ulCount < cstrEscape.size( ) - 2 )
			{
				memset( pBuf, 0, sizeof( pBuf ) / sizeof( char ) );

				pBuf[0] = cstrEscape[ulCount + 1];
				pBuf[1] = cstrEscape[ulCount + 2];

				uiChar = 0;

				if( ( isalpha( pBuf[0] ) || isdigit( pBuf[0] ) ) && ( isalpha( pBuf[1] ) || isdigit( pBuf[1] ) ) && sscanf( pBuf, "%02X", &uiChar ) )
//				if( sscanf( pBuf, "%02X", &uiChar ) )
				{

					strString += (unsigned char)uiChar;

					ulCount += 3;
					
				}
				else
				{
					strString += cstrEscape[ulCount];

					ulCount++;
				}
					
			}
			else
			{
				UTIL_LogPrint( string( gmapLANG_CFG["escaped_string"] + "\n" ).c_str( ) );

				return strString;
			}
		}
		else if( cstrEscape[ulCount] == '+' )
		{
			strString += ' ';

			ulCount++;
		}
		else
		{
			strString += cstrEscape[ulCount];

			ulCount++;
		}
	}

	return strString;
}

inline BYTE toHex(const BYTE &x)
{
return x > 9 ? x + 55: x + 48;
}

const string UTIL_HashToString( const string &cstrHash )
{
	// convert a hash to a readable string

	string strString = string( );
	char pBuf[4];
	unsigned char ucChar = 0;

	for( unsigned long ulCount = 0; ulCount < cstrHash.size( ); ulCount++ )
	{
		memset( pBuf, 0, sizeof( pBuf ) / sizeof( char ) );

		ucChar = cstrHash[ulCount];

		snprintf( pBuf, sizeof( pBuf ) / sizeof( char ), "%02x", ucChar );

		strString += pBuf;
	}

	return strString;
}

const string UTIL_BytesToString( int64 iBytes )
{
	const unsigned int cuiB( (unsigned int)( iBytes % 1024 ) );
	iBytes /= 1024;
	const unsigned int cuiKB( (unsigned int)( iBytes % 1024 ) );
	iBytes /= 1024;
	const unsigned int cuiMB( (unsigned int)( iBytes % 1024 ) );
	iBytes /= 1024;
	const unsigned int cuiGB( (unsigned int)( iBytes % 1024 ) );
	iBytes /= 1024;
	const unsigned int cuiTB( (unsigned int)( iBytes % 1024 ) );
	iBytes /= 1024;
	const unsigned int cuiPB( (unsigned int)iBytes );

	// B -> KB -> MB -> GB -> TB -> PB -> EB -> ZB -> YB

	string strBytes = string( );

	if( cuiPB > 0 )
	{
		const unsigned int cuiFrac( (unsigned int)( (float)cuiTB / (float)1024 * (float)100 ) );

		strBytes += CAtomInt( cuiPB ).toString( );
		strBytes += ".";

		if( CAtomInt( cuiFrac ).toString( ).size( ) == 1 )
			strBytes += "0";

		strBytes += CAtomInt( cuiFrac ).toString( );
		strBytes += " PB";
	}
	else if( cuiTB > 0 )
	{
		const unsigned int cuiFrac( (unsigned int)( (float)cuiGB / (float)1024 * (float)100 ) );

		strBytes += CAtomInt( cuiTB ).toString( );
		strBytes += ".";

		if( CAtomInt( cuiFrac ).toString( ).size( ) == 1 )
			strBytes += "0";

		strBytes += CAtomInt( cuiFrac ).toString( );
		strBytes += " TB";
	}
	else if( cuiGB > 0 )
	{
		const unsigned int cuiFrac( (unsigned int)( (float)cuiMB / (float)1024 * (float)100 ) );

		strBytes += CAtomInt( cuiGB ).toString( );
		strBytes += ".";

		if( CAtomInt( cuiFrac ).toString( ).size( ) == 1 )
			strBytes += "0";

		strBytes += CAtomInt( cuiFrac ).toString( );
		strBytes += " GB";
	}
	else if( cuiMB > 0 )
	{
		const unsigned int cuiFrac( (unsigned int)( (float)cuiKB / (float)1024 * (float)100 ) );

		strBytes += CAtomInt( cuiMB ).toString( );
		strBytes += ".";

		if( CAtomInt( cuiFrac ).toString( ).size( ) == 1 )
			strBytes += "0";

		strBytes += CAtomInt( cuiFrac ).toString( );
		strBytes += " MB";
	}
	else if( cuiKB > 0 )
	{
		const unsigned int cuiFrac( (unsigned int)( (float)cuiB / (float)1024 * (float)100 ) );

		strBytes += CAtomInt( cuiKB ).toString( );
		strBytes += ".";

		if( CAtomInt( cuiFrac ).toString( ).size( ) == 1 )
			strBytes += "0";

		strBytes += CAtomInt( cuiFrac ).toString( );
		strBytes += " KB";
	}
	else
	{
		strBytes += CAtomInt( cuiB ).toString( );
		strBytes += " B";
	}

	return strBytes;
}

const string UTIL_SecondsToString( unsigned long ulSeconds )
{
	ulSeconds /= 60;
	const unsigned int cuiM( ulSeconds % 60 );
	ulSeconds /= 60;
	const unsigned int cuiH( ulSeconds % 24 );
	ulSeconds /= 24;
	const unsigned int cuiD( ulSeconds );

	string strSeconds = string( );

	strSeconds += CAtomInt( cuiD ).toString( );
	strSeconds += "d ";

	if( CAtomInt( cuiH ).toString( ).size( ) == 1 )
		strSeconds += "0";

	strSeconds += CAtomInt( cuiH ).toString( );
	strSeconds += ":";

	if( CAtomInt( cuiM ).toString( ).size( ) == 1 )
		strSeconds += "0";

	strSeconds += CAtomInt( cuiM ).toString( );

	return strSeconds;
}

const string UTIL_PassedToString( time_t tNow, time_t tAdded, const string &cstrAdded )
{
	string strPassed = string( );
	
	if( !cstrAdded.empty( ) && tAdded == 0 )
	{
		struct tm time_tm;
		int64 year, month, day, hour, minute, second;
		sscanf( cstrAdded.c_str( ), "%d-%d-%d %d:%d:%d",&year,&month,&day,&hour,&minute,&second );
		time_tm.tm_year = year-1900;
		time_tm.tm_mon = month-1;
		time_tm.tm_mday = day;
		time_tm.tm_hour = hour;
		time_tm.tm_min = minute;
		time_tm.tm_sec = second;
		
		tAdded = mktime(&time_tm);
	}
	
	time_t tTimePassed = difftime( tNow, tAdded );
	
	if( tTimePassed > 0 )
	{
		int64 day_passed = -1, hour_passed = -1, minute_passed = -1, second_passed = -1;
		second_passed =  tTimePassed % 60;
		tTimePassed /= 60;
		minute_passed =  tTimePassed % 60;
		tTimePassed /= 60;
		hour_passed =  tTimePassed % 24;
		tTimePassed /= 24;
		day_passed =  tTimePassed;
		if( day_passed > 0 )
			strPassed = UTIL_Xsprintf( gmapLANG_CFG["added_day_passed"].c_str( ), CAtomInt( day_passed ).toString( ).c_str( ), CAtomInt( hour_passed ).toString( ).c_str( ) );
		else if( hour_passed > 0 )
			strPassed = UTIL_Xsprintf( gmapLANG_CFG["added_hour_passed"].c_str( ), CAtomInt( hour_passed ).toString( ).c_str( ), CAtomInt( minute_passed ).toString( ).c_str( ) );
		else if( minute_passed > 0 )
			strPassed = UTIL_Xsprintf( gmapLANG_CFG["added_minute_passed"].c_str( ), CAtomInt( minute_passed ).toString( ).c_str( ), CAtomInt( second_passed ).toString( ).c_str( ) );
		else if( second_passed >= 0 )
			strPassed = UTIL_Xsprintf( gmapLANG_CFG["added_second_passed"].c_str( ), CAtomInt( second_passed ).toString( ).c_str( ) );
	}
	
	return strPassed;
}

const string UTIL_StringToEscaped( const string &cstrString )
{
	// according to RFC 2396

	string strEscape = string( );
	unsigned char ucChar = 0;
	char pBuf[4];

	for( unsigned long ulCount = 0; ulCount < cstrString.size( ); ulCount++ )
	{
        ucChar = cstrString[ulCount];

		if( isalpha( ucChar ) || isdigit( ucChar ) ||
			ucChar == '-' ||
			ucChar == '_' ||
			ucChar == '.' ||
			ucChar == '!' ||
			ucChar == '~' ||
			ucChar == '*' ||
			ucChar == '\'' ||
			ucChar == '(' ||
			ucChar == ')' )
		{
			// found an unreserved character

			strEscape += ucChar;
		}
		else if( ucChar == ' ' )
			strEscape += '+';
		else
		{
			// found a reserved character
			memset( pBuf, 0, sizeof( pBuf ) / sizeof( char ) );

			snprintf( pBuf, sizeof( pBuf ) / sizeof( char ), "%02X", ucChar );

			strEscape += "%";
			strEscape += pBuf;
		}
	}

	return strEscape;
}

const string UTIL_StringToEscapedStrict( const string &cstrString )
{
	string strEscape = string( );
	unsigned char ucChar = 0;
	char pBuf[4];

	for( unsigned long ulCount = 0; ulCount < cstrString.size( ); ulCount++ )
	{
		ucChar = cstrString[ulCount];

		if( isalpha( ucChar ) || isdigit( ucChar ) ||
			ucChar == '-' ||
			ucChar == '_' ||
			ucChar == '.' ||
			ucChar == '!' ||
			ucChar == '~' ||
			ucChar == '*' ||
			ucChar == '\'' ||
			ucChar == '(' ||
			ucChar == ')' )
		{
			// found an unreserved character

			strEscape += ucChar;
		}
		else
		{
			// found a reserved character
			memset( pBuf, 0, sizeof( pBuf ) / sizeof( char ) );

			snprintf( pBuf, sizeof( pBuf ) / sizeof( char ), "%02X", ucChar );

			strEscape += "%";
			strEscape += pBuf;
		}
	}

	return strEscape;
}

const string UTIL_StringToHash( const string &cstrString )
{
	// convert a readable string hash to a 20 character hash

	string strHash = string( );
	unsigned int uiChar = 0;
	char pBuf[4];

	if( cstrString.size( ) != 40 )
		return string( );

	for( unsigned long ulCount = 0; ulCount < cstrString.size( ); ulCount += 2 )
	{
		memset( pBuf, 0, sizeof( pBuf ) / sizeof( char ) );

		pBuf[0] = cstrString[ulCount];
		pBuf[1] = cstrString[ulCount + 1];

		sscanf( pBuf, "%02x", &uiChar );

		strHash += (unsigned char)uiChar;
	}

	return strHash;
}

const string UTIL_AccessToText( const unsigned char cucAccess )
{
	if( cucAccess & ACCESS_LEADER )
		return gmapLANG_CFG["users_leader_access"];
	else if( cucAccess & ACCESS_ADMIN )
		return gmapLANG_CFG["users_admin_access"];
	else if( cucAccess & ACCESS_EDIT )
		return gmapLANG_CFG["users_edit_access"];
	else if( cucAccess & ACCESS_UPLOAD )
		return gmapLANG_CFG["users_upload_access"];
	else if( cucAccess & ACCESS_COMMENTS )
		return gmapLANG_CFG["users_comments_access"];
	else if( cucAccess & ACCESS_DL )
		return gmapLANG_CFG["users_dl_access"];
	else if( cucAccess & ACCESS_VIEW )
		return gmapLANG_CFG["users_view_access"];
	else
		return gmapLANG_CFG["access_none"];
}

const string UTIL_AccessToString( const unsigned char cucAccess )
{
	if( cucAccess & ACCESS_LEADER )
		return gmapLANG_CFG["access_leader"];
	else if( cucAccess & ACCESS_ADMIN )
		return gmapLANG_CFG["access_admin"];
	else if( cucAccess & ACCESS_EDIT )
		return gmapLANG_CFG["access_moderator"];
	else if( cucAccess & ACCESS_UPLOAD )
		return gmapLANG_CFG["access_uploader"];
	else if( cucAccess & ACCESS_COMMENTS )
		return gmapLANG_CFG["access_member"];
	else if( cucAccess & ACCESS_DL )
		return gmapLANG_CFG["access_downloader"];
	else if( cucAccess & ACCESS_VIEW )
		return gmapLANG_CFG["access_basic"];
	else
		return gmapLANG_CFG["access_none"];
}

const string UTIL_GroupToString( const unsigned char cucGroup )
{
	if( cucGroup & GROUP_FRIENDS )
		return gmapLANG_CFG["group_friends"];
	else if( cucGroup & GROUP_RETIRED )
		return gmapLANG_CFG["group_retired"];
	else if( cucGroup & GROUP_VIP )
		return gmapLANG_CFG["group_vip"];
	else
		return gmapLANG_CFG["group_none"];
}

const string UTIL_UserClass( const unsigned char cucAccess, const unsigned char cucGroup )
{
	if( cucGroup & GROUP_FRIENDS )
		return gmapLANG_CFG["class_friends"];
	else if( cucGroup & GROUP_RETIRED )
		return gmapLANG_CFG["class_retired"];
	else if( cucGroup & GROUP_VIP )
		return gmapLANG_CFG["class_vip"];
	else
	{
		if( cucAccess & ACCESS_LEADER )
			return gmapLANG_CFG["class_leader"];
		else if( cucAccess & ACCESS_ADMIN )
			return gmapLANG_CFG["class_admin"];
		else if( cucAccess & ACCESS_EDIT )
			return gmapLANG_CFG["class_moderator"];
		else if( cucAccess & ACCESS_UPLOAD )
			return gmapLANG_CFG["class_uploader"];
//		else if( cucAccess & ACCESS_COMMENTS )
		else if( !( cucAccess & ACCESS_VIEW ) )
			return gmapLANG_CFG["class_banned"];
		else
			return gmapLANG_CFG["class_member"];
//		else if( cucAccess & ACCESS_DL )
//			return gmapLANG_CFG["class_downloader"];
//		else if( cucAccess & ACCESS_VIEW )
//			return gmapLANG_CFG["class_basic"];
//		else
//			return gmapLANG_CFG["class_none"];
	}
}

const int64 UTIL_StringTo64( const char *sz64 )
{
	int64 iI64;

#if defined( WIN32 )
	sscanf( sz64, "%I64d", &iI64 );
#elif defined( __FREEBSD__ ) || defined( __OPENBSD__ ) || defined( __NETBSD__ )
	sscanf( sz64, "%qd", &iI64 );
#else
	sscanf( sz64, "%lld", &iI64 );
#endif

	return iI64;
}

const string UTIL_InfoHash( CAtom *pTorrent )
{
	if( pTorrent && pTorrent->isDicti( ) )
	{
		CAtom *pInfo = ( (CAtomDicti *)pTorrent )->getItem( "info" );

		if( pInfo && pInfo->isDicti( ) )
		{
			// encode the string

			const string cstrData( Encode( pInfo ) );

			// hash it

			CSHA1 hasher;

			hasher.Update( (const unsigned char *)cstrData.c_str( ), (unsigned int)cstrData.size( ) );
			hasher.Final( );

			char szInfoHash[64];
			memset( szInfoHash, 0, sizeof( szInfoHash ) / sizeof( char ) );

			hasher.ReportHash( szInfoHash );

			return UTIL_StringToHash( szInfoHash );
		}
	}

	return string( );
}

const bool UTIL_CheckFile( const char *szFile )
{
	// check if file exists

	FILE *pFile = FILE_ERROR;

	pFile = fopen( szFile, "rb");
	
	if( pFile == FILE_ERROR )
		return false;

	fclose( pFile );

	return true;
}

void UTIL_MakeFile( const char *szFile, const string &cstrContents )
{
	FILE *pFile = FILE_ERROR;

	pFile = fopen( szFile, "wb" );
	
	if( pFile == FILE_ERROR )
	{
		UTIL_LogFilePrint( string( gmapLANG_CFG["makefile"] + "\n" ).c_str( ), szFile );

		return;
	}

	fwrite( cstrContents.c_str( ), sizeof( char ), cstrContents.size( ), pFile );
	fclose( pFile );
}

void UTIL_DeleteFile( const char *szFile )
{
	if( unlink( szFile ) == 0 )
		UTIL_LogFilePrint( string( gmapLANG_CFG["deletefile"] + "\n" ).c_str( ), szFile );
	else
	{
#ifdef WIN32
		UTIL_LogFilePrint( string( gmapLANG_CFG["deletefile_error"] + "\n" ).c_str( ), szFile );
#else
		UTIL_LogFilePrint( string( gmapLANG_CFG["deletefile_error_stream"] + "\n" ).c_str( ), szFile, strerror( errno ) );
#endif
	}
}

void UTIL_MoveFile( const char *szFile, const char *szDest )
{
	if( UTIL_CheckFile( szDest ) )
		UTIL_LogFilePrint( string( gmapLANG_CFG["movefile"] + "\n" ).c_str( ), szDest );
	else
		UTIL_MakeFile( szDest, UTIL_ReadFile( szFile ) );

	// thanks MrMister

	UTIL_DeleteFile( szFile );
}

const string UTIL_ReadFile( const char *szFile )
{
	FILE *pFile = FILE_ERROR;

	pFile = fopen( szFile, "rb" );
	
	if( pFile == FILE_ERROR )
	{
		UTIL_LogFilePrint( string( gmapLANG_CFG["readfile"] + "\n" ).c_str( ), szFile );

		return string( );
	}

	// Find the end of the file
	fseek( pFile, 0, SEEK_END );

	// Remember the file size for later
	const unsigned long culFileSize( ftell( pFile ) );

	// Reset to the start of the file
	fseek( pFile, 0, SEEK_SET );

	// Allocate memory for the data buffer
	char *pData = (char *)malloc( sizeof( char ) * culFileSize );

	// Read the data
	fread( pData, sizeof( char ), culFileSize, pFile );

	// Close the file
	fclose( pFile );

	// Place the data in a string
	const string cstrFile( pData, culFileSize );

	// Free the data buffer memory
	free( pData );

	// Return the data
	return cstrFile;
}

const unsigned long UTIL_SizeFile( const char *szFile )
{
	FILE *pFile = FILE_ERROR;

	pFile = fopen( szFile, "rb" );
	
	if( pFile == FILE_ERROR )
	{
		UTIL_LogFilePrint( string( gmapLANG_CFG["sizefile"] + "\n" ).c_str( ), szFile );

		return 0;
	}

	fseek( pFile, 0, SEEK_END );
	const unsigned long culFileSize( (unsigned long) ftell( pFile ) );
	fclose( pFile );

	return culFileSize;
}

const string UTIL_ToLower( const string &cstrUpper )
{
	string strUpper = cstrUpper;

	for( unsigned long ulCount = 0; ulCount < strUpper.size( ); ulCount++ )
		strUpper[ulCount] = (unsigned char)tolower( strUpper[ulCount] );

	return strUpper;
}

const string UTIL_StripPath( const string &cstrPath )
{
	string :: size_type iFileStart = cstrPath.rfind( CHAR_BS );

	if( iFileStart == string :: npos )
	{
		iFileStart = cstrPath.rfind( CHAR_FS );

		if( iFileStart == string :: npos )
			iFileStart = 0;
		else
			iFileStart++;
	}
	else
		iFileStart++;

	return cstrPath.substr( iFileStart );
}

const string UTIL_RemoveHTML( const string &cstrHTML )
{
	string strHTML = cstrHTML;

	for( unsigned long ulCount = 0; ulCount < strHTML.size( ); ulCount++ )
	{
		if( strHTML[ulCount] == '<' )
			strHTML.replace( ulCount, 1, "&lt;" );
		else if( strHTML[ulCount] == '>' )
			strHTML.replace( ulCount, 1, "&gt;" );
		else if( strHTML[ulCount] == '&' )
			strHTML.replace( ulCount, 1, "&amp;" );
		else if( strHTML[ulCount] == '"' )
			strHTML.replace( ulCount, 1, "&quot;" );
		else if( strHTML[ulCount] == ' ' && ulCount > 0 && strHTML[ulCount - 1] == ' ' )
			strHTML.replace( ulCount, 1, "&nbsp;" );
		else if( strHTML[ulCount] == '\n' )
		{
			if( ulCount > 0 )
			{
				if( strHTML[ulCount - 1] == '\r' )
				{
					strHTML.replace( ulCount - 1, 2, "<br>" );

					ulCount += 2;
				}
				else
				{
					strHTML.replace( ulCount, 1, "<br>" );

					ulCount += 3;
				}
			}
			else
			{
				strHTML.replace( ulCount, 1, "<br>" );

				ulCount += 3;
			}
		}
	}
	
	return strHTML;

// 	return UTIL_BBCode( strHTML );
}

const string UTIL_RemoveHTML2( const string &cstrHTML )
{
	string strHTML = cstrHTML;

	for( unsigned long ulCount = 0; ulCount < strHTML.size( ); ulCount++ )
	{
		if( strHTML[ulCount] == '<' )
			strHTML.replace( ulCount, 1, "&lt;" );
		else if( strHTML[ulCount] == '>' )
			strHTML.replace( ulCount, 1, "&gt;" );
		else if( strHTML[ulCount] == '&' )
			strHTML.replace( ulCount, 1, "&amp;" );
		else if( strHTML[ulCount] == '"' )
			strHTML.replace( ulCount, 1, "&quot;" );
		else if( strHTML[ulCount] == ' ' && ulCount > 0 && strHTML[ulCount - 1] == ' ' )
			strHTML.replace( ulCount, 1, "&nbsp;" );
		else if( strHTML[ulCount] == '\n' )
		{
			if( ulCount > 0 )
			{
				if( strHTML[ulCount - 1] == '\r' )
				{
					strHTML.replace( ulCount - 1, 2, "<br>" );

					ulCount += 2;
				}
				else
				{
					strHTML.replace( ulCount, 1, "<br>" );

					ulCount += 3;
				}
			}
			else
			{
				strHTML.replace( ulCount, 1, "<br>" );

				ulCount += 3;
			}
		}
	}

	return UTIL_BBCode( strHTML );
}

const string UTIL_RemoveHTML3( const string &cstrHTML )
{
	string strHTML = cstrHTML;

	for( unsigned long ulCount = 0; ulCount < strHTML.size( ); ulCount++ )
	{
		if( strHTML[ulCount] == '<' )
			strHTML.replace( ulCount, 1, "&lt;" );
		else if( strHTML[ulCount] == '>' )
			strHTML.replace( ulCount, 1, "&gt;" );
		else if( strHTML[ulCount] == '&' )
			strHTML.replace( ulCount, 1, "&amp;" );
		else if( strHTML[ulCount] == '"' )
			strHTML.replace( ulCount, 1, "&quot;" );
		else if( strHTML[ulCount] == ' ' && ulCount > 0 && strHTML[ulCount - 1] == ' ' )
			strHTML.replace( ulCount, 1, "&nbsp;" );
	}
	
	return strHTML;
}

const string UTIL_BBCode( const string &cstrHTML )
{
	string strHTML = cstrHTML;
	string strURL;
	string :: size_type iStart = 0;
	string :: size_type iURL = 0;
	string :: size_type iEnd = 0;
	
	iStart = UTIL_ToLower( strHTML ).find( "[url=" );
	while( iStart != string :: npos )
	{
		iURL = UTIL_ToLower( strHTML ).find( "]", iStart );
		iEnd = UTIL_ToLower( strHTML ).find( "[/url]", iStart );
		if( iURL < iEnd && strHTML[ iStart - 1 ] != '[' && strHTML[ iEnd + 6 ] != ']' && iEnd != string :: npos )
		{
			strHTML.replace( iEnd, 6, "</a>" );
			strHTML.replace( iURL, 1, "\">" );
			strHTML.replace( iStart, 5, "<a target=\"_blank\" href=\"" );
			iStart = UTIL_ToLower( strHTML ).find( "[url=", iStart );
		}
		else
			iStart = UTIL_ToLower( strHTML ).find( "[url=", iStart + 5 );
	}
	iStart = UTIL_ToLower( strHTML ).find( "[img=" );
	while( iStart != string :: npos )
	{
		iEnd = UTIL_ToLower( strHTML ).find( "]", iStart );
		if( strHTML[ iStart - 1 ] != '[' && strHTML[ iEnd + 1 ] != ']' && iEnd != string :: npos )
		{
			strHTML.replace( iEnd, 1, "\">" );
			strHTML.replace( iStart, 5, "<img class=\"code\" alt=\"image\" src=\"" );
			iStart = UTIL_ToLower( strHTML ).find( "[img=", iStart );
		}
		else
			iStart = UTIL_ToLower( strHTML ).find( "[img=", iStart + 5 );
	}
	iStart = UTIL_ToLower( strHTML ).find( "[color=" );
	while( iStart != string :: npos )
	{
		iEnd = UTIL_ToLower( strHTML ).find( "[/color]", iStart );
		if( iEnd != string :: npos )
			iStart = UTIL_ToLower( strHTML ).substr( 0, iEnd ).rfind( "[color=" );
		iURL = UTIL_ToLower( strHTML ).find( "]", iStart );
		if( iURL < iEnd && strHTML[ iStart - 1 ] != '[' && strHTML[ iEnd + 8 ] != ']' && iEnd != string :: npos )
		{
			strHTML.replace( iEnd, 8, "</font>" );
			strHTML.replace( iURL, 1, "\">" );
			strHTML.replace( iStart, 7, "<font color=\"" );
			iStart = UTIL_ToLower( strHTML ).find( "[color=" );
		}
		else
			iStart = UTIL_ToLower( strHTML ).find( "[color=", iStart + 7 );
	}
	iStart = UTIL_ToLower( strHTML ).find( "[font=" );
	while( iStart != string :: npos )
	{
		iEnd = UTIL_ToLower( strHTML ).find( "[/font]", iStart );
		if( iEnd != string :: npos )
			iStart = UTIL_ToLower( strHTML ).substr( 0, iEnd ).rfind( "[font=" );
		iURL = UTIL_ToLower( strHTML ).find( "]", iStart );
		if( iURL < iEnd && strHTML[ iStart - 1 ] != '[' && strHTML[ iEnd + 7 ] != ']' && iEnd != string :: npos )
		{
			strHTML.replace( iEnd, 7, "</font>" );
			strHTML.replace( iURL, 1, "\">" );
			strHTML.replace( iStart, 6, "<font face=\"" );
			iStart = UTIL_ToLower( strHTML ).find( "[font=" );
		}
		else
			iStart = UTIL_ToLower( strHTML ).find( "[font=", iStart + 6 );
	}
	iStart = UTIL_ToLower( strHTML ).find( "[size=" );
	while( iStart != string :: npos )
	{
		iEnd = UTIL_ToLower( strHTML ).find( "[/size]", iStart );
		if( iEnd != string :: npos )
			iStart = UTIL_ToLower( strHTML ).substr( 0, iEnd ).rfind( "[size=" );
		iURL = UTIL_ToLower( strHTML ).find( "]", iStart );
		if( iURL < iEnd && strHTML[ iStart - 1 ] != '[' && strHTML[ iEnd + 7 ] != ']' && iEnd != string :: npos )
		{
			strHTML.replace( iEnd, 7, "</font>" );
			strHTML.replace( iURL, 1, "\">" );
			strHTML.replace( iStart, 6, "<font size=\"" );
			iStart = UTIL_ToLower( strHTML ).find( "[size=" );
		}
		else
			iStart = UTIL_ToLower( strHTML ).find( "[size=", iStart + 6 );
	}
	iStart = UTIL_ToLower( strHTML ).find( "[quote=" );
	while( iStart != string :: npos )
	{
		iEnd = UTIL_ToLower( strHTML ).find( "[/quote]", iStart );
		if( iEnd != string :: npos )
			iStart = UTIL_ToLower( strHTML ).substr( 0, iEnd ).rfind( "[quote=" );
		iURL = UTIL_ToLower( strHTML ).find( "]", iStart );
		if( iURL < iEnd && strHTML[ iStart - 1 ] != '[' && strHTML[ iEnd + 8 ] != ']' && iEnd != string :: npos )
		{
			strHTML.replace( iEnd, 8, "</fieldset>" );
			strHTML.replace( iURL, 1, "</b></legend>" );
			strHTML.replace( iStart, 7, "<fieldset class=\"quote\"><legend><b>" + gmapLANG_CFG["quote"] );
			iStart = UTIL_ToLower( strHTML ).find( "[quote=" );
		}
		else
			iStart = UTIL_ToLower( strHTML ).find( "[quote=", iStart + 7 );
	}
	iStart = UTIL_ToLower( strHTML ).find( "[url]" );
	while( iStart != string :: npos )
	{
		iEnd = UTIL_ToLower( strHTML ).find( "[/url]", iStart );
		if( iEnd != string :: npos && strHTML[ iStart - 1 ] != '[' && strHTML[ iEnd + 6 ] != ']' )
		{
			strURL = string( strHTML.substr( iStart + 5, iEnd - iStart - 5 ) );
			strHTML.replace( iEnd, 6, "</a>" );
			strHTML.replace( iStart, 5, "<a href=\"\" target=\"_blank\">" );
			strHTML.insert( iStart + 9, strURL );
			iStart = UTIL_ToLower( strHTML ).find( "[url]", iStart );
		}
		else
			iStart = UTIL_ToLower( strHTML ).find( "[url]", iStart + 5 );
	}
	iStart = UTIL_ToLower( strHTML ).find( "[img]" );
	while( iStart != string :: npos )
	{
		iEnd = UTIL_ToLower( strHTML ).find( "[/img]", iStart );
		if( iEnd != string :: npos && strHTML[ iStart - 1 ] != '[' && strHTML[ iEnd + 6 ] != ']' )
		{
			strHTML.replace( iEnd, 6, "\">" );
			strHTML.replace( iStart, 5, "<img class=\"code\" alt=\"image\" src=\"" );
			iStart = UTIL_ToLower( strHTML ).find( "[img]", iStart );
		}
		else
			iStart = UTIL_ToLower( strHTML ).find( "[img]", iStart + 5 );
	}
	iStart = UTIL_ToLower( strHTML ).find( "[quote]" );
	while( iStart != string :: npos )
	{
		iEnd = UTIL_ToLower( strHTML ).find( "[/quote]", iStart );
		if( iEnd != string :: npos )
			iStart = UTIL_ToLower( strHTML ).substr( 0, iEnd ).rfind( "[quote]" );
		if( iEnd != string :: npos && strHTML[ iStart - 1 ] != '[' && strHTML[ iEnd + 8 ] != ']' )
		{
			strHTML.replace( iEnd, 8, "</td></tr></table>" );
			strHTML.replace( iStart, 7, "<b>" + gmapLANG_CFG["quote"] + "</b><table class=\"quote\"><tr class=\"quote\"><td class=\"quote\">" );
			iStart = UTIL_ToLower( strHTML ).find( "[quote]" );
		}
		else
			iStart = UTIL_ToLower( strHTML ).find( "[quote]", iStart + 7 );
	}
	iStart = UTIL_ToLower( strHTML ).find( "[pre]" );
	while( iStart != string :: npos )
	{
		iEnd = UTIL_ToLower( strHTML ).find( "[/pre]", iStart );
		if( iEnd != string :: npos )
			iStart = UTIL_ToLower( strHTML ).substr( 0, iEnd ).rfind( "[pre]" );
		if( iEnd != string :: npos && strHTML[ iStart - 1 ] != '[' && strHTML[ iEnd + 6 ] != ']' )
		{
			strHTML.replace( iEnd, 6, "</pre>" );
			strHTML.replace( iStart, 5, "<pre>" );
			iStart = UTIL_ToLower( strHTML ).find( "[pre]" );
		}
		else
			iStart = UTIL_ToLower( strHTML ).find( "[pre]", iStart + 5 );
	}
	iStart = UTIL_ToLower( strHTML ).find( "[b]" );
	while( iStart != string :: npos )
	{
		iEnd = UTIL_ToLower( strHTML ).find( "[/b]", iStart );
		if( iEnd != string :: npos && strHTML[ iStart - 1 ] != '[' && strHTML[ iEnd + 4 ] != ']' )
		{
			strHTML.replace( iEnd, 4, "</b>" );
			strHTML.replace( iStart, 3, "<b>" );
			iStart = UTIL_ToLower( strHTML ).find( "[b]", iStart );
		}
		else
			iStart = UTIL_ToLower( strHTML ).find( "[b]", iStart + 3 );
	}
	iStart = UTIL_ToLower( strHTML ).find( "[i]" );
	while( iStart != string :: npos )
	{
		iEnd = UTIL_ToLower( strHTML ).find( "[/i]", iStart );
		if( iEnd != string :: npos && strHTML[ iStart - 1 ] != '[' && strHTML[ iEnd + 4 ] != ']' )
		{
			strHTML.replace( iEnd, 4, "</i>" );
			strHTML.replace( iStart, 3, "<i>" );
			iStart = UTIL_ToLower( strHTML ).find( "[i]", iStart );
		}
		else
			iStart = UTIL_ToLower( strHTML ).find( "[i]", iStart + 3 );
	}
	iStart = UTIL_ToLower( strHTML ).find( "[u]" );
	while( iStart != string :: npos )
	{
		iEnd = UTIL_ToLower( strHTML ).find( "[/u]", iStart );
		if( iEnd != string :: npos && strHTML[ iStart - 1 ] != '[' && strHTML[ iEnd + 4 ] != ']' )
		{
			strHTML.replace( iEnd, 4, "</u>" );
			strHTML.replace( iStart, 3, "<u>" );
			iStart = UTIL_ToLower( strHTML ).find( "[u]", iStart );
		}
		else
			iStart = UTIL_ToLower( strHTML ).find( "[u]", iStart + 3 );
	}
	iStart = UTIL_ToLower( strHTML ).find( "[return]" );
	while( iStart != string :: npos )
	{
		if( strHTML[ iStart - 1 ] != '[' && strHTML[ iStart + 8 ] != ']' )
		{
			strHTML.replace( iStart, 8, "<br>" );
			iStart = UTIL_ToLower( strHTML ).find( "[return]", iStart );
		}
		else
			iStart = UTIL_ToLower( strHTML ).find( "[return]", iStart + 8 );
	}
	iStart = UTIL_ToLower( strHTML ).find( "[[" );
	while( iStart != string :: npos )
	{
		iEnd = UTIL_ToLower( strHTML ).find( "]]", iStart );
		if( iEnd != string :: npos )
			iStart = UTIL_ToLower( strHTML ).substr( 0, iEnd ).rfind( "[[" );
		if( iEnd != string :: npos )
		{
			strHTML.replace( iEnd, 2, "]" );
			strHTML.replace( iStart, 2, "[" );
			iStart = UTIL_ToLower( strHTML ).find( "[[" );
		}
		else
			iStart = UTIL_ToLower( strHTML ).find( "[[", iStart + 2 );
	}
	return strHTML;
}

void UTIL_StripName( const string &cstrCompName, string &strReturnName1, string &strReturnName2 )
{
	string strCompName = "]" + cstrCompName + "[";
	unsigned int uiCount = 0;
	unsigned int uiStart = 0;
	int iZero = 0, iAdd = 0;
	bool bNoReturnName1 = false;
	if( strCompName.find( "[]" ) != string :: npos )
		bNoReturnName1 = true;
	for( uiCount = 0; uiCount < strCompName.size( ); uiCount++ )
	{
		if( strCompName[uiCount] == '[' )
		{
			iZero++;
			iAdd = 1;
		}
		else if( strCompName[uiCount] == ']' )
		{
			iZero--;
			iAdd = -1;
		}
		if( iZero == 0 )
		{
			iZero += iAdd;
			if( uiCount - uiStart - 1 > 0 )
			{
				if( isascii( strCompName[ uiStart + 1 ] ) && isascii( strCompName[ uiCount - 1 ] ) && ( uiCount - uiStart - 1 >= 5 ) && !bNoReturnName1 )
					if( !( isdigit( strCompName[ uiStart + 1 ] ) && isdigit( strCompName[ uiCount - 1 ] ) ) )
					{
						if( strReturnName1.empty( ) )
							strReturnName1 += strCompName.substr( uiStart + 1, uiCount - uiStart - 1 );
						else
							strReturnName2 += " [" + strCompName.substr( uiStart + 1, uiCount - uiStart - 1 ) + "]";
					}
					else
					{
						if( !strReturnName2.empty( ) )
							strReturnName2 += " ";
						if( !strReturnName1.empty( ) )
							strReturnName2 += "[";
						strReturnName2 += strCompName.substr( uiStart + 1, uiCount - uiStart - 1 );
						if( !strReturnName1.empty( ) )
							strReturnName2 += "]";
						
					}
				else
				{
					if( !strReturnName2.empty( ) )
						strReturnName2 += " ";
					if( !strReturnName1.empty( ) )
						strReturnName2 += "[";
					strReturnName2 += strCompName.substr( uiStart + 1, uiCount - uiStart - 1 );
					if( !strReturnName1.empty( ) )
						strReturnName2 += "]";
				}
			}
			uiStart = uiCount;
		}
	}
	uiCount--;
	if( iZero != 1 && uiCount - uiStart - 1 > 0 )
	{
		if( isascii( strCompName[ uiStart + 1 ] ) && isascii( strCompName[ uiCount - 1 ] ) && ( uiCount - uiStart - 1 >= 5 ) && !bNoReturnName1 )
			if( !( isdigit( strCompName[ uiStart + 1 ] ) && isdigit( strCompName[ uiCount - 1 ] ) ) )
			{
				if( strReturnName1.empty( ) )
					strReturnName1 += strCompName.substr( uiStart + 1, uiCount - uiStart - 1 );
				else
					strReturnName2 += " [" + strCompName.substr( uiStart + 1, uiCount - uiStart - 1 ) + "]";
			}
			else
			{
				if( !strReturnName2.empty( ) )
					strReturnName2 += " ";
				if( !strReturnName1.empty( ) )
					strReturnName2 += "[";
				strReturnName2 += strCompName.substr( uiStart + 1, uiCount - uiStart - 1 );
				if( !strReturnName1.empty( ) )
					strReturnName2 += "]";
				
			}
		else
		{
			if( !strReturnName2.empty( ) )
				strReturnName2 += " ";
			if( !strReturnName1.empty( ) )
				strReturnName2 += "[";
			strReturnName2 += strCompName.substr( uiStart + 1, uiCount - uiStart - 1 );
			if( !strReturnName1.empty( ) )
				strReturnName2 += "]";
		}
	}
//	bool bNoReturnName1 = false;
//	string strCompName = cstrCompName;
//	string :: size_type iStart;
//	string :: size_type iEnd;
//	strReturnName2 += strCompName.substr( 0, iStart );
//	
//	if( strCompName.find( "[]" ) != string :: npos )
//		bNoReturnName1 = true;

//	iStart = strCompName.find( '[' );
//	iEnd = strCompName.find( ']', iStart );
//		
//	while( iStart != string :: npos && iEnd != string :: npos )
//	{
//		if( isalnum( strCompName[ iStart + 1 ] ) && isalnum( strCompName[ iEnd - 1 ] ) && ( iEnd - iStart - 1 >= 5 ) && !bNoReturnName1 )
//			if( !( ( strCompName[ iStart + 1 ] >= '0' && strCompName[ iStart + 1 ] <= '9' ) && ( strCompName[ iEnd - 1 ] >= '0' && strCompName[ iEnd - 1 ] <= '9' ) ) )
//			{
//				if( strReturnName1.empty( ) )
//					strReturnName1 += strCompName.substr( iStart + 1, iEnd - iStart - 1 );
//				else
//					strReturnName2 += " " + strCompName.substr( iStart, iEnd - iStart + 1 );
//			}
//			else
//			{
//				if( !strReturnName1.empty( ) )
//					if( !strReturnName2.empty( ) )
//						strReturnName2 += " " + strCompName.substr( iStart, iEnd - iStart + 1 );
//					else
//						strReturnName2 += strCompName.substr( iStart, iEnd - iStart + 1 );
//				else
//					if( !strReturnName2.empty( ) )
//						strReturnName2 += " " + strCompName.substr( iStart + 1, iEnd - iStart - 1 );
//					else
//						strReturnName2 += strCompName.substr( iStart + 1, iEnd - iStart - 1 );
//			}
//		else
//		{
//			if( !strReturnName1.empty( ) )
//				if( !strReturnName2.empty( ) )
//					strReturnName2 += " " + strCompName.substr( iStart, iEnd - iStart + 1 );
//				else
//					strReturnName2 += strCompName.substr( iStart, iEnd - iStart + 1 );
//			else
//				if( !strReturnName2.empty( ) )
//					strReturnName2 += " " + strCompName.substr( iStart + 1, iEnd - iStart - 1 );
//				else
//					strReturnName2 += strCompName.substr( iStart + 1, iEnd - iStart - 1 );
//		}
//		iStart = strCompName.find( "[", iEnd + 1 );
//		if( iStart != string :: npos )
//		{
//			if( strCompName.find( "]", iStart ) == string :: npos )
//				iStart = string :: npos;
//		}
//		if( iStart - iEnd > 1 && !strCompName.substr( iEnd + 1, iStart - iEnd -1 ).empty( ) )
//		{
//			if( strReturnName2.empty( ) )
//				strReturnName2 += strCompName.substr( iEnd + 1, iStart - iEnd -1 );
//			else
//				strReturnName2 += " " + strCompName.substr( iEnd + 1, iStart - iEnd -1 );
//		}
//		iEnd = strCompName.find( "]", iStart );
//	}
//// 	if( iStart != string :: npos )
//// 		strReturnName2 += strCompName.substr( iStart, iEnd - iStart );
	if( strReturnName1.empty( ) )
	{
		if( strReturnName2.empty( ) )
			strReturnName1 = cstrCompName;
		else
		{
			strReturnName1 = strReturnName2;
			strReturnName2.erase();
		}
	}
}

const string UTIL_HTMLJoin( vector< pair< string, string > > &vecParams, const string &cstrStart, const string &cstrJoin, const string &cstrEqual )
{
	string strJoined = string( );
	
	for( vector< pair< string, string > > :: iterator ulKey = vecParams.begin( ); ulKey != vecParams.end( ); ulKey++ )
	{
		if( !(*ulKey).second.empty( ) )
		{
			if( strJoined.empty( ) )
				strJoined += cstrStart;
			else
				strJoined += cstrJoin;
			strJoined += (*ulKey).first + cstrEqual + UTIL_StringToEscaped( (*ulKey).second );
		}
	}
	return strJoined;
}

const vector<string> UTIL_SplitToVector( const string &cstrJoined, const string &cstrSplit )
{
	vector<string> vecSplit;
	vecSplit.reserve(64);
	vecSplit.clear( );
	
	string :: size_type iSplitSize = cstrSplit.size( );
	
	if( !cstrJoined.empty( ) )
	{
		if( !cstrSplit.empty( ) )
		{
			string :: size_type iStart = 0;
			string :: size_type iEnd = 0;
	//		iEnd = cstrJoined.find( cstrSplit );
			while( iStart != string :: npos && iEnd != string :: npos )
			{
				iEnd = cstrJoined.find( cstrSplit, iStart );
				if( iEnd > iStart )
					vecSplit.push_back( cstrJoined.substr( iStart, iEnd - iStart ) );

				if( iEnd != string :: npos )
					iStart = iEnd + iSplitSize;
			}
		}
		else
			vecSplit.push_back( cstrJoined );
	}
	
	return vecSplit;
}

const vector<string> UTIL_SplitToVectorStrict( const string &cstrJoined, const string &cstrSplit )

{
	vector<string> vecSplit;
	vecSplit.reserve(64);
	vecSplit.clear( );
	
	string :: size_type iSplitSize = cstrSplit.size( );
	
	if( !cstrJoined.empty( ) )
	{
		string :: size_type iStart = 0;
		string :: size_type iEnd = 0;
//		iEnd = cstrJoined.find( cstrSplit );
		while( iStart != string :: npos && iEnd != string :: npos )
		{
			iEnd = cstrJoined.find( cstrSplit, iStart );
//			if( iEnd > iStart )
			vecSplit.push_back( cstrJoined.substr( iStart, iEnd - iStart ) );

			if( iEnd != string :: npos )
				iStart = iEnd + iSplitSize;
		}
	}
	
	return vecSplit;
}

const bool UTIL_MatchVector( const string &cstrText, vector<string> &vecMatch, const unsigned char cucMatchMethod )
{
	if( !vecMatch.empty( ) )  
	{
		switch( cucMatchMethod )
		{
		case MATCH_METHOD_NONCASE_AND:
		{
			const string cstrTextLower( UTIL_ToLower( cstrText ) );
			
			for( vector<string> :: iterator ulVecKey = vecMatch.begin( ); ulVecKey != vecMatch.end( ); ulVecKey++ )
			{
				if( (*ulVecKey).find( "!" ) == 0 )
				{
					if( cstrTextLower.find( UTIL_ToLower( (*ulVecKey).substr( 1 ) ) ) != string :: npos )
						return false;
				}
				else
					if( cstrTextLower.find( UTIL_ToLower( *ulVecKey ) ) == string :: npos )
						return false;
			}
			return true;
		}
		case MATCH_METHOD_NONCASE_OR:
		{
			const string cstrTextLower( UTIL_ToLower( cstrText ) );
			
			for( vector<string> :: iterator ulVecKey = vecMatch.begin( ); ulVecKey != vecMatch.end( ); ulVecKey++ )
			{
				if( (*ulVecKey).find( "!" ) == 0 )
				{
					if( cstrTextLower.find( UTIL_ToLower( (*ulVecKey).substr( 1 ) ) ) == string :: npos )
						return true;
				}
				else
					if( cstrTextLower.find( UTIL_ToLower( *ulVecKey ) ) != string :: npos )
						return true;
			}
			return false;
		}
		case MATCH_METHOD_NONCASE_EQ:
		{
			const string cstrTextLower( UTIL_ToLower( cstrText ) );
			
			for( vector<string> :: iterator ulVecKey = vecMatch.begin( ); ulVecKey != vecMatch.end( ); ulVecKey++ )
			{
				if( cstrTextLower == UTIL_ToLower( *ulVecKey ) )
					return true;
			}
			return false;
		}
		default:
			return true;
		}
	}
	if( cucMatchMethod == MATCH_METHOD_NONCASE_EQ )
		return false;
	return true;
}

const string UTIL_PageBar( unsigned long ulCount, const string &cstrPage, const unsigned int cuiPerPage, const string &cstrRef, const string &cstrParams, const bool bPageBarTop, const bool bShowPageNum )
{
	string strPageNav = string( );
	string strPageNum = string( );
	unsigned long ulStart = 0;
	
	if( !cstrPage.empty( ) )
		ulStart = (unsigned long)atoi( cstrPage.c_str( ) ) * cuiPerPage;
	
	if( ulCount && cuiPerPage > 0 )
	{
		if( ulStart > 0 )
			strPageNav += "<a title=\"" + gmapLANG_CFG["jump_to_page"] + ": " + CAtomInt( ( ulStart / cuiPerPage ) ).toString( ) + "\" href=\"" + cstrRef + "?page=" + CAtomInt( ( ulStart / cuiPerPage ) - 1 ).toString( ) + cstrParams + "\">";
			
		strPageNav += gmapLANG_CFG["last_page"];
		
		if( ulStart > 0 )
			strPageNav += "</a>";
		
		strPageNav += "<span class=\"pipe\"> | </span>";
		
		if( ulStart + cuiPerPage < ulCount )
			strPageNav += "<a title=\"" + gmapLANG_CFG["jump_to_page"] + ": " + CAtomInt( ( ulStart / cuiPerPage ) + 2 ).toString( ) + "\" href=\"" + cstrRef + "?page=" + CAtomInt( ( ulStart / cuiPerPage ) + 1 ).toString( ) + cstrParams + "\">";

		strPageNav += gmapLANG_CFG["next_page"];
		
		if( ulStart + cuiPerPage < ulCount )
			strPageNav += "</a>";
		
		strPageNum += gmapLANG_CFG["jump_to_page"] + ": \n";

		for( unsigned long ulPerPage = 0; ulPerPage < ulCount; ulPerPage += cuiPerPage )
		{
			strPageNum += " ";
			
			if( ( ulPerPage/cuiPerPage <= 2 ) || (  ( ulCount - ulPerPage )/cuiPerPage <= 2 ) || ( abs( ulStart - ulPerPage )/cuiPerPage <=2 ) )
			{
				// don't link to current page
				if( ulPerPage != ulStart )
					strPageNum += "<a title=\"" + gmapLANG_CFG["jump_to_page"] + ": " + CAtomInt( ( ulPerPage / cuiPerPage ) + 1 ).toString( ) + "\" href=\"" + cstrRef + "?page=" + CAtomInt( ulPerPage / cuiPerPage ).toString( ) + cstrParams + "\">";

				if( bShowPageNum )
					strPageNum += CAtomInt( ( ulPerPage / cuiPerPage ) + 1 ).toString( );
				else
				{
					if( ulPerPage + cuiPerPage < ulCount )
						strPageNum += CAtomInt( ulPerPage + 1 ).toString( ) + " - " + CAtomInt( ulPerPage + cuiPerPage ).toString( );
					else
						strPageNum += CAtomInt( ulPerPage + 1 ).toString( ) + " - " + CAtomInt( ulCount ).toString( );
				}
				
				if( ulPerPage != ulStart )
					strPageNum += "</a>\n";

				// don't display a bar after the last page
				if( ulPerPage + cuiPerPage < ulCount )
					strPageNum += "<span class=\"pagenum_pipe\"> | </span>";
			}
			else
			{
				if( ( ulPerPage/cuiPerPage == 3 && ulPerPage < ulStart ) || (  ( ulCount - ulPerPage )/cuiPerPage == 3 && ulPerPage > ulStart ) )
					strPageNum += "...<span class=\"pagenum_pipe\"> | </span>";
			}
		}
		
		if( bPageBarTop )
			return "<p class=\"pagenum_top_bar\">\n" + strPageNav + "<br>\n" + strPageNum + "</p>\n\n";
		else
			return "<p class=\"pagenum_bottom\">\n" + strPageNum + "<br>\n" + strPageNav + "</p>\n\n";
	}
	else
		return string( );
}

const string UTIL_PageBarAJAX( unsigned long ulCount, const string &cstrPage, const unsigned int cuiPerPage, const string &cstrRef, const string &cstrParams, const string &cstrTagName, const string &cstrID, const bool bPageBarTop, const bool bShowPageNum )
{
	string strPageNav = string( );
	string strPageNum = string( );
	unsigned long ulStart = 0;
	
	if( !cstrPage.empty( ) )
		ulStart = (unsigned long)atoi( cstrPage.c_str( ) ) * cuiPerPage;
	
	if( ulCount && cuiPerPage > 0 )
	{
		if( ulStart > 0 )
			strPageNav += "<a title=\"" + gmapLANG_CFG["jump_to_page"] + ": " + CAtomInt( ( ulStart / cuiPerPage ) ).toString( ) + "\" href=\"javascript: ;\" onClick=\"javascript: load('" + cstrTagName + "','" + cstrID + "','" + cstrRef + "?page=" + CAtomInt( ( ulStart / cuiPerPage ) - 1 ).toString( ) + cstrParams + "');\">";
			
		strPageNav += gmapLANG_CFG["last_page"];
		
		if( ulStart > 0 )
			strPageNav += "</a>";
		
		strPageNav += "<span class=\"pipe\"> | </span>";
		
		if( ulStart + cuiPerPage < ulCount )
			strPageNav += "<a title=\"" + gmapLANG_CFG["jump_to_page"] + ": " + CAtomInt( ( ulStart / cuiPerPage ) + 2 ).toString( ) + "\" href=\"javascript: ;\" onClick=\"javascript: load('" + cstrTagName + "','" + cstrID + "','" + cstrRef + "?page=" + CAtomInt( ( ulStart / cuiPerPage ) + 1 ).toString( ) + cstrParams + "');\">";

		strPageNav += gmapLANG_CFG["next_page"];
		
		if( ulStart + cuiPerPage < ulCount )

			strPageNav += "</a>";
		
		strPageNum += gmapLANG_CFG["jump_to_page"] + ": \n";

		for( unsigned long ulPerPage = 0; ulPerPage < ulCount; ulPerPage += cuiPerPage )
		{
			strPageNum += " ";
			
			if( ( ulPerPage/cuiPerPage <= 2 ) || (  ( ulCount - ulPerPage )/cuiPerPage <= 2 ) || ( abs( ulStart - ulPerPage )/cuiPerPage <=2 ) )
			{
				// don't link to current page
				if( ulPerPage != ulStart )
					strPageNum += "<a title=\"" + gmapLANG_CFG["jump_to_page"] + ": " + CAtomInt( ( ulPerPage / cuiPerPage ) + 1 ).toString( ) + "\" href=\"javascript: ;\" onClick=\"javascript: load('" + cstrTagName + "','" + cstrID + "','" + cstrRef + "?page=" + CAtomInt( ulPerPage / cuiPerPage ).toString( ) + cstrParams + "');\">";

				if( bShowPageNum )
					strPageNum += CAtomInt( ( ulPerPage / cuiPerPage ) + 1 ).toString( );
				else
				{
					if( ulPerPage + cuiPerPage < ulCount )
						strPageNum += CAtomInt( ulPerPage + 1 ).toString( ) + " - " + CAtomInt( ulPerPage + cuiPerPage ).toString( );
					else
						strPageNum += CAtomInt( ulPerPage + 1 ).toString( ) + " - " + CAtomInt( ulCount ).toString( );
				}
				
				if( ulPerPage != ulStart )
					strPageNum += "</a>\n";

				// don't display a bar after the last page
				if( ulPerPage + cuiPerPage < ulCount )
					strPageNum += "<span class=\"pagenum_pipe\"> | </span>";
			}
			else
			{
				if( ( ulPerPage/cuiPerPage == 3 && ulPerPage < ulStart ) || (  ( ulCount - ulPerPage )/cuiPerPage == 3 && ulPerPage > ulStart ) )
					strPageNum += "...<span class=\"pagenum_pipe\"> | </span>";
			}
		}
		
		if( bPageBarTop )
			return "<p class=\"pagenum_top_bar\">\n" + strPageNav + "<br>\n" + strPageNum + "</p>\n\n";
		else
			return "<p class=\"pagenum_bottom\">\n" + strPageNum + "<br>\n" + strPageNav + "</p>\n\n";
	}
	else
		return string( );
}

const string UTIL_JS_Edit_Tool_Bar( const string &cstrTextarea )
{
	string strJSToolBar = string( );
	
	strJSToolBar += "var myAgent = navigator.userAgent.toLowerCase();\n";
	strJSToolBar += "var myVersion = parseInt(navigator.appVersion);\n";
	strJSToolBar += "var is_ie = ((myAgent.indexOf(\"msie\") != -1) && (myAgent.indexOf(\"opera\") == -1));\n";
	strJSToolBar += "var is_nav = ((myAgent.indexOf(\"mozilla\")!=-1) && (myAgent.indexOf(\"spoofer\")==-1)\n";
	strJSToolBar += "&& (myAgent.indexOf(\"compatible\") == -1) && (myAgent.indexOf(\"opera\")==-1)\n";
	strJSToolBar += "&& (myAgent.indexOf(\"webtv\") ==-1) && (myAgent.indexOf(\"hotjava\")==-1));\n";
	
	strJSToolBar += "var is_win = ((myAgent.indexOf(\"win\")!=-1) || (myAgent.indexOf(\"16bit\")!=-1));\n";
	strJSToolBar += "var is_mac = (myAgent.indexOf(\"mac\")!=-1);\n";
	
	strJSToolBar += "function doInsert(ibTag) {\n";
	strJSToolBar += "var isClose = false;\n";
	strJSToolBar += "var obj_ta = document." + cstrTextarea + ";\n";
	strJSToolBar += "if ( (myVersion >= 4) && is_ie && is_win) {\n";
	strJSToolBar += "  if(obj_ta.isTextEdit) {\n";
	strJSToolBar += "    obj_ta.focus();\n";
	strJSToolBar += "    var sel = document.selection;\n";
	strJSToolBar += "    var rng = sel.createRange();\n";
	strJSToolBar += "    rng.collapse;\n";
	strJSToolBar += "    if((sel.type == \"Text\" || sel.type == \"None\") && rng != null)\n";
	strJSToolBar += "      rng.text = ibTag; }\n";
	strJSToolBar += "  else\n";
	strJSToolBar += "    obj_ta.value += ibTag; }\n";
	strJSToolBar += "else if (obj_ta.selectionStart || obj_ta.selectionStart == '0') {\n";
	strJSToolBar += "  var startPos = obj_ta.selectionStart;\n";
	strJSToolBar += "  var endPos = obj_ta.selectionEnd;\n";
	strJSToolBar += "  obj_ta.value = obj_ta.value.substring(0, startPos) + ibTag + obj_ta.value.substring(endPos, obj_ta.value.length);\n";
	strJSToolBar += "  obj_ta.selectionEnd = startPos + ibTag.length; }\n";
	strJSToolBar += "else\n";
	strJSToolBar += "  obj_ta.value += ibTag;\n";
	strJSToolBar += "obj_ta.focus();\n";
	strJSToolBar += "return isClose;\n";
	strJSToolBar += "}\n\n";
	
	strJSToolBar += "function doInsertSelect(ibTag,ibClsTag) {\n";
	strJSToolBar += "var isClose = false;\n";
	strJSToolBar += "var obj_ta = document." + cstrTextarea + ";\n";
	strJSToolBar += "if ( (myVersion >= 4) && is_ie && is_win) {\n";
	strJSToolBar += "  if(obj_ta.isTextEdit) {\n";
	strJSToolBar += "    obj_ta.focus();\n";
	strJSToolBar += "    var sel = document.selection;\n";
	strJSToolBar += "    var rng = sel.createRange();\n";
	strJSToolBar += "    rng.collapse;\n";
	strJSToolBar += "    if((sel.type == \"Text\" || sel.type == \"None\") && rng != null)\n";
	strJSToolBar += "      var length = rng.text.length;\n";
	strJSToolBar += "      rng.text = ibTag + rng.text + ibClsTag;\n";
	strJSToolBar += "      rng.moveStart(\"character\", -length - ibClsTag.length);\n";
	strJSToolBar += "      rng.moveEnd(\"character\", -ibClsTag.length);\n";
	strJSToolBar += "      rng.select(); }\n";
	strJSToolBar += "  else\n";
	strJSToolBar += "    obj_ta.value += ibTag; }\n";
	strJSToolBar += "else if (obj_ta.selectionStart || obj_ta.selectionStart == '0') {\n";
	strJSToolBar += "  var startPos = obj_ta.selectionStart;\n";
	strJSToolBar += "  var endPos = obj_ta.selectionEnd;\n";
	strJSToolBar += "  obj_ta.value = obj_ta.value.substring(0, startPos) + ibTag + obj_ta.value.substring(startPos, endPos) + ibClsTag + obj_ta.value.substring(endPos, obj_ta.value.length);\n";
	strJSToolBar += "  obj_ta.selectionStart = startPos + ibTag.length;\n";
	strJSToolBar += "  obj_ta.selectionEnd = endPos + ibTag.length; }\n";
	strJSToolBar += "else\n";
	strJSToolBar += "  obj_ta.value += ibTag + ibClsTag;\n";
	strJSToolBar += "obj_ta.focus();\n";
	strJSToolBar += "return isClose;\n";
	strJSToolBar += "}\n\n";
	
	strJSToolBar += "function insertFont(theval,thetag) {\n";
	strJSToolBar += "  if (theval == 0) return;\n";
	strJSToolBar += "  doInsertSelect(\"[\" + thetag + \"=\" + theval + \"]\",\"[/\" + thetag + \"]\");\n";
	strJSToolBar += "  document." + cstrTextarea.substr( 0, cstrTextarea.find( '.' ) ) + ".size.selectedIndex = 0;\n";
	strJSToolBar += "  document." + cstrTextarea.substr( 0, cstrTextarea.find( '.' ) ) + ".color.selectedIndex = 0;";
//	strJSToolBar += "  document.edittorrent.font.selectedIndex = 0;";
	strJSToolBar += "}\n\n";
	
	strJSToolBar += "function tag_image(PromptImageURL, PromptError) {\n";
	strJSToolBar += "  var FoundErrors = '';\n";
	strJSToolBar += "  var enterURL = prompt(PromptImageURL, \"http://\");\n";
	strJSToolBar += "  if (!enterURL || enterURL==\"http://\") {\n";
	strJSToolBar += "    alert(PromptError + \" \" + PromptImageURL);\n";
	strJSToolBar += "    return; }\n";
	strJSToolBar += "  doInsert(\"[img]\"+enterURL+\"[/img]\");\n";
	strJSToolBar += "}\n\n";
	
	strJSToolBar += "function tag_url(PromptURL, PromptTitle, PromptError) {\n";
	strJSToolBar += "  var FoundErrors = '';\n";
	strJSToolBar += "  var enterURL = prompt(PromptURL, \"http://\");\n";
	strJSToolBar += "  var enterTITLE = prompt(PromptTitle, \"\");\n";
	strJSToolBar += "  if (!enterURL || enterURL==\"\" || enterURL==\"http://\") {FoundErrors += \" \" + PromptURL;}\n";
	strJSToolBar += "  if (FoundErrors) {alert(PromptError+FoundErrors);return;}\n";
	strJSToolBar += "  if (!enterTITLE || enterTITLE==\"\") {doInsert(\"[url]\"+enterURL+\"[/url]\");return;}\n";
	strJSToolBar += "  doInsert(\"[url=\"+enterURL+\"]\"+enterTITLE+\"[/url]\");\n";
	strJSToolBar += "}\n\n";
	
	return strJSToolBar;
}

const string UTIL_Edit_Tool_Bar( )
{
	string strToolBar = string( );
	
	strToolBar += "<input style=\"font-weight: bold\" type=\"button\" name=\"B\" value=\"" + gmapLANG_CFG["insert_b"] + "\" onclick=\"javascript: doInsertSelect('[b]', '[/b]')\">\n";
	strToolBar += "<input style=\"font-style: italic\" type=\"button\" name=\"I\" value=\"" + gmapLANG_CFG["insert_i"] + "\" onclick=\"javascript: doInsertSelect('[i]', '[/i]')\">\n";
	strToolBar += "<input style=\"text-decoration: underline\" type=\"button\" name=\"U\" value=\"" + gmapLANG_CFG["insert_u"] + "\" onclick=\"javascript: doInsertSelect('[u]', '[/u]')\">\n";
	strToolBar += "<select name=\"size\" onchange=\"insertFont(this.options[this.selectedIndex].value, 'size')\">\n";
	strToolBar += "<option value=\"0\">" + gmapLANG_CFG["insert_fontsize"] + "</option>\n";
	strToolBar += "<option value=\"1\">1</option>\n";
	strToolBar += "<option value=\"2\">2</option>\n";
	strToolBar += "<option value=\"3\">3</option>\n";
	strToolBar += "<option value=\"4\">4</option>\n";
	strToolBar += "<option value=\"5\">5</option>\n";
	strToolBar += "<option value=\"6\">6</option>\n";
	strToolBar += "<option value=\"7\">7</option>\n";
	strToolBar += "</select>\n";
	strToolBar += "<select name=\"color\" onchange=\"insertFont(this.options[this.selectedIndex].value, 'color')\">\n";
	strToolBar += "<option value=\"0\">" + gmapLANG_CFG["insert_fontcolor"] + "</option>\n";
	strToolBar += "<option style=\"background-color: black\" value=\"Black\">Black</option>\n";
	strToolBar += "<option style=\"background-color: sienna\" value=\"Sienna\">Sienna</option>\n";
	strToolBar += "<option style=\"background-color: darkolivegreen\" value=\"DarkOliveGreen\">Dark Olive Green</option>\n";
	strToolBar += "<option style=\"background-color: darkgreen\" value=\"DarkGreen\">Dark Green</option>\n";
	strToolBar += "<option style=\"background-color: darkslateblue\" value=\"DarkSlateBlue\">Dark Slate Blue</option>\n";
	strToolBar += "<option style=\"background-color: navy\" value=\"Navy\">Navy</option>\n";
	strToolBar += "<option style=\"background-color: indigo\" value=\"Indigo\">Indigo</option>\n";
	strToolBar += "<option style=\"background-color: darkslategray\" value=\"DarkSlateGray\">Dark Slate Gray</option>\n";
	strToolBar += "<option style=\"background-color: darkred\" value=\"DarkRed\">Dark Red</option>\n";
	strToolBar += "<option style=\"background-color: darkorange\" value=\"DarkOrange\">Dark Orange</option>\n";
	strToolBar += "<option style=\"background-color: olive\" value=\"Olive\">Olive</option>\n";
	strToolBar += "<option style=\"background-color: green\" value=\"Green\">Green</option>\n";
	strToolBar += "<option style=\"background-color: teal\" value=\"Teal\">Teal</option>\n";
	strToolBar += "<option style=\"background-color: blue\" value=\"Blue\">Blue</option>\n";
	strToolBar += "<option style=\"background-color: slategray\" value=\"SlateGray\">Slate Gray</option>\n";
	strToolBar += "<option style=\"background-color: dimgray\" value=\"DimGray\">Dim Gray</option>\n";
	strToolBar += "<option style=\"background-color: red\" value=\"Red\">Red</option>\n";
	strToolBar += "<option style=\"background-color: sandybrown\" value=\"SandyBrown\">Sandy Brown</option>\n";
	strToolBar += "<option style=\"background-color: yellowgreen\" value=\"YellowGreen\">Yellow Green</option>\n";
	strToolBar += "<option style=\"background-color: seagreen\" value=\"SeaGreen\">Sea Green</option>\n";
	strToolBar += "<option style=\"background-color: mediumturquoise\" value=\"MediumTurquoise\">Medium Turquoise</option>\n";
	strToolBar += "<option style=\"background-color: royalblue\" value=\"RoyalBlue\">Royal Blue</option>\n";
	strToolBar += "<option style=\"background-color: purple\" value=\"Purple\">Purple</option>\n";
	strToolBar += "<option style=\"background-color: gray\" value=\"Gray\">Gray</option>\n";
	strToolBar += "<option style=\"background-color: magenta\" value=\"Magenta\">Magenta</option>\n";
	strToolBar += "<option style=\"background-color: orange\" value=\"Orange\">Orange</option>\n";
	strToolBar += "<option style=\"background-color: yellow\" value=\"Yellow\">Yellow</option>\n";
	strToolBar += "<option style=\"background-color: lime\" value=\"Lime\">Lime</option>\n";
	strToolBar += "<option style=\"background-color: cyan\" value=\"Cyan\">Cyan</option>\n";
	strToolBar += "<option style=\"background-color: deepskyblue\" value=\"DeepSkyBlue\">Deep Sky Blue</option>\n";
	strToolBar += "<option style=\"background-color: darkorchid\" value=\"DarkOrchid\">Dark Orchid</option>\n";
	strToolBar += "<option style=\"background-color: silver\" value=\"Silver\">Silver</option>\n";
	strToolBar += "<option style=\"background-color: pink\" value=\"Pink\">Pink</option>\n";
	strToolBar += "<option style=\"background-color: wheat\" value=\"Wheat\">Wheat</option>\n";
	strToolBar += "<option style=\"background-color: lemonchiffon\" value=\"LemonChiffon\">Lemon Chiffon</option>\n";
	strToolBar += "<option style=\"background-color: palegreen\" value=\"PaleGreen\">Pale Green</option>\n";
	strToolBar += "<option style=\"background-color: paleturquoise\" value=\"PaleTurquoise\">Pale Turquoise</option>\n";
	strToolBar += "<option style=\"background-color: lightblue\" value=\"LightBlue\">Light Blue</option>\n";
	strToolBar += "<option style=\"background-color: plum\" value=\"Plum\">Plum</option>\n"; 
	strToolBar += "<option style=\"background-color: white\" value=\"White\">White</option>\n";
	strToolBar += "</select>\n";
	
//	strToolBar += "<select name=\"font\" onchange=\"insertFont(this.options[this.selectedIndex].value, 'font')\">";
//	strToolBar += "<option value=\"0\">" + gmapLANG_CFG["insert_fonttype"] + "</option>";
//	strToolBar += "<option value=\"Arial\">Arial</option>";
//	strToolBar += "<option value=\"Arial Black\">Arial Black</option>";
//	strToolBar += "<option value=\"Arial Narrow\">Arial Narrow</option>";
//	strToolBar += "<option value=\"Book Antiqua\">Book Antiqua</option>";
//	strToolBar += "<option value=\"Century Gothic\">Century Gothic</option>";
//	strToolBar += "<option value=\"Comic Sans MS\">Comic Sans MS</option>";
//	strToolBar += "<option value=\"Courier New\">Courier New</option>";
//	strToolBar += "<option value=\"Fixedsys\">Fixedsys</option>";
//	strToolBar += "<option value=\"Garamond\">Garamond</option>";
//	strToolBar += "<option value=\"Georgia\">Georgia</option>";
//	strToolBar += "<option value=\"Impact\">Impact</option>";
//	strToolBar += "<option value=\"Lucida Console\">Lucida Console</option>";
//	strToolBar += "<option value=\"Lucida Sans Unicode\">Lucida Sans Unicode</option>";
//	strToolBar += "<option value=\"Microsoft Sans Serif\">Microsoft Sans Serif</option>";
//	strToolBar += "<option value=\"Palatino Linotype\">Palatino Linotype</option>";
//	strToolBar += "<option value=\"System\">System</option>";
//	strToolBar += "<option value=\"Tahoma\">Tahoma</option>";
//	strToolBar += "<option value=\"Times New Roman\">Times New Roman</option>";
//	strToolBar += "<option value=\"Trebuchet MS\">Trebuchet MS</option>";
//	strToolBar += "<option value=\"Verdana\">Verdana</option>";
//	strToolBar += "</select>";
	
	strToolBar += "<input type=\"button\" name=\"IMG\" value=\"" + gmapLANG_CFG["insert_img"] + "\" onclick=\"javascript: tag_image('" + gmapLANG_CFG["insert_img_fill"] + "','" + gmapLANG_CFG["insert_error"] + "')\">\n";
	strToolBar += "<input type=\"button\" name=\"URL\" value=\"" + gmapLANG_CFG["insert_url"] + "\" onclick=\"javascript: tag_url('" + gmapLANG_CFG["insert_url_fill"] + "','" + gmapLANG_CFG["insert_url_title"] + "','" + gmapLANG_CFG["insert_error"] +"')\">\n";
	strToolBar += "<input type=\"button\" name=\"QUOTE\" value=\"" + gmapLANG_CFG["insert_quote"] + "\" onclick=\"javascript: doInsertSelect('[quote]', '[/quote]')\">";
	
	return strToolBar;
}

const string UTIL_FailureReason( const string &cstrFailureReason )
{
	CAtomDicti dict;

	dict.setItem( "failure reason", new CAtomString( cstrFailureReason ) );

	return Encode( &dict );
}

// tphogan - moved UTIL_Compact code into tracker_announce.cpp

// =Xotic= - moved UTIL_DecodeHTTPPost into client.cpp

//addition by labarks
const string UTIL_Date( )
{
	// format date

	time_t tNow = time( 0 );

	char pTime[256];
	memset( pTime, 0, sizeof( pTime ) / sizeof( char ) );
	strftime( pTime, sizeof( pTime ) / sizeof( char ), "%a, %d %b %Y %H:%M:%S", localtime( &tNow ) );

#if defined( __APPLE__ ) || defined( __FREEBSD__ ) || defined( __OPENBSD__ ) || defined( __NETBSD__ )
	long timezone = -( localtime( &tNow )->tm_gmtoff );
#endif

	string strDate = pTime;

	// timezone has the wrong sign, change it

	if( timezone > 0 )
		strDate += " -";
	else
		strDate += " +";

	if( abs( timezone / 3600 ) % 60 < 10 )
		strDate += "0";

	strDate += CAtomInt( abs( timezone / 3600 ) % 60 ).toString( );

	if( abs( timezone / 60 ) % 60 < 10 )
		strDate += "0";

	strDate += CAtomInt( abs( timezone / 60 ) % 60 ).toString( );

	return strDate;
}

// labarks end
const string UTIL_PubDate( )
{
	// format date

	time_t tNow = time( 0 );

	char pTime[256];
	memset( pTime, 0, sizeof( pTime ) / sizeof( char ) );
	strftime( pTime, sizeof( pTime ) / sizeof( char ), "%a, %d %b %Y 00:00:01", localtime( &tNow ) );

#if defined( __APPLE__ ) || defined( __FREEBSD__ ) || defined( __OPENBSD__ ) || defined( __NETBSD__ )
	long timezone = -( localtime( &tNow )->tm_gmtoff );
#endif

	string strDate = pTime;

	// timezone has the wrong sign, change it

	if( timezone > 0 )
		strDate += " -";
	else
		strDate += " +";

	if( abs( timezone / 3600 ) % 60 < 10 )
		strDate += "0";

	strDate += CAtomInt( abs( timezone / 3600 ) % 60 ).toString( );

	if( abs( timezone / 60 ) % 60 < 10 )
		strDate += "0";

	strDate += CAtomInt( abs( timezone / 60 ) % 60 ).toString( );

	return strDate;
}

// XBNBT
const bool UTIL_CheckDir( const char *szFile )
{
	// check if directory exists
	if( ( access( szFile, 0 ) ) != DIR_ERROR  )
		return true;
	else
		return false;
}

// Return only the displayable characters; alphas, digits and puncts
const string UTIL_StringToDisplay( const string &strString )
{
	string strEscape = string( );
	unsigned char ucChar = 0;

	for( unsigned long ulCount = 0; ulCount < strString.size( ); ulCount++ )
	{
		ucChar = strString[ulCount];

		if( isalpha( ucChar ) || isdigit( ucChar ) || ispunct( ucChar ) || ucChar == ' ' )
			strEscape += ucChar;
	}

	return strEscape;
}

// Strip Line Feeds
const string UTIL_StringStripLF( const string &strString )
{
	string strEscape = string( );
	unsigned char ucChar = 0;

	for( unsigned long ulCount = 0; ulCount < strString.size( ); ulCount++ )
	{
		ucChar = strString[ulCount];

		if( ucChar != '\r' )
			strEscape += ucChar;
	}

	return strEscape;
}

// XBNBT
const string UTIL_Xsprintf( const char *format, ... )
{
	char pBuf[1024];
	memset( pBuf, 0, sizeof( pBuf ) / sizeof( char ) );

	va_list args;
	va_start( args, format );
	//va_arg( args, char * );
	vsnprintf( pBuf, sizeof( pBuf ) / sizeof( char ), format, args );
	va_end( args );

	return string( pBuf );
}

// Is the IP address in the IP ban list?
const bool UTIL_IsIPBanList( const string &cstrPeerIP, CAtomList *m_pIPBannedList )
{
	string tempPeerIP = string( );
	
	tempPeerIP = cstrPeerIP;
	
	vector<CAtom *>list;
	list.reserve(1000);
	list = ( (CAtomList *)m_pIPBannedList )->getValue( ); 

	CAtomString *str = 0;
	string currentline = string( );

	for( unsigned long ulCount = 0; ulCount < list.size( ); ulCount++ )
	{ 
		str = (CAtomString *)list[ulCount]; 
		currentline = str->getValue();
		
// 		string :: size_type iStar = 0;
// 		
// 		iStar = currentline.find( "*" );
// 		
// 		if( iStar != string :: npos && iStar )
// 			tempPeerIP.replace( iStar, 1, "*" );

		if ( currentline == tempPeerIP )
			return true;
	}
	return false;
}

// Is the client type in the client ban list?
const bool UTIL_IsClientBanList( const string &cstrPeerID, CAtomList *m_pClientBannedList, const bool &bUserAgent ) 
{
	string tempPeerID = string( );

	if( bUserAgent )
		tempPeerID = cstrPeerID;
	else
		tempPeerID = UTIL_StringToEscaped(cstrPeerID);

	vector<CAtom *>list;
	list.reserve(100);
	list = ( (CAtomList *)m_pClientBannedList )->getValue( ); 

	CAtomString *str = 0;
	string currentline = string( );

	for( unsigned long ulCount = 0; ulCount < list.size( ); ulCount++ ) 
	{ 
		string tempPeerID2 = tempPeerID;

		str = (CAtomString *)list[ulCount]; 
		currentline = str->getValue();
		
		if( tempPeerID2.length( ) >= currentline.length( ) )
		{
			string :: size_type iStar = currentline.find( "*" );
		
			while( iStar != string :: npos )
			{
				tempPeerID2 = tempPeerID2.replace( iStar, 1, "*" );
				iStar = currentline.find( "*", iStar + 1 );
			}
		}

		if ( currentline == tempPeerID2.substr( 0, currentline.length( ) ) && tempPeerID2 != "" )
			return true;
		else if ( currentline == tempPeerID2.c_str( ) )
			return true;
	}
	return false; 
} 
