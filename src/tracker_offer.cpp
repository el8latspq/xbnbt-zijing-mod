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

#include <fcntl.h>

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

void CTracker :: serverResponseOfferGET( struct request_t *pRequest, struct response_t *pResponse )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
		if( IsIPBanned( pRequest, pResponse, btv, gmapLANG_CFG["offer_page"], string( CSS_OFFER ), NOT_INDEX ) )
			return;
	
	// Check that user has view authority
	if( !pRequest->user.strUID.empty( ) && ( pRequest->user.ucAccess & m_ucAccessViewOffers ) )
	{
		// Was a search submited?
		if( pRequest->mapParams["top_submit_search_button"] == gmapLANG_CFG["search"] || pRequest->mapParams["bottom_submit_search_button"] == gmapLANG_CFG["search"] )
		{
			string cstrSearch( pRequest->mapParams["search"] );
			string cstrUploader = string( );
			string cstrFilter( pRequest->mapParams["tag"] );
			const string cstrPerPage( pRequest->mapParams["per_page"] );
			const string cstrSearchMode( pRequest->mapParams["smode"] );
			
			if( cstrSearchMode == "uploader" )
			{
				cstrUploader = cstrSearch;
				cstrSearch.erase( );
			}
			
			for( map<string, string> :: iterator it = pRequest->mapParams.begin( ); it != pRequest->mapParams.end( ); it++ )
			{
				if( it->first.substr( 0, 6 ) == "tag" && it->second == "on" )
				{
					if( !cstrFilter.empty( ) )
						cstrFilter += "+";
					cstrFilter += it->first.substr( 6 );
				}
			}

			string strPageParameters = OFFER_HTML;
			
			vector< pair< string, string > > vecParams;
			vecParams.reserve(64);
			
			vecParams.push_back( pair<string, string>( string( "search" ), cstrSearch ) );
			vecParams.push_back( pair<string, string>( string( "tag" ), cstrFilter ) );
			vecParams.push_back( pair<string, string>( string( "uploader" ), cstrUploader ) );
			vecParams.push_back( pair<string, string>( string( "per_page" ), cstrPerPage ) );
			
			strPageParameters += UTIL_HTMLJoin( vecParams, string( "?" ), string( "&" ), string( "=" ) );

			return JS_ReturnToPage( pRequest, pResponse, strPageParameters );
		}
		
		if( pRequest->mapParams["top_clear_filter_and_search_button"] == gmapLANG_CFG["clear_filter_search"] || pRequest->mapParams["bottom_clear_filter_and_search_button"] == gmapLANG_CFG["clear_filter_search"] )
		{
			const string cstrSearch( pRequest->mapParams["search"] );
			const string cstrSearchResp( UTIL_StringToEscaped( cstrSearch ) );
			const string cstrSearchMode( pRequest->mapParams["smode"] );
			
			string strPageParameters = OFFER_HTML;
	
			if( !cstrSearch.empty( ) )
			{
				strPageParameters += "?";
				if( cstrSearchMode == "name" )
					strPageParameters += "search=" + cstrSearchResp;
				else
					strPageParameters += "uploader=" + cstrSearchResp;
			}
			
			return JS_ReturnToPage( pRequest, pResponse, strPageParameters );
		}
		
		string strReturnPage( pRequest->mapParams["return"] );
		
		if( strReturnPage.empty( ) )
			strReturnPage = RESPONSE_STR_OFFER_HTML;
		
		const string strReturnPageResp( UTIL_StringToEscaped( strReturnPage ) );
		
		//
		// allow offer
		//
		if( ( pRequest->user.ucAccess & m_ucAccessAllowOffers ) )
		{
			if( pRequest->mapParams.find( "allow" ) != pRequest->mapParams.end( ) )
			{
				string strAllowID( pRequest->mapParams["allow"] );
				const string strOK( pRequest->mapParams["ok"] );
				
				if( strAllowID.find( " " ) != string :: npos )
					strAllowID.erase( );
				
				string strAllowHash = string( );
				
				CMySQLQuery *pQueryOffer = new CMySQLQuery( "SELECT bhash,bfilename,bname,badded,bsize,bfiles,bcomment,btag,btitle,bintr,bip,buploader,buploaderid,bcomments,bimdb,bimdbid,bimdbupdated FROM offer WHERE bid=" + strAllowID );
			
				vector<string> vecQueryOffer;
			
				vecQueryOffer.reserve(17);

				vecQueryOffer = pQueryOffer->nextRow( );
				
				if( vecQueryOffer.size( ) == 17 )
					strAllowHash = vecQueryOffer[0];
				
				delete pQueryOffer;
				
				if( !strAllowHash.empty( ) )
				{
					CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bid FROM allowed WHERE bhash=\'" + UTIL_StringToMySQL( strAllowHash ) + "\'" );
						
					vector<string> vecQuery;
					
					vecQuery.reserve(1);

					vecQuery = pQuery->nextRow( );
					
					delete pQuery;
					
					if( vecQuery.size( ) == 1 )
					{
						// Output common HTML head
						HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["offer_page"], string( CSS_OFFER ), string( ), NOT_INDEX, CODE_200 );
						// failed
						pResponse->strContent += "<p class=\"failed\">" + gmapLANG_CFG["failed"] + "</p>\n";
						// A file with the uploaded file's info hash already exists.

						pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["upload_hash_exists"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_offer"] + "\" href=\"" + strReturnPage + "\">" ).c_str( ), "</a>" ) + "</p>\n";
						// Output common HTML tail
						HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_OFFER ) );

						return;
					}
					else
					{
						if( strOK == "1" )
						{
							if( vecQueryOffer.size( ) == 17 && !vecQueryOffer[1].empty( ) )
							{
								if( UTIL_CheckFile( string( m_strAllowedDir + vecQueryOffer[1] ).c_str( ) ) )
								{
									// Output common HTML head
									HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["offer_page"], string( CSS_OFFER ), string( ), NOT_INDEX, CODE_200 );
									// failed
									pResponse->strContent += "<p class=\"failed\">" + gmapLANG_CFG["failed"] + "</p>\n";
									//The uploaded file already exists.
									pResponse->strContent += "<p class=\"body_upload\">" + UTIL_Xsprintf( gmapLANG_CFG["upload_file_exists"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_offer"] + "\" href=\"" + RESPONSE_STR_OFFER_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";
									
									// Output common HTML tail
									HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_OFFER ) );
									
									return;
								}
									
								CMySQLQuery *pQueryUsers = new CMySQLQuery( "SELECT buid FROM users WHERE buid=" + vecQueryOffer[12] );
			
								vector<string> vecQueryUsers;
							
								vecQueryUsers.reserve(1);

								vecQueryUsers = pQueryUsers->nextRow( );
								
								delete pQueryUsers;
							
								if( vecQueryUsers.size( ) == 1 && !vecQueryUsers[0].empty( ) )
								{
									if( !pRequest->user.strUID.empty( ) )
									{
										string strTitle = gmapLANG_CFG["admin_allow_offer_title"];

										string strMessage = UTIL_Xsprintf( gmapLANG_CFG["admin_allow_offer"].c_str( ), UTIL_AccessToString( pRequest->user.ucAccess ).c_str( ), pRequest->user.strLogin.c_str( ), vecQueryOffer[1].c_str( ) );
										
										sendMessage( pRequest->user.strLogin, pRequest->user.strUID, vecQueryUsers[0], pRequest->strIP, strTitle, strMessage );
									}

								}

								if( !vecQueryOffer[1].empty( ) )
								{
//									string strDefaultDown = CFG_GetString( "bnbt_free_rule_down_default", "100" );
//									string strDefaultUp = CFG_GetString( "bnbt_free_rule_up_default", "100" );
//									string strFreeDown = CFG_GetString( "bnbt_free_rule_down_default", "100" );
//									string strFreeUp = CFG_GetString( "bnbt_free_rule_up_default", "100" );
//									string strFreeTime = CFG_GetString( "bnbt_free_rule_time_default", "0" );
//									string strDefaultRule = CFG_GetString( "bnbt_free_rule_" + vecQueryOffer[7] + "_default", string( ) );
//									string strRule = CFG_GetString( "bnbt_free_rule_" + vecQueryOffer[7], string( ) );
//									string :: size_type iSplit = 0;
//									string :: size_type iSplitTime = 0;
//									if( !strDefaultRule.empty( ) )
//									{
//										iSplit = strDefaultRule.find( "|" );
//										if( iSplit == string :: npos ) 
//											strDefaultDown = strDefaultRule.substr( 0, iSplit );
//										else
//										{
//											strDefaultDown = strDefaultRule.substr( 0, iSplit );
//											strDefaultUp = strDefaultRule.substr( iSplit + 1 );
//										}
//									}
//									if( !strRule.empty( ) )
//									{
//										iSplit = strRule.find( "|" );
//										iSplitTime = strRule.find( "|", iSplit + 1 );
//										if( iSplit == string :: npos ) 
//										{
//											if( iSplitTime == string :: npos )
//												strFreeDown = strRule.substr( 0, iSplit );
//										}
//										else
//										{
//											if( iSplitTime == string :: npos )
//											{
//												strFreeDown = strRule.substr( 0, iSplit );
//												strFreeUp = strRule.substr( iSplit + 1 );
//											}
//											else
//											{
//												strFreeDown = strRule.substr( 0, iSplit );
//												strFreeUp = strRule.substr( iSplit + 1, iSplitTime - iSplit - 1 );
//												strFreeTime = strRule.substr( iSplitTime + 1 );
//											}
//										}
//									}
									UTIL_MoveFile( string( m_strOfferDir + vecQueryOffer[1] ).c_str( ), string( m_strAllowedDir + vecQueryOffer[1] ).c_str( ) );
									string strAllowedID = string( );
									
									strAllowedID = parseTorrent( string( m_strAllowedDir + vecQueryOffer[1] ).c_str( ) );
									
									if( !strAllowedID.empty( ) )
									{
										int64 iSize = 0;
						
										CMySQLQuery *pQueryAllowed = new CMySQLQuery( "SELECT badded,bsize FROM allowed WHERE bid=" + strAllowedID );
						
										vector<string> vecQueryAllowed;

										vecQueryAllowed.reserve(2);
						
										vecQueryAllowed = pQueryAllowed->nextRow( );
						
										delete pQueryAllowed;
						
										if( vecQueryAllowed.size( ) == 2 && !vecQueryAllowed[1].empty( ) )
											iSize = UTIL_StringTo64( vecQueryAllowed[1].c_str( ) );
											
										string strDefaultDown = CFG_GetString( "bnbt_free_rule_down_default", "100" );
										string strDefaultUp = CFG_GetString( "bnbt_free_rule_up_default", "100" );
										string strFreeDown = CFG_GetString( "bnbt_free_rule_down_default", "100" );
										string strFreeUp = CFG_GetString( "bnbt_free_rule_up_default", "100" );
										string strFreeTime = CFG_GetString( "bnbt_free_rule_time_default", "0" );
						
										unsigned char ucRule = 1;
										string strRule = CFG_GetString( "bnbt_free_rule_" + CAtomInt( ucRule ).toString( ), string( ) );
										vector<string> vecRule;
										vecRule.reserve(9);
					
										vector<string> vecTags;
										vecTags.reserve(64);
										vector<string> vecKeyword;
										vecKeyword.reserve(64);
					
										vecRule = UTIL_SplitToVectorStrict( strRule, "|" );
			
										while( !vecRule.empty( ) && vecRule.size( ) == 9 )
										{
											vecTags = UTIL_SplitToVector( vecRule[0], " " );
											if( UTIL_MatchVector( vecQueryOffer[7], vecTags, MATCH_METHOD_NONCASE_OR ) )
											{
												vecKeyword = UTIL_SplitToVector( vecRule[1], " " );
												if( UTIL_MatchVector( vecQueryOffer[8], vecKeyword, MATCH_METHOD_NONCASE_OR ) )
												{
													vecKeyword = UTIL_SplitToVector( vecRule[2], " " );
													if( UTIL_MatchVector( vecQueryOffer[8], vecKeyword, MATCH_METHOD_NONCASE_AND ) )
													{
														int64 iFreeSize = 0;
														if( !vecRule[3].empty( ) )
															iFreeSize = UTIL_StringTo64( vecRule[3].c_str( ) ) * 1024 * 1024 * 1024;
														if( iSize > iFreeSize )
														{
															strDefaultDown = vecRule[4];
															strDefaultUp = vecRule[5];
															strFreeDown = vecRule[6];
															strFreeUp = vecRule[7];
															strFreeTime = vecRule[8];
														}
													}
												}
											}
											strRule = CFG_GetString( "bnbt_free_rule_" + CAtomInt( ++ucRule ).toString( ), string( ) );
											vecRule = UTIL_SplitToVectorStrict( strRule, "|" );
										}
										
										modifyTag( strAllowedID, vecQueryOffer[7], vecQueryOffer[8], vecQueryOffer[9], vecQueryOffer[11], vecQueryOffer[12], vecQueryOffer[10], strDefaultDown, strDefaultUp, strFreeDown, strFreeUp, strFreeTime, vecQueryOffer[13], false, false );
									
										CMySQLQuery mq01( "UPDATE allowed SET bimdb=\'" + UTIL_StringToMySQL( vecQueryOffer[14] ) + "\',bimdbid=\'" + UTIL_StringToMySQL( vecQueryOffer[15] ) + "\',bimdbupdated=\'" + UTIL_StringToMySQL( vecQueryOffer[16] ) + "\' WHERE bid=" + strAllowedID );
									
										CMySQLQuery mq02( "UPDATE comments SET comments.btid=" + strAllowedID + " WHERE comments.boid=" + strAllowID );
										
										if( vecQueryAllowed.size( ) == 2 && !vecQueryAllowed[0].empty( ) )
											m_pCache->setLatest( vecQueryAllowed[0] );
										m_pCache->Reset( );
										
										CMySQLQuery mq03( "DELETE FROM offer WHERE bid=" + strAllowID );
									
										m_pCache->Reset( true );
										
										if( !vecQueryOffer[12].empty( ) )
										{
											CMySQLQuery *pQueryFriend = new CMySQLQuery( "SELECT buid FROM friends WHERE bfriendid=" + vecQueryOffer[12] );
			
											vector<string> vecQueryFriend;
	
											vecQueryFriend.reserve(1);

											vecQueryFriend = pQueryFriend->nextRow( );
				
											while( vecQueryFriend.size( ) == 1 )
											{
												if( !vecQueryFriend[0].empty( ) )
												{
													CMySQLQuery mq01( "INSERT INTO talktorrent (buid,bfriendid,btid,bposted) VALUES(" + vecQueryFriend[0] + "," + pRequest->user.strUID + "," + strAllowedID + ",'" + UTIL_StringToMySQL( vecQueryAllowed[0] ) + "')" );
													CMySQLQuery mq02( "UPDATE users SET btalktorrent=btalktorrent+1 WHERE buid=" + vecQueryFriend[0] );
												}
						
												vecQueryFriend = pQueryFriend->nextRow( );
											}
		
											delete pQueryFriend;
										}
										
										UTIL_LogFilePrint( "allowOffer: %s allowed offer %s\n", pRequest->user.strLogin.c_str( ), vecQueryOffer[1].c_str( ) );
									}
									else
									{
										// failed
										pResponse->strContent += "<p class=\"failed\">" + gmapLANG_CFG["failed"] + "</p>\n";
									}
									
									// Output common HTML head
									HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["offer_page"], string( CSS_OFFER ), string( ), NOT_INDEX, CODE_200 );

									// Allowed the offer

									pResponse->strContent += "<p class=\"deleted\">" + UTIL_Xsprintf( gmapLANG_CFG["offer_allow_offer"].c_str( ), strAllowID.c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_offer"] + "\" href=\"" + strReturnPage + "\">" ).c_str( ), "</a>" ) + "</p>\n";

									// Output common HTML tail
									HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_OFFER ) );

									return;
								}

							}
						}
						else
						{
							// Output common HTML head
							HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["offer_page"], string( CSS_OFFER ), string( ), NOT_INDEX, CODE_200 );
							pResponse->strContent += "<div class=\"torrent_delete\">\n";

							pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["allow_offer_q"].c_str( ), strAllowID.c_str( ) ) + "</p>\n";
							pResponse->strContent += "<p class=\"delete\"><a title=\"" + gmapLANG_CFG["yes"] + "\" href=\"" + RESPONSE_STR_OFFER_HTML + "?allow=" + strAllowID + "&amp;ok=1";
							pResponse->strContent += "&amp;return=" + strReturnPageResp;
							pResponse->strContent += "\">" + gmapLANG_CFG["yes"] + "</a>\n";
							pResponse->strContent += "<span class=\"pipe\"> | </span><a title=\"" + gmapLANG_CFG["no"] + "\" href=\"" + string( JS_BACK ) + "\">" + gmapLANG_CFG["no"] + "</a></p>\n";
							pResponse->strContent += "\n</div>";
							// Output common HTML tail
							HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_OFFER ) );

							return;
						}
					}
				}
				else
				{
					// Output common HTML head
					HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["offer_page"], string( CSS_OFFER ), string( ), NOT_INDEX, CODE_200 );

					pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["offer_invalid_hash"].c_str( ), strAllowID.c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_offer"] + "\" href=\"" + strReturnPage + "\">" ).c_str( ), "</a>" ) + "</p>\n";

					// Output common HTML tail
					HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_OFFER ) );

					return;
				}
			}
		}

		//
		// delete offer
		//

		// Check that user has edit authority
		if( ( pRequest->user.ucAccess & m_ucAccessDelOffers ) )
		{
			if( pRequest->mapParams.find( "del" ) != pRequest->mapParams.end( ) )
			{
				string strDelID( pRequest->mapParams["del"] );
				const string strDelReason( UTIL_RemoveHTML( pRequest->mapParams["reason"] ) );
				const string strOK( pRequest->mapParams["ok"] );
				
				if( strDelID.find( " " ) != string :: npos )
					strDelID.erase( );
				
				CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bid,bfilename,buploaderid FROM offer WHERE bid=" + strDelID );
			
				vector<string> vecQuery;
			
				vecQuery.reserve(3);

				vecQuery = pQuery->nextRow( );
				
				delete pQuery;
				
				if( vecQuery.size( ) == 3 && !vecQuery[0].empty( ) )
				{
					if( !vecQuery[1].empty( ) )
					{
						string strPageParameters = OFFER_HTML;
						if( pRequest->mapParams["submit_delete_button"] == gmapLANG_CFG["yes"] )
						{
							strPageParameters += "?del=" + strDelID + "&reason=" + strDelReason + "&ok=1";
							return JS_ReturnToPage( pRequest, pResponse, strPageParameters );
						}
						
						if( strOK == "1" )
						{

							// delete from disk

							string strFileName = vecQuery[1];
							
							if( !strFileName.empty( ) )
							{
								if( m_strArchiveDir.empty( ) )
									UTIL_DeleteFile( string( m_strOfferDir + strFileName ).c_str( ) );
								else
									UTIL_MoveFile( string( m_strOfferDir + strFileName ).c_str( ), string( m_strArchiveDir + strFileName ).c_str( ) );
							}
							
							if( vecQuery.size( ) == 3 )
							{
								CMySQLQuery *pQueryUsers = new CMySQLQuery( "SELECT buid FROM users WHERE buid=" + vecQuery[2] );
			
								vector<string> vecQueryUsers;
							
								vecQueryUsers.reserve(1);

								vecQueryUsers = pQueryUsers->nextRow( );
								
								delete pQueryUsers;
								
								if( vecQueryUsers.size( ) == 1 && !vecQueryUsers[0].empty( ) )
								{
									if( !pRequest->user.strLogin.empty( ) )
									{
										string strTitle = gmapLANG_CFG["admin_delete_offer_title"];

										string strMessage = UTIL_Xsprintf( gmapLANG_CFG["admin_delete_offer"].c_str( ), UTIL_AccessToString( pRequest->user.ucAccess ).c_str( ), pRequest->user.strLogin.c_str( ), strFileName.c_str( ), strDelReason.c_str( ) );
										
										sendMessage( pRequest->user.strLogin, pRequest->user.strUID, vecQueryUsers[0], pRequest->strIP, strTitle, strMessage );
									}

								}
							}
								
							UTIL_LogFilePrint( "deleteOffer: %s deleted offer %s\n", pRequest->user.strLogin.c_str( ), strFileName.c_str( ) );
							UTIL_LogFilePrint( "deleteOffer: delete reason %s\n", strDelReason.c_str( ) );

							deleteTag( strDelID, true );

							// Output common HTML head
							HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["offer_page"], string( CSS_OFFER ), string( ), NOT_INDEX, CODE_200 );

							// Deleted the torrent
							pResponse->strContent += "<p class=\"deleted\">" + UTIL_Xsprintf( gmapLANG_CFG["offer_deleted_offer"].c_str( ), strDelID.c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_offer"] + "\" href=\"" + strReturnPage + "\">" ).c_str( ), "</a>" ) + "</p>\n";

							// Output common HTML tail
							HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_OFFER ) );

							return;
						}
						else
						{
							// Added and modified by =Xotic=
							// The Trinity Edition - Modification Begins
							// The following replaces the OK response with a YES | NO option
							// when DELETING A TORRENT

							// Output common HTML head
							HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["offer_page"], string( CSS_OFFER ), string( ), NOT_INDEX, CODE_200 );
							
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
							pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["delete_offer_q"].c_str( ), strDelID.c_str( ) ) + "</p>\n";
							pResponse->strContent += "<form name=\"deletetorrent\" method=\"get\" action=\"" + string( RESPONSE_STR_OFFER_HTML ) + "\" onSubmit=\"return validate( this )\">";
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
							HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_OFFER ) );

							return;
						}
					}
				}
				// Output common HTML head
				HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["offer_page"], string( CSS_OFFER ), string( ), NOT_INDEX, CODE_200 );

				pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["offer_invalid_hash"].c_str( ), strDelID.c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_offer"] + "\" href=\"" + strReturnPage + "\">" ).c_str( ), "</a>" ) + "</p>\n";

				// Output common HTML tail
				HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_OFFER ) );

				return;
			}
		}
		
		unsigned long ulKeySize = 0;
		
		struct torrent_t *pTorrents = 0;
		
		if( m_pCache )
		{
			pTorrents = m_pCache->getCache( true );
			ulKeySize = m_pCache->getSize( true );
		}
		
		// Are we tracking any files?	
		if( ulKeySize == 0 )
		{
			// Output common HTML head
			HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["offer_page"], string( CSS_OFFER ), string( ), NOT_INDEX, CODE_200 );

			// No files are being tracked!
			pResponse->strContent += "<p class=\"no_files\">" + gmapLANG_CFG["index_no_files"] + "</p>\n";

			// Output common HTML tail
			HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_OFFER ) );

			return;
		}

