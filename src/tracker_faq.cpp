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

void CTracker :: serverResponseFAQGET( struct request_t *pRequest, struct response_t *pResponse )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
		if( IsIPBanned( pRequest, pResponse, btv, gmapLANG_CFG["faq_page"], string( CSS_FAQ ), NOT_INDEX ) )
			return;
		
	int64 faqcount=0;
	int64 faqnum;
	string faqfile;
	string faqname;
	string strFAQName;
	string strFAQ;
	faqfile = "faq" + CAtomInt( faqcount ).toString( );
	const string cstrFAQNum( pRequest->mapParams["faq"] );
	while( access( string( m_strFAQDir + faqfile + ".name" ).c_str( ), 0 ) == 0 )
	{							
		faqcount++;
		faqfile = "faq" + CAtomInt( faqcount ).toString( );
	}
	
	if( !cstrFAQNum.empty( ) )
	{
		if( !pRequest->user.strUID.empty( ) && ( pRequest->user.ucAccess & m_ucAccessAdmin ) )
		{
			// Output common HTML head
			HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["faq_page"], string( CSS_FAQ ), string( ), NOT_INDEX, CODE_200 );
			
			if( cstrFAQNum == "new" )
			{
				faqfile = "faq" + CAtomInt( faqcount ).toString( );
				UTIL_MakeFile( string( m_strFAQDir + faqfile + ".name" ).c_str( ), gmapLANG_CFG["new_faq"] );
				UTIL_MakeFile( string( m_strFAQDir + faqfile ).c_str( ), gmapLANG_CFG["new_faq_content"] );
				pResponse->strContent += "<p class=\"changed_faq\">" + UTIL_Xsprintf( gmapLANG_CFG["faq_return"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_faq"] + "\" href=\"" + RESPONSE_STR_FAQ_HTML + "?faq=" + CAtomInt( faqcount ).toString( ) + "&amp;edit=1\">" ).c_str( ), "</a>" ) + "</p>\n";
				// Output common HTML tail
				HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_FAQ ) );
				
				return;
			}
			if( cstrFAQNum == "del" )
			{
				faqfile = "faq" + CAtomInt( faqcount - 1 ).toString( );
				UTIL_DeleteFile( string( m_strFAQDir + faqfile + ".name" ).c_str( ) );
				UTIL_DeleteFile( string( m_strFAQDir + faqfile ).c_str( ) );
				pResponse->strContent += "<p class=\"changed_faq\">" + UTIL_Xsprintf( gmapLANG_CFG["faq_return"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_faq"] + "\" href=\"" + RESPONSE_STR_FAQ_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";
				// Output common HTML tail
				HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_FAQ ) );
				
				return;
			}
			
// 			faqfile = "faq" + cstrFAQNum;
// 			if( access( string( m_strFAQDir + faqfile + ".name" ).c_str( ), 0 ) == 0 )
// 			{
// 				struct stat info;
// 				
// 				strFAQName = UTIL_ReadFile( string( m_strFAQDir + faqfile + ".name" ).c_str( ) );

// 				memset( strPostTime, 0, sizeof( strPostTime ) / sizeof( char ) );
// 				stat( string( m_strFAQDir + faqfile ).c_str( ), &info );
// 				strftime( strPostTime, sizeof( strPostTime ) / sizeof( char ), "%Y-%m-%d %H:%M:%S", localtime( &info.st_mtime ) );

