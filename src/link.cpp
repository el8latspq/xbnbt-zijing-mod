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

#include "bnbt.h"
#include "atom.h"
#include "bencode.h"
#include "config.h"
#include "link.h"
#include "md5.h"
#include "server.h"
#include "tracker.h"
#include "util.h"

//
// CLink
//

CLink :: CLink( )
{
	m_mtxQueued.Initialize( );

	m_bKill = false;

	m_strIP = CFG_GetString( "bnbt_tlink_connect", string( ) );
	m_strPass = CFG_GetString( "bnbt_tlink_password", string( ) );

	memset( &sin, 0, sizeof( sin ) );

	sin.sin6_family = AF_INET6;

	// map host name

	struct hostent *pHE;

	pHE = gethostbyname( m_strIP.c_str( ) );
	
	inet_pton( AF_INET6, m_strIP.c_str( ), &sin.sin6_addr );
	
	if( pHE )
		memcpy( &sin.sin6_addr, pHE->h_addr, pHE->h_length );
	else if( inet_pton( AF_INET6, m_strIP.c_str( ), &sin.sin6_addr ) == NULL )
	{
		UTIL_LogPrint( "link error (%s) - unable to get host entry\n", getName( ).c_str( ) );

		Kill( );
	}
	
	sin.sin6_port = htons( (u_short)CFG_GetInt( "bnbt_tlink_port", 5204 ) );
	
	if( sin.sin6_port == 0 )
	{
		UTIL_LogPrint( "link error (%s) - invalid port %d\n", getName( ).c_str( ), CFG_GetInt( "bnbt_tlink_port", 5204 ) );

		Kill( );
	}

	m_sckLink = INVALID_SOCKET;
}

CLink :: ~CLink( )
{
	linkmsg_t lmClose;

	lmClose.len = 0;
	lmClose.type = LINKMSG_CLOSE;
	lmClose.msg.erase( );

	Send( lmClose );

	int iReturnShutdown = shutdown( m_sckLink, 2 );

	if( iReturnShutdown == SOCKET_ERROR )
		UTIL_LogPrint( "link error - Shutdown Socket (%s)\n", GetLastErrorString( ) );
	
	iReturnShutdown = closesocket( m_sckLink );
	
	if( iReturnShutdown == SOCKET_ERROR )
		UTIL_LogPrint( "link error - Closing Socket (%s)\n", GetLastErrorString( ) );
	else
	{
		if( gbDebug )
			UTIL_LogPrint( "link - Socket Closed\n" );
	}

	m_mtxQueued.Destroy( );

	UTIL_LogPrint( "link (%s) - link broken\n", getName( ).c_str( ) );
}

void CLink :: Kill( )
{
	m_bKill = true;
}

void CLink :: Go( )
{
	if( m_bKill )
		return;

	bool bNoWarning = true;

	// map protocol name to protocol number

	struct protoent *pPE;

	pPE = getprotobyname( "tcp" );
	
	if( pPE == 0 )
	{
		UTIL_LogPrint( "link error (%s) - unable to get tcp protocol entry (error %s)\n", getName( ).c_str( ), GetLastErrorString( ) );

		return;
	}

	// allocate socket

	m_sckLink = socket( PF_INET6, SOCK_STREAM, pPE->p_proto );
	
	if( m_sckLink == INVALID_SOCKET )
	{
		UTIL_LogPrint( "link error (%s) - unable to allocate socket (error %s)\n", getName( ).c_str( ), GetLastErrorString( ) );

		return;
	}

	// connect socket

	if( connect( m_sckLink, (struct sockaddr *)&sin, sizeof( sin ) ) == SOCKET_ERROR )
	{
		UTIL_LogPrint( "link error (%s) - unable to connect (error %s)\n", getName( ).c_str( ), GetLastErrorString( ) );

		return;
	}

	UTIL_LogPrint( "link (%s) - link established\n", getName( ).c_str( ) );

	// Added by =Xotic=
	gtXStats.date.sLinkEstablished = UTIL_Date( );
	// --------------------------- End of Addition

	struct linkmsg_t lmSend;
	struct linkmsg_t lmReceive;

	lmSend.len = ( long )strlen( LINK_VER );
	lmSend.type = LINKMSG_VERSION;
	lmSend.msg = LINK_VER;

	Send( lmSend );

	lmReceive = Receive( true );

	if( lmReceive.type != LINKMSG_VERSION || lmReceive.msg != LINK_VER )
	{
		UTIL_LogPrint( "link error (%s) - incompatible version, disconnecting\n", getName( ).c_str( ) );

		return;
	}

	lmReceive = Receive( true );

	if( lmReceive.type != LINKMSG_INFO )
	{
		UTIL_LogPrint( "link error (%s) - unexpected message, disconnecting\n", getName( ).c_str( ) );

		return;
	}

	CAtom *pInfo = Decode( lmReceive.msg );

	if( pInfo && pInfo->isDicti( ) )
	{
		CAtom *pNonce = ( (CAtomDicti *)pInfo )->getItem( "nonce" );

		string strHashMe = string( );

		if( pNonce )
			strHashMe = m_strPass + ":" + pNonce->toString( );
		else
			strHashMe = m_strPass;

		unsigned char szMD5[16];
		memset( szMD5, 0, sizeof( szMD5 ) / sizeof( unsigned char ) );

		MD5_CTX md5;

		MD5Init( &md5 );
		MD5Update( &md5, (const unsigned char *)strHashMe.c_str( ), (unsigned int)strHashMe.size( ) );
		MD5Final( szMD5, &md5 );

		lmSend.len = sizeof( szMD5 );
		lmSend.type = LINKMSG_PASSWORD;
		lmSend.msg = string( (char *)szMD5, sizeof( szMD5 ) );

		Send( lmSend );

		delete pInfo;
	}
	else
	{
		UTIL_LogPrint( "link error (%s) - bad info message, disconnecting\n", getName( ).c_str( ) );

		return;
	}

	lmSend.len = 0;
	lmSend.type = LINKMSG_READY;
	lmSend.msg.erase( );

	Send( lmSend );

	UTIL_LogPrint( "link (%s) - ready\n", getName( ).c_str( ) );

	vector<struct linkmsg_t> vecTemp;
	vecTemp.reserve( 100 );

	CAtom *pParams = 0;

	CAtomDicti *pParamsDicti = 0;

	CAtom *pInfoHash = 0;
	CAtom *pIP = 0;
	CAtom *pEvent = 0;
	CAtom *pPort = 0;
	CAtom *pUploaded = 0;
	CAtom *pDownloaded = 0;
	CAtom *pLeft = 0;
	CAtom *pPeerID = 0;
	CAtom *pKey = 0;

	struct announce_t ann;
	
	while( bNoWarning )
	{
		if( m_bKill )
			return;

		// send

		m_mtxQueued.Claim( );
		vecTemp = m_vecQueued;
		m_vecQueued.clear( );
		m_mtxQueued.Release( );

		for( vector<struct linkmsg_t> :: iterator it = vecTemp.begin( ); it != vecTemp.end( ); it++ )
			Send( *it );

		// receive

		lmReceive = Receive( false );

		if( lmReceive.type == LINKMSG_ERROR || lmReceive.type == LINKMSG_NONE )
		{
			// ignore
		}
		else if( lmReceive.type == LINKMSG_ANNOUNCE )
		{
			pParams = Decode( lmReceive.msg );

			if( pParams && pParams->isDicti( ) )
			{
				pParamsDicti = (CAtomDicti *)pParams;

				pInfoHash = pParamsDicti->getItem( "info_hash" );
				pIP = pParamsDicti->getItem( "ip" );
				pEvent = pParamsDicti->getItem( "event" );
				pPort = pParamsDicti->getItem( "port" );
				pUploaded = pParamsDicti->getItem( "uploaded" );
				pDownloaded = pParamsDicti->getItem( "downloaded" );
				pLeft = pParamsDicti->getItem( "left" );
				pPeerID = pParamsDicti->getItem( "peer_id" );
				pKey = pParamsDicti->getItem( "key" );

				if( pInfoHash && pIP && pPort && pUploaded && pDownloaded && pLeft && pPeerID && pKey )
				{
					ann.strInfoHash = pInfoHash->toString( );
					ann.strIP = pIP->toString( );
					ann.uiPort = (unsigned int)( (CAtomLong *)pPort )->getValue( );
					ann.iUploaded = ( (CAtomLong *)pUploaded )->getValue( );
					ann.iDownloaded = ( (CAtomLong *)pDownloaded )->getValue( );
					ann.iLeft = ( (CAtomLong *)pLeft )->getValue( );
					ann.strPeerID = pPeerID->toString( );
					ann.strKey = pKey->toString( );

					// assume strEvent is legit

					if( pEvent )
						ann.strEvent = pEvent->toString( );

					gpServer->getTracker( )->QueueAnnounce( ann );
				}

				delete pParams;
			}
		}
		else if( lmReceive.type == LINKMSG_CLOSE )
		{
			UTIL_LogPrint( "link warning (%s) - other end closing connection\n", getName( ).c_str( ) );

			return;
		}
		else
			UTIL_LogPrint( "link warning (%s) - unexpected message %d\n", getName( ).c_str( ), lmReceive.type );
	}
}

void CLink :: Send( struct linkmsg_t lm )
{
	if( m_bKill )
		return;

	m_strSendBuf.erase( );
	m_strSendBuf += CAtomLong( lm.len ).toString( );
	m_strSendBuf += "|";
	m_strSendBuf += CAtomInt( lm.type ).toString( );
	m_strSendBuf += "|";
	m_strSendBuf += lm.msg;

	int iSend = 0;

	while( !m_strSendBuf.empty( ) )
	{
		iSend = send( m_sckLink, m_strSendBuf.c_str( ), ( int )m_strSendBuf.size( ), MSG_NOSIGNAL );

		if( iSend == SOCKET_ERROR )
		{
			UTIL_LogPrint( "link error (%s) - send error (error %s)\n", getName( ).c_str( ), GetLastErrorString( ) );

			Kill( );

			return;
		}
		else if( iSend == 0 )
		{
			UTIL_LogPrint( "link error (%s) - send error (error %s)\n", getName( ).c_str( ), GetLastErrorString( ) );

			Kill( );

			return;
		}
		else if( iSend > 0 )
		{
			m_strSendBuf = m_strSendBuf.substr( iSend );

			gtXStats.tcp.iSendLink += iSend;
		}
	}
}

