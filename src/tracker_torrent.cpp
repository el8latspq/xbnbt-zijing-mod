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
 #include <time.h>
#else
 #include <sys/time.h>
#endif

#include "bnbt.h"
#include "bnbt_mysql.h"
#include "atom.h"
#include "bencode.h"
#include "config.h"
#include "tracker.h"
#include "util.h"

void CTracker :: serverResponseTorrent( struct request_t *pRequest, struct response_t *pResponse )
{
//	const string urlEncoding( const string sIn );
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
		if( IsIPBanned( pRequest, pResponse, btv, "Torrent server", string( CSS_INDEX ), NOT_INDEX ) )
			return;
		
	bool bPasskey = false;
	string strPasskey( pRequest->mapParams["passkey"] );
	
	if( strPasskey.find( " " ) != string :: npos )
		strPasskey.erase( );
	
	if( !strPasskey.empty( ) )
	{
		CMySQLQuery *pQuery = new CMySQLQuery( "SELECT buid FROM users WHERE bpasskey=\'" + UTIL_StringToMySQL( strPasskey ) + "\'" );
	
		vector<string> vecQuery;
		
		vecQuery.reserve(1);

		vecQuery = pQuery->nextRow( );
		
		delete pQuery;
		
		if( vecQuery.size( ) == 1 && !vecQuery[0].empty( ) )
			bPasskey = true;
	}
	
	if( !pRequest->user.strUID.empty( ) )
	{
		CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bpasskey FROM users WHERE buid=" + pRequest->user.strUID );
	
		vector<string> vecQuery;
		
		vecQuery.reserve(1);

		vecQuery = pQuery->nextRow( );
		
		delete pQuery;
		
		if( vecQuery.size( ) == 1 && !vecQuery[0].empty( ) )
			strPasskey = vecQuery[0];
	}

	if( !m_bAllowTorrentDownloads || ( !( pRequest->user.ucAccess & m_ucAccessDownTorrents ) && !bPasskey ) )
	{
		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, "Torrent server", string( CSS_INDEX ), string( ), NOT_INDEX, CODE_403 );

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INDEX ) );

		return;
	}

	// rip apart URL of the form "/torrents/<hash>.torrent"

	string strTorrent = pRequest->strURL.substr( 10 );

	const string :: size_type ciExt( strTorrent.rfind( "." ) );
	const string cstrExt( getFileExt( strTorrent ) );

	if( ciExt != string :: npos && UTIL_ToLower( cstrExt ) == ".torrent" )
		strTorrent = strTorrent.substr( 0, ciExt );
	
	if( strTorrent.find_first_not_of( "1234567890" ) != string :: npos )
		strTorrent.erase( );
	
	if( !strTorrent.empty( ) )
	{
		CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bfilename,bnodownload FROM allowed WHERE bid=" + strTorrent );
						
		vector<string> vecQuery;
	
		vecQuery.reserve(2);

		vecQuery = pQuery->nextRow( );
	
		delete pQuery;
	
		if( vecQuery.size( ) == 2 )
		{
			if( vecQuery[1] == "1" )
			{
				// Output common HTML head
				HTML_Common_Begin(  pRequest, pResponse, "Torrent server", string( CSS_INDEX ), string( ), NOT_INDEX, CODE_403 );

				// Output common HTML tail
				HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INDEX ) );

				return;
			}
			else if( !vecQuery[0].empty( ) )
			{
				const string cstrPath( m_strAllowedDir + vecQuery[0] );

				if( UTIL_CheckFile( cstrPath.c_str( ) ) )
				{
					pResponse->strCode = "200 " + gmapLANG_CFG["server_response_200"];

					//addition by labarks
//					const string cstrFile( pRequest->strURL.substr(10) );	
				
					if( pRequest->mapHeaders["User-Agent"].find( "MSIE" ) != string :: npos )
					{
//						if ( cstrFile == vecQuery[0] )
//						{
//							pResponse->mapHeaders.insert( pair<string, string>( "Content-Type", gmapMime[".torrent"] + "; name=\"" + UTIL_StringToEscaped( cstrFile ) + "\"" ) );
//							pResponse->mapHeaders.insert( pair<string, string>( "Content-Disposition", "attachment; filename=\"" + UTIL_StringToEscaped( cstrFile ) + "\"" ) );
//						}
//						else
//						{
							pResponse->mapHeaders.insert( pair<string, string>( "Content-Type", gmapMime[".torrent"] + "; name=\"" + UTIL_StringToEscaped( vecQuery[0] ) + "\"" ) );
							pResponse->mapHeaders.insert( pair<string, string>( "Content-Disposition", "attachment; filename=\"" + UTIL_StringToEscaped( vecQuery[0] ) + "\"" ) );
//						}
					}
					else
					{
//						if ( cstrFile == vecQuery[0] )
//						{
//							pResponse->mapHeaders.insert( pair<string, string>( "Content-Type", gmapMime[".torrent"] + "; name=\"" + cstrFile + "\"" ) );
//							pResponse->mapHeaders.insert( pair<string, string>( "Content-Disposition", "attachment; filename=\"" + cstrFile + "\"" ) );
//						}
//						else
//						{
							pResponse->mapHeaders.insert( pair<string, string>( "Content-Type", gmapMime[".torrent"] + "; name=\"" + vecQuery[0] + "\"" ) );
							pResponse->mapHeaders.insert( pair<string, string>( "Content-Disposition", "attachment; filename=\"" + vecQuery[0] + "\"" ) );
//						}
					}

					// cache for awhile
					time_t tNow = time( 0 ) + m_uiTorrentExpires * 60;
					char *szTime = asctime( gmtime( &tNow ) );
					szTime[strlen( szTime ) - 1] = TERM_CHAR;

					pResponse->mapHeaders.insert( pair<string, string>( "Expires", string( szTime ) + " GMT" ) );
				
					if( m_bDontCompressTorrent )
						pResponse->bCompressOK = false;
				
					const string cstrData( UTIL_ReadFile( cstrPath.c_str( ) ) );

					if( !m_strForceAnnounceURL.empty( ) && m_bForceAnnounceOnDL )
					{
						CAtom *pData = Decode( cstrData );

						if( pData && pData->isDicti( ) )
						{
							if( !strPasskey.empty( ) )
								( (CAtomDicti *)pData )->setItem( "announce", new CAtomString( m_strForceAnnounceURL + "?passkey=" + strPasskey ) );

							if( m_bEnableAnnounceList )
							{
								unsigned char ucAnnounceList = 1;
								string strKey = "xbnbt_announce_list" + CAtomInt( ucAnnounceList ).toString( );
								string strAnnounceUrl = CFG_GetString( strKey, string( ) );

								if( !strAnnounceUrl.empty( ) )
								{
									CAtomList *pAnnounceList = new CAtomList( );

									CAtomList *pAnnounceListItem = new CAtomList( );
									if( !strPasskey.empty( ) )
										pAnnounceListItem->addItem( new CAtomString( m_strForceAnnounceURL + "?passkey=" + strPasskey ) );
									pAnnounceList->addItem( pAnnounceListItem );

									while( !strAnnounceUrl.empty( ) )
									{
										CAtomList *pAnnounceListItem1 = new CAtomList( );
										if( !strPasskey.empty( ) )
											pAnnounceListItem1->addItem( new CAtomString( strAnnounceUrl + "?passkey=" + strPasskey ) );
										pAnnounceList->addItem( pAnnounceListItem1 );
										strKey = "xbnbt_announce_list" + CAtomInt( ++ucAnnounceList ).toString( );
										strAnnounceUrl = CFG_GetString( strKey, string( ) );
									}

									( (CAtomDicti *)pData )->setItem( "announce-list", new CAtomList( *pAnnounceList ) );

									delete pAnnounceList;
								}
							}
							else
								( (CAtomDicti *)pData )->delItem( "announce-list" );
							pResponse->strContent = Encode( pData );
							gtXStats.file.iTorrent++;
						}

						delete pData;
					}
					else
					{
						pResponse->strContent = cstrData;
						gtXStats.file.iTorrent++;
					}
				}
				else
					pResponse->strCode = "404 " + gmapLANG_CFG["server_response_404"];
			}
			else
				pResponse->strCode = "404 " + gmapLANG_CFG["server_response_404"];
		}
		else
			pResponse->strCode = "404 " + gmapLANG_CFG["server_response_404"];
	}
	else
		pResponse->strCode = "404 " + gmapLANG_CFG["server_response_404"];
}