// 		if( m_bCountUniquePeers )
// 		{
// 			CMySQLQuery *pQueryIP = new CMySQLQuery( "SELECT bip from ips" );
// 			
// 			if( pQueryIP->numRows( ) > gtXStats.peer.iGreatestUnique )
// 				gtXStats.peer.iGreatestUnique = pQueryIP->numRows( );
// 			
// 			delete pQueryIP;
// 		}

		// Sort
		const string strSort( pRequest->mapParams["sort"] );
		
		if( m_bSort )
		{
			const unsigned char cucSort( (unsigned char)atoi( strSort.c_str( ) ) );
			if( !strSort.empty( ) )
				m_pCache->sort( cucSort, true, true );
			else
				if( m_bShowAdded )
					m_pCache->sort( SORT_DADDED, true, true );
		}
		else
			if( m_bShowAdded )
				m_pCache->sort( SORT_DADDED, true, true );
		
//		if( m_bSort )
//		{
//			const unsigned char cucSort( (unsigned char)atoi( strSort.c_str( ) ) );

//			if( !strSort.empty( ) )
//			{
//				switch( cucSort )
//				{
//				case SORT_ANAME:
//					qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), asortByName );
//					break;
//				case SORT_AADDED:
//					if( m_bShowAdded )
//						qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), asortByAdded );
//					break;
//				case SORT_ASIZE:
//					if( m_bShowSize )
//						qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), asortBySize );
//					break;
//				case SORT_AFILES:
//					if( m_bShowNumFiles )
//						qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), asortByFiles );
//					break;
//				case SORT_ACOMMENTS:
//					if( m_bAllowComments )
//						qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), asortByComments );
//					break;
//				case SORT_ATAG:
//					qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), asortByTag );
//					break;
//				case SORT_AUPLOADER:

