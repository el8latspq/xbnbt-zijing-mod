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

// #include <fcntl.h>
#include <sys/stat.h>

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
#include "tracker.h"
#include "util.h"

void CTracker :: serverResponseInfoGET( struct request_t *pRequest, struct response_t *pResponse )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
		if( IsIPBanned( pRequest, pResponse, btv, gmapLANG_CFG["info_page"], string( CSS_INFO ), NOT_INDEX ) )
			return;

//	if( !pRequest->user.strUID.empty( ) && ( m_ucInfoAccess == 0 || ( pRequest->user.ucAccess & m_ucInfoAccess ) ) )
	if( ( m_ucInfoAccess == 0 || ( pRequest->user.ucAccess & m_ucInfoAccess ) ) )
	{
		time_t tNow = time( 0 );
		char *szTime = asctime( localtime( &tNow ) );
		szTime[strlen( szTime ) - 1] = TERM_CHAR;
		
		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["info_page"], string( CSS_INFO ), string( ), NOT_INDEX, CODE_200 );
		
		const string cstrPostAction( pRequest->mapParams["action"] );
//		string cstrPostNum( pRequest->mapParams["post"] );
		
		// Set a past time
		struct tm tmPast = *gmtime( &tNow );
		tmPast.tm_mon--;
		mktime( &tmPast );

		char pTime[256];
		memset( pTime, 0, sizeof( pTime ) / sizeof( char ) );

		strftime( pTime, sizeof( char ) * sizeof( pTime ), "%a, %d-%b-%Y %H:%M:%S GMT", &tmPast );
		
		pResponse->mapHeaders.insert( pair<string, string>( "Pragma", "No-Cache" ) );
		
		pResponse->mapHeaders.insert( pair<string, string>( "Set-Cookie", string( "announce=\"0\"; expires=" ) + pTime + "; path=/" ) );
		
		pResponse->strContent += "<script type=\"text/javascript\">\n";
		pResponse->strContent += "<!--\n";

		// hide
		pResponse->strContent += "function hide(id) {\n";
		pResponse->strContent += "  var element = document.getElementById( id );\n";
		pResponse->strContent += "  element.style.display=\"none\";\n";
		pResponse->strContent += "}\n";
		
		// display
		pResponse->strContent += "function display(id) {\n";
		pResponse->strContent += "  var element = document.getElementById( id );\n";
		pResponse->strContent += "  element.style.display=\"\";\n";
		pResponse->strContent += "}\n";
		
		pResponse->strContent += "var hided = \"true\";\n";
		
		pResponse->strContent += "function change(id) {\n";
		pResponse->strContent += "  var element = document.getElementById( id );\n";
		pResponse->strContent += "  if (element.style.display == \"\") {\n";
		pResponse->strContent += "    hide( id ); }\n";
		pResponse->strContent += "  else {\n";
		pResponse->strContent += "    display( id ); }\n";
		pResponse->strContent += "}\n";
		
		pResponse->strContent += "//-->\n";
		pResponse->strContent += "</script>\n\n";
		
		pResponse->strContent += "<table class=\"post_info_table\"><tr class=\"post_info\">\n";

		pResponse->strContent += "<td class=\"info\"><div class=\"info_table\">\n";
		pResponse->strContent += "<table summary=\"tinfo\">\n";
		pResponse->strContent += "<tr><th colspan=2>" + gmapLANG_CFG["info_tracker_info"] + "</th></tr>\n";

		pResponse->strContent += "<tr class=\"info_tracker_version\">";
		pResponse->strContent += "<th class=\"info_field\">" + gmapLANG_CFG["info_tracker_version"] + "</th>\n";
		pResponse->strContent += "<td class=\"info_field\">" + XBNBT_VER + "</td></tr>\n";

		pResponse->strContent += "<tr class=\"info_server_time\">";
		pResponse->strContent += "<th class=\"info_field\">" + gmapLANG_CFG["info_server_time"] + "</th>\n";
		pResponse->strContent += "<td class=\"info_field\">" + string( szTime ) + "</td></tr>\n";

		pResponse->strContent += "<tr class=\"info_uptime\">";
		pResponse->strContent += "<th class=\"info_field\">" + gmapLANG_CFG["info_uptime"] + "</th>\n";
		pResponse->strContent += "<td class=\"info_field\">" + UTIL_SecondsToString( GetTime( ) - GetStartTime( ) ) + "</td></tr>\n";

		if( !m_strTitle.empty( ) )
		{
			pResponse->strContent += "<tr class=\"info_tracker_title\">";
			pResponse->strContent += "<th class=\"info_field\">" + gmapLANG_CFG["info_tracker_title"] + "</th>\n";
			pResponse->strContent += "<td class=\"info_field\">" + m_strTitle + "</td></tr>\n";
		}

		if( !m_strDescription.empty( ) )
		{
			pResponse->strContent += "<tr class=\"info_tracker_desc\">";
			pResponse->strContent += "<th class=\"info_field\">" + gmapLANG_CFG["info_tracker_desc"] + "</th>\n";
			pResponse->strContent += "<td class=\"info_field\">" + m_strDescription + "</td></tr>\n";
		}

//		if( !m_strForceAnnounceURL.empty( ) && ( xtorrent.bForceAnnounceDL || xtorrent.bForceAnnounceUL ) )
//		{
//			pResponse->strContent += "<tr class=\"info_forced_announce_url\">";
//			pResponse->strContent += "<th class=\"info_field\">" + gmapLANG_CFG["forced_announce_url"] + "</th>\n";
//			pResponse->strContent += "<td class=\"info_field\">" + UTIL_RemoveHTML( m_strForceAnnounceURL ) + "</td></tr>\n";
//		}
//		else
//		{
//			pResponse->strContent += "<tr class=\"info_announce_url\">";
//			pResponse->strContent += "<th class=\"info_field\">" + gmapLANG_CFG["announce_url"] + "</th>\n";
//			pResponse->strContent += "<td class=\"info_field\">http://";
//			pResponse->strContent += "<script type=\"text/javascript\">document.write( parent.location.host );</script>";
//			pResponse->strContent += RESPONSE_STR_ANNOUNCE + "</td></tr>\n";
//		}

//		pResponse->strContent += "<tr class=\"info_torrent_size\">";
//		pResponse->strContent += "<th class=\"info_field\">" + gmapLANG_CFG["info_torrent_size"] + "</th>\n";
//		pResponse->strContent += "<td class=\"info_field\">" + UTIL_BytesToString( guiMaxRecvSize ) + "</td></tr>\n";
		
		unsigned long ulKeySize = 0;
		unsigned long ulActives = 0;
		struct torrent_t *pTorrents = 0;
		
		if( m_pCache )
		{
			pTorrents = m_pCache->getCache( );
			ulKeySize = m_pCache->getSize( );
		}
		
		for( unsigned long ulKey = 0; ulKey < ulKeySize; ulKey++ )
		{
			if( pTorrents[ulKey].uiSeeders > 0 || pTorrents[ulKey].uiLeechers > 0 )
				ulActives++;
		}
		
		pResponse->strContent += "<tr class=\"info_torrents\">";
		pResponse->strContent += "<th class=\"info_field\">" + gmapLANG_CFG["info_torrents"] + "</th>\n";
		pResponse->strContent += "<td class=\"info_field\">" + CAtomLong( ulKeySize ).toString( ) + " / " + CAtomLong( ulActives ).toString( ) + "</td></tr>\n";

//		if( !rssdump.strName.empty( ) && ( m_ucDumpRSSFileMode == 0 || m_ucDumpRSSFileMode == 2 ) )
//		{
//			pResponse->strContent += "<tr class=\"info_rss\">";
//			pResponse->strContent += "<th class=\"info_field\">" + gmapLANG_CFG["info_rss"] + "</th>\n";
//			pResponse->strContent += "<td class=\"info_field\">";

//			if( !rssdump.strURL.empty( ) )
//				pResponse->strContent += "<a title=\"" + gmapLANG_CFG["navbar_rss"] + "\" rel=\"" + STR_TARGET_REL + "\" href=\"" + rssdump.strURL + rssdump.strName + "\">" + rssdump.strURL + rssdump.strName + "</a>";
//			else if( m_bServeLocal )
//				pResponse->strContent += "<script type=\"text/javascript\">" + m_strRSSLocalLink + "</script>";

