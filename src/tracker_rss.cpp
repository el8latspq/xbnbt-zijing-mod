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
 #include <sys/stat.h>
 #include <time.h>
#else
 #include <dirent.h>
//  #include <fcntl.h>
 #include <sys/stat.h>
 #include <sys/time.h>
#endif

#include "bnbt.h"
#include "bnbt_mysql.h"
#include "atom.h"
#include "bencode.h"
#include "config.h"
#include "html.h"
#include "md5.h"
#include "server.h"
#include "sort.h"
#include "tracker.h"
#include "util.h"

void CTracker :: serverResponseXML( struct request_t *pRequest, struct response_t *pResponse )
{
	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
	{
		// Set the start time
		const struct bnbttv btv( UTIL_CurrentTime( ) );

		if( IsIPBanned( pRequest, pResponse, btv, "IP Banned", string( CSS_INDEX ), NOT_INDEX ) )
			return;
	}

	pResponse->strCode = "200 " + gmapLANG_CFG["server_response_200"];

	pResponse->mapHeaders.insert( pair<string, string>( "Content-Type", gmapMime[".xml"] ) );

	pResponse->strContent = xmldump.strFile;
}

void CTracker :: serverResponseRSS( struct request_t *pRequest, struct response_t *pResponse )
{
	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
	{
		// Set the start time
		const struct bnbttv btv( UTIL_CurrentTime( ) );

		if( IsIPBanned( pRequest, pResponse, btv, "IP Banned", string( CSS_INDEX ), NOT_INDEX ) )
			return;
	}
	
	const string strSearch( pRequest->mapParams["search"] );
	const string strUploader( pRequest->mapParams["uploader"] );
	const string strIMDbID( pRequest->mapParams["imdb"] );
	const string strMatch( pRequest->mapParams["match"] );
	
	const string strChannelTag( pRequest->mapParams["tag"] );
	const string strMedium( pRequest->mapParams["medium"] );
	const string strQuality( pRequest->mapParams["quality"] );
	const string strEncode( pRequest->mapParams["encode"] );
	
	const string cstrDay( pRequest->mapParams["day"] );
	const string cstrSection( pRequest->mapParams["section"] );
	const string cstrMode( pRequest->mapParams["mode"] );
	const string cstrPasskey( pRequest->mapParams["passkey"] );

	string strUID = string( );
	unsigned char ucAccess = 0;

	if( !cstrPasskey.empty( ) )
	{
		CMySQLQuery *pQuery = new CMySQLQuery( "SELECT buid,baccess FROM users WHERE bpasskey=\'" + UTIL_StringToMySQL( cstrPasskey ) + "\'" );
				
		vector<string> vecQuery;
		
		vecQuery.reserve(2);

		vecQuery = pQuery->nextRow( );
		
		delete pQuery;

		if( vecQuery.size( ) == 2 )
		{
			strUID = vecQuery[0];
			ucAccess = (unsigned char)atoi( vecQuery[1].c_str( ) );
		}
	}
	
	vector<string> vecBookmark;
	vecBookmark.reserve(64);
	vector<string> vecFriend;
	vecFriend.reserve(64);

	if( !strUID.empty( ) )
	{
		if( ucAccess & m_ucAccessBookmark )
		{
			CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bid FROM bookmarks WHERE buid=" + strUID );
		
			vector<string> vecQuery;
		
			vecQuery.reserve(1);

			vecQuery = pQuery->nextRow( );

			while( vecQuery.size( ) == 1 )
			{
				vecBookmark.push_back( vecQuery[0] );

				vecQuery = pQuery->nextRow( );
			}
			
			delete pQuery;
		}
		
		if( cstrMode == "MyFriends" )
		{
			CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bfriendid FROM friends WHERE buid=" + strUID );
		
			vector<string> vecQuery;
		
			vecQuery.reserve(1);

			vecQuery = pQuery->nextRow( );

			while( vecQuery.size( ) == 1 )
			{
				vecFriend.push_back( vecQuery[0] );

				vecQuery = pQuery->nextRow( );
			}
			
			delete pQuery;
		}
	}

	vector<string> vecSearch;
	vecSearch.reserve(64);
	vector<string> vecUploader;
	vecUploader.reserve(64);
	vector<string> vecIMDb;
	vecIMDb.reserve(64);
	vector<string> vecFilter;
	vecFilter.reserve(64);
	vector<string> vecMedium;
	vecMedium.reserve(64);
	vector<string> vecQuality;
	vecQuality.reserve(64);
	vector<string> vecEncode;
	vecEncode.reserve(64);
	
	vecSearch = UTIL_SplitToVector( strSearch, " " );
	vecUploader = UTIL_SplitToVector( strUploader, " " );
	vecIMDb = UTIL_SplitToVector( strIMDbID, " " );
	vecFilter = UTIL_SplitToVector( strChannelTag, " " );
	vecMedium = UTIL_SplitToVector( strMedium, " " );
	vecQuality = UTIL_SplitToVector( strQuality, " " );
	vecEncode = UTIL_SplitToVector( strEncode, " " );

	pResponse->strCode = "200 " + gmapLANG_CFG["server_response_200"];

	pResponse->mapHeaders.insert( pair<string, string>( "Content-Type", gmapMime[".rss"] ) );
	pResponse->mapHeaders.insert( pair<string, string>( "Content-Type", gmapMime[".xml"] ) );

// 	pResponse->strContent = rssdump.strFile;
	
	if( !m_strTitle.empty( ) && !m_strDescription.empty( ) && ( !rssdump.strURL.empty( ) || m_bServeLocal ) )
	{

		string strData = string( );
		
		// xml version and encoding
		strData = "<?xml version=\"1.0\" encoding=\"" + gstrCharSet + "\" ?>\n";

		// xsl client-side processing template
		// * need to sort this *
		if( !rssdump.strURL.empty( ) )
			strData = "<?xml-stylesheet type=\"" + gmapMime[".xsl"] + "\" href=\"" + rssdump.strURL + "rss.xsl\" ?>\n";
		else if( m_bServeLocal )
			strData = "<?xml-stylesheet type=\"" + gmapMime[".xsl"] + "\" href=\"rss.xsl\" ?>\n";

		// rss version
		strData += "<rss version=\"2.0\" xmlns:torrentItem=\"http://xbnbt.sourceforge.net/ns/torrentItem#\">\n";

		// channel
		strData += "<channel>\n";

		// title
//		strData += "<title>" + m_strTitle + ( !strChannelTag.empty( ) ? " - " + strChannelTag : string( ) ) + "</title>\n";
		strData += "<title>" + m_strTitle;

		string strTitle = string( );
		
		if( !vecSearch.empty( ) )
		{
		 	strTitle += UTIL_RemoveHTML( strSearch );
		}

		if( !vecUploader.empty( ) )
		{
			if( !strTitle.empty( ) )
				strTitle += " - ";

		 	strTitle += UTIL_RemoveHTML( strUploader );
		}
			
		if( !vecFilter.empty( ) )
		{
			if( !m_vecTags.empty( ) )
			{
				if( !strTitle.empty( ) )
					strTitle += " - ";

				string strNameIndex = string( );
				string strTag = string( );
				
				for( vector<string> :: iterator ulKey = vecFilter.begin( ); ulKey != vecFilter.end( ); ulKey++ )
				{
					for( vector< pair< string, string > > :: iterator ulTagKey = m_vecTags.begin( ); ulTagKey != m_vecTags.end( ); ulTagKey++ )
					{
						strNameIndex = (*ulTagKey).first;
						strTag = (*ulTagKey).second;

						if( (*ulKey).length( ) > 2 )
						{
							if( *ulKey == strNameIndex )
								strTitle += UTIL_RemoveHTML( strTag );
						}
						else
						{
							if( *ulKey + "01" == strNameIndex )
								strTitle += UTIL_RemoveHTML( strTag.substr( 0, strTag.find( ' ' ) ) );
						}

					}

					if( ulKey + 1 != vecFilter.end( ) )
						strTitle += " &amp; ";
				}
			}
		}
		
		if( !vecMedium.empty( ) )
		{
			if( !m_vecMediums.empty( ) )
			{
				if( !strTitle.empty( ) )
					strTitle += " - ";

				for( vector<string> :: iterator ulKey = vecMedium.begin( ); ulKey != vecMedium.end( ); ulKey++ )
				{
					for( vector< string > :: iterator ulTagKey = m_vecMediums.begin( ); ulTagKey != m_vecMediums.end( ); ulTagKey++ )
					{
						if( *ulKey == *ulTagKey )
							strTitle += gmapLANG_CFG["medium_"+*ulTagKey];
					}

					if( ulKey + 1 != vecMedium.end( ) )
						strTitle += " &amp; ";
				}
			}
		}
		
		if( !vecQuality.empty( ) )
		{
			if( !m_vecQualities.empty( ) )
			{
				if( !strTitle.empty( ) )
					strTitle += " - ";

				for( vector<string> :: iterator ulKey = vecQuality.begin( ); ulKey != vecQuality.end( ); ulKey++ )
				{
					for( vector< string > :: iterator ulTagKey = m_vecQualities.begin( ); ulTagKey != m_vecQualities.end( ); ulTagKey++ )
					{
						if( *ulKey == *ulTagKey )
							strTitle += gmapLANG_CFG["quality_"+*ulTagKey];
					}

					if( ulKey + 1 != vecQuality.end( ) )
						strTitle += " &amp; ";
				}
			}
		}
		
		if( !vecEncode.empty( ) )
		{
			if( !m_vecEncodes.empty( ) )
			{
				if( !strTitle.empty( ) )
					strTitle += " - ";

				for( vector<string> :: iterator ulKey = vecEncode.begin( ); ulKey != vecEncode.end( ); ulKey++ )
				{
					for( vector< string > :: iterator ulTagKey = m_vecEncodes.begin( ); ulTagKey != m_vecEncodes.end( ); ulTagKey++ )
					{
						if( *ulKey == *ulTagKey )
							strTitle += gmapLANG_CFG["encode_"+*ulTagKey];
					}

					if( ulKey + 1 != vecEncode.end( ) )
						strTitle += " &amp; ";
				}
			}
		}
		
		if( !strTitle.empty( ) )
			strTitle = " - " + strTitle;

		strData += strTitle + "</title>\n";

		// link
		if( !rssdump.strURL.empty( ) )
			strData += "<link>" + rssdump.strURL + "</link>\n";
		else if( m_bServeLocal )
			strData += "<link>/</link>\n";

		// description
		strData += "<description>" + m_strDescription + "</description>\n";

		// category
		if( !strChannelTag.empty( ) )
			strData += "<category domain=\"" + rssdump.strURL + INDEX_HTML + "?tag=" + strChannelTag + "\" >" + strChannelTag + "</category>\n";
		else
			strData += "<category domain=\"" + rssdump.strURL + INDEX_HTML + "\">All Torrents</category>\n";

		// copyright
		if( !m_strDumpRSScopyright.empty( ) )
			strData += "<copyright>" + m_strDumpRSScopyright + "</copyright>\n";

		// managingEditor
		if( !m_strDumpRSSmanagingEditor.empty( ) )
			strData += "<managingEditor>" + m_strDumpRSSmanagingEditor + "</managingEditor>\n";

		// webMaster
		if( !m_strWebmaster.empty( ) )
			strData += "<webMaster>" + m_strWebmaster + "</webMaster>\n";

		// pubDate
		const string cstrPubDate( UTIL_PubDate( ) );

		if( !cstrPubDate.empty( ) )
			strData += "<pubDate>" + cstrPubDate + "</pubDate>\n";
		else
			UTIL_LogPrint( "saveRSS: missing required elements for RSS 2.0 channel pubDate\n" );

		// lastBuildDate
		const string cstrDate( UTIL_Date( ) );

		if( !cstrDate.empty( ) )
			strData += "<lastBuildDate>" + cstrDate + "</lastBuildDate>\n";
		else
			UTIL_LogPrint( "saveRSS: missing required elements for RSS 2.0 channel lastBuildDate\n" );

		// language
		if( !m_strLanguage.empty( ) )
			strData += "<language>" + m_strLanguage + "</language>\n";

		// generator
		if( !XBNBT_VER.empty( ) )
			strData += "<generator>" + XBNBT_VER + "</generator>\n";

		// docs
		strData += "<docs>http://blogs.law.harvard.edu/tech/rss</docs>\n";

		// ttl
		if( m_uiDumpRSSttl > 0 )
			strData += "<ttl>" + CAtomInt( m_uiDumpRSSttl ).toString( ) + "</ttl>\n";

		// image
		if( !m_strDumpRSSurl.empty( ) && !m_strTitle.empty( ) &&  !m_strDescription.empty( ) )
		{
			strData += "<image>\n";

			// url
			strData += "<url>" + m_strDumpRSSurl + "</url>\n";
			// title
			strData += "<title>" + m_strTitle + ( !strChannelTag.empty( ) ? " - " + strChannelTag : string( ) ) + "</title>\n";
			// description
			strData += "<description>" + m_strDescription + "</description>\n";

			// link
			if( !rssdump.strURL.empty( ) )
				strData += "<link>" + rssdump.strURL + "</link>\n";
			else if( m_bServeLocal )
				strData += "<link>/</link>\n";
			else
				UTIL_LogPrint( "saveRSS: missing required elements for RSS 2.0 channel image link\n" );

			// width
			if( m_uiDumpRSSwidth > 0 )
			{
				if( m_uiDumpRSSwidth > 144 )
				{
					m_uiDumpRSSwidth = 144;
					UTIL_LogPrint( "saveRSS: setting RSS 2.0 channel image width to max. 144\n" );
				}

				strData += "<width>" + CAtomInt( m_uiDumpRSSwidth ).toString( ) + "</width>\n";
			}

			// height
			if( m_uiDumpRSSheight > 0 )
			{
				if( m_uiDumpRSSheight > 400 )
				{
					m_uiDumpRSSheight = 400;
					UTIL_LogPrint( "saveRSS: setting RSS 2.0 channel image height to max. 400\n" );
				}

				strData += "<height>" + CAtomInt( m_uiDumpRSSheight ).toString( ) + "</height>\n";
			}

			strData += "</image>\n";
		}
		else
			UTIL_LogPrint( "saveRSS: missing required elements for RSS 2.0 channel image\n" );

		// rating 
		if( !m_strRating.empty() )
			strData += "<rating>" + m_strRating + "</rating>\n";

		// textInput
		if( m_bSearch )
		{
			if( !gmapLANG_CFG["search"].empty( ) && !gmapLANG_CFG["torrent_search"].empty( ) && !HTML_MakeURLFromFQDN( m_strTrackerFQDN, m_uiPort ).empty( ) )
			{
				strData += "<textInput>\n";

				// title
				strData += "<title>" + gmapLANG_CFG["search"] + "</title>\n";
				// description
				strData += "<description>" + gmapLANG_CFG["torrent_search"] + "</description>\n";
				// name
				strData += "<name>search</name>\n";
				// link
				strData += "<link>" + HTML_MakeURLFromFQDN( m_strTrackerFQDN, m_uiPort ) + INDEX_HTML + "</link>\n";

				strData += "</textInput>\n";
			}
			else
				UTIL_LogPrint( "saveRSS: missing required elements for RSS 2.0 channel textInput\n" );
		}

		//
		// Items
		//

		// retrieve torrent data

		unsigned long culKeySize = 0;
		
		struct torrent_t *pTorrents = 0;
		
		if( m_pCache )
		{
			pTorrents = m_pCache->getCache( );
			culKeySize = m_pCache->getSize( );
		}

		m_pCache->sort( SORT_DADDED, true );

		unsigned long ulLimit = 0;

		if( m_uiDumpRSSLimit == 0 || (unsigned long)m_uiDumpRSSLimit > culKeySize )
			ulLimit = culKeySize;
		else
			ulLimit = m_uiDumpRSSLimit;
		
		const string cstrLimit( pRequest->mapParams["limit"] );
		
		if( !cstrLimit.empty( ) )
			ulLimit = (unsigned long)atoi( cstrLimit.c_str( ) );

		string strTorrentLink = string( );
		unsigned long ulFileSize = 0;
		unsigned char ucCount = 0;
		unsigned long ulCount = 0;
		
		time_t now_t = time( 0 );

		unsigned char ucMatchMethod = MATCH_METHOD_NONCASE_AND;

		if( strMatch == "or" )
			ucMatchMethod = MATCH_METHOD_NONCASE_OR;
		else if( strMatch == "eor" )
			ucMatchMethod = MATCH_METHOD_NONCASE_EQ;

		for( unsigned long ulLoop = 0; ulLoop < culKeySize; ulLoop++ )
		{
			if( ulCount >= ulLimit )
				break;
			
			if( !vecSearch.empty( ) && !UTIL_MatchVector( pTorrents[ulLoop].strName, vecSearch, ucMatchMethod ) )
				continue;
			if( !vecUploader.empty( ) && !UTIL_MatchVector( pTorrents[ulLoop].strUploader, vecUploader, ucMatchMethod ) )
				continue;
			if( !vecIMDb.empty( ) && !UTIL_MatchVector( pTorrents[ulLoop].strIMDbID, vecIMDb, ucMatchMethod ) )
				continue;
				
			if( !vecFilter.empty( ) )  
			{    
				// only display entries that match the filter  
				bool bFoundKey = false;
				
				for( vector<string> :: iterator ulVecKey = vecFilter.begin( ); ulVecKey != vecFilter.end( ); ulVecKey++ )
				{
					if( (*ulVecKey).length( ) > 2 )
					{

						if( pTorrents[ulLoop].strTag == *ulVecKey )
						{
							bFoundKey = true;
							break;
						}
					}
					else
					{
						if( pTorrents[ulLoop].strTag.substr( 0, 2 - pTorrents[ulLoop].strTag.length( ) % 2 ) == *ulVecKey )
						{
							bFoundKey = true;
							break;
						}
					}
				}
				if( bFoundKey == false )
					continue;
			}
			
			if( !UTIL_MatchVector( pTorrents[ulLoop].strName, vecMedium, MATCH_METHOD_NONCASE_OR ) )
				continue;
			if( !UTIL_MatchVector( pTorrents[ulLoop].strName, vecQuality, MATCH_METHOD_NONCASE_OR ) )
				continue;
			if( !UTIL_MatchVector( pTorrents[ulLoop].strName, vecEncode, MATCH_METHOD_NONCASE_OR ) )
				continue;
			
//			if( !vecMedium.empty( ) )
//			{
//				// only count entries that match the search
//				
//				bool bFoundKey = false;
//				
//				for( vector<string> :: iterator ulVecKey = vecMedium.begin( ); ulVecKey != vecMedium.end( ); ulVecKey++ )
//				{
//					if( pTorrents[ulLoop].strLowerName.find( *ulVecKey ) != string :: npos )
//					{
//						bFoundKey = true;
//						break;
//					}
//				}
//				if( bFoundKey == false )
//					continue;
//			}
//			
//			if( !vecQuality.empty( ) )
//			{
//				// only count entries that match the search
//				
//				bool bFoundKey = false;
//				
//				for( vector<string> :: iterator ulVecKey = vecQuality.begin( ); ulVecKey != vecQuality.end( ); ulVecKey++ )
//				{
//					if( pTorrents[ulLoop].strLowerName.find( *ulVecKey ) != string :: npos )
//					{
//						bFoundKey = true;
//						break;
//					}
//				}
//				if( bFoundKey == false )
//					continue;
//			}
//			
//			if( !vecEncode.empty( ) )
//			{
//				// only count entries that match the search
//				
//				bool bFoundKey = false;
//				
//				for( vector<string> :: iterator ulVecKey = vecEncode.begin( ); ulVecKey != vecEncode.end( ); ulVecKey++ )
//				{
//					if( pTorrents[ulLoop].strLowerName.find( *ulVecKey ) != string :: npos )
//					{
//						bFoundKey = true;
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
//					if( UTIL_ToLower( pTorrents[ulLoop].strUploader ).find( UTIL_ToLower( *ulVecKey ) ) == string :: npos )
//					{
//						bFoundKey = false;
//						break;
//					}
//				}
//				if( bFoundKey == false )
//					continue;
//			}
			
			if( !cstrDay.empty( ) )
			{
				struct tm *now_tm, time_tm;
				int64 year, month, day, hour, minute, second;
				float passed = 0;
				sscanf( pTorrents[ulLoop].strAdded.c_str( ), "%d-%d-%d %d:%d:%d",&year,&month,&day,&hour,&minute,&second );
				time_tm.tm_year = year-1900;
				time_tm.tm_mon = month-1;
				time_tm.tm_mday = day;
				time_tm.tm_hour = hour;
				time_tm.tm_min = minute;
				time_tm.tm_sec = second;
				passed = difftime(now_t, mktime(&time_tm));
				if( passed > atoi( cstrDay.c_str( ) ) * 3600 * 24 )
					continue;
			}
			
			if( !cstrSection.empty( ) )
			{
				if( cstrSection == "hot" && pTorrents[ulLoop].uiSeeders + pTorrents[ulLoop].uiLeechers < CFG_GetInt( "bnbt_hot_count" ,20 ) )
					continue;
					
				if( cstrSection == "classic1" && pTorrents[ulLoop].ucClassic != 1 )
					continue;
				
				if( cstrSection == "classic2" && pTorrents[ulLoop].ucClassic != 2 )
					continue;
					
				if( cstrSection == "classic3" && pTorrents[ulLoop].ucClassic != 3 )
					continue;
				
				if( cstrSection == "req" && !pTorrents[ulLoop].bReq )
					continue;
				
				if( cstrSection == "free" )
				{
//					if( m_bFreeGlobal )
//					{
//						if( m_iFreeDownGlobal < pTorrents[ulLoop].iDefaultDown )
//							pTorrents[ulLoop].iDefaultDown = m_iFreeDownGlobal;
//						if( m_iFreeUpGlobal > pTorrents[ulLoop].iDefaultUp )
//							pTorrents[ulLoop].iDefaultUp = m_iFreeUpGlobal;
//					}
//					
					pTorrents[ulLoop].iFreeDown = pTorrents[ulLoop].iDefaultDown;
					pTorrents[ulLoop].iFreeUp = pTorrents[ulLoop].iDefaultUp;

					if( pTorrents[ulLoop].iFreeTo > 0 && pTorrents[ulLoop].iFreeTo > now_t )
					{
						if( pTorrents[ulLoop].iTimeDown < pTorrents[ulLoop].iFreeDown )
							pTorrents[ulLoop].iFreeDown = pTorrents[ulLoop].iTimeDown;
						if( pTorrents[ulLoop].iTimeUp > pTorrents[ulLoop].iFreeUp )
							pTorrents[ulLoop].iFreeUp = pTorrents[ulLoop].iTimeUp;
					}

					if( pTorrents[ulLoop].iFreeDown != 0 )
						continue;
				}
			}

			if( !cstrMode.empty( ) )
			{
				// only display entries that match the mode
				if( cstrMode == "Seeded" )
				{
					if( !pTorrents[ulLoop].uiSeeders )
						continue;
				}
				else if( cstrMode == "Unseeded"  )
				{
					if( pTorrents[ulLoop].uiSeeders )
						continue;
				}
// 				else if(( cstrMode == "MyTorrents"  ) && ( pRequest->user.ucAccess & ACCESS_UPLOAD ) )
				else if( cstrMode == "MyTorrents" )
				{
					if( pTorrents[ulLoop].strUploaderID != strUID )
						continue;
				}
				else if( cstrMode == "MyBookmarks" )
				{
					if( !UTIL_MatchVector( pTorrents[ulLoop].strID, vecBookmark, MATCH_METHOD_NONCASE_EQ ) )
						continue;
				}
				else if( cstrMode == "MyFriends" )
				{
					if( !UTIL_MatchVector( pTorrents[ulLoop].strUploaderID, vecFriend, MATCH_METHOD_NONCASE_EQ ) )
						continue;
				}
			}
		
			ulCount++;

			// item
			strData += "<item>\n";

			// title

			if( !pTorrents[ulLoop].strName.empty( ) )
				strData += "<title>" + UTIL_RemoveHTML( pTorrents[ulLoop].strName ) + "</title>\n";
			else
				UTIL_LogPrint( "saveRSS: missing required elements for RSS 2.0 item title\n" );

			// link
			if( m_bShowStats )
			{
				if( ( !rssdump.strURL.empty( ) || m_bServeLocal ) && !pTorrents[ulLoop].strID.empty( ) )
					strData += "<link>" + rssdump.strURL + STATS_HTML + "?id=" + pTorrents[ulLoop].strID + "</link>\n";
				else
					UTIL_LogPrint( "saveRSS: missing required elements for RSS 2.0 item link\n" );
			}
			
			
			// description
			strData += "<description><![CDATA[\n";

			// tag
			if( !pTorrents[ulLoop].strTag.empty( ) )
			{
				if( !m_vecTags.empty( ) )
				{
					string strNameIndex = string( );
					string strTag = string( );

					for( vector< pair< string, string > > :: iterator it2 = m_vecTags.begin( ); it2 != m_vecTags.end( ); it2++ )
					{
						strNameIndex = (*it2).first;
						strTag = (*it2).second;

						if( strNameIndex == pTorrents[ulLoop].strTag )
						{
							strData += gmapLANG_CFG["tag"] + ": " + strTag + "<br>\n";
							break;
						}

					}
				}
			}
				
			// info hash
//			strData += gmapLANG_CFG["info_hash"] + ": " + pTorrents[ulLoop].strInfoHash + "<br>\n";

			// size
			strData += gmapLANG_CFG["size"] + ": " + UTIL_BytesToString( pTorrents[ulLoop].iSize ) + "<br>\n";

			// files
			strData += gmapLANG_CFG["files"] + ": " + CAtomInt( pTorrents[ulLoop].uiFiles ).toString( ) + "<br>\n";

			strData += "]]></description>\n";

			// author
			if( m_bShowUploader )
			{
				if( !pTorrents[ulLoop].strUploader.empty( ) )
					strData += "<author>" + UTIL_RemoveHTML( pTorrents[ulLoop].strUploader ) + "</author>\n";
				else
					strData += "<author>(Unknown)</author>\n";
				//UTIL_LogPrint( "saveRSS: missing required elements for RSS 2.0 item author\n" );
			}
			else
				strData += "<author>(Unknown)</author>\n";

			// category
			if( ( !rssdump.strURL.empty( ) || m_bServeLocal ) && !pTorrents[ulLoop].strTag.empty( ) )
			{
				if( !m_vecTags.empty( ) )
				{
					string strNameIndex = string( );
					string strTag = string( );

					for( vector< pair< string, string > > :: iterator it2 = m_vecTags.begin( ); it2 != m_vecTags.end( ); it2++ )
					{
						strNameIndex = (*it2).first;
						strTag = (*it2).second;

						if( strNameIndex == pTorrents[ulLoop].strTag )
						{
							strData += "<category domain=\"" + rssdump.strURL + INDEX_HTML + "?tag=" + UTIL_StringToEscaped( pTorrents[ulLoop].strTag ) + "\">" + strTag + "</category>\n";
							break;
						}

					}
				}
			}
			else if( m_bServeLocal )
				strData += "<category domain=\"/\">Unknown</category>\n";
			else
				UTIL_LogPrint( "saveRSS: missing required elements for RSS 2.0 item category\n" );

			// comments
			if( m_bAllowComments )
			{
				if( ( !rssdump.strURL.empty( ) || m_bServeLocal ) && !pTorrents[ulLoop].strID.empty( ) )
					strData += "<comments>" + rssdump.strURL + COMMENTS_HTML + "?id=" + pTorrents[ulLoop].strID + "</comments>\n";
				else
					UTIL_LogPrint( "saveRSS: missing required elements for RSS 2.0 item comments\n" );
			}

			// enclosure
			if( m_bAllowTorrentDownloads && pTorrents[ulLoop].bAllow && !pTorrents[ulLoop].bPost )
			{
				strTorrentLink = string( );

				if( ( !rssdump.strURL.empty( ) || m_bServeLocal ) && !pTorrents[ulLoop].strID.empty( ) )
				{
					strTorrentLink = rssdump.strURL + STR_TORRENTS + CHAR_FS + pTorrents[ulLoop].strID + ".torrent";
					if( pRequest->user.strUID.empty( ) && !cstrPasskey.empty( ) )
						strTorrentLink += "?passkey=" + cstrPasskey;
				}

				ulFileSize = UTIL_SizeFile( string(m_strAllowedDir + pTorrents[ulLoop].strFileName ).c_str( ) );

				if( !strTorrentLink.empty( ) && !m_strAllowedDir.empty( ) && !pTorrents[ulLoop].strFileName.empty( ) && ulFileSize > 0 && !gmapMime[".torrent"].empty( ) )
					strData += "<enclosure url=\"" + strTorrentLink + "\" length=\"" + CAtomLong( ulFileSize ).toString( ) + "\" type=\"" + gmapMime[".torrent"] + "\" />\n";
				else
					UTIL_LogPrint( "saveRSS: missing required elements for RSS 2.0 item enclosure (%s)(%s)\n", pTorrents[ulLoop].strFileName.c_str( ), CAtomLong( ulFileSize ).toString( ).c_str( ) );
			}

			// guid
			if( !pTorrents[ulLoop].strID.empty( ) )
				strData += "<guid isPermaLink=\"false\">" + pTorrents[ulLoop].strID + "</guid>\n";
			else
				UTIL_LogPrint( "saveRSS: missing required elements for RSS 2.0 item guid\n" );

			// pubDate
			if( m_bShowAdded )
			{
				if( !pTorrents[ulLoop].strAdded.empty( ) )
					strData += "<pubDate>" + UTIL_AddedToDate( pTorrents[ulLoop].strAdded ) + "</pubDate>\n";
				else
					UTIL_LogPrint( "saveRSS: missing required elements for RSS 2.0 item pubDate\n" );
			}

			// source
			if( ( !rssdump.strURL.empty( ) || m_bServeLocal ) && !rssdump.strName.empty( ) )
				strData += "<source url=\"" + rssdump.strURL + rssdump.strName + "\">" + m_strTitle + ( !strChannelTag.empty( ) ? " - " + strChannelTag : string( ) ) + "</source>\n";
			else
				UTIL_LogPrint( "saveRSS: missing required elements for RSS 2.0 item source\n" );

			// torrentItem

			// infohash
			if( !gmapLANG_CFG["info_hash"].empty( ) ) 
				strData += "<torrentItem:infohash title=\"" + gmapLANG_CFG["info_hash"] + "\" />\n";
			else
				UTIL_LogPrint( "saveRSS: missing required elements for torrentItem:infohash\n" );

			// comments
			if( m_bAllowComments )
			{
				if( !gmapLANG_CFG["comments"].empty( ) ) 
					strData += "<torrentItem:comments title=\"" + gmapLANG_CFG["comments"] + "\" />\n";
				else
					UTIL_LogPrint( "saveRSS: missing required elements for torrentItem:comments\n" );
			}

			// infostat
			if( m_bShowStats )
			{
				if( !gmapLANG_CFG["stats_file_info"].empty( ) ) 
					strData += "<torrentItem:infostat title=\"" + gmapLANG_CFG["stats_file_info"] + "\" />\n";
				else
					UTIL_LogPrint( "saveRSS: missing required elements for torrentItem:infostat\n" );
			}

			// download
			if( m_bAllowTorrentDownloads )
			{
				if( !gmapLANG_CFG["download"].empty( ) ) 
					strData += "<torrentItem:download title=\"" + gmapLANG_CFG["download"] + "\" />\n";
				else
					UTIL_LogPrint( "saveRSS: missing required elements for torrentItem:download\n" );

			}

			// size
			if( m_bShowSize )
			{
				if( !CAtomLong( pTorrents[ulLoop].iSize ).toString( ).empty( ) && !gmapLANG_CFG["size"].empty( ) ) 
					strData += "<torrentItem:size title=\"" + gmapLANG_CFG["size"] + "\">" + UTIL_BytesToString( pTorrents[ulLoop].iSize ) + "</torrentItem:size>\n";
				else
					UTIL_LogPrint( "saveRSS: missing required elements for torrentItem:size\n" );
			}

			// files
			if( m_bShowNumFiles )
			{
				if( !CAtomInt( pTorrents[ulLoop].uiFiles ).toString( ).empty( ) && !gmapLANG_CFG["files"].empty( ) ) 
					strData += "<torrentItem:files title=\"" + gmapLANG_CFG["files"] + "\">" + CAtomInt( pTorrents[ulLoop].uiFiles ).toString( ) + "</torrentItem:files>\n";
				else
					UTIL_LogPrint( "saveRSS: missing required elements for torrentItem:files\n" );
			}

			// seeders
			if( !CAtomInt( pTorrents[ulLoop].uiSeeders ).toString( ).empty( ) && !gmapLANG_CFG["stats_seeders"].empty( ) ) 
				strData += "<torrentItem:seeders title=\"" + gmapLANG_CFG["stats_seeders"] + "\">" + CAtomInt( pTorrents[ulLoop].uiSeeders ).toString( ) + "</torrentItem:seeders>\n";
			else
				UTIL_LogPrint( "saveRSS: missing required elements for torrentItem:seeders\n" );

			// leechers
			if( !CAtomInt( pTorrents[ulLoop].uiLeechers ).toString( ).empty( ) && !gmapLANG_CFG["stats_leechers"].empty( ) ) 
				strData += "<torrentItem:leechers title=\"" + gmapLANG_CFG["stats_leechers"] + "\">" + CAtomInt( pTorrents[ulLoop].uiLeechers ).toString( ) + "</torrentItem:leechers>\n";
			else
				UTIL_LogPrint( "saveRSS: missing required elements for torrentItem:leechers\n" );

			// completed
			if( m_bShowCompleted )
			{
				if( !CAtomLong( pTorrents[ulLoop].ulCompleted ).toString( ).empty( ) && !gmapLANG_CFG["completed"].empty( ) ) 
					strData += "<torrentItem:completed title=\"" + gmapLANG_CFG["completed"] + "\">" + CAtomLong( pTorrents[ulLoop].ulCompleted ).toString( ) + "</torrentItem:completed>\n";
				else
					UTIL_LogPrint( "saveRSS: missing required elements for torrentItem:completed\n" );
			}

			strData += "</item>\n";
		}

		// free the memory
//		delete [] pTorrents;

		strData += "</channel>\n";

		strData += "</rss>\n";
		
		pResponse->strContent = strData;
	}
	else
		UTIL_LogPrint( "saveRSS: missing required elements for RSS 2.0\n" );
}

