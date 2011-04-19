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
#include "sort.h"
#include "tracker.h"
#include "util.h"

void CTracker :: serverResponseUsers( struct request_t *pRequest, struct response_t *pResponse )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
		if( IsIPBanned( pRequest, pResponse, btv, gmapLANG_CFG["users_page"], string( CSS_USERS ), NOT_INDEX ) )
			return;
	
	// Check that user has admin authority
	if( pRequest->user.ucAccess & ACCESS_ADMIN )
	{
		// Was a search submited?
// 		if( pRequest->mapParams["top_submit_search_button"] == STR_SUBMIT || pRequest->mapParams["bottom_submit_search_button"] == STR_SUBMIT )
		if( pRequest->mapParams["top_submit_search_button"] == gmapLANG_CFG["search"] || pRequest->mapParams["bottom_submit_search_button"] == gmapLANG_CFG["search"] )
		{
			const string cstrSearch( pRequest->mapParams["search"] );
			const string cstrSort( pRequest->mapParams["sort"] );
			const string cstrPerPage( pRequest->mapParams["per_page"] );

			string strPageParameters = USERS_HTML;

			if( !cstrSearch.empty( ) || !cstrSort.empty( ) || !cstrPerPage.empty( ) )
				strPageParameters += "?";
	
			if( !cstrSearch.empty( ) )
				strPageParameters += "search=" + cstrSearch;

			if( !cstrSearch.empty( ) && !cstrSort.empty( ) )
				strPageParameters += "&";
						
			if( !cstrSort.empty( ) )
				strPageParameters += "sort=" + cstrSort;

			if( ( !cstrSearch.empty( ) || !cstrSort.empty( ) ) && !cstrPerPage.empty( ) )
				strPageParameters += "&";

			if( !cstrPerPage.empty( ) )
				strPageParameters += "per_page=" + cstrPerPage;

			JS_ReturnToPage( pRequest, pResponse, strPageParameters );
			return;
		}

		if( pRequest->mapParams["top_clear_filter_and_search_button"] == "Clear" || pRequest->mapParams["bottom_clear_filter_and_search_button"] == "Clear" )
			return JS_ReturnToPage( pRequest, pResponse, USERS_HTML );

		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["users_page"], string( CSS_USERS ), string( ), NOT_INDEX, CODE_200 );

		// javascript
		pResponse->strContent += "<script type=\"text/javascript\">\n";
		pResponse->strContent += "<!--\n";

		pResponse->strContent += "function delete_user_confirm( USER )\n";
		pResponse->strContent += "{\n";
		pResponse->strContent += "var name=confirm(\"" + gmapLANG_CFG["delete"] + " xbnbt_announce_list\" + USER )\n";
		pResponse->strContent += "if (name==true)\n";
		pResponse->strContent += "{\n";
		pResponse->strContent += "window.location=\"" + RESPONSE_STR_USERS_HTML + "?up_deluser=\" + USER\n";
		pResponse->strContent += "}\n";
		pResponse->strContent += "}\n\n";

		pResponse->strContent += "function edit_user( USER ) {\n";
		pResponse->strContent += "window.location=\"" + RESPONSE_STR_USERS_HTML + "?up_edituser\" + USER\n";
		pResponse->strContent += "}\n\n";

		pResponse->strContent += "function clear_search_and_filters( ) {\n";
		pResponse->strContent += "window.location=\"" + RESPONSE_STR_USERS_HTML + "\"\n";
		pResponse->strContent += "}\n\n";

		pResponse->strContent += "//-->\n";
		pResponse->strContent += "</script>\n\n";

#if defined ( XBNBT_MYSQL )
		// XBNBT MySQL Users Integration
		if( gbMySQLUsersOverrideUsers )
		{
			pResponse->strContent += "<p class=\"mysql_users_info\">" + UTIL_Xsprintf( gmapLANG_CFG["users_load_success"].c_str( ), m_strMySQLUsersLoadLast.c_str( ) ) + "</p>\n";
			pResponse->strContent += "<p class=\"mysql_users_info\">" + UTIL_Xsprintf( gmapLANG_CFG["users_load_status"].c_str( ), m_strMySQLUsersLoadLastStatus.c_str( ) ) + "</p>\n";
			pResponse->strContent += "<p class=\"mysql_users_info\">" + UTIL_Xsprintf( gmapLANG_CFG["users_load_interval"].c_str( ), CAtomInt( m_uiMySQLUsersLoadInterval ).toString( ).c_str( ) ) + "</p>\n";
		}
		else
		{
#endif
			//
			// create user
			//

			// Do we have the compulsory user parameters?
			if( pRequest->mapParams.find( "us_login" ) != pRequest->mapParams.end( ) &&
				pRequest->mapParams.find( "us_password" ) != pRequest->mapParams.end( ) &&
				pRequest->mapParams.find( "us_password_verify" ) != pRequest->mapParams.end( ) &&
				pRequest->mapParams.find( "us_email" ) != pRequest->mapParams.end( ) )
			{
				// Get the submitted data
				const string cstrLogin( pRequest->mapParams["us_login"] );
				const string cstrPass( pRequest->mapParams["us_password"] );
				const string cstrPass2( pRequest->mapParams["us_password_verify"] );
				const string cstrMail( pRequest->mapParams["us_email"] );
				const string cstrAccView( pRequest->mapParams["us_access_view"] );
				const string cstrAccDL( pRequest->mapParams["us_access_dl"] );
				const string cstrAccComments( pRequest->mapParams["us_access_comments"] );
				const string cstrAccUpload( pRequest->mapParams["us_access_upload"] );
				const string cstrAccEdit( pRequest->mapParams["us_access_edit"] );
				const string cstrAccAdmin( pRequest->mapParams["us_access_admin"] );
				const string cstrAccSignup( pRequest->mapParams["us_access_signup"] );

				// Do we have data from the compulsory user parameters?
				if( cstrLogin.empty( ) || cstrPass.empty( ) || cstrPass2.empty( ) || cstrMail.empty( ) )
				{
					// The Trinity Edition - Modification Begins
					// Created Links to either "Make Corrections" or "Start Over"
					// when creating a NEW USER FAILS because ALL FIELDS WERE NOT FILLED IN

					pResponse->strContent += "<p class=\"failed_message_users\">" + gmapLANG_CFG["users_fields_create_fail"] + "</p>\n";
					pResponse->strContent += "<p class=\"failed_message_users\">" + gmapLANG_CFG["users_do_q"] + "</p>\n";
					pResponse->strContent += "<p class=\"failed_message_users\"><a title=\"" + gmapLANG_CFG["users_make_correct"] + "\" href=\"" + string( JS_BACK ) + "\">" + gmapLANG_CFG["users_make_correct"] + "</a><span class=\"pipe\">|</span><a title=\"" + gmapLANG_CFG["users_start_over"] + "\" href=\"" + RESPONSE_STR_USERS_HTML + "\">" + gmapLANG_CFG["users_start_over"] + "</a></p>\n\n";                

					// Output common HTML tail
					HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_USERS ) );

					return;
				}
				else
				{
					// Do the passwords match?
					if( cstrPass == cstrPass2 )
					{
						// Does the user already exist?
						if( m_pUsers->getItem( cstrLogin ) )
						{
							// The Trinity Edition - Modification Begins
							// Created Links to either "Make Corrections" or "Start Over"
							// when creating a NEW USER FAILS because THE USER ALREADY EXISTS

							pResponse->strContent += "<p class=\"failed_message_users\">" + UTIL_Xsprintf( gmapLANG_CFG["users_exists_create_fail"].c_str( ), UTIL_RemoveHTML( cstrLogin ).c_str( ) ) + "</p>\n";
							pResponse->strContent += "<p class=\"failed_message_users\">" + gmapLANG_CFG["users_do_q"] + "</p>\n";
							pResponse->strContent += "<p class=\"failed_message_users\"><a title=\"" + gmapLANG_CFG["users_make_correct"] + "\" href=\"" + string( JS_BACK ) + "\">" + gmapLANG_CFG["users_make_correct"] + "</a><span class=\"pipe\">|</span><a title=\"" + gmapLANG_CFG["users_start_over"] + "\" href=\"" + RESPONSE_STR_USERS_HTML + "\">" + gmapLANG_CFG["users_start_over"] + "</a></p>\n\n";    

							// Output common HTML tail
							HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_USERS ) );

							return;
						}
						else
						{
							// Build the users access level
							unsigned char ucAccess = 0;

							if( cstrAccView == "on" )
								ucAccess += ACCESS_VIEW;
							if( cstrAccDL == "on" )
								ucAccess += ACCESS_DL;
							if( cstrAccComments == "on" )
								ucAccess += ACCESS_COMMENTS;
							if( cstrAccUpload == "on" )
								ucAccess += ACCESS_UPLOAD;
							if( cstrAccEdit == "on" )
								ucAccess += ACCESS_EDIT;
							if( cstrAccAdmin == "on" )
								ucAccess += ACCESS_ADMIN;
							if( cstrAccSignup == "on" )
								ucAccess += ACCESS_SIGNUP;

							// Add the user
							addUser( cstrLogin, cstrPass, ucAccess, cstrMail );

							// The Trinity Edition - Modification Begins
							// The following places the Return to Users link in its own paragraph
							// as opposed to immediately following the "Created user..." message.

							pResponse->strContent += "<p class=\"created_message_users\">" + UTIL_Xsprintf( gmapLANG_CFG["users_created_user"].c_str( ), UTIL_RemoveHTML( cstrLogin ).c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_users"] + "\" href=\"" + RESPONSE_STR_USERS_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";

							// Output common HTML tail
							HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_USERS ) );

							return;
						}
					}
					else
					{
						// The Trinity Edition - Modification Begins
						// Created Links to either "Make Corrections" or "Start Over"
						// when creating a NEW USER FAILS because THE PASSWORDS DO NOT MATCH

						pResponse->strContent += "<p class=\"failed_message_users\">" + gmapLANG_CFG["users_password_create_fail"] + "</p>\n";
						pResponse->strContent += "<p class=\"failed_message_users\">" + gmapLANG_CFG["users_do_q"] + "</p>\n";
						pResponse->strContent += "<p class=\"failed_message_users\"><a title=\"" + gmapLANG_CFG["users_make_correct"] + "\" href=\"" + string( JS_BACK ) + "\">" + gmapLANG_CFG["users_make_correct"] + "</a><span class=\"pipe\">|</span><a title=\"" + gmapLANG_CFG["users_start_over"] + "\" href=\"" + RESPONSE_STR_USERS_HTML + "\">" + gmapLANG_CFG["users_start_over"] + "</a></p>\n\n";

						// Output common HTML tail
						HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_USERS ) );

						return;
					}
				}
			}
