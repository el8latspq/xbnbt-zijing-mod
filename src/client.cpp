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

#ifndef WIN32
 #include <fcntl.h>
#endif

#include <zlib.h>

#include "bnbt.h"
#include "bnbt_mysql.h"
#include "atom.h"
#include "base64.h"
#include "client.h"
#include "config.h"
#include "md5.h"
#include "server.h"
#include "tracker.h"
#include "util.h"

/*
===================
UTIL_DecodeHTTPPost
===================

[
{
disposition->{
key1->string (quotes stripped)
key2->string (quotes stripped)
key3->string (quotes stripped)
key4->string (quotes stripped)
...
}
data->string
},

{
disposition->{
key1->string (quotes stripped)
key2->string (quotes stripped)
key3->string (quotes stripped)
key4->string (quotes stripped)
...
}
data->string
}
]
*/

// Used for uploading torrents

#define C_STR_BOUNDARY "boundary="
#define C_STR_NEWLINE "\r\n"
#define C_STR_DOUBLE_NEWLINE "\r\n\r\n"
#define C_STR_SEGSTART "--"
#define C_STR_SEGEND "\r\n--"
#define C_STR_DISPSTART "Content-Disposition: "
#define C_STR_WSPACE " "
#define C_STR_ITEMEND ";"
#define C_STR_ITEMEQ "="
#define C_STR_ITEMQU "?"
#define C_STR_ITEMAND "&"
#define C_STR_COLON ":"
#define C_STR_SEMICOLON ";"
#define CHAR_QUOTE '"'

inline CAtomList *UTIL_DecodeHTTPPost( const string &cstrPost )
{
	// find the boundary

	string :: size_type iBoundary = cstrPost.find( C_STR_BOUNDARY );

	if( iBoundary == string :: npos )
		return 0;

	iBoundary += ( sizeof(C_STR_BOUNDARY) - 1 );

	string strBoundary = cstrPost.substr( iBoundary );

	const string :: size_type ciBoundEnd( strBoundary.find( C_STR_NEWLINE ) );

	if( ciBoundEnd == string :: npos )
		return 0;

	strBoundary = strBoundary.substr( 0, ciBoundEnd );

	// strBoundary now contains the boundary

	string :: size_type iContent = cstrPost.find( C_STR_DOUBLE_NEWLINE );

	if( iContent == string :: npos )
		return 0;

	iContent += ( sizeof(C_STR_DOUBLE_NEWLINE) - 1 );

	const string cstrContent( cstrPost.substr( iContent ) );

	// decode

	CAtomList *pList = new CAtomList( );

	CAtomDicti *pSegment = 0;

	string :: size_type iSegStart = 0;
	string :: size_type iSegEnd = 0;

	string :: size_type iDispPrev = 0;
	string :: size_type iDispPos = 0;

	CAtomDicti *pDisp = 0;

	string strSeg = string( );
	string strDisp = string( );

	string :: size_type iDispStart = 0;
	string :: size_type iDispEnd = 0;

	string strCurr = string( );

	string :: size_type iSplit = 0;
	string :: size_type iKeyStart = 0;

	string strKey = string( );
	string strValue = string( );

	string :: size_type iDataStart = 0;

	bool bDoSegmentLoop = true;

	while( bDoSegmentLoop )
	{
		// segment start

		iSegStart = cstrContent.find( strBoundary, iSegStart );

		if( iSegStart == string :: npos )
			return pList;

		iSegStart += strBoundary.size( );

		if( cstrContent.substr( iSegStart, 2 ) == C_STR_SEGSTART )
			return pList;

		iSegStart += ( sizeof(C_STR_SEGSTART) - 1 );

		// segment end

		iSegEnd = cstrContent.find( strBoundary, iSegStart );

		if( iSegEnd == string :: npos )
		{
			UTIL_LogPrint( string( gmapLANG_CFG["decode_http_post_end"] + "\n" ).c_str( ) );

			delete pList;

			pList = 0;

			return 0;
		}

		iSegEnd -= ( sizeof(C_STR_SEGEND) - 1 );

		// found segment

		pSegment = new CAtomDicti( );

		pList->addItem( pSegment );

		// this could do with some serious optimizing...

		strSeg = cstrContent.substr( iSegStart, iSegEnd - iSegStart );

		iDispStart = strSeg.find( C_STR_DISPSTART );

		if( iDispStart == string :: npos )
		{
			UTIL_LogPrint( string( gmapLANG_CFG["decode_http_post_notfound"] + "\n" ).c_str( ) );

			delete pList;

			pList = 0;

			return 0;
		}

		iDispStart += ( sizeof(C_STR_DISPSTART) - 1 );

		iDispEnd = strSeg.find( C_STR_NEWLINE, iDispStart );

		if( iDispEnd == string :: npos )
		{
			UTIL_LogPrint( string( gmapLANG_CFG["decode_http_post_disposition"] + "\n" ).c_str( ) );

			delete pList;

			pList = 0;

			return 0;
		}

		strDisp = strSeg.substr( iDispStart, iDispEnd - iDispStart );

		iDispPrev = 0;
		iDispPos = 0;

		pDisp = new CAtomDicti( );

		pSegment->setItem( "disposition", pDisp );

		bool bDoItemLoop = true;

		while( bDoItemLoop )
		{
			// assume a semicolon indicates the end of the item and will never appear inside the item (probably a bad assumption)

			iDispPrev = iDispPos;
			iDispPos = strDisp.find( C_STR_ITEMEND, iDispPos );

			if( iDispPos == string :: npos )
			{
				// decode last item

				iDispPos = strDisp.size( );
			}

			strCurr = strDisp.substr( iDispPrev, iDispPos - iDispPrev );

			iSplit = strCurr.find( C_STR_ITEMEQ );

			if( iSplit == string :: npos )
			{
				// found a key without value, i.e. "form-data", useless so ignore it

				if( iDispPos == strDisp.size( ) )
					break;

				// + strlen( ";" )

				iDispPos++;

				continue;
			}

			// strip whitespace

			iKeyStart = strCurr.find_first_not_of( C_STR_WSPACE );

			if( iKeyStart == string :: npos || iKeyStart > iSplit )
			{
				UTIL_LogPrint( string( gmapLANG_CFG["decode_http_post_disposition"] + "\n" ).c_str( ) );

				delete pList;

				pList = 0;

				return 0;
			}

			strKey = strCurr.substr( iKeyStart, iSplit - iKeyStart );
			strValue = strCurr.substr( iSplit + 1 );

			// strip quotes

			if( strValue.size( ) > 1 && strValue[0] == CHAR_QUOTE )
				strValue = strValue.substr( 1, strValue.size( ) - 2 );

			pDisp->setItem( strKey, new CAtomString( strValue ) );

			if( iDispPos == strDisp.size( ) )
				bDoItemLoop = false;

			// + strlen( ";" )

			iDispPos++;
		}

		// data

		iDataStart = strSeg.find( C_STR_DOUBLE_NEWLINE );

		if( iDataStart == string :: npos )
		{
			UTIL_LogPrint( string( gmapLANG_CFG["decode_http_post_segment"] + "\n" ).c_str( ) );

			delete pList;

			pList = 0;

			return 0;
		}

		iDataStart += ( sizeof(C_STR_DOUBLE_NEWLINE) - 1 );

		pSegment->setItem( "data", new CAtomString( strSeg.substr( iDataStart ) ) );
	}

	// this should never happen, so who cares

	delete pList;

	pList = 0;

	return 0;
}

