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

void CTracker :: serverResponseSignupGET( struct request_t *pRequest, struct response_t *pResponse )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
		if( IsIPBanned( pRequest, pResponse, btv, gmapLANG_CFG["signup_page"], string( CSS_SIGNUP ), NOT_INDEX ) )
			return;

// 		if( pRequest->user.ucAccess & ACCESS_SIGNUP )
	if( pRequest->user.ucAccess & m_ucAccessSignupDirect )
	{
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["signup_page"], string( CSS_SIGNUP ), string( ), NOT_INDEX, CODE_200 );

		pResponse->strContent += "<div class=\"signup_form\">\n";
		pResponse->strContent += "<table class=\"signup_form\">\n";
		pResponse->strContent += "<form name=\"us_details\" method=\"post\" action=\"" + RESPONSE_STR_SIGNUP_HTML + "\" enctype=\"multipart/form-data\">\n";
		pResponse->strContent += "<tr class=\"signup_form\">\n";
		pResponse->strContent += "<th class=\"signup_form\" colspan=\"2\">\n" + gmapLANG_CFG["signup"] + "</th>\n</tr>\n";
		
		pResponse->strContent += "<tr class=\"signup_form\">\n";
		pResponse->strContent += "<th class=\"signup_form\">\n" + gmapLANG_CFG["signup_name"] + "</th>\n";
		pResponse->strContent += "<td class=\"signup_form\">\n";
		pResponse->strContent += "<input id=\"login\" name=\"us_login\" alt=\"[" + gmapLANG_CFG["signup_name"] + "]\" type=text size=24 maxlength=" + CAtomInt( m_uiNameLength ).toString( ) + ">\n";
		pResponse->strContent += "</td>\n</tr>\n";
		pResponse->strContent += "<tr class=\"signup_form\">\n";
		pResponse->strContent += "<th class=\"signup_form\">\n" + gmapLANG_CFG["password"] + "</th>\n";
		pResponse->strContent += "<td class=\"signup_form\">\n";
		pResponse->strContent += "<input id=\"password\" name=\"us_password\" alt=\"[" + gmapLANG_CFG["password"] + "]\" type=password size=20>\n";
		pResponse->strContent += "</td>\n</tr>\n";
		pResponse->strContent += "<tr class=\"signup_form\">\n";
		pResponse->strContent += "<th class=\"signup_form\">\n" + gmapLANG_CFG["verify_password"] + "</th>\n";
		pResponse->strContent += "<td class=\"signup_form\">\n";
		pResponse->strContent += "<input id=\"verify\" name=\"us_password_verify\" alt=\"[" + gmapLANG_CFG["verify_password"] + "]\" type=password size=20>\n";
		pResponse->strContent += "</td>\n</tr>\n";
		pResponse->strContent += "<tr class=\"signup_form\">\n";
		pResponse->strContent += "<th class=\"signup_form\">\n" + gmapLANG_CFG["email"] + "</th>\n";
		pResponse->strContent += "<td class=\"signup_form\">\n";
		pResponse->strContent += "<input id=\"email\" name=\"us_email\" alt=\"[" + gmapLANG_CFG["email"] + "]\" type=text size=40>\n";
		pResponse->strContent += "</td>\n</tr>\n";
		
		pResponse->strContent += "<tr class=\"signup_form\">\n";
		pResponse->strContent += "<td class=\"signup_form\" colspan=\"2\">\n";
		pResponse->strContent += "<ul>\n";
		// Names must be less than %s characters long
		pResponse->strContent += "<li class=\"signup\">" + UTIL_Xsprintf( gmapLANG_CFG["signup_info_name_length"].c_str( ), CAtomInt( m_uiNameLength ).toString( ).c_str( ) ) + "</li>\n";
		pResponse->strContent += "<li class=\"signup\">" + gmapLANG_CFG["signup_info_case"] + "</li>\n";
		pResponse->strContent += "<li class=\"signup\">" + gmapLANG_CFG["no_html"] + "</li>\n";
		pResponse->strContent += "</ul>\n";
		pResponse->strContent += "</td>\n</tr>\n";
		
		pResponse->strContent += "<tr class=\"signup_form\">\n";
		pResponse->strContent += "<td class=\"signup_form\" colspan=\"2\">\n";
		pResponse->strContent += "<div class=\"signup_form_button\">";
		pResponse->strContent += Button_Submit( "submit_signup", string( gmapLANG_CFG["signup"] ) );
		pResponse->strContent += "</div>\n";
		pResponse->strContent += "</td>\n</tr>\n";
		pResponse->strContent += "</form>\n";
		pResponse->strContent += "</table>\n";
		pResponse->strContent += "</div>\n\n";
	}
	else
		// Not Authorised
		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["signup_page"], string( CSS_SIGNUP ), string( ), NOT_INDEX, CODE_401 );

	// Output common HTML tail
	HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_SIGNUP ) );
}

void CTracker :: serverResponseSignupPOST( struct request_t *pRequest, struct response_t *pResponse, CAtomList *pPost )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
		if( IsIPBanned( pRequest, pResponse, btv, gmapLANG_CFG["signup_page"], string( CSS_SIGNUP ), NOT_INDEX ) )
			return;
		
	string cstrLogin = string( );
	string cstrPass = string( );
	string cstrPass2 = string( );
	string cstrMail = string( );
		
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
						else if( strName == "us_password")
							cstrPass = pData->toString( );
						else if( strName == "us_password_verify")
							cstrPass2 = pData->toString( );
						else if( strName == "us_email" )
							cstrMail = pData->toString( );

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
	