struct linkmsg_t CLink :: Receive( bool bBlock )
{
	bool bNoWarning = true;

	struct linkmsg_t lm;

	lm.len = 0;
	lm.type = LINKMSG_NONE;

	if( m_bKill )
		return lm;

	char pTemp[16384];

	if( bBlock )
	{
		int iRecv = 0;

		while( bNoWarning )
		{
			memset( pTemp, 0, sizeof(pTemp) / sizeof(char) );

			iRecv = recv( m_sckLink, pTemp, sizeof(pTemp), 0 );

			if( iRecv == SOCKET_ERROR )
			{
				UTIL_LogPrint( "link error (%s) - receive error (error %s)\n", getName( ).c_str( ), GetLastErrorString( ) );

				Kill( );

				return lm;
			}
			else if( iRecv == 0 )
			{
				UTIL_LogPrint( "link error (%s) - receive error (error %s)\n", getName( ).c_str( ), GetLastErrorString( ) );

				Kill( );

				return lm;
			}
			else if( iRecv > 0 )
			{
				m_strReceiveBuf += string( pTemp, iRecv );

				gtXStats.tcp.iRecvLink += iRecv;
			}

			lm = Parse( );

			if( lm.type != LINKMSG_NONE )
				return lm;
		}
	}
	else
	{
		fd_set fdLink;

		FD_ZERO( &fdLink );
		FD_SET( m_sckLink, &fdLink );

		// block for 100 ms to keep from eating up all cpu time

		struct timeval tv;

		tv.tv_sec = 0;
		tv.tv_usec = 100000;

#ifdef WIN32
		if( select( 1, &fdLink, 0, 0, &tv ) == SOCKET_ERROR )
#else
		if( select( m_sckLink + 1, &fdLink, 0, 0, &tv ) == SOCKET_ERROR )
#endif
		{
			UTIL_LogPrint( "link warning (%s) - select error (error %s)\n", getName( ).c_str( ), GetLastErrorString( ) );

			FD_ZERO( &fdLink );

			MILLISLEEP( 100 );
		}

		if( FD_ISSET( m_sckLink, &fdLink ) )
		{
			memset( pTemp, 0, sizeof(pTemp) / sizeof(char) );

			const int ciRecv( recv( m_sckLink, pTemp, sizeof(pTemp), 0 ) );

			if( ciRecv == SOCKET_ERROR )
			{
				UTIL_LogPrint( "link error (%s) - receive error (error %s)\n", getName( ).c_str( ), GetLastErrorString( ) );

				Kill( );

				return lm;
			}
			else if( ciRecv == 0 )
			{
				UTIL_LogPrint( "link error (%s) - receive error (error %s)\n", getName( ).c_str( ), GetLastErrorString( ) );

				Kill( );

				return lm;
			}
			else if( ciRecv > 0 )
			{
				m_strReceiveBuf += string( pTemp, ciRecv );

				gtXStats.tcp.iRecvLink += ciRecv;
			}
		}

		lm = Parse( );
	}

	return lm;
}

struct linkmsg_t CLink :: Parse( )
{
	linkmsg_t lm;

	lm.len = 0;
	lm.type = LINKMSG_NONE;

	if( m_bKill )
		return lm;

	const string :: size_type ciDelim1( m_strReceiveBuf.find_first_not_of( "1234567890" ) );

	if( ciDelim1 != string :: npos )
	{
		if( ciDelim1 > 0 )
		{
			lm.len = (long)atoi( m_strReceiveBuf.substr( 0, ciDelim1 ).c_str( ) );

			const string :: size_type ciDelim2( m_strReceiveBuf.find_first_not_of( "1234567890", ciDelim1 + 1 ) );

			if( ciDelim2 != string :: npos )
			{
				if( ciDelim2 > ciDelim1 )
				{
					lm.type = (char)atoi( m_strReceiveBuf.substr( ciDelim1 + 1, ciDelim2 - ciDelim1 - 1 ).c_str( ) );

					if( m_strReceiveBuf.size( ) > ciDelim2 + lm.len )
					{
						lm.msg = m_strReceiveBuf.substr( ciDelim2 + 1, lm.len );

						m_strReceiveBuf = m_strReceiveBuf.substr( ciDelim2 + lm.len + 1 );
					}
				}
				else
				{
					UTIL_LogPrint( "link error (%s) - unexpected character, disconnecting\n", getName( ).c_str( ) );

					Kill( );
				}
			}
		}
		else
		{
			UTIL_LogPrint( "link error (%s) - unexpected character, disconnecting\n", getName( ).c_str( ) );

			Kill( );
		}
	}

	return lm;
}

string CLink :: getName( )
{
	return m_strIP + ":" + CAtomInt( ntohs( sin.sin6_port ) ).toString( );
}

void CLink :: Queue( struct linkmsg_t lm )
{
	m_mtxQueued.Claim( );
	m_vecQueued.push_back( lm );
	m_mtxQueued.Release( );
}

void StartLink( )
{
	while( gpServer )
	{
		gpLink = new CLink( );

		gpLink->Go( );

		delete gpLink;

		gpLink = 0;

		MILLISLEEP( 10000 );
	}
}

//
// CLinkClient
//

CLinkClient :: CLinkClient( SOCKET sckLink, struct sockaddr_in6 sinAddress )
{
	m_mtxQueued.Initialize( );

	m_bKill = false;
	m_bActive = false;

	m_sckLink = sckLink;

	sin = sinAddress;

	UTIL_LogPrint( "link (%s) - link established\n", getName( ).c_str( ) );
}

CLinkClient :: ~CLinkClient( )
{
	linkmsg_t lmClose;

	lmClose.len = 0;
	lmClose.type = LINKMSG_CLOSE;
	lmClose.msg.erase( );

	Send( lmClose );

	int iReturnShutdown = shutdown( m_sckLink, 2 );

	if( iReturnShutdown == SOCKET_ERROR )
		UTIL_LogPrint( "link error - Shutdown Socket (%s)\n", GetLastErrorString( ) );
	
	iReturnShutdown = closesocket( m_sckLink );
	
	if( iReturnShutdown == SOCKET_ERROR )
		UTIL_LogPrint( "link error - Closing Socket (%s)\n", GetLastErrorString( ) );
	else
	{
		if( gbDebug )
			UTIL_LogPrint( "link - Socket Closed\n" );
	}

	m_mtxQueued.Destroy( );

	UTIL_LogPrint( "link (%s) - link broken\n", getName( ).c_str( ) );
}

void CLinkClient :: Kill( )
{
	m_bKill = true;
}

void CLinkClient :: Go( )
{
	bool bNoWarning = true;

	struct linkmsg_t lmSend;
	struct linkmsg_t lmReceive;

	lmSend.len = ( long )strlen( LINK_VER );
	lmSend.type = LINKMSG_VERSION;
	lmSend.msg = LINK_VER;

	Send( lmSend );

	lmReceive = Receive( true );

	if( lmReceive.type != LINKMSG_VERSION || lmReceive.msg != LINK_VER )
	{
		UTIL_LogPrint( "link error (%s) - incompatible version, disconnecting\n", getName( ).c_str( ) );

		return;
	}

	// todo -> change nonce

	const string cstrNonce( "hello" );

	CAtomDicti *pInfo = new CAtomDicti( );

	pInfo->setItem( "nonce", new CAtomString( cstrNonce ) );

	lmSend.len = pInfo->EncodedLength( );
	lmSend.type = LINKMSG_INFO;
	lmSend.msg = Encode( pInfo );

    Send( lmSend );

	delete pInfo;

	lmReceive = Receive( true );

	if( lmReceive.type != LINKMSG_PASSWORD )
	{
		UTIL_LogPrint( "link error (%s) - unexpected message, disconnecting\n", getName( ).c_str( ) );

		return;
	}

	const string cstrHashMe( gpLinkServer->m_strPass + ":" + cstrNonce );

	unsigned char szMD5[16];
	memset( szMD5, 0, sizeof(szMD5) / sizeof(unsigned char) );

	MD5_CTX md5;

	MD5Init( &md5 );
	MD5Update( &md5, (const unsigned char *)cstrHashMe.c_str( ), (unsigned int)cstrHashMe.size( ) );
	MD5Final( szMD5, &md5 );

	const string strMD5( string( (char *)szMD5, sizeof(szMD5) ) );

	if( strMD5 == lmReceive.msg )
		UTIL_LogPrint( "link (%s) - password accepted\n", getName( ).c_str( ) );
	else
	{
		UTIL_LogPrint( "link error (%s) - bad password, disconnecting\n", getName( ).c_str( ) );

		return;
	}

	vector<struct linkmsg_t> vecTemp;
	vecTemp.reserve(100);

	CAtom *pParams = 0;
	CAtomDicti *pParamsDicti = 0;


	CAtom *pInfoHash = 0;
	CAtom *pIP = 0;
	CAtom *pEvent = 0;
	CAtom *pPort = 0;
	CAtom *pUploaded = 0;
	CAtom *pDownloaded = 0;
	CAtom *pLeft = 0;
	CAtom *pPeerID = 0;
	CAtom *pKey = 0;

	struct announce_t ann;

	while( bNoWarning )
	{
		if( m_bKill )
			return;

		// send

		if( m_bActive )
		{
			m_mtxQueued.Claim( );
			vecTemp = m_vecQueued;
			m_vecQueued.clear( );
			m_mtxQueued.Release( );

			for( vector<struct linkmsg_t> :: iterator it = vecTemp.begin( ); it != vecTemp.end( ); it++ )
				Send( *it );
		}

		// receive

		lmReceive = Receive( false );

		if( lmReceive.type == LINKMSG_ERROR || lmReceive.type == LINKMSG_NONE )
		{
			// ignore
		}
		else if( lmReceive.type == LINKMSG_READY )
		{
			if( gbDebug )
				UTIL_LogPrint( "link (%s) - ready\n", getName( ).c_str( ) );

			m_bActive = true;
		}
		else if( lmReceive.type == LINKMSG_ANNOUNCE )
		{
			pParams = Decode( lmReceive.msg );

			if( pParams && pParams->isDicti( ) )
			{
				pParamsDicti = (CAtomDicti *)pParams;

				pInfoHash = pParamsDicti->getItem( "info_hash" );
				pIP = pParamsDicti->getItem( "ip" );
				pEvent = pParamsDicti->getItem( "event" );
				pPort = pParamsDicti->getItem( "port" );
				pUploaded = pParamsDicti->getItem( "uploaded" );
				pDownloaded = pParamsDicti->getItem( "downloaded" );
				pLeft = pParamsDicti->getItem( "left" );
				pPeerID = pParamsDicti->getItem( "peer_id" );
				pKey = pParamsDicti->getItem( "key" );

				if( pInfoHash && pIP && pPort && pUploaded && pDownloaded && pLeft && pPeerID )
				{
					ann.strInfoHash = pInfoHash->toString( );
					ann.strIP = pIP->toString( );
					ann.uiPort = (unsigned int)( (CAtomLong *)pPort )->getValue( );
					ann.iUploaded = ( (CAtomLong *)pUploaded )->getValue( );
					ann.iDownloaded = ( (CAtomLong *)pDownloaded )->getValue( );
					ann.iLeft = ( (CAtomLong *)pLeft )->getValue( );
					ann.strPeerID = pPeerID->toString( );
					ann.strKey = pKey->toString( );

					// assume strEvent is legit

					if( pEvent )
						ann.strEvent = pEvent->toString( );

					gpServer->getTracker( )->QueueAnnounce( ann );
				}

				delete pParams;
			}

			gpLinkServer->Queue( lmReceive, getName( ) );
		}
		else if( lmReceive.type == LINKMSG_CLOSE )
		{
			UTIL_LogPrint( "link warning (%s) - other end closing connection\n", getName( ).c_str( ) );

			return;
		}
		else
			UTIL_LogPrint( "link warning (%s) - unexpected message %d\n", getName( ).c_str( ), lmReceive.type );
	}
}

