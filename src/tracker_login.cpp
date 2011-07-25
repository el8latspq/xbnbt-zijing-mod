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
#include "config.h"
#include "html.h"
#include "md5.h"
#include "sort.h"
#include "tracker.h"
#include "util.h"

void CTracker :: serverResponseLoginGET( struct request_t *pRequest, struct response_t *pResponse )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
		if( IsIPBanned( pRequest, pResponse, btv, gmapLANG_CFG["login_page"], string( CSS_LOGIN ), NOT_INDEX ) )
			return;

	// Not authorized, user must supply a login
	if( pRequest->user.strUID.empty( ) )
	{
		const string cstrReturnPage( pRequest->mapParams["return"] );
		
		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["login_page"], string( CSS_LOGIN ), string( ), NOT_INDEX, CODE_200 );
		
		pResponse->strContent += "<div class=\"login\">\n";
		pResponse->strContent += "<table class=\"login\">\n";
		pResponse->strContent += "<form method=\"post\" name=\"login\" action=\"" + RESPONSE_STR_LOGIN_HTML + "\" enctype=\"multipart/form-data\">\n";
		if( !cstrReturnPage.empty( ) )
			pResponse->strContent += "<input id=\"id_return\" name=\"return\" type=hidden value=\"" + cstrReturnPage + "\">\n";
		pResponse->strContent += "<tr class=\"login\">\n";
		pResponse->strContent += "<th class=\"login\" colspan=\"2\">\n" + gmapLANG_CFG["login"] + "</th>\n</tr>\n";
		pResponse->strContent += "<tr class=\"login\">\n";
		pResponse->strContent += "<th class=\"login\">\n" + gmapLANG_CFG["user_name"] + "</th>\n";
		pResponse->strContent += "<td class=\"login\">\n";
		pResponse->strContent += "<input id=\"id_username\" name=\"username\" alt=\"[" + gmapLANG_CFG["user_name"] + "]\" type=text size=24>\n";
		pResponse->strContent += "</td>\n</tr>\n";
		pResponse->strContent += "<tr class=\"login\">\n";
		pResponse->strContent += "<th class=\"login\">\n" + gmapLANG_CFG["password"] + "</th>\n";
		pResponse->strContent += "<td class=\"login\">\n";
		pResponse->strContent += "<input id=\"id_password\" name=\"password\" alt=\"[" + gmapLANG_CFG["password"] + "]\" type=password size=24>\n";
		pResponse->strContent += "</td>\n</tr>\n";
		pResponse->strContent += "<tr class=\"login\">\n";
		pResponse->strContent += "<td class=\"login\" colspan=\"2\">\n";
		pResponse->strContent += "<input id=\"id_expires\" name=\"expires\" type=checkbox> <label for=\"id_expires\">" + gmapLANG_CFG["expires"] + "</label>\n";
		pResponse->strContent += "</td>\n</tr>\n";
		pResponse->strContent += "<tr class=\"login\">\n";
		pResponse->strContent += "<td class=\"login\" colspan=\"2\">\n";

		// Adds Cancel button beside Create User
		pResponse->strContent += "<div class=\"login_button\">\n";

		pResponse->strContent += Button_Submit( "submit_login", string( gmapLANG_CFG["login"] ) );
		pResponse->strContent += Button_Back( "cancel_login", string( gmapLANG_CFG["cancel"] ) );

		pResponse->strContent += "\n</div>\n</td>\n</tr>\n";
		
		pResponse->strContent += "<tr class=\"login\">\n";
		pResponse->strContent += "<td class=\"login\" colspan=\"2\">\n";
		pResponse->strContent += UTIL_Xsprintf( gmapLANG_CFG["recover_forget"].c_str( ), string( "<a href=\"" + RESPONSE_STR_RECOVER_HTML + "\">" ).c_str( ), string( "</a>" ).c_str( ) );
		pResponse->strContent += "</td>\n</tr>\n";

		// finish
		pResponse->strContent += "</form>\n</table>\n</div>\n";
			
		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_LOGIN ) );

		return;
	}

	//
	// User cookies
	//

	// Set the current time
	time_t tNow = time( 0 );

	// Set a future time
	struct tm tmFuture = *gmtime( &tNow );
	tmFuture.tm_mon++;
	mktime( &tmFuture );

	// Set a past time
	struct tm tmPast = *gmtime( &tNow );
	tmPast.tm_mon--;
	mktime( &tmPast );

	char pTime[256];
	memset( pTime, 0, sizeof( pTime ) / sizeof( char ) );

	// Tell the browser not to cache
	pResponse->mapHeaders.insert( pair<string, string>( "Pragma", "No-Cache" ) );

	const string cstrLogout( pRequest->mapParams["logout"] );

	// If the user signs out then expire the cookie, otherwise refresh the cookie
	if( cstrLogout == "1" )
	{
		strftime( pTime, sizeof( char ) * sizeof( pTime ), "%a, %d-%b-%Y %H:%M:%S GMT", &tmPast );

		// Set the users cookie login
		pResponse->mapHeaders.insert( pair<string, string>( "Set-Cookie", "login=\"" + UTIL_StringToEscaped( pRequest->user.strLogin ) + "\"; expires=" + pTime + "; path=/" ) );
		// Set the users cookie uid
		pResponse->mapHeaders.insert( pair<string, string>( "Set-Cookie", "uid=\"" + pRequest->user.strUID + "\"; expires=" + pTime + "; path=/" ) );
		// Set the users cookie password
		pResponse->mapHeaders.insert( pair<string, string>( "Set-Cookie", "md5=\"" + UTIL_StringToEscaped( pRequest->user.strMD5 ) + "\"; expires=" + pTime + "; path=/" ) );
		pResponse->mapHeaders.insert( pair<string, string>( "Set-Cookie", "per_page=\"" + CAtomInt( m_uiPerPage ).toString( )  + "\"; expires=" + pTime + "; path=/" ) );
	}
//	else
//		strftime( pTime, sizeof( char ) * sizeof( pTime ), "%a, %d-%b-%Y %H:%M:%S GMT", &tmFuture );

	// Set the users cookie login
//	pResponse->mapHeaders.insert( pair<string, string>( "Set-Cookie", "login=\"" + UTIL_StringToEscaped( pRequest->user.strLogin ) + "\"; expires=" + pTime + "; path=/" ) );
	// Set the users cookie uid
//	pResponse->mapHeaders.insert( pair<string, string>( "Set-Cookie", "uid=\"" + pRequest->user.strUID + "\"; expires=" + pTime + "; path=/" ) );
	// Set the users cookie password
//	pResponse->mapHeaders.insert( pair<string, string>( "Set-Cookie", "md5=\"" + UTIL_StringToEscaped( pRequest->user.strMD5 ) + "\"; expires=" + pTime + "; path=/" ) );

	// User per page cookies
//	unsigned int uiUserPerPage = 0;

	// Did we receive any per page parameters? Set the users per page cookie details
//	if( pRequest->mapParams["submit_user_per_page_form_button"] == gmapLANG_CFG["submit"] )
//	{
//		uiUserPerPage = (unsigned int)atoi( pRequest->mapParams["user_per_page"].c_str( ) );

//		if( uiUserPerPage < 1 || uiUserPerPage > 65534 )
//			uiUserPerPage = m_uiPerPage;

//		pResponse->mapHeaders.insert( pair<string, string>( "Set-Cookie", "per_page=\"" + CAtomInt( uiUserPerPage ).toString( ) + "\"; expires=" + pTime + "; path=/" ) );
//		
//		if( !pRequest->user.strUID.empty( ) )
//			CMySQLQuery mq01( "UPDATE users_prefs SET bperpage=" + CAtomInt( uiUserPerPage ).toString( ) + " WHERE buid=" + pRequest->user.strUID );

//		return JS_ReturnToPage( pRequest, pResponse, LOGIN_HTML + "?show=preferences" );
//	}
//	else if( pRequest->mapParams["lp_default_perpage"] == "1" )
//	{
//		pResponse->mapHeaders.insert( pair<string, string>( "Set-Cookie", "per_page=\"" + CAtomInt( m_uiPerPage ).toString( ) + "\"; expires=" + pTime + "; path=/" ) );
//		
//		if( !pRequest->user.strUID.empty( ) )
//			CMySQLQuery mq01( "UPDATE users_prefs SET bperpage=" + CAtomInt( m_uiPerPage ).toString( ) + " WHERE buid=" + pRequest->user.strUID );

//		return JS_ReturnToPage( pRequest, pResponse, LOGIN_HTML + "?show=preferences" );
//	}
//	else
//	{
//		if( pRequest->mapCookies["per_page"].empty( ) )
//			uiUserPerPage = m_uiPerPage;
//		else
//			uiUserPerPage = (unsigned int)atoi( pRequest->mapCookies["per_page"].c_str( ) );

//		if( uiUserPerPage < 1 || uiUserPerPage > 65534 )
//			uiUserPerPage = m_uiPerPage;

//		pResponse->mapHeaders.insert( pair<string, string>( "Set-Cookie", "per_page=\"" + CAtomInt( uiUserPerPage ).toString( ) + "\"; expires=" + pTime + "; path=/" ) );
//	}
	
