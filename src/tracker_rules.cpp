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

#include <fcntl.h>

#if defined ( WIN32 )
 #include <time.h>
#else
 #include <sys/time.h>
#endif

#include "bnbt.h"
#include "bnbt_mysql.h"
#include "atom.h"
#include "html.h"
#include "config.h"
#include "tracker.h"
#include "util.h"

void CTracker :: serverResponseRulesGET( struct request_t *pRequest, struct response_t *pResponse )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
		if( IsIPBanned( pRequest, pResponse, btv, gmapLANG_CFG["rules_page"], string( CSS_RULES ), NOT_INDEX ) )
			return;
		
	int64 rulescount=0;
	int64 rulesnum;
	string rulesfile;
	string rulesname;
	string strRulesName;
	string strRules;
	rulesfile = "rules" + CAtomInt( rulescount ).toString( );
	const string cstrRulesNum( pRequest->mapParams["rules"] );
	while( access( string( m_strRulesDir + rulesfile + ".name" ).c_str( ), 0 ) == 0 )
	{							
		rulescount++;
		rulesfile = "rules" + CAtomInt( rulescount ).toString( );
	}
	
	if( !cstrRulesNum.empty( ) )
	{
		if( !pRequest->user.strUID.empty( ) && ( pRequest->user.ucAccess & m_ucAccessAdmin ) )
		{
			// Output common HTML head
			HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["rules_page"], string( CSS_RULES ), string( ), NOT_INDEX, CODE_200 );	
			if( cstrRulesNum == "new" )
			{
				rulesfile = "rules" + CAtomInt( rulescount ).toString( );
				UTIL_MakeFile( string( m_strRulesDir + rulesfile + ".name" ).c_str( ), gmapLANG_CFG["new_rules"] );
				UTIL_MakeFile( string( m_strRulesDir + rulesfile ).c_str( ), gmapLANG_CFG["new_rules_content"] );
				pResponse->strContent += "<p class=\"changed_rules\">" + UTIL_Xsprintf( gmapLANG_CFG["rules_return"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_rules"] + "\" href=\"" + RESPONSE_STR_RULES_HTML + "?rules=" + CAtomInt( rulescount ).toString( ) + "&amp;edit=1\">" ).c_str( ), "</a>" ) + "</p>\n";
				// Output common HTML tail
				HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_RULES ) );
				
				return;
			}
			if( cstrRulesNum == "del" )
			{
				rulesfile = "rules" + CAtomInt( rulescount - 1 ).toString( );
				UTIL_DeleteFile( string( m_strRulesDir + rulesfile + ".name" ).c_str( ) );
				UTIL_DeleteFile( string( m_strRulesDir + rulesfile ).c_str( ) );
				pResponse->strContent += "<p class=\"changed_rules\">" + UTIL_Xsprintf( gmapLANG_CFG["rules_return"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_rules"] + "\" href=\"" + RESPONSE_STR_RULES_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";
				// Output common HTML tail
				HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_RULES ) );
				
				return;
			}
			
// 			rulesfile = "rules" + cstrRulesNum;
// 			if( access( string( m_strRulesDir + rulesfile + ".name" ).c_str( ), 0 ) == 0 )
// 			{
// 				struct stat info;
// 				
// 				strRulesName = UTIL_ReadFile( string( m_strRulesDir + rulesfile + ".name" ).c_str( ) );

// 				memset( strPostTime, 0, sizeof( strPostTime ) / sizeof( char ) );
// 				stat( string( m_strRulesDir + rulesfile ).c_str( ), &info );
// 				strftime( strPostTime, sizeof( strPostTime ) / sizeof( char ), "%Y-%m-%d %H:%M:%S", localtime( &info.st_mtime ) );