void CLinkClient :: Send( struct linkmsg_t lm )
{
	if( m_bKill )
		return;

	m_strSendBuf.erase( );
	m_strSendBuf += CAtomLong( lm.len ).toString( );
	m_strSendBuf += "|";
	m_strSendBuf += CAtomInt( lm.type ).toString( );
	m_strSendBuf += "|";
	m_strSendBuf += lm.msg;

	int iSend = 0;

	while( !m_strSendBuf.empty( ) )
	{
		iSend = send( m_sckLink, m_strSendBuf.c_str( ), ( int )m_strSendBuf.size( ), MSG_NOSIGNAL );

		if( iSend == SOCKET_ERROR )
		{
			UTIL_LogPrint( "link error (%s) - send error (error %s)\n", getName( ).c_str( ), GetLastErrorString( ) );

			Kill( );

			return;
		}
		else if( iSend == 0 )
		{
			UTIL_LogPrint( "link error (%s) - send error (error %s)\n", getName( ).c_str( ), GetLastErrorString( ) );

			Kill( );

			return;
		}
		else if( iSend > 0 )
		{
			m_strSendBuf = m_strSendBuf.substr( iSend );

			gtXStats.tcp.iSendLink += iSend;
		}
	}
}

struct linkmsg_t CLinkClient :: Receive( bool bBlock )
{
	bool bNoWarning = true;

	struct linkmsg_t lm;

	lm.len = 0;
	lm.type = LINKMSG_NONE;

	if( m_bKill )
		return lm;

	char pTemp[16384];

	int iRecv = 0;

	if( bBlock )
	{
		while( bNoWarning )
		{
			memset( pTemp, 0, sizeof(pTemp) / sizeof(char) );

			iRecv = recv( m_sckLink, pTemp, sizeof(pTemp), 0 );

			if( iRecv == SOCKET_ERROR )
			{
				UTIL_LogPrint( "link error (%s) - receive error (error %s)\n", getName( ).c_str( ), GetLastErrorString( ) );

				Kill( );

				return lm;
			}
			else if( iRecv == 0 )
			{
				UTIL_LogPrint( "link error (%s) - receive error (error %s)\n", getName( ).c_str( ), GetLastErrorString( ) );

				Kill( );

				return lm;
			}
			else if( iRecv > 0 )
			{
				m_strReceiveBuf += string( pTemp, iRecv );

				gtXStats.tcp.iRecvLink += iRecv;
			}

			lm = Parse( );

			if( lm.type != LINKMSG_NONE )
				return lm;
		}
	}
	else
	{
		fd_set fdLink;

		FD_ZERO( &fdLink );
		FD_SET( m_sckLink, &fdLink );

		// block for 100 ms to keep from eating up all cpu time

		struct timeval tv;

		tv.tv_sec = 0;
		tv.tv_usec = 100000;

#ifdef WIN32
		if( select( 1, &fdLink, 0, 0, &tv ) == SOCKET_ERROR )
#else
		if( select( m_sckLink + 1, &fdLink, 0, 0, &tv ) == SOCKET_ERROR )
#endif
		{
			UTIL_LogPrint( "link warning (%s) - select error (error %s)\n", getName( ).c_str( ), GetLastErrorString( ) );

			FD_ZERO( &fdLink );

			MILLISLEEP( 100 );
		}

		if( FD_ISSET( m_sckLink, &fdLink ) )
		{
			memset( pTemp, 0, sizeof(pTemp) / sizeof(char)  );

			iRecv = recv( m_sckLink, pTemp, sizeof(pTemp), 0 );

			if( iRecv == SOCKET_ERROR )
			{
				UTIL_LogPrint( "link error (%s) - receive error (error %s)\n", getName( ).c_str( ), GetLastErrorString( ) );

				Kill( );

				return lm;
			}
			else if( iRecv == 0 )
			{
				UTIL_LogPrint( "link error (%s) - receive error (error %s)\n", getName( ).c_str( ), GetLastErrorString( ) );

				Kill( );

				return lm;
			}
			else if( iRecv > 0 )
			{
				m_strReceiveBuf += string( pTemp, iRecv );

				gtXStats.tcp.iRecvLink += iRecv;
			}
		}

		lm = Parse( );
	}

	return lm;
}

struct linkmsg_t CLinkClient :: Parse( )
{
	linkmsg_t lm;

	lm.len = 0;
	lm.type = LINKMSG_NONE;

	if( m_bKill )
		return lm;

	const string :: size_type ciDelim1( m_strReceiveBuf.find_first_not_of( "1234567890" ) );

	if( ciDelim1 != string :: npos )
	{
		if( ciDelim1 > 0 )
		{
			lm.len = (long)atoi( m_strReceiveBuf.substr( 0, ciDelim1 ).c_str( ) );

			const string :: size_type ciDelim2( m_strReceiveBuf.find_first_not_of( "1234567890", ciDelim1 + 1 ) );

			if( ciDelim2 != string :: npos )
			{
				if( ciDelim2 > ciDelim1 )
				{
					lm.type = (char)atoi( m_strReceiveBuf.substr( ciDelim1 + 1, ciDelim2 - ciDelim1 - 1 ).c_str( ) );

					if( m_strReceiveBuf.size( ) > ciDelim2 + lm.len )
					{
						lm.msg = m_strReceiveBuf.substr( ciDelim2 + 1, lm.len );

						m_strReceiveBuf = m_strReceiveBuf.substr( ciDelim2 + lm.len + 1 );
					}
				}
				else
				{
					UTIL_LogPrint( "link error (%s) - unexpected character, disconnecting\n", getName( ).c_str( ) );

					Kill( );
				}
			}
		}
		else
		{
			UTIL_LogPrint( "link error (%s) - unexpected character, disconnecting\n", getName( ).c_str( ) );

			Kill( );
		}
	}

	return lm;
}

string CLinkClient :: getName( )
{
	char strIP6[INET6_ADDRSTRLEN];
	inet_ntop( AF_INET6, &sin.sin6_addr, strIP6, INET6_ADDRSTRLEN );
	string strIP = string( strIP6 );
	if( strIP.rfind( "." ) != string :: npos && strIP.find( "::ffff:" ) == 0 )
		strIP = strIP.substr(strIP.rfind( ":" ) + 1,sizeof( strIP )/sizeof( char ) - strIP.rfind( ":" ) - 1);
	return string( strIP ) + ":" + CAtomInt( ntohs( sin.sin6_port ) ).toString( );
}

void CLinkClient :: Queue( struct linkmsg_t lm )
{
	m_mtxQueued.Claim( );
	m_vecQueued.push_back( lm );
	m_mtxQueued.Release( );
}

void StartLinkClient( CLinkClient *pLinkClient )
{
	if( pLinkClient )
	{
		pLinkClient->Go( );

		gpLinkServer->m_mtxLinks.Claim( );

		for( vector<CLinkClient *> :: iterator it = gpLinkServer->m_vecLinks.begin( ); it != gpLinkServer->m_vecLinks.end( ); it++ )
		{
			if( *it == pLinkClient )
			{
				delete *it;

				gpLinkServer->m_vecLinks.erase( it );

				break;
			}
		}

		gpLinkServer->m_mtxLinks.Release( );
	}
}

//
// CLinkServer
//

