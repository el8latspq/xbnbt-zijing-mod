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
#include "bencode.h"
#include "config.h"
#include "html.h"
#include "md5.h"
#include "sort.h"
#include "tracker.h"
#include "util.h"

void CTracker :: serverResponseUsersGET( struct request_t *pRequest, struct response_t *pResponse )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
		if( IsIPBanned( pRequest, pResponse, btv, gmapLANG_CFG["users_page"], string( CSS_USERS ), NOT_INDEX ) )
			return;
		
	string cstrUID( pRequest->mapParams["uid"] );
	
	if( cstrUID.find_first_not_of( "1234567890" ) != string :: npos )
		cstrUID.erase( );
	
	// Check that user is itself
	if( !cstrUID.empty( ) && pRequest->user.strUID == cstrUID )
	{
		const string cstrAction( pRequest->mapParams["action"] );
		const string cstrOK( pRequest->mapParams["ok"] );
		
		string cstrUser = string( );
		
		CMySQLQuery *pQuery = new CMySQLQuery( "SELECT busername FROM users WHERE buid=" + cstrUID );
			
		vector<string> vecQuery;
	
		vecQuery.reserve(1);

		vecQuery = pQuery->nextRow( );
		
		delete pQuery;
		
		if( vecQuery.size( ) == 1 )
			cstrUser = vecQuery[0];
		
		if( !cstrUser.empty( ) )
		{
			if( cstrAction == "edit" )
			{
				// Output common HTML head
				HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["users_page"], string( CSS_USERS ), string( ), NOT_INDEX, CODE_200 );

				// Does the user exist?
				if( vecQuery.size( ) == 1 )
				{
					// Compose an edit users page view

					pResponse->strContent += "<div class=\"edit_users\">\n";
					pResponse->strContent += "<table class=\"edit_users\">\n";
					pResponse->strContent += "<form method=\"post\" name=\"editusers\" action=\"" + RESPONSE_STR_USERS_HTML + "\" enctype=\"multipart/form-data\">\n";
					
					// Edit User
					pResponse->strContent += "<tr class=\"edit_users\">\n";
					pResponse->strContent += "<th class=\"edit_users\" colspan=\"2\">\n";
					pResponse->strContent += UTIL_Xsprintf( gmapLANG_CFG["users_editing_user"].c_str( ), cstrUser.c_str( ) ) + "</th>\n</tr>\n" ;
					
					pResponse->strContent += "<input name=\"uid\" type=hidden value=\"" + cstrUID + "\">\n";
					pResponse->strContent += "<input name=\"action\" type=hidden value=\"edit\">\n";
					pResponse->strContent += "<input name=\"ok\" type=hidden value=1>\n";
					pResponse->strContent += "<tr class=\"edit_users\">\n";
					pResponse->strContent += "<th class=\"edit_users\">\n" + gmapLANG_CFG["users_password_old"] + "</th>";
					pResponse->strContent += "<td class=\"edit_users\">\n";

					// Original code
					pResponse->strContent += "<input id=\"id_old_password\" name=\"us_old_password\" alt=\"[" + gmapLANG_CFG["users_old_password"] + "]\" type=password size=20></td>\n</tr>\n";
					pResponse->strContent += "<tr class=\"edit_users\">\n";
					pResponse->strContent += "<th class=\"edit_users\">\n" + gmapLANG_CFG["users_password"] + "</th>";
					pResponse->strContent += "<td class=\"edit_users\">\n";
					pResponse->strContent += "<div class=\"edit_input_users\"><input id=\"id_password\" name=\"us_password\" alt=\"[" + gmapLANG_CFG["users_password"] + "]\" type=password size=20></td>\n</tr>\n";
					pResponse->strContent += "<tr class=\"edit_users\">\n";
					pResponse->strContent += "<th class=\"edit_users\">\n" + gmapLANG_CFG["users_verify_password"] + "</th>";
					pResponse->strContent += "<td class=\"edit_users\">\n";
					pResponse->strContent += "<div class=\"edit_input_users\"><input id=\"id_verify_password\" name=\"us_password_verify\" alt=\"[" + gmapLANG_CFG["users_verify_password"] + "]\" type=password size=20></td>\n</tr>\n";
// 					pResponse->strContent += "<div class=\"edit_input_users\"><input id=\"id_email\" name=\"us_email\" alt=\"[" + gmapLANG_CFG["email"] + "]\" type=text size=40";                   


// 					CAtom *pMailToEdit = ( (CAtomDicti *)pUserToEdit )->getItem( "email" );


					// Original code
// 						if( pMailToEdit )
// 							pResponse->strContent += " value=\"" + pMailToEdit->toString( ) + "\"";
// 
// 						pResponse->strContent += "> <label for=\"id_email\">" + gmapLANG_CFG["email"] + "</label><br><br></div>\n";


					// The Trinity Edition - Addition Begins
					// The following creates a CANCEL EDIT button
					// when EDITING A USER

					pResponse->strContent += "<tr class=\"edit_users\">\n";
					pResponse->strContent += "<td class=\"edit_users\" colspan=\"2\">\n";
					pResponse->strContent += "<div class=\"edit_users_button\">\n";
					pResponse->strContent += Button_Submit( "submit_edit", string( gmapLANG_CFG["edit_user"] ) );
					pResponse->strContent += Button_Back( "cancel_edit", string( gmapLANG_CFG["cancel"] ) );
					pResponse->strContent += "\n</div>\n</td>\n</tr>\n";

					// finish
					pResponse->strContent += "</form>\n</table>\n</div>\n";

					// Output common HTML tail
					HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_USERS ) );

					return;
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
			else if( cstrAction == "reset" )
			{
				// Output common HTML head
				HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["users_page"], string( CSS_USERS ), string( ), NOT_INDEX, CODE_200 );
				// Are you sure it is ok to reset the passkey?
				if( cstrOK == "1" )
				{
					// reset the passkey
					InitPasskey( cstrUser );

					// Inform the operator that the passkey was reset
					pResponse->strContent += "<p class=\"deleted_users\">" + UTIL_Xsprintf( gmapLANG_CFG["users_reset_passkey"].c_str( ), cstrUser.c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_users"] + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "?uid=" + cstrUID + "\">" ).c_str( ), "</a>" ) + "</p>\n\n" ;

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

					pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["users_reset_passkey_q"].c_str( ), cstrUser.c_str( ) ) + "</p>\n\n" ;
					pResponse->strContent += "<p class=\"delete\"><a title=\"" + gmapLANG_CFG["yes"] + "\" href=\"" + RESPONSE_STR_USERS_HTML + "?uid=" + cstrUID + "&amp;action=reset&amp;ok=1\">" + gmapLANG_CFG["yes"] + "</a> | <a title=\"" + gmapLANG_CFG["no"] + "\" href=\"" + string( JS_BACK ) + "\">" + gmapLANG_CFG["no"] + "</a></p>\n";

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
	else if( !pRequest->user.strUID.empty( ) && ( pRequest->user.ucAccess & m_ucAccessViewUsers ) )
	{
		if( cstrUID.empty( ) )
		{
			// Was a search submited?
			if( pRequest->mapParams["top_submit_search_button"] == gmapLANG_CFG["search"] || pRequest->mapParams["bottom_submit_search_button"] == gmapLANG_CFG["search"] )
			{
				const string cstrSearch( pRequest->mapParams["search"] );
				const string cstrSort( pRequest->mapParams["sort"] );
				const string cstrPerPage( pRequest->mapParams["per_page"] );
				const string cstrSearchMode( pRequest->mapParams["smode"] );
				const string cstrShow( pRequest->mapParams["show"] );

				string strPageParameters = USERS_HTML;
				
				vector< pair< string, string > > vecParams;
				vecParams.reserve(64);
		
				vecParams.push_back( pair<string, string>( string( "per_page" ), cstrPerPage ) );
				vecParams.push_back( pair<string, string>( string( "search" ), cstrSearch ) );
				vecParams.push_back( pair<string, string>( string( "sort" ), cstrSort ) );
				vecParams.push_back( pair<string, string>( string( "smode" ), cstrSearchMode ) );
				vecParams.push_back( pair<string, string>( string( "show" ), cstrShow ) );
				
				strPageParameters += UTIL_HTMLJoin( vecParams, string( "?" ), string( "&" ), string( "=" ) );

				JS_ReturnToPage( pRequest, pResponse, strPageParameters );
				return;
			}

			if( pRequest->mapParams["top_clear_filter_and_search_button"] == gmapLANG_CFG["clear_filter_search"] || pRequest->mapParams["bottom_clear_filter_and_search_button"] == gmapLANG_CFG["clear_filter_search"] )
			{
				const string cstrSearch( pRequest->mapParams["search"] );
				const string cstrSearchResp( UTIL_StringToEscaped( cstrSearch ) );

				string strPageParameters = USERS_HTML;
		
				if( !cstrSearch.empty( ) )
					strPageParameters += "?search=" + cstrSearchResp;

				JS_ReturnToPage( pRequest, pResponse, strPageParameters );
				return;
			}

			// Output common HTML head
			HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["users_page"], string( CSS_USERS ), string( ), NOT_INDEX, CODE_200 );

			// javascript
			pResponse->strContent += "<script type=\"text/javascript\">\n";
			pResponse->strContent += "<!--\n";
			
			pResponse->strContent += "function validate( theform ) {\n";
			pResponse->strContent += "if( theform.us_warned.checked == true && theform.us_warned_time.value != \"0\" && theform.us_warned_reason.value == \"\" ) {\n";
			pResponse->strContent += "  alert( \"" + gmapLANG_CFG["js_fill_warned_reason"] + "\" );\n";
			pResponse->strContent += "  return false; }\n";
			pResponse->strContent += "else { return true; }\n";
			pResponse->strContent += "}\n\n";

			pResponse->strContent += "function delete_user_confirm( USER )\n";
			pResponse->strContent += "{\n";
			pResponse->strContent += "var name=confirm(\"" + gmapLANG_CFG["delete"] + " xbnbt_announce_list\" + USER )\n";
			pResponse->strContent += "if (name==true)\n";
			pResponse->strContent += "{\n";
			pResponse->strContent += "window.location=\"" + RESPONSE_STR_USERS_HTML + "?up_deluser=\" + USER\n";
			pResponse->strContent += "}\n";
			pResponse->strContent += "}\n\n";

//			pResponse->strContent += "function edit_user( USER ) {\n";
//			pResponse->strContent += "window.location=\"" + RESPONSE_STR_USERS_HTML + "?up_edituser\" + USER\n";
//			pResponse->strContent += "}\n\n";

//			pResponse->strContent += "function clear_search_and_filters( ) {\n";
//			pResponse->strContent += "window.location=\"" + RESPONSE_STR_USERS_HTML + "\"\n";
//			pResponse->strContent += "}\n\n";

			pResponse->strContent += "//-->\n";
			pResponse->strContent += "</script>\n\n";

			if( pRequest->user.ucAccess & m_ucAccessCreateUsers )
			{
				//
				// create user
				//

				// Compose a new users page view
				pResponse->strContent += "<div class=\"create_users\">\n";
				pResponse->strContent += "<table class=\"create_users\">\n";
				pResponse->strContent += "<form method=\"post\" name=\"createusers\" action=\"" + RESPONSE_STR_USERS_HTML + "\" enctype=\"multipart/form-data\">\n";
				pResponse->strContent += "<tr class=\"create_users\">\n";
				pResponse->strContent += "<th class=\"create_users\" colspan=\"2\">\n" + gmapLANG_CFG["users_create_user_title"] + "</th>\n</tr>\n";
				pResponse->strContent += "<tr class=\"create_users\">\n";
				pResponse->strContent += "<th class=\"create_users\">\n" + gmapLANG_CFG["user_name"] + "</th>\n";
				pResponse->strContent += "<td class=\"create_users\">\n";
				pResponse->strContent += "<input id=\"id_login\" name=\"us_login\" alt=\"[" + gmapLANG_CFG["user_name"] + "]\" type=text size=24></td>\n</tr>\n";
				pResponse->strContent += "<tr class=\"create_users\">\n";
				pResponse->strContent += "<th class=\"create_users\">\n" + gmapLANG_CFG["password"] + "</th>\n";
				pResponse->strContent += "<td class=\"create_users\">\n";
				pResponse->strContent += "<input id=\"id_password\" name=\"us_password\" alt=\"[" + gmapLANG_CFG["password"] + "]\" type=password size=20></td>\n</tr>\n";
				pResponse->strContent += "<tr class=\"create_users\">\n";
				pResponse->strContent += "<th class=\"create_users\">\n" + gmapLANG_CFG["verify_password"] + "</th>\n";
				pResponse->strContent += "<td class=\"create_users\">\n";
				pResponse->strContent += "<input id=\"id_verify\" name=\"us_password_verify\" alt=\"[" + gmapLANG_CFG["verify_password"] + "]\" type=password size=20></td>\n</tr>\n";
				pResponse->strContent += "<tr class=\"create_users\">\n";
				pResponse->strContent += "<th class=\"create_users\">\n" + gmapLANG_CFG["email"] + "</th>\n";
				pResponse->strContent += "<td class=\"create_users\">\n";
				pResponse->strContent += "<input id=\"id_email\" name=\"us_email\" alt=\"[" + gmapLANG_CFG["email"] + "]\" type=text size=40></td>\n</tr>\n";
				pResponse->strContent += "<tr class=\"create_users\">\n";
				pResponse->strContent += "<th class=\"create_users\">\n" + gmapLANG_CFG["access"] + "</th>\n";
				pResponse->strContent += "<td class=\"create_users\">\n";
				if( m_ucAccessCreateUsers > ACCESS_VIEW )
					pResponse->strContent += "<div class=\"input_create_users\"><input id=\"id_view\" name=\"us_access_view\" alt=\"[" + gmapLANG_CFG["users_view_access"] + "]\" type=checkbox> <label for=\"id_view\">" + gmapLANG_CFG["users_view_access"] + "</label><br></div>\n";
				if( m_ucAccessCreateUsers > ACCESS_DL )
					pResponse->strContent += "<div class=\"input_create_users\"><input id=\"id_download\" name=\"us_access_dl\" alt=\"[" + gmapLANG_CFG["users_dl_access"] + "]\" type=checkbox> <label for=\"id_download\">" + gmapLANG_CFG["users_dl_access"] + "</label><br></div>\n";
				if( m_ucAccessCreateUsers > ACCESS_COMMENTS )
					pResponse->strContent += "<div class=\"input_create_users\"><input id=\"id_comments\" name=\"us_access_comments\" alt=\"[" + gmapLANG_CFG["users_comments_access"] + "]\" type=checkbox> <label for=\"id_comments\">" + gmapLANG_CFG["users_comments_access"] + "</label><br></div>\n";
				if( m_ucAccessCreateUsers > ACCESS_UPLOAD )
					pResponse->strContent += "<div class=\"input_create_users\"><input id=\"id_upload\" name=\"us_access_upload\" alt=\"[" + gmapLANG_CFG["users_upload_access"] + "]\" type=checkbox> <label for=\"id_upload\">" + gmapLANG_CFG["users_upload_access"] + "</label><br></div>\n";
				if( m_ucAccessCreateUsers > ACCESS_EDIT )
					pResponse->strContent += "<div class=\"input_create_users\"><input id=\"id_edit\" name=\"us_access_edit\" alt=\"[" + gmapLANG_CFG["users_edit_access"] + "]\" type=checkbox> <label for=\"id_edit\">" + gmapLANG_CFG["users_edit_access"] + "</label><br></div>\n";
				if( m_ucAccessCreateUsers > ACCESS_ADMIN )
					pResponse->strContent += "<div class=\"input_create_users\"><input id=\"id_admin\" name=\"us_access_admin\" alt=\"[" + gmapLANG_CFG["users_admin_access"] + "]\" type=checkbox> <label for=\"id_admin\">" + gmapLANG_CFG["users_admin_access"] + "</label><br><br></div>\n";
	// 				pResponse->strContent += "<div class=\"input_create_users\"><input id=\"id_signup\" name=\"us_access_signup\" alt=\"[" + gmapLANG_CFG["users_signup_access"] + "]\" type=checkbox> <label for=\"id_signup\">" + gmapLANG_CFG["users_signup_access"] + "</label><br><br></div>\n";
				pResponse->strContent += "</td>\n</tr>\n";

				// Adds Cancel button beside Create User
				pResponse->strContent += "<tr class=\"create_users\">\n";
				pResponse->strContent += "<td class=\"create_users\" colspan=\"2\">\n";
				pResponse->strContent += "<div class=\"create_users_buttons\">\n";

				pResponse->strContent += Button_Submit( "submit_create", string( gmapLANG_CFG["create_user"] ) );
				pResponse->strContent += Button_Back( "cancel_create", string( gmapLANG_CFG["cancel"] ) );

				pResponse->strContent += "\n</div>\n</td>\n</tr>\n";

				// finish
				pResponse->strContent += "</form>\n</table>\n</div>\n";
			}

			//
			// user table
			//

			// Does the users database exist?

			unsigned long culKeySize = 0;
			
			struct user_t *pUsersT = 0;
			
			if( m_pCache )
			{
//				m_pCache->ResetUsers( );
				pUsersT = m_pCache->getCacheUsers( );
				culKeySize = m_pCache->getSizeUsers( );
			}
			
			// Are there any registered users?
			if( culKeySize == 0 )
			{
				// There are no registered users!
				pResponse->strContent += "<p class=\"no_users\">" + gmapLANG_CFG["users_nousers_warning"] + "</p>\n";

				// Output common HTML tail
				HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_USERS ) );

				return;
			}

			// Populate the users structure for display

			// Sort the users
			const string cstrSort( pRequest->mapParams["sort"] );
			
			if( !cstrSort.empty( ) )
			{
				const unsigned char cucSort( (unsigned char)atoi( cstrSort.c_str( ) ) );
				m_pCache->sortUsers( cucSort );
			}
			else
				m_pCache->sortUsers( SORTU_DCREATED );

