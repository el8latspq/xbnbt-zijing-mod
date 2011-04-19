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

#include "bnbt.h"
#include "bnbt_mysql.h"
#include "atom.h"
#include "config.h"
#include "html.h"
#include "md5.h"
#include "tracker.h"
#include "util.h"

void CTracker :: serverResponseSignupLilyGET( struct request_t *pRequest, struct response_t *pResponse )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
		if( IsIPBanned( pRequest, pResponse, btv, gmapLANG_CFG["signup_page"], string( CSS_SIGNUP ), NOT_INDEX ) )
			return;

#if defined ( XBNBT_MYSQL )
	// XBNBT MySQL Users Integration
	if( gbMySQLUsersOverrideUsers )
	{
		// redirect
		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["signup_page"], string( CSS_SIGNUP ), string( m_strForumLink ), NOT_INDEX, CODE_200 );
	}
	else
	{
#endif

		if( pRequest->user.ucAccess & ACCESS_SIGNUP )
		{
			HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["signup_page"], string( CSS_SIGNUP ), string( ), NOT_INDEX, CODE_200 );


			pResponse->strContent += "<div class=\"signup_form\">\n";
			pResponse->strContent += "<table class=\"signup_form\">\n";
			pResponse->strContent += "<tr>\n";
			pResponse->strContent += "<td>\n";
			pResponse->strContent += "<form name=\"us_details\" method=\"post\" action=\"" + RESPONSE_STR_SIGNUP_LILY_HTML + "\" enctype=\"multipart/form-data\">\n";
			pResponse->strContent += "<p class=\"signup_form_header\">" + gmapLANG_CFG["signup"] + "</p>\n";
			pResponse->strContent += "<ul>\n";

			// Names must be less than %s characters long
			pResponse->strContent += "<li class=\"signup\">" + UTIL_Xsprintf( gmapLANG_CFG["signup_info_name_length"].c_str( ), CAtomInt( m_uiNameLength ).toString( ).c_str( ) ) + "</li>\n";

			pResponse->strContent += "<li class=\"signup\">" + gmapLANG_CFG["signup_info_case"] + "</li>\n";
			pResponse->strContent += "<li class=\"signup\">" + gmapLANG_CFG["signup_info_lily"] + "</li>\n";
			pResponse->strContent += "<li class=\"signup\">" + gmapLANG_CFG["signup_info_lily_1"] + "</li>\n";
			pResponse->strContent += "<li class=\"signup\">" + gmapLANG_CFG["no_html"] + "</li>\n";
			pResponse->strContent += "</ul>\n";
			pResponse->strContent += "<div class=\"signup_input\">\n";
			pResponse->strContent += "<p class=\"signup_form\"><input id=\"login\" name=\"us_login\" alt=\"[" + gmapLANG_CFG["signup_name"] + "]\" type=text size=24 maxlength=" + CAtomInt( m_uiNameLength ).toString( ) + "> <label for=\"signup_name\">" + gmapLANG_CFG["signup_name"] + "</label></p>\n";
			pResponse->strContent += "<p class=\"signup_form\"><input id=\"lilyid\" name=\"us_lilyid\" alt=\"[" + gmapLANG_CFG["lilyid"] + "]\" type=text size=24 maxlength=" + CAtomInt( m_uiNameLength ).toString( ) + "> <label for=\"lilyid\">" + gmapLANG_CFG["lilyid"] + "</label></p>\n";

			pResponse->strContent += "<p class=\"signup_form_button\">";
			pResponse->strContent += Button_Submit( "submit_signup", string( gmapLANG_CFG["signup"] ) );
			pResponse->strContent += "</p>\n";
			pResponse->strContent += "</div>\n\n";
			pResponse->strContent += "</form>\n";
			pResponse->strContent += "</td>\n";
			pResponse->strContent += "</tr>\n";
			pResponse->strContent += "</table>\n";
			pResponse->strContent += "</div>\n\n";
		}
		else
			// Not Authorised
			// Output common HTML head
			HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["signup_page"], string( CSS_SIGNUP ), string( ), NOT_INDEX, CODE_401 );

#if defined ( XBNBT_MYSQL )
	// XBNBT MySQL Users Integration
	}
#endif

	// Output common HTML tail
	HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_SIGNUP ) );
}