CLinkServer :: CLinkServer( )
{
	m_mtxLinks.Initialize( );

	m_strBind = CFG_GetString( "bnbt_tlink_bind", string( ) );
	m_strPass = CFG_GetString( "bnbt_tlink_password", string( ) );

	// map protocol name to protocol number

	struct protoent *pPE;

	pPE = getprotobyname( "tcp" );
	
	if( pPE  == 0 )
		UTIL_LogPrint( "link server error - unable to get tcp protocol entry (error %s)\n", GetLastErrorString( ) );

	// allocate socket

	m_sckLinkServer = socket( PF_INET6, SOCK_STREAM, pPE->p_proto );

	if( m_sckLinkServer == INVALID_SOCKET )
		UTIL_LogPrint( "link server error - unable to allocate socket (error %s)\n", GetLastErrorString( ) );

	// Binding socket to IP and port

	struct sockaddr_in6 sin;

	memset( &sin, 0, sizeof( sin ) );

	sin.sin6_family = AF_INET6;

	if( !m_strBind.empty( ) )
	{
		// bind to m_strBind

		if( gbDebug )
			UTIL_LogPrint( "link server - binding to %s\n", m_strBind.c_str( ) );

		inet_pton( AF_INET6, m_strBind.c_str( ), &sin.sin6_addr );

		if( inet_pton( AF_INET6, m_strBind.c_str( ), &sin.sin6_addr ) == NULL )
			UTIL_LogPrint( "link server error - unable to bind to %s\n", m_strBind.c_str( ) );
	}
	else
	{
		// bind to all available addresses

		if( gbDebug )
			UTIL_LogPrint( "link server - binding to all available addresses\n" );

		sin.sin6_addr = in6addr_any;
	}

	sin.sin6_port = htons( (u_short)CFG_GetInt( "bnbt_tlink_port", 5204 ) );

	if( sin.sin6_port == 0 )
		UTIL_LogPrint( "link server error - invalid port %d\n", CFG_GetInt( "bnbt_tlink_port", 5204 ) );

	// bind socket

#ifdef WIN32
	// TCP window size 
	// Send
	if( setsockopt( m_sckLinkServer, SOL_SOCKET, SO_SNDBUF, (const char *)&guiSO_SNDBUF, sizeof(guiSO_SNDBUF) ) == SOCKET_ERROR )
		UTIL_LogPrint( "link warning - setsockopt SO_SNDBUF (%s)\n", GetLastErrorString( ) );

	// Receive
	if( setsockopt( m_sckLinkServer, SOL_SOCKET, SO_RCVBUF, (const char *)&guiSO_RECBUF, sizeof(guiSO_RECBUF) ) == SOCKET_ERROR )
		UTIL_LogPrint( "link warning - setsockopt SO_RCVBUF (%s)\n", GetLastErrorString( ) );

	// 	Allows the socket to be bound to an address that is already in use.
	const int ciOptVal(1);

	if( setsockopt( m_sckLinkServer, SOL_SOCKET, SO_REUSEADDR, (const char *)&ciOptVal, sizeof(int) ) == SOCKET_ERROR )
		UTIL_LogPrint( "link warning - setsockopt SO_REUSEADDR (%s)\n", GetLastErrorString( ) );

	// Naggle's Algorithm
	if( setsockopt( m_sckLinkServer, SOL_SOCKET, TCP_NODELAY, (const char *)&gbTCP_NODELAY, sizeof(int) ) == SOCKET_ERROR )
		UTIL_LogPrint( "server warning - setsockopt TCP_NODELAY (%s)\n", GetLastErrorString( ) );

	// bind
	if( bind( m_sckLinkServer, (SOCKADDR*)&sin, sizeof( sin ) ) == SOCKET_ERROR )
	{
		UTIL_LogPrint( "link error - unable to bind socket (error %s)\n", GetLastErrorString( ) );

		const int ciReturnCloseSocket( closesocket( m_sckLinkServer ) );

		if( ciReturnCloseSocket == SOCKET_ERROR )
			UTIL_LogPrint( "link error - Closing Socket (%s)\n", GetLastErrorString( ) );
		else
		{
			if( gbDebug )
				UTIL_LogPrint( "link - Socket Closed\n" );
		}
	}
#else
	// TCP window size 
	// Send
	if( setsockopt( m_sckLinkServer, SOL_SOCKET, SO_SNDBUF, (const char *)&guiSO_SNDBUF, (socklen_t)sizeof(guiSO_SNDBUF) ) == SOCKET_ERROR )
		UTIL_LogPrint( "link warning - setsockopt SO_SNDBUF (%s)\n", GetLastErrorString( ) );

	// Receive
	if( setsockopt( m_sckLinkServer, SOL_SOCKET, SO_RCVBUF, (const char *)&guiSO_RECBUF, (socklen_t)sizeof(guiSO_RECBUF) ) == SOCKET_ERROR )
		UTIL_LogPrint( "link warning - setsockopt SO_RCVBUF (%s)\n", GetLastErrorString( ) );

	//	Allows the socket to be bound to an address that is already in use.
	// SO_REUSEADDR is not supported on Linux 
	const int ciOptVal(1);

	if( setsockopt( m_sckLinkServer, SOL_SOCKET, SO_REUSEADDR, (const void *)&ciOptVal, (socklen_t)sizeof(ciOptVal) ) == SOCKET_ERROR );
		UTIL_LogPrint( "link warning - setsockopt SO_REUSEADDR (%s)\n", GetLastErrorString( ) );
	
	// Naggle's Algorithm
	if( setsockopt( m_sckLinkServer, SOL_SOCKET, TCP_NODELAY, (const char *)&gbTCP_NODELAY, sizeof(int) ) == SOCKET_ERROR )
		UTIL_LogPrint( "server warning - setsockopt TCP_NODELAY (%s)\n", GetLastErrorString( ) );

	// bind
	if( bind( m_sckLinkServer, (struct sockaddr *)&sin, sizeof( sin ) ) == SOCKET_ERROR )
	{
		UTIL_LogPrint( "link error - unable to bind socket (error %s)\n", GetLastErrorString( ) );
		
		const int iReturnShutdown( closesocket( m_sckLinkServer ) );

		if( iReturnShutdown == SOCKET_ERROR )
			UTIL_LogPrint( "link error - Closing Socket (%s)\n", GetLastErrorString( ) );
		else
		{
			if( gbDebug )
				UTIL_LogPrint( "link - Socket Closed\n" );
		}
	}
#endif

	// listen, queue length 8

	if( listen( m_sckLinkServer, 8 ) == SOCKET_ERROR )
	{
		UTIL_LogPrint( "link error - unable to listen (error %s)\n", GetLastErrorString( ) );

		const int ciReturnCloseSocket( closesocket( m_sckLinkServer ) );

		if( ciReturnCloseSocket == SOCKET_ERROR )
			UTIL_LogPrint( "link error - Closing Socket (%s)\n", GetLastErrorString( ) );
		else
		{
			if( gbDebug )
				UTIL_LogPrint( "link - Socket Closed\n" );
		}
	}

	UTIL_LogPrint( "link server - start\n" );
}

CLinkServer :: ~CLinkServer( )
{
	const int ciReturnCloseSocket( closesocket( m_sckLinkServer ) );

	bool bNoWarning = true;

	if( ciReturnCloseSocket == SOCKET_ERROR )
		UTIL_LogPrint( "link error - Closing Socket (%s)\n", GetLastErrorString( ) );
	else
	{
		if( gbDebug )
			UTIL_LogPrint( "link - Socket Closed\n" );
	}

	for( vector<CLinkClient *> :: iterator it = m_vecLinks.begin( ); it != m_vecLinks.end( ); it++ )
		(*it)->Kill( );

	const unsigned long culStart( GetTime( ) );

	while( bNoWarning )
	{
		if( !m_vecLinks.empty( ) )
		{
			UTIL_LogPrint( "link server - waiting for %d links to disconnect\n", m_vecLinks.size( ) );

			MILLISLEEP( 1000 );
		}
		else
			break;

		if( GetTime( ) - culStart > 60 )
		{
			UTIL_LogPrint( "link server - waited 60 seconds, exiting anyway\n" );

			break;
		}
	}

	m_mtxLinks.Destroy( );

	UTIL_LogPrint( "link server - exit\n" );
}

void CLinkServer :: Update( )
{
	fd_set fdLinkServer;

	FD_ZERO( &fdLinkServer );
	FD_SET( m_sckLinkServer, &fdLinkServer );

	struct timeval tv;

	tv.tv_sec = 0;
	tv.tv_usec = 0;

#ifdef WIN32
	if( select( 1, &fdLinkServer, 0, 0, &tv ) == SOCKET_ERROR )
#else
	if( select( m_sckLinkServer + 1, &fdLinkServer, 0, 0, &tv ) == SOCKET_ERROR )
#endif
	{
		UTIL_LogPrint( "link server error - select error (error %s)\n", GetLastErrorString( ) );

		FD_ZERO( &fdLinkServer );

		MILLISLEEP( 100 );
	}

	if( FD_ISSET( m_sckLinkServer, &fdLinkServer ) )
	{
		struct sockaddr_in6 adrFrom;

		int iAddrLen = sizeof(adrFrom);

		SOCKET sckLinkClient;

#ifdef WIN32
		sckLinkClient = accept( m_sckLinkServer, (struct sockaddr *)&adrFrom, (int *)&iAddrLen );
		
		if( sckLinkClient == INVALID_SOCKET )
#else
		sckLinkClient = accept( m_sckLinkServer, (struct sockaddr *)&adrFrom, (socklen_t *)&iAddrLen );

		if( sckLinkClient == INVALID_SOCKET )
#endif
			UTIL_LogPrint( "link server error - accept error (error %s)\n", GetLastErrorString( ) );
		else
		{
			CLinkClient *pLinkClient = new CLinkClient( sckLinkClient, adrFrom );

#ifdef WIN32
			int iLinkClientThreadResult = (int)_beginthread( ( void (*)(void *) )StartLinkClient, 0, (void *)pLinkClient );

			if( iLinkClientThreadResult == -1 )
				UTIL_LogPrint( "error - unable to spawn link client thread\n" );
#else
			pthread_t thread;

			// set detached state since we don't need to join with any threads

			pthread_attr_t attr;
			pthread_attr_init( &attr );
			pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );

			int iReturnThread = pthread_create( &thread, &attr, ( void * (*)(void *) )StartLinkClient, (void *)pLinkClient );

			if( iReturnThread != 0 )
				UTIL_LogPrint( "error - unable to spawn link thread (error %s)\n", strerror( iReturnThread ) );
#endif

			m_mtxLinks.Claim( );
			m_vecLinks.push_back( pLinkClient );
			m_mtxLinks.Release( );
		}
	}
}

void CLinkServer :: Queue( struct linkmsg_t lm )
{
	m_mtxLinks.Claim( );

	for( vector<CLinkClient *> :: iterator it = m_vecLinks.begin( ); it != m_vecLinks.end( ); it++ )
		(*it)->Queue( lm );

	m_mtxLinks.Release( );
}

void CLinkServer :: Queue( struct linkmsg_t lm, string strExclude )
{
	m_mtxLinks.Claim( );

	for( vector<CLinkClient *> :: iterator it = m_vecLinks.begin( ); it != m_vecLinks.end( ); it++ )
	{
		if( (*it)->getName( ) != strExclude )
			(*it)->Queue( lm );
	}

	m_mtxLinks.Release( );
}

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

//
// CHUBLink
//