inline CAtomList *UTIL_DecodeHTTPPostUrlencoded( const string &cstrPost )
{
	string :: size_type iContent = cstrPost.find( C_STR_DOUBLE_NEWLINE );

	if( iContent == string :: npos )
		return 0;

	iContent += ( sizeof(C_STR_DOUBLE_NEWLINE) - 1 );

	string strContent( cstrPost.substr( iContent ) );

	// decode

	CAtomList *pList = new CAtomList( );

	CAtomDicti *pSegment = 0;

	CAtomDicti *pDisp = 0;

	string :: size_type iSplit = 0;
	string :: size_type iEnd = 0;

	string strKey = string( );
	string strValue = string( );

	string :: size_type iDataStart = 0;
	
	// grab params

	if( !strContent.empty( ) )
	{
		strKey = string( );
		strValue = string( );

		bool bGrabParams = true;

		while( bGrabParams )
		{
			iSplit = strContent.find( C_STR_ITEMEQ );
			iEnd = strContent.find( C_STR_ITEMAND );

			if( iSplit == string :: npos )
			{
				UTIL_LogPrint( "client warning: malformed HTTP request (found param key without value)\n" );

				return pList;
			}

			strKey = UTIL_EscapedToString( strContent.substr( 0, iSplit ) );
			strValue = UTIL_EscapedToString( strContent.substr( iSplit + 1, iEnd - iSplit - 1 ) );
			
			pSegment = new CAtomDicti( );
		
			pList->addItem( pSegment );
			
			pDisp = new CAtomDicti( );
			
			pSegment->setItem( "disposition", pDisp );
			
			pDisp->setItem( "name", new CAtomString( strKey ) );
			
			pSegment->setItem( "data", new CAtomString( strValue ) );

			strContent = strContent.substr( iEnd + 1, strContent.size( ) - iEnd - 1 );

			if( iEnd == string :: npos )
				return pList;
		}
	}

//	bool bDoSegmentLoop = true;

//	while( bDoSegmentLoop )
//	{
//		// segment start

//		iSegStart = cstrContent.find( strBoundary, iSegStart );

//		if( iSegStart == string :: npos )
//			return pList;

//		iSegStart += strBoundary.size( );

//		if( cstrContent.substr( iSegStart, 2 ) == C_STR_SEGSTART )
//			return pList;

//		iSegStart += ( sizeof(C_STR_SEGSTART) - 1 );

//		// segment end

//		iSegEnd = cstrContent.find( strBoundary, iSegStart );

//		if( iSegEnd == string :: npos )
//		{
//			UTIL_LogPrint( string( gmapLANG_CFG["decode_http_post_end"] + "\n" ).c_str( ) );

//			delete pList;

//			pList = 0;

//			return 0;
//		}

//		iSegEnd -= ( sizeof(C_STR_SEGEND) - 1 );

//		// found segment

//		pSegment = new CAtomDicti( );

//		pList->addItem( pSegment );

//		// this could do with some serious optimizing...

//		strSeg = cstrContent.substr( iSegStart, iSegEnd - iSegStart );

//		iDispStart = strSeg.find( C_STR_DISPSTART );

//		if( iDispStart == string :: npos )
//		{
//			UTIL_LogPrint( string( gmapLANG_CFG["decode_http_post_notfound"] + "\n" ).c_str( ) );

//			delete pList;

//			pList = 0;

//			return 0;
//		}

//		iDispStart += ( sizeof(C_STR_DISPSTART) - 1 );

//		iDispEnd = strSeg.find( C_STR_NEWLINE, iDispStart );

//		if( iDispEnd == string :: npos )
//		{
//			UTIL_LogPrint( string( gmapLANG_CFG["decode_http_post_disposition"] + "\n" ).c_str( ) );

//			delete pList;

//			pList = 0;

//			return 0;
//		}

//		strDisp = strSeg.substr( iDispStart, iDispEnd - iDispStart );

//		iDispPrev = 0;
//		iDispPos = 0;

//		pDisp = new CAtomDicti( );

//		pSegment->setItem( "disposition", pDisp );

//		bool bDoItemLoop = true;

//		while( bDoItemLoop )
//		{
//			// assume a semicolon indicates the end of the item and will never appear inside the item (probably a bad assumption)

//			iDispPrev = iDispPos;
//			iDispPos = strDisp.find( C_STR_ITEMEND, iDispPos );

//			if( iDispPos == string :: npos )
//			{
//				// decode last item

//				iDispPos = strDisp.size( );
//			}

//			strCurr = strDisp.substr( iDispPrev, iDispPos - iDispPrev );

//			iSplit = strCurr.find( C_STR_ITEMEQ );

//			if( iSplit == string :: npos )
//			{
//				// found a key without value, i.e. "form-data", useless so ignore it

//				if( iDispPos == strDisp.size( ) )
//					break;

//				// + strlen( ";" )

//				iDispPos++;

//				continue;
//			}

//			// strip whitespace

//			iKeyStart = strCurr.find_first_not_of( C_STR_WSPACE );

//			if( iKeyStart == string :: npos || iKeyStart > iSplit )
//			{
//				UTIL_LogPrint( string( gmapLANG_CFG["decode_http_post_disposition"] + "\n" ).c_str( ) );

//				delete pList;

//				pList = 0;

//				return 0;
//			}

//			strKey = strCurr.substr( iKeyStart, iSplit - iKeyStart );
//			strValue = strCurr.substr( iSplit + 1 );

//			// strip quotes

//			if( strValue.size( ) > 1 && strValue[0] == CHAR_QUOTE )
//				strValue = strValue.substr( 1, strValue.size( ) - 2 );

//			pDisp->setItem( strKey, new CAtomString( strValue ) );

//			if( iDispPos == strDisp.size( ) )
//				bDoItemLoop = false;

//			// + strlen( ";" )

//			iDispPos++;
//		}

//		// data

//		iDataStart = strSeg.find( C_STR_DOUBLE_NEWLINE );

//		if( iDataStart == string :: npos )
//		{
//			UTIL_LogPrint( string( gmapLANG_CFG["decode_http_post_segment"] + "\n" ).c_str( ) );

//			delete pList;

//			pList = 0;

//			return 0;
//		}

//		iDataStart += ( sizeof(C_STR_DOUBLE_NEWLINE) - 1 );

//		pSegment->setItem( "data", new CAtomString( strSeg.substr( iDataStart ) ) );
//	}

	// this should never happen, so who cares

	delete pList;

	pList = 0;

	return 0;
}

