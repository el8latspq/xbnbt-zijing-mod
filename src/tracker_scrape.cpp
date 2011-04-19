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
#include "bnbt_mysql.h"
#include "atom.h"
#include "bencode.h"
#include "config.h"
#include "tracker.h"
#include "util.h"

void CTracker :: serverResponseScrape( struct request_t *pRequest, struct response_t *pResponse )
{
	if( gbDebug )
		if( gucDebugLevel & DEBUG_SCRAPE )
			UTIL_LogPrint( "serverResponseScrape: started\n" );

	// Is scrape allowed?
	if( !m_bAllowScrape )
	{
		if( gbDebug )
			if( gucDebugLevel & DEBUG_SCRAPE )
				UTIL_LogPrint( "serverResponseScrape: scrape (disabled)\n" );

		pResponse->strCode = "403 " + gmapLANG_CFG["server_response_403"];

		gtXStats.scrape.iDisallowed++;

		if( gbDebug )
			if( gucDebugLevel & DEBUG_SCRAPE )
				UTIL_LogPrint( "serverResponseScrape: completed\n" );

		return;
	}

	// Authorise scrape
	if( m_ucAuthScrapeAccess != 0 )
	{
		if( gbDebug )
			if( gucDebugLevel & DEBUG_SCRAPE )
				UTIL_LogPrint( "serverResponseScrape: authorise (enabled)\n" );

		if( !( pRequest->user.ucAccess & m_ucAuthScrapeAccess ) )
		{
			pResponse->strCode = "401 " + gmapLANG_CFG["server_response_401"];
			pResponse->mapHeaders.insert( pair<string, string>( "Content-Type", gmapMime[".txt"] ) );
			pResponse->mapHeaders.insert( pair<string, string>( "WWW-Authenticate", string( "Basic realm=\"" ) + gstrRealm + "\"" ) );
			pResponse->strContent = UTIL_FailureReason( gmapLANG_CFG["server_response_401"] );
			pResponse->bCompressOK = false;

			gtXStats.scrape.iNotAuthorized++;

			if( gbDebug )
			{
				if( gucDebugLevel & DEBUG_SCRAPE )
				{
					UTIL_LogPrint( "serverResponseScrape: authorisation failed\n" );
					UTIL_LogPrint( "serverResponseScrape: completed\n" );
				}
			}

			return;
		}

		if( gbDebug )
			if( gucDebugLevel & DEBUG_SCRAPE )
				UTIL_LogPrint( "serverResponseScrape: authorisation passed\n" );
	}

	// Set the HTTP code, headers and compession
	pResponse->strCode = "200 " + gmapLANG_CFG["server_response_200"];
	pResponse->mapHeaders.insert( pair<string, string>( "Content-Type", gmapMime[".txt"] ) );
	pResponse->bCompressOK = true;

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0)
	{
		if( gbDebug )
			if( gucDebugLevel & DEBUG_SCRAPE )
				UTIL_LogPrint( "serverResponseScrape: IP banning (enabled)\n" );

		// retrieve ip

		const string strTempIP( pRequest->mapParams["ip"] );

		const string cstrIPConv( pRequest->strIP.c_str( ) );

		switch( m_ucIPBanMode )
		{
		case IP_BLACKLIST:
			if( UTIL_IsIPBanList( pRequest->strIP, m_pIPBannedList ) || UTIL_IsIPBanList( strTempIP, m_pIPBannedList ) )
			{
				if( gbDebug )
					if( gucDebugLevel & DEBUG_SCRAPE )
						UTIL_LogPrint( "serverResponseScrape: IP blacklisted\n" );
				
				pResponse->strContent = UTIL_FailureReason( gmapLANG_CFG["announce_ip_banned"] );
				pResponse->bCompressOK = false;

				gtXStats.scrape.iNotAuthorized++;

				if( gbDebug )
					if( gucDebugLevel & DEBUG_SCRAPE )
						UTIL_LogPrint( "serverResponseScrape: completed\n" );

				return;
			}

			break;
		case IP_VIPLIST:
			if( UTIL_IsIPBanList( pRequest->strIP, m_pIPBannedList ) || UTIL_IsIPBanList( strTempIP, m_pIPBannedList ) )
			{
				if( gbDebug )
					if( gucDebugLevel & DEBUG_SCRAPE )
						UTIL_LogPrint( "serverResponseScrape: IP not cleared\n" );

				pResponse->strContent = UTIL_FailureReason( gmapLANG_CFG["announce_ip_not_cleared"] );
				pResponse->bCompressOK = false;

				gtXStats.scrape.iNotAuthorized++;

				if( gbDebug )
					if( gucDebugLevel & DEBUG_SCRAPE )
						UTIL_LogPrint( "serverResponseScrape: completed\n" );

				return;
			}

			break;
		default:
			if( gbDebug )
				if( gucDebugLevel & DEBUG_SCRAPE )
					UTIL_LogPrint( "serverResponseScrape: IP banning method unknown (disabled)\n" );
		}
	}
	else
	{
		if( gbDebug )
			if( gucDebugLevel & DEBUG_SCRAPE )
				UTIL_LogPrint( "serverResponseScrape: IP banning (disabled)\n" );
	}

	// Peer ID/User Agent banning
	if( m_ucBanMode != 0 )
	{
		if( gbDebug )
			if( gucDebugLevel & DEBUG_SCRAPE )
				UTIL_LogPrint( "serverResponseScrape: user agent banning (enabled)\n" );

		string strUserAgent = string( );

		const string strUserAgentA( pRequest->mapHeaders["User-Agent"] );
		const string strUserAgentB( pRequest->mapHeaders["User-agent"] );
		const string cstrPeerID( pRequest->mapParams["peer_id"] );

		if( strUserAgentA.empty( ) && strUserAgentB.empty( ) )
			strUserAgent = string( );
		else if( strUserAgentA == strUserAgentB )
			strUserAgent = strUserAgentA;
		else
		{
			if( !strUserAgentA.empty( ) && strUserAgentB.empty( ) )
				strUserAgent = strUserAgentA;
			else if( strUserAgentA.empty( ) && !strUserAgentB.empty( ) )
				strUserAgent = strUserAgentB;
			else if( !strUserAgentA.empty( ) && !strUserAgentB.empty( ) )
				strUserAgent = strUserAgentB + "; spoof( " + strUserAgentA + " )";
		}

		bool bClientBanned = UTIL_IsClientBanList( strUserAgent, m_pClientBannedList, true );

		if( !bClientBanned )
			bClientBanned = UTIL_IsClientBanList( cstrPeerID, m_pClientBannedList, false );

		switch( m_ucBanMode )
		{
		case ID_BLACKLIST:
			if ( bClientBanned )
			{
				if( gbDebug )
					if( gucDebugLevel & DEBUG_SCRAPE )
						UTIL_LogPrint( "serverResponseScrape: ID or user agent blacklisted\n" );

				pResponse->strContent = UTIL_FailureReason( gmapLANG_CFG["announce_client_banned"] );
				pResponse->bCompressOK = false;

				gtXStats.scrape.iNotAuthorized++;

				if( gbDebug )
					if( gucDebugLevel & DEBUG_SCRAPE )
						UTIL_LogPrint( "serverResponseScrape: completed\n" );

				return;
			}

			break;
		case ID_VIPLIST:
			if ( bClientBanned )
			{
				if( gbDebug )
					if( gucDebugLevel & DEBUG_SCRAPE )
						UTIL_LogPrint( "serverResponseScrape: ID or user agent not cleared\n" );

				pResponse->strContent = UTIL_FailureReason( gmapLANG_CFG["announce_client_banned"] );
				pResponse->bCompressOK = false;

				gtXStats.scrape.iNotAuthorized++;

				if( gbDebug && ( gucDebugLevel & DEBUG_SCRAPE ) )
					UTIL_LogPrint( "serverResponseScrape: completed\n" );

				return;
			}

			break;
		default:
			if( gbDebug )
				if( gucDebugLevel & DEBUG_SCRAPE )
					UTIL_LogPrint( "serverResponseScrape: ID or user agent banning mode unknown (disabled)\n" );
		}
	}
	else
	{
		if( gbDebug )
			if( gucDebugLevel & DEBUG_SCRAPE )
				UTIL_LogPrint( "serverResponseScrape: ID or user agent banning (disabled)\n" );
	}

	// Refresh the fast cache