CHUBLink :: CHUBLink( )
{
	m_mtxQueued.Initialize( );

	m_bKill = false;

	m_strIP = CFG_GetString( "xbnbt_thlink_connect", string( ) );
	m_strPass = CFG_GetString( "xbnbt_thlink_password", string( ) );

	memset( &sin, 0, sizeof( sin ) );

	sin.sin6_family = AF_INET6;

	// map host name

	struct hostent *pHE;

	pHE = gethostbyname( m_strIP.c_str( ) );
	
	inet_pton( AF_INET6, m_strIP.c_str( ), &sin.sin6_addr );
	
	if( pHE )
		memcpy( &sin.sin6_addr, pHE->h_addr, pHE->h_length );
	else if( inet_pton( AF_INET6, m_strIP.c_str( ), &sin.sin6_addr ) == NULL )
	{
		UTIL_LogPrint( "HUB link error (%s) - unable to get host entry\n", getName( ).c_str( ) );

		Kill( );
	}
	
	sin.sin6_port = htons( (u_short)CFG_GetInt( "xbnbt_thlink_port", 5205 ) );
	
	if( sin.sin6_port == 0 )
	{
		UTIL_LogPrint( "HUB link error (%s) - invalid port %d\n", getName( ).c_str( ), CFG_GetInt( "xbnbt_thlink_port", 5205 ) );

		Kill( );
	}

	m_sckLink = INVALID_SOCKET;
}

CHUBLink :: ~CHUBLink( )
{
	linkmsg_t lmClose;

	lmClose.len = 0;
	lmClose.type = LINKMSG_CLOSE;
	lmClose.msg.erase( );

	Send( lmClose );

	int iReturnShutdown = shutdown( m_sckLink, 2 );

	if( iReturnShutdown == SOCKET_ERROR )
		UTIL_LogPrint( "HUB link error - Shutdown Socket (%s)\n", GetLastErrorString( ) );
	
	iReturnShutdown = closesocket( m_sckLink );
	
	if( iReturnShutdown == SOCKET_ERROR )
		UTIL_LogPrint( "HUB link error - Closing Socket (%s)\n", GetLastErrorString( ) );
	else
	{
		if( gbDebug )
			UTIL_LogPrint( "HUB link - Socket Closed\n" );
	}

	m_mtxQueued.Destroy( );

	UTIL_LogPrint( "HUB link (%s) - link broken\n", getName( ).c_str( ) );
}

void CHUBLink :: Kill( )
{
	m_bKill = true;
}

void CHUBLink :: Go( )
{
	if( m_bKill )
		return;

	bool bNoWarning = true;

	// map protocol name to protocol number

	struct protoent *pPE;

	pPE = getprotobyname( "tcp" );
	
	if( pPE == 0 )
	{
		UTIL_LogPrint( "HUB link error (%s) - unable to get tcp protocol entry (error %s)\n", getName( ).c_str( ), GetLastErrorString( ) );

		return;
	}

	// allocate socket

	m_sckLink = socket( PF_INET6, SOCK_STREAM, pPE->p_proto );
	
	if( m_sckLink == INVALID_SOCKET )
	{
		UTIL_LogPrint( "HUB link error (%s) - unable to allocate socket (error %s)\n", getName( ).c_str( ), GetLastErrorString( ) );

		return;
	}

	// connect socket

	if( connect( m_sckLink, (struct sockaddr *)&sin, sizeof( sin ) ) == SOCKET_ERROR )
	{
		UTIL_LogPrint( "HUB link error (%s) - unable to connect (error %s)\n", getName( ).c_str( ), GetLastErrorString( ) );

		return;
	}

	UTIL_LogPrint( "HUB link (%s) - link established\n", getName( ).c_str( ) );

	// Added by =Xotic=
	gtXStats.date.sHubLinkEstablished = UTIL_Date( );
	// --------------------------- End of Addition

	struct linkmsg_t lmSend;
	struct linkmsg_t lmReceive;

	lmSend.len = ( long )strlen( LINK_VER );
	lmSend.type = LINKMSG_VERSION;
	lmSend.msg = LINK_VER;

	Send( lmSend );

	lmReceive = Receive( true );

	if( lmReceive.type != LINKMSG_VERSION || lmReceive.msg != LINK_VER )
	{
		UTIL_LogPrint( "HUB link error (%s) - incompatible version, disconnecting\n", getName( ).c_str( ) );

		return;
	}

	lmReceive = Receive( true );

	if( lmReceive.type != LINKMSG_INFO )
	{
		UTIL_LogPrint( "HUB link error (%s) - unexpected message, disconnecting\n", getName( ).c_str( ) );

		return;
	}

	CAtom *pInfo = Decode( lmReceive.msg );

	if( pInfo && pInfo->isDicti( ) )
	{
		CAtom *pNonce = ( (CAtomDicti *)pInfo )->getItem( "nonce" );

		string strHashMe = string( );

		if( pNonce )
			strHashMe = m_strPass + ":" + pNonce->toString( );
		else
			strHashMe = m_strPass;

		unsigned char szMD5[16];
		memset( szMD5, 0, sizeof(szMD5) / sizeof(unsigned char) );

		MD5_CTX md5;

		MD5Init( &md5 );
		MD5Update( &md5, (const unsigned char *)strHashMe.c_str( ), (unsigned int)strHashMe.size( ) );
		MD5Final( szMD5, &md5 );

		lmSend.len = sizeof(szMD5);
		lmSend.type = LINKMSG_PASSWORD;
		lmSend.msg = string( (char *)szMD5, sizeof(szMD5) );

		Send( lmSend );

		delete pInfo;
	}
	else
	{
		UTIL_LogPrint( "HUB link error (%s) - bad info message, disconnecting\n", getName( ).c_str( ) );

		return;
	}

	lmSend.len = 0;
	lmSend.type = LINKMSG_READY;
	lmSend.msg.erase( );

	Send( lmSend );

	UTIL_LogPrint( "HUB link (%s) - ready\n", getName( ).c_str( ) );

	vector<struct linkmsg_t> vecTemp;
	vecTemp.reserve( 100 );

	CAtom *pParams = 0;

	CAtomDicti *pParamsDicti = 0;

	CAtom *pInfoHash = 0;
	CAtom *pIP = 0;
	CAtom *pEvent = 0;
	CAtom *pPort = 0;
	CAtom *pUploaded = 0;
	CAtom *pDownloaded = 0;
	CAtom *pLeft = 0;
	CAtom *pPeerID = 0;
	CAtom *pKey = 0;

	struct announce_t ann;

	while( bNoWarning )
	{
		if( m_bKill )
			return;

		// send

		m_mtxQueued.Claim( );
		vecTemp = m_vecQueued;
		m_vecQueued.clear( );
		m_mtxQueued.Release( );

		for( vector<struct linkmsg_t> :: iterator it = vecTemp.begin( ); it != vecTemp.end( ); it++ )
			Send( *it );

		// receive

		lmReceive = Receive( false );

		if( lmReceive.type == LINKMSG_ERROR || lmReceive.type == LINKMSG_NONE )
		{
			// ignore
		}
		else if( lmReceive.type == LINKMSG_ANNOUNCE )
		{
			pParams = Decode( lmReceive.msg );

			if( pParams && pParams->isDicti( ) )
			{
				pParamsDicti = (CAtomDicti *)pParams;

				pInfoHash = pParamsDicti->getItem( "info_hash" );
				pIP = pParamsDicti->getItem( "ip" );
				pEvent = pParamsDicti->getItem( "event" );
				pPort = pParamsDicti->getItem( "port" );
				pUploaded = pParamsDicti->getItem( "uploaded" );
				pDownloaded = pParamsDicti->getItem( "downloaded" );
				pLeft = pParamsDicti->getItem( "left" );
				pPeerID = pParamsDicti->getItem( "peer_id" );
				pKey = pParamsDicti->getItem( "key" );

				if( pInfoHash && pIP && pPort && pUploaded && pDownloaded && pLeft && pPeerID && pKey )
				{
					ann.strInfoHash = pInfoHash->toString( );
					ann.strIP = pIP->toString( );
					ann.uiPort = (unsigned int)( (CAtomLong *)pPort )->getValue( );
					ann.iUploaded = ( (CAtomLong *)pUploaded )->getValue( );
					ann.iDownloaded = ( (CAtomLong *)pDownloaded )->getValue( );
					ann.iLeft = ( (CAtomLong *)pLeft )->getValue( );
					ann.strPeerID = pPeerID->toString( );
					ann.strKey = pKey->toString( );

					// assume strEvent is legit

					if( pEvent )
						ann.strEvent = pEvent->toString( );

					gpServer->getTracker( )->QueueAnnounce( ann );
				}

				delete pParams;
			}
		}
		else if( lmReceive.type == LINKMSG_CLOSE )
		{
			UTIL_LogPrint( "HUB link warning (%s) - other end closing connection\n", getName( ).c_str( ) );

			return;
		}
		else
			UTIL_LogPrint( "HUB link warning (%s) - unexpected message %d\n", getName( ).c_str( ), lmReceive.type );
	}
}

void CHUBLink :: Send( struct linkmsg_t lm )
{
	if( m_bKill )
		return;

	m_strSendBuf.erase( );
	m_strSendBuf += CAtomLong( lm.len ).toString( );
	m_strSendBuf += "|";
	m_strSendBuf += CAtomInt( lm.type ).toString( );
	m_strSendBuf += "|";
	m_strSendBuf += lm.msg;

	int iSend = 0;

	while( !m_strSendBuf.empty( ) )
	{
		iSend = send( m_sckLink, m_strSendBuf.c_str( ), ( int )m_strSendBuf.size( ), MSG_NOSIGNAL );

		if( iSend == SOCKET_ERROR )
		{
			UTIL_LogPrint( "HUB link error (%s) - send error (error %s)\n", getName( ).c_str( ), GetLastErrorString( ) );

			Kill( );

			return;
		}
		else if( iSend == 0 )
		{
			UTIL_LogPrint( "HUB link error (%s) - send error (error %s)\n", getName( ).c_str( ), GetLastErrorString( ) );

			Kill( );

			return;
		}
		else if( iSend > 0 )
		{
			m_strSendBuf = m_strSendBuf.substr( iSend );
			
			gtXStats.tcp.iSendHub += iSend;
		}
	}
}

struct linkmsg_t CHUBLink :: Receive( bool bBlock )
{
	bool bNoWarning = true;

	struct linkmsg_t lm;

	lm.len = 0;
	lm.type = LINKMSG_NONE;

	if( m_bKill )
		return lm;

	char pTemp[16384];