// 	if( pRequest->user.ucAccess & ACCESS_SIGNUP )
	if( pRequest->user.ucAccess & m_ucAccessSignupDirect )
	{
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["signup_page"], string( CSS_SIGNUP ), string( ), NOT_INDEX, CODE_200 );

		if(  !cstrLogin.empty( ) && !cstrPass.empty( ) && !cstrPass2.empty( ) && !cstrMail.empty( ) )
		{
// 			if( cstrLogin[0] == ' ' || cstrLogin[cstrLogin.size( ) - 1] == ' ' || cstrLogin.size( ) > m_uiNameLength )
			if( cstrLogin.find_first_of( " .%&<>\"\n\r" ) != string :: npos || cstrLogin.size( ) > m_uiNameLength )
			{
				// Unable to signup. Your name must be less than " + CAtomInt( m_uiNameLength ).toString( ) + " characters long and it must not start or end with spaces.
				pResponse->strContent += "<p class=\"signup_failed\">" + UTIL_Xsprintf( gmapLANG_CFG["signup_name_error"].c_str( ), CAtomInt( m_uiNameLength ).toString( ).c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_sign_up"] + "\" href=\"" + RESPONSE_STR_SIGNUP_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n\n";

				// Output common HTML tail
				HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_SIGNUP ) );

				return;
			}

			if( cstrMail.find( "@" ) == string :: npos || cstrMail.find( "." ) == string :: npos )
			{
				// Unable to signup. Your e-mail address is invalid.
				pResponse->strContent += "<p class=\"signup_failed\">" + UTIL_Xsprintf( gmapLANG_CFG["signup_email_error"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_sign_up"] + "\" href=\"" + RESPONSE_STR_SIGNUP_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n\n";

				// Output common HTML tail
				HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_SIGNUP ) );

				return;
			}

			if( cstrPass == cstrPass2 )
			{
// 				if( m_pUsers->getItem( cstrLogin ) )
				if( !getUserLogin( cstrLogin ).empty( ) )
				{
					// Unable to signup. The user \"" + UTIL_RemoveHTML( cstrLogin ) + "\" already exists.
					pResponse->strContent += "<p class=\"signup_failed\">" + UTIL_Xsprintf( gmapLANG_CFG["signup_exists_error"].c_str( ), UTIL_RemoveHTML( cstrLogin ).c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_sign_up"] + "\" href=\"" + RESPONSE_STR_SIGNUP_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n\n";

					// Output common HTML tail
					HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_SIGNUP ) );

					return;
				}
				else
				{
 					string strAdded = addUser( cstrLogin, cstrPass, m_ucMemberAccess, cstrMail );
					if( !strAdded.empty( ) )
					{
						m_pCache->addRowUsers( strAdded );
//						m_pCache->ResetUsers( );
						// Thanks! You've successfully signed up!
						pResponse->strContent += "<p class=\"signup_ok\">" + UTIL_Xsprintf( gmapLANG_CFG["signup_success"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_login"] + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n\n";
					}
					else
						pResponse->strContent += "<p class=\"signup_failed\">" + UTIL_Xsprintf( gmapLANG_CFG["users_max_create_fail"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_sign_up"] + "\" href=\"" + RESPONSE_STR_SIGNUP_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";

					// Output common HTML tail
					HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_SIGNUP ) );

					return;
				}
			}
			else
			{
				// Unable to signup. The passwords did not match.
				pResponse->strContent += "<p class=\"signup_failed\">" + UTIL_Xsprintf( gmapLANG_CFG["signup_password_error"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_sign_up"] + "\" href=\"" + RESPONSE_STR_SIGNUP_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n\n";

				// Output common HTML tail
				HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_SIGNUP ) );

				return;
			}
		}
		else
		{
			//Unable to signup. You must fill in all the fields.
			pResponse->strContent += "<p class=\"signup_failed\">" + UTIL_Xsprintf( gmapLANG_CFG["signup_fields_error"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_sign_up"] + "\" href=\"" + RESPONSE_STR_SIGNUP_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n\n";

			// Output common HTML tail
			HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_SIGNUP ) );

			return;
		}
	}
	else
	{
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["signup_page"], string( CSS_SIGNUP ), string( ), NOT_INDEX, CODE_401 );
			
		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_SIGNUP ) );

		return;
	}
		
}