//			pResponse->strContent += "</td></tr>\n";
//		}

		// Refresh the fast cache
		if( GetTime( ) > m_ulRefreshFastCacheNext )
		{
			// Refresh
			RefreshFastCache( );


			// Set the next refresh time
			m_ulRefreshFastCacheNext = GetTime( ) + m_uiRefreshFastCacheInterval;
		}

		pResponse->strContent += "<tr class=\"info_total_peers\">";
		pResponse->strContent += "<th class=\"info_field\">" + gmapLANG_CFG["info_total_peers"] + "</th>\n";
		pResponse->strContent += "<td class=\"info_field\">" + CAtomLong( m_ulPeers ).toString( ) + " ( " + CAtomLong( m_ulSeedersTotal ).toString( ) + " / " + CAtomLong( m_ulLeechersTotal ).toString( ) + " )" + "</td></tr>\n";

		if( m_bCountUniquePeers )
		{
			CMySQLQuery *pQueryIP = new CMySQLQuery( "SELECT COUNT(*) FROM ips WHERE bcount>0" );
			
			vector<string> vecQueryIP;
	
			vecQueryIP.reserve(1);
		
			vecQueryIP = pQueryIP->nextRow( );
			
			delete pQueryIP;
			
			int64 iGreatestUnique = UTIL_StringTo64( vecQueryIP[0].c_str( ) );
				
			if( iGreatestUnique > gtXStats.peer.iGreatestUnique )
				gtXStats.peer.iGreatestUnique = iGreatestUnique;

			pResponse->strContent += "<tr class=\"info_unique_peers\">";
			pResponse->strContent += "<th class=\"info_field\">" + gmapLANG_CFG["info_unique_peers"] + "</th>\n";
			pResponse->strContent += "<td class=\"info_field\">" + vecQueryIP[0] + "</td></tr>\n";
		}

		// Registered users
		CMySQLQuery *pQueryUser = new CMySQLQuery( "SELECT COUNT(*) FROM users" );
		
		vector<string> vecQueryUser;
	
		vecQueryUser.reserve(1);
		
		vecQueryUser = pQueryUser->nextRow( );
	
		delete pQueryUser;
		
		if( vecQueryUser.size( ) == 1 )
//		if( m_pCache )
		{
			pResponse->strContent += "<tr class=\"info_reg_users\">";
			pResponse->strContent += "<th class=\"info_field\">" + gmapLANG_CFG["info_reg_users"] + "</th>\n";
			pResponse->strContent += "<td class=\"info_field\">" + vecQueryUser[0] + "</td></tr>\n";
//			pResponse->strContent += "<td class=\"info_field\">" + CAtomLong( m_pCache->getSizeUsers( ) ).toString( ) + "</td></tr>\n";
			pResponse->strContent += "<tr class=\"info_max_users\">";
			pResponse->strContent += "<th class=\"info_field\">" + gmapLANG_CFG["info_max_users"] + "</th>\n";
			pResponse->strContent += "<td class=\"info_field\">" + CFG_GetString( "bnbt_max_users", string( "1000" ) ) + "</td></tr>\n";
		}

		// Total announce
		pResponse->strContent += "<tr class=\"info_stats_announces\">";
		pResponse->strContent += "<th class=\"info_field\">" + gmapLANG_CFG["stats_announces"] + "</th>\n";
		pResponse->strContent += "<td class=\"info_field\">" + CAtomLong( gtXStats.announce.iAnnounce ).toString( ) + "</td></tr>\n";

		// Total scrape
		pResponse->strContent += "<tr class=\"info_stats_scrapes\">";
		pResponse->strContent += "<th class=\"info_field\">" + gmapLANG_CFG["stats_scrapes"] + "</th>\n";
		pResponse->strContent += "<td class=\"info_field\">" + CAtomLong( gtXStats.scrape.iScrape ).toString( ) + "</td></tr>\n";

		pResponse->strContent += "</table>\n</div>\n";
		
		if( !pRequest->user.strUID.empty( ) )
		{
			pResponse->strContent += "<p>";
		
			pResponse->strContent += "<div class=\"vote_table\">\n";
			pResponse->strContent += "<table class=\"vote_table\" summary=\"tvote\">\n";
			pResponse->strContent += "<tr><th colspan=2>" + gmapLANG_CFG["info_vote"] + "</th></tr>\n";
		
			if(  pRequest->user.ucAccess & m_ucAccessAdmin )
			{
				pResponse->strContent += "<tr class=\"voteheader\">\n";
				pResponse->strContent += "<td class=\"admin\" colspan=2>[<a class=\"black\" href=\"" + RESPONSE_STR_VOTES_HTML + "\">" + gmapLANG_CFG["vote_admin"] + "</a>]</td>";
				pResponse->strContent += "</tr>\n";
			}
		
			CMySQLQuery *pQueryVotes = new CMySQLQuery( "SELECT bid,btitle,bopen,bclosed,UNIX_TIMESTAMP(bclosed),bvotecount FROM votes WHERE bopen!=0 AND bclosed=0 ORDER BY bopen DESC" );
		
			vector<string> vecQueryVotes;
	
			vecQueryVotes.reserve(6);
		
			vecQueryVotes = pQueryVotes->nextRow( );
		
			if( vecQueryVotes.size( ) == 0 )
			{
				delete pQueryVotes;
			
				pQueryVotes = new CMySQLQuery( "SELECT bid,btitle,bopen,bclosed,UNIX_TIMESTAMP(bclosed),bvotecount FROM votes WHERE bopen!=0 ORDER BY bopen DESC LIMIT 1" );
			
				vecQueryVotes = pQueryVotes->nextRow( );
			}
		
			while( vecQueryVotes.size( ) == 6 )
			{
				string strOption = string( );
			
				if( !pRequest->user.strUID.empty( ) )
				{
					CMySQLQuery *pQueryTicket = new CMySQLQuery( "SELECT boptionid FROM votesticket WHERE bid=" + vecQueryVotes[0] + " AND buid=" + pRequest->user.strUID + " ORDER BY boptionid" );
		
					vector<string> vecQueryTicket;

					vecQueryTicket.reserve(1);

					vecQueryTicket = pQueryTicket->nextRow( );
			
					delete pQueryTicket;
			
					if( vecQueryTicket.size( ) == 1 )
						strOption = vecQueryTicket[0];
				}
				else
					strOption = "0";
			
	//			pResponse->strContent += "<tr class=\"voteheader\">\n";
	//			pResponse->strContent += "<th class=\"votetime\">" + gmapLANG_CFG["vote_time_open"] + "</th>\n";
	//			pResponse->strContent += "<td class=\"votetime\">" + vecQueryVotes[2] + "</th>\n";
	//			pResponse->strContent += "<th class=\"votetime\">" + gmapLANG_CFG["vote_time_closed"] + "</th>\n";
	//			pResponse->strContent += "<td class=\"votetime\">" + vecQueryVotes[3] + "</th>\n";
			
	//			pResponse->strContent += "</tr>\n";
			
				CMySQLQuery *pQueryVote = new CMySQLQuery( "SELECT boptionid,boption,bvotecount FROM votesoption WHERE bid=" + vecQueryVotes[0] + " ORDER BY boptionid" );
		
				vector<string> vecQueryVote;
	
				vecQueryVote.reserve(3);

				vecQueryVote = pQueryVote->nextRow( );
			
				if( vecQueryVote.size( ) == 3 )
				{
					string strSkip = string( );
				
					if( strOption.empty( ) && vecQueryVotes[4] == "0" )
					{
						pResponse->strContent += "<form name=\"vote\" method=\"post\" action=\"" + RESPONSE_STR_VOTES_HTML + "\" enctype=\"multipart/form-data\">\n";
						pResponse->strContent += "<input name=\"vote_for\" type=hidden value=\"" + vecQueryVotes[0] + "\">\n";
					}
					pResponse->strContent += "<tr class=\"vote\">";
					pResponse->strContent += "<td class=\"vote\" colspan=2>";
					pResponse->strContent += "<p class=\"votetitle\">" + UTIL_RemoveHTML( vecQueryVotes[1] ) + "</p>";

					while( vecQueryVote.size( ) == 3 )
					{
						if( vecQueryVote[0] != "0" )
						{
							pResponse->strContent += "<p class=\"voteoption\">";
							if( strOption.empty( ) && vecQueryVotes[4] == "0" )
								pResponse->strContent += "<input name=\"vote_selectd\" type=radio value=\"" + vecQueryVote[0] + "\">" + UTIL_RemoveHTML( vecQueryVote[1] );
							else
								pResponse->strContent += UTIL_RemoveHTML( vecQueryVote[1] ) + ": " + vecQueryVote[2] + " / " + vecQueryVotes[5];
					
							pResponse->strContent += "</p>\n";
						}
						else
						{
							if( strOption.empty( ) && vecQueryVotes[4] == "0" )
								strSkip = "<p class=\"voteoption\"><input name=\"vote_selectd\" type=radio value=\"" + vecQueryVote[0] + "\">" + UTIL_RemoveHTML( vecQueryVote[1] ) + "</p>\n";
						}
				
						vecQueryVote = pQueryVote->nextRow( );
					}
					if( strOption.empty( ) && vecQueryVotes[4] == "0" )
					{
						pResponse->strContent += strSkip;
						pResponse->strContent += "<p class=\"votebutton\">" + Button_Submit( "submit_vote", string( gmapLANG_CFG["vote"] ) ) + "</p>";
					}
					else
						pResponse->strContent += "<p class=\"votetotal\">" + UTIL_Xsprintf( gmapLANG_CFG["vote_total"].c_str( ), vecQueryVotes[5].c_str( ) ) + "</p>";
					pResponse->strContent += "</td>\n</tr>\n";
					if( strOption.empty( ) && vecQueryVotes[4] == "0" )
						pResponse->strContent += "</form>\n";
				}
			
				delete pQueryVote;
			
				vecQueryVotes = pQueryVotes->nextRow( );
			}
		
			delete pQueryVotes;
		
			pResponse->strContent += "</table>\n</div>\n";
		}
		
		pResponse->strContent += "</td>\n";
		
		int64 last_time_info = 0;

		if( !pRequest->user.strUID.empty( ) )
		{
			CMySQLQuery *pQueryUser = new CMySQLQuery( "SELECT UNIX_TIMESTAMP(blast_info) FROM users WHERE buid=" + pRequest->user.strUID );
	
			vector<string> vecQueryUser;
	
			vecQueryUser.reserve(1);
			
			vecQueryUser = pQueryUser->nextRow( );
		
			delete pQueryUser;
			
			if( vecQueryUser.size( ) == 1 )
			{
				last_time_info = UTIL_StringTo64( vecQueryUser[0].c_str( ) );
			}
			
			CMySQLQuery mq01( "UPDATE users SET blast_info=NOW() WHERE buid=" + pRequest->user.strUID );
		}
		
		pResponse->strContent += "<td class=\"post\">\n<table class=\"post_table\">\n";
		
