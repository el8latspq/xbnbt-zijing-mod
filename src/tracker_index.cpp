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

// #include <fcntl.h>
#include <sys/stat.h>

#include "bnbt.h"
#include "bnbt_mysql.h"
#include "atom.h"
#include "bencode.h"
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
	if( !pRequest->user.strUID.empty( ) && ( pRequest->user.ucAccess & m_ucAccessViewTorrents ) )
	{
		// Was a search submited?
		if( pRequest->mapParams["top_submit_search_button"] == gmapLANG_CFG["search"] )
		{
			string cstrSearch( pRequest->mapParams["search"] );
			string cstrUploader = string( );
			string cstrFilter( pRequest->mapParams["tag"] );
			string cstrMedium( pRequest->mapParams["medium"] );
			string cstrQuality( pRequest->mapParams["quality"] );
			string cstrEncode( pRequest->mapParams["encode"] );
			const string cstrPerPage( pRequest->mapParams["per_page"] );
			const string cstrNoTag( pRequest->mapParams["notag"] );
			const string cstrSearchMode( pRequest->mapParams["smode"] );
			const string cstrMatch( pRequest->mapParams["match"] );
			
			if( cstrSearchMode == "uploader" )
			{
				cstrUploader = cstrSearch;
				cstrSearch.erase( );
			}
			
			for( map<string, string> :: iterator it = pRequest->mapParams.begin( ); it != pRequest->mapParams.end( ); it++ )
			{
				if( it->first.substr( 0, 3 ) == "tag" && it->second == "on" )
				{
					if( !cstrFilter.empty( ) )
						cstrFilter += " ";
					cstrFilter += it->first.substr( 3 );
				}
				
				if( it->first.substr( 0, 7 ) == "medium_" && it->second == "on" )
				{
					if( !cstrMedium.empty( ) )
						cstrMedium += " ";
					cstrMedium += it->first.substr( 7 );
				}
				
				if( it->first.substr( 0, 8 ) == "quality_" && it->second == "on" )
				{
					if( !cstrQuality.empty( ) )
						cstrQuality += " ";
					cstrQuality += it->first.substr( 8 );
				}
				
				if( it->first.substr( 0, 7 ) == "encode_" && it->second == "on" )
				{
					if( !cstrEncode.empty( ) )
						cstrEncode += " ";
					cstrEncode += it->first.substr( 7 );
				}
			}

			string strPageParameters = INDEX_HTML;
			
			vector< pair< string, string > > vecParams;
			vecParams.reserve(64);
			
			vecParams.push_back( pair<string, string>( string( "search" ), cstrSearch ) );
			vecParams.push_back( pair<string, string>( string( "match" ), cstrMatch ) );
			vecParams.push_back( pair<string, string>( string( "tag" ), cstrFilter ) );
			vecParams.push_back( pair<string, string>( string( "medium" ), cstrMedium ) );
			vecParams.push_back( pair<string, string>( string( "quality" ), cstrQuality ) );
			vecParams.push_back( pair<string, string>( string( "encode" ), cstrEncode ) );
			vecParams.push_back( pair<string, string>( string( "uploader" ), cstrUploader ) );
			vecParams.push_back( pair<string, string>( string( "per_page" ), cstrPerPage ) );
			vecParams.push_back( pair<string, string>( string( "notag" ), cstrNoTag ) );
			
			strPageParameters += UTIL_HTMLJoin( vecParams, string( "?" ), string( "&" ), string( "=" ) );

			return JS_ReturnToPage( pRequest, pResponse, strPageParameters );
		}

		if( pRequest->mapParams["top_clear_filter_and_search_button"] == gmapLANG_CFG["clear_filter_search"] )
		{
			const string cstrSearch( pRequest->mapParams["search"] );
			const string cstrSearchResp( UTIL_StringToEscaped( cstrSearch ) );
			const string cstrSearchMode( pRequest->mapParams["smode"] );
			const string cstrMatch( pRequest->mapParams["match"] );
			
			string strPageParameters = INDEX_HTML;
	
			if( !cstrSearch.empty( ) )
			{
				strPageParameters += "?";
				if( cstrSearchMode == "name" )
					strPageParameters += "search=" + cstrSearchResp;
				else
					strPageParameters += "uploader=" + cstrSearchResp;
				strPageParameters += "&match=" + cstrMatch;
			}
			
			return JS_ReturnToPage( pRequest, pResponse, strPageParameters );
		}
		
		string strReturnPage( pRequest->mapParams["return"] );
		
		if( strReturnPage.empty( ) )
			strReturnPage = RESPONSE_STR_INDEX_HTML;

		//
		// delete torrent
		//

		// Check that user has edit authority
		if( pRequest->user.ucAccess & m_ucAccessDelTorrents )
		{
			if( pRequest->mapParams.find( "del" ) != pRequest->mapParams.end( ) )
			{
				string strDelID( pRequest->mapParams["del"] );
				const string strDelReason( UTIL_RemoveHTML( pRequest->mapParams["reason"] ) );
				const string strOK( pRequest->mapParams["ok"] );
				const string strReturnPageResp( UTIL_StringToEscaped( strReturnPage ) );
				
				if( strDelID.find( " " ) != string :: npos )
					strDelID.erase( );
				
				string strPageParameters = INDEX_HTML;
				if( pRequest->mapParams["submit_delete_button"] == gmapLANG_CFG["yes"] )
				{
					strPageParameters += "?del=" + strDelID + "&reason=" + strDelReason + "&ok=1";
					strPageParameters += "&return=" + strReturnPageResp;
					return JS_ReturnToPage( pRequest, pResponse, strPageParameters );
				}
				
				CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bid,bfilename,buploaderid FROM allowed WHERE bid=" + strDelID );
			
				vector<string> vecQuery;
			
				vecQuery.reserve(3);

				vecQuery = pQuery->nextRow( );
				
				delete pQuery;
				
				if( vecQuery.size( ) == 3 && !vecQuery[0].empty( ) )
				{
					if( strOK == "1" )
					{
						// delete from disk

						string strFileName = vecQuery[1];

						if( !strFileName.empty( ) )
						{
							if( m_strArchiveDir.empty( ) )
								UTIL_DeleteFile( string( m_strAllowedDir + strFileName ).c_str( ) );
							else
								UTIL_MoveFile( string( m_strAllowedDir + strFileName ).c_str( ), string( m_strArchiveDir + strFileName ).c_str( ) );
						}

						CMySQLQuery *pQueryUsers = new CMySQLQuery( "SELECT buid FROM users WHERE buid=" + vecQuery[2] + " UNION SELECT buid FROM bookmarks WHERE bid=" + strDelID + " GROUP BY buid" );
	
						vector<string> vecQueryUsers;
					
						vecQueryUsers.reserve(1);

						vecQueryUsers = pQueryUsers->nextRow( );
						
						while( vecQueryUsers.size( ) == 1 && !vecQueryUsers[0].empty( ) )
						{
							if( !pRequest->user.strUID.empty( ) )
							{
								string strTitle = gmapLANG_CFG["admin_delete_torrent_title"];
								string strMessage = string( );
								if( vecQuery[2] == vecQueryUsers[0] )
								{
									strMessage = UTIL_Xsprintf( gmapLANG_CFG["admin_delete_torrent"].c_str( ), UTIL_AccessToString( pRequest->user.ucAccess ).c_str( ), pRequest->user.strLogin.c_str( ), strFileName.c_str( ), strDelReason.c_str( ) );
									sendMessage( pRequest->user.strLogin, pRequest->user.strUID, vecQueryUsers[0], pRequest->strIP, strTitle, strMessage );
								}
								else
								{
									strMessage = UTIL_Xsprintf( gmapLANG_CFG["admin_delete_torrent_bookmarked"].c_str( ), UTIL_AccessToString( pRequest->user.ucAccess ).c_str( ), pRequest->user.strLogin.c_str( ), strFileName.c_str( ), strDelReason.c_str( ) );
									sendMessage( "", "0", vecQueryUsers[0], "127.0.0.1", strTitle, strMessage );
								}
							}

							vecQueryUsers = pQueryUsers->nextRow( );
						}

						delete pQueryUsers;
							
						UTIL_LogFilePrint( "deleteTorrent: %s deleted torrent %s\n", pRequest->user.strLogin.c_str( ), strFileName.c_str( ) );
						UTIL_LogFilePrint( "deleteTorrent: delete reason %s\n", strDelReason.c_str( ) );


						deleteTag( strDelID, false );

						// Output common HTML head

						HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["index_page"], string( CSS_INDEX ), string( ), NOT_INDEX, CODE_200 );

						// Deleted the torrent
						pResponse->strContent += "<p class=\"deleted\">" + UTIL_Xsprintf( gmapLANG_CFG["index_deleted_torrent"].c_str( ), strDelID.c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_index"] + "\" href=\"" + strReturnPage + "\">" ).c_str( ), "</a>" ) + "</p>\n";

						// Output common HTML tail
						HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INDEX ) );

						return;
					}
					else
					{
						// Added and modified by =Xotic=
						// The Trinity Edition - Modification Begins
						// The following replaces the OK response with a YES | NO option
						// when DELETING A TORRENT

						// Output common HTML head
						HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["index_page"], string( CSS_INDEX ), string( ), NOT_INDEX, CODE_200 );
						
						pResponse->strContent += "<script type=\"text/javascript\">\n";
						pResponse->strContent += "<!--\n";

						pResponse->strContent += "function validate( theform ) {\n";
						pResponse->strContent += "if( theform.reason.value == \"\" ) {\n";
						pResponse->strContent += "  alert( \"" + gmapLANG_CFG["js_fill_delete_reason"] + "\" );\n";
						pResponse->strContent += "  return false; }\n";
						pResponse->strContent += "else { return true; }\n";
						pResponse->strContent += "}\n\n";

						pResponse->strContent += "//-->\n";
						pResponse->strContent += "</script>\n\n";

						pResponse->strContent += "<div class=\"torrent_delete\">\n";
						pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["delete_torrent_q"].c_str( ), strDelID.c_str( ) ) + "</p>\n";
						pResponse->strContent += "<form name=\"deletetorrent\" method=\"get\" action=\"" + string( RESPONSE_STR_INDEX_HTML ) + "\" onSubmit=\"return validate( this )\">";
						pResponse->strContent += "<p class=\"delete\"><input name=\"del\" type=hidden value=\"" + strDelID + "\"></p>\n";
						pResponse->strContent += "<p class=\"delete\"><input name=\"ok\" type=hidden value=\"1\"></p>\n";
						pResponse->strContent += "<p class=\"delete\"><input name=\"return\" type=hidden value=\"" + strReturnPage + "\"></p>\n";
						pResponse->strContent += "<p class=\"delete\"><label for=\"delete\">" + gmapLANG_CFG["delete_torrent_reason"] + "</label>\n";
						pResponse->strContent += "<input name=\"reason\" id=\"reason\" alt=\"[" + gmapLANG_CFG["delete_torrent_reason"] + "]\" type=text size=40 maxlength=" + CAtomInt( MAX_FILENAME_LEN ).toString( ) + " value=\"\"></p>";
						pResponse->strContent += "<div>\n";
						pResponse->strContent += Button_Submit( "submit_delete", string( gmapLANG_CFG["yes"] ) );
						pResponse->strContent += Button_Back( "cancel_delete", string( gmapLANG_CFG["no"] ) );
						pResponse->strContent += "\n</div>\n</div></form>\n";

						// Output common HTML tail
						HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INDEX ) );

						return;
					}
				}
				else
				{
					// Output common HTML head
					HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["index_page"], string( CSS_INDEX ), string( ), NOT_INDEX, CODE_200 );

					pResponse->strContent += "<p class=\"not_exist\">" + UTIL_Xsprintf( gmapLANG_CFG["torrent_not_exist"].c_str( ), strDelID.c_str( ) );

					// Output common HTML tail
					HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INDEX ) );

					return;
				}

			}
		}

		// Compose the page

		unsigned long ulKeySize = 0;
		
		struct torrent_t *pTorrents = 0;
		
		if( m_pCache )
		{
			pTorrents = m_pCache->getCache( );
			ulKeySize = m_pCache->getSize( );
		}
		
		// Are we tracking any files?	
		if( ulKeySize == 0 )
		{
			// Output common HTML head
			HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["index_page"], string( CSS_INDEX ), string( ), IS_INDEX, CODE_200 );

			// No files are being tracked!
			pResponse->strContent += "<p class=\"no_files\">" + gmapLANG_CFG["index_no_files"] + "</p>\n";

			// Output common HTML tail
			HTML_Common_End( pRequest, pResponse, btv, IS_INDEX, string( CSS_INDEX ) );

			return;
		}
		
		m_pCache->setFree( );
		
		time_t now_t = time( 0 );

		bool bFreeGlobal = CFG_GetInt( "bnbt_free_global", 0 ) == 0 ? false : true;
	
		int64 iFreeDownGlobal = CFG_GetInt( "bnbt_free_down_global", 100 );
		int64 iFreeUpGlobal = CFG_GetInt( "bnbt_free_up_global", 100 );

//		if( m_bCountUniquePeers )
//		{
//			CMySQLQuery *pQueryIP = new CMySQLQuery( "SELECT bip from ips" );
//			
//			if( pQueryIP->numRows( ) > gtXStats.peer.iGreatestUnique )
//				gtXStats.peer.iGreatestUnique = pQueryIP->numRows( );
//			
//			delete pQueryIP;
//		}

		// Sort
		const string strSort( pRequest->mapParams["sort"] );
		const string cstrNoTop( pRequest->mapParams["notop"] );
		
		bool bNoTop = false;
		
		if( !cstrNoTop.empty( ) && cstrNoTop == "1" )
			bNoTop = true;
		
		if( m_bSort )
		{
			const unsigned char cucSort( (unsigned char)atoi( strSort.c_str( ) ) );
			if( !strSort.empty( ) )
				m_pCache->sort( cucSort, bNoTop );
			else
				if( m_bShowAdded )
					m_pCache->sort( SORT_DADDED, bNoTop );
		}
		else
			if( m_bShowAdded )
				m_pCache->sort( SORT_DADDED, bNoTop );
		
//		if( m_bSort )
//		{
//			const unsigned char cucSort( (unsigned char)atoi( strSort.c_str( ) ) );

