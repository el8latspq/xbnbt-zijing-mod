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
#include "atom.h"
#include "config.h"
#include "html.h"
#include "md5.h"
#include "sort.h"
#include "tracker.h"
#include "util.h"

void CTracker :: serverResponseLogin( struct request_t *pRequest, struct response_t *pResponse )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
		if( IsIPBanned( pRequest, pResponse, btv, gmapLANG_CFG["login_page"], string( CSS_LOGIN ), NOT_INDEX ) )
			return;
		
	
// 	UTIL_LogPrint( "Logout: (%s)\n", pRequest->mapCookies["logout"].c_str( ) ); ;
	// Not authorized, user must supply a login
	if( pRequest->user.strLogin.empty( ) )
	{
		const string cstrUsername( pRequest->mapParams["username"] );
		const string cstrPassword( pRequest->mapParams["password"] );
		
		if( !cstrUsername.empty( ) && ( pRequest->mapParams["submit_login_button"] == gmapLANG_CFG["login"] ) )
		{

			const string cstrA1( cstrUsername + ":" + gstrRealm + ":" + cstrPassword );

			unsigned char szMD5[16];
			memset( szMD5, 0, sizeof( szMD5 ) / sizeof( unsigned char ) );

			MD5_CTX md5;

			MD5Init( &md5 );
			MD5Update( &md5, (const unsigned char *)cstrA1.c_str( ), (unsigned int)cstrA1.size( ) );
			MD5Final( szMD5, &md5 );
			
			const string strMD5 = string( (char *)szMD5, sizeof(szMD5) / sizeof(unsigned char) );
			
			// checkUser( cstrUsername, strMD5 );
			
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
			
			strftime( pTime, sizeof( char ) * sizeof( pTime ), "%a, %d-%b-%Y %H:%M:%S GMT", &tmFuture );

			const string cstrLogout( pRequest->mapParams["logout"] );

			HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["login_page"], string( CSS_LOGIN ), string( ), NOT_INDEX, CODE_200 );
			
			
			// Tell the browser not to cache
			pResponse->mapHeaders.insert( pair<string, string>( "Pragma", "No-Cache" ) );
			// Set the users cookie login
			pResponse->mapHeaders.insert( pair<string, string>( "Set-Cookie", "login=\"" + cstrUsername + "\"; expires=" + pTime + "; path=/" ) );
			// Set the users cookie password
			pResponse->mapHeaders.insert( pair<string, string>( "Set-Cookie", "md5=\"" + UTIL_StringToEscaped( strMD5 )+ "\"; expires=" + pTime + "; path=/" ) );
			
			pResponse->strContent += "<script type=\"text/javascript\">\n";
			pResponse->strContent += "<!--\n";
			
			pResponse->strContent += "window.location=\"" + RESPONSE_STR_LOGIN_HTML + "\"\n";

			pResponse->strContent += "//-->\n";
			pResponse->strContent += "</script>\n\n";
			
			HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_LOGIN ) );
			
			return;
		}
		else
		{
		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["login_page"], string( CSS_LOGIN ), string( ), NOT_INDEX, CODE_200 );
		pResponse->strContent += "<div class=\"login\">\n";
		pResponse->strContent += "<table class=\"login\">\n";
		pResponse->strContent += "<tr class=\"login\">\n";
		pResponse->strContent += "<td class=\"login\">\n";
		pResponse->strContent += "<form method=\"get\" name=\"login\" action=\"" + RESPONSE_STR_LOGIN_HTML + "\">\n";
		pResponse->strContent += "<p class=\"create_users\">" + gmapLANG_CFG["login"] + "</p>\n";
		pResponse->strContent += "<div class=\"input_username\"><input id=\"id_username\" name=\"username\" alt=\"[" + gmapLANG_CFG["navbar_users"] + "]\" type=text size=24> <label for=\"id_username\">" + gmapLANG_CFG["navbar_users"] + "</label><br><br></div>\n";
		pResponse->strContent += "<div class=\"input_password\"><input id=\"id_password\" name=\"password\" alt=\"[" + gmapLANG_CFG["password"] + "]\" type=password size=20> <label for=\"id_password\">" + gmapLANG_CFG["password"] + "</label><br><br></div>\n";

		// Adds Cancel button beside Create User
		pResponse->strContent += "<div class=\"create_users_buttons\">\n";

		pResponse->strContent += Button_Submit( "submit_login", string( gmapLANG_CFG["login"] ) );
		pResponse->strContent += Button_Back( "cancel_login", string( gmapLANG_CFG["cancel"] ) );

		pResponse->strContent += "\n</div>\n";

		// finish
		pResponse->strContent += "</form>\n</td>\n</tr>\n</table>\n</div>\n";
			
		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_LOGIN ) );

		return;
		}
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

	const string cstrLogout( pRequest->mapParams["logout"] );

	// If the user signs out then expire the cookie, otherwise refresh the cookie
	if( cstrLogout == "1" )
		strftime( pTime, sizeof( char ) * sizeof( pTime ), "%a, %d-%b-%Y %H:%M:%S GMT", &tmPast );
	else
		strftime( pTime, sizeof( char ) * sizeof( pTime ), "%a, %d-%b-%Y %H:%M:%S GMT", &tmFuture );

	// Tell the browser not to cache
	pResponse->mapHeaders.insert( pair<string, string>( "Pragma", "No-Cache" ) );
	// Set the users cookie login
	pResponse->mapHeaders.insert( pair<string, string>( "Set-Cookie", "login=\"" + pRequest->user.strLogin + "\"; expires=" + pTime + "; path=/" ) );
	// Set the users cookie password
	pResponse->mapHeaders.insert( pair<string, string>( "Set-Cookie", "md5=\"" + UTIL_StringToEscaped( pRequest->user.strMD5 ) + "\"; expires=" + pTime + "; path=/" ) );
	// Set the users cookie logout