#if defined ( XBNBT_MYSQL )
			// XBNBT mySQL users integration
		}
#endif

		// Get the user details from the submitted action request
		const string cstrPass( pRequest->mapParams["us_password"] );
		const string cstrPass2( pRequest->mapParams["us_password_verify"] );
		const string cstrMail( pRequest->mapParams["us_email"] );
		const string cstrAccView( pRequest->mapParams["us_access_view"] );
		const string cstrAccDL( pRequest->mapParams["us_access_dl"] );
		const string cstrAccComments( pRequest->mapParams["us_access_comments"] );
		const string cstrAccUpload( pRequest->mapParams["us_access_upload"] );
		const string cstrAccEdit( pRequest->mapParams["us_access_edit"] );
		const string cstrAccAdmin( pRequest->mapParams["us_access_admin"] );
		const string cstrAccSignup( pRequest->mapParams["us_access_signup"] );
		const string cstrUser( UTIL_EscapedToString( UTIL_RemoveHTML( pRequest->mapParams["user"] ) ) );
		const string cstrAction( pRequest->mapParams["action"] );
		const string cstrOK( pRequest->mapParams["ok"] );


		//
		// edit user
		//

		if( cstrAction == "edit" )
		{
			CAtom *pUserToEdit = m_pUsers->getItem( cstrUser );

			// Does the user exist?
			if( pUserToEdit && pUserToEdit->isDicti( ) )
			{
				// Are you sure it's ok to edit the user?
				if( cstrOK == "1" )
				{
#if defined ( XBNBT_MYSQL )
					// XBNBT MySQL Users Integration
					if( !gbMySQLUsersOverrideUsers )
					{
#endif
						// Do we have the users password and verification password?
						if( !cstrPass.empty( ) && !cstrPass2.empty( ) )
						{
							// Are they the same?
							if( cstrPass == cstrPass2 )
							{
								// Set the users password

								const string cstrA1( cstrUser + ":" + gstrRealm + ":" + cstrPass );

								unsigned char szMD5[16];
								memset( szMD5, 0, sizeof( szMD5 ) / sizeof( unsigned char ) );

								MD5_CTX md5;

								MD5Init( &md5 );
								MD5Update( &md5, (const unsigned char *)cstrA1.c_str( ), (unsigned int)cstrA1.size( ) );
								MD5Final( szMD5, &md5 );

								( (CAtomDicti *)pUserToEdit )->setItem( "md5", new CAtomString( string( (char *)szMD5, sizeof( szMD5 ) ) ) );
							}
							else
							{
								// The Trinity Edition - Modification Begins
								// Created Links to either "Make Corrections" or "Start Over"
								// when EDITING USER FAILS because THE PASSWORDS DO NOT MATCH

								pResponse->strContent += "<p class=\"failed_message_users\">" + gmapLANG_CFG["users_password_edit_fail"] + "</p>\n";
								pResponse->strContent += "<p class=\"failed_message_users\">" + gmapLANG_CFG["users_do_q"] + "</p>\n";
								pResponse->strContent += "<p class=\"failed_message_users\"><a title=\"" + gmapLANG_CFG["users_make_correct"] + "\" href=\"" + string( JS_BACK ) + "\">" + gmapLANG_CFG["users_make_correct"] + "</a><span class=\"pipe\">|</span><a title=\"" + gmapLANG_CFG["users_start_over"] + "\" href=\"" + RESPONSE_STR_USERS_HTML + "\">" + gmapLANG_CFG["users_start_over"] + "</a></p>\n\n";

								// Output common HTML tail
								HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_USERS ) );

								return;
							}
						}

						// Set the users email address
						( (CAtomDicti *)pUserToEdit )->setItem( "email", new CAtomString( cstrMail ) );

#if defined ( XBNBT_MYSQL )
						// XBNBT mySQL users integration
					}
#endif

					// Construct the users access level
					unsigned char ucAccess = 0;

					if( cstrAccView == "on" )
						ucAccess += ACCESS_VIEW;
					if( cstrAccDL == "on" )
						ucAccess += ACCESS_DL;
					if( cstrAccComments == "on" )
						ucAccess += ACCESS_COMMENTS;
					if( cstrAccUpload == "on" )
						ucAccess += ACCESS_UPLOAD;
					if( cstrAccEdit == "on" )
						ucAccess += ACCESS_EDIT;
					if( cstrAccAdmin == "on" )
						ucAccess += ACCESS_ADMIN;
					if( cstrAccSignup == "on" )
						ucAccess += ACCESS_SIGNUP;

					// Set the users access level
					( ( CAtomDicti * )pUserToEdit )->setItem( "access", new CAtomLong( ucAccess ) );

#if defined ( XBNBT_MYSQL )
					// XBNBT MySQL Users Integration
					if( gbMySQLUsersOverrideUsers )
						UTIL_SetMySQLUsersMemberAccess( cstrUser, ucAccess );
					else
#endif
						// Original code - Save the users
						saveUsers( );

					// Inform the operator that the user was modified
					pResponse->strContent += "<p class=\"created_message_users\">" + UTIL_Xsprintf( gmapLANG_CFG["users_edited_user"].c_str( ), cstrUser.c_str( ) , string( "<a title=\"" + gmapLANG_CFG["navbar_users"] + "\" href=\"" + RESPONSE_STR_USERS_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n\n" ;

					// Output common HTML tail
					HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_USERS ) );

					return;
				}
				else
				{
					// Compose an edit users page view

					unsigned char ucAccess = 0;

					CAtom *pAccessToEdit = ( (CAtomDicti *)pUserToEdit )->getItem( "access" );

					if( pAccessToEdit && dynamic_cast<CAtomLong *>( pAccessToEdit ) )
						ucAccess = (unsigned char)dynamic_cast<CAtomLong *>( pAccessToEdit )->getValue( );

					pResponse->strContent += "<div class=\"edit_users\">\n";
					pResponse->strContent += "<table class=\"edit_users\">\n";
					pResponse->strContent += "<tr class=\"edit_users\">\n";
					pResponse->strContent += "<td class=\"edit_users\">\n";
					pResponse->strContent += "<form method=\"get\" name=\"editusers\" action=\"" + RESPONSE_STR_USERS_HTML + "\">\n";
					// Edit User
					pResponse->strContent += "<p class=\"edit_users\">" + UTIL_Xsprintf( gmapLANG_CFG["users_editing_user"].c_str( ), cstrUser.c_str( ) ) + "</p>\n\n" ;
					pResponse->strContent += "<div class=\"edit_input_users\"><input name=\"user\" type=hidden value=\"" + cstrUser + "\"></div>\n";
					pResponse->strContent += "<div class=\"edit_input_users\"><input name=\"action\" type=hidden value=\"edit\"></div>\n";
					pResponse->strContent += "<div class=\"edit_input_users\"><input name=\"ok\" type=hidden value=1></div>\n";

#if defined ( XBNBT_MYSQL )
					// XBNBT MySQL Users Integration
					if( !gbMySQLUsersOverrideUsers )
					{
#endif

						// Original code
						pResponse->strContent += "<div class=\"edit_input_users\"><input id=\"id_password\" name=\"us_password\" alt=\"[" + gmapLANG_CFG["users_password"] + "]\" type=password size=20> <label for=\"id_password\">" + gmapLANG_CFG["users_password"] + "</label><br>\n</div>\n";
						pResponse->strContent += "<div class=\"edit_input_users\"><input id=\"id_verify_password\" name=\"us_password_verify\" alt=\"[" + gmapLANG_CFG["users_verify_password"] + "]\" type=password size=20> <label for=\"id_verify_password\">" + gmapLANG_CFG["users_verify_password"] + "</label><br><br>\n</div>\n";
						pResponse->strContent += "<div class=\"edit_input_users\"><input id=\"id_email\" name=\"us_email\" alt=\"[" + gmapLANG_CFG["email"] + "]\" type=text size=40";                   

#if defined ( XBNBT_MYSQL )
					// XBNBT MySQL Users Integration
					}
#endif

					CAtom *pMailToEdit = ( (CAtomDicti *)pUserToEdit )->getItem( "email" );

#if defined ( XBNBT_MYSQL )
					// XBNBT MySQL Users Integration
					if( gbMySQLUsersOverrideUsers )
					{
						if( pMailToEdit )
							pResponse->strContent += "<div><a title=\"" + pMailToEdit->toString( ) + "\" href=\"mailto:" + pMailToEdit->toString( ) + "\">" + pMailToEdit->toString( ) + "</a><br><br></div>\n";
					}
					else
					{
#endif

						// Original code
						if( pMailToEdit )
							pResponse->strContent += " value=\"" + pMailToEdit->toString( ) + "\"";

						pResponse->strContent += "> <label for=\"id_email\">" + gmapLANG_CFG["email"] + "</label><br><br></div>\n";

#if defined ( XBNBT_MYSQL )
					// XBNBT MySQL Users Integration
					}
#endif

					// Display the users access level
					pResponse->strContent += "<div class=\"edit_input_users\"><input id=\"id_view\" name=\"us_access_view\" alt=\"[" + gmapLANG_CFG["users_view_access"] + "]\" type=checkbox";

					if( ucAccess & ACCESS_VIEW )
						pResponse->strContent += " checked";

					pResponse->strContent += "> <label for=\"id_view\">" + gmapLANG_CFG["users_view_access"] + "</label><br></div>\n";
					pResponse->strContent += "<div class=\"edit_input_users\"><input id=\"id_download\" name=\"us_access_dl\" alt=\"[" + gmapLANG_CFG["users_dl_access"] + "]\" type=checkbox";

					if( ucAccess & ACCESS_DL )
						pResponse->strContent += " checked";

					pResponse->strContent += "> <label for=\"id_download\">" + gmapLANG_CFG["users_dl_access"] + "</label><br></div>\n";
					pResponse->strContent += "<div class=\"edit_input_users\"><input id=\"id_comments\" name=\"us_access_comments\" alt=\"[" + gmapLANG_CFG["users_comments_access"] + "]\" type=checkbox";

					if( ucAccess & ACCESS_COMMENTS )
						pResponse->strContent += " checked";

					pResponse->strContent += "> <label for=\"id_comments\">" + gmapLANG_CFG["users_comments_access"] + "</label><br></div>\n";
					pResponse->strContent += "<div class=\"edit_input_users\"><input id=\"id_upload\" name=\"us_access_upload\"  alt=\"[" + gmapLANG_CFG["users_upload_access"] + "]\"type=checkbox";

					if( ucAccess & ACCESS_UPLOAD )
						pResponse->strContent += " checked";

					pResponse->strContent += "> <label for=\"id_upload\">" + gmapLANG_CFG["users_upload_access"] + "</label><br></div>\n";
					pResponse->strContent += "<div class=\"edit_input_users\"><input id=\"id_edit\" name=\"us_access_edit\" alt=\"[" + gmapLANG_CFG["users_edit_access"] + "]\" type=checkbox";

					if( ucAccess & ACCESS_EDIT )
						pResponse->strContent += " checked";

					pResponse->strContent += "> <label for=\"id_edit\">" + gmapLANG_CFG["users_edit_access"] + "</label><br></div>\n";
					pResponse->strContent += "<div class=\"edit_input_users\"><input id=\"id_admin\" name=\"us_access_admin\" alt=\"[" + gmapLANG_CFG["users_admin_access"] + "]\" type=checkbox";

					if( ucAccess & ACCESS_ADMIN )
						pResponse->strContent += " checked";

					pResponse->strContent += "> <label for=\"id_admin\">" + gmapLANG_CFG["users_admin_access"] + "</label><br></div>\n";
					pResponse->strContent += "<div class=\"edit_input_users\"><input id=\"id_signup\" name=\"us_access_signup\" alt=\"[" + gmapLANG_CFG["users_signup_access"] + "]\" type=checkbox";

					if( ucAccess & ACCESS_SIGNUP )
						pResponse->strContent += " checked";

					pResponse->strContent += "> <label for=\"id_signup\">" + gmapLANG_CFG["users_signup_access"] + "</label><br><br></div>\n";

					// The Trinity Edition - Addition Begins
					// The following creates a CANCEL EDIT button
					// when EDITING A USER                  pResponse->strContent += "<div>\n";

					pResponse->strContent += "<div class=\"edit_input_users\">\n";
					
					pResponse->strContent += Button_Submit( "submit_edit", string( gmapLANG_CFG["edit_user"] ) );
					pResponse->strContent += Button_Back( "cancel_edit", string( gmapLANG_CFG["cancel"] ) );
					
					pResponse->strContent += "\n</div>\n";

					// finish
					pResponse->strContent += "</form>\n</td>\n</tr>\n</table>\n</div>\n";

					// Output common HTML tail
					HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_USERS ) );

					return;
				}
			}
			else
			{
				// The Trinity Edition - Modification Begins
				// Modified the "Return to Users Page" to use BROWSER BACK
				// as opposed to reloading the Users Page
				// when EDITING USER FAILS because THE USER DOES NOT EXIST

				pResponse->strContent += "<p class=\"created_message_users\">" + UTIL_Xsprintf( gmapLANG_CFG["users_edit_nouser"].c_str( ), cstrUser.c_str( ) ) + "</p>\n\n" ;
				pResponse->strContent += "<p class=\"failed_message_users\"><a title=\"" + gmapLANG_CFG["previous_page"] + "\" href=\"" + string( JS_BACK ) + "\">" + gmapLANG_CFG["previous_page"] + "</a></p>\n";

				// Output common HTML tail
				HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_USERS ) );

				return;
			}
		}

		//
		// delete user
		//

		else if( cstrAction == "delete" )
		{

#if defined ( XBNBT_MYSQL )
			// XBNBY mySQL users integration
			if( !gbMySQLUsersOverrideUsers )
			{
#endif

				// Are you sure it is ok to delete the user?
				if( cstrOK == "1" )
				{
					// Deelete the user
					deleteUser( cstrUser );
 
					// Inform the operator that the user was deleted
					pResponse->strContent += "<p class=\"deleted_users\">" + UTIL_Xsprintf( gmapLANG_CFG["users_deleted_user"].c_str( ), cstrUser.c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_users"] + "\" href=\"" + RESPONSE_STR_USERS_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n\n" ;

					// Output common HTML tail
					HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_USERS ) );

					return;
				}
				else
				{
					// The Trinity Edition - Modification Begins
					// The following replaces the OK response with a YES | NO option
					// when DELETING A USER
					// Are you sure you want to delete the user

					pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["users_delete_q"].c_str( ), cstrUser.c_str( ) ) + "</p>\n\n" ;
					pResponse->strContent += "<p class=\"delete\"><a title=\"" + gmapLANG_CFG["yes"] + "\" href=\"" + RESPONSE_STR_USERS_HTML + "?user=" + cstrUser + "&amp;action=delete&amp;ok=1\">" + gmapLANG_CFG["yes"] + "</a> | <a title=\"" + gmapLANG_CFG["no"] + "\" href=\"" + string( JS_BACK ) + "\">" + gmapLANG_CFG["no"] + "</a></p>\n";

					// Output common HTML tail
					HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_USERS ) );

					return;
				}