void CTracker :: serverResponseInviteGET( struct request_t *pRequest, struct response_t *pResponse )
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

	bool bNJU = false;

	if( pRequest->strIP.find( ":" ) == string :: npos )
		bNJU = true;
	else
	{
		if( pRequest->strIP.find( "2001:da8:1007:" ) == 0 || pRequest->strIP.find( "2001:250:5002:" ) == 0 )
			bNJU = true;
	}

	if( bNJU && !( pRequest->user.ucAccess & m_ucAccessSignupDirect ) )
	{
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["signup_page"], string( CSS_SIGNUP ), string( ), NOT_INDEX, CODE_200 );
		pResponse->strContent += "<p class=\"signup_failed\">" + gmapLANG_CFG["invite_nju"] + "</p>";
	}
	else if( pRequest->user.ucAccess & m_ucAccessSignup )
	{
		const string cstrCode( pRequest->mapParams["code"] );
		
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["signup_page"], string( CSS_SIGNUP ), string( ), NOT_INDEX, CODE_200 );

		pResponse->strContent += "<div class=\"signup_form\">\n";
		pResponse->strContent += "<table class=\"signup_form\">\n";
		pResponse->strContent += "<form name=\"us_details\" method=\"post\" action=\"" + RESPONSE_STR_INVITE_HTML + "\" enctype=\"multipart/form-data\">\n";
		pResponse->strContent += "<tr class=\"signup_form\">\n";
		pResponse->strContent += "<th class=\"signup_form\" colspan=\"2\">\n" + gmapLANG_CFG["navbar_sign_up_invite"] + "</th>\n</tr>\n";
		
		pResponse->strContent += "<tr class=\"signup_form\">\n";
		pResponse->strContent += "<th class=\"signup_form\">\n" + gmapLANG_CFG["signup_name"] + "</th>\n";
		pResponse->strContent += "<td class=\"signup_form\">\n";
		pResponse->strContent += "<input id=\"login\" name=\"us_login\" alt=\"[" + gmapLANG_CFG["signup_name"] + "]\" type=text size=24 maxlength=" + CAtomInt( m_uiNameLength ).toString( ) + ">\n";
		pResponse->strContent += gmapLANG_CFG["signup_name_note"];
		pResponse->strContent += "</td>\n</tr>\n";
		pResponse->strContent += "<tr class=\"signup_form\">\n";
		pResponse->strContent += "<th class=\"signup_form\">\n" + gmapLANG_CFG["email"] + "</th>\n";
		pResponse->strContent += "<td class=\"signup_form\">\n";
		pResponse->strContent += "<input id=\"email\" name=\"us_email\" alt=\"[" + gmapLANG_CFG["email"] + "]\" type=text size=40>\n";
		pResponse->strContent += "</td>\n</tr>\n";
		pResponse->strContent += "<tr class=\"signup_form\">\n";
		pResponse->strContent += "<th class=\"signup_form\">\n" + gmapLANG_CFG["invite_code"] + "</th>\n";
		pResponse->strContent += "<td class=\"signup_form\">\n";
		pResponse->strContent += "<input id=\"code\" name=\"us_code\" alt=\"[" + gmapLANG_CFG["invite_code"] + "]\" type=text size=40";
		if( !cstrCode.empty( ) )
			pResponse->strContent += " value=\"" + UTIL_RemoveHTML( cstrCode ) + "\"";
		pResponse->strContent += ">\n";
		pResponse->strContent += "</td>\n</tr>\n";
		pResponse->strContent += "<tr class=\"signup_form\">\n";
		pResponse->strContent += "<td class=\"signup_form\" colspan=\"2\">\n";
		pResponse->strContent += "<ul>\n";
		// Names must be less than %s characters long
		pResponse->strContent += "<li class=\"signup\">" + UTIL_Xsprintf( gmapLANG_CFG["signup_info_name_length"].c_str( ), CAtomInt( m_uiNameLength ).toString( ).c_str( ) ) + "</li>\n";
		pResponse->strContent += "<li class=\"signup\">" + gmapLANG_CFG["signup_invite_note"] + "</li>\n";
		pResponse->strContent += "<li class=\"signup\">" + gmapLANG_CFG["no_html"] + "</li>\n";
		pResponse->strContent += "</ul>\n";
		pResponse->strContent += "</td>\n</tr>\n";
		pResponse->strContent += "<tr class=\"signup_form\">\n";
		pResponse->strContent += "<td class=\"signup_form\" colspan=\"2\">\n";
		pResponse->strContent += "<div class=\"signup_form_button\">\n";
		pResponse->strContent += "<input name=\"submit_signup_button\"alt=\"[" + gmapLANG_CFG["signup"] + "]\" type=submit value=\"" + gmapLANG_CFG["signup"] + "\"";
		if( CFG_GetInt( "bnbt_invite_enable", 0 ) == 0 )
			pResponse->strContent += " disabled=true";
		pResponse->strContent += ">";
		if( CFG_GetInt( "bnbt_invite_enable", 0 ) == 0 )
			pResponse->strContent += "<span class=\"red\">" + gmapLANG_CFG["invite_function_invite_close"] + "</span>";
		pResponse->strContent += "</div>\n";
		pResponse->strContent += "</td>\n</tr>\n";
		pResponse->strContent += "</form>\n";
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

