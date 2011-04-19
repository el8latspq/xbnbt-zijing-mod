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

#include <signal.h>

#if defined ( WIN32 )
 #include <time.h>
#else
 #include <sys/time.h>
#endif

#include "bnbt.h"
#include "bnbt_mysql.h"
#include "atom.h"
#include "config.h"
#include "link.h"
#include "server.h"	  
#include "util.h"

#ifdef WIN32
 #include "util_ntservice.h"
#endif

#ifndef WIN32
 int GetLastError( ) { return errno; }
#endif

// Convert errors into a string
static inline const char *UTIL_ErrorToString( const int &ciError )
{
	switch( ciError )
	{
	case EWOULDBLOCK: return "EWOULDBLOCK";
	case EINPROGRESS: return "EINPROGRESS";
	case EALREADY: return "EALREADY";
	case ENOTSOCK: return "ENOTSOCK";
	case EDESTADDRREQ: return "EDESTADDRREQ";
	case EMSGSIZE: return "EMSGSIZE";
	case EPROTOTYPE: return "EPROTOTYPE";
	case ENOPROTOOPT: return "ENOPROTOOPT";
	case EPROTONOSUPPORT: return "EPROTONOSUPPORT";
	case ESOCKTNOSUPPORT: return "ESOCKTNOSUPPORT";
	case EOPNOTSUPP: return "EOPNOTSUPP";
	case EPFNOSUPPORT: return "EPFNOSUPPORT";
	case EAFNOSUPPORT: return "EAFNOSUPPORT";
	case EADDRINUSE: return "EADDRINUSE";
	case EADDRNOTAVAIL: return "EADDRNOTAVAIL";
	case ENETDOWN: return "ENETDOWN";
	case ENETUNREACH: return "ENETUNREACH";
	case ENETRESET: return "ENETRESET";
	case ECONNABORTED: return "ECONNABORTED";
	case ECONNRESET: return "ECONNRESET";
	case ENOBUFS: return "ENOBUFS";
	case EISCONN: return "EISCONN";
	case ENOTCONN: return "ENOTCONN";
	case ESHUTDOWN: return "ESHUTDOWN";
	case ETOOMANYREFS: return "ETOOMANYREFS";
	case ETIMEDOUT: return "ETIMEDOUT";
	case ECONNREFUSED: return "ECONNREFUSED";
	case ELOOP: return "ELOOP";
	case ENAMETOOLONG: return "ENAMETOOLONG";
	case EHOSTDOWN: return "EHOSTDOWN";
	case EHOSTUNREACH: return "EHOSTUNREACH";
	case ENOTEMPTY: return "ENOTEMPTY";
	case EUSERS: return "EUSERS";
	case EDQUOT: return "EDQUOT";
	case ESTALE: return "ESTALE";
	case EREMOTE: return "EREMOTE";
	case EFAULT: return "EFAULT";
	case EINTR: return "EINTR";
	case EBADF: return "EBADF";
	case EACCES: return "EACCES";
	case EINVAL: return "EINVAL";
	case EMFILE: return "EMFILE";
#ifdef EPROCLIM
	case EPROCLIM: return "EPROCLIM";
#endif
#ifdef SYSNOTREADY
	case SYSNOTREADY: return "SYSNOTREADY";
#endif
#ifdef VERNOTSUPPORTED
	case VERNOTSUPPORTED: return "VERNOTSUPPORTED";
#endif
#ifdef NOTINITIALISED
	case NOTINITIALISED: return "NOTINITIALISED";
#endif
#ifdef EDISCON
	case EDISCON: return "EDISCON";
#endif
#ifdef ENOMORE
	case ENOMORE: return "ENOMORE";
#endif
#ifdef ECANCELLED
	case ECANCELLED: return "ECANCELLED";
#endif
#ifdef EINVALIDPROCTABLE
	case EINVALIDPROCTABLE: return "EINVALIDPROCTABLE";
#endif
#ifdef EINVALIDPROVIDER
	case EINVALIDPROVIDER: return "EINVALIDPROVIDER";
#endif
#ifdef EPROVIDERFAILEDINIT
	case EPROVIDERFAILEDINIT: return "EPROVIDERFAILEDINIT";
#endif
#ifdef SYSCALLFAILURE
	case SYSCALLFAILURE: return "SYSCALLFAILURE";
#endif
#ifdef SERVICE_NOT_FOUND
	case SERVICE_NOT_FOUND: return "SERVICE_NOT_FOUND";
#endif
#ifdef TYPE_NOT_FOUND
	case TYPE_NOT_FOUND: return "TYPE_NOT_FOUND";
#endif
#ifdef _E_NO_MORE
	case _E_NO_MORE: return "_E_NO_MORE";
#endif
#ifdef _E_CANCELLED
	case _E_CANCELLED: return "_E_CANCELLED";
#endif
#ifdef EREFUSED
	case EREFUSED: return "EREFUSED";
#endif
#ifdef HOST_NOT_FOUND
	case HOST_NOT_FOUND: return "HOST_NOT_FOUND";
#endif
#ifdef TRY_AGAIN
	case TRY_AGAIN: return "TRY_AGAIN";
#endif
#ifdef NO_RECOVERY
	case NO_RECOVERY: return "NO_RECOVERY";
#endif
#if defined WIN32 && defined NO_DATA
	case NO_DATA: return "NO_DATA";
	//case NO_ADDRESS: return "NO_ADDRESS";
#endif
#ifdef EPIPE
	case EPIPE: return "EPIPE";
#endif
	default: return UTIL_Xsprintf( "UNKNOWN %s", CAtomInt( ciError ).toString( ).c_str( ) ).c_str( );
	}
}   