//			if( !cstrSort.empty( ) )
//			{
//				const unsigned char cucSort( (unsigned char)atoi( cstrSort.c_str( ) ) );

//				switch ( cucSort )
//				{
//				case SORTU_ALOGIN:
//					qsort( pUsersT, culKeySize, sizeof( struct user_t ), asortuByLogin );
//					break;
//				case SORTU_AACCESS:
//					qsort( pUsersT, culKeySize, sizeof( struct user_t ), asortuByAccess );
//					break;
//				case SORTU_AGROUP:
//					qsort( pUsersT, culKeySize, sizeof( struct user_t ), asortuByGroup );
//					break;
//				case SORTU_AINVITER:
//					qsort( pUsersT, culKeySize, sizeof( struct user_t ), asortuByInviter );
//					break;
//				case SORTU_AEMAIL:
//					qsort( pUsersT, culKeySize, sizeof( struct user_t ), asortuByMail );
//					break;
//				case SORTU_ACREATED:
//					qsort( pUsersT, culKeySize, sizeof( struct user_t ), asortuByCreated );
//					break;
//				case SORTU_ALAST:
//					qsort( pUsersT, culKeySize, sizeof( struct user_t ), asortuByLast );
//					break;
//				case SORTU_AWARNED:
//					qsort( pUsersT, culKeySize, sizeof( struct user_t ), asortuByWarned );
//					break;
//				case SORTU_ASHARERATIO:
//					qsort( pUsersT, culKeySize, sizeof( struct user_t ), asortuByShareRatio );
//					break;
//				case SORTU_AUPPED:
//					qsort( pUsersT, culKeySize, sizeof( struct user_t ), asortuByUpped );
//					break;
//				case SORTU_ADOWNED:
//					qsort( pUsersT, culKeySize, sizeof( struct user_t ), asortuByDowned );
//					break;
//				case SORTU_ABONUS:
//					qsort( pUsersT, culKeySize, sizeof( struct user_t ), asortuByBonus );
//					break;
//				case SORTU_ASEEDBONUS:
//					qsort( pUsersT, culKeySize, sizeof( struct user_t ), asortuBySeedBonus );
//					break;
//				case SORTU_DLOGIN:
//					qsort( pUsersT, culKeySize, sizeof( struct user_t ), dsortuByLogin );
//					break;
//				case SORTU_DACCESS:
//					qsort( pUsersT, culKeySize, sizeof( struct user_t ), dsortuByAccess );
//					break;
//				case SORTU_DGROUP:
//					qsort( pUsersT, culKeySize, sizeof( struct user_t ), dsortuByGroup );
//					break;
//				case SORTU_DINVITER:
//					qsort( pUsersT, culKeySize, sizeof( struct user_t ), dsortuByInviter );
//					break;
//				case SORTU_DEMAIL:
//					qsort( pUsersT, culKeySize, sizeof( struct user_t ), dsortuByMail );
//					break;
//				case SORTU_DLAST:
//					qsort( pUsersT, culKeySize, sizeof( struct user_t ), dsortuByLast );
//					break;
//				case SORTU_DWARNED:
//					qsort( pUsersT, culKeySize, sizeof( struct user_t ), dsortuByWarned );
//					break;
//				case SORTU_DSHARERATIO:
//					qsort( pUsersT, culKeySize, sizeof( struct user_t ), dsortuByShareRatio );
//					break;
//				case SORTU_DUPPED:
//					qsort( pUsersT, culKeySize, sizeof( struct user_t ), dsortuByUpped );
//					break;
//				case SORTU_DDOWNED:
//					qsort( pUsersT, culKeySize, sizeof( struct user_t ), dsortuByDowned );
//					break;
//				case SORTU_DBONUS:
//					qsort( pUsersT, culKeySize, sizeof( struct user_t ), dsortuByBonus );
//					break;
//				case SORTU_DSEEDBONUS:
//					qsort( pUsersT, culKeySize, sizeof( struct user_t ), dsortuBySeedBonus );
//					break;
//				case SORTU_DCREATED:
//				default:
//					qsort( pUsersT, culKeySize, sizeof( struct user_t ), dsortuByCreated );
//				}
//			}
//			else
//			{
//				// default action is to sort by created

//				qsort( pUsersT, culKeySize, sizeof( struct user_t ), dsortuByCreated );
//			}

			// some preliminary search crap

			const string cstrSearch( pRequest->mapParams["search"] );
//			const string cstrLowerSearch( UTIL_ToLower( cstrSearch ) );
//			const string cstrSearchResp( UTIL_StringToEscaped( cstrSearch ) );
			const string cstrSearchMode( pRequest->mapParams["smode"] );
			const string cstrShow( pRequest->mapParams["show"] );

			vector<string> vecSearch;
			vecSearch.reserve(64);
			
			vecSearch = UTIL_SplitToVector( cstrSearch, " " );

			pResponse->strContent += "\n<hr class=\"users_hr\">\n\n";

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

			// Count matching users for top of page
			unsigned long ulFound = 0;

			// Loop through the users
//			for( unsigned long ulKey = 0; ulKey < culKeySize; ulKey++ )
//			{
//				bool bFoundKey = true;
//				string :: size_type iStart = 0;
//				string :: size_type iEnd = 0;
//				string strKeyword = string( );
//				iStart = cstrLowerSearch.find_first_not_of( " " );
//		
//				while( iStart != string :: npos && iEnd != string :: npos )
//				{
//					iEnd = cstrLowerSearch.find_first_of( " ", iStart );
//					strKeyword = cstrLowerSearch.substr( iStart, iEnd - iStart );
//					if( pUsersT[ulKey].strLowerLogin.find( strKeyword ) == string :: npos && pUsersT[ulKey].strLowerMail.find( strKeyword ) == string :: npos )
//						bFoundKey = false;
//					if( iEnd != string :: npos )
//						iStart = cstrLowerSearch.find_first_not_of( " ", iEnd );
//			
//				}
//				if( bFoundKey == false )
//					continue;