// 	pResponse->mapHeaders.insert( pair<string, string>( "Set-Cookie", "logout=\"0\"; expires=; path=/" ) );
// 	if( cstrLogout == "1" )
// 		pResponse->mapHeaders.insert( pair<string, string>( "Authorization", "" ) );
// 	{
// 		pResponse->mapHeaders.insert( pair<string, string>( "Set-Cookie", "logout=\"1\"; expires=; path=/" ) );
// 		pResponse->mapHeaders.insert( pair<string, string>( "Set-Cookie", "login=\"" + pRequest->user.strLogin + "\"; expires=; path=/" ) );
// 		pResponse->mapHeaders.insert( pair<string, string>( "Set-Cookie", "md5=\"" + UTIL_StringToEscaped( pRequest->user.strMD5 ) + "\"; expires=; path=/" ) );
// 	}

	// User per page cookies
	unsigned int uiUserPerPage = 0;

	// Did we receive any per page parameters? Set the users per page cookie details
// 	if( pRequest->mapParams["submit_user_per_page_form_button"] == STR_SUBMIT )
	if( pRequest->mapParams["submit_user_per_page_form_button"] == gmapLANG_CFG["submit"] )
	{
		uiUserPerPage = (unsigned int)atoi( pRequest->mapParams["user_per_page"].c_str( ) );

		if( uiUserPerPage < 1 || uiUserPerPage > 65534 )
			uiUserPerPage = m_uiPerPage;

		pResponse->mapHeaders.insert( pair<string, string>( "Set-Cookie", "per_page=\"" + CAtomInt( uiUserPerPage ).toString( ) + "\"; expires=" + pTime + "; path=/" ) );

		return JS_ReturnToPage( pRequest, pResponse, LOGIN_HTML );
	}
	else if( pRequest->mapParams["lp_default_perpage"] == "1" )
	{
		pResponse->mapHeaders.insert( pair<string, string>( "Set-Cookie", "per_page=\"" + CAtomInt( m_uiPerPage ).toString( ) + "\"; expires=" + pTime + "; path=/" ) );

		return JS_ReturnToPage( pRequest, pResponse, LOGIN_HTML );
	}
	else if( pRequest->mapCookies["per_page"].empty( ) )
	{
		pResponse->mapHeaders.insert( pair<string, string>( "Set-Cookie", "per_page=\"" + CAtomInt( m_uiPerPage ).toString( ) + "\"; expires=" + pTime + "; path=/" ) );

		uiUserPerPage = m_uiPerPage;
	}
	else
	{
		uiUserPerPage = (unsigned int)atoi( pRequest->mapCookies["per_page"].c_str( ) );

		if( uiUserPerPage < 1 || uiUserPerPage > 65534 )
			uiUserPerPage = m_uiPerPage;

		pResponse->mapHeaders.insert( pair<string, string>( "Set-Cookie", "per_page=\"" + CAtomInt( uiUserPerPage ).toString( ) + "\"; expires=" + pTime + "; path=/" ) );
	}

	// Was a search submited?
