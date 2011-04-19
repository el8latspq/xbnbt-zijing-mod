/***
*
* BNBT Beta 8.0 - A C++ BitTorrent Tracker
* Copyright (C) 2003-2004 Trevor Hogan
*
* This library is free software; you can redistribute ulKey and/or
* modify ulKey under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that ulKey will be useful,
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
#include "config.h"
#include "html.h"
#include "server.h"
#include "sort.h"
#include "tracker.h"
#include "util.h"

void CTracker :: serverResponseIndex( struct request_t *pRequest, struct response_t *pResponse )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
		if( IsIPBanned( pRequest, pResponse, btv, gmapLANG_CFG["index_page"], string( CSS_INDEX ), NOT_INDEX ) )
			return;
	
	// Check that user has view authority
	if( pRequest->user.ucAccess & ACCESS_VIEW )
	{
		// Was a search submited?
// 		if( pRequest->mapParams["top_submit_search_button"] == STR_SUBMIT || pRequest->mapParams["bottom_submit_search_button"] == STR_SUBMIT )
		if( pRequest->mapParams["top_submit_search_button"] == gmapLANG_CFG["search"] || pRequest->mapParams["bottom_submit_search_button"] == gmapLANG_CFG["search"] )
		{
			const string cstrSearch( pRequest->mapParams["search"] );
			const string cstrSort( pRequest->mapParams["sort"] );
			const string cstrPerPage( pRequest->mapParams["per_page"] );
			const string cstrMode( pRequest->mapParams["mode"] );
			const string cstrUploader( pRequest->mapParams["uploader"] );

			string strPageParameters = INDEX_HTML;

			if( !cstrSearch.empty( ) || !cstrSort.empty( ) || !cstrPerPage.empty( ) )
				strPageParameters += "?";
	
			if( !cstrSearch.empty( ) )
				strPageParameters += "search=" + cstrSearch;

			if( !cstrSearch.empty( ) && !cstrSort.empty( ) )
				strPageParameters += "&";
						
			if( !cstrSort.empty( ) )
				strPageParameters += "sort=" + cstrSort;

			if( ( !cstrSearch.empty( ) || !cstrSort.empty( ) ) && !cstrMode.empty( ) )
				strPageParameters += "&";
						
			if( !cstrMode.empty( ) )
				strPageParameters += "mode=" + cstrMode;

			if( ( !cstrSearch.empty( ) || !cstrSort.empty( ) || !cstrMode.empty( ) ) && !cstrUploader.empty( ) )
				strPageParameters += "&";

			if( !cstrUploader.empty( ) )
				strPageParameters += "uploader=" + cstrUploader;

			if( ( !cstrSearch.empty( ) || !cstrSort.empty( ) || !cstrMode.empty( ) || !cstrUploader.empty( ) ) && !cstrPerPage.empty( ) )
				strPageParameters += "&";

			if( !cstrPerPage.empty( ) )
				strPageParameters += "per_page=" + cstrPerPage;

			return JS_ReturnToPage( pRequest, pResponse, strPageParameters );
		}

		if( pRequest->mapParams["top_clear_filter_and_search_button"] == "Clear" || pRequest->mapParams["bottom_clear_filter_and_search_button"] == "Clear" )
			return JS_ReturnToPage( pRequest, pResponse, INDEX_HTML );

		//
		// delete torrent
		//

		// Check that user has edit authority
		if( pRequest->user.ucAccess & ACCESS_EDIT )
		{
			if( pRequest->mapParams.find( "del" ) != pRequest->mapParams.end( ) )
			{
				const string strDelHashString( pRequest->mapParams["del"] );
				const string strDelHash( UTIL_StringToHash( strDelHashString ) );
				const string strOK ( pRequest->mapParams["ok"] );

				if( strDelHash.empty( ) )
				{     
					// Output common HTML head
					HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["index_page"], string( CSS_INDEX ), string( ), false, false );

					pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["index_invalid_hash"].c_str( ), strDelHashString.c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_index"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";

					// Output common HTML tail
					HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INDEX ) );

					return;
				}
				else
				{
					if( strOK == "1" )
					{
						if( m_pAllowed )
						{
							// delete from disk

							CAtom *pList = m_pAllowed->getItem( strDelHash );

							if( pList && dynamic_cast<CAtomList *>( pList ) )
							{
								vector<CAtom *> vecTorrent;
								vecTorrent.reserve( 6 );
								vecTorrent = dynamic_cast<CAtomList *>( pList )->getValue( );

								if( vecTorrent.size( ) == 6 )
								{
									CAtom *pFileName = vecTorrent[0];

									if( pFileName )
									{
										if( m_strArchiveDir.empty( ) )
											UTIL_DeleteFile( string( m_strAllowedDir + pFileName->toString( ) ).c_str( ) );
										else
											UTIL_MoveFile( string( m_strAllowedDir + pFileName->toString( ) ).c_str( ), string( m_strArchiveDir + pFileName->toString( ) ).c_str( ) );
									}
								}
							}

							m_pAllowed->delItem( strDelHash );
							m_pDFile->delItem( strDelHash );
							deleteTag( strDelHash );

							// Output common HTML head
							HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["index_page"], string( CSS_INDEX ), string( ), NOT_INDEX, CODE_200 );

							// Deleted the torrent
							pResponse->strContent += "<p class=\"deleted\">" + UTIL_Xsprintf( gmapLANG_CFG["index_deleted_torrent"].c_str( ), strDelHashString.c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_index"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";

							// Output common HTML tail
							HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INDEX ) );

							return;
						}
					}
					else
					{
						// Added and modified by =Xotic=
						// The Trinity Edition - Modification Begins
						// The following replaces the OK response with a YES | NO option
						// when DELETING A TORRENT

						// Output common HTML head
						HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["index_page"], string( CSS_INDEX ), string( ), NOT_INDEX, CODE_200 );

						pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["delete_torrent_q"].c_str( ), strDelHashString.c_str( ) ) + "</p>\n";
						pResponse->strContent += "<p class=\"delete\"><a title=\"" + gmapLANG_CFG["yes"] + "\"href=\"" + RESPONSE_STR_INDEX_HTML + "?del=" + strDelHashString + "&amp;ok=1\">" + gmapLANG_CFG["yes"] + "</a>\n";
						pResponse->strContent += "<span class=\"pipe\">|</span><a title=\"" + gmapLANG_CFG["no"] + "\" href=\"" + string( JS_BACK ) + "\">" + gmapLANG_CFG["no"] + "</a></p>\n";

						// Output common HTML tail
						HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INDEX ) );

						return;
					}
				}
			}
		}
		
		if( pRequest->user.ucAccess & ACCESS_EDIT )
		{
			if( pRequest->mapParams.find( "top" ) != pRequest->mapParams.end( ) )
			{
				const string strTopHashString( pRequest->mapParams["top"] );
				if( strTopHashString.empty( ) )
				{     
					// Output common HTML head
					HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["index_page"], string( CSS_INDEX ), string( ), false, false );

					pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["index_invalid_hash"].c_str( ), strTopHashString.c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_index"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";

					// Output common HTML tail
					HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INDEX ) );

					return;
				}
				else
				{
					if( m_pAllowed )
					{
						UTIL_MakeFile( string( m_strTopDir + strTopHashString ).c_str( ), "" );
					}
					// Output common HTML head
					HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["index_page"], string( CSS_INDEX ), string( ), NOT_INDEX, CODE_200 );

					// Deleted the torrent
					pResponse->strContent += "<p class=\"deleted\">" + UTIL_Xsprintf( gmapLANG_CFG["index_top_torrent"].c_str( ), strTopHashString.c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_index"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";

					// Output common HTML tail
					HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INDEX ) );
					
					return;
				}
			}
		}
		
		if( pRequest->user.ucAccess & ACCESS_EDIT )
		{
			if( pRequest->mapParams.find( "normal" ) != pRequest->mapParams.end( ) )
			{
				const string strNormalHashString( pRequest->mapParams["normal"] );
				if( strNormalHashString.empty( ) )
				{     
					// Output common HTML head
					HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["index_page"], string( CSS_INDEX ), string( ), false, false );

					pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["index_invalid_hash"].c_str( ), strNormalHashString.c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_index"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";

					// Output common HTML tail
					HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INDEX ) );

					return;
				}
				else
				{
					if( m_pAllowed )
					{
						UTIL_DeleteFile( string( m_strTopDir + strNormalHashString ).c_str( ) );
					}
					// Output common HTML head
					HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["index_page"], string( CSS_INDEX ), string( ), NOT_INDEX, CODE_200 );

					// Deleted the torrent
					pResponse->strContent += "<p class=\"deleted\">" + UTIL_Xsprintf( gmapLANG_CFG["index_top_torrent"].c_str( ), strNormalHashString.c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_index"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";

					// Output common HTML tail
					HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INDEX ) );
					
					return;
				}
			}
		}
		
		if( pRequest->user.ucAccess & ACCESS_EDIT )
		{
			if( pRequest->mapParams.find( "hl" ) != pRequest->mapParams.end( ) )
			{
				const string strHLHashString( pRequest->mapParams["hl"] );
				if( strHLHashString.empty( ) )
				{     
					// Output common HTML head
					HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["index_page"], string( CSS_INDEX ), string( ), false, false );

					pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["index_invalid_hash"].c_str( ), strHLHashString.c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_index"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";

					// Output common HTML tail
					HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INDEX ) );

					return;
				}
				else
				{
					if( m_pAllowed )
					{
						UTIL_MakeFile( string( m_strHLDir + strHLHashString ).c_str( ), "" );
					}
					// Output common HTML head
					HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["index_page"], string( CSS_INDEX ), string( ), NOT_INDEX, CODE_200 );

					// Deleted the torrent
					pResponse->strContent += "<p class=\"deleted\">" + UTIL_Xsprintf( gmapLANG_CFG["index_hl_torrent"].c_str( ), strHLHashString.c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_index"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";

					// Output common HTML tail
					HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INDEX ) );
					
					return;
				}
			}
		}
		
		if( pRequest->user.ucAccess & ACCESS_EDIT )
		{
			if( pRequest->mapParams.find( "nohl" ) != pRequest->mapParams.end( ) )
			{
				const string strNoHLHashString( pRequest->mapParams["nohl"] );
				if( strNoHLHashString.empty( ) )
				{     
					// Output common HTML head
					HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["index_page"], string( CSS_INDEX ), string( ), false, false );

					pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["index_invalid_hash"].c_str( ), strNoHLHashString.c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_index"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";

					// Output common HTML tail
					HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INDEX ) );

					return;
				}
				else
				{
					if( m_pAllowed )
					{
						UTIL_DeleteFile( string( m_strHLDir + strNoHLHashString ).c_str( ) );
					}
					// Output common HTML head
					HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["index_page"], string( CSS_INDEX ), string( ), NOT_INDEX, CODE_200 );

					// Deleted the torrent
					pResponse->strContent += "<p class=\"deleted\">" + UTIL_Xsprintf( gmapLANG_CFG["index_hl_torrent"].c_str( ), strNoHLHashString.c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_index"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";

					// Output common HTML tail
					HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INDEX ) );
					
					return;
				}
			}
		}
		
		if( pRequest->user.ucAccess & ACCESS_VIEW )
		{
			if( pRequest->mapParams.find( "req" ) != pRequest->mapParams.end( ) )
			{
				const string strReqHashString( pRequest->mapParams["req"] );
				if( strReqHashString.empty( ) )
				{     
					// Output common HTML head
					HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["index_page"], string( CSS_INDEX ), string( ), false, false );

					pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["index_invalid_hash"].c_str( ), strReqHashString.c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_index"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";

					// Output common HTML tail
					HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INDEX ) );

					return;
				}
				else
				{
					if( m_pAllowed )
					{
						UTIL_MakeFile( string( m_strReqDir + strReqHashString ).c_str( ), "" );
					}
					// Output common HTML head
					HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["index_page"], string( CSS_INDEX ), string( ), NOT_INDEX, CODE_200 );

					// Deleted the torrent
					pResponse->strContent += "<p class=\"deleted\">" + UTIL_Xsprintf( gmapLANG_CFG["index_req_torrent"].c_str( ), strReqHashString.c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_index"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";

					// Output common HTML tail
					HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INDEX ) );
					
					return;
				}
			}
		}
		
		if( pRequest->user.ucAccess & ACCESS_VIEW )
		{
			if( pRequest->mapParams.find( "noreq" ) != pRequest->mapParams.end( ) )
			{
				const string strNoReqHashString( pRequest->mapParams["noreq"] );
				if( strNoReqHashString.empty( ) )
				{     
					// Output common HTML head
					HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["index_page"], string( CSS_INDEX ), string( ), false, false );

					pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["index_invalid_hash"].c_str( ), strNoReqHashString.c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_index"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";

					// Output common HTML tail
					HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INDEX ) );

					return;
				}
				else
				{
					if( m_pAllowed )
					{
						UTIL_DeleteFile( string( m_strReqDir + strNoReqHashString ).c_str( ) );
					}
					// Output common HTML head
					HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["index_page"], string( CSS_INDEX ), string( ), NOT_INDEX, CODE_200 );

					// Deleted the torrent
					pResponse->strContent += "<p class=\"deleted\">" + UTIL_Xsprintf( gmapLANG_CFG["index_req_cancel_torrent"].c_str( ), strNoReqHashString.c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_index"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";

					// Output common HTML tail
					HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INDEX ) );
					
					return;
				}
			}
		}

		// Compose the page
		if( m_pDFile )
		{
			// Are we tracking any files?
			if( m_pDFile->isEmpty( ) )
			{
				// Output common HTML head
				HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["index_page"], string( CSS_INDEX ), string( ), IS_INDEX, CODE_200 );

				// No files are being tracked!
				pResponse->strContent += "<p class=\"no_files\">" + gmapLANG_CFG["index_no_files"] + "</p>\n";

				// Output common HTML tail
				HTML_Common_End( pRequest, pResponse, btv, IS_INDEX, string( CSS_INDEX ) );

				return;
			}

			// Populate the torrents structure for display

			map<string, CAtom *> *pmapDicti = m_pDFile->getValuePtr( );

			unsigned long ulKeySize = (unsigned long)pmapDicti->size( );

			// add the torrents into this structure one by one and sort ulKey afterwards

			struct torrent_t *pTorrents = new struct torrent_t[ulKeySize];

			unsigned long ulCount = 0;

			vector<CAtom *> vecTorrent;
			vecTorrent.reserve( 6 );

			CAtom *pList = 0;
			CAtom *pFileName = 0;
			CAtom *pName = 0;
			CAtom *pRatio = 0;
			CAtom *pAllow = 0;
			CAtom *pTop = 0;
			CAtom *pAdded = 0;
			CAtom *pSize = 0;
			CAtom *pFiles = 0;
			CAtom *pCommentList = 0;
			CAtom *pDicti = 0;
			CAtom *pTag = 0;
			CAtom *pUploader = 0;
			CAtom *pInfoLink = 0;
			CAtom *pIP = 0;
			CAtom *pFC = 0;
			const string strRatio[4] = { "5%", "10%", "15%", "20%" };
			
			for( map<string, CAtom *> :: iterator ulKey = pmapDicti->begin( ); ulKey != pmapDicti->end( ); ulKey++ )
			{
				pTorrents[ulCount].strInfoHash = (*ulKey).first;
				pTorrents[ulCount].strName = gmapLANG_CFG["unknown"];
				pTorrents[ulCount].strLowerName = gmapLANG_CFG["unknown"];
				pTorrents[ulCount].strRatio = strRatio[1];
				pTorrents[ulCount].uiSeeders = 0;
				pTorrents[ulCount].uiLeechers = 0;
				pTorrents[ulCount].ulCompleted = 0;
				pTorrents[ulCount].iTransferred = 0;
				pTorrents[ulCount].iSize = 0;
				pTorrents[ulCount].uiFiles = 0;
				pTorrents[ulCount].uiComments = 0;
				pTorrents[ulCount].iAverageLeft = 0;
				pTorrents[ulCount].ucAverageLeftPercent = 0;
				pTorrents[ulCount].iMinLeft = 0;
				pTorrents[ulCount].iMaxiLeft = 0;
				pTorrents[ulCount].iAllow = 1;
				pTorrents[ulCount].iTop = 0;
				pTorrents[ulCount].iHL = 0;

				if( (*ulKey).second->isDicti( ) )
				{
					// grab data from m_pAllowed   

					if( m_pAllowed )
					{
						pList = m_pAllowed->getItem( pTorrents[ulCount].strInfoHash );

						if( pList && dynamic_cast<CAtomList *>( pList ) )
						{
							vecTorrent = dynamic_cast<CAtomList *>( pList )->getValue( );

							if( vecTorrent.size( ) == 6 )
							{
								pFileName = vecTorrent[0];
								pName = vecTorrent[1];
								pAdded = vecTorrent[2];
								pSize = vecTorrent[3];
								pFiles = vecTorrent[4];

								if( pFileName )
									pTorrents[ulCount].strFileName = pFileName->toString( );

								if( pName )
								{
									// stick a lower case version in strNameLower for non case sensitive searching and sorting

									pTorrents[ulCount].strName = pName->toString( );
									pTorrents[ulCount].strLowerName = UTIL_ToLower( pTorrents[ulCount].strName );
								}

								if( pAdded )
									pTorrents[ulCount].strAdded = pAdded->toString( );

								if( pSize && dynamic_cast<CAtomLong *>( pSize ) )
									pTorrents[ulCount].iSize = dynamic_cast<CAtomLong *>( pSize )->getValue( );

								if( pFiles && dynamic_cast<CAtomInt *>( pFiles ) )
									pTorrents[ulCount].uiFiles = (unsigned int)dynamic_cast<CAtomInt *>( pFiles )->getValue( );
							}
						}

						if( m_bAllowComments )
						{
							if( m_pComments )
							{
								pCommentList = m_pComments->getItem( pTorrents[ulCount].strInfoHash );

								if( pCommentList && dynamic_cast<CAtomList *>( pCommentList ) )
									pTorrents[ulCount].uiComments = (unsigned int)dynamic_cast<CAtomList *>( pCommentList )->getValuePtr( )->size( );
							}
						}
					}

					// grab data from m_pTags

					if( m_pTags )
					{
						pDicti = m_pTags->getItem( pTorrents[ulCount].strInfoHash );

						if( pDicti && pDicti->isDicti( ) )
						{
							pTag = ( (CAtomDicti *)pDicti )->getItem( "tag" );
							pName = ( (CAtomDicti *)pDicti )->getItem( "name" );
							pUploader = ( (CAtomDicti *)pDicti )->getItem( "uploader" );
							pInfoLink = ( (CAtomDicti *)pDicti )->getItem( "infolink" );
							pIP = ( (CAtomDicti *)pDicti )->getItem( "ip" );
							pRatio = ( (CAtomDicti *)pDicti )->getItem( "ratio" );
							pAllow = ( (CAtomDicti *)pDicti )->getItem( "allow" );

							if( pTag )
								pTorrents[ulCount].strTag = pTag->toString( );

							if( pName )
							{
								// this will overwrite the previous name, ulKey.e. the filename

								pTorrents[ulCount].strName = pName->toString( );
								pTorrents[ulCount].strLowerName = UTIL_ToLower( pTorrents[ulCount].strName );
							}

							if( pUploader )
								pTorrents[ulCount].strUploader = pUploader->toString( );

							if( pInfoLink )
								pTorrents[ulCount].strInfoLink = pInfoLink->toString( );

							if( pIP )
								pTorrents[ulCount].strIP = pIP->toString( );
							
							if( pRatio )
								pTorrents[ulCount].strRatio = pRatio->toString( );
							
						}
					}

					// grab data from m_pFastCache  

					// Refresh the fast cache
					if( GetTime( ) > m_ulRefreshFastCacheNext )
					{
						// Refresh
						RefreshFastCache( );

						// Set the next refresh time
						m_ulRefreshFastCacheNext = GetTime( ) + m_uiRefreshFastCacheInterval;
					}

					if( m_bCountUniquePeers )
					{
						if( m_pIPs->getValuePtr( )->size( ) > gtXStats.peer.iGreatestUnique )
							gtXStats.peer.iGreatestUnique = m_pIPs->getValuePtr( )->size( );
					}

					pFC = m_pFastCache->getItem( (*ulKey).first );

					if( pFC && dynamic_cast<CAtomList *>( pFC ) )
					{
						vecTorrent = dynamic_cast<CAtomList *>( pFC )->getValue( );

						pTorrents[ulCount].uiSeeders = dynamic_cast<CAtomInt *>( vecTorrent[0] )->getValue( );
						pTorrents[ulCount].uiLeechers = dynamic_cast<CAtomInt *>( vecTorrent[1] )->getValue( );
						pTorrents[ulCount].ulCompleted = dynamic_cast<CAtomInt *>( vecTorrent[2] )->getValue( );

						if( pTorrents[ulCount].uiLeechers > 0 )
							pTorrents[ulCount].iAverageLeft = dynamic_cast<CAtomLong *>( vecTorrent[3] )->getValue( ) / pTorrents[ulCount].uiLeechers;

						pTorrents[ulCount].iMinLeft = dynamic_cast<CAtomLong *>( vecTorrent[4] )->getValue( );
						pTorrents[ulCount].iMaxiLeft = dynamic_cast<CAtomLong *>( vecTorrent[5] )->getValue( );
					}

					// misc calculations

					if( m_pAllowed && m_bShowTransferred )
						pTorrents[ulCount].iTransferred = pTorrents[ulCount].ulCompleted * pTorrents[ulCount].iSize;

					if( pTorrents[ulCount].iSize > 0 )
						pTorrents[ulCount].ucAverageLeftPercent = (unsigned char)( ( (float)pTorrents[ulCount].iAverageLeft / pTorrents[ulCount].iSize ) * 100 );
					
					if( access( string( m_strTopDir + UTIL_HashToString( pTorrents[ulCount].strInfoHash ) ).c_str( ), 0 ) == 0 )
						 pTorrents[ulCount].iTop = 1;
					
					if( access( string( m_strHLDir + UTIL_HashToString( pTorrents[ulCount].strInfoHash ) ).c_str( ), 0 ) == 0 )
						 pTorrents[ulCount].iHL = 1;
					
					time_t now_t = time( NULL );
					struct tm *now_tm, time_tm;
					// int64 year, month, day, hour, minute, second, day_passed, hour_passed, day_limit;
					int64 year, month, day, hour, minute, second, day_left = -1, hour_left, day_limit;
					float passed;
					now_tm = localtime( &now_t );
					time_tm = *now_tm;
					sscanf( pTorrents[ulCount].strAdded.c_str( ), "%d-%d-%d %d:%d:%d",&year,&month,&day,&hour,&minute,&second );
					time_tm.tm_year = year-1900;
					time_tm.tm_mon = month-1;
					time_tm.tm_mday = day;
					time_tm.tm_hour = hour;
					time_tm.tm_min = minute;
					time_tm.tm_sec = second;
					passed = difftime(now_t,mktime(&time_tm));
					// day_passed = ( int ) passed / 86400;
					// hour_passed = ( int ) passed % 86400 / 3600;
					day_limit = CFG_GetInt( "bnbt_ratio_day_limit", 3 );
					
					float ratio;
					string Allow = "1";
					if( pTorrents[ulCount].ulCompleted != 0 )
						if( pTorrents[ulCount].uiSeeders != 0 )
							ratio = (float)pTorrents[ulCount].uiSeeders / pTorrents[ulCount].ulCompleted;
						else
							ratio = (float)( pTorrents[ulCount].uiSeeders + 1 ) / pTorrents[ulCount].ulCompleted;
					else
						ratio = 2;
					for( int iratio = 0; iratio < 4; iratio++ )
						if( ( strRatio[iratio] == pTorrents[ulCount].strRatio ) && ( (iratio+1)/20.0 > ratio ) && ( day_limit * 86400 - ( int ) passed > 0 ) )
						{
							Allow = "0";
							pTorrents[ulCount].iAllow = 0;
						}
					if( !( pAllow->toString( ) == Allow ) )
						addTag( pTorrents[ulCount].strInfoHash, pTorrents[ulCount].strTag, "", "", "", "", "", "", Allow );
				}

				ulCount++;
			}

			// Sort
			const string strSort( pRequest->mapParams["sort"] );
			
			if( m_bSort )
			{
				const unsigned char cucSort( (unsigned char)atoi( strSort.c_str( ) ) );

				if( !strSort.empty( ) )
				{
					switch( cucSort )
					{
					case SORT_ANAME:
						if( m_pAllowed && m_bShowNames )
							qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), asortByName );
						break;
					case SORT_ACOMPLETE:
						qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), asortByComplete );
						break;
					case SORT_AINCOMPLETE:
						qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), asortByDL );
						break;
					case SORT_AADDED:
						if( m_pAllowed && m_bShowAdded )
							qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), asortByAdded );
						break;
					case SORT_ASIZE:
						if( m_pAllowed && m_bShowSize )
							qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), asortBySize );
						break;
					case SORT_AFILES:
						if( m_pAllowed && m_bShowNumFiles )
							qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), asortByFiles );
						break;
					case SORT_ACOMMENTS:
						if( m_pAllowed && m_bAllowComments )
							qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), asortByComments );
						break;
					case SORT_AAVGLEFT:
						if( m_bShowAverageLeft )
						{
							if( m_pAllowed )
							{
								if( m_bShowLeftAsProgress )
									qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByAvgLeftPercent );
								else
									qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), asortByAvgLeftPercent );
							}
							else
								qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), asortByAvgLeft );
						}
						break;
					case SORT_ACOMPLETED:
						if( m_bShowCompleted )
							qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), asortByCompleted );
						break;
					case SORT_ATRANSFERRED:
						if( m_pAllowed && m_bShowTransferred )
							qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), asortByTransferred );
						break;
					case SORT_ATAG:
						qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), asortByTag );
						break;
					case SORT_AUPLOADER:
						if( m_bShowUploader )
							qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), asortByUploader );
						break;
					case SORT_AIP:
						if( pRequest->user.ucAccess & ACCESS_ADMIN )
							qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), asortByIP );
						break;
					case SORT_DNAME:
						if( m_pAllowed && m_bShowNames )
							qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByName );
						break;
					case SORT_DCOMPLETE:
						qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByComplete );
						break;
					case SORT_DINCOMPLETE:
						qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByDL );
						break;
					case SORT_DADDED:
						if( m_pAllowed && m_bShowAdded )
							qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByAdded );
						break;
					case SORT_DSIZE:
						if( m_pAllowed && m_bShowSize )
							qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortBySize );
						break;
					case SORT_DFILES:
						if( m_pAllowed && m_bShowNumFiles )
							qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByFiles );
						break;
					case SORT_DCOMMENTS:
						if( m_pAllowed && m_bAllowComments )
							qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByComments );
						break;
					case SORT_DAVGLEFT:
						if( m_bShowAverageLeft )
						{
							if( m_pAllowed )
							{
								if( m_bShowLeftAsProgress )
									qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), asortByAvgLeftPercent );
								else
									qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByAvgLeftPercent );
							}
							else
								qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByAvgLeft );
						}
						break;
					case SORT_DCOMPLETED:
						if( m_bShowCompleted )
							qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByCompleted );
						break;
					case SORT_DTRANSFERRED:
						if( m_pAllowed && m_bShowTransferred )
							qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByTransferred );
						break;
					case SORT_DTAG:
						qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByTag );
						break;
					case SORT_DUPLOADER:
						if( m_bShowUploader )
							qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByUploader );
						break;
					case SORT_DIP:
						if( pRequest->user.ucAccess & ACCESS_ADMIN )
							qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByIP );
						break;
					default:
						// default action is to sort by added if we can
						if( m_pAllowed && m_bShowAdded )
							qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByAdded );
					}
				}
				else
				{
					// default action is to sort by added if we can

					if( m_pAllowed && m_bShowAdded )
						qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByAdded );
				}
			}
			else
			{
				// sort is disabled, but default action is to sort by added if we can

				if( m_pAllowed && m_bShowAdded )
					qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByAdded );
			}

			// Main Header
			HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["index_page"], string( CSS_INDEX ), string( ), IS_INDEX, CODE_200 );
	
			// javascript
			pResponse->strContent += "<script type=\"text/javascript\">\n";
			pResponse->strContent += "<!--\n";

			pResponse->strContent += "function delete_torrent_confirm( TORRENT )\n";
			pResponse->strContent += "{\n";
			pResponse->strContent += "var name=confirm(\"" + gmapLANG_CFG["delete"] + " xbnbt_announce_list\" + TORRENT )\n";
			pResponse->strContent += "if (name==true)\n";
			pResponse->strContent += "{\n";
			pResponse->strContent += "window.location=\"" + RESPONSE_STR_USERS_HTML + "?up_deluser=\" + TORRENT\n";
			pResponse->strContent += "}\n";
			pResponse->strContent += "}\n\n";

			pResponse->strContent += "function clear_search_and_filters( ) {\n";
			pResponse->strContent += "window.location=\"" + RESPONSE_STR_INDEX_HTML + "\"\n";
			pResponse->strContent += "}\n\n";

			pResponse->strContent += "//-->\n";
			pResponse->strContent += "</script>\n\n";

			// some preliminary search crap

			const string strSearch( pRequest->mapParams["search"] );
			const string strLowerSearch( UTIL_ToLower( strSearch ) );
			const string strSearchResp( UTIL_StringToEscaped( strSearch ) );

			if( !strSearch.empty( ) && m_pAllowed && m_bShowNames && m_bSearch )
				pResponse->strContent += "<p class=\"search_results\">" + UTIL_Xsprintf( gmapLANG_CFG["search_results_for"].c_str( ), string( "\"<span class=\"filtered_by\">" + UTIL_RemoveHTML( strSearch ) + "</span>\"").c_str( ) ) + "</p>\n";

			// filters

			const string strFilter( pRequest->mapParams["filter"] );

			// Sub-filtering mode
			const string cstrMode( pRequest->mapParams["mode"] );

			const string cstrUploader( pRequest->mapParams["uploader"] );

			const string cstrPerPage( pRequest->mapParams["per_page"] );

			if( !m_vecTags.empty( ) )
			{
				pResponse->strContent += "<div class=\"index_filter\">\n\n";

				bool bFound = false;

				vector< pair< string, string > > :: iterator it2 = m_vecTagsMouse.begin( );

				unsigned char ucTag = 1;

				for( vector< pair< string, string > > :: iterator ulTagKey = m_vecTags.begin( ); ulTagKey != m_vecTags.end( ); ulTagKey++ )
				{
					if( !bFound )

						// Added by =Xotic= 
						// The Trinity Edition 7.5r3 - Modification Begins
						// The following changes the CLEAR FILTER link to read
						// CLEAR FILTER AND SEARCH RESULTS which appears before the
						// Table of Torrents
						// ulTagKey also sets a CSS class "clearfilter" which can be used
						// to HIDE this link using the following CSS command 
						// .clearfilter{display:none}

						pResponse->strContent += "<p class=\"clearfilter\"><a title=\"" + UTIL_RemoveHTML( gmapLANG_CFG["clear_filter_search"] ) + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "\">" + gmapLANG_CFG["clear_filter_search"] + "</a></p>\n\n";

					if( !(*ulTagKey).first.empty( ) )
// 						pResponse->strContent += "<a title=\"" + UTIL_RemoveHTML( gmapLANG_CFG["filter_by"] + ": " + (*ulTagKey).first ) + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?filter=" + UTIL_StringToEscaped( (*ulTagKey).first );
						pResponse->strContent += "<a title=\"" + UTIL_RemoveHTML( gmapLANG_CFG["filter_by"] + ": " + (*ulTagKey).first ) + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?filter=" + CAtomInt( ucTag ).toString( );
					
					if( !cstrPerPage.empty( ) )
						pResponse->strContent += "&amp;per_page=" + cstrPerPage;

					if( !strSort.empty( ) )
						pResponse->strContent += "&amp;sort=" + strSort;

					if( !strSearch.empty( ) )
						pResponse->strContent += "&amp;search=" + strSearchResp;

					if( !cstrUploader.empty( ) )
     		                                pResponse->strContent += "&amp;uploader=" + cstrUploader;
					

					if( !cstrMode.empty( ) )
						pResponse->strContent += "&amp;mode=" + cstrMode;

					pResponse->strContent += "\"";

					// Assigns functions to onMouseOver and onMouseOut for each Tag Image
					// Activated by setting "bnbt_use_mouseovers" to 1
					// Generates code that validates with HTML 4.01 Strict

					if( m_bUseMouseovers && !(*ulTagKey).first.empty( ) && !(*ulTagKey).second.empty( ) && !(*it2).second.empty( ) )
					{
						pResponse->strContent += " onMouseOver=\"hoverOnTag" + CAtomInt( ucTag ).toString( ) + "(); return true\"";
						pResponse->strContent += " onMouseOut=\"hoverOffTag" + CAtomInt( ucTag ).toString( ) + "(); return true\"";
					}

					pResponse->strContent += ">\n";


					if( !(*ulTagKey).first.empty( ) && !(*ulTagKey).second.empty( ) )

						// Sets a TITLE parameter for each Tag Image, so that a TOOLTIP box will popup when
						// the user's mouse pointer hovers over the Tag Image.

						pResponse->strContent += "<img class=\"tag\" src=\"" + (*ulTagKey).second + "\" alt=\"[" + UTIL_RemoveHTML( gmapLANG_CFG["filter_by"] + ": " + (*ulTagKey).first ) + "]\" title=\"" + UTIL_RemoveHTML( gmapLANG_CFG["filter_by"] + ": " + (*ulTagKey).first ) + "\" name=\"bnbt_tag" + CAtomInt( ucTag ).toString( ) + "\">";
					else
						pResponse->strContent += UTIL_RemoveHTML( (*ulTagKey).first );

					pResponse->strContent += "</a>\n\n";  

					// RSS ( Thanks labarks )
					if( m_ucDumpRSSFileMode != 0 )
					{
						const string cstrRSSByTag( rssdump.strURL + rssdump.strName.substr( 0, rssdump.strName.length( ) - rssdump.strExt.length( ) ) + "-" + (*ulTagKey).first + rssdump.strExt );

						if( !cstrRSSByTag.empty( ) )
						{
							if( !rssdump.strName.empty( ) )
							{
								if( !rssdump.strURL.empty( ) )
									pResponse->strContent += "<span class=\"dash\">&nbsp;-&nbsp;</span><a rel=\"" + STR_TARGET_REL + "\" title=\"" + m_strTitle + ": " + gmapLANG_CFG["navbar_rss"] + " - " + (*ulTagKey).first + "\" class=\"rss\" href=\"" + cstrRSSByTag + "\">" + gmapLANG_CFG["navbar_rss"] + "</a>\n";
								else if( m_bServeLocal )
									pResponse->strContent += "<span class=\"dash\">&nbsp;-&nbsp;</span><a rel=\"" + STR_TARGET_REL + "\" title=\"" + m_strTitle + ": " + gmapLANG_CFG["navbar_rss"] + " - " + (*ulTagKey).first + "\" class=\"rss\" href=\"" + cstrRSSByTag + "\">" + gmapLANG_CFG["navbar_rss"] + "</a>\n";
							}
						}
					}

					if( ulTagKey + 1 != m_vecTags.end( ) )
						pResponse->strContent += "<span class=\"pipe\">|</span>\n";

					bFound = true;
					ucTag++;
					it2++;
				}
				pResponse->strContent += "</div>\n\n";
			}

			// Sub-filtering mode
			pResponse->strContent += "<p class=\"subfilter\">" + gmapLANG_CFG["index_subfilter"] + ": \n";

			// All
			pResponse->strContent += "<a title=\"" + gmapLANG_CFG["subfilter_all"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?";
			
			if( !cstrPerPage.empty( ) )
				pResponse->strContent += "&amp;per_page=" + cstrPerPage;

			if( !strSort.empty( ) )
				pResponse->strContent += "&amp;sort=" + strSort;

			if( !strSearch.empty( ) )
				pResponse->strContent += "&amp;search=" + strSearchResp;

			if( !strFilter.empty( ) )
				pResponse->strContent += "&amp;filter=" + UTIL_StringToEscaped( strFilter );

			pResponse->strContent += "\">" + gmapLANG_CFG["subfilter_all"] + "</a>\n\n";

			// Seeded
			pResponse->strContent += "<span class=\"pipe\">|</span>\n";
			pResponse->strContent += "<a title=\"" + gmapLANG_CFG["subfilter_seeded"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?mode=Seeded";

			if( !cstrPerPage.empty( ) )
				pResponse->strContent += "&amp;per_page=" + cstrPerPage;
		
			if( !strSort.empty( ) )
				pResponse->strContent += "&amp;sort=" + strSort;

			if( !strSearch.empty( ) )
				pResponse->strContent += "&amp;search=" + strSearchResp;

			if( !strFilter.empty( ) )
				pResponse->strContent += "&amp;filter=" + UTIL_StringToEscaped( strFilter );

			pResponse->strContent += "\">" + gmapLANG_CFG["subfilter_seeded"] + "</a>\n\n";

			// Unseeded
			pResponse->strContent += "<span class=\"pipe\">|</span>\n";
			pResponse->strContent += "<a title=\"" + gmapLANG_CFG["subfilter_unseeded"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?mode=Unseeded";
			
			if( !cstrPerPage.empty( ) )
				pResponse->strContent += "&amp;per_page=" + cstrPerPage;

			if( !strSort.empty( ) )
				pResponse->strContent += "&amp;sort=" + strSort;

			if( !strSearch.empty( ) )
				pResponse->strContent += "&amp;search=" + strSearchResp;

			if( !strFilter.empty( ) )
				pResponse->strContent += "&amp;filter=" + UTIL_StringToEscaped( strFilter );

			pResponse->strContent += "\">" + gmapLANG_CFG["subfilter_unseeded"] + "</a>\n\n";
			
			// ReqSeeders
			pResponse->strContent += "<span class=\"pipe\">|</span>\n";
			pResponse->strContent += "<a class=\"hot\" title=\"" + gmapLANG_CFG["subfilter_reqseeders"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?mode=ReqSeeders";
			
			if( !cstrPerPage.empty( ) )
				pResponse->strContent += "&amp;per_page=" + cstrPerPage;

			if( !strSort.empty( ) )
				pResponse->strContent += "&amp;sort=" + strSort;

			if( !strSearch.empty( ) )
				pResponse->strContent += "&amp;search=" + strSearchResp;

			if( !strFilter.empty( ) )
				pResponse->strContent += "&amp;filter=" + UTIL_StringToEscaped( strFilter );

			pResponse->strContent += "\">" + gmapLANG_CFG["subfilter_reqseeders"] + "</a>\n\n";

			// MyTorrents
			if ( pRequest->user.ucAccess & ACCESS_UPLOAD )
			{
				pResponse->strContent += "<span class=\"pipe\">|</span>\n";
				pResponse->strContent += "<a title=\"" + gmapLANG_CFG["subfilter_mytorrents"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?mode=MyTorrents";
			
				if( !cstrPerPage.empty( ) )
					pResponse->strContent += "&amp;per_page=" + cstrPerPage;

				if( !strSort.empty( ) )
					pResponse->strContent += "&amp;sort=" + strSort;

				if( !strSearch.empty( ) )
					pResponse->strContent += "&amp;search=" + strSearchResp;

				if( !strFilter.empty( ) )
					pResponse->strContent += "&amp;filter=" + UTIL_StringToEscaped( strFilter );

				pResponse->strContent += "\">" + gmapLANG_CFG["subfilter_mytorrents"] + "</a>\n\n";
			}

			pResponse->strContent += "</p>\n\n";

			// which page are we viewing

			unsigned long ulStart = 0;
			unsigned int uiOverridePerPage = 0;

			if ( cstrPerPage.empty( ) )
			{
				if( pRequest->mapCookies["per_page"].empty( ) )
					uiOverridePerPage = m_uiPerPage;
				else
				{
					uiOverridePerPage = (unsigned int)atoi( pRequest->mapCookies["per_page"].c_str( ) );

					if( uiOverridePerPage > 65534 )
						uiOverridePerPage = m_uiPerPage;
				}
			}
			else
			{
				uiOverridePerPage = (unsigned int)atoi( cstrPerPage.c_str( ) );

				if( uiOverridePerPage > 65534 )
					uiOverridePerPage = m_uiPerPage;
			}

			// Count matching torrents for top of page
			unsigned long ulFound = 0;

			for( unsigned long ulKey = 0; ulKey < ulKeySize; ulKey++ )
			{
				if( !strFilter.empty( ) )
				{
					// only count entries that match the filter
					if( pTorrents[ulKey].strTag != strFilter )
						continue;
				}

				if( !strSearch.empty( ) )
				{
					// only count entries that match the search
					if( pTorrents[ulKey].strLowerName.find( strLowerSearch ) == string :: npos )
						continue;
				}

				if( !cstrUploader.empty( ) )
				{
					if( pTorrents[ulKey].strUploader != cstrUploader )
						continue;
				}

				// check seeded/unseeded torrents
				if( !cstrMode.empty( ) )
				{
					// only count entries that match the mode
					if( cstrMode == "Seeded" )
					{
						if( !pTorrents[ulKey].uiSeeders )
							continue;
					}
					else if( cstrMode == "Unseeded"  )
					{
						if( pTorrents[ulKey].uiSeeders )
							continue;
					}
					else if( cstrMode == "ReqSeeders"  )
					{
// 						if( pTorrents[ulKey].uiSeeders || !pTorrents[ulKey].uiLeechers )
						if( access( string( m_strReqDir + UTIL_HashToString( pTorrents[ulKey].strInfoHash ) ).c_str( ), 0 ) != 0 )
							continue;
					}
					else if( ( cstrMode == "MyTorrents"  ) && ( pRequest->user.ucAccess & ACCESS_UPLOAD ) )
					{
						if( pTorrents[ulKey].strUploader != pRequest->user.strLogin )
							continue;
					}
				}

				ulFound++;
			}

			// search, filter and count messages
			pResponse->strContent += "<p class=\"search_filter\">\n";

			if( !strSearch.empty() )
				pResponse->strContent += "<span class=\"search_results_alt\">" + gmapLANG_CFG["result_search"] + ": \"</span><span class=\"filtered_by\">" + UTIL_RemoveHTML( strSearch ) + "</span>\"\n";

			if( !strSearch.empty() && !strFilter.empty() )
				pResponse->strContent += "<span class=\"search_results_alt\"> - </span>\n";

			if( !strFilter.empty() )
				pResponse->strContent += "<span class=\"search_results_alt\">" + gmapLANG_CFG["result_filter"] + ": \"</span><span class=\"filtered_by\">" + UTIL_RemoveHTML( strFilter ) + "</span>\"\n";

			if( ( !strSearch.empty() || !strFilter.empty() ) && !cstrMode.empty() )
				pResponse->strContent += "<span class=\"search_results_alt\"> - </span>\n";

			if( !cstrMode.empty() )
				pResponse->strContent += "<span class=\"search_results_alt\">" + gmapLANG_CFG["result_subfilter"] + ": \"</span><span class=\"filtered_by\">" + UTIL_RemoveHTML( cstrMode ) + "</span>\"\n";

			pResponse->strContent += "</p>\n\n";

			// How many results?
			switch( ulFound )
			{
			case RESULTS_ZERO:
				pResponse->strContent += "<p class=\"results\">" + gmapLANG_CFG["result_none_found"] + "</p>\n\n";
				break;
			case RESULTS_ONE:
				pResponse->strContent += "<p class=\"results\">" + gmapLANG_CFG["result_1_found"] + "</p>\n\n";
				break;
			default:
				// Many results found
				pResponse->strContent += "<p class=\"results\">" + UTIL_Xsprintf( gmapLANG_CFG["result_x_found"].c_str( ), CAtomInt( ulFound ).toString( ).c_str( ) ) + "</p>\n\n";
			}

			// Top search
			if( m_pAllowed && m_bShowNames && m_bSearch )
			{
				pResponse->strContent += "<form class=\"search_index_top\" name=\"topsearch\" method=\"get\" action=\"" + RESPONSE_STR_INDEX_HTML + "\">\n";

				if( !cstrPerPage.empty( ) )
					pResponse->strContent += "<p><input name=\"per_page\" type=hidden value=\"" + cstrPerPage + "\"></p>\n";

				if( !strSort.empty( ) )
					pResponse->strContent += "<p><input name=\"sort\" type=hidden value=\"" + strSort + "\"></p>\n";

				if( !strFilter.empty( ) )
					pResponse->strContent += "<p><input name=\"filter\" type=hidden value=\"" + UTIL_RemoveHTML( strFilter ) + "\"></p>\n";

				if( !cstrUploader.empty( ) )
					pResponse->strContent += "<p><input name=\"uploader\" type=hidden value=\"" + cstrUploader + "\"></p>\n";

				if( !cstrMode.empty( ) )
					pResponse->strContent += "<p><input name=\"mode\" type=hidden value=\"" + cstrMode + "\"></p>\n";

				if( m_bUseButtons )
				{
					pResponse->strContent += "<p><label for=\"toptorrentsearch\">" + gmapLANG_CFG["torrent_search"] + "</label> <input name=\"search\" id=\"toptorrentsearch\" alt=\"[" + gmapLANG_CFG["torrent_search"] + "]\" type=text size=40>\n";

					pResponse->strContent += Button_Submit( "top_submit_search", gmapLANG_CFG["search"] );
					pResponse->strContent += Button_JS_Link( "top_clear_filter_and_search", gmapLANG_CFG["clear_filter_search"], "clear_search_and_filters( )" );

					pResponse->strContent += "</p>\n";
				}
				else
					pResponse->strContent += "<p><label for=\"toptorrentsearch\">" + gmapLANG_CFG["torrent_search"] + "</label> <input name=\"search\" id=\"toptorrentsearch\" alt=\"[" + gmapLANG_CFG["torrent_search"] + "]\" type=text size=40> <a title=\"" + gmapLANG_CFG["clear_filter_search"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "\">" + gmapLANG_CFG["clear_filter_search"] + "</a></p>\n";

				pResponse->strContent += "</form>\n\n";
			}
			
			
			if( ulFound && uiOverridePerPage > 0 )
			{
				const string cstrPage( pRequest->mapParams["page"] );

				if( !cstrPage.empty( ) )
					ulStart = (unsigned long)atoi( cstrPage.c_str( ) ) * uiOverridePerPage;

				// The Trinity Edition - Modification Begins
				// The following code changes "Page" to "You are viewing page" which appears before the current page number
				// ulKey also creates an internal document link, called "Jump to Page Navigation and Torrent Search" that will
				// bring the user to the bottom of the Table of Torrents
				// Sets a CSS class "pagenavjumplink" which can be used to HIDE this link
				// using the following CSS command: .pagenavjumplink{display:none}

				pResponse->strContent += "<p class=\"pagenum_top\"> " + UTIL_Xsprintf( gmapLANG_CFG["viewing_page_num"].c_str( ), CAtomInt( ( ulStart / uiOverridePerPage ) + 1 ).toString( ).c_str( ) );
				pResponse->strContent += "<span class=\"pagenavjumplinkpipe\">|</span><span class=\"pagenavjumplink\"><a title=\"" + gmapLANG_CFG["index_jump"] + "\" href=\"#search\">" + gmapLANG_CFG["index_jump"] + "</a></span></p>\n\n";

				pResponse->strContent += "<p class=\"pagenum_top_bar\">";
				
				if( ulStart > 0 )
				{
					pResponse->strContent += "<a title=\"" + gmapLANG_CFG["jump_to_page"] + ": " + CAtomInt( ( ulStart / uiOverridePerPage ) ).toString( ) + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?page=" + CAtomInt( ( ulStart / uiOverridePerPage ) - 1 ).toString( );
					
					if( !cstrPerPage.empty( ) )
						pResponse->strContent += "&amp;per_page=" + cstrPerPage;

					if( !strSort.empty( ) )
						pResponse->strContent += "&amp;sort=" + strSort;

					if( !strSearch.empty( ) )
						pResponse->strContent += "&amp;search=" + strSearchResp;

					if( !strFilter.empty( ) )
						pResponse->strContent += "&amp;filter=" + UTIL_StringToEscaped( strFilter );
					if( !cstrUploader.empty( ) )
						pResponse->strContent += "&amp;uploader=" + cstrUploader;

					if( !cstrMode.empty( ) )
						pResponse->strContent += "&amp;mode=" + cstrMode;

					pResponse->strContent += "\">";
				}
				pResponse->strContent += gmapLANG_CFG["last_page"];
				
				if( ulStart > 0 )
						pResponse->strContent += "</a>";
				
				pResponse->strContent += "<span class=\"pipe\"> | </span>";
				
				if( ulStart + uiOverridePerPage < ulFound )
				{
					pResponse->strContent += "<a title=\"" + gmapLANG_CFG["jump_to_page"] + ": " + CAtomInt( ( ulStart / uiOverridePerPage ) + 2 ).toString( ) + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?page=" + CAtomInt( ( ulStart / uiOverridePerPage ) + 1 ).toString( );
					
					if( !cstrPerPage.empty( ) )
						pResponse->strContent += "&amp;per_page=" + cstrPerPage;

					if( !strSort.empty( ) )
						pResponse->strContent += "&amp;sort=" + strSort;

					if( !strSearch.empty( ) )
						pResponse->strContent += "&amp;search=" + strSearchResp;

					if( !strFilter.empty( ) )
						pResponse->strContent += "&amp;filter=" + UTIL_StringToEscaped( strFilter );
					if( !cstrUploader.empty( ) )
						pResponse->strContent += "&amp;uploader=" + cstrUploader;

					if( !cstrMode.empty( ) )
						pResponse->strContent += "&amp;mode=" + cstrMode;

					pResponse->strContent += "\">";
				}
				pResponse->strContent += gmapLANG_CFG["next_page"];
				
				if( ulStart + uiOverridePerPage < ulFound )
						pResponse->strContent += "</a>";
				
				pResponse->strContent += "<br>\n";
				
				
					
				// page numbers
// 				pResponse->strContent += "<p class=\"pagenum_top_bar\">" + gmapLANG_CFG["jump_to_page"] + ": \n";
 	//			if( ulFound )
				pResponse->strContent += gmapLANG_CFG["jump_to_page"] + ": \n";

				for( unsigned long ulPerPage = 0; ulPerPage < ulFound; ulPerPage += uiOverridePerPage )
				{
					pResponse->strContent += " ";

					// don't link to current page
					if( ulPerPage != ulStart )
					{
						pResponse->strContent += "<a title=\"" + gmapLANG_CFG["jump_to_page"] + ": " + CAtomInt( ( ulPerPage / uiOverridePerPage ) + 1 ).toString( ) + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?page=" + CAtomInt( ulPerPage / uiOverridePerPage ).toString( );

						if( !cstrPerPage.empty( ) )
							pResponse->strContent += "&amp;per_page=" + cstrPerPage;

						if( !strSort.empty( ) )
							pResponse->strContent += "&amp;sort=" + strSort;

						if( !strSearch.empty( ) )
							pResponse->strContent += "&amp;search=" + strSearchResp;

						if( !strFilter.empty( ) )
							pResponse->strContent += "&amp;filter=" + UTIL_StringToEscaped( strFilter );
						if( !cstrUploader.empty( ) )
     			                                pResponse->strContent += "&amp;uploader=" + cstrUploader;

						if( !cstrMode.empty( ) )
							pResponse->strContent += "&amp;mode=" + cstrMode;

						pResponse->strContent += "\">";
					}

					pResponse->strContent += CAtomInt( ( ulPerPage / uiOverridePerPage ) + 1 ).toString( );

					if( ulPerPage != ulStart )
						pResponse->strContent += "</a>\n";

					// don't display a bar after the last page
					if( ulPerPage + uiOverridePerPage < ulFound )
						pResponse->strContent += "\n<span class=\"pipe\">|</span>";
				}

				pResponse->strContent += "</p>\n\n";
			}
			

						
			// for correct page numbers after searching
			bool bFound = false;

			unsigned long ulAdded = 0;
			unsigned long ulSkipped = 0;
			unsigned int ucTag = 0;
			unsigned char ucPercent = 0;			
			
			string strTemp = string( );
			
			ulFound = 0;

			for( unsigned long ulKey = 0; ulKey < ulKeySize; ulKey++ )
			{
				if( !strFilter.empty( ) )  
				{    
					// only display entries that match the filter  

					if( pTorrents[ulKey].strTag != strFilter )
						continue;
				}

				if( !strSearch.empty( ) )
				{
					// only display entries that match the search   

					if( pTorrents[ulKey].strLowerName.find( strLowerSearch ) == string :: npos )
						continue;
				}

				// check uploader's torrents
				if( !cstrUploader.empty( ) )
				{
					if( pTorrents[ulKey].strUploader != cstrUploader )
                                                continue;
				}

				if( !cstrMode.empty( ) )
				{
					// only display entries that match the mode
					if( cstrMode == "Seeded" )
					{
						if( !pTorrents[ulKey].uiSeeders )
							continue;
					}
					else if( cstrMode == "Unseeded"  )
					{
						if( pTorrents[ulKey].uiSeeders )
							continue;
					}
					else if( cstrMode == "ReqSeeders"  )
					{
// 						if( pTorrents[ulKey].uiSeeders || !pTorrents[ulKey].uiLeechers )
						if( access( string( m_strReqDir + UTIL_HashToString( pTorrents[ulKey].strInfoHash ) ).c_str( ), 0 ) != 0 )
							continue;
					}
					else if(( cstrMode == "MyTorrents"  ) && ( pRequest->user.ucAccess & ACCESS_UPLOAD ) )
					{
						if( pTorrents[ulKey].strUploader != pRequest->user.strLogin )
							continue;
					}
				}

				ulFound++;

				if( uiOverridePerPage == 0 || ulAdded < uiOverridePerPage )
				{
					// create the table and display the headers first
					if( !bFound )
					{
						// output table headers

						// The following code sets a CSS class "torrent_table" using a <table> tag that can be used
						// to HIDE the entire Table of Torrents using the following CSS command:
						// table.torrent_table{display:none}
						
						pResponse->strContent += "<table class=\"post_table\">\n";
						pResponse->strContent += "<tr>\n";
						pResponse->strContent += "<th class=\"postheader\">" + gmapLANG_CFG["post_header"] + "</th>";
						
						if(  pRequest->user.ucAccess & ACCESS_ADMIN )
							pResponse->strContent += "<th class=\"postadmin\">" + gmapLANG_CFG["admin"] + "</th>";
						
						pResponse->strContent += "</tr>\n";
						
						int64 postcount=0;
						int64 postnum;
						string postfile;
						string postname;
						string strPostName;
						string strPost;
						postfile = "post" + CAtomInt( postcount ).toString( );
						while( access( string( m_strPostDir + postfile + ".name" ).c_str( ), 0 ) == 0 )
						{							
							postcount++;
							postfile = "post" + CAtomInt( postcount ).toString( );
						}
						for( postnum = 0; postnum < postcount; postnum++ )
						{
							postfile = "post" + CAtomInt( postnum ).toString( );
							strPostName = UTIL_ReadFile( string( m_strPostDir + postfile + ".name" ).c_str( ) );
							pResponse->strContent += "<tr>\n";
							pResponse->strContent += "<td class=\"postname\"><a class=\"post\" href=\"" + RESPONSE_STR_STATS_HTML + "?post=" + CAtomInt( postnum ).toString( ) + "\">" + strPostName + "</a></td>";
							if(  pRequest->user.ucAccess & ACCESS_ADMIN )
							{
								if( postnum == 0 )
								{
									if( postnum == postcount - 1 )
										pResponse->strContent += "<td class=\"admin\">[" + gmapLANG_CFG["move_up"] + "][<a href=\"" + RESPONSE_STR_STATS_HTML + "?post=" + CAtomInt( postnum ).toString( ) + "&amp;edit=1\">" + gmapLANG_CFG["edit"] + "</a>][" + gmapLANG_CFG["move_down"] + "]</td>";
									else
										pResponse->strContent += "<td class=\"admin\">[" + gmapLANG_CFG["move_up"] + "][<a href=\"" + RESPONSE_STR_STATS_HTML + "?post=" + CAtomInt( postnum ).toString( ) + "&amp;edit=1\">" + gmapLANG_CFG["edit"] + "</a>][<a href=\"" + RESPONSE_STR_STATS_HTML + "?post=" + CAtomInt( postnum ).toString( ) + "&amp;edit=2\">" + gmapLANG_CFG["move_down"] + "</a>]</td>";
								}
								else
									if( postnum == postcount - 1 )
									{
// 										pResponse->strContent += "<td class=\"admin\">[<a href=\"" + RESPONSE_STR_STATS_HTML + "?post=" + CAtomInt( postnum ).toString( ) + "&amp;edit=0\">" + gmapLANG_CFG["move_up"] + "</a>][<a href=\"" + RESPONSE_STR_STATS_HTML + "?post=" + CAtomInt( postnum ).toString( ) + "&amp;edit=1\">" + gmapLANG_CFG["edit"] + "</a>][<a href=\"" + RESPONSE_STR_STATS_HTML + "?post=" + CAtomInt( postnum ).toString( ) + "&amp;edit=2\">" + gmapLANG_CFG["move_down"] + "</a>]</td>";
										pResponse->strContent += "<td class=\"admin\">[<a href=\"" + RESPONSE_STR_STATS_HTML + "?post=" + CAtomInt( postnum ).toString( ) + "&amp;edit=0\">" + gmapLANG_CFG["move_up"] + "</a>][<a href=\"" + RESPONSE_STR_STATS_HTML + "?post=" + CAtomInt( postnum ).toString( ) + "&amp;edit=1\">" + gmapLANG_CFG["edit"] + "</a>][" + gmapLANG_CFG["move_down"] + "]</td>";
									}
									else
										pResponse->strContent += "<td class=\"admin\">[<a href=\"" + RESPONSE_STR_STATS_HTML + "?post=" + CAtomInt( postnum ).toString( ) + "&amp;edit=0\">" + gmapLANG_CFG["move_up"] + "</a>][<a href=\"" + RESPONSE_STR_STATS_HTML + "?post=" + CAtomInt( postnum ).toString( ) + "&amp;edit=1\">" + gmapLANG_CFG["edit"] + "</a>][<a href=\"" + RESPONSE_STR_STATS_HTML + "?post=" + CAtomInt( postnum ).toString( ) + "&amp;edit=2\">" + gmapLANG_CFG["move_down"] + "</a>]</td>";
							}
							pResponse->strContent += "</tr>\n";
						}
						if(  pRequest->user.ucAccess & ACCESS_ADMIN )
						{
							pResponse->strContent += "<tr>\n";
							pResponse->strContent += "<td class=\"postname\"></td><td class=\"admin\">[<a href=\"" + RESPONSE_STR_STATS_HTML + "?post=new\">" + gmapLANG_CFG["post_new"] + "</a>][<a href=\"" + RESPONSE_STR_STATS_HTML + "?post=del\">" + gmapLANG_CFG["post_del"] + "</a>]</td>";
							pResponse->strContent += "</tr>\n";
						}
						
						pResponse->strContent += "</table>\n<p>\n";

						pResponse->strContent += "<table class=\"torrent_table\" summary=\"files\">\n";

						pResponse->strContent += "<tr>\n";

						// <th> tag

						if( !m_vecTags.empty( ) )     
						{
							pResponse->strContent += "<th class=\"tag\" id=\"tagheader\">" + gmapLANG_CFG["tag"];
							if( m_bSort )
							{
								pResponse->strContent += "<br><div><a class=\"sort\" title=\"" + gmapLANG_CFG["sort_tag_ascending"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?sort=" + SORTSTR_ATAG;

								if( !cstrPerPage.empty( ) )
									pResponse->strContent += "&amp;per_page=" + cstrPerPage;

								if( !strSearch.empty( ) )
									pResponse->strContent += "&amp;search=" + strSearchResp;

								if( !strFilter.empty( ) )
									pResponse->strContent += "&amp;filter=" + UTIL_StringToEscaped( strFilter );
								if( !cstrUploader.empty( ) )
     					                                pResponse->strContent += "&amp;uploader=" + cstrUploader;


								if( !cstrMode.empty( ) )
									pResponse->strContent += "&amp;mode=" + cstrMode;

								pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a class=\"sort\" title=\"" + gmapLANG_CFG["sort_tag_descending"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?sort=" + SORTSTR_DTAG;

								if( !cstrPerPage.empty( ) )
									pResponse->strContent += "&amp;per_page=" + cstrPerPage;

								if( !strSearch.empty( ) )
									pResponse->strContent += "&amp;search=" + strSearchResp;

								if( !strFilter.empty( ) )
									pResponse->strContent += "&amp;filter=" + UTIL_StringToEscaped( strFilter );
								if( !cstrUploader.empty( ) )
     		       			 	                        pResponse->strContent += "&amp;uploader=" + cstrUploader;

								if( !cstrMode.empty( ) )
									pResponse->strContent += "&amp;mode=" + cstrMode;

								pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a></div>";
							}

							pResponse->strContent += "</th>\n";
						}         

						// <th> info hash

						if( m_bShowInfoHash )
							pResponse->strContent += "<th class=\"hash\" id=\"hashheader\">" + gmapLANG_CFG["info_hash"] + "</th>\n";

						// <th> seeders

						pResponse->strContent += "<th class=\"number\" id=\"seedersheader\">" + gmapLANG_CFG["seeders"];

						if( m_bSort )
						{
							pResponse->strContent += "<br><div><a class=\"sort\" title=\"" + gmapLANG_CFG["sort_seeders_ascending"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?sort=" + SORTSTR_ACOMPLETE;

							if( !cstrPerPage.empty( ) )
								pResponse->strContent += "&amp;per_page=" + cstrPerPage;

							if( !strSearch.empty( ) )
								pResponse->strContent += "&amp;search=" + strSearchResp;

							if( !strFilter.empty( ) )
								pResponse->strContent += "&amp;filter=" + UTIL_StringToEscaped( strFilter );
							if( !cstrUploader.empty( ) )
								pResponse->strContent += "&amp;uploader=" + cstrUploader;

							if( !cstrMode.empty( ) )
								pResponse->strContent += "&amp;mode=" + cstrMode;

							pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a class=\"sort\" title=\"" + gmapLANG_CFG["sort_seeders_descending"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?sort=" + SORTSTR_DCOMPLETE;

							if( !cstrPerPage.empty( ) )
								pResponse->strContent += "&amp;per_page=" + cstrPerPage;

							if( !strSearch.empty( ) )
								pResponse->strContent += "&amp;search=" + strSearchResp;

							if( !strFilter.empty( ) )
								pResponse->strContent += "&amp;filter=" + UTIL_StringToEscaped( strFilter );
							if( !cstrUploader.empty( ) )
								pResponse->strContent += "&amp;uploader=" + cstrUploader;

							
							if( !cstrMode.empty( ) )
								pResponse->strContent += "&amp;mode=" + cstrMode;

							pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a></div>";
						}

						pResponse->strContent += "</th>\n";

						// <th> leechers

						pResponse->strContent += "<th class=\"number\" id=\"leechersheader\">" + gmapLANG_CFG["leechers"];

						if( m_bSort )
						{
							pResponse->strContent += "<br><div><a class=\"sort\" title=\"" + gmapLANG_CFG["sort_leechers_ascending"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?sort=" + SORTSTR_AINCOMPLETE;

							if( !cstrPerPage.empty( ) )
								pResponse->strContent += "&amp;per_page=" + cstrPerPage;

							if( !strSearch.empty( ) )
								pResponse->strContent += "&amp;search=" + strSearchResp;

							if( !strFilter.empty( ) )
								pResponse->strContent += "&amp;filter=" + UTIL_StringToEscaped( strFilter );
							if( !cstrUploader.empty( ) )
								pResponse->strContent += "&amp;uploader=" + cstrUploader;

							if( !cstrMode.empty( ) )
								pResponse->strContent += "&amp;mode=" + cstrMode;

							pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a class=\"sort\" title=\"" + gmapLANG_CFG["sort_leechers_descending"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?sort=" + SORTSTR_DINCOMPLETE;

							if( !cstrPerPage.empty( ) )
								pResponse->strContent += "&amp;per_page=" + cstrPerPage;

							if( !strSearch.empty( ) )
								pResponse->strContent += "&amp;search=" + strSearchResp;

							if( !strFilter.empty( ) )
								pResponse->strContent += "&amp;filter=" + UTIL_StringToEscaped( strFilter );
							if( !cstrUploader.empty( ) )
								pResponse->strContent += "&amp;uploader=" + cstrUploader;

							if( !cstrMode.empty( ) )
								pResponse->strContent += "&amp;mode=" + cstrMode;

							pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a></div>";
						}

						pResponse->strContent += "</th>\n";

						// <th> completed

						if( m_bShowCompleted )
						{
							pResponse->strContent += "<th class=\"number\" id=\"completedheader\">" + gmapLANG_CFG["completed"];

							if( m_bSort )
							{
								pResponse->strContent += "<br><div><a class=\"sort\" title=\"" + gmapLANG_CFG["sort_completed_ascending"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?sort=" + SORTSTR_ACOMPLETED;

								if( !cstrPerPage.empty( ) )
									pResponse->strContent += "&amp;per_page=" + cstrPerPage;

								if( !strSearch.empty( ) )
									pResponse->strContent += "&amp;search=" + strSearchResp;

								if( !strFilter.empty( ) )
									pResponse->strContent += "&amp;filter=" + UTIL_StringToEscaped( strFilter );
								if( !cstrUploader.empty( ) )
     		       			 	                        pResponse->strContent += "&amp;uploader=" + cstrUploader;

								if( !cstrMode.empty( ) )
									pResponse->strContent += "&amp;mode=" + cstrMode;

								pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a class=\"sort\" title=\"" + gmapLANG_CFG["sort_completed_descending"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?sort=" + SORTSTR_DCOMPLETED;

								if( !cstrPerPage.empty( ) )
									pResponse->strContent += "&amp;per_page=" + cstrPerPage;

								if( !strSearch.empty( ) )
									pResponse->strContent += "&amp;search=" + strSearchResp;

								if( !strFilter.empty( ) )
									pResponse->strContent += "&amp;filter=" + UTIL_StringToEscaped( strFilter );
								if( !cstrUploader.empty( ) )
     		       			 	                        pResponse->strContent += "&amp;uploader=" + cstrUploader;

								if( !cstrMode.empty( ) )
									pResponse->strContent += "&amp;mode=" + cstrMode;

								pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a></div>";
							}

							pResponse->strContent += "</th>\n";
						}

						// <th> name

						if( m_pAllowed && m_bShowNames )
						{
							pResponse->strContent += "<th class=\"name\" id=\"nameheader\">" + gmapLANG_CFG["name"];

							if( m_bSort )
							{
								pResponse->strContent += "<br><div><a class=\"sort\" title=\"" + gmapLANG_CFG["sort_name_ascending"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?sort=" + SORTSTR_ANAME;

								if( !cstrPerPage.empty( ) )
									pResponse->strContent += "&amp;per_page=" + cstrPerPage;

								if( !strSearch.empty( ) )
									pResponse->strContent += "&amp;search=" + strSearchResp;

								if( !strFilter.empty( ) )
									pResponse->strContent += "&amp;filter=" + UTIL_StringToEscaped( strFilter );
								if( !cstrUploader.empty( ) )
     		       			 	                        pResponse->strContent += "&amp;uploader=" + cstrUploader;

								if( !cstrMode.empty( ) )
									pResponse->strContent += "&amp;mode=" + cstrMode;

								pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a class=\"sort\" title=\"" + gmapLANG_CFG["sort_name_descending"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?sort=" + SORTSTR_DNAME;

								if( !cstrPerPage.empty( ) )
									pResponse->strContent += "&amp;per_page=" + cstrPerPage;

								if( !strSearch.empty( ) )
									pResponse->strContent += "&amp;search=" + strSearchResp;

								if( !strFilter.empty( ) )
									pResponse->strContent += "&amp;filter=" + UTIL_StringToEscaped( strFilter );
								if( !cstrUploader.empty( ) )
     		       			 	                        pResponse->strContent += "&amp;uploader=" + cstrUploader;

								if( !cstrMode.empty( ) )
									pResponse->strContent += "&amp;mode=" + cstrMode;

								pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a></div>";
							}

							pResponse->strContent += "</th>\n";
						}

						// <th> info link

						if( m_bAllowInfoLink )
							pResponse->strContent += "<th class=\"infolink\" id=\"infolinkheader\">" + gmapLANG_CFG["info"] + "</th>\n";
						// <th> torrent    

						if( m_pAllowed && m_bAllowTorrentDownloads && ( pRequest->user.ucAccess & ACCESS_DL ) )
							pResponse->strContent += "<th class=\"download\" id=\"downloadheader\">" + gmapLANG_CFG["torrent"] + "</th>\n";

						// <th> comments

						if( m_pAllowed && m_bAllowComments )
						{
							if( m_pComments )
							{
								pResponse->strContent += "<th class=\"number\" id=\"commentsheader\">" + gmapLANG_CFG["comments"];

								if( m_bSort )
								{
									pResponse->strContent += "<br><div><a class=\"sort\" title=\"" + gmapLANG_CFG["sort_comments_ascending"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?sort=" + SORTSTR_ACOMMENTS;

									if( !cstrPerPage.empty( ) )
										pResponse->strContent += "&amp;per_page=" + cstrPerPage;

									if( !strSearch.empty( ) )
										pResponse->strContent += "&amp;search=" + strSearchResp;

									if( !strFilter.empty( ) )
										pResponse->strContent += "&amp;filter=" + UTIL_StringToEscaped( strFilter );
									if( !cstrUploader.empty( ) )
										pResponse->strContent += "&amp;uploader=" + cstrUploader;

									if( !cstrMode.empty( ) )
										pResponse->strContent += "&amp;mode=" + cstrMode;

									pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a class=\"sort\" title=\"" + gmapLANG_CFG["sort_comments_descending"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?sort=" + SORTSTR_DCOMMENTS;

									if( !cstrPerPage.empty( ) )
										pResponse->strContent += "&amp;per_page=" + cstrPerPage;

									if( !strSearch.empty( ) )
										pResponse->strContent += "&amp;search=" + strSearchResp;

									if( !strFilter.empty( ) )
										pResponse->strContent += "&amp;filter=" + UTIL_StringToEscaped( strFilter );
									if( !cstrUploader.empty( ) )
										pResponse->strContent += "&amp;uploader=" + cstrUploader;

									if( !cstrMode.empty( ) )
										pResponse->strContent += "&amp;mode=" + cstrMode;

									pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a></div>";
								}

								pResponse->strContent += "</th>\n";
							}
						}

						// <th> added

						if( m_pAllowed && m_bShowAdded_Index )
						{
							pResponse->strContent += "<th class=\"date\" id=\"addedheader\">" + gmapLANG_CFG["added"];

							if( m_bSort )
							{
								pResponse->strContent += "<br><div><a class=\"sort\" title=\"" + gmapLANG_CFG["sort_added_ascending"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?sort=" + SORTSTR_AADDED;

								if( !cstrPerPage.empty( ) )
									pResponse->strContent += "&amp;per_page=" + cstrPerPage;

								if( !strSearch.empty( ) )
									pResponse->strContent += "&amp;search=" + strSearchResp;

								if( !strFilter.empty( ) )
									pResponse->strContent += "&amp;filter=" + UTIL_StringToEscaped( strFilter );
								if( !cstrUploader.empty( ) )
     		       			 	                        pResponse->strContent += "&amp;uploader=" + cstrUploader;

								if( !cstrMode.empty( ) )
									pResponse->strContent += "&amp;mode=" + cstrMode;

								pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a class=\"sort\" title=\"" + gmapLANG_CFG["sort_added_descending"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?sort=" + SORTSTR_DADDED;

								if( !cstrPerPage.empty( ) )
									pResponse->strContent += "&amp;per_page=" + cstrPerPage;

								if( !strSearch.empty( ) )
									pResponse->strContent += "&amp;search=" + strSearchResp;

								if( !strFilter.empty( ) )
									pResponse->strContent += "&amp;filter=" + UTIL_StringToEscaped( strFilter );
								if( !cstrUploader.empty( ) )
     		       			 	                        pResponse->strContent += "&amp;uploader=" + cstrUploader;

								if( !cstrMode.empty( ) )
									pResponse->strContent += "&amp;mode=" + cstrMode;

								pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a></div>";
							}

							pResponse->strContent += "</th>\n";
						}

						// <th> size

						if( m_pAllowed && m_bShowSize )
						{
							pResponse->strContent += "<th class=\"bytes\" id=\"sizeheader\">" + gmapLANG_CFG["size"];

							if( m_bSort )
							{
								pResponse->strContent += "<br><div><a class=\"sort\" title=\"" + gmapLANG_CFG["sort_size_ascending"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?sort=" + SORTSTR_ASIZE;

								if( !cstrPerPage.empty( ) )
									pResponse->strContent += "&amp;per_page=" + cstrPerPage;

								if( !strSearch.empty( ) )
									pResponse->strContent += "&amp;search=" + strSearchResp;

								if( !strFilter.empty( ) )
									pResponse->strContent += "&amp;filter=" + UTIL_StringToEscaped( strFilter );
								if( !cstrUploader.empty( ) )
     		       			 	                        pResponse->strContent += "&amp;uploader=" + cstrUploader;

								if( !cstrMode.empty( ) )
									pResponse->strContent += "&amp;mode=" + cstrMode;

								pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a class=\"sort\" title=\"" + gmapLANG_CFG["sort_size_descending"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?sort=" + SORTSTR_DSIZE;

								if( !cstrPerPage.empty( ) )
									pResponse->strContent += "&amp;per_page=" + cstrPerPage;

								if( !strSearch.empty( ) )
									pResponse->strContent += "&amp;search=" + strSearchResp;

								if( !strFilter.empty( ) )
									pResponse->strContent += "&amp;filter=" + UTIL_StringToEscaped( strFilter );
								if( !cstrUploader.empty( ) )
     		       			 	                        pResponse->strContent += "&amp;uploader=" + cstrUploader;

								if( !cstrMode.empty( ) )
									pResponse->strContent += "&amp;mode=" + cstrMode;

								pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a></div>";
							}

							pResponse->strContent += "</th>\n";
						}

						// <th> files

						if( m_pAllowed && m_bShowNumFiles )
						{
							// Modified by =Xotic=
							pResponse->strContent += "<th class=\"number\" id=\"filesheader\">" + gmapLANG_CFG["files"];

							if( m_bSort )
							{
								pResponse->strContent += "<br><div><a class=\"sort\" title=\"" + gmapLANG_CFG["sort_files_ascending"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?sort=" + SORTSTR_AFILES;

								if( !cstrPerPage.empty( ) )
									pResponse->strContent += "&amp;per_page=" + cstrPerPage;

								if( !strSearch.empty( ) )
									pResponse->strContent += "&amp;search=" + strSearchResp;

								if( !strFilter.empty( ) )
									pResponse->strContent += "&amp;filter=" + UTIL_StringToEscaped( strFilter );
								if( !cstrUploader.empty( ) )
     		       			 	                        pResponse->strContent += "&amp;uploader=" + cstrUploader;

								if( !cstrMode.empty( ) )
									pResponse->strContent += "&amp;mode=" + cstrMode;

								pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a class=\"sort\" title=\"" + gmapLANG_CFG["sort_files_descending"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?sort=" + SORTSTR_DFILES;

								if( !cstrPerPage.empty( ) )
									pResponse->strContent += "&amp;per_page=" + cstrPerPage;

								if( !strSearch.empty( ) )
									pResponse->strContent += "&amp;search=" + strSearchResp;

								if( !strFilter.empty( ) )
									pResponse->strContent += "&amp;filter=" + UTIL_StringToEscaped( strFilter );
								if( !cstrUploader.empty( ) )
     		       			 	                        pResponse->strContent += "&amp;uploader=" + cstrUploader;

								if( !cstrMode.empty( ) )
									pResponse->strContent += "&amp;mode=" + cstrMode;

								pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a></div>";
							}

							pResponse->strContent += "</th>\n";
						}

// 						// <th> seeders
// 
// 						pResponse->strContent += "<th class=\"number\" id=\"seedersheader\">" + gmapLANG_CFG["seeders"];
// 
// 						if( m_bSort )
// 						{
// 							pResponse->strContent += "<br><div><a class=\"sort\" title=\"" + gmapLANG_CFG["sort_seeders_ascending"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?sort=" + SORTSTR_ACOMPLETE;
// 
// 							if( !cstrPerPage.empty( ) )
// 								pResponse->strContent += "&amp;per_page=" + cstrPerPage;
// 
// 							if( !strSearch.empty( ) )
// 								pResponse->strContent += "&amp;search=" + strSearchResp;
// 
// 							if( !strFilter.empty( ) )
// 								pResponse->strContent += "&amp;filter=" + UTIL_StringToEscaped( strFilter );
// 
// 							if( !cstrMode.empty( ) )
// 								pResponse->strContent += "&amp;mode=" + cstrMode;
// 
// 							pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a class=\"sort\" title=\"" + gmapLANG_CFG["sort_seeders_descending"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?sort=" + SORTSTR_DCOMPLETE;
// 
// 							if( !cstrPerPage.empty( ) )
// 								pResponse->strContent += "&amp;per_page=" + cstrPerPage;
// 
// 							if( !strSearch.empty( ) )
// 								pResponse->strContent += "&amp;search=" + strSearchResp;
// 
// 							if( !strFilter.empty( ) )
// 								pResponse->strContent += "&amp;filter=" + UTIL_StringToEscaped( strFilter );
// 
// 							if( !cstrMode.empty( ) )
// 								pResponse->strContent += "&amp;mode=" + cstrMode;
// 
// 							pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a></div>";
// 						}
// 
// 						pResponse->strContent += "</th>\n";
// 
// 						// <th> leechers
// 
// 						pResponse->strContent += "<th class=\"number\" id=\"leechersheader\">" + gmapLANG_CFG["leechers"];
// 
// 						if( m_bSort )
// 						{
// 							pResponse->strContent += "<br><div><a class=\"sort\" title=\"" + gmapLANG_CFG["sort_leechers_ascending"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?sort=" + SORTSTR_AINCOMPLETE;
// 
// 							if( !cstrPerPage.empty( ) )
// 								pResponse->strContent += "&amp;per_page=" + cstrPerPage;
// 
// 							if( !strSearch.empty( ) )
// 								pResponse->strContent += "&amp;search=" + strSearchResp;
// 
// 							if( !strFilter.empty( ) )
// 								pResponse->strContent += "&amp;filter=" + UTIL_StringToEscaped( strFilter );
// 
// 							if( !cstrMode.empty( ) )
// 								pResponse->strContent += "&amp;mode=" + cstrMode;
// 
// 							pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a class=\"sort\" title=\"" + gmapLANG_CFG["sort_leechers_descending"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?sort=" + SORTSTR_DINCOMPLETE;
// 
// 							if( !cstrPerPage.empty( ) )
// 								pResponse->strContent += "&amp;per_page=" + cstrPerPage;
// 
// 							if( !strSearch.empty( ) )
// 								pResponse->strContent += "&amp;search=" + strSearchResp;
// 
// 							if( !strFilter.empty( ) )
// 								pResponse->strContent += "&amp;filter=" + UTIL_StringToEscaped( strFilter );
// 
// 							if( !cstrMode.empty( ) )
// 								pResponse->strContent += "&amp;mode=" + cstrMode;
// 
// 							pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a></div>";
// 						}
// 
// 						pResponse->strContent += "</th>\n";
// 
// 						// <th> completed
// 
// 						if( m_bShowCompleted )
// 						{
// 							pResponse->strContent += "<th class=\"number\" id=\"completedheader\">" + gmapLANG_CFG["completed"];
// 
// 							if( m_bSort )
// 							{
// 								pResponse->strContent += "<br><div><a class=\"sort\" title=\"" + gmapLANG_CFG["sort_completed_ascending"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?sort=" + SORTSTR_ACOMPLETED;
// 
// 								if( !cstrPerPage.empty( ) )
// 									pResponse->strContent += "&amp;per_page=" + cstrPerPage;
// 
// 								if( !strSearch.empty( ) )
// 									pResponse->strContent += "&amp;search=" + strSearchResp;
// 
// 								if( !strFilter.empty( ) )
// 									pResponse->strContent += "&amp;filter=" + UTIL_StringToEscaped( strFilter );
// 
// 								if( !cstrMode.empty( ) )
// 									pResponse->strContent += "&amp;mode=" + cstrMode;
// 
// 								pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a class=\"sort\" title=\"" + gmapLANG_CFG["sort_completed_descending"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?sort=" + SORTSTR_DCOMPLETED;
// 
// 								if( !cstrPerPage.empty( ) )
// 									pResponse->strContent += "&amp;per_page=" + cstrPerPage;
// 
// 								if( !strSearch.empty( ) )
// 									pResponse->strContent += "&amp;search=" + strSearchResp;
// 
// 								if( !strFilter.empty( ) )
// 									pResponse->strContent += "&amp;filter=" + UTIL_StringToEscaped( strFilter );
// 
// 								if( !cstrMode.empty( ) )
// 									pResponse->strContent += "&amp;mode=" + cstrMode;
// 
// 								pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a></div>";
// 							}
// 
// 							pResponse->strContent += "</th>\n";
// 						}

						// <th> transferred

						if( m_pAllowed && m_bShowTransferred )
						{
							pResponse->strContent += "<th class=\"bytes\" id=\"transferredheader\">" + gmapLANG_CFG["transferred"];

							if( m_bSort )
							{
								pResponse->strContent += "<br><div><a class=\"sort\" title=\"" + gmapLANG_CFG["sort_transferred_ascending"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?sort=" + SORTSTR_ATRANSFERRED;

								if( !cstrPerPage.empty( ) )
									pResponse->strContent += "&amp;per_page=" + cstrPerPage;

								if( !strSearch.empty( ) )
									pResponse->strContent += "&amp;search=" + strSearchResp;

								if( !strFilter.empty( ) )
									pResponse->strContent += "&amp;filter=" + UTIL_StringToEscaped( strFilter );
								if( !cstrUploader.empty( ) )
     		       			 	                        pResponse->strContent += "&amp;uploader=" + cstrUploader;

								if( !cstrMode.empty( ) )
									pResponse->strContent += "&amp;mode=" + cstrMode;

								pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a class=\"sort\" title=\"" + gmapLANG_CFG["sort_transferred_descending"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?sort=" + SORTSTR_DTRANSFERRED;

								if( !cstrPerPage.empty( ) )
									pResponse->strContent += "&amp;per_page=" + cstrPerPage;

								if( !strSearch.empty( ) )
									pResponse->strContent += "&amp;search=" + strSearchResp;

								if( !strFilter.empty( ) )
									pResponse->strContent += "&amp;filter=" + UTIL_StringToEscaped( strFilter );
								if( !cstrUploader.empty( ) )
     		       			 	                        pResponse->strContent += "&amp;uploader=" + cstrUploader;

								if( !cstrMode.empty( ) )
									pResponse->strContent += "&amp;mode=" + cstrMode;

								pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a></div>";
							}

							pResponse->strContent += "</th>\n";
						}

						// <th> min left

						if( m_bShowMinLeft )
						{
							if( m_pAllowed && m_bShowLeftAsProgress )
								pResponse->strContent += "<th class=\"percent\" id=\"minprogressheader\">" + gmapLANG_CFG["min_progress"] + "</th>\n";
							else
								pResponse->strContent += "<th class=\"percent\" id=\"minleftheader\">" + gmapLANG_CFG["min_left"] + "</th>\n";
						}

						// <th> average left

						if( m_bShowAverageLeft )
						{
							if( m_pAllowed && m_bShowLeftAsProgress )
								pResponse->strContent += "<th class=\"percent\" id=\"avgprogressheader\">" + gmapLANG_CFG["avg_progress"];
							else
								pResponse->strContent += "<th class=\"percent\" id=\"avgleftheader\">" + gmapLANG_CFG["avg_left"];

							if( m_bSort )
							{
								pResponse->strContent += "<br><div><a class=\"sort\" title=\"" + gmapLANG_CFG["sort_avgleft_ascending"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?sort=" + SORTSTR_AAVGLEFT;

								if( !cstrPerPage.empty( ) )
									pResponse->strContent += "&amp;per_page=" + cstrPerPage;

								if( !strSearch.empty( ) )
									pResponse->strContent += "&amp;search=" + strSearchResp;

								if( !strFilter.empty( ) )
									pResponse->strContent += "&amp;filter=" + UTIL_StringToEscaped( strFilter );
								if( !cstrUploader.empty( ) )
     		       			 	                        pResponse->strContent += "&amp;uploader=" + cstrUploader;

								if( !cstrMode.empty( ) )
									pResponse->strContent += "&amp;mode=" + cstrMode;

								pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a class=\"sort\" title=\"" + gmapLANG_CFG["sort_avgleft_descending"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?sort=" + SORTSTR_DAVGLEFT;

								if( !cstrPerPage.empty( ) )
									pResponse->strContent += "&amp;per_page=" + cstrPerPage;

								if( !strSearch.empty( ) )
									pResponse->strContent += "&amp;search=" + strSearchResp;

								if( !strFilter.empty( ) )
									pResponse->strContent += "&amp;filter=" + UTIL_StringToEscaped( strFilter );
								if( !cstrUploader.empty( ) )
     		       			 	                        pResponse->strContent += "&amp;uploader=" + cstrUploader;

								if( !cstrMode.empty( ) )
									pResponse->strContent += "&amp;mode=" + cstrMode;

								pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a></div>";
							}

							pResponse->strContent += "</th>\n";
						}

						// <th> maxi left

						if( m_bShowMaxiLeft )
						{
							if( m_pAllowed && m_bShowLeftAsProgress )
								pResponse->strContent += "<th class=\"percent\" id=\"maxprogressheader\">" + gmapLANG_CFG["max_progress"] + "</th>\n";
							else
								pResponse->strContent += "<th class=\"percent\" id=\"maxleftheader\">" + gmapLANG_CFG["max_left"] + "</th>\n";
						}

						// <th> uploader

						if( m_bShowUploader )
						{
							pResponse->strContent += "<th class=\"uploader\" id=\"uploaderheader\">" + gmapLANG_CFG["uploader"];
							if( m_bSort )
							{
								pResponse->strContent += "<br><div><a class=\"sort\" title=\"" + gmapLANG_CFG["sort_uploader_ascending"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?sort=" + SORTSTR_AUPLOADER;

								if( !cstrPerPage.empty( ) )
									pResponse->strContent += "&amp;per_page=" + cstrPerPage;

								if( !strSearch.empty( ) )
									pResponse->strContent += "&amp;search=" + strSearchResp;

								if( !strFilter.empty( ) )
									pResponse->strContent += "&amp;filter=" + UTIL_StringToEscaped( strFilter );
								if( !cstrUploader.empty( ) )
     		       			 	                        pResponse->strContent += "&amp;uploader=" + cstrUploader;

								if( !cstrMode.empty( ) )
									pResponse->strContent += "&amp;mode=" + cstrMode;

								pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a class=\"sort\" title=\"" + gmapLANG_CFG["sort_uploader_descending"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?sort=" + SORTSTR_DUPLOADER;

								if( !cstrPerPage.empty( ) )
									pResponse->strContent += "&amp;per_page=" + cstrPerPage;

								if( !strSearch.empty( ) )
									pResponse->strContent += "&amp;search=" + strSearchResp;

								if( !strFilter.empty( ) )
									pResponse->strContent += "&amp;filter=" + UTIL_StringToEscaped( strFilter );
								if( !cstrUploader.empty( ) )
     		       			 	                        pResponse->strContent += "&amp;uploader=" + cstrUploader;

								if( !cstrMode.empty( ) )
									pResponse->strContent += "&amp;mode=" + cstrMode;

								pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a></div>";
							}

							pResponse->strContent += "</th>\n";
						}

						// <th> ip

						if( m_bShowIP && ( pRequest->user.ucAccess & ACCESS_ADMIN ) )
						{
							pResponse->strContent += "<th class=\"ip\" id=\"ipheader\">" + gmapLANG_CFG["ip"];
							if( m_bSort )
							{
								pResponse->strContent += "<br><div><a class=\"sort\" title=\"" + gmapLANG_CFG["sort_ip_ascending"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?sort=" + SORTSTR_AIP;

								if( !cstrPerPage.empty( ) )
									pResponse->strContent += "&amp;per_page=" + cstrPerPage;

								if( !strSearch.empty( ) )
									pResponse->strContent += "&amp;search=" + strSearchResp;

								if( !strFilter.empty( ) )
									pResponse->strContent += "&amp;filter=" + UTIL_StringToEscaped( strFilter );
								if( !cstrUploader.empty( ) )
     		       			 	                        pResponse->strContent += "&amp;uploader=" + cstrUploader;

								if( !cstrMode.empty( ) )
									pResponse->strContent += "&amp;mode=" + cstrMode;

								pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a class=\"sort\" title=\"" + gmapLANG_CFG["sort_ip_descending"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?sort=" + SORTSTR_DIP;

								if( !cstrPerPage.empty( ) )
									pResponse->strContent += "&amp;per_page=" + cstrPerPage;

								if( !strSearch.empty( ) )
									pResponse->strContent += "&amp;search=" + strSearchResp;

								if( !strFilter.empty( ) )
									pResponse->strContent += "&amp;filter=" + UTIL_StringToEscaped( strFilter );
								if( !cstrUploader.empty( ) )
     		       			 	                        pResponse->strContent += "&amp;uploader=" + cstrUploader;

								if( !cstrMode.empty( ) )
									pResponse->strContent += "&amp;mode=" + cstrMode;

								pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a></div>";
							}

							pResponse->strContent += "</th>\n";
						}

						// <th> info link

						// if( m_bAllowInfoLink )
							// pResponse->strContent += "<th class=\"infolink\" id=\"infolinkheader\">" + gmapLANG_CFG["info"] + "</th>\n";

						// <th> admin
						
// 						if( pRequest->user.ucAccess & ACCESS_EDIT )
// 						{
// 							if( m_pAllowed )
// 								pResponse->strContent += "<th id=\"topheader\">" + gmapLANG_CFG["top"] + "</th>\n";   
// 						}

						if( pRequest->user.ucAccess & ACCESS_EDIT )
						{
							if( m_pAllowed )
								pResponse->strContent += "<th id=\"adminheader\">" + gmapLANG_CFG["admin"] + "</th>\n";   
						}

						pResponse->strContent += "</tr>\n";

						// signal table created
						bFound = true;
					}

					if( ulSkipped == ulStart )
					{
						// output table rows

						if( ulAdded % 2 )
							pResponse->strContent += "<tr class=\"even\">\n";
						else
							pResponse->strContent += "<tr class=\"odd\">\n";

						// <td> tag 

						if( !m_vecTags.empty( ) )
						{
							pResponse->strContent += "<td>";

							strTemp = pTorrents[ulKey].strTag;

							ucTag = 1;

							for( vector< pair< string, string > > :: iterator it2 = m_vecTags.begin( ); it2 != m_vecTags.end( ); it2++ )
							{
// 								if( (*it2).first == pTorrents[ulKey].strTag )
								if( CAtomInt( ucTag ).toString( ) == pTorrents[ulKey].strTag )
								{
// 									pResponse->strContent += "<a title=\"" + UTIL_RemoveHTML( gmapLANG_CFG["filter_by"] + ": " + pTorrents[ulKey].strTag ) + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?filter=" + UTIL_StringToEscaped( (*it2).first );
									pResponse->strContent += "<a title=\"" + UTIL_RemoveHTML( gmapLANG_CFG["filter_by"] + ": " + (*it2).first ) + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?filter=" + CAtomInt( ucTag ).toString( );

									if( !cstrPerPage.empty( ) )
										pResponse->strContent += "&amp;per_page=" + cstrPerPage;

									if( !strSort.empty( ) )
										pResponse->strContent += "&amp;sort=" + strSort;

									if( !strSearch.empty( ) )
										pResponse->strContent += "&amp;search=" + strSearchResp;
									if( !cstrUploader.empty( ) )
										pResponse->strContent += "&amp;uploader=" + cstrUploader;

									if( !cstrMode.empty( ) )
										pResponse->strContent += "&amp;mode=" + cstrMode;

									pResponse->strContent += "\"";

									// Assigns functions to onMouseOver and onMouseOut for each Tag Image
									// Activated by setting "bnbt_use_mouseovers" to 1
									// Generates code that validates with HTML 4.01 Strict

									if( m_bUseMouseovers )
									{
										pResponse->strContent += " onMouseOver=\"hoverOnTag" + CAtomInt( ucTag ).toString( ) + "(); return true\"";
										pResponse->strContent += " onMouseOut=\"hoverOffTag" + CAtomInt( ucTag ).toString( ) + "(); return true\"";
									}

									pResponse->strContent += ">\n";

									if( !(*it2).second.empty( ) )

										// Sets a TITLE parameter for each Tag Image, so that a TOOLTIP box will popup when
										// the user's mouse pointer hovers over the Tag Image.

// 										pResponse->strContent += "<img src=\"" + (*it2).second + "\" alt=\"[" + UTIL_RemoveHTML( gmapLANG_CFG["filter_by"] + ": "+ pTorrents[ulKey].strTag ) + "]\" title=\"" + UTIL_RemoveHTML( gmapLANG_CFG["filter_by"] + ": " + pTorrents[ulKey].strTag ) + "\" name=\"" + CAtomInt( ulAdded ).toString( ) + "xbnbt_tag" + CAtomInt( ucTag ).toString( ) + "\">";
										pResponse->strContent += "<img src=\"" + (*it2).second + "\" alt=\"[" + UTIL_RemoveHTML( gmapLANG_CFG["filter_by"] + ": "+ (*it2).first ) + "]\" title=\"" + UTIL_RemoveHTML( gmapLANG_CFG["filter_by"] + ": " + (*it2).first ) + "\" name=\"" + CAtomInt( ulAdded ).toString( ) + "xbnbt_tag" + CAtomInt( ucTag ).toString( ) + "\">";
									else
										pResponse->strContent += (*it2).first;
// 										pResponse->strContent += UTIL_RemoveHTML( pTorrents[ulKey].strTag );

									pResponse->strContent += "</a>";
								}

								ucTag++;
							}

							pResponse->strContent += "</td>\n";
						}

						// <td> info hash

						if( m_bShowInfoHash )
						{
							pResponse->strContent += "<td class=\"hash\">";

							if( m_bShowStats )
								pResponse->strContent += "<a class=\"stats\" title=\"" + gmapLANG_CFG["info_hash"] + ": " + UTIL_HashToString( pTorrents[ulKey].strInfoHash ) + "\" href=\"" + RESPONSE_STR_STATS_HTML + "?info_hash=" + UTIL_HashToString( pTorrents[ulKey].strInfoHash ) + "\">";

							pResponse->strContent += UTIL_HashToString( pTorrents[ulKey].strInfoHash );

							if( m_bShowStats )
								pResponse->strContent += "</a>";

							pResponse->strContent += "</td>\n";
						}
						
						// <td> seeders
						
					//	if( access( string( m_strReqDir + UTIL_HashToString( pTorrents[ulKey].strInfoHash ) ).c_str( ), 0 ) == 0 )
					//	{
					//		pResponse->strContent += "<td><a class=\"";

					//		if( pTorrents[ulKey].uiSeeders == 0 )
					//			pResponse->strContent += "red\" ";
					//		else if( pTorrents[ulKey].uiSeeders < 5 )
					//			pResponse->strContent += "yellow\" ";
					//		else
					//			pResponse->strContent += "green\" ";
					//		pResponse->strContent += "title=\"" + gmapLANG_CFG["req_seeders_cancel"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?noreq=" + UTIL_HashToString( pTorrents[ulKey].strInfoHash ) + "\">";
					//		pResponse->strContent += CAtomInt( pTorrents[ulKey].uiSeeders ).toString( ) + "</a></td>\n";
					//	}
					//	else
					//	{
							pResponse->strContent += "<td class=\"number_";

							if( pTorrents[ulKey].uiSeeders == 0 )
					//			pResponse->strContent += "red\"><a class=\"red\" title=\"" + gmapLANG_CFG["req_seeders"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?req=" + UTIL_HashToString( pTorrents[ulKey].strInfoHash ) + "\">";
								pResponse->strContent += "red\">";
							else if( pTorrents[ulKey].uiSeeders < 5 )
								pResponse->strContent += "yellow\">";
							else
								pResponse->strContent += "green\">";
							
					//		if( pTorrents[ulKey].uiSeeders == 0 )
					//			pResponse->strContent += CAtomInt( pTorrents[ulKey].uiSeeders ).toString( ) + "</a></td>\n";
					//		else
								pResponse->strContent += CAtomInt( pTorrents[ulKey].uiSeeders ).toString( ) + "</td>\n";
					//	}

						

						// <td> leechers

						pResponse->strContent += "<td class=\"number_";

						if( pTorrents[ulKey].uiLeechers == 0 )
							pResponse->strContent += "red\">";
						else if( pTorrents[ulKey].uiLeechers < 5 )
							pResponse->strContent += "yellow\">";
						else
							pResponse->strContent += "green\">";

						pResponse->strContent += CAtomInt( pTorrents[ulKey].uiLeechers ).toString( ) + "</td>\n";

						// <td> completed

						if( m_bShowCompleted )
							pResponse->strContent += "<td class=\"number\">" + CAtomInt( pTorrents[ulKey].ulCompleted ).toString( ) + "</td>\n";

						// <td> name

						if( m_pAllowed && m_bShowNames )
						{
							time_t now_t = time( NULL );
							struct tm *now_tm, time_tm;
							// int64 year, month, day, hour, minute, second, day_passed, hour_passed, day_limit;
							int64 year, month, day, hour, minute, second, day_left = -1, hour_left, day_limit;
							float passed;
							now_tm = localtime( &now_t );
							time_tm = *now_tm;
							sscanf( pTorrents[ulKey].strAdded.c_str( ), "%d-%d-%d %d:%d:%d",&year,&month,&day,&hour,&minute,&second );
							time_tm.tm_year = year-1900;
							time_tm.tm_mon = month-1;
							time_tm.tm_mday = day;
							time_tm.tm_hour = hour;
							time_tm.tm_min = minute;
							time_tm.tm_sec = second;
							passed = difftime(now_t,mktime(&time_tm));
							// day_passed = ( int ) passed / 86400;
							// hour_passed = ( int ) passed % 86400 / 3600;
							day_limit = CFG_GetInt( "bnbt_ratio_day_limit", 3 );
							if( day_limit * 86400 - ( int ) passed > 0 )
							{
								day_left = ( day_limit * 86400 - ( int ) passed + 3600 ) / 86400;
								hour_left = ( day_limit * 86400 - ( int ) passed + 3600 ) % 86400 / 3600;
							}
						//	else
						//		pTorrents[ulKey].iAllow = 1;
							
							pResponse->strContent += "<td class=\"name\">";

							if( m_bShowStats )
							{
								if( pTorrents[ulKey].iTop )
									pResponse->strContent += "<a class=\"stats_top\" title=\"";
								else
									if( pTorrents[ulKey].iHL )
										pResponse->strContent += "<a class=\"stats_hl\" title=\"";
									else
										pResponse->strContent += "<a class=\"stats\" title=\"";
								pResponse->strContent += gmapLANG_CFG["name"] + ": " + UTIL_RemoveHTML( pTorrents[ulKey].strName ) + "\" href=\"" + RESPONSE_STR_STATS_HTML + "?info_hash=" + UTIL_HashToString( pTorrents[ulKey].strInfoHash ) + "\">";
								
								if( pTorrents[ulKey].iTop )
									pResponse->strContent += gmapLANG_CFG["top"] + ": ";
							
								pResponse->strContent += UTIL_RemoveHTML( pTorrents[ulKey].strName ) + "</a>";
							}
							else
							{
								if( pTorrents[ulKey].iTop )
									pResponse->strContent += gmapLANG_CFG["top"] + ": ";
								pResponse->strContent += UTIL_RemoveHTML( pTorrents[ulKey].strName );
							}
								if( pTorrents[ulKey].uiLeechers + pTorrents[ulKey].ulCompleted >= CFG_GetInt( "bnbt_hot_count" ,20 ) )
									pResponse->strContent += "<span class=\"hot\"> " + gmapLANG_CFG["hot"] + "</span>";
							
							

							if( day_left >= 0 )
							{
								pResponse->strContent += "<br>" + gmapLANG_CFG["upload_ratio"] + ": " + UTIL_RemoveHTML( pTorrents[ulKey].strRatio );
								pResponse->strContent += " " + UTIL_Xsprintf( gmapLANG_CFG["ratio_time_left"].c_str( ), CAtomInt( day_left ).toString( ).c_str( ), CAtomInt( hour_left ).toString( ).c_str( ) );
								if( !pTorrents[ulKey].iAllow )
								{
									unsigned long irequire;
									for( int iratio = 0; iratio < 4; iratio++ )
										if( strRatio[iratio] == pTorrents[ulKey].strRatio )
											irequire = int( (iratio+1)*( pTorrents[ulKey].ulCompleted )/20.0 ) + 1 - pTorrents[ulKey].uiSeeders;
// 									pResponse->strContent += "<span class=\"warm_note\"> (" + gmapLANG_CFG["ratio_warning"] + ")";
									pResponse->strContent += "<span class=\"warm_note\"> (" + UTIL_Xsprintf( gmapLANG_CFG["ratio_warning"].c_str( ), CAtomInt( irequire ).toString( ).c_str( ) ) + ")</span>";
								}

								if( access( string( m_strReqDir + UTIL_HashToString( pTorrents[ulKey].strInfoHash ) ).c_str( ), 0 ) == 0 )
								{
									pResponse->strContent += "<span> (<a class=\"yellow\" title=\"" + gmapLANG_CFG["req_seeders_cancel"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?noreq=" + UTIL_HashToString( pTorrents[ulKey].strInfoHash ) + "\">" + gmapLANG_CFG["cancel"] + gmapLANG_CFG["subfilter_reqseeders"];
									pResponse->strContent += "</a>)</span>";
								}
								else
									if( pTorrents[ulKey].uiSeeders == 0 ) 
									{
										pResponse->strContent += "<span> (<a class=\"red\" title=\"" + gmapLANG_CFG["req_seeders"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?req=" + UTIL_HashToString( pTorrents[ulKey].strInfoHash ) + "\">" + gmapLANG_CFG["subfilter_reqseeders"];
										pResponse->strContent += "</a>)</span>";
									}
							}
							else
							{
								if( access( string( m_strReqDir + UTIL_HashToString( pTorrents[ulKey].strInfoHash ) ).c_str( ), 0 ) == 0 )
								{
									pResponse->strContent += "<br><span> (<a class=\"yellow\" title=\"" + gmapLANG_CFG["req_seeders_cancel"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?noreq=" + UTIL_HashToString( pTorrents[ulKey].strInfoHash ) + "\">" + gmapLANG_CFG["cancel"] + gmapLANG_CFG["subfilter_reqseeders"];
									pResponse->strContent += "</a>)</span>";
								}
								else
									if( pTorrents[ulKey].uiSeeders == 0 )
									{
										pResponse->strContent += "<br><span> (<a class=\"red\" title=\"" + gmapLANG_CFG["req_seeders"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?req=" + UTIL_HashToString( pTorrents[ulKey].strInfoHash ) + "\">" + gmapLANG_CFG["subfilter_reqseeders"];
										pResponse->strContent += "</a>)</span>";
									}
							}
							pResponse->strContent += "</td>\n";
						}

						// <td> info link

						if( m_bAllowInfoLink )
						{
							pResponse->strContent += "<td class=\"infolink\">";

							if( !pTorrents[ulKey].strInfoLink.empty( ) )
								pResponse->strContent += "<a rel=\"" + STR_TARGET_REL + "\" title=\"" + gmapLANG_CFG["link"] + ": " + UTIL_RemoveHTML( pTorrents[ulKey].strInfoLink ) + "\" href=\"" + UTIL_RemoveHTML( pTorrents[ulKey].strInfoLink ) + "\">" + gmapLANG_CFG["link"] + "</a>";

							pResponse->strContent += "</td>\n";
						}
						
						// <td> torrent


						if( m_pAllowed && m_bAllowTorrentDownloads && ( pRequest->user.ucAccess & ACCESS_DL ) )
						{
							if( pTorrents[ulKey].iAllow || ! pRequest->user.strLogin.empty( ) )
							{
								// The Trinity Edition - Modification Begins
								// The following adds "[" and "]" around the DL (download) link
								
								if( m_strExternalTorrentDir.empty( ) )
								{
									pResponse->strContent += "<td class=\"download\">[<a class=\"download\" title=\"" + gmapLANG_CFG["stats_download_torrent"] + ": " + UTIL_HashToString( pTorrents[ulKey].strInfoHash ) + ".torrent\" href=\"";
									pResponse->strContent += RESPONSE_STR_TORRENTS + UTIL_HashToString( pTorrents[ulKey].strInfoHash ) + ".torrent";
								}
								else
								{
									pResponse->strContent += "<td class=\"download\">[<a class=\"download\" title=\"" + gmapLANG_CFG["stats_download_torrent"] + ": " +  UTIL_HashToString( pTorrents[ulKey].strFileName ) + "\" href=\"";
									pResponse->strContent += m_strExternalTorrentDir + UTIL_StringToEscapedStrict( pTorrents[ulKey].strFileName );
								}

								pResponse->strContent += "\">" + gmapLANG_CFG["download"] + "</a>]</td>\n";
							}
							else
								pResponse->strContent += "<td class=\"download\">[" + gmapLANG_CFG["download"] + "]</td>\n";
						}


						// <td> comments

						if( m_pAllowed && m_bAllowComments )
						{
							if( m_pComments )
								pResponse->strContent += "<td class=\"number\"><a title=\"" + gmapLANG_CFG["comments"] + ": " + UTIL_RemoveHTML( pTorrents[ulKey].strName ) + "\" href=\"" + RESPONSE_STR_COMMENTS_HTML + "?info_hash=" + UTIL_HashToString( pTorrents[ulKey].strInfoHash ) + "\">" + CAtomInt( pTorrents[ulKey].uiComments ).toString( ) + "</a></td>\n";
						}

						// <td> added

						if( m_pAllowed && m_bShowAdded_Index )
						{
							pResponse->strContent += "<td class=\"date\">";

							if( !pTorrents[ulKey].strAdded.empty( ) )
							{
								// strip year and seconds from time

								pResponse->strContent += pTorrents[ulKey].strAdded.substr( 5, pTorrents[ulKey].strAdded.size( ) - 8 );
							}

							pResponse->strContent += "</td>\n";
						}

						// <td> size

						if( m_pAllowed && m_bShowSize )
							pResponse->strContent += "<td class=\"bytes\">" + UTIL_BytesToString( pTorrents[ulKey].iSize ) + "</td>\n";

						// <td> files

						if( m_pAllowed && m_bShowNumFiles )
							pResponse->strContent += "<td class=\"number\">" + CAtomInt( pTorrents[ulKey].uiFiles ).toString( ) + "</td>\n";

// 						// <td> seeders
// 
// 						pResponse->strContent += "<td class=\"number_";
// 
// 						if( pTorrents[ulKey].uiSeeders == 0 )
// 							pResponse->strContent += "red\">";
// 						else if( pTorrents[ulKey].uiSeeders < 5 )
// 							pResponse->strContent += "yellow\">";
// 						else
// 							pResponse->strContent += "green\">";
// 
// 						pResponse->strContent += CAtomInt( pTorrents[ulKey].uiSeeders ).toString( ) + "</td>\n";
// 
// 						// <td> leechers
// 
// 						pResponse->strContent += "<td class=\"number_";
// 
// 						if( pTorrents[ulKey].uiLeechers == 0 )
// 							pResponse->strContent += "red\">";
// 						else if( pTorrents[ulKey].uiLeechers < 5 )
// 							pResponse->strContent += "yellow\">";
// 						else
// 							pResponse->strContent += "green\">";
// 
// 						pResponse->strContent += CAtomInt( pTorrents[ulKey].uiLeechers ).toString( ) + "</td>\n";
// 
// 						// <td> completed
// 
// 						if( m_bShowCompleted )
// 							pResponse->strContent += "<td class=\"number\">" + CAtomInt( pTorrents[ulKey].ulCompleted ).toString( ) + "</td>\n";

						// <td> transferred

						if( m_pAllowed && m_bShowTransferred )
							pResponse->strContent += "<td class=\"bytes\">" + UTIL_BytesToString( pTorrents[ulKey].iTransferred ) + "</td>\n";

						// <td> min left

						if( m_bShowMinLeft )
						{
							pResponse->strContent += "<td class=\"percent\">";

							if( pTorrents[ulKey].uiLeechers == 0 )
								pResponse->strContent += gmapLANG_CFG["na"];
							else
							{
								if( m_pAllowed )
								{
									ucPercent = 0;

									if( pTorrents[ulKey].iSize > 0 )
									{
										if( m_bShowLeftAsProgress )
											ucPercent = (unsigned char)( 100 - (unsigned char)( ( (float)pTorrents[ulKey].iMaxiLeft / pTorrents[ulKey].iSize ) * 100 ) );
										else
											ucPercent = (unsigned char)( ( (float)pTorrents[ulKey].iMinLeft / pTorrents[ulKey].iSize ) * 100 );
									}

									pResponse->strContent += CAtomInt( ucPercent ).toString( ) + "%";
								}
								else
									pResponse->strContent += UTIL_BytesToString( pTorrents[ulKey].iMinLeft );
							}

							pResponse->strContent += "</td>\n";
						}

						// <td> average left

						if( m_bShowAverageLeft )
						{
							pResponse->strContent += "<td class=\"percent\">";

							if( pTorrents[ulKey].uiLeechers == 0 )
								pResponse->strContent += gmapLANG_CFG["na"];
							else
							{
								if( m_pAllowed )
								{
									if( m_bShowLeftAsProgress )
										ucPercent = (unsigned char)( 100 - pTorrents[ulKey].ucAverageLeftPercent );
									else
										ucPercent = pTorrents[ulKey].ucAverageLeftPercent;

									pResponse->strContent += CAtomInt( ucPercent ).toString( ) + "%";

									if( !imagefill.strFile.empty( ) && !imagetrans.strFile.empty( ) )
									{
										pResponse->strContent += "<br>";

										string strFillFileName = imagefill.strName;

										if( strFillFileName.empty( ) )
											strFillFileName = "imagebarfill.png";

										if( ucPercent > 0 )
										{
											if( !imagefill.strURL.empty( ) )
												pResponse->strContent += "<img class=\"percent\" src=\"" + imagefill.strURL + strFillFileName + "\" width=" + CAtomInt( ucPercent ).toString( ) + " height=8 alt=\"[" + gmapLANG_CFG["completed"] + "]\" name=\"Completed\" title=\"" + gmapLANG_CFG["completed"] + "\">";
											else if( m_bServeLocal )
												pResponse->strContent += "<img class=\"percent\" src=\"" + strFillFileName + "\" width=" + CAtomInt( ucPercent ).toString( ) + " height=8 alt=\"[" + gmapLANG_CFG["completed"] + "]\" name=\"Completed\" title=\"" + gmapLANG_CFG["completed"] + "\">";
										}

										string strTransFileName = imagetrans.strName;

										if( strTransFileName.empty( ) )
											strTransFileName = "imagebartrans.png";

										if( ucPercent < 100 )
										{
											if( !imagetrans.strURL.empty( ) )
												pResponse->strContent += "<img class=\"percent\" src=\"" + imagetrans.strURL + strTransFileName + "\" width=" + CAtomInt( 100 - ucPercent ).toString( ) + " height=8 alt=\"[" + gmapLANG_CFG["remaining"] + "]\" name=\"Remaining\" title=\"" + gmapLANG_CFG["remaining"] + "\">";
											else if( m_bServeLocal )
												pResponse->strContent += "<img class=\"percent\" src=\"" + strTransFileName + "\" width=" + CAtomInt( 100 - ucPercent ).toString( ) + " height=8 alt=\"[" + gmapLANG_CFG["remaining"] + "]\" name=\"Remaining\" title=\"" + gmapLANG_CFG["remaining"] + "\">";
										}
									}
								}
								else
									pResponse->strContent += UTIL_BytesToString( pTorrents[ulKey].iAverageLeft ) + "%";

							}
							pResponse->strContent += "</td>\n";
						}

						// <td> maxi left

						if( m_bShowMaxiLeft )
						{
							pResponse->strContent += "<td class=\"percent\">";

							if( pTorrents[ulKey].uiLeechers == 0 )
								pResponse->strContent += gmapLANG_CFG["na"];
							else
							{
								if( m_pAllowed )
								{
									ucPercent = 0;

									if( pTorrents[ulKey].iSize > 0 )
									{
										if( m_bShowLeftAsProgress )
											ucPercent = (unsigned char)( 100 - (int)( ( (float)pTorrents[ulKey].iMinLeft / pTorrents[ulKey].iSize ) * 100 ) );
										else
											ucPercent = (unsigned char)( ( (float)pTorrents[ulKey].iMaxiLeft / pTorrents[ulKey].iSize ) * 100 );
									}

									pResponse->strContent += CAtomInt( ucPercent ).toString( ) + "%";
								}
								else
									pResponse->strContent += UTIL_BytesToString( pTorrents[ulKey].iMaxiLeft );
							}

							pResponse->strContent += "</td>\n";
						}

						// <td> uploader

						if( m_bShowUploader )
							pResponse->strContent += "<td class=\"uploader\"><a class=\"uploader\" href=\"" + RESPONSE_STR_INDEX_HTML + "?uploader=" + UTIL_RemoveHTML( pTorrents[ulKey].strUploader ) + "\">" + UTIL_RemoveHTML( pTorrents[ulKey].strUploader ) + "</a></td>\n";

						// <td> ip

						if( m_bShowIP && ( pRequest->user.ucAccess & ACCESS_ADMIN ) )
							pResponse->strContent += "<td class=\"ip\">" + UTIL_RemoveHTML( pTorrents[ulKey].strIP ) + "</td>\n";

						// <td> info link

						// if( m_bAllowInfoLink )
						// {
							// pResponse->strContent += "<td class=\"infolink\">";

							// if( !pTorrents[ulKey].strInfoLink.empty( ) )
								// pResponse->strContent += "<a rel=\"" + STR_TARGET_REL + "\" title=\"" + gmapLANG_CFG["link"] + ": " + UTIL_RemoveHTML( pTorrents[ulKey].strInfoLink ) + "\" href=\"" + UTIL_RemoveHTML( pTorrents[ulKey].strInfoLink ) + "\">" + gmapLANG_CFG["link"] + "</a>";

							// pResponse->strContent += "</td>\n";
						// }

						// <td> admin

// 						if( pRequest->user.ucAccess & ACCESS_EDIT )
// 						{
// 							if( m_pAllowed )
// 							{
// 								if( pTorrents[ulKey].iTop )
// 									pResponse->strContent += "<td class=\"admin\">[<a title=\"" + gmapLANG_CFG["cancel"] + ": " + UTIL_HashToString( pTorrents[ulKey].strInfoHash ) + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?normal=" + UTIL_HashToString( pTorrents[ulKey].strInfoHash ) + "\">" + gmapLANG_CFG["cancel"] + "</a>]</td>\n";
// 								else
// 									pResponse->strContent += "<td class=\"admin\">[<a title=\"" + gmapLANG_CFG["top"] + ": " + UTIL_HashToString( pTorrents[ulKey].strInfoHash ) + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?top=" + UTIL_HashToString( pTorrents[ulKey].strInfoHash ) + "\">" + gmapLANG_CFG["top"] + "</a>]</td>\n";
// 							}
// 						}
						
						if( pRequest->user.ucAccess & ACCESS_EDIT )
						{
							if( m_pAllowed )
							{
								if( pTorrents[ulKey].iTop )
									pResponse->strContent += "<td class=\"admin\">[<a title=\"" + gmapLANG_CFG["normal"] + ": " + UTIL_HashToString( pTorrents[ulKey].strInfoHash ) + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?normal=" + UTIL_HashToString( pTorrents[ulKey].strInfoHash ) + "\">" + gmapLANG_CFG["normal"] + "</a>]";
								else
									pResponse->strContent += "<td class=\"admin\">[<a title=\"" + gmapLANG_CFG["top"] + ": " + UTIL_HashToString( pTorrents[ulKey].strInfoHash ) + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?top=" + UTIL_HashToString( pTorrents[ulKey].strInfoHash ) + "\">" + gmapLANG_CFG["top"] + "</a>]";
								
								if( pTorrents[ulKey].iHL )
									pResponse->strContent += "[<a title=\"" + gmapLANG_CFG["black"] + ": " + UTIL_HashToString( pTorrents[ulKey].strInfoHash ) + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?nohl=" + UTIL_HashToString( pTorrents[ulKey].strInfoHash ) + "\">" + gmapLANG_CFG["black"] + "</a>]";
								else
									pResponse->strContent += "[<a title=\"" + gmapLANG_CFG["hl"] + ": " + UTIL_HashToString( pTorrents[ulKey].strInfoHash ) + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?hl=" + UTIL_HashToString( pTorrents[ulKey].strInfoHash ) + "\">" + gmapLANG_CFG["hl"] + "</a>]";
								
								pResponse->strContent += "[<a title=\"" + gmapLANG_CFG["edit"] + ": " + UTIL_HashToString( pTorrents[ulKey].strInfoHash ) + "\" href=\"" + RESPONSE_STR_STATS_HTML + "?info_hash=" + UTIL_HashToString( pTorrents[ulKey].strInfoHash ) + "&amp;edit=1\">" + gmapLANG_CFG["edit"] + "</a>]";
							
								pResponse->strContent += "[<a title=\"" + gmapLANG_CFG["delete"] + ": " + UTIL_HashToString( pTorrents[ulKey].strInfoHash ) + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?del=" + UTIL_HashToString( pTorrents[ulKey].strInfoHash ) + "\">" + gmapLANG_CFG["delete"] + "</a>]</td>\n";        
							}
						}

						pResponse->strContent += "</tr>\n";

						// increment row counter for row colour
						ulAdded++;
					}
					else
						ulSkipped++;
				}
			}

			// free the memory
			delete [] pTorrents;

			// some finishing touches

			if( bFound )
				pResponse->strContent += "</table>\n\n";

			// Bottom search
			if( ulFound && m_pAllowed && m_bShowNames && m_bSearch )
			{
				pResponse->strContent += "<form class=\"search_index\" name=\"bottomsearch\" method=\"get\" action=\"" + RESPONSE_STR_INDEX_HTML + "\">\n";

				if( !cstrPerPage.empty( ) )
					pResponse->strContent += "<p><input name=\"per_page\" type=hidden value=\"" + cstrPerPage + "\"></p>\n";

				if( !strSort.empty( ) )
					pResponse->strContent += "<p><input name=\"sort\" type=hidden value=\"" + strSort + "\"></p>\n";

				if( !strFilter.empty( ) )
					pResponse->strContent += "<p><input name=\"filter\"type=hidden value=\"" + UTIL_RemoveHTML( strFilter ) + "\"></p>\n";

				if( !cstrMode.empty( ) )
					pResponse->strContent += "<p><input name=\"mode\" type=hidden value=\"" + cstrMode + "\"></p>\n";

				if( m_bUseButtons )
				{
					pResponse->strContent += "<p><label for=\"bottomtorrentsearch\">" + gmapLANG_CFG["torrent_search"] + "</label> <input name=\"search\" id=\"bottomtorrentsearch\" alt=\"[" + gmapLANG_CFG["torrent_search"] + "]\" type=text size=40>\n";

					pResponse->strContent += Button_Submit( "bottom_submit_search", gmapLANG_CFG["search"] );
					pResponse->strContent += Button_JS_Link( "bottom_clear_filter_and_search", gmapLANG_CFG["clear_filter_search"], "clear_search_and_filters( )" );

					pResponse->strContent += "</p>\n";
				}
				else
					pResponse->strContent += "<p><label for=\"bottomtorrentsearch\">" + gmapLANG_CFG["torrent_search"] + "</label> <input name=\"search\" id=\"bottomtorrentsearch\" alt=\"[" + gmapLANG_CFG["torrent_search"] + "]\" type=text size=40> <a title=\"" + gmapLANG_CFG["clear_filter_search"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "\">" + gmapLANG_CFG["clear_filter_search"] + "</a></p>\n";

				pResponse->strContent += "</form>\n\n";
			}

			// page numbers

			if( ulFound && uiOverridePerPage > 0 )
			{
				pResponse->strContent += "<p class=\"pagenum_bottom\">" + gmapLANG_CFG["jump_to_page"] + ": \n";

				for( unsigned long ulKey = 0; ulKey < ulFound; ulKey += uiOverridePerPage )
				{
					pResponse->strContent += " ";

					// don't link to current page

					if( ulKey != ulStart )
					{
						pResponse->strContent += "<a title=\"" + gmapLANG_CFG["jump_to_page"] + ": " + CAtomInt( ( ulKey / uiOverridePerPage ) + 1 ).toString( ) + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?page=" + CAtomInt( ulKey / uiOverridePerPage ).toString( );

						if( !cstrPerPage.empty( ) )
							pResponse->strContent += "&amp;per_page=" + cstrPerPage;

						if( !strSort.empty( ) )
							pResponse->strContent += "&amp;sort=" + strSort;

						if( !strSearch.empty( ) )
							pResponse->strContent += "&amp;search=" + strSearchResp;

						if( !strFilter.empty( ) )
							pResponse->strContent += "&amp;filter=" + UTIL_StringToEscaped( strFilter );
						if( !cstrUploader.empty( ) )
							pResponse->strContent += "&amp;uploader=" + cstrUploader;

						if( !cstrMode.empty( ) )
							pResponse->strContent += "&amp;mode=" + cstrMode;

						pResponse->strContent += "\">";
					}

					pResponse->strContent += CAtomInt( ( ulKey / uiOverridePerPage ) + 1 ).toString( );

					if( ulKey != ulStart )
						pResponse->strContent += "</a>\n";

					// don't display a bar after the last page

					if( ulKey + uiOverridePerPage < ulFound )
						pResponse->strContent += "\n<span class=\"pipe\">|</span>";
				}

// 				pResponse->strContent += "</p>\n\n";
		//	}
			
				pResponse->strContent += "<br>";
			
// 			pResponse->strContent += "<p class=\"pagenum_bottom\">";
				
				if( ulStart > 0 )
				{
					pResponse->strContent += "<a title=\"" + gmapLANG_CFG["jump_to_page"] + ": " + CAtomInt( ( ulStart / uiOverridePerPage ) ).toString( ) + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?page=" + CAtomInt( ( ulStart / uiOverridePerPage ) - 1 ).toString( );
					
					if( !cstrPerPage.empty( ) )
						pResponse->strContent += "&amp;per_page=" + cstrPerPage;

					if( !strSort.empty( ) )
						pResponse->strContent += "&amp;sort=" + strSort;

					if( !strSearch.empty( ) )
						pResponse->strContent += "&amp;search=" + strSearchResp;

					if( !strFilter.empty( ) )
						pResponse->strContent += "&amp;filter=" + UTIL_StringToEscaped( strFilter );
					if( !cstrUploader.empty( ) )
						pResponse->strContent += "&amp;uploader=" + cstrUploader;

					if( !cstrMode.empty( ) )
						pResponse->strContent += "&amp;mode=" + cstrMode;

					pResponse->strContent += "\">";
				}
				pResponse->strContent += gmapLANG_CFG["last_page"];
				
				if( ulStart > 0 )
						pResponse->strContent += "</a>";
				
				pResponse->strContent += "<span class=\"pipe\"> | </span>";
				
				if( ulStart + uiOverridePerPage < ulFound )
				{
					pResponse->strContent += "<a title=\"" + gmapLANG_CFG["jump_to_page"] + ": " + CAtomInt( ( ulStart / uiOverridePerPage ) + 2 ).toString( ) + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?page=" + CAtomInt( ( ulStart / uiOverridePerPage ) + 1 ).toString( );
					
					if( !cstrPerPage.empty( ) )
						pResponse->strContent += "&amp;per_page=" + cstrPerPage;

					if( !strSort.empty( ) )
						pResponse->strContent += "&amp;sort=" + strSort;

					if( !strSearch.empty( ) )
						pResponse->strContent += "&amp;search=" + strSearchResp;

					if( !strFilter.empty( ) )
						pResponse->strContent += "&amp;filter=" + UTIL_StringToEscaped( strFilter );
					if( !cstrUploader.empty( ) )
						pResponse->strContent += "&amp;uploader=" + cstrUploader;

					if( !cstrMode.empty( ) )
						pResponse->strContent += "&amp;mode=" + cstrMode;

					pResponse->strContent += "\">";
				}
				pResponse->strContent += gmapLANG_CFG["next_page"];
				
				if( ulStart + uiOverridePerPage < ulFound )
						pResponse->strContent += "</a>";
				
				pResponse->strContent += "</p>\n\n";
			}
		}        

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, IS_INDEX, string( CSS_INDEX ) );
	}
	else
	{
		// Not authorised

		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["index_page"], string( CSS_INDEX ), string( ), NOT_INDEX, CODE_401 );

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INDEX ) );
	}
}