char gpBuf[GPBUF_SIZE];

CClient :: CClient( SOCKET &sckClient, struct sockaddr_in6 &sinAddress, const unsigned int &cuiTimeOut, const char &ccCompression )
{
	if( gbDebug )
		UTIL_LogPrint( "client: Socket Opening\n" );

	m_bFailed = false;

	m_sckClient = sckClient;

	// make socket non blocking

#ifdef WIN32
	u_long ulMode = 1;

	if( ioctlsocket( m_sckClient, FIONBIO, &ulMode ) == SOCKET_ERROR )
#else
	if( fcntl( m_sckClient, F_SETFL, fcntl( m_sckClient, F_GETFL ) | O_NONBLOCK ) == SOCKET_ERROR )
#endif
		UTIL_LogPrint( "client warning: socket blocking (error %s)\n", GetLastErrorString( ) );

	m_uiTimeOut = cuiTimeOut;
	m_cCompression = ccCompression;
	rqst.sin = sinAddress;
	char charIP6[INET6_ADDRSTRLEN];
	inet_ntop( AF_INET6, &rqst.sin.sin6_addr, charIP6, INET6_ADDRSTRLEN );
	string strIP = string( charIP6 );
	string :: size_type iStart = strIP.rfind( ":" );
	if( strIP.rfind( "." ) != string :: npos && strIP.find( "::ffff:" ) == 0 )
		strIP = strIP.substr( iStart + 1, sizeof( strIP )/sizeof( char ) - iStart - 1 );
	rqst.strIP = strIP;
	
	m_bBlocked = false;
	
	if( UTIL_IsIPBanList( rqst.strIP, gpServer->m_pIPBlockedList ) )
	{
		m_bBlocked = true;
		
		return;
	}
	
	epfd_client = epoll_create(128);
	
	ev.data.fd = m_sckClient;
					
	ev.events = EPOLLIN;

	if( epoll_ctl( epfd_client, EPOLL_CTL_ADD, m_sckClient, &ev ) == SOCKET_ERROR )
	{
		UTIL_LogPrint( "client warning: epoll_ctl add (error %s)\n", GetLastErrorString( ) );

		m_bFailed = true;
		
		return;
	}
		
	Reset( );

}