#if defined ( XBNBT_MYSQL )
				// XBNBY mySQL users integration
			}
#endif

		}

#if defined ( XBNBT_MYSQL )
		// XBNBT MySQL Users Integration
		if( !gbMySQLUsersOverrideUsers )
		{
#endif

			//
			// create user
			//

			// Compose a new users page view
			pResponse->strContent += "<div class=\"create_users\">\n";
			pResponse->strContent += "<table class=\"create_users\">\n";
			pResponse->strContent += "<tr class=\"create_users\">\n";
			pResponse->strContent += "<td class=\"create_users\">\n";
			pResponse->strContent += "<form method=\"get\" name=\"createusers\" action=\"" + RESPONSE_STR_USERS_HTML + "\">\n";
			pResponse->strContent += "<p class=\"create_users\">" + gmapLANG_CFG["users_create_user_title"] + "</p>\n";
			pResponse->strContent += "<div class=\"input_create_users\"><input id=\"id_login\" name=\"us_login\" alt=\"[" + gmapLANG_CFG["login"] + "]\" type=text size=24> <label for=\"id_login\">" + gmapLANG_CFG["login"] + "</label><br><br></div>\n";
			pResponse->strContent += "<div class=\"input_create_users\"><input id=\"id_password\" name=\"us_password\" alt=\"[" + gmapLANG_CFG["password"] + "]\" type=password size=20> <label for=\"id_password\">" + gmapLANG_CFG["password"] + "</label><br></div>\n";
			pResponse->strContent += "<div class=\"input_create_users\"><input id=\"id_verify\" name=\"us_password_verify\" alt=\"[" + gmapLANG_CFG["verify_password"] + "]\" type=password size=20> <label for=\"id_verify\">" + gmapLANG_CFG["verify_password"] + "</label><br><br></div>\n";
			pResponse->strContent += "<div class=\"input_create_users\"><input id=\"id_email\" name=\"us_email\" alt=\"[" + gmapLANG_CFG["email"] + "]\" type=text size=40> <label for=\"id_email\">" + gmapLANG_CFG["email"] + "</label><br><br></div>\n";
			pResponse->strContent += "<div class=\"input_create_users\"><input id=\"id_view\" name=\"us_access_view\" alt=\"[" + gmapLANG_CFG["users_view_access"] + "]\" type=checkbox> <label for=\"id_view\">" + gmapLANG_CFG["users_view_access"] + "</label><br></div>\n";
			pResponse->strContent += "<div class=\"input_create_users\"><input id=\"id_download\" name=\"us_access_dl\" alt=\"[" + gmapLANG_CFG["users_dl_access"] + "]\" type=checkbox> <label for=\"id_download\">" + gmapLANG_CFG["users_dl_access"] + "</label><br></div>\n";
			pResponse->strContent += "<div class=\"input_create_users\"><input id=\"id_comments\" name=\"us_access_comments\" alt=\"[" + gmapLANG_CFG["users_comments_access"] + "]\" type=checkbox> <label for=\"id_comments\">" + gmapLANG_CFG["users_comments_access"] + "</label><br></div>\n";
			pResponse->strContent += "<div class=\"input_create_users\"><input id=\"id_upload\" name=\"us_access_upload\" alt=\"[" + gmapLANG_CFG["users_upload_access"] + "]\" type=checkbox> <label for=\"id_upload\">" + gmapLANG_CFG["users_upload_access"] + "</label><br></div>\n";
			pResponse->strContent += "<div class=\"input_create_users\"><input id=\"id_edit\" name=\"us_access_edit\" alt=\"[" + gmapLANG_CFG["users_edit_access"] + "]\" type=checkbox> <label for=\"id_edit\">" + gmapLANG_CFG["users_edit_access"] + "</label><br></div>\n";
			pResponse->strContent += "<div class=\"input_create_users\"><input id=\"id_admin\" name=\"us_access_admin\" alt=\"[" + gmapLANG_CFG["users_admin_access"] + "]\" type=checkbox> <label for=\"id_admin\">" + gmapLANG_CFG["users_admin_access"] + "</label><br></div>\n";
			pResponse->strContent += "<div class=\"input_create_users\"><input id=\"id_signup\" name=\"us_access_signup\" alt=\"[" + gmapLANG_CFG["users_signup_access"] + "]\" type=checkbox> <label for=\"id_signup\">" + gmapLANG_CFG["users_signup_access"] + "</label><br><br></div>\n";

			// Adds Cancel button beside Create User
			pResponse->strContent += "<div class=\"create_users_buttons\">\n";

			pResponse->strContent += Button_Submit( "submit_create", string( gmapLANG_CFG["create_user"] ) );
			pResponse->strContent += Button_Back( "cancel_create", string( gmapLANG_CFG["cancel"] ) );

			pResponse->strContent += "\n</div>\n";

			// finish
			pResponse->strContent += "</form>\n</td>\n</tr>\n</table>\n</div>\n";

#if defined ( XBNBT_MYSQL )
			// XBNBT MySQL Users Integration
		}