//		if( pRequest->user.ucAccess & m_ucAccessAdmin )
//		{
			pResponse->strContent += "<tr>\n";
			pResponse->strContent += "<td class=\"admin\" colspan=2>[<a class=\"black\" href=\"" + RESPONSE_STR_ANNOUNCEMENTS_HTML + "\">" + gmapLANG_CFG["post_all"] + "</a>]</td>";
			pResponse->strContent += "</tr>\n";
//		}
			
		CMySQLQuery *pQueryAnn = new CMySQLQuery( "SELECT bid,bposted,UNIX_TIMESTAMP(bposted),btitle,bannouncement,baccess FROM announcements ORDER BY bposted DESC" );
		
		vector<string> vecQueryAnn;
	
		vecQueryAnn.reserve(6);

		vecQueryAnn = pQueryAnn->nextRow( );
		
//		bool bFirst = true;
		unsigned int uiCount = 0;
		
		while( vecQueryAnn.size( ) == 6 )
		{
			if( atoi( vecQueryAnn[5].c_str( ) ) < pRequest->user.ucAccess )
			{
				if( ( UTIL_StringTo64( vecQueryAnn[2].c_str( ) ) > last_time_info && last_time_info > 0 ) || uiCount < CFG_GetInt( "bnbt_announcements_max", 3 ) )
				{
					pResponse->strContent += "<tr class=\"postheader\">\n";
					pResponse->strContent += "<td class=\"posttime\">" + vecQueryAnn[1] + "</td>\n";
					pResponse->strContent += "<td class=\"postname\"><a class=\"post\" href=\"javascript:;\" onclick=\"javascript: change('postcontent" + vecQueryAnn[0] + "');\">" + UTIL_RemoveHTML( vecQueryAnn[3] ) + "</a>";
					if( vecQueryAnn[5] != "0" )
						pResponse->strContent += " [<span class=\"hot\">" + UTIL_AccessToString( atoi( vecQueryAnn[5].c_str( ) ) ) + "</span>]";
					if( ( UTIL_StringTo64( vecQueryAnn[2].c_str( ) ) > last_time_info && last_time_info > 0 ) || uiCount == 0 )
						pResponse->strContent += " <span class=\"new\">(" + gmapLANG_CFG["new"] + ")</span>";
					pResponse->strContent += "</td>\n";
					pResponse->strContent += "</tr>\n";
					pResponse->strContent += "<tr class=\"postcontent\" id=\"postcontent" + vecQueryAnn[0] + "\"";
					if( ( UTIL_StringTo64( vecQueryAnn[2].c_str( ) ) > last_time_info && last_time_info > 0 ) || uiCount == 0 )
						pResponse->strContent += " style=\"display: show\">";
					else
						pResponse->strContent += " style=\"display: none\">";
				
					pResponse->strContent += "<td class=\"postcontent\" colspan=2>";

					pResponse->strContent += UTIL_RemoveHTML2( vecQueryAnn[4] ) + "</td>\n</tr>";

//					bFirst = false;

					uiCount++;
				}
			}
			vecQueryAnn = pQueryAnn->nextRow( );
		}
		
		delete pQueryAnn;
		
		pResponse->strContent += "</table>\n</td>\n";
		pResponse->strContent += "</tr></table>\n";
	
		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INFO ) );
	}
	else
	{
		// Not authorised

		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["info_page"], string( CSS_INFO ), string( ), NOT_INDEX, CODE_401 );

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INFO ) );
	}
}