// 	if( pRequest->mapParams["top_submit_search_button"] == STR_SUBMIT || pRequest->mapParams["bottom_submit_search_button"] == STR_SUBMIT )
	if( pRequest->mapParams["top_submit_search_button"] == gmapLANG_CFG["search"] || pRequest->mapParams["bottom_submit_search_button"] == gmapLANG_CFG["search"] )
	{
		const string cstrSearch( pRequest->mapParams["search"] );
		const string cstrSort( pRequest->mapParams["sort"] );
		const string cstrPerPage( pRequest->mapParams["per_page"] );

		string strPageParameters = LOGIN_HTML;

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

		return JS_ReturnToPage( pRequest, pResponse, strPageParameters );
	}

	if( pRequest->mapParams["top_clear_filter_and_search_button"] == "Clear" || pRequest->mapParams["bottom_clear_filter_and_search_button"] == "Clear" )
		return JS_ReturnToPage( pRequest, pResponse, LOGIN_HTML );

	// Output common HTML head
	HTML_Common_Begin( pRequest, pResponse, gmapLANG_CFG["login_page"], string( CSS_LOGIN ), string( ), NOT_INDEX, CODE_200 );

	// Javascript
	pResponse->strContent += "<script type=\"text/javascript\">\n";
	pResponse->strContent += "<!--\n";

	pResponse->strContent += "function default_perpage_confirm( )\n";
	pResponse->strContent += "{\n";
	pResponse->strContent += "var name=confirm(\"Default\\n\")\n";
	pResponse->strContent += "if (name==true)\n";
	pResponse->strContent += "{\n";
	pResponse->strContent += "window.location=\"" + RESPONSE_STR_LOGIN_HTML + "?lp_default_perpage=1\"\n";;
	pResponse->strContent += "}\n";
	pResponse->strContent += "}\n\n";

	pResponse->strContent += "function clear_search_and_filters( ) {\n";
	pResponse->strContent += "window.location=\"" + RESPONSE_STR_LOGIN_HTML + "\"\n";
	pResponse->strContent += "}\n\n";

	pResponse->strContent += "//-->\n";
	pResponse->strContent += "</script>\n\n";

	// Check if the user signed out
	if( cstrLogout == "1" ) 
		pResponse->strContent += "<p class=\"logout\">" + gmapLANG_CFG["logging_out"] + "</p>\n";
	else
	{
		// Can the user delete their own torrents?
		if( m_bDeleteOwnTorrents )
		{
			// Did the user ask to delete a torrent?
			if( pRequest->mapParams.find( "del" ) != pRequest->mapParams.end( ) )
			{
				// What did the user reuest?
				const string ccstrDelHashString( pRequest->mapParams["del"] );
				const string cstrDelHash( UTIL_StringToHash( ccstrDelHashString ) );
				const string cstrOK( pRequest->mapParams["ok"] );

				// Did the request contain a hash string?
				if( cstrDelHash.empty( ) )
				{
					// Unable to delete torrent. The info hash is invalid.
					pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["login_invalid_hash"].c_str( ), ccstrDelHashString.c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_login"] + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";

					// Output common HTML tail
					HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_LOGIN ) );

					return;
				}
				else
				{
					// Is it ok to delete?
					if( cstrOK == "1" )
					{
						if( m_pTags )
						{
							CAtom *pTagInfo = m_pTags->getItem( cstrDelHash );

							if( pTagInfo && pTagInfo->isDicti( ) )
							{
								CAtom *pUploader = ( (CAtomDicti *)pTagInfo )->getItem( "uploader" );

								string strUploader = string( );

								if( pUploader )
									strUploader = pUploader->toString( );

								// Did you upload the torrent?
								if( strUploader != pRequest->user.strLogin )
								{
									// Unable to delete torrent. You didn't upload that torrent.
									pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["login_not_owned"].c_str( ), ccstrDelHashString.c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_login"] + " href=\"" + RESPONSE_STR_LOGIN_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";

									// Output common HTML tail
									HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_LOGIN ) );

									return;
								}
							}
						}

						// delete the torrent from the disk
						if( m_pAllowed )
						{
							CAtom *pList = m_pAllowed->getItem( cstrDelHash );

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
										// If the archive directory is set and exists, move the torrent there, otherwise delete it
										if( !m_strArchiveDir.empty( ) && UTIL_CheckDir( m_strArchiveDir.c_str( ) ) )
											UTIL_MoveFile( string( m_strAllowedDir + pFileName->toString( ) ).c_str( ), string( m_strArchiveDir + pFileName->toString( ) ).c_str( ) );
										else
											UTIL_DeleteFile( string( m_strAllowedDir + pFileName->toString( ) ).c_str( ) );
									}
								}
							}

							// Delete the torrent entry from the databases
							m_pAllowed->delItem( cstrDelHash );
							m_pDFile->delItem( cstrDelHash );
							deleteTag( cstrDelHash );

							// The torrent has been deleted
							pResponse->strContent += "<p class=\"deleted\">" + UTIL_Xsprintf( gmapLANG_CFG["login_deleted_torrent"].c_str( ), ccstrDelHashString.c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_login"] + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";

							// Output common HTML tail
							HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_LOGIN ) );

							return;
						}
					}
					else
					{
						// Are you sure you want to delete the torrent?
						pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["delete_torrent_q"].c_str( ), ccstrDelHashString.c_str( ) ) + "</p>\n";
						pResponse->strContent += "<p class=\"delete\"><a title=\"" + gmapLANG_CFG["yes"] + "\"href=\"" + RESPONSE_STR_LOGIN_HTML + "?del=" + ccstrDelHashString + "&amp;ok=1\">" + gmapLANG_CFG["yes"] + "</a>\n";
						pResponse->strContent += "<span class=\"pipe\">|</span><a title=\"" + gmapLANG_CFG["no"] + "\" href=\"" + string( JS_BACK ) + "\">" + gmapLANG_CFG["no"] + "</a></p>\n";

						// Output common HTML tail
						HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_LOGIN ) );

						return;
					}
				}
			}
		}