//	if( !pRequest->user.strUID.empty( ) && ( pRequest->user.ucAccess & m_ucAccessBookmark ) )
//	{
//		if( pRequest->mapParams.find( "bookmark" ) != pRequest->mapParams.end( ) )
//		{
//			string strBookmarkID( pRequest->mapParams["bookmark"] );
//			const string strShare( pRequest->mapParams["share"] );
//			
//			if( strBookmarkID.find( " " ) != string :: npos )
//				strBookmarkID.erase( );
//			
//			if( !strBookmarkID.empty( ) )
//			{
//				CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bid FROM allowed WHERE bid=" + strBookmarkID );
//		
//				vector<string> vecQuery;
//		
//				vecQuery.reserve(1);
//
//				vecQuery = pQuery->nextRow( );
//			
//				delete pQuery;
//			
//				if( vecQuery.size( ) == 1 && !vecQuery[0].empty( ) )
//				{
//					CMySQLQuery *pQueryBookmark = new CMySQLQuery( "SELECT buid,bid FROM bookmarks WHERE buid=" + pRequest->user.strUID + " AND bid=" + strBookmarkID );
//		
//					vector<string> vecQueryBookmark;
//		
//					vecQueryBookmark.reserve(2);
//
//					vecQueryBookmark = pQueryBookmark->nextRow( );
//				
//					delete pQueryBookmark;
//				
//					if( vecQueryBookmark.size( ) == 2 )
//					{
//						if( strShare == "0" || strShare == "1" )
//							CMySQLQuery mq01( "UPDATE bookmarks SET bshare=" + strShare + " WHERE buid=" + pRequest->user.strUID + " AND bid=" + strBookmarkID );
//					}
//					else
//					{
//						if( strShare == "0" || strShare == "1" )
//							CMySQLQuery mq01( "INSERT INTO bookmarks (buid,bid,bshare) VALUES(" + pRequest->user.strUID + "," + strBookmarkID + "," + strShare + ")" );
//						else
//							CMySQLQuery mq01( "INSERT INTO bookmarks (buid,bid) VALUES(" + pRequest->user.strUID + "," + strBookmarkID + ")" );
//					}
//
//					// Output common HTML head
//					HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["login_page"], string( CSS_LOGIN ), string( ), NOT_INDEX, CODE_200 );
//
//					// Deleted the torrent
//					pResponse->strContent += "<p class=\"deleted\">" + UTIL_Xsprintf( gmapLANG_CFG["index_bookmark_torrent"].c_str( ), strBookmarkID.c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_login"] + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "?show=bookmarks\">" ).c_str( ), "</a>" ) + "</p>\n";
//
//					// Output common HTML tail
//					HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_LOGIN ) );
//				
//					return;
//				}
//				else
//				{
//					// Output common HTML head
//					HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["login_page"], string( CSS_LOGIN ), string( ), NOT_INDEX, CODE_200 );
//
//					pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["index_invalid_hash"].c_str( ), strBookmarkID.c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_index"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";
//
//					// Output common HTML tail
//					HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_LOGIN ) );
//
//					return;
//				}
//			}
//		}
//	}
//	
//	if( !pRequest->user.strUID.empty( ) && ( pRequest->user.ucAccess & m_ucAccessBookmark ) )
//	{
//		if( pRequest->mapParams.find( "nobookmark" ) != pRequest->mapParams.end( ) )
//		{
//			string strNoBookmarkID( pRequest->mapParams["nobookmark"] );
//			
//			if( strNoBookmarkID.find( " " ) != string :: npos )
//				strNoBookmarkID.erase( );
//			
//			if( !strNoBookmarkID.empty( ) )
//			{
//				CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bid FROM allowed WHERE bid=" + strNoBookmarkID );
//		
//				vector<string> vecQuery;
//		
//				vecQuery.reserve(1);
//
//				vecQuery = pQuery->nextRow( );
//			
//				delete pQuery;
//			
//				if( vecQuery.size( ) == 1 && !vecQuery[0].empty( ) )
//				{
//					CMySQLQuery mq01( "DELETE FROM bookmarks WHERE buid=" + pRequest->user.strUID + " AND bid=" + strNoBookmarkID );
//
//					// Output common HTML head
//					HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["login_page"], string( CSS_LOGIN ), string( ), NOT_INDEX, CODE_200 );
//
//					// Deleted the torrent
//					pResponse->strContent += "<p class=\"deleted\">" + UTIL_Xsprintf( gmapLANG_CFG["index_bookmark_torrent"].c_str( ), strNoBookmarkID.c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_login"] + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "?show=bookmarks\">" ).c_str( ), "</a>" ) + "</p>\n";
//
//					// Output common HTML tail
//					HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_LOGIN ) );
//				
//					return;
//				}
//				else
//				{
//					// Output common HTML head
//					HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["login_page"], string( CSS_LOGIN ), string( ), NOT_INDEX, CODE_200 );
//
//					pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["index_invalid_hash"].c_str( ), strNoBookmarkID.c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_index"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";
//
//					// Output common HTML tail
//					HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_LOGIN ) );
//
//					return;
//				}
//			}
//		}
//	}
//	
//	if( !pRequest->user.strUID.empty( ) && ( pRequest->user.ucAccess & m_ucAccessMessages ) )
//	{
//		if( pRequest->mapParams.find( "friend" ) != pRequest->mapParams.end( ) )
//		{
//			string strFriendID( pRequest->mapParams["friend"] );
//			
//			if( strFriendID.find( " " ) != string :: npos )
//				strFriendID.erase( );
//			
//			if( !strFriendID.empty( ) && strFriendID != pRequest->user.strUID )
//			{
//				CMySQLQuery *pQuery = new CMySQLQuery( "SELECT buid,busername FROM users WHERE buid=" + strFriendID );
//		
//				vector<string> vecQuery;
//		
//				vecQuery.reserve(2);
//
//				vecQuery = pQuery->nextRow( );
//			
//				delete pQuery;
//			
//				if( vecQuery.size( ) == 2 && !vecQuery[0].empty( ) )
//				{
//					CMySQLQuery mq01( "INSERT INTO friends (buid,bfriendid,bfriendname) VALUES(" + pRequest->user.strUID + "," + strFriendID + ",\'" + UTIL_StringToMySQL( vecQuery[1] ) + "\')" );
//				
//					CMySQLQuery *pQueryTalk = new CMySQLQuery( "SELECT bid,bposted FROM talk WHERE buid=" + strFriendID );
//				
//					vector<string> vecQueryTalk;
//
//					vecQueryTalk.reserve(2);
//
//					vecQueryTalk = pQueryTalk->nextRow( );
//				
//					while( vecQueryTalk.size( ) == 2 )
//					{
//						if( !vecQueryTalk[0].empty( ) )
//							CMySQLQuery mq02( "INSERT IGNORE INTO talkhome (buid,bfriendid,btalkid,bposted) VALUES(" + pRequest->user.strUID + "," + strFriendID + "," + vecQueryTalk[0] + ",'" + UTIL_StringToMySQL( vecQueryTalk[1] ) + "')" );
//						
//						vecQueryTalk = pQueryTalk->nextRow( );
//					}
//				
//					delete pQueryTalk;
//					
//					CMySQLQuery *pQueryAllowed = new CMySQLQuery( "SELECT bid,badded FROM allowed WHERE buploaderid=" + strFriendID + " AND badded>NOW()-INTERVAL 1 DAY" );
//				
//					vector<string> vecQueryAllowed;
//
//					vecQueryAllowed.reserve(2);
//
//					vecQueryAllowed = pQueryAllowed->nextRow( );
//				
//					while( vecQueryAllowed.size( ) == 2 )
//					{
//						if( !vecQueryAllowed[0].empty( ) )
//							CMySQLQuery mq01( "INSERT IGNORE INTO talktorrent (buid,bfriendid,btid,bposted) VALUES(" + pRequest->user.strUID + "," + strFriendID + "," + vecQueryAllowed[0] + ",'" + UTIL_StringToMySQL( vecQueryAllowed[1] ) + "')" );
//						
//						vecQueryAllowed = pQueryAllowed->nextRow( );
//					}
//				
//					delete pQueryAllowed;
//
//					// Output common HTML head
//					HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["login_page"], string( CSS_LOGIN ), string( ), NOT_INDEX, CODE_200 );
//
//					// Add friend
//					pResponse->strContent += "<p class=\"deleted\">" + UTIL_Xsprintf( gmapLANG_CFG["friend_done"].c_str( ), strFriendID.c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_login"] + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "?show=friends\">" ).c_str( ), "</a>" ) + "</p>\n";
//
//					// Output common HTML tail
//					HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_LOGIN ) );
//				
//					return;
//				}
//				else
//				{
//					// Output common HTML head
//					HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["login_page"], string( CSS_LOGIN ), string( ), NOT_INDEX, CODE_200 );
//
//					pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["user_not_exist"].c_str( ), strFriendID.c_str( ) ) + "</p>\n";
//
//					// Output common HTML tail
//					HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_LOGIN ) );
//
//					return;
//				}
//			}
//		}
//	}
//	
//	if( !pRequest->user.strUID.empty( ) && pRequest->user.ucAccess & m_ucAccessMessages )
//	{
//		if( pRequest->mapParams.find( "nofriend" ) != pRequest->mapParams.end( ) )
//		{
//			string strNoFriendID( pRequest->mapParams["nofriend"] );
//			
//			if( strNoFriendID.find( " " ) != string :: npos )
//				strNoFriendID.erase( );
//			
//			if( !strNoFriendID.empty( ) )
//			{
//				CMySQLQuery *pQuery = new CMySQLQuery( "SELECT buid FROM users WHERE buid=" + strNoFriendID );
//		
//				vector<string> vecQuery;
//		
//				vecQuery.reserve(1);
//
//				vecQuery = pQuery->nextRow( );
//			
//				delete pQuery;
//			
//				if( vecQuery.size( ) == 1 && !vecQuery[0].empty( ) )
//				{
//					CMySQLQuery mq01( "DELETE FROM friends WHERE buid=" + pRequest->user.strUID + " AND bfriendid=" + strNoFriendID );
//				
//					CMySQLQuery mq02( "DELETE FROM talkhome WHERE buid=" + pRequest->user.strUID + " AND bfriendid=" + strNoFriendID );
//
//					// Output common HTML head
//					HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["login_page"], string( CSS_LOGIN ), string( ), NOT_INDEX, CODE_200 );
//
//					// Remove friend
//					pResponse->strContent += "<p class=\"deleted\">" + UTIL_Xsprintf( gmapLANG_CFG["friend_done"].c_str( ), strNoFriendID.c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_login"] + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "?show=friends\">" ).c_str( ), "</a>" ) + "</p>\n";
//
//					// Output common HTML tail
//					HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_LOGIN ) );
//				
//					return;
//				}
//				else
//				{
//					// Output common HTML head
//					HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["login_page"], string( CSS_LOGIN ), string( ), NOT_INDEX, CODE_200 );
//
//					pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["user_not_exist"].c_str( ), strNoFriendID.c_str( ) ) + "</p>\n";
//
//					// Output common HTML tail
//					HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_LOGIN ) );
//
//					return;
//				}
//			}
//		}
//	}

	// Was a search submited?
	if( pRequest->mapParams["top_submit_search_button"] == gmapLANG_CFG["search"] || pRequest->mapParams["bottom_submit_search_button"] == gmapLANG_CFG["search"] )
	{
		const string cstrSearch( pRequest->mapParams["search"] );
		string cstrUID( pRequest->mapParams["uid"] );
		const string cstrDetailShow( pRequest->mapParams["show"] );
		const string cstrSort( pRequest->mapParams["sort"] );
		const string cstrPerPage( pRequest->mapParams["per_page"] );
		
		string strPageParameters = LOGIN_HTML;
		
		if( cstrUID.find( " " ) != string :: npos )
			cstrUID.erase( );
		
		vector< pair< string, string > > vecParams;
		vecParams.reserve(64);
		
		vecParams.push_back( pair<string, string>( string( "uid" ), cstrUID ) );
		vecParams.push_back( pair<string, string>( string( "show" ), cstrDetailShow ) );
		vecParams.push_back( pair<string, string>( string( "search" ), cstrSearch ) );
		vecParams.push_back( pair<string, string>( string( "sort" ), cstrSort ) );
		vecParams.push_back( pair<string, string>( string( "per_page" ), cstrPerPage ) );
		
		strPageParameters += UTIL_HTMLJoin( vecParams, string( "?" ), string( "&" ), string( "=" ) );

		return JS_ReturnToPage( pRequest, pResponse, strPageParameters );
	}

	if( pRequest->mapParams["top_clear_filter_and_search_button"] == gmapLANG_CFG["clear_filter_search"] || pRequest->mapParams["bottom_clear_filter_and_search_button"] == gmapLANG_CFG["clear_filter_search"] )
	{
		const string cstrSearch( pRequest->mapParams["search"] );
		string cstrUID( pRequest->mapParams["uid"] );
		const string cstrDetailShow( pRequest->mapParams["show"] );
		
		string strPageParameters = LOGIN_HTML;
		
		if( cstrUID.find( " " ) != string :: npos )
			cstrUID.erase( );
		
		vector< pair< string, string > > vecParams;
		vecParams.reserve(64);
		
		vecParams.push_back( pair<string, string>( string( "uid" ), cstrUID ) );
		vecParams.push_back( pair<string, string>( string( "show" ), cstrDetailShow ) );
		vecParams.push_back( pair<string, string>( string( "search" ), cstrSearch ) );
		
		strPageParameters += UTIL_HTMLJoin( vecParams, string( "?" ), string( "&" ), string( "=" ) );
		
		return JS_ReturnToPage( pRequest, pResponse, strPageParameters );
	}

	// Check if the user signed out
	if( cstrLogout == "1" )
	{
		// Output common HTML head
		HTML_Common_Begin( pRequest, pResponse, gmapLANG_CFG["login_page"], string( CSS_LOGIN ), RESPONSE_STR_LOGIN_HTML, NOT_INDEX, CODE_200 );
		pResponse->strContent += "<p class=\"logout\">" + gmapLANG_CFG["logging_out"] + "</p>\n";
	}
	else
	{
		// Output common HTML head
		HTML_Common_Begin( pRequest, pResponse, gmapLANG_CFG["login_page"], string( CSS_LOGIN ), string( ), NOT_INDEX, CODE_200 );
		
		const string strReturnPage( pRequest->mapParams["return"] );
		const string strReturnPageResp( UTIL_StringToEscaped( strReturnPage ) );

		// Can the user delete their own torrents?
		if( m_bDeleteOwnTorrents && ( pRequest->user.ucAccess & m_ucAccessDelOwn ) )
		{
			// Did the user ask to delete a torrent?
			if( pRequest->mapParams.find( "del" ) != pRequest->mapParams.end( ) || pRequest->mapParams.find( "odel" ) != pRequest->mapParams.end( ) )
			{
				string cstrDelID = string( );

				bool bOffer = false;
				
				if( pRequest->mapParams.find( "del" ) != pRequest->mapParams.end( ) )
					cstrDelID = pRequest->mapParams["del"];
				else if( pRequest->mapParams.find( "odel" ) != pRequest->mapParams.end( ) )
				{
					cstrDelID = pRequest->mapParams["odel"];
					bOffer = true;
				}
				if( cstrDelID.find( " " ) != string :: npos )
					cstrDelID.erase( );
				// What did the user reuest?
				string strDatabase = string( );
				if( bOffer )
					strDatabase = "offer";
				else
					strDatabase = "allowed";
				
				CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bid,bfilename,buploaderid FROM " + strDatabase + " WHERE bid=" + cstrDelID );
			
				vector<string> vecQuery;
			
				vecQuery.reserve(3);

				vecQuery = pQuery->nextRow( );
				
				delete pQuery;

				const string cstrOK( pRequest->mapParams["ok"] );
				
				// Did the request contain a hash string?
				if( vecQuery.size( ) == 0 )
				{
					// Unable to delete torrent. The info hash is invalid.
					if( strReturnPage.empty( ) )
						pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["login_invalid_hash"].c_str( ), cstrDelID.c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_login"] + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";
					else
						pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["login_invalid_hash"].c_str( ), cstrDelID.c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_login"] + "\" href=\"" + strReturnPage + "\">" ).c_str( ), "</a>" ) + "</p>\n";

					// Output common HTML tail
					HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_LOGIN ) );

					return;
				}
				else
				{
					if( vecQuery.size( ) == 3 && !vecQuery[0].empty( ) )
					{
						// Is it ok to delete?
						if( cstrOK == "1" )
						{
							if( !vecQuery[2].empty( ) )
							{
							// Did you upload the torrent?
								if( vecQuery[2] != pRequest->user.strUID )
								{
									// Unable to delete torrent. You didn't upload that torrent.
									if( strReturnPage.empty( ) )
										pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["login_not_owned"].c_str( ), cstrDelID.c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_login"] + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";
									else
										pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["login_not_owned"].c_str( ), cstrDelID.c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_login"] + "\" href=\"" + strReturnPage + "\">" ).c_str( ), "</a>" ) + "</p>\n";

									// Output common HTML tail
									HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_LOGIN ) );

									return;
								}
							}

							if( !vecQuery[1].empty( ) )
							{
								// If the archive directory is set and exists, move the torrent there, otherwise delete it
								if( !m_strArchiveDir.empty( ) && UTIL_CheckDir( m_strArchiveDir.c_str( ) ) )
								{
									if( bOffer )
										UTIL_MoveFile( string( m_strOfferDir + vecQuery[1] ).c_str( ), string( m_strArchiveDir + vecQuery[1] ).c_str( ) );
									else
										UTIL_MoveFile( string( m_strAllowedDir + vecQuery[1] ).c_str( ), string( m_strArchiveDir + vecQuery[1] ).c_str( ) );
								}
								else
								{
									if( bOffer )
										UTIL_DeleteFile( string( m_strOfferDir + vecQuery[1] ).c_str( ) );
									else
										UTIL_DeleteFile( string( m_strAllowedDir + vecQuery[1] ).c_str( ) );
								}
							}

							if( !bOffer )
							{
								CMySQLQuery *pQueryUsers = new CMySQLQuery( "SELECT buid FROM bookmarks WHERE bid=" + cstrDelID );
		
								vector<string> vecQueryUsers;
							
								vecQueryUsers.reserve(1);

								vecQueryUsers = pQueryUsers->nextRow( );
								
								while( vecQueryUsers.size( ) == 1 && !vecQueryUsers[0].empty( ) )
								{
									if( !pRequest->user.strUID.empty( ) && vecQuery[2] != vecQueryUsers[0] )
									{
										string strTitle = gmapLANG_CFG["admin_delete_torrent_title"];
										string strMessage = UTIL_Xsprintf( gmapLANG_CFG["admin_delete_torrent_bookmarked"].c_str( ), UTIL_AccessToString( pRequest->user.ucAccess ).c_str( ), pRequest->user.strLogin.c_str( ), vecQuery[1].c_str( ), "" );
										sendMessage( "", "0", vecQueryUsers[0], "127.0.0.1", strTitle, strMessage );
									}

									vecQueryUsers = pQueryUsers->nextRow( );
								}

								delete pQueryUsers;
							}

							if( bOffer )
								UTIL_LogFilePrint( "deleteOffer: %s deleted offer %s\n", pRequest->user.strLogin.c_str( ), vecQuery[1].c_str( ) );
							else
								UTIL_LogFilePrint( "deleteTorrent: %s deleted torrent %s\n", pRequest->user.strLogin.c_str( ), vecQuery[1].c_str( ) );
							// Delete the torrent entry from the databases

							deleteTag( cstrDelID, bOffer );

							// The torrent has been deleted
							if( bOffer )
							{
								if( strReturnPage.empty( ) )
									pResponse->strContent += "<p class=\"deleted\">" + UTIL_Xsprintf( gmapLANG_CFG["offer_deleted_offer"].c_str( ), cstrDelID.c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_offer"] + "\" href=\"" + RESPONSE_STR_OFFER_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";
								else
									pResponse->strContent += "<p class=\"deleted\">" + UTIL_Xsprintf( gmapLANG_CFG["offer_deleted_offer"].c_str( ), cstrDelID.c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_offer"] + "\" href=\"" + strReturnPage + "\">" ).c_str( ), "</a>" ) + "</p>\n";
							}
							else
							{
								if( strReturnPage.empty( ) )
									pResponse->strContent += "<p class=\"deleted\">" + UTIL_Xsprintf( gmapLANG_CFG["login_deleted_torrent"].c_str( ), cstrDelID.c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_login"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";
								else
									pResponse->strContent += "<p class=\"deleted\">" + UTIL_Xsprintf( gmapLANG_CFG["login_deleted_torrent"].c_str( ), cstrDelID.c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_login"] + "\" href=\"" + strReturnPage + "\">" ).c_str( ), "</a>" ) + "</p>\n";
							}

							// Output common HTML tail
							HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_LOGIN ) );

							return;
						}
						else
						{
							// Are you sure you want to delete the torrent?
							if( bOffer )
								pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["delete_offer_q"].c_str( ), cstrDelID.c_str( ) ) + "</p>\n";
							else
								pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["delete_torrent_q"].c_str( ), cstrDelID.c_str( ) ) + "</p>\n";
							if( bOffer )
								pResponse->strContent += "<p class=\"delete\"><a title=\"" + gmapLANG_CFG["yes"] + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "?odel=" + cstrDelID;
							else
								pResponse->strContent += "<p class=\"delete\"><a title=\"" + gmapLANG_CFG["yes"] + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "?del=" + cstrDelID;
							pResponse->strContent += "&amp;ok=1";
							if( !strReturnPage.empty( ) )
								pResponse->strContent += "&amp;return=" + strReturnPageResp;
							pResponse->strContent += "\">" + gmapLANG_CFG["yes"] + "</a>\n";
							pResponse->strContent += "<span class=\"pipe\"> | </span><a title=\"" + gmapLANG_CFG["no"] + "\" href=\"" + string( JS_BACK ) + "\">" + gmapLANG_CFG["no"] + "</a></p>\n";

							// Output common HTML tail
							HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_LOGIN ) );

							return;
						}
					}
					else
					{
						pResponse->strContent += "<p class=\"not_exist\">" + UTIL_Xsprintf( gmapLANG_CFG["torrent_not_exist"].c_str( ), cstrDelID.c_str( ) );
						HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_LOGIN ) );
						return;
					}
				}
			}
		}

		string cstrUID( pRequest->mapParams["uid"] );
		
		if( cstrUID.find( " " ) != string :: npos )
			cstrUID.erase( );
		
		user_t user;

		if( cstrUID.empty( ) )
			user = getUser( pRequest->user.strUID, pRequest->user.strUID, pRequest->user.ucAccess );
		else
			user = getUser( cstrUID, pRequest->user.strUID, pRequest->user.ucAccess );
		
		if( user.strUID.empty( ) )
		{
			pResponse->strContent += "<p class=\"not_exist\">" + UTIL_Xsprintf( gmapLANG_CFG["user_not_exist"].c_str( ), cstrUID.c_str( ) );
			HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_LOGIN ) );
			return;
		}
		
		// Javascript
		pResponse->strContent += "<script type=\"text/javascript\">\n";
		pResponse->strContent += "<!--\n";
		
		pResponse->strContent += "function validate( theform ) {\n";
		pResponse->strContent += "if( theform.trade.value == \"\" ) {\n";
		pResponse->strContent += "  alert( \"" + gmapLANG_CFG["js_fill_fields"] + "\" );\n";
		pResponse->strContent += "  return false; }\n";
		pResponse->strContent += "else { return true; }\n";
		pResponse->strContent += "}\n\n";

		pResponse->strContent += "function default_perpage_confirm( )\n";
		pResponse->strContent += "{\n";
		pResponse->strContent += "var name=confirm(\"" + gmapLANG_CFG["default"] + "?\\n\")\n";
		pResponse->strContent += "if (name==true)\n";
		pResponse->strContent += "{\n";
		pResponse->strContent += "window.location=\"" + RESPONSE_STR_LOGIN_HTML + "?lp_default_perpage=1\"\n";;
		pResponse->strContent += "}\n";
		pResponse->strContent += "}\n\n";
		
		pResponse->strContent += "if (window.XMLHttpRequest)\n";
		pResponse->strContent += "{// code for IE7+, Firefox, Chrome, Opera, Safari\n";
		pResponse->strContent += "  xmlhttp=new XMLHttpRequest(); }\n";
		pResponse->strContent += "else\n";
		pResponse->strContent += "{// code for IE6, IE5\n";
		pResponse->strContent += "  xmlhttp=new ActiveXObject(\"Microsoft.XMLHTTP\"); }\n";
		
		pResponse->strContent += "function xmlparser(text) {\n";
		pResponse->strContent += "  try //Internet Explorer\n";
		pResponse->strContent += "  {\n";
		pResponse->strContent += "    xmlDoc=new ActiveXObject(\"Microsoft.XMLDOM\");\n";
		pResponse->strContent += "    xmlDoc.async=\"false\";\n";
		pResponse->strContent += "    xmlDoc.loadXML(text);\n";
		pResponse->strContent += "  }\n";
		pResponse->strContent += "  catch(e)\n";
		pResponse->strContent += "  {\n";
		pResponse->strContent += "    try //Firefox, Mozilla, Opera, etc.\n";
		pResponse->strContent += "    {\n";
		pResponse->strContent += "      parser=new DOMParser();\n";
		pResponse->strContent += "      xmlDoc=parser.parseFromString(text,\"text/xml\");\n";
		pResponse->strContent += "    }\n";
		pResponse->strContent += "    catch(e) {alert(e.message)}\n";
		pResponse->strContent += "  }\n";
		pResponse->strContent += "  return xmlDoc;\n";
		pResponse->strContent += "}\n";

		// bookmark
		pResponse->strContent += "function bookmark(id,bookmark_link,nobookmark_link) {\n";
		pResponse->strContent += "  var bookmarkLink = document.getElementById( 'bookmark'+id );\n";
		pResponse->strContent += "  var shareLink = document.getElementById( 'share'+id );\n";
		pResponse->strContent += "  xmlhttp.onreadystatechange=function() {\n";
		pResponse->strContent += "    if (xmlhttp.readyState==4 && xmlhttp.status==200) {\n";
		pResponse->strContent += "      var xmldoc = xmlparser(xmlhttp.responseText);\n";
		pResponse->strContent += "      var queryCode = xmldoc.getElementsByTagName('code')[0].childNodes[0].nodeValue;\n";
		pResponse->strContent += "      if( queryCode=='1' || queryCode=='2' || queryCode=='3' ) {\n";
		pResponse->strContent += "        if (bookmarkLink.innerHTML == bookmark_link) {\n";
		pResponse->strContent += "          bookmarkLink.innerHTML = nobookmark_link;\n";
		pResponse->strContent += "          shareLink.style.display = \"\"; }\n";
		pResponse->strContent += "        else {\n";
		pResponse->strContent += "          bookmarkLink.innerHTML = bookmark_link;\n";
		pResponse->strContent += "          shareLink.style.display = \"none\"; }\n";
		pResponse->strContent += "      }\n";
		pResponse->strContent += "    }\n";
		pResponse->strContent += "  }\n";
		pResponse->strContent += "  if (bookmarkLink.innerHTML == bookmark_link) {\n";
		pResponse->strContent += "    xmlhttp.open(\"GET\",\"" + RESPONSE_STR_QUERY_HTML + "?type=bookmark&action=add&id=\" + id,true);\n";
		pResponse->strContent += "    xmlhttp.send(); }\n";
		pResponse->strContent += "  else {\n";
		pResponse->strContent += "    xmlhttp.open(\"GET\",\"" + RESPONSE_STR_QUERY_HTML + "?type=bookmark&action=remove&id=\" + id,true);\n";
		pResponse->strContent += "    xmlhttp.send(); }\n";
		pResponse->strContent += "}\n";
		
		// share
		pResponse->strContent += "function share(id,share_link,noshare_link) {\n";
		pResponse->strContent += "  var shareLink = document.getElementById( 'share'+id );\n";
		pResponse->strContent += "  xmlhttp.onreadystatechange=function() {\n";
		pResponse->strContent += "    if (xmlhttp.readyState==4 && xmlhttp.status==200) {\n";
		pResponse->strContent += "      var xmldoc = xmlparser(xmlhttp.responseText);\n";
		pResponse->strContent += "      var queryCode = xmldoc.getElementsByTagName('code')[0].childNodes[0].nodeValue;\n";
		pResponse->strContent += "      if( queryCode=='1' || queryCode=='4' ) {\n";
		pResponse->strContent += "        if (shareLink.innerHTML == share_link) {\n";
		pResponse->strContent += "          shareLink.innerHTML = noshare_link;\n";
		pResponse->strContent += "          shareLink.className = \"noshare\"; }\n";
		pResponse->strContent += "        else {\n";
		pResponse->strContent += "          shareLink.innerHTML = share_link;\n";
		pResponse->strContent += "          shareLink.className = \"share\"; }\n";
		pResponse->strContent += "      }\n";
		pResponse->strContent += "    }\n";
		pResponse->strContent += "  }\n";
		pResponse->strContent += "  if (shareLink.innerHTML == share_link) {\n";
		pResponse->strContent += "    xmlhttp.open(\"GET\",\"" + RESPONSE_STR_QUERY_HTML + "?type=bookmark&action=share&set=1&id=\" + id,true);\n";
		pResponse->strContent += "    xmlhttp.send(); }\n";
		pResponse->strContent += "  else {\n";
		pResponse->strContent += "    xmlhttp.open(\"GET\",\"" + RESPONSE_STR_QUERY_HTML + "?type=bookmark&action=share&set=0&id=\" + id,true);\n";
		pResponse->strContent += "    xmlhttp.send(); }\n";
		pResponse->strContent += "}\n";
		
		// friend
		pResponse->strContent += "function friend(id,friend_link,nofriend_link) {\n";
		pResponse->strContent += "  var friendLink = document.getElementById( 'friend'+id );\n";
		pResponse->strContent += "  xmlhttp.onreadystatechange=function() {\n";
		pResponse->strContent += "    if (xmlhttp.readyState==4 && xmlhttp.status==200) {\n";
		pResponse->strContent += "      var xmldoc = xmlparser(xmlhttp.responseText);\n";
		pResponse->strContent += "      var queryCode = xmldoc.getElementsByTagName('code')[0].childNodes[0].nodeValue;\n";
		pResponse->strContent += "      if( queryCode=='1' || queryCode=='2' || queryCode=='3' ) {\n";
		pResponse->strContent += "        if (friendLink.innerHTML == friend_link)\n";
		pResponse->strContent += "          friendLink.innerHTML = nofriend_link;\n";
		pResponse->strContent += "        else\n";
		pResponse->strContent += "          friendLink.innerHTML = friend_link;\n";
		pResponse->strContent += "      }\n";
		pResponse->strContent += "    }\n";
		pResponse->strContent += "  }\n";
		pResponse->strContent += "  if (friendLink.innerHTML == friend_link) {\n";
		pResponse->strContent += "    xmlhttp.open(\"GET\",\"" + RESPONSE_STR_QUERY_HTML + "?type=friend&action=add&uid=\" + id,true);\n";
		pResponse->strContent += "    xmlhttp.send(); }\n";
		pResponse->strContent += "  else {\n";
		pResponse->strContent += "    xmlhttp.open(\"GET\",\"" + RESPONSE_STR_QUERY_HTML + "?type=friend&action=remove&uid=\" + id,true);\n";
		pResponse->strContent += "    xmlhttp.send(); }\n";
		pResponse->strContent += "}\n";
		
		// get torrent
		pResponse->strContent += "function get_torrent(talkID,torrentID) {\n";
		pResponse->strContent += "  var divTalk = document.getElementById( talkID );\n";
		pResponse->strContent += "  var divTorrent = document.getElementById( talkID+'_torrent'+torrentID );\n";
		pResponse->strContent += "  var exist = false;\n";
		pResponse->strContent += "  xmlhttp.onreadystatechange=function() {\n";
		pResponse->strContent += "    if (xmlhttp.readyState==4 && xmlhttp.status==200) {\n";
		pResponse->strContent += "      var e = document.createElement('div');\n";
		pResponse->strContent += "      e.innerHTML = xmlhttp.responseText;\n";
		pResponse->strContent += "      var elements = e.getElementsByTagName('div');\n";
		pResponse->strContent += "      for (var i = 0; i < elements.length; i++) {\n";
		pResponse->strContent += "        if (elements[i].id == 'torrent'+torrentID) {\n";
		pResponse->strContent += "          divTalk.innerHTML = divTalk.innerHTML + '<div id=\"' + talkID + '_torrent' + torrentID + '\" class=\"talk_torrent\">' + elements[i].innerHTML + '</div>';\n";
//		pResponse->strContent += "  	      window.location.hash = \"#\";\n";
//		pResponse->strContent += "          document.postatalk.talk.focus();\n";
		pResponse->strContent += "          exist = true;\n";
		pResponse->strContent += "          break; }\n";
		pResponse->strContent += "      }\n";
		pResponse->strContent += "      if(!exist)\n";
		pResponse->strContent += "          divTalk.innerHTML = divTalk.innerHTML + '<div id=\"' + talkID + '_torrent' + torrentID + '\" class=\"talk_torrent\">" + UTIL_Xsprintf( gmapLANG_CFG["torrent_not_exist"].c_str( ), "' + torrentID + '" ) + "</div>';\n";
		pResponse->strContent += "    }\n";
		pResponse->strContent += "  }\n";
		pResponse->strContent += "  if(!divTorrent) {\n";
		pResponse->strContent += "    xmlhttp.open(\"GET\",'" + RESPONSE_STR_TALK_HTML + "?tid=' + torrentID,true);\n";