void CTracker :: serverResponseOffer( struct request_t *pRequest, struct response_t *pResponse )
{
//	const string urlEncoding( const string sIn );
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
		if( IsIPBanned( pRequest, pResponse, btv, "Torrent server", string( CSS_OFFER ), NOT_INDEX ) )
			return;

	if( !m_bAllowTorrentDownloads || !( pRequest->user.ucAccess & m_ucAccessDownTorrents ) || ( !( pRequest->user.ucAccess & m_ucAccessUploadOffers ) && !( pRequest->user.ucAccess & m_ucAccessAllowOffers ) ) )
	{
		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, "Torrent server", string( CSS_OFFER ), string( ), NOT_INDEX, CODE_403 );

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_OFFER ) );

		return;
	}
	
	if( pRequest->user.strUID.empty( ) )
	{
		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, "Torrent server", string( CSS_INDEX ), string( ), NOT_INDEX, CODE_403 );

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INDEX ) );

		return;
	}
	
	string strPasskey = string( );

	CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bpasskey FROM users WHERE buid=" + pRequest->user.strUID );

	vector<string> vecQuery;
	
	vecQuery.reserve(1);

	vecQuery = pQuery->nextRow( );
	
	delete pQuery;
	
	if( vecQuery.size( ) == 1 && !vecQuery[0].empty( ) )
		strPasskey = vecQuery[0];
	
	// rip apart URL of the form "/offers/<hash>.torrent"

	string strTorrent = pRequest->strURL.substr( 8 );

	const string :: size_type ciExt( strTorrent.rfind( "." ) );
	const string cstrExt( getFileExt( strTorrent ) );

	if( ciExt != string :: npos && UTIL_ToLower( cstrExt ) == ".torrent" )
		strTorrent = strTorrent.substr( 0, ciExt );
	
	if( strTorrent.find_first_not_of( "1234567890" ) != string :: npos )
		strTorrent.erase( );
	
	if( !strTorrent.empty( ) )
	{
		CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bfilename FROM offer WHERE bid=" + strTorrent );
						
		vector<string> vecQuery;
	
		vecQuery.reserve(1);

		vecQuery = pQuery->nextRow( );
	
		delete pQuery;
	
		if( vecQuery.size( ) == 1 )
		{
			if( !vecQuery[0].empty( ) )
			{
				const string cstrPath( m_strOfferDir + vecQuery[0] );

				if( UTIL_CheckFile( cstrPath.c_str( ) ) )
				{
					pResponse->strCode = "200 " + gmapLANG_CFG["server_response_200"];

					//addition by labarks
	//				const string cstrFile( pRequest->strURL.substr(10) );	
				
					if( pRequest->mapHeaders["User-Agent"].find( "MSIE" ) != string :: npos )
					{
	//					if ( cstrFile == vecQuery[0] )
	//					{
	//						pResponse->mapHeaders.insert( pair<string, string>( "Content-Type", gmapMime[".torrent"] + "; name=\"" + UTIL_StringToEscaped( cstrFile ) + "\"" ) );
	//						pResponse->mapHeaders.insert( pair<string, string>( "Content-Disposition", "attachment; filename=\"" + UTIL_StringToEscaped( cstrFile ) + "\"" ) );
	//					}
	//					else
	//					{
							pResponse->mapHeaders.insert( pair<string, string>( "Content-Type", gmapMime[".torrent"] + "; name=\"" + UTIL_StringToEscaped( vecQuery[0] ) + "\"" ) );
							pResponse->mapHeaders.insert( pair<string, string>( "Content-Disposition", "attachment; filename=\"" + UTIL_StringToEscaped( vecQuery[0] ) + "\"" ) );
	//					}
					}
					else
					{
	//					if ( cstrFile == vecQuery[0] )
	//					{
	//						pResponse->mapHeaders.insert( pair<string, string>( "Content-Type", gmapMime[".torrent"] + "; name=\"" + cstrFile + "\"" ) );
	//						pResponse->mapHeaders.insert( pair<string, string>( "Content-Disposition", "attachment; filename=\"" + cstrFile + "\"" ) );
	//					}
	//					else
	//					{
							pResponse->mapHeaders.insert( pair<string, string>( "Content-Type", gmapMime[".torrent"] + "; name=\"" + vecQuery[0] + "\"" ) );
							pResponse->mapHeaders.insert( pair<string, string>( "Content-Disposition", "attachment; filename=\"" + vecQuery[0] + "\"" ) );
	//					}
					}

					// cache for awhile
					time_t tNow = time( 0 ) + m_uiTorrentExpires * 60;
					char *szTime = asctime( gmtime( &tNow ) );
					szTime[strlen( szTime ) - 1] = TERM_CHAR;

					pResponse->mapHeaders.insert( pair<string, string>( "Expires", string( szTime ) + " GMT" ) );
				
					if( m_bDontCompressTorrent )
						pResponse->bCompressOK = false;
				
					const string cstrData( UTIL_ReadFile( cstrPath.c_str( ) ) );

					if( !m_strForceAnnounceURL.empty( ) && m_bForceAnnounceOnDL )
					{
						CAtom *pData = Decode( cstrData );

						if( pData && pData->isDicti( ) )
						{
							( (CAtomDicti *)pData )->setItem( "announce", new CAtomString( m_strForceAnnounceURL + "?passkey=" + strPasskey ) );
	
							if( m_bEnableAnnounceList )
							{
								unsigned char ucAnnounceList = 1;
								string strKey = "xbnbt_announce_list" + CAtomInt( ucAnnounceList ).toString( );
								string strAnnounceUrl = CFG_GetString( strKey, string( ) );

								if( !strAnnounceUrl.empty( ) )
								{
									CAtomList *pAnnounceList = new CAtomList( );

									CAtomList *pAnnounceListItem = new CAtomList( );
									pAnnounceListItem->addItem( new CAtomString( m_strForceAnnounceURL + "?passkey=" + strPasskey ) );
									pAnnounceList->addItem( pAnnounceListItem );

									while( !strAnnounceUrl.empty( ) )
									{
										CAtomList *pAnnounceListItem1 = new CAtomList( );
										pAnnounceListItem1->addItem( new CAtomString( strAnnounceUrl + "?passkey=" + strPasskey ) );
										pAnnounceList->addItem( pAnnounceListItem1 );
										strKey = "xbnbt_announce_list" + CAtomInt( ++ucAnnounceList ).toString( );
										strAnnounceUrl = CFG_GetString( strKey, string( ) );
									}

									( (CAtomDicti *)pData )->setItem( "announce-list", new CAtomList( *pAnnounceList ) );

									delete pAnnounceList;
								}
							}
							pResponse->strContent = Encode( pData );
	// 							gtXStats.file.iTorrent++;
						}

						delete pData;
					}
					else
					{
						pResponse->strContent = cstrData;
	// 						gtXStats.file.iTorrent++;
					}
				}
				else
					pResponse->strCode = "404 " + gmapLANG_CFG["server_response_404"];
			}
			else
				pResponse->strCode = "404 " + gmapLANG_CFG["server_response_404"];
		}
		else
			pResponse->strCode = "404 " + gmapLANG_CFG["server_response_404"];
	}
	else
		pResponse->strCode = "404 " + gmapLANG_CFG["server_response_404"];
}

//inline BYTE toHex(const BYTE &x)
//{
//return x > 9 ? x + 55: x + 48;
//}

//const string urlEncoding( const string sIn )
//{
////      cout << "size: " << sIn.size() << endl;
//     string sOut;
//     for( int ix = 0; ix < sIn.size(); ix++ )
//     {
//         BYTE buf[4];
//         memset( buf, 0, 4 );
//         if( isascii( (BYTE)sIn[ix] ) )
//         {
//             buf[0] = sIn[ix];
//         }
////          else if ( isspace( (BYTE)sIn[ix] ) )
////          {
////              buf[0] = '+';
////          }
////          else if ( ( (BYTE)sIn[ix] ) == '.' )
//// 	{
////              buf[0] = '.';
////          }
//         else
//         {
//             buf[0] = '%';
//             buf[1] = toHex( (BYTE)sIn[ix] >> 4 );
//             buf[2] = toHex( (BYTE)sIn[ix] % 16);
//         }
//         sOut += (char *)buf;
//     }
//     return sOut;
//}