void CTracker :: serverResponseInvitePOST( struct request_t *pRequest, struct response_t *pResponse, CAtomList *pPost )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
		if( IsIPBanned( pRequest, pResponse, btv, gmapLANG_CFG["signup_page"], string( CSS_SIGNUP ), NOT_INDEX ) )
			return;
		
	string cstrLogin = string( );
	string cstrPass = string( );
	string cstrPass2 = string( );
	string cstrMail = string( );
	string cstrCode = string( );
		
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
						else if( strName == "us_password")
							cstrPass = pData->toString( );
						else if( strName == "us_password_verify")
							cstrPass2 = pData->toString( );
						else if( strName == "us_email" )
							cstrMail = pData->toString( );
						else if( strName == "us_code" )
							cstrCode = pData->toString( );
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
	
	if( pRequest->user.ucAccess & m_ucAccessSignup )
	{
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["signup_page"], string( CSS_SIGNUP ), string( ), NOT_INDEX, CODE_200 );

		if(  !cstrLogin.empty( ) && !cstrMail.empty( ) && !cstrCode.empty( ) )
		{
// 			if( cstrLogin[0] == ' ' || cstrLogin[cstrLogin.size( ) - 1] == ' ' || cstrLogin.size( ) > m_uiNameLength )
			if( cstrLogin.find_first_of( " .%&<>\"\n\r" ) != string :: npos || cstrLogin.size( ) > m_uiNameLength )
			{
				// Unable to signup. Your name must be less than " + CAtomInt( m_uiNameLength ).toString( ) + " characters long and it must not start or end with spaces.
				pResponse->strContent += "<p class=\"signup_failed\">" + UTIL_Xsprintf( gmapLANG_CFG["signup_name_error"].c_str( ), CAtomInt( m_uiNameLength ).toString( ).c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_sign_up_invite"] + "\" href=\"" + RESPONSE_STR_INVITE_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n\n";

				// Output common HTML tail
				HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_SIGNUP ) );

				return;
			}

			if( cstrMail.find( "@" ) == string :: npos || cstrMail.find( "@" ) == 0 || cstrMail.find( "." ) == string :: npos )
			{
				// Unable to signup. Your e-mail address is invalid.
				pResponse->strContent += "<p class=\"signup_failed\">" + UTIL_Xsprintf( gmapLANG_CFG["signup_email_error"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_sign_up_invite"] + "\" href=\"" + RESPONSE_STR_INVITE_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n\n";

				// Output common HTML tail
				HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_SIGNUP ) );

				return;
			}
			unsigned char ucIndex = 1;
			string strSchoolMail = gmapLANG_CFG["signup_mail"+CAtomInt( ucIndex ).toString( )];
			while( !strSchoolMail.empty( ) )
			{
				if( cstrMail.substr( cstrMail.find( "@" ) ) == strSchoolMail )
				{
					// Unable to signup. Your e-mail address is invalid.
					pResponse->strContent += "<p class=\"signup_failed\">" + UTIL_Xsprintf( gmapLANG_CFG["signup_email_error_invite_school"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_sign_up_school"] + "\" href=\"" + RESPONSE_STR_SIGNUP_SCHOOL_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n\n";

					// Output common HTML tail
					HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_SIGNUP ) );

					return;
				}
				strSchoolMail = gmapLANG_CFG["signup_mail"+CAtomInt( ++ucIndex ).toString( )];
			}

			if( !getUserLogin( cstrLogin ).empty( ) )
			{
				// Unable to signup. The user \"" + UTIL_RemoveHTML( cstrLogin ) + "\" already exists.
				pResponse->strContent += "<p class=\"signup_failed\">" + UTIL_Xsprintf( gmapLANG_CFG["signup_exists_error"].c_str( ), UTIL_RemoveHTML( cstrLogin ).c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_sign_up_invite"] + "\" href=\"" + RESPONSE_STR_INVITE_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n\n";

				// Output common HTML tail
				HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_SIGNUP ) );

				return;
			}
			else
			{
				CMySQLQuery *pQueryMail = new CMySQLQuery( "SELECT bemail FROM users WHERE bemail=\'" + UTIL_StringToMySQL( cstrMail ) + "\'" );
				
				if( pQueryMail->nextRow( ).size( ) == 1 )
				{
					pResponse->strContent += "<p class=\"signup_failed\">" + UTIL_Xsprintf( gmapLANG_CFG["signup_exists_error_mail"].c_str( ), UTIL_RemoveHTML( cstrLogin ).c_str( ), UTIL_RemoveHTML( cstrMail ).c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_sign_up_invite"] + "\" href=\"" + RESPONSE_STR_INVITE_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n\n";
				}
				else
				{
					CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bcode,bownerid,users.busername FROM invites LEFT JOIN users ON invites.bownerid=users.buid WHERE bcode=\'" + UTIL_StringToMySQL( cstrCode ) + "\' AND bquota>0 AND bused=0" );
				
					vector<string> vecQuery;
	
					vecQuery.reserve(3);

					vecQuery = pQuery->nextRow( );
				
					delete pQuery;
		
					if( vecQuery.size( ) == 3 )
					{
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
					
	 					string strAdded = addUser( cstrLogin, cstrPass, m_ucMemberAccess, cstrMail );
						if( !strAdded.empty( ) )
						{
							system( string( "./sendschool.sh \"" + cstrMail + "\" \"" + cstrLogin + "\" \"" + cstrPass + "\" &" ).c_str( ) );
							CMySQLQuery mq01( "UPDATE invites SET binvitee=\'" +  UTIL_StringToMySQL( cstrLogin ) + "\',binviteeid=" + strAdded + ",bquota=bquota-1 WHERE bcode=\'" + UTIL_StringToMySQL( cstrCode ) + "\' AND bquota>0" );
							CMySQLQuery mq02( "UPDATE invites SET bused=1 WHERE bcode=\'" + UTIL_StringToMySQL( cstrCode ) + "\' AND bquota=0" );
							CMySQLQuery mq03( "UPDATE users SET binviter=\'" +  UTIL_StringToMySQL( vecQuery[2] ) +"\', binviterid=" + UTIL_StringToMySQL( vecQuery[1] ) + " WHERE buid=" + strAdded );
							m_pCache->addRowUsers( strAdded );
//							m_pCache->ResetUsers( );
							// Thanks! You've successfully signed up!
							pResponse->strContent += "<p class=\"signup_ok\">" + UTIL_Xsprintf( gmapLANG_CFG["signup_success_mail"].c_str( ), UTIL_RemoveHTML( cstrMail ).c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_login"] + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n\n";
						}
						else
							pResponse->strContent += "<p class=\"signup_failed\">" + UTIL_Xsprintf( gmapLANG_CFG["users_max_create_fail"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_sign_up_invite"] + "\" href=\"" + RESPONSE_STR_INVITE_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";
					}
					else
						pResponse->strContent += "<p class=\"signup_failed\">" + UTIL_Xsprintf( gmapLANG_CFG["signup_code_error"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_sign_up_invite"] + "\" href=\"" + RESPONSE_STR_INVITE_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";
				}
				
				delete pQueryMail;

				// Output common HTML tail
				HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_SIGNUP ) );

				return;
			}
		}
		else
		{
			//Unable to signup. You must fill in all the fields.
			pResponse->strContent += "<p class=\"signup_failed\">" + UTIL_Xsprintf( gmapLANG_CFG["signup_fields_error"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_sign_up_invite"] + "\" href=\"" + RESPONSE_STR_INVITE_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n\n";

			// Output common HTML tail
			HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_SIGNUP ) );

			return;
		}
	}
	else
	{
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["signup_page"], string( CSS_SIGNUP ), string( ), NOT_INDEX, CODE_401 );

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_SIGNUP ) );

		return;
	}
		
}

