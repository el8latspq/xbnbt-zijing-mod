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
#include "link.h"  
#include "tracker.h"
#include "util.h"	

void CTracker :: serverResponseAnnounce( struct request_t *pRequest,struct response_t *pResponse )
{
	// Authorise announce?
	if( m_ucAuthAnnounceAccess != 0 )
	{
		if( gbDebug && ( gucDebugLevel & DEBUG_ANNOUNCE ) )
			UTIL_LogPrint( "serverResponseAnnounce: authorise (enabled)\n" );

		if( !( pRequest->user.ucAccess & m_ucAuthAnnounceAccess ) )
		{
			pResponse->strCode = "401 " + gmapLANG_CFG["server_response_401"];
			pResponse->mapHeaders.insert( pair<string, string>( "Content-Type", gmapMime[".txt"] ) );
			pResponse->mapHeaders.insert( pair<string, string>( "WWW-Authenticate", string( "Basic realm=\"" ) + gstrRealm + "\"" ) );
			pResponse->strContent = UTIL_FailureReason( gmapLANG_CFG["server_response_401"] );
			pResponse->bCompressOK = false;

			gtXStats.announce.iNotAuthorized++;

			if( gbDebug && ( gucDebugLevel & DEBUG_ANNOUNCE ) )
			{
				UTIL_LogPrint( "serverResponseAnnounce: authorisation failed\n" );
				UTIL_LogPrint( "serverResponseAnnounce: completed\n" );
			}

			return;
		}

		if( gbDebug && ( gucDebugLevel & DEBUG_ANNOUNCE ) )
			UTIL_LogPrint( "serverResponseAnnounce: authorisation passed\n" );
	}

	// Set the HTML response
	pResponse->strCode = "200 " + gmapLANG_CFG["server_response_200"];
	pResponse->mapHeaders.insert( pair<string, string>( "Content-Type", gmapMime[".txt"] ) );
	pResponse->bCompressOK = true;

	// retrieve the info hash
	const string cstrInfoHash( pRequest->mapParams["info_hash"] );

	// hash is missing
	if( cstrInfoHash.empty( ) )
	{
		pResponse->strContent = UTIL_FailureReason( gmapLANG_CFG["announce_hash_missing"] );
		pResponse->bCompressOK = false;

		gtXStats.announce.iAnnMissing++;

		if( gbDebug && ( gucDebugLevel & DEBUG_ANNOUNCE ) )
			UTIL_LogPrint( "serverResponseAnnounce: hash is missing\n" );

		return;
	}

	// not matched
	if( m_pAllowed )
	{
		if( !m_pAllowed->getItem( cstrInfoHash ) )
		{
			pResponse->strContent = UTIL_FailureReason( gmapLANG_CFG["announce_download_not_authorized"] );
			pResponse->bCompressOK = false;

			gtXStats.announce.iAnnNotAuth++;

			if( gbDebug && ( gucDebugLevel & DEBUG_ANNOUNCE ) )
				UTIL_LogPrint( "serverResponseAnnounce: hash not matched\n" );

			return;
		}
	}

	string strIP = inet_ntoa( pRequest->sin.sin_addr );
	string strTempIP = string( );
	string strIPConv = string( );

	if( m_ucIPBanMode != 0 || m_bBlockNATedIP || m_bLocalOnly )
	{
		if( gbDebug && ( gucDebugLevel & DEBUG_ANNOUNCE ) )
			UTIL_LogPrint( "serverResponseAnnounce: retrieve IP\n" );

		// retrieve ip
		strTempIP = pRequest->mapParams["ip"];
		strIPConv = strIP.c_str( );
	}

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
	{
		if( gbDebug && ( gucDebugLevel & DEBUG_ANNOUNCE ) )
			UTIL_LogPrint( "serverResponseAnnounce: IP banning (enabled)\n" );

		switch( m_ucIPBanMode )
		{
		case IP_BLACKLIST:
			if( UTIL_IsIPBanList( strIP, m_pIPBannedList ) || UTIL_IsIPBanList( strTempIP, m_pIPBannedList ) )
			{
				if( gbDebug && ( gucDebugLevel & DEBUG_ANNOUNCE ) )
					UTIL_LogPrint( "serverResponseAnnounce: IP blacklisted\n" );

				pResponse->strContent = UTIL_FailureReason( gmapLANG_CFG["announce_ip_banned"] );
				pResponse->bCompressOK = false;

				gtXStats.announce.iIPBanned++;

				if( gbDebug && ( gucDebugLevel & DEBUG_ANNOUNCE ) )
					UTIL_LogPrint( "serverResponseAnnounce: completed\n" );

				return;
			}

			break;
		case IP_VIPLIST:
			if( UTIL_IsIPBanList( strIP, m_pIPBannedList ) || UTIL_IsIPBanList( strTempIP, m_pIPBannedList ) )
			{
				if( gbDebug && ( gucDebugLevel & DEBUG_ANNOUNCE ) )
					UTIL_LogPrint( "serverResponseAnnounce: IP not cleared\n" );

				pResponse->strContent = UTIL_FailureReason( gmapLANG_CFG["announce_ip_not_cleared"] );
				pResponse->bCompressOK = false;

				gtXStats.announce.iIPNotCleared++;

				if( gbDebug && ( gucDebugLevel & DEBUG_ANNOUNCE ) )
					UTIL_LogPrint( "serverResponseAnnounce: completed\n" );

				return;
			}

			break;
		default:
			if( gbDebug && ( gucDebugLevel & DEBUG_ANNOUNCE ) )
				UTIL_LogPrint( "serverResponseAnnounce: IP banning method unknown (disabled)\n" );
		}
	}

	CAtomDicti *pData = new CAtomDicti( );

	// Private IP Spoofing Blocking
	if( m_bBlockNATedIP )
	{
		if( gbDebug && ( gucDebugLevel & DEBUG_ANNOUNCE ) )
			UTIL_LogPrint( "serverResponseAnnounce: Private IP Spoofing Blocking (enabled)\n" );

		if( !strTempIP.empty( ) && ( strTempIP.substr(0,8) == "192.168." || strTempIP.substr(0,8) == "169.254." || strTempIP.substr(0,3) == "10." || strTempIP.substr(0,7) == "172.16." || strTempIP.substr(0,7) == "172.17." || strTempIP.substr(0,7) == "172.18." || strTempIP.substr(0,7) == "172.19." || strTempIP.substr(0,7) == "172.20." || strTempIP.substr(0,7) == "172.21." || strTempIP.substr(0,7) == "172.22." || strTempIP.substr(0,7) == "172.23." || strTempIP.substr(0,7) == "172.24." || strTempIP.substr(0,7) == "172.25." || strTempIP.substr(0,7) == "172.26." || strTempIP.substr(0,7) == "172.27." || strTempIP.substr(0,7) == "172.28." || strTempIP.substr(0,7) == "172.29." || strTempIP.substr(0,7) == "172.30." || strTempIP.substr(0,7) == "172.31." || strTempIP.substr(0,7) == "172.32." || strTempIP == "127.0.0.1" ) )
		{

			if( gbDebug && ( gucDebugLevel & DEBUG_ANNOUNCE ) )
				UTIL_LogPrint( "serverResponseAnnounce: Private IP Spoofing Blocking (detected)\n" );
	
			pData->setItem("warning message", new CAtomString( "A122: This tracker does not permit you to specify your IP to it. Using the IP you are connecting from instead." ) );

			strTempIP = string( );
		}
	}

	// Public IP Spoofing Blocking
	if( m_bLocalOnly )
	{
		if( gbDebug && ( gucDebugLevel & DEBUG_ANNOUNCE ) )
			UTIL_LogPrint( "serverResponseAnnounce: Public IP Spoofing Blocking (enabled)\n" );

		if( !strIPConv.empty( ) && ( strIPConv.substr(0,8) == "192.168." || strIPConv.substr(0,8) == "169.254." || strIPConv.substr(0,3) == "10." || strIPConv.substr(0,7) == "172.16." || strIPConv.substr(0,7) == "172.17." || strIPConv.substr(0,7) == "172.18." || strIPConv.substr(0,7) == "172.19." || strIPConv.substr(0,7) == "172.20." || strIPConv.substr(0,7) == "172.21." || strIPConv.substr(0,7) == "172.22." || strIPConv.substr(0,7) == "172.23." || strIPConv.substr(0,7) == "172.24." || strIPConv.substr(0,7) == "172.25." || strIPConv.substr(0,7) == "172.26." || strIPConv.substr(0,7) == "172.27." || strIPConv.substr(0,7) == "172.28." || strIPConv.substr(0,7) == "172.29." || strIPConv.substr(0,7) == "172.30." || strIPConv.substr(0,7) == "172.31." || strIPConv.substr(0,7) == "172.32." || strIPConv == "127.0.0.1" ) )
		{
			if( !strTempIP.empty( ) && strTempIP.find_first_not_of( "1234567890." ) == string :: npos )
				strIP = strTempIP;
			else
			{
				if( !strTempIP.empty( ) )
					pData->setItem("warning message", new CAtomString( "A473: The IP you have specified is invalid. Using the IP you are connecting from instead." ) );

				strTempIP = string( );	  
			}
		}
		else if( !strIPConv.empty( ) && !( strIPConv.substr(0,8) == "192.168." || strIPConv.substr(0,8) == "169.254." || strIPConv.substr(0,3) == "10." || strIPConv.substr(0,7) == "172.16." || strIPConv.substr(0,7) == "172.17." || strIPConv.substr(0,7) == "172.18." || strIPConv.substr(0,7) == "172.19." || strIPConv.substr(0,7) == "172.20." || strIPConv.substr(0,7) == "172.21." || strIPConv.substr(0,7) == "172.22." || strIPConv.substr(0,7) == "172.23." || strIPConv.substr(0,7) == "172.24." || strIPConv.substr(0,7) == "172.25." || strIPConv.substr(0,7) == "172.26." || strIPConv.substr(0,7) == "172.27." || strIPConv.substr(0,7) == "172.28." || strIPConv.substr(0,7) == "172.29." || strIPConv.substr(0,7) == "172.30." || strIPConv.substr(0,7) == "172.31." || strIPConv.substr(0,7) == "172.32." || strIPConv == "127.0.0.1" ) )
		{
			if( !strTempIP.empty( ) )
				pData->setItem("warning message", new CAtomString( "A824: This tracker does not permit you to specify your IP to it. Using the IP you are connecting from instead." ) );

			strTempIP = string( );	
		}
	}
	else
	{
		if( !strTempIP.empty( ) && strTempIP.find_first_not_of( "1234567890." ) == string :: npos )
			strIP = strTempIP;
		else
		{			
			if( !strTempIP.empty( ) )
				pData->setItem("warning message", new CAtomString( "A373: The IP you have specified is invalid. Using the IP you are connecting from instead." ) );

			strTempIP = string( );
		}
	}

	// retrieve a ton of parameters
	const string cstrEvent( pRequest->mapParams["event"] );
	const string cstrPort( pRequest->mapParams["port"] );
	const string cstrUploaded( pRequest->mapParams["uploaded"] );
	const string cstrDownloaded( pRequest->mapParams["downloaded"] );
	const string cstrLeft( pRequest->mapParams["left"] );
	const string cstrPeerID( pRequest->mapParams["peer_id"] );
	const string cstrNumWant( pRequest->mapParams["numwant"] );
	const string cstrNoPeerID( pRequest->mapParams["no_peer_id"] );
	const string cstrCompact( pRequest->mapParams["compact"] );

	string cstrKey = string( );

	// Announce key support
	if( m_bAnnounceKeySupport )
	{
		if( gbDebug && ( gucDebugLevel & DEBUG_ANNOUNCE ) )
			UTIL_LogPrint( "serverResponseAnnounce: Announce key support (enabled)\n" );

		cstrKey = pRequest->mapParams["key"];

		if( cstrKey.empty( ) )
			pData->setItem("warning message", new CAtomString( "A000: This tracker supports key passing. Please enable your client." ) );
	}

	// CBTT
	string strUserAgent = string( );
	string strUserAgentA = string( );
	string strUserAgentB = string( );

	// Get the user agent
	if( m_bPeerSpoofRestrict || m_ucBanMode != 0 || m_ucShowPeerInfo != 0 )
	{
		if( gbDebug && ( gucDebugLevel & DEBUG_ANNOUNCE ) )
			UTIL_LogPrint( "serverResponseAnnounce: getting user agent\n" );

		strUserAgentA = pRequest->mapHeaders["User-Agent"];
		strUserAgentB = pRequest->mapHeaders["User-agent"];

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
	}

	// Peer spoof restriction
	if( m_bPeerSpoofRestrict )
	{
		if( gbDebug && ( gucDebugLevel & DEBUG_ANNOUNCE ) )
			UTIL_LogPrint( "serverResponseAnnounce: Peer spoof restriction (enabled)\n" );

		if( ( "-AZ" != cstrPeerID.substr(0,3) && "Azureus" == strUserAgent.substr(0,7) ) || ( "BitTorrent/3." == strUserAgent.substr(0,13) && ( "S5" == cstrPeerID.substr(0,2) || UTIL_EscapedToString("S%05") == cstrPeerID.substr(0,2) ) ) )
		{
			if( gbDebug && ( gucDebugLevel & DEBUG_ANNOUNCE ) )
				UTIL_LogPrint( "serverResponseAnnounce: Peer spoof restriction (detected)\n" );

			pResponse->strContent = UTIL_FailureReason( gmapLANG_CFG["announce_client_spoof"] );
			pResponse->bCompressOK = false;

			gtXStats.announce.iPeerSpoofRestrict++;

			if( gbDebug && ( gucDebugLevel & DEBUG_ANNOUNCE ) )
				UTIL_LogPrint( "serverResponseAnnounce: completed\n" );

			return;
		}
	}

	// Peer ID/User Agent banning
	if( m_ucBanMode != 0 )
	{
		if( gbDebug && ( gucDebugLevel & DEBUG_ANNOUNCE ) )
			UTIL_LogPrint( "serverResponseAnnounce: user agent banning (enabled)\n" );

		bool bClientBanned = UTIL_IsClientBanList( strUserAgent, m_pClientBannedList, true );

		if( !bClientBanned )
			bClientBanned = UTIL_IsClientBanList( cstrPeerID, m_pClientBannedList, false );

		switch( m_ucBanMode )
		{
		case ID_BLACKLIST:
			if ( bClientBanned )
			{
				if( gbDebug && ( gucDebugLevel & DEBUG_ANNOUNCE ) )
					UTIL_LogPrint( "serverResponseAnnounce: ID or user agent blacklisted\n" );

				pResponse->strContent = UTIL_FailureReason( gmapLANG_CFG["announce_client_banned"] );
				pResponse->bCompressOK = false;

				gtXStats.announce.iClientBanned++;

				if( gbDebug && ( gucDebugLevel & DEBUG_ANNOUNCE ) )
					UTIL_LogPrint( "serverResponseAnnounce: completed\n" );

				return;
			}

			break;
		case ID_VIPLIST:
			if ( !bClientBanned )
			{
				if( gbDebug && ( gucDebugLevel & DEBUG_ANNOUNCE ) )
					UTIL_LogPrint( "serverResponseAnnounce: ID or user agent not cleared\n" );

				pResponse->strContent = UTIL_FailureReason( gmapLANG_CFG["announce_client_banned"] );
				pResponse->bCompressOK = false;

				gtXStats.announce.iClientBanned++;

				if( gbDebug && ( gucDebugLevel & DEBUG_ANNOUNCE ) )
					UTIL_LogPrint( "serverResponseAnnounce: completed\n" );

				return;
			}

			break;
		default:
			if( gbDebug && ( gucDebugLevel & DEBUG_ANNOUNCE ) )
				UTIL_LogPrint( "serverResponseAnnounce: ID or user agent banning mode unknown (disabled)\n" );
		}
	}

	// invalid event
	if( !cstrEvent.empty( ) )
	{
		if( cstrEvent != EVENT_STR_STARTED && cstrEvent != EVENT_STR_COMPLETED && cstrEvent != EVENT_STR_STOPPED )
		{
			pResponse->strContent = UTIL_FailureReason( gmapLANG_CFG["announce_invalid_event"] );
			pResponse->bCompressOK = false;

			gtXStats.announce.iInvalidEvent++;

			if( gbDebug && ( gucDebugLevel & DEBUG_ANNOUNCE ) )
				UTIL_LogPrint( "serverResponseAnnounce: completed\n" );

			return;
		}
	}

	// port missing
	if( cstrPort.empty( ) )
	{
		pResponse->strContent = UTIL_FailureReason( gmapLANG_CFG["announce_port_missing"] );
		pResponse->bCompressOK = false;

		gtXStats.announce.iPortMissing++;

		if( gbDebug && ( gucDebugLevel & DEBUG_ANNOUNCE ) )
			UTIL_LogPrint( "serverResponseAnnounce: completed\n" );

		return;
	}

	// upload missing
	if( cstrUploaded.empty( ) )
	{
		pResponse->strContent = UTIL_FailureReason( gmapLANG_CFG["announce_uploaded_missing"] );
		pResponse->bCompressOK = false;

		gtXStats.announce.iUploadedMissing++;

		if( gbDebug && ( gucDebugLevel & DEBUG_ANNOUNCE ) )
			UTIL_LogPrint( "serverResponseAnnounce: completed\n" );

		return;
	}

	// download missing
	if( cstrDownloaded.empty( ) )
	{
		pResponse->strContent = UTIL_FailureReason( gmapLANG_CFG["announce_downloaded_missing"] );
		pResponse->bCompressOK = false;

		gtXStats.announce.iDownloadedMissing++;

		if( gbDebug && ( gucDebugLevel & DEBUG_ANNOUNCE ) )
			UTIL_LogPrint( "serverResponseAnnounce: completed\n" );

		return;
	}

	// left missing
	if( cstrLeft.empty( ) )
	{
		pResponse->strContent = UTIL_FailureReason( gmapLANG_CFG["announce_left_missing"] );
		pResponse->bCompressOK = false;

		gtXStats.announce.iLeftMissing++;

		if( gbDebug && ( gucDebugLevel & DEBUG_ANNOUNCE ) )
			UTIL_LogPrint( "serverResponseAnnounce: completed\n" );

		return;
	}

	// peer id length incorrect
	if( cstrPeerID.size( ) != 20 )
	{
		pResponse->strContent = UTIL_FailureReason( gmapLANG_CFG["announce_peerid_length"] );
		pResponse->bCompressOK = false;

		gtXStats.announce.iPeerIDLength++;

		if( gbDebug && ( gucDebugLevel & DEBUG_ANNOUNCE ) )
			UTIL_LogPrint( "serverResponseAnnounce: completed\n" );

		return;
	}

	struct announce_t ann;

	ann.strInfoHash = cstrInfoHash;
	ann.strIP = strIP;
	ann.strEvent = cstrEvent;
	ann.uiPort = (unsigned int)atoi( cstrPort.c_str( ) );
	ann.iUploaded = UTIL_StringTo64( cstrUploaded.c_str( ) );
	ann.iDownloaded = UTIL_StringTo64( cstrDownloaded.c_str( ) );
	ann.iLeft = UTIL_StringTo64( cstrLeft.c_str( ) );
	ann.strPeerID = cstrPeerID;

	// Announce key support
	if( m_bAnnounceKeySupport )
		ann.strKey = cstrKey;

	// Peer information support
	if( m_ucShowPeerInfo != 0 )
		ann.strUserAgent = strUserAgent;

	// CBTT

	// Listen port is blacklisted
	if( m_bBlacklistP2PPorts )
	{
		if( ( ann.uiPort >= 6881 && ann.uiPort <= 6999 ) || ( ann.uiPort >= 411 && ann.uiPort <= 413 ) || ann.uiPort == 1214 || ann.uiPort == 4662 || ( ann.uiPort >= 6346 && ann.uiPort <= 6347 ) )
		{
			pResponse->strContent = UTIL_FailureReason( UTIL_Xsprintf( gmapLANG_CFG["announce_blacklisted_port"].c_str( ), cstrPort.c_str( ) ) );
			pResponse->bCompressOK = false;

			gtXStats.announce.iPortBlacklisted++;

			if( gbDebug && ( gucDebugLevel & DEBUG_ANNOUNCE ) )
				UTIL_LogPrint( "serverResponseAnnounce: completed\n" );

			return;
		}
	}

	if(  m_bRestrictOverflow )
	{
		int64 iOverFlowLimit = UTIL_StringTo64( m_strOverFlowLimit.c_str() );
		int64 iOverFlowminimum = UTIL_StringTo64( "107374182400" );

		if( iOverFlowLimit < iOverFlowminimum )
			iOverFlowLimit = iOverFlowminimum;

		if( ann.iDownloaded < 0 || ann.iDownloaded >= iOverFlowLimit )
		{
			// Invalid downloaded value reported
			pResponse->strContent = UTIL_FailureReason( gmapLANG_CFG["announce_downloaded_invalid"] );
			pResponse->bCompressOK = false;

			gtXStats.announce.iDownloadedInvalid++;

			if( gbDebug && ( gucDebugLevel & DEBUG_ANNOUNCE ) )
				UTIL_LogPrint( "serverResponseAnnounce: completed\n" );

			return;
		}

		if( ann.iUploaded < 0 || ann.iUploaded >= iOverFlowLimit )
		{
			// Invalid uploaded value reported
			pResponse->strContent = UTIL_FailureReason( gmapLANG_CFG["announce_uploaded_invalid"] );
			pResponse->bCompressOK = false;

			gtXStats.announce.iUploadedInvalid++;

			if( gbDebug && ( gucDebugLevel & DEBUG_ANNOUNCE ) )
				UTIL_LogPrint( "serverResponseAnnounce: completed\n" );

			return;
		}
	}

	bool bRespond = true;

	Announce( ann, bRespond );

	unsigned int uiRSize = m_uiResponseSize;

	// populate cache
	if( bRespond && m_pCached )
	{
		if( !cstrNumWant.empty( ) )
			uiRSize = (unsigned int)atoi( cstrNumWant.c_str( ) );

		if( uiRSize > m_uiMaxGive )
			uiRSize = m_uiMaxGive;

		if( !m_pCached->getItem( ann.strInfoHash ) )
			m_pCached->setItem( ann.strInfoHash, new CAtomList( ) );

		CAtom *pCache = m_pCached->getItem( ann.strInfoHash );

		if( pCache && dynamic_cast<CAtomList *>( pCache ) )
		{
			CAtomList *pCacheList = dynamic_cast<CAtomList *>( pCache );

			if( pCacheList->getValuePtr( )->size( ) < uiRSize )
			{

#ifdef BNBT_MYSQL
				// to be done
				if( m_bMySQLOverrideDState )
				{
					CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bid,bip,bport,bkey FROM dstate WHERE bhash=\'" + UTIL_StringToMySQL( ann.strInfoHash ) + "\'" );

					vector<string> vecQuery;
					vecQuery.reserve(4);

					CAtomDicti *pAdd = 0;

					vecQuery = pQuery->nextRow( );

					while( vecQuery.size( ) == 4 )
					{
						pAdd = new CAtomDicti( );

						pAdd->setItem( "peer id", new CAtomString( vecQuery[0] ) );
						pAdd->setItem( "ip", new CAtomString( vecQuery[1] ) );
						pAdd->setItem( "port", new CAtomLong( (long)atoi( vecQuery[2].c_str( ) ) ) );
						pAdd->setItem( "key", new CAtomLong( (long)atoi( vecQuery[3].c_str( ) ) ) );

						pCacheList->addItem( pAdd );

						vecQuery = pQuery->nextRow( );
					}

					delete pQuery;
				}
				else
				{
#endif

					// Do we have a dfile?
					if( m_pDFile )
					{
						// If it does not contain the info hash information then create a new entry
						if( !m_pDFile->getItem( ann.strInfoHash ) )
							m_pDFile->setItem( ann.strInfoHash, new CAtomDicti( ) );

						// Get the list of peers from the info hash
						CAtom *pPeers = m_pDFile->getItem( ann.strInfoHash );

						// Do we have a list of peers?
						if( pPeers && pPeers->isDicti( ) )
						{
							// Here is the map to obtain the list of peers
							map<string, CAtom *> *pmapPeersDicti = ( (CAtomDicti *)pPeers )->getValuePtr( );

							// We need the peer with it's IP and port
							CAtomDicti *pAdd = 0;
							CAtom *pIP = 0;
							CAtom *pPort = 0;

							// Loop through the map to obtain the list peers
							for( map<string, CAtom *> :: iterator it = pmapPeersDicti->begin( ); it != pmapPeersDicti->end( ); it++ )
							{
								if( (*it).second->isDicti( ) )
								{
									// Get a new peer from the map with it's IP and port for the list of peers
									pAdd = new CAtomDicti( );

									pIP = ( (CAtomDicti *)(*it).second )->getItem( "ip" );
									pPort = ( (CAtomDicti *)(*it).second )->getItem( "port" );

									pAdd->setItem( "peer id", new CAtomString( (*it).first ) );

									// Add the peers to the list of peers
									if( pIP )
										pAdd->setItem( "ip", new CAtomString( pIP->toString( ) ) );

									if( pPort && dynamic_cast<CAtomLong *>( pPort ) )
										pAdd->setItem( "port", new CAtomLong( *dynamic_cast<CAtomLong *>( pPort ) ) );

									pCacheList->addItem( pAdd );
								}
							}
						}
					}

#ifdef BNBT_MYSQL
				}
#endif

				// Randomise the list of peers
				pCacheList->Randomize( );
			}
		}

		// clamp response

		if( pCache && dynamic_cast<CAtomList *>( pCache ) )
		{
			CAtomList *pCacheList = dynamic_cast<CAtomList *>( pCache );

			CAtom *pPeers = 0;

			if( cstrCompact == "1" )
			{
				// compact announce

				string strPeers = string( );

				vector<CAtom *> *pvecList = pCacheList->getValuePtr( );

				CAtom *pIP = 0;
				CAtom *pPort = 0;

				bool bOK = false;

				char pCompact[6];
				char szIP[16];
				char *szCur = 0;
				char *pSplit = 0;

				unsigned int uiPort = 0;

				for( vector<CAtom *> :: iterator it = pvecList->begin( ); it != pvecList->end( ); )
				{
					if( (*it)->isDicti( ) )
					{
						if( strPeers.size( ) / 6 >= uiRSize )
							break;

						pIP = ( (CAtomDicti *)(*it) )->getItem( "ip" );
						pPort = ( (CAtomDicti *)(*it) )->getItem( "port" );

						if( pIP && pPort && dynamic_cast<CAtomLong *>( pPort ) )
						{
							// tphogan - this is a much more efficient version of UTIL_Compact

							bOK = true;

							memset( pCompact, 0, sizeof( pCompact ) / sizeof( char ) );

							memset( szIP, 0, sizeof( szIP ) / sizeof( char ) );

							szCur = szIP;

							strncpy( szIP, pIP->toString( ).c_str( ), sizeof( szIP ) / sizeof( char ) );

							// first three octets
							for( unsigned char ucCount = 0; ucCount < 3; ucCount++ )
							{
								pSplit = (char *)strstr( szCur, "." );

								if( pSplit )
								{
									*pSplit = TERM_CHAR;
									pCompact[ucCount] = (char)atoi( szCur );
									szCur = pSplit + 1;
								}
								else
								{
									bOK = false;

									break;
								}
							}

							if( bOK )
							{
								// fourth octet
								pCompact[3] = (char)atoi( szCur );

								// port
								uiPort = (unsigned int)dynamic_cast<CAtomLong *>( pPort )->getValue( );

#ifdef BNBT_BIG_ENDIAN
								pCompact[5] = (char)( ( uiPort & 0xFF00 ) >> 8 );
								pCompact[4] = (char)( uiPort & 0xFF );
#else
								pCompact[4] = (char)( ( uiPort & 0xFF00 ) >> 8 );
								pCompact[5] = (char)( uiPort & 0xFF );
#endif

								strPeers += string( pCompact, 6 );
							}
						}

						delete *it;

						it = pvecList->erase( it );
					}
					else
						it++;
				}

				pPeers = new CAtomString( strPeers );

				// don't compress
				pResponse->bCompressOK = false;

				gtXStats.announce.iCompact++;
			}
			else
			{
				// regular announce

				CAtomList *pPeersList = new CAtomList( );

				vector<CAtom *> *pvecList = pCacheList->getValuePtr( );

				for( vector<CAtom *> :: iterator it = pvecList->begin( ); it != pvecList->end( ); )
				{
					if( (*it)->isDicti( ) )
					{
						if( pPeersList->getValuePtr( )->size( ) >= uiRSize )
							break;

						if( cstrNoPeerID == "1" )
							( (CAtomDicti *)(*it) )->delItem( "peer id" );

						pPeersList->addItem( new CAtomDicti( *(CAtomDicti *)(*it) ) );

						delete *it;

						it = pvecList->erase( it );
					}
					else
						it++;
				}

				pPeers = pPeersList;

				if( cstrNoPeerID == "1" )
					gtXStats.announce.iNopeerid++;
				else
					gtXStats.announce.iRegular++;
			}

			pData->setItem( "peers", pPeers );
		}
	}

	if( !bRespond )
	{
		if( gbDebug )
			UTIL_LogPrint( "serverResponseAnnounce: not returning peer list\n" );

		pData->setItem( "peers", new CAtomDicti( ) );
	}

	pData->setItem( "interval", new CAtomLong( m_uiAnnounceInterval ) );

	pData->setItem( "private", new CAtomLong( m_ucPrivateTracker ) );
	
	// Refresh the fast cache
	if( GetTime( ) > m_ulRefreshFastCacheNext )
	{
		// Refresh
		RefreshFastCache( );

		// Set the next refresh time
		m_ulRefreshFastCacheNext = GetTime( ) + m_uiRefreshFastCacheInterval;
	}

#ifdef BNBT_MYSQL
	if( !m_bMySQLOverrideDState )
	{
#endif
		if( m_pFastCache )
		{
			CAtom *pFastCache = m_pFastCache->getItem( cstrInfoHash );

			if( pFastCache && dynamic_cast<CAtomList *>( pFastCache ) )
			{
				vector<CAtom *> vecTorrent;
				vecTorrent.reserve( 6 );
				vecTorrent = dynamic_cast<CAtomList *>( pFastCache )->getValue( );

				pData->setItem( "complete", new CAtomInt( *dynamic_cast<CAtomInt *>( vecTorrent[0] ) ) );
				pData->setItem( "incomplete", new CAtomInt( *dynamic_cast<CAtomInt *>( vecTorrent[1] ) ) );
			}
		}
#ifdef BNBT_MYSQL
	}
	else
	{
			CMySQLQuery *pQuery = 0;

			vector<string> vecQuery;
			vecQuery.reserve(4);

			pQuery = new CMySQLQuery( "SELECT bseeders,bleechers,bcompleted,bhash FROM torrents" );

			vecQuery = pQuery->nextRow( );

			while( vecQuery.size( ) == 4 )
			{
				pData->setItem( "complete", new CAtomInt( atoi( vecQuery[0].c_str( ) ) ) );
				pData->setItem( "incomplete", new CAtomInt( atoi( vecQuery[1].c_str( ) ) ) );

				vecQuery = pQuery->nextRow( );
			}

			delete pQuery;
	}
#endif

	pResponse->strContent = Encode( pData );
		
	delete pData;

	if( gbDebug && ( gucDebugLevel & DEBUG_ANNOUNCE ) )
		UTIL_LogPrint( "serverResponseAnnounce: %s\n", pResponse->strContent.c_str( ) );

	//
	// link
	//

	if( bRespond && ( gpLinkServer || gpLink ) )
	{
		CAtomDicti *pAnnounce = new CAtomDicti( );

		pAnnounce->setItem( "info_hash", new CAtomString( ann.strInfoHash ) );
		pAnnounce->setItem( "ip", new CAtomString( ann.strIP ) );

		if( !cstrEvent.empty( ) )
			pAnnounce->setItem( "event", new CAtomString( ann.strEvent ) );

		pAnnounce->setItem( "port", new CAtomLong( ann.uiPort ) );
		pAnnounce->setItem( "uploaded", new CAtomLong( ann.iUploaded ) );
		pAnnounce->setItem( "downloaded", new CAtomLong( ann.iDownloaded ) );
		pAnnounce->setItem( "left", new CAtomLong( ann.iLeft ) );
		pAnnounce->setItem( "peer_id", new CAtomString( ann.strPeerID ) );
		pAnnounce->setItem( "key", new CAtomString( ann.strKey ) );
		

		struct linkmsg_t lm;

		lm.len = pAnnounce->EncodedLength( );
		lm.type = LINKMSG_ANNOUNCE;
		lm.msg = Encode( pAnnounce );

		if( gpLinkServer )
			gpLinkServer->Queue( lm );
		else if( gpLink )
			gpLink->Queue( lm );

		delete pAnnounce;
	}

	//
	// HUB link
	//

	if( bRespond && ( gpHUBLinkServer || gpHUBLink ) )
	{
		CAtomDicti *pAnnounce = new CAtomDicti( );

		pAnnounce->setItem( "info_hash", new CAtomString( ann.strInfoHash ) );
		pAnnounce->setItem( "ip", new CAtomString( ann.strIP ) );

		if( !cstrEvent.empty( ) )
			pAnnounce->setItem( "event", new CAtomString( ann.strEvent ) );

		pAnnounce->setItem( "port", new CAtomLong( ann.uiPort ) );
		pAnnounce->setItem( "uploaded", new CAtomLong( ann.iUploaded ) );
		pAnnounce->setItem( "downloaded", new CAtomLong( ann.iDownloaded ) );
		pAnnounce->setItem( "left", new CAtomLong( ann.iLeft ) );
		pAnnounce->setItem( "peer_id", new CAtomString( ann.strPeerID ) );
		pAnnounce->setItem( "key", new CAtomString( ann.strKey ) );
		

		struct linkmsg_t lm;

		lm.len = pAnnounce->EncodedLength( );
		lm.type = LINKMSG_ANNOUNCE;
		lm.msg = Encode( pAnnounce );

		if( gpHUBLinkServer )
			gpHUBLinkServer->Queue( lm );
		else if( gpHUBLink )
			gpHUBLink->Queue( lm );

		delete pAnnounce;
	}

	if( gbDebug && ( gucDebugLevel & DEBUG_ANNOUNCE ) )
		UTIL_LogPrint( "serverResponseAnnounce: completed\n" );
}