//		pResponse->strContent += "    xmlhttp.open(\"GET\",'" + RESPONSE_STR_TALK_HTML + "?tid=' + torrentID + '&from=' + talkID,true);\n";
		pResponse->strContent += "    xmlhttp.send(); }\n";
		pResponse->strContent += "  else {\n";
		pResponse->strContent += "    if( divTorrent.style.display == \"none\" )\n";
		pResponse->strContent += "      divTorrent.style.display = \"\";\n";
		pResponse->strContent += "    else\n";
		pResponse->strContent += "      divTorrent.style.display = \"none\"; }\n";
		pResponse->strContent += "}\n\n";

//			pResponse->strContent += "function clear_search_and_filters( ) {\n";
//			pResponse->strContent += "window.location=\"" + RESPONSE_STR_LOGIN_HTML + "?uid=" + user.strUID + "&amp;show=torrents" + "\"\n";
//			pResponse->strContent += "}\n\n";

		pResponse->strContent += "//-->\n";
		pResponse->strContent += "</script>\n\n";
		
		pResponse->strContent += "<table class=\"user_main\">\n";
		pResponse->strContent += "<tr class=\"user_main\">\n";
		
		pResponse->strContent += "<td class=\"user_main_table\">";
		
		bool bShareRatioWarned = checkShareRatio( user.ulDownloaded, user.flShareRatio );
		
		pResponse->strContent += "<h3>" + UTIL_RemoveHTML( user.strLogin );
		if( !( user.ucAccess & ACCESS_VIEW ) )
			pResponse->strContent += "<img title=\"" + gmapLANG_CFG["user_banned"] + "\" src=\"files/warned1.gif\">";
		if( m_bRatioRestrict && bShareRatioWarned )
			pResponse->strContent += "<img title=\"" + gmapLANG_CFG["user_shareratio_warned"] + "\" src=\"files/warned3.gif\">";
		if( user.tWarned > 0 )
			pResponse->strContent += "<img title=\"" + gmapLANG_CFG["user_warned"] + "\" src=\"files/warned.gif\">";

		if( pRequest->user.strUID != user.strUID )
		{
			pResponse->strContent += "<span class=\"user_friend\">[<a class=\"friend\" id=\"friend" + user.strUID + "\" class=\"red\" href=\"javascript: ;\" onclick=\"javascript: friend('" + user.strUID + "','" + gmapLANG_CFG["friend_add"] + "','" + gmapLANG_CFG["friend_remove"] + "');\">";
			
			CMySQLQuery *pQueryFriend = new CMySQLQuery( "SELECT buid,bfriendid FROM friends WHERE buid=" + pRequest->user.strUID + " AND bfriendid=" + user.strUID );

			vector<string> vecQueryFriend;

			vecQueryFriend.reserve(2);

			vecQueryFriend = pQueryFriend->nextRow( );
			
			delete pQueryFriend;
			
			if( vecQueryFriend.size( ) == 2 && !vecQueryFriend[0].empty( ) )
				pResponse->strContent += gmapLANG_CFG["friend_remove"];
			else
				pResponse->strContent += gmapLANG_CFG["friend_add"];
				
			pResponse->strContent += "</a>]</span>";
		}

		pResponse->strContent += "<br><span>[<a class=\"black\" title=\"" + gmapLANG_CFG["talk_my_talk"] + "\" href=\"" + RESPONSE_STR_TALK_HTML + "?uid=" + user.strUID + "\">" + gmapLANG_CFG["talk_my_talk"] + "</a>]</span>";

		if( pRequest->user.ucAccess & m_ucAccessComments )
		{
			if( pRequest->user.strUID != user.strUID )
				pResponse->strContent += "<span class=\"user_talk\">[<a class=\"black\" title=\"" + gmapLANG_CFG["talk_to"] + "\" href=\"" + RESPONSE_STR_TALK_HTML + "?talk=" + UTIL_StringToEscaped( "@" + user.strLogin + " " ) + "\">" + gmapLANG_CFG["talk_to"] + "</a>]</span>";
		}
		
		if( pRequest->user.ucAccess & m_ucAccessMessages && pRequest->user.strUID != user.strUID )
		{
			pResponse->strContent += "<span class=\"user_message\">[<a class=\"black\" title=\"" + gmapLANG_CFG["messages_send_message"] + "\" href=\"" + RESPONSE_STR_MESSAGES_HTML + "?sendto=" + user.strUID + "\">" + gmapLANG_CFG["messages_send_message"] + "</a>]</span>";
		}
			
		pResponse->strContent += "</h3>";
		
		pResponse->strContent += "<table class=\"user_detail\">\n<tr class=\"user_detail\">\n";
		if( ( pRequest->user.ucAccess & m_ucAccessEditUsers && ( user.ucAccess < m_ucAccessEditUsers || pRequest->user.ucAccess & m_ucAccessEditAdmins ) ) || pRequest->user.strUID == user.strUID )
			pResponse->strContent += "<td class=\"user_detail\"><a href=\"" + RESPONSE_STR_USERS_HTML + "?uid=" + user.strUID + "&amp;action=edit\">" + gmapLANG_CFG["user_detail_edit"] + "</a></td>\n\n";
		if( pRequest->user.strUID == user.strUID )
		{
			if( CFG_GetInt( "bnbt_bonus_trade_enable", 0 ) == 0 ? false : true )
				pResponse->strContent += "<td class=\"user_detail\"><a class=\"user_detail\" href=\"" + RESPONSE_STR_LOGIN_HTML + "?show=bonus\">" + gmapLANG_CFG["user_detail_bonus"] + "</td>\n";
			pResponse->strContent += "<td class=\"user_detail\"><a class=\"user_detail\" href=\"" + RESPONSE_STR_LOGIN_HTML + "?show=preferences\">" + gmapLANG_CFG["user_detail_preferences"] + "</td>\n";
		}
		pResponse->strContent += "</tr></table>\n";
		
		CMySQLQuery *pQueryTalk = new CMySQLQuery( "SELECT bid,busername,buid,bposted,btalk,btalkstore,breply,breply_real,breplyto,breplytoid,breplytimes,brt,brtto,brttoid FROM talk WHERE buid=" + user.strUID + " AND breply=0 ORDER BY bposted DESC LIMIT 1" );
		
		vector<string> vecQueryTalk;

		vecQueryTalk.reserve(14);

		vecQueryTalk = pQueryTalk->nextRow( );
		
		if( vecQueryTalk.size( ) == 14 )
		{
			pResponse->strContent += "<div class=\"user_main_talk\">";
			pResponse->strContent += "<table class=\"user_main_talk\" summary=\"talk\">\n";
		
			while( vecQueryTalk.size( ) == 14 )
			{
				pResponse->strContent += GenerateTalk( vecQueryTalk, user.ucAccess, string( ), user.strUID, string( ), true, false );

				vecQueryTalk = pQueryTalk->nextRow( );
			}
			
			pResponse->strContent += "</table>\n";
			pResponse->strContent += "</div>\n";
		}
		
		delete pQueryTalk;

		pResponse->strContent += "<div class=\"user_table\">\n";
		pResponse->strContent += "<table summary=\"user\">\n";

		pResponse->strContent += "<tr class=\"user_table\"><th class=\"user_table\">" + gmapLANG_CFG["created"] + "</th><td class=\"user_table\">" + user.strCreated + "</td></tr>";
		if( ( ( pRequest->user.ucAccess & m_ucAccessUserDetails ) || pRequest->user.strUID == user.strUID ) && !user.strInviter.empty( ) )
		{
			pResponse->strContent += "<tr class=\"user_table\"><th class=\"user_table\">" + gmapLANG_CFG["inviter"] + "</th><td class=\"user_table\">";
			pResponse->strContent += getUserLink( user.strInviterID, user.strInviter );
			pResponse->strContent += "</td></tr>";
		}
		if( ( ( pRequest->user.ucAccess & m_ucAccessUserDetails ) || pRequest->user.strUID == user.strUID ) && !user.strInvites.empty( ) )
			pResponse->strContent += "<tr class=\"user_table\"><th class=\"user_table\">" + gmapLANG_CFG["invites"] + "</th><td class=\"user_table\">" + user.strInvites + "</td></tr>";
		if( user.tLast )
		{
			char pTime[256];
			memset( pTime, 0, sizeof( pTime ) / sizeof( char ) );
			strftime( pTime, sizeof( pTime ) / sizeof( char ), "%Y-%m-%d %H:%M:%S", localtime( &user.tLast ) );
			pResponse->strContent += "<tr class=\"user_table\"><th class=\"user_table\">" + gmapLANG_CFG["user_last"] + "</th><td class=\"user_table\">" + pTime + "</td></tr>";
		}
		else
			pResponse->strContent += "<tr class=\"user_table\"><th class=\"user_table\">" + gmapLANG_CFG["user_last"] + "</th><td class=\"user_table\">" + gmapLANG_CFG["na"] + "</td></tr>";
		if( ( pRequest->user.ucAccess & m_ucAccessUserDetails ) || pRequest->user.strUID == user.strUID )
		{
			if( !user.strIP.empty( ) )
				pResponse->strContent += "<tr class=\"user_table\"><th class=\"user_table\">" + gmapLANG_CFG["user_ip"] + "</th><td class=\"user_table\">" + user.strIP + "</td></tr>";
			else
				pResponse->strContent += "<tr class=\"user_table\"><th class=\"user_table\">" + gmapLANG_CFG["user_ip"] + "</th><td class=\"user_table\">" + gmapLANG_CFG["unknown"] + "</td></tr>";
		}
		if( ( pRequest->user.ucAccess & m_ucAccessUserDetails ) || pRequest->user.strUID == user.strUID )
		{
			if( user.tWarned )
			{
				char pTime[256];
				memset( pTime, 0, sizeof( pTime ) / sizeof( char ) );
				strftime( pTime, sizeof( pTime ) / sizeof( char ), "%Y-%m-%d %H:%M:%S", localtime( &user.tWarned ) );
				pResponse->strContent += "<tr class=\"user_table\"><th class=\"user_table\">" + gmapLANG_CFG["warned"] + "</th><td class=\"user_table\">" + pTime + "</td></tr>";
			}
			else
				pResponse->strContent += "<tr class=\"user_table\"><th class=\"user_table\">" + gmapLANG_CFG["warned"] + "</th><td class=\"user_table\">" + gmapLANG_CFG["na"] + "</td></tr>";
		}
		pResponse->strContent += "<tr class=\"user_table\"><th class=\"user_table\">" + gmapLANG_CFG["access"] + "</th><td class=\"user_table\">";
		pResponse->strContent += UTIL_AccessToString( user.ucAccess );
		
		string strClass = UTIL_UserClass( user.ucAccess, user.ucGroup );
		if( m_bRatioRestrict && bShareRatioWarned && strClass == gmapLANG_CFG["class_member"] )
			strClass = "share_warned";
		
		if( user.ucGroup == 0 && !user.strTitle.empty( ) )
			pResponse->strContent += " <span class=\"" + strClass + "\">" + user.strTitle + "</span>";
		pResponse->strContent += "</td></tr>";
		pResponse->strContent += "<tr class=\"user_table\"><th class=\"user_table\">" + gmapLANG_CFG["group"] + "</th><td class=\"user_table\">";
		
		pResponse->strContent += UTIL_GroupToString( user.ucGroup );
		if( user.ucGroup && !user.strTitle.empty( ) )
			pResponse->strContent += " <span class=\"" + strClass + "\">" + user.strTitle + "</span>";
//				else
//					pResponse->strContent += gmapLANG_CFG["group_none"];
		pResponse->strContent += "</td></tr>";
		
		if( ( ( pRequest->user.ucAccess & m_ucAccessUserDetails ) || pRequest->user.strUID == user.strUID ) && !user.strPasskey.empty( ) )
		{
			pResponse->strContent += "<tr class=\"user_table\"><th class=\"user_table\">" + gmapLANG_CFG["passkey"] + "</th><td class=\"user_table\">" + user.strPasskey;
			pResponse->strContent += " [<a href=\"" + RESPONSE_STR_USERS_HTML + "?uid=" + user.strUID + "&amp;action=reset\">" + gmapLANG_CFG["reset"]+ "</a>]</td></tr>";
		}
		
		if( ( -1.001 < user.flShareRatio ) && ( user.flShareRatio < -0.999 ) )
			pResponse->strContent += "<tr class=\"user_table\"><th class=\"user_table\">" + gmapLANG_CFG["share_ratio"] + "</th><td class=\"user_table\">" + gmapLANG_CFG["perfect"] + "</td></tr>";
		else
		{
			char szFloat[16];
			memset( szFloat, 0, sizeof( szFloat ) / sizeof( char ) );
			snprintf( szFloat, sizeof( szFloat ) / sizeof( char ), "%0.3f", user.flShareRatio );
			pResponse->strContent += "<tr class=\"user_table\"><th class=\"user_table\">" + gmapLANG_CFG["share_ratio"] + "</th><td class=\"user_table\">" + szFloat + "</td></tr>";
		}
		pResponse->strContent += "<tr class=\"user_table\"><th class=\"user_table\">" + gmapLANG_CFG["user_uploaded"] + "</th><td class=\"user_table\">" + UTIL_BytesToString( user.ulUploaded ) + "</td></tr>";
		pResponse->strContent += "<tr class=\"user_table\"><th class=\"user_table\">" + gmapLANG_CFG["user_downloaded"] + "</th><td class=\"user_table\">" + UTIL_BytesToString( user.ulDownloaded ) + "</td></tr>";
		pResponse->strContent += "<tr class=\"user_table\"><th class=\"user_table\">" + gmapLANG_CFG["user_bonus"] + "</th><td class=\"user_table\">\n";
		pResponse->strContent += CAtomLong( user.ulBonus / 100 ).toString( ) + "." + CAtomInt( ( user.ulBonus % 100 ) / 10 ).toString( ) + CAtomInt( user.ulBonus % 10 ).toString( );
		pResponse->strContent += "</td></tr>";

		if( user.flSeedBonus > -0.999 )
		{
			char szFloat[16];
			memset( szFloat, 0, sizeof( szFloat ) / sizeof( char ) );
			snprintf( szFloat, sizeof( szFloat ) / sizeof( char ), "%0.2f", user.flSeedBonus );
// 					if( !user.strSeedBonus.empty( ) )
				pResponse->strContent += "<tr class=\"user_table\"><th class=\"user_table\">" + gmapLANG_CFG["user_seed_bonus"] + gmapLANG_CFG["per_hour"] + "</th><td class=\"user_table\">" + szFloat + "</td></tr>";
// 					else
// 						pResponse->strContent += "<tr class=\"user_table\"><th class=\"user_table\">" + gmapLANG_CFG["user_seed_bonus"] + "</th><td class=\"user_table\">" + gmapLANG_CFG["na"] + "</td></tr>";
		}

		if( ( ( pRequest->user.ucAccess & m_ucAccessUserDetails ) || pRequest->user.strUID == user.strUID ) && !user.strMail.empty( ) )
			pResponse->strContent += "<tr class=\"user_table\"><th class=\"user_table\">" + gmapLANG_CFG["email"] + "</th><td class=\"user_table\">" + UTIL_RemoveHTML( user.strMail ) + "</td></tr>";
		pResponse->strContent += "</table></div>\n";
		
		pResponse->strContent += "</td>\n";
		
		pResponse->strContent += "<td class=\"user_main_detail\">";
		
		pResponse->strContent += "<table class=\"user_detail\">\n<tr class=\"user_detail\">\n";