void CTracker :: serverResponseAnnouncementsGET( struct request_t *pRequest, struct response_t *pResponse )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
		if( IsIPBanned( pRequest, pResponse, btv, gmapLANG_CFG["info_page"], string( CSS_INFO ), NOT_INDEX ) )
			return;

	if( !pRequest->user.strUID.empty( ) && ( m_ucInfoAccess == 0 || ( pRequest->user.ucAccess & m_ucInfoAccess ) ) )
	{
		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["info_page"], string( CSS_INFO ), string( ), NOT_INDEX, CODE_200 );
		
		const string cstrPostAction( pRequest->mapParams["action"] );
		string cstrPostNum( pRequest->mapParams["post"] );
		
		pResponse->strContent += "<script type=\"text/javascript\">\n";
		pResponse->strContent += "<!--\n";
		
		pResponse->strContent += UTIL_JS_Edit_Tool_Bar( "post_content.post_content" );

		// hide
		pResponse->strContent += "function hide(id) {\n";
		pResponse->strContent += "  var element = document.getElementById( id );\n";
		pResponse->strContent += "  element.style.display=\"none\";\n";
		pResponse->strContent += "}\n";
		
		// display
		pResponse->strContent += "function display(id) {\n";
		pResponse->strContent += "  var element = document.getElementById( id );\n";
		pResponse->strContent += "  element.style.display=\"\";\n";
		pResponse->strContent += "}\n";
		
		pResponse->strContent += "var hided = \"true\";\n";
		
		pResponse->strContent += "function change(id) {\n";
		pResponse->strContent += "  var element = document.getElementById( id );\n";
		pResponse->strContent += "  if (element.style.display == \"\") {\n";
		pResponse->strContent += "    hide( id ); }\n";
		pResponse->strContent += "  else {\n";
		pResponse->strContent += "    display( id ); }\n";
		pResponse->strContent += "}\n";
		
		pResponse->strContent += "//-->\n";
		pResponse->strContent += "</script>\n\n";
		
		if( pRequest->user.ucAccess & m_ucAccessAdmin )
		{
			if( cstrPostAction == "new" )
			{
				pResponse->strContent += "<div class=\"change_post\">\n";
				pResponse->strContent += "<p class=\"change_post\">" + UTIL_RemoveHTML( gmapLANG_CFG["post_new"] ) + "</p>\n";
				pResponse->strContent += "<table class=\"change_post\">\n";
				pResponse->strContent += "<form name=\"post_content\" method=\"post\" action=\"" + RESPONSE_STR_ANNOUNCEMENTS_HTML + "\" enctype=\"multipart/form-data\">\n";
				pResponse->strContent += "<input name=\"action\" type=hidden value=\"new\">\n";
				pResponse->strContent += "<tr class=\"change_post\">\n<th class=\"change_post\">" + gmapLANG_CFG["new_post_access"] + "</th>\n";
				pResponse->strContent += "<td class=\"change_post\"><select name=\"access\">\n";
				pResponse->strContent += "<option value=\"0\" selected>" + gmapLANG_CFG["access_basic"] + "\n";
				pResponse->strContent += "<option value=\"8\">" + gmapLANG_CFG["access_uploader"] + "\n";
				pResponse->strContent += "<option value=\"16\">" + gmapLANG_CFG["access_moderator"] + "\n";
				pResponse->strContent += "<option value=\"32\">" + gmapLANG_CFG["access_admin"] + "\n";
				pResponse->strContent += "</select>\n";
				pResponse->strContent += "<tr class=\"change_post\">\n<th class=\"change_post\">" + gmapLANG_CFG["new_post_name"] + "</th>\n";
				pResponse->strContent += "<td class=\"change_post\"><input id=\"post_name\" name=\"post_name\" alt=\"[" + gmapLANG_CFG["new_post_name"] + "]\" type=text size=96 maxlength=" + CAtomInt( MAX_FILENAME_LEN ).toString( ) + " value=\"" + UTIL_RemoveHTML( gmapLANG_CFG["new_post"] ) + "\"></td>\n</tr>\n";
				pResponse->strContent += "<tr class=\"change_post\">\n<th class=\"change_post\">" + gmapLANG_CFG["new_post"] + "</th>\n";
				pResponse->strContent += "<td class=\"change_post\">";
				pResponse->strContent += UTIL_Edit_Tool_Bar( );

				pResponse->strContent += "<br><textarea id=\"post_content\" name=\"post_content\" rows=10 cols=96>" + UTIL_RemoveHTML3( gmapLANG_CFG["new_post_content"] ) + "</textarea></td>\n</tr>\n";
				pResponse->strContent += "<tr class=\"change_post\">\n<td class=\"change_post\" colspan=\"2\">";
				pResponse->strContent += "<div class=\"change_post_button\">\n";
				pResponse->strContent += Button_Submit( "submit_change", string( gmapLANG_CFG["post_new"] ) );
				pResponse->strContent += "\n</div>\n</td>\n</tr>\n";
				pResponse->strContent += "</form>\n</table>\n</div>\n\n";
				// Output common HTML tail
				HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INFO ) );
				
				return;
			}
			if( cstrPostAction == "del" )
			{
				CMySQLQuery *pQueryAnn = new CMySQLQuery( "SELECT bid FROM announcements ORDER BY bposted DESC LIMIT 1" );
		
				vector<string> vecQueryAnn;
			
				vecQueryAnn.reserve(1);

				vecQueryAnn = pQueryAnn->nextRow( );
				
				delete pQueryAnn;
				
				if( vecQueryAnn.size( ) == 1 )
					CMySQLQuery mq01( "DELETE FROM announcements WHERE bid=" + vecQueryAnn[0] );
				pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["post_deleted"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_info"] + "\" href=\"" + RESPONSE_STR_ANNOUNCEMENTS_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";
				
				JS_ReturnToPage( pRequest, pResponse, ANNOUNCEMENTS_HTML );
				
				// Output common HTML tail
				HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INFO ) );
				
				return;
			}
			if( cstrPostAction == "edit" )
			{
				if( !cstrPostNum.empty( ) )
				{
					CMySQLQuery *pQueryAnn = new CMySQLQuery( "SELECT bid,btitle,bannouncement FROM announcements WHERE bid=" + cstrPostNum );
		
					vector<string> vecQueryAnn;
				
					vecQueryAnn.reserve(3);

					vecQueryAnn = pQueryAnn->nextRow( );
					
					delete pQueryAnn;
					
					if( vecQueryAnn.size( ) == 3 )
					{
						pResponse->strContent += "<div class=\"change_post\">\n";
						pResponse->strContent += "<p class=\"change_post\">" + gmapLANG_CFG["stats_change_post"] + "</p>\n";
						pResponse->strContent += "<h3>" + UTIL_RemoveHTML( vecQueryAnn[1] ) + "</h3>\n";
						pResponse->strContent += "<table class=\"change_post\">\n";
						pResponse->strContent += "<form name=\"post_content\" method=\"post\" action=\"" + RESPONSE_STR_ANNOUNCEMENTS_HTML + "\" enctype=\"multipart/form-data\">\n";
						pResponse->strContent += "<input name=\"post\" type=hidden value=\"" + vecQueryAnn[0] + "\">\n";
						pResponse->strContent += "<input name=\"action\" type=hidden value=\"edit\">\n";
						pResponse->strContent += "<tr class=\"change_post\">\n<th class=\"change_post\">" + gmapLANG_CFG["new_post_name"] + "</th>\n";
						pResponse->strContent += "<td class=\"change_post\"><input id=\"post_name\" name=\"post_name\" alt=\"[" + gmapLANG_CFG["new_post_name"] + "]\" type=text size=96 maxlength=" + CAtomInt( MAX_FILENAME_LEN ).toString( ) + " value=\"" + UTIL_RemoveHTML( vecQueryAnn[1] ) + "\"></td>\n</tr>\n";
						pResponse->strContent += "<tr class=\"change_post\">\n<th class=\"change_post\">" + gmapLANG_CFG["new_post"] + "</th>\n";
						pResponse->strContent += "<td class=\"change_post\">";
						pResponse->strContent += UTIL_Edit_Tool_Bar( );

						pResponse->strContent += "<br><textarea id=\"post_content\" name=\"post_content\" rows=10 cols=96>" + UTIL_RemoveHTML3( vecQueryAnn[2] ) + "</textarea></td>\n</tr>\n";
						pResponse->strContent += "<tr class=\"change_post\">\n<td class=\"change_post_button\" colspan=\"2\">";
						pResponse->strContent += Button_Submit( "submit_change", string( gmapLANG_CFG["post_change"] ) );
						pResponse->strContent += "\n</td>\n</tr>\n";
						pResponse->strContent += "</form>\n</table>\n</div>\n\n";
						// Output common HTML tail
						HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INFO ) );
						
						return;
					}
				}
				pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["stats_invalid_operation"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_info"] + "\" href=\"" + RESPONSE_STR_ANNOUNCEMENTS_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";
				// Output common HTML tail
				HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INFO ) );
				
				return;
			}
		}
			
		int64 last_time_info = 0;

		if( !pRequest->user.strUID.empty( ) )
		{
			CMySQLQuery *pQueryUser = new CMySQLQuery( "SELECT UNIX_TIMESTAMP(blast_info) FROM users WHERE buid=" + pRequest->user.strUID );
	
			vector<string> vecQueryUser;
	
			vecQueryUser.reserve(1);
			
			vecQueryUser = pQueryUser->nextRow( );
		
			delete pQueryUser;
			
			if( vecQueryUser.size( ) == 1 )
			{
				last_time_info = UTIL_StringTo64( vecQueryUser[0].c_str( ) );
			}
			
			CMySQLQuery mq01( "UPDATE users SET blast_info=NOW() WHERE buid=" + pRequest->user.strUID );
		}
		
		pResponse->strContent += "<table class=\"post_table_admin\">\n";
		
		if(  pRequest->user.ucAccess & m_ucAccessAdmin )
		{
			pResponse->strContent += "<tr>\n";
			pResponse->strContent += "<td class=\"admin\" colspan=3>[<a class=\"black\" href=\"" + RESPONSE_STR_ANNOUNCEMENTS_HTML + "?action=new\">" + gmapLANG_CFG["post_new"] + "</a>] [<a class=\"red\" href=\"javascript: ;\" onClick=\"javascript: if( confirm('" + gmapLANG_CFG["post_delete_q"] + "') ) window.location='" + RESPONSE_STR_ANNOUNCEMENTS_HTML + "?action=del'\">" + gmapLANG_CFG["post_del"] + "</a>]</td>";
			pResponse->strContent += "</tr>\n";
		}
			
		CMySQLQuery *pQueryAnn = new CMySQLQuery( "SELECT bid,bposted,UNIX_TIMESTAMP(bposted),btitle,bannouncement,baccess FROM announcements ORDER BY bposted DESC" );
		
		vector<string> vecQueryAnn;
	
		vecQueryAnn.reserve(6);

		vecQueryAnn = pQueryAnn->nextRow( );
		
		bool bFirst = true;
		
		while( vecQueryAnn.size( ) == 6 )
		{
			if( atoi( vecQueryAnn[5].c_str( ) ) < pRequest->user.ucAccess )
			{
				pResponse->strContent += "<tr class=\"postheader\">\n";
				pResponse->strContent += "<td class=\"posttime\">" + vecQueryAnn[1] + "</td>\n";
				pResponse->strContent += "<td class=\"postname\"><a class=\"post\" href=\"javascript:;\" onclick=\"javascript: change('postcontent" + vecQueryAnn[0] + "');\">" + UTIL_RemoveHTML( vecQueryAnn[3] ) + "</a>";
				if( vecQueryAnn[5] != "0" )
					pResponse->strContent += " [<span class=\"hot\">" + UTIL_AccessToString( atoi( vecQueryAnn[5].c_str( ) ) ) + "</span>]";
				if( ( UTIL_StringTo64( vecQueryAnn[2].c_str( ) ) > last_time_info && last_time_info > 0 ) || bFirst )
					pResponse->strContent += " <span class=\"new\">(" + gmapLANG_CFG["new"] + ")</span>";
				pResponse->strContent += "</td>";
				if( pRequest->user.ucAccess & m_ucAccessAdmin )
				{
					pResponse->strContent += "<td class=\"postadmin\">[<a href=\"" + RESPONSE_STR_ANNOUNCEMENTS_HTML + "?post=" + vecQueryAnn[0] + "&amp;action=edit\">" + gmapLANG_CFG["edit"] + "</a>]</td>";
				}
				pResponse->strContent += "</tr>\n";
				pResponse->strContent += "<tr class=\"postcontent\" id=\"postcontent" + vecQueryAnn[0] + "\"";
				if( vecQueryAnn[0] == cstrPostNum || ( UTIL_StringTo64( vecQueryAnn[2].c_str( ) ) > last_time_info && last_time_info > 0 ) || ( bFirst && cstrPostNum.empty( ) ) )
					pResponse->strContent += " style=\"display: show\">";
				else
					pResponse->strContent += " style=\"display: none\">";
				
				pResponse->strContent += "<td class=\"postcontent\"";
				if(  pRequest->user.ucAccess & m_ucAccessAdmin )
					pResponse->strContent += " colspan=3>";
				else
					pResponse->strContent += " colspan=2>";
				pResponse->strContent += UTIL_RemoveHTML2( vecQueryAnn[4] ) + "</td>\n</tr>";

//				if( bFirst )
					bFirst = false;
			}
			vecQueryAnn = pQueryAnn->nextRow( );
		}
		
		delete pQueryAnn;
		
		pResponse->strContent += "</table>\n";
	
		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INFO ) );
	}
	else
	{
		// Not authorised

		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["info_page"], string( CSS_INFO ), string( ), NOT_INDEX, CODE_401 );

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INFO ) );
	}
}