	if( bBlock )
	{
		int iRecv = 0;

		while( bNoWarning )
		{
			memset( pTemp, 0, sizeof(pTemp) / sizeof(char) );

			iRecv = recv( m_sckLink, pTemp, sizeof(pTemp), 0 );

			if( iRecv == SOCKET_ERROR )
			{
				UTIL_LogPrint( "HUB link error (%s) - receive error (error %s)\n", getName( ).c_str( ), GetLastErrorString( ) );

				Kill( );

				return lm;
			}
			else if( iRecv == 0 )
			{
				UTIL_LogPrint( "HUB link error (%s) - receive error (error %s)\n", getName( ).c_str( ), GetLastErrorString( ) );

				Kill( );

				return lm;
			}
			else if( iRecv > 0 )
			{
				m_strReceiveBuf += string( pTemp, iRecv );

				gtXStats.tcp.iRecvHub += iRecv;
			}

			lm = Parse( );

			if( lm.type != LINKMSG_NONE )
				return lm;
		}
	}
	else
	{
		fd_set fdLink;

		FD_ZERO( &fdLink );
		FD_SET( m_sckLink, &fdLink );

		// block for 100 ms to keep from eating up all cpu time

		struct timeval tv;

		tv.tv_sec = 0;
		tv.tv_usec = 100000;

#ifdef WIN32
		if( select( 1, &fdLink, 0, 0, &tv ) == SOCKET_ERROR )
#else
		if( select( m_sckLink + 1, &fdLink, 0, 0, &tv ) == SOCKET_ERROR )
#endif
		{
			UTIL_LogPrint( "HUB link warning (%s) - select error (error %s)\n", getName( ).c_str( ), GetLastErrorString( ) );

			FD_ZERO( &fdLink );

			MILLISLEEP( 100 );
		}

		if( FD_ISSET( m_sckLink, &fdLink ) )
		{
			memset( pTemp, 0, sizeof(pTemp) / sizeof(char) );

			const int ciRecv( recv( m_sckLink, pTemp, sizeof(pTemp), 0 ) );

			if( ciRecv == SOCKET_ERROR )
			{
				UTIL_LogPrint( "HUB link error (%s) - receive error (error %s)\n", getName( ).c_str( ), GetLastErrorString( ) );

				Kill( );

				return lm;
			}
			else if( ciRecv == 0 )
			{
				UTIL_LogPrint( "HUB link error (%s) - receive error (error %s)\n", getName( ).c_str( ), GetLastErrorString( ) );

				Kill( );

				return lm;
			}
			else if( ciRecv > 0 )
			{
				m_strReceiveBuf += string( pTemp, ciRecv );

				gtXStats.tcp.iRecvHub += ciRecv;
			}
		}

		lm = Parse( );
	}

	return lm;
}

struct linkmsg_t CHUBLink :: Parse( )
{
	linkmsg_t lm;

	lm.len = 0;
	lm.type = LINKMSG_NONE;

	if( m_bKill )
		return lm;

	const string :: size_type ciDelim1( m_strReceiveBuf.find_first_not_of( "1234567890" ) );

	if( ciDelim1 != string :: npos )
	{
		if( ciDelim1 > 0 )
		{
			lm.len = (long)atoi( m_strReceiveBuf.substr( 0, ciDelim1 ).c_str( ) );

			const string :: size_type ciDelim2( m_strReceiveBuf.find_first_not_of( "1234567890", ciDelim1 + 1 ) );

			if( ciDelim2 != string :: npos )
			{
				if( ciDelim2 > ciDelim1 )
				{
					lm.type = (char)atoi( m_strReceiveBuf.substr( ciDelim1 + 1, ciDelim2 - ciDelim1 - 1 ).c_str( ) );

					if( m_strReceiveBuf.size( ) > ciDelim2 + lm.len )
					{
						lm.msg = m_strReceiveBuf.substr( ciDelim2 + 1, lm.len );

						m_strReceiveBuf = m_strReceiveBuf.substr( ciDelim2 + lm.len + 1 );
					}
				}
				else
				{
					UTIL_LogPrint( "HUB link error (%s) - unexpected character, disconnecting\n", getName( ).c_str( ) );

					Kill( );
				}
			}
		}
		else
		{
			UTIL_LogPrint( "HUB link error (%s) - unexpected character, disconnecting\n", getName( ).c_str( ) );

			Kill( );
		}
	}

	return lm;
}

string CHUBLink :: getName( )
{
	return m_strIP + ":" + CAtomInt( ntohs( sin.sin6_port ) ).toString( );
}

void CHUBLink :: Queue( struct linkmsg_t lm )
{
	m_mtxQueued.Claim( );
	m_vecQueued.push_back( lm );
	m_mtxQueued.Release( );
}

void StartHUBLink( )
{
	while( gpServer )
	{
		gpHUBLink = new CHUBLink( );

		gpHUBLink->Go( );

		delete gpHUBLink;

		gpHUBLink = 0;

		MILLISLEEP( 10000 );
	}
}

//
// CHUBLinkClient
//

CHUBLinkClient :: CHUBLinkClient( SOCKET sckLink, struct sockaddr_in6 sinAddress )
{
	m_mtxQueued.Initialize( );

	m_bKill = false;
	m_bActive = false;

	m_sckLink = sckLink;

	sin = sinAddress;

	UTIL_LogPrint( "HUB link (%s) - link established\n", getName( ).c_str( ) );
}

CHUBLinkClient :: ~CHUBLinkClient( )
{
	linkmsg_t lmClose;

	lmClose.len = 0;
	lmClose.type = LINKMSG_CLOSE;
	lmClose.msg.erase( );

	Send( lmClose );

	int iReturnShutdown = shutdown( m_sckLink, 2 );

	if( iReturnShutdown == SOCKET_ERROR )
		UTIL_LogPrint( "HUB link error - Shutdown Socket (%s)\n", GetLastErrorString( ) );
	
	iReturnShutdown = closesocket( m_sckLink );
	
	if( iReturnShutdown == SOCKET_ERROR )
		UTIL_LogPrint( "HUB link error - Closing Socket (%s)\n", GetLastErrorString( ) );
	else
	{
		if( gbDebug )
			UTIL_LogPrint( "HUB link - Socket Closed\n" );
	}

	m_mtxQueued.Destroy( );

	UTIL_LogPrint( "HUB link (%s) - link broken\n", getName( ).c_str( ) );
}

void CHUBLinkClient :: Kill( )
{
	m_bKill = true;
}

void CHUBLinkClient :: Go( )
{
	bool bNoWarning = true;

	struct linkmsg_t lmSend;
	struct linkmsg_t lmReceive;

	lmSend.len = ( long )strlen( LINK_VER );
	lmSend.type = LINKMSG_VERSION;
	lmSend.msg = LINK_VER;

	Send( lmSend );

	lmReceive = Receive( true );

	if( lmReceive.type != LINKMSG_VERSION || lmReceive.msg != LINK_VER )
	{
		UTIL_LogPrint( "HUB link error (%s) - incompatible version, disconnecting\n", getName( ).c_str( ) );

		return;
	}

	// todo -> change nonce

	const string cstrNonce( "hello" );

	CAtomDicti *pInfo = new CAtomDicti( );

	pInfo->setItem( "nonce", new CAtomString( cstrNonce ) );

	lmSend.len = pInfo->EncodedLength( );
	lmSend.type = LINKMSG_INFO;
	lmSend.msg = Encode( pInfo );

    Send( lmSend );

	delete pInfo;

	lmReceive = Receive( true );

	if( lmReceive.type != LINKMSG_PASSWORD )
	{
		UTIL_LogPrint( "HUB link error (%s) - unexpected message, disconnecting\n", getName( ).c_str( ) );

		return;
	}

	const string cstrHashMe( gpHUBLinkServer->m_strPass + ":" + cstrNonce );

	unsigned char szMD5[16];
	memset( szMD5, 0, sizeof(szMD5) / sizeof(unsigned char)  );

	MD5_CTX md5;

	MD5Init( &md5 );
	MD5Update( &md5, (const unsigned char *)cstrHashMe.c_str( ), (unsigned int)cstrHashMe.size( ) );
	MD5Final( szMD5, &md5 );

	const string strMD5( string( (char *)szMD5, sizeof(szMD5) ) );

	if( strMD5 == lmReceive.msg )
		UTIL_LogPrint( "HUB link (%s) - password accepted\n", getName( ).c_str( ) );
	else
	{
		UTIL_LogPrint( "HUB link error (%s) - bad password, disconnecting\n", getName( ).c_str( ) );

		return;
	}

	vector<struct linkmsg_t> vecTemp;
	vecTemp.reserve(100);

	CAtom *pParams = 0;
	CAtomDicti *pParamsDicti = 0;


	CAtom *pInfoHash = 0;
	CAtom *pIP = 0;
	CAtom *pEvent = 0;
	CAtom *pPort = 0;
	CAtom *pUploaded = 0;
	CAtom *pDownloaded = 0;
	CAtom *pLeft = 0;
	CAtom *pPeerID = 0;
	CAtom *pKey = 0;

	struct announce_t ann;

	while( bNoWarning )
	{
		if( m_bKill )
			return;

		// send

		if( m_bActive )
		{
			m_mtxQueued.Claim( );
			vecTemp = m_vecQueued;
			m_vecQueued.clear( );
			m_mtxQueued.Release( );

			for( vector<struct linkmsg_t> :: iterator it = vecTemp.begin( ); it != vecTemp.end( ); it++ )
				Send( *it );
		}

		// receive

		lmReceive = Receive( false );

		if( lmReceive.type == LINKMSG_ERROR || lmReceive.type == LINKMSG_NONE )
		{
			// ignore
		}
		else if( lmReceive.type == LINKMSG_READY )
		{
			if( gbDebug )
				UTIL_LogPrint( "HUB link (%s) - ready\n", getName( ).c_str( ) );

			m_bActive = true;
		}
		else if( lmReceive.type == LINKMSG_ANNOUNCE )
		{
			pParams = Decode( lmReceive.msg );

			if( pParams && pParams->isDicti( ) )
			{
				pParamsDicti = (CAtomDicti *)pParams;

				pInfoHash = pParamsDicti->getItem( "info_hash" );
				pIP = pParamsDicti->getItem( "ip" );
				pEvent = pParamsDicti->getItem( "event" );
				pPort = pParamsDicti->getItem( "port" );
				pUploaded = pParamsDicti->getItem( "uploaded" );
				pDownloaded = pParamsDicti->getItem( "downloaded" );
				pLeft = pParamsDicti->getItem( "left" );
				pPeerID = pParamsDicti->getItem( "peer_id" );
				pKey = pParamsDicti->getItem( "key" );

				if( pInfoHash && pIP && pPort && pUploaded && pDownloaded && pLeft && pPeerID )
				{
					ann.strInfoHash = pInfoHash->toString( );
					ann.strIP = pIP->toString( );
					ann.uiPort = (unsigned int)( (CAtomLong *)pPort )->getValue( );
					ann.iUploaded = ( (CAtomLong *)pUploaded )->getValue( );
					ann.iDownloaded = ( (CAtomLong *)pDownloaded )->getValue( );
					ann.iLeft = ( (CAtomLong *)pLeft )->getValue( );
					ann.strPeerID = pPeerID->toString( );
					ann.strKey = pKey->toString( );

					// assume strEvent is legit

					if( pEvent )
						ann.strEvent = pEvent->toString( );

					gpServer->getTracker( )->QueueAnnounce( ann );
				}

				delete pParams;
			}

			gpHUBLinkServer->Queue( lmReceive, getName( ) );
		}
		else if( lmReceive.type == LINKMSG_CLOSE )
		{
			UTIL_LogPrint( "HUB link warning (%s) - other end closing connection\n", getName( ).c_str( ) );

			return;
		}
		else
			UTIL_LogPrint( "HUB link warning (%s) - unexpected message %d\n", getName( ).c_str( ), lmReceive.type );
	}
}