void CTracker :: serverResponseRSSXSL( struct request_t *pRequest, struct response_t *pResponse )
{
	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
	{
		// Set the start time
		const struct bnbttv btv( UTIL_CurrentTime( ) );

		if( IsIPBanned( pRequest, pResponse, btv, "IP Banned", string( CSS_INDEX ), NOT_INDEX ) )
			return;
	}

	pResponse->strCode = "200 " + gmapLANG_CFG["server_response_200"];

	pResponse->mapHeaders.insert( pair<string, string>( "Content-Type", gmapMime[".xsl"] ) );

	pResponse->strContent = rssxsl.strFile;
}

// Save the XML file
// void CTracker :: saveXML( )
// {
// 	string strData = string( );
// 	string tmpData = string( );
// 	unsigned int uiDL = 0;
// 	unsigned int uiComplete = 0;
// 	unsigned long ulCompleted = 0;
// 
// 	//addition by labarks
// 	strData += "<?xml version=\"1.0\" encoding=\"" + gstrCharSet + "\"?>\n";
// 
// 	strData += "<torrents>\n";
// 
// 		if( m_pDFile )
// 		{
// 			map<string, CAtom *> *pmapDicti = m_pDFile->getValuePtr( );
// 			map<string, CAtom *> *pmapPeersDicti;
// 
// 			CAtom *pCompleted = 0;
// 			CAtom *pIP = 0;
// 			CAtom *pUpped = 0;
// 			CAtom *pDowned = 0;
// 			CAtom *pLef = 0;
// 			CAtom *pConn = 0;
// 			CAtomDicti *pPeerDicti = 0;
// 
// 			for( map<string, CAtom *> :: iterator it = pmapDicti->begin( ); it != pmapDicti->end( ); it++ )
// 			{
// 				strData += "<torrent hash=\"";
// 				strData += UTIL_HashToString( (*it).first );
// 				strData += "\"";
// 				
// 				CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bid,bfilename,bname,badded,bsize,bfiles,btag,btitle,buploader FROM allowed WHERE bhash=\'" + UTIL_StringToMySQL( (*it).first ) + "\'" );
// 						
// 				vector<string> vecQuery;
// 				
// 				vecQuery.reserve(9);
// 
// 				vecQuery = pQuery->nextRow( );
// 				
// 				delete pQuery;
// 				
// 				if( vecQuery.size( ) == 9 )
// 				{
// 					if( !vecQuery[1].empty( ) )
// 						strData += " filename=\"" + UTIL_RemoveHTML( vecQuery[0] ) + "\"";
// 
// 					if( !vecQuery[2].empty( ) )
// 						strData += " name=\"" + UTIL_RemoveHTML( vecQuery[1] ) + "\"";
// 
// 					if( !vecQuery[3].empty( ) )
// 						strData += " added=\"" + vecQuery[3] + "\"";
// 
// 					if( !vecQuery[4].empty( ) )
// 						strData += " size=\"" + vecQuery[4] + "\"";
// 
// 					if( !vecQuery[5].empty( ) )
// 						strData += " files=\"" + vecQuery[5] + "\"";
// 
// 					ulCompleted = 0;
// 
// 					if( m_pCompleted )
// 					{
// 						pCompleted = m_pCompleted->getItem( (*it).first );
// 
// 						if( pCompleted && pCompleted->isLong( ) )
// 							ulCompleted = (unsigned int)( (CAtomLong *)pCompleted )->getValue( );
// 					}
// 					strData += " completed=\"" + CAtomInt( ulCompleted ).toString( ) + "\"";
// 				
// 					if( !vecQuery[6].empty( ) )
// 						strData += " tag=\"" + UTIL_RemoveHTML( vecQuery[6] ) + "\"";
// 
// 					if( !vecQuery[7].empty( ) )
// 						strData += " uploadname=\"" + UTIL_RemoveHTML( vecQuery[7] ) + "\"";
// 
// 					if( m_bShowUploader )
// 					{
// 						if( !vecQuery[8].empty( ) )
// 							strData += " uploader=\"" + UTIL_RemoveHTML( vecQuery[8] ) + "\"";
// 					}
// 				}
// 
// 				tmpData = "";
// 				uiDL = 0;
// 				uiComplete = 0;
// 
// 				if( (*it).second->isDicti( ) )
// 				{
// 					pmapPeersDicti = ( (CAtomDicti *)(*it).second )->getValuePtr( );
// 
// 					if( m_bDumpXMLPeers )
// 					{
// 						if( !pmapPeersDicti->empty( ) )
// 						{
// 							tmpData += "<peers>\n";
// 
// 							for( map<string, CAtom *> :: iterator it2 = pmapPeersDicti->begin( ); it2 != pmapPeersDicti->end( ); it2++ )
// 							{