CClient :: ~CClient( )
{
	if( closesocket( m_sckClient ) == SOCKET_ERROR )
		UTIL_LogPrint( "client error: Closing Socket (error %s)\n", GetLastErrorString( ) );
	else if( gbDebug )
		UTIL_LogPrint( "client: Socket Closed\n" );
		
//	ev.data.fd = m_sckClient;
//					
//	ev.events = EPOLLIN | EPOLLOUT;

//	epoll_ctl( epfd_client, EPOLL_CTL_DEL, m_sckClient, &ev );
	
	close( epfd_client );
}

bool CClient :: Update( )
{
	if( m_bFailed )
	{
		if( gbDebug && ( gucDebugLevel & DEBUG_CLIENT ) )
                        UTIL_LogPrint( "client warning: epoll_ctl add failed\n" );

		return true;
        }
	
	if( m_bBlocked )
	{
		if( gbDebug && ( gucDebugLevel & DEBUG_CLIENT ) )
			UTIL_LogPrint( "client warning: %s blocked\n", rqst.strIP.c_str( ) );
			
		return true;
	}
	
	if( gbDebug && ( gucDebugLevel & DEBUG_CLIENT ) )
		UTIL_LogPrint( "client: update start\n" );

	if( GetTime( ) > m_ulLast + m_uiTimeOut )
	{
		if( gbDebug && ( gucDebugLevel & DEBUG_CLIENT ) )
			UTIL_LogPrint( "client warning: socket timed out (%d s, state(%d), reset(%d))\n", m_uiTimeOut, m_ucState, m_bReset );

		return true;
	}

	bool bGrabParams = true;
	bool bGrabHeaders = true;
	bool bGrabCookies = true;

	char cCompress = COMPRESS_NONE;
	char *szAuth = 0; 
	unsigned char *pBuf = 0;
	unsigned char *pNextIn = 0;

	unsigned char szMD5[16];

	string :: size_type iNewLine = 0;
	string :: size_type iDoubleNewLine = 0;
	string :: size_type iMethodEnd = 0;
	string :: size_type iParamsStart = 0;
	string :: size_type iURLEnd = 0;
	string :: size_type iWhite = 0;
	string :: size_type iSplit = 0;
	string :: size_type iEnd = 0;

	string strAuthorization = string( );
	string strContentLength = string( );
	string strCookies = string( );
	string strLogin = string( );
	string strUID = string( );
	string strMD5 = string( );
	string strRequest = string( );
	string strTemp = string( );
	string strType = string( );
	string strBase64 = string( );
	string strAuth = string( );
	string strPass = string( );
	string strA1 = string( );
	string strAcceptEncoding = string( );
	string strKey = string( );
	string strValue = string( );

	int iRecv = 0;
	int iSend = 0;
	int windowBits = 0;
	int iResult = 0;
	unsigned int uiSize = 0;
	
	switch( m_ucState )
	{
	case CS_RECVHEADERS :
	{
		if( gbDebug && ( gucDebugLevel & DEBUG_CLIENT ) )
			UTIL_LogPrint( "client: receive headers\n" );

		nfds = epoll_wait( epfd_client, events, 16, 0 );

		for( int i = 0; i < nfds; i++ )
		{
//			if(events[i].data.fd==m_sckClient && events[i].events & EPOLLIN)
//				UTIL_LogPrint( "client state: %d epoll in\n", m_sckClient );

			if( events[i].data.fd == m_sckClient && events[i].events & EPOLLIN )
			{
				m_ulLast = GetTime( );

				memset( gpBuf, 0, sizeof(gpBuf) / sizeof(char) );

				iRecv = recv( m_sckClient, gpBuf, sizeof(gpBuf), 0 );
				
//				UTIL_LogPrint( "client: receive %d\n", iRecv );

				if( iRecv == SOCKET_ERROR && GetLastError( ) != EWOULDBLOCK )
				{
					if( gbDebug && ( gucDebugLevel & DEBUG_CLIENT ) )
						UTIL_LogPrint( "client error: receive headers error (error %s)\n", GetLastErrorString( ) );
					else if( GetLastError( ) != EPIPE && GetLastError( ) != ECONNRESET )
						UTIL_LogPrint( "client error: receive headers error (error %s)\n", GetLastErrorString( ) );

					return true;
				}
				else if( iRecv == 0 )
				{
					if( gbDebug && ( gucDebugLevel & DEBUG_CLIENT ) )
						UTIL_LogPrint( "client error: receive headers returned 0\n" );

					return true;
				}
				else if( iRecv > 0 )
				{
					m_strReceiveBuf += string( gpBuf, iRecv );

					if( m_strReceiveBuf.size( ) > guiMaxRecvSize )
					{
						UTIL_LogPrint( "client error: exceeded max receive header size\n" );

						return true;
					}

					gtXStats.tcp.iRecv += iRecv;
				}
				else
				{
					UTIL_LogPrint( "client error: receive header returned garbage\n" );

					return true;
				}
			}
		}

		if( m_strReceiveBuf.find( C_STR_DOUBLE_NEWLINE ) != string :: npos )
			m_ucState = CS_PROCESSHEADERS;
		else
			break;
	}
	case CS_PROCESSHEADERS:
	{
		if( gbDebug && ( gucDebugLevel & DEBUG_CLIENT ) )
			UTIL_LogPrint( "client: process headers\n" );

		// grab method

		iMethodEnd = m_strReceiveBuf.find( C_STR_WSPACE );

		if( iMethodEnd == string :: npos )
		{
			UTIL_LogPrint( "client error: malformed HTTP request (can't find method)\n" );

			return true;
		}

		rqst.strMethod = m_strReceiveBuf.substr( 0, iMethodEnd );

		// grab url

		strTemp = m_strReceiveBuf.substr( iMethodEnd + 1 );

		iURLEnd = strTemp.find( C_STR_WSPACE );

		if( iURLEnd == string :: npos )
		{
			UTIL_LogPrint( "client error: malformed HTTP request (can't find URL)\n" );

			return true;
		}

		strTemp = strTemp.substr( 0, iURLEnd );

		string strTemp2( strTemp );

		iParamsStart = strTemp.find( C_STR_ITEMQU );

		if( iParamsStart == string :: npos )
		{
			rqst.strURL = strTemp;
			rqst.hasQuery = false;
		}
		else
		{
			rqst.strURL = strTemp.substr( 0, iParamsStart );
			rqst.hasQuery = true;
		}

		// grab params

		if( iParamsStart != string :: npos )
		{
			strTemp = strTemp.substr( iParamsStart + 1 );
			rqst.hasQuery = true;

			iSplit = 0;
			iEnd = 0;

			strKey = string( );
			strValue = string( );

			bGrabParams = true;

			while( bGrabParams )
			{
				iSplit = strTemp.find( C_STR_ITEMEQ );
				iEnd = strTemp.find( C_STR_ITEMAND );

				if( iSplit == string :: npos )
				{
					UTIL_LogPrint( "client warning: malformed HTTP request (found param key without value)\n" );
					UTIL_LogPrint( "%s %s\n", rqst.strIP.c_str( ), strTemp2.c_str( ) );

					bGrabParams = false;

					continue;
				}

				strKey = UTIL_EscapedToString( strTemp.substr( 0, iSplit ) );
				strValue = UTIL_EscapedToString( strTemp.substr( iSplit + 1, iEnd - iSplit - 1 ) );

				// multimap for scrape, regular map for everything else
				if ( rqst.strURL == RESPONSE_STR_SCRAPE )
					rqst.multiParams.insert( pair<string,string>( strKey, strValue ) );
				else
					rqst.mapParams.insert( pair<string, string>( strKey, strValue ) );

				strTemp = strTemp.substr( iEnd + 1, strTemp.size( ) - iEnd - 1 );

				if( iEnd == string :: npos )
					bGrabParams = false;
			}
		}

		// grab headers

		iNewLine = m_strReceiveBuf.find( C_STR_NEWLINE );
		iDoubleNewLine = m_strReceiveBuf.find( C_STR_DOUBLE_NEWLINE );

		if( iNewLine != iDoubleNewLine )
		{
			strTemp = m_strReceiveBuf.substr( iNewLine + ( sizeof(C_STR_NEWLINE) - 1 ), iDoubleNewLine - iNewLine - ( sizeof(C_STR_NEWLINE) - 1 ) );

			iSplit = 0;
			iEnd = 0;

			strKey = string( );
			strValue = string( );

			bGrabHeaders = true;

			while( bGrabHeaders )
			{
				iSplit = strTemp.find( C_STR_COLON );
				iEnd = strTemp.find( C_STR_NEWLINE );

				// http://www.addict3d.org/index.php?page=viewarticle&type=security&ID=4861
				if( iSplit == string :: npos || iSplit == 0 )
				{
					UTIL_LogPrint( "client warning: malformed HTTP request (bad header)\n" );

					bGrabHeaders = false;
				}

				strKey = strTemp.substr( 0, iSplit );
				strValue = strTemp.substr( iSplit + ( sizeof(": ") - 1 ), iEnd - iSplit - ( sizeof(C_STR_NEWLINE) - 1 ) );

				rqst.mapHeaders.insert( pair<string, string>( strKey, strValue ) );

				strTemp = strTemp.substr( iEnd + ( sizeof(C_STR_NEWLINE) - 1 ) );

				if( iEnd == string :: npos )
					bGrabHeaders = false;
			}
		}

		// grab cookies

		strCookies = rqst.mapHeaders["Cookie"];

		if( !strCookies.empty( ) )
		{
			iWhite = 0;

			iSplit = 0;
			iEnd = 0;		

			strKey = string( );
			strValue = string( );

			bGrabCookies = true;

			while( bGrabCookies )
			{
				iWhite = strCookies.find_first_not_of( C_STR_WSPACE );

				if( iWhite != string :: npos )
					strCookies = strCookies.substr( iWhite );

				iSplit = strCookies.find( C_STR_ITEMEQ );
				iEnd = strCookies.find( C_STR_SEMICOLON );

				if( iSplit == string :: npos )
				{
					UTIL_LogPrint( "client warning: malformed HTTP request (found cookie key without value)\n" );


					bGrabCookies = false;
				}

				strKey = UTIL_EscapedToString( strCookies.substr( 0, iSplit ) );
				strValue = UTIL_EscapedToString( strCookies.substr( iSplit + 1, iEnd - iSplit - 1 ) );

				// strip quotes

				if( strValue.size( ) > 1 && strValue[0] == CHAR_QUOTE )
					strValue = strValue.substr( 1, strValue.size( ) - 2 );

				rqst.mapCookies.insert( pair<string, string>( strKey, strValue ) );

				strCookies = strCookies.substr( iEnd + 1, strCookies.size( ) - iEnd - 1 );

				if( iEnd == string :: npos )
					bGrabCookies = false;
			}
		}

		// grab authentication
		strLogin = rqst.mapCookies["login"];
		strUID = rqst.mapCookies["uid"];
		strMD5 = rqst.mapCookies["md5"];

		strAuthorization = rqst.mapHeaders["Authorization"];

		if( !strAuthorization.empty( ) )
		{
			iWhite = strAuthorization.find( C_STR_WSPACE );

			if( iWhite != string :: npos )
			{
				strType = strAuthorization.substr( 0, iWhite );
				strBase64 = strAuthorization.substr( iWhite + 1 );

				if( strType == "Basic" )
				{
					szAuth = b64decode( strBase64.c_str( ) );

					if( szAuth )
					{
						strAuth = szAuth;

						free( szAuth );

						iSplit = strAuth.find( C_STR_COLON );

						if( iSplit != string :: npos )
						{
							strLogin = strAuth.substr( 0, iSplit );
							strPass = strAuth.substr( iSplit + 1 );

							// Original code
							// calculate md5 hash of A1

							strA1 = strLogin + C_STR_COLON + gstrPasswordKey + C_STR_COLON + strPass;

							memset( szMD5, 0, sizeof(szMD5) / sizeof(unsigned char) );

							MD5_CTX md5;

							MD5Init( &md5 );
							MD5Update( &md5, (const unsigned char *)strA1.c_str( ), (unsigned int)strA1.size( ) );
							MD5Final( szMD5, &md5 );

							strMD5 = string( (char *)szMD5, sizeof(szMD5) / sizeof(unsigned char) );
							
							strUID = gpServer->getTracker( )->checkUserMD5( strLogin, strMD5 );
						}
					}
				}
			}
		}

		rqst.user = gpServer->getTracker( )->checkUser( strUID, strMD5 );

		if( rqst.strMethod == "POST" )
			m_ucState = CS_RECVBODY;
		else
		{
			m_ucState = CS_MAKERESPONSE;

			break;
		}
	}
	case CS_RECVBODY:
	{
		if( gbDebug && ( gucDebugLevel & DEBUG_CLIENT ) )
			UTIL_LogPrint( "client: receive body\n" );

		strContentLength = rqst.mapHeaders["Content-Length"];

		if( strContentLength.empty( ) )
		{
			UTIL_LogPrint( "client error: malformed HTTP request (no Content-Length with POST)\n" );

			return true;
		}

		nfds = epoll_wait( epfd_client, events, 16, 0 );

		for( int i = 0; i < nfds; i++ )
		{
			if( events[i].data.fd == m_sckClient && events[i].events & EPOLLIN )
			{
				m_ulLast = GetTime( );

				memset( gpBuf, 0, sizeof(gpBuf) / sizeof(char) );

				iRecv = recv( m_sckClient, gpBuf, sizeof(gpBuf), 0 );

				if( iRecv == SOCKET_ERROR && GetLastError( ) != EWOULDBLOCK )
				{
					if( gbDebug && ( gucDebugLevel & DEBUG_CLIENT ) )
						UTIL_LogPrint( "client error: receive body error (error %s)\n", GetLastErrorString( ) );
					else if( GetLastError( ) != EPIPE && GetLastError( ) != ECONNRESET )
						UTIL_LogPrint( "client error: receive body error (error %s)\n", GetLastErrorString( ) );

					return true;
				}
				else if( iRecv == 0 )
				{
					UTIL_LogPrint( "client error: receive body returned 0\n" );

					return true;
				}
				else if( iRecv > 0 )
				{
					m_strReceiveBuf += string( gpBuf, iRecv );

					if( m_strReceiveBuf.size( ) > guiMaxRecvSize )
					{
						UTIL_LogPrint( "client error: exceeded max receive body size\n" );

						return true;
					}

					gtXStats.tcp.iRecv += iRecv;
				}
				else
				{
					UTIL_LogPrint( "client error: receive body returned garbage\n" );

					return true;
				}
			}
		}

		if( m_strReceiveBuf.size( ) >= m_strReceiveBuf.find( C_STR_DOUBLE_NEWLINE ) + ( sizeof(C_STR_DOUBLE_NEWLINE) - 1 ) + atol( strContentLength.c_str( ) ) )
			m_ucState = CS_MAKERESPONSE;
		else
			break;
	}
	case CS_MAKERESPONSE:
	{
		if( gbDebug )
			UTIL_LogPrint( "client - make response\n" );

		if( rqst.strMethod == "GET" )
			gpServer->getTracker( )->serverResponseGET( &rqst, &rsp );
		else if( rqst.strMethod == "POST" )
		{
			CAtomList *pPost = 0;
			
			if( rqst.mapHeaders["Content-Type"].find( "application/x-www-form-urlencoded" ) != string :: npos )
				pPost = UTIL_DecodeHTTPPostUrlencoded( m_strReceiveBuf );
			else
				pPost = UTIL_DecodeHTTPPost( m_strReceiveBuf );

			gpServer->getTracker( )->serverResponsePOST( &rqst, &rsp, pPost );

			if( pPost )
				delete pPost;
		}
		else
			rsp.strCode = "400 Bad Request";

		// compress

		cCompress = COMPRESS_NONE;

		if( rsp.bCompressOK && m_cCompression > 0 )
		{
			strAcceptEncoding = UTIL_ToLower( rqst.mapHeaders["Accept-Encoding"] );

			if( strAcceptEncoding.find( "gzip" ) != string :: npos )
				cCompress = COMPRESS_GZIP;
			else if( strAcceptEncoding.find( "deflate" ) != string :: npos )
				cCompress = COMPRESS_DEFLATE;
		}

		if( !rsp.strContent.empty( ) && cCompress != COMPRESS_NONE )
		{
			// allocate avail_in * 1.001 + 18 bytes (12 + 6 for gzip)

			uiSize = (unsigned int)( rsp.strContent.size( ) * 1.001 + 18 );

			pBuf = new unsigned char[uiSize];
			memset( pBuf, 0, sizeof(pBuf) / sizeof(unsigned char) );

			z_stream_s zCompress;

			//unsigned char
			pNextIn = new unsigned char[uiSize];
			memset( pNextIn, 0, sizeof(pNextIn) / sizeof(unsigned char) );
			snprintf( (char *)pNextIn, uiSize / sizeof(unsigned char), "%s", rsp.strContent.c_str( ) );

			zCompress.next_in = pNextIn;
			zCompress.avail_in = (unsigned int)rsp.strContent.size( );
			zCompress.next_out = pBuf;
			zCompress.avail_out = uiSize;
			zCompress.zalloc = (alloc_func)0;
			zCompress.zfree = (free_func)0;
			zCompress.opaque = (voidpf)0;
			zCompress.total_in = 0;
			zCompress.total_out = 0;

			windowBits = 15;

			if( cCompress == COMPRESS_GZIP )
				windowBits = 31;

			iResult = deflateInit2( &zCompress, m_cCompression, Z_DEFLATED, windowBits, 8, Z_DEFAULT_STRATEGY );

			if( iResult == Z_OK )
			{
				iResult = deflate( &zCompress, Z_FINISH );

				if( iResult == Z_STREAM_END )
				{
					if( zCompress.total_in > zCompress.total_out )
					{
						if( gbDebug )
							UTIL_LogPrint( "client: (zlib) compressed %lu bytes to %lu bytes\n", zCompress.total_in, zCompress.total_out );

						if( cCompress == COMPRESS_GZIP )
							rsp.mapHeaders.insert( pair<string, string>( "Content-Encoding", "gzip" ) );
						else
							rsp.mapHeaders.insert( pair<string, string>( "Content-Encoding", "deflate" ) );

						rsp.strContent = string( (char *)pBuf, zCompress.total_out );
					}

					deflateEnd( &zCompress );

					delete [] pBuf;
				}
				else
				{
					if( iResult != Z_OK )
						UTIL_LogPrint( "client warning: (zlib) deflate error (%d) on \"%s\", in = %u, sending raw\n", iResult, rqst.strURL.c_str( ), rsp.strContent.size( ) );

					deflateEnd( &zCompress );

					delete [] pBuf;
				}
			}
			else
			{
				UTIL_LogPrint( "client warning: (zlib) deflateInit2 error (%d), sending raw\n", iResult );

				delete [] pBuf;
			}

			delete [] pNextIn;
		}

		// keep alive

		if( UTIL_ToLower( rqst.mapHeaders["Connection"] ) == "keep-alive" )
		{
			m_bKeepAlive = true;

			rsp.mapHeaders.insert( pair<string, string>( "Connection", "Keep-Alive" ) );
			rsp.mapHeaders.insert( pair<string, string>( "Keep-Alive", CAtomInt( m_uiTimeOut - 1 ).toString( ) ) );
		}
		else
		{
			m_bKeepAlive = false;

			rsp.mapHeaders.insert( pair<string, string>( "Connection", "Close" ) );
		}

		rsp.mapHeaders.insert( pair<string, string>( "Content-Length", CAtomLong( rsp.strContent.size( ) ).toString( ) ) );

		// access log

		strRequest = string( );

		iNewLine = m_strReceiveBuf.find( C_STR_NEWLINE );

		if( iNewLine != string :: npos )
			strRequest = m_strReceiveBuf.substr( 0, iNewLine );

		UTIL_AccessLogPrint( rqst.strIP.c_str( ), rqst.user.strLogin, strRequest, atoi( rsp.strCode.substr( 0, 3 ).c_str( ) ), (int)rsp.strContent.size( ) );

		// compose send buffer

		// fix for \r\n issues with non-tolerant HTTP client implementations - DWK
		m_strSendBuf += "HTTP/1.1 " + rsp.strCode + "\r\n";

		for( multimap<string, string> :: iterator it = rsp.mapHeaders.begin( ); it != rsp.mapHeaders.end( ); it++ )
			m_strSendBuf += (*it).first + ": " + (*it).second + "\r\n";

		m_strSendBuf += "\r\n";
		m_strSendBuf += rsp.strContent;

		m_ucState = CS_SEND;

		ev.data.fd = m_sckClient;

		ev.events = EPOLLOUT;

		if( epoll_ctl( epfd_client, EPOLL_CTL_MOD, m_sckClient, &ev ) == SOCKET_ERROR )
		{
			UTIL_LogPrint( "client warning: epoll_ctl mod (error %s)\n", GetLastErrorString( ) );
		
			return true;
		}
	}
	case CS_SEND:
	{
		if( gbDebug )
			UTIL_LogPrint( "client - send\n" );

		nfds = epoll_wait( epfd_client, events, 16, 0 );

		for( int i = 0; i < nfds; i++ )
		{
			if( events[i].data.fd == m_sckClient && events[i].events & EPOLLOUT )
			{
				m_ulLast = GetTime( );

				iSend = send( m_sckClient, m_strSendBuf.c_str( ), (int)m_strSendBuf.size( ), MSG_NOSIGNAL );

				if( iSend == SOCKET_ERROR && GetLastError( ) != EWOULDBLOCK )
				{
					if( gbDebug && ( gucDebugLevel & DEBUG_CLIENT ) )
						UTIL_LogPrint( "client error: send error (error %s)\n", GetLastErrorString( ) );
					else if( GetLastError( ) != EPIPE && GetLastError( ) != ECONNRESET )
						UTIL_LogPrint( "client error: send error (error %s)\n", GetLastErrorString( ) );

					return true;
				}
				else if( iSend == 0 )
				{
					UTIL_LogPrint( "client error: send returned 0\n" );

					return true;
				}
				else if( iSend > 0 )
				{
					m_strSendBuf = m_strSendBuf.substr( iSend );

					gtXStats.tcp.iSend += iSend;

					if( m_strSendBuf.empty( ) )
					{
						if( m_bKeepAlive )
						{
							if( gbDebug && ( gucDebugLevel & DEBUG_CLIENT ) )
								UTIL_LogPrint( "client: keep alive\n" );

							Reset( );
							
							ev.data.fd = m_sckClient;
					
							ev.events = EPOLLIN;

							if( epoll_ctl( epfd_client, EPOLL_CTL_MOD, m_sckClient, &ev ) == SOCKET_ERROR )
							{
								UTIL_LogPrint( "client warning: epoll_ctl mod (error %s)\n", GetLastErrorString( ) );

								return true;
							}

							return false;
						}
						else
							return true;
					}
				}
				else
				{
					UTIL_LogPrint( "client error: send returned garbage\n" );

					return true;
				}
			}
		}

		break;
	}
	default:
	{
		UTIL_LogPrint( "client error: unknown state\n" );
	}
	}

	if( gbDebug && ( gucDebugLevel & DEBUG_CLIENT ) )
		UTIL_LogPrint( "client: update end\n" );

	m_bReset = false;
	return false;
}

