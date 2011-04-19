// // Copyright (C) 2003-2004 Trevor Hogan
//

#ifndef BNBT_H
 #define BNBT_H

// XBNBT
#define XBNBT_VER string("XBNBT 81b.3.5 ZiJing Mod")

#if defined ( WIN32 )
 #define FD_SETSIZE 256

 #include <process.h>
 #include <winsock2.h>
#else
 #include <stdio.h>
 #include <arpa/inet.h>
 #include <errno.h>
 #include <netdb.h>
 #include <netinet/tcp.h>
 #include <netinet/in.h>
 #include <netinet/ip6.h>
 #include <netinet/icmp6.h>
 #include <pthread.h>
 #include <sys/socket.h>
 #include <unistd.h>
#endif

// tphogan - these new includes may have broken BSD support

#include <cstring>
#include <iostream>
#include <algorithm>
#include <map>
#include <string>
#include <vector>
#include <utility>

// tphogan - eliminate namespace pollution

using std :: endl;
using std :: ifstream;
using std :: ofstream;
using std :: map;
using std :: multimap;
using std :: pair;
using std :: string;
using std :: vector;

#ifdef WIN32
 #define WIN32_LEAN_AND_MEAN
#endif

#ifdef __GNUWIN32__
 #define unlink remove
#endif

//
// SOLARIS USERS - IF YOUR SYSTEM IS LITTLE ENDIAN, REMOVE THE NEXT 3 LINES
//  also see sha1.h
//

#if defined( __APPLE__ ) || defined( __SOLARIS__ )
 #define BNBT_BIG_ENDIAN
#endif

// large integers

#ifdef WIN32
 typedef unsigned __int8 uint8;
 typedef __int64 int64;
 typedef unsigned __int64 uint64;
#else
 typedef unsigned char uint8;
 typedef long long int64;
 typedef unsigned long long uint64;
 typedef unsigned long int DWORD;
 typedef unsigned short int WORD;
 typedef unsigned char BYTE;
#endif

#ifdef WIN32
 #define snprintf _snprintf
 #define vsnprintf _vsnprintf
#endif

// mutex

class CMutex
{
public:
#ifdef WIN32
	void Initialize( ) { InitializeCriticalSection( &cs ); }
	void Destroy( ) { DeleteCriticalSection( &cs ); }
	void Claim( ) { EnterCriticalSection( &cs ); }
	void Release( ) { LeaveCriticalSection( &cs ); }

	CRITICAL_SECTION cs;
#else
	void Initialize( ) { pthread_mutex_init( &mtx, 0 ); }
	void Destroy( ) { pthread_mutex_destroy( &mtx ); }
	void Claim( ) { pthread_mutex_lock( &mtx ); }
	void Release( ) { pthread_mutex_unlock( &mtx ); }

	pthread_mutex_t mtx;
#endif
};				   

// stl

#ifdef WIN32
 #pragma warning( disable : 4786 )
#endif

// path seperator

#ifdef WIN32
 #define PATH_SEP '\\'
 #define STR_PATH_SEP string( 1,PATH_SEP )
#else
 #define PATH_SEP '/'
 #define STR_PATH_SEP string( 1,PATH_SEP )
#endif

#define CHAR_BS '\\'
#define CHAR_FS '/'

// Termination character character

#define TERM_CHAR '\0'

// this fixes MSVC loop scoping issues

/*

#ifdef WIN32
 #define for if( 0 ) { } else for
#endif

*/

// time stuff

unsigned long GetTime( );
unsigned long GetStartTime( );

#ifdef WIN32
 #define MILLISLEEP( x ) Sleep( x )
#else
 #define MILLISLEEP( x ) usleep( ( x ) * 1000 )
#endif

// network