void CHUBLinkClient :: Send( struct linkmsg_t lm )
{
	if( m_bKill )
		return;

	m_strSendBuf.erase( );
	m_strSendBuf += CAtomLong( lm.len ).toString( );
	m_strSendBuf += "|";
	m_strSendBuf += CAtomInt( lm.type ).toString( );
	m_strSendBuf += "|";
	m_strSendBuf += lm.msg;

	int iSend = 0;

	while( !m_strSendBuf.empty( ) )
	{
		iSend = send( m_sckLink, m_strSendBuf.c_str( ), ( int )m_strSendBuf.size( ), MSG_NOSIGNAL );

		if( iSend == SOCKET_ERROR )
		{
			UTIL_LogPrint( "HUB link error (%s) - send error (error %s)\n", getName( ).c_str( ), GetLastErrorString( ) );

			Kill( );

			return;
		}
		else if( iSend == 0 )
		{
			UTIL_LogPrint( "HUB link error (%s) - send error (error %s)\n", getName( ).c_str( ), GetLastErrorString( ) );

			Kill( );

			return;
		}
		else if( iSend > 0 )
		{
			m_strSendBuf = m_strSendBuf.substr( iSend );

			gtXStats.tcp.iSendHub += iSend;
		}
	}
}

struct linkmsg_t CHUBLinkClient :: Receive( bool bBlock )
{
	bool bNoWarning = true;

	struct linkmsg_t lm;

	lm.len = 0;
	lm.type = LINKMSG_NONE;

	if( m_bKill )
		return lm;

	char pTemp[16384];

	int iRecv = 0;

	if( bBlock )
	{
		while( bNoWarning )
		{
			memset( pTemp, 0, sizeof(pTemp) / sizeof(char) );

			iRecv = recv( m_sckLink, pTemp, sizeof(pTemp), 0 );

			if( iRecv == SOCKET_ERROR )
			{
				UTIL_LogPrint( "HUB link error (%s) - receive error (error %s)\n", getName( ).c_str( ), GetLastErrorString( ) );

				Kill( );

				return lm;
			}
			else if( iRecv == 0 )
			{
				UTIL_LogPrint( "HUB link error (%s) - receive error (error %s)\n", getName( ).c_str( ), GetLastErrorString( ) );

				Kill( );

				return lm;
			}
			else if( iRecv > 0 )
			{
				m_strReceiveBuf += string( pTemp, iRecv );

				gtXStats.tcp.iRecvHub += iRecv;
			}

			lm = Parse( );

			if( lm.type != LINKMSG_NONE )
				return lm;
		}
	}
	else
	{
		fd_set fdLink;

		FD_ZERO( &fdLink );
		FD_SET( m_sckLink, &fdLink );

		// block for 100 ms to keep from eating up all cpu time

		struct timeval tv;

		tv.tv_sec = 0;
		tv.tv_usec = 100000;

#ifdef WIN32
		if( select( 1, &fdLink, 0, 0, &tv ) == SOCKET_ERROR )
#else
		if( select( m_sckLink + 1, &fdLink, 0, 0, &tv ) == SOCKET_ERROR )
#endif
		{
			UTIL_LogPrint( "HUB link warning (%s) - select error (error %s)\n", getName( ).c_str( ), GetLastErrorString( ) );

			FD_ZERO( &fdLink );

			MILLISLEEP( 100 );
		}

		if( FD_ISSET( m_sckLink, &fdLink ) )
		{
			memset( pTemp, 0, sizeof(pTemp) / sizeof(char) );

			iRecv = recv( m_sckLink, pTemp, sizeof(pTemp), 0 );

			if( iRecv == SOCKET_ERROR )
			{
				UTIL_LogPrint( "HUB link error (%s) - receive error (error %s)\n", getName( ).c_str( ), GetLastErrorString( ) );

				Kill( );

				return lm;
			}
			else if( iRecv == 0 )
			{
				UTIL_LogPrint( "HUB link error (%s) - receive error (error %s)\n", getName( ).c_str( ), GetLastErrorString( ) );

				Kill( );

				return lm;
			}
			else if( iRecv > 0 )
			{
				m_strReceiveBuf += string( pTemp, iRecv );

				gtXStats.tcp.iRecvHub += iRecv;
			}
		}

		lm = Parse( );
	}

	return lm;
}

struct linkmsg_t CHUBLinkClient :: Parse( )
{
	linkmsg_t lm;

	lm.len = 0;
	lm.type = LINKMSG_NONE;

	if( m_bKill )
		return lm;

	const string :: size_type ciDelim1( m_strReceiveBuf.find_first_not_of( "1234567890" ) );

	if( ciDelim1 != string :: npos )
	{
		if( ciDelim1 > 0 )
		{
			lm.len = (long)atoi( m_strReceiveBuf.substr( 0, ciDelim1 ).c_str( ) );

			const string :: size_type ciDelim2( m_strReceiveBuf.find_first_not_of( "1234567890", ciDelim1 + 1 ) );

			if( ciDelim2 != string :: npos )
			{
				if( ciDelim2 > ciDelim1 )
				{
					lm.type = (char)atoi( m_strReceiveBuf.substr( ciDelim1 + 1, ciDelim2 - ciDelim1 - 1 ).c_str( ) );

					if( m_strReceiveBuf.size( ) > ciDelim2 + lm.len )
					{
						lm.msg = m_strReceiveBuf.substr( ciDelim2 + 1, lm.len );

						m_strReceiveBuf = m_strReceiveBuf.substr( ciDelim2 + lm.len + 1 );
					}
				}
				else
				{
					UTIL_LogPrint( "HUB link error (%s) - unexpected character, disconnecting\n", getName( ).c_str( ) );

					Kill( );
				}
			}
		}
		else
		{
			UTIL_LogPrint( "HUB link error (%s) - unexpected character, disconnecting\n", getName( ).c_str( ) );

			Kill( );
		}
	}

	return lm;
}

string CHUBLinkClient :: getName( )
{
	char strIP6[INET6_ADDRSTRLEN];
	inet_ntop( AF_INET6, &sin.sin6_addr, strIP6, INET6_ADDRSTRLEN );
	string strIP = string( strIP6 );
	if( strIP.rfind( "." ) != string :: npos && strIP.find( "::ffff:" ) == 0 )
		strIP = strIP.substr(strIP.rfind( ":" ) + 1,sizeof( strIP )/sizeof( char ) - strIP.rfind( ":" ) - 1);
	return string( strIP ) + ":" + CAtomInt( ntohs( sin.sin6_port ) ).toString( );
}

void CHUBLinkClient :: Queue( struct linkmsg_t lm )
{
	m_mtxQueued.Claim( );
	m_vecQueued.push_back( lm );
	m_mtxQueued.Release( );
}

void StartHUBLinkClient( CHUBLinkClient *pLinkClient )
{
	if( pLinkClient )
	{
		pLinkClient->Go( );

		gpHUBLinkServer->m_mtxLinks.Claim( );

		for( vector<CHUBLinkClient *> :: iterator i = gpHUBLinkServer->m_vecLinks.begin( ); i != gpHUBLinkServer->m_vecLinks.end( ); i++ )
		{
			if( *i == pLinkClient )
			{
				delete *i;

				gpHUBLinkServer->m_vecLinks.erase( i );

				break;
			}
		}

		gpHUBLinkServer->m_mtxLinks.Release( );
	}
}

//
// CHUBLinkServer
//