void CTracker :: serverResponseAnnouncementsPOST( struct request_t *pRequest, struct response_t *pResponse, CAtomList *pPost )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
		if( IsIPBanned( pRequest, pResponse, btv, gmapLANG_CFG["post_page"], string( CSS_INFO ), NOT_INDEX ) )
			return;
	
	if( !pRequest->user.strUID.empty( ) && ( pRequest->user.ucAccess & m_ucAccessAdmin ) )
	{
		string cstrPostNum = string( );
		string strPostAction = string( );
		string strPostAccess = string( );
		string strPostName = string( );
		string strPost = string( );
		
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
							
							if( strName == "post" )
								cstrPostNum = pData->toString( );
							else if( strName == "action" )
								strPostAction = pData->toString( );
							else if( strName == "access" )
								strPostAccess = pData->toString( );
							else if( strName == "post_name" )
								strPostName = pData->toString( );
							else if( strName == "post_content" )
								strPost = pData->toString( );
						}
						else
						{
							// Output common HTML head
							HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["info_page"], string( CSS_INFO ), string( ), NOT_INDEX, CODE_400 );

							// failed
							pResponse->strContent += "<p class=\"failed\">" + gmapLANG_CFG["failed"] + "</p>\n";
							// Signal a bad request
							pResponse->strContent += "<p class=\"body_upload\">400 " + gmapLANG_CFG["server_response_400"] + "</p>\n";

							// Output common HTML tail
							HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INFO ) );

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
			HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["info_page"], string( CSS_INFO ), string( ), NOT_INDEX, CODE_400 );

			// failed
			pResponse->strContent += "<p class=\"failed\">" + gmapLANG_CFG["failed"] + "</p>\n";
			// Signal a bad request
			pResponse->strContent += "<p class=\"body_upload\">400 " + gmapLANG_CFG["server_response_400"] + "</p>\n";

			// Output common HTML tail
			HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INFO ) );

			if( gbDebug )
				UTIL_LogPrint( "Upload Warning - Bad request (no post received)\n" );

			return;
		}
		
		if( strPostName.empty( ) )
			strPostName = gmapLANG_CFG["new_post"];
		if( strPost.empty( ) )
			strPostName = gmapLANG_CFG["new_post_content"];
		
		if( !strPostAction.empty( ) )
		{
			HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["post_page"], string( CSS_INFO ), string( ANNOUNCEMENTS_HTML ), NOT_INDEX, CODE_200 );
			
			if( strPostAction == "new" )
			{
				string strQuery = "INSERT INTO announcements (bposted,btitle,bannouncement,baccess) VALUES(NOW(),\'";
				strQuery += UTIL_StringToMySQL( strPostName );
				strQuery += "\',\'" + UTIL_StringToMySQL( strPost );
				strQuery += "\'," + strPostAccess;
				strQuery += ")";

				CMySQLQuery mq01( strQuery );

				pResponse->strContent += "<p class=\"changed_post\">" + UTIL_Xsprintf( gmapLANG_CFG["post_return"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_info"] + "\" href=\"" + RESPONSE_STR_ANNOUNCEMENTS_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";
				
				HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INFO ) );
				
				return;
			}
			else if( strPostAction == "edit" )
			{
				string strQuery = "UPDATE announcements SET btitle=\'";
				strQuery += UTIL_StringToMySQL( strPostName );
				strQuery += "\',bannouncement=\'" + UTIL_StringToMySQL( strPost );
				strQuery += "\' WHERE bid=" + cstrPostNum;
				
				CMySQLQuery mq01( strQuery );
				
				pResponse->strContent += "<div class=\"changed_post\">\n";
				pResponse->strContent += "<table class=\"changed_post\">\n";
				pResponse->strContent += "<tr>\n<td>\n<ul>\n";
				pResponse->strContent += "<li class=\"changed_post\">" + UTIL_Xsprintf( gmapLANG_CFG["post_changed_name"].c_str( ), UTIL_RemoveHTML( strPostName ).c_str( ) ) + "</li>\n";
				pResponse->strContent += "</ul>\n</td>\n</tr>\n</table>\n</div>\n";
				
				pResponse->strContent += "<p class=\"changed_post\">" + UTIL_Xsprintf( gmapLANG_CFG["post_return"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_info"] + "\" href=\"" + RESPONSE_STR_ANNOUNCEMENTS_HTML + "?post=" + cstrPostNum + "\">" ).c_str( ), "</a>" ) + "</p>\n";
				
				HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INFO ) );
				
				return;
			}
			else
				HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INFO ) );
		}
	}
	else
	{
		// Not authorised

		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["info_page"], string( CSS_INFO ), string( ), NOT_INDEX, CODE_401 );

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INFO ) );
	}
}