const char *GetLastErrorString( )
{
#ifdef WIN32
	return UTIL_ErrorToString( GetLastError( ) );
#else
	return UTIL_ErrorToString( errno );
#endif
}

time_t gtStartTime;

unsigned long GetTime( )
{
// 	return (unsigned long)( time( 0 ) - gtStartTime );
	return (unsigned long)( time( 0 ) );
}

unsigned long GetStartTime( )
{
// 	return (unsigned long)( time( 0 ) - gtStartTime );
	return (unsigned long)( gtStartTime );
}

CServer *gpServer = 0;	 
CMutex gmtxOutput;
CLink *gpLink = 0;
CLinkServer *gpLinkServer = 0;
CHUBLink *gpHUBLink = 0;
CHUBLinkServer *gpHUBLinkServer = 0;
string gstrLogDir = string( );
string gstrLogFile = string( );
string gstrLogFilePattern = string( );
FILE *gpLog = 0;
string gstrErrorLogDir = string( );
string gstrErrorLogFile = string( );
string gstrErrorLogFilePattern = string( );
FILE *gpErrorLog = 0;
string gstrAccessLogDir = string( );
string gstrAccessLogFile = string( );
string gstrAccessLogFilePattern = string( );
FILE *gpAccessLog = 0;
unsigned long gulLogCount = 0;
unsigned long gulErrorLogCount = 0;
unsigned long gulAccessLogCount = 0;
unsigned int guiFlushInterval = 0;
bool gbDebug = false;
unsigned long gulRestartServerNext = 0;
unsigned int guiRestartServerInterval = 0;
unsigned int guiMaxConns = 0;
unsigned int guiMaxRecvSize = 0;
string gstrStyle = string( );
string gstrCharSet = string( );
string gstrRealm = string( );

// The Trinity Edition 7.5r3 - Addition Begins
// declares variable for Custom NT Service Name
string gstrNTServiceName = string( );

// TCP window size
unsigned int guiSO_SNDBUF = 0;
unsigned int guiSO_RECBUF = 0;

// Naggles algorithm
bool gbTCP_NODELAY = false;

// PID
string gstrPID = string( );

// Debug level
unsigned char gucDebugLevel = 0;

// The signal catching routine
void sigCatcher( int sig )
{
	if( gbDebug && ( gucDebugLevel & DEBUG_BNBT ) )
		UTIL_LogPrint( "sigCatcher: reset the signals \n" );

	// reset the signal catcher
	signal( SIGABRT, sigCatcher );
	signal( SIGINT, sigCatcher );
	signal( SIGTERM, sigCatcher );
#if defined SIGHUP
	signal( SIGHUP, sigCatcher );
#endif

	if( gbDebug && ( gucDebugLevel & DEBUG_BNBT ) )
		UTIL_LogPrint( "sigCatcher: reset the signals completed \n" );

	if( gbDebug && ( gucDebugLevel & DEBUG_BNBT ) )
	{
		switch( sig )
		{
		case SIGABRT:
			UTIL_LogPrint( "sigCatcher: caught SIGABRT\n" );

			break;
		case SIGINT:
			UTIL_LogPrint( "sigCatcher: caught SIGINT\n" );

			break;
		case SIGTERM:
			UTIL_LogPrint( "sigCatcher: caught SIGTERM\n" );

			break;
#if defined SIGHUP
		case SIGHUP:
			UTIL_LogPrint( "sigCatcher: caught SIGHUP\n" );

			break;
#endif
		default:
			UTIL_LogPrint( "sigCatcher: caught unknown signal (signal %d)\n", sig );
		}
	}

	if( gpServer )
	{
		if( gpServer->isDying( ) )
		{
			if( gbDebug && ( gucDebugLevel & DEBUG_BNBT ) )
				UTIL_LogPrint( "sigCatcher: server is dying -> exit\n" );

			exit( 1 );
		}
		else
		{
			if( gbDebug && ( gucDebugLevel & DEBUG_BNBT ) )
				UTIL_LogPrint( "sigCatcher: server sending kill\n" );

			gpServer->Kill( );
		}
	}
	else
	{
		if( gbDebug && ( gucDebugLevel & DEBUG_BNBT ) )
			UTIL_LogPrint( "sigCatcher: server closed -> exit\n" );
		
		exit( 1 );
	}
	
	if( gbDebug && ( gucDebugLevel & DEBUG_BNBT ) )
		UTIL_LogPrint( "sigCatcher: killing server\n" );
}

