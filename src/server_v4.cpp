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

#include "bnbt.h"
#include "atom.h"
#include "client.h"
#include "config.h"
#include "server.h"
#include "tracker.h"
#include "util.h"

#ifdef WIN32
 #include "util_ntservice.h"
#endif

CServer :: CServer( )
{
	m_vecClients.reserve( guiMaxConns );

	m_bKill = false;

	m_uiSocketTimeOut = CFG_GetInt( "socket_timeout", 15 );
	m_strBind = CFG_GetString( "bind", string( ) );
	m_cCompression = (char)CFG_GetInt( "bnbt_compression_level", 6 );

	// clamp compression

	if( m_cCompression > 9 )
		m_cCompression = 9;

	struct sockaddr_in sin;

	memset( &sin, 0, sizeof( sin ) );

	sin.sin_family = AF_INET;

	if( !m_strBind.empty( ) )
	{
		// bind to m_strBind

		if( gbDebug && ( gucDebugLevel & DEBUG_SERVER ) )
			UTIL_LogPrint( "server - binding to %s\n", m_strBind.c_str( ) );

		sin.sin_addr.s_addr = inet_addr( m_strBind.c_str( ) );

		if( sin.sin_addr.s_addr == INADDR_NONE || sin.sin_addr.s_addr == 0 )
		{
			UTIL_LogPrint( ( gmapLANG_CFG["unable_to_bind"] + "\n" ).c_str( ), m_strBind.c_str( ) );

			exit( 1 );
		}
	}
	else
	{
		// bind to all available addresses

		if( gbDebug && ( gucDebugLevel & DEBUG_SERVER ) )
			UTIL_LogPrint( ( gmapLANG_CFG["binding_to_all"] + "\n" ).c_str( ) );

		sin.sin_addr.s_addr = INADDR_ANY;
	}

	// tphogan - legacy support, check for "port" config entry
	// by doing this here "port" will always be required
	// so in fact this isn't entirely legacy support since it'll continue to be used

	sin.sin_port = htons( (u_short)CFG_GetInt( "port", 6969 ) ) ;

	if( sin.sin_port == 0 )
		UTIL_LogPrint( ( gmapLANG_CFG["binding_to_all"] + "\n" ).c_str( ), CAtomInt( CFG_GetInt( "port", 6969 ) ).toString( ).c_str( ) );
	else if( !AddListener( sin ) )
		UTIL_LogPrint( ( gmapLANG_CFG["unable_to_listen"] + "\n" ).c_str( ), CAtomInt( CFG_GetInt( "port", 6969 ) ).toString( ).c_str( ) );
	else
		UTIL_LogPrint( ( gmapLANG_CFG["listen_on_port"] + "\n" ).c_str( ), CAtomInt( CFG_GetInt( "port", 6969 ) ).toString( ).c_str( ) );

	// tphogan - retrieve list of ports from config file for multiport listeners
	// do we want to support multiple bindings as well?
	// this code will bind every socket to the same address

	unsigned char ucPort = 1;

	string strName = "port" + CAtomInt( ucPort ).toString( );
	string strPort = CFG_GetString( strName, string( ) );

	while( !strPort.empty( ) )
	{
		sin.sin_port = htons( (u_short)atoi( strPort.c_str( ) ) ) ;

		if( sin.sin_port == 0 )
			UTIL_LogPrint( ( gmapLANG_CFG["invalid_ports"] + "\n" ).c_str( ), strPort.c_str( ), strName.c_str( ) );
		else if( !AddListener( sin ) )
			UTIL_LogPrint( ( gmapLANG_CFG["unable_to_listens"] + "\n" ).c_str( ), strPort.c_str( ), strName.c_str( ) );
		else
			UTIL_LogPrint( ( gmapLANG_CFG["listen_on_ports"] + "\n" ).c_str( ), strPort.c_str( ), strName.c_str( ) );

		strName = "port" + CAtomInt( ++ucPort ).toString( );
		strPort = CFG_GetString( strName, string( ) );
	}

	// tphogan - we didn't exit on invalid ports above
	// so make sure we're listening on at least one valid port
	// however, since "port" is always forced greater than zero in CFG_SetDefaults
	// this should only happen if a valid port is denied for "port"
	// for example, address in use or not enough privs for ports < 1024

	if( m_vecListeners.empty( ) )
	{
		UTIL_LogPrint( ( gmapLANG_CFG["not_listening"] + "\n" ).c_str( ) );

		exit( 1 );
	}

	m_pTracker = new CTracker( );

	UTIL_LogPrint( ( gmapLANG_CFG["server_start"] + "\n" ).c_str( ) );
}