void CTracker :: serverResponseVotesGET( struct request_t *pRequest, struct response_t *pResponse )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
		if( IsIPBanned( pRequest, pResponse, btv, gmapLANG_CFG["info_page"], string( CSS_INFO ), NOT_INDEX ) )
			return;

	if( !pRequest->user.strUID.empty( ) && ( pRequest->user.ucAccess & m_ucAccessAdmin ) )
	{
		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["info_page"], string( CSS_INFO ), string( ), NOT_INDEX, CODE_200 );
		
		const string cstrAction( pRequest->mapParams["action"] );
		string cstrVote( pRequest->mapParams["vote"] );
		
		pResponse->strContent += "<script type=\"text/javascript\">\n";
		pResponse->strContent += "<!--\n";
		
		pResponse->strContent += UTIL_JS_Edit_Tool_Bar( "post_content.post_content" );

		// hide
		pResponse->strContent += "function hide(id) {\n";
		pResponse->strContent += "  var element = document.getElementById( id );\n";
		pResponse->strContent += "  element.style.display=\"none\";\n";
		pResponse->strContent += "}\n";
		
		// display
		pResponse->strContent += "function display(id) {\n";
		pResponse->strContent += "  var element = document.getElementById( id );\n";
		pResponse->strContent += "  element.style.display=\"\";\n";
		pResponse->strContent += "}\n";
		
		pResponse->strContent += "var hided = \"true\";\n";
		
		pResponse->strContent += "function change(id) {\n";
		pResponse->strContent += "  var element = document.getElementById( id );\n";
		pResponse->strContent += "  if (element.style.display == \"\") {\n";
		pResponse->strContent += "    hide( id ); }\n";
		pResponse->strContent += "  else {\n";
		pResponse->strContent += "    display( id ); }\n";
		pResponse->strContent += "}\n";
		
		pResponse->strContent += "//-->\n";
		pResponse->strContent += "</script>\n\n";
		
//		if( pRequest->user.ucAccess & m_ucAccessAdmin )
//		{
			if( !cstrAction.empty( ) )
				{
				if( cstrAction == "new" )
				{
					unsigned int uiOptionMax = (unsigned int)CFG_GetInt( "bnbt_vote_option_max", 16 );
				
					pResponse->strContent += "<form name=\"vote_option\" method=\"post\" action=\"" + RESPONSE_STR_VOTES_HTML + "\" enctype=\"multipart/form-data\">\n";
					pResponse->strContent += "<input name=\"action\" type=hidden value=\"new\">\n";
					pResponse->strContent += "<table class=\"change_vote\">\n";
					pResponse->strContent += "<tr class=\"change_vote\">\n";
					pResponse->strContent += "<td class=\"change_vote\">" + gmapLANG_CFG["vote_title"] + "</td>";
					pResponse->strContent += "<td class=\"change_vote\"><input name=\"title\" type=text size=96 maxlength=" + CAtomInt( MAX_FILENAME_LEN ).toString( ) + " value=\"\"></td>\n</tr>\n";
					pResponse->strContent += "<tr class=\"change_vote\">\n";
					pResponse->strContent += "<td class=\"change_vote\">" + gmapLANG_CFG["vote_option_skip"] + "</td>";
					pResponse->strContent += "<td class=\"change_vote\"><input name=\"option0\" type=text size=96 maxlength=" + CAtomInt( MAX_FILENAME_LEN ).toString( ) + " value=\"" + gmapLANG_CFG["vote_option_skip_desc"] + "\"></td>\n</tr>\n";
				
					for( unsigned int uiOption = 1; uiOption <= uiOptionMax; uiOption++ )
					{
						pResponse->strContent += "<tr class=\"change_vote\">\n";
						pResponse->strContent += "<td class=\"change_vote\">" + CAtomInt( uiOption ).toString( ) + "</td>";
						pResponse->strContent += "<td class=\"change_vote\"><input name=\"option" + CAtomInt( uiOption ).toString( ) + "\" type=text size=96 maxlength=" + CAtomInt( MAX_FILENAME_LEN ).toString( ) + " value=\"\"></td>\n</tr>\n";
					}
					pResponse->strContent += "<tr class=\"change_vote\">\n";
					pResponse->strContent += "<td class=\"change_vote\" colspan=2>\n";
					pResponse->strContent += Button_Submit( "submit_vote_new", string( gmapLANG_CFG["vote_new"] ) );
					pResponse->strContent += "</td></tr>\n";
					pResponse->strContent += "</table>\n";
					pResponse->strContent += "</form>\n";
				
					HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INFO ) );
				
					return;
				}
				if( cstrAction == "del" )
				{
					CMySQLQuery *pQueryVotes = new CMySQLQuery( "SELECT bid FROM votes ORDER BY bcreated DESC LIMIT 1" );
		
					vector<string> vecQueryVotes;
			
					vecQueryVotes.reserve(1);

					vecQueryVotes = pQueryVotes->nextRow( );
				
					delete pQueryVotes;
				
					if( vecQueryVotes.size( ) == 1 )
						CMySQLQuery mq01( "DELETE FROM votes WHERE bid=" + vecQueryVotes[0] );
					pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["vote_deleted"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_info"] + "\" href=\"" + RESPONSE_STR_VOTES_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";
				
					JS_ReturnToPage( pRequest, pResponse, VOTES_HTML );
				
					// Output common HTML tail
					HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INFO ) );
				
					return;
				}
				if( cstrAction == "edit" )
				{
					if( !cstrVote.empty( ) )
					{
						unsigned int uiOptionMax = (unsigned int)CFG_GetInt( "bnbt_vote_option_max", 16 );
					
						unsigned int uiOption = 0;
					
						CMySQLQuery *pQueryVotes = new CMySQLQuery( "SELECT bid,btitle FROM votes WHERE bid=" + cstrVote );
		
						vector<string> vecQueryVotes;
				
						vecQueryVotes.reserve(2);

						vecQueryVotes = pQueryVotes->nextRow( );
					
						delete pQueryVotes;
					
						if( vecQueryVotes.size( ) == 2 )
						{
							CMySQLQuery *pQueryVote = new CMySQLQuery( "SELECT boptionid,boption,bvotecount FROM votesoption WHERE bid=" + vecQueryVotes[0] + " ORDER BY boptionid" );
		
							vector<string> vecQueryVote;
	
							vecQueryVote.reserve(3);

							vecQueryVote = pQueryVote->nextRow( );
						
							pResponse->strContent += "<form name=\"vote_option\" method=\"post\" action=\"" + RESPONSE_STR_VOTES_HTML + "\" enctype=\"multipart/form-data\">\n";
							pResponse->strContent += "<input name=\"vote\" type=hidden value=\"" + cstrVote + "\">\n";
							pResponse->strContent += "<input name=\"action\" type=hidden value=\"edit\">\n";
							pResponse->strContent += "<table class=\"change_vote\">\n";
							pResponse->strContent += "<tr class=\"change_vote\">\n";
							pResponse->strContent += "<td class=\"change_vote\">" + gmapLANG_CFG["vote_title"] + "</td>";
							pResponse->strContent += "<td class=\"change_vote\"><input name=\"title\" type=text size=96 maxlength=" + CAtomInt( MAX_FILENAME_LEN ).toString( ) + " value=\"" + UTIL_RemoveHTML( vecQueryVotes[1] ) + "\"></td>\n</tr>\n";
			
							while( vecQueryVote.size( ) == 3 || uiOption < uiOptionMax )
							{
								pResponse->strContent += "<tr class=\"change_vote\">\n";
								pResponse->strContent += "<td class=\"change_vote\">";

								if( uiOption == 0 )
									pResponse->strContent += gmapLANG_CFG["vote_option_skip"];
								else
									pResponse->strContent += CAtomInt( uiOption ).toString( );

								pResponse->strContent += "</td>";
								pResponse->strContent += "<td class=\"change_vote\"><input name=\"option" + CAtomInt( uiOption ).toString( ) + "\" type=text size=96 maxlength=" + CAtomInt( MAX_FILENAME_LEN ).toString( ) + " value=\"";
								if( vecQueryVote.size( ) == 3 && uiOption == UTIL_StringTo64( vecQueryVote[0].c_str( ) ) )
								{
									pResponse->strContent += UTIL_RemoveHTML( vecQueryVote[1] );
								
									vecQueryVote = pQueryVote->nextRow( );
								}
								pResponse->strContent += "\"></td>\n</tr>\n";
							
								uiOption++;
							}
			
							delete pQueryVote;
						
							pResponse->strContent += "<tr class=\"change_vote\">\n";
							pResponse->strContent += "<td class=\"change_vote\" colspan=2>\n";
							pResponse->strContent += Button_Submit( "submit_vote_edit", string( gmapLANG_CFG["vote_edit"] ) );
							pResponse->strContent += "</td></tr>\n";
							pResponse->strContent += "</table>\n";
							pResponse->strContent += "</form>\n";
						
							// Output common HTML tail
							HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INFO ) );
						
							return;
						}
					}
				}
				if( cstrAction == "open" )
				{
					if( !cstrVote.empty( ) )
					{
						CMySQLQuery *pQueryVotes = new CMySQLQuery( "SELECT bid,UNIX_TIMESTAMP(bopen),UNIX_TIMESTAMP(bclosed) FROM votes WHERE bid=" + cstrVote );
		
						vector<string> vecQueryVotes;
				
						vecQueryVotes.reserve(3);

						vecQueryVotes = pQueryVotes->nextRow( );
					
						delete pQueryVotes;
					
						if( vecQueryVotes.size( ) == 3 )
						{
							if( vecQueryVotes[1] == "0" && vecQueryVotes[2] == "0" )
								CMySQLQuery mq01( "UPDATE votes SET bopen=NOW() WHERE bid=" + cstrVote );
							else if( vecQueryVotes[1] != "0" && vecQueryVotes[2] != "0" )
								CMySQLQuery mq02( "UPDATE votes SET bclosed=0 WHERE bid=" + cstrVote );
						}
					
						pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["vote_deleted"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_info"] + "\" href=\"" + RESPONSE_STR_VOTES_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";
				
						JS_ReturnToPage( pRequest, pResponse, VOTES_HTML );
				
						// Output common HTML tail
						HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INFO ) );
					
						return;
					}
				}
				if( cstrAction == "close" )
				{
					if( !cstrVote.empty( ) )
					{
						CMySQLQuery *pQueryVotes = new CMySQLQuery( "SELECT bid,UNIX_TIMESTAMP(bopen),UNIX_TIMESTAMP(bclosed) FROM votes WHERE bid=" + cstrVote );
		
						vector<string> vecQueryVotes;
				
						vecQueryVotes.reserve(3);

						vecQueryVotes = pQueryVotes->nextRow( );
					
						delete pQueryVotes;
					
						if( vecQueryVotes.size( ) == 3 )
						{
							if( vecQueryVotes[1] != "0" && vecQueryVotes[2] == "0" )
								CMySQLQuery mq01( "UPDATE votes SET bclosed=NOW() WHERE bid=" + cstrVote );
						}
					
						pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["vote_deleted"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_info"] + "\" href=\"" + RESPONSE_STR_VOTES_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";
				
						JS_ReturnToPage( pRequest, pResponse, VOTES_HTML );
				
						// Output common HTML tail
						HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INFO ) );
					
						return;
					}
				}
				pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["stats_invalid_operation"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_info"] + "\" href=\"" + RESPONSE_STR_VOTES_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";
				// Output common HTML tail
				HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INFO ) );
			
				return;
			}