//					if( m_bShowUploader )
//						qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), asortByUploader );
//					break;
//				case SORT_AIP:
//					if( pRequest->user.ucAccess & m_ucAccessSortIP )
//						qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), asortByIP );
//					break;
//				case SORT_DNAME:
//					qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByName );
//					break;
//				case SORT_DADDED:
//					if( m_bShowAdded )
//						qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByAdded );
//					break;
//				case SORT_DSIZE:
//					if( m_bShowSize )
//						qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortBySize );
//					break;
//				case SORT_DFILES:
//					if( m_bShowNumFiles )
//						qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByFiles );
//					break;
//				case SORT_DCOMMENTS:
//					if( m_bAllowComments )
//						qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByComments );
//					break;
//				case SORT_DTAG:
//					qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByTag );
//					break;
//				case SORT_DUPLOADER:
//					if( m_bShowUploader )
//						qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByUploader );
//					break;
//				case SORT_DIP:
//					if( pRequest->user.ucAccess & m_ucAccessSortIP )
//						qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByIP );
//					break;
//				default:
//					// default action is to sort by added if we can
//					if( m_bShowAdded )
//						qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByAdded );
//				}
//			}
//			else
//			{
//				// default action is to sort by added if we can

//				if( m_bShowAdded )
//					qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByAdded );
//			}
//		}
//		else
//		{
//			// sort is disabled, but default action is to sort by added if we can