CHUBLinkServer :: CHUBLinkServer( )
{
	m_mtxLinks.Initialize( );

	m_strBind = CFG_GetString( "xbnbt_thlink_bind", string( ) );
	m_strPass = CFG_GetString( "xbnbt_thlink_password", string( ) );

	// map protocol name to protocol number

	struct protoent *pPE;

	pPE = getprotobyname( "tcp" );
	
	if( pPE  == 0 )
		UTIL_LogPrint( "HUB link server error - unable to get tcp protocol entry (error %s)\n", GetLastErrorString( ) );

	// allocate socket

	m_sckLinkServer = socket( PF_INET6, SOCK_STREAM, pPE->p_proto );

	if( m_sckLinkServer == INVALID_SOCKET )
		UTIL_LogPrint( "HUB link server error - unable to allocate socket (error %s)\n", GetLastErrorString( ) );

	// Binding socket to IP and port

	struct sockaddr_in6 sin;

	memset( &sin, 0, sizeof( sin ) );

	sin.sin6_family = AF_INET6;

	if( !m_strBind.empty( ) )
	{
		// bind to m_strBind

		if( gbDebug )
			UTIL_LogPrint( "HUB link server - binding to %s\n", m_strBind.c_str( ) );

		inet_pton( AF_INET6, m_strBind.c_str( ), &sin.sin6_addr );

		if( inet_pton( AF_INET6, m_strBind.c_str( ), &sin.sin6_addr ) == NULL )
			UTIL_LogPrint( "HUB link server error - unable to bind to %s\n", m_strBind.c_str( ) );
	}
	else
	{
		// bind to all available addresses

		if( gbDebug )
			UTIL_LogPrint( "HUB link server - binding to all available addresses\n" );

		sin.sin6_addr = in6addr_any;
	}

	sin.sin6_port = htons( (u_short)CFG_GetInt( "xbnbt_thlink_port", 5205 ) );

	if( sin.sin6_port == 0 )
		UTIL_LogPrint( "HUB link server error - invalid port %d\n", CFG_GetInt( "xbnbt_thlink_port", 5205 ) );

	// bind socket

#ifdef WIN32
	// TCP window size 
	// Send
	if( setsockopt( m_sckLinkServer, SOL_SOCKET, SO_SNDBUF, (const char *)&guiSO_SNDBUF, sizeof(guiSO_SNDBUF) ) == SOCKET_ERROR )
		UTIL_LogPrint( "HUB link warning - setsockopt SO_SNDBUF (%s)\n", GetLastErrorString( ) );

	// Receive
	if( setsockopt( m_sckLinkServer, SOL_SOCKET, SO_RCVBUF, (const char *)&guiSO_RECBUF, sizeof(guiSO_RECBUF) ) == SOCKET_ERROR )
		UTIL_LogPrint( "HUB link warning - setsockopt SO_RCVBUF (%s)\n", GetLastErrorString( ) );

	// 	Allows the socket to be bound to an address that is already in use.
	const int ciOptVal(1);

	if( setsockopt( m_sckLinkServer, SOL_SOCKET, SO_REUSEADDR, (const char *)&ciOptVal, sizeof(int) ) == SOCKET_ERROR )
		UTIL_LogPrint( "HUB link warning - setsockopt SO_REUSEADDR (%s)\n", GetLastErrorString( ) );

	// Naggle's Algorithm
	if( setsockopt( m_sckLinkServer, SOL_SOCKET, TCP_NODELAY, (const char *)&gbTCP_NODELAY, sizeof(int) ) == SOCKET_ERROR )
		UTIL_LogPrint( "server warning - setsockopt TCP_NODELAY (%s)\n", GetLastErrorString( ) );

	// bind
	if( bind( m_sckLinkServer, (SOCKADDR*)&sin, sizeof( sin ) ) == SOCKET_ERROR )
	{
		UTIL_LogPrint( "HUB link error - unable to bind socket (error %s)\n", GetLastErrorString( ) );

		const int ciReturnCloseSocket( closesocket( m_sckLinkServer ) );

		if( ciReturnCloseSocket == SOCKET_ERROR )
			UTIL_LogPrint( "HUB link error - Closing Socket (%s)\n", GetLastErrorString( ) );
		else
		{
			if( gbDebug )
				UTIL_LogPrint( "HUB link - Socket Closed\n" );
		}
	}
#else
	// TCP window size 
	// Send
	if( setsockopt( m_sckLinkServer, SOL_SOCKET, SO_SNDBUF, (const char *)&guiSO_SNDBUF, (socklen_t)sizeof(guiSO_SNDBUF) ) == SOCKET_ERROR )
		UTIL_LogPrint( "HUB link warning - setsockopt SO_SNDBUF (%s)\n", GetLastErrorString( ) );

	// Receive
	if( setsockopt( m_sckLinkServer, SOL_SOCKET, SO_RCVBUF, (const char *)&guiSO_RECBUF, (socklen_t)sizeof(guiSO_RECBUF) ) == SOCKET_ERROR )
		UTIL_LogPrint( "HUB link warning - setsockopt SO_RCVBUF (%s)\n", GetLastErrorString( ) );
 
	//	Allows the socket to be bound to an address that is already in use.
	// SO_REUSEADDR is not supported on Linux 
	const int ciOptVal(1);

	if( setsockopt( m_sckLinkServer, SOL_SOCKET, SO_REUSEADDR, (const void *)&ciOptVal, (socklen_t)sizeof(ciOptVal) ) == SOCKET_ERROR );
		UTIL_LogPrint( "HUB link warning - setsockopt SO_REUSEADDR (%s)\n", GetLastErrorString( ) );

	// Naggle's Algorithm
	if( setsockopt( m_sckLinkServer, SOL_SOCKET, TCP_NODELAY, (const char *)&gbTCP_NODELAY, sizeof(int) ) == SOCKET_ERROR )
		UTIL_LogPrint( "server warning - setsockopt TCP_NODELAY (%s)\n", GetLastErrorString( ) );

	// bind
	if( bind( m_sckLinkServer, (struct sockaddr *)&sin, sizeof( sin ) ) == SOCKET_ERROR )
	{
		UTIL_LogPrint( "HUB link error - unable to bind socket (error %s)\n", GetLastErrorString( ) );
		
		const int iReturnShutdown( closesocket( m_sckLinkServer ) );

		if( iReturnShutdown == SOCKET_ERROR )
			UTIL_LogPrint( "HUB link error - Closing Socket (%s)\n", GetLastErrorString( ) );
		else
		{
			if( gbDebug )
				UTIL_LogPrint( "HUB link - Socket Closed\n" );
		}
	}
#endif

	// listen, queue length 8

	if( listen( m_sckLinkServer, 8 ) == SOCKET_ERROR )
	{
		UTIL_LogPrint( "HUB link error - unable to listen (error %s)\n", GetLastErrorString( ) );

		const int ciReturnCloseSocket( closesocket( m_sckLinkServer ) );

		if( ciReturnCloseSocket == SOCKET_ERROR )
			UTIL_LogPrint( "HUB link error - Closing Socket (%s)\n", GetLastErrorString( ) );
		else
		{
			if( gbDebug )
				UTIL_LogPrint( "HUB link - Socket Closed\n" );
		}
	}

	UTIL_LogPrint( "HUB link server - start\n" );
}

CHUBLinkServer :: ~CHUBLinkServer( )
{
	const int ciReturnCloseSocket( closesocket( m_sckLinkServer ) );

	bool bNoWarning = true;

	if( ciReturnCloseSocket == SOCKET_ERROR )
		UTIL_LogPrint( "HUB link error - Closing Socket (%s)\n", GetLastErrorString( ) );
	else
	{
		if( gbDebug )
			UTIL_LogPrint( "HUB link - Socket Closed\n" );
	}

	for( vector<CHUBLinkClient *> :: iterator i = m_vecLinks.begin( ); i != m_vecLinks.end( ); i++ )
		(*i)->Kill( );

	const unsigned long culStart( GetTime( ) );

	while( bNoWarning )
	{
		if( !m_vecLinks.empty( ) )
		{
			UTIL_LogPrint( "HUB link server - waiting for %d links to disconnect\n", m_vecLinks.size( ) );

			MILLISLEEP( 1000 );
		}
		else
			break;

		if( GetTime( ) - culStart > 60 )
		{
			UTIL_LogPrint( "HUB link server - waited 60 seconds, exiting anyway\n" );

			break;
		}
	}

	m_mtxLinks.Destroy( );

	UTIL_LogPrint( "HUB link server - exit\n" );
}

void CHUBLinkServer :: Update( )
{
	fd_set fdLinkServer;

	FD_ZERO( &fdLinkServer );
	FD_SET( m_sckLinkServer, &fdLinkServer );

	struct timeval tv;

	tv.tv_sec = 0;
	tv.tv_usec = 0;

#ifdef WIN32
	if( select( 1, &fdLinkServer, 0, 0, &tv ) == SOCKET_ERROR )
#else
	if( select( m_sckLinkServer + 1, &fdLinkServer, 0, 0, &tv ) == SOCKET_ERROR )
#endif
	{
		UTIL_LogPrint( "HUB link server error - select error (error %s)\n", GetLastErrorString( ) );

		FD_ZERO( &fdLinkServer );

		MILLISLEEP( 100 );
	}

	if( FD_ISSET( m_sckLinkServer, &fdLinkServer ) )
	{
		struct sockaddr_in6 adrFrom;

	int iAddrLen = sizeof( adrFrom );

		SOCKET sckLinkClient;

#ifdef WIN32
		sckLinkClient = accept( m_sckLinkServer, (struct sockaddr *)&adrFrom, (int *)&iAddrLen );
		
		if( sckLinkClient == INVALID_SOCKET )
#else
		sckLinkClient = accept( m_sckLinkServer, (struct sockaddr *)&adrFrom, (socklen_t *)&iAddrLen );

		if( sckLinkClient == INVALID_SOCKET )
#endif
			UTIL_LogPrint( "HUB link server error - accept error (error %s)\n", GetLastErrorString( ) );
		else
		{
			CHUBLinkClient *pLinkClient = new CHUBLinkClient( sckLinkClient, adrFrom );

#ifdef WIN32
			int iHubLinkClientThreadResult = (int)_beginthread( ( void (*)(void *) )StartHUBLinkClient, 0, (void *)pLinkClient );

			if( iHubLinkClientThreadResult == -1 )
				UTIL_LogPrint( "HUB link error - unable to spawn link client thread\n" );
#else
			pthread_t thread;

			// set detached state since we don't need to join with any threads

			pthread_attr_t attr;
			pthread_attr_init( &attr );
			pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );

			int iReturnThread = pthread_create( &thread, &attr, ( void * (*)(void *) )StartHUBLinkClient, (void *)pLinkClient );

			if( iReturnThread != 0 )
				UTIL_LogPrint( "HUB link error - unable to spawn link thread (error %s)\n", strerror( iReturnThread ) );
#endif

			m_mtxLinks.Claim( );
			m_vecLinks.push_back( pLinkClient );
			m_mtxLinks.Release( );
		}
	}
}

void CHUBLinkServer :: Queue( struct linkmsg_t lm )
{
	m_mtxLinks.Claim( );

	for( vector<CHUBLinkClient *> :: iterator i = m_vecLinks.begin( ); i != m_vecLinks.end( ); i++ )
		(*i)->Queue( lm );

	m_mtxLinks.Release( );
}

void CHUBLinkServer :: Queue( struct linkmsg_t lm, string strExclude )
{
	m_mtxLinks.Claim( );

	for( vector<CHUBLinkClient *> :: iterator i = m_vecLinks.begin( ); i != m_vecLinks.end( ); i++ )
	{
		if( (*i)->getName( ) != strExclude )
			(*i)->Queue( lm );
	}

	m_mtxLinks.Release( );
}