// 				strFAQ = UTIL_ReadFile( string( m_strFAQDir + faqfile ).c_str( ) );
// 			}
			if( pRequest->mapParams.find( "edit" ) != pRequest->mapParams.end( ) )
			{
				const string strEdit( pRequest->mapParams["edit"] );
				faqfile = "faq" + cstrFAQNum;
				if( access( string( m_strFAQDir + faqfile + ".name" ).c_str( ), 0 ) == 0 )
				{
					strFAQName = UTIL_ReadFile( string( m_strFAQDir + faqfile + ".name" ).c_str( ) );
					strFAQ = UTIL_ReadFile( string( m_strFAQDir + faqfile ).c_str( ) );
				}
				else
				{
					strFAQName = gmapLANG_CFG["new_faq"];
					strFAQ = gmapLANG_CFG["new_faq_content"];
				}
				if( strEdit == "1" )
				{
					pResponse->strContent += "<div class=\"change_faq\">\n";
					pResponse->strContent += "<p class=\"change_faq\">" + gmapLANG_CFG["change_faq"] + "</p>\n";
					pResponse->strContent += "<p class=\"change_faq\">" + UTIL_RemoveHTML( strFAQName ) + "</p>\n";
					pResponse->strContent += "<table class=\"change_faq\">\n";
					pResponse->strContent += "<form method=\"post\" action=\"" + RESPONSE_STR_FAQ_HTML + "\" enctype=\"multipart/form-data\">\n";
					
					pResponse->strContent += "<input name=\"faq\" type=hidden value=\"" + cstrFAQNum + "\">\n";
					pResponse->strContent += "<tr class=\"change_faq\">\n";
					pResponse->strContent += "<th class=\"change_faq\">" + gmapLANG_CFG["faq_new_name"] + "</th>\n";
					pResponse->strContent += "<td class=\"change_faq\">\n";
					pResponse->strContent += "<input id=\"faq_name\" name=\"faq_name\" alt=\"[" + gmapLANG_CFG["faq_new_name"] + "]\" type=text size=96 maxlength=" + CAtomInt( MAX_FILENAME_LEN ).toString( ) + " value=\"" + UTIL_RemoveHTML( strFAQName ) + "\">\n</td>\n</tr>";
					pResponse->strContent += "<tr class=\"change_faq\">\n";
					pResponse->strContent += "<th class=\"change_faq\">" + gmapLANG_CFG["faq_new_content"] + "</th>\n";
					pResponse->strContent += "<td class=\"change_faq\">\n";
					pResponse->strContent += "<textarea id=\"faq_content\" name=\"faq_content\" rows=10 cols=96>" + UTIL_RemoveHTML3( strFAQ ) + "</textarea>\n</td>\n</tr>\n";
					pResponse->strContent += "<tr class=\"change_faq\">\n";
					pResponse->strContent += "<td class=\"change_faq\" colspan=\"2\">\n";
					pResponse->strContent += "<div class=\"change_faq_button\">\n";
					pResponse->strContent += Button_Submit( "submit_change", string( gmapLANG_CFG["faq_change"] ) );
					pResponse->strContent += Button_Back( "cancel_change", string( gmapLANG_CFG["cancel"] ) );
					pResponse->strContent += "\n</div>\n</td>\n</tr>\n";
					pResponse->strContent += "</form>\n</table>\n</div>\n\n";
//					pResponse->strContent += "<hr class=\"stats_hr\">\n\n";
				}
				else
				{
					if( strEdit == "0" )
					{
						const string cstrFAQUp = "faq" + CAtomInt( atoi( cstrFAQNum.c_str( ) ) - 1 ).toString( );
						UTIL_MoveFile( string( m_strFAQDir + faqfile ).c_str( ), string( m_strFAQDir + faqfile + ".temp" ).c_str( ) );
						UTIL_MoveFile( string( m_strFAQDir + faqfile + ".name" ).c_str( ), string( m_strFAQDir + faqfile + ".name.temp" ).c_str( ) );
						UTIL_MoveFile( string( m_strFAQDir + cstrFAQUp ).c_str( ), string( m_strFAQDir + faqfile ).c_str( ) );
						UTIL_MoveFile( string( m_strFAQDir + cstrFAQUp + ".name" ).c_str( ), string( m_strFAQDir + faqfile + ".name" ).c_str( ) );
						UTIL_MoveFile( string( m_strFAQDir + faqfile + ".temp" ).c_str( ), string( m_strFAQDir + cstrFAQUp ).c_str( ) );
						UTIL_MoveFile( string( m_strFAQDir + faqfile + ".name.temp" ).c_str( ), string( m_strFAQDir + cstrFAQUp + ".name" ).c_str( ) );
					}
					else
					{
						if( strEdit == "2" )
						{
							const string cstrFAQDown = "faq" + CAtomInt( atoi( cstrFAQNum.c_str( ) ) + 1 ).toString( );
							UTIL_MoveFile( string( m_strFAQDir + faqfile ).c_str( ), string( m_strFAQDir + faqfile + ".temp" ).c_str( ) );
							UTIL_MoveFile( string( m_strFAQDir + faqfile + ".name" ).c_str( ), string( m_strFAQDir + faqfile + ".name.temp" ).c_str( ) );
							UTIL_MoveFile( string( m_strFAQDir + cstrFAQDown ).c_str( ), string( m_strFAQDir + faqfile ).c_str( ) );
							UTIL_MoveFile( string( m_strFAQDir + cstrFAQDown + ".name" ).c_str( ), string( m_strFAQDir + faqfile + ".name" ).c_str( ) );
							UTIL_MoveFile( string( m_strFAQDir + faqfile + ".temp" ).c_str( ), string( m_strFAQDir + cstrFAQDown ).c_str( ) );
							UTIL_MoveFile( string( m_strFAQDir + faqfile + ".name.temp" ).c_str( ), string( m_strFAQDir + cstrFAQDown + ".name" ).c_str( ) );
						}
						else
						{
							pResponse->strContent += "<p class=\"changed_faq\">" + UTIL_Xsprintf( gmapLANG_CFG["faq_invalid_operation"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_faq"] + "\" href=\"" + RESPONSE_STR_FAQ_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";
						}
					}
					pResponse->strContent += "<p class=\"changed_faq\">" + UTIL_Xsprintf( gmapLANG_CFG["faq_return"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_faq"] + "\" href=\"" + RESPONSE_STR_FAQ_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";
				}
				// Output common HTML tail
				HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_FAQ ) );
				
				return;
			}
		}
		else
		{
			// Not authorised

			// Output common HTML head
 			HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["faq_page"], string( CSS_FAQ ), string( ), NOT_INDEX, CODE_401 );

			// Output common HTML tail
			HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_FAQ ) );
			
			return;
		}
	}
	
	if( !pRequest->user.strUID.empty( ) && ( pRequest->user.ucAccess & m_ucAccessView ) )
	{
		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["faq_page"], string( CSS_FAQ ), string( ), NOT_INDEX, CODE_200 );
		
		pResponse->strContent += "<table class=\"faq_main\"><tr><td class=\"faq_title\">";
		pResponse->strContent += "<p class=\"faqtitle\" id=\"top\">" + gmapLANG_CFG["faq_top"] + "\n</p></td></tr>";
		pResponse->strContent += "<tr><td class=\"faq_top\"><table class=\"faq_table\">\n";

		for( faqnum = 0; faqnum < faqcount; faqnum++ )
		{
			faqfile = "faq" + CAtomInt( faqnum ).toString( );
			strFAQName = UTIL_ReadFile( string( m_strFAQDir + faqfile + ".name" ).c_str( ) );
			pResponse->strContent += "<tr>\n";
			pResponse->strContent += "<td class=\"faq_name\"><a class=\"faq\" href=\"" + RESPONSE_STR_FAQ_HTML + "#faq" + CAtomInt( faqnum ).toString( ) + "\">" + UTIL_RemoveHTML( strFAQName ) + "</a></td>";
			if(  pRequest->user.ucAccess & m_ucAccessAdmin )
			{
				if( faqnum == 0 )
				{
					if( faqnum == faqcount - 1 )
						pResponse->strContent += "<td class=\"faq_admin\">[" + gmapLANG_CFG["move_up"] + "][<a href=\"" + RESPONSE_STR_FAQ_HTML + "?faq=" + CAtomInt( faqnum ).toString( ) + "&amp;edit=1\">" + gmapLANG_CFG["edit"] + "</a>][" + gmapLANG_CFG["move_down"] + "]</td>";
					else
						pResponse->strContent += "<td class=\"faq_admin\">[" + gmapLANG_CFG["move_up"] + "][<a href=\"" + RESPONSE_STR_FAQ_HTML + "?faq=" + CAtomInt( faqnum ).toString( ) + "&amp;edit=1\">" + gmapLANG_CFG["edit"] + "</a>][<a href=\"" + RESPONSE_STR_FAQ_HTML + "?faq=" + CAtomInt( faqnum ).toString( ) + "&amp;edit=2\">" + gmapLANG_CFG["move_down"] + "</a>]</td>";
				}
				else
					if( faqnum == faqcount - 1 )
					{
// 						pResponse->strContent += "<td class=\"admin\">[<a href=\"" + RESPONSE_STR_FAQ_HTML + "?post=" + CAtomInt( faqnum ).toString( ) + "&amp;edit=0\">" + gmapLANG_CFG["move_up"] + "</a>][<a href=\"" + RESPONSE_STR_FAQ_HTML + "?post=" + CAtomInt( faqnum ).toString( ) + "&amp;edit=1\">" + gmapLANG_CFG["edit"] + "</a>][<a href=\"" + RESPONSE_STR_FAQ_HTML + "?post=" + CAtomInt( faqnum ).toString( ) + "&amp;edit=2\">" + gmapLANG_CFG["move_down"] + "</a>]</td>";
						pResponse->strContent += "<td class=\"faq_admin\">[<a href=\"" + RESPONSE_STR_FAQ_HTML + "?faq=" + CAtomInt( faqnum ).toString( ) + "&amp;edit=0\">" + gmapLANG_CFG["move_up"] + "</a>][<a href=\"" + RESPONSE_STR_FAQ_HTML + "?faq=" + CAtomInt( faqnum ).toString( ) + "&amp;edit=1\">" + gmapLANG_CFG["edit"] + "</a>][" + gmapLANG_CFG["move_down"] + "]</td>";
					}
					else
						pResponse->strContent += "<td class=\"faq_admin\">[<a href=\"" + RESPONSE_STR_FAQ_HTML + "?faq=" + CAtomInt( faqnum ).toString( ) + "&amp;edit=0\">" + gmapLANG_CFG["move_up"] + "</a>][<a href=\"" + RESPONSE_STR_FAQ_HTML + "?faq=" + CAtomInt( faqnum ).toString( ) + "&amp;edit=1\">" + gmapLANG_CFG["edit"] + "</a>][<a href=\"" + RESPONSE_STR_FAQ_HTML + "?faq=" + CAtomInt( faqnum ).toString( ) + "&amp;edit=2\">" + gmapLANG_CFG["move_down"] + "</a>]</td>";
			}
			pResponse->strContent += "</tr>\n";
		}
		if(  pRequest->user.ucAccess & m_ucAccessAdmin )
		{
			pResponse->strContent += "<tr>\n";
			pResponse->strContent += "<td class=\"faq_name\"></td><td class=\"faq_admin\">[<a href=\"" + RESPONSE_STR_FAQ_HTML + "?faq=new\">" + gmapLANG_CFG["faq_new"] + "</a>][<a href=\"" + RESPONSE_STR_FAQ_HTML + "?faq=del\">" + gmapLANG_CFG["faq_del"] + "</a>]</td>";
			pResponse->strContent += "</tr>\n";
		}
	
		pResponse->strContent += "</table></td></tr></table>\n\n";
	
		for( faqnum = 0; faqnum < faqcount; faqnum++ )
		{
			faqfile = "faq" + CAtomInt( faqnum ).toString( );
			if( access( string( m_strFAQDir + faqfile + ".name" ).c_str( ), 0 ) == 0 )
			{
				strFAQName = UTIL_ReadFile( string( m_strFAQDir + faqfile + ".name" ).c_str( ) );

				strFAQ = UTIL_ReadFile( string( m_strFAQDir + faqfile ).c_str( ) );
			}
			else
			{
				strFAQName = gmapLANG_CFG["new_faq"];
				strFAQ = gmapLANG_CFG["new_faq_content"];
			}
			
			pResponse->strContent += "<table class=\"faq_main\"><tr><td class=\"faq_title\">\n";
			pResponse->strContent += "<p class=\"faqtitle\" id=\"faq" + CAtomInt( faqnum ).toString( ) + "\">" + UTIL_RemoveHTML( strFAQName );
			pResponse->strContent += " - <a class=\"faq\" href=\"" + RESPONSE_STR_FAQ_HTML + "#top" + "\">" + gmapLANG_CFG["faq_top"] + "</a></p></td></tr>";
			if( !strFAQ.empty( ) )
			{
				pResponse->strContent += "<tr><td class=\"faq_table\"><table class=\"faq_table\" summary=\"file info\">\n";
				pResponse->strContent += "<tr class=\"faq_content\">";
				pResponse->strContent += "<td class=\"faq_content\">" + UTIL_RemoveHTML2( strFAQ ) + "</td>\n</tr>\n";

				pResponse->strContent += "</table></td></tr>\n";
			}
			pResponse->strContent += "</table>\n\n";
		}

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_FAQ ) );
	}
	else
	{
		// Not authorised

		// Output common HTML head
		HTML_Common_Begin( pRequest, pResponse, gmapLANG_CFG["faq_page"], string( CSS_FAQ ), string( ), NOT_INDEX, CODE_401 );

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_FAQ ) );
	}
}