void CTracker :: serverResponseSignupSchoolGET( struct request_t *pRequest, struct response_t *pResponse )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
		if( IsIPBanned( pRequest, pResponse, btv, gmapLANG_CFG["signup_page"], string( CSS_SIGNUP ), NOT_INDEX ) )
			return;

	if( pRequest->user.ucAccess & m_ucAccessSignup )
	{
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["signup_page"], string( CSS_SIGNUP ), string( ), NOT_INDEX, CODE_200 );
		
		pResponse->strContent += "<script type=\"text/javascript\">\n";
		pResponse->strContent += "<!--\n";
		
		pResponse->strContent += "function sample( tag ) {\n";
		pResponse->strContent += "var sample = document.getElementById( \"type_sample\" + tag );\n";
		pResponse->strContent += "var note = document.getElementById( \"type_note\" );\n";
		pResponse->strContent += "  note.innerHTML = sample.innerHTML;\n";
		pResponse->strContent += "}\n\n";
		
		pResponse->strContent += "//-->\n";
		pResponse->strContent += "</script>\n\n";

		pResponse->strContent += "<div class=\"signup_form\">\n";
		pResponse->strContent += "<table class=\"signup_form\">\n";
		pResponse->strContent += "<form name=\"us_details\" method=\"post\" action=\"" + RESPONSE_STR_SIGNUP_SCHOOL_HTML + "\" enctype=\"multipart/form-data\">\n";
		pResponse->strContent += "<tr class=\"signup_form\">\n";
		pResponse->strContent += "<th class=\"signup_form\" colspan=\"2\">\n" + gmapLANG_CFG["navbar_sign_up_school"] + "</th>\n</tr>\n";
		pResponse->strContent += "<tr class=\"signup_form\">\n";
		pResponse->strContent += "<th class=\"signup_form\">\n" + gmapLANG_CFG["signup_type"] + "</th>\n";
		pResponse->strContent += "<td class=\"signup_form\">\n";
		pResponse->strContent += "<select id=\"signup_type\" name=\"us_type\" onchange=\"sample(this.options[this.options.selectedIndex].value)\">\n";
		unsigned char ucIndex = 0;
		if( CFG_GetInt( "bnbt_signup_lily_enable", 0 ) == 0 )
			ucIndex = 1;
		string strSignupMail = gmapLANG_CFG["signup_type"+CAtomInt( ucIndex ).toString( )];
		while( !strSignupMail.empty( ) )
		{
			pResponse->strContent += "<option value=\"" + CAtomInt( ucIndex ).toString( ) + "\">" + strSignupMail + "\n";
			strSignupMail = gmapLANG_CFG["signup_type"+CAtomInt( ++ucIndex ).toString( )];
		}
		pResponse->strContent += "</select>\n";
		pResponse->strContent += "<input id=\"signup_mail\" name=\"us_mail\" type=text size=24>\n";
		ucIndex = 0;
		if( CFG_GetInt( "bnbt_signup_lily_enable", 0 ) == 0 )
			ucIndex = 1;
		pResponse->strContent += "<span id=\"type_note\">" + UTIL_Xsprintf( gmapLANG_CFG["signup_type_note"].c_str( ), gmapLANG_CFG["signup_mail"+CAtomInt( ucIndex ).toString( )].c_str( ) ) + "</span>";
		pResponse->strContent += "</td>\n</tr>\n";
		pResponse->strContent += "<tr class=\"signup_form\">\n";
		pResponse->strContent += "<th class=\"signup_form\">\n" + gmapLANG_CFG["signup_name"] + "</th>\n";
		pResponse->strContent += "<td class=\"signup_form\">\n";
		pResponse->strContent += "<input id=\"signup_name\" name=\"us_login\" alt=\"[" + gmapLANG_CFG["signup_name"] + "]\" type=text size=24 maxlength=" + CAtomInt( m_uiNameLength ).toString( ) + ">\n";
		pResponse->strContent += gmapLANG_CFG["signup_name_note"];
		pResponse->strContent += "</td>\n</tr>\n";
		
		strSignupMail = gmapLANG_CFG["signup_type"+CAtomInt( ucIndex ).toString( )];
		while( !strSignupMail.empty( ) )
		{
			pResponse->strContent += "<span id=\"type_sample" + CAtomInt( ucIndex ).toString( ) + "\" style=\"display: none\">" + UTIL_Xsprintf( gmapLANG_CFG["signup_type_note"].c_str( ), gmapLANG_CFG["signup_mail"+CAtomInt( ucIndex ).toString( )].c_str( ) ) + "</span>\n";
			strSignupMail = gmapLANG_CFG["signup_type"+CAtomInt( ++ucIndex ).toString( )];
		}
		pResponse->strContent += "<tr class=\"signup_form\">\n";
		pResponse->strContent += "<td class=\"signup_form\" colspan=\"2\">\n";
		pResponse->strContent += "<ul>\n";
		// Names must be less than %s characters long
		pResponse->strContent += "<li class=\"signup\">" + UTIL_Xsprintf( gmapLANG_CFG["signup_info_name_length"].c_str( ), CAtomInt( m_uiNameLength ).toString( ).c_str( ) ) + "</li>\n";
		pResponse->strContent += "<li class=\"signup\"><span class=\"red\">" + gmapLANG_CFG["signup_info_lily"] + "</span></li>\n";
		pResponse->strContent += "<li class=\"signup\"><span class=\"red\">" + gmapLANG_CFG["signup_info_lily_1"] + "</span></li>\n";
		pResponse->strContent += "<li class=\"signup\">" + gmapLANG_CFG["signup_info_lily_2"] + "</li>\n";
		pResponse->strContent += "<li class=\"signup\">" + gmapLANG_CFG["no_html"] + "</li>\n";
		pResponse->strContent += "</ul>\n";
		pResponse->strContent += "</td>\n</tr>\n";
		
		pResponse->strContent += "<tr class=\"signup_form\">\n";
		pResponse->strContent += "<td class=\"signup_form\" colspan=\"2\">\n";
		pResponse->strContent += "<div class=\"signup_form_button\">\n";
		pResponse->strContent += Button_Submit( "submit_signup", string( gmapLANG_CFG["signup"] ) );
		pResponse->strContent += "</div>\n\n";
		pResponse->strContent += "</td>\n</tr>\n";
		pResponse->strContent += "</form>\n";
		pResponse->strContent += "</table>\n";
		pResponse->strContent += "</div>\n\n";
	}
	else
		// Not Authorised
		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["signup_page"], string( CSS_SIGNUP ), string( ), NOT_INDEX, CODE_401 );

	// Output common HTML tail
	HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_SIGNUP ) );
}