void CTracker :: serverResponseSignupLilyPOST( struct request_t *pRequest, struct response_t *pResponse, CAtomList *pPost )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
		if( IsIPBanned( pRequest, pResponse, btv, gmapLANG_CFG["signup_page"], string( CSS_SIGNUP ), NOT_INDEX ) )
			return;
		
	string cstrLogin = string( );
	string cstrLilyID = string( );

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
						
						if( strName == "us_login")
							cstrLogin = pData->toString( );
						else if( strName == "us_lilyid")
							cstrLilyID = UTIL_ToLower( pData->toString( ) );
					}
					else
					{
						// Output common HTML head
						HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["signup_page"], string( CSS_SIGNUP ), string( ), NOT_INDEX, CODE_400 );

						// failed
						pResponse->strContent += "<p class=\"failed\">" + gmapLANG_CFG["failed"] + "</p>\n";
						// Signal a bad request
						pResponse->strContent += "<p class=\"body_upload\">400 " + gmapLANG_CFG["server_response_400"] + "</p>\n";

						// Output common HTML tail
						HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_SIGNUP ) );

						if( gbDebug )
							UTIL_LogPrint( "Login Warning - Bad request (no users name)\n" );

						return;
					}
				}
			}
		}
	}
	else
	{
		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["signup_page"], string( CSS_SIGNUP ), string( ), NOT_INDEX, CODE_400 );

		// failed
		pResponse->strContent += "<p class=\"failed\">" + gmapLANG_CFG["failed"] + "</p>\n";
		// Signal a bad request
		pResponse->strContent += "<p class=\"body_upload\">400 " + gmapLANG_CFG["server_response_400"] + "</p>\n";

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_SIGNUP ) );

		if( gbDebug )
			UTIL_LogPrint( "Upload Warning - Bad request (no post received)\n" );

		return;
	}
	
	if( pRequest->user.ucAccess & ACCESS_SIGNUP )
	{
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["signup_page"], string( CSS_SIGNUP ), string( ), NOT_INDEX, CODE_200 );

		if(  !cstrLogin.empty( ) && !cstrLilyID.empty( ) )
		{
// 			if( cstrLogin[0] == ' ' || cstrLogin[cstrLogin.size( ) - 1] == ' ' || cstrLogin.size( ) > m_uiNameLength )
			if( cstrLogin.find( ' ' ) != string :: npos || cstrLogin.find( '.' ) != string :: npos || cstrLogin.find( '%' ) != string :: npos || cstrLogin.find( '&' ) != string :: npos || cstrLilyID.find( ' ' ) != string :: npos || cstrLilyID.find( '.' ) != string :: npos || cstrLilyID.find( '%' ) != string :: npos || cstrLilyID.find( '&' ) != string :: npos || cstrLogin.size( ) > m_uiNameLength )
			{
				// Unable to signup. Your name must be less than " + CAtomInt( m_uiNameLength ).toString( ) + " characters long and it must not start or end with spaces.
				pResponse->strContent += "<p class=\"signup_failed\">" + UTIL_Xsprintf( gmapLANG_CFG["signup_name_error"].c_str( ), CAtomInt( m_uiNameLength ).toString( ).c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_signup"] + "\" href=\"" + RESPONSE_STR_SIGNUP_LILY_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n\n";

				// Output common HTML tail
				HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_SIGNUP ) );

				return;
			}
			time_t tNow = time( 0 );
			char pTime[256];
			memset( pTime, 0, sizeof( pTime ) / sizeof( char ) );
			strftime( pTime, sizeof( pTime ) / sizeof( char ), "%Y %m/%d %H:%M:%S", localtime( &tNow ) );
			
			const string cstrA1( cstrLogin + ":" + gstrRealm + ":" + pTime );

			unsigned char szMD5[16];
			memset( szMD5, 0, sizeof( szMD5 ) / sizeof( unsigned char ) );

			MD5_CTX md5;

			MD5Init( &md5 );
			MD5Update( &md5, (const unsigned char *)cstrA1.c_str( ), (unsigned int)cstrA1.size( ) );
			MD5Final( szMD5, &md5 );
			
			const string cstrPass = UTIL_HashToString( string( (char *)szMD5, sizeof( szMD5 ) / sizeof( unsigned char ) ) ).substr( 0, 8 );
			const string cstrMail = cstrLilyID + "@bbs.nju.edu.cn";

			if( !getUser( cstrLogin, m_ucGuestAccess ).strLogin.empty( ) )
			{
				// Unable to signup. The user \"" + UTIL_RemoveHTML( cstrLogin ) + "\" already exists.
				pResponse->strContent += "<p class=\"signup_failed\">" + UTIL_Xsprintf( gmapLANG_CFG["signup_exists_error"].c_str( ), UTIL_RemoveHTML( cstrLogin ).c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_signup"] + "\" href=\"" + RESPONSE_STR_SIGNUP_LILY_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n\n";
			}
			else
			{
				CMySQLQuery *pQuery = new CMySQLQuery( "SELECT blilyid FROM lily WHERE blilyid=\'" + UTIL_StringToMySQL( cstrLilyID ) + "\'" );
				
				if( pQuery->nextRow( ).size( ) == 1 )
				{
					pResponse->strContent += "<p class=\"signup_failed\">" + UTIL_Xsprintf( gmapLANG_CFG["signup_exists_error_lily"].c_str( ), UTIL_RemoveHTML( cstrLogin ).c_str( ), UTIL_RemoveHTML( cstrLilyID ).c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_signup"] + "\" href=\"" + RESPONSE_STR_SIGNUP_LILY_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n\n";
				}
				else
				{
					if( addUser( cstrLogin, cstrPass, m_ucMemberAccess, cstrMail ) )
					{
						CMySQLQuery mq01( "INSERT INTO lily (blilyid,busername,bpassword) VALUES(\'" + UTIL_StringToMySQL( cstrLilyID ) + "\',\'" + UTIL_StringToMySQL( cstrLogin ) + "\',\'" + UTIL_StringToMySQL( cstrPass ) + "\')" );
						
						system( string( "echo \"ID: " + cstrLogin + " Password: " + cstrPass + "\" | mutt -s \"ZiJingBT\" " + cstrMail ).c_str( ) );
	
						// Thanks! You've successfully signed up!
						pResponse->strContent += "<p class=\"signup_ok\">" + UTIL_Xsprintf( gmapLANG_CFG["signup_success_lily"].c_str( ), UTIL_RemoveHTML( cstrLilyID ).c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_signup"] + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n\n";
					}
					else
						pResponse->strContent += "<p class=\"signup_failed\">" + UTIL_Xsprintf( gmapLANG_CFG["users_max_create_fail"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_users"] + "\" href=\"" + RESPONSE_STR_USERS_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";
				}
				delete pQuery;
			}
			
			HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_SIGNUP ) );
		}
	}
	else
	{
		//Unable to signup. You must fill in all the fields.
		pResponse->strContent += "<p class=\"signup_failed\">" + UTIL_Xsprintf( gmapLANG_CFG["signup_fields_error"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_signup"] + "\" href=\"" + RESPONSE_STR_SIGNUP_LILY_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n\n";

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_SIGNUP ) );

		return;
	}
		
}