//			if( m_bShowAdded )
//				qsort( pTorrents, ulKeySize, sizeof( struct torrent_t ), dsortByAdded );
//		}

		// Main Header
		time_t now_t = time( 0 );

		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["offer_page"], string( CSS_OFFER ), string( ), NOT_INDEX, CODE_200 );

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

		pResponse->strContent += "//-->\n";
		pResponse->strContent += "</script>\n\n";
		
		if( !pRequest->user.strUID.empty( ) )
		{
			CMySQLQuery mq01( "UPDATE users SET blast_offer=NOW() WHERE buid=" + pRequest->user.strUID );
		}
		
		// some preliminary search crap

		string strSearch( pRequest->mapParams["search"] );
//		const string strLowerSearch( UTIL_ToLower( strSearch ) );

		const string strUploader( pRequest->mapParams["uploader"] );
//		const string cstrLowerUploader( UTIL_ToLower( cstrUploader ) );

		// filters

		const string strFilter( pRequest->mapParams["tag"] );

		const string cstrPerPage( pRequest->mapParams["per_page"] );
		
		bool bAddedPassed = false;
		
		if( !pRequest->user.strUID.empty( ) )
		{
			CMySQLQuery *pQueryPrefs = new CMySQLQuery( "SELECT baddedpassed FROM users_prefs WHERE buid=" + pRequest->user.strUID );
	
			map<string, string> mapPrefs;

			mapPrefs = pQueryPrefs->nextRowMap( );

			delete pQueryPrefs;

			if( !mapPrefs["baddedpassed"].empty( ) && mapPrefs["baddedpassed"] == "1" )
				bAddedPassed = true;
		}
		
		vector<string> vecSearch;
		vecSearch.reserve(64);
		vector<string> vecUploader;
		vecUploader.reserve(64);
		vector<string> vecFilter;
		vecFilter.reserve(64);
		
		vecSearch = UTIL_SplitToVector( strSearch, " " );
		vecUploader = UTIL_SplitToVector( strUploader, " " );
		vecFilter = UTIL_SplitToVector( strFilter, " " );

		// Top search
		if( m_bSearch )
		{
			pResponse->strContent += "<form class=\"search_index_top\" name=\"topsearch\" method=\"get\" action=\"" + RESPONSE_STR_OFFER_HTML + "\">\n";

			if( !cstrPerPage.empty( ) )
				pResponse->strContent += "<p><input name=\"per_page\" type=hidden value=\"" + cstrPerPage + "\"></p>\n";

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

				pResponse->strContent += Button_Submit( "top_submit_search", gmapLANG_CFG["search"] );
				pResponse->strContent += Button_Submit( "top_clear_filter_and_search", gmapLANG_CFG["clear_filter_search"] );

				pResponse->strContent += "</p>\n";
//			}
//			else
//				pResponse->strContent += "<p><label for=\"toptorrentsearch\">" + gmapLANG_CFG["torrent_search"] + "</label> <input name=\"search\" id=\"toptorrentsearch\" alt=\"[" + gmapLANG_CFG["torrent_search"] + "]\" type=text size=40> <a title=\"" + gmapLANG_CFG["clear_filter_search"] + "\" href=\"" + RESPONSE_STR_OFFER_HTML + "\">" + gmapLANG_CFG["clear_filter_search"] + "</a></p>\n";

			pResponse->strContent += "</form>\n\n";
		}

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
			strResult += "<span class=\"search_results_alt\">" + gmapLANG_CFG["result_search"] + gmapLANG_CFG["uploader"] + ": </span>";
			strResult += "<span class=\"filtered_by_search\">";
		 	strResult += UTIL_RemoveHTML( strUploader );
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
				
				strResult += "<span class=\"search_results_alt\">" + gmapLANG_CFG["result_tag"] + ": ";
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
						strResult += " &amp; \n";
				}
				strResult += "</span>\n";
			}
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

		for( unsigned long ulKey = 0; ulKey < ulKeySize; ulKey++ )
		{
			if( !UTIL_MatchVector( pTorrents[ulKey].strName, vecSearch, MATCH_METHOD_NONCASE_AND ) )
				continue;
			if( !UTIL_MatchVector( pTorrents[ulKey].strUploader, vecUploader, MATCH_METHOD_NONCASE_AND ) )
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
					vecParams.push_back( pair<string, string>( string( "tag" ), strFilter ) );
					vecParams.push_back( pair<string, string>( string( "uploader" ), strUploader ) );
					
					strJoined = UTIL_RemoveHTML( UTIL_HTMLJoin( vecParams, string( "&" ), string( "&" ), string( "=" ) ) );
					
					pResponse->strContent += "<table class=\"torrent_table\" summary=\"files\">\n";

					pResponse->strContent += "<tr>\n";

					// <th> tag

					if( !m_vecTags.empty( ) )     
					{
						pResponse->strContent += "<th class=\"tag\" id=\"tagheader\">" + gmapLANG_CFG["tag"];

						pResponse->strContent += "</th>\n";
					}         

					// <th> name

// 					if( pRequest->user.ucAccess & ACCESS_DL )
						pResponse->strContent += "<th class=\"name\" id=\"nameheader\" colspan=2>";
// 					else
// 						pResponse->strContent += "<th class=\"name\" id=\"nameheader\">";

					if( m_bSort )
					{
						pResponse->strContent += "<a class=\"table_header\" href=\"" + RESPONSE_STR_OFFER_HTML + "?sort=";

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
					
					pResponse->strContent += "<th class=\"exist\" id=\"existheader\">" + gmapLANG_CFG["offer_exist"] + "</th>\n";
					
					pResponse->strContent += "<th class=\"seeded\" id=\"seededheader\">" + gmapLANG_CFG["offer_seeded"] + "</th>\n";

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
							pResponse->strContent += "<a class=\"table_header\" href=\"" + RESPONSE_STR_OFFER_HTML + "?sort=";

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
							pResponse->strContent += "<a class=\"table_header\" href=\"" + RESPONSE_STR_OFFER_HTML + "?sort=";

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
							pResponse->strContent += "<a class=\"table_header\" href=\"" + RESPONSE_STR_OFFER_HTML + "?sort=";

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

					// <th> uploader

					if( m_bShowUploader )
					{
						pResponse->strContent += "<th class=\"uploader\" id=\"uploaderheader\">";
						
						if( m_bSort )
						{
							pResponse->strContent += "<a class=\"table_header\" href=\"" + RESPONSE_STR_OFFER_HTML + "?sort=";
							
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

//					if( m_bShowIP && ( pRequest->user.ucAccess & m_ucAccessShowIP ) )
//					{
//						pResponse->strContent += "<th class=\"ip\" id=\"ipheader\">";
//						if( m_bSort )
//						{
//							pResponse->strContent += "<a class=\"table_header\" href=\"" + RESPONSE_STR_OFFER_HTML + "?sort=";
//							
//							if( strSort == SORTSTR_DIP )
//								pResponse->strContent += SORTSTR_AIP;
//							else
//								pResponse->strContent += SORTSTR_DIP;

//							if( !cstrPerPage.empty( ) )
//								pResponse->strContent += "&amp;per_page=" + cstrPerPage;

//							if( !strSearch.empty( ) )
//								pResponse->strContent += "&amp;search=" + strSearchResp;

//							if( !strFilter.empty( ) )
//								pResponse->strContent += "&amp;filter=" + UTIL_StringToEscaped( strFilter );
//							
//							if( !strUploader.empty( ) )
//								pResponse->strContent += "&amp;uploader=" + strUploader;
//							
//							pResponse->strContent += "\">";
//						}
//						
//						pResponse->strContent += gmapLANG_CFG["ip"];
//					
//						if( m_bSort )
//							pResponse->strContent += "</a>";

//						pResponse->strContent += "</th>\n";
//					}

					if( ( pRequest->user.ucAccess & m_ucAccessAllowOffers ) || ( pRequest->user.ucAccess & m_ucAccessEditOffers ) || ( pRequest->user.ucAccess & m_ucAccessDelOffers ) )
						pResponse->strContent += "<th id=\"adminheader\">" + gmapLANG_CFG["admin"] + "</th>\n";   

					pResponse->strContent += "</tr>\n";

					// signal table created
					bFound = true;
				}

				if( ulSkipped == ulStart )
				{
					// output table rows

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
								pResponse->strContent += "<a class=\"index_filter\" title=\"" + UTIL_RemoveHTML( gmapLANG_CFG["filter_by"] + ": " + strTag ) + "\" href=\"" + RESPONSE_STR_OFFER_HTML + "?tag=" + strNameIndex;

								vector< pair< string, string > > vecParams;
								vecParams.reserve(64);
	
								vecParams.push_back( pair<string, string>( string( "per_page" ), cstrPerPage ) );
								vecParams.push_back( pair<string, string>( string( "sort" ), strSort ) );
								vecParams.push_back( pair<string, string>( string( "search" ), strSearch ) );
								vecParams.push_back( pair<string, string>( string( "uploader" ), strUploader ) );
								
								pResponse->strContent += UTIL_RemoveHTML( UTIL_HTMLJoin( vecParams, string( "&" ), string( "&" ), string( "=" ) ) );

								pResponse->strContent += "\">\n";

								// Assigns functions to onMouseOver and onMouseOut for each Tag Image
								// Activated by setting "bnbt_use_mouseovers" to 1
								// Generates code that validates with HTML 4.01 Strict

// 								Sets a TITLE parameter for each Tag Image, so that a TOOLTIP box will popup when
// 								the user's mouse pointer hovers over the Tag Image.
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
			
					vecParams.push_back( pair<string, string>( string( "per_page" ), cstrPerPage ) );
					vecParams.push_back( pair<string, string>( string( "sort" ), strSort ) );
					vecParams.push_back( pair<string, string>( string( "search" ), strSearch ) );
					vecParams.push_back( pair<string, string>( string( "tag" ), strFilter ) );
					vecParams.push_back( pair<string, string>( string( "uploader" ), strUploader ) );
					vecParams.push_back( pair<string, string>( string( "page" ), cstrPage ) );
			
					strReturn = UTIL_RemoveHTML( UTIL_StringToEscaped( RESPONSE_STR_OFFER_HTML + UTIL_HTMLJoin( vecParams, string( "?" ), string( "&" ), string( "=" ) ) ) );
					
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
					
					time_t tTimeAdded = mktime(&time_tm);

					pResponse->strContent += "<td class=\"name\">";
					strEngName.erase( );
					strChiName.erase( );
					UTIL_StripName( pTorrents[ulKey].strName.c_str( ), strEngName, strChiName );

					if( m_bShowStats )
					{
						pResponse->strContent += "<a class=\"stats\" title=\"";
						
						pResponse->strContent += gmapLANG_CFG["name"] + ": " + UTIL_RemoveHTML( pTorrents[ulKey].strName ) + "\" href=\"" + RESPONSE_STR_STATS_HTML + "?oid=" + pTorrents[ulKey].strID;
//						pResponse->strContent += "&amp;return=" + strReturn;
						pResponse->strContent += "\">";
					
						pResponse->strContent += UTIL_RemoveHTML( strEngName );
						if( !strChiName.empty( ) )
							pResponse->strContent += "<br>" + UTIL_RemoveHTML( strChiName );
						pResponse->strContent += "</a>";
					}
					else
					{
						pResponse->strContent += UTIL_RemoveHTML( strEngName );
						if( !strChiName.empty( ) )
							pResponse->strContent += "<br>" + UTIL_RemoveHTML( strChiName );
					}

					pResponse->strContent += "</td>\n";
					
					pResponse->strContent += "<td class=\"download\"></td>";
					
					// <td> exist
					
					CMySQLQuery *pQueryAllowed = new CMySQLQuery( "SELECT bid FROM allowed WHERE bhash=\'" + UTIL_StringToMySQL( pTorrents[ulKey].strInfoHash ) + "\'" );
						
					if( pQueryAllowed->nextRow( ).size( ) == 1 )
						pResponse->strContent += "<td class=\"admin\"><span class=\"green\">" + gmapLANG_CFG["offer_yes"] + "</span></td>";
					else
						pResponse->strContent += "<td class=\"admin\"><span class=\"red\">" + gmapLANG_CFG["offer_no"] + "</span></td>";
					
					delete pQueryAllowed;
					
					// <td> seeded
					if( pTorrents[ulKey].uiSeeders > 0 )
						pResponse->strContent += "<td class=\"admin\"><span class=\"green\">" + gmapLANG_CFG["offer_yes"] + "</span></td>";
					else
						pResponse->strContent += "<td class=\"admin\"><span class=\"red\">" + gmapLANG_CFG["offer_no"] + "</span></td>";

					// <td> comments

					if( m_bAllowComments )
					{
						pResponse->strContent += "<td class=\"number\"><a class=\"number\" title=\"" + gmapLANG_CFG["comments"] + ": " + UTIL_RemoveHTML( pTorrents[ulKey].strName ) + "\" href=\"" + RESPONSE_STR_COMMENTS_HTML + "?oid=" + pTorrents[ulKey].strID;
						pResponse->strContent += "&amp;return=" + strReturn;
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

//					if( m_bShowIP && ( pRequest->user.ucAccess & m_ucAccessShowIP ) )
//						pResponse->strContent += "<td class=\"ip\">" + UTIL_RemoveHTML( pTorrents[ulKey].strIP ) + "</td>\n";
					
					if( ( pRequest->user.ucAccess & m_ucAccessAllowOffers ) || ( pRequest->user.ucAccess & m_ucAccessEditOffers ) || ( pRequest->user.ucAccess & m_ucAccessDelOffers ) )
					{
						pResponse->strContent += "<td class=\"admin\">";
						if( pRequest->user.ucAccess & m_ucAccessAllowOffers )
						{
							pResponse->strContent += "[<a class=\"black\" title=\"" + gmapLANG_CFG["allow"] + ": " + UTIL_RemoveHTML( pTorrents[ulKey].strName ) + "\" href=\"" + RESPONSE_STR_OFFER_HTML + "?allow=" + pTorrents[ulKey].strID;
							pResponse->strContent += "&amp;return=" + strReturn;
							pResponse->strContent += "\">" + gmapLANG_CFG["allow"] + "</a>]";
						}
						if( pRequest->user.ucAccess & m_ucAccessEditOffers )
						{
							pResponse->strContent += "[<a class=\"black\" title=\"" + gmapLANG_CFG["edit"] + ": " + UTIL_RemoveHTML( pTorrents[ulKey].strName ) + "\" href=\"" + RESPONSE_STR_STATS_HTML + "?oid=" + pTorrents[ulKey].strID + "&amp;action=edit&amp;show=contents";
							pResponse->strContent += "&amp;return=" + strReturn;
							pResponse->strContent += "\">" + gmapLANG_CFG["edit"] + "</a>]";
						}
						if( pRequest->user.ucAccess & m_ucAccessDelOffers )
						{
							pResponse->strContent += "<br>[<a class=\"red\" title=\"" + gmapLANG_CFG["delete"] + ": " + UTIL_RemoveHTML( pTorrents[ulKey].strName ) + "\" href=\"" + RESPONSE_STR_OFFER_HTML + "?del=" + pTorrents[ulKey].strID;
							pResponse->strContent += "&amp;return=" + strReturn;
							pResponse->strContent += "\">" + gmapLANG_CFG["delete"] + "</a>]</td>\n";
						}
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

		// free the memory
//		delete [] pTorrents;

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
		
		vector< pair< string, string > > vecParams;
		vecParams.reserve(64);
		string strJoined = string( );
		
		vecParams.push_back( pair<string, string>( string( "per_page" ), cstrPerPage ) );
		vecParams.push_back( pair<string, string>( string( "sort" ), strSort ) );
		vecParams.push_back( pair<string, string>( string( "search" ), strSearch ) );
		vecParams.push_back( pair<string, string>( string( "tag" ), strFilter ) );
		vecParams.push_back( pair<string, string>( string( "uploader" ), strUploader ) );
		
		strJoined = UTIL_RemoveHTML( UTIL_HTMLJoin( vecParams, string( "&" ), string( "&" ), string( "=" ) ) );

		// page numbers
		
		strInsert += UTIL_PageBar( ulFound, cstrPage, uiOverridePerPage, RESPONSE_STR_OFFER_HTML, strJoined, true );
		
		if( iCountGoesHere != string :: npos )
			pResponse->strContent.insert( iCountGoesHere, strInsert );
		
		// page numbers

		pResponse->strContent += UTIL_PageBar( ulFound, cstrPage, uiOverridePerPage, RESPONSE_STR_OFFER_HTML, strJoined, false );

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_OFFER ) );
	}
	else
	{
		// Not authorised

		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["offer_page"], string( CSS_OFFER ), string( ), NOT_INDEX, CODE_401 );

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_OFFER ) );
	}
}