void CTracker :: serverResponseSignupSchoolPOST( struct request_t *pRequest, struct response_t *pResponse, CAtomList *pPost )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
		if( IsIPBanned( pRequest, pResponse, btv, gmapLANG_CFG["signup_page"], string( CSS_SIGNUP ), NOT_INDEX ) )
			return;
	
	string cstrType = string( );
	string cstrLogin = string( );
	string cstrMail = string( );
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
						else if( strName == "us_mail")
							cstrMail = UTIL_ToLower( pData->toString( ) );
						else if( strName == "us_type" )
							cstrType = pData->toString( );
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
	
	if( cstrType == "0" && CFG_GetInt( "bnbt_signup_lily_enable", 0 ) == 1 && !cstrMail.empty( ) )
	{
		cstrLilyID = cstrMail;
		cstrMail = cstrLilyID + gmapLANG_CFG["signup_mail0"];
	}

	string cstrLowerMail = UTIL_ToLower( cstrMail );
	
	if( pRequest->user.ucAccess & m_ucAccessSignup )
	{
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["signup_page"], string( CSS_SIGNUP ), string( ), NOT_INDEX, CODE_200 );

		if( !cstrLogin.empty( ) && !cstrMail.empty( ) )
		{
// 			if( cstrLogin[0] == ' ' || cstrLogin[cstrLogin.size( ) - 1] == ' ' || cstrLogin.size( ) > m_uiNameLength )
			if( cstrLogin.find_first_of( " .%&<>\"\n\r" ) != string :: npos || cstrMail.find_first_of( " %&<>\"\n\r" ) != string :: npos || cstrLogin.size( ) > m_uiNameLength )
			{
				// Unable to signup. Your name must be less than " + CAtomInt( m_uiNameLength ).toString( ) + " characters long and it must not start or end with spaces.
				pResponse->strContent += "<p class=\"signup_failed\">" + UTIL_Xsprintf( gmapLANG_CFG["signup_name_error"].c_str( ), CAtomInt( m_uiNameLength ).toString( ).c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_sign_up_school"] + "\" href=\"" + RESPONSE_STR_SIGNUP_SCHOOL_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n\n";

				// Output common HTML tail
				HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_SIGNUP ) );

				return;
			}
			if( cstrMail.find( "@" ) == string :: npos || cstrMail.find( "@" ) == 0 || cstrMail.find( "." ) == string :: npos )
			{
				// Unable to signup. Your e-mail address is invalid.
				pResponse->strContent += "<p class=\"signup_failed\">" + UTIL_Xsprintf( gmapLANG_CFG["signup_email_error"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_sign_up_school"] + "\" href=\"" + RESPONSE_STR_SIGNUP_SCHOOL_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n\n";

				// Output common HTML tail
				HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_SIGNUP ) );

				return;
			}
			if( cstrMail.substr( cstrMail.find( "@" ) ) != gmapLANG_CFG["signup_mail"+cstrType] )
			{
				// Unable to signup. Your e-mail address is invalid.
				pResponse->strContent += "<p class=\"signup_failed\">" + UTIL_Xsprintf( gmapLANG_CFG["signup_email_error"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_sign_up_school"] + "\" href=\"" + RESPONSE_STR_SIGNUP_SCHOOL_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n\n";

				// Output common HTML tail
				HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_SIGNUP ) );

				return;
			}
			if( cstrType == "1" && cstrLowerMail.find( "b0" ) != 0 && cstrLowerMail.find( "b1" ) != 0 && cstrLowerMail.find( "mg" ) != 0 && cstrLowerMail.find( "mf" ) != 0 && cstrLowerMail.find( "dg" ) != 0 )
			{
				// Unable to signup. Your e-mail address is invalid.
				pResponse->strContent += "<p class=\"signup_failed\">" + UTIL_Xsprintf( gmapLANG_CFG["signup_email_error_smail"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_sign_up_school"] + "\" href=\"" + RESPONSE_STR_SIGNUP_SCHOOL_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n\n";

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

			if( !getUserLogin( cstrLogin ).empty( ) )
			{
				// Unable to signup. The user \"" + UTIL_RemoveHTML( cstrLogin ) + "\" already exists.
				pResponse->strContent += "<p class=\"signup_failed\">" + UTIL_Xsprintf( gmapLANG_CFG["signup_exists_error"].c_str( ), UTIL_RemoveHTML( cstrLogin ).c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_sign_up_school"] + "\" href=\"" + RESPONSE_STR_SIGNUP_SCHOOL_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n\n";
			}
			else
			{
// 				CMySQLQuery *pQuery = new CMySQLQuery( "SELECT blilyid FROM lily WHERE blilyid=\'" + UTIL_StringToMySQL( cstrLilyID ) + "\'" );
				CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bemail FROM users WHERE bemail=\'" + UTIL_StringToMySQL( cstrMail ) + "\'" );
				
				if( pQuery->nextRow( ).size( ) == 1 )
				{
					pResponse->strContent += "<p class=\"signup_failed\">" + UTIL_Xsprintf( gmapLANG_CFG["signup_exists_error_mail"].c_str( ), UTIL_RemoveHTML( cstrLogin ).c_str( ), UTIL_RemoveHTML( cstrMail ).c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_sign_up_school"] + "\" href=\"" + RESPONSE_STR_SIGNUP_SCHOOL_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n\n";
				}
				else
				{
					string strAdded = addUser( cstrLogin, cstrPass, m_ucMemberAccess, cstrMail );
					if( !strAdded.empty( ) )
					{
						if( !cstrLilyID.empty( ) )
						{
							CMySQLQuery mq01( "INSERT INTO lily (blilyid,busername,buid,bpassword) VALUES(\'" + UTIL_StringToMySQL( cstrLilyID ) + "\',\'" + UTIL_StringToMySQL( cstrLogin ) + "\'," + strAdded + ",\'" + UTIL_StringToMySQL( cstrPass ) + "\')" );
//							CMySQLQuery mq02( "UPDATE lily,users SET lily.buid=users.buid WHERE lily.blilyid=\'" + UTIL_StringToMySQL( cstrLilyID ) + "\' AND lily.busername=users.busername" );
							system( string( "./sendlily.sh \"" + cstrMail + "\" \"" + cstrLogin + "\" \"" + cstrPass + "\" &" ).c_str( ) );
							pResponse->strContent += "<p class=\"signup_ok\">" + UTIL_Xsprintf( gmapLANG_CFG["signup_success_lily"].c_str( ), UTIL_RemoveHTML( cstrLilyID ).c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_login"] + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n\n";
						}
						else
						{
							system( string( "./sendschool.sh \"" + cstrMail + "\" \"" + cstrLogin + "\" \"" + cstrPass + "\" &" ).c_str( ) );
// 							system( string( "echo \"ID: " + cstrLogin + " Password: " + cstrPass + "\" | mutt -s \"ZiJingBT\" " + cstrMail ).c_str( ) );
							// Thanks! You've successfully signed up!
							pResponse->strContent += "<p class=\"signup_ok\">" + UTIL_Xsprintf( gmapLANG_CFG["signup_success_mail"].c_str( ), UTIL_RemoveHTML( cstrMail ).c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_login"] + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n\n";
						}
						m_pCache->addRowUsers( strAdded );
//						m_pCache->ResetUsers( );
					}
					else
						pResponse->strContent += "<p class=\"signup_failed\">" + UTIL_Xsprintf( gmapLANG_CFG["users_max_create_fail"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_users"] + "\" href=\"" + RESPONSE_STR_SIGNUP_SCHOOL_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";
				}
				delete pQuery;
			}
			
			HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_SIGNUP ) );
		}
		else
		{
			//Unable to signup. You must fill in all the fields.
			pResponse->strContent += "<p class=\"signup_failed\">" + UTIL_Xsprintf( gmapLANG_CFG["signup_fields_error"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_sign_up_school"] + "\" href=\"" + RESPONSE_STR_SIGNUP_SCHOOL_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n\n";

			// Output common HTML tail
			HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_SIGNUP ) );

			return;
		}
	}
	else
	{
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["signup_page"], string( CSS_SIGNUP ), string( ), NOT_INDEX, CODE_401 );

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_SIGNUP ) );

		return;
	}
		
}

void CTracker :: serverResponseRecoverGET( struct request_t *pRequest, struct response_t *pResponse )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
		if( IsIPBanned( pRequest, pResponse, btv, gmapLANG_CFG["recover_page"], string( CSS_SIGNUP ), NOT_INDEX ) )
			return;

	if( pRequest->user.ucAccess & m_ucAccessSignup )
	{
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["recover_page"], string( CSS_SIGNUP ), string( ), NOT_INDEX, CODE_200 );
		
		pResponse->strContent += "<script type=\"text/javascript\">\n";
		pResponse->strContent += "<!--\n";
		
		pResponse->strContent += "function sample( tag ) {\n";
		pResponse->strContent += "var sample = document.getElementById( \"type_sample\" + tag );\n";
		pResponse->strContent += "var note = document.getElementById( \"type_note\" );\n";
		pResponse->strContent += "  note.innerHTML = sample.innerHTML;\n";
		pResponse->strContent += "}\n\n";
		
		pResponse->strContent += "//-->\n";
		pResponse->strContent += "</script>\n\n";

		pResponse->strContent += "<div class=\"signup_form\">\n";
		pResponse->strContent += "<table class=\"signup_form\">\n";
		pResponse->strContent += "<form name=\"us_details\" method=\"post\" action=\"" + RESPONSE_STR_RECOVER_HTML + "\" enctype=\"multipart/form-data\">\n";
		pResponse->strContent += "<tr class=\"signup_form\">\n";
		pResponse->strContent += "<th class=\"signup_form\" colspan=\"2\">\n" + gmapLANG_CFG["recover"] + "</th>\n</tr>\n";
		pResponse->strContent += "<tr class=\"signup_form\">\n";
		pResponse->strContent += "<th class=\"signup_form\">\n" + gmapLANG_CFG["signup_type"] + "</th>\n";
		pResponse->strContent += "<td class=\"signup_form\">\n";
		pResponse->strContent += "<select id=\"signup_type\" name=\"us_type\">\n";
		pResponse->strContent += "<option value=\"0\">" + gmapLANG_CFG["signup_type0"] + "\n";
		pResponse->strContent += "<option value=\"1\">" + gmapLANG_CFG["signup_type_mail"] + "\n";
		pResponse->strContent += "</select>\n";
		pResponse->strContent += "<input id=\"signup_mail\" name=\"us_mail\" type=text size=24>\n";
		pResponse->strContent += "</td>\n</tr>\n";
		pResponse->strContent += "<tr class=\"signup_form\">\n";
		pResponse->strContent += "<th class=\"signup_form\">\n" + gmapLANG_CFG["signup_name"] + "</th>\n";
		pResponse->strContent += "<td class=\"signup_form\">\n";
		pResponse->strContent += "<input id=\"signup_name\" name=\"us_login\" alt=\"[" + gmapLANG_CFG["signup_name"] + "]\" type=text size=24 maxlength=" + CAtomInt( m_uiNameLength ).toString( ) + ">\n";
		pResponse->strContent += "</td>\n</tr>\n";
			
		pResponse->strContent += "<tr class=\"signup_form\">\n";
		pResponse->strContent += "<td class=\"signup_form\" colspan=\"2\">\n";
		pResponse->strContent += "<ul>\n";
		// Names must be less than %s characters long

		pResponse->strContent += "<li class=\"signup\">" + gmapLANG_CFG["recover_note_1"] + "</li>\n";
		pResponse->strContent += "<li class=\"signup\">" + gmapLANG_CFG["recover_note_2"] + "</li>\n";
		pResponse->strContent += "</ul>\n";
		pResponse->strContent += "</td>\n</tr>\n";
		
		pResponse->strContent += "<tr class=\"signup_form\">\n";
		pResponse->strContent += "<td class=\"signup_form\" colspan=\"2\">\n";
		pResponse->strContent += "<div class=\"signup_form_button\">\n";
		pResponse->strContent += Button_Submit( "submit_signup", string( gmapLANG_CFG["submit"] ) );
		pResponse->strContent += "</div>\n\n";
		pResponse->strContent += "</td>\n</tr>\n";
		pResponse->strContent += "</form>\n";
		pResponse->strContent += "</table>\n";
		pResponse->strContent += "</div>\n\n";
	}
	else
		// Not Authorised
		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["signup_page"], string( CSS_SIGNUP ), string( ), NOT_INDEX, CODE_401 );

	// Output common HTML tail
	HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_SIGNUP ) );
}