//		pResponse->strContent += "<td class=\"user_detail\"><a class=\"user_detail\" href=\"" + RESPONSE_STR_LOGIN_HTML + "?uid=" + user.strUID + "\">" + gmapLANG_CFG["user_detail_main"] + "</td>\n";
		
		pResponse->strContent += "<td class=\"user_detail\"><a class=\"user_detail\" href=\"" + RESPONSE_STR_LOGIN_HTML + "?uid=" + user.strUID + "&amp;show=bookmarks\">";
		if( pRequest->user.strUID == user.strUID )
			pResponse->strContent += gmapLANG_CFG["user_detail_bookmarks"];
		else
			pResponse->strContent += gmapLANG_CFG["user_detail_shares"];
		pResponse->strContent += "</td>\n";
		
		pResponse->strContent += "<td class=\"user_detail\"><a class=\"user_detail\" href=\"" + RESPONSE_STR_LOGIN_HTML + "?uid=" + user.strUID + "&amp;show=comments\">" + gmapLANG_CFG["user_detail_comments"] + "</td>\n";
		
		pResponse->strContent += "<td class=\"user_detail\"><a class=\"user_detail\" href=\"" + RESPONSE_STR_LOGIN_HTML + "?uid=" + user.strUID + "&amp;show=torrents\">" + gmapLANG_CFG["user_detail_torrents"] + "</td>\n";
		
		if( ( pRequest->user.ucAccess & m_ucAccessUserDetails ) || pRequest->user.strUID == user.strUID )
		{
			pResponse->strContent += "<td class=\"user_detail\"><a class=\"user_detail\" href=\"" + RESPONSE_STR_LOGIN_HTML + "?uid=" + user.strUID + "&amp;show=active\">" + gmapLANG_CFG["user_detail_active"] + "</td>\n";
			pResponse->strContent += "<td class=\"user_detail\"><a class=\"user_detail\" href=\"" + RESPONSE_STR_LOGIN_HTML + "?uid=" + user.strUID + "&amp;show=completed\">" + gmapLANG_CFG["user_detail_completed"] + "</td>\n";
		}
		
		if( pRequest->user.strUID == user.strUID )
		{
			pResponse->strContent += "<td class=\"user_detail\"><a class=\"user_detail\" href=\"" + RESPONSE_STR_LOGIN_HTML + "?show=friends\">" + gmapLANG_CFG["user_detail_friends"] + "</td>\n";
			pResponse->strContent += "<td class=\"user_detail\"><a class=\"user_detail\" href=\"" + RESPONSE_STR_LOGIN_HTML + "?show=friendeds\">" + gmapLANG_CFG["user_detail_friendeds"] + "</td>\n";
		}

		if( ( pRequest->user.ucAccess & m_ucAccessAdmin ) || pRequest->user.strUID == user.strUID )
		{
			pResponse->strContent += "<td class=\"user_detail\"><a class=\"user_detail\" href=\"" + RESPONSE_STR_LOGIN_HTML + "?uid=" + user.strUID + "&amp;show=invites\">" + gmapLANG_CFG["user_detail_invites"] + "</td>\n";
		}
		
		pResponse->strContent += "</tr></table>\n<p>\n";
		
		const string cstrDetailShow( pRequest->mapParams["show"] );
		
		if( cstrDetailShow == "preferences" )
		{
			if( pRequest->user.strUID == user.strUID && ( pRequest->user.ucAccess & m_ucAccessView ) )
			{
				CMySQLQuery *pQueryPrefs = new CMySQLQuery( "SELECT bdefaulttag,baddedpassed,bperpage,bsavesent,bmsgcomment,bmsgcommentbm FROM users_prefs WHERE buid=" + user.strUID );
				
				map<string, string> mapPrefs;

				mapPrefs = pQueryPrefs->nextRowMap( );
				
				delete pQueryPrefs;
				
				// User per page cookie
				pResponse->strContent += "<form method=\"post\" class=\"preferences\" name=\"preferences\" action=\"" + RESPONSE_STR_LOGIN_HTML + "\" enctype=\"multipart/form-data\">\n";
				pResponse->strContent += "<table class=\"preferences\">\n";
				pResponse->strContent += "<tr class=\"preferences\">\n<th class=\"preferences\">" + gmapLANG_CFG["prefs_torrents"] + "</th>";
				pResponse->strContent += "<td class=\"preferences\">";

				if( !m_vecTags.empty( ) )
				{
					pResponse->strContent += gmapLANG_CFG["prefs_default_tag"];
					pResponse->strContent += "<table class=\"index_filter\">\n";
					pResponse->strContent += "<tr class=\"index_filter\">\n";
					
					string strFilter( mapPrefs["bdefaulttag"] );
					
					vector<string> vecFilter;
					vecFilter.reserve(64);
					
					vecFilter = UTIL_SplitToVector( strFilter, " " );

					string strNameIndex = string( );
					string strTag = string( );

					for( vector< pair< string, string > > :: iterator ulTagKey = m_vecTags.begin( ); ulTagKey != m_vecTags.end( ); ulTagKey++ )
					{
						if( !(*ulTagKey).first.empty( ) )
						{
							strNameIndex = (*ulTagKey).first;
							strTag = (*ulTagKey).second;
							if( strNameIndex == "201" || strNameIndex == "301" || strNameIndex == "401" )
								pResponse->strContent += "</tr><tr class=\"index_filter\">\n";
//							if( strNameIndex == "101" || strNameIndex == "201" || strNameIndex == "301" )
//							{
//								pResponse->strContent += "<td class=\"index_filter\">";
//								pResponse->strContent += "<input name=\"checker" + strNameIndex.substr( 0, 1 ) + "\" type=checkbox onclick=\"javascript: check(form,'checker" + strNameIndex.substr( 0, 1 ) + "','filter" + strNameIndex.substr( 0, 1 ) + "')\">";
//								pResponse->strContent += "<a class=\"filter_by\" title=\"" + UTIL_RemoveHTML( gmapLANG_CFG["filter_by"] + ": " + strTag.substr( 0, strTag.find( ' ' ) ) )+ "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?filter=" + strNameIndex[0] + "\">";
//								pResponse->strContent += strTag.substr( 0, strTag.find( ' ' ) ) + "</a> : </td>\n\n";
//							}
//							else if( strNameIndex == "401" )
//								pResponse->strContent += "<td class=\"index_filter\"></td>\n\n";
							pResponse->strContent += "<td class=\"index_filter\">";
							pResponse->strContent += "<input name=\"tag" + strNameIndex + "\" type=checkbox";
							for( vector<string> :: iterator ulKey = vecFilter.begin( ); ulKey != vecFilter.end( ); ulKey++ )
								if( *ulKey == strNameIndex )
									pResponse->strContent += " checked";
							pResponse->strContent += ">";

							pResponse->strContent += UTIL_RemoveHTML( strTag );

							pResponse->strContent += "</td>\n\n";
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
				pResponse->strContent += gmapLANG_CFG["prefs_torrents_per_page"] + "<input name=\"torrents_per_page\" alt=\"[" + gmapLANG_CFG["prefs_torrents_per_page"] + "]\" type=text size=5 maxlength=3 value=\"";
				if( !mapPrefs["bperpage"].empty( ) )
					pResponse->strContent += mapPrefs["bperpage"];
				else
					pResponse->strContent += "0";
				pResponse->strContent += "\">\n";
				pResponse->strContent += UTIL_Xsprintf( gmapLANG_CFG["prefs_torrents_per_page_default"].c_str( ), CAtomInt( m_uiPerPageMax ).toString( ).c_str( ) );
				pResponse->strContent += "<br>" + gmapLANG_CFG["prefs_torrents_added"];
				pResponse->strContent += "<input id=\"id_added_uploaded\" name=\"torrents_added\" alt=\"[" + gmapLANG_CFG["prefs_torrents_added_uploaded"] + "]\" type=radio value=\"uploaded\"";
				if( !mapPrefs["baddedpassed"].empty( ) && mapPrefs["baddedpassed"] == "0" )
					pResponse->strContent += " checked=\"checked\"";
				pResponse->strContent += "><label for=\"id_added_uploaded\">" + gmapLANG_CFG["prefs_torrents_added_uploaded"] + "</label>";
				pResponse->strContent += "<input id=\"id_added_passed\" name=\"torrents_added\" alt=\"[" + gmapLANG_CFG["prefs_torrents_added_passed"] + "]\" type=radio value=\"passed\"";
				if( !mapPrefs["baddedpassed"].empty( ) && mapPrefs["baddedpassed"] == "1" )
					pResponse->strContent += " checked=\"checked\"";
				pResponse->strContent += "><label for=\"id_added_passed\">" + gmapLANG_CFG["prefs_torrents_added_passed"] + "</label>";
				pResponse->strContent += "</td>\n</tr>\n";
				
				pResponse->strContent += "<tr class=\"preferences\"><th class=\"preferences\">" + gmapLANG_CFG["prefs_messages"] + "</th>";
				pResponse->strContent += "<td class=\"preferences\">";
				pResponse->strContent += "<input name=\"messages_save_sent\" alt=\"[" + gmapLANG_CFG["prefs_messages_save_sent"] + "]\" type=checkbox";
				if( !mapPrefs["bsavesent"].empty( ) && mapPrefs["bsavesent"] == "1" )
					pResponse->strContent += " checked=\"checked\"";
				pResponse->strContent += ">" + gmapLANG_CFG["prefs_messages_save_sent"];
				pResponse->strContent += "<br><input name=\"messages_new_comment\" alt=\"[" + gmapLANG_CFG["prefs_messages_new_comment"] + "]\" type=checkbox";
				if( !mapPrefs["bmsgcomment"].empty( ) && mapPrefs["bmsgcomment"] == "1" )
					pResponse->strContent += " checked=\"checked\"";
				pResponse->strContent += ">" + gmapLANG_CFG["prefs_messages_new_comment"];
				pResponse->strContent += "<br><input name=\"messages_new_comment_bookmarked\" alt=\"[" + gmapLANG_CFG["prefs_messages_new_comment_bookmarked"] + "]\" type=checkbox";
				if( !mapPrefs["bmsgcommentbm"].empty( ) && mapPrefs["bmsgcommentbm"] == "1" )
					pResponse->strContent += " checked=\"checked\"";
				pResponse->strContent += ">" + gmapLANG_CFG["prefs_messages_new_comment_bookmarked"];
				pResponse->strContent += "</td>\n</tr>\n";
				pResponse->strContent += "<tr class=\"preferences\">\n<th class=\"preferences\">" + gmapLANG_CFG["prefs_save"] + "</th>";
				pResponse->strContent += "<td class=\"preferences\">";
				pResponse->strContent += Button_Submit( "submit_user_preferences", gmapLANG_CFG["submit"] );
				pResponse->strContent += "<span class=\"red\">";
				if( !pRequest->mapParams["saved"].empty( ) && pRequest->mapParams["saved"] == "1" )
					pResponse->strContent += gmapLANG_CFG["prefs_saved"];
				pResponse->strContent += "</span>";
//				pResponse->strContent += Button_JS_Link( "user_per_page_default", gmapLANG_CFG["default"], "default_perpage_confirm( )" );
				pResponse->strContent += "</td>\n</tr>\n";
				pResponse->strContent += "</table/>\n";
				pResponse->strContent += "</form/>\n";
			}
		}
		else if( cstrDetailShow == "bonus" )
		{
			if( pRequest->user.strUID == user.strUID && ( CFG_GetInt( "bnbt_bonus_trade_enable", 0 ) == 0 ? false : true ) )
			{
				const string cstrTrade( pRequest->mapParams["trade"] );
				int iTradeRate = CFG_GetInt( "bnbt_bonus_trade_rate", 500 );
				string strRatioLimit = CFG_GetString( "bnbt_bonus_trade_ratio_limit", "5.0" );
				if( cstrTrade.empty( ) )
				{
					pResponse->strContent += "<div class=\"bonus_function\">\n";
					pResponse->strContent += "<form name=\"bonustrade\" method=\"get\" action=\"" + string( RESPONSE_STR_LOGIN_HTML ) + "\" onSubmit=\"return validate( this )\">";
					pResponse->strContent += "<input name=\"show\" type=hidden value=\"" + cstrDetailShow + "\">\n";
					
					pResponse->strContent += "<table class=\"bonus_function\">\n";
					pResponse->strContent += "<tr class=\"bonus_function\">\n";
					pResponse->strContent += "<th class=\"bonus_function\">" + gmapLANG_CFG["bonus_function"] + "</th>";
					pResponse->strContent += "<th class=\"bonus_function\">" + gmapLANG_CFG["bonus_function_rate"] + "</th>";
					pResponse->strContent += "<th class=\"bonus_function\">" + gmapLANG_CFG["bonus_function_deal"] + "</th>";
					pResponse->strContent += "</tr>\n";
					pResponse->strContent += "<tr class=\"bonus_function\">\n";
					pResponse->strContent += "<td class=\"bonus_function\">\n";
					pResponse->strContent += gmapLANG_CFG["bonus_function_trade"] + "\n";
					pResponse->strContent += "<input name=\"trade\" alt=\"[" + gmapLANG_CFG["bonus_function_trade"] + "]\" type=text size=5 maxlength=3 value=\"\"> GB";
					pResponse->strContent += "<br>" + UTIL_Xsprintf( gmapLANG_CFG["bonus_function_trade_note"].c_str( ), strRatioLimit.c_str( ) );
					pResponse->strContent += "</td>\n";
					pResponse->strContent += "<td class=\"bonus_function\">\n";
					pResponse->strContent += UTIL_Xsprintf( gmapLANG_CFG["bonus_function_trade_rate"].c_str( ), CAtomInt( iTradeRate ).toString( ).c_str( ) );
					pResponse->strContent += "</td>\n";
					pResponse->strContent += "<td class=\"bonus_function\">\n";
					pResponse->strContent += "<input name=\"submit_bonus_trade_button\" alt=\"" + gmapLANG_CFG["yes"] + "\" type=submit value=\"" + gmapLANG_CFG["yes"] + "\"";
					if( user.flShareRatio > atof( strRatioLimit.c_str( ) ) || user.flShareRatio < 0 )
						pResponse->strContent += " disabled=true";
					pResponse->strContent += ">\n";
					pResponse->strContent += "</td>\n";
					pResponse->strContent += "</tr>\n";
					pResponse->strContent += "</table>\n";
					pResponse->strContent += "</form>\n</div>\n";

				}
				else
				{
					const string cstrOK( pRequest->mapParams["ok"] );
					string strPageParameters = LOGIN_HTML;
					if( pRequest->mapParams["submit_bonus_trade_button"] == gmapLANG_CFG["yes"] )
					{
						strPageParameters += "?show=bonus&trade=" + cstrTrade;
						if( !cstrOK.empty( ) )
							strPageParameters += "&ok=" + cstrOK;
//						pResponse->strContent += "<script type=\"text/javascript\">\n";
//						pResponse->strContent += "<!--\n";
//						
//						pResponse->strContent += "window.location=\"" + strPageParameters + "\"\n";

//						pResponse->strContent += "//-->\n";
//						pResponse->strContent += "</script>\n\n";
 							return JS_ReturnToPage( pRequest, pResponse, strPageParameters );
//						return;
					}
					bool bNum = true;
					if( cstrTrade.find_first_not_of( "1234567890" ) != string :: npos )
						bNum  = false;
					if( bNum && !( user.flShareRatio > atof( strRatioLimit.c_str( ) ) || user.flShareRatio < 0 ) )
					{
						int64 iTrade = atoi( cstrTrade.c_str( ) );
						if( user.ulBonus >= iTrade * iTradeRate * 100 )
						{
							if( cstrOK == "1" )
							{
								if( user.ulBonus > 0 )
								{
//										m_pCache->setUserData( user.strUID, iTrade * 1024 * 1024 * 1024, 0, -iTrade * iTradeRate * 100 );
									CMySQLQuery mq01( "UPDATE users SET bbonus=bbonus-" + CAtomLong( iTrade * iTradeRate * 100 ).toString( ) + ",buploaded=buploaded+" + CAtomLong( iTrade * 1024 * 1024 * 1024 ).toString( ) + " WHERE buid=" + user.strUID );
									
									pResponse->strContent += "<p class=\"bonustrade\"><span class=\"red\">" + gmapLANG_CFG["bonus_function_trade_succeed"] + "</span></p>\n";
									pResponse->strContent += "<p class=\"bonustrade\"><span class=\"blue\">" + UTIL_Xsprintf( gmapLANG_CFG["bonus_function_trade_detail"].c_str( ), cstrTrade.c_str( ), CAtomInt( iTrade * iTradeRate ).toString( ).c_str( ) ) + "</span></p>\n";
									
									UTIL_LogFilePrint( "tradeBonus: %s traded %s Bonus for %s GB upload\n", user.strLogin.c_str( ), CAtomInt( iTrade * iTradeRate ).toString( ).c_str( ), cstrTrade.c_str( ) );
									return JS_ReturnToPage( pRequest, pResponse, LOGIN_HTML + "?show=bonus" );
//									pResponse->strContent += "<script type=\"text/javascript\">\n";
//									pResponse->strContent += "<!--\n";
//									
//									pResponse->strContent += "window.location=\"" + RESPONSE_STR_LOGIN_HTML + "\"\n";

//									pResponse->strContent += "//-->\n";
//									pResponse->strContent += "</script>\n\n";
								}
							}
							else
							{
								pResponse->strContent += "<div class=\"bonus_function\">\n";
								pResponse->strContent += "<form name=\"bonustrade\" method=\"get\" action=\"" + string( RESPONSE_STR_LOGIN_HTML ) + "\" onSubmit=\"return validate( this )\">";
								pResponse->strContent += "<p class=\"bonustrade\"><input name=\"show\" type=hidden value=\"" + cstrDetailShow + "\"></p>\n";
								pResponse->strContent += "<p class=\"bonustrade\"><input name=\"trade\" type=hidden value=\"" + cstrTrade + "\"></p>\n";
								pResponse->strContent += "<p class=\"bonustrade\"><span class=\"blue\">" + UTIL_Xsprintf( gmapLANG_CFG["bonus_function_trade_detail"].c_str( ), cstrTrade.c_str( ), CAtomInt( iTrade * iTradeRate ).toString( ).c_str( ) ) + "</span></p>\n";
								pResponse->strContent += "<p class=\"bonustrade\"><input name=\"ok\" type=hidden value=\"1\"></p>\n";
								pResponse->strContent += Button_Submit( "submit_bonus_trade", string( gmapLANG_CFG["yes"] ) );
								pResponse->strContent += Button_Back( "cancel_bonus_trade", string( gmapLANG_CFG["no"] ) );
								pResponse->strContent += "</form></div>\n";
							}
						}
						else
						{
							pResponse->strContent += "<p class=\"bonustrade\">" + UTIL_Xsprintf( gmapLANG_CFG["bonus_function_trade_not_enough"].c_str( ), CAtomInt( iTrade * iTradeRate ).toString( ).c_str( ) ) + "</p>\n";
							
						}
					}
				}
			}
		}
		else if( cstrDetailShow == "invites" )
		{
			if( ( pRequest->user.strUID == user.strUID && ( pRequest->user.ucAccess & m_ucAccessView ) ) || ( pRequest->user.ucAccess & m_ucAccessAdmin ) )
			{
				const string cstrInvite( pRequest->mapParams["invite"] );
				
				if( pRequest->user.strUID == user.strUID )
				{
					if( cstrInvite.empty( ) )
					{
						pResponse->strContent += "<div class=\"invite_function\">\n";
						pResponse->strContent += "<form name=\"invite\" method=\"get\" action=\"" + string( RESPONSE_STR_LOGIN_HTML ) + "\" onSubmit=\"return validate( this )\">";
						pResponse->strContent += "<p class=\"invite\"><input name=\"show\" type=hidden value=\"" + cstrDetailShow + "\"></p>\n";
						pResponse->strContent += "<p class=\"invite\"><span>" + UTIL_Xsprintf( gmapLANG_CFG["invite_function_invites_left"].c_str( ), string( "<span class=\"red\">" + user.strInvites + "</span>" ).c_str( ) );
						pResponse->strContent += "<input name=\"invite\"alt=\"[" + gmapLANG_CFG["invite_function_create_invite"] + "]\" type=submit value=\"" + gmapLANG_CFG["invite_function_create_invite"] + "\"";
						if( CFG_GetInt( "bnbt_invite_enable", 0 ) == 0 || !( user.ucAccess & m_ucAccessInvites ) || (unsigned int)atoi( user.strInvites.c_str( ) ) == 0 || ( m_bRatioRestrict && checkShareRatio( user.ulDownloaded, user.flShareRatio ) ) )
							pResponse->strContent += " disabled=\"yes\"";
						pResponse->strContent += ">";
						if( CFG_GetInt( "bnbt_invite_enable", 0 ) == 0 || !( user.ucAccess & m_ucAccessInvites ) )
							pResponse->strContent += "<span class=\"red\">" + gmapLANG_CFG["invite_function_invite_close"] + "</span>";
						pResponse->strContent += "</span></p>";

						pResponse->strContent += "</form>\n</div>\n";
					}
					else
					{
						if( pRequest->mapParams["invite"] == gmapLANG_CFG["invite_function_create_invite"] )
						{
							if( (unsigned int)atoi( user.strInvites.c_str( ) ) > 0 && CFG_GetInt( "bnbt_invite_enable", 0 ) == 1 && ( user.ucAccess & m_ucAccessInvites ) && !( m_bRatioRestrict && checkShareRatio( user.ulDownloaded, user.flShareRatio ) ) )
							{
								unsigned char szMD5[16];
								memset( szMD5, 0, sizeof( szMD5 ) / sizeof( unsigned char ) );
								MD5_CTX md5;

								time_t tNow = time( 0 );
								char pTime[256];
								memset( pTime, 0, sizeof( pTime ) / sizeof( char ) );
								strftime( pTime, sizeof( pTime ) / sizeof( char ), "%Y-%m-%d %H:%M:%S", localtime( &tNow ) );
								const string cstrA1( user.strLogin + ":" + gstrRealm + ":" + pTime );

								MD5Init( &md5 );
								MD5Update( &md5, (const unsigned char *)cstrA1.c_str( ), (unsigned int)cstrA1.size( ) );
								MD5Final( szMD5, &md5 );
								
								CMySQLQuery mq01( "INSERT INTO invites (bcode,bownerid,bcreated) VALUES(\'" + UTIL_StringToMySQL( UTIL_HashToString( string( (char *)szMD5, sizeof( szMD5 ) / sizeof( unsigned char ) ) ) ) + "\'," + UTIL_StringToMySQL( user.strUID ) + ",NOW())" );
								CMySQLQuery mq02( "UPDATE users SET binvites=binvites-1 WHERE buid=" + user.strUID + " AND binvites>0" );
							}
							
							pResponse->strContent += "<script type=\"text/javascript\">\n";
							pResponse->strContent += "<!--\n";
							
							pResponse->strContent += "window.location=\"" + RESPONSE_STR_LOGIN_HTML + "?show=invites\"\n";

							pResponse->strContent += "//-->\n";
							pResponse->strContent += "</script>\n\n";

							return;
						}
					}
				}
				
				pResponse->strContent += "<table class=\"user_detail_table\" id=\"user_invites\">\n";
				pResponse->strContent += "<tr><th colspan=4>" + gmapLANG_CFG["user_detail_invites"] + "</th></tr>\n";
					
				CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bcode,bcreated,bused,binvitee,binviteeid FROM invites WHERE bownerid=" + user.strUID + " ORDER BY bcreated DESC" );
				
				vector<string> vecQuery;
			
				vecQuery.reserve(5);

				vecQuery = pQuery->nextRow( );
				
				if( vecQuery.size( ) == 5 )
				{
					pResponse->strContent += "<tr>\n";
					
					pResponse->strContent += "<th class=\"hash\" id=\"nameheader\">" + gmapLANG_CFG["invite_code"];

					pResponse->strContent += "</th>\n";
					
					pResponse->strContent += "<th class=\"date\" id=\"createdheader\">" + gmapLANG_CFG["invite_created"];

					pResponse->strContent += "</th>\n";
					
					pResponse->strContent += "<th class=\"status\" id=\"statusheader\">" + gmapLANG_CFG["invite_status"];

					pResponse->strContent += "</th>\n";
					
					pResponse->strContent += "<th class=\"uploader\" id=\"inviteeheader\">" + gmapLANG_CFG["invitee"];

					pResponse->strContent += "</th>\n";
					
					pResponse->strContent += "</tr>\n";
					
					while( vecQuery.size( ) == 5 )
					{
						pResponse->strContent += "<tr>\n";
						
						pResponse->strContent += "<td class=\"hash\">" + vecQuery[0] + "</td>\n";
						pResponse->strContent += "<td class=\"date\">" + vecQuery[1] + "</td>\n";
						pResponse->strContent += "<td class=\"status\">";
						if( vecQuery[2] == "0" )
							pResponse->strContent +=  "<span class=\"red\">" + gmapLANG_CFG["invite_unused"] + "</span>";
						else
							pResponse->strContent +=  "<span class=\"green\">" + gmapLANG_CFG["invite_used"] + "</span>";
						pResponse->strContent += "</td>\n";
						pResponse->strContent += "<td class=\"uploader\">";
						pResponse->strContent += getUserLink( vecQuery[4], vecQuery[3] );
						pResponse->strContent += "</td>\n";
						
						pResponse->strContent += "</tr>\n";
						
						vecQuery = pQuery->nextRow( );
					}
				}
				else
					pResponse->strContent += "<tr><td>" + gmapLANG_CFG["result_none_found"] + "</td></tr>\n";
				
				delete pQuery;
				
				pResponse->strContent += "</table>\n";
			}
		}
		else if( cstrDetailShow == "friends" )
		{
			if( pRequest->user.strUID == user.strUID && ( pRequest->user.ucAccess & m_ucAccessView ) )
			{
				pResponse->strContent += "<table class=\"user_detail_table\" id=\"user_friends\">\n";
				pResponse->strContent += "<tr><th colspan=4>" + gmapLANG_CFG["user_detail_friends"] + "</th></tr>\n";
				
				CMySQLQuery *pQuery = new CMySQLQuery( "SELECT buid,bfriendid,bfriendname FROM friends WHERE buid=" + user.strUID );
				
				vector<string> vecQuery;
			
				vecQuery.reserve(3);

				vecQuery = pQuery->nextRow( );
				
				if( vecQuery.size( ) == 3 )
				{
					pResponse->strContent += "<tr>\n";
					
					pResponse->strContent += "<th class=\"uploader\">" + gmapLANG_CFG["friend_name"];

					pResponse->strContent += "</th>\n";
					
					pResponse->strContent += "<th class=\"date\">" + gmapLANG_CFG["friend_last"];

					pResponse->strContent += "</th>\n";
					
					pResponse->strContent += "<th class=\"admin\">" + gmapLANG_CFG["friend_do"];

					pResponse->strContent += "</th>\n";
					
					pResponse->strContent += "<th class=\"admin\">" + gmapLANG_CFG["friend_admin"];

					pResponse->strContent += "</th>\n";
					
					pResponse->strContent += "</tr>\n";

					while( vecQuery.size( ) == 3 )
					{
						pResponse->strContent += "<tr>\n";
						
						pResponse->strContent += "<td class=\"uploader\">" +getUserLink( vecQuery[1], vecQuery[2] ); + "</td>\n";
						pResponse->strContent += "<td class=\"date\">";
						
						CMySQLQuery *pQueryFriend = new CMySQLQuery( "SELECT blast FROM users WHERE buid=" + vecQuery[1] );
				
						vector<string> vecQueryFriend;
			
						vecQueryFriend.reserve(1);

						vecQueryFriend = pQueryFriend->nextRow( );
						
						delete pQueryFriend;
						
						if( vecQueryFriend.size( ) == 1 && !vecQueryFriend.empty( ) )
							pResponse->strContent += vecQueryFriend[0];
						else
							pResponse->strContent += gmapLANG_CFG["na"];
						
						pResponse->strContent += "</td>\n";
						
						pResponse->strContent += "<td class=\"admin\">";
						if( pRequest->user.ucAccess & m_ucAccessMessages )
						{
							pResponse->strContent += "[<a class=\"black\" title=\"" + gmapLANG_CFG["talk_to"] + "\" href=\"" + RESPONSE_STR_TALK_HTML + "?talk=" + UTIL_StringToEscaped( "@" + vecQuery[2] + " " ) + "\">" + gmapLANG_CFG["talk_to"] + "</a>]";
						}
		
						if( pRequest->user.ucAccess & m_ucAccessComments )
						{
							pResponse->strContent += "[<a class=\"black\" title=\"" + gmapLANG_CFG["messages_send_message"] + "\" href=\"" + RESPONSE_STR_MESSAGES_HTML + "?sendto=" + vecQuery[1] + "\">" + gmapLANG_CFG["messages_send_message"] + "</a>]";
						}
						pResponse->strContent += "</td>\n";
						pResponse->strContent += "<td class=\"admin\">";
						pResponse->strContent += "[<a id=\"friend" + vecQuery[1] + "\" class=\"friend\" href=\"javascript: ;\" onclick=\"javascript: friend('" + vecQuery[1] + "','" + gmapLANG_CFG["friend_add"] + "','" + gmapLANG_CFG["friend_remove"] + "');\">";
						pResponse->strContent += gmapLANG_CFG["friend_remove"] + "</a>]";
						pResponse->strContent += "</td>\n";
						pResponse->strContent += "</tr>\n";
						
						vecQuery = pQuery->nextRow( );
					}
				}
				else
					pResponse->strContent += "<tr><td>" + gmapLANG_CFG["result_none_found"] + "</td></tr>\n";
				
				delete pQuery;
				
				pResponse->strContent += "</table>\n";
			}
		}
		else if( cstrDetailShow == "friendeds" )
		{
			if( pRequest->user.strUID == user.strUID && ( pRequest->user.ucAccess & m_ucAccessView ) )
			{
				pResponse->strContent += "<table class=\"user_detail_table\" id=\"user_friendeds\">\n";
				pResponse->strContent += "<tr><th colspan=4>" + gmapLANG_CFG["user_detail_friendeds"] + "</th></tr>\n";
				
				CMySQLQuery *pQuery = new CMySQLQuery( "SELECT buid FROM friends WHERE bfriendid=" + user.strUID );
				
				vector<string> vecQuery;
			
				vecQuery.reserve(1);

				vecQuery = pQuery->nextRow( );
				
				if( vecQuery.size( ) == 1 )
				{
					pResponse->strContent += "<tr>\n";

					pResponse->strContent += "<th class=\"uploader\">" + gmapLANG_CFG["friended_name"];

					pResponse->strContent += "</th>\n";
					
					pResponse->strContent += "<th class=\"date\">" + gmapLANG_CFG["friended_last"];

					pResponse->strContent += "</th>\n";
					
					pResponse->strContent += "<th class=\"admin\">" + gmapLANG_CFG["friend_do"];

					pResponse->strContent += "</th>\n";
					
					pResponse->strContent += "<th class=\"admin\">" + gmapLANG_CFG["friend_admin"];

					pResponse->strContent += "</th>\n";

					pResponse->strContent += "</tr>\n";

					while( vecQuery.size( ) == 1 )
					{
						CMySQLQuery *pQueryFriended = new CMySQLQuery( "SELECT busername,blast FROM users WHERE buid=" + vecQuery[0] );
				
						vector<string> vecQueryFriended;
			
						vecQueryFriended.reserve(1);

						vecQueryFriended = pQueryFriended->nextRow( );
						
						delete pQueryFriended;
						
						if( vecQueryFriended.size( ) == 2 )
						{
							pResponse->strContent += "<tr>\n";
						
							pResponse->strContent += "<td class=\"uploader\">" + getUserLink( vecQuery[0], vecQueryFriended[0] ); + "</td>\n";
							pResponse->strContent += "<td class=\"date\">";
						
							if( !vecQueryFriended[1].empty( ) )
								pResponse->strContent += vecQueryFriended[1];
							else
								pResponse->strContent += gmapLANG_CFG["na"];
						
							pResponse->strContent += "</td>\n";
							
							pResponse->strContent += "<td class=\"admin\">";
							if( pRequest->user.ucAccess & m_ucAccessMessages )
							{
								pResponse->strContent += "[<a class=\"black\" title=\"" + gmapLANG_CFG["talk_to"] + "\" href=\"" + RESPONSE_STR_TALK_HTML + "?talk=" + UTIL_StringToEscaped( "@" + vecQueryFriended[0] + " " ) + "\">" + gmapLANG_CFG["talk_to"] + "</a>]";
							}
		
							if( pRequest->user.ucAccess & m_ucAccessComments )
							{
								pResponse->strContent += "[<a class=\"black\" title=\"" + gmapLANG_CFG["messages_send_message"] + "\" href=\"" + RESPONSE_STR_MESSAGES_HTML + "?sendto=" + vecQuery[0] + "\">" + gmapLANG_CFG["messages_send_message"] + "</a>]";
							}
							pResponse->strContent += "</td>\n";
						
							pResponse->strContent += "<td class=\"admin\">";

							CMySQLQuery *pQueryFriend = new CMySQLQuery( "SELECT bfriendid FROM friends WHERE buid=" + user.strUID + " AND bfriendid=" + vecQuery[0] );
					
							vector<string> vecQueryFriend;
				
							vecQueryFriend.reserve(1);

							vecQueryFriend = pQueryFriend->nextRow( );
							
							delete pQueryFriend;

							if( vecQueryFriend.size( ) == 0 )
							{
								pResponse->strContent += "[<a id=\"friend" + vecQuery[0] + "\" class=\"friend\" href=\"javascript: ;\" onclick=\"javascript: friend('" + vecQuery[0] + "','" + gmapLANG_CFG["friend_add"] + "','" + gmapLANG_CFG["friend_remove"] + "');\">";
								pResponse->strContent += gmapLANG_CFG["friend_add"] + "</a>]";
							}
							else if( vecQueryFriend.size( ) == 1 )
								pResponse->strContent += gmapLANG_CFG["friended_friended"];

							pResponse->strContent += "</td>\n";

							pResponse->strContent += "</tr>\n";
						}
						
						vecQuery = pQuery->nextRow( );
					}
				}
				else
					pResponse->strContent += "<tr><td>" + gmapLANG_CFG["result_none_found"] + "</td></tr>\n";
				
				delete pQuery;
				
				pResponse->strContent += "</table>\n";
			}
		}
		else if( cstrDetailShow == "comments" )
		{
			CMySQLQuery *pQueryComment = 0;
//			if( pRequest->user.strUID == user.strUID )
//				pQueryComment = new CMySQLQuery( "SELECT bid,busername,buid,bip,bposted,bcomment,btid,boid FROM comments WHERE buid=" + user.strUID + " OR buid in (SELECT bfriendid FROM friends WHERE buid=" + user.strUID + ") ORDER BY bposted DESC" );
//			else
				pQueryComment = new CMySQLQuery( "SELECT bid,busername,buid,bip,bposted,bcomment,btid,boid FROM comments WHERE buid=" + user.strUID + " ORDER BY bposted DESC" );
			
			vector<string> vecQueryComment;
		
			vecQueryComment.reserve(8);

			vecQueryComment = pQueryComment->nextRow( );
			
			bool bFound = false;
			
			unsigned long ulCount = 0;

			while( vecQueryComment.size( ) == 8 )
			{
				if( !bFound )
				{
					pResponse->strContent += "<div class=\"comments_table_posted\">\n";
					pResponse->strContent += "<table class=\"comments_table_posted\" summary=\"comments\">\n";

					bFound = true;
				}
				
				string strID = string( );
				string strIP = string( );
				string strName = string( );
				string strNameID = string( );
				string strTime = string( );
				string strComText = string( );
				
				string :: size_type iStart = 0;

				strID = vecQueryComment[0];
				
				if( !vecQueryComment[1].empty( ) )
					strName = vecQueryComment[1];
				if( !vecQueryComment[2].empty( ) )
					strNameID = vecQueryComment[2];
				
				strIP = vecQueryComment[3];
				strTime = vecQueryComment[4];
				strComText = vecQueryComment[5];

				// strip ip

				iStart = strIP.rfind( "." );

				if( iStart != string :: npos )
				{
					// don't strip ip for mods

					if( !( pRequest->user.ucAccess & m_ucAccessShowIP ) && pRequest->user.strUID != strNameID )
						strIP = strIP.substr( 0, iStart + 1 ) + "xxx";
				}
				else
				{
					iStart = strIP.rfind( ":" );
					if( iStart != string :: npos )
					{
						// don't strip ip for mods

						if( !( pRequest->user.ucAccess & m_ucAccessShowIP ) && pRequest->user.strUID != strNameID )
							strIP = strIP.substr( 0, iStart + 1 ) + "xxxx";
					}
				}

				//
				// header
				//

				// Comment by
				string strUserLink = getUserLinkFull( strNameID, strName );
				
				if( strUserLink.empty( ) )
					strUserLink = gmapLANG_CFG["unknown"];
				
				if( strIP.empty( ) )
					strIP = gmapLANG_CFG["unknown"];
				
				if( strTime.empty( ) )
					strTime = gmapLANG_CFG["unknown"];
				
				pResponse->strContent += "<tr class=\"com_header\"><td class=\"com_header\">" + UTIL_Xsprintf( gmapLANG_CFG["comments_posted_by"].c_str( ), strID.c_str( ), strUserLink.c_str( ), strTime.c_str( ) );
//				pResponse->strContent += "<tr class=\"com_header\"><td class=\"com_header\">" + UTIL_Xsprintf( gmapLANG_CFG["user_detail_comments_posted_by"].c_str( ), strID.c_str( ), strIP.c_str( ), strTime.c_str( ) );
				
				if( !vecQueryComment[6].empty( ) && vecQueryComment[6] != "0" )
					pResponse->strContent += "<a href=\"" + RESPONSE_STR_STATS_HTML + "?id=" + vecQueryComment[6] + "\">" + gmapLANG_CFG["user_detail_comments_to_torrent"] + "</a>";
				else if( !vecQueryComment[7].empty( ) )
					pResponse->strContent += "<a href=\"" + RESPONSE_STR_STATS_HTML + "?oid=" + vecQueryComment[7] + "\">" + gmapLANG_CFG["user_detail_comments_to_offer"] + "</a>";

				pResponse->strContent += "</td></tr>\n";

				//
				// body
				//
				
				pResponse->strContent += "<tr class=\"com_body\"><td class=\"com_body\">" + UTIL_RemoveHTML2( strComText ) + "</td></tr>\n";

				ulCount++;
				
				vecQueryComment = pQueryComment->nextRow( );
			}
			delete pQueryComment;

			if( bFound )
			{
				pResponse->strContent += "</table>\n";
				pResponse->strContent += "</div>\n";
			}
			else
				pResponse->strContent += "<p class=\"comments_table_posted\">" + gmapLANG_CFG["comments_no_comment"] + "</p>\n";
		}
		else
		{
			// Compose the page

			unsigned long culKeySize = 0;
			
			struct torrent_t *pTorrents = 0;
			
			if( m_pCache )
				pTorrents = m_pCache->getCache( );
				
			if( pTorrents )
				culKeySize = m_pCache->getSize( );
				
			// Are we tracking any files?	
			if( culKeySize == 0 )
			{
				// No files are being tracked!
				pResponse->strContent += "<p class=\"results\">" + gmapLANG_CFG["result_none_found"] + "</p>\n\n";

				// Output common HTML tail
				HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_LOGIN ) );

				return;
			}
			
			m_pCache->setFree( );

			time_t now_t = time( 0 );

			bool bFreeGlobal = CFG_GetInt( "bnbt_free_global", 0 ) == 0 ? false : true;

			int64 iFreeDownGlobal = CFG_GetInt( "bnbt_free_down_global", 100 );
			int64 iFreeUpGlobal = CFG_GetInt( "bnbt_free_up_global", 100 );
			
			string strEngName = string( );
			string strChiName = string( );
			
			// Sort
			const string cstrSort( pRequest->mapParams["sort"] );
			
			if( m_bSort )
			{
				const unsigned char cucSort( (unsigned char)atoi( cstrSort.c_str( ) ) );
				if( !cstrSort.empty( ) )
					m_pCache->sort( cucSort, true );
				else
					if( m_bShowAdded )
						m_pCache->sort( SORT_DADDED, true );
			}
			else
				if( m_bShowAdded )
					m_pCache->sort( SORT_DADDED, true );

			if( cstrDetailShow == "torrents" )
			{
				// some preliminary search crap

				const string cstrSearch( pRequest->mapParams["search"] );
//				const string cstrLowerSearch( UTIL_ToLower( cstrSearch ) );

				vector<string> vecSearch;
				vecSearch.reserve(64);
			
				vecSearch = UTIL_SplitToVector( cstrSearch, " " );

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

				// Count matching torrents for top of page
				unsigned long ulFound = 0;


//				for( unsigned long ulKey = 0; ulKey < culKeySize; ulKey++ )
//				{
//					bool bFoundKey = true;
//					string :: size_type iStart = 0;
//					string :: size_type iEnd = 0;
//					string strKeyword = string( );
//					iStart = cstrLowerSearch.find_first_not_of( " " );
//			
//					while( iStart != string :: npos && iEnd != string :: npos )
//					{
//						iEnd = cstrLowerSearch.find_first_of( " ", iStart );
//						strKeyword = cstrLowerSearch.substr( iStart, iEnd - iStart );
//						if( pTorrents[ulKey].strLowerName.find( strKeyword ) == string :: npos )
//							bFoundKey = false;
//						if( iEnd != string :: npos )
//							iStart = cstrLowerSearch.find_first_not_of( " ", iEnd );
//				
//					}
//					if( bFoundKey == false )
//						continue;

//					// only count entries that match the current user
//					if( pTorrents[ulKey].strUploaderID != user.strUID )
//						continue;

//					ulFound++;
//				}

				// Top search
				if( culKeySize && m_bSearch )
				{
					pResponse->strContent += "<form class=\"search_login_top\" name=\"topsearch\" method=\"get\" action=\"" + RESPONSE_STR_LOGIN_HTML + "\">\n";
					
					if( !cstrUID.empty( ) )
						pResponse->strContent += "<p><input name=\"uid\" type=hidden value=\"" + cstrUID + "\"></p>\n";
					
					if( !cstrDetailShow.empty( ) )
						pResponse->strContent += "<p><input name=\"show\" type=hidden value=\"" + cstrDetailShow + "\"></p>\n";

					if( !cstrPerPage.empty( ) )
						pResponse->strContent += "<p><input name=\"per_page\" type=hidden value=\"" + cstrPerPage + "\"></p>\n";

					if( !cstrSort.empty( ) )
						pResponse->strContent += "<p><input name=\"sort\" type=hidden value=\"" + cstrSort + "\"></p>\n";
					
//					if( m_bUseButtons )
//					{
						pResponse->strContent += "<p><label for=\"toptorrentsearch\">" + gmapLANG_CFG["torrent_search"] + "</label> <input name=\"search\" id=\"toptorrentsearch\" alt=\"[" + gmapLANG_CFG["torrent_search"] + "]\" type=text size=40";
						
						pResponse->strContent += " value=\"" + UTIL_RemoveHTML( cstrSearch ) + "\">\n";

						pResponse->strContent += Button_Submit( "top_submit_search", gmapLANG_CFG["search"] );
						pResponse->strContent += Button_Submit( "top_clear_filter_and_search", gmapLANG_CFG["clear_filter_search"] );

						pResponse->strContent += "</p>\n";
//					}
//					else
//						pResponse->strContent += "<p><label for=\"toptorrentsearch\">" + gmapLANG_CFG["torrent_search"] + "</label> <input name=\"search\" id=\"toptorrentsearch\" alt=\"[" + gmapLANG_CFG["torrent_search"] + "]\" type=text size=40> <a title=\"" + gmapLANG_CFG["clear_filter_search"] + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "\">" + gmapLANG_CFG["clear_filter_search"] + "</a></p>\n";

					pResponse->strContent += "</form>\n\n";
				}
				
				// search messages
				pResponse->strContent += "<p class=\"search_filter\">\n";

				if( !vecSearch.empty( ) )
					pResponse->strContent += "<span class=\"search_results_alt\">" + gmapLANG_CFG["result_search"] + ": \"</span><span class=\"filtered_by\">" + UTIL_RemoveHTML( cstrSearch ) + "</span>\"\n";

				pResponse->strContent += "</p>\n\n";
			
				string :: size_type iCountGoesHere = string :: npos;
		
				iCountGoesHere = pResponse->strContent.size( );
				
				const string cstrPage( pRequest->mapParams["page"] );

				if( !cstrPage.empty( ) )
					ulStart = ( unsigned long )( atoi( cstrPage.c_str( ) ) * uiOverridePerPage );

				bool bFound = false;

				unsigned long ulAdded = 0;
				unsigned long ulSkipped = 0;
				
				// output table headers

				pResponse->strContent += "<div class=\"login_table\">\n";
				pResponse->strContent += "<table summary=\"ytinfo\">\n";
				pResponse->strContent += "<tr><th colspan=4>" + gmapLANG_CFG["user_detail_torrents"] + "</th></tr>\n";

				// for correct page numbers after searching

				for( unsigned long ulKey = 0; ulKey < culKeySize; ulKey++ )
				{
					if( !UTIL_MatchVector( pTorrents[ulKey].strName, vecSearch, MATCH_METHOD_NONCASE_AND ) )
						continue;
					
//					if( !vecSearch.empty( ) )
//					{
//						// only display entries that match the search   
//						bool bFoundKey = true;
//				
//						for( vector<string> :: iterator ulVecKey = vecSearch.begin( ); ulVecKey != vecSearch.end( ); ulVecKey++ )
//						{
//							if( pTorrents[ulKey].strLowerName.find( UTIL_ToLower( *ulVecKey ) ) == string :: npos )
//							{
//								bFoundKey = false;
//								break;
//							}
//						}
//						if( bFoundKey == false )
//							continue;
//					}

					if( pTorrents[ulKey].strUploaderID != user.strUID )
						continue;

					ulFound++;

					if( uiOverridePerPage == 0 || ulAdded < uiOverridePerPage )
					{
						// create the table and display the headers first
						if( !bFound )
						{
							vector< pair< string, string > > vecParams;
							vecParams.reserve(64);
							string strJoined = string( );

							vecParams.push_back( pair<string, string>( string( "uid" ), cstrUID ) );
							vecParams.push_back( pair<string, string>( string( "show" ), cstrDetailShow ) );
							vecParams.push_back( pair<string, string>( string( "per_page" ), cstrPerPage ) );
							vecParams.push_back( pair<string, string>( string( "search" ), cstrSearch ) );

							strJoined = UTIL_RemoveHTML( UTIL_HTMLJoin( vecParams, string( "&" ), string( "&" ), string( "=" ) ) );
							
							

							pResponse->strContent += "<tr>\n";

							// <th> tag
							if( !m_vecTags.empty( ) )
							{
								pResponse->strContent += "<th class=\"tag\" id=\"tagheader\">" + gmapLANG_CFG["tag"];

								pResponse->strContent += "</th>\n";
							}

							// Name
							pResponse->strContent += "<th class=\"name\" id=\"nameheader\">" + gmapLANG_CFG["name"];

							if( m_bSort )
							{
								pResponse->strContent += "<br><div><a class=\"sort\" title=\"" + gmapLANG_CFG["sort_name_ascending"] + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "?sort=" + SORTSTR_ANAME;
								
								pResponse->strContent += strJoined;

								pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a class=\"sort\" title=\"" + gmapLANG_CFG["sort_name_descending"] + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "?sort=" + SORTSTR_DNAME;
								
								pResponse->strContent += strJoined;

								pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a></div>";
							}

							pResponse->strContent += "</th>\n";						

							// Added
							pResponse->strContent += "<th class=\"date\" id=\"addedheader\">" + gmapLANG_CFG["added"];

							if( m_bSort )
							{
								pResponse->strContent += "<br><div><a class=\"sort\" title=\"" + gmapLANG_CFG["sort_added_ascending"] + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "?sort=" + SORTSTR_AADDED;
								
								pResponse->strContent += strJoined;

								pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a class=\"sort\" title=\"" + gmapLANG_CFG["sort_added_descending"] + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "?sort=" + SORTSTR_DADDED;
								
								pResponse->strContent += strJoined;

								pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a></div>";
							}

							pResponse->strContent += "</th>\n";

							// Admin
							if( pRequest->user.strUID == user.strUID )
								pResponse->strContent += "<th class=\"admin\" id=\"adminheader\">" + gmapLANG_CFG["admin"] + "</th>\n";

							pResponse->strContent += "</tr>\n";

							// signal table created
							bFound = true;
						}

						if( ulSkipped == ulStart )
						{
							// output table rows
							pResponse->strContent += "<tr>\n";

							// display the tag
							pResponse->strContent += "<td class=\"tag\">";
							vector< pair< string, string > > :: iterator it2 = m_vecTagsMouse.begin( );
							if( !m_vecTags.empty( ) )
							{
								string strNameIndex = string( );
								string strTag = string( );
								for( vector< pair< string, string > > :: iterator it1 = m_vecTags.begin( ); it1!= m_vecTags.end( ); it1++ )

								{
									strNameIndex = (*it1).first;
									strTag = (*it1).second;

									if( strNameIndex == pTorrents[ulKey].strTag )
									{
										if ( !(*it2).second.empty( ) )
											pResponse->strContent += "<img class=\"tag\" src=\"" + (*it2).second + "\" alt=\"[" + strTag + "]\" title=\"" + strTag + "\" name=\"" + strTag + "\">";
										else
											pResponse->strContent += strTag;
										break;
									}
									it2++;
								}
								
							}
							pResponse->strContent += "</td>\n";
							
							vector< pair< string, string > > vecParams;
							vecParams.reserve(64);
							string strReturn = string( );

							vecParams.push_back( pair<string, string>( string( "uid" ), cstrUID ) );
							vecParams.push_back( pair<string, string>( string( "show" ), cstrDetailShow ) );
							vecParams.push_back( pair<string, string>( string( "per_page" ), cstrPerPage ) );
							vecParams.push_back( pair<string, string>( string( "sort" ), cstrSort ) );
							vecParams.push_back( pair<string, string>( string( "search" ), cstrSearch ) );
							vecParams.push_back( pair<string, string>( string( "page" ), cstrPage ) );

							strReturn = UTIL_RemoveHTML( UTIL_StringToEscaped( RESPONSE_STR_LOGIN_HTML + UTIL_HTMLJoin( vecParams, string( "?" ), string( "&" ), string( "=" ) ) ) );

							// display the name and stats link
// 							pResponse->strContent += "<td class=\"name_down\"><table class=\"name_down\">";

							int64 day_left = -1, hour_left = -1, minute_left = -1;
							
							time_t tTimeFree = pTorrents[ulKey].iFreeTo - now_t;
				
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
							
//							if( pTorrents[ulKey].iFreeTo > now_t )
//							{
//								day_left = ( pTorrents[ulKey].iFreeTo - now_t + 60 ) / 86400;
//								hour_left = ( pTorrents[ulKey].iFreeTo - now_t + 60 ) % 86400 / 3600;
//								minute_left = ( pTorrents[ulKey].iFreeTo - now_t + 60 ) % 3600 / 60;
//							}
							pResponse->strContent += "<td class=\"torrent_name\">";
							strEngName.erase( );
							strChiName.erase( );
							UTIL_StripName( pTorrents[ulKey].strName.c_str( ), strEngName, strChiName );
							if( pTorrents[ulKey].iFreeDown == 0 )
								pResponse->strContent += "<a class=\"stats_free\" title=\"";
							else
								pResponse->strContent += "<a class=\"stats\" title=\"";
							pResponse->strContent += gmapLANG_CFG["name"] + ": " + UTIL_RemoveHTML( pTorrents[ulKey].strName )+ "\" href=\"" + RESPONSE_STR_STATS_HTML + "?id=" + pTorrents[ulKey].strID;
//							if( !strReturn.empty( ) )
//								pResponse->strContent += "&amp;return=" + strReturn;
							pResponse->strContent += "\">";
							pResponse->strContent += UTIL_RemoveHTML( strEngName );
							if( !strChiName.empty( ) )
								pResponse->strContent += "<br><span class=\"stats\">" + UTIL_RemoveHTML( strChiName ) + "</span>";
							pResponse->strContent += "</a>";
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
									pResponse->strContent += "<span class=\"free_up\" title=\"" + UTIL_Xsprintf( gmapLANG_CFG["free_up"].c_str( ), CAtomInt( pTorrents[ulKey].iFreeUp ).toString( ).c_str( ) ) + "\">" + UTIL_Xsprintf( gmapLANG_CFG["free_up_short"].c_str( ), CAtomInt( pTorrents[ulKey].iFreeUp ).toString( ).c_str( ) )+ "</span>";
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

							if( m_bShowAdded )
							{
								pResponse->strContent += "<td class=\"date\">";

								if( !pTorrents[ulKey].strAdded.empty( ) )
								{
									const string :: size_type br = pTorrents[ulKey].strAdded.find( ' ' );
									pResponse->strContent += pTorrents[ulKey].strAdded.substr( 0, br );
									if( br != string :: npos )
										pResponse->strContent += "<br>" +  pTorrents[ulKey].strAdded.substr( br + 1 );
									// strip year and seconds from time
								}

								pResponse->strContent += "</td>\n";
							}

							if( pRequest->user.strUID == user.strUID )
							{
								// display user's torrent administration if any
								if( ( pRequest->user.ucAccess & m_ucAccessEditOwn ) || ( pRequest->user.ucAccess & m_ucAccessDelOwn ) )
								{
									pResponse->strContent += "<td class=\"admin\">";
									if( pRequest->user.ucAccess & m_ucAccessEditOwn )
									{
										pResponse->strContent += "[<a class=\"black\" title=\"" + gmapLANG_CFG["edit"] + ": " + UTIL_RemoveHTML( pTorrents[ulKey].strName ) + "\" href=\"" + RESPONSE_STR_STATS_HTML + "?id=" + pTorrents[ulKey].strID + "&amp;action=edit&amp;show=contents";
//										if( !strReturn.empty( ) )
											pResponse->strContent += "&amp;return=" + strReturn;
										pResponse->strContent += "\">" + gmapLANG_CFG["edit"] + "</a>]";
									}
									if( m_bDeleteOwnTorrents && ( pRequest->user.ucAccess & m_ucAccessDelOwn ) )
									{
										pResponse->strContent += "[<a class=\"red\" title=\"" + gmapLANG_CFG["delete"] + ": " + UTIL_RemoveHTML( pTorrents[ulKey].strName ) + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "?del=" + pTorrents[ulKey].strID;
//										if( !strReturn.empty( ) )
											pResponse->strContent += "&amp;return=" + strReturn;
										pResponse->strContent += "\">" + gmapLANG_CFG["delete"] + "</a>]";
									}
									pResponse->strContent += "</td>\n";
								}
								else
									pResponse->strContent += "<td class=\"admin\">" + gmapLANG_CFG["na"] + "</td>\n";
							}	
							// end row
							pResponse->strContent += "</tr>\n";

							// increment row counter for row colour
							ulAdded++;
						}
						else
							ulSkipped++;
					}
				}
				
				if( ulFound == RESULTS_ZERO )
					pResponse->strContent += "<tr><td>" + gmapLANG_CFG["result_none_found"] + "</td></tr>\n";

				// some finishing touches

				pResponse->strContent += "</table>\n";
				pResponse->strContent += "</div>\n\n";
				
				string strInsert = string( );
				
//				switch( ulFound )
//				{
//				case RESULTS_ZERO:
//					strInsert += "<p class=\"results\">" + gmapLANG_CFG["result_none_found"] + "</p>\n\n";
//					break;
//				case RESULTS_ONE:
//					pResponse->strContent += "<p class=\"results\">" + gmapLANG_CFG["result_1_found"] + "</p>\n\n";
//					break;
//				default:
//					// Many results found
//					pResponse->strContent += "<p class=\"results\">" + UTIL_Xsprintf( gmapLANG_CFG["result_x_found"].c_str( ), CAtomInt( ulFound ).toString( ).c_str( ) ) + "</p>\n\n";
//				}
				
				vector< pair< string, string > > vecParams;
				vecParams.reserve(64);
				string strJoined = string( );

				vecParams.push_back( pair<string, string>( string( "uid" ), cstrUID ) );
				vecParams.push_back( pair<string, string>( string( "show" ), cstrDetailShow ) );
				vecParams.push_back( pair<string, string>( string( "per_page" ), cstrPerPage ) );
				vecParams.push_back( pair<string, string>( string( "sort" ), cstrSort ) );
				vecParams.push_back( pair<string, string>( string( "search" ), cstrSearch ) );

				strJoined = UTIL_RemoveHTML( UTIL_HTMLJoin( vecParams, string( "&" ), string( "&" ), string( "=" ) ) );

				// page numbers top
				
				strInsert += UTIL_PageBar( ulFound, cstrPage, uiOverridePerPage, RESPONSE_STR_LOGIN_HTML, strJoined, true );
				
				if( iCountGoesHere != string :: npos )
					pResponse->strContent.insert( iCountGoesHere, strInsert );
				
				// page numbers bottom

				pResponse->strContent += UTIL_PageBar( ulFound, cstrPage, uiOverridePerPage, RESPONSE_STR_LOGIN_HTML, strJoined, false );
			}
			else if( cstrDetailShow == "bookmarks" || cstrDetailShow.empty( ) )
			{
//				if( ( pRequest->user.ucAccess & m_ucAccessUserDetails ) || pRequest->user.strUID == user.strUID )
				pResponse->strContent += "<table class=\"user_detail_table\" id=\"user_bookmarks\">\n";
				pResponse->strContent += "<tr><th colspan=8>";
				if( pRequest->user.strUID == user.strUID )
					pResponse->strContent += gmapLANG_CFG["user_detail_bookmarks"];
				else
					pResponse->strContent += gmapLANG_CFG["user_detail_shares"];
				pResponse->strContent += "</th></tr>\n";
				
				CMySQLQuery *pQuery = 0;
				
				if( pRequest->user.strUID == user.strUID )
					pQuery = new CMySQLQuery( "SELECT bid,bshare FROM bookmarks WHERE buid=" + user.strUID );
				else
					pQuery = new CMySQLQuery( "SELECT bid,bshare FROM bookmarks WHERE buid=" + user.strUID + " AND bshare=1" );
				
				vector<string> vecQuery;
				vecQuery.reserve(2);
				
				vecQuery = pQuery->nextRow( );
				
				if( vecQuery.size( ) == 2 )
				{
					pResponse->strContent += "<tr>\n";

					if( !m_vecTags.empty( ) )
					{
						pResponse->strContent += "<th class=\"tag\" id=\"tagheader\">" + gmapLANG_CFG["tag"];
						pResponse->strContent += "</th>\n";
					}
					
					// Name
					pResponse->strContent += "<th class=\"name\" id=\"nameheader\">" + gmapLANG_CFG["name"];

					pResponse->strContent += "</th>\n";
					
		
					// <th> seeders

					pResponse->strContent += "<th class=\"number\" id=\"seedersheader\">" + gmapLANG_CFG["seeders"];

					pResponse->strContent += "</th>\n";

					// <th> leechers

					pResponse->strContent += "<th class=\"number\" id=\"leechersheader\">" + gmapLANG_CFG["leechers"];

					pResponse->strContent += "</th>\n";
					
					// <th> added

					pResponse->strContent += "<th class=\"date\" id=\"addedheader\">" + gmapLANG_CFG["added"];

					pResponse->strContent += "</th>\n";

					// <th> size

					pResponse->strContent += "<th class=\"bytes\" id=\"sizeheader\">" + gmapLANG_CFG["size"];

					pResponse->strContent += "</th>\n";
					
					// Admin
					if( pRequest->user.strUID == user.strUID )
					{
						pResponse->strContent += "<th class=\"share\" id=\"share\">" + gmapLANG_CFG["share"] + "</th>\n";
						pResponse->strContent += "<th class=\"admin\" id=\"adminheader\">" + gmapLANG_CFG["admin"] + "</th>\n";
					}
	
					while( vecQuery.size( ) == 2 )
					{
						for( unsigned long ulKey = 0; ulKey < culKeySize; ulKey++ )
						{
							if( pTorrents[ulKey].strID == vecQuery[0] )
							{
								pResponse->strContent += "<tr>";
							
								// <td> tag
								vector< pair< string, string > > :: iterator it2 = m_vecTagsMouse.begin( );
								if( !m_vecTags.empty( ) )
								{
									pResponse->strContent += "<td class=\"tag\">\n";
									
									string strNameIndex = string( );
									string strTag = string( );

									for( vector< pair< string, string > > :: iterator it1 = m_vecTags.begin( ); it1 != m_vecTags.end( ); it1++ )
									{
										strNameIndex = (*it1).first;
										strTag = (*it1).second;
										if( strNameIndex == pTorrents[ulKey].strTag )
										{

											if( !(*it2).second.empty( ) )

												// Sets a TITLE parameter for each Tag Image, so that a TOOLTIP box will popup when
												// the user's mouse pointer hovers over the Tag Image.

												pResponse->strContent += "<img class=\"tag\" src=\"" + (*it2).second + "\" alt=\"[" + strTag + "]\" title=\"" + strTag + "\" name=\"" + strTag + "\">";

											break;
										}
										it2++;

									}
								}

								pResponse->strContent += "</td>\n";
								
								// <td> name
								
								int64 day_left = -1, hour_left = -1, minute_left = -1;
								
								time_t tTimeFree = pTorrents[ulKey].iFreeTo - now_t;
				
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
						
//								if( pTorrents[ulKey].iFreeTo > now_t )
//								{
//									day_left = ( pTorrents[ulKey].iFreeTo - now_t + 60 ) / 86400;
//									hour_left = ( pTorrents[ulKey].iFreeTo - now_t + 60 ) % 86400 / 3600;
//									minute_left = ( pTorrents[ulKey].iFreeTo - now_t + 60 ) % 3600 / 60;

//								}
								pResponse->strContent += "<td class=\"torrent_name\">";
								strEngName.erase( );
								strChiName.erase( );
								UTIL_StripName( pTorrents[ulKey].strName.c_str( ), strEngName, strChiName );
								if( pTorrents[ulKey].iFreeDown == 0 )
									pResponse->strContent += "<a class=\"stats_free\" title=\"";
								else
									pResponse->strContent += "<a class=\"stats\" title=\"";
								pResponse->strContent += gmapLANG_CFG["name"] + ": " + UTIL_RemoveHTML( pTorrents[ulKey].strName )+ "\" href=\"" + RESPONSE_STR_STATS_HTML + "?id=" + pTorrents[ulKey].strID + "\">";
								pResponse->strContent += UTIL_RemoveHTML( strEngName );
								if( !strChiName.empty( ) )
									pResponse->strContent += "<br><span class=\"stats\">" + UTIL_RemoveHTML( strChiName ) + "</span>";
								pResponse->strContent += "</a>";
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
										pResponse->strContent += "<span class=\"free_up\" title=\"" + UTIL_Xsprintf( gmapLANG_CFG["free_up"].c_str( ), CAtomInt( pTorrents[ulKey].iFreeUp ).toString( ).c_str( ) ) + "\">" + UTIL_Xsprintf( gmapLANG_CFG["free_up_short"].c_str( ), CAtomInt( pTorrents[ulKey].iFreeUp ).toString( ).c_str( ) )+ "</span>";
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
								
								// <td> seeders
								
								pResponse->strContent += "<td class=\"number\">";
								pResponse->strContent += CAtomInt( pTorrents[ulKey].uiSeeders ).toString( );
								pResponse->strContent += "</td>\n";
								
								// <td> leechers
								
								pResponse->strContent += "<td class=\"number\">";
								pResponse->strContent += CAtomInt( pTorrents[ulKey].uiLeechers ).toString( );
								pResponse->strContent += "</td>\n";
								
								// <td> added

// 										if( m_bShowAdded )
// 										{
									pResponse->strContent += "<td class=\"date\">";

									if( !pTorrents[ulKey].strAdded.empty( ) )
									{
										const string :: size_type br = pTorrents[ulKey].strAdded.find( ' ' );
										pResponse->strContent += pTorrents[ulKey].strAdded.substr( 0, br );
										if( br != string :: npos )
											pResponse->strContent += "<br>" +  pTorrents[ulKey].strAdded.substr( br + 1 );
										// strip year and seconds from time
									}

									pResponse->strContent += "</td>\n";
// 										}

								// <td> size

								if( m_bShowSize )
								{
									const string :: size_type br = UTIL_BytesToString( pTorrents[ulKey].iSize ).find( ' ' );
									pResponse->strContent += "<td class=\"bytes\">" + UTIL_BytesToString( pTorrents[ulKey].iSize ).substr( 0, br );
									if( br != string :: npos )
										pResponse->strContent += "<br>" + UTIL_BytesToString( pTorrents[ulKey].iSize ).substr( br + 1 );
									pResponse->strContent += "</td>\n";
								}
								
								if( pRequest->user.strUID == user.strUID )
								{
									if( !vecQuery[1].empty( ) )
									{
										pResponse->strContent += "<td class=\"share\">";
										pResponse->strContent += "[<a id=\"share" + pTorrents[ulKey].strID + "\"";
										if( vecQuery[1] == "1" )
											pResponse->strContent += " class=\"noshare\"";
										else
											pResponse->strContent += " class=\"share\"";
										pResponse->strContent += " href=\"javascript: ;\" onclick=\"javascript: share('" + pTorrents[ulKey].strID + "','" + gmapLANG_CFG["stats_share"] + "','" + gmapLANG_CFG["stats_no_share"] + "');\">";
										if( vecQuery[1] == "1" )
											pResponse->strContent += gmapLANG_CFG["stats_no_share"] + "</a>]";
										else
											pResponse->strContent += gmapLANG_CFG["stats_share"] + "</a>]";
										pResponse->strContent += "</td>\n";
									}
									
									pResponse->strContent += "<td class=\"admin\">";
									
									
									
									pResponse->strContent += "[<a id=\"bookmark" + pTorrents[ulKey].strID + "\" class=\"bookmark\" href=\"javascript: ;\" onclick=\"javascript: bookmark('" + pTorrents[ulKey].strID + "','" + gmapLANG_CFG["stats_bookmark"] + "','" + gmapLANG_CFG["stats_no_bookmark"] + "');\">";
									pResponse->strContent += gmapLANG_CFG["stats_no_bookmark"] + "</a>]";
									pResponse->strContent += "</td>\n";
								}
								
								pResponse->strContent += "</tr>\n";
								
								break;
							}
						}
						vecQuery = pQuery->nextRow( );
					}
				}
				else
					pResponse->strContent += "<tr><td>" + gmapLANG_CFG["result_none_found"] + "</td></tr>\n";
				
				delete pQuery;
				
				pResponse->strContent += "</table>";
			}
			else if( cstrDetailShow == "active" )
			{
				if( ( pRequest->user.ucAccess & m_ucAccessUserDetails ) || pRequest->user.strUID == user.strUID )
				{
					pResponse->strContent += "<table class=\"user_detail_table\" id=\"user_seeding\">\n";
					pResponse->strContent += "<tr><th colspan=9>" + gmapLANG_CFG["user_seeding"] + "</th></tr>\n";
					
					CMySQLQuery *pQuery = 0;
					
					pQuery = new CMySQLQuery( "SELECT bid,buploaded,bdownloaded FROM dstate WHERE buid=" + user.strUID + " AND bleft=0" );
		
					vector<string> vecQuery;
					
					vecQuery.reserve(3);

					vecQuery = pQuery->nextRow( );

					if( vecQuery.size( ) == 3 )
					{
						pResponse->strContent += "<tr>\n";
						
						if( !m_vecTags.empty( ) )
						{
							pResponse->strContent += "<th class=\"tag\" id=\"tagheader\">" + gmapLANG_CFG["tag"];
							pResponse->strContent += "</th>\n";
						}
						
						// Name
						pResponse->strContent += "<th class=\"name\" id=\"nameheader\">" + gmapLANG_CFG["name"];

						pResponse->strContent += "</th>\n";
			
						// <th> seeders

						pResponse->strContent += "<th class=\"number\" id=\"seedersheader\">" + gmapLANG_CFG["seeders"];

						pResponse->strContent += "</th>\n";

						// <th> leechers

						pResponse->strContent += "<th class=\"number\" id=\"leechersheader\">" + gmapLANG_CFG["leechers"];

						pResponse->strContent += "</th>\n";
						
						
						// <th> added

						pResponse->strContent += "<th class=\"date\" id=\"addedheader\">" + gmapLANG_CFG["added"];

						pResponse->strContent += "</th>\n";

						// <th> size

						pResponse->strContent += "<th class=\"bytes\" id=\"sizeheader\">" + gmapLANG_CFG["size"];

						pResponse->strContent += "</th>\n";
						
						// Uploaded

						pResponse->strContent += "<th class=\"bytes\">" + gmapLANG_CFG["uploaded"];

						// Downloaded
						pResponse->strContent += "</th>\n";

						pResponse->strContent += "<th class=\"bytes\">" + gmapLANG_CFG["downloaded"];

						pResponse->strContent += "</th>\n";
						
						if( m_bShowShareRatios )
							pResponse->strContent += "<th class=\"number\">" + gmapLANG_CFG["share_ratio"] + "</th>\n</tr>\n";
						
						while( vecQuery.size( ) == 3 )
						{
							for( unsigned long ulKey = 0; ulKey < culKeySize; ulKey++ )
							{
								if( pTorrents[ulKey].strID == vecQuery[0] )
								{
									pResponse->strContent += "<tr>";
								
									// <td> tag
									
									vector< pair< string, string > > :: iterator it2 = m_vecTagsMouse.begin( );
									if( !m_vecTags.empty( ) )
									{
										pResponse->strContent += "<td class=\"tag\">\n";
										
// 												string :: size_type iStart = 0;
										string strNameIndex = string( );
										string strTag = string( );

										for( vector< pair< string, string > > :: iterator it1 = m_vecTags.begin( ); it1 != m_vecTags.end( ); it1++ )
										{
// 													iStart = (*it2).first.find( "_" );
											strNameIndex = (*it1).first;
											strTag = (*it1).second;
											if( strNameIndex == pTorrents[ulKey].strTag )
											{


												if( !(*it2).second.empty( ) )

													// Sets a TITLE parameter for each Tag Image, so that a TOOLTIP box will popup when
													// the user's mouse pointer hovers over the Tag Image.

													pResponse->strContent += "<img class=\"tag\" src=\"" + (*it2).second + "\" alt=\"[" + strTag + "]\" title=\"" + strTag + "\" name=\"" + strTag + "\">";

												break;
											}
											it2++;

										}
									}

									pResponse->strContent += "</td>\n";
									
									// <td> name
									
									int64 day_left = -1, hour_left = -1, minute_left = -1;
									
									time_t tTimeFree = pTorrents[ulKey].iFreeTo - now_t;
				
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
							
//									if( pTorrents[ulKey].iFreeTo > now_t )
//									{
//										day_left = ( pTorrents[ulKey].iFreeTo - now_t + 60 ) / 86400;
//										hour_left = ( pTorrents[ulKey].iFreeTo - now_t + 60 ) % 86400 / 3600;
//										minute_left = ( pTorrents[ulKey].iFreeTo - now_t + 60 ) % 3600 / 60;
//									}
									pResponse->strContent += "<td class=\"torrent_name\">";
									strEngName.erase( );
									strChiName.erase( );
									UTIL_StripName( pTorrents[ulKey].strName.c_str( ), strEngName, strChiName );
									if( pTorrents[ulKey].iFreeDown == 0 )
										pResponse->strContent += "<a class=\"stats_free\" title=\"";
									else
										pResponse->strContent += "<a class=\"stats\" title=\"";
									pResponse->strContent += gmapLANG_CFG["name"] + ": " + UTIL_RemoveHTML( pTorrents[ulKey].strName )+ "\" href=\"" + RESPONSE_STR_STATS_HTML + "?id=" + pTorrents[ulKey].strID + "\">";
									pResponse->strContent += UTIL_RemoveHTML( strEngName );
									if( !strChiName.empty( ) )
										pResponse->strContent += "<br><span class=\"stats\">" + UTIL_RemoveHTML( strChiName ) + "</span>";
									pResponse->strContent += "</a>";
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
											pResponse->strContent += "<span class=\"free_up\" title=\"" + UTIL_Xsprintf( gmapLANG_CFG["free_up"].c_str( ), CAtomInt( pTorrents[ulKey].iFreeUp ).toString( ).c_str( ) ) + "\">" + UTIL_Xsprintf( gmapLANG_CFG["free_up_short"].c_str( ), CAtomInt( pTorrents[ulKey].iFreeUp ).toString( ).c_str( ) )+ "</span>";
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
									
									// <td> seeders
									
									pResponse->strContent += "<td class=\"number\">";
									pResponse->strContent += CAtomInt( pTorrents[ulKey].uiSeeders ).toString( );
									pResponse->strContent += "</td>\n";
									
									// <td> leechers
									
									pResponse->strContent += "<td class=\"number\">";
									pResponse->strContent += CAtomInt( pTorrents[ulKey].uiLeechers ).toString( );
									pResponse->strContent += "</td>\n";
									
									// <td> added

// 									if( m_bShowAdded )
// 									{
										pResponse->strContent += "<td class=\"date\">";

										if( !pTorrents[ulKey].strAdded.empty( ) )
										{
											const string :: size_type br = pTorrents[ulKey].strAdded.find( ' ' );
											pResponse->strContent += pTorrents[ulKey].strAdded.substr( 0, br );
											if( br != string :: npos )
												pResponse->strContent += "<br>" +  pTorrents[ulKey].strAdded.substr( br + 1 );
											// strip year and seconds from time
										}

										pResponse->strContent += "</td>\n";
// 									}

									// <td> size

									if( m_bShowSize )
									{
										const string :: size_type br = UTIL_BytesToString( pTorrents[ulKey].iSize ).find( ' ' );
										pResponse->strContent += "<td class=\"bytes\">" + UTIL_BytesToString( pTorrents[ulKey].iSize ).substr( 0, br );
										if( br != string :: npos )
											pResponse->strContent += "<br>" + UTIL_BytesToString( pTorrents[ulKey].iSize ).substr( br + 1 );
										pResponse->strContent +="</td>\n";
									}
									
									// Uploaded
									int64 iUpped = 0;
									if( !vecQuery[1].empty( ) )
										iUpped = UTIL_StringTo64( vecQuery[1].c_str( ) );
									pResponse->strContent += "<td class=\"bytes\">" + UTIL_BytesToString( iUpped ) + "</td>\n";

									// Downloaded
									int64 iDowned = 0;
									float flShareRatio = 0.0;
									if( !vecQuery[2].empty( ) )
									{
										iDowned = UTIL_StringTo64( vecQuery[2].c_str( ) );

										if( m_bShowShareRatios )
										{
											if( iDowned > 0 )
												flShareRatio = (float)iUpped / (float)iDowned;
											else if( iUpped == 0 )
												flShareRatio = 0.0;
											else
												flShareRatio = -1.0;
										}
									}
									pResponse->strContent += "<td class=\"bytes\">" + UTIL_BytesToString( iDowned ) + "</td>\n";

									// Share ratios
									char szFloat[16];
									if( m_bShowShareRatios )
									{
										pResponse->strContent += "<td class=\"number\">";

										if( ( -1.001 < flShareRatio ) && ( flShareRatio < -0.999 ) )
											pResponse->strContent += gmapLANG_CFG["perfect"];
										else
										{
											memset( szFloat, 0, sizeof( szFloat ) / sizeof( char ) );
											snprintf( szFloat, sizeof( szFloat ) / sizeof( char ), "%0.3f", flShareRatio );

											pResponse->strContent += szFloat;
										}

										pResponse->strContent += "</td>\n";
									}
									pResponse->strContent += "</tr>\n";
									
									break;
								}
							}
							vecQuery = pQuery->nextRow( );
						}
					}
					else
						pResponse->strContent += "<tr><td>" + gmapLANG_CFG["result_none_found"] + "</td></tr>\n";
					
					delete pQuery;
					
					pResponse->strContent += "</table>\n<p>\n";
					
					pResponse->strContent += "<table class=\"user_detail_table\" id=\"user_leeching\">\n";
					pResponse->strContent += "<tr><th colspan=9>" + gmapLANG_CFG["user_leeching"] + "</th></tr>\n";
					
					pQuery = new CMySQLQuery( "SELECT bid,buploaded,bdownloaded FROM dstate WHERE buid=" + user.strUID + " AND bleft!=0" );
					
					vecQuery = pQuery->nextRow( );
					
					if( vecQuery.size( ) == 3 )
					{
						pResponse->strContent += "<tr>\n";

						if( !m_vecTags.empty( ) )
						{
							pResponse->strContent += "<th class=\"tag\" id=\"tagheader\">" + gmapLANG_CFG["tag"];
							pResponse->strContent += "</th>\n";
						}
						
						// Name
						pResponse->strContent += "<th class=\"name\" id=\"nameheader\">" + gmapLANG_CFG["name"];

						pResponse->strContent += "</th>\n";
						
			
						// <th> seeders

						pResponse->strContent += "<th class=\"number\" id=\"seedersheader\">" + gmapLANG_CFG["seeders"];

						pResponse->strContent += "</th>\n";

						// <th> leechers

						pResponse->strContent += "<th class=\"number\" id=\"leechersheader\">" + gmapLANG_CFG["leechers"];

						pResponse->strContent += "</th>\n";
						
						// <th> added

						pResponse->strContent += "<th class=\"date\" id=\"addedheader\">" + gmapLANG_CFG["added"];

						pResponse->strContent += "</th>\n";

						// <th> size

						pResponse->strContent += "<th class=\"bytes\" id=\"sizeheader\">" + gmapLANG_CFG["size"];

						pResponse->strContent += "</th>\n";
						
						// Uploaded
						pResponse->strContent += "<th class=\"bytes\">" + gmapLANG_CFG["uploaded"];
						
						pResponse->strContent += "</th>\n";

						// Downloaded
						pResponse->strContent += "<th class=\"bytes\">" + gmapLANG_CFG["downloaded"];

						pResponse->strContent += "</th>\n";
						
						// Share ratios
						pResponse->strContent += "<th class=\"number\">" + gmapLANG_CFG["share_ratio"];

						pResponse->strContent += "</th>\n</tr>\n";
						
						while( vecQuery.size( ) == 3 )
						{
							for( unsigned long ulKey = 0; ulKey < culKeySize; ulKey++ )
							{
								if( pTorrents[ulKey].strID == vecQuery[0] )
								{
									pResponse->strContent += "<tr>";
								
									// <td> tag
									
									vector< pair< string, string > > :: iterator it2 = m_vecTagsMouse.begin( );
									if( !m_vecTags.empty( ) )
									{
										pResponse->strContent += "<td class=\"tag\">\n";
										
										string strNameIndex = string( );
										string strTag = string( );

										for( vector< pair< string, string > > :: iterator it1 = m_vecTags.begin( ); it1 != m_vecTags.end( ); it1++ )
										{
											strNameIndex = (*it1).first;
											strTag = (*it1).second;
											if( strNameIndex == pTorrents[ulKey].strTag )
											{

												if( !(*it2).second.empty( ) )

													// Sets a TITLE parameter for each Tag Image, so that a TOOLTIP box will popup when
													// the user's mouse pointer hovers over the Tag Image.

													pResponse->strContent += "<img class=\"tag\" src=\"" + (*it2).second + "\" alt=\"[" + strTag + "]\" title=\"" + strTag + "\" name=\"" + strTag + "\">";

												break;
											}
											it2++;

										}
									}

									pResponse->strContent += "</td>\n";
									
									// <td> name
									
									int64 day_left = -1, hour_left = -1, minute_left = -1;
									
									time_t tTimeFree = pTorrents[ulKey].iFreeTo - now_t;
				
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
							
//									if( pTorrents[ulKey].iFreeTo > now_t )
//									{
//										day_left = ( pTorrents[ulKey].iFreeTo - now_t + 60 ) / 86400;
//										hour_left = ( pTorrents[ulKey].iFreeTo - now_t + 60 ) % 86400 / 3600;
//										minute_left = ( pTorrents[ulKey].iFreeTo - now_t + 60 ) % 3600 / 60;
//									}
									pResponse->strContent += "<td class=\"torrent_name\">";
									strEngName.erase( );
									strChiName.erase( );
									UTIL_StripName( pTorrents[ulKey].strName.c_str( ), strEngName, strChiName );
									if( pTorrents[ulKey].iFreeDown == 0 )
										pResponse->strContent += "<a class=\"stats_free\" title=\"";
									else
										pResponse->strContent += "<a class=\"stats\" title=\"";
									pResponse->strContent += gmapLANG_CFG["name"] + ": " + UTIL_RemoveHTML( pTorrents[ulKey].strName )+ "\" href=\"" + RESPONSE_STR_STATS_HTML + "?id=" + pTorrents[ulKey].strID + "\">";
									pResponse->strContent += UTIL_RemoveHTML( strEngName );
									if( !strChiName.empty( ) )
										pResponse->strContent += "<br><span class=\"stats\">" + UTIL_RemoveHTML( strChiName ) + "</span>";
									pResponse->strContent += "</a>";
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
											pResponse->strContent += "<span class=\"free_up\" title=\"" + UTIL_Xsprintf( gmapLANG_CFG["free_up"].c_str( ), CAtomInt( pTorrents[ulKey].iFreeUp ).toString( ).c_str( ) ) + "\">" + UTIL_Xsprintf( gmapLANG_CFG["free_up_short"].c_str( ), CAtomInt( pTorrents[ulKey].iFreeUp ).toString( ).c_str( ) )+ "</span>";
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
									
									// <td> seeders
									
									pResponse->strContent += "<td class=\"number\">";
									pResponse->strContent += CAtomInt( pTorrents[ulKey].uiSeeders ).toString( );
									pResponse->strContent += "</td>\n";
									
									// <td> leechers
									
									pResponse->strContent += "<td class=\"number\">";
									pResponse->strContent += CAtomInt( pTorrents[ulKey].uiLeechers ).toString( );
									pResponse->strContent += "</td>\n";
									
									// <td> added

// 									if( m_bShowAdded )
// 									{
										pResponse->strContent += "<td class=\"date\">";


										if( !pTorrents[ulKey].strAdded.empty( ) )
										{
											const string :: size_type br = pTorrents[ulKey].strAdded.find( ' ' );
											pResponse->strContent += pTorrents[ulKey].strAdded.substr( 0, br );
											if( br != string :: npos )
												pResponse->strContent += "<br>" +  pTorrents[ulKey].strAdded.substr( br + 1 );
											// strip year and seconds from time
										}

										pResponse->strContent += "</td>\n";
// 									}

									// <td> size

									if( m_bShowSize )
									{
										const string :: size_type br = UTIL_BytesToString( pTorrents[ulKey].iSize ).find( ' ' );
										pResponse->strContent += "<td class=\"bytes\">" + UTIL_BytesToString( pTorrents[ulKey].iSize ).substr( 0, br );
										if( br != string :: npos )
											pResponse->strContent += "<br>" + UTIL_BytesToString( pTorrents[ulKey].iSize ).substr( br + 1 );
										pResponse->strContent += "</td>\n";
									}
									
									// Uploaded
									int64 iUpped = 0;
									if( !vecQuery[1].empty( ) )
										iUpped = UTIL_StringTo64( vecQuery[1].c_str( ) );
									pResponse->strContent += "<td class=\"bytes\">" + UTIL_BytesToString( iUpped ) + "</td>\n";

									// Downloaded
									int64 iDowned = 0;
									float flShareRatio = 0.0;
									if( !vecQuery[2].empty( ) )
									{
										iDowned = UTIL_StringTo64( vecQuery[2].c_str( ) );

										if( m_bShowShareRatios )
										{
											if( iDowned > 0 )
												flShareRatio = (float)iUpped / (float)iDowned;
											else if( iUpped == 0 )
												flShareRatio = 0.0;
											else
												flShareRatio = -1.0;
										}
									}
									pResponse->strContent += "<td class=\"bytes\">" + UTIL_BytesToString( iDowned ) + "</td>\n";

									// Share ratios
									char szFloat[16];
									if( m_bShowShareRatios )
									{
										pResponse->strContent += "<td class=\"number\">";

										if( ( -1.001 < flShareRatio ) && ( flShareRatio < -0.999 ) )
											pResponse->strContent += gmapLANG_CFG["perfect"];
										else
										{
											memset( szFloat, 0, sizeof( szFloat ) / sizeof( char ) );
											snprintf( szFloat, sizeof( szFloat ) / sizeof( char ), "%0.3f", flShareRatio );

											pResponse->strContent += szFloat;
										}

										pResponse->strContent += "</td>\n";
									}
									pResponse->strContent += "</tr>\n";
									
									break;
								}
							}
							vecQuery = pQuery->nextRow( );
						}
					}
					else
						pResponse->strContent += "<tr><td>" + gmapLANG_CFG["result_none_found"] + "</td></tr>\n";
					
					delete pQuery;
					
					pResponse->strContent += "</table>\n";
				}
			}
			else if( cstrDetailShow == "completed" )
			{
				if( ( pRequest->user.ucAccess & m_ucAccessUserDetails ) || pRequest->user.strUID == user.strUID )
				{
					pResponse->strContent += "<table class=\"user_detail_table\" id=\"user_completed\">\n";
					pResponse->strContent += "<tr><th colspan=9>" + gmapLANG_CFG["user_completed"] + "</th></tr>\n";
					
					CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bid,buploaded,bdownloaded,bcompleted FROM peers WHERE buid=" +  user.strUID + " AND bcompleted!=0 ORDER BY bcompleted DESC" );
					
					vector<string> vecQuery;
					vecQuery.reserve(4);
					
					vecQuery = pQuery->nextRow( );
					
					if( vecQuery.size( ) == 4 )
					{
						pResponse->strContent += "<tr>\n";

						if( !m_vecTags.empty( ) )
						{
							pResponse->strContent += "<th class=\"tag\" id=\"tagheader\">" + gmapLANG_CFG["tag"];
							pResponse->strContent += "</th>\n";
						}
						
						// Name
						pResponse->strContent += "<th class=\"name\" id=\"nameheader\">" + gmapLANG_CFG["name"];

						pResponse->strContent += "</th>\n";
						
			
						// <th> seeders

						pResponse->strContent += "<th class=\"number\" id=\"seedersheader\">" + gmapLANG_CFG["seeders"];

						pResponse->strContent += "</th>\n";

						// <th> leechers

						pResponse->strContent += "<th class=\"number\" id=\"leechersheader\">" + gmapLANG_CFG["leechers"];

						pResponse->strContent += "</th>\n";
						
// 						// <th> added

// 						pResponse->strContent += "<th class=\"date\" id=\"addedheader\">" + gmapLANG_CFG["added"];
						
						// <th> started

//							pResponse->strContent += "<th class=\"date\" id=\"startedheader\">" + gmapLANG_CFG["user_detail_start"];
//							pResponse->strContent += "</th>\n";
						
						// <th> completed

						pResponse->strContent += "<th class=\"date\" id=\"completedheader\">" + gmapLANG_CFG["user_detail_complete"];

						pResponse->strContent += "</th>\n";

						// <th> size


						pResponse->strContent += "<th class=\"bytes\" id=\"sizeheader\">" + gmapLANG_CFG["size"];

						pResponse->strContent += "</th>\n";
						
						// Uploaded
						pResponse->strContent += "<th class=\"bytes\">" + gmapLANG_CFG["uploaded"];
						
						pResponse->strContent += "</th>\n";

						// Downloaded
						pResponse->strContent += "<th class=\"bytes\">" + gmapLANG_CFG["downloaded"];

						pResponse->strContent += "</th>\n";
						
						// Share ratios
						pResponse->strContent += "<th class=\"number\">" + gmapLANG_CFG["share_ratio"];

						pResponse->strContent += "</th>\n</tr>\n";
		
						while( vecQuery.size( ) == 4 )
						{
							for( unsigned long ulKey = 0; ulKey < culKeySize; ulKey++ )
							{
								if( pTorrents[ulKey].strID == vecQuery[0] )
								{
									pResponse->strContent += "<tr>";
								
									// <td> tag
									vector< pair< string, string > > :: iterator it2 = m_vecTagsMouse.begin( );
									if( !m_vecTags.empty( ) )
									{
										pResponse->strContent += "<td class=\"tag\">\n";
										
										string strNameIndex = string( );
										string strTag = string( );

										for( vector< pair< string, string > > :: iterator it1 = m_vecTags.begin( ); it1 != m_vecTags.end( ); it1++ )
										{
											strNameIndex = (*it1).first;
											strTag = (*it1).second;
											if( strNameIndex == pTorrents[ulKey].strTag )
											{

												if( !(*it2).second.empty( ) )

													// Sets a TITLE parameter for each Tag Image, so that a TOOLTIP box will popup when
													// the user's mouse pointer hovers over the Tag Image.

													pResponse->strContent += "<img class=\"tag\" src=\"" + (*it2).second + "\" alt=\"[" + strTag + "]\" title=\"" + strTag + "\" name=\"" + strTag + "\">";

												break;
											}
											it2++;

										}
									}


									pResponse->strContent += "</td>\n";
									
									// <td> name
									
									int64 day_left = -1, hour_left = -1, minute_left = -1;
									
									time_t tTimeFree = pTorrents[ulKey].iFreeTo - now_t;
				
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
							




//									if( pTorrents[ulKey].iFreeTo > now_t )
//									{
//										day_left = ( pTorrents[ulKey].iFreeTo - now_t + 60 ) / 86400;
//										hour_left = ( pTorrents[ulKey].iFreeTo - now_t + 60 ) % 86400 / 3600;
//										minute_left = ( pTorrents[ulKey].iFreeTo - now_t + 60 ) % 3600 / 60;
//									}
									pResponse->strContent += "<td class=\"torrent_name\">";
									strEngName.erase( );
									strChiName.erase( );
									UTIL_StripName( pTorrents[ulKey].strName.c_str( ), strEngName, strChiName );
									if( pTorrents[ulKey].iFreeDown == 0 )
										pResponse->strContent += "<a class=\"stats_free\" title=\"";
									else
										pResponse->strContent += "<a class=\"stats\" title=\"";
									pResponse->strContent += gmapLANG_CFG["name"] + ": " + UTIL_RemoveHTML( pTorrents[ulKey].strName )+ "\" href=\"" + RESPONSE_STR_STATS_HTML + "?id=" + pTorrents[ulKey].strID + "\">";

									pResponse->strContent += UTIL_RemoveHTML( strEngName );
									if( !strChiName.empty( ) )
										pResponse->strContent += "<br><span class=\"stats\">" + UTIL_RemoveHTML( strChiName ) + "</span>";
									pResponse->strContent += "</a>";
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

											pResponse->strContent += "<span class=\"free_up\" title=\"" + UTIL_Xsprintf( gmapLANG_CFG["free_up"].c_str( ), CAtomInt( pTorrents[ulKey].iFreeUp ).toString( ).c_str( ) ) + "\">" + UTIL_Xsprintf( gmapLANG_CFG["free_up_short"].c_str( ), CAtomInt( pTorrents[ulKey].iFreeUp ).toString( ).c_str( ) )+ "</span>";
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
									
									// <td> seeders
									
									pResponse->strContent += "<td class=\"number\">";
									pResponse->strContent += CAtomInt( pTorrents[ulKey].uiSeeders ).toString( );
									pResponse->strContent += "</td>\n";
									
									// <td> leechers
									
									pResponse->strContent += "<td class=\"number\">";
									pResponse->strContent += CAtomInt( pTorrents[ulKey].uiLeechers ).toString( );
									pResponse->strContent += "</td>\n";
									
									// <td> started

//										pResponse->strContent += "<td class=\"date\">";

//										if( !vecQuery[3].empty( ) )
//										{
//											const string :: size_type br = vecQuery[3].find( ' ' );
//											pResponse->strContent += vecQuery[3].substr( 0, br );
//											if( br != string :: npos )
//												pResponse->strContent += "<br>" +  vecQuery[3].substr( br + 1 );
//											// strip year and seconds from time
//										}

//										pResponse->strContent += "</td>\n";
									
									// <td> completed

									pResponse->strContent += "<td class=\"date\">";

									if( !vecQuery[3].empty( ) )
									{
										const string :: size_type br = vecQuery[3].find( ' ' );
										pResponse->strContent += vecQuery[3].substr( 0, br );
										if( br != string :: npos )
											pResponse->strContent += "<br>" +  vecQuery[3].substr( br + 1 );
										// strip year and seconds from time
									}

									pResponse->strContent += "</td>\n";

									// <td> size

									if( m_bShowSize )
									{
										const string :: size_type br = UTIL_BytesToString( pTorrents[ulKey].iSize ).find( ' ' );
										pResponse->strContent += "<td class=\"bytes\">" + UTIL_BytesToString( pTorrents[ulKey].iSize ).substr( 0, br );
										if( br != string :: npos )
											pResponse->strContent += "<br>" + UTIL_BytesToString( pTorrents[ulKey].iSize ).substr( br + 1 );
										pResponse->strContent += "</td>\n";
									}
									
									// Uploaded
									int64 iUpped = 0;
									if( !vecQuery[1].empty( ) )
										iUpped = UTIL_StringTo64( vecQuery[1].c_str( ) );
									pResponse->strContent += "<td class=\"bytes\">" + UTIL_BytesToString( iUpped ) + "</td>\n";

									// Downloaded
									int64 iDowned = 0;
									float flShareRatio = 0.0;
									if( !vecQuery[2].empty( ) )
									{
										iDowned = UTIL_StringTo64( vecQuery[2].c_str( ) );

										if( m_bShowShareRatios )
										{
											if( iDowned > 0 )
												flShareRatio = (float)iUpped / (float)iDowned;
											else if( iUpped == 0 )
												flShareRatio = 0.0;
											else
												flShareRatio = -1.0;
										}
									}
									pResponse->strContent += "<td class=\"bytes\">" + UTIL_BytesToString( iDowned ) + "</td>\n";

									// Share ratios
									char szFloat[16];
									if( m_bShowShareRatios )
									{
										pResponse->strContent += "<td class=\"number\">";

										if( ( -1.001 < flShareRatio ) && ( flShareRatio < -0.999 ) )
											pResponse->strContent += gmapLANG_CFG["perfect"];
										else
										{
											memset( szFloat, 0, sizeof( szFloat ) / sizeof( char ) );
											snprintf( szFloat, sizeof( szFloat ) / sizeof( char ), "%0.3f", flShareRatio );

											pResponse->strContent += szFloat;
										}

										pResponse->strContent += "</td>\n";
									}
									pResponse->strContent += "</tr>\n";
									
									break;
								}
							}
							vecQuery = pQuery->nextRow( );
						}
					}
					else
						pResponse->strContent += "<tr><td>" + gmapLANG_CFG["result_none_found"] + "</td></tr>\n";
						
					delete pQuery;
					
					pResponse->strContent += "</table>";
				}
			}
		}
		
		pResponse->strContent += "</td>\n</tr>\n</table>\n";
	}

	// Output common HTML tail
	HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_LOGIN ) );
}