//			if( !strSort.empty( ) )
//			{
//				if( !bNoTop )
//				{
//					switch( cucSort )
//					{
//					case SORT_ANAME:
//						qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), asortByName );
//						break;
//					case SORT_ACOMPLETE:
//						qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), asortByComplete );
//						break;
//					case SORT_AINCOMPLETE:
//						qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), asortByDL );
//						break;
//					case SORT_AADDED:
//						if( m_bShowAdded )
//							qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), asortByAdded );
//						break;
//					case SORT_ASIZE:
//						if( m_bShowSize )
//							qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), asortBySize );
//						break;
//					case SORT_AFILES:
//						if( m_bShowNumFiles )
//							qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), asortByFiles );
//						break;
//					case SORT_ACOMMENTS:
//						if( m_bAllowComments )
//							qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), asortByComments );
//						break;
//	// 				case SORT_AAVGLEFT:
//	// 					if( m_bShowAverageLeft )
//	// 					{
//	// 						if( m_bShowLeftAsProgress )
//	// 							qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByAvgLeftPercent );
//	// 						else
//	// 							qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), asortByAvgLeftPercent );
//	// 					}
//	// 					break;
//					case SORT_ACOMPLETED:
//						if( m_bShowCompleted )
//							qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), asortByCompleted );
//						break;
//	//				case SORT_ATRANSFERRED:
//	//					if( m_bShowTransferred )
//	//						qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), asortByTransferred );
//	//					break;
//					case SORT_ATAG:
//						qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), asortByTag );
//						break;
//					case SORT_AUPLOADER:
//						if( m_bShowUploader )
//							qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), asortByUploader );
//						break;
//	//				case SORT_AIP:
//	//					if( pRequest->user.ucAccess & m_ucAccessSortIP )
//	//						qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), asortByIP );
//	//					break;
//					case SORT_DNAME:
//						qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByName );
//						break;
//					case SORT_DCOMPLETE:
//						qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByComplete );
//						break;
//					case SORT_DINCOMPLETE:
//						qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByDL );
//						break;
//					case SORT_DADDED:
//						if( m_bShowAdded )
//							qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByAdded );
//						break;
//					case SORT_DSIZE:
//						if( m_bShowSize )
//							qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortBySize );
//						break;
//					case SORT_DFILES:
//						if( m_bShowNumFiles )
//							qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByFiles );
//						break;
//					case SORT_DCOMMENTS:
//						if( m_bAllowComments )
//							qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByComments );
//						break;
//	// 				case SORT_DAVGLEFT:
//	// 					if( m_bShowAverageLeft )
//	// 					{
//	// 						if( m_bShowLeftAsProgress )
//	// 							qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), asortByAvgLeftPercent );
//	// 						else
//	// 							qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByAvgLeftPercent );
//	// 					}
//	// 					break;
//					case SORT_DCOMPLETED:
//						if( m_bShowCompleted )
//							qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByCompleted );
//						break;
//	//				case SORT_DTRANSFERRED:
//	//					if( m_bShowTransferred )
//	//						qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByTransferred );
//	//					break;
//					case SORT_DTAG:
//						qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByTag );
//						break;
//					case SORT_DUPLOADER:
//						if( m_bShowUploader )
//							qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByUploader );
//						break;
//	//				case SORT_DIP:
//	//					if( pRequest->user.ucAccess & m_ucAccessSortIP )
//	//						qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByIP );
//	//					break;
//					default:
//						// default action is to sort by added if we can
//						if( m_bShowAdded )
//							qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByAdded );
//					}
//				}
//				else
//				{
//					switch( cucSort )
//					{
//					case SORT_ANAME:
//						qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), asortByNameNoTop );
//						break;
//					case SORT_ACOMPLETE:
//						qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), asortByCompleteNoTop );
//						break;
//					case SORT_AINCOMPLETE:
//						qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), asortByDLNoTop );
//						break;
//					case SORT_AADDED:
//						if( m_bShowAdded )
//							qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), asortByAddedNoTop );
//						break;
//					case SORT_ASIZE:
//						if( m_bShowSize )
//							qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), asortBySizeNoTop );
//						break;
//					case SORT_AFILES:
//						if( m_bShowNumFiles )
//							qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), asortByFilesNoTop );
//						break;
//					case SORT_ACOMMENTS:
//						if( m_bAllowComments )
//							qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), asortByCommentsNoTop );
//						break;
//					case SORT_ACOMPLETED:
//						if( m_bShowCompleted )
//							qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), asortByCompletedNoTop );
//						break;
//					case SORT_ATAG:
//						qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), asortByTagNoTop );
//						break;
//					case SORT_AUPLOADER:
//						if( m_bShowUploader )
//							qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), asortByUploaderNoTop );
//						break;
//					case SORT_DNAME:
//						qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByNameNoTop );
//						break;
//					case SORT_DCOMPLETE:
//						qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByCompleteNoTop );
//						break;
//					case SORT_DINCOMPLETE:
//						qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByDLNoTop );
//						break;
//					case SORT_DADDED:
//						if( m_bShowAdded )
//							qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByAddedNoTop );
//						break;
//					case SORT_DSIZE:
//						if( m_bShowSize )
//							qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortBySizeNoTop );
//						break;
//					case SORT_DFILES:
//						if( m_bShowNumFiles )
//							qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByFilesNoTop );
//						break;
//					case SORT_DCOMMENTS:
//						if( m_bAllowComments )
//							qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByCommentsNoTop );
//						break;
//					case SORT_DCOMPLETED:
//						if( m_bShowCompleted )
//							qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByCompletedNoTop );
//						break;
//					case SORT_DTAG:
//						qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByTagNoTop );
//						break;
//					case SORT_DUPLOADER:
//						if( m_bShowUploader )
//							qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByUploaderNoTop );
//						break;
//					default:
//						// default action is to sort by added if we can
//						if( m_bShowAdded )
//							qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByAddedNoTop );
//					}
//				}
//			}
//			else
//			{
//				// default action is to sort by added if we can

//				if( m_bShowAdded )
//					if( !bNoTop )
//						qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByAdded );
//					else
//						qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByAddedNoTop );
//			}
//		}
//		else
//		{
//			// sort is disabled, but default action is to sort by added if we can

//			if( m_bShowAdded )
//				if( !bNoTop )
//					qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByAdded );
//				else
//					qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByAddedNoTop );
//		}
			
			

		// Main Header
		
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["index_page"], string( CSS_INDEX ), string( ), IS_INDEX, CODE_200 );
		
		pResponse->strContent += "<script type=\"text/javascript\">\n";
		pResponse->strContent += "<!--\n";
		
		pResponse->strContent += "function getCookie( name ) {\n";
		pResponse->strContent += "  var start = document.cookie.indexOf( name + \"=\" );\n";
		pResponse->strContent += "  var len = start + name.length + 1;\n";
		pResponse->strContent += "  if ( ( !start ) && ( name != document.cookie.substring( 0, name.length ) ) ) {\n";
		pResponse->strContent += "    return null;\n";
		pResponse->strContent += "  }\n";
		pResponse->strContent += "  if ( start == -1 ) return null;\n";
		pResponse->strContent += "  var end = document.cookie.indexOf( ';', len );\n";
		pResponse->strContent += "  if ( end == -1 ) end = document.cookie.length;\n";
		pResponse->strContent += "  return unescape( document.cookie.substring( len, end ) );\n";
		pResponse->strContent += "}\n";
		
		pResponse->strContent += "function setCookie( name, value, expires, path, domain, secure ) {\n";
		pResponse->strContent += "  var today = new Date();\n";
		pResponse->strContent += "  today.setTime( today.getTime() );\n";
		pResponse->strContent += "  if ( expires ) {\n";
		pResponse->strContent += "    expires = expires * 1000 * 60 * 60 * 24;\n";
		pResponse->strContent += "  }\n";
		pResponse->strContent += "  var expires_date = new Date( today.getTime() + (expires) );\n";
		pResponse->strContent += "  document.cookie = name+'='+escape( value ) +\n";
		pResponse->strContent += "  ( ( expires ) ? ';expires='+expires_date.toGMTString() : '' ) + //expires.toGMTString()\n";
		pResponse->strContent += "  ( ( path ) ? ';path=' + path : '' ) +\n";
		pResponse->strContent += "  ( ( domain ) ? ';domain=' + domain : '' ) +\n";
		pResponse->strContent += "  ( ( secure ) ? ';secure' : '' );\n";
		pResponse->strContent += "}\n";
		

		pResponse->strContent += "function deleteCookie( name, path, domain ) {\n";
		pResponse->strContent += "  if ( getCookie( name ) ) document.cookie = name + '=' +\n";
		pResponse->strContent += "    ( ( path ) ? ';path=' + path : '') +\n";
		pResponse->strContent += "    ( ( domain ) ? ';domain=' + domain : '' ) +\n";

		pResponse->strContent += "    ';expires=Thu, 01-Jan-1970 00:00:01 GMT';\n";
		pResponse->strContent += "}\n";
		
		// hide
		pResponse->strContent += "function hide( id ) {\n";
		pResponse->strContent += "  var element = document.getElementById( id );\n";
		pResponse->strContent += "  element.style.display=\"none\";\n";
		pResponse->strContent += "  setCookie( 'hide_filter', '1', '30', '', '', '' );\n";
		pResponse->strContent += "}\n";
		
		// display
		pResponse->strContent += "function display( id ) {\n";
		pResponse->strContent += "  var element = document.getElementById( id );\n";
		pResponse->strContent += "  element.style.display=\"\";\n";
		pResponse->strContent += "  setCookie( 'hide_filter', '0', '30', '', '', '' );\n";
		pResponse->strContent += "}\n";
		
		pResponse->strContent += "function change(id,show_name,hide_name) {\n";
		pResponse->strContent += "  var element = document.getElementById( id );\n";
		pResponse->strContent += "  if (element.style.display == \"\") {\n";
		pResponse->strContent += "    hide( id );\n";
		pResponse->strContent += "    return '<span class=\"blue\">' + show_name + '</span>'; }\n";
		pResponse->strContent += "  else {\n";
		pResponse->strContent += "    display( id );\n";
		pResponse->strContent += "    return '<span class=\"red\">' + hide_name + '</span>'; }\n";
		pResponse->strContent += "}\n";
		
		pResponse->strContent += "var checkflag = \"false\";\n";
		pResponse->strContent += "function check(field,checker,name) {\n";
		pResponse->strContent += "  for (i = 0; i < field.length; i++) {\n";
		pResponse->strContent += "    if (field[i].name == checker) {\n";
		pResponse->strContent += "      if (field[i].checked == true) {\n";
		pResponse->strContent += "        checkflag = \"true\"; }\n";
		pResponse->strContent += "      else {\n";
		pResponse->strContent += "        checkflag = \"false\"; }\n";
		pResponse->strContent += "    }\n";
		pResponse->strContent += "  }\n";
		pResponse->strContent += "  for (i = 0; i < field.length; i++) {\n";
		pResponse->strContent += "    if (field[i].name.indexOf(name) != -1) {\n";
		pResponse->strContent += "      if (checkflag == \"true\") {\n";
		pResponse->strContent += "        field[i].checked = true; }\n";
		pResponse->strContent += "      else {\n";
		pResponse->strContent += "        field[i].checked = false; }\n";
		pResponse->strContent += "      }\n";
		pResponse->strContent += "  }\n";
		pResponse->strContent += "}\n";
		
// 		pResponse->strContent += "var element_filter = document.getElementById( 'id_a_index_filter' );\n";
		pResponse->strContent += "if( getCookie( 'hide_filter' ) == \"0\" ) {\n";
// 		pResponse->strContent += "  element_filter.innerHTML=check('id_index_filter','" + gmapLANG_CFG["index_filter_show"] + "','" + gmapLANG_CFG["index_filter_hide"] + "');\n";
		pResponse->strContent += "  hided = \"false\";\n";
		pResponse->strContent += "}\n\n";

		pResponse->strContent += "if (window.XMLHttpRequest)\n";
		pResponse->strContent += "{// code for IE7+, Firefox, Chrome, Opera, Safari\n";
		pResponse->strContent += "  xmlhttp=new XMLHttpRequest(); }\n";
		pResponse->strContent += "else\n";
		pResponse->strContent += "{// code for IE6, IE5\n";
		pResponse->strContent += "  xmlhttp=new ActiveXObject(\"Microsoft.XMLHTTP\"); }\n";

		// bookmark
		pResponse->strContent += "function bookmark(id,bookmark_link,nobookmark_link) {\n";
		pResponse->strContent += "  var bookmarkLink = document.getElementById( 'bookmark'+id );\n";
		pResponse->strContent += "  xmlhttp.onreadystatechange=function() {\n";
		pResponse->strContent += "    if (xmlhttp.readyState==4) {\n";
		pResponse->strContent += "      if (xmlhttp.status==200) {\n";
		pResponse->strContent += "        if (bookmarkLink.title == bookmark_link) {\n";
		pResponse->strContent += "          bookmarkLink.title = nobookmark_link;\n";
		pResponse->strContent += "          bookmarkLink.innerHTML = '" + gmapLANG_CFG["bookmarked_icon"] + "'; }\n";
		pResponse->strContent += "        else {\n";
		pResponse->strContent += "          bookmarkLink.title = bookmark_link;\n";
		pResponse->strContent += "          bookmarkLink.innerHTML = '" + gmapLANG_CFG["bookmark_icon"] + "'; }\n";
		pResponse->strContent += "      }\n";
		pResponse->strContent += "    }\n";
		pResponse->strContent += "  }\n";
		pResponse->strContent += "  if (bookmarkLink.title == bookmark_link) {\n";
		pResponse->strContent += "    xmlhttp.open(\"GET\",\"" + RESPONSE_STR_LOGIN_HTML + "?bookmark=\" + id,true);\n";
		pResponse->strContent += "    xmlhttp.send(); }\n";
		pResponse->strContent += "  else {\n";
		pResponse->strContent += "    xmlhttp.open(\"GET\",\"" + RESPONSE_STR_LOGIN_HTML + "?nobookmark=\" + id,true);\n";
		pResponse->strContent += "    xmlhttp.send(); }\n";
		pResponse->strContent += "}\n";

		pResponse->strContent += "//-->\n";
		pResponse->strContent += "</script>\n\n";
		
		int64 ulUploaded = 0;
		int64 ulDownloaded = 0;
		float flShareRatio = 0;
		int64 ulBonus = 0;

		int64 last_time = 0;
		int64 last_time_info = 0;
		int64 warned = 0;

		if( !pRequest->user.strUID.empty( ) )
		{
			CMySQLQuery *pQueryUser = new CMySQLQuery( "SELECT buploaded,bdownloaded,bbonus,UNIX_TIMESTAMP(blast_index),UNIX_TIMESTAMP(blast_info),UNIX_TIMESTAMP(bwarned) FROM users WHERE buid=" + pRequest->user.strUID );
	
			vector<string> vecQueryUser;
	
			vecQueryUser.reserve(6);
			
			vecQueryUser = pQueryUser->nextRow( );
		
			delete pQueryUser;
			
			if( vecQueryUser.size( ) == 6 )
			{
				ulUploaded = UTIL_StringTo64( vecQueryUser[0].c_str( ) );
				ulDownloaded = UTIL_StringTo64( vecQueryUser[1].c_str( ) );
				ulBonus = UTIL_StringTo64( vecQueryUser[2].c_str( ) );
		
				if( ulDownloaded == 0 )
				{
					if( ulUploaded == 0 )
						flShareRatio = 0;
					else
						flShareRatio = -1;
				}
				else
					flShareRatio = (float)ulUploaded / (float)ulDownloaded;
				
				last_time = UTIL_StringTo64( vecQueryUser[3].c_str( ) );
				last_time_info = UTIL_StringTo64( vecQueryUser[4].c_str( ) );
				warned = UTIL_StringTo64( vecQueryUser[5].c_str( ) );
			}
		}
		
		bool bAnnounce = false;
		
		if( !pRequest->user.strUID.empty( ) )
		{
			CMySQLQuery *pQueryAnn = new CMySQLQuery( "SELECT UNIX_TIMESTAMP(bposted),baccess FROM announcements ORDER BY bposted DESC" );
		
			vector<string> vecQueryAnn;
	
			vecQueryAnn.reserve(2);

			vecQueryAnn = pQueryAnn->nextRow( );
		
			while( vecQueryAnn.size( ) == 2 )
			{
				if( atoi( vecQueryAnn[1].c_str( ) ) < pRequest->user.ucAccess )
				{
					if( last_time_info < UTIL_StringTo64( vecQueryAnn[0].c_str( ) ) )
						bAnnounce = true;
					
					break;
				}
				vecQueryAnn = pQueryAnn->nextRow( );
			}
			delete pQueryAnn;
		}

		if( !pRequest->user.strUID.empty( ) )
		{
			if( warned > 0 )
			{
				pResponse->strContent += "<table class=\"index_warned\">\n";
				pResponse->strContent += "<tr class=\"index_warned\">\n";
				pResponse->strContent += "<td class=\"index_warned\">\n";
				pResponse->strContent += gmapLANG_CFG["index_warned"];
				pResponse->strContent += "</td></tr></table>";
			}
			if( m_bRatioRestrict )
			{
				for( int i = 0; i < 6; i++ )
					if( ulDownloaded / 1024 / 1024 / 1024 >= RequiredDown[i] )
					{
						if( flShareRatio < RequiredRatio[i] )
						{
							char szFloat[16];
							memset( szFloat, 0, sizeof( szFloat ) / sizeof( char ) );
							snprintf( szFloat, sizeof( szFloat ) / sizeof( char ), "%0.3f", RequiredRatio[i] );
							pResponse->strContent += "<table class=\"index_ratio_warned\">\n";
							pResponse->strContent += "<tr class=\"index_ratio_warned\">\n";
							pResponse->strContent += "<td class=\"index_ratio_warned\">\n";
							pResponse->strContent += UTIL_Xsprintf( gmapLANG_CFG["index_ratio_warned"].c_str( ), CAtomInt( RequiredDown[i] ).toString( ).c_str( ), string( szFloat ).c_str( ) );
							pResponse->strContent += "</td></tr></table>";
						}
						
						break;
					}
					else if( ulDownloaded / 1024.0 / 1024 / 1024 > RequiredDown[i] * 0.9 )
					{
						if( ulUploaded / 1024.0 / 1024 / 1024 < RequiredDown[i] * RequiredRatio[i] )
						{
							char szFloat[16];
							memset( szFloat, 0, sizeof( szFloat ) / sizeof( char ) );
							snprintf( szFloat, sizeof( szFloat ) / sizeof( char ), "%0.2f", RequiredDown[i] * RequiredRatio[i] );
							pResponse->strContent += "<table class=\"index_ratio_warned\">\n";
							pResponse->strContent += "<tr class=\"index_ratio_warned\">\n";
							pResponse->strContent += "<td class=\"index_ratio_warned\">\n";
							pResponse->strContent += UTIL_Xsprintf( gmapLANG_CFG["index_ratio_warned_nearly"].c_str( ), CAtomInt( RequiredDown[i] ).toString( ).c_str( ), string( szFloat ).c_str( ) );
							pResponse->strContent += "</td></tr></table>";
						}
					}
			}
			if( bAnnounce )
			{
				pResponse->strContent += "<table class=\"index_announce\">\n";
				pResponse->strContent += "<tr class=\"index_announce\">\n";
				pResponse->strContent += "<td class=\"index_announce\">\n";
				pResponse->strContent += "<a class=\"index_announce\" href=\"" + RESPONSE_STR_INFO_HTML + "\">" + gmapLANG_CFG["index_announce"] + "</a>";
				pResponse->strContent += "</td></tr></table>";
			}
			if( pResponse->strContent.find( gmapLANG_CFG["messages_have_unread"] ) != string :: npos )
			{
				pResponse->strContent += "<table class=\"index_messages_unread\">\n";
				pResponse->strContent += "<tr class=\"index_messages_unread\">\n";
				pResponse->strContent += "<td class=\"index_messages_unread\">\n";
				pResponse->strContent += "<a class=\"index_messages_unread\" href=\"" + RESPONSE_STR_MESSAGES_HTML + "\">" + gmapLANG_CFG["messages_unread_notice"] + "</a>";
				pResponse->strContent += "</td></tr></table>\n";
			}
		}

		// some preliminary search crap

		string strSearch( pRequest->mapParams["search"] );