// 				strRules = UTIL_ReadFile( string( m_strRulesDir + rulesfile ).c_str( ) );
// 			}
			if( pRequest->mapParams.find( "edit" ) != pRequest->mapParams.end( ) )
			{
				const string strEdit( pRequest->mapParams["edit"] );
				rulesfile = "rules" + cstrRulesNum;
				if( access( string( m_strRulesDir + rulesfile + ".name" ).c_str( ), 0 ) == 0 )
				{
					strRulesName = UTIL_ReadFile( string( m_strRulesDir + rulesfile + ".name" ).c_str( ) );
					strRules = UTIL_ReadFile( string( m_strRulesDir + rulesfile ).c_str( ) );
				}
				else
				{
					strRulesName = gmapLANG_CFG["new_rules"];
					strRules = gmapLANG_CFG["new_rules_content"];
				}
				if( strEdit == "1" )
				{
					pResponse->strContent += "<div class=\"change_rules\">\n";
					pResponse->strContent += "<p class=\"change_rules\">" + gmapLANG_CFG["change_rules"] + "</p>\n";
					pResponse->strContent += "<p class=\"change_rules\">" + UTIL_RemoveHTML( strRulesName ) + "</p>\n";
					pResponse->strContent += "<table class=\"change_rules\">\n";
					pResponse->strContent += "<form method=\"post\" action=\"" + RESPONSE_STR_RULES_HTML + "\" enctype=\"multipart/form-data\">\n";
					pResponse->strContent += "<input name=\"rules\" type=hidden value=\"" + cstrRulesNum + "\">\n";
					pResponse->strContent += "<tr class=\"change_rules\">\n";
					pResponse->strContent += "<th class=\"change_rules\">" + gmapLANG_CFG["rules_new_name"] + "</th>\n";
					pResponse->strContent += "<td class=\"change_rules\">\n";
					pResponse->strContent += "<input id=\"rules_name\" name=\"rules_name\" alt=\"[" + gmapLANG_CFG["rules_new_name"] + "]\" type=text size=96 maxlength=" + CAtomInt( MAX_FILENAME_LEN ).toString( ) + " value=\"" + UTIL_RemoveHTML( strRulesName ) + "\">\n</td>\n</tr>";
					pResponse->strContent += "<tr class=\"change_rules\">\n";
					pResponse->strContent += "<th class=\"change_rules\">" + gmapLANG_CFG["rules_new_content"] + "</th>\n";
					pResponse->strContent += "<td class=\"change_rules\">\n";
					pResponse->strContent += "<textarea id=\"rules_content\" name=\"rules_content\" rows=10 cols=96>" + UTIL_RemoveHTML3( strRules ) + "</textarea>\n</td>\n</tr>\n";
					pResponse->strContent += "<tr class=\"change_rules\">\n";
					pResponse->strContent += "<td class=\"change_rules\" colspan=\"2\">\n";
					pResponse->strContent += "<div class=\"change_rules_button\">\n";
					pResponse->strContent += Button_Submit( "submit_change", string( gmapLANG_CFG["rules_change"] ) );
					pResponse->strContent += Button_Back( "cancel_change", string( gmapLANG_CFG["cancel"] ) );
					pResponse->strContent += "\n</div>\n</td>\n</tr>\n";
					pResponse->strContent += "</form>\n</table>\n</div>\n\n";
//					pResponse->strContent += "<hr class=\"stats_hr\">\n\n";
				}
				else
				{
					if( strEdit == "0" )
					{
						const string cstrRulesUp = "rules" + CAtomInt( atoi( cstrRulesNum.c_str( ) ) - 1 ).toString( );
						UTIL_MoveFile( string( m_strRulesDir + rulesfile ).c_str( ), string( m_strRulesDir + rulesfile + ".temp" ).c_str( ) );
						UTIL_MoveFile( string( m_strRulesDir + rulesfile + ".name" ).c_str( ), string( m_strRulesDir + rulesfile + ".name.temp" ).c_str( ) );
						UTIL_MoveFile( string( m_strRulesDir + cstrRulesUp ).c_str( ), string( m_strRulesDir + rulesfile ).c_str( ) );
						UTIL_MoveFile( string( m_strRulesDir + cstrRulesUp + ".name" ).c_str( ), string( m_strRulesDir + rulesfile + ".name" ).c_str( ) );
						UTIL_MoveFile( string( m_strRulesDir + rulesfile + ".temp" ).c_str( ), string( m_strRulesDir + cstrRulesUp ).c_str( ) );
						UTIL_MoveFile( string( m_strRulesDir + rulesfile + ".name.temp" ).c_str( ), string( m_strRulesDir + cstrRulesUp + ".name" ).c_str( ) );
					}
					else
					{
						if( strEdit == "2" )
						{
							const string cstrRulesDown = "rules" + CAtomInt( atoi( cstrRulesNum.c_str( ) ) + 1 ).toString( );
							UTIL_MoveFile( string( m_strRulesDir + rulesfile ).c_str( ), string( m_strRulesDir + rulesfile + ".temp" ).c_str( ) );
							UTIL_MoveFile( string( m_strRulesDir + rulesfile + ".name" ).c_str( ), string( m_strRulesDir + rulesfile + ".name.temp" ).c_str( ) );
							UTIL_MoveFile( string( m_strRulesDir + cstrRulesDown ).c_str( ), string( m_strRulesDir + rulesfile ).c_str( ) );
							UTIL_MoveFile( string( m_strRulesDir + cstrRulesDown + ".name" ).c_str( ), string( m_strRulesDir + rulesfile + ".name" ).c_str( ) );
							UTIL_MoveFile( string( m_strRulesDir + rulesfile + ".temp" ).c_str( ), string( m_strRulesDir + cstrRulesDown ).c_str( ) );
							UTIL_MoveFile( string( m_strRulesDir + rulesfile + ".name.temp" ).c_str( ), string( m_strRulesDir + cstrRulesDown + ".name" ).c_str( ) );
						}
						else
						{
							pResponse->strContent += "<p class=\"changed_rules\">" + UTIL_Xsprintf( gmapLANG_CFG["rules_invalid_operation"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_rules"] + "\" href=\"" + RESPONSE_STR_RULES_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";
						}
					}
					pResponse->strContent += "<p class=\"changed_rules\">" + UTIL_Xsprintf( gmapLANG_CFG["rules_return"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_rules"] + "\" href=\"" + RESPONSE_STR_RULES_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";
				}
				// Output common HTML tail
				HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_RULES ) );
				
				return;
			}
		}
		else
		{
			// Not authorised

			// Output common HTML head
 			HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["rules_page"], string( CSS_RULES ), string( ), NOT_INDEX, CODE_401 );

			// Output common HTML tail
			HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_RULES ) );
		}
	}
	
	if( !pRequest->user.strUID.empty( ) && ( pRequest->user.ucAccess & m_ucAccessView ) )
	{
		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["rules_page"], string( CSS_RULES ), string( ), NOT_INDEX, CODE_200 );
		
		pResponse->strContent += "<table class=\"rules_main\"><tr><td class=\"rules_title\">";
		pResponse->strContent += "<p class=\"rulestitle\" id=\"top\">" + gmapLANG_CFG["rules_top"] + "\n</p></td></tr>";
		pResponse->strContent += "<tr><td class=\"rules_top\"><table class=\"rules_table\">\n";

		for( rulesnum = 0; rulesnum < rulescount; rulesnum++ )
		{
			rulesfile = "rules" + CAtomInt( rulesnum ).toString( );
			strRulesName = UTIL_ReadFile( string( m_strRulesDir + rulesfile + ".name" ).c_str( ) );
			pResponse->strContent += "<tr>\n";
			pResponse->strContent += "<td class=\"rules_name\"><a class=\"rules\" href=\"" + RESPONSE_STR_RULES_HTML + "#rules" + CAtomInt( rulesnum ).toString( ) + "\">" + UTIL_RemoveHTML( strRulesName ) + "</a></td>";
			if(  pRequest->user.ucAccess & m_ucAccessAdmin )
			{
				if( rulesnum == 0 )
				{
					if( rulesnum == rulescount - 1 )
						pResponse->strContent += "<td class=\"rules_admin\">[" + gmapLANG_CFG["move_up"] + "][<a href=\"" + RESPONSE_STR_RULES_HTML + "?rules=" + CAtomInt( rulesnum ).toString( ) + "&amp;edit=1\">" + gmapLANG_CFG["edit"] + "</a>][" + gmapLANG_CFG["move_down"] + "]</td>";
					else
						pResponse->strContent += "<td class=\"rules_admin\">[" + gmapLANG_CFG["move_up"] + "][<a href=\"" + RESPONSE_STR_RULES_HTML + "?rules=" + CAtomInt( rulesnum ).toString( ) + "&amp;edit=1\">" + gmapLANG_CFG["edit"] + "</a>][<a href=\"" + RESPONSE_STR_RULES_HTML + "?rules=" + CAtomInt( rulesnum ).toString( ) + "&amp;edit=2\">" + gmapLANG_CFG["move_down"] + "</a>]</td>";
				}
				else
					if( rulesnum == rulescount - 1 )
					{
// 						pResponse->strContent += "<td class=\"admin\">[<a href=\"" + RESPONSE_STR_RULES_HTML + "?post=" + CAtomInt( rulesnum ).toString( ) + "&amp;edit=0\">" + gmapLANG_CFG["move_up"] + "</a>][<a href=\"" + RESPONSE_STR_RULES_HTML + "?post=" + CAtomInt( rulesnum ).toString( ) + "&amp;edit=1\">" + gmapLANG_CFG["edit"] + "</a>][<a href=\"" + RESPONSE_STR_RULES_HTML + "?post=" + CAtomInt( rulesnum ).toString( ) + "&amp;edit=2\">" + gmapLANG_CFG["move_down"] + "</a>]</td>";
						pResponse->strContent += "<td class=\"rules_admin\">[<a href=\"" + RESPONSE_STR_RULES_HTML + "?rules=" + CAtomInt( rulesnum ).toString( ) + "&amp;edit=0\">" + gmapLANG_CFG["move_up"] + "</a>][<a href=\"" + RESPONSE_STR_RULES_HTML + "?rules=" + CAtomInt( rulesnum ).toString( ) + "&amp;edit=1\">" + gmapLANG_CFG["edit"] + "</a>][" + gmapLANG_CFG["move_down"] + "]</td>";
					}
					else
						pResponse->strContent += "<td class=\"rules_admin\">[<a href=\"" + RESPONSE_STR_RULES_HTML + "?rules=" + CAtomInt( rulesnum ).toString( ) + "&amp;edit=0\">" + gmapLANG_CFG["move_up"] + "</a>][<a href=\"" + RESPONSE_STR_RULES_HTML + "?rules=" + CAtomInt( rulesnum ).toString( ) + "&amp;edit=1\">" + gmapLANG_CFG["edit"] + "</a>][<a href=\"" + RESPONSE_STR_RULES_HTML + "?rules=" + CAtomInt( rulesnum ).toString( ) + "&amp;edit=2\">" + gmapLANG_CFG["move_down"] + "</a>]</td>";
			}
			pResponse->strContent += "</tr>\n";
		}
		if(  pRequest->user.ucAccess & m_ucAccessAdmin )
		{
			pResponse->strContent += "<tr>\n";
			pResponse->strContent += "<td class=\"rules_name\"></td><td class=\"rules_admin\">[<a href=\"" + RESPONSE_STR_RULES_HTML + "?rules=new\">" + gmapLANG_CFG["rules_new"] + "</a>][<a href=\"" + RESPONSE_STR_RULES_HTML + "?rules=del\">" + gmapLANG_CFG["rules_del"] + "</a>]</td>";
			pResponse->strContent += "</tr>\n";
		}
	
		pResponse->strContent += "</table></td></tr></table>\n\n";
	
		for( rulesnum = 0; rulesnum < rulescount; rulesnum++ )
		{
			rulesfile = "rules" + CAtomInt( rulesnum ).toString( );
			if( access( string( m_strRulesDir + rulesfile + ".name" ).c_str( ), 0 ) == 0 )
			{
// 					struct stat info;
				
				strRulesName = UTIL_ReadFile( string( m_strRulesDir + rulesfile + ".name" ).c_str( ) );

				strRules = UTIL_ReadFile( string( m_strRulesDir + rulesfile ).c_str( ) );
			}
			else
			{
				strRulesName = gmapLANG_CFG["new_rules"];
				strRules = gmapLANG_CFG["new_rules_content"];
			}
			
			
			
			pResponse->strContent += "<table class=\"rules_main\"><tr><td class=\"rules_title\">\n";
			pResponse->strContent += "<p class=\"rulestitle\" id=\"rules" + CAtomInt( rulesnum ).toString( ) + "\">" + UTIL_RemoveHTML( strRulesName );
			pResponse->strContent += " - <a class=\"rules\" href=\"" + RESPONSE_STR_RULES_HTML + "#top" + "\">" + gmapLANG_CFG["rules_top"] + "</a></p></td></tr>";
			if( !strRules.empty( ) )
			{
				pResponse->strContent += "<tr><td class=\"rules_table\"><table class=\"rules_table\" summary=\"file info\">\n";
				pResponse->strContent += "<tr class=\"rules_content\">";
				pResponse->strContent += "<td class=\"rules_content\">" + UTIL_RemoveHTML2( strRules ) + "</td>\n</tr>\n";

				pResponse->strContent += "</table></td></tr>\n";
			}
			pResponse->strContent += "</table>\n\n";
		}

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_RULES ) );
	}
	else
	{
		// Not authorised

		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["rules_page"], string( CSS_RULES ), string( ), NOT_INDEX, CODE_401 );

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_RULES ) );
	}

}