CServer :: ~CServer( )
{
	for( vector<SOCKET> :: iterator itListener = m_vecListeners.begin( ); itListener != m_vecListeners.end( ); itListener++ )
		closesocket( *itListener );

	for( vector<CClient *> :: iterator itClient = m_vecClients.begin( ); itClient != m_vecClients.end( ); itClient++ )
		delete *itClient;

	m_vecListeners.clear( );
	m_vecClients.clear( );

	if( m_pTracker )
		delete m_pTracker;

	m_pTracker = 0;

	UTIL_LogPrint( ( gmapLANG_CFG["server_exit"] + "\n" ).c_str( ) );
}

void CServer :: Kill( )
{
	m_bKill = true;
}

bool CServer :: isDying( )
{
	return m_bKill;
}

bool CServer :: Update( const bool &bBlock )
{
	if( m_vecClients.size( ) < guiMaxConns )
	{
		// tphogan - check every listener for new connections
		
		fd_set fdServer;
		FD_ZERO( &fdServer );
		
		struct timeval tv;
		struct sockaddr_in adrFrom;
		int iAddrLen = 0;
		SOCKET sckClient = 0;

		for( vector<SOCKET> :: iterator itListener = m_vecListeners.begin( ); itListener != m_vecListeners.end( ); itListener++ )
		{
			FD_CLR( *itListener, &fdServer );
			FD_SET( *itListener, &fdServer );

			// tphogan - only block on the first listener
			// this is actually a bit of a hack but it don't feel like doing it "right" :)

			if( bBlock && itListener == m_vecListeners.begin( ) )
			{
				// block for 100 ms to keep from eating up all cpu time

				tv.tv_sec = 0;
				tv.tv_usec = 100000;
			}
			else
			{
				tv.tv_sec = 0;
				tv.tv_usec = 0;
			}

#ifdef WIN32
			if( select( 1, &fdServer, 0, 0, &tv ) == SOCKET_ERROR )
#else
			if( select( *itListener + 1, &fdServer, 0, 0, &tv ) == SOCKET_ERROR )
#endif
			{
				UTIL_LogPrint( ( gmapLANG_CFG["select_error"] + "\n" ).c_str( ), GetLastErrorString( ) );

				FD_CLR( *itListener, &fdServer );
			}

			if( FD_ISSET( *itListener, &fdServer ) !=0 )
			{
				iAddrLen = sizeof( adrFrom );

#ifdef WIN32
				sckClient = accept( *itListener, (struct sockaddr *)&adrFrom, &iAddrLen );

				if( sckClient == INVALID_SOCKET )
#else
				sckClient = accept( *itListener, (struct sockaddr *)&adrFrom, (socklen_t *)&iAddrLen );

				if( sckClient == INVALID_SOCKET )
#endif
					UTIL_LogPrint( ( gmapLANG_CFG["accept_error"] + "\n" ).c_str( ), GetLastErrorString( ) );
				else
					m_vecClients.push_back( new CClient( sckClient, adrFrom, m_uiSocketTimeOut, m_cCompression ) );
			}
		}
	}
	else
	{
		// maximum connections reached

		// tphogan - reduced from 100 ms to 10 ms
		// it's very difficult to tell if the backlog is due to legitimate load or hung clients
		// hung clients don't eat CPU time so the server's CPU usage will skyrocket
		// but if it's due to load then sleeping for 100 ms is a terrible idea!
		// someone should take a look at this and rewrite it eventually

		UTIL_LogPrint( "Server Info - Max. connections reached\n" );

		MILLISLEEP( 10 );
	}

	// process the clients
	for( vector<CClient *> :: iterator itClient = m_vecClients.begin( ); itClient != m_vecClients.end( ); )
	{
		if( (*itClient)->Update( ) )
		{
			delete *itClient;

			itClient = m_vecClients.erase( itClient );
		}
		else
			itClient++;
	}

	if( m_pTracker )
		m_pTracker->Update( );

	return m_bKill;
}

CTracker *CServer :: getTracker( )
{
	return m_pTracker;   
}