#ifdef WIN32
 #define EADDRINUSE WSAEADDRINUSE
 #define EADDRNOTAVAIL WSAEADDRNOTAVAIL
 #define EAFNOSUPPORT WSAEAFNOSUPPORT
 #define EALREADY WSAEALREADY
 #define ECONNABORTED WSAECONNABORTED
 #define ECONNREFUSED WSAECONNREFUSED
 #define ECONNRESET WSAECONNRESET
 #define EDESTADDRREQ WSAEDESTADDRREQ
 #define EDQUOT WSAEDQUOT
 #define EHOSTDOWN WSAEHOSTDOWN
 #define EHOSTUNREACH WSAEHOSTUNREACH
 #define EINPROGRESS WSAEINPROGRESS
 #define EISCONN WSAEISCONN
 #define ELOOP WSAELOOP
 #define EMSGSIZE WSAEMSGSIZE
 // defined in Windows
 //#define ENAMETOOLONG WSAENAMETOOLONG
 #define ENETDOWN WSAENETDOWN
 #define ENETRESET WSAENETRESET
 #define ENETUNREACH WSAENETUNREACH
 #define ENOBUFS WSAENOBUFS
 #define ENOPROTOOPT WSAENOPROTOOPT
 #define ENOTCONN WSAENOTCONN
 // defined in Windows
 //#define ENOTEMPTY WSAENOTEMPTY
 #define ENOTSOCK WSAENOTSOCK
 #define EOPNOTSUPP WSAEOPNOTSUPP
 #define EPFNOSUPPORT WSAEPFNOSUPPORT
 #define EPROTONOSUPPORT WSAEPROTONOSUPPORT
 #define EPROTOTYPE WSAEPROTOTYPE
 #define EREMOTE WSAEREMOTE
 #define ESHUTDOWN WSAESHUTDOWN
 #define ESOCKTNOSUPPORT WSAESOCKTNOSUPPORT
 #define ESTALE WSAESTALE
 #define ETIMEDOUT WSAETIMEDOUT
 #define ETOOMANYREFS WSAETOOMANYREFS
 #define EUSERS WSAEUSERS
 #define EWOULDBLOCK WSAEWOULDBLOCK
 // defined in Windows
 //#define EFAULT WSAEFAULT
 #define ENETDOWN WSAENETDOWN
 // defined in Windows
 //#define EINTR WSAEINTR
 //#define EBADF WSAEBADF
 //#define EACCES WSAEACCES
 //#define EINVAL WSAEINVAL
 //#define EMFILE WSAEMFILE
 #define EPROCLIM WSAEPROCLIM
 #define SYSNOTREADY WSASYSNOTREADY
 #define VERNOTSUPPORTED WSAVERNOTSUPPORTED
 #define NOTINITIALISED WSANOTINITIALISED
 #define EDISCON WSAEDISCON
 #define ENOMORE WSAENOMORE
 #define ECANCELLED WSAECANCELLED
 #define EINVALIDPROCTABLE WSAEINVALIDPROCTABLE
 #define EINVALIDPROVIDER WSAEINVALIDPROVIDER
 #define EPROVIDERFAILEDINIT WSAEPROVIDERFAILEDINIT
 #define SYSCALLFAILURE WSASYSCALLFAILURE
 #define SERVICE_NOT_FOUND WSASERVICE_NOT_FOUND
 #define TYPE_NOT_FOUND WSATYPE_NOT_FOUND
 #define _E_NO_MORE WSA_E_NO_MORE
 #define _E_CANCELLED WSA_E_CANCELLED
 #define EREFUSED WSAEREFUSED
 #define HOST_NOT_FOUND WSAHOST_NOT_FOUND
 #define TRY_AGAIN WSATRY_AGAIN
 #define NO_RECOVERY WSANO_RECOVERY
 #define NO_DATA WSANO_DATA
 // defined in Windows
 //#define NO_ADDRESS WSANO_DATA
#endif

#ifndef WIN32
 #define closesocket close
 typedef int SOCKET;
 extern int GetLastError( );
#endif

extern const char *GetLastErrorString( );

#ifdef __APPLE__
 typedef int socklen_t;
 typedef int sockopt_len_t;
#endif

#ifndef INVALID_SOCKET
 #define INVALID_SOCKET -1
#endif

#ifndef SOCKET_ERROR
 #define SOCKET_ERROR -1
#endif

#ifndef INADDR_NONE
 #define INADDR_NONE -1
#endif

#ifndef MSG_NOSIGNAL
 #define MSG_NOSIGNAL 0
#endif

#define FILE_ERROR 0
#define DIR_ERROR -1

class CAtom;
class CAtomInt;
class CAtomLong;
class CAtomString;
class CAtomList;
class CAtomDicti;

class CServer;
class CTracker;
class CClient;

class CLink;
class CLinkServer;

class CHUBLink;
class CHUBLinkServer;

struct response_t
{
	string strCode;
	multimap<string, string> mapHeaders;
	string strContent;
	bool bCompressOK;
};

// user access levels

#define ACCESS_VIEW				( 1 << 0 )		// 1
#define ACCESS_DL				( 1 << 1 )		// 2
#define ACCESS_COMMENTS			( 1 << 2 )		// 4
#define ACCESS_UPLOAD			( 1 << 3 )		// 8
#define ACCESS_EDIT				( 1 << 4 )		// 16
#define ACCESS_ADMIN			( 1 << 5 )		// 32
#define ACCESS_LEADER			( 1 << 6 )		// 64