void CTracker :: serverResponseFAQPOST( struct request_t *pRequest, struct response_t *pResponse, CAtomList *pPost )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
		if( IsIPBanned( pRequest, pResponse, btv, gmapLANG_CFG["faq_page"], string( CSS_FAQ ), NOT_INDEX ) )
			return;
	
	if( !pRequest->user.strUID.empty( ) && ( pRequest->user.ucAccess & m_ucAccessAdmin ) )
	{
		string cstrFAQNum = string( );
		string strNewFAQName = string( );
		string strNewFAQ = string( );
		
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
							
							if( strName == "faq" )
								cstrFAQNum = pData->toString( );
							else if( strName == "faq_name" )
								strNewFAQName = pData->toString( );
							else if( strName == "faq_content" )
								strNewFAQ = pData->toString( );
						}
						else
						{
							// Output common HTML head
							HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["faq_page"], string( CSS_FAQ ), string( ), NOT_INDEX, CODE_400 );

							// failed
							pResponse->strContent += "<p class=\"failed\">" + gmapLANG_CFG["failed"] + "</p>\n";
							// Signal a bad request
							pResponse->strContent += "<p class=\"body_upload\">400 " + gmapLANG_CFG["server_response_400"] + "</p>\n";

							// Output common HTML tail
							HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_FAQ ) );

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
			HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["faq_page"], string( CSS_FAQ ), string( ), NOT_INDEX, CODE_400 );

			// failed
			pResponse->strContent += "<p class=\"failed\">" + gmapLANG_CFG["failed"] + "</p>\n";
			// Signal a bad request
			pResponse->strContent += "<p class=\"body_upload\">400 " + gmapLANG_CFG["server_response_400"] + "</p>\n";

			// Output common HTML tail
			HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_FAQ ) );

			if( gbDebug )
				UTIL_LogPrint( "Upload Warning - Bad request (no post received)\n" );

			return;
		}
		if( !strNewFAQName.empty( ) )
		{
			if( pRequest->user.ucAccess & m_ucAccessAdmin )
			{
				HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["faq_page"], string( CSS_FAQ ), string( ), NOT_INDEX, CODE_200 );
				
				string faqfile( "faq" + cstrFAQNum );
				UTIL_MakeFile( ( string( m_strFAQDir + faqfile + ".name" ).c_str( ) ), strNewFAQName );
				UTIL_MakeFile( ( string( m_strFAQDir + faqfile ).c_str( ) ), strNewFAQ );
				
				pResponse->strContent += "<div class=\"changed_faq\">\n";
				pResponse->strContent += "<table class=\"changed_faq\">\n";
				pResponse->strContent += "<tr>\n<td>\n<ul>\n";
				if( !strNewFAQName.empty( ) )
					pResponse->strContent += "<li class=\"changed_faq\">" + UTIL_Xsprintf( gmapLANG_CFG["faq_changed_name"].c_str( ), UTIL_RemoveHTML( strNewFAQName ).c_str( ) ) + "</li>\n";
				pResponse->strContent += "</ul>\n</td>\n</tr>\n</table>\n</div>\n";
				
				pResponse->strContent += "<p class=\"changed_faq\">" + UTIL_Xsprintf( gmapLANG_CFG["faq_return"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_faq"] + "\" href=\"" + RESPONSE_STR_FAQ_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";
				
				HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_FAQ ) );
				
				return;
			}
		}
	}
}