//		const string strLowerSearch( UTIL_ToLower( strSearch ) );

		const string strUploader( pRequest->mapParams["uploader"] );
//		const string cstrLowerUploader( UTIL_ToLower( strUploader ) );

		string strMatch( pRequest->mapParams["match"] );
		// filters

		string strFilter( pRequest->mapParams["tag"] );
		const string strMedium( pRequest->mapParams["medium"] );
		const string strQuality( pRequest->mapParams["quality"] );
		const string strEncode( pRequest->mapParams["encode"] );
		
		const string cstrDay( pRequest->mapParams["day"] );
		
		const string cstrNoTag( pRequest->mapParams["notag"] );
		
		// Section
		const string cstrSection( pRequest->mapParams["section"] );

		// Sub-filtering mode
		const string cstrMode( pRequest->mapParams["mode"] );

		const string cstrPerPage( pRequest->mapParams["per_page"] );
		
		bool bDefaultTag = false;
		bool bAddedPassed = false;
		
		if( !pRequest->user.strUID.empty( ) )
		{
			CMySQLQuery *pQueryPrefs = new CMySQLQuery( "SELECT bdefaulttag,baddedpassed FROM users_prefs WHERE buid=" + pRequest->user.strUID );
	
			map<string, string> mapPrefs;

			mapPrefs = pQueryPrefs->nextRowMap( );

			delete pQueryPrefs;
		
			if( strFilter.empty( ) && cstrNoTag != "1" && !mapPrefs["bdefaulttag"].empty( ) )
			{
				strFilter = mapPrefs["bdefaulttag"];
				bDefaultTag = true;
			}
				
			if( !mapPrefs["baddedpassed"].empty( ) && mapPrefs["baddedpassed"] == "1" )
				bAddedPassed = true;
		}

		vector<string> vecBookmark;
		vecBookmark.reserve(64);

		if( !pRequest->user.strUID.empty( ) && ( pRequest->user.ucAccess & m_ucAccessBookmark ) )
		{
			CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bid FROM bookmarks WHERE buid=" + pRequest->user.strUID );
		
			vector<string> vecQuery;
		
			vecQuery.reserve(1);

			vecQuery = pQuery->nextRow( );

			while( vecQuery.size( ) == 1 )
			{
				vecBookmark.push_back( vecQuery[0] );

				vecQuery = pQuery->nextRow( );
			}
			
			delete pQuery;
		}
		
		vector<string> vecSearch;
		vecSearch.reserve(64);
		vector<string> vecUploader;
		vecUploader.reserve(64);
		vector<string> vecFilter;
		vecFilter.reserve(64);
		vector<string> vecMedium;
		vecMedium.reserve(64);
		vector<string> vecQuality;
		vecQuality.reserve(64);
		vector<string> vecEncode;
		vecEncode.reserve(64);
		
		vecSearch = UTIL_SplitToVector( strSearch, " " );
		vecUploader = UTIL_SplitToVector( strUploader, " " );
		vecFilter = UTIL_SplitToVector( strFilter, " " );
		vecMedium = UTIL_SplitToVector( strMedium, " " );
		vecQuality = UTIL_SplitToVector( strQuality, " " );
		vecEncode = UTIL_SplitToVector( strEncode, " " );

		// Top search
		if( m_bSearch )
		{
			pResponse->strContent += "<form class=\"search_index_top\" name=\"topsearch\" method=\"get\" action=\"" + RESPONSE_STR_INDEX_HTML + "\">\n";
			
			pResponse->strContent += "<a class=\"index_filter\" href=\"javascript: ;\" onclick=\"javascript: this.innerHTML=change('id_index_filter','" + gmapLANG_CFG["index_filter_show"] + "','" + gmapLANG_CFG["index_filter_hide"] + "')\">";
			
			if( pRequest->mapCookies["hide_filter"] == "0" )
			{
				pResponse->strContent += "<span class=\"red\">" + gmapLANG_CFG["index_filter_hide"] + "</span></a>";
				pResponse->strContent += "<div id=\"id_index_filter\" class=\"index_filter\">\n";
			}
			else
			{
				pResponse->strContent += "<span class=\"blue\">" + gmapLANG_CFG["index_filter_show"] + "</span></a>";
				pResponse->strContent += "<div id=\"id_index_filter\" class=\"index_filter\" style=\"display: none\">\n";
			}
			
			if( !m_vecTags.empty( ) )
			{
				pResponse->strContent += "<table class=\"index_filter\">\n";
				pResponse->strContent += "<tr class=\"index_filter\">\n";

				unsigned char ucTag = 0;

				string strNameIndex = string( );
				string strTag = string( );

				for( vector< pair< string, string > > :: iterator ulTagKey = m_vecTags.begin( ); ulTagKey != m_vecTags.end( ); ulTagKey++ )
				{
					// Added by =Xotic= 
					// The Trinity Edition 7.5r3 - Modification Begins
					// The following changes the CLEAR FILTER link to read
					// CLEAR FILTER AND SEARCH RESULTS which appears before the
					// Table of Torrents
					// ulTagKey also sets a CSS class "clearfilter" which can be used
					// to HIDE this link using the following CSS command 
					// .clearfilter{display:none}

					if( !(*ulTagKey).first.empty( ) )
					{
						strNameIndex = (*ulTagKey).first;
						strTag = (*ulTagKey).second;
						if( strNameIndex == "201" || strNameIndex == "301" || strNameIndex == "401" )
							pResponse->strContent += "</tr><tr class=\"index_filter\">\n";
						if( strNameIndex == "101" || strNameIndex == "201" || strNameIndex == "301" )
						{
							pResponse->strContent += "<td class=\"index_filter\">";
							pResponse->strContent += "<input name=\"checker" + strNameIndex.substr( 0, 1 ) + "\" type=checkbox onclick=\"javascript: check(form,'checker" + strNameIndex.substr( 0, 1 ) + "','tag" + strNameIndex.substr( 0, 1 ) + "')\">";
							pResponse->strContent += "<a class=\"filter_by\" title=\"" + UTIL_RemoveHTML( gmapLANG_CFG["filter_by"] + ": " + strTag.substr( 0, strTag.find( ' ' ) ) )+ "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?tag=" + strNameIndex[0] + "\">";
							pResponse->strContent += strTag.substr( 0, strTag.find( ' ' ) ) + "</a> : </td>\n\n";
						}
						else if( strNameIndex == "401" )
							pResponse->strContent += "<td class=\"index_filter\"></td>\n\n";
						pResponse->strContent += "<td class=\"index_filter";
						for( vector<string> :: iterator ulKey = vecFilter.begin( ); ulKey != vecFilter.end( ); ulKey++ )
							if( *ulKey == strNameIndex || *ulKey == strNameIndex.substr( 0, 1 ) )
								pResponse->strContent += "_selected";
//						if( strNameIndex == strFilter )
//						if( strFilter.find( strNameIndex ) != string :: npos || strNameIndex.substr( 0, 1 ) == strFilter )
//							pResponse->strContent += "_selected";
						pResponse->strContent += "\">";
						pResponse->strContent += "<input name=\"tag" + strNameIndex + "\" type=checkbox";
//						if( strFilter.find( strNameIndex ) != string :: npos || strNameIndex.substr( 0, 1 ) == strFilter )
//							pResponse->strContent += " checked";
						for( vector<string> :: iterator ulKey = vecFilter.begin( ); ulKey != vecFilter.end( ); ulKey++ )
							if( *ulKey == strNameIndex || *ulKey == strNameIndex.substr( 0, 1 ) )
								pResponse->strContent += " checked";
						pResponse->strContent += "><a class=\"filter_by\" title=\"" + UTIL_RemoveHTML( gmapLANG_CFG["filter_by"] + ": " + strTag )+ "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?tag=" + strNameIndex;
						pResponse->strContent += "\">\n";

						pResponse->strContent += UTIL_RemoveHTML( strTag );

						pResponse->strContent += "</a></td>\n\n";
					}

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
				}
				pResponse->strContent += "</tr></table>\n\n";
			}
			
			if( !m_vecMediums.empty( ) || !m_vecQualities.empty( ) || !m_vecEncodes.empty( ) )
			{
				pResponse->strContent += "<table class=\"index_filter\">\n";
				pResponse->strContent += "<tr class=\"index_filter\">\n";
			
				if( !m_vecMediums.empty( ) )
				{
					pResponse->strContent += "<td class=\"index_filter\">" + gmapLANG_CFG["result_medium"] + " : </td>";
					for( vector< string > :: iterator ulTagKey = m_vecMediums.begin( ); ulTagKey != m_vecMediums.end( ); ulTagKey++ )
					{
						pResponse->strContent += "<td class=\"index_filter";
						for( vector<string> :: iterator ulKey = vecMedium.begin( ); ulKey != vecMedium.end( ); ulKey++ )
							if( *ulKey == *ulTagKey )
								pResponse->strContent += "_selected";
//						if( strMedium.find( *ulTagKey ) != string :: npos )
//							pResponse->strContent += "_selected";
						pResponse->strContent += "\">";
						pResponse->strContent += "<input name=\"medium_" + *ulTagKey + "\" type=checkbox";
						for( vector<string> :: iterator ulKey = vecMedium.begin( ); ulKey != vecMedium.end( ); ulKey++ )
							if( *ulKey == *ulTagKey )
								pResponse->strContent += " checked";
//						if( strMedium.find( *ulTagKey ) != string :: npos )
//							pResponse->strContent += " checked";
						pResponse->strContent += "><a class=\"filter_by\" href=\"" + RESPONSE_STR_INDEX_HTML + "?medium=" + *ulTagKey + "\">" + gmapLANG_CFG["medium_"+*ulTagKey] + "</td>\n";
					}
				}
				
				if( !m_vecQualities.empty( ) )
				{
					if( !m_vecMediums.empty( ) )
						pResponse->strContent += "<td class=\"index_filter\"></td>";
					pResponse->strContent += "<td class=\"index_filter\">" + gmapLANG_CFG["result_quality"] + " : </td>";
					for( vector< string > :: iterator ulTagKey = m_vecQualities.begin( ); ulTagKey != m_vecQualities.end( ); ulTagKey++ )
					{
						pResponse->strContent += "<td class=\"index_filter";
						for( vector<string> :: iterator ulKey = vecQuality.begin( ); ulKey != vecQuality.end( ); ulKey++ )
							if( *ulKey == *ulTagKey )
								pResponse->strContent += "_selected";
//						if( strQuality.find( *ulTagKey ) != string :: npos )
//							pResponse->strContent += "_selected";
						pResponse->strContent += "\">";
						pResponse->strContent += "<input name=\"quality_" + *ulTagKey + "\" type=checkbox";
						for( vector<string> :: iterator ulKey = vecQuality.begin( ); ulKey != vecQuality.end( ); ulKey++ )
							if( *ulKey == *ulTagKey )
								pResponse->strContent += " checked";
//						if( strQuality.find( *ulTagKey ) != string :: npos )
//							pResponse->strContent += " checked";
						pResponse->strContent += "><a class=\"filter_by\" href=\"" + RESPONSE_STR_INDEX_HTML + "?quality=" + *ulTagKey + "\">" + gmapLANG_CFG["quality_"+*ulTagKey] + "</td>\n";
					}
				}
				pResponse->strContent += "</tr>\n";
				
				if( !m_vecEncodes.empty( ) )
				{
					pResponse->strContent += "<tr class=\"index_filter\">\n";
					pResponse->strContent += "<td class=\"index_filter\">" + gmapLANG_CFG["result_encode"] + " : </td>";
				
					for( vector< string > :: iterator ulTagKey = m_vecEncodes.begin( ); ulTagKey != m_vecEncodes.end( ); ulTagKey++ )
					{
						pResponse->strContent += "<td class=\"index_filter";

						for( vector<string> :: iterator ulKey = vecEncode.begin( ); ulKey != vecEncode.end( ); ulKey++ )
							if( *ulKey == *ulTagKey )
								pResponse->strContent += "_selected";
//						if( strEncode.find( *ulTagKey ) != string :: npos )
//							pResponse->strContent += "_selected";
						pResponse->strContent += "\">";
						pResponse->strContent += "<input name=\"encode_" + *ulTagKey + "\" type=checkbox";
						for( vector<string> :: iterator ulKey = vecEncode.begin( ); ulKey != vecEncode.end( ); ulKey++ )
							if( *ulKey == *ulTagKey )
								pResponse->strContent += " checked";
//						if( strEncode.find( *ulTagKey ) != string :: npos )
//							pResponse->strContent += " checked";
						pResponse->strContent += "><a class=\"filter_by\" href=\"" + RESPONSE_STR_INDEX_HTML + "?encode=" + *ulTagKey + "\">" + gmapLANG_CFG["encode_"+*ulTagKey] + "</td>\n";
					}
					pResponse->strContent += "<td class=\"index_filter\"></td>";
//					pResponse->strContent += "<td class=\"index_filter\" colspan=2>";
//					pResponse->strContent += "<input type=button value=\"" + gmapLANG_CFG["index_no_default_tag"] + "\" onClick=\"javascript: window.location='" + INDEX_HTML + "?notag=1';\">";
//					pResponse->strContent += "</td>";
					pResponse->strContent += "</tr>\n";
				}
				
				pResponse->strContent += "</table>\n";
			}

			pResponse->strContent += "</div>\n\n";

			if( !cstrPerPage.empty( ) )
				pResponse->strContent += "<input name=\"per_page\" type=hidden value=\"" + cstrPerPage + "\">\n";
			if( !cstrNoTag.empty( ) )
				pResponse->strContent += "<input name=\"notag\" type=hidden value=\"" + cstrNoTag + "\">\n";

//			if( m_bUseButtons )
//			{
				pResponse->strContent += "<p><label for=\"toptorrentsearch\">" + gmapLANG_CFG["torrent_search"] + "</label> <input name=\"search\" id=\"toptorrentsearch\" alt=\"[" + gmapLANG_CFG["torrent_search"] + "]\" type=text size=40";
				
				if( !strSearch.empty( ) || !strUploader.empty( ) )
				{
					pResponse->strContent += " value=\"";
					if( !strSearch.empty( ) )
						pResponse->strContent += UTIL_RemoveHTML( strSearch );
					else if( !strUploader.empty( ) )
						pResponse->strContent += UTIL_RemoveHTML( strUploader );
					pResponse->strContent += "\"";
				}
				pResponse->strContent += ">\n";
				pResponse->strContent += "<select id=\"smode\" name=\"smode\">";
				pResponse->strContent += "\n<option value=\"name\">" + gmapLANG_CFG["name"];
				pResponse->strContent += "\n<option value=\"uploader\"";
				if( !strUploader.empty( ) )
					pResponse->strContent += " selected";
				pResponse->strContent += ">" + gmapLANG_CFG["uploader"];
				pResponse->strContent += "\n</select>\n";

				pResponse->strContent += "<select id=\"match\" name=\"match\">";
				pResponse->strContent += "\n<option value=\"and\">" + gmapLANG_CFG["match_and"];
				pResponse->strContent += "\n<option value=\"or\"";
				if( !strMatch.empty( ) && strMatch == "or" )
					pResponse->strContent += " selected";
				pResponse->strContent += ">" + gmapLANG_CFG["match_or"];
				pResponse->strContent += "\n<option value=\"eor\"";
				if( !strMatch.empty( ) && strMatch == "eor" )
					pResponse->strContent += " selected";
				pResponse->strContent += ">" + gmapLANG_CFG["match_eor"];
				pResponse->strContent += "\n</select>\n";

				pResponse->strContent += Button_Submit( "top_submit_search", gmapLANG_CFG["search"] );
				pResponse->strContent += Button_Submit( "top_clear_filter_and_search", gmapLANG_CFG["clear_filter_search"] );

				pResponse->strContent += "</p>\n";
//			}
//			else
//				pResponse->strContent += "<p><label for=\"toptorrentsearch\">" + gmapLANG_CFG["torrent_search"] + "</label> <input name=\"search\" id=\"toptorrentsearch\" alt=\"[" + gmapLANG_CFG["torrent_search"] + "]\" type=text size=40> <a title=\"" + gmapLANG_CFG["clear_filter_search"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "\">" + gmapLANG_CFG["clear_filter_search"] + "</a></p>\n";

			pResponse->strContent += "</form>\n\n";
		}

		// Sub-filtering mode
		pResponse->strContent += "<p class=\"subfilter\">" + gmapLANG_CFG["index_subfilter"] + ": \n";

		// All
		pResponse->strContent += "<a title=\"" + gmapLANG_CFG["subfilter_all"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?mode=All";
		
		vector< pair< string, string > > vecParams;
		vecParams.reserve(64);
		string strJoined = string( );
		
		vecParams.push_back( pair<string, string>( string( "per_page" ), cstrPerPage ) );
		vecParams.push_back( pair<string, string>( string( "sort" ), strSort ) );
		vecParams.push_back( pair<string, string>( string( "search" ), strSearch ) );
		vecParams.push_back( pair<string, string>( string( "match" ), strMatch ) );
		vecParams.push_back( pair<string, string>( string( "tag" ), strFilter ) );
		vecParams.push_back( pair<string, string>( string( "medium" ), strMedium ) );
		vecParams.push_back( pair<string, string>( string( "quality" ), strQuality ) );
		vecParams.push_back( pair<string, string>( string( "encode" ), strEncode ) );
		vecParams.push_back( pair<string, string>( string( "uploader" ), strUploader ) );
		vecParams.push_back( pair<string, string>( string( "section" ), cstrSection ) );
		vecParams.push_back( pair<string, string>( string( "notop" ), cstrNoTop ) );
		vecParams.push_back( pair<string, string>( string( "day" ), cstrDay ) );
		vecParams.push_back( pair<string, string>( string( "notag" ), cstrNoTag ) );
		
		strJoined = UTIL_RemoveHTML( UTIL_HTMLJoin( vecParams, string( "&" ), string( "&" ), string( "=" ) ) );
		
		pResponse->strContent += strJoined;
		
		pResponse->strContent += "\">" + gmapLANG_CFG["subfilter_all"] + "</a>\n\n";

		// Seeded
		pResponse->strContent += "<span class=\"pipe\"> | </span>\n";
		pResponse->strContent += "<a title=\"" + gmapLANG_CFG["subfilter_seeded"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?mode=Seeded";
		
		pResponse->strContent += strJoined;

		pResponse->strContent += "\">" + gmapLANG_CFG["subfilter_seeded"] + "</a>\n\n";

		// Unseeded
		pResponse->strContent += "<span class=\"pipe\"> | </span>\n";
		pResponse->strContent += "<a title=\"" + gmapLANG_CFG["subfilter_unseeded"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?mode=Unseeded";
		
		pResponse->strContent += strJoined;

		pResponse->strContent += "\">" + gmapLANG_CFG["subfilter_unseeded"] + "</a>\n\n";
		

		// MyTorrents

		if( ( pRequest->user.ucAccess & m_ucAccessViewTorrents ) && !pRequest->user.strUID.empty( ) )
		{
			pResponse->strContent += "<span class=\"pipe\"> | </span>\n";
			pResponse->strContent += "<a title=\"" + gmapLANG_CFG["subfilter_mytorrents"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?mode=MyTorrents";
		
			pResponse->strContent += strJoined;

			pResponse->strContent += "\">" + gmapLANG_CFG["subfilter_mytorrents"] + "</a>\n\n";
		}
		
		pResponse->strContent += "<span class=\"pipe\"> | </span>\n";
		
		vecParams.clear( );
		
		vecParams.push_back( pair<string, string>( string( "per_page" ), cstrPerPage ) );
		vecParams.push_back( pair<string, string>( string( "tag" ), strFilter ) );
		vecParams.push_back( pair<string, string>( string( "medium" ), strMedium ) );
		vecParams.push_back( pair<string, string>( string( "quality" ), strQuality ) );
		vecParams.push_back( pair<string, string>( string( "encode" ), strEncode ) );
		vecParams.push_back( pair<string, string>( string( "notag" ), cstrNoTag ) );
		
		strJoined = UTIL_RemoveHTML( UTIL_HTMLJoin( vecParams, string( "&" ), string( "&" ), string( "=" ) ) );
		
		// Hot
		pResponse->strContent += "[ <a class=\"hot\" title=\"" + gmapLANG_CFG["section_hot"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?section=hot";
		
		pResponse->strContent += strJoined;
		
		pResponse->strContent += "\">" + gmapLANG_CFG["section_hot"] + "</a> ]\n\n";

		
		// Classic
		pResponse->strContent += "[ <a class=\"classic_level_1\" title=\"" + gmapLANG_CFG["section_classic1"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?section=classic1";
		
		pResponse->strContent += strJoined;
		
		pResponse->strContent += "\">" + gmapLANG_CFG["section_classic1"] + "</a> ]\n\n";
		
		pResponse->strContent += "[ <a class=\"classic_level_2\" title=\"" + gmapLANG_CFG["section_classic2"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?section=classic2";
		
		pResponse->strContent += strJoined;
		
		pResponse->strContent += "\">" + gmapLANG_CFG["section_classic2"] + "</a> ]\n\n";
		
		pResponse->strContent += "[ <a class=\"classic_level_3\" title=\"" + gmapLANG_CFG["section_classic3"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?section=classic3";
		
		pResponse->strContent += strJoined;
		
		pResponse->strContent += "\">" + gmapLANG_CFG["section_classic3"] + "</a> ]\n\n";
		
		pResponse->strContent += "<span class=\"pipe\"> | </span>\n";
		
		// ReqSeeders
		pResponse->strContent += "[ <a class=\"req\" title=\"" + gmapLANG_CFG["section_reqseeders"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?section=req";
		
		pResponse->strContent += strJoined;

		
		pResponse->strContent += "\">" + gmapLANG_CFG["section_reqseeders"] + "</a> ]\n\n";
		
		// Free
		pResponse->strContent += "[ <a class=\"stats_free\" title=\"" + gmapLANG_CFG["section_free"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?section=free";
		
		pResponse->strContent += strJoined;
		
		pResponse->strContent += "\">" + gmapLANG_CFG["section_free"] + "</a> ]\n\n";
		
		pResponse->strContent += "<span class=\"pipe\"> | </span>\n";
		
		unsigned int uiHotDay = CFG_GetInt( "bnbt_hot_day", 3 );
		
		// Hot Rank
		pResponse->strContent += "* <a class=\"hot\" title=\"" + gmapLANG_CFG["section_hot_rank"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?sort=19&amp;notop=1&amp;day=";
		
		pResponse->strContent += CAtomInt( uiHotDay ).toString( );
		
		pResponse->strContent += strJoined;
		
		pResponse->strContent += "\">" + gmapLANG_CFG["section_hot_rank"] + "</a> *\n\n";

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

				if( uiOverridePerPage < 1 || uiOverridePerPage > m_uiPerPageMax )
					uiOverridePerPage = m_uiPerPage;
			}
		}
		else
		{
			uiOverridePerPage = (unsigned int)atoi( cstrPerPage.c_str( ) );

			if( uiOverridePerPage > m_uiPerPageMax )
				uiOverridePerPage = m_uiPerPage;
		}

		// Count matching torrents for top of page
		unsigned long ulFound = 0;

//		for( unsigned long ulKey = 0; ulKey < ulKeySize; ulKey++ )
//		{
//			if( !strFilter.empty( ) )
//			{
//				bool bFoundKey = false;
//				string :: size_type iStart = 0;
//				string :: size_type iEnd = 0;
//				string strKeyword = string( );
//				iStart = strFilter.find_first_not_of( " " );
//				
//				while( iStart != string :: npos && iEnd != string :: npos )
//				{
//					iEnd = strFilter.find_first_of( " ", iStart );
//					strKeyword = strFilter.substr( iStart, iEnd - iStart );
//					if( strKeyword.length( ) > 2 )
//					{
//						if( pTorrents[ulKey].strTag == strKeyword )
//						{
//							bFoundKey = true;
//							break;
//						}
//					}
//					else
//					{
//						if( pTorrents[ulKey].strTag.substr( 0, 2 - pTorrents[ulKey].strTag.length( ) % 2 ) == strKeyword )
//						{
//							bFoundKey = true;
//							break;
//						}
//					}
//					if( iEnd != string :: npos )
//						iStart = strFilter.find_first_not_of( " ", iEnd );
//					
//				}
//				if( bFoundKey == false )
//					continue;
//				// only count entries that match the filter
//			}
//			
//			if( !strMedium.empty( ) )
//			{
//				// only count entries that match the search
//				
//				bool bFoundKey = false;
//				string :: size_type iStart = 0;
//				string :: size_type iEnd = 0;
//				string strKeyword = string( );
//				iStart = strMedium.find_first_not_of( " " );
//				
//				while( iStart != string :: npos && iEnd != string :: npos )
//				{
//					iEnd = strMedium.find_first_of( " ", iStart );
//					strKeyword = strMedium.substr( iStart, iEnd - iStart );
//					if( pTorrents[ulKey].strLowerName.find( strKeyword ) != string :: npos )
//					{
//						bFoundKey = true;
//						break;
//					}
//					if( iEnd != string :: npos )
//						iStart = strMedium.find_first_not_of( " ", iEnd );
//					
//				}
//				if( bFoundKey == false )
//					continue;
//			}
//			
//			if( !strQuality.empty( ) )
//			{
//				// only count entries that match the search
//				
//				bool bFoundKey = false;
//				string :: size_type iStart = 0;
//				string :: size_type iEnd = 0;
//				string strKeyword = string( );
//				iStart = strQuality.find_first_not_of( " " );
//				
//				while( iStart != string :: npos && iEnd != string :: npos )
//				{
//					iEnd = strQuality.find_first_of( " ", iStart );
//					strKeyword = strQuality.substr( iStart, iEnd - iStart );
//					if( pTorrents[ulKey].strLowerName.find( strKeyword ) != string :: npos )
//					{
//						bFoundKey = true;
//						break;
//					}
//					if( iEnd != string :: npos )
//						iStart = strQuality.find_first_not_of( " ", iEnd );
//					
//				}
//				if( bFoundKey == false )
//					continue;
//			}
//			
//			if( !strEncode.empty( ) )
//			{
//				// only count entries that match the search
//				
//				bool bFoundKey = false;
//				string :: size_type iStart = 0;
//				string :: size_type iEnd = 0;
//				string strKeyword = string( );
//				iStart = strEncode.find_first_not_of( " " );
//				
//				while( iStart != string :: npos && iEnd != string :: npos )
//				{
//					iEnd = strEncode.find_first_of( " ", iStart );
//					strKeyword = strEncode.substr( iStart, iEnd - iStart );
//					if( pTorrents[ulKey].strLowerName.find( strKeyword ) != string :: npos )
//					{
//						bFoundKey = true;
//						break;
//					}
//					if( iEnd != string :: npos )
//						iStart = strEncode.find_first_not_of( " ", iEnd );
//					
//				}
//				if( bFoundKey == false )
//					continue;
//			}

//			if( !strSearch.empty( ) )
//			{
//				// only count entries that match the search
//				
//				bool bFoundKey = true;
//				string :: size_type iStart = 0;
//				string :: size_type iEnd = 0;
//				string strKeyword = string( );
//				iStart = strLowerSearch.find_first_not_of( " " );
//				
//				while( iStart != string :: npos && iEnd != string :: npos )
//				{
//					iEnd = strLowerSearch.find_first_of( " ", iStart );
//					strKeyword = strLowerSearch.substr( iStart, iEnd - iStart );
//					if( pTorrents[ulKey].strLowerName.find( strKeyword ) == string :: npos )
//						bFoundKey = false;
//					if( iEnd != string :: npos )
//						iStart = strLowerSearch.find_first_not_of( " ", iEnd );
//					
//				}
//				if( bFoundKey == false )
//					continue;
//			}

//			if( !strUploader.empty( ) )
//			{
//				if( UTIL_ToLower( pTorrents[ulKey].strUploader ).find( cstrLowerUploader ) == string :: npos )
//					continue;
//			}
//			
//			if( !cstrDay.empty( ) )
//			{
//				struct tm *now_tm, time_tm;
//				int64 year, month, day, hour, minute, second;
//				float passed;
//				sscanf( pTorrents[ulKey].strAdded.c_str( ), "%d-%d-%d %d:%d:%d",&year,&month,&day,&hour,&minute,&second );
//				time_tm.tm_year = year-1900;
//				time_tm.tm_mon = month-1;
//				time_tm.tm_mday = day;
//				time_tm.tm_hour = hour;
//				time_tm.tm_min = minute;
//				time_tm.tm_sec = second;
//				passed = difftime(now_t, mktime(&time_tm));
//				if( passed > ( (time_t)( atoi( cstrDay.c_str( ) ) * 3600 * 24 ) ) )
//					continue;
//			}
//			
//			if( !cstrSection.empty( ) )
//			{
//				if( cstrSection == "classic1" && pTorrents[ulKey].ucClassic != 1 )
//					continue;
//				
//				if( cstrSection == "classic2" && pTorrents[ulKey].ucClassic != 2 )
//					continue;
//					
//				if( cstrSection == "classic3" && pTorrents[ulKey].ucClassic != 3 )
//					continue;
//				
//				if( cstrSection == "req" && !pTorrents[ulKey].bReq )
//					continue;
//				
//				if( cstrSection == "free" && pTorrents[ulKey].iFreeDown != 0 )
//					continue;
//			}
//					

//			// check seeded/unseeded torrents
//			if( !cstrMode.empty( ) )
//			{
//				// only count entries that match the mode
//				if( cstrMode == "Seeded" )
//				{
//					if( !pTorrents[ulKey].uiSeeders )
//						continue;
//				}
//				else if( cstrMode == "Unseeded"  )
//				{
//					if( pTorrents[ulKey].uiSeeders )
//						continue;
//				}
//				else if( ( cstrMode == "MyTorrents"  ) && ( pRequest->user.ucAccess & m_ucAccessViewTorrents ) )
//				{
//					if( pTorrents[ulKey].strUploader != pRequest->user.strLogin )
//						continue;
//				}
//			}

//			ulFound++;
//		}
		
		// search, filter and count messages
		pResponse->strContent += "<p class=\"search_filter\">\n";
		
		string strResult = string( );

		if( !vecSearch.empty( ) )
		{
			strResult += "<span class=\"search_results_alt\">" + gmapLANG_CFG["result_search"] + ": </span>";
			strResult += "<span class=\"filtered_by_search\">";
		 	strResult += UTIL_RemoveHTML( strSearch );
			strResult += "</span>\n";
		}
		
		if( !vecUploader.empty() )
		{
			if( !strResult.empty( ) )
				strResult += "<span class=\"search_results_alt\"> - </span>\n";

			strResult += "<span class=\"search_results_alt\">" + gmapLANG_CFG["result_search"] + gmapLANG_CFG["uploader"] + ": </span>";
			strResult += "<span class=\"filtered_by_search\">";
		 	strResult += UTIL_RemoveHTML( strUploader );
			strResult += "</span>\n";
		}
			
		if( !strMatch.empty() )
		{
			strResult += "<span class=\"search_results_alt\">" + gmapLANG_CFG["result_match"] + ": </span>";
			strResult += "<span class=\"filtered_by_search\">";
		 	strResult += gmapLANG_CFG["match_"+strMatch];
			strResult += "</span>\n";
		}

		if( !vecFilter.empty() )
		{
			if( !m_vecTags.empty( ) )
			{
				if( !strResult.empty( ) )
					strResult += "<span class=\"search_results_alt\"> - </span>\n";
				
				string strNameIndex = string( );
				string strTag = string( );
				
				strResult += "<span class=\"search_results_alt\">" + gmapLANG_CFG["result_tag"] + ": </span>";
				strResult += "<span class=\"filtered_by_tag\">";
				
				for( vector<string> :: iterator ulKey = vecFilter.begin( ); ulKey != vecFilter.end( ); ulKey++ )
				{
					for( vector< pair< string, string > > :: iterator ulTagKey = m_vecTags.begin( ); ulTagKey != m_vecTags.end( ); ulTagKey++ )
					{
						strNameIndex = (*ulTagKey).first;
						strTag = (*ulTagKey).second;

						if( (*ulKey).length( ) > 2 )
						{
							if( *ulKey == strNameIndex )
								strResult += UTIL_RemoveHTML( strTag );
						}
						else
						{
							if( *ulKey + "01" == strNameIndex )
								strResult += UTIL_RemoveHTML( strTag.substr( 0, strTag.find( ' ' ) ) );
						}

					}

					if( ulKey + 1 != vecFilter.end( ) )
						strResult += " &amp; ";
				}
				strResult += "</span>\n";
				
				if( bDefaultTag )
					strResult += "<input type=button value=\"" + gmapLANG_CFG["index_no_default_tag"] + "\" onClick=\"javascript: window.location='" + INDEX_HTML + "?notag=1';\">";
			}
		}
		
		if( !vecMedium.empty( ) )
		{
			if( !m_vecMediums.empty( ) )
			{
				if( !strResult.empty( ) )
					strResult += "<span class=\"search_results_alt\"> - </span>\n";
				strResult += "<span class=\"search_results_alt\">" + gmapLANG_CFG["result_medium"] + ": </span>";
				strResult += "<span class=\"filtered_by_medium\">";
				for( vector<string> :: iterator ulKey = vecMedium.begin( ); ulKey != vecMedium.end( ); ulKey++ )
				{
					for( vector< string > :: iterator ulTagKey = m_vecMediums.begin( ); ulTagKey != m_vecMediums.end( ); ulTagKey++ )
					{
						if( *ulKey == *ulTagKey )
							strResult += gmapLANG_CFG["medium_"+*ulTagKey];
					}

					if( ulKey + 1 != vecMedium.end( ) )
						strResult += " &amp; ";
				}
				strResult += "</span>\n";
			}
		}
		
		if( !vecQuality.empty( ) )
		{
			if( !m_vecQualities.empty( ) )
			{
				if( !strResult.empty( ) )
					strResult += "<span class=\"search_results_alt\"> - </span>\n";
				strResult += "<span class=\"search_results_alt\">" + gmapLANG_CFG["result_quality"] + ": </span>";
				strResult += "<span class=\"filtered_by_quality\">";
				for( vector<string> :: iterator ulKey = vecQuality.begin( ); ulKey != vecQuality.end( ); ulKey++ )
				{
					for( vector< string > :: iterator ulTagKey = m_vecQualities.begin( ); ulTagKey != m_vecQualities.end( ); ulTagKey++ )
					{
						if( *ulKey == *ulTagKey )
							strResult += gmapLANG_CFG["quality_"+*ulTagKey];
					}

					if( ulKey + 1 != vecQuality.end( ) )
						strResult += " &amp; ";
				}
				strResult += "</span>\n";
			}
		}
		
		if( !vecEncode.empty( ) )
		{
			if( !m_vecEncodes.empty( ) )
			{
				if( !strResult.empty( ) )
					strResult += "<span class=\"search_results_alt\"> - </span>\n";
				strResult += "<span class=\"search_results_alt\">" + gmapLANG_CFG["result_encode"] + ": </span>";
				strResult += "<span class=\"filtered_by_encode\">";
				for( vector<string> :: iterator ulKey = vecEncode.begin( ); ulKey != vecEncode.end( ); ulKey++ )
				{
					for( vector< string > :: iterator ulTagKey = m_vecEncodes.begin( ); ulTagKey != m_vecEncodes.end( ); ulTagKey++ )
					{
						if( *ulKey == *ulTagKey )
							strResult += gmapLANG_CFG["encode_"+*ulTagKey];
					}

					if( ulKey + 1 != vecEncode.end( ) )
						strResult += " &amp; ";
				}
				strResult += "</span>\n";
			}
		}
		
		if( !cstrMode.empty() )
		{
			if( !strResult.empty( ) )
				strResult += "<span class=\"search_results_alt\"> - </span>\n";
			strResult += "<span class=\"search_results_alt\">" + gmapLANG_CFG["result_subfilter"] + ": </span><span class=\"filtered_by_mode\">" + UTIL_RemoveHTML( gmapLANG_CFG["subfilter_" + UTIL_ToLower( cstrMode )] ) + "</span>\n";
		}
			
		pResponse->strContent += strResult;

		pResponse->strContent += "</p>\n\n";
		
		string :: size_type iCountGoesHere = string :: npos;
		
		iCountGoesHere = pResponse->strContent.size( );

		const string cstrPage( pRequest->mapParams["page"] );

		if( !cstrPage.empty( ) )
			ulStart = (unsigned long)atoi( cstrPage.c_str( ) ) * uiOverridePerPage;
			
		// for correct page numbers after searching
		bool bFound = false;

		unsigned long ulAdded = 0;
		unsigned long ulSkipped = 0;
		unsigned int ucTag = 0;
		unsigned char ucPercent = 0;			
		
		string strEngName = string( );
		string strChiName = string( );

		unsigned char ucMatchMethod = MATCH_METHOD_NONCASE_AND;

		if( strMatch == "or" )
			ucMatchMethod = MATCH_METHOD_NONCASE_OR;
		else if( strMatch == "eor" )
			ucMatchMethod = MATCH_METHOD_NONCASE_EQ;

		for( unsigned long ulKey = 0; ulKey < ulKeySize; ulKey++ )
		{
			if( !vecSearch.empty( ) && !UTIL_MatchVector( pTorrents[ulKey].strName, vecSearch, ucMatchMethod ) )
				continue;
			if( !vecUploader.empty( ) && !UTIL_MatchVector( pTorrents[ulKey].strUploader, vecUploader, ucMatchMethod ) )
				continue;
			
			if( !vecFilter.empty( ) )  
			{    
				// only display entries that match the filter  
				bool bFoundKey = false;
				
				for( vector<string> :: iterator ulVecKey = vecFilter.begin( ); ulVecKey != vecFilter.end( ); ulVecKey++ )
				{
					if( (*ulVecKey).length( ) > 2 )
					{
						if( pTorrents[ulKey].strTag == *ulVecKey )
						{
							bFoundKey = true;
							break;
						}
					}
					else
					{
						if( pTorrents[ulKey].strTag.substr( 0, 2 - pTorrents[ulKey].strTag.length( ) % 2 ) == *ulVecKey )
						{
							bFoundKey = true;
							break;
						}
					}
				}
				if( bFoundKey == false )
					continue;
			}
			
			if( !UTIL_MatchVector( pTorrents[ulKey].strName, vecMedium, MATCH_METHOD_NONCASE_OR ) )
				continue;
			if( !UTIL_MatchVector( pTorrents[ulKey].strName, vecQuality, MATCH_METHOD_NONCASE_OR ) )
				continue;
			if( !UTIL_MatchVector( pTorrents[ulKey].strName, vecEncode, MATCH_METHOD_NONCASE_OR ) )
				continue;
				
//			if( !vecMedium.empty( ) )
//			{
//				// only count entries that match the search
//				
//				bool bFoundKey = false;
//				
//				for( vector<string> :: iterator ulVecKey = vecMedium.begin( ); ulVecKey != vecMedium.end( ); ulVecKey++ )
//				{
//					if( pTorrents[ulKey].strLowerName.find( *ulVecKey ) != string :: npos )
//					{
//						bFoundKey = true;
//						break;
//					}
//				}
//				if( bFoundKey == false )
//					continue;
//			}
//			
//			if( !vecQuality.empty( ) )
//			{
//				// only count entries that match the search
//				
//				bool bFoundKey = false;
//				
//				for( vector<string> :: iterator ulVecKey = vecQuality.begin( ); ulVecKey != vecQuality.end( ); ulVecKey++ )
//				{
//					if( pTorrents[ulKey].strLowerName.find( *ulVecKey ) != string :: npos )
//					{
//						bFoundKey = true;
//						break;
//					}
//				}
//				if( bFoundKey == false )
//					continue;
//			}
//			
//			if( !vecEncode.empty( ) )
//			{
//				// only count entries that match the search
//				
//				bool bFoundKey = false;
//				
//				for( vector<string> :: iterator ulVecKey = vecEncode.begin( ); ulVecKey != vecEncode.end( ); ulVecKey++ )
//				{
//					if( pTorrents[ulKey].strLowerName.find( *ulVecKey ) != string :: npos )
//					{
//						bFoundKey = true;
//						break;
//					}
//				}
//				if( bFoundKey == false )
//					continue;
//			}

//			if( !vecSearch.empty( ) )
//			{
//				// only display entries that match the search   
//				bool bFoundKey = true;
//				
//				for( vector<string> :: iterator ulVecKey = vecSearch.begin( ); ulVecKey != vecSearch.end( ); ulVecKey++ )
//				{
//					if( pTorrents[ulKey].strLowerName.find( UTIL_ToLower( *ulVecKey ) ) == string :: npos )
//					{
//						bFoundKey = false;
//						break;
//					}
//				}
//				if( bFoundKey == false )
//					continue;
//			}
//			
//			if( !vecUploader.empty( ) )
//			{
//				// only display entries that match the search   
//				bool bFoundKey = true;
//				
//				for( vector<string> :: iterator ulVecKey = vecUploader.begin( ); ulVecKey != vecUploader.end( ); ulVecKey++ )
//				{
//					if( UTIL_ToLower( pTorrents[ulKey].strUploader ).find( UTIL_ToLower( *ulVecKey ) ) == string :: npos )
//					{
//						bFoundKey = false;
//						break;
//					}
//				}
//				if( bFoundKey == false )
//					continue;
//			}

			// check uploader's torrents
//			if( !strUploader.empty( ) )
//			{
//				if( UTIL_ToLower( pTorrents[ulKey].strUploader ).find( cstrLowerUploader ) == string :: npos )
//					continue;
//			}
			
			if( !cstrDay.empty( ) )
			{
				struct tm time_tm;
				int64 year, month, day, hour, minute, second;
				float passed;
				sscanf( pTorrents[ulKey].strAdded.c_str( ), "%d-%d-%d %d:%d:%d",&year,&month,&day,&hour,&minute,&second );
				time_tm.tm_year = year-1900;
				time_tm.tm_mon = month-1;
				time_tm.tm_mday = day;
				time_tm.tm_hour = hour;
				time_tm.tm_min = minute;
				time_tm.tm_sec = second;
				passed = difftime(now_t, mktime(&time_tm));
				if( passed > ( (time_t)( atoi( cstrDay.c_str( ) ) * 3600 * 24 ) ) )
					continue;
			}
			
			if( !cstrSection.empty( ) )
			{
				if( cstrSection == "hot" && pTorrents[ulKey].uiSeeders + pTorrents[ulKey].uiLeechers < CFG_GetInt( "bnbt_hot_count" ,20 ) )
					continue;
					
				if( cstrSection == "classic1" && pTorrents[ulKey].ucClassic != 1 )
					continue;
				
				if( cstrSection == "classic2" && pTorrents[ulKey].ucClassic != 2 )
					continue;
					
				if( cstrSection == "classic3" && pTorrents[ulKey].ucClassic != 3 )
					continue;
				
				if( cstrSection == "req" && !pTorrents[ulKey].bReq )
					continue;
				
				if( cstrSection == "free" && pTorrents[ulKey].iFreeDown != 0 )
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
// 				else if(( cstrMode == "MyTorrents"  ) && ( pRequest->user.ucAccess & ACCESS_UPLOAD ) )
				else if( ( cstrMode == "MyTorrents" ) && ( pRequest->user.ucAccess & m_ucAccessViewTorrents ) )
				{
					if( pTorrents[ulKey].strUploaderID != pRequest->user.strUID )
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
					
					vector< pair< string, string > > vecParams;
					vecParams.reserve(64);
					string strJoined = string( );
			
					vecParams.push_back( pair<string, string>( string( "per_page" ), cstrPerPage ) );
					vecParams.push_back( pair<string, string>( string( "search" ), strSearch ) );
					vecParams.push_back( pair<string, string>( string( "match" ), strMatch ) );
					vecParams.push_back( pair<string, string>( string( "tag" ), strFilter ) );
					vecParams.push_back( pair<string, string>( string( "medium" ), strMedium ) );
					vecParams.push_back( pair<string, string>( string( "quality" ), strQuality ) );
					vecParams.push_back( pair<string, string>( string( "encode" ), strEncode ) );
					vecParams.push_back( pair<string, string>( string( "uploader" ), strUploader ) );
					vecParams.push_back( pair<string, string>( string( "mode" ), cstrMode ) );
					vecParams.push_back( pair<string, string>( string( "section" ), cstrSection ) );
					vecParams.push_back( pair<string, string>( string( "notop" ), cstrNoTop ) );
					vecParams.push_back( pair<string, string>( string( "day" ), cstrDay ) );
					vecParams.push_back( pair<string, string>( string( "notag" ), cstrNoTag ) );
					
					strJoined = UTIL_RemoveHTML( UTIL_HTMLJoin( vecParams, string( "&" ), string( "&" ), string( "=" ) ) );
					
					pResponse->strContent += "<table class=\"torrent_table\" summary=\"files\">\n";

					pResponse->strContent += "<tr>\n";

					// <th> tag

					if( !m_vecTags.empty( ) )     
					{
						pResponse->strContent += "<th class=\"tag\" id=\"tagheader\">" + gmapLANG_CFG["tag"];

						pResponse->strContent += "</th>\n";
					}         

					// <th> seeders

					pResponse->strContent += "<th class=\"number\" id=\"seedersheader\">";

					if( m_bSort )
					{
						pResponse->strContent += "<a class=\"table_header\" href=\"" + RESPONSE_STR_INDEX_HTML + "?sort=";

						if( strSort == SORTSTR_DCOMPLETE )
							pResponse->strContent += SORTSTR_ACOMPLETE;
						else
							pResponse->strContent += SORTSTR_DCOMPLETE;
				
						pResponse->strContent += strJoined;
						
						pResponse->strContent += "\">";
					}
					
					if( !gmapLANG_CFG["seeders_icon"].empty( ) )
						pResponse->strContent += gmapLANG_CFG["seeders_icon"];
					else
						pResponse->strContent += gmapLANG_CFG["seeders"];
					
					if( m_bSort )
						pResponse->strContent += "</a>";
					pResponse->strContent += "</th>\n";

					// <th> leechers

					pResponse->strContent += "<th class=\"number\" id=\"leechersheader\">";

					if( m_bSort )
					{
						pResponse->strContent += "<a class=\"table_header\" href=\"" + RESPONSE_STR_INDEX_HTML + "?sort=";

						if( strSort == SORTSTR_DINCOMPLETE )
							pResponse->strContent += SORTSTR_AINCOMPLETE;
						else
							pResponse->strContent += SORTSTR_DINCOMPLETE;

						pResponse->strContent += strJoined;
						
						pResponse->strContent += "\">";
					}

					if( !gmapLANG_CFG["leechers_icon"].empty( ) )
						pResponse->strContent += gmapLANG_CFG["leechers_icon"];
					else
						pResponse->strContent += gmapLANG_CFG["leechers"];
					
					if( m_bSort )
						pResponse->strContent += "</a>";
					pResponse->strContent += "</th>\n";

					// <th> completed

					if( m_bShowCompleted )
					{
						pResponse->strContent += "<th class=\"number\" id=\"completedheader\">";

						if( m_bSort )
						{
							pResponse->strContent += "<a class=\"table_header\" href=\"" + RESPONSE_STR_INDEX_HTML + "?sort=";

							if( strSort == SORTSTR_DCOMPLETED )
								pResponse->strContent += SORTSTR_ACOMPLETED;
							else
								pResponse->strContent += SORTSTR_DCOMPLETED;

							pResponse->strContent += strJoined;
							
							pResponse->strContent += "\">";
						}
						
						if( !gmapLANG_CFG["completed_icon"].empty( ) )
							pResponse->strContent += gmapLANG_CFG["completed_icon"];
						else
							pResponse->strContent += gmapLANG_CFG["completed"];
					
						if( m_bSort )
							pResponse->strContent += "</a>";

						pResponse->strContent += "</th>\n";
					}

					// <th> name

					pResponse->strContent += "<th class=\"name\" id=\"nameheader\" colspan=2>";

					if( m_bSort )
					{
						pResponse->strContent += "<a class=\"table_header\" href=\"" + RESPONSE_STR_INDEX_HTML + "?sort=";

						if( strSort == SORTSTR_ANAME )
							pResponse->strContent += SORTSTR_DNAME;
						else
							pResponse->strContent += SORTSTR_ANAME;

						pResponse->strContent += strJoined;
						
						pResponse->strContent += "\">";
					}
					
					pResponse->strContent += gmapLANG_CFG["name"];
				
					if( m_bSort )
						pResponse->strContent += "</a>";

					pResponse->strContent += "</th>\n";

					// <th> comments

					if( m_bAllowComments )
					{
						pResponse->strContent += "<th class=\"number\" id=\"commentsheader\">";
						
						if( !gmapLANG_CFG["comments_icon"].empty( ) )
							pResponse->strContent += gmapLANG_CFG["comments_icon"];
						else
							pResponse->strContent += gmapLANG_CFG["comments"];

						pResponse->strContent += "</th>\n";
					}

					// <th> added

					if( m_bShowAdded_Index )
					{
						pResponse->strContent += "<th class=\"date\" id=\"addedheader\">";

						if( m_bSort )
						{
							pResponse->strContent += "<a class=\"table_header\" href=\"" + RESPONSE_STR_INDEX_HTML + "?sort=";

							if( strSort == SORTSTR_DADDED || strSort.empty( ) )
								pResponse->strContent += SORTSTR_AADDED;
							else
								pResponse->strContent += SORTSTR_DADDED;

							pResponse->strContent += strJoined;
							
							pResponse->strContent += "\">";
						}
						
						pResponse->strContent += gmapLANG_CFG["added"];
					
						if( m_bSort )
							pResponse->strContent += "</a>";

						pResponse->strContent += "</th>\n";
					}

					// <th> size

					if( m_bShowSize )
					{
						pResponse->strContent += "<th class=\"bytes\" id=\"sizeheader\">";


						if( m_bSort )
						{
							pResponse->strContent += "<a class=\"table_header\" href=\"" + RESPONSE_STR_INDEX_HTML + "?sort=";

							if( strSort == SORTSTR_DSIZE )
								pResponse->strContent += SORTSTR_ASIZE;
							else
								pResponse->strContent += SORTSTR_DSIZE;

							pResponse->strContent += strJoined;
							
							pResponse->strContent += "\">";
						}
						
						pResponse->strContent += gmapLANG_CFG["size"];
					
						if( m_bSort )
							pResponse->strContent += "</a>";

						pResponse->strContent += "</th>\n";
					}

					// <th> files

					if( m_bShowNumFiles )
					{
						// Modified by =Xotic=

						pResponse->strContent += "<th class=\"number\" id=\"filesheader\">";

						if( m_bSort )
						{
							pResponse->strContent += "<a class=\"table_header\" href=\"" + RESPONSE_STR_INDEX_HTML + "?sort=";

							if( strSort == SORTSTR_DFILES )
								pResponse->strContent += SORTSTR_AFILES;
							else
								pResponse->strContent += SORTSTR_DFILES;

							pResponse->strContent += strJoined;
							
							pResponse->strContent += "\">";
						}
						
						pResponse->strContent += gmapLANG_CFG["files"];
					
						if( m_bSort )
							pResponse->strContent += "</a>";

						pResponse->strContent += "</th>\n";
					}

					// <th> transferred

					if( m_bShowTransferred )
					{
						pResponse->strContent += "<th class=\"bytes\" id=\"transferredheader\">";

						if( m_bSort )
						{
							pResponse->strContent += "<a class=\"table_header\" href=\"" + RESPONSE_STR_INDEX_HTML + "?sort=";
							
							if( strSort == SORTSTR_DTRANSFERRED )
								pResponse->strContent += SORTSTR_ATRANSFERRED;
							else
								pResponse->strContent += SORTSTR_DTRANSFERRED;

							pResponse->strContent += strJoined;
							
							pResponse->strContent += "\">";
						}
						
						pResponse->strContent += gmapLANG_CFG["transferred"];
					
						if( m_bSort )
							pResponse->strContent += "</a>";

						pResponse->strContent += "</th>\n";
					}

					// <th> min left

// 					if( m_bShowMinLeft )
// 					{
// 						if( m_bShowLeftAsProgress )
// 							pResponse->strContent += "<th class=\"percent\" id=\"minprogressheader\">" + gmapLANG_CFG["min_progress"] + "</th>\n";
// 						else
// 							pResponse->strContent += "<th class=\"percent\" id=\"minleftheader\">" + gmapLANG_CFG["min_left"] + "</th>\n";
// 					}

					// <th> average left

// 					if( m_bShowAverageLeft )
// 					{
// // 							if( m_pAllowed && m_bShowLeftAsProgress )
// // 								pResponse->strContent += "<th class=\"percent\" id=\"avgprogressheader\">" + gmapLANG_CFG["avg_progress"];
// // 							else
// // 								pResponse->strContent += "<th class=\"percent\" id=\"avgleftheader\">" + gmapLANG_CFG["avg_left"];
// 						if( m_bShowLeftAsProgress )
// 							pResponse->strContent += "<th class=\"percent\" id=\"avgprogressheader\">";
// 						else
// 							pResponse->strContent += "<th class=\"percent\" id=\"avgleftheader\">";
// 
// 						if( m_bSort )
// 						{
// // 								pResponse->strContent += "<br><div><a class=\"sort\" title=\"" + gmapLANG_CFG["sort_avgleft_ascending"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?sort=" + SORTSTR_AAVGLEFT;
// 							
// 							pResponse->strContent += "<a class=\"table_header\" href=\"" + RESPONSE_STR_INDEX_HTML + "?sort=";
// 							
// 							if( strSort == SORTSTR_DAVGLEFT )
// 								pResponse->strContent += SORTSTR_AAVGLEFT;
// 							else
// 								pResponse->strContent += SORTSTR_DAVGLEFT;
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
// 							if( !strUploader.empty( ) )
// 								pResponse->strContent += "&amp;uploader=" + strUploader;
// 							
// 							if( !cstrSection.empty( ) )
// 								pResponse->strContent += "&amp;sect=" + cstrSection;
// 
// 							if( !cstrMode.empty( ) )
// 								pResponse->strContent += "&amp;mode=" + cstrMode;
// 							
// 							pResponse->strContent += "\">";
// 
// // 								pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a class=\"sort\" title=\"" + gmapLANG_CFG["sort_avgleft_descending"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?sort=" + SORTSTR_DAVGLEFT;
// // 

// // 								if( !cstrPerPage.empty( ) )
// // 									pResponse->strContent += "&amp;per_page=" + cstrPerPage;
// // 
// // 								if( !strSearch.empty( ) )
// // 									pResponse->strContent += "&amp;search=" + strSearchResp;
// // 
// // 								if( !strFilter.empty( ) )
// // 									pResponse->strContent += "&amp;filter=" + UTIL_StringToEscaped( strFilter );
// // 								if( !strUploader.empty( ) )
// //      		       			 	                        pResponse->strContent += "&amp;uploader=" + strUploader;
// // 
// // 								if( !cstrMode.empty( ) )
// // 									pResponse->strContent += "&amp;mode=" + cstrMode;
// // 
// // 								pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a></div>";
// 						}
// 						
// 						if( m_bShowLeftAsProgress )
// 							pResponse->strContent += gmapLANG_CFG["avg_progress"];
// 						else
// 							pResponse->strContent += gmapLANG_CFG["avg_left"];
// 					
// 						if( m_bSort )
// 							pResponse->strContent += "</a>";
// 
// 						pResponse->strContent += "</th>\n";
// 					}

// 					// <th> maxi left
// 
// 					if( m_bShowMaxiLeft )
// 					{
// 						if( m_bShowLeftAsProgress )
// 							pResponse->strContent += "<th class=\"percent\" id=\"maxprogressheader\">" + gmapLANG_CFG["max_progress"] + "</th>\n";
// 						else
// 							pResponse->strContent += "<th class=\"percent\" id=\"maxleftheader\">" + gmapLANG_CFG["max_left"] + "</th>\n";
// 					}

					// <th> uploader

					if( m_bShowUploader )
					{
						pResponse->strContent += "<th class=\"uploader\" id=\"uploaderheader\">";
						
						if( m_bSort )
						{
							pResponse->strContent += "<a class=\"table_header\" href=\"" + RESPONSE_STR_INDEX_HTML + "?sort=";
							
							if( strSort == SORTSTR_AUPLOADER )
								pResponse->strContent += SORTSTR_DUPLOADER;
							else
								pResponse->strContent += SORTSTR_AUPLOADER;

							pResponse->strContent += strJoined;
							
							pResponse->strContent += "\">";
						}
						
						pResponse->strContent += gmapLANG_CFG["uploader"];
					
						if( m_bSort )
							pResponse->strContent += "</a>";

						pResponse->strContent += "</th>\n";
					}

					// <th> ip

					if( m_bShowIP && ( pRequest->user.ucAccess & m_ucAccessShowIP ) )
					{
						pResponse->strContent += "<th class=\"ip\" id=\"ipheader\">";
						if( m_bSort )
						{
							pResponse->strContent += "<a class=\"table_header\" href=\"" + RESPONSE_STR_INDEX_HTML + "?sort=";
							
							if( strSort == SORTSTR_DIP )
								pResponse->strContent += SORTSTR_AIP;
							else
								pResponse->strContent += SORTSTR_DIP;

							pResponse->strContent += strJoined;
							
							pResponse->strContent += "\">";
						}
						
						pResponse->strContent += gmapLANG_CFG["ip"];
					
						if( m_bSort )
							pResponse->strContent += "</a>";

						pResponse->strContent += "</th>\n";
					}

					if( ( pRequest->user.ucAccess & m_ucAccessEditTorrents ) || ( pRequest->user.ucAccess & m_ucAccessDelTorrents ) )
					{
						pResponse->strContent += "<th id=\"adminheader\">" + gmapLANG_CFG["admin"] + "</th>\n";   
					}

					pResponse->strContent += "</tr>\n";

					// signal table created
					bFound = true;
				}

				if( ulSkipped == ulStart )
				{
					// output table rows

					if( pTorrents[ulKey].ucTop > 0 && !bNoTop )
					{
						if( pTorrents[ulKey].ucTop > 2 )
							pResponse->strContent += "<tr class=\"top_global\">\n";
						else if( pTorrents[ulKey].ucTop > 1 )
							pResponse->strContent += "<tr class=\"top\">\n";
						else
							pResponse->strContent += "<tr class=\"top_float\">\n";
					}
					else
//						if( pTorrents[ulKey].bHL )
//							pResponse->strContent += "<tr class=\"hl\">\n";
//						else
							pResponse->strContent += "<tr class=\"normal\">\n";

					// <td> tag 

					if( !m_vecTags.empty( ) )
					{
						pResponse->strContent += "<td class=\"tag\">";

						string strNameIndex = string( );
						string strTag = string( );
						
						vector< pair< string, string > > :: iterator it2 = m_vecTagsMouse.begin( );

						for( vector< pair< string, string > > :: iterator it1 = m_vecTags.begin( ); it1 != m_vecTags.end( ); it1++ )
						{
							strNameIndex = (*it1).first;
							strTag = (*it1).second;
							if( strNameIndex == pTorrents[ulKey].strTag )
							{
								pResponse->strContent += "<a class=\"index_filter\" title=\"" + UTIL_RemoveHTML( gmapLANG_CFG["filter_by"] + ": " + strTag ) + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?tag=" + strNameIndex;
								
								vector< pair< string, string > > vecParams;
								vecParams.reserve(64);
				
								vecParams.push_back( pair<string, string>( string( "per_page" ), cstrPerPage ) );
								vecParams.push_back( pair<string, string>( string( "sort" ), strSort ) );
								vecParams.push_back( pair<string, string>( string( "search" ), strSearch ) );
								vecParams.push_back( pair<string, string>( string( "match" ), strMatch ) );
								vecParams.push_back( pair<string, string>( string( "uploader" ), strUploader ) );
								vecParams.push_back( pair<string, string>( string( "medium" ), strMedium ) );
								vecParams.push_back( pair<string, string>( string( "quality" ), strQuality ) );
								vecParams.push_back( pair<string, string>( string( "encode" ), strEncode ) );
								vecParams.push_back( pair<string, string>( string( "mode" ), cstrMode ) );
								vecParams.push_back( pair<string, string>( string( "section" ), cstrSection ) );
								vecParams.push_back( pair<string, string>( string( "notop" ), cstrNoTop ) );
								vecParams.push_back( pair<string, string>( string( "day" ), cstrDay ) );
								vecParams.push_back( pair<string, string>( string( "notag" ), cstrNoTag ) );
				
								pResponse->strContent += UTIL_RemoveHTML( UTIL_HTMLJoin( vecParams, string( "&" ), string( "&" ), string( "=" ) ) );

								pResponse->strContent += "\">";
								
								// Sets a TITLE parameter for each Tag Image, so that a TOOLTIP box will popup when
								// the user's mouse pointer hovers over the Tag Image.

								if( !(*it2).second.empty( ) )
									pResponse->strContent += "<img class=\"tag\" src=\"" + (*it2).second + "\" alt=\"[" + UTIL_RemoveHTML( gmapLANG_CFG["filter_by"] + ": "+ strTag ) + "]\" title=\"" + UTIL_RemoveHTML( gmapLANG_CFG["filter_by"] + ": " + strTag ) + "\" name=\"" + CAtomInt( ulAdded ).toString( ) + "xbnbt_tag" + strNameIndex + "\">";
								else
									pResponse->strContent += strTag;

								pResponse->strContent += "</a>";
								break;
							}
							it2++;

						}

						pResponse->strContent += "</td>\n";
					}
					
					vector< pair< string, string > > vecParams;
					vecParams.reserve(64);
					string strReturn = string( );
					string strJoined = string( );
			
					vecParams.push_back( pair<string, string>( string( "per_page" ), cstrPerPage ) );
					vecParams.push_back( pair<string, string>( string( "sort" ), strSort ) );
					vecParams.push_back( pair<string, string>( string( "search" ), strSearch ) );
					vecParams.push_back( pair<string, string>( string( "match" ), strMatch ) );
					vecParams.push_back( pair<string, string>( string( "tag" ), strFilter ) );
					vecParams.push_back( pair<string, string>( string( "medium" ), strMedium ) );
					vecParams.push_back( pair<string, string>( string( "quality" ), strQuality ) );
					vecParams.push_back( pair<string, string>( string( "encode" ), strEncode ) );
					vecParams.push_back( pair<string, string>( string( "uploader" ), strUploader ) );
					vecParams.push_back( pair<string, string>( string( "mode" ), cstrMode ) );
					vecParams.push_back( pair<string, string>( string( "section" ), cstrSection ) );
					vecParams.push_back( pair<string, string>( string( "notop" ), cstrNoTop ) );
					vecParams.push_back( pair<string, string>( string( "day" ), cstrDay ) );
					vecParams.push_back( pair<string, string>( string( "notag" ), cstrNoTag ) );
					vecParams.push_back( pair<string, string>( string( "page" ), cstrPage ) );
			
					strReturn = RESPONSE_STR_INDEX_HTML + UTIL_HTMLJoin( vecParams, string( "?" ), string( "&" ), string( "=" ) );
					
					vecParams.clear( );
					vecParams.push_back( pair<string, string>( string( "id" ), pTorrents[ulKey].strID ) );
					vecParams.push_back( pair<string, string>( string( "return" ), strReturn ) );
					
					strJoined = UTIL_RemoveHTML( UTIL_HTMLJoin( vecParams, string( "?" ), string( "&" ), string( "=" ) ) );
					
					// <td> seeders
					
					pResponse->strContent += "<td class=\"number_";

					if( pTorrents[ulKey].uiSeeders == 0 )
						pResponse->strContent += "red\">";
					else if( pTorrents[ulKey].uiSeeders < 5 )
						pResponse->strContent += "green\">";
					else
						pResponse->strContent += "blue\">";
					
					if( pTorrents[ulKey].uiSeeders > 0 )
					{
//						if( pTorrents[ulKey].uiSeeders < 5 )
//							pResponse->strContent += "<a class=\"number6_green\"";
//						else
//							pResponse->strContent += "<a class=\"number6_blue\"";
						if( pTorrents[ulKey].uiSeeders < 5 )
							pResponse->strContent += "<a class=\"green\"";
						else
							pResponse->strContent += "<a class=\"blue\"";
					
						pResponse->strContent += " href=\"" + RESPONSE_STR_STATS_HTML + strJoined;
						pResponse->strContent += "&amp;show=active#seeders\">";
						pResponse->strContent += CAtomInt( pTorrents[ulKey].uiSeeders ).toString( ) + "</a>";
					}
					else
						pResponse->strContent += CAtomInt( pTorrents[ulKey].uiSeeders ).toString( );
//						pResponse->strContent += "<span class=\"red\">" + CAtomInt( pTorrents[ulKey].uiSeeders ).toString( ) + "</span>";
//					pResponse->strContent += "<br>";
//					if( pTorrents[ulKey].uiSeeders6 > 0 )
//					{
//						pResponse->strContent += "<span class=\"number6\">|</span><span title=\"" + gmapLANG_CFG["seeders_ipv6"] + "\" class=\"number6_";
////						if( pTorrents[ulKey].uiSeeders6 == 0 )
////							pResponse->strContent += "red\">";
//						if( pTorrents[ulKey].uiSeeders6 < 5 )
//							pResponse->strContent += "green\">";
//						else
//							pResponse->strContent += "blue\">";
//						pResponse->strContent += CAtomInt( pTorrents[ulKey].uiSeeders6 ).toString( ) + "</span>";
//					}
					pResponse->strContent += "</td>\n";

					// <td> leechers

					pResponse->strContent += "<td class=\"number_";

					if( pTorrents[ulKey].uiLeechers == 0 )
						pResponse->strContent += "red\">";
					else if( pTorrents[ulKey].uiLeechers < 5 )
						pResponse->strContent += "green\">";
					else
						pResponse->strContent += "blue\">";
					
					if( pTorrents[ulKey].uiLeechers > 0 )
					{
						if( pTorrents[ulKey].uiLeechers < 5 )
							pResponse->strContent += "<a class=\"green\"";
						else
							pResponse->strContent += "<a class=\"blue\"";
					
						pResponse->strContent += " href=\"" + RESPONSE_STR_STATS_HTML + strJoined;
						pResponse->strContent += "&amp;show=active#leechers\">";
						pResponse->strContent += CAtomInt( pTorrents[ulKey].uiLeechers ).toString( ) + "</a>";
					}
					else
						pResponse->strContent += CAtomInt( pTorrents[ulKey].uiLeechers ).toString( );
//					pResponse->strContent += "<br>";
//					if( pTorrents[ulKey].uiLeechers6 > 0 )

//					{
//						pResponse->strContent += "<span class=\"number6\">|</span><span title=\"" + gmapLANG_CFG["leechers_ipv6"] + "\" class=\"number6_";
////						if( pTorrents[ulKey].uiLeechers6 == 0 )
////							pResponse->strContent += "red\">";
//						if( pTorrents[ulKey].uiLeechers6 < 5 )
//							pResponse->strContent += "green\">";
//						else
//							pResponse->strContent += "blue\">";
//						pResponse->strContent += CAtomInt( pTorrents[ulKey].uiLeechers6 ).toString( ) + "</span>";
//					}
					pResponse->strContent += "</td>\n";

					// <td> completed

					if( m_bShowCompleted )
					{
						pResponse->strContent += "<td class=\"number\">";
						if( pTorrents[ulKey].ulCompleted > 0 )
						{
							pResponse->strContent += "<a class=\"number\" href=\"" + RESPONSE_STR_STATS_HTML + strJoined;
							pResponse->strContent += "&amp;show=completes#completes\">";
						}
						pResponse->strContent += CAtomInt( pTorrents[ulKey].ulCompleted ).toString( );
						if( pTorrents[ulKey].ulCompleted > 0 )
							pResponse->strContent += "</a>";
//						pResponse->strContent += "<br><span class=\"number6\">-</span></td>\n";
					}

					// <td> name


					struct tm time_tm;
					int64 year, month, day, hour, minute, second, day_left = -1, hour_left = -1, minute_left = -1;
					sscanf( pTorrents[ulKey].strAdded.c_str( ), "%d-%d-%d %d:%d:%d",&year,&month,&day,&hour,&minute,&second );
					time_tm.tm_year = year-1900;
					time_tm.tm_mon = month-1;
					time_tm.tm_mday = day;
					time_tm.tm_hour = hour;
					time_tm.tm_min = minute;
					time_tm.tm_sec = second;
					
					time_t tTimeFree = pTorrents[ulKey].iFreeTo - now_t;
					time_t tTimeAdded = mktime(&time_tm);
					
					if( tTimeFree > 0 )
					{
						tTimeFree += 60;
						tTimeFree /= 60;
						minute_left =  tTimeFree % 60;
						tTimeFree /= 60;
						hour_left =  tTimeFree % 24;
						tTimeFree /= 24;
						day_left =  tTimeFree;
					}

//					if( pTorrents[ulKey].iFreeTo > now_t )
//					{
//						day_left = ( pTorrents[ulKey].iFreeTo - now_t + 60 ) / 86400;
//						hour_left = ( pTorrents[ulKey].iFreeTo - now_t + 60 ) % 86400 / 3600;
//						minute_left = ( pTorrents[ulKey].iFreeTo - now_t + 60 ) % 3600 / 60;
//					}

					strEngName.erase( );
					strChiName.erase( );
					pResponse->strContent += "<td class=\"name\">";
					UTIL_StripName( pTorrents[ulKey].strName.c_str( ), strEngName, strChiName );
					if( !pRequest->user.strUID.empty( ) && ( int64 )tTimeAdded > last_time )
						pResponse->strContent += "<span class=\"new\">(" + gmapLANG_CFG["new"] + ") </span>";
					if( pTorrents[ulKey].bReq )
						pResponse->strContent += "<span class=\"req\">[" + gmapLANG_CFG["section_reqseeders"] + "] </span>";
					if( m_bShowStats )
					{
						if( pTorrents[ulKey].ucTop > 0 && !bNoTop )
							pResponse->strContent += "<span class=\"top\">" + gmapLANG_CFG["top_level_"+CAtomInt( pTorrents[ulKey].ucTop ).toString( )] + ": </span>";
						if( pTorrents[ulKey].iFreeDown == 0 )
							pResponse->strContent += "<a class=\"stats_free\" title=\"";
						else if( pTorrents[ulKey].ucTop > 0 && !bNoTop )
							pResponse->strContent += "<a class=\"stats_top\" title=\"";
						else if( pTorrents[ulKey].ucClassic == 1 )
							pResponse->strContent += "<a class=\"classic_level_1\" title=\"";
						else
							pResponse->strContent += "<a class=\"stats\" title=\"";
						
						pResponse->strContent += gmapLANG_CFG["name"] + ": " + UTIL_RemoveHTML( pTorrents[ulKey].strName ) + "\" href=\"" + RESPONSE_STR_STATS_HTML + "?id=" + pTorrents[ulKey].strID;
						pResponse->strContent += "\">";
					
						pResponse->strContent += UTIL_RemoveHTML( strEngName );
						if( pTorrents[ulKey].ucClassic > 1 )
							pResponse->strContent += " [<span class=\"classic_level_" + CAtomInt( pTorrents[ulKey].ucClassic ).toString( ) + "\">" + gmapLANG_CFG["classic_level_"+CAtomInt(pTorrents[ulKey].ucClassic).toString( )] + "</span>]";
						if( pTorrents[ulKey].uiSeeders + pTorrents[ulKey].uiLeechers >= CFG_GetInt( "bnbt_hot_count" ,20 ) )
							pResponse->strContent += " <span class=\"hot\">(" + gmapLANG_CFG["hot"] + ")</span>";
						if( !strChiName.empty( ) )
							pResponse->strContent += "<br>" + UTIL_RemoveHTML( strChiName );
						pResponse->strContent += "</a>";
					}
					else
					{
						if( pTorrents[ulKey].ucTop > 0 )
							pResponse->strContent += gmapLANG_CFG["top_level_"+CAtomInt( pTorrents[ulKey].ucTop ).toString( )] + ": ";
						pResponse->strContent += UTIL_RemoveHTML( strEngName );
						if( pTorrents[ulKey].uiSeeders + pTorrents[ulKey].uiLeechers >= CFG_GetInt( "bnbt_hot_count" ,20 ) )
							pResponse->strContent += "<span class=\"hot\"> " + gmapLANG_CFG["hot"] + "</span>";
						if( !strChiName.empty( ) )
							pResponse->strContent += "<br>" + UTIL_RemoveHTML( strChiName );
					}
					
					if( pTorrents[ulKey].iFreeDown != 100 || pTorrents[ulKey].iFreeUp != 100 )
					{
						if( pTorrents[ulKey].iFreeDown != 100 )
						{
							if( pTorrents[ulKey].iDefaultDown == 0 && !( bFreeGlobal && iFreeDownGlobal == 0 ) )
								pResponse->strContent += "<span class=\"free_down_free\" title=\"" + gmapLANG_CFG["free_down_free"] + "\">" + gmapLANG_CFG["free_down_free_short"] + "</span>";
							else if( pTorrents[ulKey].iFreeDown > 0 )
								pResponse->strContent += "<span class=\"free_down\" title=\"" + UTIL_Xsprintf( gmapLANG_CFG["free_down"].c_str( ), CAtomInt( pTorrents[ulKey].iFreeDown ).toString( ).c_str( ) ) + "\">" + UTIL_Xsprintf( gmapLANG_CFG["free_down_short"].c_str( ), CAtomInt( pTorrents[ulKey].iFreeDown ).toString( ).c_str( ) )+ "</span>";
						}
						if( pTorrents[ulKey].iFreeUp != 100 )
							pResponse->strContent += "<span class=\"free_up\" title=\"" + UTIL_Xsprintf( gmapLANG_CFG["free_up"].c_str( ), CAtomInt( pTorrents[ulKey].iFreeUp ).toString( ).c_str( ) ) + "\"> " + UTIL_Xsprintf( gmapLANG_CFG["free_up_short"].c_str( ), CAtomInt( pTorrents[ulKey].iFreeUp ).toString( ).c_str( ) )+ "</span>";
						if( day_left >= 0 && ( pTorrents[ulKey].iDefaultDown > pTorrents[ulKey].iFreeDown || pTorrents[ulKey].iDefaultUp < pTorrents[ulKey].iFreeUp ) )
						{
							pResponse->strContent += "<span class=\"free_recover\" title=\"" + gmapLANG_CFG["free_recover"];
							pResponse->strContent += UTIL_Xsprintf( gmapLANG_CFG["free_down"].c_str( ), CAtomInt( pTorrents[ulKey].iDefaultDown ).toString( ).c_str( ) ) + UTIL_Xsprintf( gmapLANG_CFG["free_up"].c_str( ), CAtomInt( pTorrents[ulKey].iDefaultUp ).toString( ).c_str( ) ) + "\">";
							if( day_left > 0 )
								pResponse->strContent += UTIL_Xsprintf( gmapLANG_CFG["free_day_left"].c_str( ), CAtomInt( day_left ).toString( ).c_str( ), CAtomInt( hour_left ).toString( ).c_str( ) );
							else if( hour_left > 0 )
								pResponse->strContent += UTIL_Xsprintf( gmapLANG_CFG["free_hour_left"].c_str( ), CAtomInt( hour_left ).toString( ).c_str( ), CAtomInt( minute_left ).toString( ).c_str( ) );
							else if( minute_left >= 0 )
								pResponse->strContent += UTIL_Xsprintf( gmapLANG_CFG["free_minute_left"].c_str( ), CAtomInt( minute_left ).toString( ).c_str( ) );
							pResponse->strContent += "</span>";
						}
					}
					
					pResponse->strContent += "</td>\n";

					pResponse->strContent += "<td class=\"download\">";

					if( m_bAllowTorrentDownloads && ( pRequest->user.ucAccess & m_ucAccessDownTorrents ) && pTorrents[ulKey].bAllow )
					{
						// The Trinity Edition - Modification Begins
						// The following adds "[" and "]" around the DL (download) link
						
						pResponse->strContent += "<a class=\"download\" title=\"" + gmapLANG_CFG["stats_download_torrent"] + ": " + pTorrents[ulKey].strFileName + "\" href=\"";
						pResponse->strContent += RESPONSE_STR_TORRENTS + pTorrents[ulKey].strID + ".torrent";

					//	pResponse->strContent += "\">" + gmapLANG_CFG["download"] + "</a>]</td>\n";
						pResponse->strContent += "\">" + gmapLANG_CFG["download_icon"] + "</a>";
					}

					if( pRequest->user.ucAccess & m_ucAccessBookmark )
					{
						bool bBookmarked = false;

						if( UTIL_MatchVector( pTorrents[ulKey].strID, vecBookmark, MATCH_METHOD_NONCASE_EQ ) )
							bBookmarked = true;
						
						pResponse->strContent += "<a id=\"bookmark" + pTorrents[ulKey].strID + "\" title=\"";

						if( bBookmarked )
							pResponse->strContent += gmapLANG_CFG["stats_no_bookmark"];
						else
							pResponse->strContent += gmapLANG_CFG["stats_bookmark"];

						pResponse->strContent += "\" class=\"bookmark_icon\" href=\"javascript: ;\" onclick=\"javascript: bookmark('" + pTorrents[ulKey].strID + "','" + gmapLANG_CFG["stats_bookmark"] + "','" + gmapLANG_CFG["stats_no_bookmark"] + "');\">";
						if( UTIL_MatchVector( pTorrents[ulKey].strID, vecBookmark, MATCH_METHOD_NONCASE_EQ ) )
							pResponse->strContent += gmapLANG_CFG["bookmarked_icon"] + "</a>";
						else
							pResponse->strContent += gmapLANG_CFG["bookmark_icon"] + "</a>";
					}

					if( !pTorrents[ulKey].strIMDb.empty( ) )
						pResponse->strContent += "<br><a class=\"imdb\" target=\"_blank\" href=\"" + gmapLANG_CFG["imdb_url"] + pTorrents[ulKey].strIMDbID + "/\">" + gmapLANG_CFG["imdb"] + ": " + pTorrents[ulKey].strIMDb + "</a>";
					pResponse->strContent += "</td>\n";
					
					// <td> comments

					if( m_bAllowComments )
					{
						pResponse->strContent += "<td class=\"number\"><a class=\"number\" title=\"" + gmapLANG_CFG["comments"] + ": " + UTIL_RemoveHTML( pTorrents[ulKey].strName ) + "\" href=\"" + RESPONSE_STR_COMMENTS_HTML + strJoined;
						pResponse->strContent += "\">" + CAtomInt( pTorrents[ulKey].uiComments ).toString( ) + "</a></td>\n";
					}

					// <td> added

					if( m_bShowAdded_Index )
					{
						pResponse->strContent += "<td class=\"date\">";

						if( !pTorrents[ulKey].strAdded.empty( ) )
						{
							string strAdded = pTorrents[ulKey].strAdded;
							
							if( bAddedPassed )
							{
								strAdded = UTIL_PassedToString( now_t, tTimeAdded, string( ) );
								if( strAdded.empty( ) )
									strAdded = pTorrents[ulKey].strAdded;
							}
							
							const string :: size_type br = strAdded.find( ' ' );
							pResponse->strContent += strAdded.substr( 0, br );
							if( br != string :: npos )
								pResponse->strContent += "<br>" +  strAdded.substr( br + 1 );
						}

						pResponse->strContent += "</td>\n";
					}

					// <td> size

					if( m_bShowSize )
					{
						const string :: size_type br = UTIL_BytesToString( pTorrents[ulKey].iSize ).find( ' ' );
						pResponse->strContent += "<td class=\"bytes\">" + UTIL_BytesToString( pTorrents[ulKey].iSize ).substr( 0, br );
						if( br != string :: npos )
							pResponse->strContent += "<br>" + UTIL_BytesToString( pTorrents[ulKey].iSize ).substr( br + 1 );
						pResponse->strContent += "</td>\n";
					}

					// <td> files

					if( m_bShowNumFiles )
						pResponse->strContent += "<td class=\"number\">" + CAtomInt( pTorrents[ulKey].uiFiles ).toString( ) + "</td>\n";

					// <td> transferred

//					if( m_bShowTransferred )
//						pResponse->strContent += "<td class=\"bytes\">" + UTIL_BytesToString( pTorrents[ulKey].iTransferred ) + "</td>\n";

					// <td> min left

// 					if( m_bShowMinLeft )
// 					{
// 						pResponse->strContent += "<td class=\"percent\">";
// 
// 						if( pTorrents[ulKey].uiLeechers == 0 )
// 							pResponse->strContent += gmapLANG_CFG["na"];
// 						else
// 						{
// 							ucPercent = 0;
// 
// 							if( pTorrents[ulKey].iSize > 0 )
// 							{
// 								if( m_bShowLeftAsProgress )
// 									ucPercent = (unsigned char)( 100 - (unsigned char)( ( (float)pTorrents[ulKey].iMaxiLeft / pTorrents[ulKey].iSize ) * 100 ) );
// 								else
// 									ucPercent = (unsigned char)( ( (float)pTorrents[ulKey].iMinLeft / pTorrents[ulKey].iSize ) * 100 );
// 							}
// 
// 							pResponse->strContent += CAtomInt( ucPercent ).toString( ) + "%";
// 						}
// 
// 						pResponse->strContent += "</td>\n";
// 					}

					// <td> average left

// 					if( m_bShowAverageLeft )
// 					{
// 						pResponse->strContent += "<td class=\"percent\">";
// 
// 						if( pTorrents[ulKey].uiLeechers == 0 )
// 							pResponse->strContent += gmapLANG_CFG["na"];
// 						else
// 						{
// 							if( m_bShowLeftAsProgress )
// 								ucPercent = (unsigned char)( 100 - pTorrents[ulKey].ucAverageLeftPercent );
// 							else
// 								ucPercent = pTorrents[ulKey].ucAverageLeftPercent;
// 
// 							pResponse->strContent += CAtomInt( ucPercent ).toString( ) + "%";
// 
// 							if( !imagefill.strFile.empty( ) && !imagetrans.strFile.empty( ) )
// 							{

// 								pResponse->strContent += "<br>";
// 
// 								string strFillFileName = imagefill.strName;
// 
// 								if( strFillFileName.empty( ) )
// 									strFillFileName = "imagebarfill.png";
// 
// 								if( ucPercent > 0 )
// 								{
// 									if( !imagefill.strURL.empty( ) )
// 										pResponse->strContent += "<img class=\"percent\" src=\"" + imagefill.strURL + strFillFileName + "\" width=" + CAtomInt( ucPercent ).toString( ) + " height=8 alt=\"[" + gmapLANG_CFG["completed"] + "]\" name=\"Completed\" title=\"" + gmapLANG_CFG["completed"] + "\">";
// 									else if( m_bServeLocal )
// 										pResponse->strContent += "<img class=\"percent\" src=\"" + strFillFileName + "\" width=" + CAtomInt( ucPercent ).toString( ) + " height=8 alt=\"[" + gmapLANG_CFG["completed"] + "]\" name=\"Completed\" title=\"" + gmapLANG_CFG["completed"] + "\">";
// 								}
// 
// 								string strTransFileName = imagetrans.strName;
// 
// 								if( strTransFileName.empty( ) )
// 									strTransFileName = "imagebartrans.png";
// 
// 								if( ucPercent < 100 )
// 								{
// 									if( !imagetrans.strURL.empty( ) )
// 										pResponse->strContent += "<img class=\"percent\" src=\"" + imagetrans.strURL + strTransFileName + "\" width=" + CAtomInt( 100 - ucPercent ).toString( ) + " height=8 alt=\"[" + gmapLANG_CFG["remaining"] + "]\" name=\"Remaining\" title=\"" + gmapLANG_CFG["remaining"] + "\">";
// 									else if( m_bServeLocal )
// 										pResponse->strContent += "<img class=\"percent\" src=\"" + strTransFileName + "\" width=" + CAtomInt( 100 - ucPercent ).toString( ) + " height=8 alt=\"[" + gmapLANG_CFG["remaining"] + "]\" name=\"Remaining\" title=\"" + gmapLANG_CFG["remaining"] + "\">";
// 								}
// 							}
// 						}
// 						pResponse->strContent += "</td>\n";
// 					}

					// <td> maxi left

// 					if( m_bShowMaxiLeft )
// 					{
// 						pResponse->strContent += "<td class=\"percent\">";
// 
// 						if( pTorrents[ulKey].uiLeechers == 0 )
// 							pResponse->strContent += gmapLANG_CFG["na"];
// 						else
// 						{
// 							ucPercent = 0;
// 
// 							if( pTorrents[ulKey].iSize > 0 )
// 							{
// 								if( m_bShowLeftAsProgress )
// 									ucPercent = (unsigned char)( 100 - (int)( ( (float)pTorrents[ulKey].iMinLeft / pTorrents[ulKey].iSize ) * 100 ) );
// 								else
// 									ucPercent = (unsigned char)( ( (float)pTorrents[ulKey].iMaxiLeft / pTorrents[ulKey].iSize ) * 100 );
// 							}
// 
// 							pResponse->strContent += CAtomInt( ucPercent ).toString( ) + "%";
// 						}
// 
// 						pResponse->strContent += "</td>\n";
// 					}

					// <td> uploader

					if( m_bShowUploader )
					{
						pResponse->strContent += "<td class=\"uploader\">";
						
						if( !pTorrents[ulKey].strUploaderID.empty( ) )
						{
							pResponse->strContent += getUserLink( pTorrents[ulKey].strUploaderID, pTorrents[ulKey].strUploader );
						}
						else
							pResponse->strContent += UTIL_RemoveHTML( pTorrents[ulKey].strUploader );
						pResponse->strContent += "</td>\n";
					}

					// <td> ip

					if( m_bShowIP && ( pRequest->user.ucAccess & m_ucAccessShowIP ) )
						pResponse->strContent += "<td class=\"ip\">" + UTIL_RemoveHTML( pTorrents[ulKey].strIP ) + "</td>\n";

					if( ( pRequest->user.ucAccess & m_ucAccessEditTorrents ) || ( pRequest->user.ucAccess & m_ucAccessDelTorrents ) )
					{
						pResponse->strContent += "<td class=\"admin\">";
						if( pRequest->user.ucAccess & m_ucAccessEditTorrents )
							pResponse->strContent += "[<a class=\"black\" title=\"" + gmapLANG_CFG["edit"] + ": " + UTIL_RemoveHTML( pTorrents[ulKey].strName ) + "\" href=\"" + RESPONSE_STR_STATS_HTML + strJoined + "&amp;action=edit&amp;show=contents";
						pResponse->strContent += "\">" + gmapLANG_CFG["edit"] + "</a>]";
						if( pRequest->user.ucAccess & m_ucAccessDelTorrents )
							pResponse->strContent += "<br>[<a class=\"red\" title=\"" + gmapLANG_CFG["delete"] + ": " + UTIL_RemoveHTML( pTorrents[ulKey].strName ) + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?del=" + pTorrents[ulKey].strID;
						pResponse->strContent += "&amp;return=" + UTIL_RemoveHTML( UTIL_StringToEscaped( strReturn ) );
						pResponse->strContent += "\">" + gmapLANG_CFG["delete"] + "</a>]";
						pResponse->strContent += "</td>\n";
					}

					pResponse->strContent += "</tr>\n";

					// increment row counter for row colour
					ulAdded++;
				}
				else
					ulSkipped++;
			}
		}

		// some finishing touches

		if( bFound )
			pResponse->strContent += "</table>\n\n";

		string strInsert = string( );
		
		// How many results?
		switch( ulFound )
		{
		case RESULTS_ZERO:
			strInsert += "<p class=\"results\">" + gmapLANG_CFG["result_none_found"] + "</p>\n\n";
			break;
// 		case RESULTS_ONE:
// 			pResponse->strContent += "<p class=\"results\">" + gmapLANG_CFG["result_1_found"] + "</p>\n\n";
// 			break;
// 		default:
// 			// Many results found
// 			pResponse->strContent += "<p class=\"results\">" + UTIL_Xsprintf( gmapLANG_CFG["result_x_found"].c_str( ), CAtomInt( ulFound ).toString( ).c_str( ) ) + "</p>\n\n";
		}
		
		vecParams.clear( );

		vecParams.push_back( pair<string, string>( string( "per_page" ), cstrPerPage ) );
		vecParams.push_back( pair<string, string>( string( "sort" ), strSort ) );
		vecParams.push_back( pair<string, string>( string( "search" ), strSearch ) );
		vecParams.push_back( pair<string, string>( string( "match" ), strMatch ) );
		vecParams.push_back( pair<string, string>( string( "tag" ), strFilter ) );
		vecParams.push_back( pair<string, string>( string( "medium" ), strMedium ) );
		vecParams.push_back( pair<string, string>( string( "quality" ), strQuality ) );
		vecParams.push_back( pair<string, string>( string( "encode" ), strEncode ) );
		vecParams.push_back( pair<string, string>( string( "uploader" ), strUploader ) );
		vecParams.push_back( pair<string, string>( string( "mode" ), cstrMode ) );
		vecParams.push_back( pair<string, string>( string( "section" ), cstrSection ) );
		vecParams.push_back( pair<string, string>( string( "notop" ), cstrNoTop ) );
		vecParams.push_back( pair<string, string>( string( "day" ), cstrDay ) );
		vecParams.push_back( pair<string, string>( string( "notag" ), cstrNoTag ) );
		
		strJoined = UTIL_RemoveHTML( UTIL_HTMLJoin( vecParams, string( "&" ), string( "&" ), string( "=" ) ) );
		
		// page numbers top
		
		strInsert += UTIL_PageBar( ulFound, cstrPage, uiOverridePerPage, RESPONSE_STR_INDEX_HTML, strJoined, true );
		
		if( iCountGoesHere != string :: npos )
			pResponse->strContent.insert( iCountGoesHere, strInsert );
		
		// page numbers bottom
		
		pResponse->strContent += UTIL_PageBar( ulFound, cstrPage, uiOverridePerPage, RESPONSE_STR_INDEX_HTML, strJoined, false );

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