#endif

		//
		// user table
		//

		// Does the users database exist?
		if( m_pUsers )
		{
			// Are there any registered users?
			if( m_pUsers->isEmpty( ) )
			{
				// There are no registered users!
				pResponse->strContent += "<p class=\"no_users\">" + gmapLANG_CFG["users_nousers_warning"] + "</p>\n";

				// Output common HTML tail
				HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_USERS ) );

				return;
			}

			// Populate the users structure for display

			map<string, CAtom *> *pmapDicti = m_pUsers->getValuePtr( );

			const unsigned long culKeySize( (unsigned long)pmapDicti->size( ) );

			// add the users into this structure one by one and sort it afterwards

			struct user_t *pUsersT = new struct user_t[culKeySize];

			unsigned long ulCount = 0;

			CAtom *pMD5 = 0;
			CAtom *pAccess = 0;
			CAtom *pMail = 0;
			CAtom *pCreated = 0;

			for( map<string, CAtom *> :: iterator it = pmapDicti->begin( ); it != pmapDicti->end( ); it++ )
			{
				pUsersT[ulCount].strLogin = (*it).first;
				pUsersT[ulCount].strLowerLogin = UTIL_ToLower( pUsersT[ulCount].strLogin );
				pUsersT[ulCount].ucAccess = m_ucGuestAccess;

				if( (*it).second->isDicti( ) )
				{
					pMD5 = ( (CAtomDicti *)(*it).second )->getItem( "md5" );
					pAccess = ( (CAtomDicti *)(*it).second )->getItem( "access" );
					pMail = ( (CAtomDicti *)(*it).second )->getItem( "email" );
					pCreated = ( (CAtomDicti *)(*it).second )->getItem( "created" );

					if( pMD5 )
						pUsersT[ulCount].strMD5 = pMD5->toString( );

					if( pMail )
					{
						pUsersT[ulCount].strMail = pMail->toString( );
						pUsersT[ulCount].strLowerMail = UTIL_ToLower( pUsersT[ulCount].strMail );
					}

					if( pAccess && dynamic_cast<CAtomLong *>( pAccess ) )
						pUsersT[ulCount].ucAccess = (unsigned char)dynamic_cast<CAtomLong *>( pAccess )->getValue( );

					if( pCreated )
						pUsersT[ulCount].strCreated = pCreated->toString( );
				}

				ulCount++;
			}

			// Sort the users
			const string cstrSort( pRequest->mapParams["sort"] );

			if( !cstrSort.empty( ) )
			{
				const unsigned char cucSort( (unsigned char)atoi( cstrSort.c_str( ) ) );

				switch ( cucSort )
				{
				case SORTU_ALOGIN:
					qsort( pUsersT, culKeySize, sizeof( struct user_t ), asortuByLogin );
					break;
				case SORTU_AACCESS:
					qsort( pUsersT, culKeySize, sizeof( struct user_t ), asortuByAccess );
					break;
				case SORTU_AEMAIL:
					qsort( pUsersT, culKeySize, sizeof( struct user_t ), asortuByMail );
					break;
				case SORTU_ACREATED:
					qsort( pUsersT, culKeySize, sizeof( struct user_t ), asortuByCreated );
					break;
				case SORTU_DLOGIN:
					qsort( pUsersT, culKeySize, sizeof( struct user_t ), dsortuByLogin );
					break;
				case SORTU_DACCESS:
					qsort( pUsersT, culKeySize, sizeof( struct user_t ), dsortuByAccess );
					break;
				case SORTU_DEMAIL:
					qsort( pUsersT, culKeySize, sizeof( struct user_t ), dsortuByMail );
					break;
				case SORTU_DCREATED:
				default:
					qsort( pUsersT, culKeySize, sizeof( struct user_t ), dsortuByCreated );
				}
			}
			else
			{
				// default action is to sort by created

				qsort( pUsersT, culKeySize, sizeof( struct user_t ), dsortuByCreated );
			}

			// some preliminary search crap

			const string cstrSearch( pRequest->mapParams["search"] );
			const string cstrLowerSearch( UTIL_ToLower( cstrSearch ) );
			const string cstrSearchResp( UTIL_StringToEscaped( cstrSearch ) );