//				// Found a user! Increment the count.
//				ulFound++;
//			}
//
			pResponse->strContent += "<p class=\"subfilter\">";
			pResponse->strContent += "<a";
			if( cstrShow.empty( ) )
				pResponse->strContent += " class=\"blue\"";
			pResponse->strContent += " href=\"" + RESPONSE_STR_USERS_HTML + "\">" + gmapLANG_CFG["users_show_all"] + "</a>";
			pResponse->strContent += "<span class=\"pipe\"> | </span>";
			pResponse->strContent += "<a";
			if( cstrShow == "invited" )
				pResponse->strContent += " class=\"blue\"";
			pResponse->strContent += " href=\"" + RESPONSE_STR_USERS_HTML + "?show=invited\">" + gmapLANG_CFG["users_show_invited"] + "</a>";
			pResponse->strContent += "<span class=\"pipe\"> | </span>";
			pResponse->strContent += "<a";
			if( cstrShow == "sharewarned" )
				pResponse->strContent += " class=\"blue\"";
			pResponse->strContent += " href=\"" + RESPONSE_STR_USERS_HTML + "?show=sharewarned\">" + gmapLANG_CFG["users_show_sharewarned"] + "</a>";
			pResponse->strContent += "<span class=\"pipe\"> | </span>";
			pResponse->strContent += "<a";
			if( cstrShow == "warned" )
				pResponse->strContent += " class=\"blue\"";
			pResponse->strContent += " href=\"" + RESPONSE_STR_USERS_HTML + "?show=warned\">" + gmapLANG_CFG["users_show_warned"] + "</a>";
			pResponse->strContent += "<span class=\"pipe\"> | </span>";
			pResponse->strContent += "<a";
			if( cstrShow == "banned" )
				pResponse->strContent += " class=\"blue\"";
			pResponse->strContent += " href=\"" + RESPONSE_STR_USERS_HTML + "?show=banned\">" + gmapLANG_CFG["users_show_banned"] + "</a>";
			pResponse->strContent += "</p>";

			// Top search
			if( m_bSearch )
			{
				pResponse->strContent += "<form class=\"search_users_top\" name=\"topsearch\" method=\"get\" action=\"" + RESPONSE_STR_USERS_HTML + "\">\n";

				if( !cstrPerPage.empty( ) )
					pResponse->strContent += "<p><input name=\"per_page\" type=hidden value=\"" + cstrPerPage + "\"></p>\n";

				if( !cstrSort.empty( ) )
					pResponse->strContent += "<p><input name=\"sort\" type=hidden value=\"" + cstrSort + "\"></p>\n";

				if( !cstrShow.empty( ) )
					pResponse->strContent += "<p><input name=\"show\" type=hidden value=\"" + cstrShow + "\"></p>\n";

//				if( m_bUseButtons )
//				{
					pResponse->strContent += "<p><label for=\"topusersearch\">" + gmapLANG_CFG["user_search"] + "</label> <input name=\"search\" id=\"topusersearch\" alt=\"[" + gmapLANG_CFG["user_search"] + "]\" type=text size=40";
					
					pResponse->strContent += " value=\"" + UTIL_RemoveHTML( cstrSearch ) + "\">\n";

					pResponse->strContent += "<select id=\"smode\" name=\"smode\">";
					pResponse->strContent += "\n<option value=\"name\">" + gmapLANG_CFG["user_name"];
					pResponse->strContent += "\n<option value=\"mail\"";
					if( cstrSearchMode == "mail" )
						pResponse->strContent += " selected";
					pResponse->strContent += ">" + gmapLANG_CFG["email"];
					pResponse->strContent += "\n</select>\n";
					pResponse->strContent += Button_Submit( "top_submit_search", gmapLANG_CFG["search"] );
					pResponse->strContent += Button_Submit( "top_clear_filter_and_search", gmapLANG_CFG["clear_filter_search"] );

					pResponse->strContent += "</p>\n";
//				}
//				else
//					pResponse->strContent += "<p><label for=\"topusersearch\">" + gmapLANG_CFG["user_search"] + "</label> <input name=\"search\" id=\"topusersearch\" alt=\"[" + gmapLANG_CFG["user_search"] + "]\" type=text size=40> <a title=\"" + gmapLANG_CFG["clear_filter_search"] + "\" href=\"" + RESPONSE_STR_USERS_HTML + "\">" + gmapLANG_CFG["clear_filter_search"] + "</a></p>\n";

				pResponse->strContent += "</form>\n\n";
			}
			
			// What was the search criteria used?
			if( !vecSearch.empty() )
			{
				pResponse->strContent += "<p class=\"search_filter\">\n";
				pResponse->strContent += "<span class=\"search_results_alt\">" + gmapLANG_CFG["result_search"] + ": \"</span><span class=\"filtered_by\">" + UTIL_RemoveHTML( cstrSearch ) + "</span>\"\n";
				pResponse->strContent += "</p>\n\n";
			}


			string :: size_type iCountGoesHere = string :: npos;
		
			iCountGoesHere = pResponse->strContent.size( );
			
			const string cstrPage( pRequest->mapParams["page"] );

			if( !cstrPage.empty( ) )
				ulStart = (unsigned long)atoi( cstrPage.c_str( ) ) * uiOverridePerPage;

			bool bFound = false;

			unsigned long ulAdded = 0;
			unsigned long ulSkipped = 0;
			
			char szFloat[16];
			memset( szFloat, 0, sizeof( szFloat ) / sizeof( char ) );

			// for correct page numbers after searching

			for( unsigned long ulKey = 0; ulKey < culKeySize; ulKey++ )
			{
				if( cstrSearchMode == "name" && !UTIL_MatchVector( pUsersT[ulKey].strLogin, vecSearch, MATCH_METHOD_NONCASE_AND ) )
					continue;
				if( cstrSearchMode == "mail" && !UTIL_MatchVector( pUsersT[ulKey].strMail, vecSearch, MATCH_METHOD_NONCASE_AND ) )
					continue;
				if( cstrShow == "invited" && pUsersT[ulKey].strInviter.empty( ) )
					continue;
				bool bShareRatioWarned = checkShareRatio( pUsersT[ulKey].ulDownloaded, pUsersT[ulKey].flShareRatio );
				if( cstrShow == "sharewarned" && !( m_bRatioRestrict && bShareRatioWarned ) )
					continue;
				if( cstrShow == "warned" && !pUsersT[ulKey].tWarned )
					continue;
				if( cstrShow == "banned" && pUsersT[ulKey].ucAccess & ACCESS_VIEW )
					continue;
			
//				if( !vecSearch.empty( ) )
//				{
//					// only display entries that match the search   
//					bool bFoundKey = true;
//				
//					for( vector<string> :: iterator ulVecKey = vecSearch.begin( ); ulVecKey != vecSearch.end( ); ulVecKey++ )
//					{
//						if( pUsersT[ulKey].strLowerLogin.find( UTIL_ToLower( *ulVecKey ) ) == string :: npos )
//						{
//							bFoundKey = false;
//							break;
//						}
//					}
//					if( bFoundKey == false )
//						continue;
//				}

				ulFound++;

				// Have we choosen to display all the results or are there fewer results than a page?
				if( uiOverridePerPage == DISPLAY_ALL || (unsigned int)ulAdded < uiOverridePerPage )
				{											   
					// create the table and display the headers first and once
					if( !bFound )
					{
						vector< pair< string, string > > vecParams;
						vecParams.reserve(64);
						string strJoined = string( );
			
						vecParams.push_back( pair<string, string>( string( "per_page" ), cstrPerPage ) );
						vecParams.push_back( pair<string, string>( string( "search" ), cstrSearch ) );
						vecParams.push_back( pair<string, string>( string( "smode" ), cstrSearchMode ) );
						vecParams.push_back( pair<string, string>( string( "show" ), cstrShow ) );
						
						strJoined = UTIL_RemoveHTML( UTIL_HTMLJoin( vecParams, string( "&" ), string( "&" ), string( "=" ) ) );
						
						pResponse->strContent += "<div class=\"users_table\">\n";
						pResponse->strContent += "<table summary=\"users\">\n";
						pResponse->strContent += "<tr><th class=\"uploader\">" + gmapLANG_CFG["user_name"];
						pResponse->strContent += "<br><a title=\"" + gmapLANG_CFG["sort_login_ascending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_USERS_HTML + "?sort=" + SORTUSTR_ALOGIN;
						
						pResponse->strContent += strJoined;

						pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a title=\"" + gmapLANG_CFG["sort_login_descending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_USERS_HTML + "?sort=" + SORTUSTR_DLOGIN;

						pResponse->strContent += strJoined;

						pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a>";
						pResponse->strContent += "</th>\n<th class=\"access\">" + gmapLANG_CFG["access"];
						pResponse->strContent += "<br><a title=\"" + gmapLANG_CFG["sort_access_ascending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_USERS_HTML + "?sort=" + SORTUSTR_AACCESS;
						
						pResponse->strContent += strJoined;

						pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a title=\"" + gmapLANG_CFG["sort_access_descending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_USERS_HTML + "?sort=" + SORTUSTR_DACCESS;

						pResponse->strContent += strJoined;
						
						pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a>";
						pResponse->strContent += "</th>\n<th class=\"group\">" + gmapLANG_CFG["group"];
						pResponse->strContent += "<br><a title=\"" + gmapLANG_CFG["sort_group_ascending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_USERS_HTML + "?sort=" + SORTUSTR_AGROUP;
						
						pResponse->strContent += strJoined;

						pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a title=\"" + gmapLANG_CFG["sort_group_descending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_USERS_HTML + "?sort=" + SORTUSTR_DGROUP;

						pResponse->strContent += strJoined;

						pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a>";
						
						if( pRequest->user.ucAccess & m_ucAccessEditUsers )
						{
							pResponse->strContent += "</th>\n<th class=\"inviter\">" + gmapLANG_CFG["inviter"];
							pResponse->strContent += "<br><a title=\"" + gmapLANG_CFG["sort_inviter_ascending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_USERS_HTML + "?sort=" + SORTUSTR_AINVITER;
						
							pResponse->strContent += strJoined;

							pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a title=\"" + gmapLANG_CFG["sort_inviter_descending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_USERS_HTML + "?sort=" + SORTUSTR_DINVITER;

							pResponse->strContent += strJoined;
						
							pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a>";
						
							pResponse->strContent += "</th>\n<th class=\"mail\">" + gmapLANG_CFG["email"];
							pResponse->strContent += "<br><a title=\"" + gmapLANG_CFG["sort_email_ascending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_USERS_HTML + "?sort=" + SORTUSTR_AEMAIL;
						
							pResponse->strContent += strJoined;

							pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a title=\"" + gmapLANG_CFG["sort_email_descending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_USERS_HTML + "?sort=" + SORTUSTR_DEMAIL;

							pResponse->strContent += strJoined;

							pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a>";
						}
						
						pResponse->strContent += "</th>\n<th class=\"number\">" + gmapLANG_CFG["share_ratio"];
						pResponse->strContent += "<br><a title=\"" + gmapLANG_CFG["sort_ratio_ascending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_USERS_HTML + "?sort=" + SORTUSTR_ASHARERATIO;
						
						pResponse->strContent += strJoined;

						pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a title=\"" + gmapLANG_CFG["sort_ratio_descending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_USERS_HTML + "?sort=" + SORTUSTR_DSHARERATIO;

						pResponse->strContent += strJoined;

						pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a>";
						pResponse->strContent += "</th>\n<th class=\"bytes\">" + gmapLANG_CFG["user_uploaded"];
						pResponse->strContent += "<br><a title=\"" + gmapLANG_CFG["sort_uploaded_ascending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_USERS_HTML + "?sort=" + SORTUSTR_AUPPED;
						
						pResponse->strContent += strJoined;

						pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a title=\"" + gmapLANG_CFG["sort_uploaded_descending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_USERS_HTML + "?sort=" + SORTUSTR_DUPPED;

						pResponse->strContent += strJoined;

						pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a>";
						pResponse->strContent += "</th>\n<th class=\"bytes\">" + gmapLANG_CFG["user_downloaded"];
						pResponse->strContent += "<br><a title=\"" + gmapLANG_CFG["sort_downloaded_ascending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_USERS_HTML + "?sort=" + SORTUSTR_ADOWNED;
						
						pResponse->strContent += strJoined;

						pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a title=\"" + gmapLANG_CFG["sort_downloaded_descending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_USERS_HTML + "?sort=" + SORTUSTR_DDOWNED;

						pResponse->strContent += strJoined;

						pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a>";
						pResponse->strContent += "</th>\n<th class=\"number\">" + gmapLANG_CFG["user_bonus"];
						pResponse->strContent += "<br><a title=\"" + gmapLANG_CFG["sort_bonus_ascending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_USERS_HTML + "?sort=" + SORTUSTR_ABONUS;
						
						pResponse->strContent += strJoined;

						pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a title=\"" + gmapLANG_CFG["sort_bonus_descending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_USERS_HTML + "?sort=" + SORTUSTR_DBONUS;

						pResponse->strContent += strJoined;

						pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a>";
						pResponse->strContent += "</th>\n<th class=\"number\">" + gmapLANG_CFG["user_seed_bonus"];
						pResponse->strContent += "<br><a title=\"" + gmapLANG_CFG["sort_seed_bonus_ascending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_USERS_HTML + "?sort=" + SORTUSTR_ASEEDBONUS;
						
						pResponse->strContent += strJoined;

						pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a title=\"" + gmapLANG_CFG["sort_seed_bonus_descending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_USERS_HTML + "?sort=" + SORTUSTR_DSEEDBONUS;

						pResponse->strContent += strJoined;

						pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a>";
						pResponse->strContent += "</th>\n<th class=\"date\">" + gmapLANG_CFG["created"];
						pResponse->strContent += "<br><a title=\"" + gmapLANG_CFG["sort_created_ascending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_USERS_HTML + "?sort=" + SORTUSTR_ACREATED;

						pResponse->strContent += strJoined;

						pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a title=\"" + gmapLANG_CFG["sort_created_descending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_USERS_HTML + "?sort=" + SORTUSTR_DCREATED;

						pResponse->strContent += strJoined;

						pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a>";
						pResponse->strContent += "</th>\n<th class=\"date\">" + gmapLANG_CFG["user_last"];
						pResponse->strContent += "<br><a title=\"" + gmapLANG_CFG["sort_last_ascending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_USERS_HTML + "?sort=" + SORTUSTR_ALAST;

						pResponse->strContent += strJoined;

						pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a title=\"" + gmapLANG_CFG["sort_last_descending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_USERS_HTML + "?sort=" + SORTUSTR_DLAST;

						pResponse->strContent += strJoined;

						pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a>";
						pResponse->strContent += "</th>\n<th class=\"date\">" + gmapLANG_CFG["warned"];
						pResponse->strContent += "<br><a title=\"" + gmapLANG_CFG["sort_warned_ascending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_USERS_HTML + "?sort=" + SORTUSTR_AWARNED;

						pResponse->strContent += strJoined;

						pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a title=\"" + gmapLANG_CFG["sort_warned_descending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_USERS_HTML + "?sort=" + SORTUSTR_DWARNED;

						pResponse->strContent += strJoined;

						pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a></th>\n";
						if( ( pRequest->user.ucAccess & m_ucAccessEditUsers ) || ( pRequest->user.ucAccess & m_ucAccessDelUsers ) )
							pResponse->strContent += "<th class=\"admin\">" + gmapLANG_CFG["admin"] + "</th>";
						
						pResponse->strContent += "</tr>\n";

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

						pResponse->strContent += "<td class=\"uploader\">";
						pResponse->strContent += getUserLink( pUsersT[ulKey].strUID, pUsersT[ulKey].strLogin );
						pResponse->strContent += "</td>";
						pResponse->strContent += "<td class=\"access\">" + UTIL_AccessToString( pUsersT[ulKey].ucAccess );
						pResponse->strContent += "</td>\n";
						pResponse->strContent += "<td class=\"group\">";
//						if( pUsersT[ulKey].ucGroup )
							pResponse->strContent += UTIL_GroupToString( pUsersT[ulKey].ucGroup );
//						else
//							pResponse->strContent += gmapLANG_CFG["group_none"];
						if( pRequest->user.ucAccess & m_ucAccessEditUsers )
						{
							
							pResponse->strContent += "</td><td class=\"uploader\">";
							if( !pUsersT[ulKey].strInviter.empty( ) )
							{
								pResponse->strContent += getUserLink( pUsersT[ulKey].strInviterID, pUsersT[ulKey].strInviter );
							}
							else
								pResponse->strContent += gmapLANG_CFG["na"];
						
							pResponse->strContent += "</td>\n<td class=\"mail\">";

							// The Trinity Edition - Modification Begins
							// The following makes user email addresses HYPERLINKED

							pResponse->strContent += "<a title=\"" + UTIL_RemoveHTML( pUsersT[ulKey].strMail ) + "\" href=\"mailto:"; 
							pResponse->strContent += UTIL_RemoveHTML( pUsersT[ulKey].strMail ); 
							pResponse->strContent += "\">"; 
							pResponse->strContent += UTIL_RemoveHTML( pUsersT[ulKey].strMail );
							pResponse->strContent += "</a>";
						}
						
						pResponse->strContent += "</td>\n<td class=\"number\">";
						if( ( -1.001 < pUsersT[ulKey].flShareRatio ) && ( pUsersT[ulKey].flShareRatio < -0.999 ) )
							pResponse->strContent += gmapLANG_CFG["perfect"];
						else
						{
							snprintf( szFloat, sizeof( szFloat ) / sizeof( char ), "%0.3f", pUsersT[ulKey].flShareRatio );
							pResponse->strContent += szFloat;
						}
						pResponse->strContent += "</td>\n<td class=\"bytes\">";
						pResponse->strContent += UTIL_BytesToString( pUsersT[ulKey].ulUploaded );
						pResponse->strContent += "</td>\n<td class=\"bytes\">";
						pResponse->strContent += UTIL_BytesToString( pUsersT[ulKey].ulDownloaded );
						pResponse->strContent += "</td>\n<td class=\"number\">";
						pResponse->strContent += CAtomLong( pUsersT[ulKey].ulBonus / 100 ).toString( ) + "." + CAtomInt( ( pUsersT[ulKey].ulBonus % 100 ) / 10 ).toString( ) + CAtomInt( pUsersT[ulKey].ulBonus % 10 ).toString( );
						pResponse->strContent += "</td>\n<td class=\"number\">";
						snprintf( szFloat, sizeof( szFloat ) / sizeof( char ), "%0.2f", pUsersT[ulKey].flSeedBonus );
						pResponse->strContent += szFloat;
						pResponse->strContent += "</td>\n<td class=\"date\">";

						// strip year and seconds from time
						if( !pUsersT[ulKey].strCreated.empty( ) )
							pResponse->strContent += pUsersT[ulKey].strCreated.substr( 2, pUsersT[ulKey].strCreated.size( ) - 5 );
						
						pResponse->strContent += "</td>\n";
						
						if( pUsersT[ulKey].tLast )
						{
							char pTime[256];
							memset( pTime, 0, sizeof( pTime ) / sizeof( char ) );
							strftime( pTime, sizeof( pTime ) / sizeof( char ), "%Y-%m-%d %H:%M:%S", localtime( &pUsersT[ulKey].tLast ) );
							pResponse->strContent += "<td class=\"date\">" + string( pTime ).substr( 2, string( pTime ).size( ) - 5 ) + "</td>\n";
						}
						else
							pResponse->strContent += "<td class=\"date\">" + gmapLANG_CFG["na"] + "</td>\n";
						
						if( pUsersT[ulKey].tWarned )
						{
							char pTime[256];
							memset( pTime, 0, sizeof( pTime ) / sizeof( char ) );
							strftime( pTime, sizeof( pTime ) / sizeof( char ), "%Y-%m-%d %H:%M:%S", localtime( &pUsersT[ulKey].tWarned ) );
							pResponse->strContent += "<td class=\"date\">" + string( pTime ).substr( 2, string( pTime ).size( ) - 5 ) + "</td>\n";
						}
						else
							pResponse->strContent += "<td class=\"date\">" + gmapLANG_CFG["na"] + "</td>\n";
						if( ( pRequest->user.ucAccess & m_ucAccessEditUsers ) || ( pRequest->user.ucAccess & m_ucAccessDelUsers ) )
						{
							pResponse->strContent += "<td class=\"admin\">";
							if( pRequest->user.ucAccess & m_ucAccessEditUsers )
								pResponse->strContent += "[<a class=\"black\" title=\"" + gmapLANG_CFG["edit"] + "\" href=\"" + RESPONSE_STR_USERS_HTML + "?uid=" + pUsersT[ulKey].strUID + "&amp;action=edit\">" + gmapLANG_CFG["edit"] + "</a>]";
							if( pRequest->user.ucAccess & m_ucAccessDelUsers )
								pResponse->strContent += "[<a class=\"red\" title=\"" + gmapLANG_CFG["delete"] + "\" href=\"" + RESPONSE_STR_USERS_HTML + "?uid=" + pUsersT[ulKey].strUID+ "&amp;action=delete\">" + gmapLANG_CFG["delete"] + "</a>]";
							pResponse->strContent += "</td>";
						}
						pResponse->strContent += "</tr>\n";

						ulAdded++;
					}
					else
						ulSkipped++;
				}
			}

			// some finishing touches

			if( bFound )
			{
				pResponse->strContent += "</table>\n";
				pResponse->strContent += "</div>\n\n";
			}
			
			string strInsert = string( );
			
			// How many results found?
			switch( ulFound )
			{
			case RESULTS_ZERO:
				strInsert += "<p class=\"results\">" + gmapLANG_CFG["result_none_found"] + "</p>\n\n";
				break;
//			case RESULTS_ONE:
//				pResponse->strContent += "<p class=\"results\">" + gmapLANG_CFG["result_1_found"] + "</p>\n\n";
//				break;
			default:
				// Many results found
				strInsert += "<p class=\"results\">" + UTIL_Xsprintf( gmapLANG_CFG["result_x_found"].c_str( ), CAtomInt( ulFound ).toString( ).c_str( ) ) + "</p>\n\n";
			}
			
			vector< pair< string, string > > vecParams;
			vecParams.reserve(64);
			string strJoined = string( );

			vecParams.push_back( pair<string, string>( string( "per_page" ), cstrPerPage ) );
			vecParams.push_back( pair<string, string>( string( "sort" ), cstrSort ) );
			vecParams.push_back( pair<string, string>( string( "search" ), cstrSearch ) );
			vecParams.push_back( pair<string, string>( string( "smode" ), cstrSearchMode ) );
			vecParams.push_back( pair<string, string>( string( "show" ), cstrShow ) );
			
			strJoined = UTIL_RemoveHTML( UTIL_HTMLJoin( vecParams, string( "&" ), string( "&" ), string( "=" ) ) );
			
			// page numbers top
			
			strInsert += UTIL_PageBar( ulFound, cstrPage, uiOverridePerPage, RESPONSE_STR_USERS_HTML, strJoined, true, true );
			
			if( iCountGoesHere != string :: npos )
				pResponse->strContent.insert( iCountGoesHere, strInsert );

			// page numbers bottom
			pResponse->strContent += UTIL_PageBar( ulFound, cstrPage, uiOverridePerPage, RESPONSE_STR_USERS_HTML, strJoined, false, true );

			// Output common HTML tail
			HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_USERS ) );
		}
		else
		{
			const string cstrAction( pRequest->mapParams["action"] );
			const string cstrOK( pRequest->mapParams["ok"] );
			
			if( !cstrAction.empty( ) )
			{
				string cstrUser = string( );
				
				CMySQLQuery *pQuery = new CMySQLQuery( "SELECT busername,bemail,baccess,bgroup,btitle,buploaded,bdownloaded,bbonus,UNIX_TIMESTAMP(bwarned),bnote,binvites,binvitable FROM users WHERE buid=" + cstrUID );
					
				vector<string> vecQuery;
			
				vecQuery.reserve(12);

				vecQuery = pQuery->nextRow( );
				
				delete pQuery;
				
				unsigned char ucAccess = 0;
				
				if( vecQuery.size( ) == 12 )
				{
					cstrUser = vecQuery[0];
					ucAccess = atoi( vecQuery[2].c_str( ) );
				}
				
				if( ucAccess > m_ucAccessEditUsers && !( pRequest->user.ucAccess & m_ucAccessEditAdmins ) )
				{
					// Output common HTML head
					HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["index_page"], string( CSS_USERS ), string( ), NOT_INDEX, CODE_401 );
					// Output common HTML tail
					HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_USERS ) );
					
					return;
				}
				
				//
				// edit user
				//

				if( cstrAction == "edit" && ( pRequest->user.ucAccess & m_ucAccessEditUsers ) )
				{
					// Output common HTML head
					HTML_Common_Begin( pRequest, pResponse, gmapLANG_CFG["users_page"], string( CSS_USERS ), string( ), NOT_INDEX, CODE_200 );
					// Does the user exist?
					if( vecQuery.size( ) == 12 && !cstrUser.empty( ) )
					{
						// Compose an edit users page view

						unsigned char ucAccess = 0;
						unsigned char ucGroup = 0;

						if( !vecQuery[2].empty( ) )
							ucAccess = (unsigned char)atoi( vecQuery[2].c_str( ) );
						
						if( !vecQuery[3].empty( ) )
							ucGroup = (unsigned char)atoi( vecQuery[3].c_str( ) );

						pResponse->strContent += "<div class=\"edit_users\">\n";
						pResponse->strContent += "<table class=\"edit_users\">\n";
						pResponse->strContent += "<form method=\"post\" name=\"editusers\" action=\"" + RESPONSE_STR_USERS_HTML + "\" onSubmit=\"return validate( this )\" enctype=\"multipart/form-data\">\n";
						pResponse->strContent += "<input name=\"uid\" type=hidden value=\"" + cstrUID + "\">\n";
						pResponse->strContent += "<input name=\"action\" type=hidden value=\"edit\">\n";
						pResponse->strContent += "<input name=\"ok\" type=hidden value=1>\n";
						// Edit User
						pResponse->strContent += "<tr class=\"edit_users\">\n";
						pResponse->strContent += "<th class=\"edit_users\" colspan=\"2\">\n" + UTIL_Xsprintf( gmapLANG_CFG["users_editing_user"].c_str( ), cstrUser.c_str( ) ) + "</th>\n\n" ;

						// Original code
						pResponse->strContent += "<tr class=\"edit_users\">\n";
						pResponse->strContent += "<th class=\"edit_users\">\n" + gmapLANG_CFG["users_password"] + "</th>";
						pResponse->strContent += "<td class=\"edit_users\">\n";
						pResponse->strContent += "<input id=\"id_password\" name=\"us_password\" alt=\"[" + gmapLANG_CFG["users_password"] + "]\" type=password size=20>" + gmapLANG_CFG["users_password_optional"] + "</td>\n</tr>\n";
						pResponse->strContent += "<tr class=\"edit_users\">\n";
						pResponse->strContent += "<th class=\"edit_users\">\n" + gmapLANG_CFG["users_verify_password"] + "</th>";
						pResponse->strContent += "<td class=\"edit_users\">\n";
						pResponse->strContent += "<div class=\"edit_input_users\"><input id=\"id_verify_password\" name=\"us_password_verify\" alt=\"[" + gmapLANG_CFG["users_verify_password"] + "]\" type=password size=20>" + gmapLANG_CFG["users_password_optional"] + "</td>\n</tr>\n";
						pResponse->strContent += "<tr class=\"edit_users\">\n";
						pResponse->strContent += "<th class=\"edit_users\">\n" + gmapLANG_CFG["email"] + "</th>";
						pResponse->strContent += "<td class=\"edit_users\">\n";
						pResponse->strContent += "<input id=\"id_email\" name=\"us_email\" alt=\"[" + gmapLANG_CFG["email"] + "]\" type=text size=40";
						if( !vecQuery[1].empty( ) )
							pResponse->strContent += " value=\"" + vecQuery[1] + "\"";
						pResponse->strContent += "></td>\n</tr>\n";

						pResponse->strContent += "<tr class=\"edit_users\">\n";
						pResponse->strContent += "<th class=\"edit_users\">\n" + gmapLANG_CFG["user_uploaded"] + "</th>";
						pResponse->strContent += "<td class=\"edit_users\">\n";
						pResponse->strContent += "<input id=\"id_uploaded\" name=\"us_uploaded\" alt=\"[" + gmapLANG_CFG["user_uploaded"] + "]\" type=text size=20";
						if( !vecQuery[5].empty( ) )
							pResponse->strContent += " value=\"" + vecQuery[5] + "\"";
						pResponse->strContent += "></td>\n</tr>\n";
						pResponse->strContent += "<tr class=\"edit_users\">\n";
						pResponse->strContent += "<th class=\"edit_users\">\n" + gmapLANG_CFG["user_downloaded"] + "</th>";
						pResponse->strContent += "<td class=\"edit_users\">\n";
						pResponse->strContent += "<input id=\"id_downloaded\" name=\"us_downloaded\" alt=\"[" + gmapLANG_CFG["user_downloaded"] + "]\" type=text size=20";
						if( !vecQuery[6].empty( ) )
							pResponse->strContent += " value=\"" +  vecQuery[6] + "\"";
						pResponse->strContent += "></td>\n</tr>\n";
						pResponse->strContent += "<tr class=\"edit_users\">\n";
						pResponse->strContent += "<th class=\"edit_users\">\n" + gmapLANG_CFG["user_bonus"] + "</th>";
						pResponse->strContent += "<td class=\"edit_users\">\n";
						pResponse->strContent += "<input id=\"id_bonus\" name=\"us_bonus\" alt=\"[" + gmapLANG_CFG["user_bonus"] + "]\" type=text size=20";
						if( !vecQuery[7].empty( ) )
							pResponse->strContent += " value=\"" + vecQuery[7] + "\"";
						pResponse->strContent += "></td>\n</tr>\n";
						pResponse->strContent += "<tr class=\"edit_users\">\n";
						pResponse->strContent += "<th class=\"edit_users\">\n" + gmapLANG_CFG["user_warned"] + "</th>";
						pResponse->strContent += "<td class=\"edit_users\">\n";
						pResponse->strContent += "<input id=\"id_warned\" name=\"us_warned\" alt=\"[" + gmapLANG_CFG["user_warned"] + "]\" type=checkbox";
						if( !vecQuery[8].empty( ) && vecQuery[8] != "0" )
							pResponse->strContent += " checked=\"checked\"";
						pResponse->strContent += "><label for=\"id_warned\">" + gmapLANG_CFG["user_warned"] + "</label><br>\n";
						pResponse->strContent += "<input id=\"id_warned_time\" name=\"us_warned_time\" alt=\"[" + gmapLANG_CFG["user_warned_time"] + "]\" type=text size=20 value=\"0\">";
						pResponse->strContent += gmapLANG_CFG["user_warned_time"] + "<br>\n";
						pResponse->strContent += "<input name=\"us_warned_reason\" id=\"id_warned_reason\" alt=\"[" + gmapLANG_CFG["user_warned_reason"] + "]\" type=text size=40 maxlength=" + CAtomInt( MAX_FILENAME_LEN ).toString( ) + " value=\"\">";
						pResponse->strContent += gmapLANG_CFG["user_warned_reason"] + "</td>\n</tr>\n";
						
						pResponse->strContent += "<tr class=\"edit_users\">\n";
						pResponse->strContent += "<th class=\"edit_users\">\n" + gmapLANG_CFG["user_invites"] + "</th>";
						pResponse->strContent += "<td class=\"edit_users\">\n";
						pResponse->strContent += "<input id=\"id_invitable\" name=\"us_invitable\" alt=\"[" + gmapLANG_CFG["user_invitable"] + "]\" type=checkbox";
						if( !vecQuery[11].empty( ) && vecQuery[11] != "0" )
							pResponse->strContent += " checked=\"checked\"";
						pResponse->strContent += "><label for=\"id_invitable\">" + gmapLANG_CFG["user_invitable"] + "</label><br>\n";
						pResponse->strContent += "<input id=\"id_invites\" name=\"us_invites\" alt=\"[" + gmapLANG_CFG["user_invites"] + "]\" type=text size=20";
						if( !vecQuery[10].empty( ) )
							pResponse->strContent += " value=\"" + vecQuery[10] + "\"";
						pResponse->strContent += ">" + gmapLANG_CFG["user_invites"];
						pResponse->strContent += "</td>\n</tr>\n";
						
						pResponse->strContent += "<tr class=\"edit_users\">\n";
						pResponse->strContent += "<th class=\"edit_users\">\n" + gmapLANG_CFG["user_note"] + "</th>";
						pResponse->strContent += "<td class=\"edit_users\">\n";
						pResponse->strContent += "<textarea id=\"notearea\" name=\"us_note\" rows=4 cols=40>";
						if( !vecQuery[9].empty( ) )
							pResponse->strContent += UTIL_RemoveHTML3( vecQuery[9] );
						pResponse->strContent += "</textarea></td>\n</tr>\n";
						
						// Display the users access level
						pResponse->strContent += "<tr class=\"edit_users\">\n";
						pResponse->strContent += "<th class=\"edit_users\">\n" + gmapLANG_CFG["access"] + "</th>";
						pResponse->strContent += "<td class=\"edit_users\">\n";
						pResponse->strContent += "<input id=\"id_view\" name=\"us_access_view\" alt=\"[" + gmapLANG_CFG["users_view_access"] + "]\" type=checkbox";

						if( ucAccess & ACCESS_VIEW )
							pResponse->strContent += " checked";

						pResponse->strContent += "> <label for=\"id_view\">" + gmapLANG_CFG["users_view_access"] + "</label><br></div>\n";
						pResponse->strContent += "<input id=\"id_download\" name=\"us_access_dl\" alt=\"[" + gmapLANG_CFG["users_dl_access"] + "]\" type=checkbox";

						if( ucAccess & ACCESS_DL )
							pResponse->strContent += " checked";

						pResponse->strContent += "> <label for=\"id_download\">" + gmapLANG_CFG["users_dl_access"] + "</label><br>\n";
						pResponse->strContent += "<input id=\"id_comments\" name=\"us_access_comments\" alt=\"[" + gmapLANG_CFG["users_comments_access"] + "]\" type=checkbox";

						if( ucAccess & ACCESS_COMMENTS )
							pResponse->strContent += " checked";

						pResponse->strContent += "> <label for=\"id_comments\">" + gmapLANG_CFG["users_comments_access"] + "</label><br>\n";
						pResponse->strContent += "<input id=\"id_upload\" name=\"us_access_upload\"  alt=\"[" + gmapLANG_CFG["users_upload_access"] + "]\"type=checkbox";

						if( ucAccess & ACCESS_UPLOAD )
							pResponse->strContent += " checked";

						pResponse->strContent += "> <label for=\"id_upload\">" + gmapLANG_CFG["users_upload_access"] + "</label><br>\n";
						pResponse->strContent += "<input id=\"id_edit\" name=\"us_access_edit\" alt=\"[" + gmapLANG_CFG["users_edit_access"] + "]\" type=checkbox";

						if( ucAccess & ACCESS_EDIT )
							pResponse->strContent += " checked";

						pResponse->strContent += "> <label for=\"id_edit\">" + gmapLANG_CFG["users_edit_access"] + "</label><br>\n";
						pResponse->strContent += "<input id=\"id_admin\" name=\"us_access_admin\" alt=\"[" + gmapLANG_CFG["users_admin_access"] + "]\" type=checkbox";

						if( ucAccess & ACCESS_ADMIN )
							pResponse->strContent += " checked";

						pResponse->strContent += "> <label for=\"id_admin\">" + gmapLANG_CFG["users_admin_access"] + "</label></td>\n</tr>\n";
// 						pResponse->strContent += "<div class=\"edit_input_users\"><input id=\"id_signup\" name=\"us_access_signup\" alt=\"[" + gmapLANG_CFG["users_signup_access"] + "]\" type=checkbox";
// 
// 						if( ucAccess & ACCESS_SIGNUP )
// 							pResponse->strContent += " checked";

// 						pResponse->strContent += "> <label for=\"id_signup\">" + gmapLANG_CFG["users_signup_access"] + "</label><br><br></div>\n";
						
						pResponse->strContent += "<tr class=\"edit_users\">\n";
						pResponse->strContent += "<th class=\"edit_users\">\n" + gmapLANG_CFG["group"] + "</th>";
						pResponse->strContent += "<td class=\"edit_users\">\n";
						pResponse->strContent += "<input id=\"id_none\" name=\"us_group\" alt=\"[" + gmapLANG_CFG["group_none"] + "]\" type=radio value=\"none\"";

						if( !ucGroup )
							pResponse->strContent += " checked";

						pResponse->strContent += "> <label for=\"id_none\">" + gmapLANG_CFG["group_none"] + "</label>\n";
						
						pResponse->strContent += "<input id=\"id_friends\" name=\"us_group\" alt=\"[" + gmapLANG_CFG["group_friends"] + "]\" type=radio value=\"friends\"";

						if( ucGroup & GROUP_FRIENDS )
							pResponse->strContent += " checked";

						pResponse->strContent += "> <label for=\"id_friends\">" + gmapLANG_CFG["group_friends"] + "</label>\n";
						
						pResponse->strContent += "<input id=\"id_retired\" name=\"us_group\" alt=\"[" + gmapLANG_CFG["group_retired"] + "]\" type=radio value=\"retired\"";

						if( ucGroup & GROUP_RETIRED )
							pResponse->strContent += " checked";

						pResponse->strContent += "> <label for=\"id_retired\">" + gmapLANG_CFG["group_retired"] + "</label>\n";
						
						pResponse->strContent += "<input id=\"id_vip\" name=\"us_group\" alt=\"[" + gmapLANG_CFG["group_vip"] + "]\" type=radio value=\"vip\"";

						if( ucGroup & GROUP_VIP )
							pResponse->strContent += " checked";

						pResponse->strContent += "> <label for=\"id_vip\">" + gmapLANG_CFG["group_vip"] + "</label></td>\n</tr>\n";
						
						pResponse->strContent += "<tr class=\"edit_users\">\n";
						pResponse->strContent += "<th class=\"edit_users\">\n" + gmapLANG_CFG["title"] + "</th>";
						pResponse->strContent += "<td class=\"edit_users\">\n";
						pResponse->strContent += "<input name=\"us_title\" id=\"id_title\" alt=\"[" + gmapLANG_CFG["title"] + "]\" type=text size=40 maxlength=32 value=\"";
						pResponse->strContent += UTIL_RemoveHTML( vecQuery[4] ) + "\">";
						pResponse->strContent += "</td>\n</tr>\n";


						// The Trinity Edition - Addition Begins
						// The following creates a CANCEL EDIT button
						// when EDITING A USER

						pResponse->strContent += "<tr class=\"edit_users\">\n";
						pResponse->strContent += "<td class=\"edit_users\" colspan=\"2\">\n";
						pResponse->strContent += "<div class=\"edit_users_button\">\n";
						
						pResponse->strContent += Button_Submit( "submit_edit", string( gmapLANG_CFG["edit_user"] ) );
						pResponse->strContent += Button_Back( "cancel_edit", string( gmapLANG_CFG["cancel"] ) );
						
						pResponse->strContent += "\n</div>\n</td>\n</tr>\n";

						// finish
						pResponse->strContent += "</form>\n</table>\n</div><p>\n";
						
						pResponse->strContent += "<div class=\"edit_users\">\n";
						pResponse->strContent += "<table class=\"edit_users\">\n";
						pResponse->strContent += "<tr class=\"edit_users\">\n";
						pResponse->strContent += "<td class=\"edit_users\">\n";
						pResponse->strContent += "<form method=\"post\" name=\"editusers\" action=\"" + RESPONSE_STR_USERS_HTML + "\" enctype=\"multipart/form-data\">\n";
						pResponse->strContent += "<p class=\"edit_users\">" + UTIL_Xsprintf( gmapLANG_CFG["users_editing_user_add_minus"].c_str( ), cstrUser.c_str( ) ) + "</p>\n\n" ;
						pResponse->strContent += "<div class=\"edit_input_users\"><input name=\"uid\" type=hidden value=\"" + cstrUID + "\"></div>\n";
						pResponse->strContent += "<div class=\"edit_input_users\"><input name=\"action\" type=hidden value=\"edit\"></div>\n";
						pResponse->strContent += "<div class=\"edit_input_users\"><input name=\"ok\" type=hidden value=2></div>\n";
						pResponse->strContent += "<div class=\"edit_input_users\">" + gmapLANG_CFG["users_editing_user_add"] + "<input id=\"id_add_uploaded\" name=\"us_add_uploaded\" alt=\"[" + gmapLANG_CFG["users_editing_user_add"] + gmapLANG_CFG["user_uploaded"] + "]\" type=text size=20";
						pResponse->strContent += " value=\"0\"> <label for=\"id_uploaded\">(GB)" + gmapLANG_CFG["user_uploaded"] + "</label><br></div>\n";
						pResponse->strContent += "<div class=\"edit_input_users\">" + gmapLANG_CFG["users_editing_user_add"] + "<input id=\"id_add_downloaded\" name=\"us_add_downloaded\" alt=\"[" + gmapLANG_CFG["users_editing_user_add"] + gmapLANG_CFG["user_downloaded"] + "]\" type=text size=20";
						pResponse->strContent += " value=\"0\"> <label for=\"id_downloaded\">(GB)" + gmapLANG_CFG["user_downloaded"] + "</label><br></div>\n";
						pResponse->strContent += "<div class=\"edit_input_users\">" + gmapLANG_CFG["users_editing_user_add"] + "<input id=\"id_add_bonus\" name=\"us_add_bonus\" alt=\"[" + gmapLANG_CFG["users_editing_user_add"] + gmapLANG_CFG["user_bonus"] + "]\" type=text size=20";
						pResponse->strContent += " value=\"0\"> <label for=\"id_bonus\">" + gmapLANG_CFG["user_bonus"] + "</label><br><br></div>\n";
						pResponse->strContent += "<div class=\"edit_input_users\">\n";
						
						pResponse->strContent += Button_Submit( "submit_edit", string( gmapLANG_CFG["edit_user"] ) );
						pResponse->strContent += Button_Back( "cancel_edit", string( gmapLANG_CFG["cancel"] ) );
						
						pResponse->strContent += "\n</div>\n";
						pResponse->strContent += "</form></td></tr>\n";
						
						pResponse->strContent += "<tr class=\"edit_users\">\n";
						pResponse->strContent += "<td class=\"edit_users\">\n";
						pResponse->strContent += "<form method=\"post\" name=\"editusers\" action=\"" + RESPONSE_STR_USERS_HTML + "\" enctype=\"multipart/form-data\">\n";
						pResponse->strContent += "<p><div class=\"edit_input_users\"><input name=\"uid\" type=hidden value=\"" + cstrUID + "\"></div>\n";
						pResponse->strContent += "<div class=\"edit_input_users\"><input name=\"action\" type=hidden value=\"edit\"></div>\n";
						pResponse->strContent += "<div class=\"edit_input_users\"><input name=\"ok\" type=hidden value=0></div>\n";
						pResponse->strContent += "<div class=\"edit_input_users\">" + gmapLANG_CFG["users_editing_user_minus"] + "<input id=\"id_minus_uploaded\" name=\"us_minus_uploaded\" alt=\"[" + gmapLANG_CFG["users_editing_user_minus"] + gmapLANG_CFG["user_uploaded"] + "]\" type=text size=20";
						pResponse->strContent += " value=\"0\"> <label for=\"id_uploaded\">(GB)" + gmapLANG_CFG["user_uploaded"] + "</label><br></div>\n";
						pResponse->strContent += "<div class=\"edit_input_users\">" + gmapLANG_CFG["users_editing_user_minus"] + "<input id=\"id_minus_downloaded\" name=\"us_minus_downloaded\" alt=\"[" + gmapLANG_CFG["users_editing_user_minus"] + gmapLANG_CFG["user_downloaded"] + "]\" type=text size=20";
						pResponse->strContent += " value=\"0\"> <label for=\"id_downloaded\">(GB)" + gmapLANG_CFG["user_downloaded"] + "</label><br></div>\n";
						pResponse->strContent += "<div class=\"edit_input_users\">" + gmapLANG_CFG["users_editing_user_minus"] + "<input id=\"id_minus_bonus\" name=\"us_minus_bonus\" alt=\"[" + gmapLANG_CFG["users_editing_user_minus"] + gmapLANG_CFG["user_bonus"] + "]\" type=text size=20";
						pResponse->strContent += " value=\"0\"> <label for=\"id_bonus\">" + gmapLANG_CFG["user_bonus"] + "</label><br><br></div>\n";
						
						pResponse->strContent += "<div class=\"edit_input_users\">\n";
						
						pResponse->strContent += Button_Submit( "submit_edit", string( gmapLANG_CFG["edit_user"] ) );
						pResponse->strContent += Button_Back( "cancel_edit", string( gmapLANG_CFG["cancel"] ) );
						
						pResponse->strContent += "\n</div>\n";
						pResponse->strContent += "</form>\n</td>\n</tr>\n</table>\n</div>\n";

						// Output common HTML tail
						HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_USERS ) );

						return;
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

				else if( cstrAction == "delete" && ( pRequest->user.ucAccess & m_ucAccessDelUsers ) )
				{
					// Output common HTML head
					HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["users_page"], string( CSS_USERS ), string( ), NOT_INDEX, CODE_200 );

					if( vecQuery.size( ) == 12 && !cstrUser.empty( ) )
					{
						// Are you sure it is ok to delete the user?
						if( cstrOK == "1" )
						{
							// Delete the user
							deleteUser( cstrUID );
							
							UTIL_LogFilePrint( "deleteUser: %s deleted user %s\n", pRequest->user.strLogin.c_str( ), cstrUser.c_str( ) );
		
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
							pResponse->strContent += "<p class=\"delete\"><a title=\"" + gmapLANG_CFG["yes"] + "\" href=\"" + RESPONSE_STR_USERS_HTML + "?uid=" + cstrUID + "&amp;action=delete&amp;ok=1\">" + gmapLANG_CFG["yes"] + "</a> | <a title=\"" + gmapLANG_CFG["no"] + "\" href=\"" + string( JS_BACK ) + "\">" + gmapLANG_CFG["no"] + "</a></p>\n";

							// Output common HTML tail
							HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_USERS ) );

							return;
						}
					}
					else
					{
						pResponse->strContent += "<p class=\"created_message_users\">" + UTIL_Xsprintf( gmapLANG_CFG["users_edit_nouser"].c_str( ), cstrUser.c_str( ) ) + "</p>\n\n" ;
						pResponse->strContent += "<p class=\"failed_message_users\"><a title=\"" + gmapLANG_CFG["previous_page"] + "\" href=\"" + string( JS_BACK ) + "\">" + gmapLANG_CFG["previous_page"] + "</a></p>\n";

						// Output common HTML tail
						HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_USERS ) );

						return;
					}
				}
				else if( cstrAction == "reset" )
				{
					// Output common HTML head
					HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["users_page"], string( CSS_USERS ), string( ), NOT_INDEX, CODE_200 );
					if( vecQuery.size( ) == 12 && !cstrUser.empty( ) )
					{
						// Are you sure it is ok to reset the passkey?
						if( cstrOK == "1" )
						{
							// reset the passkey
							InitPasskey( cstrUser );

							// Inform the operator that the passkey was reset
							pResponse->strContent += "<p class=\"deleted_users\">" + UTIL_Xsprintf( gmapLANG_CFG["users_reset_passkey"].c_str( ), cstrUser.c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_users"] + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "?uid=" + cstrUID + "\">" ).c_str( ), "</a>" ) + "</p>\n\n" ;

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

							pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["users_reset_passkey_q"].c_str( ), cstrUser.c_str( ) ) + "</p>\n\n" ;
							pResponse->strContent += "<p class=\"delete\"><a title=\"" + gmapLANG_CFG["yes"] + "\" href=\"" + RESPONSE_STR_USERS_HTML + "?uid=" + cstrUID + "&amp;action=reset&amp;ok=1\">" + gmapLANG_CFG["yes"] + "</a> | <a title=\"" + gmapLANG_CFG["no"] + "\" href=\"" + string( JS_BACK ) + "\">" + gmapLANG_CFG["no"] + "</a></p>\n";

							// Output common HTML tail
							HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_USERS ) );

							return;
						}
					}
					else
					{
						pResponse->strContent += "<p class=\"created_message_users\">" + UTIL_Xsprintf( gmapLANG_CFG["users_edit_nouser"].c_str( ), cstrUser.c_str( ) ) + "</p>\n\n" ;
						pResponse->strContent += "<p class=\"failed_message_users\"><a title=\"" + gmapLANG_CFG["previous_page"] + "\" href=\"" + string( JS_BACK ) + "\">" + gmapLANG_CFG["previous_page"] + "</a></p>\n";

						// Output common HTML tail
						HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_USERS ) );

						return;
					}
				}
			}
		}
	}
	else
	{
		// Not authorised

		// Output common HTML head
		HTML_Common_Begin( pRequest, pResponse, gmapLANG_CFG["users_page"], string( CSS_USERS ), string( ), NOT_INDEX, CODE_401 );

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_USERS ) );
	}
}

void CTracker :: serverResponseUsersPOST( struct request_t *pRequest, struct response_t *pResponse, CAtomList *pPost )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
		if( IsIPBanned( pRequest, pResponse, btv, gmapLANG_CFG["users_page"], string( CSS_USERS ), NOT_INDEX ) )
			return;
		
	string cstrLogin = string( );
	string cstrPass0 = string( );
	string cstrPass = string( );
	string cstrPass2 = string( );
	string cstrMail = string( );
	string cstrUploaded = string( );
	string cstrDownloaded = string( );
	string cstrBonus = string( );
	string cstrWarned = string( );
	string cstrWarnedTime = string( );
	string cstrWarnedReason = string( );
	string cstrInvitable = string( );
	string cstrInvites = string( );

	string cstrNote = string( );
	string cstrAccView = string( );
	string cstrAccDL = string( );
	string cstrAccComments = string( );
	string cstrAccUpload = string( );
	string cstrAccEdit = string( );
	string cstrAccAdmin = string( );
	string cstrAccSignup = string( );
	string cstrGroup = string( );
	string cstrTitle = string( );
	string cstrUID = string( );
	string cstrAction = string( );
	string cstrOK = string( );
	string cstrAddUploaded = string( );
	string cstrAddDownloaded = string( );
	string cstrAddBonus = string( );
	string cstrMinusUploaded = string( );
	string cstrMinusDownloaded = string( );
	string cstrMinusBonus = string( );
	bool bCreate = false;
	
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
						{
							cstrLogin = pData->toString( );
							bCreate = true;
						}
						else if( strName == "us_old_password")
							cstrPass0 = pData->toString( );
						else if( strName == "us_password")
							cstrPass = pData->toString( );
						else if( strName == "us_password_verify")
							cstrPass2 = pData->toString( );
						else if( strName == "us_email" )
							cstrMail = pData->toString( );
						else if( strName == "us_uploaded" )
							cstrUploaded = pData->toString( );
						else if( strName == "us_downloaded" )
							cstrDownloaded = pData->toString( );
						else if( strName == "us_bonus" )
							cstrBonus = pData->toString( );
						else if( strName == "us_warned" )
							cstrWarned = pData->toString( );
						else if( strName == "us_warned_time" )
							cstrWarnedTime = pData->toString( );
						else if( strName == "us_warned_reason" )
							cstrWarnedReason = pData->toString( );
						else if( strName == "us_invitable" )
							cstrInvitable = pData->toString( );
						else if( strName == "us_invites" )
							cstrInvites = pData->toString( );
						else if( strName == "us_note" )
							cstrNote = pData->toString( );
						else if( strName == "us_access_view" )
							cstrAccView = pData->toString( );
						else if( strName == "us_access_dl" )
							cstrAccDL = pData->toString( );
						else if( strName == "us_access_comments" )
							cstrAccComments = pData->toString( );
						else if( strName == "us_access_upload" )
							cstrAccUpload = pData->toString( );
						else if( strName == "us_access_edit" )
							cstrAccEdit = pData->toString( );
						else if( strName == "us_access_admin" )
							cstrAccAdmin = pData->toString( );
// 						else if( strName == "us_access_signup" )
// 							cstrAccSignup = pData->toString( );
						else if( strName == "us_group" )
							cstrGroup = pData->toString( );
						else if( strName == "us_title" )
							cstrTitle = pData->toString( );
						else if( strName == "uid" )
							cstrUID = pData->toString( );
						else if( strName == "action" )
							cstrAction = pData->toString( );
						else if( strName == "ok" )
							cstrOK = pData->toString( );
						else if( strName == "us_add_uploaded" )
							cstrAddUploaded = pData->toString( );
						else if( strName == "us_add_downloaded" )
							cstrAddDownloaded = pData->toString( );
						else if( strName == "us_add_bonus" )
							cstrAddBonus = pData->toString( );
						else if( strName == "us_minus_uploaded" )
							cstrMinusUploaded = pData->toString( );
						else if( strName == "us_minus_downloaded" )
							cstrMinusDownloaded = pData->toString( );
						else if( strName == "us_minus_bonus" )
							cstrMinusBonus = pData->toString( );

					}
					else
					{
						// Output common HTML head
						HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["users_page"], string( CSS_USERS ), string( ), NOT_INDEX, CODE_400 );

						// failed
						pResponse->strContent += "<p class=\"failed\">" + gmapLANG_CFG["failed"] + "</p>\n";
						// Signal a bad request
						pResponse->strContent += "<p class=\"body_upload\">400 " + gmapLANG_CFG["server_response_400"] + "</p>\n";

						// Output common HTML tail
						HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_USERS ) );

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
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["users_page"], string( CSS_USERS ), string( ), NOT_INDEX, CODE_400 );

		// failed
		pResponse->strContent += "<p class=\"failed\">" + gmapLANG_CFG["failed"] + "</p>\n";
		// Signal a bad request
		pResponse->strContent += "<p class=\"body_upload\">400 " + gmapLANG_CFG["server_response_400"] + "</p>\n";

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_USERS ) );

		if( gbDebug )
			UTIL_LogPrint( "Upload Warning - Bad request (no post received)\n" );

		return;
	}
	
	if( cstrUID.find_first_not_of( "1234567890" ) != string :: npos )
		cstrUID.erase( );
	
	if( !cstrUID.empty( ) && pRequest->user.strUID == cstrUID )
	{
		if( cstrAction == "edit" )
		{
			// Output common HTML head
			HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["users_page"], string( CSS_USERS ), string( ), NOT_INDEX, CODE_200 );
			
			string cstrUser = string( );

			CMySQLQuery *pQuery = new CMySQLQuery( "SELECT busername,bmd5 FROM users WHERE buid=" + cstrUID );
			
			vector<string> vecQuery;

			vecQuery.reserve(2);

			vecQuery = pQuery->nextRow( );

			delete pQuery;
			
			if( vecQuery.size( ) == 2 && !vecQuery[0].empty( ) )
				cstrUser = vecQuery[0];

			// Does the user exist?
			if( !cstrUser.empty( ) )
			{
				// Are you sure it's ok to edit the user?
				if( cstrOK == "1" )
				{
					const string cstrA0( cstrUser + ":" + gstrPasswordKey + ":" + cstrPass0 );

					unsigned char szMD5[16];
					memset( szMD5, 0, sizeof( szMD5 ) / sizeof( unsigned char ) );

					MD5_CTX md5;

					MD5Init( &md5 );
					MD5Update( &md5, (const unsigned char *)cstrA0.c_str( ), (unsigned int)cstrA0.size( ) );
					MD5Final( szMD5, &md5 );

					if( vecQuery[1] == string( (char *)szMD5, sizeof( szMD5 ) ) )
					{
						// Do we have the users password and verification password?
						if( !cstrPass.empty( ) && !cstrPass2.empty( ) )
						{
							// Are they the same?
							if( cstrPass == cstrPass2 )
							{
								// Set the users password

								const string cstrA1( cstrUser + ":" + gstrPasswordKey + ":" + cstrPass );

								unsigned char szMD5[16];
								memset( szMD5, 0, sizeof( szMD5 ) / sizeof( unsigned char ) );

								MD5_CTX md5;

								MD5Init( &md5 );
								MD5Update( &md5, (const unsigned char *)cstrA1.c_str( ), (unsigned int)cstrA1.size( ) );
								MD5Final( szMD5, &md5 );

								CMySQLQuery mq02( "UPDATE users SET bmd5=\'" + UTIL_StringToMySQL( string( (char *)szMD5, sizeof( szMD5 ) / sizeof( unsigned char ) ) ) + "\' WHERE buid=" + cstrUID );
								
// 								if( !( (CAtomDicti *)pUserToEdit )->getItem( "passkey" ) )
// 								{
// 									InitPasskey( cstrUser );
// 								}
								
								// Set the users email address
// 								( (CAtomDicti *)pUserToEdit )->setItem( "email", new CAtomString( cstrMail ) );
								// Original code - Save the users
								
								UTIL_LogFilePrint( "editUser: %s edited user %s\n", pRequest->user.strLogin.c_str( ), cstrUser.c_str( ) );
								

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
								pResponse->strContent += "<p class=\"failed_message_users\"><a title=\"" + gmapLANG_CFG["users_make_correct"] + "\" href=\"" + string( JS_BACK ) + "\">" + gmapLANG_CFG["users_make_correct"] + "</a><span class=\"pipe\"> | </span><a title=\"" + gmapLANG_CFG["users_start_over"] + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "\">" + gmapLANG_CFG["users_start_over"] + "</a></p>\n\n";

								// Output common HTML tail
								HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_USERS ) );

								return;
							}
						}
						if( cstrPass.empty( ) && cstrPass2.empty( ) )
						{

							// Set the users email address
// 							( (CAtomDicti *)pUserToEdit )->setItem( "email", new CAtomString( cstrMail ) );

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
						pResponse->strContent += "<p class=\"failed_message_users\"><a title=\"" + gmapLANG_CFG["users_make_correct"] + "\" href=\"" + string( JS_BACK ) + "\">" + gmapLANG_CFG["users_make_correct"] + "</a><span class=\"pipe\"> | </span><a title=\"" + gmapLANG_CFG["users_start_over"] + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "\">" + gmapLANG_CFG["users_start_over"] + "</a></p>\n\n";

						// Output common HTML tail
						HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_USERS ) );

						return;
					}
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
	}
	else if( ( pRequest->user.ucAccess & m_ucAccessCreateUsers ) || ( pRequest->user.ucAccess & m_ucAccessEditUsers ) )
	{
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

//		pResponse->strContent += "function edit_user( USER ) {\n";
//		pResponse->strContent += "window.location=\"" + RESPONSE_STR_USERS_HTML + "?up_edituser\" + USER\n";
//		pResponse->strContent += "}\n\n";

//		pResponse->strContent += "function clear_search_and_filters( ) {\n";
//		pResponse->strContent += "window.location=\"" + RESPONSE_STR_USERS_HTML + "\"\n";
//		pResponse->strContent += "}\n\n";

		pResponse->strContent += "//-->\n";
		pResponse->strContent += "</script>\n\n";
		
		//
		// create user
		//
		if( bCreate && ( pRequest->user.ucAccess & m_ucAccessCreateUsers ) )
		{
			// Do we have the compulsory user parameters?
			if( !cstrLogin.empty( ) && !cstrPass.empty( ) && !cstrPass2.empty( ) && !cstrMail.empty( ) )
			{
				if( cstrLogin.find_first_of( " .%&<>\"\n\r" ) != string :: npos || cstrLogin.size( ) > m_uiNameLength )
				{
					// Unable to signup. Your name must be less than " + CAtomInt( m_uiNameLength ).toString( ) + " characters long and it must not start or end with spaces.
					pResponse->strContent += "<p class=\"failed_message_users\">" + UTIL_Xsprintf( gmapLANG_CFG["users_login_create_fail"].c_str( ), UTIL_RemoveHTML( cstrLogin ).c_str( ) ) + "</p>\n";
					pResponse->strContent += "<p class=\"failed_message_users\">" + gmapLANG_CFG["users_do_q"] + "</p>\n";
					pResponse->strContent += "<p class=\"failed_message_users\"><a title=\"" + gmapLANG_CFG["users_make_correct"] + "\" href=\"" + string( JS_BACK ) + "\">" + gmapLANG_CFG["users_make_correct"] + "</a><span class=\"pipe\"> | </span><a title=\"" + gmapLANG_CFG["users_start_over"] + "\" href=\"" + RESPONSE_STR_USERS_HTML + "\">" + gmapLANG_CFG["users_start_over"] + "</a></p>\n\n";   

					// Output common HTML tail
					HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_SIGNUP ) );

					return;
				}
				
				if( cstrMail.find( "@" ) == string :: npos || cstrMail.find( "." ) == string :: npos )
				{
					// Unable to signup. Your e-mail address is invalid.
					pResponse->strContent += "<p class=\"failed_message_users\">" + UTIL_Xsprintf( gmapLANG_CFG["users_email_create_fail"].c_str( ), UTIL_RemoveHTML( cstrMail ).c_str( ) ) + "</p>\n";
					pResponse->strContent += "<p class=\"failed_message_users\">" + gmapLANG_CFG["users_do_q"] + "</p>\n";
					pResponse->strContent += "<p class=\"failed_message_users\"><a title=\"" + gmapLANG_CFG["users_make_correct"] + "\" href=\"" + string( JS_BACK ) + "\">" + gmapLANG_CFG["users_make_correct"] + "</a><span class=\"pipe\"> | </span><a title=\"" + gmapLANG_CFG["users_start_over"] + "\" href=\"" + RESPONSE_STR_USERS_HTML + "\">" + gmapLANG_CFG["users_start_over"] + "</a></p>\n\n";  

					// Output common HTML tail
					HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_SIGNUP ) );

					return;
				}
				
				// Do the passwords match?
				if( cstrPass == cstrPass2 )
				{
					// Does the user already exist?
					if( !getUserLogin( cstrLogin ).empty( ) )
					{
						// The Trinity Edition - Modification Begins
						// Created Links to either "Make Corrections" or "Start Over"
						// when creating a NEW USER FAILS because THE USER ALREADY EXISTS

						pResponse->strContent += "<p class=\"failed_message_users\">" + UTIL_Xsprintf( gmapLANG_CFG["users_exists_create_fail"].c_str( ), UTIL_RemoveHTML( cstrLogin ).c_str( ) ) + "</p>\n";
						pResponse->strContent += "<p class=\"failed_message_users\">" + gmapLANG_CFG["users_do_q"] + "</p>\n";
						pResponse->strContent += "<p class=\"failed_message_users\"><a title=\"" + gmapLANG_CFG["users_make_correct"] + "\" href=\"" + string( JS_BACK ) + "\">" + gmapLANG_CFG["users_make_correct"] + "</a><span class=\"pipe\"> | </span><a title=\"" + gmapLANG_CFG["users_start_over"] + "\" href=\"" + RESPONSE_STR_USERS_HTML + "\">" + gmapLANG_CFG["users_start_over"] + "</a></p>\n\n";    

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
// 						if( cstrAccSignup == "on" )
// 							ucAccess += ACCESS_SIGNUP;

						// Add the user
// 						addUser( cstrLogin, cstrPass, ucAccess, cstrMail );

						// The Trinity Edition - Modification Begins
						// The following places the Return to Users link in its own paragraph
						// as opposed to immediately following the "Created user..." message.
						
						string strAdded = addUser( cstrLogin, cstrPass, ucAccess, cstrMail );
						if( !strAdded.empty( ) )
						{
							m_pCache->addRowUsers( strAdded );
//							m_pCache->ResetUsers( );
							pResponse->strContent += "<p class=\"created_message_users\">" + UTIL_Xsprintf( gmapLANG_CFG["users_created_user"].c_str( ), UTIL_RemoveHTML( cstrLogin ).c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_users"] + "\" href=\"" + RESPONSE_STR_USERS_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";

							UTIL_LogFilePrint( "addUser: %s created user %s\n", pRequest->user.strLogin.c_str( ), cstrLogin.c_str( ) );
						}
						else
							pResponse->strContent += "<p class=\"created_message_users\">" + UTIL_Xsprintf( gmapLANG_CFG["users_max_create_fail"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_users"] + "\" href=\"" + RESPONSE_STR_USERS_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";

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
					pResponse->strContent += "<p class=\"failed_message_users\"><a title=\"" + gmapLANG_CFG["users_make_correct"] + "\" href=\"" + string( JS_BACK ) + "\">" + gmapLANG_CFG["users_make_correct"] + "</a><span class=\"pipe\"> | </span><a title=\"" + gmapLANG_CFG["users_start_over"] + "\" href=\"" + RESPONSE_STR_USERS_HTML + "\">" + gmapLANG_CFG["users_start_over"] + "</a></p>\n\n";

					// Output common HTML tail
					HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_USERS ) );

					return;
				}
			}
			else
			{
				// The Trinity Edition - Modification Begins
				// Created Links to either "Make Corrections" or "Start Over"
				// when creating a NEW USER FAILS because ALL FIELDS WERE NOT FILLED IN

				pResponse->strContent += "<p class=\"failed_message_users\">" + gmapLANG_CFG["users_fields_create_fail"] + "</p>\n";
				pResponse->strContent += "<p class=\"failed_message_users\">" + gmapLANG_CFG["users_do_q"] + "</p>\n";
				pResponse->strContent += "<p class=\"failed_message_users\"><a title=\"" + gmapLANG_CFG["users_make_correct"] + "\" href=\"" + string( JS_BACK ) + "\">" + gmapLANG_CFG["users_make_correct"] + "</a><span class=\"pipe\"> | </span><a title=\"" + gmapLANG_CFG["users_start_over"] + "\" href=\"" + RESPONSE_STR_USERS_HTML + "\">" + gmapLANG_CFG["users_start_over"] + "</a></p>\n\n";                

				// Output common HTML tail
				HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_USERS ) );

				return;
			}
		}
			
		//
		// edit user
		//

		if( cstrAction == "edit" && ( pRequest->user.ucAccess & m_ucAccessEditUsers ) )
		{
			string cstrUser = string( );
			
			CMySQLQuery *pQuery = new CMySQLQuery( "SELECT busername,baccess,bgroup,buploaded,bdownloaded,bbonus,UNIX_TIMESTAMP(bwarned),binvites FROM users WHERE buid=" + cstrUID );
			
			vector<string> vecQuery;

			vecQuery.reserve(8);

			vecQuery = pQuery->nextRow( );

			delete pQuery;
			
			if( vecQuery.size( ) == 8 && !vecQuery[0].empty( ) )
				cstrUser = vecQuery[0];

			// Does the user exist?
			if( !cstrUser.empty( ) )
			{
				// Are you sure it's ok to edit the user?
				if( cstrOK == "1" )
				{
					// Do we have the users password and verification password?
					if( !cstrPass.empty( ) && !cstrPass2.empty( ) )
					{
						// Are they the same?
						if( cstrPass == cstrPass2 )
						{
							// Set the users password

							const string cstrA1( cstrUser + ":" + gstrPasswordKey + ":" + cstrPass );

							unsigned char szMD5[16];
							memset( szMD5, 0, sizeof( szMD5 ) / sizeof( unsigned char ) );

							MD5_CTX md5;

							MD5Init( &md5 );
							MD5Update( &md5, (const unsigned char *)cstrA1.c_str( ), (unsigned int)cstrA1.size( ) );
							MD5Final( szMD5, &md5 );

							CMySQLQuery mq01( "UPDATE users SET bmd5=\'" + UTIL_StringToMySQL( string( (char *)szMD5, sizeof( szMD5 ) / sizeof( unsigned char ) ) ) + "\' WHERE buid=" + cstrUID );
						}
						else
						{
							// The Trinity Edition - Modification Begins
							// Created Links to either "Make Corrections" or "Start Over"
							// when EDITING USER FAILS because THE PASSWORDS DO NOT MATCH

							pResponse->strContent += "<p class=\"failed_message_users\">" + gmapLANG_CFG["users_password_edit_fail"] + "</p>\n";
							pResponse->strContent += "<p class=\"failed_message_users\">" + gmapLANG_CFG["users_do_q"] + "</p>\n";
							pResponse->strContent += "<p class=\"failed_message_users\"><a title=\"" + gmapLANG_CFG["users_make_correct"] + "\" href=\"" + string( JS_BACK ) + "\">" + gmapLANG_CFG["users_make_correct"] + "</a><span class=\"pipe\"> | </span><a title=\"" + gmapLANG_CFG["users_start_over"] + "\" href=\"" + RESPONSE_STR_USERS_HTML + "\">" + gmapLANG_CFG["users_start_over"] + "</a></p>\n\n";

							// Output common HTML tail
							HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_USERS ) );

							return;
						}
					}
					
					string strQuery = "UPDATE users set bemail=\'" + UTIL_StringToMySQL( cstrMail ) + "\'";

					bool bNum = true;
					for( int i = 0; i < cstrUploaded.length( ) && bNum ; i++ )
						if( !isdigit( cstrUploaded[i] ) )
							bNum  = false;
					for( int i = 0; i < cstrDownloaded.length( ) && bNum ; i++ )
						if( !isdigit( cstrDownloaded[i] ) )
							bNum  = false;
					for( int i = 0; i < cstrBonus.length( ) && bNum ; i++ )
						if( !isdigit( cstrBonus[i] ) )
							bNum  = false;
					for( int i = 0; i < cstrWarnedTime.length( ) && bNum ; i++ )
						if( !isdigit( cstrWarnedTime[i] ) )
							bNum  = false;
					for( int i = 0; i < cstrInvites.length( ) && bNum ; i++ )
						if( !isdigit( cstrInvites[i] ) )
							bNum  = false;
					if( bNum )
					{
						UTIL_LogFilePrint( "editUser: %s edit user %s start\n", pRequest->user.strLogin.c_str( ), cstrUser.c_str( ) );
						UTIL_LogFilePrint( "editUser: Uploaded: From %s To %s\n", vecQuery[3].c_str( ), cstrUploaded.c_str( ) );
						UTIL_LogFilePrint( "editUser: Downloaded: From %s To %s\n", vecQuery[4].c_str( ), cstrDownloaded.c_str( ) );
						UTIL_LogFilePrint( "editUser: Bonus: From %s To %s\n", vecQuery[5].c_str( ), cstrBonus.c_str( ) );
						if( cstrWarned == "on" && !cstrWarnedTime.empty( ) && cstrWarnedTime != "0" )
						{
							if( !pRequest->user.strLogin.empty( ) )
							{
								time_t tNow = time( 0 );
								char *szTime = asctime( localtime( &tNow ) );
								szTime[strlen( szTime ) - 1] = TERM_CHAR;
								
								string strTitle = gmapLANG_CFG["admin_warn_user_title"];

								string strMessage = UTIL_Xsprintf( gmapLANG_CFG["admin_warn_user"].c_str( ), UTIL_AccessToString( pRequest->user.ucAccess ).c_str( ), pRequest->user.strLogin.c_str( ), cstrWarnedTime.c_str( ), cstrWarnedReason.c_str( ) );
								
								sendMessage( pRequest->user.strLogin, pRequest->user.strUID, cstrUID, pRequest->strIP, strTitle, strMessage );
								
								cstrNote = cstrNote.insert( 0, UTIL_Xsprintf( "[%s] %s", szTime, string( strMessage + "\n\r" ).c_str( ) ) );
							}
							UTIL_LogFilePrint( "editUser: Warned: %s Day(s)\n", cstrWarnedTime.c_str( ) );
						}
						else
							UTIL_LogFilePrint( "editUser: Warned: No\n" );
						if( cstrInvitable == "on" )
							UTIL_LogFilePrint( "editUser: Invitable: Yes\n" );
						else
							UTIL_LogFilePrint( "editUser: Invitable: No\n" );
						UTIL_LogFilePrint( "editUser: Invites: From %s To %s\n", vecQuery[7].c_str( ), cstrInvites.c_str( ) );
						
						strQuery += ",buploaded=" + cstrUploaded;
						strQuery += ",bdownloaded=" + cstrDownloaded;
						strQuery += ",bbonus=" + cstrBonus;
						if( cstrWarned == "on" )
							if( !vecQuery[6].empty( ) && vecQuery[6] != "0" )
							{
								if( !cstrWarnedTime.empty( ) )
									strQuery += ",bwarned=bwarned+INTERVAL " + cstrWarnedTime + " DAY";
							}
							else
								strQuery += ",bwarned=NOW()+INTERVAL " + cstrWarnedTime + " DAY";
						else
							strQuery += ",bwarned=0";
						strQuery += ",binvites=" + cstrInvites;
						if( cstrInvitable == "on" )
							strQuery += ",binvitable=1";
						else
							strQuery += ",binvitable=0";
					}

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
// 					if( cstrAccSignup == "on" )
// 						ucAccess += ACCESS_SIGNUP;
					
					unsigned char ucGroup = 0;
					
					if( cstrGroup == "friends" )
						ucGroup = GROUP_FRIENDS;
					if( cstrGroup == "retired" )
						ucGroup = GROUP_RETIRED;
					if( cstrGroup == "vip" )
						ucGroup = GROUP_VIP;

					// Set the users access level
					strQuery += ",baccess=" + CAtomInt( ucAccess ).toString( );
					strQuery += ",bgroup=" + CAtomInt( ucGroup ).toString( );
					strQuery += ",btitle=\'" + UTIL_StringToMySQL( cstrTitle ) + "\'";
					UTIL_LogFilePrint( "editUser: Access: From %s To %s\n", UTIL_AccessToString( atoi( vecQuery[1].c_str( ) ) ).c_str( ), UTIL_AccessToString( ucAccess ).c_str( ) );
					UTIL_LogFilePrint( "editUser: Group: From %s To %s\n", UTIL_GroupToString( atoi( vecQuery[2].c_str( ) ) ).c_str( ), UTIL_GroupToString( ucGroup ).c_str( ) );
					
					if( atoi( vecQuery[1].c_str( ) ) != ucAccess || atoi( vecQuery[2].c_str( ) ) != ucGroup )
					{
						time_t tNow = time( 0 );
						char *szTime = asctime( localtime( &tNow ) );
						szTime[strlen( szTime ) - 1] = TERM_CHAR;
						
						string strTitle = gmapLANG_CFG["admin_edit_user_access_title"];

						string strMessage = UTIL_Xsprintf( gmapLANG_CFG["admin_edit_user_access"].c_str( ), UTIL_AccessToString( pRequest->user.ucAccess ).c_str( ), pRequest->user.strLogin.c_str( ), UTIL_AccessToString( atoi( vecQuery[1].c_str( ) ) ).c_str( ), UTIL_AccessToString( ucAccess ).c_str( ), UTIL_GroupToString( atoi( vecQuery[2].c_str( ) ) ).c_str( ), UTIL_GroupToString( ucGroup ).c_str( ) );
						
						sendMessage( pRequest->user.strLogin, pRequest->user.strUID, cstrUID, pRequest->strIP, strTitle, strMessage );
						
						cstrNote = cstrNote.insert( 0, UTIL_Xsprintf( "[%s] %s", szTime, string( strMessage + "\n\r" ).c_str( ) ) );
					}
					
					strQuery += ",bnote=\'" + UTIL_StringToMySQL( cstrNote ) + "\'";

					strQuery += " WHERE buid=" + cstrUID;
					CMySQLQuery mq01( strQuery );
					m_pCache->setRowUsers( cstrUID );
					UTIL_LogFilePrint( "editUser: %s edited user %s complete\n", pRequest->user.strLogin.c_str( ), cstrUser.c_str( ) );
						
// 					if( !( ( (CAtomDicti *)pUserToEdit )->getItem( "passkey" ) ) )
// 					{
// 						InitPasskey( cstrUser );
// 					}

					// Inform the operator that the user was modified
					pResponse->strContent += "<p class=\"created_message_users\">" + UTIL_Xsprintf( gmapLANG_CFG["users_edited_user"].c_str( ), cstrUser.c_str( ) , string( "<a title=\"" + gmapLANG_CFG["navbar_users"] + "\" href=\"" + RESPONSE_STR_USERS_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n\n" ;

					// Output common HTML tail
					HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_USERS ) );

					return;
				}
				else if( cstrOK == "0" )
				{
					bool bNum = true;
					for( int i = 0; i < cstrMinusUploaded.length( ) && bNum ; i++ )
						if( !isdigit( cstrMinusUploaded[i] ) )
							bNum  = false;
					for( int i = 0; i < cstrMinusDownloaded.length( ) && bNum ; i++ )
						if( !isdigit( cstrMinusDownloaded[i] ) )
							bNum  = false;
					for( int i = 0; i < cstrMinusBonus.length( ) && bNum ; i++ )
						if( !isdigit( cstrMinusBonus[i] ) )
							bNum  = false;
					if( bNum )
					{
						int64 iOldUploaded = 0, iOldDownloaded = 0, iOldBonus = 0;
						int64 iMinusUploaded = 0, iMinusDownloaded = 0, iMinusBonus = 0;
						if( !vecQuery[3].empty( ) )
							iOldUploaded = UTIL_StringTo64( vecQuery[3].c_str( ) );
						if( !vecQuery[4].empty( ) )
							iOldDownloaded = UTIL_StringTo64( vecQuery[4].c_str( ) );
						if( !vecQuery[5].empty( ) )
							iOldBonus = UTIL_StringTo64( vecQuery[5].c_str( ) );
						
						if( !cstrMinusUploaded.empty( ) )
							iMinusUploaded = UTIL_StringTo64( cstrMinusUploaded.c_str( ) ) * 1024 * 1024 * 1024;
						if( !cstrMinusDownloaded.empty( ) )
							iMinusDownloaded = UTIL_StringTo64( cstrMinusDownloaded.c_str( ) ) * 1024 * 1024 * 1024;
						if( !cstrMinusBonus.empty( ) )
							iMinusBonus = UTIL_StringTo64( cstrMinusBonus.c_str( ) ) * 100;
						
						iOldUploaded -= iMinusUploaded;
						iOldDownloaded -= iMinusDownloaded;
						iOldBonus -= iMinusBonus;
						
						if( iOldUploaded >= 0 && iOldDownloaded >= 0 && iOldBonus >= 0 )
						{
							UTIL_LogFilePrint( "editUser: %s edit user %s start\n", pRequest->user.strLogin.c_str( ), cstrUser.c_str( ) );
							UTIL_LogFilePrint( "editUser: Minus Uploaded: From %s To %s\n", vecQuery[3].c_str( ), CAtomLong( iOldUploaded ).toString( ).c_str( ) );
							UTIL_LogFilePrint( "editUser: Minus Downloaded: From %s To %s\n", vecQuery[4].c_str( ), CAtomLong( iOldDownloaded ).toString( ).c_str( ) );
							UTIL_LogFilePrint( "editUser: Minus Bonus: From %s To %s\n", vecQuery[5].c_str( ), CAtomLong( iOldBonus ).toString( ).c_str( ) );
						
//							m_pCache->setUserData( cstrUID, -iMinusUploaded, -iMinusDownloaded, -iMinusBonus );
							
							CMySQLQuery mq01( "UPDATE users SET buploaded=" + CAtomLong( iOldUploaded ).toString( ) + ",bdownloaded=" + CAtomLong( iOldDownloaded ).toString( ) + ",bbonus=" + CAtomLong( iOldBonus ).toString( ) + " WHERE buid=" + cstrUID );

							UTIL_LogFilePrint( "editUser: %s edited user %s complete\n", pRequest->user.strLogin.c_str( ), cstrUser.c_str( ) );

							// Inform the operator that the user was modified
							pResponse->strContent += "<p class=\"created_message_users\">" + UTIL_Xsprintf( gmapLANG_CFG["users_edited_user"].c_str( ), cstrUser.c_str( ) , string( "<a title=\"" + gmapLANG_CFG["navbar_users"] + "\" href=\"" + RESPONSE_STR_USERS_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n\n" ;

							// Output common HTML tail
							HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_USERS ) );

							return;
						}

					}
				}
				else if( cstrOK == "2" )
				{
					bool bNum = true;
					for( int i = 0; i < cstrAddUploaded.length( ) && bNum ; i++ )
						if( !isdigit( cstrAddUploaded[i] ) )
							bNum  = false;
					for( int i = 0; i < cstrAddDownloaded.length( ) && bNum ; i++ )
						if( !isdigit( cstrAddDownloaded[i] ) )
							bNum  = false;
					for( int i = 0; i < cstrAddBonus.length( ) && bNum ; i++ )
						if( !isdigit( cstrAddBonus[i] ) )
							bNum  = false;
					if( bNum )
					{
						int64 iOldUploaded = 0, iOldDownloaded = 0, iOldBonus = 0;
						int64 iAddUploaded = 0, iAddDownloaded = 0, iAddBonus = 0;
						if( !vecQuery[3].empty( ) )
							iOldUploaded = UTIL_StringTo64( vecQuery[3].c_str( ) );
						if( !vecQuery[4].empty( ) )
							iOldDownloaded = UTIL_StringTo64( vecQuery[4].c_str( ) );
						if( !vecQuery[5].empty( ) )
							iOldBonus = UTIL_StringTo64( vecQuery[5].c_str( ) );
						
						if( !cstrAddUploaded.empty( ) )
							iAddUploaded = UTIL_StringTo64( cstrAddUploaded.c_str( ) ) * 1024 * 1024 * 1024;
						if( !cstrAddDownloaded.empty( ) )
							iAddDownloaded = UTIL_StringTo64( cstrAddDownloaded.c_str( ) ) * 1024 * 1024 * 1024;
						if( !cstrAddBonus.empty( ) )
							iAddBonus = UTIL_StringTo64( cstrAddBonus.c_str( ) ) * 100;
						
						iOldUploaded += iAddUploaded;
						iOldDownloaded += iAddDownloaded;
						iOldBonus += iAddBonus;
						
						UTIL_LogFilePrint( "editUser: %s edit user %s start\n", pRequest->user.strLogin.c_str( ), cstrUser.c_str( ) );
						UTIL_LogFilePrint( "editUser: Add Uploaded: From %s To %s\n", vecQuery[3].c_str( ), CAtomLong( iOldUploaded ).toString( ).c_str( ) );
						UTIL_LogFilePrint( "editUser: Add Downloaded: From %s To %s\n", vecQuery[4].c_str( ), CAtomLong( iOldDownloaded ).toString( ).c_str( ) );
						UTIL_LogFilePrint( "editUser: Add Bonus: From %s To %s\n", vecQuery[5].c_str( ), CAtomLong( iOldBonus ).toString( ).c_str( ) );
//						m_pCache->setUserData( cstrUID, iAddUploaded, iAddDownloaded, iAddBonus );
						CMySQLQuery mq01( "UPDATE users SET buploaded=" + CAtomLong( iOldUploaded ).toString( ) + ",bdownloaded=" + CAtomLong( iOldDownloaded ).toString( ) + ",bbonus=" + CAtomLong( iOldBonus ).toString( ) + " WHERE buid=" + cstrUID );
					
						UTIL_LogFilePrint( "editUser: %s edited user %s complete\n", pRequest->user.strLogin.c_str( ), cstrUser.c_str( ) );

						// Inform the operator that the user was modified
						pResponse->strContent += "<p class=\"created_message_users\">" + UTIL_Xsprintf( gmapLANG_CFG["users_edited_user"].c_str( ), cstrUser.c_str( ) , string( "<a title=\"" + gmapLANG_CFG["navbar_users"] + "\" href=\"" + RESPONSE_STR_USERS_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n\n" ;

						// Output common HTML tail
						HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_USERS ) );

						return;

					}
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
	}
	else
	{
		// Not authorised

		// Output common HTML head
		HTML_Common_Begin( pRequest, pResponse, gmapLANG_CFG["users_page"], string( CSS_USERS ), string( ), NOT_INDEX, CODE_401 );

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_USERS ) );
	}
}