// 								if( (*it2).second->isDicti( ) )
// 								{
// 									pPeerDicti = (CAtomDicti *)(*it2).second;
// 
// 									pIP = pPeerDicti->getItem( "ip" );
// 									pUpped = pPeerDicti->getItem( "uploaded" );
// 									pDowned = pPeerDicti->getItem( "downloaded" );
// 									pLef = pPeerDicti->getItem( "left" );
// 									pConn = pPeerDicti->getItem( "connected" );
// 
// 									if( ( (CAtomLong *)pLef )->getValue( ) == 0 )
// 										uiComplete++;
// 									else
// 										uiDL++;
// 
// 									tmpData += "<peer id=\"";
// 									tmpData += UTIL_HashToString( (*it2).first );
// 									tmpData += "\"";
// 
// 									if( pIP )
// 										tmpData += " ip=\"" + pIP->toString( ) + "\"";
// 
// 									if( pUpped )
// 										tmpData += " uploaded=\"" + pUpped->toString( ) + "\"";
// 
// 									if( pDowned )
// 										tmpData += " downloaded=\"" + pDowned->toString( ) + "\"";
// 
// 									if( pLef && pLef->isLong( ) )
// 										tmpData += " left=\"" + pLef->toString( ) + "\"";
// 
// 									if( pConn && pConn->isLong( ) )
// 										tmpData += " connected=\"" + CAtomLong( GetTime( ) - (unsigned long)( (CAtomLong *)pConn )->getValue( ) ).toString( ) + "\"";
// 
// 									tmpData += " />\n";
// 								}
// 							}
// 
// 							tmpData += "</peers>\n";
// 						}
// 					}
// 				}
// 
// 				strData += " leecher=\""+ CAtomInt( uiDL ).toString( ) + "\"";
// 				strData += " seeder=\""+ CAtomInt( uiComplete ).toString( ) + "\"";
// 				strData += ">\n";
// 
// 				if( m_bDumpXMLPeers )
// 					strData += tmpData;
// 
// 				strData += "</torrent>\n";
// 			}
// 		}
// 
// 	strData += "</torrents>\n";
// 
// 	// write the file
// 	if( !xmldump.strName.empty( ) )
// 	{
// 		string strXMLFile = string( );
// 
// 		if( !xmldump.strDir.empty( ) )
// 			strXMLFile = xmldump.strDir + PATH_SEP + xmldump.strName;
// 		else
// 			strXMLFile = xmldump.strName;
// 
// 		FILE *pFile = FILE_ERROR;
// 
// 		pFile = fopen( strXMLFile.c_str( ), "wb" );
// 
// 		if( pFile == FILE_ERROR )
// 		{
// 			UTIL_LogPrint( string( gmapLANG_CFG["unable_to_write_file"] + "\n" ).c_str( ), strXMLFile.c_str( ) );
// 
// 			if( gbDebug )
// 				if( gucDebugLevel & DEBUG_TRACKER )
// 					UTIL_LogPrint( "saveXML: completed\n" );
// 
// 			return;
// 		}
// 
// 		fwrite( strData.c_str( ), sizeof( char ), strData.size( ), pFile );
// 		fclose( pFile );
// 	}
// }