#if defined ( XBNBT_MYSQL )
			// XBNBT MySQL Users Integration
			if( !gbMySQLUsersOverrideUsers )
#endif

				pResponse->strContent += "\n<hr class=\"users_hr\">\n\n";

			// Search results for:
			if( !cstrSearch.empty( ) )
				pResponse->strContent += "<p class=\"search_results\">" + UTIL_Xsprintf( gmapLANG_CFG["search_results_for"].c_str( ), string( "\"<span class=\"filtered_by\">" + UTIL_RemoveHTML( cstrSearch ) + "</span>\"").c_str( ) ) + "</p>\n";

			// which page are we viewing

			unsigned long ulStart = 0;
			unsigned int uiOverridePerPage = 0;

			const string cstrPerPage( pRequest->mapParams["per_page"] );

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

			// Count matching users for top of page
			unsigned long ulFound = 0;

			// Loop through the users
			for( unsigned long ulKey = 0; ulKey < culKeySize; ulKey++ )
			{
				// Do we have a search value?
				if( !cstrSearch.empty( ) )
				{
					// If the entry does not contain the search value then get the next entry
					if( pUsersT[ulKey].strLowerLogin.find( cstrLowerSearch ) == string :: npos )
						continue;
				}

				// Found a user! Increment the count.
				ulFound++;
			}

			// What was the search criteria used?
			pResponse->strContent += "<p class=\"search_filter\">\n";

			if( !cstrSearch.empty() )
				pResponse->strContent += "<span class=\"search_results_alt\">" + gmapLANG_CFG["result_search"] + ": \"</span><span class=\"filtered_by\">" + UTIL_RemoveHTML( cstrSearch ) + "</span>\"\n";

			pResponse->strContent += "</p>\n\n";

			// How many results found?
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
			if( m_pAllowed && m_bSearch )
			{
				pResponse->strContent += "<form class=\"search_users_top\" name=\"topsearch\" method=\"get\" action=\"" + RESPONSE_STR_USERS_HTML + "\">\n";

				if( !cstrPerPage.empty( ) )
					pResponse->strContent += "<p><input name=\"per_page\" type=hidden value=\"" + cstrPerPage + "\"></p>\n";

				if( !cstrSort.empty( ) )
					pResponse->strContent += "<p><input name=\"sort\" type=hidden value=\"" + cstrSort + "\"></p>\n";

				if( m_bUseButtons )
				{
					pResponse->strContent += "<p><label for=\"topusersearch\">" + gmapLANG_CFG["user_search"] + "</label> <input name=\"search\" id=\"topusersearch\" alt=\"[" + gmapLANG_CFG["user_search"] + "]\" type=text size=40>\n";

					pResponse->strContent += Button_Submit( "top_submit_search", gmapLANG_CFG["search"] );
					pResponse->strContent += Button_JS_Link( "top_clear_filter_and_search", gmapLANG_CFG["clear_filter_search"], "clear_search_and_filters( )" );

					pResponse->strContent += "</p>\n";
				}
				else
					pResponse->strContent += "<p><label for=\"topusersearch\">" + gmapLANG_CFG["user_search"] + "</label> <input name=\"search\" id=\"topusersearch\" alt=\"[" + gmapLANG_CFG["user_search"] + "]\" type=text size=40> <a title=\"" + gmapLANG_CFG["clear_filter_search"] + "\" href=\"" + RESPONSE_STR_USERS_HTML + "\">" + gmapLANG_CFG["clear_filter_search"] + "</a></p>\n";

				pResponse->strContent += "</form>\n\n";
			}

			// page numbers
			if( uiOverridePerPage > 0 )
			{
				const string cstrPage( pRequest->mapParams["page"] );

				if( !cstrPage.empty( ) )
					ulStart = (unsigned long)atoi( cstrPage.c_str( ) ) * uiOverridePerPage;

				pResponse->strContent += "<p class=\"pagenum_top_bar\">" + gmapLANG_CFG["jump_to_page"] + ": \n";

				for( unsigned long ulKey = 0; ulKey < ulFound; ulKey += uiOverridePerPage )
				{
					pResponse->strContent += " ";

					// don't link to current page
					if( ulKey != ulStart )
					{
						pResponse->strContent += "<a title=\"" + gmapLANG_CFG["jump_to_page"] + ": " + CAtomInt( ( ulKey / uiOverridePerPage ) + 1 ).toString( ) + "\" href=\"" + RESPONSE_STR_USERS_HTML + "?page=" + CAtomInt( ulKey / uiOverridePerPage ).toString( );

						if( !cstrPerPage.empty( ) )
							pResponse->strContent += "&amp;per_page=" + cstrPerPage;

						if( !cstrSort.empty( ) )
							pResponse->strContent += "&amp;sort=" + cstrSort;

						if( !cstrSearch.empty( ) )
							pResponse->strContent += "&amp;search=" + cstrSearchResp;

						pResponse->strContent += "\">";
					}

					pResponse->strContent += CAtomInt( ( ulKey / uiOverridePerPage ) + 1 ).toString( );

					if( ulKey != ulStart )
						pResponse->strContent += "</a>\n";

					// don't display a bar after the last page
					if( ulKey + uiOverridePerPage < ulFound )
						pResponse->strContent += "\n<span class=\"pipe\">|</span>";
				}

				pResponse->strContent += "</p>\n\n";
			}

			bool bFound = false;

			unsigned long ulAdded = 0;
			unsigned long ulSkipped = 0;

			// for correct page numbers after searching

			ulFound = 0;

			for( unsigned long ulKey = 0; ulKey < culKeySize; ulKey++ )
			{
				if( !cstrSearch.empty( ) )
				{
					// only display entries that match the search

					if( pUsersT[ulKey].strLowerLogin.find( cstrLowerSearch ) == string :: npos )
						continue;
				}

				ulFound++;

				// Have we choosen to display all the results or are there fewer results than a page?
				if( uiOverridePerPage == DISPLAY_ALL || (unsigned int)ulAdded < uiOverridePerPage )
				{											   
					// create the table and display the headers first and once
					if( !bFound )
					{
						pResponse->strContent += "<div class=\"users_table\">\n";
						pResponse->strContent += "<table summary=\"users\">\n";
						pResponse->strContent += "<tr><th class=\"name\">" + gmapLANG_CFG["login"];
						pResponse->strContent += "<br><a title=\"" + gmapLANG_CFG["sort_login_ascending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_USERS_HTML + "?sort=" + SORTUSTR_ALOGIN;
						
						if( !cstrPerPage.empty( ) )
							pResponse->strContent += "&amp;per_page=" + cstrPerPage;

						if( !cstrSearch.empty( ) )
							pResponse->strContent += "&amp;search=" + cstrSearchResp;

						pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a title=\"" + gmapLANG_CFG["sort_login_descending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_USERS_HTML + "?sort=" + SORTUSTR_DLOGIN;

						if( !cstrPerPage.empty( ) )
							pResponse->strContent += "&amp;per_page=" + cstrPerPage;

						if( !cstrSearch.empty( ) )
							pResponse->strContent += "&amp;search=" + cstrSearchResp;

						pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a>";
						pResponse->strContent += "</th><th>" + gmapLANG_CFG["access"];
						pResponse->strContent += "<br><a title=\"" + gmapLANG_CFG["sort_access_ascending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_USERS_HTML + "?sort=" + SORTUSTR_AACCESS;
						
						if( !cstrPerPage.empty( ) )
							pResponse->strContent += "&amp;per_page=" + cstrPerPage;

						if( !cstrSearch.empty( ) )
							pResponse->strContent += "&amp;search=" + cstrSearchResp;

						pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a title=\"" + gmapLANG_CFG["sort_access_descending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_USERS_HTML + "?sort=" + SORTUSTR_DACCESS;

						if( !cstrPerPage.empty( ) )
							pResponse->strContent += "&amp;per_page=" + cstrPerPage;

						if( !cstrSearch.empty( ) )
							pResponse->strContent += "&amp;search=" + cstrSearchResp;

						pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a>";
						pResponse->strContent += "</th><th>" + gmapLANG_CFG["email"];
						pResponse->strContent += "<br><a title=\"" + gmapLANG_CFG["sort_email_ascending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_USERS_HTML + "?sort=" + SORTUSTR_AEMAIL;
						
						if( !cstrPerPage.empty( ) )
							pResponse->strContent += "&amp;per_page=" + cstrPerPage;

						if( !cstrSearch.empty( ) )
							pResponse->strContent += "&amp;search=" + cstrSearchResp;

						pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a title=\"" + gmapLANG_CFG["sort_email_descending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_USERS_HTML + "?sort=" + SORTUSTR_DEMAIL;

						if( !cstrPerPage.empty( ) )
							pResponse->strContent += "&amp;per_page=" + cstrPerPage;

						if( !cstrSearch.empty( ) )
							pResponse->strContent += "&amp;search=" + cstrSearchResp;

						pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a>";
						pResponse->strContent += "</th><th>" + gmapLANG_CFG["created"];
						pResponse->strContent += "<br><a title=\"" + gmapLANG_CFG["sort_created_ascending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_USERS_HTML + "?sort=" + SORTUSTR_ACREATED;

						if( !cstrPerPage.empty( ) )
							pResponse->strContent += "&amp;per_page=" + cstrPerPage;

						if( !cstrSearch.empty( ) )
							pResponse->strContent += "&amp;search=" + cstrSearchResp;

						pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a title=\"" + gmapLANG_CFG["sort_created_descending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_USERS_HTML + "?sort=" + SORTUSTR_DCREATED;

						if( !cstrPerPage.empty( ) )
							pResponse->strContent += "&amp;per_page=" + cstrPerPage;

						if( !cstrSearch.empty( ) )
							pResponse->strContent += "&amp;search=" + cstrSearchResp;

						pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a>";
						pResponse->strContent += "</th><th>" + gmapLANG_CFG["admin"] + "</th></tr>\n";

						// signal table created and headers ouput once
						bFound = true;
					}

					if( ulSkipped == ulStart )
					{
						// output table rows

						if( ulAdded % 2 )
							pResponse->strContent += "<tr class=\"even\">\n";
						else
							pResponse->strContent += "<tr class=\"odd\">\n";

						pResponse->strContent += "<td class=\"name\">";
						pResponse->strContent += UTIL_RemoveHTML( pUsersT[ulKey].strLogin );
						pResponse->strContent += "</td><td>";
						pResponse->strContent += UTIL_AccessToString( pUsersT[ulKey].ucAccess );
						pResponse->strContent += "</td><td>";

						// The Trinity Edition - Modification Begins
						// The following makes user email addresses HYPERLINKED

						pResponse->strContent += "<a title=\"" + UTIL_RemoveHTML( pUsersT[ulKey].strMail ) + "\" href=\"mailto:"; 
						pResponse->strContent += UTIL_RemoveHTML( pUsersT[ulKey].strMail ); 
						pResponse->strContent += "\">"; 
						pResponse->strContent += UTIL_RemoveHTML( pUsersT[ulKey].strMail );
						pResponse->strContent += "</a>"; 
						pResponse->strContent += "</td><td>";

						// strip year and seconds from time
						if( !pUsersT[ulKey].strCreated.empty( ) )
							pResponse->strContent += pUsersT[ulKey].strCreated.substr( 5, pUsersT[ulKey].strCreated.size( ) - 8 );

#if defined ( XBNBT_MYSQL )
						// XBNBT MySQL Users Integration
						if( gbMySQLUsersOverrideUsers )
							pResponse->strContent += "</td><td>[<a title=\"" + gmapLANG_CFG["edit"] + "\" href=\"" + RESPONSE_STR_USERS_HTML + "?user=" + UTIL_StringToEscaped( pUsersT[ulKey].strLogin ) + "&amp;action=edit\">" + gmapLANG_CFG["edit"] + "</a>]</td></tr>\n";
						else    
#endif

							pResponse->strContent += "</td><td>[<a title=\"" + gmapLANG_CFG["edit"] + "\" href=\"" + RESPONSE_STR_USERS_HTML + "?user=" + UTIL_StringToEscaped( pUsersT[ulKey].strLogin ) + "&amp;action=edit\">" + gmapLANG_CFG["edit"] + "</a>] [<a title=\"" + gmapLANG_CFG["delete"] + "\" href=\"" + RESPONSE_STR_USERS_HTML + "?user=" + UTIL_StringToEscaped( pUsersT[ulKey].strLogin ) + "&amp;action=delete\">" + gmapLANG_CFG["delete"] + "</a>]</td></tr>\n";

						ulAdded++;
					}
					else
						ulSkipped++;
				}
			}

			// free the memory
			delete [] pUsersT;

			// some finishing touches

			if( bFound )
			{
				pResponse->strContent += "</table>\n";
				pResponse->strContent += "</div>\n\n";
			}

			// Bottom search
			if( ulFound && m_pAllowed && m_bSearch )
			{
				pResponse->strContent += "<form class=\"search_users\" name=\"bottomsearch\" method=\"get\" action=\"" + RESPONSE_STR_USERS_HTML + "\">\n";

				if( !cstrPerPage.empty( ) )
					pResponse->strContent += "<p><input name=\"per_page\" type=hidden value=\"" + cstrPerPage + "\"></p>\n";

				if( !cstrSort.empty( ) )
					pResponse->strContent += "<p><input name=\"sort\" type=hidden value=\"" + cstrSort + "\"></p>\n";

				if( m_bUseButtons )
				{
					pResponse->strContent += "<p class=\"search_users\"><label for=\"bottomusersearch\">" + gmapLANG_CFG["user_search"] + "</label> <input id=\"bottomusersearch\" name=\"search\" alt=\"[" + gmapLANG_CFG["user_search"] + "]\" type=text size=40>\n";

					pResponse->strContent += Button_Submit( "bottom_submit_search", gmapLANG_CFG["search"] );
					pResponse->strContent += Button_JS_Link( "bottom_clear_filter_and_search", gmapLANG_CFG["clear_filter_search"], "clear_search_and_filters( )" );

					pResponse->strContent += "</p>\n";
				}
				else
					pResponse->strContent += "<p class=\"search_users\"><label for=\"bottomusersearch\">" + gmapLANG_CFG["user_search"] + "</label> <input id=\"bottomusersearch\" name=\"search\" alt=\"[" + gmapLANG_CFG["user_search"] + "]\" type=text size=40> <a title=\"" + gmapLANG_CFG["clear_filter_search"] + "\" href=\"" + RESPONSE_STR_USERS_HTML + "\"> " + gmapLANG_CFG["clear_filter_search"] + "</a></p>\n";

				pResponse->strContent += "</form>\n\n";				
			}

			// page numbers
			if( ulFound && uiOverridePerPage > 0 )
			{
				pResponse->strContent += "<p class=\"pagenum_bottom_users\">" + gmapLANG_CFG["jump_to_page"] + ": \n";

				for( unsigned long ulCount2 = 0; ulCount2 < ulFound; ulCount2 += uiOverridePerPage )
				{
					pResponse->strContent += " ";

					// don't link to current page

					if( ulCount2 != ulStart )
					{
						pResponse->strContent += "<a title=\"" + gmapLANG_CFG["jump_to_page"] + ": " + CAtomInt( ( ulCount2 / uiOverridePerPage ) + 1 ).toString( ) + "\" href=\"" + RESPONSE_STR_USERS_HTML + "?page=" + CAtomInt( ulCount2 / uiOverridePerPage ).toString( );

						if( !cstrPerPage.empty( ) )
							pResponse->strContent += "&amp;per_page=" + cstrPerPage;

						if( !cstrSort.empty( ) )
							pResponse->strContent += "&amp;sort=" + cstrSort;

						if( !cstrSearch.empty( ) )
							pResponse->strContent += "&amp;search=" + cstrSearchResp;

						pResponse->strContent += "\">";
					}

					pResponse->strContent += CAtomInt( ( ulCount2 / uiOverridePerPage ) + 1 ).toString( );

					if( ulCount2 != ulStart )
						pResponse->strContent += "</a>\n";

					// don't display a bar after the last page

					if( ulCount2 + uiOverridePerPage < ulFound )
						pResponse->strContent += "\n<span class=\"pipe\">|</span>";
				}

				pResponse->strContent += "</p>\n\n";
			}
		}

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_USERS ) );
	}
	else
	{
		const string cstrPass0( pRequest->mapParams["us_old_password"] );
		const string cstrPass( pRequest->mapParams["us_password"] );
		const string cstrPass2( pRequest->mapParams["us_password_verify"] );
		const string cstrAction( pRequest->mapParams["action"] );
		const string cstrUser( UTIL_EscapedToString( UTIL_RemoveHTML( pRequest->user.strLogin ) ) );
		const string cstrMail( pRequest->mapParams["us_email"] );
		const string cstrOK( pRequest->mapParams["ok"] );

		if( cstrAction == "edit" )
		{
		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["users_page"], string( CSS_USERS ), string( ), NOT_INDEX, CODE_200 );


			CAtom *pUserToEdit = m_pUsers->getItem( cstrUser );

			// Does the user exist?
			if( pUserToEdit && pUserToEdit->isDicti( ) )
			{
				// Are you sure it's ok to edit the user?
				if( cstrOK == "1" )
				{
					const string cstrA0( cstrUser + ":" + gstrRealm + ":" + cstrPass0 );

					unsigned char szMD5[16];
					memset( szMD5, 0, sizeof( szMD5 ) / sizeof( unsigned char ) );

					MD5_CTX md5;

					MD5Init( &md5 );
					MD5Update( &md5, (const unsigned char *)cstrA0.c_str( ), (unsigned int)cstrA0.size( ) );
					MD5Final( szMD5, &md5 );

					if( ( (CAtomDicti *)pUserToEdit )->getItem( "md5" )->toString( ) == string( (char *)szMD5, sizeof( szMD5 ) ) )
					{
						// Do we have the users password and verification password?
						if( !cstrPass.empty( ) && !cstrPass2.empty( ) )
						{
							// Are they the same?
							if( cstrPass == cstrPass2 )
							{
								// Set the users password

								const string cstrA1( cstrUser + ":" + gstrRealm + ":" + cstrPass );

								unsigned char szMD5[16];
								memset( szMD5, 0, sizeof( szMD5 ) / sizeof( unsigned char ) );

								MD5_CTX md5;

								MD5Init( &md5 );
								MD5Update( &md5, (const unsigned char *)cstrA1.c_str( ), (unsigned int)cstrA1.size( ) );
								MD5Final( szMD5, &md5 );

								( (CAtomDicti *)pUserToEdit )->setItem( "md5", new CAtomString( string( (char *)szMD5, sizeof( szMD5 ) ) ) );
								// Set the users email address
								( (CAtomDicti *)pUserToEdit )->setItem( "email", new CAtomString( cstrMail ) );
								// Original code - Save the users
								saveUsers( );

								// Inform the operator that the user was modified
								pResponse->strContent += "<p class=\"created_message_users\">" + UTIL_Xsprintf( gmapLANG_CFG["users_edited_user2"].c_str( ), cstrUser.c_str( ) , string( "<a title=\"" + gmapLANG_CFG["navbar_users"] + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n\n" ;

								// Output common HTML tail
								HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_USERS ) );

								return;

							}
							else
							{
								pResponse->strContent += "<p class=\"failed_message_users\">" + gmapLANG_CFG["users_password_edit_fail3"] + "</p>\n";
								pResponse->strContent += "<p class=\"failed_message_users\">" + gmapLANG_CFG["users_do_q"] + "</p>\n";
								pResponse->strContent += "<p class=\"failed_message_users\"><a title=\"" + gmapLANG_CFG["users_make_correct"] + "\" href=\"" + string( JS_BACK ) + "\">" + gmapLANG_CFG["users_make_correct"] + "</a><span class=\"pipe\">|</span><a title=\"" + gmapLANG_CFG["users_start_over"] + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "\">" + gmapLANG_CFG["users_start_over"] + "</a></p>\n\n";

								// Output common HTML tail
								HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_USERS ) );

								return;
							}
						}
						if( cstrPass.empty( ) && cstrPass2.empty( ) )
						{

							// Set the users email address
							( (CAtomDicti *)pUserToEdit )->setItem( "email", new CAtomString( cstrMail ) );
							// Original code - Save the users
							saveUsers( );

							// Inform the operator that the user was modified
							pResponse->strContent += "<p class=\"created_message_users\">" + UTIL_Xsprintf( gmapLANG_CFG["users_edited_user3"].c_str( ), cstrUser.c_str( ) , string( "<a title=\"" + gmapLANG_CFG["navbar_users"] + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n\n" ;

							// Output common HTML tail
							HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_USERS ) );

						}
					}
					else
					{
						// The Trinity Edition - Modification Begins
						// Created Links to either "Make Corrections" or "Start Over"
						// when EDITING USER FAILS because THE PASSWORDS DO NOT MATCH

						pResponse->strContent += "<p class=\"failed_message_users\">" + gmapLANG_CFG["users_password_edit_fail2"] + "</p>\n";
						pResponse->strContent += "<p class=\"failed_message_users\">" + gmapLANG_CFG["users_do_q"] + "</p>\n";
						pResponse->strContent += "<p class=\"failed_message_users\"><a title=\"" + gmapLANG_CFG["users_make_correct"] + "\" href=\"" + string( JS_BACK ) + "\">" + gmapLANG_CFG["users_make_correct"] + "</a><span class=\"pipe\">|</span><a title=\"" + gmapLANG_CFG["users_start_over"] + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "\">" + gmapLANG_CFG["users_start_over"] + "</a></p>\n\n";

						// Output common HTML tail
						HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_USERS ) );

						return;
					}
				
				}
				else
				{
					// Compose an edit users page view

					pResponse->strContent += "<div class=\"edit_users\">\n";
					pResponse->strContent += "<table class=\"edit_users\">\n";
					pResponse->strContent += "<tr class=\"edit_users\">\n";
					pResponse->strContent += "<td class=\"edit_users\">\n";
					pResponse->strContent += "<form method=\"get\" name=\"editusers\" action=\"" + RESPONSE_STR_USERS_HTML + "\">\n";
					// Edit User
					pResponse->strContent += "<p class=\"edit_users\">" + UTIL_Xsprintf( gmapLANG_CFG["users_editing_user"].c_str( ), cstrUser.c_str( ) ) + "</p>\n\n" ;
					pResponse->strContent += "<div class=\"edit_input_users\"><input name=\"user\" type=hidden value=\"" + cstrUser + "\"></div>\n";
					pResponse->strContent += "<div class=\"edit_input_users\"><input name=\"action\" type=hidden value=\"edit\"></div>\n";
					pResponse->strContent += "<div class=\"edit_input_users\"><input name=\"ok\" type=hidden value=1></div>\n";

					// Original code
					pResponse->strContent += "<div class=\"edit_input_users\"><input id=\"id_old_password\" name=\"us_old_password\" alt=\"[" + gmapLANG_CFG["users_old_password"] + "]\" type=password size=20> <label for=\"id_old_password\">" + gmapLANG_CFG["users_password_old"] + "</label><br>\n</div>\n";
					pResponse->strContent += "<div class=\"edit_input_users\"><input id=\"id_password\" name=\"us_password\" alt=\"[" + gmapLANG_CFG["users_password"] + "]\" type=password size=20> <label for=\"id_password\">" + gmapLANG_CFG["users_password"] + "</label><br>\n</div>\n";
					pResponse->strContent += "<div class=\"edit_input_users\"><input id=\"id_verify_password\" name=\"us_password_verify\" alt=\"[" + gmapLANG_CFG["users_verify_password"] + "]\" type=password size=20> <label for=\"id_verify_password\">" + gmapLANG_CFG["users_verify_password"] + "</label><br><br>\n</div>\n";
					pResponse->strContent += "<div class=\"edit_input_users\"><input id=\"id_email\" name=\"us_email\" alt=\"[" + gmapLANG_CFG["email"] + "]\" type=text size=40";                   


					CAtom *pMailToEdit = ( (CAtomDicti *)pUserToEdit )->getItem( "email" );


						// Original code
						if( pMailToEdit )
							pResponse->strContent += " value=\"" + pMailToEdit->toString( ) + "\"";

						pResponse->strContent += "> <label for=\"id_email\">" + gmapLANG_CFG["email"] + "</label><br><br></div>\n";


					// The Trinity Edition - Addition Begins
					// The following creates a CANCEL EDIT button
					// when EDITING A USER                  pResponse->strContent += "<div>\n";

					pResponse->strContent += "<div class=\"edit_input_users\">\n";
					
					pResponse->strContent += Button_Submit( "submit_edit", string( gmapLANG_CFG["edit_user"] ) );
					pResponse->strContent += Button_Back( "cancel_edit", string( gmapLANG_CFG["cancel"] ) );
					
					pResponse->strContent += "\n</div>\n";

					// finish
					pResponse->strContent += "</form>\n</td>\n</tr>\n</table>\n</div>\n";

					// Output common HTML tail
					HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_USERS ) );

					return;
				}
			}
			else
			{
				// The Trinity Edition - Modification Begins
				// Modified the "Return to Users Page" to use BROWSER BACK
				// as opposed to reloading the Users Page
				// when EDITING USER FAILS because THE USER DOES NOT EXIST

				pResponse->strContent += "<p class=\"created_message_users\">" + UTIL_Xsprintf( gmapLANG_CFG["users_edit_nouser"].c_str( ), cstrUser.c_str( ) ) + "</p>\n\n" ;
				pResponse->strContent += "<p class=\"failed_message_users\"><a title=\"" + gmapLANG_CFG["previous_page"] + "\" href=\"" + string( JS_BACK ) + "\">" + gmapLANG_CFG["previous_page"] + "</a></p>\n";

				// Output common HTML tail
				HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_USERS ) );

				return;
			}
		}
		else
		{
			// Not authorised

			// Output common HTML head
			HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["index_page"], string( CSS_USERS ), string( ), NOT_INDEX, CODE_401 );

			// Output common HTML tail
			HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_USERS ) );
		}
	}
}