// The main programme
int main( int argc, char *argv[] )
{
	printf( "%s\n", XBNBT_VER.c_str( ) );

#ifdef WIN32
	if( argv[0] )
	{
		char *szEndPos = strrchr( argv[0], CHAR_BS );

		if( szEndPos )
		{
			char *szEXEPath = new char[szEndPos - argv[0] + 1];
			memcpy( szEXEPath, argv[0], szEndPos - argv[0] );
			szEXEPath[szEndPos - argv[0]] = TERM_CHAR;

			SetCurrentDirectory( szEXEPath );

			delete [] szEXEPath;
		}
	}

	if( argc > 1 )
	{
		// The Trinity Edition 7.5r3 - Addition Begins
		// Added for Custom NT Service Name Code
		CFG_Open( CFG_FILE );
#define BNBT_SERVICE_NAME const_cast<LPSTR> (CFG_GetString( "cbtt_service_name", "BNBT Service" ).c_str())
		CFG_Close( CFG_FILE );

		printf( "Service name %s\n", string( BNBT_SERVICE_NAME ).c_str( ) );

		// install service
		if( _stricmp( argv[1], "-i" ) == 0 )
		{
			if( UTIL_NTServiceTest( ) )
				printf( "BNBT Service is already installed!\n" );
			else
			{
				if( UTIL_NTServiceInstall( ) )
					printf( "BNBT Service installed.\n" );
				else
					printf( "BNBT Service failed to install (error %d).\n", GetLastError( ) );
			}

			return 0;
		}
		// uninstall service
		else if( _stricmp( argv[1], "-u" ) == 0 )
		{
			if( !UTIL_NTServiceTest( ) )
				printf( "BNBT Service is not installed!\n" );
			else
			{
				if( UTIL_NTServiceUninstall( ) )
					printf( "BNBT Service uninstalled.\n" );
				else
					printf( "BNBT Service failed to uninstall (error %d).\n", GetLastError( ) );
			}

			return 0;
		}
		// start
		else if( _stricmp( argv[1], "-start" ) == 0 )
		{
			if( !UTIL_NTServiceTest( ) )
				printf( "BNBT Service is not installed!\n" );
			else
			{
				printf( "Starting BNBT Service.\n" );

				if( !UTIL_NTServiceStart( ) )
					printf( "BNBT Service failed to start (error %d).\n", GetLastError( ) );
			}

			return 0;
		}
		// stop
		else if( _stricmp( argv[1], "-stop" ) == 0 )
		{
			if( !UTIL_NTServiceTest( ) )
				printf( "BNBT Service is not installed!\n" );
			else
			{
				printf( "Stopping BNBT Service.\n" );

				if( !UTIL_NTServiceStop( ) )
					printf( "BNBT Service failed to stop (error %d).\n", GetLastError( ) );
			}

			return 0;
		}
		// internal start
		else if( _stricmp( argv[1], "-s" ) == 0 )
		{
			SERVICE_TABLE_ENTRY st[] = { { BNBT_SERVICE_NAME, NTServiceMain }, { 0, 0 }	};

			StartServiceCtrlDispatcher( st );

			return 0;
		}
		// Print version information
		else if( _stricmp( argv[1], "-v" ) == 0 )
		{
			printf( "%s\n", XBNBT_VER.c_str( ) );

			return 0;
		}
		// Print help
		else if( ( _stricmp( argv[1], "-h" ) || _stricmp( argv[1], "-?" ) ) == 0 )
		{
			printf("usage: bnbt [-i] [-u] [-start] [-stop] [-s] [-v] [-?|-h]\n" );
			printf("Options:\n" );
			printf("-?, -h          : This help\n" );
			printf("-i              : Install NT Service\n" );
			printf("-u              : Unstall NT Service\n" );
			printf("-start          : Start NT Service\n" );
			printf("-stop           : Stop NT Service\n" );
			printf("-s              : Internal start NT Service\n" );
			printf("-v              : Show version\n" );

			return 0;
		}
	}
#else
	// read command-line arguments
    //char *example = 0;
    for ( int arg = 0; arg < argc; arg++ )
	{
        if ( strcmp( argv[arg], "-v" ) == 0 || strcmp( argv[arg], "-version" ) == 0 )
		{
			printf( "%s\n", XBNBT_VER.c_str( ) );
			return 0;
        }
        //if (0 == strcmp(argv[arg], "-example")) {
        //    if (arg+1 < argc) example = argv[arg+1];
        //}
        if ( strcmp( argv[arg], "-help" ) == 0 || strcmp( argv[arg], "-?" ) == 0 || strcmp( argv[arg], "-h" ) == 0 )
		{
            fprintf( stderr, "usage:  %s [-v|-version] [-?|-h|-help]\n", argv[0] );
            return 0;
        }
    }
#endif

	printf( "Setting signals ... " );

#if defined ( SIGPIPE )
	// disable SIGPIPE since some systems like OS X don't define MSG_NOSIGNAL
	signal( SIGPIPE, SIG_IGN );
#endif

	// catch SIGABRT, SIGINT,  SIGTERM and SIGHUP
	signal( SIGABRT, sigCatcher );
	signal( SIGINT, sigCatcher );
	signal( SIGTERM, sigCatcher );
#if defined ( SIGHUP )
	signal( SIGHUP, sigCatcher );
#endif

	printf( "Done\n" );
	return bnbtmain( );
}