//	if( GetTime( ) > m_ulRefreshFastCacheNext )
//	{
//		// Refresh
//		RefreshFastCache( );

//		// Set the next refresh time
//		m_ulRefreshFastCacheNext = GetTime( ) + m_uiRefreshFastCacheInterval;
//	}

	// Create the dictionaries
	CAtomDicti *pScrape = new CAtomDicti( );
	CAtomDicti *pFiles = new CAtomDicti( );
	CAtomDicti *pFlags = new CAtomDicti( );

	// Add the files and flags dictionaries to the scrape dictionary
	pScrape->setItem( "files", pFiles );
	pScrape->setItem( "flags", pFlags );

	// Set the minimum scrape request interval flag
	pFlags->setItem( "min_request_interval", new CAtomLong( m_uiMinRequestInterval ) );
	
	CAtomDicti *pTorrent = 0;

	CMySQLQuery *pQuery = 0;

	vector<string> vecQuery;
	vecQuery.reserve( 5 );
    
	if ( !pRequest->hasQuery )
	{
		//
		// full scrape
		//

		if( gbDebug )
			if( gucDebugLevel & DEBUG_SCRAPE )
				UTIL_LogPrint( "serverResponseScrape: no query, full scrape\n" );

		pQuery = new CMySQLQuery( "SELECT bseeders,bleechers,bcompleted,bhash,bname FROM allowed" );

		vecQuery = pQuery->nextRow( );

		while( vecQuery.size( ) == 5 )
		{
			pTorrent = new CAtomDicti( );

			pTorrent->setItem( "complete", new CAtomInt( atoi( vecQuery[0].c_str( ) ) ) );
			pTorrent->setItem( "incomplete", new CAtomInt( atoi( vecQuery[1].c_str( ) ) ) );
			pTorrent->setItem( "downloaded", new CAtomInt( atoi( vecQuery[2].c_str( ) ) ) );

			if( !vecQuery[4].empty( ) )
				pTorrent->setItem( "name", new CAtomString( vecQuery[4] ) );

			pFiles->setItem( vecQuery[3], pTorrent );

			vecQuery = pQuery->nextRow( );
		}

		delete pQuery;

		pResponse->strContent = Encode( pScrape );

		delete pScrape;

		gtXStats.scrape.iFull++;

		if( gbDebug )
		{
			if( gucDebugLevel & DEBUG_SCRAPE )
			{
				UTIL_LogPrint( "serverResponseScrape: %s\n", pResponse->strContent.c_str( ) );
				UTIL_LogPrint( "serverResponseScrape: completed\n" );
			}
		}

		return;
	}
	else
	{
		//
		// single scrape
		//

		if( gbDebug )
			if( gucDebugLevel & DEBUG_SCRAPE )
				UTIL_LogPrint( "serverResponseScrape: has a query\n" );

		// Initialise the mySQL query string before the loop
		string strQuery = string( );

		unsigned int uiCount = 0;

		// Begin the multi hash scrape loop
		for( multimap<string, string> :: iterator itInfoHash = pRequest->multiParams.lower_bound( "info_hash" ); itInfoHash != pRequest->multiParams.upper_bound( "info_hash" ); itInfoHash++ )
		{
			strQuery = "SELECT bseeders,bleechers,bcompleted,bname FROM allowed WHERE bhash=\'" + UTIL_StringToMySQL( (*itInfoHash).second.c_str( ) ) + "\'";

			pQuery = new CMySQLQuery( strQuery );
			
			vecQuery.reserve(4);
			
			vecQuery = pQuery->nextRow( );

			if( vecQuery.size( ) == 4 )
			{
				pTorrent = new CAtomDicti( );

				pTorrent->setItem( "complete", new CAtomInt( atoi( vecQuery[0].c_str( ) ) ) );
				pTorrent->setItem( "incomplete", new CAtomInt( atoi( vecQuery[1].c_str( ) ) ) );
				pTorrent->setItem( "downloaded", new CAtomInt( atoi( vecQuery[2].c_str( ) ) ) );
				
				if( !vecQuery[3].empty( ) )
					pTorrent->setItem( "name", new CAtomString( vecQuery[3] ) );

				pFiles->setItem( (*itInfoHash).second , pTorrent );
			}

			delete pQuery;

			uiCount++;
		}

		pResponse->strContent = Encode( pScrape );

		delete pScrape;

		if( gbDebug )
		{
			if( gucDebugLevel & DEBUG_SCRAPE )
			{
				UTIL_LogPrint( "serverResponseScrape: torrents scraped(%u)\n", uiCount );
				UTIL_LogPrint( "serverResponseScrape: %s\n", pResponse->strContent.c_str( ) );
			}
		}

		if( uiCount == 1 )
			gtXStats.scrape.iSingle++;
		else if( uiCount > 1 )
			gtXStats.scrape.iMultiple++;
	}

	if( gbDebug )
		if( gucDebugLevel & DEBUG_SCRAPE ) 
			UTIL_LogPrint( "serverResponseScrape: completed\n" );
}