void CClient :: Reset( )
{
	if( gbDebug && ( gucDebugLevel & DEBUG_CLIENT ) )
		UTIL_LogPrint( "client: reset\n" );

	m_ucState = CS_RECVHEADERS;
	m_strReceiveBuf.erase( );
	m_strReceiveBuf.reserve( 1024 );
	m_strSendBuf.erase( );
	m_strSendBuf.reserve( 1024 );
	rqst.strMethod.erase( );
	rqst.strURL.erase( );
	rqst.mapParams.clear( );
	rqst.mapHeaders.clear( );
	rqst.mapCookies.clear( );
	rqst.user.strUID.erase( );
	rqst.user.strLogin.erase( );
//	rqst.user.strLowerLogin.erase( );
	rqst.user.strMD5.erase( );
//	rqst.user.strMail.erase( );
//	rqst.user.strLowerMail.erase( );
//	rqst.user.strCreated.erase( );
//	rqst.user.strPasskey.erase( );
	rqst.user.ucAccess = 0;
	rqst.user.ucGroup = 0;
//	rqst.user.strTitle.erase( );
//	rqst.user.ulUploaded = 0;
//	rqst.user.ulDownloaded = 0;
//	rqst.user.ulBonus = 0;
//	rqst.user.strIP.erase( );
//	rqst.user.ulSeeding = 0;
//	rqst.user.ulLeeching = 0;
//	rqst.user.strSeeding.erase( );
//	rqst.user.strLeeching.erase( );
//	rqst.user.flSeedBonus = 0;
//	rqst.user.flShareRatio = 0;
//	rqst.user.tWarned = 0;
//	rqst.user.tLast = 0;
//	rqst.user.tLast_Index = 0;
//	rqst.user.tLast_Info = 0;
//	rqst.user.strInvites.erase( );
//	rqst.user.strInviter.erase( );
//	rqst.user.strInviterID.erase( );
//	rqst.user.strTalk.erase( );
//	rqst.user.strTalkRef.erase( );
	rsp.strCode.erase( );
	rsp.mapHeaders.clear( );
	rsp.strContent.erase( );
	rsp.strContent.reserve( 1024 );
	rsp.bCompressOK = true;
	m_ulLast = GetTime( );
	m_bReset = true;
}