int bnbtmain( )
{
	gmtxOutput.Initialize( );

	UTIL_LogPrint( "Tracker Start\n" );

	srand( (unsigned int)time( 0 ) );

	gtStartTime = time( 0 );

	CFG_Open( CFG_FILE );
	CFG_SetDefaults( );
	CFG_Close( CFG_FILE );

	// XBNBT Language Configuration
	LANG_CFG_Init( LANG_CFG_FILE );

	UTIL_LogPrint( "Setting debug level, access and error log files\n" );

	guiFlushInterval = CFG_GetInt( "bnbt_flush_interval", 100 );
	
	// Log
	gstrLogDir = CFG_GetString( "bnbt_log_dir", string( ) );
	gstrLogFilePattern = CFG_GetString( "bnbt_log_file_pattern", "%Y-%m-%d.log" );
	
	if( !gstrLogDir.empty( ) && gstrLogDir[gstrLogDir.size( ) - 1] != PATH_SEP )
		gstrLogDir += PATH_SEP;

	gpLog = 0;
	gulLogCount = 0;
	
	// Error log
	gstrErrorLogDir = CFG_GetString( "bnbt_error_log_dir", string( ) );
	gstrErrorLogFilePattern = CFG_GetString( "bnbt_error_log_file_pattern", "%Y-%m-%de.log" );
	
	if( !gstrErrorLogDir.empty( ) && gstrErrorLogDir[gstrErrorLogDir.size( ) - 1] != PATH_SEP )
		gstrErrorLogDir += PATH_SEP;

	gpErrorLog = 0;
	gulErrorLogCount = 0;

	// Access log
	gstrAccessLogDir = CFG_GetString( "bnbt_access_log_dir", string( ) );
	gstrAccessLogFilePattern = CFG_GetString( "bnbt_access_log_file_pattern", "%Y-%m-%da.log" );

	if( !gstrAccessLogDir.empty( ) && gstrAccessLogDir[gstrAccessLogDir.size( ) - 1] != PATH_SEP )
		gstrAccessLogDir += PATH_SEP;

	gpAccessLog = 0;
	gulAccessLogCount = 0;

	// Set the debug level
	gbDebug = CFG_GetInt( "bnbt_debug", 1 ) == 0 ? false : true;
	gucDebugLevel = (unsigned char)CFG_GetInt( "bnbt_debug_level", 0 );

	UTIL_LogPrint( "***********************************************\n" );

	if( gbDebug && ( gucDebugLevel & DEBUG_BNBT ) )
	{
		UTIL_LogPrint( "Debug is on at level (%u)\n", gucDebugLevel );
		
		if( gstrLogDir.empty( ) )
			UTIL_LogPrint( "Log directory is not set\n" );
		else
			UTIL_LogPrint( "Log dir (%s)\n", gstrLogDir.c_str( ) );

		if( gstrErrorLogDir.empty( ) )
			UTIL_LogPrint( "Error log directory is not set\n" );
		else
			UTIL_LogPrint( "Error log dir (%s)\n", gstrErrorLogDir.c_str( ) );

		if( gstrErrorLogDir.empty( ) )
			UTIL_LogPrint( "Access log directory is not set\n" );
		else
			UTIL_LogPrint( "Access log dir (%s)\n", gstrAccessLogDir.c_str( ) );
	}

	// Other globals
	if( gbDebug && ( gucDebugLevel & DEBUG_BNBT ) )
		UTIL_LogPrint( "Setting global variables\n" );

	// PID
	gstrPID = CFG_GetString( "bnbt_pid_file", string( ) );

	if( gstrPID.empty( ) )
	{
		if( gbDebug && ( gucDebugLevel & DEBUG_BNBT ) )
			UTIL_LogPrint( "PID file is not set\n" );
	}
	else
	{
		// Write the process ID to file
		const long clPID( (long)getpid( ) );

		UTIL_LogPrint( "Recording PID (%ld) to file (%s)\n", clPID, gstrPID.c_str( ) );

		FILE *pFile = FILE_ERROR;

		pFile = fopen( gstrPID.c_str( ), "wt" );

		if( pFile == FILE_ERROR )
			UTIL_LogPrint( "Unable to write PID file (%s)\n", gstrPID.c_str( ) );
		else
		{
			fprintf( pFile, "%ld", clPID );
			fclose( pFile );
		}

		if( gbDebug && ( gucDebugLevel & DEBUG_BNBT ) )
			UTIL_LogPrint( "Recording PID completed\n" );
	}
	
	guiRestartServerInterval = CFG_GetInt( "bnbt_restart_interval", 24 );
	gulRestartServerNext = GetTime( ) + guiRestartServerInterval * 3600;

	guiMaxConns = CFG_GetInt( "bnbt_max_conns", 64 );
	guiMaxRecvSize = CFG_GetInt( "bnbt_max_recv_size", 524288 );
	gstrStyle = CFG_GetString( "bnbt_style_sheet", string( ) );
	gstrCharSet = CFG_GetString( "bnbt_charset", "utf-8" );
	gstrRealm = CFG_GetString( "bnbt_realm", "BNBT" );

	// The Trinity Edition - Addition Begins
	// Sets the value for the Custom NT Service Name variable
	gstrNTServiceName = CFG_GetString( "cbtt_service_name", "BNBT Service" );

	// TCP window size
	guiSO_RECBUF = CFG_GetInt( "xbnbt_so_recbuf", 128 ) * 1024;
	guiSO_SNDBUF = CFG_GetInt( "xbnbt_so_sndbuf", 128 ) * 1024;

	// Naggles algorithm
	gbTCP_NODELAY = CFG_GetInt( "xbnbt_tcp_nodelay", 0 ) == 0 ? false : true;

#ifdef WIN32
	// start winsock
	if( gbDebug && ( gucDebugLevel & DEBUG_BNBT ) )
		UTIL_LogPrint( "Starting the windows sockets\n" );

	WSADATA wsaData;

	const int ciResult( WSAStartup( MAKEWORD(2,2), &wsaData ) );

	if ( ciResult != NO_ERROR )
	{
		UTIL_LogPrint( ( gmapLANG_CFG["unable_to_start_winsock"] + "\n" ).c_str( ), GetLastErrorString( ) );

		// Delete the PID file
		if( !gstrPID.empty( ) )
		{
			if( gbDebug && ( gucDebugLevel & DEBUG_BNBT ) )
				UTIL_LogPrint( "Deleting the PID file\n" );

			UTIL_DeleteFile( gstrPID.c_str( ) );
		}
		
		// Close the log
		if( gpLog )
		{
			if( gbDebug && ( gucDebugLevel & DEBUG_BNBT ) )
				UTIL_LogPrint( "Closing the log\n" );

			fclose( gpLog );
		}

		// Close the access log
		if( gpAccessLog )
		{
			if( gbDebug && ( gucDebugLevel & DEBUG_BNBT ) )
				UTIL_LogPrint( "Closing the access log\n" );

			fclose( gpAccessLog );
		}

		// Close the error log
		if( gpErrorLog )
		{
			if( gbDebug && ( gucDebugLevel & DEBUG_BNBT ) )
				UTIL_LogPrint( "Closing the error log\n" );

			fclose( gpErrorLog );
		}

		UTIL_LogPrint( "Tracker Stop\n" );

		gmtxOutput.Destroy( );

		return 1;
	}
#endif

	// start mysql
	if( gbDebug )
		if( gucDebugLevel & DEBUG_BNBT )
			UTIL_LogPrint( "Setting MySQL dstate global variables\n" );

	if( !( gpMySQL = mysql_init( 0 ) ) )
	{
		UTIL_LogPrint( ( gmapLANG_CFG["bnbt_mysql_error"] + "\n" ).c_str( ), mysql_error( gpMySQL ) );

		// Delete the PID file
		if( !gstrPID.empty( ) )
		{
			if( gbDebug )
				if( gucDebugLevel & DEBUG_BNBT )
					UTIL_LogPrint( "Deleting the PID file\n" );

			UTIL_DeleteFile( gstrPID.c_str( ) );
		}
		
		// Close the log
		if( gpLog )
		{
			if( gbDebug )
				if( gucDebugLevel & DEBUG_BNBT )
					UTIL_LogPrint( "Closing the log\n" );

			fclose( gpLog );
		}

		// Close the access log
		if( gpAccessLog )
		{
			if( gbDebug )
				if( gucDebugLevel & DEBUG_BNBT )
					UTIL_LogPrint( "Closing the access log\n" );

			fclose( gpAccessLog );
		}

		// Close the error log
		if( gpErrorLog )
		{
			if( gbDebug )
				if( gucDebugLevel & DEBUG_BNBT )
					UTIL_LogPrint( "Closing the error log\n" );

			fclose( gpErrorLog );
		}

		UTIL_LogPrint( "Tracker Stop\n" );

		gmtxOutput.Destroy( );

		return 1;
	}

	gstrMySQLHost = CFG_GetString( "mysql_host", string( ) );
	gstrMySQLDatabase = CFG_GetString( "mysql_database", "bnbt" );
	gstrMySQLUser = CFG_GetString( "mysql_user", string( ) );
	gstrMySQLPassword = CFG_GetString( "mysql_password", string( ) );
	guiMySQLPort = CFG_GetInt( "mysql_port", 0 );
	
	if( !( mysql_real_connect( gpMySQL, gstrMySQLHost.c_str( ), gstrMySQLUser.c_str( ), gstrMySQLPassword.c_str( ), 0, guiMySQLPort, 0, 0 ) ) )
	{
		UTIL_LogPrint( ( gmapLANG_CFG["bnbt_mysql_error"] + "\n" ).c_str( ), mysql_error( gpMySQL ) );

		// Delete the PID file
		if( !gstrPID.empty( ) )
		{
			if( gbDebug )
				if( gucDebugLevel & DEBUG_BNBT )
					UTIL_LogPrint( "Deleting the PID file\n" );

			UTIL_DeleteFile( gstrPID.c_str( ) );
		}
		
		// Close the log
		if( gpLog )
		{
			if( gbDebug )
				if( gucDebugLevel & DEBUG_BNBT )
					UTIL_LogPrint( "Closing the log\n" );

			fclose( gpLog );
		}

		// Close the access log
		if( gpAccessLog )
		{
			if( gbDebug )
				if( gucDebugLevel & DEBUG_BNBT )
					UTIL_LogPrint( "Closing the access log\n" );

			fclose( gpAccessLog );
		}

		// Close the error log
		if( gpErrorLog )
		{
			if( gbDebug )
				if( gucDebugLevel & DEBUG_BNBT )
					UTIL_LogPrint( "Closing the error log\n" );

			fclose( gpErrorLog );
		}

		UTIL_LogPrint( "Tracker Stop\n" );

		gmtxOutput.Destroy( );

		return 1;
	}
	
	UTIL_LogPrint( ( gmapLANG_CFG["bnbt_mysql_connected"] + "\n" ).c_str( ) );

	UTIL_MySQLCreateDatabase( );

	if( mysql_select_db( gpMySQL, gstrMySQLDatabase.c_str( ) ) )
	{
		UTIL_LogPrint( ( gmapLANG_CFG["bnbt_mysql_error"] + "\n" ).c_str( ), mysql_error( gpMySQL ) );

		// Delete the PID file
		if( !gstrPID.empty( ) )
		{
			if( gbDebug )
				if( gucDebugLevel & DEBUG_BNBT )
					UTIL_LogPrint( "Deleting the PID file\n" );

			UTIL_DeleteFile( gstrPID.c_str( ) );
		}
		
		// Close the log
		if( gpLog )
		{
			if( gbDebug )
				if( gucDebugLevel & DEBUG_BNBT )
					UTIL_LogPrint( "Closing the log\n" );

			fclose( gpLog );
		}

		// Close the access log
		if( gpAccessLog )
		{
			if( gbDebug )
				if( gucDebugLevel & DEBUG_BNBT )
					UTIL_LogPrint( "Closing the access log\n" );

			fclose( gpAccessLog );
		}

		// Close the error log
		if( gpErrorLog )
		{
			if( gbDebug )
				if( gucDebugLevel & DEBUG_BNBT )
					UTIL_LogPrint( "Closing the error log\n" );

			fclose( gpErrorLog );
		}

		UTIL_LogPrint( "Tracker Stop\n" );

		gmtxOutput.Destroy( );

		return 1;
	}

	UTIL_MySQLCreateTables( );

	// Create the server
	if( gbDebug && ( gucDebugLevel & DEBUG_BNBT ) )
		UTIL_LogPrint( "Creating server\n" );

	gpServer = new CServer( );

	//
	// Link
	//

//	if( gbDebug && ( gucDebugLevel & DEBUG_BNBT ) )
//		UTIL_LogPrint( "Creating link client/server\n" );

//	gpLink = 0;
//	gpLinkServer = 0;

//	if( CFG_GetInt( "bnbt_tlink_server", 0 ) != 0 )
//		gpLinkServer = new CLinkServer( );
//	else
//	{
//		if( !CFG_GetString( "bnbt_tlink_connect", string( ) ).empty( ) )
//		{
//#ifdef WIN32
//            int iLinkThreadResult = (int)_beginthread( ( void (*)(void *) )StartLink, 0, 0 );
//            
//			if( iLinkThreadResult == -1 )
//				UTIL_LogPrint( ( gmapLANG_CFG["unable_to_spawn_link_thread_win32"] + "\n" ).c_str( ) );
//#else
//			pthread_t thread;

//			// set detached state since we don't need to join with any threads

//			pthread_attr_t attr;
//			pthread_attr_init( &attr );
//			pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );

//			int iLinkThreadResult = pthread_create( &thread, &attr, ( void * (*)(void *) )StartLink, 0 );

//			if( iLinkThreadResult != 0 )
//				UTIL_LogPrint( ( gmapLANG_CFG["unable_to_spawn_link_thread"] + "\n" ).c_str( ), strerror( iLinkThreadResult ) );
//#endif
//		}
//	}

	//
	// HUBLink
	//

//	if( gbDebug && ( gucDebugLevel & DEBUG_BNBT ) )
//		UTIL_LogPrint( "Creating hublink client/server\n" );

//	gpHUBLink = 0;
//	gpHUBLinkServer = 0;

//	if( CFG_GetInt( "xbnbt_thlink_server", 0 ) != 0 )
//		gpHUBLinkServer = new CHUBLinkServer( );
//	else
//	{
//		if( !CFG_GetString( "xbnbt_thlink_connect", string( ) ).empty( ) )
//		{
//#ifdef WIN32
//            int iHubLinkThreadResult = (int)_beginthread( ( void (*)(void *) )StartHUBLink, 0, 0 );
//            
//			if( iHubLinkThreadResult == -1 )
//				UTIL_LogPrint( ( gmapLANG_CFG["unable_to_spawn_hublink_thread_win32"] + "\n" ).c_str( ) );
//#else
//			pthread_t thread;

//			// set detached state since we don't need to join with any threads

//			pthread_attr_t attr;
//			pthread_attr_init( &attr );
//			pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );

//			int iHubLinkThreadResult = pthread_create( &thread, &attr, ( void * (*)(void *) )StartHUBLink, 0 );

//			if( iHubLinkThreadResult != 0 )
//				UTIL_LogPrint( ( gmapLANG_CFG["unable_to_spawn_hublink_thread"] + "\n" ).c_str( ), strerror( iHubLinkThreadResult ) );
//#endif
//		}
//	}

	//
	// update loop
	//

	if( gbDebug && ( gucDebugLevel & DEBUG_BNBT ) )
		UTIL_LogPrint( "Entering update loop\n" );

	bool bDoUpdate = true;

	while( bDoUpdate )
	{
		if( gbDebug && ( gucDebugLevel & DEBUG_LOOPS ) && gpServer )
			UTIL_LogPrint( "Updating server\n" );

		if( gpServer && gpServer->Update( true ) )
		{
			if( gbDebug && ( gucDebugLevel & DEBUG_BNBT ) )
				UTIL_LogPrint( "Deleting server\n" );

			delete gpServer;

			gpServer = 0;	 

//			if( gpLinkServer )
//			{
//				if( gbDebug && ( gucDebugLevel & DEBUG_BNBT ) )
//					UTIL_LogPrint( "Deleting link server\n" );

//				delete gpLinkServer;

//				gpLinkServer = 0;
//			}

//			if( gpHUBLinkServer )
//			{
//				if( gbDebug && ( gucDebugLevel & DEBUG_BNBT ) )
//					UTIL_LogPrint( "Deleting hublink server\n" );

//				delete gpHUBLinkServer;

//				gpHUBLinkServer = 0;
//			}

			if( gbDebug && ( gucDebugLevel & DEBUG_BNBT ) )
				UTIL_LogPrint( "Exit the update loop\n" );

			bDoUpdate = false;
		}
		else
		if( gpServer && guiRestartServerInterval > 0 && GetTime( ) > gulRestartServerNext )
		{
			if( gbDebug && ( gucDebugLevel & DEBUG_BNBT ) )
				UTIL_LogPrint( "Restarting server\n" );

			delete gpServer;

			gpServer = new CServer( );
			
			gulRestartServerNext = GetTime( ) + guiRestartServerInterval;
		}

//		if( gpLinkServer && gucDebugLevel != 0 )
//		{
//			if( gbDebug && ( gucDebugLevel & DEBUG_LOOPS ) )
//				UTIL_LogPrint( "Updating link server\n" );

//			gpLinkServer->Update( );
//		}

//		if( gpHUBLinkServer && gucDebugLevel != 0 )
//		{
//			if( gbDebug && ( gucDebugLevel & DEBUG_LOOPS ) )
//				UTIL_LogPrint( "Updating hublink server\n" );

//			gpHUBLinkServer->Update( );
//		}
	}

	//
	// wait for the link or it might make a big mess
	//

//	if( gpLink )
//	{
//		if( gbDebug && ( gucDebugLevel & DEBUG_BNBT ) )
//			UTIL_LogPrint( "Killing link client\n" );
//		
//		gpLink->Kill( );
//	}

//	const unsigned long culStart( GetTime( ) );

//	while( gpLink )
//	{
//		UTIL_LogPrint( ( gmapLANG_CFG["wait_link_disconnect"] + "\n" ).c_str( ) );

//		MILLISLEEP( 1000 );

//		if( GetTime( ) - culStart > 60 )
//		{
//			UTIL_LogPrint( ( gmapLANG_CFG["waited_link_disconnect"] + "\n" ).c_str( ) );

//			break;
//		}
//	}

	//
	// wait for the hub link or it might make a big mess
	//

//	if( gpHUBLink )
//	{
//		if( gbDebug && ( gucDebugLevel & DEBUG_BNBT ) )
//			UTIL_LogPrint( "Killing hublink client\n" );

//		gpHUBLink->Kill( );
//	}

//	const unsigned long culStartHUB( GetTime( ) );

//	while( gpHUBLink )
//	{
//		UTIL_LogPrint( ( gmapLANG_CFG["wait_hublink_disconnect"] + "\n" ).c_str( ) );

//		MILLISLEEP( 1000 );

//		if( GetTime( ) - culStartHUB > 60 )
//		{
//			UTIL_LogPrint( ( gmapLANG_CFG["waited_hublink_disconnect"] + "\n" ).c_str( ) );

//			break;
//		}
//	}

	// Close the BNBT MySQL database
	if( gpMySQL )
	{
		if( gbDebug && ( gucDebugLevel & DEBUG_BNBT ) )
			UTIL_LogPrint( "Closing MySQL dstate integration build\n" );

		mysql_close( gpMySQL );
	}

#ifdef WIN32
	// Exit windows sockets
	if( gbDebug && ( gucDebugLevel & DEBUG_BNBT ) )
		UTIL_LogPrint( "Closing the windows sockets\n" );

	WSACleanup( );
#endif

	// Delete the PID file
	if( !gstrPID.empty( ) )
	{
		if( gbDebug && ( gucDebugLevel & DEBUG_BNBT ) )
			UTIL_LogPrint( "Deleting the PID file\n" );

		UTIL_DeleteFile( gstrPID.c_str( ) );
	}
	
	// Close the log
	if( gpLog )
	{
		if( gbDebug && ( gucDebugLevel & DEBUG_BNBT ) )
			UTIL_LogPrint( "Closing the log\n" );

		fclose( gpLog );
	}

	// Close the access log
	if( gpAccessLog )
	{
		if( gbDebug && ( gucDebugLevel & DEBUG_BNBT ) )
			UTIL_LogPrint( "Closing the access log\n" );

		fclose( gpAccessLog );
	}

	// Close the error log
	if( gpErrorLog )
	{
	 	if( gbDebug && ( gucDebugLevel & DEBUG_BNBT ) )
			UTIL_LogPrint( "Closing the error log\n" );

		fclose( gpErrorLog );
	}

	UTIL_LogPrint( "Tracker Stop\n" );

	gmtxOutput.Destroy( );

	return 0;
}