//		}
			
		pResponse->strContent += "<table class=\"vote_table_admin\">\n";
		
//		if(  pRequest->user.ucAccess & m_ucAccessAdmin )
//		{
			pResponse->strContent += "<tr>\n";
			pResponse->strContent += "<td class=\"admin\" colspan=9>[<a class=\"black\" href=\"" + RESPONSE_STR_VOTES_HTML + "?action=new\">" + gmapLANG_CFG["vote_new"] + "</a>] [<a class=\"red\" href=\"javascript: ;\" onClick=\"javascript: if( confirm('" + gmapLANG_CFG["vote_delete_q"] + "') ) window.location='" + RESPONSE_STR_VOTES_HTML + "?action=del'\">" + gmapLANG_CFG["vote_del"] + "</a>]</td>";
			pResponse->strContent += "</tr>\n";
//		}
			
		CMySQLQuery *pQueryVotes = new CMySQLQuery( "SELECT bid,btitle,bcreated,bopen,bclosed,UNIX_TIMESTAMP(bopen),UNIX_TIMESTAMP(bclosed),bvotecount FROM votes ORDER BY bcreated DESC" );
		
		vector<string> vecQueryVotes;
	
		vecQueryVotes.reserve(8);

		vecQueryVotes = pQueryVotes->nextRow( );
		
		while( vecQueryVotes.size( ) == 8 )
		{
			pResponse->strContent += "<tr class=\"voteheader\">\n";
			pResponse->strContent += "<th class=\"votetime\">" + gmapLANG_CFG["vote_time_created"] + "</th>\n";
			pResponse->strContent += "<td class=\"votetime\">" + vecQueryVotes[2] + "</th>\n";
			pResponse->strContent += "<th class=\"votetime\">" + gmapLANG_CFG["vote_time_open"] + "</th>\n";
			pResponse->strContent += "<td class=\"votetime\">" + vecQueryVotes[3] + "</th>\n";
			pResponse->strContent += "<th class=\"votetime\">" + gmapLANG_CFG["vote_time_closed"] + "</th>\n";
			pResponse->strContent += "<td class=\"votetime\">" + vecQueryVotes[4] + "</th>\n";
			pResponse->strContent += "<th class=\"votetime\">" + gmapLANG_CFG["vote_status"] + "</th>\n";
			pResponse->strContent += "<td class=\"votetime\">";
			if( vecQueryVotes[5] != "0" && vecQueryVotes[6] == "0" )
				pResponse->strContent += "<span class=\"green\">" + gmapLANG_CFG["vote_status_open"] + "</span>";
			else
			{
				if( vecQueryVotes[5] != "0" )
					pResponse->strContent += "<span class=\"red\">" + gmapLANG_CFG["vote_status_closed"] + "</span>";
				else
					pResponse->strContent += "<span class=\"red\">" + gmapLANG_CFG["vote_status_not_open"] + "</span>";
			}
			pResponse->strContent += "</th>\n";
			
//			if( pRequest->user.ucAccess & m_ucAccessAdmin )
//			{
				pResponse->strContent += "<td class=\"voteadmin\">[<a href=\"" + RESPONSE_STR_VOTES_HTML + "?vote=" + vecQueryVotes[0] + "&amp;action=edit\">" + gmapLANG_CFG["vote_edit"] + "</a>]";
				if( vecQueryVotes[5] != "0" && vecQueryVotes[6] == "0" )
					pResponse->strContent += "[<a href=\"" + RESPONSE_STR_VOTES_HTML + "?vote=" + vecQueryVotes[0] + "&amp;action=close\">" + gmapLANG_CFG["vote_close"] + "</a>]</td>";
				else
					pResponse->strContent += "[<a href=\"" + RESPONSE_STR_VOTES_HTML + "?vote=" + vecQueryVotes[0] + "&amp;action=open\">" + gmapLANG_CFG["vote_open"] + "</a>]";
//			}
			
			pResponse->strContent += "</tr>\n";
			
			CMySQLQuery *pQueryVote = new CMySQLQuery( "SELECT boptionid,boption,bvotecount FROM votesoption WHERE bid=" + vecQueryVotes[0] + " ORDER BY boptionid" );
		
			vector<string> vecQueryVote;
	
			vecQueryVote.reserve(3);

			vecQueryVote = pQueryVote->nextRow( );
			
			if( vecQueryVote.size( ) == 3 )
			{
				string strSkip = string( );
				
				pResponse->strContent += "<tr class=\"vote\">";
				
				pResponse->strContent += "<td class=\"vote\" colspan=9>";
				
				pResponse->strContent += "<p class=\"votetitle\">" + UTIL_RemoveHTML( vecQueryVotes[1] ) + "</p>";

				while( vecQueryVote.size( ) == 3 )
				{
					if( vecQueryVote[0] != "0" )
					{
						pResponse->strContent += "<p class=\"voteoption\">";
					
						pResponse->strContent += UTIL_RemoveHTML( vecQueryVote[1] ) + ": " + vecQueryVote[2] + " / " + vecQueryVotes[7];
					
						pResponse->strContent += "</p>\n";
					}
					else
						strSkip = "<p class=\"votetotal\">" + UTIL_RemoveHTML( vecQueryVote[1] ) + ": " + vecQueryVote[2] + "</p>\n";
					
					vecQueryVote = pQueryVote->nextRow( );
				}
				pResponse->strContent += "<p class=\"votetotal\">" + UTIL_Xsprintf( gmapLANG_CFG["vote_total"].c_str( ), vecQueryVotes[7].c_str( ) ) + "</p>";
				
				pResponse->strContent += strSkip;
				
				pResponse->strContent += "</td>\n</tr>";
			}
			
			delete pQueryVote;
			
			vecQueryVotes = pQueryVotes->nextRow( );
		}
		
		delete pQueryVotes;
		
		pResponse->strContent += "</table>\n";
	
		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INFO ) );
	}
	else
	{
		// Not authorised

		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["info_page"], string( CSS_INFO ), string( ), NOT_INDEX, CODE_401 );

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INFO ) );
	}
}