void CTracker :: serverResponseLoginPOST( struct request_t *pRequest, struct response_t *pResponse, CAtomList *pPost )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
		if( IsIPBanned( pRequest, pResponse, btv, gmapLANG_CFG["login_page"], string( CSS_LOGIN ), NOT_INDEX ) )
			return;
		
	string cstrUsername = string( );
	string cstrPassword = string( );
	string cstrReturnPage = string( );
	string cstrFilter = string( );
	string cstrTorrentsPerPage = string( );
	string cstrTorrentsAdded = string( );
	string cstrMessagesNewComment = string( );
	string cstrMessagesNewCommentBookmarked = string( );
	string cstrMessagesSaveSent = string( );
	string cstrSubmitLogin = string( );
	string cstrSubmitPrefs = string( );
	bool bExpires = true;

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
						
						if( strName == "username" )
							cstrUsername = pData->toString( );
						else if( strName == "password" )
							cstrPassword = pData->toString( );
						else if( strName == "expires" && pData->toString( ) == "on" )
							bExpires = false;
						else if( strName == "return" )
							cstrReturnPage = pData->toString( );
						else if( strName.substr( 0, 3 ) == "tag" && pData->toString( ) == "on" )
						{
							if( !cstrFilter.empty( ) )
								cstrFilter += " ";
							cstrFilter += strName.substr( 3 );
						}
						else if( strName == "torrents_per_page" )
							cstrTorrentsPerPage = pData->toString( );
						else if( strName == "torrents_added" )
							cstrTorrentsAdded = pData->toString( );
						else if( strName == "messages_new_comment" )
							cstrMessagesNewComment = pData->toString( );
						else if( strName == "messages_new_comment_bookmarked" )
							cstrMessagesNewCommentBookmarked = pData->toString( );
						else if( strName == "messages_save_sent" )
							cstrMessagesSaveSent = pData->toString( );
						else if( strName == "submit_login_button" )
							cstrSubmitLogin = pData->toString( );
						else if( strName == "submit_user_preferences_button" )
							cstrSubmitPrefs = pData->toString( );
					}
					else
					{
						// Output common HTML head
						HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["login_page"], string( CSS_LOGIN ), string( ), NOT_INDEX, CODE_400 );

						// failed
						pResponse->strContent += "<p class=\"failed\">" + gmapLANG_CFG["failed"] + "</p>\n";
						// Signal a bad request
						pResponse->strContent += "<p class=\"body_upload\">400 " + gmapLANG_CFG["server_response_400"] + "</p>\n";

						// Output common HTML tail
						HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_LOGIN ) );

						if( gbDebug )
							UTIL_LogPrint( "Login Warning - Bad request (no login name)\n" );

						return;
					}
				}
			}
		}
	}
	else
	{
		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["login_page"], string( CSS_LOGIN ), string( ), NOT_INDEX, CODE_400 );

		// failed
		pResponse->strContent += "<p class=\"failed\">" + gmapLANG_CFG["failed"] + "</p>\n";
		// Signal a bad request
		pResponse->strContent += "<p class=\"body_upload\">400 " + gmapLANG_CFG["server_response_400"] + "</p>\n";

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_LOGIN ) );

		if( gbDebug )
			UTIL_LogPrint( "Upload Warning - Bad request (no post received)\n" );

		return;
	}
		
	if( pRequest->user.strUID.empty( ) )
	{
		if( cstrSubmitLogin == gmapLANG_CFG["login"] )
		{
			string cstrLogin = string( );
			string cstrID = string( );
			
			string strMD5 = string( );
			string strMD5Recover = string( );
			
			unsigned char ucAccess = ACCESS_VIEW;
			
			CMySQLQuery *pQuery = new CMySQLQuery( "SELECT busername,baccess FROM users WHERE busername=\'" + UTIL_StringToMySQL( cstrUsername ) + "\'" );
	
			vector<string> vecQuery;
	
			vecQuery.reserve(2);

			vecQuery = pQuery->nextRow( );
	
			delete pQuery;
			
			if( vecQuery.size( ) == 2 )
			{
				ucAccess = (unsigned char)atoi( vecQuery[1].c_str( ) );
				
				if( ucAccess & ACCESS_VIEW )
				{
					cstrLogin = vecQuery[0];
					const string cstrA1( cstrLogin + ":" + gstrPasswordKey + ":" + cstrPassword );
					const string cstrA2( cstrLogin + ":" + cstrPassword );

					unsigned char szMD5[16];
					memset( szMD5, 0, sizeof( szMD5 ) / sizeof( unsigned char ) );

					MD5_CTX md5;

					MD5Init( &md5 );
					MD5Update( &md5, (const unsigned char *)cstrA1.c_str( ), (unsigned int)cstrA1.size( ) );
					MD5Final( szMD5, &md5 );
			
					strMD5 = string( (char *)szMD5, sizeof(szMD5) / sizeof(unsigned char) );
			
					cstrID = checkUserMD5( cstrLogin, strMD5 );

					MD5Init( &md5 );
					MD5Update( &md5, (const unsigned char *)cstrA2.c_str( ), (unsigned int)cstrA2.size( ) );
					MD5Final( szMD5, &md5 );
					strMD5Recover = string( (char *)szMD5, sizeof(szMD5) / sizeof(unsigned char) );
				}
			}
			
			if( !cstrLogin.empty( ) && !( cstrID.empty( ) ) )
			{
				CMySQLQuery mq01( "UPDATE users SET bmd5_recover=\'" + UTIL_StringToMySQL( strMD5Recover ) + "\' WHERE buid=" + cstrID );

				// Set the current time
				time_t tNow = time( 0 );

				// Set a future time
				struct tm tmFuture = *gmtime( &tNow );
				tmFuture.tm_mon++;
				mktime( &tmFuture );

				char pTime[256];
				memset( pTime, 0, sizeof( pTime ) / sizeof( char ) );
				
				strftime( pTime, sizeof( char ) * sizeof( pTime ), "%a, %d-%b-%Y %H:%M:%S GMT", &tmFuture );

				if( cstrReturnPage.empty( ) )
					cstrReturnPage = RESPONSE_STR_INDEX_HTML;
					
				HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["login_page"], string( CSS_LOGIN ), cstrReturnPage, NOT_INDEX, CODE_200 );
				
				// Tell the browser not to cache
				pResponse->mapHeaders.insert( pair<string, string>( "Pragma", "No-Cache" ) );

				if( bExpires )
				{
					// Set the users cookie login
					pResponse->mapHeaders.insert( pair<string, string>( "Set-Cookie", "login=\"" + UTIL_StringToEscaped( cstrUsername ) + "\"; path=/" ) );
					// Set the users cookie uid
					pResponse->mapHeaders.insert( pair<string, string>( "Set-Cookie", "uid=\"" + cstrID + "\"; path=/" ) );
					// Set the users cookie password
					pResponse->mapHeaders.insert( pair<string, string>( "Set-Cookie", "md5=\"" + UTIL_StringToEscaped( strMD5 ) + "\"; path=/" ) );
				}
				else
				{
					// Set the users cookie login
					pResponse->mapHeaders.insert( pair<string, string>( "Set-Cookie", "login=\"" + UTIL_StringToEscaped( cstrUsername ) + "\"; expires=" + pTime + "; path=/" ) );
					// Set the users cookie uid
					pResponse->mapHeaders.insert( pair<string, string>( "Set-Cookie", "uid=\"" + cstrID + "\"; expires=" + pTime + "; path=/" ) );
					// Set the users cookie password
					pResponse->mapHeaders.insert( pair<string, string>( "Set-Cookie", "md5=\"" + UTIL_StringToEscaped( strMD5 ) + "\"; expires=" + pTime + "; path=/" ) );
				}
				
				CMySQLQuery *pQueryPrefs = new CMySQLQuery( "SELECT bperpage FROM users_prefs WHERE buid=" + cstrID );
		
				map<string, string> mapPrefs;

				mapPrefs = pQueryPrefs->nextRowMap( );

				delete pQueryPrefs;
				
				if( !mapPrefs["bperpage"].empty( ) )
				{
					unsigned int uiTorrentsPerPage = (unsigned int)atoi( mapPrefs["bperpage"].c_str( ) );
					if( uiTorrentsPerPage < 1 || uiTorrentsPerPage > m_uiPerPageMax )
						uiTorrentsPerPage = m_uiPerPage;
					if( bExpires )
						pResponse->mapHeaders.insert( pair<string, string>( "Set-Cookie", "per_page=\"" + CAtomInt( uiTorrentsPerPage ).toString( )  + "\"; path=/" ) );
					else
						pResponse->mapHeaders.insert( pair<string, string>( "Set-Cookie", "per_page=\"" + CAtomInt( uiTorrentsPerPage ).toString( )  + "\"; expires=" + pTime + "; path=/" ) );
				}
				else
				{
					if( bExpires )
						pResponse->mapHeaders.insert( pair<string, string>( "Set-Cookie", "per_page=\"" + CAtomInt( m_uiPerPage ).toString( )  + "\"; path=/" ) );
					else
						pResponse->mapHeaders.insert( pair<string, string>( "Set-Cookie", "per_page=\"" + CAtomInt( m_uiPerPage ).toString( )  + "\"; expires=" + pTime + "; path=/" ) );
				}
				
				HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_LOGIN ) );
				
				return;
			}
			else
			{
				const string cstrReturnPage( pRequest->mapParams["return"] );
				
				// Output common HTML head
				HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["login_page"], string( CSS_LOGIN ), string( ), NOT_INDEX, CODE_200 );
				
				pResponse->strContent += "<div class=\"login\">\n";
				pResponse->strContent += "<table class=\"login\">\n";
				pResponse->strContent += "<form method=\"post\" name=\"login\" action=\"" + RESPONSE_STR_LOGIN_HTML + "\" enctype=\"multipart/form-data\">\n";
				if( !cstrReturnPage.empty( ) )
					pResponse->strContent += "<input id=\"id_return\" name=\"return\" type=hidden value=\"" + cstrReturnPage + "\">\n";

				pResponse->strContent += "<tr class=\"login\">\n";
				pResponse->strContent += "<th class=\"login\" colspan=\"2\">\n" + gmapLANG_CFG["login"] + "</th>\n</tr>\n";
				pResponse->strContent += "<tr class=\"login\">\n";
				pResponse->strContent += "<td class=\"login\" colspan=\"2\">\n";
				if( !cstrLogin.empty( ) )
					pResponse->strContent += "<span class=\"red\">" + gmapLANG_CFG["login_failed_password"] + "</span>";
				else
				{
					if( ucAccess & ACCESS_VIEW )
						pResponse->strContent += "<span class=\"red\">" + gmapLANG_CFG["login_failed_username"] + "</span>";
					else
						pResponse->strContent += "<span class=\"red\">" + gmapLANG_CFG["login_failed_banned"] + "</span>";
				}
				pResponse->strContent += "</td>\n</tr>\n";
				pResponse->strContent += "<tr class=\"login\">\n";
				pResponse->strContent += "<th class=\"login\">\n" + gmapLANG_CFG["user_name"] + "</th>\n";
				pResponse->strContent += "<td class=\"login\">\n";
				pResponse->strContent += "<input id=\"id_username\" name=\"username\" alt=\"[" + gmapLANG_CFG["user_name"] + "]\" type=text size=24>\n";
				pResponse->strContent += "</td>\n</tr>\n";
				pResponse->strContent += "<tr class=\"login\">\n";
				pResponse->strContent += "<th class=\"login\">\n" + gmapLANG_CFG["password"] + "</th>\n";
				pResponse->strContent += "<td class=\"login\">\n";
				pResponse->strContent += "<input id=\"id_password\" name=\"password\" alt=\"[" + gmapLANG_CFG["password"] + "]\" type=password size=24>\n";
				pResponse->strContent += "</td>\n</tr>\n";
				pResponse->strContent += "<tr class=\"login\">\n";
				pResponse->strContent += "<td class=\"login\" colspan=\"2\">\n";
				pResponse->strContent += "<input id=\"id_expires\" name=\"expires\" type=checkbox> <label for=\"id_expires\">" + gmapLANG_CFG["expires"] + "</label>\n";
				pResponse->strContent += "</td>\n</tr>\n";
				pResponse->strContent += "<tr class=\"login\">\n";
				pResponse->strContent += "<td class=\"login\" colspan=\"2\">\n";

				// Adds Cancel button beside Create User
				pResponse->strContent += "<div class=\"login_button\">\n";

				pResponse->strContent += Button_Submit( "submit_login", string( gmapLANG_CFG["login"] ) );
				pResponse->strContent += Button_Back( "cancel_login", string( gmapLANG_CFG["cancel"] ) );

				pResponse->strContent += "\n</div>\n</td>\n</tr>\n";
				
				pResponse->strContent += "<tr class=\"login\">\n";
				pResponse->strContent += "<td class=\"login\" colspan=\"2\">\n";
				pResponse->strContent += UTIL_Xsprintf( gmapLANG_CFG["recover_forget"].c_str( ), string( "<a href=\"" + RESPONSE_STR_RECOVER_HTML + "\">" ).c_str( ), string( "</a>" ).c_str( ) );
				pResponse->strContent += "</td>\n</tr>\n";

				// finish
				pResponse->strContent += "</form>\n</table>\n</div>\n";
					
				// Output common HTML tail
				HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_LOGIN ) );

				return;
			}
		}
	}
	else
	{
		if( cstrSubmitPrefs == gmapLANG_CFG["submit"] )
		{
			unsigned char ucTorrentsAdded = 0;
			unsigned int uiTorrentsPerPage = 0;
			
			unsigned char ucMessagesSaveSent = 0;
			unsigned char ucMessagesNewComment = 0;
			unsigned char ucMessagesNewCommentBookmarked = 0;
			
			if( !cstrTorrentsAdded.empty( ) && cstrTorrentsAdded == "passed" )
				ucTorrentsAdded = 1;
			else
				ucTorrentsAdded = 0;
			if( !cstrTorrentsPerPage.empty( ) )
			{
				uiTorrentsPerPage = (unsigned int)atoi( cstrTorrentsPerPage.c_str( ) );

				if( uiTorrentsPerPage < 1 || uiTorrentsPerPage > m_uiPerPageMax )
					uiTorrentsPerPage = 0;
			}
			
			if( !cstrMessagesSaveSent.empty( ) && cstrMessagesSaveSent == "on" )
				ucMessagesSaveSent = 1;
			if( !cstrMessagesNewComment.empty( ) && cstrMessagesNewComment == "on" )
				ucMessagesNewComment = 1;
			if( !cstrMessagesNewCommentBookmarked.empty( ) && cstrMessagesNewCommentBookmarked == "on" )
				ucMessagesNewCommentBookmarked = 1;

			string strQuery( "UPDATE users_prefs SET " );
			strQuery += "bdefaulttag='" + UTIL_StringToMySQL( cstrFilter ) + "'";
			strQuery += ",baddedpassed=" + CAtomInt( ucTorrentsAdded ).toString( );
			strQuery += ",bperpage=" + CAtomInt( uiTorrentsPerPage ).toString( );
			strQuery += ",bsavesent=" + CAtomInt( ucMessagesSaveSent ).toString( );
			strQuery += ",bmsgcomment=" + CAtomInt( ucMessagesNewComment ).toString( );
			strQuery += ",bmsgcommentbm=" + CAtomInt( ucMessagesNewCommentBookmarked ).toString( );
			strQuery += " WHERE buid=" + pRequest->user.strUID;
			
			CMySQLQuery mq01( strQuery );
			
			// Set the current time
			time_t tNow = time( 0 );

			// Set a future time
			struct tm tmFuture = *gmtime( &tNow );
			tmFuture.tm_mon++;
			mktime( &tmFuture );

			char pTime[256];
			memset( pTime, 0, sizeof( pTime ) / sizeof( char ) );
			
			strftime( pTime, sizeof( char ) * sizeof( pTime ), "%a, %d-%b-%Y %H:%M:%S GMT", &tmFuture );
			
			if( uiTorrentsPerPage < 1 || uiTorrentsPerPage > m_uiPerPageMax )
				uiTorrentsPerPage = m_uiPerPage;

			pResponse->mapHeaders.insert( pair<string, string>( "Set-Cookie", "per_page=\"" + CAtomInt( uiTorrentsPerPage ).toString( )  + "\"; expires=" + pTime + "; path=/" ) );
			return JS_ReturnToPage( pRequest, pResponse, LOGIN_HTML + "?show=preferences&saved=1" );
		}
	}	
}