void CTracker :: serverResponseRulesPOST( struct request_t *pRequest, struct response_t *pResponse, CAtomList *pPost )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
		if( IsIPBanned( pRequest, pResponse, btv, gmapLANG_CFG["rules_page"], string( CSS_RULES ), NOT_INDEX ) )
			return;
	
	if( !pRequest->user.strUID.empty( ) && ( pRequest->user.ucAccess & m_ucAccessAdmin ) )
	{
		string cstrRulesNum = string( );
		string strNewRulesName = string( );
		string strNewRules = string( );
		
		if( pPost )
		{
			// Initialise segment dictionary
			CAtomDicti *pSegment = 0;
			CAtom *pDisposition = 0;
			CAtom *pData = 0;
			CAtom *pName = 0;
			CAtom *pFile = 0;
			// Get the segments from the post
			vector<CAtom *> vecSegments = pPost->getValue( );

			// Loop through the segments
			for( unsigned long ulCount = 0; ulCount < vecSegments.size( ); ulCount++ )
			{
				// Is the segment a dictionary?
				if( vecSegments[ulCount]->isDicti( ) )
				{
					// Get the segment dictionary
					pSegment = (CAtomDicti *)vecSegments[ulCount];
					// Get the disposition and the data from the segment dictionary
					pDisposition = pSegment->getItem( "disposition" );
					pData = pSegment->getItem( "data" );

					// Did we get a disposition that is a dictionary and has data?
					if( pDisposition && pDisposition->isDicti( ) && pData )
					{
						// Get the content name from the disposition
						pName = ( (CAtomDicti *)pDisposition )->getItem( "name" );

						// Did we get a content name?
						if( pName )
						{
							// What is the content name to be tested?
							string strName = pName->toString( );
							
							if( strName == "rules" )
								cstrRulesNum = pData->toString( );
							else if( strName == "rules_name" )
								strNewRulesName = pData->toString( );
							else if( strName == "rules_content" )
								strNewRules = pData->toString( );
						}
						else
						{
							// Output common HTML head
							HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["rules_page"], string( CSS_RULES ), string( ), NOT_INDEX, CODE_400 );

							// failed
							pResponse->strContent += "<p class=\"failed\">" + gmapLANG_CFG["failed"] + "</p>\n";
							// Signal a bad request
							pResponse->strContent += "<p class=\"body_upload\">400 " + gmapLANG_CFG["server_response_400"] + "</p>\n";

							// Output common HTML tail
							HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_RULES ) );

							if( gbDebug )
								UTIL_LogPrint( "Upload Warning - Bad request (no torrent name)\n" );

							return;
						}
					}
				}
			}
		}
		else
		{
			// Output common HTML head
			HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["rules_page"], string( CSS_RULES ), string( ), NOT_INDEX, CODE_400 );

			// failed
			pResponse->strContent += "<p class=\"failed\">" + gmapLANG_CFG["failed"] + "</p>\n";
			// Signal a bad request
			pResponse->strContent += "<p class=\"body_upload\">400 " + gmapLANG_CFG["server_response_400"] + "</p>\n";

			// Output common HTML tail
			HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_RULES ) );

			if( gbDebug )
				UTIL_LogPrint( "Upload Warning - Bad request (no post received)\n" );

			return;
		}
		if( !strNewRulesName.empty( ) )
		{
			if( pRequest->user.ucAccess & m_ucAccessAdmin )
			{
				HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["rules_page"], string( CSS_RULES ), string( ), NOT_INDEX, CODE_200 );
				
				string rulesfile( "rules" + cstrRulesNum );
				UTIL_MakeFile( ( string( m_strRulesDir + rulesfile + ".name" ).c_str( ) ), strNewRulesName );
				UTIL_MakeFile( ( string( m_strRulesDir + rulesfile ).c_str( ) ), strNewRules );
				
				pResponse->strContent += "<div class=\"changed_rules\">\n";
				pResponse->strContent += "<table class=\"changed_rules\">\n";
				pResponse->strContent += "<tr>\n<td>\n<ul>\n";
				if( !strNewRulesName.empty( ) )
					pResponse->strContent += "<li class=\"changed_rules\">" + UTIL_Xsprintf( gmapLANG_CFG["rules_changed_name"].c_str( ), UTIL_RemoveHTML( strNewRulesName ).c_str( ) ) + "</li>\n";
				pResponse->strContent += "</ul>\n</td>\n</tr>\n</table>\n</div>\n";
				
				pResponse->strContent += "<p class=\"changed_rules\">" + UTIL_Xsprintf( gmapLANG_CFG["rules_return"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_rules"] + "\" href=\"" + RESPONSE_STR_RULES_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";
				
				HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_RULES ) );
				
				return;
			}
		}
	}
}