#ifdef BNBT_MYSQL
		if( !m_bMySQLOverrideDState )
		{
#endif
			// Compose the page
			if( m_pDFile )
			{
				map<string, CAtom *> *pmapDicti = m_pDFile->getValuePtr( );

				const unsigned long culKeySize( (unsigned long)pmapDicti->size( ) );

				// add the torrents into this structure one by one and sort it afterwards
				struct torrent_t *pTorrents = new struct torrent_t[culKeySize];

				unsigned long ulCount = 0;

				vector<CAtom *> vecTorrent;
				vecTorrent.reserve( 6 );

				CAtom *pList = 0;
				CAtom *pFileName = 0;
				CAtom *pName = 0;
				CAtom *pAdded = 0;
				CAtom *pSize = 0;
				CAtom *pFiles = 0;
				CAtom *pCommentList = 0;
				CAtom *pDicti = 0;
				CAtom *pTag = 0;
				CAtom *pUploader = 0;
				CAtom *pInfoLink = 0;
				CAtom *pIP = 0;

				for( map<string, CAtom *> :: iterator it = pmapDicti->begin( ); it != pmapDicti->end( ); it++ )
				{
					pTorrents[ulCount].strInfoHash = (*it).first;
					pTorrents[ulCount].strName = gmapLANG_CFG["unknown"];
					pTorrents[ulCount].strLowerName = gmapLANG_CFG["unknown"];
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

					if( (*it).second->isDicti( ) )
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
							}
						}
					}

					ulCount++;
				}

				// Sort
				const string cstrSort( pRequest->mapParams["sort"] );

				if( m_bSort )
				{
					if( !cstrSort.empty( ) )
					{
						const unsigned char cucSort( (unsigned char)atoi( cstrSort.c_str( ) ) );

						switch( cucSort )
						{
						case SORT_ANAME:
							if( m_pAllowed )
								qsort( pTorrents, culKeySize, sizeof( struct torrent_t ), asortByName );
							break;
						case SORT_DNAME:
							if( m_pAllowed )
								qsort( pTorrents, culKeySize, sizeof( struct torrent_t ), dsortByName );
							break;
						case SORT_AADDED:
							if( m_pAllowed )
								qsort( pTorrents, culKeySize, sizeof( struct torrent_t ), asortByAdded );
							break;
						case SORT_ATAG:
							qsort( pTorrents, culKeySize, sizeof( struct torrent_t ), asortByTag );
							break;
						case SORT_DTAG:
							qsort( pTorrents, culKeySize, sizeof( struct torrent_t ), dsortByTag );
							break;
						case SORT_DADDED:
						default:
							// default action is to sort by added if we can
							if( m_pAllowed )
								qsort( pTorrents, culKeySize, sizeof( struct torrent_t ), dsortByAdded );
						}
					}
					else
					{
						// default action is to sort by added if we can

						if( m_pAllowed )
							qsort( pTorrents, culKeySize, sizeof( struct torrent_t ), dsortByAdded );
					}
				}
				else
				{
					// sort is disabled, but default action is to sort by added if we can   

					if( m_pAllowed )
						qsort( pTorrents, culKeySize, sizeof( struct torrent_t ), dsortByAdded );
				}

				// some preliminary search crap

				const string cstrSearch( pRequest->mapParams["search"] );
				const string cstrLowerSearch( UTIL_ToLower( cstrSearch ) );
				const string cstrSearchResp( UTIL_StringToEscaped( cstrSearch ) );

				if( !cstrSearch.empty( ) && m_pAllowed && m_bSearch )
					pResponse->strContent += "<p class=\"search_results\">" + UTIL_Xsprintf( gmapLANG_CFG["search_results_for"].c_str( ), string( "\"<span class=\"filtered_by\">" + UTIL_RemoveHTML( cstrSearch ) + "</span>\"").c_str( ) ) + "</p>\n";

				// which page are we viewing

				unsigned long ulStart = 0;
				unsigned int uiOverridePerPage = 0;

				const string cstrPerPage( pRequest->mapParams["per_page"] );

				if ( cstrPerPage.empty( ) )
					uiOverridePerPage = uiUserPerPage;
				else
				{
					uiOverridePerPage = (unsigned int)atoi( cstrPerPage.c_str( ) );

					if( uiOverridePerPage > 65534 )
						uiOverridePerPage = m_uiPerPage;
				}

				// Count matching torrents for top of page
				unsigned long ulFound = 0;

				for( unsigned long ulKey = 0; ulKey < culKeySize; ulKey++ )
				{
					if( !cstrSearch.empty( ) )
					{
						// only count entries that match the search
						if( pTorrents[ulKey].strLowerName.find( cstrLowerSearch ) == string :: npos )
							continue;
					}

					// only count entries that match the current user
					if( pTorrents[ulKey].strUploader != pRequest->user.strLogin )
						continue;

					ulFound++;
				}

				// You signed up on
				pResponse->strContent += "<p class=\"registered\">" + UTIL_Xsprintf( gmapLANG_CFG["login_signed_up"].c_str( ), pRequest->user.strCreated.c_str( ) ) + "  <a href=\"" + RESPONSE_STR_USERS_HTML + "?user=" + pRequest->user.strLogin + "&amp;action=edit\">" + gmapLANG_CFG["edit_user"] + "</a></p>\n\n";

				// User per page cookie
				pResponse->strContent += "<form method=\"get\" class=\"get_user_perpage\" name=\"getuserperpage\" onSubmit=\"return confirm(\'" + gmapLANG_CFG["submit"] + "\')\" action=\"" + RESPONSE_STR_LOGIN_HTML + "\">\n";
				pResponse->strContent += "<p><label for=\"rowsperpage\">" + gmapLANG_CFG["login_rows_per_page"] + "</label> <input name=\"user_per_page\" id=\"rowsperpage\" alt=\"[" + gmapLANG_CFG["login_rows_per_page"] + "]\" type=text size=5 value=\"" + CAtomInt( uiUserPerPage ).toString( ) + "\">\n";
				pResponse->strContent += Button_Submit( "submit_user_per_page_form", gmapLANG_CFG["submit"] );
				pResponse->strContent += Button_JS_Link( "user_per_page_default", "Default", "default_perpage_confirm( )" );
				pResponse->strContent += "</p></form>\n";

				// search messages
				pResponse->strContent += "<p class=\"search_filter\">\n";

				if( !cstrSearch.empty( ) )
					pResponse->strContent += "<span class=\"search_results_alt\">" + gmapLANG_CFG["result_search"] + ": \"</span><span class=\"filtered_by\">" + UTIL_RemoveHTML( cstrSearch ) + "</span>\"\n";

				pResponse->strContent += "</p>\n\n";

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
				if(  culKeySize && m_pAllowed && m_bSearch )
				{
					pResponse->strContent += "<form class=\"search_login_top\" name=\"topsearch\" method=\"get\" action=\"" + RESPONSE_STR_LOGIN_HTML + "\">\n";

					if( !cstrPerPage.empty( ) )
						pResponse->strContent += "<p><input name=\"per_page\" type=hidden value=\"" + cstrPerPage + "\"></p>\n";

					if( !cstrSort.empty( ) )
						pResponse->strContent += "<p><input name=\"sort\" type=hidden value=\"" + cstrSort + "\"></p>\n";

					if( m_bUseButtons )
					{
						pResponse->strContent += "<p><label for=\"toptorrentsearch\">" + gmapLANG_CFG["torrent_search"] + "</label> <input name=\"search\" id=\"toptorrentsearch\" alt=\"[" + gmapLANG_CFG["torrent_search"] + "]\" type=text size=40>\n";

						pResponse->strContent += Button_Submit( "top_submit_search", gmapLANG_CFG["search"] );
						pResponse->strContent += Button_JS_Link( "top_clear_filter_and_search", gmapLANG_CFG["clear_filter_search"], "clear_search_and_filters( )" );

						pResponse->strContent += "</p>\n";
					}
					else
						pResponse->strContent += "<p><label for=\"toptorrentsearch\">" + gmapLANG_CFG["torrent_search"] + "</label> <input name=\"search\" id=\"toptorrentsearch\" alt=\"[" + gmapLANG_CFG["torrent_search"] + "]\" type=text size=40> <a title=\"" + gmapLANG_CFG["clear_filter_search"] + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "\">" + gmapLANG_CFG["clear_filter_search"] + "</a></p>\n";

					pResponse->strContent += "</form>\n\n";
				}

				if( culKeySize && uiOverridePerPage > 0 )
				{
					const string cstrPage( pRequest->mapParams["page"] );

					if( !cstrPage.empty( ) )
						ulStart = ( unsigned long )( atoi( cstrPage.c_str( ) ) * uiOverridePerPage );

					// page numbers
					pResponse->strContent += "<p class=\"pagenum_top_bar\">" + gmapLANG_CFG["jump_to_page"] + ": \n";

					for( unsigned long ulKey = 0; ulKey < ulFound; ulKey += uiOverridePerPage )
					{
						pResponse->strContent += " ";

						// don't link to current page
						if( ulKey != ulStart )
						{
							pResponse->strContent += "<a title=\"" + gmapLANG_CFG["jump_to_page"] + ": " + CAtomInt( ( ulKey / uiOverridePerPage ) + 1 ).toString( ) + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "?page=" + CAtomInt( ulKey / uiOverridePerPage ).toString( );

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

						if( pTorrents[ulKey].strLowerName.find( cstrLowerSearch ) == string :: npos )
							continue;
					}

					if( pTorrents[ulKey].strUploader != pRequest->user.strLogin )
						continue;

					ulFound++;

					if( uiOverridePerPage == 0 || ulAdded < uiOverridePerPage )
					{
						// create the table and display the headers first
						if( !bFound )
						{	   
							// output table headers

							pResponse->strContent += "<div class=\"login_table\">\n";
							pResponse->strContent += "<table summary=\"ytinfo\">\n";
							pResponse->strContent += "<tr><th colspan=4>" + gmapLANG_CFG["login_your_torrents"] + "</th></tr>\n";

							pResponse->strContent += "<tr>\n";

							// <th> tag
							if( !m_vecTags.empty( ) )
							{
								pResponse->strContent += "<th class=\"tag\" id=\"tagheader\">" + gmapLANG_CFG["tag"];
								if( m_bSort )
								{
									pResponse->strContent += "<br>\n<div><a class=\"sort\" title=\"" + gmapLANG_CFG["sort_tag_ascending"] + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "?sort=" + SORTSTR_ATAG;

									if( !cstrPerPage.empty( ) )
										pResponse->strContent += "&amp;per_page=" + cstrPerPage;

									if( !cstrSearch.empty( ) )
										pResponse->strContent += "&amp;search=" + cstrSearchResp;

									pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a class=\"sort\" title=\"" + gmapLANG_CFG["sort_tag_descending"] + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "?sort=" + SORTSTR_DTAG;

									if( !cstrPerPage.empty( ) )
										pResponse->strContent += "&amp;per_page=" + cstrPerPage;

									if( !cstrSearch.empty( ) )
										pResponse->strContent += "&amp;search=" + cstrSearchResp;

									pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a></div>";
								}

								pResponse->strContent += "</th>\n";
							}

							// Name
							pResponse->strContent += "<th class=\"name\" id=\"nameheader\">" + gmapLANG_CFG["name"];

							if( m_bSort )
							{
								pResponse->strContent += "<br><div><a class=\"sort\" title=\"" + gmapLANG_CFG["sort_name_ascending"] + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "?sort=" + SORTSTR_ANAME;

								if( !cstrPerPage.empty( ) )
									pResponse->strContent += "&amp;per_page=" + cstrPerPage;

								if( !cstrSearch.empty( ) )
									pResponse->strContent += "&amp;search=" + cstrSearchResp;

								pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a class=\"sort\" title=\"" + gmapLANG_CFG["sort_name_descending"] + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "?sort=" + SORTSTR_DNAME;

								if( !cstrPerPage.empty( ) )
									pResponse->strContent += "&amp;per_page=" + cstrPerPage;

								if( !cstrSearch.empty( ) )
									pResponse->strContent += "&amp;search=" + cstrSearchResp;

								pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a></div>";
							}

							pResponse->strContent += "</th>\n";						

							// Added
							pResponse->strContent += "<th class=\"date\" id=\"addedheader\">" + gmapLANG_CFG["added"];

							if( m_bSort )
							{
								pResponse->strContent += "<br><div><a class=\"sort\" title=\"" + gmapLANG_CFG["sort_added_ascending"] + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "?sort=" + SORTSTR_AADDED;

								if( !cstrPerPage.empty( ) )
									pResponse->strContent += "&amp;per_page=" + cstrPerPage;

								if( !cstrSearch.empty( ) )
									pResponse->strContent += "&amp;search=" + cstrSearchResp;

								pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a class=\"sort\" title=\"" + gmapLANG_CFG["sort_added_descending"] + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "?sort=" + SORTSTR_DADDED;

								if( !cstrPerPage.empty( ) )
									pResponse->strContent += "&amp;per_page=" + cstrPerPage;

								if( !cstrSearch.empty( ) )
									pResponse->strContent += "&amp;search=" + cstrSearchResp;

								pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a></div>";
							}

							pResponse->strContent += "</th>\n";

							// Admin
							pResponse->strContent += "<th id=\"adminheader\">" + gmapLANG_CFG["admin"] + "</th>\n";   

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

							// display the tag
							unsigned char ucTag = 1;

							pResponse->strContent += "<td>" ;
							if( !m_vecTags.empty( ) )
							{
								for( vector< pair< string, string > > :: iterator k = m_vecTags.begin( ); k != m_vecTags.end( ); k++ )
								{
							//		if( (*k).first == pTorrents[ulKey].strTag )
									if( CAtomInt( ucTag ).toString( ) == pTorrents[ulKey].strTag )
									{
										if ( !(*k).second.empty( ) )
											pResponse->strContent += "<img src=\"" + (*k).second + "\" alt=\"[" + pTorrents[ulKey].strTag + "]\" title=\"" + pTorrents[ulKey].strTag + "\" name=\"" + pTorrents[ulKey].strTag + "\">";
										else
											pResponse->strContent += UTIL_RemoveHTML( pTorrents[ulKey].strTag );
									}
									ucTag++;
								}
							}
							pResponse->strContent += "</td>\n" ;

							// display the name and stats link
							pResponse->strContent += "<td class=\"name\"><a title=\"" + gmapLANG_CFG["name"] + ": " + UTIL_RemoveHTML( pTorrents[ulKey].strName )+ "\" href=\"" + RESPONSE_STR_STATS_HTML + "?info_hash=" + UTIL_HashToString( pTorrents[ulKey].strInfoHash ) + "\">" + UTIL_RemoveHTML( pTorrents[ulKey].strName ) + "</a></td>\n";

							// display the added date
							pResponse->strContent += "<td class=\"date\">" + pTorrents[ulKey].strAdded + "</td>\n";

							// display user's torrent administration if any
							if( m_bDeleteOwnTorrents )
							{
								pResponse->strContent += "<td class=\"admin\">[<a title=\"" + gmapLANG_CFG["edit"] + ": " + UTIL_HashToString( pTorrents[ulKey].strInfoHash ) + "\" href=\"" + RESPONSE_STR_STATS_HTML + "?info_hash=" + UTIL_HashToString( pTorrents[ulKey].strInfoHash ) + "&amp;edit=1\">" + gmapLANG_CFG["edit"] + "</a>]";
								pResponse->strContent += "[<a title=\"" + gmapLANG_CFG["delete"] + ": " + UTIL_HashToString( pTorrents[ulKey].strInfoHash ) + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "?del=" + UTIL_HashToString( pTorrents[ulKey].strInfoHash ) + "\">" + gmapLANG_CFG["delete"] + "</a>]</td>\n";
							}
							else
								pResponse->strContent += "<td class=\"admin\">" + gmapLANG_CFG["na"] + "</td>\n";

							// end row
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
				{
					pResponse->strContent += "</table>\n";
					pResponse->strContent += "</div>\n\n";
				}

				// Bottom search
				if( ulFound && m_pAllowed && m_bSearch )
				{
					pResponse->strContent += "<form class=\"search_login\" name=\"bottomsearch\" method=\"get\" action=\"" + RESPONSE_STR_LOGIN_HTML + "\">\n";

					if( !cstrPerPage.empty( ) )
						pResponse->strContent += "<p><input name=\"per_page\" type=hidden value=\"" + cstrPerPage + "\"></p>\n";

					if( !cstrSort.empty( ) )
						pResponse->strContent += "<p><input name=\"sort\" type=hidden value=\"" + cstrSort + "\"></p>\n";

					if( m_bUseButtons )
					{
						pResponse->strContent += "<p><label for=\"bottomtorrentsearch\">" + gmapLANG_CFG["torrent_search"] + "</label> <input name=\"search\" id=\"bottomtorrentsearch\" alt=\"[" + gmapLANG_CFG["torrent_search"] + "]\" type=text size=40>\n";

						pResponse->strContent += Button_Submit( "bottom_submit_search", gmapLANG_CFG["search"] );
						pResponse->strContent += Button_JS_Link( "bottom_clear_filter_and_search", gmapLANG_CFG["clear_filter_search"], "clear_search_and_filters( )" );

						pResponse->strContent += "</p>\n";
					}
					else
						pResponse->strContent += "<p><label for=\"bottomtorrentsearch\">" + gmapLANG_CFG["torrent_search"] + "</label> <input name=\"search\" id=\"bottomtorrentsearch\" alt=\"[" + gmapLANG_CFG["torrent_search"] + "]\" type=text size=40> <a title=\"" + gmapLANG_CFG["clear_filter_search"] + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "\">" + gmapLANG_CFG["clear_filter_search"] + "</a></p>\n";

					pResponse->strContent += "</form>\n\n";
				}

				// page numbers

				if( ulFound && uiOverridePerPage > 0 )
				{
					// Modified by =Xotic=
					pResponse->strContent += "<p class=\"pagenum_bottom\">" + gmapLANG_CFG["jump_to_page"] + ": \n";

					for( unsigned long ulKey = 0; ulKey < ulFound; ulKey += uiOverridePerPage )
					{
						pResponse->strContent += " ";

						// don't link to current page

						if( ulKey != ulStart )
						{
							pResponse->strContent += "<a title=\"" + gmapLANG_CFG["jump_to_page"] + ": " + CAtomInt( ( ulKey / uiOverridePerPage ) + 1 ).toString( ) + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "?page=" + CAtomInt( ulKey / uiOverridePerPage ).toString( );

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
			}
#ifdef BNBT_MYSQL
		}
		else
			UTIL_LogPrint( "serverResponseLogin: no dfile information\n" );
#endif
	}

	// Output common HTML tail
	HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_LOGIN ) );
}