void CTracker :: serverResponseRecoverPOST( struct request_t *pRequest, struct response_t *pResponse, CAtomList *pPost )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
		if( IsIPBanned( pRequest, pResponse, btv, gmapLANG_CFG["recover_page"], string( CSS_SIGNUP ), NOT_INDEX ) )
			return;
	
	string cstrType = string( );
	string cstrLogin = string( );
	string cstrMail = string( );
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
						else if( strName == "us_mail")
							cstrMail = UTIL_ToLower( pData->toString( ) );
						else if( strName == "us_type" )
							cstrType = pData->toString( );
					}
					else
					{
						// Output common HTML head
						HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["recover_page"], string( CSS_SIGNUP ), string( ), NOT_INDEX, CODE_400 );

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
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["recover_page"], string( CSS_SIGNUP ), string( ), NOT_INDEX, CODE_400 );

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
	
	if( cstrType == "0" && !cstrMail.empty( ) )
	{
		cstrLilyID = cstrMail;
		cstrMail = cstrLilyID + gmapLANG_CFG["signup_mail0"];
	}
	
	if( pRequest->user.ucAccess & m_ucAccessSignup )
	{
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["recover_page"], string( CSS_SIGNUP ), string( ), NOT_INDEX, CODE_200 );

		if( !cstrLogin.empty( ) && !cstrMail.empty( ) )
		{
// 			if( cstrLogin[0] == ' ' || cstrLogin[cstrLogin.size( ) - 1] == ' ' || cstrLogin.size( ) > m_uiNameLength )
			if( cstrLogin.find_first_of( " .%&<>\"\n\r" ) != string :: npos || cstrMail.find_first_of( " %&<>\"\n\r" ) != string :: npos || cstrLogin.size( ) > m_uiNameLength )
			{
				// Unable to signup. Your name must be less than " + CAtomInt( m_uiNameLength ).toString( ) + " characters long and it must not start or end with spaces.
				pResponse->strContent += "<p class=\"signup_failed\">" + UTIL_Xsprintf( gmapLANG_CFG["recover_name_error"].c_str( ), CAtomInt( m_uiNameLength ).toString( ).c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_sign_up"] + "\" href=\"" + RESPONSE_STR_RECOVER_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n\n";

				// Output common HTML tail
				HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_SIGNUP ) );

				return;
			}
			if( cstrMail.find( "@" ) == string :: npos || cstrMail.find( "@" ) == 0 || cstrMail.find( "." ) == string :: npos )
			{
				// Unable to signup. Your e-mail address is invalid.
				pResponse->strContent += "<p class=\"signup_failed\">" + UTIL_Xsprintf( gmapLANG_CFG["recover_email_error"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_sign_up"] + "\" href=\"" + RESPONSE_STR_RECOVER_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n\n";

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

			string strRealLogin = getUserLogin( cstrLogin );

			if( strRealLogin.empty( ) )
			{
				// Unable to signup. The user \"" + UTIL_RemoveHTML( cstrLogin ) + "\" already exists.
				pResponse->strContent += "<p class=\"signup_failed\">" + UTIL_Xsprintf( gmapLANG_CFG["recover_notexists_error"].c_str( ), UTIL_RemoveHTML( cstrLogin ).c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_sign_up"] + "\" href=\"" + RESPONSE_STR_RECOVER_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n\n";
			}
			else
			{
				// calculate md5 hash of A1
				const string cstrA2( strRealLogin + ":" + gstrPasswordKey + ":" + cstrPass );

				MD5Init( &md5 );
				MD5Update( &md5, (const unsigned char *)cstrA2.c_str( ), (unsigned int)cstrA2.size( ) );
				MD5Final( szMD5, &md5 );
				
// 				CMySQLQuery *pQuery = new CMySQLQuery( "SELECT blilyid FROM lily WHERE blilyid=\'" + UTIL_StringToMySQL( cstrLilyID ) + "\'" );
				CMySQLQuery *pQuery = new CMySQLQuery( "SELECT buid FROM users WHERE busername=\'" + UTIL_StringToMySQL( cstrLogin ) + "\' AND bemail=\'" + UTIL_StringToMySQL( cstrMail ) + "\'" );
				
				vector<string> vecQuery;
	
				vecQuery.reserve(1);
		
				vecQuery = pQuery->nextRow( );
				
				delete pQuery;
				
				if( vecQuery.size( ) == 0 )
				{
					pResponse->strContent += "<p class=\"signup_failed\">" + UTIL_Xsprintf( gmapLANG_CFG["recover_notexists_error_mail"].c_str( ), UTIL_RemoveHTML( cstrLogin ).c_str( ), UTIL_RemoveHTML( cstrMail ).c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_sign_up"] + "\" href=\"" + RESPONSE_STR_RECOVER_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n\n";
				}
				else
				{
					if( !vecQuery[0].empty( ) )
					{
						CMySQLQuery mq01( "UPDATE users SET bmd5=\'" + UTIL_StringToMySQL( string( (char *)szMD5, sizeof( szMD5 ) / sizeof( unsigned char ) ) ) + "\' WHERE buid=" + vecQuery[0] );
						if( !cstrLilyID.empty( ) )
						{
							system( string( "./sendlily.sh \"" + cstrMail + "\" \"" + strRealLogin + "\" \"" + cstrPass + "\" &" ).c_str( ) );
//							UTIL_LogPrint( string( "./sendlily.sh \"" + cstrMail + "\" \"" + strRealLogin + "\" \"" + cstrPass + "\" &\n" ).c_str( ) );
							pResponse->strContent += "<p class=\"signup_ok\">" + UTIL_Xsprintf( gmapLANG_CFG["recover_success_lily"].c_str( ), UTIL_RemoveHTML( cstrLilyID ).c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_login"] + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n\n";
						}
						else
						{
							system( string( "./sendschool.sh \"" + cstrMail + "\" \"" + strRealLogin + "\" \"" + cstrPass + "\" &" ).c_str( ) );
//							UTIL_LogPrint( string( "./sendschool.sh \"" + cstrMail + "\" \"" + strRealLogin + "\" \"" + cstrPass + "\" &\n" ).c_str( ) );
// 							system( string( "echo \"ID: " + cstrLogin + " Password: " + cstrPass + "\" | mutt -s \"ZiJingBT\" " + cstrMail ).c_str( ) );
							// Thanks! You've successfully signed up!
							pResponse->strContent += "<p class=\"signup_ok\">" + UTIL_Xsprintf( gmapLANG_CFG["recover_success_mail"].c_str( ), UTIL_RemoveHTML( cstrMail ).c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_login"] + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n\n";
						}
					}
//					else
//						pResponse->strContent += "<p class=\"signup_failed\">" + UTIL_Xsprintf( gmapLANG_CFG["users_max_create_fail"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_users"] + "\" href=\"" + RESPONSE_STR_USERS_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";
				}
				
			}
			
			HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_SIGNUP ) );
		}
		else
		{
			//Unable to signup. You must fill in all the fields.
			pResponse->strContent += "<p class=\"signup_failed\">" + UTIL_Xsprintf( gmapLANG_CFG["recover_fields_error"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_sign_up"] + "\" href=\"" + RESPONSE_STR_RECOVER_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n\n";

			// Output common HTML tail
			HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_SIGNUP ) );

			return;
		}
	}
	else
	{
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["recover_page"], string( CSS_SIGNUP ), string( ), NOT_INDEX, CODE_401 );

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_SIGNUP ) );

		return;
	}
		
}