void CTracker :: serverResponseVotesPOST( struct request_t *pRequest, struct response_t *pResponse, CAtomList *pPost )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
		if( IsIPBanned( pRequest, pResponse, btv, gmapLANG_CFG["info_page"], string( CSS_INFO ), NOT_INDEX ) )
			return;
	
	if( !pRequest->user.strUID.empty( ) && ( m_ucInfoAccess == 0 || ( pRequest->user.ucAccess & m_ucInfoAccess ) ) )
	{
		string strVote = string( );
		string strAction = string( );
		string strTitle = string( );
		string strOption = string( );
		string strSelected = string( );
		string strVoteFor = string( );
		vector< pair<string, string> > vecOption;
		
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
							
							if( strName == "vote" )
								strVote = pData->toString( );
							else if( strName.substr( 0, 6 ) == "option" )
							{
								strOption = pData->toString( );
								vecOption.push_back( pair<string, string>( strName.substr( 6 ), strOption ) );
							}
							else if( strName == "action" )
								strAction = pData->toString( );
							else if( strName == "title" )
								strTitle = pData->toString( );
							else if( strName == "vote_selectd" )
								strSelected = pData->toString( );
							else if( strName == "vote_for" )
								strVoteFor = pData->toString( );
						}
						else
						{
							// Output common HTML head
							HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["info_page"], string( CSS_INFO ), string( ), NOT_INDEX, CODE_400 );

							// failed
							pResponse->strContent += "<p class=\"failed\">" + gmapLANG_CFG["failed"] + "</p>\n";
							// Signal a bad request
							pResponse->strContent += "<p class=\"body_upload\">400 " + gmapLANG_CFG["server_response_400"] + "</p>\n";

							// Output common HTML tail
							HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INFO ) );

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
			HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["info_page"], string( CSS_INFO ), string( ), NOT_INDEX, CODE_400 );

			// failed
			pResponse->strContent += "<p class=\"failed\">" + gmapLANG_CFG["failed"] + "</p>\n";
			// Signal a bad request
			pResponse->strContent += "<p class=\"body_upload\">400 " + gmapLANG_CFG["server_response_400"] + "</p>\n";

			// Output common HTML tail
			HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INFO ) );

			if( gbDebug )
				UTIL_LogPrint( "Upload Warning - Bad request (no post received)\n" );

			return;
		}
		
		if( strTitle.empty( ) )
			strTitle = gmapLANG_CFG["vote_title_new"];

		if( !strAction.empty( ) && ( pRequest->user.ucAccess & m_ucAccessAdmin ) )
		{
			HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["info_page"], string( CSS_INFO ), string( VOTES_HTML ), NOT_INDEX, CODE_200 );
			
			if( !strTitle.empty( ) && !vecOption.empty( ) )
			{
				if( strAction == "new" )
				{
					CMySQLQuery *pQueryVote = new CMySQLQuery( "INSERT INTO votes (btitle,bcreated) VALUES('" + UTIL_StringToMySQL( strTitle ) + "',NOW())" );
				
					unsigned long ulLast = pQueryVote->lastInsertID( );
				
					delete pQueryVote;
				
					if( ulLast > 0 )
					{
						for( vector< pair< string, string > > :: iterator ulOption = vecOption.begin( ); ulOption != vecOption.end( ); ulOption++ )
							if( !(*ulOption).second.empty( ) )
								CMySQLQuery mq02( "INSERT INTO votesoption (bid,boptionid,boption) VALUES(" + CAtomInt( ulLast ).toString( ) + "," + (*ulOption).first + ",'" + UTIL_StringToMySQL( (*ulOption).second ) + "')" );
					}
				}
				else if( strAction == "edit" && !strVote.empty( ) )
				{
					CMySQLQuery mq01( "UPDATE votes SET btitle='" + UTIL_StringToMySQL( strTitle ) + "' WHERE bid=" + strVote );
					
					for( vector< pair< string, string > > :: iterator ulOption = vecOption.begin( ); ulOption != vecOption.end( ); ulOption++ )
					{
						if( !(*ulOption).second.empty( ) )
							CMySQLQuery mq03( "INSERT INTO votesoption (bid,boptionid,boption) VALUES(" + strVote + "," + (*ulOption).first + ",'" + UTIL_StringToMySQL( (*ulOption).second ) + "') ON DUPLICATE KEY UPDATE boption='" + UTIL_StringToMySQL( (*ulOption).second ) + "'" );
						else
							CMySQLQuery mq04( "DELETE FROM votesoption WHERE bid=" + strVote + " AND boptionid=" + (*ulOption).first );
					}
				}
				

//				pResponse->strContent += "<p class=\"changed_post\">" + UTIL_Xsprintf( gmapLANG_CFG["post_return"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_info"] + "\" href=\"" + RESPONSE_STR_ANNOUNCEMENTS_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";
				
				HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INFO ) );
				
				return;
			}
			else
				HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INFO ) );
		}
		else if( !strSelected.empty( ) )
		{
			HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["info_page"], string( CSS_INFO ), string( INFO_HTML ), NOT_INDEX, CODE_200 );
			
			CMySQLQuery *pQueryTicket = new CMySQLQuery( "SELECT boptionid FROM votesticket WHERE bid=" + strVoteFor + " AND buid=" + pRequest->user.strUID + " ORDER BY boptionid" );
		
			vector<string> vecQueryTicket;

			vecQueryTicket.reserve(1);

			vecQueryTicket = pQueryTicket->nextRow( );
			
			delete pQueryTicket;
			
			if( vecQueryTicket.size( ) == 0 )
			{
				CMySQLQuery *pQueryVote = new CMySQLQuery( "SELECT boptionid FROM votesoption WHERE bid=" + strVoteFor + " AND boptionid=" + strSelected );
		
				vector<string> vecQueryVote;
	
				vecQueryVote.reserve(1);

				vecQueryVote = pQueryVote->nextRow( );
				
				delete pQueryVote;
				
				if( vecQueryVote.size( ) == 1 )
				{
					if( strSelected != "0" )
						CMySQLQuery mq01( "UPDATE votes SET bvotecount=bvotecount+1 WHERE bid=" + strVoteFor );
					CMySQLQuery mq02( "UPDATE votesoption SET bvotecount=bvotecount+1 WHERE bid=" + strVoteFor + " AND boptionid=" + strSelected );
					CMySQLQuery mq03( "INSERT INTO votesticket (bid,buid,boptionid,bvotetime) VALUES(" + strVoteFor + "," + pRequest->user.strUID + "," + strSelected + ",NOW())" );
				}
			}
			
			HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INFO ) );
				
			return;
		}
	}
	else
	{
		// Not authorised

		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["info_page"], string( CSS_INFO ), string( ), NOT_INDEX, CODE_401 );

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INFO ) );
	}
}