bool CServer :: AddListener( struct sockaddr_in sin )
{
	SOCKET sckListener;

	// map protocol name to protocol number

	struct protoent *pPE;

	pPE = getprotobyname( "tcp" );

	if( pPE == 0 )
	{
		UTIL_LogPrint( ( gmapLANG_CFG["no_tcp_protocol"] + "\n" ).c_str( ), GetLastErrorString( ) );

		return false;
	}

	// allocate socket

	sckListener = socket( PF_INET, SOCK_STREAM, pPE->p_proto );

	if( sckListener == INVALID_SOCKET )
	{
		UTIL_LogPrint( ( gmapLANG_CFG["not_allocated_socket"] + "\n" ).c_str( ), GetLastErrorString( ) );

		return false;
	}

#ifdef WIN32
	const unsigned int cuiOptVal( 1 );

	// TCP window size 
	// Send
	if( setsockopt( sckListener, SOL_SOCKET, SO_SNDBUF, (const char *)&guiSO_SNDBUF, sizeof(guiSO_SNDBUF) ) == SOCKET_ERROR )
		UTIL_LogPrint( ( gmapLANG_CFG["no_sndbuf"] + "\n" ).c_str( ), GetLastErrorString( ) );

	// Receive
	if( setsockopt( sckListener, SOL_SOCKET, SO_RCVBUF, (const char *)&guiSO_RECBUF, sizeof(guiSO_RECBUF) ) == SOCKET_ERROR )
		UTIL_LogPrint( ( gmapLANG_CFG["no_rcvbuf"] + "\n" ).c_str( ), GetLastErrorString( ) );

	// 	Allows the socket to be bound to an address that is already in use.
	if( setsockopt( sckListener, SOL_SOCKET, SO_REUSEADDR, (const char *)&cuiOptVal, sizeof( cuiOptVal ) ) == SOCKET_ERROR )
		UTIL_LogPrint( ( gmapLANG_CFG["no_reuseaddr"] + "\n" ).c_str( ), GetLastErrorString( ) );

	// Naggle's Algorithm
	if( setsockopt( sckListener, SOL_SOCKET, TCP_NODELAY, (const char *)&gbTCP_NODELAY, sizeof( int ) ) == SOCKET_ERROR )
		UTIL_LogPrint( ( gmapLANG_CFG["no_nodelay"] + "\n" ).c_str( ), GetLastErrorString( ) );

	// bind socket
	if( bind( sckListener, (SOCKADDR*)&sin, sizeof( sin ) ) == SOCKET_ERROR )
	{
		UTIL_LogPrint( ( gmapLANG_CFG["unable_to_bind_socket"] + "\n" ).c_str( ), GetLastErrorString( ) );

		return false;
	}
#else
	const unsigned int cuiOptVal( 1 );

#ifdef SO_NOSIGPIPE
	// Ignore SIGPIPE - FreeBSD
	if( setsockopt( sckListener, SOL_SOCKET, SO_NOSIGPIPE, (const void *)&cuiOptVal, (socklen_t)sizeof( cuiOptVal ) ) == SOCKET_ERROR )
		UTIL_LogPrint( ( gmapLANG_CFG["no_nosigpipe"] + "\n" ).c_str( ), GetLastErrorString( ) );
#endif

#ifdef SO_SNDBUF
	// TCP window size Send
	if( setsockopt( sckListener, SOL_SOCKET, SO_SNDBUF, (const char *)&guiSO_SNDBUF, (socklen_t)sizeof( guiSO_SNDBUF ) ) == SOCKET_ERROR )
		UTIL_LogPrint( ( gmapLANG_CFG["no_sndbuf"] + "\n" ).c_str( ), GetLastErrorString( ) );
#endif

#ifdef SO_RCVBUF
	// TCP window size Receive
	if( setsockopt( sckListener, SOL_SOCKET, SO_RCVBUF, (const char *)&guiSO_RECBUF, (socklen_t)sizeof( guiSO_RECBUF ) ) == SOCKET_ERROR )
		UTIL_LogPrint( ( gmapLANG_CFG["no_rcvbuf"] + "\n" ).c_str( ), GetLastErrorString( ) );
#endif

#ifdef SO_REUSEADDR
	// Allows the socket to be bound to an address that is already in use.
	if( setsockopt( sckListener, SOL_SOCKET, SO_REUSEADDR, (const void *)&cuiOptVal, (socklen_t)sizeof( cuiOptVal ) ) == SOCKET_ERROR );
		UTIL_LogPrint( ( gmapLANG_CFG["no_reuseaddr"] + "\n" ).c_str( ), GetLastErrorString( ) );
#endif

#ifdef TCP_NODELAY
	// Nagle's Algorithm
	if( setsockopt( sckListener, SOL_SOCKET, TCP_NODELAY, (const char *)&gbTCP_NODELAY, (socklen_t)sizeof( int ) ) == SOCKET_ERROR )
		UTIL_LogPrint( ( gmapLANG_CFG["no_nodelay"] + "\n" ).c_str( ), GetLastErrorString( ) );
#endif

	// bind
	if( bind( sckListener, (struct sockaddr *)&sin, sizeof( sin ) ) == SOCKET_ERROR )
	{
		UTIL_LogPrint( ( gmapLANG_CFG["unable_to_bind_socket"] + "\n" ).c_str( ), GetLastErrorString( ) );

		return false;
	}
#endif

	// listen on the socket
	if( listen( sckListener, 1 ) == SOCKET_ERROR )
	{
		UTIL_LogPrint( ( gmapLANG_CFG["unable_to_listen"] + "\n" ).c_str( ), GetLastErrorString( ) );

		return false;
	}

	// Add the socket to the vector of sockets
	m_vecListeners.push_back( sckListener );

	return true;
}