// user group

#define GROUP_FRIENDS			( 1 << 0 )		// 1
#define GROUP_RETIRED			( 1 << 1 )		// 2
#define GROUP_VIP				( 1 << 2 )		// 4

// debug levels

#define DEBUG_BNBT				( 1 << 0 )		// 1
#define DEBUG_SERVER			( 1 << 1 )		// 2
#define DEBUG_CLIENT				( 1 << 2 )		// 4
#define DEBUG_TRACKER			( 1 << 3 )		// 8
#define DEBUG_ANNOUNCE			( 1 << 4 )		// 16
#define DEBUG_SCRAPE			( 1 << 5 )		// 32
#define DEBUG_LOOPS				( 1 << 6 )		// 64

struct user_t
{
	string strLogin;
	string strLowerLogin;
	string strMD5;
	string strPasskey;
	string strMail;
	string strLowerMail;
	string strCreated;
	string strIP;
	unsigned char ucAccess;
	unsigned char ucGroup;
	string strUID;
	time_t tLast;
	time_t tLast_Index;
	int64 ulSeeding;
	int64 ulLeeching;
	int64 ulUploaded;
	int64 ulDownloaded;
	float flShareRatio;
	int64 ulBonus;
	float flSeedBonus;
	time_t tWarned;
	string strInvites;
// 	unsigned int uiInvites;
	string strInviter;
	string strInviterID;
};

struct request_t
{
//	struct sockaddr_in sin;
	struct sockaddr_in6 sin;
	string strMethod;
	string strURL;
	// Harold - Adding Support for Multiple scrapes in a single /scrape call
	bool hasQuery;
	multimap<string, string> multiParams;
	map<string, string> mapParams;
	map<string, string> mapHeaders;
	map<string, string> mapCookies;
	struct user_t user;
};

#define LINK_VER			"TrackerLINK Ver. 0.1"

#define LINKMSG_ERROR		-1
#define LINKMSG_NONE		0		// not transmitted
#define LINKMSG_VERSION		1
#define LINKMSG_INFO		2
#define LINKMSG_PASSWORD	3
#define LINKMSG_READY		4
#define LINKMSG_ANNOUNCE	7
#define LINKMSG_CLOSE		99

struct linkmsg_t
{
	long len;
	char type;
	string msg;
};

// current version

#define BNBT_VER "Beta 8.1"

// Trinity edition
/*
#ifdef WIN32
 #define BNBT_SERVICE_NAME "BNBT Service"
#endif
*/

extern CServer *gpServer;	 
extern CLink *gpLink;
extern CLinkServer *gpLinkServer;
extern CHUBLink *gpHUBLink;
extern CHUBLinkServer *gpHUBLinkServer;
extern string gstrLogDir;
extern string gstrLogFile;
extern string gstrLogFilePattern;
extern FILE *gpLog;
extern string gstrErrorLogDir;
extern string gstrErrorLogFile;
extern string gstrErrorLogFilePattern;
extern FILE *gpErrorLog;
extern string gstrAccessLogDir;
extern string gstrAccessLogFilePattern;
extern string gstrAccessLogFile;
extern FILE *gpAccessLog;
extern unsigned long gulLogCount;
extern unsigned long gulErrorLogCount;
extern unsigned long gulAccessLogCount;
extern unsigned int guiFlushInterval;
extern bool gbDebug;
extern unsigned long gulRestartServerNext;
extern unsigned int guiRestartServerInterval;
extern unsigned int guiMaxConns;
extern unsigned int guiMaxRecvSize;
extern string gstrStyle;
extern string gstrCharSet;
extern string gstrRealm;

// The Trinity Edition - Modification Begins
// Sets the NT Service Name variable as a global variable
extern string gstrNTServiceName;

// Threads
extern CMutex gmtxOutput;

// TCP window size
extern unsigned int guiSO_RECBUF;
extern unsigned int guiSO_SNDBUF;

// Nagle
extern bool gbTCP_NODELAY;

// PID file
extern string gstrPID;

// Debug level
extern unsigned char gucDebugLevel;

// Mime types
extern map<string, string> gmapMime;

// XBNBT XStats
extern struct xbnbtstats_t gtXStats;

// this is basically the old main( ), but it's here to make the NT Service code neater

extern int bnbtmain( );

#endif