void CTracker :: serverResponseRank( struct request_t *pRequest, struct response_t *pResponse )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
		if( IsIPBanned( pRequest, pResponse, btv, gmapLANG_CFG["rank_page"], string( CSS_RANK ), NOT_INDEX ) )
			return;

	if( !pRequest->user.strUID.empty( ) && ( pRequest->user.ucAccess & m_ucAccessView ) )
	{
		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["rank_page"], string( CSS_RANK ), string( ), NOT_INDEX, CODE_200 );

		//
		// user table
		//

		unsigned long culKeySize = 0;
		
		struct user_t *pUsersT = 0;
		
		if( m_pCache )
		{
//			m_pCache->ResetUsers( );
			pUsersT = m_pCache->getCacheUsers( );
			culKeySize = m_pCache->getSizeUsers( );
		}
		
		// Populate the users structure for display

		// Sort the users
		const string cstrSort( pRequest->mapParams["sort"] );
		unsigned char ucSort = SORTU_DUPPED;
		
		if( !cstrSort.empty( ) )
		{
			if( cstrSort == "up" )
				ucSort = SORTU_DUPPED;
			else if( cstrSort == "down" )
				ucSort = SORTU_DDOWNED;
			else if( cstrSort == "share" )
				ucSort = SORTU_DSHARERATIO;
			else if( cstrSort == "bonus" )
				ucSort = SORTU_DBONUS;
		}

		m_pCache->sortUsers( ucSort );

		pResponse->strContent += "<p class=\"subfilter\">";
		pResponse->strContent += "<a";
		if( cstrSort.empty( ) || cstrSort == "up" )
			pResponse->strContent += " class=\"blue\"";
		pResponse->strContent += " href=\"" + RESPONSE_STR_RANK_HTML + "?sort=up\">" + gmapLANG_CFG["rank_sort_up"] + "</a>";
		pResponse->strContent += "<span class=\"pipe\"> | </span>";
		pResponse->strContent += "<a";
		if( cstrSort == "down" )
			pResponse->strContent += " class=\"blue\"";
		pResponse->strContent += " href=\"" + RESPONSE_STR_RANK_HTML + "?sort=down\">" + gmapLANG_CFG["rank_sort_down"] + "</a>";
		pResponse->strContent += "<span class=\"pipe\"> | </span>";
		pResponse->strContent += "<a";
		if( cstrSort == "share" )
			pResponse->strContent += " class=\"blue\"";
		pResponse->strContent += " href=\"" + RESPONSE_STR_RANK_HTML + "?sort=share\">" + gmapLANG_CFG["rank_sort_share"] + "</a>";
		pResponse->strContent += "<span class=\"pipe\"> | </span>";
		pResponse->strContent += "<a";
		if( cstrSort == "bonus" )
			pResponse->strContent += " class=\"blue\"";
		pResponse->strContent += " href=\"" + RESPONSE_STR_RANK_HTML + "?sort=bonus\">" + gmapLANG_CFG["rank_sort_bonus"] + "</a>";
		pResponse->strContent += "</p>";

		bool bFound = false;
		bool bSelf = false;

		const unsigned long culCount = 2 * m_uiPerPage;
		unsigned long ulFound = 0;
		
		char szFloat[16];
		memset( szFloat, 0, sizeof( szFloat ) / sizeof( char ) );

		// for correct page numbers after searching

		for( unsigned long ulKey = 0; ulKey < culKeySize && !( ulFound >= culCount && bSelf ); ulKey++ )
		{
			// create the table and display the headers first and once
			if( !bFound )
			{
				pResponse->strContent += "<div class=\"users_table\">\n";
				pResponse->strContent += "<table summary=\"users\">\n";
				pResponse->strContent += "<tr><th class=\"number\">" + gmapLANG_CFG["rank"];
				pResponse->strContent += "</th><th class=\"uploader\">" + gmapLANG_CFG["user_name"];
				pResponse->strContent += "</th>\n<th class=\"number\">" + gmapLANG_CFG["share_ratio"];
				pResponse->strContent += "</th>\n<th class=\"bytes\">" + gmapLANG_CFG["user_uploaded"];
				pResponse->strContent += "</th>\n<th class=\"bytes\">" + gmapLANG_CFG["user_downloaded"];
				pResponse->strContent += "</th>\n<th class=\"number\">" + gmapLANG_CFG["user_bonus"];
				pResponse->strContent += "</th>\n";
				pResponse->strContent += "</tr>\n";

				// signal table created and headers ouput once
				bFound = true;
			}

			if( ucSort == SORTU_DSHARERATIO && pUsersT[ulKey].ulDownloaded < (int64)10*1024*1024*1024 )
				continue;

			ulFound++;

			if( ulFound <= culCount || pUsersT[ulKey].strUID == pRequest->user.strUID )
			{
				if( pUsersT[ulKey].strUID == pRequest->user.strUID )
					bSelf = true;

				// output table rows

				if( pUsersT[ulKey].strUID == pRequest->user.strUID )
					pResponse->strContent += "<tr class=\"own\">\n";
				else if( ulFound % 2 )
					pResponse->strContent += "<tr class=\"even\">\n";
				else
					pResponse->strContent += "<tr class=\"odd\">\n";

				pResponse->strContent += "<td class=\"number\">";
				pResponse->strContent += CAtomLong( ulFound ).toString( );
				pResponse->strContent += "</td><td class=\"uploader\">";
				pResponse->strContent += getUserLink( pUsersT[ulKey].strUID, pUsersT[ulKey].strLogin );
				pResponse->strContent += "</td>\n<td class=\"number\">";
				if( ( -1.001 < pUsersT[ulKey].flShareRatio ) && ( pUsersT[ulKey].flShareRatio < -0.999 ) )
					pResponse->strContent += gmapLANG_CFG["perfect"];
				else
				{
					snprintf( szFloat, sizeof( szFloat ) / sizeof( char ), "%0.3f", pUsersT[ulKey].flShareRatio );
					pResponse->strContent += szFloat;
				}
				pResponse->strContent += "</td>\n<td class=\"bytes\">";
				pResponse->strContent += UTIL_BytesToString( pUsersT[ulKey].ulUploaded );
				pResponse->strContent += "</td>\n<td class=\"bytes\">";
				pResponse->strContent += UTIL_BytesToString( pUsersT[ulKey].ulDownloaded );
				pResponse->strContent += "</td>\n<td class=\"number\">";
				pResponse->strContent += CAtomLong( pUsersT[ulKey].ulBonus / 100 ).toString( ) + "." + CAtomInt( ( pUsersT[ulKey].ulBonus % 100 ) / 10 ).toString( ) + CAtomInt( pUsersT[ulKey].ulBonus % 10 ).toString( );
				pResponse->strContent += "</td>\n";
				pResponse->strContent += "</tr>\n";

			}
		}

		// some finishing touches

		if( bFound )
		{
			pResponse->strContent += "</table>\n";
			pResponse->strContent += "</div>\n\n";
		}
		
		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_RANK ) );
	}
	else
	{
		// Not authorised

		// Output common HTML head
		HTML_Common_Begin( pRequest, pResponse, gmapLANG_CFG["rank_page"], string( CSS_RANK ), string( ), NOT_INDEX, CODE_401 );

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_RANK ) );
	}
}

