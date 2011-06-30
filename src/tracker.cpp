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
#include "math.h"

#define Pi 3.1415926

#if defined ( XBNBT_GD )
 #include <gd.h>
#endif

// XBNBT XStats
struct xbnbtstats_t gtXStats;

// Mime types
map<string, string> gmapMime;

//
// CTracker
//

// The constructor
CTracker :: CTracker( )
{
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "CTracker: Constructor called\n" );

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "CTracker: Initialise MUTEX announce queue\n" );

	m_mtxQueued.Initialize( );

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "CTracker: Initialise BNBT variables\n" );

	m_strAllowedDir = CFG_GetString( "allowed_dir", string( ) );
	m_strOfferDir = CFG_GetString( "offer_dir", string( ) );
	m_strRulesDir = CFG_GetString( "bnbt_rules_dir", string( ) );
	m_strFAQDir = CFG_GetString( "bnbt_faq_dir", string( ) );
	m_strArchiveDir = CFG_GetString( "bnbt_archive_dir", string( ) );
	m_strFileDir = CFG_GetString( "bnbt_file_dir", string( ) );

	if( !m_strAllowedDir.empty( ) && m_strAllowedDir[m_strAllowedDir.size( ) - 1] != PATH_SEP )
		m_strAllowedDir += PATH_SEP;
	
	if( !m_strOfferDir.empty( ) && m_strOfferDir[m_strOfferDir.size( ) - 1] != PATH_SEP )
		m_strOfferDir += PATH_SEP;

	if( !m_strRulesDir.empty( ) && m_strRulesDir[m_strRulesDir.size( ) - 1] != PATH_SEP )
		m_strRulesDir += PATH_SEP;
	
	if( !m_strFAQDir.empty( ) && m_strFAQDir[m_strFAQDir.size( ) - 1] != PATH_SEP )
		m_strFAQDir += PATH_SEP;

	if( !m_strArchiveDir.empty( ) && m_strArchiveDir[m_strArchiveDir.size( ) - 1] != PATH_SEP )
		m_strArchiveDir += PATH_SEP;

	if( !m_strFileDir.empty( ) && m_strFileDir[m_strFileDir.size( ) - 1] != PATH_SEP )
		m_strFileDir += PATH_SEP;

	m_strStaticHeaderFile = CFG_GetString( "bnbt_static_header", string( ) );
	m_strStaticFooterFile = CFG_GetString( "bnbt_static_footer", string( ) );

	// RSS (thanks labarks)
	m_ucDumpRSSFileMode = (unsigned char)CFG_GetInt( "bnbt_rss_file_mode", 0 );
	m_uiDumpRSSttl = CFG_GetInt( "bnbt_rss_channel_ttl", 60 );
	m_strDumpRSSurl = CFG_GetString( "bnbt_rss_channel_image_url", string( ) );
	m_uiDumpRSSwidth = CFG_GetInt( "bnbt_rss_channel_image_width", 0 );
	m_uiDumpRSSheight = CFG_GetInt( "bnbt_rss_channel_image_height", 0 );
	m_strDumpRSScopyright = CFG_GetString( "bnbt_rss_channel_copyright", string( ) );
	m_uiDumpRSSLimit = CFG_GetInt( "bnbt_rss_limit", 25 );
	m_strDumpRSSmanagingEditor = CFG_GetString( "bnbt_rss_channel_managingeditor", string( ) );

	m_strForceAnnounceURL = CFG_GetString( "bnbt_force_announce_url", string( ) );
	m_bForceAnnounceOnDL = CFG_GetInt( "bnbt_force_announce_on_download", 0 ) == 0 ? false : true;
	m_uiParseAllowedInterval = CFG_GetInt( "parse_allowed_interval", 0 );
	m_uiGetSeedBonusInterval = CFG_GetInt( "bnbt_get_seed_bonus_interval", 300 );
	m_uiDownloaderTimeOutInterval = CFG_GetInt( "downloader_timeout_interval", 2700 );
	m_uiRefreshConfigInterval = CFG_GetInt( "bnbt_refresh_config_interval", 60 );
	m_uiRefreshCBTTListInterval = CFG_GetInt( "bnbt_refresh_cbtt_list_interval", 60 );
	m_uiRefreshIMDbInterval = CFG_GetInt( "bnbt_refresh_imdb_interval", 3 );
	m_uiRefreshStaticInterval = CFG_GetInt( "bnbt_refresh_static_interval", 10 );
	m_uiTimerInterval = CFG_GetInt( "bnbt_timer_interval", 3600 );
	m_uiRefreshFastCacheInterval = CFG_GetInt( "bnbt_refresh_fast_cache_interval", 30 );
	m_ulGetSeedBonusNext = GetTime( ) + m_uiGetSeedBonusInterval;
	m_ulDownloaderTimeOutNext = GetTime( );
// 	m_ulDownloaderTimeOutNext = GetTime( ) + m_uiDownloaderTimeOutInterval;
	m_ulRefreshConfigNext = GetTime( ) + m_uiRefreshConfigInterval * 60;
	m_ulRefreshCBTTListNext = GetTime( ) + m_uiRefreshCBTTListInterval * 60;
	m_ulRefreshIMDbNext = GetTime( ) + m_uiRefreshIMDbInterval * 60 * 60;
	m_ulRefreshStaticNext = GetTime( );  
	m_ulTimerNext = GetTime( );
	m_ulRefreshFastCacheNext = GetTime( );
	m_uiAnnounceInterval = CFG_GetInt( "announce_interval", 1800 );
	m_uiMinRequestInterval = CFG_GetInt( "min_request_interval", 18000 );
	m_uiResponseSize = CFG_GetInt( "response_size", 50 );
	m_uiMaxGive = CFG_GetInt( "max_give", 200 );
	m_bRatioRestrict = CFG_GetInt( "bnbt_ratio_restrict", 1 ) == 0 ? false : true;
	m_bAllowScrape = CFG_GetInt( "bnbt_allow_scrape", 1 ) == 0 ? false : true;
	m_bCountUniquePeers = CFG_GetInt( "bnbt_count_unique_peers", 1 ) == 0 ? false : true;
	m_bDeleteInvalid = CFG_GetInt( "bnbt_delete_invalid", 0 ) == 0 ? false : true;
	m_bParseOnStart = CFG_GetInt( "bnbt_parse_on_start", 1 ) == 0 ? false : true;
	m_bParseOnUpload = CFG_GetInt( "bnbt_parse_on_upload", 1 ) == 0 ? false : true;
	m_uiMaxTorrents = CFG_GetInt( "bnbt_max_torrents", 0 );
	m_bShowStats = CFG_GetInt( "bnbt_show_stats", 1 ) == 0 ? false : true;
	m_bAllowTorrentDownloads = CFG_GetInt( "bnbt_allow_torrent_downloads", 1 ) == 0 ? false : true;
	m_bAllowComments = CFG_GetInt( "bnbt_allow_comments", 0 ) == 0 ? false : true;
	m_bShowAdded = CFG_GetInt( "bnbt_show_added", 1 ) == 0 ? false : true;
	m_bShowAdded_Index = CFG_GetInt( "bnbt_show_added_index", 1 ) == 0 ? false : true;
	m_bShowSize = CFG_GetInt( "bnbt_show_size", 1 ) == 0 ? false : true;
	m_bShowNumFiles = CFG_GetInt( "bnbt_show_num_files", 1 ) == 0 ? false : true;
	m_bShowCompleted = CFG_GetInt( "bnbt_show_completed", 0 ) == 0 ? false : true;
	m_bShowTransferred = CFG_GetInt( "bnbt_show_transferred", 0 ) == 0 ? false : true;
	m_bShowMinLeft = CFG_GetInt( "bnbt_show_min_left", 0 ) == 0 ? false : true;
	m_bShowAverageLeft = CFG_GetInt( "bnbt_show_average_left", 0 ) == 0 ? false : true;
	m_bShowMaxiLeft = CFG_GetInt( "bnbt_show_max_left", 0 ) == 0 ? false : true;
	m_bShowLeftAsProgress = CFG_GetInt( "bnbt_show_left_as_progress", 1 ) == 0 ? false : true;
	m_bShowUploader = CFG_GetInt( "bnbt_show_uploader", 0 ) == 0 ? false : true;
	m_bSearch = CFG_GetInt( "bnbt_allow_search", 1 ) == 0 ? false : true;
	m_bSort = CFG_GetInt( "bnbt_allow_sort", 1 ) == 0 ? false : true;
	m_bShowFileComment = CFG_GetInt( "bnbt_show_file_comment", 1 ) == 0 ? false : true;
	m_bShowFileContents = CFG_GetInt( "bnbt_show_file_contents", 0 ) == 0 ? false : true;
	m_bShowShareRatios = CFG_GetInt( "bnbt_show_share_ratios", 1 ) == 0 ? false : true;
	m_bShowAvgDLRate = CFG_GetInt( "bnbt_show_average_dl_rate", 0 ) == 0 ? false : true;
	m_bShowAvgULRate = CFG_GetInt( "bnbt_show_average_ul_rate", 0 ) == 0 ? false : true;
	m_bDeleteOwnTorrents = CFG_GetInt( "bnbt_delete_own_torrents", 1 ) == 0 ? false : true;
	m_bGen = CFG_GetInt( "bnbt_show_gen_time", 1 ) == 0 ? false : true;
	m_uiPerPage = CFG_GetInt( "bnbt_per_page", 20 );
	m_uiPerPageMax = CFG_GetInt( "bnbt_per_page_max", 200 );
	m_uiUsersPerPage = CFG_GetInt( "bnbt_users_per_page", 50 );
	m_uiMaxPeersDisplay = CFG_GetInt( "bnbt_max_peers_display", 500 );
	m_ucGuestAccess = (unsigned char)CFG_GetInt( "bnbt_guest_access", ACCESS_VIEW );
	m_ucMemberAccess =(unsigned char)CFG_GetInt( "bnbt_member_access", ACCESS_VIEW | ACCESS_DL | ACCESS_COMMENTS );
	m_uiFileExpires = CFG_GetInt( "bnbt_file_expires", 1440 );
	m_uiTorrentExpires = CFG_GetInt( "bnbt_torrent_expires", 180 );
	m_uiNameLength = CFG_GetInt( "bnbt_name_length", 16 );
	m_uiCommentLength = CFG_GetInt( "bnbt_comment_length", 800 );
	m_uiMessageLength = CFG_GetInt( "bnbt_message_length", 800 );
	m_uiTalkLength = CFG_GetInt( "bnbt_talk_length", 800 );
//	m_pCached = new CAtomDicti( );
	
	m_pCache = new CCache( );
	
	loadAccess( );
	
	// Initialise XBNBT CBTT feature variables

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "CTracker: Initialise CBTT feature variables\n" );

	// DWMod Ban Code
	m_ucBanMode = (unsigned char)CFG_GetInt( "cbtt_ban_mode", 0 );
	m_bPeerSpoofRestrict = CFG_GetInt( "cbtt_restricted_peer_spoofing", 0 ) == 0 ? false : true;
	m_strClientBanFile = CFG_GetString( "cbtt_ban_file", "clientbans.bnbt" );
	// CBTT IP banning Mode
	m_ucIPBanMode = (unsigned char)CFG_GetInt( "cbtt_ip_ban_mode", 0 );
	m_strIPBanFile = CFG_GetString( "cbtt_ipban_file", "bans.bnbt" );
	// misc CBTT mod functions
	m_bDontCompressTorrent = CFG_GetInt( "cbtt_dont_compress_torrents", 1 ) == 0 ? false : true;
	m_bRestrictOverflow = CFG_GetInt( "cbtt_restrict_overflow", 0 ) == 0 ? false : true;
	m_strOverFlowLimit = CFG_GetString( "cbtt_restrict_overflow_limit", "1099511627776" );
	m_bBlockNATedIP = CFG_GetInt( "cbtt_block_private_ip", 0 ) == 0 ? false : true;
	m_bBlacklistP2PPorts = CFG_GetInt( "cbtt_blacklist_common_p2p_ports",0 ) == 0 ? false : true;
	m_ucBlockSearchRobots = (unsigned char)CFG_GetInt( "cbtt_block_search_robots", 0 );
	// adopted official tracker config value
	m_bLocalOnly = CFG_GetInt( "only_local_override_ip", 0 ) == 0 ? false : true;  
	// Private Flag for announce replies
	m_ucPrivateTracker = (unsigned char)CFG_GetInt( "bnbt_private_tracker_flag", 0 );

	// Initialise XBNBT General variables
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "CTracker: Initialise XBNBT General variables\n" );

	m_strRSSValidImage = CFG_GetString( "xbnbt_rss_valid_image", string( ) );
	m_bUsersOnline = CFG_GetInt( "xbnbt_users_online", 0 ) == 0 ? false : true;
	m_ucNavbar = (unsigned char)CFG_GetInt( "bnbt_show_navbar", 3 );
	m_ucShowPeerInfo = (unsigned char)CFG_GetInt( "bnbt_show_peer_info", 0 );
	m_ucShowValidator = (unsigned char)CFG_GetInt( "bnbt_show_validator", 0 );
	m_uiPort = CFG_GetInt( "port", 6969 );
	m_strTrackerFQDN = CFG_GetString( "bnbt_tracker_fqdn", string( ) );
	m_strTrackerURL = HTML_MakeURLFromFQDN( m_strTrackerFQDN, m_uiPort );
	m_strDescription = CFG_GetString( "bnbt_tracker_description", string( ) );
	m_strTitle = CFG_GetString( "bnbt_tracker_title", string( ) );
	m_strSubTitle = CFG_GetString( "bnbt_tracker_subtitle", string( ) );
	m_strKeywords = CFG_GetString( "bnbt_tracker_keywords", string( ) );
	m_strLanguage = CFG_GetString( "bnbt_language", "en" );
	m_strRating = CFG_GetString( "bnbt_rating", string( ) );
	m_strWebmaster = CFG_GetString( "bnbt_webmaster", string( ) );
	// Announce 'key' support
	m_bAnnounceKeySupport = CFG_GetInt( "bnbt_use_announce_key", 1 ) == 0 ? false : true;

	// Announce & Scrape & Info Authentication Support
	m_ucAuthAnnounceAccess = (unsigned char)CFG_GetInt( "bnbt_announce_access_required", 0 );
	m_ucAuthScrapeAccess = (unsigned char)CFG_GetInt( "bnbt_scrape_access_required", 0 );
	m_ucInfoAccess = (unsigned char)CFG_GetInt( "bnbt_info_access_required", 0 );
	// Custom Announce and Scrape
	m_strCustomAnnounce = CFG_GetString( "bnbt_custom_announce", "/announce.php" );
	m_strCustomScrape = CFG_GetString( "bnbt_custom_scrape", "/scrape.php" );
	// flags
	m_bFlagRSSAlert = false;
	m_bFlagXMLAlert = false;
	m_bFlagNotOwnLinkAlert = false;
	m_bFlagNotOwnHUBLinkAlert = false;
	// Show the header and footer on all pages
	m_bStaticAll = CFG_GetInt( "bnbt_show_header_footer", 0 ) == 0 ? false : true;
	// Show the uploader/modifier IP - needs work
	m_bShowIP = CFG_GetInt( "bnbt_show_uploader_ip", 0 ) == 0 ? false : true;
	// totals
	m_ulSeedersTotal = 0;
	m_ulLeechersTotal = 0;
	m_ulPeers = 0;
	// Announce-list
	m_bEnableAnnounceList = CFG_GetInt( "bnbt_announce_list_enable", 0 ) == 0 ? false : true;
	// force the announce URL on the upload of the .torrent
	m_bForceAnnounceOnUL = CFG_GetInt( "bnbt_force_announce_on_upload", 0 ) == 0 ? false : true;

	// Initialise XBNBT file server variables
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "CTracker: Initialise XBNBT file server variables\n" );

	// Serve Local
	m_bServeLocal = CFG_GetInt( "bnbt_allow_serve_local", 1 ) == 0 ? false : true;
	// robots.txt
	robots.strName = CFG_GetString( "bnbt_robots_txt", string( ) ); 
	robots.strExt = getFileExt( robots.strName );
	// favicon.ico
	favicon.strName = CFG_GetString( "favicon", string( ) );
	favicon.strDir = CFG_GetString( "favicon_dir", string( ) );
	favicon.strExt = getFileExt( favicon.strName );

	userbar.strName = CFG_GetString( "userbar", string( ) );
	userbar.strDir = CFG_GetString( "userbar_dir", string( ) );
	
	if( !userbar.strDir.empty( ) && userbar.strDir[userbar.strDir.size( ) - 1] != PATH_SEP )
		userbar.strDir += PATH_SEP;


	// CSS
	style.strName = CFG_GetString( "bnbt_style_sheet", string( ) );
	style.strURL = CFG_GetString( "bnbt_style_sheet_url", string( ) );
	style.strDir = CFG_GetString( "bnbt_style_sheet_dir", string( ) );
	style.strExt = getFileExt( style.strName );
	
	if( !style.strDir.empty( ) && style.strDir[style.strDir.size( ) - 1] != PATH_SEP )
		style.strDir += PATH_SEP;

	// XML
	xmldump.strName = CFG_GetString( "bnbt_dump_xml_file", string( ) );
	xmldump.strDir = CFG_GetString( "bnbt_dump_xml_dir", string( ) );
	xmldump.strURL = CFG_GetString( "bnbt_dump_xml_url", string( ) );
	xmldump.strExt = getFileExt( xmldump.strName );
	m_uiDumpXMLInterval = CFG_GetInt( "bnbt_dump_xml_interval", 600 );
	m_ulDumpXMLNext = GetTime( ) + m_uiDumpXMLInterval;
	m_bDumpXMLPeers = CFG_GetInt( "bnbt_dump_xml_peers", 1 ) == 0 ? false : true;

	// RSS
	rssdump.strName = CFG_GetString( "bnbt_rss_file", string( ) );
	rssdump.strURL = CFG_GetString( "bnbt_rss_online_dir", HTML_MakeURLFromFQDN( m_strTrackerFQDN, m_uiPort ) );
	rssdump.strDir = CFG_GetString( "bnbt_rss_directory", string( ) );
	rssdump.strExt = getFileExt( rssdump.strName );
	
	if( !rssdump.strDir.empty( ) && rssdump.strDir[rssdump.strDir.size( ) - 1] != PATH_SEP )
		rssdump.strDir += PATH_SEP;

	// image bar fill
	imagefill.strName = CFG_GetString( "image_bar_fill", string( ) );
	imagefill.strExt = getFileExt( imagefill.strName );
	imagefill.strDir = CFG_GetString( "image_bar_dir", string( ) );
	imagefill.strURL = CFG_GetString( "image_bar_url", string( ) );

	if( !imagefill.strDir.empty( ) && imagefill.strDir[imagefill.strDir.size( ) - 1] != PATH_SEP )
		imagefill.strDir += PATH_SEP;

	// image bar trans
	imagetrans.strName = CFG_GetString( "image_bar_trans", string( ) );
	imagetrans.strExt = getFileExt( imagetrans.strName );
	imagetrans.strDir = CFG_GetString( "image_bar_dir", string( ) );
	imagetrans.strURL = CFG_GetString( "image_bar_url", string( ) );

	if( !imagetrans.strDir.empty( ) && imagetrans.strDir[imagetrans.strDir.size( ) - 1] != PATH_SEP )
		imagetrans.strDir += PATH_SEP;

	// Initialise XBNBT XStats variables
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "CTracker: Initialise XBNBT XStats variables\n" );

	statsdump.strName = CFG_GetString( "bnbt_stats_file", "stats.bnbt" );
	m_uiXStatsInterval = CFG_GetInt( "bnbt_stats_dump_interval", 600 );
	m_ulXStatsNext = GetTime( ) + m_uiXStatsInterval;

	// Initialise XBNBT XTorrent variables
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "CTracker: Initialise XBNBT XTorrent variables\n" );

	xtorrent.bForceAnnounceDL = CFG_GetInt( "bnbt_force_announce_on_download", 0 ) == 0 ? false : true;
	xtorrent.bForceAnnounceUL = CFG_GetInt( "bnbt_force_announce_on_upload", 0 ) == 0 ? false : true;
	xtorrent.bEnableAnnounceList = CFG_GetInt( "bnbt_announce_list_enable", 0 ) == 0 ? false : true;

	// Initialise XBNBT constant variables
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "CTracker: Initialise XBNBT constant variables\n" );

	// Language
	// login1=not logged in
	
	// validate1=validate RSS without images
	m_strValidate1 = UTIL_Xsprintf( "document.write( \"<a rel=\'%s\' title=\'%s\' href=\'http://www.feedvalidator.org/check?url=http://\" + parent.location.host + \"/%s\'>%s<\\/a>\" );", STR_TARGET_REL.c_str( ), gmapLANG_CFG["valid_rss"].c_str( ), rssdump.strName.c_str( ), gmapLANG_CFG["valid_rss"].c_str( ) );
	// validate2=validate RSS with images
	m_strValidate2 = UTIL_Xsprintf( "document.write( \"<a rel=\'%s\' title=\'%s\' href=\'http://www.feedvalidator.org/check?url=http://\" + parent.location.host + \"/%s\'><img src=\'%s\' alt=\'%s\' height=\'31\' width=\'88\'><\\/a>\" );", STR_TARGET_REL.c_str( ), gmapLANG_CFG["valid_rss"].c_str( ), rssdump.strName.c_str( ), m_strRSSValidImage.c_str( ), gmapLANG_CFG["valid_rss"].c_str( ) );
	// JS Reduce Characters - JS_Valid_Check( )
	m_strJSReduce = UTIL_Xsprintf( gmapLANG_CFG["js_reduce_characters"].c_str( ), CAtomInt( m_uiCommentLength ).toString( ).c_str( ) );
	m_strJSMsgReduce = UTIL_Xsprintf( gmapLANG_CFG["js_reduce_characters"].c_str( ), CAtomInt( m_uiMessageLength ).toString( ).c_str( ) );
	// JS Message Length - JS_Valid_Check( )
// 	m_strJSLength = UTIL_Xsprintf( gmapLANG_CFG["js_message_length"].c_str( ), "\" + theform.comment.value.length + \"" );
	m_strJSLength = UTIL_Xsprintf( gmapLANG_CFG["js_comment_length"].c_str( ), "\" + theform.comment.value.getBytes() + \"" );
// 	m_strJSMsgLength = UTIL_Xsprintf( gmapLANG_CFG["js_message_length"].c_str( ), "\" + theform.message.value.length + \"" );
	m_strJSMsgLength = UTIL_Xsprintf( gmapLANG_CFG["js_message_length"].c_str( ), "\" + theform.message.value.getBytes() + \"" );
	m_strJSTalkLength = UTIL_Xsprintf( gmapLANG_CFG["js_talk_length"].c_str( ), "\" + theform.talk.value.getBytes() + \"" );
	// RSS local link for info.html
	m_strRSSLocalLink = UTIL_Xsprintf( "document.write( \"<a rel=\'%s\' title=\'%s\' href=\'http://\" + parent.location.host + \"/%s\'>http://\" + parent.location.host + \"/%s<\\/a>\" );", STR_TARGET_REL.c_str( ), gmapLANG_CFG["navbar_rss"].c_str( ), rssdump.strName.c_str( ), rssdump.strName.c_str( ) );

	// XBNBT initialise tags for internal mouseover
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "CTracker: Initialise XBNBT Tags\n" );

	initTags( );
	initShareRatio( );

#if defined ( XBNBT_GD )
	// Initialise XBNBT DynStat variables
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "CTracker: Initialise XBNBT DynStat variables\n" );

	m_strDynstatFile = CFG_GetString( "xbnbt_dynstat_file", "dynstat.bnbt" );
	m_bDynstatGenerate = CFG_GetInt( "xbnbt_dynstat_generate", 0 ) == 0 ? false : true;
	m_ucDynstatSaveMode = (unsigned char)CFG_GetInt( "xbnbt_dynstat_savemode", 0 );
	m_bDynstatShowLink = CFG_GetInt( "xbnbt_dynstat_showlink", 0 ) == 0 ? false : true;
	m_cDynstatPNGCompress = (unsigned char)CFG_GetInt( "xbnbt_dynstat_png_compress", -1 );
	m_cDynstatJPGQuality = (unsigned char)CFG_GetInt( "xbnbt_dynstat_jpg_quality", -1 );
	m_uiDynstatDumpInterval = CFG_GetInt( "xbnbt_dynstat_interval", 10 );
	m_uiDynstatBackground = CFG_GetInt( "xbnbt_dynstat_background", 0 );
	m_uiDynstatXSize = CFG_GetInt( "xbnbt_dynstat_x_size", 350 );
	m_uiDynstatYSize = CFG_GetInt( "xbnbt_dynstat_y_size", 80 );
	m_ucDynstatFontRed = (unsigned char)CFG_GetInt( "xbnbt_dynstat_font_red", 255 );
	m_ucDynstatFontGreen = (unsigned char)CFG_GetInt( "xbnbt_dynstat_font_green", 255 );
	m_ucDynstatFontBlue = (unsigned char)CFG_GetInt( "xbnbt_dynstat_font_blue", 255 );
	m_ucDynstatOutType = (unsigned char)CFG_GetInt( "xbnbt_dynstat_output_type", 0 );
	m_strDynstatDumpFileDir = CFG_GetString( "xbnbt_dynstat_dir", string( ) );

	if( !m_strDynstatDumpFileDir.empty( ) && m_strDynstatDumpFileDir[m_strDynstatDumpFileDir.size( ) - 1] != PATH_SEP )
		m_strDynstatDumpFileDir += PATH_SEP;

	m_strDynstatFontFile = CFG_GetString( "xbnbt_dynstat_font", string( ) );
	m_strDynstatLinkURL = CFG_GetString( "xbnbt_dynstat_link", string( ) );
	m_strDynstatSkinFile = CFG_GetString( "xbnbt_dynstat_skinfile", string( ) );
	m_bDynstatSkin = CFG_GetInt( "xbnbt_dynstat_skin", 0 ) == 0 ? false : true;

	// Set m_ucDynstatInType the Dynstat image input file type
	if( !m_strDynstatSkinFile.empty( ) )
	{
		const string cstrExt( getFileExt( m_strDynstatSkinFile ) );

		m_ucDynstatInType = UNK_FORMAT;

		if( cstrExt == ".png" )
			m_ucDynstatInType = PNG_FORMAT;
		else if( cstrExt == ".jpg" || cstrExt == ".jpeg" || cstrExt == ".jpe" )
			m_ucDynstatInType = JPG_FORMAT;
		else if( cstrExt == ".gif" )
			m_ucDynstatInType = GIF_FORMAT;
	}

	// Set m_strImageOutExt the Dynstat image filename extension
	switch( m_ucDynstatOutType )
	{
	case JPG_FORMAT:
		m_strImageOutExt = ".jpg";

		break;
	case GIF_FORMAT:
		m_strImageOutExt = ".gif";

		break;
	default:
		m_strImageOutExt = ".png";
	}
#endif

	// decode dfile
	
	// XBNBT Decode XStats Dictionary from file

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "CTracker: Decode XStats file\n" );

	if( statsdump.strName.empty( ) )
		m_pXStats = new CAtomDicti( );
	else
	{
		CAtom *pXStats = DecodeFile( statsdump.strName.c_str( ) );

		if( pXStats && pXStats->isDicti( ) )
			m_pXStats = (CAtomDicti *)pXStats;
		else
		{
			if( pXStats )
				delete pXStats;

			m_pXStats = new CAtomDicti( );
		}
	}

#if defined ( XBNBT_GD )
	// XBNBT decode Dynstat frozen dictionary from file
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "CTracker: Decode Dynstat file\n" );

	if( m_strDynstatFile.empty( ) )
		m_pDynstat = new CAtomDicti( );
	else
	{
		CAtom *pDynstat = DecodeFile( m_strDynstatFile.c_str( ) );

		if( pDynstat && pDynstat->isDicti( ) )
			m_pDynstat = (CAtomDicti *)pDynstat;
		else
		{
			if( pDynstat )
				delete pDynstat;

			m_pDynstat = new CAtomDicti( );
		}
	}
#endif

	// parse the allowed dir
	
	if( m_bParseOnStart )
	{

		if( !m_strAllowedDir.empty( ) )
		{
			if( gbDebug )
				if( gucDebugLevel & DEBUG_TRACKER )
					UTIL_LogPrint( "CTracker: Parsing the allowed dir\n" );

			parseTorrents( m_strAllowedDir.c_str( ) );
		}
		
		if( !m_strOfferDir.empty( ) )
		{
			if( gbDebug )
				if( gucDebugLevel & DEBUG_TRACKER )
					UTIL_LogPrint( "CTracker: Parsing the offer dir\n" );

			parseTorrents( m_strOfferDir.c_str( ) );
		}
	}

	// Set the mime types
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "CTracker: Setting the mime types\n" );

	gmapMime[".hqx"]		= "application/mac-binhex40";
	gmapMime[".exe"]		= "application/octet-stream";
	gmapMime[".pdf"]		= "application/pdf";
	gmapMime[".rss"]		= "application/rss+xml";
	gmapMime[".torrent"]	= "application/x-bittorrent";
	gmapMime[".gtar"]		= "application/x-gtar";
	gmapMime[".gz"]			= "application/x-gzip";
	gmapMime[".js"]			= "application/x-javascript";
	gmapMime[".swf"]		= "application/x-shockwave-flash";
	gmapMime[".sit"]		= "application/x-stuffit";
	gmapMime[".tar"]		= "application/x-tar";
	//gmapMime[".xml"]		= "application/xml";
	gmapMime[".xml"]		= "text/xml";
	//gmapMime[".xsl"]		= "application/xsl-xml";
	//gmapMime[".xsl"]		= "text/xsl-xml";
	gmapMime[".xsl"]		= "text/xsl";
	gmapMime[".zip"]		= "application/zip";
	gmapMime[".rar"]		= "application/rar";
	gmapMime[".bmp"]		= "image/bmp";
	gmapMime[".gif"]		= "image/gif";
	gmapMime[".jpg"]		= "image/jpeg";
	gmapMime[".jpeg"]		= "image/jpeg";
	gmapMime[".jpe"]		= "image/jpeg";
	gmapMime[".png"]		= "image/png";
	gmapMime[".tiff"]		= "image/tiff";
	gmapMime[".tif"]		= "image/tiff";
	gmapMime[".ico"]		= "image/vnd.microsoft.icon";
	gmapMime[".css"]		= "text/css";
	gmapMime[".html"]		= "text/html";
	gmapMime[".htm"]		= "text/html";
	gmapMime[".txt"]		= "text/plain";
	gmapMime[".rtf"]		= "text/rtf";

	//
	// CBTT parse list
	//

#define STR_SEPARATORS "\n\r"
	
	m_pClientBannedList = new CAtomList( );
	m_pIPBannedList = new CAtomList( );

	CBTTParseList( );

	// XBNBT XStats, import the data from the xstats file if the xstats file name has been set
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "CTracker: transferring XStats dictionary to structure\n" );

	if( !statsdump.strName.empty( ) )
	{
		if( m_pXStats )
		{
			// Announce
			if( !m_pXStats->getItem( "announce" ) )
				m_pXStats->setItem( "announce", new CAtomDicti( ) );

			CAtom *pXAnnounce = m_pXStats->getItem( "announce" );

			if( pXAnnounce && pXAnnounce->isDicti( ) )
			{
				CAtom *piAnnounce = ( (CAtomDicti *)pXAnnounce )->getItem( "iAnnounce" );
				if( piAnnounce )
					gtXStats.announce.iAnnounce = dynamic_cast<CAtomLong *>( piAnnounce )->getValue( );
				else
					gtXStats.announce.iAnnounce = 0;

				CAtom *piAnnMissing = ( (CAtomDicti *)pXAnnounce )->getItem( "iAnnMissing" );
				if( piAnnMissing )
					gtXStats.announce.iAnnMissing = dynamic_cast<CAtomLong *>( piAnnMissing )->getValue( );
				else
					gtXStats.announce.iAnnMissing = 0;

				CAtom *piAnnNotAuth = ( (CAtomDicti *)pXAnnounce )->getItem( "iAnnNotAuth" );
				if( piAnnNotAuth )
					gtXStats.announce.iAnnNotAuth = dynamic_cast<CAtomLong *>( piAnnNotAuth )->getValue( );
				else
					gtXStats.announce.iAnnNotAuth = 0;

				CAtom *piClientBanned = ( (CAtomDicti *)pXAnnounce )->getItem( "iClientBanned" );
				if( piClientBanned )
					gtXStats.announce.iClientBanned = dynamic_cast<CAtomLong *>( piClientBanned )->getValue( );
				else
					gtXStats.announce.iClientBanned = 0;

				CAtom *piCompact = ( (CAtomDicti *)pXAnnounce )->getItem( "iCompact" );
				if( piCompact )
					gtXStats.announce.iCompact = dynamic_cast<CAtomLong *>( piCompact )->getValue( );
				else
					gtXStats.announce.iCompact = 0;

				CAtom *piDownloadedInvalid = ( (CAtomDicti *)pXAnnounce )->getItem( "iDownloadedInvalid" );
				if( piDownloadedInvalid )
					gtXStats.announce.iDownloadedInvalid = dynamic_cast<CAtomLong *>( piDownloadedInvalid )->getValue( );
				else
					gtXStats.announce.iDownloadedInvalid = 0;

				CAtom *piDownloadedMissing = ( (CAtomDicti *)pXAnnounce )->getItem( "iDownloadedMissing" );
				if( piDownloadedMissing )
					gtXStats.announce.iDownloadedMissing = dynamic_cast<CAtomLong *>( piDownloadedMissing )->getValue( );
				else
					gtXStats.announce.iDownloadedMissing = 0;

				CAtom *piInvalidEvent = ( (CAtomDicti *)pXAnnounce )->getItem( "iInvalidEvent" );
				if( piInvalidEvent )
					gtXStats.announce.iInvalidEvent = dynamic_cast<CAtomLong *>( piInvalidEvent )->getValue( );
				else
					gtXStats.announce.iInvalidEvent = 0;

				CAtom *piIPBanned = ( (CAtomDicti *)pXAnnounce )->getItem( "iIPBanned" );
				if( piIPBanned )
					gtXStats.announce.iIPBanned = dynamic_cast<CAtomLong *>( piIPBanned )->getValue( );
				else
					gtXStats.announce.iIPBanned = 0;

				CAtom *piIPNotCleared = ( (CAtomDicti *)pXAnnounce )->getItem( "iIPNotCleared" );
				if( piIPNotCleared )
					gtXStats.announce.iIPNotCleared = dynamic_cast<CAtomLong *>( piIPNotCleared )->getValue( );
				else
					gtXStats.announce.iIPNotCleared = 0;

				CAtom *piLeftMissing = ( (CAtomDicti *)pXAnnounce )->getItem( "iLeftMissing" );
				if( piLeftMissing )
					gtXStats.announce.iLeftMissing = dynamic_cast<CAtomLong *>( piLeftMissing )->getValue( );
				else
					gtXStats.announce.iLeftMissing = 0;

				CAtom *piNopeerid = ( (CAtomDicti *)pXAnnounce )->getItem( "iNopeerid" );
				if( piNopeerid )
					gtXStats.announce.iNopeerid = dynamic_cast<CAtomLong *>( piNopeerid )->getValue( );
				else
					gtXStats.announce.iNopeerid = 0;

				CAtom *piPeerIDLength = ( (CAtomDicti *)pXAnnounce )->getItem( "iPeerIDLength" );
				if( piPeerIDLength )
					gtXStats.announce.iPeerIDLength = dynamic_cast<CAtomLong *>( piPeerIDLength )->getValue( );
				else
					gtXStats.announce.iPeerIDLength = 0;

				CAtom *piPeerSpoofRestrict = ( (CAtomDicti *)pXAnnounce )->getItem( "iPeerSpoofRestrict" );
				if( piPeerSpoofRestrict )
					gtXStats.announce.iPeerSpoofRestrict = dynamic_cast<CAtomLong *>( piPeerSpoofRestrict )->getValue( );
				else
					gtXStats.announce.iPeerSpoofRestrict = 0;

				CAtom *piPortBlacklisted = ( (CAtomDicti *)pXAnnounce )->getItem( "iPortBlacklisted" );
				if( piPortBlacklisted )
					gtXStats.announce.iPortBlacklisted = dynamic_cast<CAtomLong *>( piPortBlacklisted )->getValue( );
				else
					gtXStats.announce.iPortBlacklisted = 0;

				CAtom *piPortMissing = ( (CAtomDicti *)pXAnnounce )->getItem( "iPortMissing" );
				if( piPortMissing )
					gtXStats.announce.iPortMissing = dynamic_cast<CAtomLong *>( piPortMissing )->getValue( );
				else
					gtXStats.announce.iPortMissing = 0;

				CAtom *piRegular = ( (CAtomDicti *)pXAnnounce )->getItem( "iRegular" );
				if( piRegular )
					gtXStats.announce.iRegular = dynamic_cast<CAtomLong *>( piRegular )->getValue( );
				else
					gtXStats.announce.iRegular = 0;

				CAtom *piUploadedInvalid = ( (CAtomDicti *)pXAnnounce )->getItem( "iUploadedInvalid" );
				if( piUploadedInvalid )
					gtXStats.announce.iUploadedInvalid= dynamic_cast<CAtomLong *>( piUploadedInvalid )->getValue( );
				else
					gtXStats.announce.iUploadedInvalid= 0;

				CAtom *piUploadedMissing = ( (CAtomDicti *)pXAnnounce )->getItem( "iUploadedMissing" );
				if( piUploadedMissing )
					gtXStats.announce.iUploadedMissing = dynamic_cast<CAtomLong *>( piUploadedMissing )->getValue( );
				else
					gtXStats.announce.iUploadedMissing = 0;

				CAtom *piNotAuthorized = ( (CAtomDicti *)pXAnnounce )->getItem( "iNotAuthorized" );
				if( piNotAuthorized )
					gtXStats.announce.iNotAuthorized = dynamic_cast<CAtomLong *>( piNotAuthorized )->getValue( );
				else
					gtXStats.announce.iNotAuthorized = 0;

				CAtom *psLastReset = ( (CAtomDicti *)pXAnnounce )->getItem( "sLastReset" );
				if( psLastReset )
					gtXStats.announce.sLastReset = psLastReset->toString( );
				else
					gtXStats.announce.sLastReset = UTIL_Date( );
			}

#if defined ( XBNBT_GD )
			// Dynstat
			if( !m_pXStats->getItem( "dynstat" ) )
				m_pXStats->setItem( "dynstat", new CAtomDicti( ) );

			CAtom *pXDynstat = m_pXStats->getItem( "dynstat" );

			if( pXDynstat && pXDynstat->isDicti( ) )
			{
				CAtom *piFrozen = ( (CAtomDicti *)pXDynstat )->getItem( "iFrozen" );
				if( piFrozen )
					gtXStats.dynstat.iFrozen = dynamic_cast<CAtomLong *>( piFrozen )->getValue( );
				else
					gtXStats.dynstat.iFrozen = 0;

				CAtom *piProcessed = ( (CAtomDicti *)pXDynstat )->getItem( "iProcessed" );
				if( piProcessed )
					gtXStats.dynstat.iProcessed = dynamic_cast<CAtomLong *>( piProcessed )->getValue( );
				else
					gtXStats.dynstat.iProcessed = 0;

				CAtom *piRun = ( (CAtomDicti *)pXDynstat )->getItem( "iRun" );
				if( piRun )
					gtXStats.dynstat.iRun = dynamic_cast<CAtomLong *>( piRun )->getValue( );
				else
					gtXStats.dynstat.iRun = 0;

				CAtom *piTotalProcessed = ( (CAtomDicti *)pXDynstat )->getItem( "iTotalProcessed" );
				if( piTotalProcessed )
					gtXStats.dynstat.iTotalProcessed = dynamic_cast<CAtomLong *>( piTotalProcessed )->getValue( );
				else
					gtXStats.dynstat.iTotalProcessed = 0;

				CAtom *psLastRunTime = ( (CAtomDicti *)pXDynstat )->getItem( "sLastRunTime" );
				if( piTotalProcessed )
					gtXStats.dynstat.sLastRunTime = psLastRunTime->toString( );
				else
					gtXStats.dynstat.sLastRunTime = string( );

				CAtom *psElapsedTime = ( (CAtomDicti *)pXDynstat )->getItem( "sElapsedTime" );
				if( piTotalProcessed )
					gtXStats.dynstat.sElapsedTime = psElapsedTime->toString( );
				else
					gtXStats.dynstat.sElapsedTime = string( );

				CAtom *psLastReset = ( (CAtomDicti *)pXDynstat )->getItem( "sLastReset" );
				if( psLastReset )
					gtXStats.dynstat.sLastReset = psLastReset->toString( );
				else
					gtXStats.dynstat.sLastReset = UTIL_Date( );
			}
#endif

			// File
			if( !m_pXStats->getItem( "file" ) )
				m_pXStats->setItem( "file", new CAtomDicti( ) );

			CAtom *pXFile = m_pXStats->getItem( "file" );

			if( pXFile && pXFile->isDicti( ) )
			{
				CAtom *piBarFill = ( (CAtomDicti *)pXFile )->getItem( "iBarFill" );
				if( piBarFill )
					gtXStats.file.iBarFill = dynamic_cast<CAtomLong *>( piBarFill )->getValue( );
				else
					gtXStats.file.iBarFill = 0;

				CAtom *piBarTrans = ( (CAtomDicti *)pXFile )->getItem( "iBarTrans" );
				if( piBarTrans )
					gtXStats.file.iBarTrans = dynamic_cast<CAtomLong *>( piBarTrans )->getValue( );
				else
					gtXStats.file.iBarTrans = 0;

				CAtom *piCSS = ( (CAtomDicti *)pXFile )->getItem( "iCSS" );
				if( piCSS )
					gtXStats.file.iCSS = dynamic_cast<CAtomLong *>( piCSS )->getValue( );
				else
					gtXStats.file.iCSS = 0;

				CAtom *piFavicon = ( (CAtomDicti *)pXFile )->getItem( "iFavicon" );
				if( piFavicon )
					gtXStats.file.iFavicon = dynamic_cast<CAtomLong *>( piFavicon )->getValue( );
				else
					gtXStats.file.iFavicon = 0;

				CAtom *piFiles = ( (CAtomDicti *)pXFile )->getItem( "uiFiles" );
				if( piFiles )
					gtXStats.file.uiFiles = dynamic_cast<CAtomLong *>( piFiles )->getValue( );
				else
					gtXStats.file.uiFiles = 0;

				CAtom *piRobots = ( (CAtomDicti *)pXFile )->getItem( "iRobots" );
				if( piRobots )
					gtXStats.file.iRobots = dynamic_cast<CAtomLong *>( piRobots )->getValue( );
				else
					gtXStats.file.iRobots = 0;

				CAtom *piRSS = ( (CAtomDicti *)pXFile )->getItem( "iRSS" );
				if( piRSS )
					gtXStats.file.iRSS = dynamic_cast<CAtomLong *>( piRSS )->getValue( );
				else
					gtXStats.file.iRSS = 0;

				CAtom *piRSSXSL = ( (CAtomDicti *)pXFile )->getItem( "iRSSXSL" );
				if( piRSSXSL )
					gtXStats.file.iRSSXSL = dynamic_cast<CAtomLong *>( piRSSXSL )->getValue( );
				else
					gtXStats.file.iRSSXSL = 0;

				CAtom *piXML = ( (CAtomDicti *)pXFile )->getItem( "iXML" );
				if( piXML )
					gtXStats.file.iXML = dynamic_cast<CAtomLong *>( piXML )->getValue( );
				else
					gtXStats.file.iXML = 0;

				CAtom *piTorrent = ( (CAtomDicti *)pXFile )->getItem( "iTorrent" );
				if( piTorrent )
					gtXStats.file.iTorrent = dynamic_cast<CAtomLong *>( piTorrent )->getValue( );
				else
					gtXStats.file.iTorrent = 0;

				CAtom *psLastReset = ( (CAtomDicti *)pXFile )->getItem( "sLastReset" );
				if( psLastReset )
					gtXStats.file.sLastReset = psLastReset->toString( );
				else
					gtXStats.file.sLastReset = UTIL_Date( );
			}

			// Page
			if( !m_pXStats->getItem( "page" ) )
				m_pXStats->setItem( "page", new CAtomDicti( ) );

			CAtom *pXPage = m_pXStats->getItem( "page" );

			if( pXPage && pXPage->isDicti( ) )
			{
				CAtom *piAdmin = ( (CAtomDicti *)pXPage )->getItem( "iAdmin" );
				if( piAdmin )
					gtXStats.page.iAdmin = dynamic_cast<CAtomLong *>( piAdmin )->getValue( );
				else
					gtXStats.page.iAdmin = 0;

				CAtom *piComments = ( (CAtomDicti *)pXPage )->getItem( "uiComments" );
				if( piComments )
					gtXStats.page.uiComments = dynamic_cast<CAtomLong *>( piComments )->getValue( );
				else
					gtXStats.page.uiComments = 0;

				CAtom *piError404 = ( (CAtomDicti *)pXPage )->getItem( "iError404" );
				if( piError404 )
					gtXStats.page.iError404 = dynamic_cast<CAtomLong *>( piError404 )->getValue( );
				else
					gtXStats.page.iError404 = 0;

				CAtom *piIndex = ( (CAtomDicti *)pXPage )->getItem( "iIndex" );
				if( piIndex )
					gtXStats.page.iIndex = dynamic_cast<CAtomLong *>( piIndex )->getValue( );
				else
					gtXStats.page.iIndex = 0;

				CAtom *piInfo = ( (CAtomDicti *)pXPage )->getItem( "iInfo" );
				if( piInfo )
					gtXStats.page.iInfo = dynamic_cast<CAtomLong *>( piInfo )->getValue( );
				else
					gtXStats.page.iInfo = 0;
								
				CAtom *piFAQ = ( (CAtomDicti *)pXPage )->getItem( "iFAQ" );
				if( piFAQ )
					gtXStats.page.iFAQ = dynamic_cast<CAtomLong *>( piFAQ )->getValue( );
				else
					gtXStats.page.iFAQ = 0;

				CAtom *piLogin = ( (CAtomDicti *)pXPage )->getItem( "iLogin" );
				if( piLogin )
					gtXStats.page.iLogin = dynamic_cast<CAtomLong *>( piLogin )->getValue( );
				else
					gtXStats.page.iLogin = 0;
				
				CAtom *piMessages = ( (CAtomDicti *)pXPage )->getItem( "iMessages" );
				if( piMessages )
					gtXStats.page.iMessages = dynamic_cast<CAtomLong *>( piMessages )->getValue( );
				else
					gtXStats.page.iMessages = 0;
				
				CAtom *piOffer = ( (CAtomDicti *)pXPage )->getItem( "iOffer" );
				if( piOffer )
					gtXStats.page.iOffer = dynamic_cast<CAtomLong *>( piOffer )->getValue( );
				else
					gtXStats.page.iOffer = 0;

				CAtom *piRules = ( (CAtomDicti *)pXPage )->getItem( "iRules" );
				if( piRules )
					gtXStats.page.iRules = dynamic_cast<CAtomLong *>( piRules )->getValue( );
				else
					gtXStats.page.iRules = 0;
				
				CAtom *piSignup = ( (CAtomDicti *)pXPage )->getItem( "iSignup" );
				if( piSignup )
					gtXStats.page.iSignup = dynamic_cast<CAtomLong *>( piSignup )->getValue( );
				else
					gtXStats.page.iSignup = 0;

				CAtom *piStats = ( (CAtomDicti *)pXPage )->getItem( "iStats" );
				if( piStats )
					gtXStats.page.iStats = dynamic_cast<CAtomLong *>( piStats )->getValue( );
				else
					gtXStats.page.iStats = 0;

				CAtom *piTorrents = ( (CAtomDicti *)pXPage )->getItem( "iTorrents" );
				if( piTorrents )
					gtXStats.page.iTorrents= dynamic_cast<CAtomLong *>( piTorrents )->getValue( );
				else
					gtXStats.page.iTorrents= 0;

				CAtom *piUpload = ( (CAtomDicti *)pXPage )->getItem( "iUpload" );
				if( piUpload )
					gtXStats.page.iUpload = dynamic_cast<CAtomLong *>( piUpload )->getValue( );
				else
					gtXStats.page.iUpload = 0;

				CAtom *piUsers = ( (CAtomDicti *)pXPage )->getItem( "iUsers" );
				if( piUsers )
					gtXStats.page.iUsers = dynamic_cast<CAtomLong *>( piUsers )->getValue( );
				else
					gtXStats.page.iUsers = 0;

				CAtom *psLastReset = ( (CAtomDicti *)pXPage )->getItem( "sLastReset" );
				if( psLastReset )
					gtXStats.page.sLastReset = psLastReset->toString( );
				else
					gtXStats.page.sLastReset = UTIL_Date( );

				CAtom *piTags = ( (CAtomDicti *)pXPage )->getItem( "iTags" );
				if( piTags )
					gtXStats.page.iTags = dynamic_cast<CAtomLong *>( piTags )->getValue( );
				else
					gtXStats.page.iTags = 0;

				CAtom *piLanguage = ( (CAtomDicti *)pXPage )->getItem( "iLanguage" );
				if( piLanguage )
					gtXStats.page.iLanguage = dynamic_cast<CAtomLong *>( piLanguage )->getValue( );
				else
					gtXStats.page.iLanguage = 0;

				CAtom *piXStats = ( (CAtomDicti *)pXPage )->getItem( "iXStats" );
				if( piXStats )
					gtXStats.page.iXStats = dynamic_cast<CAtomLong *>( piXStats )->getValue( );
				else
					gtXStats.page.iXStats = 0;
			}

			// Scrape
			if( !m_pXStats->getItem( "scrape" ) )
				m_pXStats->setItem( "scrape", new CAtomDicti( ) );

			CAtom *pXScrape = m_pXStats->getItem( "scrape" );

			if( pXScrape && pXScrape->isDicti( ) )
			{
				CAtom *piFull = ( (CAtomDicti *)pXScrape )->getItem( "iFull" );
				if( piFull )
					gtXStats.scrape.iFull = dynamic_cast<CAtomLong *>( piFull )->getValue( );
				else
					gtXStats.scrape.iFull = 0;

				CAtom *piScrape = ( (CAtomDicti *)pXScrape )->getItem( "iScrape" );
				if( piScrape )
					gtXStats.scrape.iScrape = dynamic_cast<CAtomLong *>( piScrape )->getValue( );
				else
					gtXStats.scrape.iScrape = 0;

				CAtom *piDisallowed = ( (CAtomDicti *)pXScrape )->getItem( "iDisallowed" );
				if( piDisallowed )
					gtXStats.scrape.iDisallowed = dynamic_cast<CAtomLong *>( piDisallowed )->getValue( );
				else
					gtXStats.scrape.iDisallowed = 0;

				CAtom *piSingle = ( (CAtomDicti *)pXScrape )->getItem( "iSingle" );
				if( piSingle )
					gtXStats.scrape.iSingle = dynamic_cast<CAtomLong *>( piSingle )->getValue( );
				else
					gtXStats.scrape.iSingle = 0;

				CAtom *piMultiple = ( (CAtomDicti *)pXScrape )->getItem( "iMultiple" );
				if( piMultiple )
					gtXStats.scrape.iMultiple = dynamic_cast<CAtomLong *>( piMultiple )->getValue( );
				else
					gtXStats.scrape.iMultiple = 0;

				CAtom *piNotAuthorized = ( (CAtomDicti *)pXScrape )->getItem( "iNotAuthorized" );
				if( piNotAuthorized )
					gtXStats.scrape.iNotAuthorized = dynamic_cast<CAtomLong *>( piNotAuthorized )->getValue( );
				else
					gtXStats.scrape.iNotAuthorized = 0;

				CAtom *psLastReset = ( (CAtomDicti *)pXScrape )->getItem( "sLastReset" );
				if( psLastReset )
					gtXStats.scrape.sLastReset = psLastReset->toString( );
				else






					gtXStats.scrape.sLastReset = UTIL_Date( );
			}

			// TCP
			if( !m_pXStats->getItem( "tcp" ) )
				m_pXStats->setItem( "tcp", new CAtomDicti( ) );

			CAtom *pXTCP = m_pXStats->getItem( "tcp" );

			if( pXTCP && pXTCP->isDicti( ) )
			{
				CAtom *piRecv = ( (CAtomDicti *)pXTCP )->getItem( "iRecv" );
				if( piRecv )
					gtXStats.tcp.iRecv = dynamic_cast<CAtomLong *>( piRecv )->getValue( );
				else
					gtXStats.tcp.iRecv = 0;

				CAtom *piSend = ( (CAtomDicti *)pXTCP )->getItem( "iSend" );
				if( piSend )
					gtXStats.tcp.iSend = dynamic_cast<CAtomLong *>( piSend )->getValue( );
				else
					gtXStats.tcp.iSend = 0;

				CAtom *piRecvHub = ( (CAtomDicti *)pXTCP )->getItem( "iRecvHub" );
				if( piRecvHub )
					gtXStats.tcp.iRecvHub = dynamic_cast<CAtomLong *>( piRecvHub )->getValue( );
				else
					gtXStats.tcp.iRecvHub = 0;

				CAtom *piSendHub = ( (CAtomDicti *)pXTCP )->getItem( "iSendHub" );
				if( piSendHub )
					gtXStats.tcp.iSendHub = dynamic_cast<CAtomLong *>( piSendHub )->getValue( );
				else
					gtXStats.tcp.iSendHub = 0;

				CAtom *piRecvLink = ( (CAtomDicti *)pXTCP )->getItem( "iRecvLink" );
				if( piRecvLink )
					gtXStats.tcp.iRecvLink = dynamic_cast<CAtomLong *>( piRecvLink )->getValue( );
				else
					gtXStats.tcp.iRecvLink = 0;

				CAtom *piSendLink = ( (CAtomDicti *)pXTCP )->getItem( "iSendLink" );
				if( piSendLink )
					gtXStats.tcp.iSendLink = dynamic_cast<CAtomLong *>( piSendLink )->getValue( );
				else
					gtXStats.tcp.iSendLink = 0;

				CAtom *psLastReset = ( (CAtomDicti *)pXTCP )->getItem( "sLastReset" );
				if( psLastReset )
					gtXStats.tcp.sLastReset = psLastReset->toString( );
				else
					gtXStats.tcp.sLastReset = UTIL_Date( );
			}

			// peer
			if( !m_pXStats->getItem( "peer" ) )
				m_pXStats->setItem( "peer", new CAtomDicti( ) );

			CAtom *pXPeer = m_pXStats->getItem( "peer" );

			if( pXPeer && pXPeer->isDicti( ) )
			{
				CAtom *piGreatest = ( (CAtomDicti *)pXPeer )->getItem( "iGreatest" );
				if( piGreatest )
					gtXStats.peer.iGreatest = dynamic_cast<CAtomLong *>( piGreatest )->getValue( );
				else
					gtXStats.peer.iGreatest = 0;

				CAtom *piGreatestSeeds = ( (CAtomDicti *)pXPeer )->getItem( "iGreatestSeeds" );
				if( piGreatestSeeds )
					gtXStats.peer.iGreatestSeeds = dynamic_cast<CAtomLong *>( piGreatestSeeds )->getValue( );
				else
					gtXStats.peer.iGreatestSeeds = 0;

				CAtom *piGreatestLeechers = ( (CAtomDicti *)pXPeer )->getItem( "iGreatestLeechers" );
				if( piGreatestLeechers )
					gtXStats.peer.iGreatestLeechers = dynamic_cast<CAtomLong *>( piGreatestLeechers )->getValue( );
				else
					gtXStats.peer.iGreatestLeechers = 0;

				CAtom *piGreatestUnique = ( (CAtomDicti *)pXPeer )->getItem( "iGreatestUnique" );
				if( piGreatestUnique )
					gtXStats.peer.iGreatestUnique = dynamic_cast<CAtomLong *>( piGreatestUnique )->getValue( );
				else
					gtXStats.peer.iGreatestUnique = 0;

				CAtom *psLastReset = ( (CAtomDicti *)pXPeer )->getItem( "sLastReset" );
				if( psLastReset )
					gtXStats.peer.sLastReset = psLastReset->toString( );
				else
					gtXStats.peer.sLastReset = UTIL_Date( );
			}
		}
	}

#if defined ( XBNBT_GD )
	// XBNBT parse the Dynstat image creation engine if set to generate and the font file name has beeen set
	if( m_bDynstatGenerate && !m_strDynstatFontFile.empty( ) )
	{
		if( gbDebug )
			if( gucDebugLevel & DEBUG_TRACKER )
				UTIL_LogPrint( "CTracker: Initial parse of Dynstat\n" );

		runGenerateDynstat( );
	}
#endif

	// XML, dump the XML file if the filename has been set
// 	if( !xmldump.strName.empty( ) )
// 	{
// 		if( gbDebug )
// 			if( gucDebugLevel & DEBUG_TRACKER )
// 				UTIL_LogPrint( "Initial save of the XML file\n" );
// 
// 		saveXML( );
// 	}

//	CMySQLQuery *pQueryTalk = new CMySQLQuery( "SELECT bid,btalkstore FROM talk" );
//			
//	vector<string> vecQueryTalk;

//	vecQueryTalk.reserve(2);

//	vecQueryTalk = pQueryTalk->nextRow( );
//	
//	while( vecQueryTalk.size( ) == 2 )
//	{
//		string strTalk( vecQueryTalk[1] );
//		
//		string :: size_type iStart = 0;
//		string :: size_type iEnd = 0;
//		string :: size_type iEnd1 = 0;
//		string :: size_type iLength = 0;
//		
//		iStart = strTalk.find( "@" );
//		while( iStart != string :: npos )
//		{
//			iEnd = strTalk.find_first_of( " :", iStart );
//			iLength = 1;
//			iEnd1 = strTalk.find( "：", iStart );
//			if( iEnd1 < iEnd )
//			{
//				iEnd = iEnd1;
//				iLength = string( "：" ).size( );
//			}
//			if( iEnd != string :: npos )
//			{
//				if( iEnd - iStart - 1 > 0 && iEnd - iStart - 1 <= 16 )
//				{
//					string strUsername = strTalk.substr( iStart + 1, iEnd - iStart - 1 );
//		
//					CMySQLQuery *pQueryUser = new CMySQLQuery( "SELECT buid FROM users WHERE busername=\'" + UTIL_StringToMySQL( strUsername ) + "\'" );

//					vector<string> vecQueryUser;

//					vecQueryUser.reserve(1);

//					vecQueryUser = pQueryUser->nextRow( );

//					delete pQueryUser;

//					if( vecQueryUser.size( ) == 1 && !vecQueryUser[0].empty( ) )
//					{
//						strTalk.replace( iStart + 1, iEnd - iStart - 1 + iLength, "#" + vecQueryUser[0] + "# " );
//						
//						iStart = strTalk.find( "@", iStart + 1 + vecQueryUser[0].size( ) + 2 );
//					}
//					else
//						iStart = strTalk.find( "@", iStart + 1 );
//				}
//				else
//					iStart = strTalk.find( "@", iStart + 1 );
//			}
//			else
//				iStart = strTalk.find( "@", iEnd );
//		}
//		CMySQLQuery mq01( "UPDATE talk SET btalk=\'" + UTIL_StringToMySQL( strTalk ) + "\' WHERE bid=" + vecQueryTalk[0] );
//		
//		vecQueryTalk = pQueryTalk->nextRow( );
//	}
//	delete pQueryTalk;

//	CMySQLQuery *pQueryTalk = new CMySQLQuery( "SELECT bid,btalkstore,bposted FROM talk" );
//			
//	vector<string> vecQueryTalk;

//	vecQueryTalk.reserve(3);

//	vecQueryTalk = pQueryTalk->nextRow( );
//	
//	while( vecQueryTalk.size( ) == 3 )
//	{
//		string strTalk( vecQueryTalk[1] );
//		
//		string :: size_type iStart = 0;
//		string :: size_type iEnd = 0;
//		iStart = strTalk.find( "#" );
//		while( iStart != string :: npos )
//		{
//			iEnd = strTalk.find( "#", iStart + 1 );
//			
//			if( iEnd != string :: npos )
//			{
//				if( iEnd - iStart - 1 > 0 && iEnd - iStart - 1 < 40 )
//				{
//					string strTopic = strTalk.substr( iStart, iEnd - iStart + 1 );
//					string strTag = string( );
//					string strID = string( );
//					string strFullLink = string( );

//					if( strTopic.find_first_of( "\r\n" ) == string :: npos )
//					{
//						strTag = strTopic.substr( 1, iEnd - iStart - 1 );
//						
//						CMySQLQuery mq01( "INSERT INTO talktag (btag,bid,bposted) VALUES('" + UTIL_StringToMySQL( strTag ) + "'," + vecQueryTalk[0] + ",'" + UTIL_StringToMySQL( vecQueryTalk[2] ) + "')" );
//				
//						iStart = strTalk.find( "#", iEnd + 1 );
//					}
//					else
//						iStart = strTalk.find( "#", iEnd + 1 );
//				}
//				else
//					iStart = strTalk.find( "#", iEnd + 1 );
//			}
//			else
//				iStart = strTalk.find( "#", iEnd );
//		}
//		vecQueryTalk = pQueryTalk->nextRow( );
//	}
//	delete pQueryTalk;

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "CTracker: Constructor completed\n" );
}

// The destructor
CTracker :: ~CTracker( )
{
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "~CTracker: Destructor called\n" );

// 	if( gbDebug )
// 		if( gucDebugLevel & DEBUG_TRACKER )
// 			UTIL_LogPrint( "~CTracker: saving the XML file\n" );
// 
// 	saveXML( );

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "~CTracker: saving the XStats file\n" );

	saveXStats( );

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "~CTracker: freeing memory\n" );

	// XBNBT XStats
	if( m_pXStats )
		delete m_pXStats;

	m_pXStats = 0;

#if defined ( XBNBT_GD )
	// Dynstat Image Generator
	if( m_pDynstat )
		delete m_pDynstat;

	m_pDynstat = 0;

#endif

//	if( m_pCached )
//		delete m_pCached;
//	
//	m_pCached = 0;
	
	if( m_pCache )
		delete m_pCache;
		
	m_pCache = 0;

	// Threads
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "~CTracker: destroying the MUTEX announce queue\n" );

	m_mtxQueued.Destroy( );

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "~CTracker: Destructor completed\n" );
		
}

void CTracker :: loadAccess( )
{
	m_ucAccessDownAnnounce = (unsigned char)CFG_GetInt( "bnbt_access_down_announce", ACCESS_DL );
	
	m_ucAccessRSS = (unsigned char)CFG_GetInt( "bnbt_access_rss", ACCESS_VIEW );
	m_ucAccessDumpXML = (unsigned char)CFG_GetInt( "bnbt_access_dump_xml", ACCESS_ADMIN );
	m_ucAccessSortIP = (unsigned char)CFG_GetInt( "bnbt_access_sort_ip", ACCESS_ADMIN );
	
	m_ucAccessShowIP = (unsigned char)CFG_GetInt( "bnbt_access_show_ip", ACCESS_EDIT );
	m_ucAccessComments = (unsigned char)CFG_GetInt( "bnbt_access_comments", ACCESS_COMMENTS );
	m_ucAccessCommentsAlways = (unsigned char)CFG_GetInt( "bnbt_access_comments_always", ACCESS_EDIT );
	m_ucAccessEditComments = (unsigned char)CFG_GetInt( "bnbt_access_edit_comments", ACCESS_EDIT );
	m_ucAccessDelComments = (unsigned char)CFG_GetInt( "bnbt_access_del_comments", ACCESS_EDIT );
	m_ucAccessCommentsToMessage = (unsigned char)CFG_GetInt( "bnbt_access_comments_to_message", ACCESS_UPLOAD );
	
	m_ucAccessView = (unsigned char)CFG_GetInt( "bnbt_access_view", ACCESS_VIEW );
	m_ucAccessViewTorrents = (unsigned char)CFG_GetInt( "bnbt_access_view_torrents", ACCESS_VIEW );
	m_ucAccessDownTorrents = (unsigned char)CFG_GetInt( "bnbt_access_down_torrents", ACCESS_DL );
	m_ucAccessUploadTorrents = (unsigned char)CFG_GetInt( "bnbt_access_upload_torrents", ACCESS_UPLOAD );
	m_ucAccessUploadPosts = (unsigned char)CFG_GetInt( "bnbt_access_upload_posts", ACCESS_ADMIN );
	m_ucAccessReq = (unsigned char)CFG_GetInt( "bnbt_access_req", ACCESS_DL );
	m_ucAccessBookmark = (unsigned char)CFG_GetInt( "bnbt_access_bookmark", ACCESS_DL );
	m_ucAccessEditTorrents = (unsigned char)CFG_GetInt( "bnbt_access_edit_torrents", ACCESS_EDIT );
	m_ucAccessDelTorrents = (unsigned char)CFG_GetInt( "bnbt_access_del_torrents", ACCESS_EDIT );
	m_ucAccessViewOffers = (unsigned char)CFG_GetInt( "bnbt_access_view_offers", ACCESS_VIEW );
	m_ucAccessUploadOffers = (unsigned char)CFG_GetInt( "bnbt_access_upload_offers", ACCESS_COMMENTS );
	m_ucAccessAllowOffers = (unsigned char)CFG_GetInt( "bnbt_access_allow_offers", ACCESS_EDIT );
	m_ucAccessEditOffers = (unsigned char)CFG_GetInt( "bnbt_access_edit_offers", ACCESS_EDIT );
	m_ucAccessDelOffers = (unsigned char)CFG_GetInt( "bnbt_access_del_offers", ACCESS_EDIT );
	
	m_ucAccessViewStats = (unsigned char)CFG_GetInt( "bnbt_access_view_stats", ACCESS_VIEW );
	m_ucAccessEditOwn = (unsigned char)CFG_GetInt( "bnbt_access_edit_own", ACCESS_VIEW );
	m_ucAccessDelOwn = (unsigned char)CFG_GetInt( "bnbt_access_del_own", ACCESS_VIEW );
	
	m_ucAccessCreateUsers = (unsigned char)CFG_GetInt( "bnbt_access_create_users", ACCESS_ADMIN );
	m_ucAccessEditUsers = (unsigned char)CFG_GetInt( "bnbt_access_edit_users", ACCESS_ADMIN );
	m_ucAccessDelUsers = (unsigned char)CFG_GetInt( "bnbt_access_del_users", ACCESS_ADMIN );
	m_ucAccessEditAdmins = (unsigned char)CFG_GetInt( "bnbt_access_edit_admins", ACCESS_LEADER );

	m_ucAccessInvites = (unsigned char)CFG_GetInt( "bnbt_access_invites", ACCESS_VIEW );
	m_ucAccessMessages = (unsigned char)CFG_GetInt( "bnbt_access_messages", ACCESS_VIEW );










	
	m_ucAccessSignup = (unsigned char)CFG_GetInt( "bnbt_access_signup", ACCESS_VIEW );
	m_ucAccessSignupDirect = (unsigned char)CFG_GetInt( "bnbt_access_signup_direct", ACCESS_ADMIN );
	m_ucAccessViewLog = (unsigned char)CFG_GetInt( "bnbt_access_view_log", ACCESS_ADMIN );
	m_ucAccessAdmin = (unsigned char)CFG_GetInt( "bnbt_access_admin", ACCESS_ADMIN );
	m_ucAccessViewUsers = (unsigned char)CFG_GetInt( "bnbt_access_view_users", ACCESS_EDIT );
	m_ucAccessUserDetails = (unsigned char)CFG_GetInt( "bnbt_access_user_detail", ACCESS_EDIT );
	m_ucAccessViewXStates = (unsigned char)CFG_GetInt( "bnbt_access_view_xstates", ACCESS_ADMIN );
}

// Send message
void CTracker :: sendMessage( const string &strLogin, const string &strUID, const string &strSendToID, const string &strIP, const string &strTitle, const string &strMessage, const bool bSaveSent )
{
	CMySQLQuery *pQuery = new CMySQLQuery( "SELECT busername FROM users WHERE buid=" + strSendToID );
	
	vector<string> vecQuery;
			
	vecQuery.reserve(1);

	vecQuery = pQuery->nextRow( );
	
	if( vecQuery.size( ) == 1 )
	{
		string strQuery = "(bsendto,bsendtoid,bfrom,bfromid,bip,bsent,btitle,bmessage,bread) VALUES(\'";
		strQuery += UTIL_StringToMySQL( vecQuery[0] );
		strQuery += "\'," + strSendToID;
		strQuery += ",\'" + UTIL_StringToMySQL( strLogin );
		strQuery += "\'," + strUID;
		strQuery += ",\'" + UTIL_StringToMySQL( strIP );
		strQuery += "\',NOW(),\'" + UTIL_StringToMySQL( strTitle );
		strQuery += "\',\'" + UTIL_StringToMySQL( strMessage );
		strQuery += "\',";
		CMySQLQuery mq01( "INSERT INTO messages " + strQuery + "0)" );
		if( bSaveSent )
			CMySQLQuery mq02( "INSERT INTO messages_sent " + strQuery + "1)" );
	}
	delete pQuery;
}

// Expire downloaders 
void CTracker :: expireDownloaders( )
{
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "expireDownloaders: started\n" );

	CMySQLQuery mq01( "DELETE FROM dstate WHERE bupdated<NOW()-INTERVAL " + CAtomInt( m_uiDownloaderTimeOutInterval ).toString( ) + " SECOND" );
	
	CMySQLQuery mq02( "UPDATE offer SET bseeded=0 WHERE bseeded<NOW()-INTERVAL " + CAtomInt( m_uiDownloaderTimeOutInterval ).toString( ) + " SECOND" );
	
	CMySQLQuery mq03( "UPDATE allowed SET bseeders=0,bseeders6=0,bleechers=0,bleechers6=0" );
		
	CMySQLQuery mq04( "UPDATE allowed,(SELECT bid,COUNT(*) AS bseeders FROM dstate WHERE bleft=0 GROUP BY bid) AS seeders SET allowed.bseeders=seeders.bseeders WHERE allowed.bid=seeders.bid" );
	CMySQLQuery mq05( "UPDATE allowed,(SELECT bid,COUNT(*) AS bseeders6 FROM dstate WHERE bleft=0 AND bip6!='' GROUP BY bid) AS seeders6 SET allowed.bseeders6=seeders6.bseeders6 WHERE allowed.bid=seeders6.bid" );
	CMySQLQuery mq06( "UPDATE allowed,(SELECT bid,COUNT(*) AS bleechers FROM dstate WHERE bleft!=0 GROUP BY bid) AS leechers SET allowed.bleechers=leechers.bleechers WHERE allowed.bid=leechers.bid" );
	CMySQLQuery mq07( "UPDATE allowed,(SELECT bid,COUNT(*) AS bleechers6 FROM dstate WHERE bleft!=0 AND bip6!='' GROUP BY bid) AS leechers6 SET allowed.bleechers6=leechers6.bleechers6 WHERE allowed.bid=leechers6.bid" );
	
	m_pCache->Reset( );
	m_pCache->Reset( true );
	
	UpdateUserState( );
	
	if( m_bCountUniquePeers )
	{
		if( gbDebug )
			if( gucDebugLevel & DEBUG_TRACKER )
				UTIL_LogPrint( "CTracker: Counting the unique peers\n" );

		CountUniquePeers( );
	}

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "expireDownloaders: completed\n" );
}

// Parse the torrent directory
void CTracker :: parseTorrents( const char *szDir )
{
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "parseTorrents: started\n" );

	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	UTIL_LogPrint( "CTracker: Parsing torrents (%s)\n", szDir );
	
	string strDatabase = string( );
						
	if( szDir == m_strAllowedDir )
		strDatabase = "allowed";
	else
		strDatabase = "offer";

	char szFile[1024];
	char pTime[256];

	unsigned int uiParseCount = 0;

	string strFileName = string( );
	string strHash = string( );

	CAtom *pFile = 0;
	CAtom *pInfo = 0;
	CAtom *pName = 0;
	CAtom *pLen = 0;
	CAtom *pFiles = 0;
	CAtom *pSubLength = 0;
	CAtom *pComment = 0;
	CAtomDicti *pInfoDicti = 0;
	CAtomList *pList = 0;

	int64 iSize = 0;

	vector<CAtom *> *pvecFiles;

#ifdef WIN32
	char szMatch[1024];
	memset( szMatch, 0, sizeof( szMatch ) / sizeof( char ) );
	strncpy( szMatch, szDir, sizeof( szMatch ) / sizeof( char ) );
	strncat( szMatch, "*.torrent", ( sizeof( "*.torrent" ) - 1 ) );

	WIN32_FIND_DATA fdt;

	HANDLE hFind = FindFirstFile( szMatch, &fdt );

	FILETIME ft;
	SYSTEMTIME st;

	if( hFind != INVALID_HANDLE_VALUE )
	{
		do
#else
	DIR *pDir = opendir( szDir );

	struct dirent *dp;

	struct stat info;

	if( pDir )
	{
		while( ( dp = readdir( pDir ) ) )
#endif
		{
			// let the server accept new connections while parsing

			if( gpServer )
				gpServer->Update( false );

			memset( szFile, 0, sizeof( szFile ) / sizeof( char ) );
			strncpy( szFile, szDir, sizeof( szFile ) / sizeof( char ) );

#ifdef WIN32
			strFileName = fdt.cFileName;
#else
			strFileName = dp->d_name;
#endif

			strncat( szFile, strFileName.c_str( ), strlen( strFileName.c_str( ) ) );

#ifndef WIN32
			if( strlen( szFile ) > ( sizeof( ".torrent" ) - 1 ) )
			{
				if( strcmp( szFile + strlen( szFile ) - ( sizeof( ".torrent" ) - 1 ), ".torrent" ) )
					continue;
			}
			else
				continue;
#endif

			pFile = DecodeFile( szFile );

			if( pFile && pFile->isDicti( ) )
			{
				pInfo = ( (CAtomDicti *)pFile )->getItem( "info" );

				if( pInfo && pInfo->isDicti( ) )
				{
					pInfoDicti = (CAtomDicti *)pInfo;

					strHash = UTIL_InfoHash( pFile );

					pName = pInfoDicti->getItem( "name" );
					pLen = pInfoDicti->getItem( "length" );
					pFiles = pInfoDicti->getItem( "files" );

					if( pName && ( ( pLen && dynamic_cast<CAtomLong *>( pLen ) ) || ( pFiles && dynamic_cast<CAtomList *>( pFiles ) ) ) )
					{
						//
						// added time (i.e. modification time)
						//

						memset( pTime, 0, sizeof( pTime ) / sizeof( char ) );

#ifdef WIN32
						FileTimeToLocalFileTime( &fdt.ftLastWriteTime, &ft );
						FileTimeToSystemTime( &ft, &st );

						snprintf( pTime, sizeof( pTime ) / sizeof( char ), "%04d-%02d-%02d %02d:%02d:%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond );
#else
						stat( szFile, &info );

						strftime( pTime, sizeof( pTime ) / sizeof( char ), "%Y-%m-%d %H:%M:%S", localtime( &info.st_mtime ) );
#endif

						//
						// file size
						//

						if( !pLen )
						{
							iSize = 0;

							pvecFiles = dynamic_cast<CAtomList *>( pFiles )->getValuePtr( );

							for( vector<CAtom *> :: iterator it2 = pvecFiles->begin( ); it2 != pvecFiles->end( ); it2++ )
							{
								if( (*it2)->isDicti( ) )
								{
									pSubLength = ( (CAtomDicti *)(*it2) )->getItem( "length" );

									if( pSubLength && dynamic_cast<CAtomLong *>( pSubLength ) )
										iSize += dynamic_cast<CAtomLong *>( pSubLength )->getValue( );
								}
							}
						}

						//
						// file comment
						//

						pComment = ( (CAtomDicti *)pFile )->getItem( "comment" );
						string strComment = string( );

						if( pComment )
							strComment = pComment->toString( );
						
						if( pLen )
						{
							CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bid FROM " + strDatabase + " WHERE bhash=\'" + UTIL_StringToMySQL( strHash ) + "\'" );
							if( pQuery->nextRow( ).size( ) == 0 )
							{
								const string cstrQuery2( UTIL_Xsprintf( "INSERT INTO %s (bhash,bfilename,bname,badded,bsize,bfiles,bcomment) VALUES('%s','%s','%s','%s',%lld,1,'%s')", strDatabase.c_str( ), UTIL_StringToMySQL( strHash ).c_str( ), UTIL_StringToMySQL( UTIL_StripPath( szFile ).c_str( ) ).c_str( ), UTIL_StringToMySQL( pName->toString( ).c_str( ) ).c_str( ), pTime, CAtomLong( *dynamic_cast<CAtomLong *>( pLen ) ).getValue( ), UTIL_StringToMySQL( strComment.c_str( ) ).c_str( ) ) );
								
								CMySQLQuery mq01( cstrQuery2 );
//								mysql_real_query( gpMySQL, cstrQuery2.c_str( ), ( unsigned long )cstrQuery2.size( ) );
//								mysql_store_result( gpMySQL );

//								if( mysql_errno( gpMySQL ) )
//									UTIL_LogPrint( "mysql error - %s\n", mysql_error( gpMySQL ) );
							}
							else
							{
								const string cstrQuery2( UTIL_Xsprintf( "UPDATE %s SET bhash='%s',bfilename='%s',bname='%s',badded='%s',bsize=%lld,bfiles=1,bcomment='%s' WHERE bhash='%s'", strDatabase.c_str( ), UTIL_StringToMySQL( strHash ).c_str( ), UTIL_StringToMySQL( UTIL_StripPath( szFile ).c_str( ) ).c_str( ), UTIL_StringToMySQL( pName->toString( ).c_str( ) ).c_str( ), pTime, CAtomLong( *dynamic_cast<CAtomLong *>( pLen ) ).getValue( ), UTIL_StringToMySQL( strComment.c_str( ) ).c_str( ), UTIL_StringToMySQL( strHash ).c_str( ) ) );
								
								CMySQLQuery mq01( cstrQuery2 );
								
//								mysql_real_query( gpMySQL, cstrQuery2.c_str( ), ( unsigned long )cstrQuery2.size( ) );
//								mysql_store_result( gpMySQL );

//								if( mysql_errno( gpMySQL ) )
//									UTIL_LogPrint( "mysql error - %s\n", mysql_error( gpMySQL ) );
							}
							delete pQuery;
						}
						else
						{
							CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bid FROM " + strDatabase + " WHERE bhash=\'" + UTIL_StringToMySQL( strHash ) + "\'" );
							if( pQuery->nextRow( ).size( ) == 0 )
							{
								const string cstrQuery2( UTIL_Xsprintf( "INSERT INTO %s (bhash,bfilename,bname,badded,bsize,bfiles,bcomment) VALUES('%s','%s','%s','%s',%lld,%lu,'%s')", strDatabase.c_str( ), UTIL_StringToMySQL( strHash ).c_str( ), UTIL_StringToMySQL( strFileName.c_str( ) ).c_str( ), UTIL_StringToMySQL( pName->toString( ).c_str( ) ).c_str( ), pTime, iSize, (int)dynamic_cast<CAtomList *>( pFiles )->getValuePtr( )->size( ), UTIL_StringToMySQL( strComment.c_str( ) ).c_str( ) ) );
								
								CMySQLQuery mq01( cstrQuery2 );
//								mysql_real_query( gpMySQL, cstrQuery2.c_str( ), ( unsigned long )cstrQuery2.size( ) );
//								mysql_store_result( gpMySQL );

//								if( mysql_errno( gpMySQL ) )
//									UTIL_LogPrint( "mysql error - %s\n", mysql_error( gpMySQL ) );
							}
							else
							{
								const string cstrQuery2( UTIL_Xsprintf( "UPDATE %s SET bhash='%s',bfilename='%s',bname='%s',badded='%s',bsize=%lld,bfiles=%lu,bcomment='%s' WHERE bhash='%s'", strDatabase.c_str( ), UTIL_StringToMySQL( strHash ).c_str( ), UTIL_StringToMySQL( UTIL_StripPath( szFile ).c_str( ) ).c_str( ), UTIL_StringToMySQL( pName->toString( ).c_str( ) ).c_str( ), pTime, iSize, (int)dynamic_cast<CAtomList *>( pFiles )->getValuePtr( )->size( ), UTIL_StringToMySQL( strComment.c_str( ) ).c_str( ), UTIL_StringToMySQL( strHash ).c_str( ) ) );
								
								CMySQLQuery mq01( cstrQuery2 );
								
//								mysql_real_query( gpMySQL, cstrQuery2.c_str( ), ( unsigned long )cstrQuery2.size( ) );
//								mysql_store_result( gpMySQL );

//								if( mysql_errno( gpMySQL ) )
//									UTIL_LogPrint( "mysql error - %s\n", mysql_error( gpMySQL ) );
							}
							delete pQuery;
						}
					}
					else
					{
						UTIL_LogPrint( string( gmapLANG_CFG["parsing_torrents_infokey_incomplete"] + "\n" ).c_str( ), strFileName.c_str( ) );

						if( m_bDeleteInvalid )
						{
							if( m_strArchiveDir.empty( ) )
								UTIL_DeleteFile( szFile );
							else
								UTIL_MoveFile( szFile, ( m_strArchiveDir + strFileName ).c_str( ) );
						}
					}
				}
				else
				{
					UTIL_LogPrint( string( gmapLANG_CFG["parsing_torrents_infokey_invalid"] + "\n" ).c_str( ), strFileName.c_str( ) );

					if( m_bDeleteInvalid )
					{
						if( m_strArchiveDir.empty( ) )
							UTIL_DeleteFile( szFile );
						else
							UTIL_MoveFile( szFile, string( m_strArchiveDir + strFileName ).c_str( ) );
					}
				}
			}
			else
			{
				UTIL_LogPrint( string( gmapLANG_CFG["parsing_torrents_decode"] + "\n" ).c_str( ), strFileName.c_str( ) );

				if( m_bDeleteInvalid )
				{
					if( m_strArchiveDir.empty( ) )
						UTIL_DeleteFile( szFile );
					else
						UTIL_MoveFile( szFile, string( m_strArchiveDir + strFileName ).c_str( ) );
				}
			}

			delete pFile;

			pFile = 0;

			uiParseCount++;

#ifdef WIN32
		} while( FindNextFile( hFind, &fdt ) );

		FindClose( hFind );
#else
		}
		
		if( strDatabase == "allowed" )
			m_pCache->Reset( );
		if( strDatabase == "offer" )
			m_pCache->Reset( true );	

		closedir( pDir );
#endif
		UTIL_LogPrint( "Tracker Info - Parsed %s torrents in %s seconds\n", CAtomInt( uiParseCount ).toString( ).c_str( ), UTIL_ElapsedTimeStr( btv, UTIL_CurrentTime( ) ).c_str( ) );
	}
	else
		UTIL_LogPrint( string( gmapLANG_CFG["parsing_torrents_opening"] + "\n" ).c_str( ), szDir );

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "parseTorrents: completed\n" );
}

// Parse a single torrent
const string CTracker :: parseTorrent( const char *szFile )
{
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "parseTorrent: started\n" );

#ifdef WIN32
	HANDLE hFile = CreateFile( szFile, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0 );

	if( hFile == INVALID_HANDLE_VALUE )
	{
		UTIL_LogPrint( string( gmapLANG_CFG["parsing_torrents_reading"] + "\n" ).c_str( ), szFile );

		if( gbDebug )
			if( gucDebugLevel & DEBUG_TRACKER )
				UTIL_LogPrint( "parseTorrent: completed\n" );

		return;
	}
#endif

	int64 iSize = 0;
	
	string strReturnID = string( );

	CAtom *pTorrent = DecodeFile( szFile );

	if( pTorrent && pTorrent->isDicti( ) )
	{
		CAtom *pInfo = ( (CAtomDicti *)pTorrent )->getItem( "info" );

		if( pInfo && pInfo->isDicti( ) )
		{
			CAtomDicti *pInfoDicti = (CAtomDicti *)pInfo;

			const string cstrHash( UTIL_InfoHash( pTorrent ) );

			CAtom *pName = pInfoDicti->getItem( "name" );
			CAtom *pLen = pInfoDicti->getItem( "length" );
			CAtom *pFiles = pInfoDicti->getItem( "files" );

			if( pName && ( ( pLen && dynamic_cast<CAtomLong *>( pLen ) ) || ( pFiles && dynamic_cast<CAtomList *>( pFiles ) ) ) )
			{
				//
				// added time (it.e. modification time)
				//

				char pTime[256];
				memset( pTime, 0, sizeof( pTime ) / sizeof( char ) );

#ifdef WIN32
				FILETIME ft;
				SYSTEMTIME st;

				GetFileTime( hFile, 0, 0, &ft );

				FileTimeToLocalFileTime( &ft, &ft );
				FileTimeToSystemTime( &ft, &st );

				snprintf( pTime, sizeof( pTime ) / sizeof( char ), "%04d-%02d-%02d %02d:%02d:%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond );
#else
				struct stat info;

				stat( szFile, &info );

				strftime( pTime, sizeof( pTime ) / sizeof( char ), "%Y-%m-%d %H:%M:%S", localtime( &info.st_mtime ) );
#endif

				//
				// file size
				//

				if( !pLen )
				{
					iSize = 0;

					vector<CAtom *> *pvecFiles = dynamic_cast<CAtomList *>( pFiles )->getValuePtr( );

					CAtom *pSubLength = 0;

					for( vector<CAtom *> :: iterator it2 = pvecFiles->begin( ); it2 != pvecFiles->end( ); it2++ )
					{
						if( (*it2)->isDicti( ) )
						{
							pSubLength = ( (CAtomDicti *)(*it2) )->getItem( "length" );

							if( pSubLength && dynamic_cast<CAtomLong *>( pSubLength ) )
								iSize += dynamic_cast<CAtomLong *>( pSubLength )->getValue( );
						}
					}
				}

				//
				// file comment
				//

				CAtom *pComment = ( (CAtomDicti *)pTorrent )->getItem( "comment" );
				string strComment = string( );

				if( pComment )
					strComment = pComment->toString( );

				const string cstrPath = szFile;
				string :: size_type iFileStart = cstrPath.rfind( CHAR_BS );

				if( iFileStart == string :: npos )
				{
					iFileStart = cstrPath.rfind( CHAR_FS );

					if( iFileStart == string :: npos )
						iFileStart = 0;
					else
						iFileStart++;
				}
				else
					iFileStart++;
				
				string strDatabase = string( );
				if( m_strOfferDir == cstrPath.substr( 0, iFileStart ) )
					strDatabase = "offer";
				else
					strDatabase = "allowed";
				
				string strQuery = string( );
				
				if( pLen )
				{
					CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bid FROM " + strDatabase + " WHERE bhash=\'" + UTIL_StringToMySQL( cstrHash ) + "\'" );

					if( pQuery->nextRow( ).size( ) == 0 )
					{
						strQuery = UTIL_Xsprintf( "INSERT INTO %s (bhash,bfilename,bname,badded,bsize,bfiles,bcomment) VALUES('%s','%s','%s','%s',%lld,1,'%s')", strDatabase.c_str( ), UTIL_StringToMySQL( cstrHash ).c_str( ), UTIL_StringToMySQL( UTIL_StripPath( szFile ).c_str( ) ).c_str( ), UTIL_StringToMySQL( pName->toString( ).c_str( ) ).c_str( ), pTime, CAtomLong( *dynamic_cast<CAtomLong *>( pLen ) ).getValue( ), UTIL_StringToMySQL( strComment.c_str( ) ).c_str( ) );
					}
					else
					{
						strQuery = UTIL_Xsprintf( "UPDATE %s SET bhash='%s',bfilename='%s',bname='%s',badded='%s',bsize=%lld,bfiles=1,bcomment='%s' WHERE bhash='%s'", strDatabase.c_str( ), UTIL_StringToMySQL( cstrHash ).c_str( ), UTIL_StringToMySQL( UTIL_StripPath( szFile ).c_str( ) ).c_str( ), UTIL_StringToMySQL( pName->toString( ).c_str( ) ).c_str( ), pTime, CAtomLong( *dynamic_cast<CAtomLong *>( pLen ) ).getValue( ), UTIL_StringToMySQL( strComment.c_str( ) ).c_str( ), UTIL_StringToMySQL( cstrHash ).c_str( ) );
					}
					delete pQuery;
				}
				else
				{
					CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bid FROM " + strDatabase + " WHERE bhash=\'" + UTIL_StringToMySQL( cstrHash ) + "\'" );
					if( pQuery->nextRow( ).size( ) == 0 )
					{
						strQuery = UTIL_Xsprintf( "INSERT INTO %s (bhash,bfilename,bname,badded,bsize,bfiles,bcomment) VALUES('%s','%s','%s','%s',%lld,%lu,'%s')", strDatabase.c_str( ), UTIL_StringToMySQL( cstrHash ).c_str( ), UTIL_StringToMySQL( UTIL_StripPath( szFile ).c_str( ) ).c_str( ), UTIL_StringToMySQL( pName->toString( ).c_str( ) ).c_str( ), pTime, iSize, (int)dynamic_cast<CAtomList *>( pFiles )->getValuePtr( )->size( ), UTIL_StringToMySQL( strComment.c_str( ) ).c_str( ) );
					}
					else
					{
						strQuery = UTIL_Xsprintf( "UPDATE %s SET bhash='%s',bfilename='%s',bname='%s',badded='%s',bsize=%lld,bfiles=%lu,bcomment='%s' WHERE bhash='%s'", strDatabase.c_str( ), UTIL_StringToMySQL( cstrHash ).c_str( ), UTIL_StringToMySQL( UTIL_StripPath( szFile ).c_str( ) ).c_str( ), UTIL_StringToMySQL( pName->toString( ).c_str( ) ).c_str( ), pTime, iSize, (int)dynamic_cast<CAtomList *>( pFiles )->getValuePtr( )->size( ), UTIL_StringToMySQL( strComment.c_str( ) ).c_str( ), UTIL_StringToMySQL( cstrHash ).c_str( ) );
					}
					delete pQuery;
				}
				
				CMySQLQuery *pQuery = new CMySQLQuery( strQuery );
				
				unsigned long ulLast = pQuery->lastInsertID( );
//				CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bid FROM " + strDatabase + " WHERE bhash=\'" + UTIL_StringToMySQL( cstrHash ) + "\'" );
//	
//				vector<string> vecQuery;

//				vecQuery.reserve(1);
//	
//				vecQuery = pQuery->nextRow( );
	
				delete pQuery;
				
				if( ulLast > 0 )
					strReturnID = CAtomLong( ulLast ).toString( );
	
//				if( vecQuery.size( ) == 1 && !vecQuery[0].empty( ) )
//					strReturnID = vecQuery[0];
			}
			else
			{
				UTIL_LogPrint( string( gmapLANG_CFG["parsing_torrents_infokey_incomplete"] + "\n" ).c_str( ), szFile );

				if( m_bDeleteInvalid )
				{
					if( m_strArchiveDir.empty( ) )
						UTIL_DeleteFile( szFile );
					else
						UTIL_MoveFile( szFile, string( m_strArchiveDir + UTIL_StripPath( szFile ) ).c_str( ) );
				}
			}
		}
		else
		{
			UTIL_LogPrint( string( gmapLANG_CFG["parsing_torrents_infokey_invalid"] + "\n" ).c_str( ), szFile );

			if( m_bDeleteInvalid )
			{
				if( m_strArchiveDir.empty( ) )
					UTIL_DeleteFile( szFile );
				else
					UTIL_MoveFile( szFile, string( m_strArchiveDir + UTIL_StripPath( szFile ) ).c_str( ) );
			}
		}
	}
	else
	{
		UTIL_LogPrint( string( gmapLANG_CFG["parsing_torrents_decode"] + "\n" ).c_str( ), szFile );

		if( m_bDeleteInvalid )
		{
			if( m_strArchiveDir.empty( ) )
				UTIL_DeleteFile( szFile );
			else
				UTIL_MoveFile( szFile, string( m_strArchiveDir + UTIL_StripPath( szFile ) ).c_str( ) );
		}
	}

	delete pTorrent;

	pTorrent = 0;

#ifdef WIN32
	CloseHandle( hFile );
#endif

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "parseTorrent: completed\n" );
			
	return strReturnID;
}

// Check a torrent associated entry
const bool CTracker :: checkTag( const string &strTag )
{
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "checkTag: checking\n" );

	if( m_vecTags.empty( ) )
		return true;

	string strNameIndex = string( );
	for( vector< pair< string, string > > :: iterator it = m_vecTags.begin( ); it != m_vecTags.end( ); it++ )
	{
		strNameIndex = (*it).first;

		if( strNameIndex == strTag )
		{
			if( gbDebug )
				if( gucDebugLevel & DEBUG_TRACKER )
					UTIL_LogPrint( "checkTag: passed\n" );

			return true;
		}

	}

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "checkTag: failed\n" );

	return false;
}

// Add a torrent associated entry
const string CTracker :: addTag( const string &strInfoHash, const string &strTag, const string &strName, const string & strIntr, const string &strUploader, const string &strUploaderID, const string &strIP, const string &strDefaultDown, const string &strDefaultUp, const string &strFreeDown, const string &strFreeUp, const string &strFreeTime, const string &strComments, const bool bFromNow, const bool bOffer )
{
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "addTag: started\n" );
	
	string strDatabase = string( );
	if( bOffer )
		strDatabase = "offer";
	else
		strDatabase = "allowed";
	
	CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bid FROM " + strDatabase + " WHERE bhash=\'" + UTIL_StringToMySQL( strInfoHash ) + "\'" );
	
	if( pQuery->nextRow( ).size( ) == 0 )
	{
		string strQuery = "INSERT INTO " + strDatabase + " (bhash,btag";
		string strQueryValues = " VALUES(\'" + UTIL_StringToMySQL( strInfoHash ) + "\',\'" + UTIL_StringToMySQL( strTag ) + "\'";
		if( !strName.empty( ) )
		{
			strQuery += ",btitle";
			strQueryValues += ",\'" + UTIL_StringToMySQL( strName ) + "\'";
		}
		if( ( !strIntr.empty( ) ) )
		{
			strQuery += ",bintr";
			strQueryValues += ",\'" + UTIL_StringToMySQL( strIntr ) + "\'";
		}
// 		if( ( !strUploader.empty( ) ) && ( strUploader!="REMOVE" ) )
		if( !strUploader.empty( ) )
		{
			strQuery += ",buploader";
			strQueryValues += ",\'" + UTIL_StringToMySQL( strUploader ) + "\'";
		}
		if( !strUploaderID.empty( ) )
		{
			strQuery += ",buploaderid";
			strQueryValues += "," + UTIL_StringToMySQL( strUploaderID );
		}
		if( !strIP.empty( ) )
		{
			strQuery += ",bip";
			strQueryValues += ",\'" + strIP + "\'";
		}
		if( !strComments.empty( ) )
		{
			strQuery += ",bcomments";
			strQueryValues += "," + strComments;
		}
		if( !bOffer )
		{
			if( !strDefaultDown.empty( ) && UTIL_StringTo64( strDefaultDown.c_str( ) ) >= 0 )
			{
				strQuery += ",bdefault_down";
				strQueryValues += "," + strDefaultDown;
			}
			if( !strDefaultUp.empty( ) && UTIL_StringTo64( strDefaultUp.c_str( ) ) >= 0 )
			{
				strQuery += ",bdefault_up";
				strQueryValues += "," + strDefaultUp;
			}
			if( !strFreeDown.empty( ) && UTIL_StringTo64( strFreeDown.c_str( ) ) >= 0 )
			{
				strQuery += ",bfree_down";
				strQueryValues += "," + strFreeDown;
			}
			if( !strFreeUp.empty( ) && UTIL_StringTo64( strFreeUp.c_str( ) ) >= 0 )
			{
				strQuery += ",bfree_up";
				strQueryValues += "," + strFreeUp;
			}
			if( !strFreeTime.empty( ) && UTIL_StringTo64( strFreeTime.c_str( ) ) >= 0 )
			{
				if( !bFromNow )
				{
					strQuery += ",bfree_time";
					strQueryValues += "," + strFreeTime;
					if( UTIL_StringTo64( strFreeTime.c_str( ) ) > 0 )
					{
						strQuery += ",bfree_to";
						strQueryValues += ",badded+INTERVAL " + strFreeTime + " HOUR";
					}
					else
					{
						strQuery += ",bfree_to";
						strQueryValues += ",0";
					}
				}
				else
				{
					if( UTIL_StringTo64( strFreeTime.c_str( ) ) > 0 )
					{
						strQuery += ",bfree_to";
						strQueryValues += ",NOW()+INTERVAL " + strFreeTime + " HOUR";
					}
					else
					{
						strQuery += ",bfree_to";
						strQueryValues += ",0";
					}
				}
			}
		}
		strQuery += ")";
		strQueryValues += ")";
		
		CMySQLQuery mq01( strQuery + strQueryValues );
	}
	else
	{
		string strQuery = "UPDATE " + strDatabase + " SET btag=\'" + UTIL_StringToMySQL( strTag ) + "\'";

		if( !strName.empty( ) )
		{
			strQuery += ", btitle=\'" + UTIL_StringToMySQL( strName ) + "\'";
		}
		if( !strIntr.empty( ) )
		{
			strQuery += ", bintr=\'" + UTIL_StringToMySQL( strIntr ) + "\'";
		}
// 		if( ( !strUploader.empty( ) ) && ( strUploader!="REMOVE" ) )
		if( !strUploader.empty( ) )
		{
			strQuery += ", buploader=\'" + UTIL_StringToMySQL( strUploader ) + "\'";
		}
		if( !strUploaderID.empty( ) )
		{
			strQuery += ", buploaderid=" + UTIL_StringToMySQL( strUploaderID );
		}
		if( !strIP.empty( ) )
		{
			strQuery += ", bip=\'" + UTIL_StringToMySQL( strIP ) + "\'";
		}
		if( !strComments.empty( ) )
		{
			strQuery += ", bcomments=" + strComments;
		}
		if( !bOffer )
		{
			if( !strDefaultDown.empty( ) && UTIL_StringTo64( strDefaultDown.c_str( ) ) >= 0 )
			{
				strQuery += ", bdefault_down=" + strDefaultDown;
			}
			if( !strDefaultUp.empty( ) && UTIL_StringTo64( strDefaultUp.c_str( ) ) >= 0 )
			{
				strQuery += ", bdefault_up=" + strDefaultUp;
			}
			if( !strFreeDown.empty( ) && UTIL_StringTo64( strFreeDown.c_str( ) ) >= 0 )
			{
				strQuery += ", bfree_down=" + strFreeDown;
			}
			if( !strFreeUp.empty( ) && UTIL_StringTo64( strFreeUp.c_str( ) ) >= 0 )
			{
				strQuery += ", bfree_up=" + strFreeUp;
			}
			if( !strFreeTime.empty( ) && UTIL_StringTo64( strFreeTime.c_str( ) ) >= 0 )
			{
				if( !bFromNow )
				{
					strQuery += ", bfree_time=" + strFreeTime;
					if( UTIL_StringTo64( strFreeTime.c_str( ) ) > 0 )
						strQuery += ", bfree_to=badded+INTERVAL " + strFreeTime + " HOUR";
					else
						strQuery += ", bfree_to=0";
				}
				else
				{
					if( UTIL_StringTo64( strFreeTime.c_str( ) ) > 0 )
						strQuery += ", bfree_to=NOW()+INTERVAL " + strFreeTime + " HOUR";
					else
						strQuery += ", bfree_to=0";
				}
			}
		}
		strQuery += " WHERE bhash=\'" + UTIL_StringToMySQL( strInfoHash ) + "\'";
		CMySQLQuery mq01( strQuery );
	}
		
	delete pQuery;
	
	pQuery = new CMySQLQuery( "SELECT bid FROM " + strDatabase + " WHERE bhash=\'" + UTIL_StringToMySQL( strInfoHash ) + "\'" );
	
	vector<string> vecQuery;

	vecQuery.reserve(1);
	
	vecQuery = pQuery->nextRow( );
	
	delete pQuery;
	
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "addTag: completed\n" );
	
	if( vecQuery.size( ) == 1 && !vecQuery[0].empty( ) )
		return vecQuery[0];

	return string( );
}

void CTracker :: modifyTag( const string &strID, const string &strTag, const string &strName, const string & strIntr, const string &strUploader, const string &strUploaderID, const string &strIP, const string &strDefaultDown, const string &strDefaultUp, const string &strFreeDown, const string &strFreeUp, const string &strFreeTime, const string &strComments, const bool bFromNow, const bool bOffer )
{
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "modifyTag: started\n" );
	
	string strDatabase = string( );
	if( bOffer )
		strDatabase = "offer";
	else
		strDatabase = "allowed";
	
	CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bid FROM " + strDatabase + " WHERE bid=" + strID );
	
	if( pQuery->nextRow( ).size( ) == 1 )
	{
		string strQuery = "UPDATE " + strDatabase + " SET btag=\'" + UTIL_StringToMySQL( strTag ) + "\'";

		if( !strName.empty( ) )
		{
			strQuery += ", btitle=\'" + UTIL_StringToMySQL( strName ) + "\'";
		}
		if( !strIntr.empty( ) )
		{
			strQuery += ", bintr=\'" + UTIL_StringToMySQL( strIntr ) + "\'";
		}
// 		if( ( !strUploader.empty( ) ) && ( strUploader!="REMOVE" ) )
		if( !strUploader.empty( ) )
		{
			strQuery += ", buploader=\'" + UTIL_StringToMySQL( strUploader ) + "\'";
		}
		if( !strUploaderID.empty( ) )
		{
			strQuery += ", buploaderid=" + UTIL_StringToMySQL( strUploaderID );
		}
		if( !strIP.empty( ) )
		{
			strQuery += ", bip=\'" + UTIL_StringToMySQL( strIP ) + "\'";
		}
		if( !strComments.empty( ) )
		{
			strQuery += ", bcomments=" + strComments;
		}
		if( !bOffer )
		{
			if( !strDefaultDown.empty( ) && UTIL_StringTo64( strDefaultDown.c_str( ) ) >= 0 )
			{
				strQuery += ", bdefault_down=" + strDefaultDown;
			}
			if( !strDefaultUp.empty( ) && UTIL_StringTo64( strDefaultUp.c_str( ) ) >= 0 )
			{
				strQuery += ", bdefault_up=" + strDefaultUp;
			}
			if( !strFreeDown.empty( ) && UTIL_StringTo64( strFreeDown.c_str( ) ) >= 0 )
			{
				strQuery += ", bfree_down=" + strFreeDown;
			}
			if( !strFreeUp.empty( ) && UTIL_StringTo64( strFreeUp.c_str( ) ) >= 0 )
			{
				strQuery += ", bfree_up=" + strFreeUp;
			}
			if( !strFreeTime.empty( ) && UTIL_StringTo64( strFreeTime.c_str( ) ) >= 0 )
			{
				if( !bFromNow )
				{
					strQuery += ", bfree_time=" + strFreeTime;
					if( UTIL_StringTo64( strFreeTime.c_str( ) ) > 0 )
						strQuery += ", bfree_to=badded+INTERVAL " + strFreeTime + " HOUR";
					else
						strQuery += ", bfree_to=0";
				}
				else
				{
					if( UTIL_StringTo64( strFreeTime.c_str( ) ) > 0 )
						strQuery += ", bfree_to=NOW()+INTERVAL " + strFreeTime + " HOUR";
					else
						strQuery += ", bfree_to=0";
				}
			}
		}
		strQuery += " WHERE bid=" + strID;
		CMySQLQuery mq01( strQuery );
	}
		
	delete pQuery;

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "modifyTag: completed\n" );
}

// Delete other torrent associated entry
void CTracker :: deleteTag( const string &strID, const bool bOffer )
{
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "deleteTag: started\n" );
		
	string strDatabase = string( );
	string strIDKey = string( );
	if( bOffer )
	{
		strDatabase = "offer";
		strIDKey = "boid";
	}
	else
	{
		strDatabase = "allowed";
		strIDKey = "btid";
	}
		
	CMySQLQuery mq01( "DELETE FROM " + strDatabase + " WHERE bid=" + strID );
	
	m_pCache->Reset( bOffer );
	
	if( !bOffer )
	{
		CMySQLQuery mq02( "DELETE FROM dstate WHERE bid=" + strID );
		CMySQLQuery mq03( "DELETE FROM peers WHERE bid=" + strID );
		CMySQLQuery mq04( "DELETE FROM statistics WHERE bid=" + strID );
		CMySQLQuery mq05( "DELETE FROM bookmarks WHERE bid=" + strID );
		CMySQLQuery mq06( "DELETE FROM thanks WHERE bid=" + strID );
		CMySQLQuery mq07( "DELETE FROM talktorrent WHERE btid=" + strID );
	}
	
	CMySQLQuery mq07( "DELETE FROM comments WHERE " + strIDKey + "=" + strID );
	
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "deleteTag: completed\n" );
}

// Add a comp_state_t entry
void CTracker :: addBonus( const string &strID, const string &strUID )
{
	
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "addBonus: started\n" );
	
	CMySQLQuery *pQuery = 0;
	
	pQuery = new CMySQLQuery( "SELECT buid FROM peers WHERE bid=" + strID + " AND buid=" + strUID + " AND bcompleted!=0" );

	if( pQuery->nextRow( ).size( ) == 0 )
	{
		CMySQLQuery *pQueryTorrent = new CMySQLQuery( "SELECT bsize,buploaderid,bcompleted FROM allowed WHERE bid=" + strID );
					
		vector<string> vecQueryTorrent;
		
		vecQueryTorrent.reserve(3);

		vecQueryTorrent = pQueryTorrent->nextRow( );
		
		int64 iSize = 0;
		
		if( vecQueryTorrent.size( ) == 3 )
		{
			const int iBonusSizeUnit = CFG_GetInt( "bnbt_uploader_bonus_size_unit_mb", 10 );
			const int iQuota = CFG_GetInt( "bnbt_uploader_bonus_quota", 10 );
			if( !vecQueryTorrent[0].empty( ) )
				iSize = UTIL_StringTo64( vecQueryTorrent[0].c_str( ) );

			iSize = iSize / 1024 / 1024 / iBonusSizeUnit;
			if( iSize == 0 )
				iSize = 1;
			
			if( !vecQueryTorrent[1].empty( ) && vecQueryTorrent[2] == "0" )
			{
				CMySQLQuery *pQueryUser = new CMySQLQuery( "SELECT buid FROM users WHERE buid=" + vecQueryTorrent[1] );
				
				if( pQueryUser->nextRow( ).size( ) == 1 )
					CMySQLQuery mq01( "UPDATE users SET bbonus=bbonus+" + CAtomLong( iSize * iQuota ).toString( ) + " WHERE buid=" + vecQueryTorrent[1] );
				
				delete pQueryUser;
			}
		}

		delete pQueryTorrent;

		CMySQLQuery *pQueryUser = new CMySQLQuery( "SELECT buid FROM users WHERE buid=" + strUID );
		
		if( pQueryUser->nextRow( ).size( ) == 1 )
			CMySQLQuery mq01( "UPDATE users SET bbonus=bbonus+" + CAtomLong( iSize ).toString( ) + " WHERE buid=" + strUID );
		
		delete pQueryUser;
	}
	
	delete pQuery;

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "addBonus: completed\n" );
}

// Check the users login and password
const string CTracker :: checkUserMD5( const string &strLogin, const string &cstrMD5 )
{
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "checkUserMD5: started\n" );
			
	string strMD5 = cstrMD5;
	
	CMySQLQuery *pQuery = new CMySQLQuery( "SELECT buid,bmd5 FROM users WHERE busername=\'" + UTIL_StringToMySQL( strLogin ) + "\'" );
	
	vector<string> vecQuery;
	
	vecQuery.reserve(2);

	vecQuery = pQuery->nextRow( );
	
	delete pQuery;
	
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "checkUserMD5: completed\n" );
	
	if( vecQuery.size( ) == 2 && !vecQuery[0].empty( ) )
	{
		if( strMD5 == vecQuery[1] )
			return vecQuery[0];
	}
		
	return string( );
}

// Check the users login and password
user_t CTracker :: checkUser( const string &strUID, const string &cstrMD5 )
{
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "checkUser: started\n" );

	user_t user;

	user.ucAccess = m_ucGuestAccess;

	string strMD5 = cstrMD5;
	
	if( !strUID.empty( ) )
	{	
		CMySQLQuery *pQuery = new CMySQLQuery( "SELECT buid,busername,bmd5,baccess,bgroup FROM users WHERE buid=" + strUID );
	
		vector<string> vecQuery;
	
		vecQuery.reserve(5);

		vecQuery = pQuery->nextRow( );
	
		delete pQuery;

//	 	// if no users exist, grant full access
//	 	if( vecQuery.size( ) == 0 )
//	 	{
//	 		if( gbDebug )
//	 			if( gucDebugLevel & DEBUG_TRACKER )
//	 				UTIL_LogPrint( "checkUser: no users, full permission granted\n" );
//	 
//	 		user.ucAccess = (unsigned char)~0;
//	 	}
			
		if( vecQuery.size( ) == 5 )
		{
			// check hash

			if( strMD5 == vecQuery[2] )
			{
//				if( vecQuery[16] == "0" )
//				{
//					user.tLast_Index = GetStartTime( );
//					CMySQLQuery mq01( "UPDATE users SET blast_index=FROM_UNIXTIME(" + CAtomLong( GetStartTime( ) ).toString( ) + ") WHERE busername=\'" + UTIL_StringToMySQL( strLogin ) + "\'" );
//				}
//				else
//					user.tLast_Index = UTIL_StringTo64( vecQuery[16].c_str( ) );
//				user.tLast = GetTime( );
//				m_pCache->setLast( strUID );
				CMySQLQuery mq02( "UPDATE users SET blast=NOW() WHERE buid=" + strUID );
				user.strUID = vecQuery[0];
				user.strLogin = vecQuery[1];
//				user.strLowerLogin = UTIL_ToLower( vecQuery[1] );
//				user.strCreated = vecQuery[3];
//				user.strPasskey = vecQuery[3];
				user.strMD5 = strMD5;
//				user.strMail = vecQuery[4];
//				user.strLowerMail = UTIL_ToLower( vecQuery[4] );
				user.ucAccess = (unsigned char)atoi( vecQuery[3].c_str( ) );
				user.ucGroup = (unsigned char)atoi( vecQuery[4].c_str( ) );
//				user.strTitle = vecQuery[8];
				
//				user.ulUploaded = UTIL_StringTo64( vecQuery[6].c_str( ) );
//				user.ulDownloaded = UTIL_StringTo64( vecQuery[7].c_str( ) );
//				user.ulBonus = UTIL_StringTo64( vecQuery[8].c_str( ) );
//				user.strIP = vecQuery[9];
//				user.ulSeeding = UTIL_StringTo64( vecQuery[13].c_str( ) );
//				user.ulLeeching = UTIL_StringTo64( vecQuery[14].c_str( ) );
//				user.strSeeding = vecQuery[13];
//				user.strLeeching = vecQuery[14];
//				user.flSeedBonus = atof( vecQuery[10].c_str( ) );
				
//				user.tLast_Index = UTIL_StringTo64( vecQuery[10].c_str( ) );
//				user.tLast_Info = UTIL_StringTo64( vecQuery[11].c_str( ) );
//				user.tWarned = UTIL_StringTo64( vecQuery[12].c_str( ) );
//				user.strInvites = vecQuery[10];
//				user.strInviter = vecQuery[18];
//				user.strInviterID = vecQuery[19];
//				user.strTalk = vecQuery[22].c_str( );
//				user.strTalkRef = vecQuery[23].c_str( );
			
//				if( user.ulDownloaded == 0 )
//				{
//					if( user.ulUploaded == 0 )
//						user.flShareRatio = 0;
//					else
//						user.flShareRatio = -1;
//				}
//				else
//					user.flShareRatio = (float)user.ulUploaded / (float)user.ulDownloaded;
			}
		}
	}

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "checkUser: completed\n" );

	return user;
}

user_t CTracker :: getUser( const string &strUID, const string &strMyUID, const unsigned char ucAccess )
{
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "getUser: started\n" );

	user_t user;
	
	user.strUID.erase( );
	user.strLogin.erase( );
	user.strLowerLogin.erase( );
	user.strMD5.erase( );
	user.strMail.erase( );
	user.strLowerMail.erase( );
	user.strCreated.erase( );
	user.strPasskey.erase( );
	user.ucAccess = 0;
	user.ucGroup = 0;
	user.strTitle.erase( );
	user.ulUploaded = 0;
	user.ulDownloaded = 0;
	user.ulBonus = 0;
	user.strIP.erase( );
//	user.ulSeeding = 0;
//	user.ulLeeching = 0;
//	user.strSeeding.erase( );
//	user.strLeeching.erase( );
	user.flSeedBonus = 0;
	user.flShareRatio = 0;
	user.tWarned = 0;
	user.tLast = 0;
//	user.tLast_Index = 0;
//	user.tLast_Info = 0;
	user.strInvites.erase( );
	user.strInviter.erase( );
	user.strInviterID.erase( );
//	user.strTalk.erase( );
//	user.strTalkRef.erase( );
	
	CMySQLQuery *pQuery = new CMySQLQuery( "SELECT buid,busername,bmd5,bcreated,bpasskey,bemail,baccess,bgroup,btitle,buploaded,bdownloaded,bbonus,bip,bseedbonus,UNIX_TIMESTAMP(blast),UNIX_TIMESTAMP(bwarned),binvites,binviter,binviterid FROM users WHERE buid=" + strUID );
	
	vector<string> vecQuery;
	
	vecQuery.reserve(19);

	vecQuery = pQuery->nextRow( );
	
	delete pQuery;

	if( vecQuery.size( ) == 19 )
	{
		user.strUID = vecQuery[0];
		user.strLogin = vecQuery[1];
		user.strLowerLogin = UTIL_ToLower( vecQuery[1] );
//		if( !vecQuery[3].empty( ) )
			user.strCreated = vecQuery[3];
//		if( !vecQuery[6].empty( ) )
			user.ucAccess = (unsigned char)atoi( vecQuery[6].c_str( ) );
//		if( !vecQuery[7].empty( ) )
			user.ucGroup = (unsigned char)atoi( vecQuery[7].c_str( ) );
//		if( !vecQuery[8].empty( ) )
			user.strTitle = vecQuery[8];
//		if( !vecQuery[9].empty( ) )
			user.ulUploaded = UTIL_StringTo64( vecQuery[9].c_str( ) );
//		if( !vecQuery[10].empty( ) )
			user.ulDownloaded = UTIL_StringTo64( vecQuery[10].c_str( ) );
//		if( !vecQuery[11].empty( ) )
			user.ulBonus = UTIL_StringTo64( vecQuery[11].c_str( ) );
//		if( !vecQuery[13].empty( ) )
//			user.ulSeeding = UTIL_StringTo64( vecQuery[13].c_str( ) );
//			user.strSeeding = vecQuery[13];
//		if( !vecQuery[14].empty( ) )
//			user.ulLeeching = UTIL_StringTo64( vecQuery[14].c_str( ) );
//			user.strLeeching = vecQuery[14];
//		if( !vecQuery[16].empty( ) )
			user.tLast = UTIL_StringTo64( vecQuery[14].c_str( ) );
//		if( !vecQuery[17].empty( ) )
			user.tWarned = UTIL_StringTo64( vecQuery[15].c_str( ) );

		if( user.ulDownloaded == 0 )
		{
			if( user.ulUploaded == 0 )
				user.flShareRatio = 0;
			else
				user.flShareRatio = -1;
		}
		else
			user.flShareRatio = (float)user.ulUploaded / (float)user.ulDownloaded;
		
		if( ( ucAccess & m_ucAccessUserDetails ) || strMyUID == vecQuery[0] )
		{
//			if( !vecQuery[15].empty( ) )
				user.flSeedBonus = atof( vecQuery[13].c_str( ) );
		}
		else
			user.flSeedBonus = -1;
			
		if( ( ucAccess & m_ucAccessUserDetails ) || strMyUID == vecQuery[0] )
		{
			user.strMD5 = vecQuery[2];
			user.strPasskey = vecQuery[4];
			user.strMail = vecQuery[5];
			user.strLowerMail = UTIL_ToLower( vecQuery[5] );
			user.strIP = vecQuery[12];
			user.strInvites = vecQuery[16];
			user.strInviter = vecQuery[17];
			user.strInviterID = vecQuery[18];
		}
	}
	
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "getUser: completed\n" );

	return user;
}

const string CTracker :: getUserLogin( const string &strLogin )
{
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "getUserLogin: started\n" );
	
	CMySQLQuery *pQuery = new CMySQLQuery( "SELECT busername FROM users WHERE busername=\'" + UTIL_StringToMySQL( strLogin ) + "\'" );
	
	vector<string> vecQuery;
	
	vecQuery.reserve(1);

	vecQuery = pQuery->nextRow( );
	
	delete pQuery;

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "getUserLogin: completed\n" );
			
	if( vecQuery.size( ) == 1 && !vecQuery[0].empty( ) )
		return vecQuery[0];
		
	return string( );
}

//const bool CTracker :: checkWarned( const string &strUID )
//{
//	if( gbDebug )
//		if( gucDebugLevel & DEBUG_TRACKER )
//			UTIL_LogPrint( "checkWarned: started\n" );
//		
//	CMySQLQuery *pQuery = new CMySQLQuery( "SELECT UNIX_TIMESTAMP(bwarned) FROM users WHERE buid=" + strUID );
//	
//	vector<string> vecQuery;
//	
//	vecQuery.reserve(1);

//	vecQuery = pQuery->nextRow( );
//	
//	delete pQuery;

//	if( vecQuery.size( ) == 1 )
//	{
//		if( vecQuery[0] != "0" )
//			return true;
//		else
//			return false;
//	}
//	else
//		return false;

//}

const bool CTracker :: checkShareRatio( int64 iDownloaded, float flShareRatio )
{
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "checkShareRatio: started\n" );
			
	for( int i = 0; i < 6; i++ )
		if( iDownloaded / 1024 / 1024 / 1024 >= RequiredDown[i] )
		{
			if( flShareRatio < RequiredRatio[i] )
				return true;
			else
				return false;
		}
	
	return false;
}

const string CTracker :: getUserLink( const string &strUID, const string &strUsername )
{
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "getUserLink: started\n" );
	
	string strUserLink = string( );

	CMySQLQuery *pQuery = new CMySQLQuery( "SELECT busername,baccess,bgroup,buploaded,bdownloaded,UNIX_TIMESTAMP(bwarned) FROM users WHERE buid=" + strUID );

	vector<string> vecQuery;
	
	vecQuery.reserve(6);

	vecQuery = pQuery->nextRow( );
	
	delete pQuery;
	
	if( vecQuery.size( ) == 6 )
	{
		unsigned char ucAccess = (unsigned char)atoi( vecQuery[1].c_str( ) );
		unsigned char ucGroup = (unsigned char)atoi( vecQuery[2].c_str( ) );
		int64 iUploaded = 0, iDownloaded = 0;
		float flShareRatio = 0;
		
		iUploaded = UTIL_StringTo64( vecQuery[3].c_str( ) );
		iDownloaded = UTIL_StringTo64( vecQuery[4].c_str( ) );

		if( iDownloaded == 0 )
		{
			if( iUploaded == 0 )
				flShareRatio = 0;
			else
				flShareRatio = -1;
		}
		else
			flShareRatio = (float)iUploaded / (float)iDownloaded;
		
		bool bShareRatioWarned = checkShareRatio( iDownloaded, flShareRatio );
		string strClass = UTIL_UserClass( ucAccess, ucGroup );
		
		strUserLink += "<a class=\"";
		
		if( m_bRatioRestrict && bShareRatioWarned && strClass == gmapLANG_CFG["class_member"] )
			strUserLink += "share_warned";
		else
			strUserLink += strClass;
		
		strUserLink += "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "?uid=" + strUID + "\">";
		
		if( !strUsername.empty( ) )
			strUserLink += UTIL_RemoveHTML( strUsername );
		else
			strUserLink += vecQuery[0];
		strUserLink += "</a>";

		if( !( ucAccess & ACCESS_VIEW ) )
			strUserLink += "<img title=\"" + gmapLANG_CFG["user_banned"] + "\" src=\"files/warned1.gif\">";
		if( m_bRatioRestrict && bShareRatioWarned )
			strUserLink += "<img title=\"" + gmapLANG_CFG["user_shareratio_warned"] + "\" src=\"files/warned3.gif\">";
		if( vecQuery[5] != "0" )
			strUserLink += "<img title=\"" + gmapLANG_CFG["user_warned"] + "\" src=\"files/warned.gif\">";
	}
	else
	{
		if( !strUsername.empty( ) )
			strUserLink += "<a class=\"member\" href=\"" + RESPONSE_STR_LOGIN_HTML + "?uid=" + strUID + "\">" + UTIL_RemoveHTML( strUsername ) + "</a>";
	}
	
	return strUserLink;
}

const string CTracker :: getUserLinkFull( const string &strUID, const string &strUsername )
{
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "getUserLink: started\n" );
			
	if( strUsername.empty( ) )
		return gmapLANG_CFG["unknown"];
	else if( strUID.empty( ) )
		return UTIL_RemoveHTML( strUsername ) + " (" + gmapLANG_CFG["unknown"] + ")";
	
	string strUserLink = string( );

	CMySQLQuery *pQuery = new CMySQLQuery( "SELECT baccess,bgroup,btitle,buploaded,bdownloaded,UNIX_TIMESTAMP(bwarned) FROM users WHERE buid=" + strUID );

	vector<string> vecQuery;
	
	vecQuery.reserve(6);

	vecQuery = pQuery->nextRow( );
	
	delete pQuery;
	
	if( vecQuery.size( ) == 6 )
	{
		unsigned char ucAccess = (unsigned char)atoi( vecQuery[0].c_str( ) );
		unsigned char ucGroup = (unsigned char)atoi( vecQuery[1].c_str( ) );
		int64 iUploaded = 0, iDownloaded = 0;
		float flShareRatio = 0;
		
		iUploaded = UTIL_StringTo64( vecQuery[3].c_str( ) );
		iDownloaded = UTIL_StringTo64( vecQuery[4].c_str( ) );

		if( iDownloaded == 0 )
		{
			if( iUploaded == 0 )
				flShareRatio = 0;
			else
				flShareRatio = -1;
		}
		else
			flShareRatio = (float)iUploaded / (float)iDownloaded;
		
		bool bShareRatioWarned = checkShareRatio( iDownloaded, flShareRatio );
		string strClass = UTIL_UserClass( ucAccess, ucGroup );
		string strAccess = string( );
		
		strUserLink += "<a class=\"";
		
		if( m_bRatioRestrict && bShareRatioWarned && strClass == gmapLANG_CFG["class_member"] )
			strClass = "share_warned";
		
		strUserLink += strClass;
		
		strUserLink += "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "?uid=" + strUID + "\">" + UTIL_RemoveHTML( strUsername ) + "</a>";

		if( !( ucAccess & ACCESS_VIEW ) )
			strUserLink += "<img title=\"" + gmapLANG_CFG["user_banned"] + "\" src=\"files/warned1.gif\">";
		if( m_bRatioRestrict && bShareRatioWarned )
			strUserLink += "<img title=\"" + gmapLANG_CFG["user_shareratio_warned"] + "\" src=\"files/warned3.gif\">";
		if( vecQuery[5] != "0" )
			strUserLink += "<img title=\"" + gmapLANG_CFG["user_warned"] + "\" src=\"files/warned.gif\">";

		if( vecQuery[1] == "0" )
			strAccess = UTIL_AccessToString( ucAccess );
		else
			strAccess = UTIL_GroupToString( ucGroup );

		if( !vecQuery[2].empty( ) )
			strAccess += " <span class=\"" + strClass + "\">" + UTIL_RemoveHTML( vecQuery[2] ) + "</span>";
		
		if( strAccess.empty( ) )
			strAccess = gmapLANG_CFG["unknown"];
		
		strUserLink += " (" + strAccess + ")";
	}
	else
	{
		strUserLink += UTIL_RemoveHTML( strUsername );
		strUserLink += " (" + gmapLANG_CFG["unknown"] + ")";
	}
	
	return strUserLink;
}

const string CTracker :: getUserLinkTalk( const string &strUID, const string &strUsername )
{
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "getUserLink: started\n" );
			
	if( strUsername.empty( ) )
		return gmapLANG_CFG["unknown"];
	else if( strUID.empty( ) )
		return UTIL_RemoveHTML( strUsername );
	
	string strUserLink = string( );

	CMySQLQuery *pQuery = new CMySQLQuery( "SELECT busername,baccess,bgroup,buploaded,bdownloaded,UNIX_TIMESTAMP(bwarned) FROM users WHERE buid=" + strUID );

	vector<string> vecQuery;
	
	vecQuery.reserve(6);

	vecQuery = pQuery->nextRow( );
	
	delete pQuery;
	
	if( vecQuery.size( ) == 6 )
	{
		unsigned char ucAccess = (unsigned char)atoi( vecQuery[1].c_str( ) );
		unsigned char ucGroup = (unsigned char)atoi( vecQuery[2].c_str( ) );
		int64 iUploaded = 0, iDownloaded = 0;
		float flShareRatio = 0;
		
		iUploaded = UTIL_StringTo64( vecQuery[3].c_str( ) );
		iDownloaded = UTIL_StringTo64( vecQuery[4].c_str( ) );

		if( iDownloaded == 0 )
		{
			if( iUploaded == 0 )
				flShareRatio = 0;
			else
				flShareRatio = -1;
		}
		else
			flShareRatio = (float)iUploaded / (float)iDownloaded;
		
		bool bShareRatioWarned = checkShareRatio( iDownloaded, flShareRatio );
		string strClass = UTIL_UserClass( ucAccess, ucGroup );
		
		strUserLink += "<a class=\"";
		
		if( m_bRatioRestrict && bShareRatioWarned && strClass == gmapLANG_CFG["class_member"] )
			strUserLink += "share_warned";
		else
			strUserLink += strClass;
		
		strUserLink += "\" href=\"" + RESPONSE_STR_TALK_HTML + "?uid=" + strUID + "\">";
		
		if( !strUsername.empty( ) )
			strUserLink += UTIL_RemoveHTML( strUsername );
		else
			strUserLink += vecQuery[0];
		strUserLink += "</a>";

		if( !( ucAccess & ACCESS_VIEW ) )
			strUserLink += "<img title=\"" + gmapLANG_CFG["user_banned"] + "\" src=\"files/warned1.gif\">";
		if( m_bRatioRestrict && bShareRatioWarned )
			strUserLink += "<img title=\"" + gmapLANG_CFG["user_shareratio_warned"] + "\" src=\"files/warned3.gif\">";
		if( vecQuery[5] != "0" )
			strUserLink += "<img title=\"" + gmapLANG_CFG["user_warned"] + "\" src=\"files/warned.gif\">";
	}
	else
	{
		strUserLink += UTIL_RemoveHTML( strUsername );
	}
	
	return strUserLink;
}

// Add a user
const string CTracker :: addUser( const string &strLogin, const string &strPass, const unsigned char ucAccess, const string &strMail )
{
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "addUser: started\n" );
	
	unsigned long culKeySize = 0;
	
//	if( m_pCache )
//		culKeySize = m_pCache->getSizeUsers( );
		
	CMySQLQuery *pQuery = new CMySQLQuery( "SELECT COUNT(*) FROM users" );
	
	vector<string> vecQuery;
	
	vecQuery.reserve(1);
	
	vecQuery = pQuery->nextRow( );
	
	delete pQuery;
	
	if( vecQuery.size( ) == 1 )
		culKeySize = UTIL_StringTo64( vecQuery[0].c_str( ) );
	
	if( culKeySize >= CFG_GetInt( "bnbt_max_users", 1000 ) )
		return string( );
	
	string strQuery = "INSERT INTO users (busername,bmd5,bcreated,bemail,baccess,buploaded,bdownloaded,bbonus) VALUES(\'" + UTIL_StringToMySQL( strLogin ) + "\'";

	// calculate md5 hash of A1
	const string cstrA1( strLogin + ":" + gstrPasswordKey + ":" + strPass );

	unsigned char szMD5[16];
	memset( szMD5, 0, sizeof( szMD5 ) / sizeof( unsigned char ) );

	MD5_CTX md5;

	MD5Init( &md5 );
	MD5Update( &md5, (const unsigned char *)cstrA1.c_str( ), (unsigned int)cstrA1.size( ) );
	MD5Final( szMD5, &md5 );
	
	strQuery += ",\'" + UTIL_StringToMySQL( string( (char *)szMD5, sizeof( szMD5 ) / sizeof( unsigned char ) ) );
	
	strQuery += "\',NOW()";

	strQuery += ",\'" + UTIL_StringToMySQL( strMail );
	
	if( ucAccess )
		strQuery += "\'," + CAtomInt( ucAccess ).toString( );
	else
		strQuery += "\'," + CAtomInt( m_ucMemberAccess ).toString( );
	
	int64 iUploaded = CFG_GetInt( "bnbt_new_user_gift_uploaded", 0 );
	int64 iDownloaded = CFG_GetInt( "bnbt_new_user_gift_downloaded", 0 );
	int64 iBonus = CFG_GetInt( "bnbt_new_user_gift_bonus", 0 );
	
	iUploaded = iUploaded * 1024 * 1024 * 1024;
	iDownloaded = iDownloaded * 1024 * 1024 * 1024;
	iBonus = iBonus * 100;
	
	strQuery += "," + CAtomLong( iUploaded ).toString( );
	strQuery += "," + CAtomLong( iDownloaded ).toString( );
	strQuery += "," + CAtomLong( iBonus ).toString( );
	strQuery += ")";
	
	CMySQLQuery *pQueryUser = new CMySQLQuery( strQuery );
	
	unsigned long ulLast = pQueryUser->lastInsertID( );
	
	delete pQueryUser;
	
	string strUID = string( );
	
	if( ulLast > 0 )
	{
		InitPasskey( strLogin );
		strUID = CAtomLong( ulLast ).toString( );
		CMySQLQuery mq01( "INSERT INTO users_prefs (buid) VALUES(" + strUID + ")" );
		sendMessage( "", "0", strUID, "127.0.0.1", gmapLANG_CFG["admin_add_user_title"], gmapLANG_CFG["admin_add_user"] );
	}
	
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "addUser: completed\n" );
		
	return strUID;
}


// Delete a user
void CTracker :: deleteUser( const string &strUID )
{
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "deleteUser: started\n" );

	CMySQLQuery mq01( "DELETE FROM users WHERE buid=" + strUID );
	CMySQLQuery mq02( "DELETE FROM users_prefs WHERE buid=" + strUID );
	CMySQLQuery mq03( "DELETE FROM peers WHERE buid=" + strUID + " AND bcompleted=0" );
	CMySQLQuery mq04( "DELETE FROM messages WHERE bsendtoid=" + strUID );
	CMySQLQuery mq05( "DELETE FROM messages_sent WHERE bfromid=" + strUID );
	CMySQLQuery mq06( "DELETE FROM bookmarks WHERE buid=" + strUID );
	CMySQLQuery mq07( "DELETE FROM friends WHERE buid=" + strUID );
	CMySQLQuery mq08( "DELETE FROM friends WHERE bfriendid=" + strUID );
	CMySQLQuery mq09( "DELETE FROM invites WHERE bownerid=" + strUID );

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "deleteUser: completed\n" );
		
}


void CTracker :: InitPasskey( const string &strLogin )
{
	unsigned char szMD5[16];
	memset( szMD5, 0, sizeof( szMD5 ) / sizeof( unsigned char ) );
	MD5_CTX md5;

	time_t tNow = time( 0 );
	char pTime[256];
	memset( pTime, 0, sizeof( pTime ) / sizeof( char ) );
	strftime( pTime, sizeof( pTime ) / sizeof( char ), "%Y-%m-%d %H:%M:%S", localtime( &tNow ) );
	const string cstrA1( strLogin + ":" + gstrRealm + ":" + pTime );

	MD5Init( &md5 );
	MD5Update( &md5, (const unsigned char *)cstrA1.c_str( ), (unsigned int)cstrA1.size( ) );
	MD5Final( szMD5, &md5 );
	
	CMySQLQuery mq01( "UPDATE users SET bpasskey=\'" + UTIL_StringToMySQL( UTIL_HashToString( string( (char *)szMD5, sizeof( szMD5 ) / sizeof( unsigned char ) ) ) ) + "\' WHERE busername=\'" + UTIL_StringToMySQL( strLogin ) + "\'" );
}

// Count the unique peers
void CTracker :: CountUniquePeers( )
{
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "CountUniquePeers: started\n" );
		
	CMySQLQuery mq01( "TRUNCATE TABLE ips" );
	
	CMySQLQuery mq02( "INSERT INTO ips SELECT bip,COUNT(*) FROM dstate GROUP BY bip" );

	CMySQLQuery *pQueryIP = new CMySQLQuery( "SELECT COUNT(*) FROM ips" );
	
	vector<string> vecQueryIP;
	
	vecQueryIP.reserve(1);

	vecQueryIP = pQueryIP->nextRow( );
	
	delete pQueryIP;
	
	int64 iGreatestUnique = UTIL_StringTo64( vecQueryIP[0].c_str( ) );
	
	if( iGreatestUnique > gtXStats.peer.iGreatestUnique )
		gtXStats.peer.iGreatestUnique = iGreatestUnique;

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "CountUniquePeers: completed (%i)\n", iGreatestUnique );
}

// Add a unique peer to the count
void CTracker :: AddUniquePeer( const string &strIP )
{
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "AddUniquePeer: started (%s)\n", strIP.c_str( ) );
		
	CMySQLQuery mq01( "INSERT INTO ips VALUES(\'" + UTIL_StringToMySQL( strIP ) + "\',1) ON DUPLICATE KEY UPDATE bcount=bcount+1" );

	// increment unique count for this ip

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "AddUniquePeer: completed\n" );
}

// Remove a unique peer from the count
void CTracker :: RemoveUniquePeer( const string &strIP )
{
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "RemoveUniquePeer: started (%s)\n", strIP.c_str( ) );



	// decrement unique count for this ip
	
//	CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bcount FROM ips WHERE bip=\'" + UTIL_StringToMySQL( strIP ) + "\'" );
//	
//	vector<string> vecQuery;
//	
//	vecQuery.reserve(1);
//	
//	vecQuery = pQuery->nextRow( );
//	
//	delete pQuery;
//	
//	if( vecQuery.size( ) == 1 )
//	{
//		if( atoi( vecQuery[0].c_str( ) ) > 1 )
//			CMySQLQuery mq01( "UPDATE ips SET bcount=bcount-1 WHERE bip=\'" + UTIL_StringToMySQL( strIP ) + "\'" );
//		else
//			CMySQLQuery mq01( "DELETE FROM ips WHERE bip=\'" + UTIL_StringToMySQL( strIP ) + "\'" );
//	}
	
	CMySQLQuery mq01( "UPDATE ips SET bcount=bcount-1 WHERE bip=\'" + UTIL_StringToMySQL( strIP ) + "\' AND bcount>0" );

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "RemoveUniquePeer: completed\n" );
}

const string CTracker :: GetIMDb( const string &cstrIMDbID )

{
// 	FILE *stream;
	FILE *rstream;
	char buf[1024];
	
	string strIMDb = string( );
	
// 	system( string( "curl -x 172.16.3.38:3128 --connect-timeout 30 http://www.imdb.com/title/"+ cstrIMDbID + "/ -o imdb.tmp" ).c_str( ) );
	system( string( "curl --connect-timeout 30 http://www.imdb.com/title/"+ cstrIMDbID + "/ -o imdb.tmp" ).c_str( ) );

	string strHTML = string( buf );
	
	strHTML = UTIL_ReadFile( "imdb.tmp" );
	
	string :: size_type iStart = string :: npos;
	string :: size_type iEnd = string :: npos;
	
	if( strHTML.find( "(awaiting 5 votes)" ) != string :: npos )
		return strIMDb;
	
	iStart = strHTML.find( "class=\"starbar-meta\"" );
	
	if( iStart != string :: npos )
	{
		iStart = strHTML.find( "<b>", iStart );
		if( iStart != string :: npos )
		{
			iEnd = strHTML.find( "/", iStart );
			if( iEnd != string :: npos )
				strIMDb = strHTML.substr( iStart + 3, iEnd - iStart - 3 );
		}
	}
	
	return strIMDb;
}

void CTracker :: GetIMDbLoop( )
{
	system( string( "./getimdb.py &" ).c_str( ) );
// 	CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bid,bimdbid FROM allowed WHERE bimdbupdated<NOW()-interval 3 day AND bimdbid!=\'\' GROUP BY bimdbid UNION SELECT bid,bimdbid FROM offer WHERE bimdbupdated<NOW()-interval 3 day AND bimdbid!=\'\' GROUP BY bimdbid" );
// 			
// 	vector<string> vecQuery;
// 
// 	vecQuery.reserve(2);
// 
// 	vecQuery = pQuery->nextRow( );
// 	
// 	while( vecQuery.size( ) == 2 )
// 	{
// 		string strIMDb = string( );
// 		if( !vecQuery[1].empty( ) )
// 		{
// // 			UTIL_LogPrint( "%s\n", strIMDb.c_str( ) );
// 			strIMDb = GetIMDb( vecQuery[1] );
// 		}
// 		if( !strIMDb.empty( ) )
// 		{
// 			CMySQLQuery mq01( "UPDATE allowed SET bimdb=\'" + UTIL_StringToMySQL( strIMDb ) + "\',bimdbupdated=NOW() WHERE bimdbid=\'" + UTIL_StringToMySQL( vecQuery[1] ) + "\'" );
// 			CMySQLQuery mq02( "UPDATE offer SET bimdb=\'" + UTIL_StringToMySQL( strIMDb ) + "\',bimdbupdated=NOW() WHERE bimdbid=\'" + UTIL_StringToMySQL( vecQuery[1] ) + "\'" );
// 		}
// 		vecQuery = pQuery->nextRow( );
// 	}
// 	
// 	delete pQuery;
}
			

void CTracker :: CBTTParseList( )
{

	// CBTT parse the client ban list
	if( !m_strClientBanFile.empty( ) )
	{
		if( gbDebug )
			if( gucDebugLevel & DEBUG_TRACKER )
				UTIL_LogPrint( "CTracker: CBTT client ban parse list\n" );

		m_pClientBannedList->clear( );
		m_pClientBannedList = new CAtomList( );
//		m_pClientBannedList->clear( );

		const string cstrClientBanListData( UTIL_ReadFile( m_strClientBanFile.c_str( ) ) ); 
		const string :: size_type ciClientBanListDataLength( cstrClientBanListData.length( ) );

		// loop through the ban file and add 
		string :: size_type iStartClientBanListData = cstrClientBanListData.find_first_not_of( STR_SEPARATORS ); 
		string :: size_type iStopClientBanListData = 0;

		while ( iStartClientBanListData != string :: npos && iStartClientBanListData < ciClientBanListDataLength )
		{ 
			iStopClientBanListData = cstrClientBanListData.find_first_of( STR_SEPARATORS, iStartClientBanListData );

			if ( iStopClientBanListData == string :: npos || iStopClientBanListData > ciClientBanListDataLength ) 
				iStopClientBanListData = ciClientBanListDataLength; 

			m_pClientBannedList->addItem( new CAtomString( cstrClientBanListData.substr( iStartClientBanListData, iStopClientBanListData - iStartClientBanListData ) ) ); 

			iStartClientBanListData = cstrClientBanListData.find_first_not_of( STR_SEPARATORS, iStopClientBanListData + 1 ); 
		} 

		UTIL_LogPrint( "%s\n", gmapLANG_CFG["cbtt_client_parse"].c_str( ) );
	}

	// CBTT parse the IP ban list
	if( !m_strIPBanFile.empty( ) )
	{
		if( gbDebug )
			if( gucDebugLevel & DEBUG_TRACKER )
				UTIL_LogPrint( "CTracker: CBTT IP ban parse list\n" );

		m_pIPBannedList->clear( );
		m_pIPBannedList = new CAtomList( );
//		m_pIPBannedList->clear( );

		const string cstrIPBanFileData( UTIL_ReadFile( m_strIPBanFile.c_str( ) ) ); 
		const string :: size_type ciIPBanFileDataLength( cstrIPBanFileData.length( ) ); 

		// loop through the ban file and add shit 
		string :: size_type iStartIPBanFileData = cstrIPBanFileData.find_first_not_of( STR_SEPARATORS ); 
		string :: size_type iStopIPBanFileData = 0;

		while ( iStartIPBanFileData != string :: npos && iStartIPBanFileData < ciIPBanFileDataLength )
		{ 
			iStopIPBanFileData = cstrIPBanFileData.find_first_of( STR_SEPARATORS, iStartIPBanFileData ); 

			if ( iStopIPBanFileData == string :: npos || iStopIPBanFileData > ciIPBanFileDataLength ) 
				iStopIPBanFileData = ciIPBanFileDataLength; 

			m_pIPBannedList->addItem( new CAtomString( cstrIPBanFileData.substr( iStartIPBanFileData, iStopIPBanFileData - iStartIPBanFileData ) ) ); 

			iStartIPBanFileData = cstrIPBanFileData.find_first_not_of( STR_SEPARATORS, iStopIPBanFileData + 1 ); 
		}

		UTIL_LogPrint( "%s\n", gmapLANG_CFG["cbtt_ip_parse"].c_str( ) );
	}

	//
	// XBNBT parse list
	//

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "CTracker: XBNBT parse list\n" );
		
}

void CTracker :: RefreshStatic( )
{
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "CTracker: refreshing static files\n" );

	// Static header
	if( !m_strStaticHeaderFile.empty( ) )
		m_strStaticHeader = UTIL_ReadFile( m_strStaticHeaderFile.c_str( ) );
	else if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "CTracker: no static header file specified\n" );

	// Static footer
	if( !m_strStaticFooterFile.empty( ) )
		m_strStaticFooter = UTIL_ReadFile( m_strStaticFooterFile.c_str( ) );
	else if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "CTracker: no static footer file specified\n" );

	// robots.txt
	if( !robots.strName.empty( ) )
		robots.strFile = UTIL_ReadFile( robots.strName.c_str( ) );
	else if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "CTracker: no static robots file specified\n" );

	// favicon.ico
	bool bUseInternalFavicon = false;

	if( !favicon.strName.empty( ) )
	{
		if( UTIL_CheckFile( string( favicon.strDir + RESPONSE_STR_SEPERATOR + favicon.strName ).c_str( ) ) )
		{
			favicon.strExt = getFileExt( favicon.strName );

			if( favicon.strExt == ".ico" || favicon.strExt == ".png" || favicon.strExt == ".gif" )
			{
				if( !favicon.strDir.empty( ) )
					favicon.strFile = UTIL_ReadFile( string( favicon.strDir + RESPONSE_STR_SEPERATOR + favicon.strName ).c_str( ) );
				else
					favicon.strFile = UTIL_ReadFile( favicon.strName.c_str( ) );
			}
			else
			{
				if( gbDebug )
					if( gucDebugLevel & DEBUG_TRACKER )
						UTIL_LogPrint( "CTracker: favicon file type %s not supported\n", favicon.strExt.c_str( ) );

				bUseInternalFavicon = true;
			}
		}
		else
		{
			if( gbDebug )
			{
				if( gucDebugLevel & DEBUG_TRACKER )
				{
					if( !favicon.strDir.empty( ) )
						UTIL_LogPrint( "CTracker: favicon file %s not found\n", ( favicon.strDir + RESPONSE_STR_SEPERATOR + favicon.strName ).c_str( ) );
					else
						UTIL_LogPrint( "CTracker: favicon file %s not found\n", favicon.strName.c_str( ) );
				}
			}
		
			bUseInternalFavicon = true;
		}
	}

	else
	{
		if( gbDebug )
			if( gucDebugLevel & DEBUG_TRACKER )
				UTIL_LogPrint( "CTracker: favicon is not set\n" );

		bUseInternalFavicon = true;
	}

	if( bUseInternalFavicon && favicon.strFile.empty( ) )
		for( unsigned int uiCount = 0; uiCount < sizeof(cuiFavIconIco) / sizeof(int); uiCount++ )
			favicon.strFile += (const unsigned char)cuiFavIconIco[uiCount];

	if( !userbar.strName.empty( ) )
	{
		if( !userbar.strDir.empty( ) )
			userbar.strFile = UTIL_ReadFile( ( userbar.strDir + RESPONSE_STR_SEPERATOR + userbar.strName ).c_str( ) );
		else
			userbar.strFile = UTIL_ReadFile( userbar.strName.c_str( ) );
	}

	// XBNBT - Serve HTTP interface requests using the internal file server
	if( m_bServeLocal )
	{
		// XML
// 			if( !xmldump.strName.empty( ) )
// 			{
// 				if( !xmldump.strDir.empty( ) )
// 					xmldump.strFile = UTIL_ReadFile( ( xmldump.strDir + RESPONSE_STR_SEPERATOR + xmldump.strName ).c_str( ) );
// 				else
// 					xmldump.strFile = UTIL_ReadFile( xmldump.strName.c_str( ) );
// 			}
// 			else if( gbDebug )
// 				if( gucDebugLevel & DEBUG_TRACKER )
// 					UTIL_LogPrint( "CTracker: XML file name not set\n" );

		// RSS
// 			if( !rssdump.strName.empty( ) )
// 			{
// 				if( !rssdump.strDir.empty( ) )
// 					rssdump.strFile = UTIL_ReadFile( ( rssdump.strDir + RESPONSE_STR_SEPERATOR + rssdump.strName ).c_str( ) );
// 				else
// 					rssdump.strFile = UTIL_ReadFile( rssdump.strName.c_str( ) );
// 			}
// 			else if( gbDebug )
// 				if( gucDebugLevel & DEBUG_TRACKER )
// 					UTIL_LogPrint( "CTracker: RSS file name not set\n" );

		// RSS XSL
		if( !rssdump.strName.empty( ) )
		{
			if( !rssdump.strDir.empty( ) )
				rssxsl.strFile = UTIL_ReadFile( ( rssdump.strDir + RESPONSE_STR_SEPERATOR + "rss.xsl" ).c_str( ) );
			else
				rssxsl.strFile = UTIL_ReadFile( "rss.xsl" );
		}
		else if( gbDebug )
			if( gucDebugLevel & DEBUG_TRACKER )
				UTIL_LogPrint( "CTracker: RSS file name not set\n" );

		// CSS
		if( !style.strName.empty( ) )
		{
			if( !style.strDir.empty( ) )
				style.strFile = UTIL_ReadFile( ( style.strDir + RESPONSE_STR_SEPERATOR + style.strName ).c_str( ) );
			else
				style.strFile = UTIL_ReadFile( style.strName.c_str( ) );
		}
		else if( gbDebug )
			if( gucDebugLevel & DEBUG_TRACKER )
				UTIL_LogPrint( "CTracker: CSS file name not set\n" );

		// Image bar fill
		if( !imagefill.strName.empty( ) )
		{
			if( !imagefill.strDir.empty( ) )
				imagefill.strFile = UTIL_ReadFile( ( imagefill.strDir + RESPONSE_STR_SEPERATOR + imagefill.strName ).c_str( ) );
			else
				imagefill.strFile = UTIL_ReadFile( imagefill.strName.c_str( ) );
		}

		else if( gbDebug )
			if( gucDebugLevel & DEBUG_TRACKER )
				UTIL_LogPrint( "CTracker: Image bar fill file name not set\n" );

		if( imagefill.strFile.empty( ) )
			for( unsigned char ucCount = 0; ucCount < sizeof(cuiImageBarFillPng) / sizeof(int); ucCount++ )
				imagefill.strFile += (const unsigned char)cuiImageBarFillPng[ucCount];

		// Image bar trans
		if( !imagetrans.strName.empty( ) )
		{
			if( !imagetrans.strDir.empty( ) )
				imagetrans.strFile = UTIL_ReadFile( ( imagetrans.strDir + RESPONSE_STR_SEPERATOR + imagetrans.strName ).c_str( ) );
			else
				imagetrans.strFile = UTIL_ReadFile( imagetrans.strName.c_str( ) );
		}
		else if( gbDebug )
			if( gucDebugLevel & DEBUG_TRACKER )
				UTIL_LogPrint( "CTracker: Image bar trans file name not set\n" );

		if( imagetrans.strFile.empty( ) )
			for( unsigned char ucCount = 0; ucCount < sizeof(cuiImageBarTransPng) / sizeof(int); ucCount++ )
				imagetrans.strFile += (const unsigned char)cuiImageBarTransPng[ucCount];
	}
}

// Act upon the received announce
void CTracker :: Announce( const struct announce_t &ann, bool &bRespond )
{
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "Announce: event (%s)\n", ann.strEvent.c_str( ) );

	bool bPeerFound = false;
	bool bCompleted = false;
	bool bIPv6 = false;
	bool bIPv6Announce = false;

	// Get the list of peers for this info hash
	
//	CMySQLQuery *pQueryUser = new CMySQLQuery( "SELECT buid,busername FROM users WHERE buid=" + ann.strUID );

//	vector<string> vecQueryUser;
//	
//	vecQueryUser.reserve(2);

//	vecQueryUser = pQueryUser->nextRow( );
//	
//	delete pQueryUser;
//	
//	string strUID = string( );
//	string strUsername = string( );
//	
//	if( vecQueryUser.size( ) == 2 )
//	{
//		strUID =  vecQueryUser[0];
//		strUsername = vecQueryUser[1];
//	}
	
//	CMySQLQuery *pQueryTorrent = new CMySQLQuery( "SELECT bid FROM allowed WHERE bid=" + ann.strID );

//	vector<string> vecQueryTorrent;
//	







//	vecQueryTorrent.reserve(1);

//	vecQueryTorrent = pQueryTorrent->nextRow( );
//	
//	delete pQueryTorrent;
//	
//	string strID = string( );
//	
//	if( vecQueryTorrent.size( ) == 1 )
//	{
//		strID =  vecQueryTorrent[0];
//	}
	
	string strIPName = string( );
	
	if( ann.strIP.find( ":" ) != string :: npos )
	{
		strIPName = "bip6";
		bIPv6Announce = true;
	}
	else
		strIPName = "bip";

	// Do we have a list of peers?

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "Announce: got list of peers\n" );

	// Get the peer from the list

	CMySQLQuery *pQueryPeer = new CMySQLQuery( "SELECT bip,bip6,bport,bkey,buploaded,bdownloaded,bleft,UNIX_TIMESTAMP(bupdated),bcompleted FROM dstate WHERE bid=" + ann.strID + " AND buid=" + ann.strUID + " AND bpeerid=\'" + UTIL_StringToMySQL( ann.strPeerID ) + "\'"  );
	
	vector<string> vecQueryPeer;
		
	vecQueryPeer.reserve(9);

	vecQueryPeer = pQueryPeer->nextRow( );
	
	delete pQueryPeer;
	
	if( vecQueryPeer.size( ) == 9 )
	{
		if( gbDebug )
			if( gucDebugLevel & DEBUG_TRACKER )
				UTIL_LogPrint( "Announce: got %s)\n", ann.strPeerID.c_str( ) );

		// Signal we found the peer
		bPeerFound = true;
		string strIP = string( );
		
		if( vecQueryPeer[8] == "1" )
			bCompleted = true;
			
		if( !vecQueryPeer[1].empty( ) )
			bIPv6 = true;
			
		if( bIPv6Announce )
			strIP = vecQueryPeer[1];
		else
			strIP = vecQueryPeer[0];
		
		string strQuery = "UPDATE dstate SET busername=\'" + UTIL_StringToMySQL( ann.strUsername ) + "\'";

		// Announce key support
		if( m_bAnnounceKeySupport )
		{
			if( gbDebug )
				if( gucDebugLevel & DEBUG_TRACKER )
					UTIL_LogPrint( "Announce: key support (enabled)\n" );

			// Get the last key supplied by the peer

			if( !vecQueryPeer[3].empty( ) )
			{
				if( gbDebug )
					if( gucDebugLevel & DEBUG_TRACKER )
						UTIL_LogPrint( "Announce: last key (%s)\n", vecQueryPeer[3].c_str( ) );

				// Get the last IP supplied by the peer



				if( !strIP.empty( ) )
				{
					if( gbDebug )
						if( gucDebugLevel & DEBUG_TRACKER )
							UTIL_LogPrint( "Announce: last IP (%s)\n", strIP.c_str( ) );

					// Does the last key match the current key?
					if( vecQueryPeer[3] == ann.strKey )
					{
						if( gbDebug )
							if( gucDebugLevel & DEBUG_TRACKER )
								UTIL_LogPrint( "Announce: key matched (%s)\n", ann.strKey.c_str( ) );

						// Are the last IP and the current IP the same?
						if( strIP == ann.strIP )
						{
							if( gbDebug )
								if( gucDebugLevel & DEBUG_TRACKER )
									UTIL_LogPrint( "Announce: IP (%s) has not changed\n", ann.strIP.c_str( ) );
						}
						else
						{
							if( gbDebug )
								if( gucDebugLevel & DEBUG_TRACKER )
									UTIL_LogPrint( "Announce: IP (%s) change to (%s)\n", strIP.c_str( ), ann.strIP.c_str( ) );

							// Remove the old IP from the unique peers list
							if( m_bCountUniquePeers )
							{
								if( gbDebug )
									if( gucDebugLevel & DEBUG_TRACKER )
										UTIL_LogPrint( "Announce: remove unique peer (enabled)\n" );



								RemoveUniquePeer( strIP );
							}

							// Updating peer IP from the announce
							strQuery += "," + strIPName + "=\'" + UTIL_StringToMySQL( ann.strIP ) + "\'";

							// Add the new IP to the unique peers list
							if( m_bCountUniquePeers )
							{
								if( gbDebug )
									if( gucDebugLevel & DEBUG_TRACKER )
										UTIL_LogPrint( "Announce: count unique peer (enabled)\n" );

								AddUniquePeer( ann.strIP );
							}
						}

						// Updating peer information from the announce

						if( gbDebug )
							if( gucDebugLevel & DEBUG_TRACKER )
								UTIL_LogPrint( "Announce: updating uploaded, downloaded & left\n" );

						string strAdded = string( );
						int64 iFreeDown = 100, iFreeUp = 100, iDefaultDown = 100, iDefaultUp = 100, iFreeTo = 0;
						int64 iFreeDownGlobal = CFG_GetInt( "bnbt_free_down_global", 100 );
						int64 iFreeUpGlobal = CFG_GetInt( "bnbt_free_up_global", 100 );
						
						CMySQLQuery *pQuery = new CMySQLQuery( "SELECT badded,bdefault_down,bdefault_up,bfree_down,bfree_up,UNIX_TIMESTAMP(bfree_to) FROM allowed WHERE bid=" + ann.strID );
								
						vector<string> vecQuery;
						
						vecQuery.reserve(6);

						vecQuery = pQuery->nextRow( );
						
						delete pQuery;

						if( vecQuery.size( ) == 6 )
						{
		// 					pSize = vecTorrent[3];
							if( !vecQuery[0].empty( ) )
								strAdded = vecQuery[0];

							if( !vecQuery[1].empty( ) )
								iDefaultDown = UTIL_StringTo64( vecQuery[1].c_str( ) );
							if( !vecQuery[2].empty( ) )
								iDefaultUp = UTIL_StringTo64( vecQuery[2].c_str( ) );
							if( CFG_GetInt( "bnbt_free_global", 0 ) == 1 )
							{
								if( iFreeDownGlobal < iDefaultDown )
									iDefaultDown = iFreeDownGlobal;
								if( iFreeUpGlobal > iDefaultUp )
									iDefaultUp = iFreeUpGlobal;
							}
							
							if( !vecQuery[5].empty( ) )
								iFreeTo = UTIL_StringTo64( vecQuery[5].c_str( ) );
							time_t now_t = time( 0 );

							if( !vecQuery[3].empty( ) )
								iFreeDown = UTIL_StringTo64( vecQuery[3].c_str( ) );
							if( !vecQuery[4].empty( ) )
								iFreeUp = UTIL_StringTo64( vecQuery[4].c_str( ) );
							if( iFreeTo > 0 && iFreeTo > now_t )
							{
								if( iDefaultDown < iFreeDown )
									iFreeDown = iDefaultDown;
								if( iDefaultUp > iFreeUp )
									iFreeUp = iDefaultUp;
							}
							else
							{
								iFreeDown = iDefaultDown;
								iFreeUp = iDefaultUp;
							}
						}
						
						int64 iPeerUploaded = 0, iPeerDownloaded = 0, iPeerLeft = 0, iPeerUpdated = 0;
						
						if( !vecQueryPeer[4].empty( ) )
							iPeerUploaded = UTIL_StringTo64( vecQueryPeer[4].c_str( ) );
						if( !vecQueryPeer[5].empty( ) )
							iPeerDownloaded = UTIL_StringTo64( vecQueryPeer[5].c_str( ) );
						if( !vecQueryPeer[6].empty( ) )
							iPeerLeft = UTIL_StringTo64( vecQueryPeer[6].c_str( ) );
						if( !vecQueryPeer[7].empty( ) )
							iPeerUpdated = UTIL_StringTo64( vecQueryPeer[7].c_str( ) );
							
						int64 iUpdatedInterval = ( int64 )( GetTime( ) - iPeerUpdated );
						
						if( ann.iUploaded >= iPeerUploaded && ann.iDownloaded >= iPeerDownloaded )
						{
//							m_pCache->setUserData( ann.strUID, iFreeUp * ( ann.iUploaded - iPeerUploaded ) / 100, iFreeDown * ( ann.iDownloaded - iPeerDownloaded ) / 100, 0 );
							CMySQLQuery mq01( "UPDATE users SET buploaded=buploaded+" + CAtomLong( iFreeUp * ( ann.iUploaded - iPeerUploaded ) / 100 ).toString( ) + ",bdownloaded=bdownloaded+" + CAtomLong( iFreeDown * ( ann.iDownloaded - iPeerDownloaded ) / 100 ).toString( ) + " WHERE buid=" + ann.strUID );
							
 							string strQueryStatistics = string( );
							strQueryStatistics = "UPDATE statistics SET buploaded=buploaded+" + CAtomLong( ann.iUploaded - iPeerUploaded ).toString( ) + ",bdownloaded=bdownloaded+" + CAtomLong( ann.iDownloaded - iPeerDownloaded ).toString( );
							if( ann.iLeft > 0 || iPeerLeft > 0 )
								strQueryStatistics += ",bdowntime=bdowntime+";
							else
								strQueryStatistics += ",bseedtime=bseedtime+";
							strQueryStatistics += CAtomLong( iUpdatedInterval ).toString( );
							strQueryStatistics += " WHERE buid=" + ann.strUID + " AND bid=" + ann.strID;
			 				CMySQLQuery mq02( strQueryStatistics );
						}
						
						if( iPeerUploaded > ann.iUploaded || iPeerDownloaded > ann.iDownloaded || iUpdatedInterval == 0 )
						{
							strQuery += ",bupspeed=0,bdownspeed=0";
						}
						else
						{
							if( iUpdatedInterval > 30 )
							{
								strQuery += ",bupspeed=" + CAtomLong( ( ann.iUploaded - iPeerUploaded ) / iUpdatedInterval ).toString( );
								strQuery += ",bdownspeed=" + CAtomLong( ( ann.iDownloaded - iPeerDownloaded ) / iUpdatedInterval ).toString( );
							}
						}
						
						strQuery += ",buploaded=" + CAtomLong( ann.iUploaded ).toString( );
						strQuery += ",bdownloaded=" + CAtomLong( ann.iDownloaded ).toString( );
						strQuery += ",bleft=" + CAtomLong( ann.iLeft ).toString( );
						strQuery += ",bupdated=NOW()";
					}
					else
					{
						if( gbDebug )
							if( gucDebugLevel & DEBUG_TRACKER )
								UTIL_LogPrint( "Announce: not matched (ID %s)(KEY %s)(IP %s)\n", ann.strPeerID.c_str( ), ann.strKey.c_str( ), ann.strIP.c_str( ) );
					}
				}
				else
				{
					if( gbDebug )
						if( gucDebugLevel & DEBUG_TRACKER )
							UTIL_LogPrint( "Announce: last IP not found\n" );
					
					strQuery += "," + strIPName + "=\'" + UTIL_StringToMySQL( ann.strIP ) + "\'";
				}
			}
			else
			{
				if( gbDebug )
					if( gucDebugLevel & DEBUG_TRACKER )
						UTIL_LogPrint( "Announce: last key not found\n" );
				
				strQuery += ",bkey=\'" + UTIL_StringToMySQL( ann.strKey ) + "\'";
			}
		}
		else
		{
			// Original announce support
			if( gbDebug )
				if( gucDebugLevel & DEBUG_TRACKER )
					UTIL_LogPrint( "Announce: key support (disabled)\n" );

			// Updating peer information from the announce

			if( gbDebug )
				if( gucDebugLevel & DEBUG_TRACKER )
					UTIL_LogPrint( "Announce: updating uploaded, downloaded & left\n" );
			
			string strAdded = string( );
			int64 iFreeDown = 100, iFreeUp = 100, iDefaultDown = 100, iDefaultUp = 100, iFreeTo = 0;
			int64 iFreeDownGlobal = CFG_GetInt( "bnbt_free_down_global", 100 );
			int64 iFreeUpGlobal = CFG_GetInt( "bnbt_free_up_global", 100 );
				
			CMySQLQuery *pQuery = new CMySQLQuery( "SELECT badded,bdefault_down,bdefault_up,bfree_down,bfree_up,UNIX_TIMESTAMP(bfree_to) FROM allowed WHERE bid=" + ann.strID );
								
			vector<string> vecQuery;
			
			vecQuery.reserve(6);

			vecQuery = pQuery->nextRow( );
			
			delete pQuery;

			if( vecQuery.size( ) == 6 )
			{
// 					pSize = vecTorrent[3];
				if( !vecQuery[0].empty( ) )
					strAdded = vecQuery[0];

				if( !vecQuery[1].empty( ) )
					iDefaultDown = UTIL_StringTo64( vecQuery[1].c_str( ) );
				if( !vecQuery[2].empty( ) )
					iDefaultUp = UTIL_StringTo64( vecQuery[2].c_str( ) );
				if( CFG_GetInt( "bnbt_free_global", 0 ) == 1 )
				{
					if( iFreeDownGlobal < iDefaultDown )
						iDefaultDown = iFreeDownGlobal;
					if( iFreeUpGlobal > iDefaultUp )
						iDefaultUp = iFreeUpGlobal;
				}
				
				if( !vecQuery[5].empty( ) )
					iFreeTo = UTIL_StringTo64( vecQuery[5].c_str( ) );
				time_t now_t = time( NULL );

				if( !vecQuery[3].empty( ) )
					iFreeDown = UTIL_StringTo64( vecQuery[3].c_str( ) );
				if( !vecQuery[4].empty( ) )
					iFreeUp = UTIL_StringTo64( vecQuery[4].c_str( ) );
				if( iFreeTo > 0 && iFreeTo > now_t )
				{
					if( iDefaultDown < iFreeDown )
						iFreeDown = iDefaultDown;
					if( iDefaultUp > iFreeUp )
						iFreeUp = iDefaultUp;
				}
				else
				{
					iFreeDown = iDefaultDown;
					iFreeUp = iDefaultUp;
				}
			}
				
			int64 iPeerUploaded = 0, iPeerDownloaded = 0, iPeerLeft = 0, iPeerUpdated = 0;
			
			if( !vecQueryPeer[4].empty( ) )
				iPeerUploaded = UTIL_StringTo64( vecQueryPeer[4].c_str( ) );
			if( !vecQueryPeer[5].empty( ) )
				iPeerDownloaded = UTIL_StringTo64( vecQueryPeer[5].c_str( ) );
			if( !vecQueryPeer[6].empty( ) )
				iPeerLeft = UTIL_StringTo64( vecQueryPeer[6].c_str( ) );
			if( !vecQueryPeer[7].empty( ) )
				iPeerUpdated = UTIL_StringTo64( vecQueryPeer[7].c_str( ) );
			
			int64 iUpdatedInterval = ( int64 )( GetTime( ) - iPeerUpdated );
			
			if( ann.iUploaded >= iPeerUploaded && ann.iDownloaded >= iPeerDownloaded )
			{
//				m_pCache->setUserData( ann.strUID, iFreeUp * ( ann.iUploaded - iPeerUploaded ) / 100, iFreeDown * ( ann.iDownloaded - iPeerDownloaded ) / 100, 0 );
				CMySQLQuery mq01( "UPDATE users SET buploaded=buploaded+" + CAtomLong( iFreeUp * ( ann.iUploaded - iPeerUploaded ) / 100 ).toString( ) + ",bdownloaded=bdownloaded+" + CAtomLong( iFreeDown * ( ann.iDownloaded - iPeerDownloaded ) / 100 ).toString( ) + " WHERE buid=" + ann.strUID );
				
				string strQueryStatistics = string( );
				strQueryStatistics = "UPDATE statistics SET buploaded=buploaded+" + CAtomLong( ann.iUploaded - iPeerUploaded ).toString( ) + ",bdownloaded=bdownloaded+" + CAtomLong( ann.iDownloaded - iPeerDownloaded ).toString( );
				if( ann.iLeft > 0 || iPeerLeft > 0 )
					strQueryStatistics += ",bdowntime=bdowntime+";
				else
					strQueryStatistics += ",bseedtime=bseedtime+";
				strQueryStatistics += CAtomLong( iUpdatedInterval ).toString( );
				strQueryStatistics += " WHERE buid=" + ann.strUID + " AND bid=" + ann.strID;
 				CMySQLQuery mq02( strQueryStatistics );
			}
			
			if( iPeerUploaded > ann.iUploaded || iPeerDownloaded > ann.iDownloaded || iUpdatedInterval == 0 )
			{
				strQuery += ",bupspeed=0,bdownspeed=0";
			}
			else
			{
				if( iUpdatedInterval > 30 )
				{
					strQuery += ",bupspeed=" + CAtomLong( ( ann.iUploaded - iPeerUploaded ) / iUpdatedInterval ).toString( );
					strQuery += ",bdownspeed=" + CAtomLong( ( ann.iDownloaded - iPeerDownloaded ) / iUpdatedInterval ).toString( );
				}
			}

			strQuery += ",buploaded=" + CAtomLong( ann.iUploaded ).toString( );
			strQuery += ",bdownloaded=" + CAtomLong( ann.iDownloaded ).toString( );
			strQuery += ",bleft=" + CAtomLong( ann.iLeft ).toString( );
			strQuery += ",bupdated=NOW()";
		}
		
		// Peer information support
		if( m_ucShowPeerInfo != 0 )
		{
			if( gbDebug )
				if( gucDebugLevel & DEBUG_TRACKER )
					UTIL_LogPrint( "Announce: updating useragent\n" );
			
			strQuery += ",buseragent=\'" + UTIL_StringToMySQL( ann.strUserAgent ) + "\'";
		}
		
		strQuery += " WHERE bid=" + ann.strID + " AND buid=" + ann.strUID + " AND bpeerid=\'" + UTIL_StringToMySQL( ann.strPeerID ) + "\'";
		CMySQLQuery mq01( strQuery );
//		CMySQLQuery mq02( "UPDATE allowed SET bupdated=NOW() WHERE bid=" + ann.strID );
	}
	else
	{
		if( gbDebug )
			if( gucDebugLevel & DEBUG_TRACKER )
				UTIL_LogPrint( "Announce: peer (%s) not found\n", ann.strPeerID.c_str( ) );
	}
	
	unsigned char ucEvent = EVENT_UNKNOWN;

	if( ann.strEvent.empty( ) ) ucEvent = EVENT_UPDATE;
	else if( ann.strEvent == EVENT_STR_STARTED ) ucEvent = EVENT_STARTED;
	else if( ann.strEvent == EVENT_STR_COMPLETED ) ucEvent = EVENT_COMPLETED;
	else if( ann.strEvent == EVENT_STR_STOPPED ) ucEvent = EVENT_STOPPED;

	switch( ucEvent )
	{
	case EVENT_STARTED:
		if( bPeerFound )
		{
			if( bIPv6Announce && !bIPv6 )
			{
				if( ann.iLeft == 0 )
				{
					m_pCache->setActive( ann.strID, SET_SEEDER_ADD_V6 );
					CMySQLQuery mq03( "UPDATE allowed SET bseeders6=bseeders6+1,bupdated=NOW() WHERE bid=" + ann.strID );
				}
				else
				{
					m_pCache->setActive( ann.strID, SET_LEECHER_ADD_V6 );
					CMySQLQuery mq03( "UPDATE allowed SET bleechers6=bleechers6+1,bupdated=NOW() WHERE bid=" + ann.strID );
				}
			}
			
			if( gbDebug )
				if( gucDebugLevel & DEBUG_TRACKER )
					UTIL_LogPrint( "Announce: ignored duplicate peer started event\n" );

			bRespond = false;
			
			return;
		}
	case EVENT_UPDATE:
		if( !bPeerFound )
		{
			if( gbDebug )
				if( gucDebugLevel & DEBUG_TRACKER )
					UTIL_LogPrint( "Announce: creating peer (%s)\n", ann.strPeerID.c_str( ) );

			// Create a new peer entry

			// Set the new peer details
//			string strQuery = "INSERT INTO dstate (bhash,bid,bpeerid,busername,buid," + strIPName + ",bport,buploaded,bdownloaded,bleft,bconnected,bupdated,bupspeed,bdownspeed";
			string strQuery = "INSERT INTO dstate (bid,bpeerid,busername,buid," + strIPName + ",bport,buploaded,bdownloaded,bleft,bconnected,bupdated,bcompleted,bupspeed,bdownspeed";
			string strQueryValues = " VALUES(";
//			strQueryValues += "\'" + UTIL_StringToMySQL( ann.strInfoHash );
//			strQueryValues += "\'," + ann.strID;
			strQueryValues += ann.strID;
			strQueryValues += ",\'" + UTIL_StringToMySQL( ann.strPeerID );
			strQueryValues += "\',\'" + UTIL_StringToMySQL( ann.strUsername );
			strQueryValues += "\'," + ann.strUID;
			strQueryValues += ",\'" + UTIL_StringToMySQL( ann.strIP );
			strQueryValues += "\'," + CAtomInt( ann.uiPort ).toString( );
			strQueryValues += "," + CAtomLong( ann.iUploaded ).toString( );
			strQueryValues += "," + CAtomLong( ann.iDownloaded ).toString( );
			strQueryValues += "," + CAtomLong( ann.iLeft ).toString( );
			strQueryValues += ",NOW(),NOW()";
			if( ann.iLeft > 0 )
				strQueryValues += ",0";
			else
				strQueryValues += ",1";
			strQueryValues += ",0,0";

			// Announce key support
			if( m_bAnnounceKeySupport )
			{
				if( gbDebug )
					if( gucDebugLevel & DEBUG_TRACKER )
						UTIL_LogPrint( "Announce: key support (enabled)\n" );
					
				strQuery += ",bkey";
				strQueryValues += ",\'" + UTIL_StringToMySQL( ann.strKey ) + "\'";
			}

			// Peer information support
			if( m_ucShowPeerInfo != 0 )
			{
				if( gbDebug )
					if( gucDebugLevel & DEBUG_TRACKER )
						UTIL_LogPrint( "Announce: peer info (enabled)\n" );
				
				strQuery += ",buseragent";
				strQueryValues += ",\'" + UTIL_StringToMySQL( ann.strUserAgent ) + "\'";
			}
			strQuery += ")";
			strQueryValues += ")";
			CMySQLQuery mq01( strQuery + strQueryValues );
//			CMySQLQuery mq02( "UPDATE allowed SET bupdated=NOW() WHERE bid=" + ann.strID );
			if( ann.iLeft == 0 )
			{
				if( bIPv6Announce )
				{
					m_pCache->setActive( ann.strID, SET_SEEDER_ADD_BOTH );
					CMySQLQuery mq03( "UPDATE allowed SET bseeders=bseeders+1,bseeders6=bseeders6+1,bupdated=NOW() WHERE bid=" + ann.strID );
				}
				else
				{
					m_pCache->setActive( ann.strID, SET_SEEDER_ADD );
					CMySQLQuery mq03( "UPDATE allowed SET bseeders=bseeders+1,bupdated=NOW() WHERE bid=" + ann.strID );
				}
//				m_pCache->setUserStatus( ann.strUID, SET_USER_SEEDING_ADD );
				CMySQLQuery mq04( "UPDATE users SET bseeding=bseeding+1 WHERE buid=" + ann.strUID );
			}
			else
			{
				if( bIPv6Announce )
				{
					m_pCache->setActive( ann.strID, SET_LEECHER_ADD_BOTH );
					CMySQLQuery mq03( "UPDATE allowed SET bleechers=bleechers+1,bleechers6=bleechers6+1,bupdated=NOW() WHERE bid=" + ann.strID );


				}
				else
				{
					m_pCache->setActive( ann.strID, SET_LEECHER_ADD );
					CMySQLQuery mq03( "UPDATE allowed SET bleechers=bleechers+1,bupdated=NOW() WHERE bid=" + ann.strID );
				}
//				m_pCache->setUserStatus( ann.strUID, SET_USER_LEECHING_ADD );
				CMySQLQuery mq04( "UPDATE users SET bleeching=bleeching+1 WHERE buid=" + ann.strUID );
			}
			
			
			CMySQLQuery *pQuery = new CMySQLQuery( "SELECT buid,bid FROM statistics WHERE buid=" + ann.strUID + " AND bid="  + ann.strID );
		
			vector<string> vecQuery;
		
			vecQuery.reserve(2);

			vecQuery = pQuery->nextRow( );

			if( vecQuery.size( ) == 0 )
				CMySQLQuery mq03( "INSERT INTO statistics (buid,bid,busername,bip,buploaded,bdownloaded,bstarted,bcompleted,bdowntime,bseedtime) VALUES (" + ann.strUID + "," + ann.strID + ",\'" + UTIL_StringToMySQL( ann.strUsername ) + "\',\'" + UTIL_StringToMySQL( ann.strIP ) + "\',0,0,NOW(),0,0,0)" );
		
			delete pQuery;

			// Add to the unique peer list
			if( m_bCountUniquePeers )
			{
				if( gbDebug )
					if( gucDebugLevel & DEBUG_TRACKER )
						UTIL_LogPrint( "Announce: count unique peer (enabled)\n" );

				AddUniquePeer( ann.strIP );
			}
		}
		else
		{
			if( bIPv6Announce && !bIPv6 )
			{
				if( ann.iLeft == 0 )
				{
					m_pCache->setActive( ann.strID, SET_SEEDER_ADD_V6 );
					CMySQLQuery mq03( "UPDATE allowed SET bseeders6=bseeders6+1,bupdated=NOW() WHERE bid=" + ann.strID );
				}
				else
				{
					m_pCache->setActive( ann.strID, SET_LEECHER_ADD_V6 );
					CMySQLQuery mq03( "UPDATE allowed SET bleechers6=bleechers6+1,bupdated=NOW() WHERE bid=" + ann.strID );
				}
			}
		}		
		break;
	case EVENT_COMPLETED:
		if( bPeerFound )
		{
			if( gbDebug )
				if( gucDebugLevel & DEBUG_TRACKER )
					UTIL_LogPrint( "Announce: global completed list is good\n" );
			
			if( !bCompleted )
			{
				CMySQLQuery mq01( "UPDATE dstate SET bcompleted=1 WHERE bid=" + ann.strID + " AND buid=" + ann.strUID + " AND bpeerid=\'" + UTIL_StringToMySQL( ann.strPeerID ) + "\'" );
				CMySQLQuery mq02( "INSERT INTO peers (bid,busername,buid,bip,buploaded,bdownloaded,bcompleted) VALUES (" + ann.strID + ",\'" + UTIL_StringToMySQL( ann.strUsername ) + "\'," + ann.strUID + ",\'" + UTIL_StringToMySQL( ann.strIP ) + "\'," + CAtomLong( ann.iUploaded ).toString( ) + "," + CAtomLong( ann.iDownloaded ).toString( ) + ",NOW())" );
				CMySQLQuery mq03( "UPDATE statistics SET bcompleted=NOW() WHERE buid=" + ann.strUID + " AND bid=" + ann.strID );
				
				m_pCache->setCompleted( ann.strID, SET_COMPLETED_ADD );
				CMySQLQuery mq04( "UPDATE allowed SET bcompleted=bcompleted+1,bupdated=NOW() WHERE bid=" + ann.strID );
//					addBonus( ann.strID, ann.strUID );
				if( ann.iLeft == 0 )
				{
					if( bIPv6 )
					{
						m_pCache->setActive( ann.strID, SET_SEEDER_A_LEECHER_M_BOTH );
						CMySQLQuery mq02( "UPDATE allowed SET bseeders=bseeders+1,bseeders6=bseeders6+1,bleechers=bleechers-1,bleechers6=bleechers6-1 WHERE bid=" + ann.strID + " AND bleechers>0" );
					}
					else
					{
						m_pCache->setActive( ann.strID, SET_SEEDER_A_LEECHER_M );
						CMySQLQuery mq02( "UPDATE allowed SET bseeders=bseeders+1,bleechers=bleechers-1 WHERE bid=" + ann.strID + " AND bleechers>0" );
					}
//					m_pCache->setUserStatus( ann.strUID, SET_USER_SEEDING_A_LEECHING_M );
					CMySQLQuery mq03( "UPDATE users SET bseeding=bseeding+1,bleeching=bleeching-1 WHERE buid=" + ann.strUID + " AND bleeching>0" );
				}
			}


// 						if( gbDebug )
// 						{
// 							if( gucDebugLevel & DEBUG_TRACKER )
// 							{
// #if defined( WIN32 )
// 								UTIL_LogPrint( "Announce: updating completed (%I64d) count for (%s)\n", iCompleted + 1,  ann.strInfoHash.c_str( ) );
// #elif defined( __FREEBSD__ ) || defined( __OPENBSD__ ) || defined( __NETBSD__ )
// 								UTIL_LogPrint( "Announce: updating completed (%qd) count for (%s)\n", iCompleted + 1,  ann.strInfoHash.c_str( ) );
// #else
// 								UTIL_LogPrint( "Announce: updating completed (%lld) count for (%s)\n", iCompleted + 1,  ann.strInfoHash.c_str( ) );
// #endif
// 							}
// 						}
				
		}
		else
		{
			if( gbDebug )
				if( gucDebugLevel & DEBUG_TRACKER )
					UTIL_LogPrint( "Announce: ignored duplicate peer completed event\n" );
		
			bRespond = false;

			return;
		}

		break;
	case EVENT_STOPPED:
		if( bPeerFound )
		{
			if( gbDebug )
				if( gucDebugLevel & DEBUG_TRACKER )
					UTIL_LogPrint( "Announce: delete from peer list)\n" );

			// Delete the peer from the peers list
			CMySQLQuery mq01( "DELETE FROM dstate WHERE bid=" + ann.strID + " AND buid=" + ann.strUID + " AND bpeerid=\'" + UTIL_StringToMySQL( ann.strPeerID ) + "\'" );
			if( ann.iLeft == 0 )
			{
				if( bIPv6 )
				{
					m_pCache->setActive( ann.strID, SET_SEEDER_MINUS_BOTH );
					CMySQLQuery mq02( "UPDATE allowed SET bseeders=bseeders-1,bseeders6=bseeders6-1,bupdated=NOW() WHERE bid=" + ann.strID + " AND bseeders>0" );
				}
				else
				{
					m_pCache->setActive( ann.strID, SET_SEEDER_MINUS );
					CMySQLQuery mq02( "UPDATE allowed SET bseeders=bseeders-1,bupdated=NOW() WHERE bid=" + ann.strID + " AND bseeders>0" );
				}
//				m_pCache->setUserStatus( ann.strUID, SET_USER_SEEDING_MINUS );
				CMySQLQuery mq03( "UPDATE users SET bseeding=bseeding-1 WHERE buid=" + ann.strUID + " AND bseeding>0" );
			}
			else
			{
				if( bIPv6 )
				{
					m_pCache->setActive( ann.strID, SET_LEECHER_MINUS_BOTH );
					CMySQLQuery mq02( "UPDATE allowed SET bleechers=bleechers-1,bleechers6=bleechers6-1,bupdated=NOW() WHERE bid=" + ann.strID + " AND bleechers>0" );
				}

				else
				{
					m_pCache->setActive( ann.strID, SET_LEECHER_MINUS );
					CMySQLQuery mq02( "UPDATE allowed SET bleechers=bleechers-1,bupdated=NOW() WHERE bid=" + ann.strID + " AND bleechers>0" );
				}
//				m_pCache->setUserStatus( ann.strUID, SET_USER_LEECHING_MINUS );
				CMySQLQuery mq03( "UPDATE users SET bleeching=bleeching-1 WHERE buid=" + ann.strUID + " AND bleeching>0" );
			}

			// Remove the IP from the unique peers list
			if( m_bCountUniquePeers )
			{
				if( gbDebug )
					if( gucDebugLevel & DEBUG_TRACKER )
						UTIL_LogPrint( "Announce: remove unique peer (enabled)\n" );

				RemoveUniquePeer( ann.strIP );
			}
		}
		else
		{
			if( gbDebug )
				if( gucDebugLevel & DEBUG_TRACKER )
					UTIL_LogPrint( "Announce: ignored duplicate peer stopped event\n" );
			
			bRespond = false;

			return;
		}
		break;
	default:
		UTIL_LogPrint( "Announce: unknown event (%s)\n", ann.strEvent.c_str( ) );
		
		bRespond = false;

		return;
	}

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "Announce: event (%s) completed\n", ann.strEvent.c_str( ) );
}

void CTracker :: UpdateUserState( )
{
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "UpdateUserState: started\n" );
		
	CMySQLQuery mq01( "UPDATE users SET bseeding=0,bleeching=0" );
	
	CMySQLQuery mq02( "UPDATE users,(SELECT buid,COUNT(*) AS bseeding FROM dstate WHERE bleft=0 GROUP BY buid) AS seeding SET users.bseeding=seeding.bseeding WHERE users.buid=seeding.buid" );
	
	CMySQLQuery mq03( "UPDATE users,(SELECT buid,COUNT(*) AS bleeching FROM dstate WHERE bleft!=0 GROUP BY buid) AS leeching SET users.bleeching=leeching.bleeching WHERE users.buid=leeching.buid" );
	
	m_pCache->ResetUsers( );
}

// Refresh the fast cache
void CTracker :: RefreshFastCache( )
{
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "RefreshFastCache: started\n" );

	m_ulSeedersTotal = 0;
	m_ulLeechersTotal = 0;
	m_ulPeers = 0;

//	CMySQLQuery *pQuery = new CMySQLQuery( "SELECT SUM(bseeders),SUM(bleechers) FROM allowed" );

//	vector<string> vecQuery;
//	vecQuery.reserve(2);

//	vecQuery = pQuery->nextRow( );
//	
//	delete pQuery;

//	if( vecQuery.size( ) == 2 )
//	{
//		m_ulSeedersTotal = atoi( vecQuery[0].c_str( ) );
//		m_ulLeechersTotal = atoi( vecQuery[1].c_str( ) );
//	}

	unsigned long ulKeySize = 0;
		
	struct torrent_t *pTorrents = 0;
	
	if( m_pCache )
	{
		pTorrents = m_pCache->getCache( );
		ulKeySize = m_pCache->getSize( );
	}

	for( unsigned long ulKey = 0; ulKey < ulKeySize; ulKey++ )
	{
		m_ulSeedersTotal += pTorrents[ulKey].uiSeeders;
		m_ulLeechersTotal += pTorrents[ulKey].uiLeechers;
	}

	
	m_ulPeers = m_ulSeedersTotal + m_ulLeechersTotal;

	if( m_ulPeers > gtXStats.peer.iGreatest )
		gtXStats.peer.iGreatest = m_ulPeers;

	if( m_ulSeedersTotal > gtXStats.peer.iGreatestSeeds )
		gtXStats.peer.iGreatestSeeds = m_ulSeedersTotal;

	if( m_ulLeechersTotal > gtXStats.peer.iGreatestLeechers )
		gtXStats.peer.iGreatestLeechers = m_ulLeechersTotal;

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "RefreshFastCache: completed\n" );
	
// 	UpdateUserState( );
}

// Modified by =Xotic=
void CTracker :: serverResponseGET( struct request_t *pRequest, struct response_t *pResponse )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	unsigned char ucResponse = RESPONSE_INIT;

	if( pRequest->strURL == RESPONSE_STR_ANNOUNCE ) ucResponse = RESPONSE_ANNOUNCE;
	else if( !m_strCustomAnnounce.empty( ) && pRequest->strURL == m_strCustomAnnounce ) ucResponse = RESPONSE_ANNOUNCE;
	else if( pRequest->strURL == RESPONSE_STR_SCRAPE ) ucResponse = RESPONSE_SCRAPE;
	else if( !m_strCustomScrape.empty( ) && pRequest->strURL == m_strCustomScrape ) ucResponse = RESPONSE_SCRAPE;
	else if( pRequest->strURL == RESPONSE_STR_LOGIN_HTML ) ucResponse = RESPONSE_LOGIN;
	else if( pRequest->strURL == RESPONSE_STR_LOG_HTML ) ucResponse = RESPONSE_LOG;
	else if( pRequest->strURL == RESPONSE_STR_ADMIN_HTML ) ucResponse = RESPONSE_ADMIN;
	else if( pRequest->strURL == RESPONSE_STR_USERS_HTML ) ucResponse = RESPONSE_USERS;
	else if( pRequest->strURL == RESPONSE_STR_TAGS_HTML ) ucResponse = RESPONSE_TAGS;
	else if( pRequest->strURL == RESPONSE_STR_LANGUAGE_HTML ) ucResponse = RESPONSE_LANGUAGE;
	else if( pRequest->strURL == RESPONSE_STR_XSTATS_HTML ) ucResponse = RESPONSE_XSTATS;
	else if( pRequest->strURL == RESPONSE_STR_XTORRENT_HTML ) ucResponse = RESPONSE_XTORRENT;
	else if( !style.strName.empty( ) && pRequest->strURL == RESPONSE_STR_SEPERATOR + style.strName ) ucResponse = RESPONSE_CSS;
	else if( pRequest->strURL == RESPONSE_STR_SEPERATOR || pRequest->strURL == RESPONSE_STR_INDEX_HTML || pRequest->strURL == "/index.htm") ucResponse = RESPONSE_INDEX;
	else if( pRequest->strURL.substr( 0, 10 ) == RESPONSE_STR_TORRENTS ) ucResponse = RESPONSE_TORRENTS;
	else if( pRequest->strURL.substr( 0, 8 ) == RESPONSE_STR_OFFERS ) ucResponse = RESPONSE_OFFERS;
	else if( pRequest->strURL.substr( 0, 7 ) == RESPONSE_STR_FILES ) ucResponse = RESPONSE_FILES;
	else if( pRequest->strURL.substr( 0, 9 ) == RESPONSE_STR_USERBAR ) ucResponse = RESPONSE_USERBAR;
	else if( !imagefill.strName.empty( ) && pRequest->strURL == RESPONSE_STR_SEPERATOR + imagefill.strName ) ucResponse = RESPONSE_IMAGEFILL;
	else if( imagefill.strName.empty( ) && pRequest->strURL == "/imagebarfill.png" ) ucResponse = RESPONSE_IMAGEFILL;
	else if( !imagetrans.strName.empty( ) && pRequest->strURL == RESPONSE_STR_SEPERATOR + imagetrans.strName ) ucResponse = RESPONSE_IMAGETRANS;
	else if( imagetrans.strName.empty( ) && pRequest->strURL == "/imagebartrans.png" ) ucResponse = RESPONSE_IMAGETRANS;
	else if( pRequest->strURL == RESPONSE_STR_OFFER_HTML ) ucResponse = RESPONSE_OFFER;
	else if( pRequest->strURL == RESPONSE_STR_UPLOAD_HTML ) ucResponse = RESPONSE_UPLOAD;
	else if( pRequest->strURL == RESPONSE_STR_FILE_UPLOAD_HTML ) ucResponse = RESPONSE_FILE_UPLOAD;
	else if( pRequest->strURL == RESPONSE_STR_SUB_UPLOAD_HTML ) ucResponse = RESPONSE_SUB_UPLOAD;
	else if( pRequest->strURL == RESPONSE_STR_STATS_HTML ) ucResponse = RESPONSE_STATS;
	else if( pRequest->strURL == RESPONSE_STR_INFO_HTML ) ucResponse = RESPONSE_INFO;
	else if( pRequest->strURL == RESPONSE_STR_RULES_HTML ) ucResponse = RESPONSE_RULES;
	else if( pRequest->strURL == RESPONSE_STR_FAQ_HTML ) ucResponse = RESPONSE_FAQ;
	else if( pRequest->strURL == RESPONSE_STR_STAFF_HTML ) ucResponse = RESPONSE_STAFF;
	else if( pRequest->strURL == RESPONSE_STR_RANK_HTML ) ucResponse = RESPONSE_RANK;
	else if( pRequest->strURL == RESPONSE_STR_MESSAGES_HTML ) ucResponse = RESPONSE_MESSAGES;
	else if( pRequest->strURL == RESPONSE_STR_TALK_HTML ) ucResponse = RESPONSE_TALK;
	else if( pRequest->strURL == RESPONSE_STR_COMMENTS_HTML ) ucResponse = RESPONSE_COMMENTS;
	else if( pRequest->strURL == RESPONSE_STR_SIGNUP_HTML ) ucResponse = RESPONSE_SIGNUP;
	else if( pRequest->strURL == RESPONSE_STR_INVITE_HTML ) ucResponse = RESPONSE_INVITE;
	else if( pRequest->strURL == RESPONSE_STR_RECOVER_HTML ) ucResponse = RESPONSE_RECOVER;
	else if( pRequest->strURL == RESPONSE_STR_SIGNUP_SCHOOL_HTML ) ucResponse = RESPONSE_SIGNUP_SCHOOL;
	else if( pRequest->strURL == RESPONSE_STR_ANNOUNCEMENTS_HTML ) ucResponse = RESPONSE_ANNOUNCEMENTS;
	else if( pRequest->strURL == RESPONSE_STR_VOTES_HTML ) ucResponse = RESPONSE_VOTES;
	else if( pRequest->strURL == RESPONSE_STR_ROBOTS_TXT ) ucResponse = RESPONSE_ROBOTS;
	else if( pRequest->strURL == RESPONSE_STR_FAVICON_ICO ) ucResponse = RESPONSE_FAVICON;
	else if( pRequest->strURL == RESPONSE_STR_BENCODE_INFO ) ucResponse = RESPONSE_BENCODE;
// 	else if( !rssdump.strName.empty( ) && pRequest->strURL == RESPONSE_STR_SEPERATOR + rssdump.strName ) ucResponse = RESPONSE_RSS;
	else if( pRequest->strURL == RESPONSE_STR_RSS_HTML ) ucResponse = RESPONSE_RSS;
	else if( !rssdump.strName.empty( ) && pRequest->strURL == RESPONSE_STR_RSS_XSL ) ucResponse = RESPONSE_RSSXSL;
	else if( !xmldump.strName.empty( ) && pRequest->strURL == RESPONSE_STR_SEPERATOR + xmldump.strName ) ucResponse = RESPONSE_XML;
	else
	{
		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, "Request server: " + gmapLANG_CFG["server_response_404"], string( CSS_INDEX ), string( ), NOT_INDEX, CODE_404 );

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INDEX ) );

		gtXStats.page.iError404++;

		return;
	}

	switch( ucResponse )
	{
	case RESPONSE_ANNOUNCE:
		serverResponseAnnounce( pRequest, pResponse );
		gtXStats.announce.iAnnounce++;

		break;
	case RESPONSE_SCRAPE:
		serverResponseScrape( pRequest, pResponse );
		gtXStats.scrape.iScrape++;

		break;
	case RESPONSE_FAVICON:
		if( !favicon.strFile.empty( ) )
		{
			serverResponseIcon( pRequest, pResponse );
			gtXStats.file.iFavicon++;
		}
		else
		{
			pResponse->strCode = "404 " + gmapLANG_CFG["server_response_404"];  
			gtXStats.page.iError404++;
		}

		break;
	case RESPONSE_CSS:
		if( m_bServeLocal && !style.strFile.empty( ) )
		{
			serverResponseCSS( pRequest, pResponse );
			gtXStats.file.iCSS++;
		}
		else
		{
			pResponse->strCode = "404 " + gmapLANG_CFG["server_response_404"];  


			gtXStats.page.iError404++;
		}

		break;
	case RESPONSE_RSS:
		if( m_bServeLocal && !rssdump.strName.empty( ) && ( pRequest->user.ucAccess & m_ucAccessRSS ) )
		{
			serverResponseRSS( pRequest, pResponse );
			gtXStats.file.iRSS++;
		}
		else
		{
			pResponse->strCode = "404 " + gmapLANG_CFG["server_response_404"];  
			gtXStats.page.iError404++;
		}

		break;
	case RESPONSE_RSSXSL:
		if( m_bServeLocal && !rssdump.strName.empty( ) && ( pRequest->user.ucAccess & m_ucAccessRSS ) )
		{
			serverResponseRSSXSL( pRequest, pResponse );
			gtXStats.file.iRSSXSL++;
		}
		else
		{
			pResponse->strCode = "404 " + gmapLANG_CFG["server_response_404"];  
			gtXStats.page.iError404++;
		}

		break;
	case RESPONSE_XML:
		if( m_bServeLocal && !xmldump.strFile.empty( ) && ( pRequest->user.ucAccess & m_ucAccessDumpXML ) )
		{
			serverResponseXML( pRequest, pResponse );
			gtXStats.file.iXML++;
		}
		else
		{
			pResponse->strCode = "404 " + gmapLANG_CFG["server_response_404"];  
			gtXStats.page.iError404++;
		}

		break;
	case RESPONSE_ROBOTS:
		if( m_bServeLocal && !robots.strFile.empty( ) )
		{
			serverResponseRobots( pRequest, pResponse );
			gtXStats.file.iRobots++;
		}
		else
		{
			pResponse->strCode = "404 " + gmapLANG_CFG["server_response_404"];  
			gtXStats.page.iError404++;
		}

		break;
	case RESPONSE_INDEX:
		serverResponseIndex( pRequest, pResponse );
		gtXStats.page.iIndex++;

		break;
	case RESPONSE_STATS:
		serverResponseStatsGET( pRequest, pResponse );
		gtXStats.page.iStats++;

		break;
	case RESPONSE_LOGIN:
		serverResponseLoginGET( pRequest, pResponse );
		gtXStats.page.iLogin++;

		break;

	case RESPONSE_SIGNUP:
		serverResponseSignupGET( pRequest, pResponse );
		gtXStats.page.iSignup++;
		
		break;
	case RESPONSE_INVITE:
		serverResponseInviteGET( pRequest, pResponse );
		gtXStats.page.iSignup++;

		break;
	case RESPONSE_RECOVER:
		serverResponseRecoverGET( pRequest, pResponse );
//		gtXStats.page.iSignup++;


		break;
	case RESPONSE_SIGNUP_SCHOOL:
		serverResponseSignupSchoolGET( pRequest, pResponse );
		gtXStats.page.iSignup++;

		break;
	case RESPONSE_OFFER:
		serverResponseOfferGET( pRequest, pResponse );

		gtXStats.page.iOffer++;

		break;
	case RESPONSE_UPLOAD:
		serverResponseUploadGET( pRequest, pResponse );
		gtXStats.page.iUpload++;

		break;
	case RESPONSE_FILE_UPLOAD:
		serverResponseFileUploadGET( pRequest, pResponse );
// 		gtXStats.page.iUpload++;

		break;
	case RESPONSE_SUB_UPLOAD:
		serverResponseSubUploadGET( pRequest, pResponse );
// 		gtXStats.page.iUpload++;

		break;
	case RESPONSE_INFO:
		serverResponseInfoGET( pRequest, pResponse );
		gtXStats.page.iInfo++;

		break;
	case RESPONSE_RULES:
		serverResponseRulesGET( pRequest, pResponse );
		gtXStats.page.iRules++;

		break;
	case RESPONSE_FAQ:
		serverResponseFAQGET( pRequest, pResponse );
		gtXStats.page.iFAQ++;

		break;
	case RESPONSE_STAFF:
		serverResponseStaff( pRequest, pResponse );
// 		gtXStats.page.iFAQ++;

		break;
	case RESPONSE_RANK:
		serverResponseRank( pRequest, pResponse );

		break;
	case RESPONSE_MESSAGES:
		serverResponseMessagesGET( pRequest, pResponse );
		gtXStats.page.iMessages++;

		break;
	case RESPONSE_TALK:
		serverResponseTalkGET( pRequest, pResponse );
//		gtXStats.page.iMessages++;

		break;
	case RESPONSE_LOG:
		serverResponseLog( pRequest, pResponse );
// 		gtXStats.page.iAdmin++;

		break;
	case RESPONSE_ADMIN:
		serverResponseAdmin( pRequest, pResponse );
		gtXStats.page.iAdmin++;

		break;
	case RESPONSE_USERS:
		serverResponseUsersGET( pRequest, pResponse );
		gtXStats.page.iUsers++;


		break;
	case RESPONSE_COMMENTS:
		serverResponseCommentsGET( pRequest, pResponse );
		gtXStats.page.uiComments++;

		break;
	case RESPONSE_TORRENTS:
		serverResponseTorrent( pRequest, pResponse );
		gtXStats.page.iTorrents++;

		break;
	case RESPONSE_OFFERS:
		serverResponseOffer( pRequest, pResponse );
// 		gtXStats.page.iTorrents++;

		break;
	case RESPONSE_ANNOUNCEMENTS:
		serverResponseAnnouncementsGET( pRequest, pResponse );

		break;
	case RESPONSE_VOTES:
		serverResponseVotesGET( pRequest, pResponse );

		break;
	case RESPONSE_TAGS:
		serverResponseTags( pRequest, pResponse );
		gtXStats.page.iTags++;

		break;
	case RESPONSE_LANGUAGE:
		serverResponseLanguage( pRequest, pResponse );
		gtXStats.page.iLanguage++;

		break;	
	case RESPONSE_XSTATS:
		serverResponseXStats( pRequest, pResponse );
		gtXStats.page.iXStats++;

		break;
	case RESPONSE_XTORRENT:
		serverResponseXTorrent( pRequest, pResponse );
		gtXStats.page.iXTorrent++;

		break;
	case RESPONSE_FILES:
		serverResponseFile( pRequest, pResponse );
		gtXStats.file.uiFiles++;


		break;
	case RESPONSE_USERBAR:
		serverResponseUserbar( pRequest, pResponse );

		break;
	case RESPONSE_IMAGEFILL:
		if( m_bServeLocal && !imagefill.strFile.empty( ) && ( pRequest->user.ucAccess & m_ucAccessView ) )
		{
			serverResponseImagefill( pRequest, pResponse );
			gtXStats.file.iBarFill++;
		}
		else
		{
			pResponse->strCode = "404 " + gmapLANG_CFG["server_response_404"];  
			gtXStats.page.iError404++;
		}

		break;
	case RESPONSE_IMAGETRANS:
		if( m_bServeLocal && !imagetrans.strFile.empty( ) && ( pRequest->user.ucAccess & m_ucAccessView ) )
		{
			serverResponseImagetrans( pRequest, pResponse );
			gtXStats.file.iBarTrans++;
		}
		else
		{
			pResponse->strCode = "404 " + gmapLANG_CFG["server_response_404"];  
			gtXStats.page.iError404++;
		}

		break;
	case RESPONSE_BENCODE:
		serverResponseBencodeInfo( pRequest, pResponse );

		break;
	default:
		pResponse->strCode = "404 " + gmapLANG_CFG["server_response_404"];  
		gtXStats.page.iError404++;
	}
}

// Server Responses
void CTracker :: serverResponsePOST( struct request_t *pRequest, struct response_t *pResponse, CAtomList *pPost )

{
	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
	{
		// Set the start time
		const struct bnbttv btv( UTIL_CurrentTime( ) );

		if( IsIPBanned( pRequest, pResponse, btv, "IP Banned", string( CSS_INDEX ), NOT_INDEX ) )
			return;
	}

	if( pPost )
	{
		if( pRequest->strURL == RESPONSE_STR_UPLOAD_HTML )
			serverResponseUploadPOST( pRequest, pResponse, pPost );
		else if( pRequest->strURL == RESPONSE_STR_STATS_HTML )
			serverResponseStatsPOST( pRequest, pResponse, pPost );
		else if( pRequest->strURL == RESPONSE_STR_COMMENTS_HTML )
			serverResponseCommentsPOST( pRequest, pResponse, pPost );
		else if( pRequest->strURL == RESPONSE_STR_TALK_HTML )
			serverResponseTalkPOST( pRequest, pResponse, pPost );
		else if( pRequest->strURL == RESPONSE_STR_RULES_HTML )
			serverResponseRulesPOST( pRequest, pResponse, pPost );
		else if( pRequest->strURL == RESPONSE_STR_FAQ_HTML )
			serverResponseFAQPOST( pRequest, pResponse, pPost );
		else if( pRequest->strURL == RESPONSE_STR_MESSAGES_HTML )
			serverResponseMessagesPOST( pRequest, pResponse, pPost );
//		else if( pRequest->strURL == RESPONSE_STR_INFO_HTML )
//			serverResponseInfoPOST( pRequest, pResponse, pPost );
		else if( pRequest->strURL == RESPONSE_STR_LOGIN_HTML )
			serverResponseLoginPOST( pRequest, pResponse, pPost );
		else if( pRequest->strURL == RESPONSE_STR_USERS_HTML )
			serverResponseUsersPOST( pRequest, pResponse, pPost );
		else if( pRequest->strURL == RESPONSE_STR_SIGNUP_HTML )
			serverResponseSignupPOST( pRequest, pResponse, pPost );
		else if( pRequest->strURL == RESPONSE_STR_INVITE_HTML )
			serverResponseInvitePOST( pRequest, pResponse, pPost );
		else if( pRequest->strURL == RESPONSE_STR_RECOVER_HTML )
			serverResponseRecoverPOST( pRequest, pResponse, pPost );
		else if( pRequest->strURL == RESPONSE_STR_SIGNUP_SCHOOL_HTML )
			serverResponseSignupSchoolPOST( pRequest, pResponse, pPost );
		else if( pRequest->strURL == RESPONSE_STR_ANNOUNCEMENTS_HTML )
			serverResponseAnnouncementsPOST( pRequest, pResponse, pPost );
		else if( pRequest->strURL == RESPONSE_STR_VOTES_HTML )
			serverResponseVotesPOST( pRequest, pResponse, pPost );
		else
			pResponse->strCode = "404 " + gmapLANG_CFG["server_response_404"];
	}
	else
		pResponse->strCode = "500 " + gmapLANG_CFG["server_response_500"];
}

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
	const string strMatch( pRequest->mapParams["match"] );
	
	const string strChannelTag( pRequest->mapParams["tag"] );
	const string strMedium( pRequest->mapParams["medium"] );
	const string strQuality( pRequest->mapParams["quality"] );
	const string strEncode( pRequest->mapParams["encode"] );
	
	const string cstrDay( pRequest->mapParams["day"] );
	const string cstrPasskey( pRequest->mapParams["passkey"] );
	
	vector<string> vecSearch;
	vecSearch.reserve(64);
	vector<string> vecUploader;
	vecUploader.reserve(64);
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
			if( m_bAllowTorrentDownloads )
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

void CTracker :: serverResponseCSS( struct request_t *pRequest, struct response_t *pResponse )
{
	// Verify that the IP is permitted to access the tracker
// 	if( m_ucIPBanMode != 0 )
// 	{
// 		// Set the start time
// 		const struct bnbttv btv( UTIL_CurrentTime( ) );
// 
// 		if( IsIPBanned( pRequest, pResponse, btv, "IP Banned", string( CSS_INDEX ), NOT_INDEX ) )
// 			return;
// 	}

	pResponse->strCode = "200 " + gmapLANG_CFG["server_response_200"];

	pResponse->mapHeaders.insert( pair<string, string>( "Content-Type", gmapMime[".css"] ) );

	pResponse->strContent = style.strFile;
}

void CTracker :: serverResponseImagefill( struct request_t *pRequest, struct response_t *pResponse )
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

	pResponse->mapHeaders.insert( pair<string, string>( "Content-Type", gmapMime[imagefill.strExt] ) );

	pResponse->bCompressOK = false;

	pResponse->strContent = imagefill.strFile;
}

void CTracker :: serverResponseImagetrans( struct request_t *pRequest, struct response_t *pResponse )
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

	pResponse->mapHeaders.insert( pair<string, string>( "Content-Type", gmapMime[imagetrans.strExt] ) );

	pResponse->bCompressOK = false;

	pResponse->strContent = imagetrans.strFile;
}

void CTracker :: serverResponseIcon( struct request_t *pRequest, struct response_t *pResponse )
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

	pResponse->mapHeaders.insert( pair<string, string>( "Content-Type", gmapMime[favicon.strExt] ) );

	pResponse->bCompressOK = false;

	pResponse->strContent = string( favicon.strFile );
}

void CTracker :: serverResponseRobots( struct request_t *pRequest, struct response_t *pResponse )
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

	pResponse->mapHeaders.insert( pair<string, string>( "Content-Type", gmapMime[".txt"] ) );

	pResponse->strContent = robots.strFile;
}

void CTracker :: serverResponseBencodeInfo( struct request_t *pRequest, struct response_t *pResponse )
{

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
	{
		// Set the start time
		const struct bnbttv btv( UTIL_CurrentTime( ) );

		if( IsIPBanned( pRequest, pResponse, btv, "IP Banned", string( CSS_INDEX ), NOT_INDEX ) )
			return;
	}

	pResponse->strCode = "200 OK";

	pResponse->mapHeaders.insert( pair<string, string>( "Content-Type", gmapMime[".txt"] ) );

	CAtomDicti *pData = new CAtomDicti( );

	pData->setItem( "version", new CAtomString( string( XBNBT_VER ) ) );

	CMySQLQuery *pQuery = new CMySQLQuery( "SELECT COUNT(*) FROM allowed" );
	
	vector<string> vecQuery;

	vecQuery.reserve(1);

	vecQuery = pQuery->nextRow( );

	delete pQuery;

	pData->setItem( "files", new CAtomString( vecQuery[0] ) );
	
	unsigned long ulPeers = 0;

	CMySQLQuery *pQueryPeer = new CMySQLQuery( "SELECT COUNT(*) FROM dstate" );
	
	vector<string> vecQueryPeer;

	vecQueryPeer.reserve(1);

	vecQueryPeer = pQueryPeer->nextRow( );

	delete pQueryPeer;

	pData->setItem( "peers", new CAtomString( vecQueryPeer[0] ) );
	
	CMySQLQuery *pQueryIP = new CMySQLQuery( "SELECT COUNT(*) FROM ips WHERE bcount>0" );
				
	vector<string> vecQueryIP;

	vecQueryIP.reserve(1);

	vecQueryIP = pQueryIP->nextRow( );

	delete pQueryIP;

	if( m_bCountUniquePeers )
		pData->setItem( "unique", new CAtomString( vecQueryIP[0] ) );

	pResponse->strContent = Encode( pData );
	pResponse->bCompressOK = false;

	delete pData;
}

// The main update routine
void CTracker :: Update( )
{
	if( m_uiTimerInterval != 0 && GetTime( ) > m_ulTimerNext )
	{
		if( gbDebug )
			if( gucDebugLevel & DEBUG_TRACKER )
				UTIL_LogPrint( "timer - timer task\n" );

		time_t tNow = time( 0 );
		char pTime[256];
		memset( pTime, 0, sizeof( pTime ) / sizeof( char ) );
		strftime( pTime, sizeof( pTime ) / sizeof( char ), "%H", localtime( &tNow ) );
		
		if( string( pTime ) == "00" )
			CMySQLQuery mq01( "DELETE FROM talktorrent WHERE bposted<NOW()-INTERVAL 1 DAY" );

		m_ulTimerNext = GetTime( ) + m_uiTimerInterval;
	}

	// Should we save the dfile?
	if( GetTime( ) > m_ulGetSeedBonusNext )
	{
		if( gbDebug )
			if( gucDebugLevel & DEBUG_TRACKER )
				UTIL_LogPrint( "CTracker: adding seedbonus\n" );
			
		CMySQLQuery mq01( "UPDATE users SET bseedbonus=0.00" );
		
		CMySQLQuery *pQuerySeeding = new CMySQLQuery( "SELECT bid,buid,bupspeed FROM dstate WHERE bleft=0 ORDER BY buid" );
		
		vector<string> vecQuerySeeding;
	
		vecQuerySeeding.reserve(3);

		vecQuerySeeding = pQuerySeeding->nextRow( );

		string strUID = string( );
		time_t now_t = GetTime( );
		
		int uiSeeding = 0;
		float flSeedBonus = 0.0;

		while( vecQuerySeeding.size( ) == 3 )
		{
			if( strUID.empty( ) || strUID == vecQuerySeeding[1] )
			{
				if( strUID.empty( ) )
				{
					strUID = vecQuerySeeding[1];
					uiSeeding = 0;
					flSeedBonus = 0.0;
				}

				string strAdded = string( );
				int64 iSize = 0;
				int64 ulUpSpeed = 0;
				int uiSeeders = 0;
				
				float passed = 0;
			
				CMySQLQuery *pQuery = new CMySQLQuery( "SELECT badded,bsize,bseeders FROM allowed WHERE bid=" + vecQuerySeeding[0] );
				
				vector<string> vecQuery;
				
				vecQuery.reserve(3);

				vecQuery = pQuery->nextRow( );
				
				if( vecQuery.size( ) == 3 )
				{
					if( !vecQuery[0].empty( ) )
						strAdded = vecQuery[0];

					if( !vecQuery[1].empty( ) )
						iSize = UTIL_StringTo64( vecQuery[1].c_str( ) );
					
					if( !vecQuery[2].empty( ) )
						uiSeeders = atoi( vecQuery[2].c_str( ) );
						
					struct tm *now_tm, time_tm;
					int64 year, month, day, hour, minute, second;
					
					sscanf( strAdded.c_str( ), "%d-%d-%d %d:%d:%d",&year,&month,&day,&hour,&minute,&second );
					time_tm.tm_year = year-1900;
					time_tm.tm_mon = month-1;
					time_tm.tm_mday = day;
					time_tm.tm_hour = hour;
					time_tm.tm_min = minute;
					time_tm.tm_sec = second;
					passed = difftime(now_t, mktime(&time_tm));
					
					if( !vecQuerySeeding[2].empty( ) && uiSeeders > 0 )
					{
						ulUpSpeed = UTIL_StringTo64( vecQuerySeeding[2].c_str( ) );
						if( uiSeeders > 0 )
						{
							uiSeeding++;
							flSeedBonus += ( 1.0 - 1.0 / pow( 10, passed / 3600 / 24 / 7 / 4 ) ) * ( 1.5 - 0.5 / pow( 10, ulUpSpeed / 1024.0 / 1024 / 10 ) ) * sqrt( iSize / 1024.0 / 1024 / 1024 ) * ( 1 + sqrt( 2 ) / pow( 10, ( uiSeeders - 1 ) / ( 10.0 - 1.0 ) ) ) / 100;
						}
// 						flSeedBonus += sqrt( passed / 3600 / 24 ) * ( iSize / 1024.0 / 1024 / 1024 ) * ( 1 + 5 * 2 * atan( ulUpSpeed / 1024.0 / 1024 ) / Pi ) / uiSeeders / uiSeeders / 5000;
					}
				}
				delete pQuery;
				
				vecQuerySeeding = pQuerySeeding->nextRow( );
			}
			else
			{
				if( uiSeeding >20 )
					uiSeeding = 20;
				flSeedBonus = uiSeeding + 80 * 2 * atan( flSeedBonus ) / Pi;
				char szFloat[16];
				memset( szFloat, 0, sizeof( szFloat ) / sizeof( char ) );

				snprintf( szFloat, sizeof( szFloat ) / sizeof( char ), "%0.2f", flSeedBonus );
				
				string strQuery = "UPDATE users SET bseedbonus=";
				strQuery += szFloat;
				strQuery += ",bbonus=bbonus+" + CAtomLong( flSeedBonus * m_uiGetSeedBonusInterval * 100.0 / 3600 ).toString( ) + " WHERE buid=" + strUID;
				
				CMySQLQuery mq02( strQuery );

				
				strUID.erase( );
			}
		}
		
		if( !strUID.empty( ) )
		{
			if( uiSeeding >20 )
				uiSeeding = 20;
			flSeedBonus = uiSeeding + 80 * 2 * atan( flSeedBonus ) / Pi;
			char szFloat[16];
			memset( szFloat, 0, sizeof( szFloat ) / sizeof( char ) );
			snprintf( szFloat, sizeof( szFloat ) / sizeof( char ), "%0.2f", flSeedBonus );
		
			string strQuery = "UPDATE users SET bseedbonus=";
			strQuery += szFloat;
			strQuery += ",bbonus=bbonus+" + CAtomLong( flSeedBonus * m_uiGetSeedBonusInterval * 100.0 / 3600 ).toString( ) + " WHERE buid=" + strUID;
		
			CMySQLQuery mq02( strQuery );
		}
		
		delete pQuerySeeding;
		
		m_pCache->ResetUsers( );

		m_ulGetSeedBonusNext = GetTime( ) + m_uiGetSeedBonusInterval;
	}

// 	if( GetTime( ) > m_ulRefreshConfigNext )
// 	{
// 		if( gbDebug )
// 			if( gucDebugLevel & DEBUG_TRACKER )
// 				UTIL_LogPrint( "CTracker: refreshing config files\n" );
// 			
// 		// Main Configuration file
// 		CFG_Open( CFG_FILE );
// 		CFG_SetDefaults( );
// 		CFG_Close( CFG_FILE );
// 		
// 		// Language Configuration file
// 		LANG_CFG_Init( LANG_CFG_FILE );
// 		
// 		m_ulRefreshConfigNext = GetTime( ) + m_uiRefreshConfigInterval * 60;
// 	}
	
#define STR_SEPARATORS "\n\r"	
 	if( GetTime( ) > m_ulRefreshCBTTListNext )
 	{
 		CBTTParseList( );
 		
 		m_ulRefreshCBTTListNext = GetTime( ) + m_uiRefreshCBTTListInterval * 60;
 	}

	if( GetTime( ) > m_ulRefreshIMDbNext )
	{
		GetIMDbLoop( );
		
		m_ulRefreshIMDbNext = GetTime( ) + m_uiRefreshIMDbInterval * 60 * 60;
	}
	// Should we refresh the static files?
	if( GetTime( ) > m_ulRefreshStaticNext )
	{
		RefreshStatic( );

		m_ulRefreshStaticNext = GetTime( ) + m_uiRefreshStaticInterval * 60;
	}

	// Expire downloaders
	if( GetTime( ) > m_ulDownloaderTimeOutNext )
	{
		if( gbDebug )
			if( gucDebugLevel & DEBUG_TRACKER )
				UTIL_LogPrint( ( gmapLANG_CFG["expiring_downloaders"] + "\n" ).c_str( ) );

		expireDownloaders( );
		
		m_ulDownloaderTimeOutNext = GetTime( ) + m_uiDownloaderTimeOutInterval;
	}

	// Dump XML
// 	if( !xmldump.strName.empty( ) && GetTime( ) > m_ulDumpXMLNext )
// 	{
// 		if( gbDebug )
// 			if( gucDebugLevel & DEBUG_TRACKER )
// 				UTIL_LogPrint( "CTracker: dumping xml\n" );
// 
// 		saveXML( );
// 
// 		m_ulDumpXMLNext = GetTime( ) + m_uiDumpXMLInterval;
// 	}

	// Dump XStats
	if( !statsdump.strName.empty( ) && GetTime( ) > m_ulXStatsNext )
	{
		if( gbDebug )
			if( gucDebugLevel & DEBUG_TRACKER )
				UTIL_LogPrint( ( gmapLANG_CFG["dumping_stats"] + "\n" ).c_str( ) );

		saveXStats( );

		m_ulXStatsNext = GetTime( ) + m_uiXStatsInterval;
	}

	// queued announces
//	if( gpLinkServer || gpLink || gpHUBLinkServer || gpHUBLink )
//	{
//		m_mtxQueued.Claim( );
//		vector<struct announce_t> vecTemp;
//		vecTemp.reserve(guiMaxConns);
//		vecTemp = m_vecQueued;
//		m_vecQueued.clear( );
//		m_mtxQueued.Release( );
//		bool bRespond = true;

//		for( vector<struct announce_t> :: iterator it = vecTemp.begin( ); it != vecTemp.end( ); it++ )
//		{

//			CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bfilename FROM allowed WHERE bhash=\'" + UTIL_StringToMySQL( (*it).strInfoHash ) + "\'" );
//						
//			vector<string> vecQuery;
//			
//			vecQuery.reserve(1);

//			vecQuery = pQuery->nextRow( );
//			
//			if( vecQuery.size( ) == 0 )
//				continue;

//			Announce( *it, bRespond );
//			
//			delete pQuery;
//		}
//	}

#if defined ( XBNBT_GD )
	// Dynstat Image Generator
	if( m_bDynstatGenerate && m_uiDynstatDumpInterval && GetTime( ) > m_ulDynstatDumpNext )
	{
		if( gbDebug )
			if( gucDebugLevel & DEBUG_TRACKER )
				UTIL_LogPrint( "CTracker: Parse of Dynstat: Start\n" );

		runGenerateDynstat( );

		if( gbDebug )
			if( gucDebugLevel & DEBUG_TRACKER )
				UTIL_LogPrint( "CTracker: Parse of Dynstat: Completed\n" );
	}
#endif
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

// XBNBT IP Banning
const bool CTracker :: IsIPBanned( struct request_t *pRequest, struct response_t *pResponse, const struct bnbttv &btv, const string &cstrPageLabel, const string &cstrCSSLabel, const bool &bIsIndex )
{
	// retrieve ip

	switch( m_ucIPBanMode )
	{
	case IP_BLACKLIST:
		if( UTIL_IsIPBanList( pRequest->strIP, m_pIPBannedList ) )
		{
			// Forbidden

			// Output common HTML head
			HTML_Common_Begin(  pRequest, pResponse, cstrPageLabel, cstrCSSLabel, string( ), bIsIndex, CODE_403 );

			// Output common HTML tail
			HTML_Common_End( pRequest, pResponse, btv, bIsIndex, cstrCSSLabel );

			return true;
		}

		break;
	case IP_VIPLIST:
		if( !UTIL_IsIPBanList( pRequest->strIP, m_pIPBannedList ) )
		{
			// Forbidden

			// Output common HTML head
			HTML_Common_Begin(  pRequest, pResponse, cstrPageLabel, cstrCSSLabel, string( ), bIsIndex, CODE_403 );

			// Output common HTML tail
			HTML_Common_End( pRequest, pResponse, btv, bIsIndex, cstrCSSLabel );

			return true;
		}

		break;
	default:
		// IP Banning off
		;
	}

	return false;
}

// Initialise XBNBT tags for internal mouseover
void CTracker :: initTags( )

{
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "initTags: started\n" );

	unsigned char ucTag = 1;
	unsigned int ucTag_Index = ucTag * 100 + 1;

	string strName = "bnbt_tag" + CAtomInt( ucTag_Index ).toString( );
	string strTag = CFG_GetString( strName, string( ) );
	string strQualities = CFG_GetString( "bnbt_qualities", string( ) );
	string strMediums = CFG_GetString( "bnbt_mediums", string( ) );
	string strEncodes = CFG_GetString( "bnbt_encodes", string( ) );

	m_vecTags.reserve(64);
	m_vecTagsMouse.reserve(64);
	
	m_vecQualities.reserve(64);
	m_vecMediums.reserve(64);
	m_vecEncodes.reserve(64);

	string :: size_type iSplit = 0;
	string :: size_type iSplitNext = 0;

	while( !strTag.empty( ) )
	{
		iSplit = strTag.find( "|" );
		iSplitNext = strTag.find( "|", iSplit + 1 );

		if( iSplit == string :: npos ) 
		{
			if( iSplitNext == string :: npos )
			{
				m_vecTags.push_back( pair<string, string>( CAtomInt( ucTag_Index ).toString( ), strTag ) );
				m_vecTagsMouse.push_back( pair<string, string>( CAtomInt( ucTag_Index ).toString( ), string( ) ) );
			}
		}
		else
		{
			if( iSplitNext == string :: npos )
			{
				m_vecTags.push_back( pair<string, string>( CAtomInt( ucTag_Index ).toString( ), strTag.substr( 0, iSplit ) ) );
				m_vecTagsMouse.push_back( pair<string, string>( CAtomInt( ucTag_Index ).toString( ), strTag.substr( iSplit + 1 ) ) );
			}
			else
			{
				m_vecTags.push_back( pair<string, string>( CAtomInt( ucTag_Index ).toString( ), strTag.substr( 0, iSplit ) ) );
				m_vecTagsMouse.push_back( pair<string, string>( CAtomInt( ucTag_Index ).toString( ), strTag.substr( iSplit + 1, iSplitNext - iSplit - 1 ) ) );
			}
		}
		
		strName = "bnbt_tag" + CAtomInt( ++ucTag_Index ).toString( );
		strTag = CFG_GetString( strName, string( ) );
		if( strTag.empty( ) )
		{
			ucTag_Index = ++ucTag * 100 + 1;
			strName = "bnbt_tag" + CAtomInt( ucTag_Index ).toString( );
			strTag = CFG_GetString( strName, string( ) );
		}
			
	}
	
	iSplit = 0;
	iSplitNext = 0;
	
	while( iSplitNext != string :: npos )
	{
		iSplitNext = strQualities.find( "|", iSplit + 1 );
		m_vecQualities.push_back( strQualities.substr( iSplit, iSplitNext - iSplit ) );
		iSplit = iSplitNext + 1;
	}
	
	iSplit = 0;
	iSplitNext = 0;
	
	while( iSplitNext != string :: npos )
	{
		iSplitNext = strMediums.find( "|", iSplit + 1 );
		m_vecMediums.push_back( strMediums.substr( iSplit, iSplitNext - iSplit ) );
		iSplit = iSplitNext + 1;
	}
	
	iSplit = 0;
	iSplitNext = 0;
	
	while( iSplitNext != string :: npos )
	{
		iSplitNext = strEncodes.find( "|", iSplit + 1 );
		m_vecEncodes.push_back( strEncodes.substr( iSplit, iSplitNext - iSplit ) );
		iSplit = iSplitNext + 1;
	}

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "initTags: completed\n" );
}

void CTracker :: initShareRatio( )
{
	string strDownloaded = CFG_GetString( "bnbt_share_control_down", "300|200|120|60|30|0" );
	string strRatio = CFG_GetString( "bnbt_share_control_ratio", "0.7|0.6|0.5|0.4|0.3|-2" );
	
	string :: size_type iSplit = 0;
	string :: size_type iSplitNext = 0;
	
	unsigned char ucCount = 0;
	
	while( iSplitNext != string :: npos )
	{
		iSplitNext = strDownloaded.find( "|", iSplit + 1 );
		RequiredDown[ucCount] = atoi( strDownloaded.substr( iSplit, iSplitNext - iSplit ).c_str( ) );
		iSplit = iSplitNext + 1;
		ucCount++;
	}
	
	iSplit = 0;
	iSplitNext = 0;
	ucCount = 0;
	
	while( iSplitNext != string :: npos )
	{
		iSplitNext = strRatio.find( "|", iSplit + 1 );
		RequiredRatio[ucCount] = atof( strRatio.substr( iSplit, iSplitNext - iSplit ).c_str( ) );
		iSplit = iSplitNext + 1;
		ucCount++;
	}
}

// Javascrpt

// Javascript for Return to page
void CTracker :: JS_ReturnToPage( struct request_t *pRequest, struct response_t *pResponse, const string &cstrPageParameters )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
		if( IsIPBanned( pRequest, pResponse, btv, string( ), string( CSS_INDEX ), NOT_INDEX ) )
			return;

	// Output common HTML head
	HTML_Common_Begin(  pRequest, pResponse, string( ), string( CSS_INDEX ), string( ), NOT_INDEX, CODE_200 );

	// Run Script
	if( !cstrPageParameters.empty( ) )
		pResponse->strContent += "<script type=\"text/javascript\">window.location=\"/" + cstrPageParameters + "\"</script>\n\n";
	else
	{
		pResponse->strContent += "<div>Internal Return To Page Error!</div>\n\n";
		UTIL_LogPrint( "CTracker: JS_ReturnToPage: Internal Return To Page Error!\n" );
	}

	// Output common HTML tail
	HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INDEX ) );
}

// The navigation bar
void CTracker :: HTML_Nav_Bar( struct request_t *pRequest, struct response_t *pResponse, const string &cstrCSS, bool bNewIndex, bool bNewOffer  )
{
	if( !cstrCSS.empty( ) )
	{
		// This funtion builds a navbar
		// start navbar
		pResponse->strContent += "<div class=\"navbar_" + cstrCSS + "\">\n";
		pResponse->strContent += "<table class=\"navbar\">\n";
		pResponse->strContent += "<tr class=\"navbar\">\n";

		// Index (RTT)
		if( !pRequest->user.strUID.empty( ) && ( pRequest->user.ucAccess & m_ucAccessViewTorrents ) )
		{
			pResponse->strContent += "<td class=\"navbar_index\"><a title=\"" + gmapLANG_CFG["navbar_index"] + "\" class=\"navbar_index\" href=\"" + RESPONSE_STR_INDEX_HTML + "\">" + gmapLANG_CFG["navbar_index"] + "</a>";
			if( bNewIndex )
				pResponse->strContent += "<span class=\"navbar_new\">(" + gmapLANG_CFG["new"] + ")</span>";
			pResponse->strContent += "</td>\n";
// 			pResponse->strContent += "<td class=\"navbar_pipe_login_" + cstrCSS + "\">|</span>";
// 			pResponse->strContent += "<span class=\"navbar_index_" + cstrCSS + "\"><a title=\"" + gmapLANG_CFG["navbar_index"] + "\" class=\"navbar_index_" + cstrCSS + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "\">" + gmapLANG_CFG["navbar_index"] + "</a></span>\n";
// 			pResponse->strContent += "<span class=\"navbar_pipe_login_" + cstrCSS + "\">|</span>";
		}
		
		// upload
		if( !pRequest->user.strUID.empty( ) && ( pRequest->user.ucAccess & m_ucAccessUploadTorrents ) )
			pResponse->strContent += "<td class=\"navbar_upload\"><a title=\"" + gmapLANG_CFG["navbar_upload"] + "\" class=\"navbar_upload\" href=\"" + RESPONSE_STR_UPLOAD_HTML + "\">" + gmapLANG_CFG["navbar_upload"] + "</a></td>\n";
		else if( ( !m_strOfferDir.empty( ) && !pRequest->user.strUID.empty( ) && ( pRequest->user.ucAccess & m_ucAccessUploadOffers ) ) )
			pResponse->strContent += "<td class=\"navbar_upload\"><a title=\"" + gmapLANG_CFG["navbar_upload_offer"] + "\" class=\"navbar_upload\" href=\"" + RESPONSE_STR_UPLOAD_HTML + "\">" + gmapLANG_CFG["navbar_upload_offer"] + "</a></td>\n";
// 			pResponse->strContent += "<span class=\"navbar_pipe_upload_" + cstrCSS + "\">|</span><span class=\"navbar_upload_" + cstrCSS + "\"><a title=\"" + gmapLANG_CFG["navbar_upload"] + "\" class=\"navbar_upload_" + cstrCSS + "\" href=\"" + RESPONSE_STR_UPLOAD_HTML + "\">" + gmapLANG_CFG["navbar_upload"] + "</a></span>\n";
		
		// offer
		if( !pRequest->user.strUID.empty( ) && ( pRequest->user.ucAccess & m_ucAccessViewOffers ) )
		{
			pResponse->strContent += "<td class=\"navbar_offer\"><a title=\"" + gmapLANG_CFG["navbar_offer"] + "\" class=\"navbar_offer\" href=\"" + RESPONSE_STR_OFFER_HTML + "\">" + gmapLANG_CFG["navbar_offer"] + "</a>";
			if( bNewOffer )
				pResponse->strContent += "<span class=\"navbar_new\">(" + gmapLANG_CFG["new"] + ")</span>";
			pResponse->strContent += "</td>\n";
		}

		// login & my torrents
		if( pRequest->user.strUID.empty( ) )
			pResponse->strContent += "<td class=\"navbar_login\"><a title=\"" + gmapLANG_CFG["navbar_login"] + "\" class=\"navbar_login\" href=\"" + RESPONSE_STR_LOGIN_HTML + "\">" + gmapLANG_CFG["navbar_login"] + "</a></td>\n";
// 			pResponse->strContent += "<span class=\"navbar_login_" + cstrCSS + "\"><a title=\"" + gmapLANG_CFG["navbar_login"] + "\" class=\"navbar_login_" + cstrCSS + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "\">" + gmapLANG_CFG["navbar_login"] + "</a></span>\n";
		else
			pResponse->strContent += "<td class=\"navbar_login\"><a title=\"" + gmapLANG_CFG["navbar_my_torrents"] + "\" class=\"navbar_login\" href=\"" + RESPONSE_STR_LOGIN_HTML + "\">" + gmapLANG_CFG["navbar_my_torrents"] + "</a></td>\n";
// 			pResponse->strContent += "<span class=\"navbar_login_" + cstrCSS + "\"><a title=\"" + gmapLANG_CFG["navbar_my_torrents"] + "\" class=\"navbar_login_" + cstrCSS + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "\">" + gmapLANG_CFG["navbar_my_torrents"] + "</a></span>\n";

		// logout
// 		if( !pRequest->user.strLogin.empty( ) )
// 			pResponse->strContent += "<td class=\"navbar_logout\"><a title=\"" + gmapLANG_CFG["navbar_logout"] + "\" class=\"navbar_logout\" href=\"" + RESPONSE_STR_LOGIN_HTML + "?logout=1\">" + gmapLANG_CFG["navbar_logout"] + "</a></td>\n";
// 			pResponse->strContent += "<span class=\"navbar_pipe_logout_" + cstrCSS + "\">|</span><span class=\"navbar_logout_" + cstrCSS + "\"><a title=\"" + gmapLANG_CFG["navbar_logout"] + "\" class=\"navbar_logout_" + cstrCSS + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "?logout=1\">" + gmapLANG_CFG["navbar_logout"] + "</a></span>\n";

		// signup
		if( pRequest->user.ucAccess & m_ucAccessSignup )
		{
			if( pRequest->user.strUID.empty( ) )
				pResponse->strContent += "<td class=\"navbar_signup\"><a title=\"" + gmapLANG_CFG["navbar_sign_up_school"] + "\" class=\"navbar_signup\" href=\"" + RESPONSE_STR_SIGNUP_SCHOOL_HTML + "\">" + gmapLANG_CFG["navbar_sign_up_school"] + "</a> / " + "<a title=\"" + gmapLANG_CFG["navbar_sign_up_invite"] + "\" class=\"navbar_signup\" href=\"" + RESPONSE_STR_INVITE_HTML + "\">" + gmapLANG_CFG["navbar_sign_up_invite"] + "</a></td>\n";
			else if( pRequest->user.ucAccess & m_ucAccessSignupDirect )
				pResponse->strContent += "<td class=\"navbar_signup\"><a title=\"" + gmapLANG_CFG["navbar_sign_up"] + "\" class=\"navbar_signup\" href=\"" + RESPONSE_STR_SIGNUP_HTML + "\">" + gmapLANG_CFG["navbar_sign_up"] + "</a></td>\n";
// 			pResponse->strContent += "<span class=\"navbar_pipe_signup_" + cstrCSS + "\">|</span><span class=\"navbar_signup_" + cstrCSS + "\"><a title=\"" + gmapLANG_CFG["navbar_sign_up"] + "\" class=\"navbar_signup_" + cstrCSS + "\" href=\"" + RESPONSE_STR_SIGNUP_HTML + "\">" + gmapLANG_CFG["navbar_sign_up"] + "</a></span>\n";
		}

		// RSS
// 		if( ( pRequest->user.ucAccess & ACCESS_VIEW ) && !rssdump.strName.empty( ) )
// 		{
// 			if( !rssdump.strURL.empty( ) )
// 				pResponse->strContent += "<td class=\"navbar_rss\"><a rel=\"" + STR_TARGET_REL + "\" title=\"" + gmapLANG_CFG["navbar_rss"] + "\" class=\"navbar_rss\" href=\"" + rssdump.strURL + rssdump.strName + "\">" + gmapLANG_CFG["navbar_rss"] + "</a></td>\n";
// // 				pResponse->strContent += "<span class=\"navbar_pipe_rss_" + cstrCSS + "\">|</span><span class=\"navbar_rss_" + cstrCSS + "\"><a rel=\"" + STR_TARGET_REL + "\" title=\"" + gmapLANG_CFG["navbar_rss"] + "\" class=\"navbar_rss_" + cstrCSS + "\" href=\"" + rssdump.strURL + rssdump.strName + "\">" + gmapLANG_CFG["navbar_rss"] + "</a></span>\n";
// 			else if( m_bServeLocal )
// 				pResponse->strContent += "<td class=\"navbar_rss\"><a rel=\"" + STR_TARGET_REL + "\" title=\"" + gmapLANG_CFG["navbar_rss"] + "\" class=\"navbar_rss\" href=\"" + RESPONSE_STR_SEPERATOR + rssdump.strName + "\">" + gmapLANG_CFG["navbar_rss"] + "</a></td>\n";

// // 				pResponse->strContent += "<span class=\"navbar_pipe_rss_" + cstrCSS + "\">|</span><span class=\"navbar_rss_" + cstrCSS + "\"><a rel=\"" + STR_TARGET_REL + "\" title=\"" + gmapLANG_CFG["navbar_rss"] + "\" class=\"navbar_rss_" + cstrCSS + "\" href=\"" + RESPONSE_STR_SEPERATOR + rssdump.strName + "\">" + gmapLANG_CFG["navbar_rss"] + "</a></span>\n";
// 		}

		// XML
// 		if( ( pRequest->user.ucAccess & ACCESS_ADMIN ) && !xmldump.strName.empty( ) )
// 		{
// 			if( !xmldump.strURL.empty( ) )
// 				pResponse->strContent += "<td class=\"navbar_xml\"><a rel=\"" + STR_TARGET_REL + "\" title=\"" + gmapLANG_CFG["navbar_xml"] + "\" class=\"navbar_xml\" href=\"" + xmldump.strURL + xmldump.strName + "\">" + gmapLANG_CFG["navbar_xml"] + "</a></td>\n";
// // 				pResponse->strContent += "<span class=\"navbar_pipe_xml_" + cstrCSS + "\">|</span><span class=\"navbar_xml_" + cstrCSS + "\"><a rel=\"" + STR_TARGET_REL + "\" title=\"" + gmapLANG_CFG["navbar_xml"] + "\" class=\"navbar_xml_" + cstrCSS + "\" href=\"" + xmldump.strURL + xmldump.strName + "\">" + gmapLANG_CFG["navbar_xml"] + "</a></span>\n";
// 			else if( m_bServeLocal )
// 				pResponse->strContent += "<td class=\"navbar_xml\"><a rel=\"" + STR_TARGET_REL + "\" title=\"" + gmapLANG_CFG["navbar_xml"] + "\" class=\"navbar_xml\" href=\"" + RESPONSE_STR_SEPERATOR + xmldump.strName + "\">" + gmapLANG_CFG["navbar_xml"] + "</a></td>\n";
// // 				pResponse->strContent += "<span class=\"navbar_pipe_xml_" + cstrCSS + "\">|</span><span class=\"navbar_xml_" + cstrCSS + "\"><a rel=\"" + STR_TARGET_REL + "\" title=\"" + gmapLANG_CFG["navbar_xml"] + "\" class=\"navbar_xml_" + cstrCSS + "\" href=\"" + RESPONSE_STR_SEPERATOR + xmldump.strName + "\">" + gmapLANG_CFG["navbar_xml"] + "</a></span>\n";
// 		}

		// info
//		if( !pRequest->user.strUID.empty( ) && ( pRequest->user.ucAccess & m_ucAccessView ) )
		if( ( pRequest->user.ucAccess & m_ucAccessView ) )
			pResponse->strContent += "<td class=\"navbar_info\"><a class=\"navbar_info\" title=\"" + gmapLANG_CFG["navbar_info"] + "\" href=\"" + RESPONSE_STR_INFO_HTML + "\">" + gmapLANG_CFG["navbar_info"] + "</a></td>\n";
// 			pResponse->strContent += "<span class=\"navbar_pipe_info_" + cstrCSS + "\">|</span><span class=\"navbar_info_" + cstrCSS + "\"><a class=\"navbar_info_" + cstrCSS + "\" title=\"" + gmapLANG_CFG["navbar_info"] + "\" href=\"" + RESPONSE_STR_INFO_HTML + "\">" + gmapLANG_CFG["navbar_info"] + "</a></span>\n";

		// rules
		if( !pRequest->user.strUID.empty( ) && ( pRequest->user.ucAccess & m_ucAccessView ) )
			pResponse->strContent += "<td class=\"navbar_rules\"><a class=\"navbar_rules\" title=\"" + gmapLANG_CFG["navbar_rules"] + "\" href=\"" + RESPONSE_STR_RULES_HTML + "\">" + gmapLANG_CFG["navbar_rules"] + "</a></td>\n";

		// faq
		if( !pRequest->user.strUID.empty( ) && ( pRequest->user.ucAccess & m_ucAccessView ) )
			pResponse->strContent += "<td class=\"navbar_faq\"><a class=\"navbar_faq\" title=\"" + gmapLANG_CFG["navbar_faq"] + "\" href=\"" + RESPONSE_STR_FAQ_HTML + "\">" + gmapLANG_CFG["navbar_faq"] + "</a></td>\n";

		// staff
		if( !pRequest->user.strUID.empty( ) && ( pRequest->user.ucAccess & m_ucAccessView ) )
			pResponse->strContent += "<td class=\"navbar_staff\"><a class=\"navbar_staff\" title=\"" + gmapLANG_CFG["navbar_staff"] + "\" href=\"" + RESPONSE_STR_STAFF_HTML + "\">" + gmapLANG_CFG["navbar_staff"] + "</a></td>\n";
		
		// staff
		if( !pRequest->user.strUID.empty( ) && ( pRequest->user.ucAccess & m_ucAccessView ) )
			pResponse->strContent += "<td class=\"navbar_rank\"><a class=\"navbar_rank\" title=\"" + gmapLANG_CFG["navbar_rank"] + "\" href=\"" + RESPONSE_STR_RANK_HTML + "\">" + gmapLANG_CFG["navbar_rank"] + "</a></td>\n";

		// log
		if( !pRequest->user.strUID.empty( ) && ( pRequest->user.ucAccess & m_ucAccessViewLog ) )
			pResponse->strContent += "<td class=\"navbar_log\"><a title=\"" + gmapLANG_CFG["navbar_log"] + "\" class=\"navbar_log\" href=\"" + RESPONSE_STR_LOG_HTML + "\">" + gmapLANG_CFG["navbar_log"] + "</a></td>\n";
// 			pResponse->strContent += "<span class=\"navbar_pipe_admin_" + cstrCSS + "\">|</span><span class=\"navbar_admin_" + cstrCSS + "\"><a title=\"" + gmapLANG_CFG["navbar_admin"] + "\" class=\"navbar_admin_" + cstrCSS + "\" href=\"" + RESPONSE_STR_ADMIN_HTML + "\">" + gmapLANG_CFG["navbar_admin"] + "</a></span>\n";

		// admin
		if( pRequest->user.ucAccess & m_ucAccessAdmin )
			pResponse->strContent += "<td class=\"navbar_admin\"><a title=\"" + gmapLANG_CFG["navbar_admin"] + "\" class=\"navbar_admin\" href=\"" + RESPONSE_STR_ADMIN_HTML + "\">" + gmapLANG_CFG["navbar_admin"] + "</a></td>\n";
// 			pResponse->strContent += "<span class=\"navbar_pipe_admin_" + cstrCSS + "\">|</span><span class=\"navbar_admin_" + cstrCSS + "\"><a title=\"" + gmapLANG_CFG["navbar_admin"] + "\" class=\"navbar_admin_" + cstrCSS + "\" href=\"" + RESPONSE_STR_ADMIN_HTML + "\">" + gmapLANG_CFG["navbar_admin"] + "</a></span>\n";

		// users
		if( !pRequest->user.strUID.empty( ) && ( pRequest->user.ucAccess & m_ucAccessViewUsers ) )
			pResponse->strContent += "<td class=\"navbar_users\"><a title=\"" + gmapLANG_CFG["navbar_users"] + "\" class=\"navbar_users\" href=\"" + RESPONSE_STR_USERS_HTML + "\">" + gmapLANG_CFG["navbar_users"] + "</a></td>\n";
// 			pResponse->strContent += "<span class=\"navbar_pipe_users_" + cstrCSS + "\">|</span><span class=\"navbar_users_" + cstrCSS + "\"><a title=\"" + gmapLANG_CFG["navbar_users"] + "\" class=\"navbar_users_" + cstrCSS + "\" href=\"" + RESPONSE_STR_USERS_HTML + "\">" + gmapLANG_CFG["navbar_users"] + "</a></span>\n";

		// tags
		if( !pRequest->user.strUID.empty( ) && ( pRequest->user.ucAccess & m_ucAccessAdmin ) )
			pResponse->strContent += "<td class=\"navbar_tags\"><a title=\"" + gmapLANG_CFG["navbar_tags"] + "\" class=\"navbar_tags\" href=\"" + RESPONSE_STR_TAGS_HTML + "\">" + gmapLANG_CFG["navbar_tags"] + "</a></td>\n";
// 			pResponse->strContent += "<span class=\"navbar_pipe_tags_" + cstrCSS + "\">|</span><span class=\"navbar_tags_" + cstrCSS + "\"><a title=\"" + gmapLANG_CFG["navbar_tags"] + "\" class=\"navbar_tags_" + cstrCSS + "\" href=\"" + RESPONSE_STR_TAGS_HTML + "\">" + gmapLANG_CFG["navbar_tags"] + "</a></span>\n";

		// language
// 		if( pRequest->user.ucAccess & ACCESS_ADMIN )
// 			pResponse->strContent += "<td class=\"navbar_language\"><a title=\"" + gmapLANG_CFG["navbar_language"] + "\" class=\"navbar_language\" href=\"" + RESPONSE_STR_LANGUAGE_HTML + "\">" + gmapLANG_CFG["navbar_language"] + "</a></td>\n";
// 			pResponse->strContent += "<span class=\"navbar_pipe_language_" + cstrCSS + "\">|</span><span class=\"navbar_language_" + cstrCSS + "\"><a title=\"" + gmapLANG_CFG["navbar_language"] + "\" class=\"navbar_language_" + cstrCSS + "\" href=\"" + RESPONSE_STR_LANGUAGE_HTML + "\">" + gmapLANG_CFG["navbar_language"] + "</a></span>\n";

		// xstats
		if( !pRequest->user.strUID.empty( ) && ( pRequest->user.ucAccess & m_ucAccessViewXStates ) )
			pResponse->strContent += "<td class=\"navbar_xstats\"><a title=\"" + gmapLANG_CFG["navbar_xstats"] + "\" class=\"navbar_xstats\" href=\"" + RESPONSE_STR_XSTATS_HTML + "\">" + gmapLANG_CFG["navbar_xstats"] + "</a></td>\n";
// 			pResponse->strContent += "<span class=\"navbar_pipe_xstats_" + cstrCSS + "\">|</span><span class=\"navbar_xstats_" + cstrCSS + "\"><a title=\"" + gmapLANG_CFG["navbar_xstats"] + "\" class=\"navbar_xstats_" + cstrCSS + "\" href=\"" + RESPONSE_STR_XSTATS_HTML + "\">" + gmapLANG_CFG["navbar_xstats"] + "</a></span>\n";

		// xtorrent
// 		if( pRequest->user.ucAccess & ACCESS_ADMIN )
// 			pResponse->strContent += "<td class=\"navbar_xtorrent\"><a title=\"" + gmapLANG_CFG["navbar_xtorrent"] + "\" class=\"navbar_xtorrent\" href=\"" + RESPONSE_STR_XTORRENT_HTML + "\">" + gmapLANG_CFG["navbar_xtorrent"] + "</a></td>\n";
// 			pResponse->strContent += "<span class=\"navbar_pipe_xtorrent_" + cstrCSS + "\">|</span><span class=\"navbar_xtorrent_" + cstrCSS + "\"><a title=\"" + gmapLANG_CFG["navbar_xtorrent"] + "\" class=\"navbar_xtorrent_" + cstrCSS + "\" href=\"" + RESPONSE_STR_XTORRENT_HTML + "\">" + gmapLANG_CFG["navbar_xtorrent"] + "</a></span>\n";


		// close navbar
		pResponse->strContent += "</tr>\n</table>\n</div>\n";
	}
	else
	{
		pResponse->strContent += "<div>Internal Navigation Bar Error!</div>\n\n";
		UTIL_LogPrint( "CTracker: HTML_Nav_Bar: Internal Navigation Bar Error!\n" );
	}
}

// This function builds the HTML for the start of every display page and is HTML 4.01 Strict compliant
void CTracker :: HTML_Common_Begin( struct request_t *pRequest, struct response_t *pResponse, const string &cstrTitle, const string &cstrCSS, const string &cstrUrl, const bool &bIndex, const unsigned int &cuiCode )
{

// 	unsigned char ucExpireSecs = 0;
// 
// 	if( cstrCSS == CSS_INDEX || cstrCSS == CSS_STATS || cstrCSS == CSS_INFO )
// 		ucExpireSecs = 15;
// 
// 	time_t tExpires = time( 0 ) + ucExpireSecs;
// 	char *szExpires = asctime( gmtime( &tExpires ) );
// 	szExpires[strlen( szExpires ) - 1] = TERM_CHAR;

	// Response code and headers
	switch( cuiCode )
	{
	case CODE_200:
		pResponse->strCode = "200 " + gmapLANG_CFG["server_response_200"];

		break;
	case CODE_400:
		pResponse->strCode = "400 " + gmapLANG_CFG["server_response_400"];

		break;
	case CODE_401:
		pResponse->strCode = "401 " + gmapLANG_CFG["server_response_401"];

// 		pResponse->mapHeaders.insert( pair<string, string>( "WWW-Authenticate", string( "Basic realm=\"" ) + gstrRealm + "\"" ) );
		
		pResponse->bCompressOK = false;

		break;
	case CODE_403:
		pResponse->strCode = "403 " + gmapLANG_CFG["server_response_403"];

		break;
	case CODE_404:
		pResponse->strCode = "404 " + gmapLANG_CFG["server_response_404"];

		break;
	default:
		UTIL_LogPrint( "CTracker: HTML_Common_Begin: response code unknown (code %u)\n", cuiCode );
		pResponse->bCompressOK = false;
	}

	// Content-Type
	if( !gstrCharSet.empty( ) )
		pResponse->mapHeaders.insert( pair<string, string>( "Content-Type", string( gmapMime[".html"] + "; charset=" ) + gstrCharSet ) );
	else
		UTIL_LogPrint( "CTracker: HTML_Common_Begin: Content-Type!\n" );

	// Expires
// 	if( !string( szExpires ).empty( ) )
// 		pResponse->mapHeaders.insert( pair<string, string>( "Expires", string( szExpires ) + " GMT" ) );	
// 	else
// 		UTIL_LogPrint( "CTracker: HTML_Common_Begin: Expires!\n" );

	// define doctype
	if( !STR_DOC_TYPE.empty( ) )
		pResponse->strContent += STR_DOC_TYPE;
	else
		UTIL_LogPrint( "CTracker: HTML_Common_Begin: doctype!\n" );

	// open html and head
	pResponse->strContent += "<html lang=\"zh\">\n\n<head>\n";

	// set title
	if( !m_strTitle.empty( ) || !cstrTitle.empty( ) )
	{
		pResponse->strContent += "<title>";

		if( !m_strTitle.empty( ) )
		{
			pResponse->strContent += m_strTitle.c_str( );

			if( !cstrTitle.empty( ) )
				pResponse->strContent += " | ";
		}

		if( !cstrTitle.empty( ) )
			pResponse->strContent += cstrTitle.c_str( );

		pResponse->strContent += "</title>\n";
	}

	// include CSS
	if( !style.strName.empty( ) )
	{
		if( !style.strURL.empty( ) )
			pResponse->strContent += "<link rel=\"stylesheet\" title=\"external\" type=\"" + gmapMime[".css"] + "\" href=\"" + style.strURL + style.strName + "\">\n";
		else if( m_bServeLocal )
			pResponse->strContent += "<link rel=\"stylesheet\" title=\"external\" type=\"" + gmapMime[".css"] + "\" href=\"/" + style.strName + "\">\n";
	}
	else
		UTIL_LogPrint( "CTracker: HTML_Common_Begin: CSS name empty!\n" );

	// Embeded Styles for browser bugfixes
	// The style tags are visible on Pre-HTML 3.2 browsers
	pResponse->strContent += "<style type=\"" + gmapMime[".css"] + "\" title=\"internal\">\n";
	pResponse->strContent += "<!--\n";
	/* IE6 scroll bar bugfix */
	pResponse->strContent += "html{overflow-x:auto}\n";
	/* Empty cells bugfix */
	pResponse->strContent += "td{empty-cells:show}\n"; 
	pResponse->strContent += "-->\n";
	pResponse->strContent += "</style>\n";

	// RSS ( Thanks labarks )
	if( bIndex && ( pRequest->user.ucAccess & m_ucAccessRSS ) && !rssdump.strName.empty( ) )
	{
		if( m_ucDumpRSSFileMode == 0 || m_ucDumpRSSFileMode == 2 )
		{
			if( !rssdump.strURL.empty( ) )
				pResponse->strContent += "<link rel=\"alternate\" type=\"" + gmapMime[".rss"] + "\" title=\"" + m_strTitle + ": " + gmapLANG_CFG["navbar_rss"] + "\" href=\"" + rssdump.strURL + rssdump.strName + "\">\n";
			else if( m_bServeLocal )
				pResponse->strContent += "<link rel=\"alternate\" type=\"" + gmapMime[".rss"] + "\" title=\"" + m_strTitle + ": " + gmapLANG_CFG["navbar_rss"] + "\" href=\"/" + rssdump.strName + "\">\n";
		}

		if( m_ucDumpRSSFileMode != 0 )
		{
			if( !m_vecTags.empty( ) )
			{
// 				const string cstrRSSByTag( rssdump.strName.substr( 0, rssdump.strName.length( ) - rssdump.strExt.length( ) ) + "-" );

				for( vector< pair< string, string > > :: iterator ulTagKey = m_vecTags.begin( ); ulTagKey != m_vecTags.end( ); ulTagKey++ )
				{
					pResponse->strContent += "<link rel=\"alternate\" type=\"" + gmapMime[".rss"] + "\" title=\"" + m_strTitle + ": " + gmapLANG_CFG["navbar_rss"] + " - ";

					if( !rssdump.strURL.empty( ) )
// 						pResponse->strContent += (*ulTagKey).first + "\" href=\"" + rssdump.strURL + cstrRSSByTag + (*ulTagKey).first + rssdump.strExt + "\">\n";
						pResponse->strContent += (*ulTagKey).first + "\" href=\"" + rssdump.strURL + rssdump.strName + "?tag=" + (*ulTagKey).first + "\">\n";
					else if( m_bServeLocal )
						pResponse->strContent += (*ulTagKey).first + "\" href=\"/" + rssdump.strName + (*ulTagKey).first + "\">\n";
				}
			}
		}
	}

	// favicon
	if( !favicon.strFile.empty( ) )
	{
		if( favicon.strExt == ".ico" || favicon.strExt == ".png" || favicon.strExt == ".gif" )
		{
			pResponse->strContent += "<link rel=\"Shortcut Icon\" type=\"" + gmapMime[favicon.strExt] + "\" href=\"" + RESPONSE_STR_FAVICON_ICO + "\">\n";
			pResponse->strContent += "<link rel=\"Icon\" type=\"" + gmapMime[favicon.strExt] + "\" href=\"" + RESPONSE_STR_FAVICON_ICO + "\">\n";
		}
		else
		{
			pResponse->strContent += "<link rel=\"Shortcut Icon\" type=\"" + gmapMime[".ico"] + "\" href=\"" + RESPONSE_STR_FAVICON_ICO + "\">\n";
			pResponse->strContent += "<link rel=\"Icon\" type=\"" + gmapMime[".ico"] + "\" href=\"" + RESPONSE_STR_FAVICON_ICO + "\">\n";
		}
	}

	// Content-Type
	if( !gstrCharSet.empty( ) )
		pResponse->strContent += "<META http-equiv=\"Content-Type\" content=\"" + gmapMime[".html"] + "; charset=" + gstrCharSet + "\">\n";

	// generator
	if( !XBNBT_VER.empty( ) )
		pResponse->strContent += "<META name=\"generator\" content=\"" + XBNBT_VER + "\">\n";

	// Author
	if( !m_strTitle.empty( ) )
		pResponse->strContent += "<META http-equiv=\"Author\" content=\"" + m_strTitle + "\">\n";

	// Sets index meta tags for search engines and crawlers. All other pages are blocked.
	// You can use robots.txt to control the index page and add further blocking.
	if( bIndex )
	{
		// description meta tag
		if( !m_strDescription.empty( ) )
			pResponse->strContent += "<META name=\"description\" content=\"" + m_strDescription + "\">\n";

		// keyword meta tag
		if( !m_strKeywords.empty( ) )
			pResponse->strContent += "<META name=\"keywords\" content=\"" + m_strKeywords + "\">\n";

		// robots meta tag
		pResponse->strContent += "<META name=\"robots\" content=\"index,follow\">\n";
	}
	else
	{
		// robots meta tag
		pResponse->strContent += "<META name=\"robots\" content=\"noindex\">\n";
	}

	// redirect if TRUE
	if( !cstrUrl.empty( ) )
		pResponse->strContent += "<META http-equiv=\"REFRESH\" content=\"0; url=" + cstrUrl + "\">\n" ;

	// script-type
	pResponse->strContent += "<META http-equiv=\"Content-Script-Type\" content=\"text/javascript\">\n";

	// Expires
// 	if( !string( szExpires ).empty( ) )
// 		pResponse->strContent += "<META http-equiv=\"Expires\" content=\"" + string( szExpires ) + " GMT\">\n";

	// anti M$ redirect
	pResponse->strContent += "<META name=\"MSSmartTagsPreventParsing\" content=\"true\">\n";


	// PICS rating
	if( !m_strRating.empty( ) )

		pResponse->strContent += "<META http-equiv=\"PICS-Label\" content=\"" + m_strRating + "\">";

	// close head and start body
	pResponse->strContent += "</head>\n\n<body>\n\n";

	// include JS_Target
	if( !STR_TARGET_REL.empty( ) && !STR_TARGET_BLANK.empty( ) )
		pResponse->strContent += JS_Target( STR_TARGET_REL, STR_TARGET_BLANK );

	// display login status (login1=not logged in) (login2=logged in)
	pResponse->strContent += "<table class=\"top_header\">\n";
	pResponse->strContent += "<tr class=\"top_header\">\n";
	pResponse->strContent += "<td class=\"top_avatar\">\n";
//	pResponse->strContent += "<a class=\"top_title\" href=\"" + RESPONSE_STR_INDEX_HTML + "\">" + m_strTitle + "</a>";
	pResponse->strContent += "<a class=\"top_title\" href=\"" + RESPONSE_STR_INDEX_HTML + "\">" + gmapLANG_CFG["avatar"] + "</a>";
//	pResponse->strContent += "<br><span class=\"top_subtitle\">" + m_strSubTitle + "</span>";
	pResponse->strContent += "</td>\n";
	
	// Display static header
	if( ( ( bIndex && ( pRequest->user.ucAccess & m_ucAccessView ) ) || m_bStaticAll ) && !m_strStaticHeader.empty( ) )
	{
		pResponse->strContent += "<td class=\"static_header\">\n";
		pResponse->strContent += "<div class=\"static_header\">\n" + m_strStaticHeader + "\n</div>\n</td>\n";
	}
	pResponse->strContent += "</tr>\n</table>\n";
	pResponse->strContent += "<table class=\"top_bar\">\n";
	pResponse->strContent += "<tr class=\"top_bar\"><td class=\"top_login\">\n";
	
	string strLocation = string( );
	
	if( pRequest->strIP.find( ":" ) == string :: npos )
	{
		if( pRequest->strIP.find( "172.21." ) == 0 || pRequest->strIP.find( "58.192." ) == 0 || pRequest->strIP.find( "180.209." ) == 0 )
			strLocation = gmapLANG_CFG["location_ipv4_nju_bras"];
		else
			strLocation = gmapLANG_CFG["location_ipv4_nju"];
	}
	else
	{
		if( pRequest->strIP.find( "2001:da8:1007:" ) == 0 || pRequest->strIP.find( "2001:250:5002:" ) == 0 )
			strLocation = gmapLANG_CFG["location_ipv6_nju"];
		else if( pRequest->strIP.find( "2001:" ) == 0 && pRequest->strIP.find( "2001:0:" ) != 0 )
			strLocation = gmapLANG_CFG["location_ipv6_cernet"];
		else
			strLocation = gmapLANG_CFG["location_ipv6_other"];
	}
	
	CMySQLQuery *pQueryUser = 0;
	
	vector<string> vecQueryUser;
	
	vecQueryUser.reserve(12);
	
	if( !pRequest->user.strUID.empty( ) )
	{
		pQueryUser = new CMySQLQuery( "SELECT buploaded,bdownloaded,bbonus,bseeding,bleeching,UNIX_TIMESTAMP(blast_index),UNIX_TIMESTAMP(blast_index_2),UNIX_TIMESTAMP(blast_offer),binvites,btalk,btalkref,btalktorrent FROM users WHERE buid=" + pRequest->user.strUID );
		
		vecQueryUser = pQueryUser->nextRow( );
	}
		
	delete pQueryUser;
	
	if( pRequest->user.strUID.empty( ) )
	{
		pResponse->strContent += "<span class=\"top_login1\">";
		pResponse->strContent += UTIL_Xsprintf( gmapLANG_CFG["login1"].c_str( ), string( "<a class=\"red\" title=\"" + gmapLANG_CFG["login"] + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "\">" ).c_str( ), "</a>" );
		pResponse->strContent += "<br>" + UTIL_Xsprintf( gmapLANG_CFG["show_ip"].c_str( ), string( "<span class=\"blue\">" + pRequest->strIP + "</span>" ).c_str( ) ) + " " + strLocation + "</span>";
	}
	else
	{
		CMySQLQuery mq01( "UPDATE users SET bip=\'" + UTIL_StringToMySQL( pRequest->strIP ) + "\' WHERE buid=" + pRequest->user.strUID );
		
		int64 ulUploaded;
		int64 ulDownloaded;
		float flShareRatio;
		int64 ulBonus;

		pResponse->strContent += "<span class=\"top_login2\">" + UTIL_Xsprintf( gmapLANG_CFG["login2"].c_str( ), getUserLink( pRequest->user.strUID, pRequest->user.strLogin ).c_str( ) );
		if( vecQueryUser.size( ) == 12 )
		{
			pRequest->user.strInvites = vecQueryUser[8];

			pResponse->strContent += " " + UTIL_Xsprintf( gmapLANG_CFG["login2_tools"].c_str( ), string( "<a class=\"bookmark\" href=\"" + RESPONSE_STR_LOGIN_HTML + "?show=bookmarks\">" ).c_str( ), "</a>", string( "<a class=\"friend\" href=\"" + RESPONSE_STR_LOGIN_HTML + "?show=friends\">" ).c_str( ), "</a>", string( "<a class=\"blue\" href=\"" + RESPONSE_STR_LOGIN_HTML + "?show=invites\">" ).c_str( ), string( vecQueryUser[8] + "</a>" ).c_str( ), string( "<a class=\"red\" title=\"" + gmapLANG_CFG["navbar_logout"] + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "?logout=1\">" ).c_str( ), "</a>" );
		}
		pResponse->strContent += "<br>" + UTIL_Xsprintf( gmapLANG_CFG["show_ip"].c_str( ), string( "<span class=\"ip\">" + pRequest->strIP + "</span>" ).c_str( ) ) + " " + strLocation + "<br>";
		if( vecQueryUser.size( ) == 12 )
		{
			ulUploaded = UTIL_StringTo64( vecQueryUser[0].c_str( ) );
			ulDownloaded = UTIL_StringTo64( vecQueryUser[1].c_str( ) );
			ulBonus = UTIL_StringTo64( vecQueryUser[2].c_str( ) );
		
			if( ulDownloaded == 0 )
			{
				if( ulUploaded == 0 )
					flShareRatio = 0;
				else
					flShareRatio = -1;
			}
			else
				flShareRatio = (float)ulUploaded / (float)ulDownloaded;
			
			pRequest->user.ulUploaded = ulUploaded;
			pRequest->user.ulDownloaded = ulDownloaded;
			pRequest->user.ulBonus = ulBonus;
			pRequest->user.flShareRatio = flShareRatio;

			char szFloat[16];
			string strShareRatio = string( );
			if( ( -1.001 < flShareRatio ) && ( flShareRatio < -0.999 ) )
				strShareRatio = gmapLANG_CFG["perfect"];
			else
			{
				memset( szFloat, 0, sizeof( szFloat ) / sizeof( char ) );
				snprintf( szFloat, sizeof( szFloat ) / sizeof( char ), "%0.3f", flShareRatio );
				strShareRatio = string( szFloat );
			}
			pResponse->strContent += UTIL_Xsprintf( gmapLANG_CFG["user_state"].c_str( ), string( "<span class=\"blue\">" + vecQueryUser[3] + "</span>" ).c_str( ), string( "<span class=\"green\">" + vecQueryUser[4] + "</span>" ).c_str( ), strShareRatio.c_str( ), string( "<span class=\"blue\">" + UTIL_BytesToString( ulUploaded ) + "</span>" ).c_str( ), string( "<span class=\"green\">" + UTIL_BytesToString( ulDownloaded ) + "</span>" ).c_str( ), string( "<a class=\"red\" href=\"" + RESPONSE_STR_LOGIN_HTML + "?show=bonus\">" + CAtomLong( ulBonus / 100 ).toString( ) + "." + CAtomInt( ( ulBonus % 100 ) / 10 ).toString( ) + CAtomInt( ulBonus % 10 ).toString( ) + "</a>" ).c_str( ) ) + " </span>";
		}
	}
	time_t tNow = time( 0 );
	char *szTime = asctime( localtime( &tNow ) );
	szTime[strlen( szTime ) - 1] = TERM_CHAR;

	unsigned long ulCount = 0;

	unsigned long ulRead = 0;
	unsigned long ulUnread = 0;
	if( !pRequest->user.strUID.empty( ) )
	{
		CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bread,COUNT(*) FROM messages WHERE bsendtoid=" + pRequest->user.strUID + " GROUP BY bread" );
				
		vector<string> vecQuery;
	
		vecQuery.reserve(2);
		
		vecQuery = pQuery->nextRow( );
		
		while( vecQuery.size( ) == 2 )
		{
			if( vecQuery[0] == "0" )
				ulUnread = (unsigned long)atoi( vecQuery[1].c_str( ) );
			if( vecQuery[0] == "1" )
				ulRead = (unsigned long)atoi( vecQuery[1].c_str( ) );
			
			vecQuery = pQuery->nextRow( );
		}
		delete pQuery;
		ulCount = ulUnread + ulRead;
	}
	pResponse->strContent += "</td>\n<td class=\"top_tool\">";
	pResponse->strContent += "<span class=\"top_tool\">" + gmapLANG_CFG["info_server_time"] + ": " + string( szTime ) + "<br>";
	if( !pRequest->user.strUID.empty( ) )
	{
		if( ulUnread > 0 )
			pResponse->strContent += "<a class=\"red\" title=\"" + gmapLANG_CFG["messages_have_unread"] + "\" href=\"" + RESPONSE_STR_MESSAGES_HTML + "\">" + gmapLANG_CFG["messages_have_unread"] + "</a>: ";
		else
			pResponse->strContent += "<a class=\"black\" title=\"" + gmapLANG_CFG["messages"] + "\" href=\"" + RESPONSE_STR_MESSAGES_HTML + "\">" + gmapLANG_CFG["messages"] + "</a>: ";
		pResponse->strContent += UTIL_Xsprintf( gmapLANG_CFG["messages_state"].c_str( ), CAtomInt( ulCount ).toString( ).c_str( ), CAtomInt( ulUnread ).toString( ).c_str( ) );
	}
	if( ( pRequest->user.ucAccess & m_ucAccessRSS ) && !rssdump.strName.empty( ) )
	{
		string cstrFilter( pRequest->mapParams["tag"] );
		const string cstrMedium( pRequest->mapParams["medium"] );
		const string cstrQuality( pRequest->mapParams["quality"] );
		const string cstrEncode( pRequest->mapParams["encode"] );
		const string cstrSearch( pRequest->mapParams["search"] );
		const string cstrUploader( pRequest->mapParams["uploader"] );
		const string cstrMatch( pRequest->mapParams["match"] );
		const string cstrNoTag( pRequest->mapParams["notag"] );

		string strPageParameters = string( );
		
		if( cstrFilter.empty( ) && !pRequest->user.strUID.empty( ) && cstrNoTag != "1" )
		{
			CMySQLQuery *pQueryPrefs = new CMySQLQuery( "SELECT bdefaulttag FROM users_prefs WHERE buid=" + pRequest->user.strUID );
		
			map<string, string> mapPrefs;

			mapPrefs = pQueryPrefs->nextRowMap( );

			delete pQueryPrefs;
			
			cstrFilter = mapPrefs["bdefaulttag"];
		}
		
		vector< pair< string, string > > vecParams;
		vecParams.reserve(64);
		
		vecParams.push_back( pair<string, string>( string( "tag" ), cstrFilter ) );
		vecParams.push_back( pair<string, string>( string( "medium" ), cstrMedium ) );
		vecParams.push_back( pair<string, string>( string( "quality" ), cstrQuality ) );
		vecParams.push_back( pair<string, string>( string( "encode" ), cstrEncode ) );
		vecParams.push_back( pair<string, string>( string( "search" ), cstrSearch ) );
		vecParams.push_back( pair<string, string>( string( "uploader" ), cstrUploader ) );
		vecParams.push_back( pair<string, string>( string( "match" ), cstrMatch ) );
		
		strPageParameters += UTIL_HTMLJoin( vecParams, string( "?" ), string( "&" ), string( "=" ) );
			
		if( !pRequest->user.strUID.empty( ) )
			pResponse->strContent += "<span class=\"pipe\"> | </span>";
		if( !rssdump.strURL.empty( ) )
			pResponse->strContent += "<a rel=\"" + STR_TARGET_REL + "\" title=\"" + gmapLANG_CFG["navbar_rss"] + "\" href=\"" + rssdump.strURL + rssdump.strName + strPageParameters + "\">" + gmapLANG_CFG["navbar_rss"] + "</a>\n";
		else if( m_bServeLocal )
			pResponse->strContent += "<a rel=\"" + STR_TARGET_REL + "\" title=\"" + gmapLANG_CFG["navbar_rss"] + "\" href=\"" + RESPONSE_STR_SEPERATOR + rssdump.strName + strPageParameters + "\">" + gmapLANG_CFG["navbar_rss"] + "</a>\n";
	}
	pResponse->strContent += "</span>";
	
	if( !pRequest->user.strUID.empty( ) )
	{
		if( vecQueryUser.size( ) == 12 )
		{
			pResponse->strContent += "<br><span class=\"top_tool\">" + gmapLANG_CFG["talk_page"] + ": ";
			pResponse->strContent += "<a href=\"" + RESPONSE_STR_TALK_HTML + "\">" + gmapLANG_CFG["talk_show_home"] + "</a>";
			if( vecQueryUser[9] != "0" )
				pResponse->strContent += "<span class=\"hot\">(" + vecQueryUser[9] + ")</span>";
			pResponse->strContent += "<span class=\"pipe\"> | </span>";
			pResponse->strContent += "<a href=\"" + RESPONSE_STR_TALK_HTML + "?show=mentions\">" + gmapLANG_CFG["talk_show_mentions"] + "</a>";
			if( vecQueryUser[10] != "0" )
				pResponse->strContent += "<span class=\"hot\">(" + vecQueryUser[10] + ")</span>";
			pResponse->strContent += "<span class=\"pipe\"> | </span>";
			pResponse->strContent += "<a href=\"" + RESPONSE_STR_TALK_HTML + "?show=torrents\">" + gmapLANG_CFG["talk_show_torrents"] + "</a>";
			if( vecQueryUser[11] != "0" )
				pResponse->strContent += "<span class=\"hot\">(" + vecQueryUser[11] + ")</span>";
			pResponse->strContent += "<span class=\"pipe\"> | </span>";
			pResponse->strContent += "<a href=\"" + RESPONSE_STR_TALK_HTML + "?show=all\">" + gmapLANG_CFG["talk_show_all"] + "</a>";
			pResponse->strContent += "</span>";
		}
	}
		
	pResponse->strContent += "</td></tr></table>\n";
	
	bool bNewIndex = false;
	bool bNewOffer = false;
	int64 last_time_index = 0;
	int64 last_time_index_2 = 0;
	int64 last_time_offer = 0;
	
	time_t now_t = time( 0 );

	if( vecQueryUser.size( ) == 12 )
	{
		last_time_index = UTIL_StringTo64( vecQueryUser[5].c_str( ) );
		last_time_index_2 = UTIL_StringTo64( vecQueryUser[6].c_str( ) );
		last_time_offer = UTIL_StringTo64( vecQueryUser[7].c_str( ) );
		if( last_time_index_2 > 0 )
		{
			if( now_t - last_time_index_2 > 0 && now_t - last_time_index_2 > CFG_GetInt( "bnbt_new_torrent_interval", 300 ) )
			{
				last_time_index = last_time_index_2;
				string strQuery = "UPDATE users SET blast_index=blast_index_2";
				if( bIndex )
					strQuery += ",blast_index_2=NOW()";
				strQuery += " WHERE buid=" + pRequest->user.strUID;
				CMySQLQuery mq01( strQuery );
			}
		}
		else if( bIndex )
			CMySQLQuery mq02( "UPDATE users SET blast_index_2=NOW() WHERE buid=" + pRequest->user.strUID );
	}
	else
	{
		last_time_index = GetTime( );
		last_time_offer = GetTime( );
	}

	if( last_time_index < m_pCache->getLatest( ) )
		bNewIndex = true;
	if( last_time_offer < m_pCache->getLatest( true ) )
		bNewOffer = true;
	// display tracker title
// 	if( bIndex && !m_strTitle.empty( ) )
// 		pResponse->strContent += "<h1 class=\"header_" + cstrCSS + "\">" + m_strTitle + "</h1>\n\n";

	// display page header
// 	if( !cstrTitle.empty( ) )
// 		pResponse->strContent += "<h3 class=\"header_" + cstrCSS + "\">" + cstrTitle + "</h3>\n\n";

// 	pResponse->strContent += "\n<p>";
	pResponse->strContent += "<table class=\"main_table\">\n<tr class=\"main_table_navbar\">\n<td class=\"main_table_navbar\">";
	// Display navbar
	
	if( ( m_ucNavbar == 1 ) || ( m_ucNavbar == 3 ) )
		HTML_Nav_Bar( pRequest, pResponse, cstrCSS, bNewIndex, bNewOffer );
	pResponse->strContent += "</td>\n</tr>\n";
	pResponse->strContent += "<tr class=\"main_table_torrent\">\n<td class=\"main_table_torrent\">\n";

	// display redirect message if TRUE
	if( !cstrUrl.empty( ) )
		pResponse->strContent += "<p class=\"redirect\">" + UTIL_Xsprintf( gmapLANG_CFG["redirect"].c_str( ), string( "<a title=\"" + cstrUrl + "\" href=\"" + cstrUrl + "\">" ).c_str( ), "</a>" ) + "</p>\n\n";

	switch( cuiCode )
	{
	case CODE_401:
		pResponse->strContent += "<p class=\"unauthorized\">401 " + gmapLANG_CFG["server_response_401"] + "</p>\n";
		
		if( pRequest->user.strUID.empty( ) )
		{
			pResponse->strContent += "<p class=\"denied\">" + UTIL_Xsprintf( gmapLANG_CFG["view_not_authorized"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["login"] + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "?return=" + UTIL_StringToEscaped( pRequest->strURL ) + "\">" ).c_str( ), "</a>" );

			if( pRequest->user.ucAccess & m_ucAccessSignup )
				pResponse->strContent += " " + gmapLANG_CFG["click_to_signup"];

			pResponse->strContent += "</p>\n\n";
		}
		else
			pResponse->strContent += "<p class=\"denied\">" + gmapLANG_CFG["view_not_authorized_access"] + "</p>\n\n";

		break;
	case CODE_403:
		pResponse->strContent += "<p class=\"unauthorized\">403 " + gmapLANG_CFG["server_response_403"] + "</p>\n";
		pResponse->strContent += "<p class=\"denied\">" + gmapLANG_CFG["view_forbidden"] + "</p>\n\n";


		break;
	case CODE_404:
		pResponse->strContent += "<p class=\"unauthorized\">404 " + gmapLANG_CFG["server_response_404"] + "</p>\n";
		pResponse->strContent += "<p class=\"denied\">" + gmapLANG_CFG["view_not_found"] + "</p>\n\n";


		break;
	default:
		;
	}
}

// This function builds the HTML for the end of every display page and is HTML 4.01 Strict compliant
void CTracker :: HTML_Common_End( struct request_t *pRequest, struct response_t *pResponse, const struct bnbttv &btv, const bool &bIndex, const string &cstrCSS )
{
	// Draw line
// 	pResponse->strContent += "<hr class=\"footer\">\n\n";

	// Display navbar
//	if( ( m_ucNavbar == 2 ) || ( m_ucNavbar == 3 ) )
//		HTML_Nav_Bar( pRequest, pResponse, cstrCSS );

	// Display users online if enabled and is index
	if( m_bUsersOnline && bIndex && ( pRequest->user.ucAccess & m_ucAccessView ) )
	{
		const unsigned int cuiOnline( ( unsigned int )gpServer->m_vecClients.size( ) );

		if ( cuiOnline > 1 )
			// Users plural
			pResponse->strContent += "<p class=\"users_online\">" + UTIL_Xsprintf( gmapLANG_CFG["users_online"].c_str( ), CAtomInt( cuiOnline ).toString( ).c_str( ) ) + "</p>\n\n";
		else
			// User singular
			pResponse->strContent += "<p class=\"users_online\">" + UTIL_Xsprintf( gmapLANG_CFG["user_online"].c_str( ), CAtomInt( cuiOnline ).toString( ).c_str( ) ) + "</p>\n\n";
	}

	pResponse->strContent += "</td></tr></table>";
	if( ( ( bIndex && ( pRequest->user.ucAccess & m_ucAccessView ) ) || m_bStaticAll ) && !m_strStaticFooter.empty( ) )
		pResponse->strContent += "<div class=\"static_footer\">\n" + m_strStaticFooter + "\n</div>\n\n";
	
	// display the powered_by logo
	if( bIndex )
	{
		// Display Powered By
		pResponse->strContent += "<p class=\"powered_by\">POWERED By \n";
		pResponse->strContent += "<a rel=\"" + STR_TARGET_REL + "\" title=\"http://xbnbt.sourceforge.net/\" href=\"http://xbnbt.sourceforge.net/\"\n";
		pResponse->strContent += "onMouseOver=\"window.status=\'" + XBNBT_VER + "\'; return true\"\n"; // BUILD INFORMATION
		pResponse->strContent += "onMouseOut=\"window.status=window.defaultStatus; return true\">" + XBNBT_VER + "</a> \n";
		pResponse->strContent += "based on BNBT " + string( BNBT_VER ) + "</p>\n\n";
	}

	// Display HTML, CSS & RSS validators
// 	if( m_ucShowValidator != 0 || ( pRequest->user.ucAccess & ACCESS_ADMIN ) )
	if( m_ucShowValidator != 0 )
	{
		string cstrCynthia = string( );
		const string cstrHashString( pRequest->mapParams["info_hash"] );

		switch( m_ucShowValidator )
		{
		case VALID_ADMIN:
		case VALID_TEXT:
			// show validator link only, default for admin
			pResponse->strContent += "<p class=\"html_valid_" + cstrCSS + "\">\n";
			// HTML
			pResponse->strContent += "<a rel=\"" + STR_TARGET_REL + "\" title=\"" + gmapLANG_CFG["valid_html"] + "\" href=\"http://validator.w3.org/check?uri=referer\">" + gmapLANG_CFG["valid_html"] + "</a>\n";
			// CSS
			pResponse->strContent += "<span class=\"pipe\">|</span><a rel=\"" + STR_TARGET_REL + "\" title=\"" + gmapLANG_CFG["valid_css"] + "\" href=\"http://jigsaw.w3.org/css-validator/check/referer\">" + gmapLANG_CFG["valid_css"] + "</a>\n";

			// RSS
			if( !rssdump.strName.empty( ) )
			{
				if( !rssdump.strURL.empty( ) )
					pResponse->strContent += "<span class=\"pipe\">|</span><a rel=\"" + STR_TARGET_REL + "\" class=\"rss_valid\" title=\"" + gmapLANG_CFG["valid_rss"] + "\" href=\"http://www.feedvalidator.org/check?url=" + rssdump.strURL + rssdump.strName + "\">" + gmapLANG_CFG["valid_rss"] + "</a>\n";
				else if( m_bServeLocal )
					pResponse->strContent += "<span class=\"pipe\">|</span><script type=\"text/javascript\">" + m_strValidate1 + "</script>";
			}

			// Cynthia
			if( cstrHashString.empty( ) )
				cstrCynthia = UTIL_Xsprintf( "document.write( \"<a rel=\'%s\' title=\'%s\' href=\'http://www.contentquality.com/mynewtester/cynthia.exe?rptmode=2&url1=http://\" + parent.location.host + \"%s\'>%s<\\/a>\" );", STR_TARGET_REL.c_str( ), gmapLANG_CFG["cynthia_tested"].c_str( ), string( pRequest->strURL ).c_str( ), gmapLANG_CFG["cynthia_tested"].c_str( ) );
			else
				cstrCynthia = UTIL_Xsprintf( "document.write( \"<a rel=\'%s\' title=\'%s\' href=\'http://www.contentquality.com/fulloptions.asp?rptmode=2&url1=http://\" + parent.location.host + \"%s?info_hash=%s\'>%s<\\/a>\" );", STR_TARGET_REL.c_str( ), gmapLANG_CFG["cynthia_tested"].c_str( ), string( pRequest->strURL ).c_str( ), cstrHashString.c_str( ), gmapLANG_CFG["cynthia_tested"].c_str( ) );

			pResponse->strContent += "<span class=\"pipe\">|</span><script type=\"text/javascript\">" + cstrCynthia + "</script></p>\n\n";

			break;
		case VALID_IMAGE:
			// show validator with image
			pResponse->strContent += "<p class=\"html_valid_" + cstrCSS + "\">\n";
			// HTML
			pResponse->strContent += "<a rel=\"" + STR_TARGET_REL + "\" title=\"" + gmapLANG_CFG["valid_html"] + "\" href=\"http://validator.w3.org/check?uri=referer\"><img style=\"border:0;width:88px;height:31px\" src=\"http://www.w3.org/Icons/valid-html401\" alt=\"" + gmapLANG_CFG["valid_html"] + "\"></a>\n";
			// CSS
			pResponse->strContent += "<span class=\"pipe\">|</span><a rel=\"" + STR_TARGET_REL + "\" title=\"" + gmapLANG_CFG["valid_css"] + "\" href=\"http://jigsaw.w3.org/css-validator/check/referer\"><img style=\"border:0;width:88px;height:31px\" src=\"http://jigsaw.w3.org/css-validator/images/vcss\" alt=\"" + gmapLANG_CFG["valid_css"] + "\"></a>\n";

			// RSS
			if( !rssdump.strName.empty( ) )
			{
				if( !rssdump.strURL.empty( ) )
					pResponse->strContent += "<span class=\"pipe\">|</span><a rel=\"" + STR_TARGET_REL + "\" class=\"rss_valid\" title=\"" + gmapLANG_CFG["valid_rss"] + "\" href=\"http://www.feedvalidator.org/check?url=" + rssdump.strURL + rssdump.strName + "\"><img style=\"border:0;width:88px;height:31px\" src=\"" + m_strRSSValidImage + "\" alt=\"" + gmapLANG_CFG["valid_rss"] + "\"</a>\n";
				else if( m_bServeLocal )
					pResponse->strContent += "<span class=\"pipe\">|</span><script type=\"text/javascript\">" + m_strValidate2 + "</script>\n";
			}

			// Cynthia
			if( cstrHashString.empty( ) )
				cstrCynthia = UTIL_Xsprintf( "document.write( \"<a rel=\'%s\' title=\'%s\' href=\'http://www.contentquality.com/mynewtester/cynthia.exe?rptmode=2&url1=http://\" + parent.location.host + \"%s\'><img style=\'border:0;width:88px;height:31px\' src=\'http://www.CynthiaSays.com/images/Ctested.gif\' alt=\'%s\'><\\/a>\" );", STR_TARGET_REL.c_str( ), gmapLANG_CFG["cynthia_tested"].c_str( ), string( pRequest->strURL ).c_str( ), gmapLANG_CFG["cynthia_tested"].c_str( ) );
			else
				cstrCynthia = UTIL_Xsprintf( "document.write( \"<a rel=\'%s\' title=\'%s\' href=\'http://www.contentquality.com/fulloptions.asp?rptmode=2&url1=http://\" + parent.location.host + \"%s?info_hash=%s\'><img style=\'border:0;width:88px;height:31px\' src=\'http://www.CynthiaSays.com/images/Ctested.gif\' alt=\'%s\'><\\/a>\" );", STR_TARGET_REL.c_str( ), gmapLANG_CFG["cynthia_tested"].c_str( ), string( pRequest->strURL ).c_str( ), cstrHashString.c_str( ), gmapLANG_CFG["cynthia_tested"].c_str( ) );

			pResponse->strContent += "<span class=\"pipe\">|</span><script type=\"text/javascript\">" + cstrCynthia + "</script>\n";

			// XBNBT Home Page
			pResponse->strContent += "<span class=\"pipe\">|</span><a rel=\"" + STR_TARGET_REL + "\" title=\"XBNBT Home Page\" href=\"http://xbnbt.sourceforge.net/\"><img style=\"border:0;width:88px;height:31px\" src=\"http://sourceforge.net/sflogo.php?group_id=115094&type=1\" alt=\"SF Logo (XBNBT)\"></a>\n</p>\n\n";

			break;
		default:
			// Do not show validator
			;
		}
	}

	// Final responses
	// XML Reset Alert
	if( m_bFlagXMLAlert )
	{
		pResponse->strContent += "<script type=\"text/javascript\">alert(\"" + gmapLANG_CFG["admin_dumping_xml"] + "\")</script>\n\n";
		m_bFlagXMLAlert = false;
	}

	// RSS Reset Alert
	if( m_bFlagRSSAlert )
	{
		pResponse->strContent += "<script type=\"text/javascript\">alert(\"" + gmapLANG_CFG["admin_rss_message"] + "\")</script>\n\n";
		m_bFlagRSSAlert = false;
	}

	// Link Reset Alert
	if( m_bFlagNotOwnLinkAlert )
	{
		pResponse->strContent += "<script type=\"text/javascript\">alert(\"" + gmapLANG_CFG["admin_not_owner"] + "\")</script>\n\n";
		m_bFlagNotOwnLinkAlert = false;
	}

	// HUB Link Reset Alert
	if( m_bFlagNotOwnHUBLinkAlert )
	{
		pResponse->strContent += "<script type=\"text/javascript\">alert(\"" + gmapLANG_CFG["admin_not_owner"] + "\")</script>\n\n";
		m_bFlagNotOwnHUBLinkAlert = false;
	}

	// Standard page generation time
	if( m_bGen && ( pRequest->user.ucAccess & m_ucAccessView ) )
		pResponse->strContent  += "<p class=\"gen_" + cstrCSS + "\">" + UTIL_Xsprintf( gmapLANG_CFG["generated_time"].c_str( ), UTIL_ElapsedTimeStr( btv, UTIL_CurrentTime( ) ).c_str( ) ) + "</p>\n\n";
	
	// Close body and and html
	pResponse->strContent += "</body>\n</html>\n";
}   

//  Mutex for link and hublink
void CTracker :: QueueAnnounce( const struct announce_t &ann )
{
	// normally called from link.cpp and hublink.cpp
	m_mtxQueued.Claim( );
	m_vecQueued.push_back( ann );
	m_mtxQueued.Release( );
} 

#if defined ( XBNBT_GD )
// XBNBT Dynstat
void CTracker :: runGenerateDynstat( )
{
	//Is the option to generate enable?
	if( m_bDynstatGenerate )
	{
		// Set the start time
		const struct bnbttv btv( UTIL_CurrentTime( ) );

		// Inform the log file that we are parsing
		UTIL_LogPrint( string( gmapLANG_CFG["parsing_dynstat"] + "\n" ).c_str( ) );

		// Check the font
		if( m_strDynstatFontFile.empty( ) )
			UTIL_LogPrint( string( gmapLANG_CFG["font_file_not_set"] + "\n" ).c_str( ) );
		else if( !UTIL_CheckFile ( m_strDynstatFontFile.c_str( ) ) )
			UTIL_LogPrint( string( gmapLANG_CFG["font_file_not_found"] + "\n" ).c_str( ), m_strDynstatFontFile.c_str( ) );
		else
		{
			// Declare the images 
			gdImagePtr imWorking = 0;
			gdImagePtr imMaster = 0;

			// Declare color indexes 
			unsigned int uiBlack = 0;


			unsigned int uiWhite = 0;
			unsigned int uiLogobgcolor = 0;
			unsigned int uiLogofontcolor = 0;
			unsigned int uiUserfontcolor = 0;

			// Declare iSize
			unsigned int uiDynstatXSize = 0;
			unsigned int uiDynstatYSize = 0;

			// Allocate the image and pBuffer: m_uiDynstatXSize pixels across by m_uiDynstatYSize pixels tall
			if ( m_bDynstatSkin )
			{
				if( m_strDynstatSkinFile.empty( ) )
				{
					// Create the Master image
					imMaster = gdImageCreate( m_uiDynstatXSize, m_uiDynstatYSize );

					UTIL_LogPrint( string( gmapLANG_CFG["skin_file_not_set"] + "\n" ).c_str( ) );
				}
				else if( !UTIL_CheckFile( m_strDynstatSkinFile.c_str( ) ) )
				{
					// Create the Master image
					imMaster = gdImageCreate( m_uiDynstatXSize, m_uiDynstatYSize );

					UTIL_LogPrint( string( gmapLANG_CFG["skin_file_not_set"] + "\n" ).c_str( ), m_strDynstatSkinFile.c_str( ) );

				}
				else
				{
					FILE *pImageinFile = FILE_ERROR;

					pImageinFile = fopen( m_strDynstatSkinFile.c_str( ), "rb" );

					if( pImageinFile != FILE_ERROR )
					{
#ifdef WIN32
						struct stat stat_buf;

						fstat( fileno( pImageinFile ), &stat_buf );

						//Read the entire thing into a pBuffer that we allocate 
						char *pBuffer = (char *)malloc( stat_buf.st_size );

						fread( pBuffer, sizeof( char ), stat_buf.st_size, pImageinFile );
#endif
						switch( m_ucDynstatInType )
						{
						case PNG_FORMAT:
							// Create the Master image PNG
#ifdef WIN32
							imMaster = gdImageCreateFromPngPtr( stat_buf.st_size, pBuffer );
#else
							imMaster = gdImageCreateFromPng( pImageinFile );
#endif
							break;
						case JPG_FORMAT:
							// Create the Master image from JPG
#ifdef WIN32
							imMaster = gdImageCreateFromJpegPtr( stat_buf.st_size, pBuffer );
#else
							imMaster = gdImageCreateFromJpeg( pImageinFile );
#endif
							break;
						case GIF_FORMAT:
							// Create the Master image from GIF
#ifdef WIN32
							imMaster = gdImageCreateFromGifPtr( stat_buf.st_size, pBuffer );
#else
							imMaster = gdImageCreateFromGif( pImageinFile );
#endif
							break;
						default:
							// Create the Master image
							imMaster = gdImageCreate( m_uiDynstatXSize, m_uiDynstatYSize );
							UTIL_LogPrint( string( gmapLANG_CFG["skin_format_not_known"] + "\n" ).c_str( ), m_strDynstatSkinFile.c_str( ) );
						}
#ifdef WIN32
						// Free the memory
						free( pBuffer );
#endif
						// Close the file
						fclose( pImageinFile );
					}
					else
					{
						UTIL_LogPrint( "Unable to open image master (%s)\n", m_strDynstatSkinFile.c_str( ) );

						// Create the Master image
						imMaster = gdImageCreate( m_uiDynstatXSize, m_uiDynstatYSize );
					}
				}
			}
			else
				// Create the Master image
				imMaster = gdImageCreate( m_uiDynstatXSize, m_uiDynstatYSize );

			// Set X and Y iSize
			uiDynstatXSize = gdImageSX( imMaster );
			uiDynstatYSize = gdImageSY( imMaster );

			// Create Working image of same iSize as Master image
			if( imMaster->trueColor != 0 )
			{
				imWorking = gdImageCreateTrueColor( uiDynstatXSize, uiDynstatYSize );

				if( gbDebug )
					if( gucDebugLevel & DEBUG_TRACKER )
						UTIL_LogPrint( string( gmapLANG_CFG["skin_true_colour"] + "\n" ).c_str( ) );
			}
			else
			{
				imWorking = gdImageCreate( uiDynstatXSize, uiDynstatYSize );

				if( gbDebug )
					if( gucDebugLevel & DEBUG_TRACKER )
						UTIL_LogPrint( string( gmapLANG_CFG["skin_pallete_colour"] + "\n" ).c_str( ) );
			}

			// Copy Master to working
			gdImageCopy(imWorking, imMaster, 0, 0, 0, 0, uiDynstatXSize, uiDynstatYSize );

			if( imWorking->trueColor != 0 )
			{

				// Allocate the color uiBlack (red, green and blue all minimum).
				// Since this is the first color in a new image, it will be the background color. 
				uiBlack = gdImageColorResolve(imWorking, 0, 0, 0);  
				// Allocate the color uiWhite (red, green and blue all maximum). 
				uiWhite = gdImageColorResolve(imWorking, 255, 255, 255);  
				// Allocate the color for the logo background 
				uiLogobgcolor = gdImageColorResolve(imWorking, 200, 200, 255);      
				// Allocate the color for the logo font 
				uiLogofontcolor = gdImageColorResolve(imWorking, 0, 0, 0);      
				// Allocate the user defined font color
				uiUserfontcolor = gdImageColorResolve(imWorking, m_ucDynstatFontRed, m_ucDynstatFontGreen, m_ucDynstatFontBlue);      
			}
			else
			{
				// Allocate the color uiBlack (red, green and blue all minimum).
				// Since this is the first color in a new image, it will be the background color. 
				uiBlack = gdImageColorAllocate(imWorking, 0, 0, 0);  
				// Allocate the color uiWhite (red, green and blue all maximum). 
				uiWhite = gdImageColorAllocate(imWorking, 255, 255, 255);  
				// Allocate the color for the logo background 
				uiLogobgcolor = gdImageColorAllocate(imWorking, 200, 200, 255);      
				// Allocate the color for the logo font 
				uiLogofontcolor = gdImageColorAllocate(imWorking, 0, 0, 0);      
				// Allocate the user defined font color
				uiUserfontcolor = gdImageColorResolve(imWorking, m_ucDynstatFontRed, m_ucDynstatFontGreen, m_ucDynstatFontBlue);      
			}

				// Start
				if( m_pDFile )
				{
					map<string, CAtom *> *pmapDicti = m_pDFile->getValuePtr( );

					const unsigned long culKeySize( (unsigned long)pmapDicti->size( ) );

					// add the torrents into this structure one by one and sort it afterwards

					struct torrent_t *pTorrents = new struct torrent_t[culKeySize];

					// Allocate for loop
					unsigned long ulKeyCount = 0;

					vector<CAtom *> vecTorrent;
					vecTorrent.reserve( 6 );

					CAtom *pList = 0;
					CAtom *pFileName = 0;
					CAtom *pName = 0;
					CAtom *pAdded = 0;
					CAtom *pSize = 0;
					CAtom *pFiles = 0;
					CAtom *pDicti = 0;
					CAtom *pTag = 0;
// 					CAtom *pFastCache = 0;
					CAtom *pDynstat = 0;
					CAtom *pIgnore = 0;
					CAtom *pIgnored = 0;

					for( map<string, CAtom *> :: iterator it = pmapDicti->begin( ); it != pmapDicti->end( ); it++ )
					{
						CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bfilename,bname,badded,bsize,bfiles,bseeders,bleechers FROM allowed WHERE bhash=\'" + UTIL_StringToMySQL( (*it).first ) + "\'" );
						
						vector<string> vecQuery;
						
						vecQuery.reserve(7);

						vecQuery = pQuery->nextRow( );
						
						if( vecQuery.size( ) == 7 )
						{

							pTorrents[ulKeyCount].strInfoHash = UTIL_HashToString( (*it).first );
							
							if( !vecQuery[0].empty( ) )
								pTorrents[ulKeyCount].strFileName = UTIL_RemoveHTML( vecQuery[0] );

							if( !vecQuery[1].empty( ) )
								pTorrents[ulKeyCount].strName += UTIL_RemoveHTML( vecQuery[1] );

							if( !vecQuery[2].empty( ) )
								pTorrents[ulKeyCount].strAdded = vecQuery[2];

							if( !vecQuery[3].empty( ) )
								pTorrents[ulKeyCount].iSize = UTIL_StringTo64( vecQuery[3].c_str( ) );

							if( !vecQuery[4].empty( ) )
								pTorrents[ulKeyCount].uiFiles = (unsigned int)atoi( vecQuery[4].c_str( ) );
							
							if( !vecQuery[5].empty( ) )
								pTorrents[ulKeyCount].uiSeeders = atoi( vecQuery[5].c_str( ) );
							
							if( !vecQuery[6].empty( ) )
								pTorrents[ulKeyCount].uiLeechers = atoi( vecQuery[6].c_str( ) );
						}
						
						delete pQuery;
						
						CMySQLQuery *pQueryTags = new CMySQLQuery( "SELECT bid,btag,bname FROM tags WHERE bhash=\'" + UTIL_StringToMySQL( (*ulKey).first ) + "\'" );
				
						vector<string> vecQueryTags;
					
						vecQueryTags.reserve(3);

						vecQueryTags = pQueryTags->nextRow( );

						if( vecQueryTags.size( ) == 3 )
						{
							if( !vecQueryTags[1].empty( ) )
								pTorrents[ulKeyCount].strTag = UTIL_RemoveHTML( vecQueryTags[1] );

							if( !vecQueryTags[2].empty( ) )
								pTorrents[ulKeyCount].strName = UTIL_RemoveHTML( vecQueryTags[2] );
						}
						
						delete pQueryTags;

						// Check and set frozen status
						if( m_pDynstat )
						{
							if( !m_pDynstat->getItem( (*it).first ) )
								m_pDynstat->setItem( (*it).first, new CAtomDicti( ) );

							pDynstat = m_pDynstat->getItem( (*it).first );

							if( pDynstat && pDynstat->isDicti( ) )
							{
								pIgnore = ( (CAtomDicti *)pDynstat )->getItem( "ignore" );
								pIgnored = ( (CAtomDicti *)pDynstat )->getItem( "ignored" );

								if( pIgnore )
									pTorrents[ulKeyCount].strIgnore = pIgnore->toString( );

								if( pIgnored )
									pTorrents[ulKeyCount].strIgnored = pIgnored->toString( );

								if( pTorrents[ulKeyCount].uiSeeders != 0 || pTorrents[ulKeyCount].uiLeechers !=0 )
								{
									( (CAtomDicti *)pDynstat )->setItem( "ignore", new CAtomString( "false" ) );
									( (CAtomDicti *)pDynstat )->setItem( "ignored", new CAtomString( "false" ) );

									pTorrents[ulKeyCount].strIgnore = "false";
									pTorrents[ulKeyCount].strIgnored = "false";
								}
								else
								{
									if( pTorrents[ulKeyCount].strIgnore == "true" ) 
									{
										( (CAtomDicti *)pDynstat )->setItem( "ignored", new CAtomString( "true" ) );

										pTorrents[ulKeyCount].strIgnored = "true";
									}
									else
									{
										( (CAtomDicti *)pDynstat )->setItem( "ignored", new CAtomString( "false" ) );

										pTorrents[ulKeyCount].strIgnored = "false";
									}

									( (CAtomDicti *)pDynstat )->setItem( "ignore", new CAtomString( "true" ) );

									pTorrents[ulKeyCount].strIgnore = "true";
								}
							}
						}

						ulKeyCount++;
					}

					// save the dictionary to file
					const string cstrData( Encode( m_pDynstat ) );

					FILE *pFile = FILE_ERROR;

					pFile = fopen( m_strDynstatFile.c_str( ), "wb" ) ;

					if( pFile == FILE_ERROR )
					{
						UTIL_LogPrint( string( gmapLANG_CFG["unable_to_write_file"] + "\n" ).c_str( ), m_strDynstatFile.c_str( ) );

						return;
					}

					fwrite( cstrData.c_str( ), sizeof( char ), cstrData.size( ), pFile );
					fclose( pFile );

					// Initialise XStats data
					gtXStats.dynstat.iFrozen = 0;
					gtXStats.dynstat.iProcessed = 0;

					// Create and write the images
					struct dynstat_t imagetext;
					unsigned long ulCount = 0; 

					int brect[8];

					unsigned char ucDynstatFontFile[256];
					unsigned char ucTextOutput[256];

#ifdef WIN32
					int iSize = 0;
					char *pData = 0;
#endif

					for( unsigned long ulKey = 0; ulCount < culKeySize && ulKey < culKeySize; ulKey++ )
					{
						if( pTorrents[ulKey].strIgnored == "true"  )
						{
							// Increment count and continue
							gtXStats.dynstat.iFrozen++;
							ulCount++;

							continue;
						}

						// Set the logo
						if( pTorrents[ulKey].strIgnore == "true"  )
							imagetext.strLogo = UTIL_Xsprintf( gmapLANG_CFG["dynstat_logo_frozen"].c_str( ), XBNBT_VER.c_str( ), UTIL_Date( ).c_str( ) );
						else
							imagetext.strLogo = UTIL_Xsprintf( gmapLANG_CFG["dynstat_logo"].c_str( ), XBNBT_VER.c_str( ), UTIL_Date( ).c_str( ), CAtomInt( m_uiDynstatDumpInterval ).toString( ).c_str( ) );

						// Set the title
						imagetext.strTitle = UTIL_Xsprintf( gmapLANG_CFG["dynstat_title"].c_str( ), pTorrents[ulKey].strName.c_str( ) );

						// Set the statistics
						imagetext.strSize = UTIL_Xsprintf( gmapLANG_CFG["dynstat_size"].c_str( ), UTIL_BytesToString( pTorrents[ulKey].iSize ).c_str( ) );
						imagetext.strFiles = UTIL_Xsprintf( gmapLANG_CFG["dynstat_files"].c_str( ), CAtomInt( pTorrents[ulKey].uiFiles ).toString( ).c_str( ) );
						imagetext.strTag = UTIL_Xsprintf( gmapLANG_CFG["dynstat_tag"].c_str( ), pTorrents[ulKey].strTag.c_str( ) );
						imagetext.strStats = UTIL_Xsprintf( "%s    %s    %s", UTIL_StringStripLF( imagetext.strSize ).c_str( ), UTIL_StringStripLF( imagetext.strFiles ).c_str( ), UTIL_StringStripLF( imagetext.strTag ).c_str( ) );

						//Set the info hash

						imagetext.strHash = UTIL_Xsprintf( gmapLANG_CFG["dynstat_hash"].c_str( ), pTorrents[ulKey].strInfoHash.c_str( ) );

						// Set the filename
						imagetext.strFilename = UTIL_Xsprintf( gmapLANG_CFG["dynstat_filename"].c_str( ), pTorrents[ulKey].strFileName.c_str( ) );

						// Set the peer details
						imagetext.strSeeders = UTIL_Xsprintf( gmapLANG_CFG["dynstat_seeders"].c_str( ), CAtomInt( pTorrents[ulKey].uiSeeders ).toString( ).c_str( ) );
						imagetext.strLeechers = UTIL_Xsprintf( gmapLANG_CFG["dynstat_leechers"].c_str( ), CAtomInt( pTorrents[ulKey].uiLeechers ).toString( ).c_str( ) );
						imagetext.strPeers = UTIL_Xsprintf( "%s    %s", UTIL_StringStripLF( imagetext.strSeeders ).c_str( ), UTIL_StringStripLF( imagetext.strLeechers ).c_str( ) );

						// Set image filename
						switch( m_ucDynstatSaveMode )
						{
						case IMAGE_BY_FILENAME:
							imagetext.strImageFileName = pTorrents[ulKey].strFileName + m_strImageOutExt;

							break;
						default:
							imagetext.strImageFileName = pTorrents[ulKey].strInfoHash + m_strImageOutExt;
						}

						// Set the image Path and Filename
						if( !imagetext.strImageFileName.empty( ) )
						{
							if( !m_strDynstatDumpFileDir.empty( ) )
							{
								if( UTIL_CheckDir( m_strDynstatDumpFileDir.c_str( ) ) )
									imagetext.strImageFullPath = m_strDynstatDumpFileDir + RESPONSE_STR_SEPERATOR +  imagetext.strImageFileName;   
								else
								{
									UTIL_LogPrint( string( gmapLANG_CFG["image_dir_not_found"] + "\n" ).c_str( ), m_strDynstatDumpFileDir.c_str( ) );

									continue;
								}
							}

							else
							{
								imagetext.strImageFullPath = imagetext.strImageFileName;   

								UTIL_LogPrint( string( gmapLANG_CFG["image_dir_not_set"] + "\n" ).c_str( ) );
							}
						}
						else
						{
							UTIL_LogPrint( string( gmapLANG_CFG["image_filename_empty"] + "\n" ).c_str( ) );

							continue;
						}

						// Output specified image
						// Open a Image file for writing. "wb" means "write binary", important under MSDOS, harmless under Unix. 
						FILE *pImageoutFile = FILE_ERROR;

						pImageoutFile = fopen( imagetext.strImageFullPath.c_str( ), "wb" );

						if( pImageoutFile != FILE_ERROR )
						{
							//Creating the Logo rectangle
							gdImageFilledRectangle( imWorking, 0, uiDynstatYSize - 16, uiDynstatXSize - 1, uiDynstatYSize - 1, uiLogobgcolor );

							// Write the text
							memset( brect, 0,  sizeof( brect ) / sizeof( int ) );

							memset( ucDynstatFontFile, 0, sizeof( ucDynstatFontFile ) / sizeof( char ) );
							snprintf( (char *)ucDynstatFontFile, sizeof( ucDynstatFontFile ) / sizeof( char ), "%s", m_strDynstatFontFile.c_str( ) );

							memset( ucTextOutput, 0, sizeof( ucTextOutput ) / sizeof( char ) );
							snprintf( (char *)ucTextOutput, sizeof( ucTextOutput ) / sizeof( char ), "%s", imagetext.strFilename.c_str( ) );
							gdImageStringFT( imWorking, &brect[0], uiUserfontcolor, (char *)ucDynstatFontFile, 8.0, 0.0, 4, uiDynstatYSize - 69, (char *)ucTextOutput );

							memset( ucTextOutput, 0,  sizeof( ucTextOutput ) / sizeof( char ) );
							snprintf( (char *)ucTextOutput, sizeof( ucTextOutput ) / sizeof( char ), "%s", imagetext.strTitle.c_str( ) );
							gdImageStringFT( imWorking, &brect[0], uiUserfontcolor, (char *)ucDynstatFontFile, 8.0, 0.0, 4, uiDynstatYSize - 57, (char *)ucTextOutput );

							memset( ucTextOutput, 0, sizeof( ucTextOutput ) / sizeof( char ) );
							snprintf( (char *)ucTextOutput, sizeof( ucTextOutput ) / sizeof( char ), "%s", imagetext.strHash.c_str( ) );
							gdImageStringFT( imWorking, &brect[0], uiUserfontcolor, (char *)ucDynstatFontFile, 8.0, 0.0, 4, uiDynstatYSize - 45, (char *)ucTextOutput );

							memset( ucTextOutput, 0, sizeof( ucTextOutput ) / sizeof( char ) );
							snprintf( (char *)ucTextOutput, sizeof( ucTextOutput ) / sizeof( char ), "%s", imagetext.strStats.c_str( ) );
							gdImageStringFT( imWorking, &brect[0], uiUserfontcolor, (char *)ucDynstatFontFile, 8.0, 0.0, 4, uiDynstatYSize - 33, (char *)ucTextOutput );

							memset( ucTextOutput, 0, sizeof( ucTextOutput ) / sizeof( char ) );
							snprintf( (char *)ucTextOutput, sizeof( ucTextOutput ) / sizeof( char ), "%s", imagetext.strPeers.c_str( ) );
							gdImageStringFT( imWorking, &brect[0], uiUserfontcolor, (char *)ucDynstatFontFile, 10.0, 0.0, 4, uiDynstatYSize - 19, (char *)ucTextOutput );

							memset( ucTextOutput, 0, sizeof( ucTextOutput ) / sizeof( char ) );
							snprintf( (char *)ucTextOutput, sizeof( ucTextOutput ) / sizeof( char ), "%s", imagetext.strLogo.c_str( ) );
							gdImageStringFT( imWorking, &brect[0], uiLogofontcolor, (char *)ucDynstatFontFile, 8.0, 0.0, 4, uiDynstatYSize - 4, (char *)ucTextOutput );
#ifdef WIN32
							iSize = 0;
							pData = 0;

							// Output the image to the disk.
							switch( m_ucDynstatOutType )
							{
							case JPG_FORMAT:
								// JPG format.
								pData = (char *) gdImageJpegPtr( imWorking, &iSize, m_cDynstatJPGQuality );
								fwrite( pData, 1, iSize, pImageoutFile );

								break;
							case GIF_FORMAT:
								// GIF format.
								pData = (char *) gdImageGifPtr( imWorking, &iSize );
								fwrite( pData, 1, iSize, pImageoutFile );

								break;
							default:
								// PNG format.
								pData = (char *) gdImagePngPtrEx( imWorking, &iSize, m_cDynstatPNGCompress );
								fwrite( pData, 1, iSize, pImageoutFile );
							}
#else
							// Output the image to the disk.
							switch( m_ucDynstatOutType )
							{
							case JPG_FORMAT:
								// JPG format.
								gdImageJpeg( imWorking, pImageoutFile, m_cDynstatJPGQuality );

								break;
							case GIF_FORMAT:
								// GIF format.
								gdImageGif( imWorking, pImageoutFile );

								break;
							default:
								// PNG format.
								gdImagePngEx( imWorking, pImageoutFile, m_cDynstatPNGCompress );
							}
#endif

#ifdef WIN32
							// Free the memory
							gdFree( pData );  
#endif
							// Close the files. 
							fclose( pImageoutFile );
						}
						else
						{
							UTIL_LogPrint( "Unable to create image file (%s)\n", imagetext.strImageFullPath.c_str( ) );

							// Create the Master image
							imMaster = gdImageCreate( m_uiDynstatXSize, m_uiDynstatYSize );
						}

						// Reset the image
						gdImageCopy( imWorking, imMaster, 0, 0, 0, 0, uiDynstatXSize, uiDynstatYSize );
						// Increment count
						gtXStats.dynstat.iProcessed++;
						gtXStats.dynstat.iTotalProcessed++;
						ulCount++;
					}

					// Free the memory
					delete [] pTorrents;
				}

			// Destroy the images in memory. 
			gdImageDestroy( imWorking );
			gdImageDestroy( imMaster );
		}

		// Set the next egeration interval
		m_ulDynstatDumpNext = GetTime( ) + m_uiDynstatDumpInterval * 60;
		// Update the XStats
		gtXStats.dynstat.sLastRunTime = UTIL_Date( );
		gtXStats.dynstat.iRun++;
		gtXStats.dynstat.sElapsedTime = UTIL_ElapsedTimeStr( btv, UTIL_CurrentTime( ) );
	}

}
#endif

CCache :: CCache( )
{
	bReset = true;
	bResetOffers = true;
	bResetUsers = true;
	bResort = true;
	bResortOffers = true;
	bResortUsers = true;
	ulSize = 0;
	ulSizeOffers = 0;
	ulSizeUsers = 0;
	pTorrents = 0;
	pOffers = 0;
	pUsers = 0;
	tLatest = 0;
	tLatestOffer = 0;
	resetCache( );
	resetCache( true );
}

CCache :: ~CCache( )
{
	delete [] pTorrents;
	delete [] pOffers;
	delete [] pUsers;
}

void CCache :: Reset( bool bOffer )
{
	if( bOffer )
	{
		bResetOffers = true;
		bResortOffers = true;
	}
	else
	{
		bReset = true;
		bResort = true;
	}
}

void CCache :: resetCache( bool bOffer )
{
	if( bReset && !bOffer )
	{
		if( pTorrents )
			delete [] pTorrents;
		
		ulSize = 0;
	
		CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bid,bfilename,bname,badded,bsize,bfiles,btag,btitle,bip,buploader,buploaderid,bimdb,bimdbid,bdefault_down,bdefault_up,bfree_down,bfree_up,UNIX_TIMESTAMP(bfree_to),btop,bclassic,breq,bnodownload,bseeders,bseeders6,bleechers,bleechers6,bcompleted,bcomments FROM allowed ORDER BY bid DESC" );
				
		vector<string> vecQuery;

		vecQuery.reserve(28);

		vecQuery = pQuery->nextRow( );
		
		if( vecQuery.size( ) == 28 )
		{
			struct tm time_tm;
			int64 year, month, day, hour, minute, second;
			sscanf( vecQuery[3].c_str( ), "%d-%d-%d %d:%d:%d",&year,&month,&day,&hour,&minute,&second );
			time_tm.tm_year = year-1900;
			time_tm.tm_mon = month-1;
			time_tm.tm_mday = day;
			time_tm.tm_hour = hour;
			time_tm.tm_min = minute;
			time_tm.tm_sec = second;
			tLatest = mktime(&time_tm);
		}

		// Populate the torrents structure for display
		
		ulSize = (unsigned long)pQuery->numRows( );
	
		pTorrents = new struct torrent_t[ulSize];

		// add the torrents into this structure one by one and sort ulKey afterwards

		unsigned long ulCount = 0;

		while( vecQuery.size( ) == 28 )
		{
			pTorrents[ulCount].strTag = "101";
			pTorrents[ulCount].strName = gmapLANG_CFG["unknown"];
			pTorrents[ulCount].strLowerName = gmapLANG_CFG["unknown"];
			pTorrents[ulCount].strID = string( );
			pTorrents[ulCount].uiSeeders = 0;
			pTorrents[ulCount].uiSeeders6 = 0;
			pTorrents[ulCount].uiLeechers = 0;
			pTorrents[ulCount].uiLeechers6 = 0;
			pTorrents[ulCount].ulCompleted = 0;
			pTorrents[ulCount].iSize = 0;
			pTorrents[ulCount].uiFiles = 0;
			pTorrents[ulCount].uiComments = 0;
			pTorrents[ulCount].bAllow = true;
			pTorrents[ulCount].ucTop = 0;
			pTorrents[ulCount].ucClassic = 0;
			pTorrents[ulCount].bReq = false;
			pTorrents[ulCount].iFreeDown = 100;
			pTorrents[ulCount].iFreeUp = 100;
			pTorrents[ulCount].iTimeDown = 100;
			pTorrents[ulCount].iTimeUp = 100;
			pTorrents[ulCount].iFreeTo = 0;
			pTorrents[ulCount].iDefaultDown = 100;
			pTorrents[ulCount].iDefaultUp = 100;
			
			if( !vecQuery[0].empty( ) )
				pTorrents[ulCount].strID = vecQuery[0];
		
			if( !vecQuery[1].empty( ) )
				pTorrents[ulCount].strFileName = vecQuery[1];

			if( !vecQuery[2].empty( ) )
			{
				// stick a lower case version in strNameLower for non case sensitive searching and sorting

				pTorrents[ulCount].strName = vecQuery[2];
				pTorrents[ulCount].strLowerName = UTIL_ToLower( vecQuery[2] );
			}
	
			if( !vecQuery[3].empty( ) )
				pTorrents[ulCount].strAdded = vecQuery[3];

			if( !vecQuery[4].empty( ) )
				pTorrents[ulCount].iSize = UTIL_StringTo64( vecQuery[4].c_str( ) );

			if( !vecQuery[5].empty( ) )
				pTorrents[ulCount].uiFiles = (unsigned int)atoi( vecQuery[5].c_str( ) );
			
			if( !vecQuery[6].empty( ) )
				pTorrents[ulCount].strTag = vecQuery[6];

			if( !vecQuery[7].empty( ) )
			{
				// this will overwrite the previous name, ulKey.e. the filename

				pTorrents[ulCount].strName = vecQuery[7];
				pTorrents[ulCount].strLowerName = UTIL_ToLower( vecQuery[7] );
			}
	
			if( !vecQuery[8].empty( ) )
				pTorrents[ulCount].strIP = vecQuery[8];

			if( !vecQuery[9].empty( ) )
				pTorrents[ulCount].strUploader = vecQuery[9];
	
			if( !vecQuery[10].empty( ) )
				pTorrents[ulCount].strUploaderID = vecQuery[10];
	
			if( !vecQuery[11].empty( ) )
				pTorrents[ulCount].strIMDb = vecQuery[11];
	
			if( !vecQuery[12].empty( ) )
				pTorrents[ulCount].strIMDbID = vecQuery[12];
	
			if( !vecQuery[13].empty( ) )
				pTorrents[ulCount].iDefaultDown = atoi( vecQuery[13].c_str( ) );
	
			if( !vecQuery[14].empty( ) )
				pTorrents[ulCount].iDefaultUp = atoi( vecQuery[14].c_str( ) );
	
			if( !vecQuery[15].empty( ) )
				pTorrents[ulCount].iTimeDown = atoi( vecQuery[15].c_str( ) );
	
			if( !vecQuery[16].empty( ) )
				pTorrents[ulCount].iTimeUp = atoi( vecQuery[16].c_str( ) );
	
			if( !vecQuery[17].empty( ) )
				pTorrents[ulCount].iFreeTo = UTIL_StringTo64( vecQuery[17].c_str( ) );
	
			if( !vecQuery[18].empty( ) )
				pTorrents[ulCount].ucTop = (unsigned char)atoi( vecQuery[18].c_str( ) );

			if( !vecQuery[19].empty( ) )
				pTorrents[ulCount].ucClassic = (unsigned char)atoi( vecQuery[19].c_str( ) );
	
			if( !vecQuery[20].empty( ) && vecQuery[20] == "1" )
				pTorrents[ulCount].bReq = true;
	
			if( !vecQuery[21].empty( ) && vecQuery[21] == "1" )
				pTorrents[ulCount].bAllow = false;

			pTorrents[ulCount].uiSeeders = atoi( vecQuery[22].c_str( ) );
			pTorrents[ulCount].uiSeeders6 = atoi( vecQuery[23].c_str( ) );
			pTorrents[ulCount].uiLeechers = atoi( vecQuery[24].c_str( ) );
			pTorrents[ulCount].uiLeechers6 = atoi( vecQuery[25].c_str( ) );
			pTorrents[ulCount].ulCompleted = atoi( vecQuery[26].c_str( ) );
			pTorrents[ulCount].uiComments = atoi( vecQuery[27].c_str( ) );

			ulCount++;

			vecQuery = pQuery->nextRow( );
		}
	
		delete pQuery;
	
		bReset = false;
		
		ucSort = SORT_DADDED;
	}
	
	if( bResetOffers && bOffer )
	{
		if( pOffers )
			delete [] pOffers;
		
		ulSizeOffers = 0;
		
		CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bid,bhash,bfilename,bname,badded,bsize,bfiles,btag,btitle,buploader,buploaderid,UNIX_TIMESTAMP(bseeded),bcomments FROM offer ORDER BY bid DESC" );
						
		vector<string> vecQuery;
		
		vecQuery.reserve(13);

		vecQuery = pQuery->nextRow( );
		
		if( vecQuery.size( ) == 13 )
		{
			struct tm time_tm;
			int64 year, month, day, hour, minute, second;
			sscanf( vecQuery[4].c_str( ), "%d-%d-%d %d:%d:%d",&year,&month,&day,&hour,&minute,&second );
			time_tm.tm_year = year-1900;
			time_tm.tm_mon = month-1;
			time_tm.tm_mday = day;
			time_tm.tm_hour = hour;
			time_tm.tm_min = minute;
			time_tm.tm_sec = second;
			tLatestOffer = mktime(&time_tm);
		}
		
		// Populate the torrents structure for display

		ulSizeOffers = (unsigned long)pQuery->numRows( );

		pOffers = new struct torrent_t[ulSizeOffers];
		
		// add the torrents into this structure one by one and sort ulKey afterwards

		unsigned long ulCount = 0;
		
		while( vecQuery.size( ) == 13 )
		{
			pOffers[ulCount].strTag = "101";
			pOffers[ulCount].strName = gmapLANG_CFG["unknown"];
			pOffers[ulCount].strLowerName = gmapLANG_CFG["unknown"];
			pOffers[ulCount].strID = string( );
			pOffers[ulCount].uiSeeders = 0;
			pOffers[ulCount].ucTop = 0;
			pOffers[ulCount].iSize = 0;
			pOffers[ulCount].uiFiles = 0;
			pOffers[ulCount].uiComments = 0;
			
			if( !vecQuery[0].empty( ) )
				pOffers[ulCount].strID =vecQuery[0];
				
			if( !vecQuery[1].empty( ) )
				pOffers[ulCount].strInfoHash = vecQuery[1];
			
			if( !vecQuery[2].empty( ) )
				pOffers[ulCount].strFileName = vecQuery[2];

			if( !vecQuery[3].empty( ) )
			{
				// stick a lower case version in strNameLower for non case sensitive searching and sorting

				pOffers[ulCount].strName = vecQuery[3];
				pOffers[ulCount].strLowerName = UTIL_ToLower( vecQuery[3] );
			}

			if( !vecQuery[4].empty( ) )
				pOffers[ulCount].strAdded = vecQuery[4];

			if( !vecQuery[5].empty( ) )
				pOffers[ulCount].iSize = UTIL_StringTo64( vecQuery[5].c_str( ) );

			if( !vecQuery[6].empty( ) )
				pOffers[ulCount].uiFiles = (unsigned int)atoi( vecQuery[6].c_str( ) );

			if( !vecQuery[7].empty( ) )
				pOffers[ulCount].strTag = vecQuery[7];

			if( !vecQuery[8].empty( ) )
			{
				// this will overwrite the previous name, ulKey.e. the filename

				pOffers[ulCount].strName = vecQuery[8];
				pOffers[ulCount].strLowerName = UTIL_ToLower( vecQuery[8] );
			}

			if( !vecQuery[9].empty( ) )
				pOffers[ulCount].strUploader = vecQuery[9];
			
			if( !vecQuery[10].empty( ) )
				pOffers[ulCount].strUploaderID = vecQuery[10];
			
			if( !vecQuery[11].empty( ) && vecQuery[11] != "0" )
				pOffers[ulCount].uiSeeders = 1;
				
			if( !vecQuery[12].empty( ) )
				pOffers[ulCount].uiComments = atoi( vecQuery[12].c_str( ) );

			ulCount++;
			
			vecQuery = pQuery->nextRow( );
		}
		
		delete pQuery;
		
		bResetOffers = false;
		
		ucSortOffers = SORT_DADDED;
	}
}

struct torrent_t *CCache :: getCache( bool bOffer )
{
	resetCache( bOffer );
	
	if( bOffer )
		return pOffers;
	return pTorrents;
}

unsigned long CCache :: getSize( bool bOffer )
{
	resetCache( bOffer );
	
	if( bOffer )
	{
		if( pOffers )
			return ulSizeOffers;
		else
			return 0;
	}

	if( pTorrents )
		return ulSize;
	else
		return 0;
}

time_t CCache :: getLatest( bool bOffer )
{
	if( bOffer )
		return tLatestOffer;
	else
		return tLatest;
}


//void CCache :: addRow( const string cstrID, bool bOffer )
//{
//	if( !bOffer )
//	{
//		struct torrent_t *pNew = new struct torrent_t[ulSize+1];
//		
//		for( unsigned long ulKey = 0; ulKey < ulSize; ulKey++ )
//		{
//			pNew[ulKey+1].strID = pTorrents[ulKey].strID;
//			pNew[ulKey+1].strFileName = pTorrents[ulKey].strFileName;
//			pNew[ulKey+1].strName = pTorrents[ulKey].strName;
//			pNew[ulKey+1].strLowerName = pTorrents[ulKey].strLowerName;
//			pNew[ulKey+1].strAdded = pTorrents[ulKey].strAdded;
//			pNew[ulKey+1].iSize = pTorrents[ulKey].iSize;
//			pNew[ulKey+1].uiFiles = pTorrents[ulKey].uiFiles;
//			pNew[ulKey+1].strTag = pTorrents[ulKey].strTag;
//			pNew[ulKey+1].strIP = pTorrents[ulKey].strIP;
//			pNew[ulKey+1].strUploader = pTorrents[ulKey].strUploader;
//			pNew[ulKey+1].strUploaderID = pTorrents[ulKey].strUploaderID;
//			pNew[ulKey+1].strIMDb = pTorrents[ulKey].strIMDb;
//			pNew[ulKey+1].strIMDbID = pTorrents[ulKey].strIMDbID;
//			pNew[ulKey+1].iDefaultDown = pTorrents[ulKey].iDefaultDown;
//			pNew[ulKey+1].iDefaultUp = pTorrents[ulKey].iDefaultUp;
//			pNew[ulKey+1].iTimeDown = pTorrents[ulKey].iTimeDown;
//			pNew[ulKey+1].iTimeUp = pTorrents[ulKey].iTimeUp;
//			pNew[ulKey+1].iFreeTo = pTorrents[ulKey].iFreeTo;
//			pNew[ulKey+1].bTop = pTorrents[ulKey].bTop;
//			pNew[ulKey+1].bHL = pTorrents[ulKey].bHL;
//			pNew[ulKey+1].bClassic = pTorrents[ulKey].bClassic;
//			pNew[ulKey+1].bReq = pTorrents[ulKey].bReq;
//			pNew[ulKey+1].bAllow = pTorrents[ulKey].bAllow;
//			pNew[ulKey+1].uiSeeders = pTorrents[ulKey].uiSeeders;
//			pNew[ulKey+1].uiLeechers = pTorrents[ulKey].uiLeechers;
//			pNew[ulKey+1].ulCompleted = pTorrents[ulKey].ulCompleted;
//			pNew[ulKey+1].uiComments = pTorrents[ulKey].uiComments;
//		}
//		
//		ulSize++;
//		
//		delete [] pTorrents;
//		
//		pTorrents = pNew;

//		CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bid,bfilename,bname,badded,bsize,bfiles,btag,btitle,bip,buploader,buploaderid,bimdb,bimdbid,bdefault_down,bdefault_up,bfree_down,bfree_up,UNIX_TIMESTAMP(bfree_to),btop,bhl,bclassic,breq,bnodownload,bseeders,bleechers,bcompleted,bcomments FROM allowed WHERE bid=" + cstrID );
//				
//		vector<string> vecQuery;

//		vecQuery.reserve(27);

//		vecQuery = pQuery->nextRow( );
//		
//		delete pQuery;

//		if( vecQuery.size( ) == 27 )
//		{
//			pTorrents[0].strTag = "101";
//			pTorrents[0].strName = gmapLANG_CFG["unknown"];
//			pTorrents[0].strLowerName = gmapLANG_CFG["unknown"];
//			pTorrents[0].strID = string( );
//			pTorrents[0].uiSeeders = 0;
//			pTorrents[0].uiLeechers = 0;
//			pTorrents[0].ulCompleted = 0;
//			pTorrents[0].iSize = 0;
//			pTorrents[0].uiFiles = 0;
//			pTorrents[0].uiComments = 0;
//			pTorrents[0].bAllow = true;
//			pTorrents[0].bTop = false;
//			pTorrents[0].bHL = false;
//			pTorrents[0].bClassic = false;
//			pTorrents[0].bReq = false;
//			pTorrents[0].iDefaultDown = 100;
//			pTorrents[0].iDefaultUp = 100;
//			pTorrents[0].iTimeDown = 100;
//			pTorrents[0].iTimeUp = 100;
//			pTorrents[0].iFreeTo = 0;
//			pTorrents[0].iFreeDown = 100;
//			pTorrents[0].iFreeUp = 100;
//		
//			if( !vecQuery[1].empty( ) )
//				pTorrents[0].strFileName = vecQuery[1];

//			if( !vecQuery[2].empty( ) )
//			{
//				// stick a lower case version in strNameLower for non case sensitive searching and sorting

//				pTorrents[0].strName = vecQuery[2];
//				pTorrents[0].strLowerName = UTIL_ToLower( pTorrents[0].strName );
//			}
//	
//			if( !vecQuery[3].empty( ) )
//				pTorrents[0].strAdded = vecQuery[3];

//			if( !vecQuery[4].empty( ) )
//				pTorrents[0].iSize = UTIL_StringTo64( vecQuery[4].c_str( ) );

//			if( !vecQuery[5].empty( ) )
//				pTorrents[0].uiFiles = (unsigned int)atoi( vecQuery[5].c_str( ) );

//			if( !vecQuery[0].empty( ) )
//				pTorrents[0].strID = vecQuery[0];
//			
//			if( !vecQuery[6].empty( ) )
//				pTorrents[0].strTag = vecQuery[6];

//			if( !vecQuery[7].empty( ) )
//			{
//				// this will overwrite the previous name, ulKey.e. the filename

//				pTorrents[0].strName = vecQuery[7];
//				pTorrents[0].strLowerName = UTIL_ToLower( pTorrents[0].strName );
//			}
//	
//			if( !vecQuery[8].empty( ) )
//				pTorrents[0].strIP = vecQuery[8];

//			if( !vecQuery[9].empty( ) )
//				pTorrents[0].strUploader = vecQuery[9];
//	
//			if( !vecQuery[10].empty( ) )
//				pTorrents[0].strUploaderID = vecQuery[10];
//	
//			if( !vecQuery[11].empty( ) )
//				pTorrents[0].strIMDb = vecQuery[11];
//	
//			if( !vecQuery[12].empty( ) )
//				pTorrents[0].strIMDbID = vecQuery[12];
//	
//			if( !vecQuery[13].empty( ) )
//				pTorrents[0].iDefaultDown = atoi( vecQuery[13].c_str( ) );
//	
//			if( !vecQuery[14].empty( ) )
//				pTorrents[0].iDefaultUp = atoi( vecQuery[14].c_str( ) );
//	
//			if( !vecQuery[15].empty( ) )
//				pTorrents[0].iTimeDown = atoi( vecQuery[15].c_str( ) );
//	
//			if( !vecQuery[16].empty( ) )
//				pTorrents[0].iTimeUp = atoi( vecQuery[16].c_str( ) );
//	
//			if( !vecQuery[17].empty( ) )
//				pTorrents[0].iFreeTo = UTIL_StringTo64( vecQuery[17].c_str( ) );
//	
//			if( !vecQuery[18].empty( ) && vecQuery[18] == "1" )
//				pTorrents[0].bTop = true;

//			if( !vecQuery[19].empty( ) && vecQuery[19] == "1" )
//				pTorrents[0].bHL = true;
//	
//			if( !vecQuery[20].empty( ) && vecQuery[20] == "1" )
//				pTorrents[0].bClassic = true;
//	
//			if( !vecQuery[21].empty( ) && vecQuery[21] == "1" )
//				pTorrents[0].bReq = true;
//	
//			if( !vecQuery[22].empty( ) && vecQuery[22] == "1" )
//				pTorrents[0].bAllow = false;

//			pTorrents[0].uiSeeders = (unsigned int)atoi( vecQuery[23].c_str( ) );
//			pTorrents[0].uiLeechers = (unsigned int)atoi( vecQuery[24].c_str( ) );
//			pTorrents[0].ulCompleted = (unsigned int)atoi( vecQuery[25].c_str( ) );
//			pTorrents[0].uiComments = (unsigned int)atoi( vecQuery[26].c_str( ) );
//		}
//	}
//	
//	if( bOffer )
//	{
//		struct torrent_t *pNew = new struct torrent_t[ulSizeOffer+1];
//		
//		for( unsigned long ulKey = 0; ulKey < ulSize; ulKey++ )
//		{
//			pNew[ulKey+1].strID = pOffers[ulKey].strID;
//			pNew[ulKey+1].strInfoHash = pOffers[ulKey].strInfoHash;
//			pNew[ulKey+1].strFileName = pOffers[ulKey].strFileName;
//			pNew[ulKey+1].strName = pOffers[ulKey].strName;
//			pNew[ulKey+1].strLowerName = pOffers[ulKey].strLowerName;
//			pNew[ulKey+1].strAdded = pOffers[ulKey].strAdded;
//			pNew[ulKey+1].iSize = pOffers[ulKey].iSize;
//			pNew[ulKey+1].uiFiles = pOffers[ulKey].uiFiles;
//			pNew[ulKey+1].strTag = pOffers[ulKey].strTag;
//			pNew[ulKey+1].strUploader = pOffers[ulKey].strUploader;
//			pNew[ulKey+1].strUploaderID = pOffers[ulKey].strUploaderID;
//			pNew[ulKey+1].uiSeeders = pOffers[ulKey].uiSeeders;
//			pNew[ulKey+1].uiComments = pOffers[ulKey].uiComments;
//		}
//		
//		ulSizeOffer++;
//		
//		delete [] pOffers;
//		
//		pOffers = pNew;
//		
//		CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bid,bhash,bfilename,bname,badded,bsize,bfiles,btag,btitle,buploader,buploaderid,UNIX_TIMESTAMP(bseeded),bcomments FROM offer" );
//						
//		vector<string> vecQuery;
//		
//		vecQuery.reserve(13);

//		vecQuery = pQuery->nextRow( );
//		
//		delete pQuery;
//		
//		if( vecQuery.size( ) == 13 )
//		{
//			pOffers[0].strTag = "101";
//			pOffers[0].strName = gmapLANG_CFG["unknown"];
//			pOffers[0].strLowerName = gmapLANG_CFG["unknown"];
//			pOffers[0].strID = string( );
//			pOffers[0].uiSeeders = 0;
//			pOffers[0].bTop = false;

//			pOffers[0].iSize = 0;
//			pOffers[0].uiFiles = 0;
//			pOffers[0].uiComments = 0;
//			
//			if( !vecQuery[0].empty( ) )
//				pOffers[0].strID =vecQuery[0];
//				
//			if( !vecQuery[1].empty( ) )
//				pOffers[0].strInfoHash =vecQuery[1];
//			
//			if( !vecQuery[2].empty( ) )
//				pOffers[0].strFileName = vecQuery[2];

//			if( !vecQuery[3].empty( ) )
//			{
//				// stick a lower case version in strNameLower for non case sensitive searching and sorting

//				pOffers[0].strName = vecQuery[3];
//				pOffers[0].strLowerName = UTIL_ToLower( pOffers[0].strName );
//			}

//			if( !vecQuery[4].empty( ) )
//				pOffers[0].strAdded = vecQuery[4];

//			if( !vecQuery[5].empty( ) )
//				pOffers[0].iSize = UTIL_StringTo64( vecQuery[5].c_str( ) );

//			if( !vecQuery[6].empty( ) )
//				pOffers[0].uiFiles = (unsigned int)atoi( vecQuery[6].c_str( ) );

//			if( !vecQuery[7].empty( ) )
//				pOffers[0].strTag = vecQuery[7];

//			if( !vecQuery[8].empty( ) )
//			{
//				// this will overwrite the previous name, ulKey.e. the filename

//				pOffers[0].strName = vecQuery[8];
//				pOffers[0].strLowerName = UTIL_ToLower( pOffers[0].strName );
//			}


//			if( !vecQuery[9].empty( ) )
//				pOffers[0].strUploader = vecQuery[9];
//			
//			if( !vecQuery[10].empty( ) )
//				pOffers[0].strUploaderID = vecQuery[10];
//			
//			if( !vecQuery[11].empty( ) && vecQuery[11] != "0" )
//				pOffers[0].uiSeeders = 1;
//				
//			if( !vecQuery[12].empty( ) )
//				pOffers[0].uiComments = (unsigned int)atoi( vecQuery[12].c_str( ) );
//		}
//	}
//}

void CCache :: sort( const unsigned char cucSort, bool bNoTop, bool bOffer )
{
	if( !bOffer && ( cucSort == ucSort ) && ( bNoTop == bSortNoTop ) && !bResort )
		return;
	if( bOffer && ( cucSort == ucSortOffers ) && !bResortOffers )
		return;
	
	resetCache( bOffer );
	
	struct torrent_t *pSort = 0;
	unsigned long ulKeySize = 0;
	
	if( bOffer )
	{
		pSort = pOffers;
		ulKeySize = ulSizeOffers;
		ucSortOffers = cucSort;
		bResortOffers = false;
	}
	else
	{
		pSort = pTorrents;
		ulKeySize = ulSize;
		ucSort = cucSort;
		bSortNoTop = bNoTop;
		bResort = false;
	}
	
	if( !bNoTop )
	{
		switch( cucSort )
		{
		case SORT_ANAME:
			qsort( pSort, ulKeySize, sizeof( struct torrent_t ), asortByName );
			break;
		case SORT_ACOMPLETE:
			qsort( pSort, ulKeySize, sizeof( struct torrent_t ), asortByComplete );
			break;
		case SORT_AINCOMPLETE:
			qsort( pSort, ulKeySize, sizeof( struct torrent_t ), asortByDL );
			break;
		case SORT_AADDED:
			qsort( pSort, ulKeySize, sizeof( struct torrent_t ), asortByAdded );
			break;
		case SORT_ASIZE:
			qsort( pSort, ulKeySize, sizeof( struct torrent_t ), asortBySize );
			break;
//		case SORT_AFILES:
//			qsort( pSort, ulKeySize, sizeof( struct torrent_t ), asortByFiles );
//			break;
//		case SORT_ACOMMENTS:
//			qsort( pSort, ulKeySize, sizeof( struct torrent_t ), asortByComments );
//			break;
// 				case SORT_AAVGLEFT:
// 					if( m_bShowAverageLeft )
// 					{
// 						if( m_bShowLeftAsProgress )
// 							qsort( pSort, ulKeySize, sizeof( struct torrent_t ), dsortByAvgLeftPercent );
// 						else
// 							qsort( pSort, ulKeySize, sizeof( struct torrent_t ), asortByAvgLeftPercent );
// 					}
// 					break;
		case SORT_ACOMPLETED:
			qsort( pSort, ulKeySize, sizeof( struct torrent_t ), asortByCompleted );
			break;
//				case SORT_ATRANSFERRED:
//					if( m_bShowTransferred )
//						qsort( pSort, ulKeySize, sizeof( struct torrent_t ), asortByTransferred );
//					break;
		case SORT_ATAG:
			qsort( pSort, ulKeySize, sizeof( struct torrent_t ), asortByTag );
			break;
		case SORT_AUPLOADER:
			qsort( pSort, ulKeySize, sizeof( struct torrent_t ), asortByUploader );
			break;
//				case SORT_AIP:
//					if( pRequest->user.ucAccess & m_ucAccessSortIP )
//						qsort( pSort, ulKeySize, sizeof( struct torrent_t ), asortByIP );
//					break;
		case SORT_DNAME:
			qsort( pSort, ulKeySize, sizeof( struct torrent_t ), dsortByName );
			break;
		case SORT_DCOMPLETE:
			qsort( pSort, ulKeySize, sizeof( struct torrent_t ), dsortByComplete );
			break;
		case SORT_DINCOMPLETE:
			qsort( pSort, ulKeySize, sizeof( struct torrent_t ), dsortByDL );
			break;
		case SORT_DADDED:
			qsort( pSort, ulKeySize, sizeof( struct torrent_t ), dsortByAdded );
			break;
		case SORT_DSIZE:
			qsort( pSort, ulKeySize, sizeof( struct torrent_t ), dsortBySize );
			break;
//		case SORT_DFILES:
//			qsort( pSort, ulKeySize, sizeof( struct torrent_t ), dsortByFiles );
//			break;
//		case SORT_DCOMMENTS:
//			qsort( pSort, ulKeySize, sizeof( struct torrent_t ), dsortByComments );
//			break;
// 				case SORT_DAVGLEFT:
// 					if( m_bShowAverageLeft )
// 					{
// 						if( m_bShowLeftAsProgress )
// 							qsort( pSort, ulKeySize, sizeof( struct torrent_t ), asortByAvgLeftPercent );
// 						else
// 							qsort( pSort, ulKeySize, sizeof( struct torrent_t ), dsortByAvgLeftPercent );
// 					}
// 					break;
		case SORT_DCOMPLETED:
			qsort( pSort, ulKeySize, sizeof( struct torrent_t ), dsortByCompleted );
			break;
//				case SORT_DTRANSFERRED:
//					if( m_bShowTransferred )
//						qsort( pSort, ulKeySize, sizeof( struct torrent_t ), dsortByTransferred );
//					break;
		case SORT_DTAG:
			qsort( pSort, ulKeySize, sizeof( struct torrent_t ), dsortByTag );
			break;
		case SORT_DUPLOADER:
			qsort( pSort, ulKeySize, sizeof( struct torrent_t ), dsortByUploader );
			break;
//				case SORT_DIP:
//					if( pRequest->user.ucAccess & m_ucAccessSortIP )
//						qsort( pSort, ulKeySize, sizeof( struct torrent_t ), dsortByIP );
//					break;
		default:
			// default action is to sort by added if we can
			qsort( pSort, ulKeySize, sizeof( struct torrent_t ), dsortByAdded );
		}
	}
	else
	{
		switch( cucSort )
		{
		case SORT_ANAME:
			qsort( pSort, ulKeySize, sizeof( struct torrent_t ), asortByNameNoTop );
			break;
		case SORT_ACOMPLETE:
			qsort( pSort, ulKeySize, sizeof( struct torrent_t ), asortByCompleteNoTop );

			break;
		case SORT_AINCOMPLETE:
			qsort( pSort, ulKeySize, sizeof( struct torrent_t ), asortByDLNoTop );
			break;
		case SORT_AADDED:
			qsort( pSort, ulKeySize, sizeof( struct torrent_t ), asortByAddedNoTop );
			break;
		case SORT_ASIZE:
			qsort( pSort, ulKeySize, sizeof( struct torrent_t ), asortBySizeNoTop );
			break;
//		case SORT_AFILES:
//			qsort( pSort, ulKeySize, sizeof( struct torrent_t ), asortByFilesNoTop );
//			break;
//		case SORT_ACOMMENTS:
//			qsort( pSort, ulKeySize, sizeof( struct torrent_t ), asortByCommentsNoTop );
//			break;
		case SORT_ACOMPLETED:
			qsort( pSort, ulKeySize, sizeof( struct torrent_t ), asortByCompletedNoTop );
			break;
		case SORT_ATAG:
			qsort( pSort, ulKeySize, sizeof( struct torrent_t ), asortByTagNoTop );
			break;
		case SORT_AUPLOADER:
			qsort( pSort, ulKeySize, sizeof( struct torrent_t ), asortByUploaderNoTop );
			break;
		case SORT_DNAME:
			qsort( pSort, ulKeySize, sizeof( struct torrent_t ), dsortByNameNoTop );
			break;
		case SORT_DCOMPLETE:
			qsort( pSort, ulKeySize, sizeof( struct torrent_t ), dsortByCompleteNoTop );
			break;
		case SORT_DINCOMPLETE:
			qsort( pSort, ulKeySize, sizeof( struct torrent_t ), dsortByDLNoTop );
			break;
		case SORT_DADDED:
			qsort( pSort, ulKeySize, sizeof( struct torrent_t ), dsortByAddedNoTop );
			break;
		case SORT_DSIZE:
			qsort( pSort, ulKeySize, sizeof( struct torrent_t ), dsortBySizeNoTop );
			break;
//		case SORT_DFILES:
//			qsort( pSort, ulKeySize, sizeof( struct torrent_t ), dsortByFilesNoTop );
//			break;
//		case SORT_DCOMMENTS:
//			qsort( pSort, ulKeySize, sizeof( struct torrent_t ), dsortByCommentsNoTop );
//			break;
		case SORT_DCOMPLETED:
			qsort( pSort, ulKeySize, sizeof( struct torrent_t ), dsortByCompletedNoTop );
			break;
		case SORT_DTAG:
			qsort( pSort, ulKeySize, sizeof( struct torrent_t ), dsortByTagNoTop );
			break;
		case SORT_DUPLOADER:
			qsort( pSort, ulKeySize, sizeof( struct torrent_t ), dsortByUploaderNoTop );
			break;
		default:
			// default action is to sort by added if we can
			qsort( pSort, ulKeySize, sizeof( struct torrent_t ), dsortByAddedNoTop );
		}
	}
}

void CCache :: setRow( const string &cstrID, bool bOffer )
{
	resetCache( bOffer );
	
	if( !bOffer )
	{
		for( unsigned long ulKey = 0; ulKey < ulSize; ulKey++ )
		{
			if( pTorrents[ulKey].strID == cstrID )
			{
				CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bid,bfilename,bname,badded,bsize,bfiles,btag,btitle,bip,buploader,buploaderid,bimdb,bimdbid,bdefault_down,bdefault_up,bfree_down,bfree_up,UNIX_TIMESTAMP(bfree_to),btop,bclassic,breq,bnodownload,bseeders,bseeders6,bleechers,bleechers6,bcompleted,bcomments FROM allowed WHERE bid=" + cstrID );
				
				vector<string> vecQuery;

				vecQuery.reserve(28);

				vecQuery = pQuery->nextRow( );
				
				delete pQuery;

				if( vecQuery.size( ) == 28 )
				{
					pTorrents[ulKey].strTag = "101";
					pTorrents[ulKey].strName = gmapLANG_CFG["unknown"];
					pTorrents[ulKey].strLowerName = gmapLANG_CFG["unknown"];
					pTorrents[ulKey].strID = string( );
					pTorrents[ulKey].uiSeeders = 0;
					pTorrents[ulKey].uiSeeders6 = 0;
					pTorrents[ulKey].uiLeechers = 0;
					pTorrents[ulKey].uiLeechers6 = 0;
					pTorrents[ulKey].ulCompleted = 0;
					pTorrents[ulKey].iSize = 0;
					pTorrents[ulKey].uiFiles = 0;
					pTorrents[ulKey].uiComments = 0;
					pTorrents[ulKey].bAllow = true;
					pTorrents[ulKey].ucTop = 0;
					pTorrents[ulKey].ucClassic = 0;
					pTorrents[ulKey].bReq = false;
					pTorrents[ulKey].iDefaultDown = 100;
					pTorrents[ulKey].iDefaultUp = 100;
					pTorrents[ulKey].iTimeDown = 100;
					pTorrents[ulKey].iTimeUp = 100;
					pTorrents[ulKey].iFreeTo = 0;
					pTorrents[ulKey].iFreeDown = 100;
					pTorrents[ulKey].iFreeUp = 100;
					
					if( !vecQuery[0].empty( ) )
						pTorrents[ulKey].strID = vecQuery[0];
	
					if( !vecQuery[1].empty( ) )
						pTorrents[ulKey].strFileName = vecQuery[1];

					if( !vecQuery[2].empty( ) )
					{
						// stick a lower case version in strNameLower for non case sensitive searching and sorting

						pTorrents[ulKey].strName = vecQuery[2];
						pTorrents[ulKey].strLowerName = UTIL_ToLower( vecQuery[2] );
					}

					if( !vecQuery[3].empty( ) )
						pTorrents[ulKey].strAdded = vecQuery[3];

					if( !vecQuery[4].empty( ) )
						pTorrents[ulKey].iSize = UTIL_StringTo64( vecQuery[4].c_str( ) );

					if( !vecQuery[5].empty( ) )
						pTorrents[ulKey].uiFiles = (unsigned int)atoi( vecQuery[5].c_str( ) );
		
					if( !vecQuery[6].empty( ) )
						pTorrents[ulKey].strTag = vecQuery[6];

					if( !vecQuery[7].empty( ) )
					{
						// this will overwrite the previous name, ulKey.e. the filename

						pTorrents[ulKey].strName = vecQuery[7];
						pTorrents[ulKey].strLowerName = UTIL_ToLower( vecQuery[7] );
					}

					if( !vecQuery[8].empty( ) )
						pTorrents[ulKey].strIP = vecQuery[8];

					if( !vecQuery[9].empty( ) )
						pTorrents[ulKey].strUploader = vecQuery[9];

					if( !vecQuery[10].empty( ) )
						pTorrents[ulKey].strUploaderID = vecQuery[10];

					if( !vecQuery[11].empty( ) )
						pTorrents[ulKey].strIMDb = vecQuery[11];

					if( !vecQuery[12].empty( ) )
						pTorrents[ulKey].strIMDbID = vecQuery[12];

					if( !vecQuery[13].empty( ) )
						pTorrents[ulKey].iDefaultDown = atoi( vecQuery[13].c_str( ) );

					if( !vecQuery[14].empty( ) )
						pTorrents[ulKey].iDefaultUp = atoi( vecQuery[14].c_str( ) );

					if( !vecQuery[15].empty( ) )
						pTorrents[ulKey].iTimeDown = atoi( vecQuery[15].c_str( ) );

					if( !vecQuery[16].empty( ) )
						pTorrents[ulKey].iTimeUp = atoi( vecQuery[16].c_str( ) );

					if( !vecQuery[17].empty( ) )
						pTorrents[ulKey].iFreeTo = UTIL_StringTo64( vecQuery[17].c_str( ) );

					if( !vecQuery[18].empty( ) )
						pTorrents[ulKey].ucTop = (unsigned char)atoi( vecQuery[18].c_str( ) );

					if( !vecQuery[19].empty( ) )
						pTorrents[ulKey].ucClassic = (unsigned char)atoi( vecQuery[19].c_str( ) );

					if( !vecQuery[20].empty( ) && vecQuery[20] == "1" )
						pTorrents[ulKey].bReq = true;

					if( !vecQuery[21].empty( ) && vecQuery[21] == "1" )
						pTorrents[ulKey].bAllow = false;

					pTorrents[ulKey].uiSeeders = atoi( vecQuery[22].c_str( ) );
					pTorrents[ulKey].uiSeeders6 = atoi( vecQuery[23].c_str( ) );
					pTorrents[ulKey].uiLeechers = atoi( vecQuery[24].c_str( ) );
					pTorrents[ulKey].uiLeechers6 = atoi( vecQuery[25].c_str( ) );
					pTorrents[ulKey].ulCompleted = atoi( vecQuery[26].c_str( ) );
					pTorrents[ulKey].uiComments = atoi( vecQuery[27].c_str( ) );
				}
				break;
			}
		}
		bResort = true;
	}
	
	if( bOffer )
	{
		for( unsigned long ulKey = 0; ulKey < ulSizeOffers; ulKey++ )
		{
			if( pOffers[ulKey].strID == cstrID )
			{
				CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bid,bhash,bfilename,bname,badded,bsize,bfiles,btag,btitle,buploader,buploaderid,UNIX_TIMESTAMP(bseeded),bcomments FROM offer WHERE bid=" + cstrID );
						
				vector<string> vecQuery;
		
				vecQuery.reserve(13);

				vecQuery = pQuery->nextRow( );
				
				delete pQuery;
		
				if( vecQuery.size( ) == 13 )
				{
					pOffers[ulKey].strTag = "101";
					pOffers[ulKey].strName = gmapLANG_CFG["unknown"];
					pOffers[ulKey].strLowerName = gmapLANG_CFG["unknown"];
					pOffers[ulKey].strID = string( );
					pOffers[ulKey].uiSeeders = 0;
					pOffers[ulKey].ucTop = 0;

					pOffers[ulKey].iSize = 0;
					pOffers[ulKey].uiFiles = 0;
					pOffers[ulKey].uiComments = 0;
			
					if( !vecQuery[0].empty( ) )
						pOffers[ulKey].strID =vecQuery[0];
						
					if( !vecQuery[1].empty( ) )
						pOffers[ulKey].strInfoHash =vecQuery[1];
			
					if( !vecQuery[2].empty( ) )
						pOffers[ulKey].strFileName = vecQuery[2];

					if( !vecQuery[3].empty( ) )
					{
						// stick a lower case version in strNameLower for non case sensitive searching and sorting

						pOffers[ulKey].strName = vecQuery[3];
						pOffers[ulKey].strLowerName = UTIL_ToLower( vecQuery[3] );
					}

					if( !vecQuery[4].empty( ) )
						pOffers[ulKey].strAdded = vecQuery[4];

					if( !vecQuery[5].empty( ) )
						pOffers[ulKey].iSize = UTIL_StringTo64( vecQuery[5].c_str( ) );

					if( !vecQuery[6].empty( ) )
						pOffers[ulKey].uiFiles = (unsigned int)atoi( vecQuery[6].c_str( ) );

					if( !vecQuery[7].empty( ) )
						pOffers[ulKey].strTag = vecQuery[7];

					if( !vecQuery[8].empty( ) )
					{
						// this will overwrite the previous name, ulKey.e. the filename


						pOffers[ulKey].strName = vecQuery[8];
						pOffers[ulKey].strLowerName = UTIL_ToLower( vecQuery[8] );
					}

					if( !vecQuery[9].empty( ) )
						pOffers[ulKey].strUploader = vecQuery[9];
			
					if( !vecQuery[10].empty( ) )
						pOffers[ulKey].strUploaderID = vecQuery[10];
			
					if( !vecQuery[11].empty( ) && vecQuery[11] != "0" )
						pOffers[ulKey].uiSeeders = 1;
						
					if( !vecQuery[12].empty( ) )
						pOffers[ulKey].uiComments = atoi( vecQuery[12].c_str( ) );
				}
				
				break;
			}
		}
		bResortOffers = true;
	}
}

void CCache :: setActive( const string &cstrID, const unsigned char cucOpt )
{
	resetCache( );
	
	for( unsigned long ulKey = 0; ulKey < ulSize; ulKey++ )
	{
		if( pTorrents[ulKey].strID == cstrID )
		{
			switch( cucOpt )
			{
			case SET_SEEDER_ADD:
				pTorrents[ulKey].uiSeeders++;
				break;
			case SET_SEEDER_ADD_V6:
				pTorrents[ulKey].uiSeeders6++;
				break;
			case SET_SEEDER_ADD_BOTH:
				pTorrents[ulKey].uiSeeders++;
				pTorrents[ulKey].uiSeeders6++;
				break;
			case SET_SEEDER_MINUS:
				if( pTorrents[ulKey].uiSeeders > 0 )
					pTorrents[ulKey].uiSeeders--;
				break;
			case SET_SEEDER_MINUS_BOTH:
				if( pTorrents[ulKey].uiSeeders > 0 )
				{
					pTorrents[ulKey].uiSeeders--;
					pTorrents[ulKey].uiSeeders6--;
				}
				break;
			case SET_LEECHER_ADD:
				pTorrents[ulKey].uiLeechers++;
				break;
			case SET_LEECHER_ADD_V6:
				pTorrents[ulKey].uiLeechers6++;
				break;
			case SET_LEECHER_ADD_BOTH:
				pTorrents[ulKey].uiLeechers++;
				pTorrents[ulKey].uiLeechers6++;
				break;
			case SET_LEECHER_MINUS:
				if( pTorrents[ulKey].uiLeechers > 0 )
					pTorrents[ulKey].uiLeechers--;
				break;
			case SET_LEECHER_MINUS_BOTH:
				if( pTorrents[ulKey].uiLeechers > 0 )
				{
					pTorrents[ulKey].uiLeechers--;
					pTorrents[ulKey].uiLeechers6--;
				}
				break;
			case SET_SEEDER_A_LEECHER_M:
				if( pTorrents[ulKey].uiLeechers > 0 )
				{
					pTorrents[ulKey].uiSeeders++;
					pTorrents[ulKey].uiLeechers--;
				}
				break;
			case SET_SEEDER_A_LEECHER_M_BOTH:
				if( pTorrents[ulKey].uiLeechers > 0 )
				{
					pTorrents[ulKey].uiSeeders++;
					pTorrents[ulKey].uiSeeders6++;
					pTorrents[ulKey].uiLeechers--;
					pTorrents[ulKey].uiLeechers6--;
				}
			}
			break;
		}
	}
	bResort = true;
}

void CCache :: setCompleted( const string &cstrID, const unsigned char cucOpt )
{
	resetCache( );
	
	for( unsigned long ulKey = 0; ulKey < ulSize; ulKey++ )
	{
		if( pTorrents[ulKey].strID == cstrID )
		{
			switch( cucOpt )
			{
			case SET_COMPLETED_ADD:
				pTorrents[ulKey].ulCompleted++;
				break;
			case SET_COMPLETED_MINUS:
				if( pTorrents[ulKey].ulCompleted > 0 )
					pTorrents[ulKey].ulCompleted--;
			}
			break;
		}
	}
	bResort = true;
}

void CCache :: setStatus( const string &cstrID, const unsigned char cucOpt )
{
	resetCache( );
	
	for( unsigned long ulKey = 0; ulKey < ulSize; ulKey++ )
	{
		if( pTorrents[ulKey].strID == cstrID )
		{
			switch( cucOpt )
			{
			case SET_STATUS_REQ:
				pTorrents[ulKey].bReq = true;
				break;
			case SET_STATUS_NOREQ:
				pTorrents[ulKey].bReq = false;
			}
			break;
		}
	}
}

void CCache :: setSeeded( const string &cstrID, const unsigned char cucOpt )
{
	resetCache( true );
	
	for( unsigned long ulKey = 0; ulKey < ulSizeOffers; ulKey++ )
	{
		if( pOffers[ulKey].strID == cstrID )
		{
			switch( cucOpt )
			{
			case SET_SEEDED_SEEDED:
				pOffers[ulKey].uiSeeders = 1;
				break;
			case SET_SEEDED_UNSEEDED:
				pOffers[ulKey].uiSeeders = 0;
			}
			break;
		}
	}
}

void CCache :: setComment( const string &cstrID, const unsigned char cucOpt, bool bOffer )
{
	resetCache( bOffer );
	
	struct torrent_t *pComment = 0;
	unsigned long ulKeySize = 0;
	
	if( bOffer )
	{
		pComment = pOffers;
		ulKeySize = ulSizeOffers;
//		bResortOffers = true;
	}
	else
	{
		pComment = pTorrents;
		ulKeySize = ulSize;
//		bResort = true;
	}
	
	for( unsigned long ulKey = 0; ulKey < ulKeySize; ulKey++ )
	{
		if( pComment[ulKey].strID == cstrID )
		{
			switch( cucOpt )
			{
			case SET_COMMENT_ADD:
				pComment[ulKey].uiComments++;
				break;
			case SET_COMMENT_MINUS:
				if( pComment[ulKey].uiComments > 0 )
					pComment[ulKey].uiComments--;
				break;
			case SET_COMMENT_CLEAR:
				pComment[ulKey].uiComments = 0;
			}
			break;
		}
	}
}

void CCache :: setFree( )
{
	resetCache( );
	
	time_t now_t = time( 0 );
	
	bool bFreeGlobal = CFG_GetInt( "bnbt_free_global", 0 ) == 0 ? false : true;
	
	int iFreeDownGlobal = CFG_GetInt( "bnbt_free_down_global", 100 );
	int iFreeUpGlobal = CFG_GetInt( "bnbt_free_up_global", 100 );
	
	for( unsigned long ulKey = 0; ulKey < ulSize; ulKey++ )
	{
		if( bFreeGlobal )
		{
			if( iFreeDownGlobal < pTorrents[ulKey].iDefaultDown )
				pTorrents[ulKey].iDefaultDown = iFreeDownGlobal;
			if( iFreeUpGlobal > pTorrents[ulKey].iDefaultUp )
				pTorrents[ulKey].iDefaultUp = iFreeUpGlobal;
		}
		
		pTorrents[ulKey].iFreeDown = pTorrents[ulKey].iDefaultDown;
		pTorrents[ulKey].iFreeUp = pTorrents[ulKey].iDefaultUp;

		if( pTorrents[ulKey].iFreeTo > 0 && pTorrents[ulKey].iFreeTo > now_t )
		{
			if( pTorrents[ulKey].iTimeDown < pTorrents[ulKey].iFreeDown )
				pTorrents[ulKey].iFreeDown = pTorrents[ulKey].iTimeDown;
			if( pTorrents[ulKey].iTimeUp > pTorrents[ulKey].iFreeUp )
				pTorrents[ulKey].iFreeUp = pTorrents[ulKey].iTimeUp;
		}
	}
}

void CCache :: setLatest( const string &cstrAdded, bool bOffer )
{
	struct tm time_tm;
	int64 year, month, day, hour, minute, second;
	sscanf( cstrAdded.c_str( ), "%d-%d-%d %d:%d:%d",&year,&month,&day,&hour,&minute,&second );
	time_tm.tm_year = year-1900;
	time_tm.tm_mon = month-1;
	time_tm.tm_mday = day;
	time_tm.tm_hour = hour;
	time_tm.tm_min = minute;
	time_tm.tm_sec = second;
	if( bOffer )
		tLatestOffer = mktime(&time_tm);
	else
		tLatest = mktime(&time_tm);
}

void CCache :: resetCacheUsers( )
{
	if( bResetUsers )
	{
		if( pUsers )
			delete [] pUsers;
		
		ulSizeUsers = 0;
		
		CMySQLQuery *pQuery = new CMySQLQuery( "SELECT buid,busername,bcreated,bemail,baccess,bgroup,buploaded,bdownloaded,bbonus,bseedbonus,UNIX_TIMESTAMP(blast),UNIX_TIMESTAMP(bwarned),binviter,binviterid FROM users ORDER BY buid DESC" );
		
		vector<string> vecQuery;

		vecQuery.reserve(14);

		vecQuery = pQuery->nextRow( );

		// Populate the users structure for display

		ulSizeUsers = (unsigned long)pQuery->numRows( );
		
		pUsers = new struct user_t[ulSizeUsers];

		// add the users into this structure one by one and sort it afterwards

		unsigned long ulCount = 0;

		while( vecQuery.size( ) == 14 )
		{
			pUsers[ulCount].strUID = vecQuery[0];
			pUsers[ulCount].strLogin = vecQuery[1];
			pUsers[ulCount].strLowerLogin = UTIL_ToLower( vecQuery[1] );
			pUsers[ulCount].ucAccess = 0;
			pUsers[ulCount].ucGroup = 0;
		
			if( !vecQuery[2].empty( ) )
				pUsers[ulCount].strCreated = vecQuery[2];

			if( !vecQuery[3].empty( ) )
			{
				pUsers[ulCount].strMail = vecQuery[3];
				pUsers[ulCount].strLowerMail = UTIL_ToLower( vecQuery[3] );
			}

			if( !vecQuery[4].empty( ) )
				pUsers[ulCount].ucAccess = (unsigned char)atoi( vecQuery[4].c_str( ) );
		
			if( !vecQuery[5].empty( ) )
				pUsers[ulCount].ucGroup = (unsigned char)atoi( vecQuery[5].c_str( ) );

			if( !vecQuery[6].empty( ) )
				pUsers[ulCount].ulUploaded = UTIL_StringTo64( vecQuery[6].c_str( ) );
		
			if( !vecQuery[7].empty( ) )
				pUsers[ulCount].ulDownloaded =UTIL_StringTo64( vecQuery[7].c_str( ) );
		
			if( pUsers[ulCount].ulDownloaded == 0 )
			{
				if( pUsers[ulCount].ulUploaded == 0 )
					pUsers[ulCount].flShareRatio = 0;
				else
					pUsers[ulCount].flShareRatio = -1;
			}
			else
				pUsers[ulCount].flShareRatio = (float)pUsers[ulCount].ulUploaded / (float)pUsers[ulCount].ulDownloaded;
		
			if( !vecQuery[8].empty( ) )
				pUsers[ulCount].ulBonus = UTIL_StringTo64( vecQuery[8].c_str( ) );
		
			if( !vecQuery[9].empty( ) )
				pUsers[ulCount].flSeedBonus = atof( vecQuery[9].c_str( ) );

			if( !vecQuery[10].empty( ) )
				pUsers[ulCount].tLast = UTIL_StringTo64( vecQuery[10].c_str( ) );
		
			if( !vecQuery[11].empty( ) )
				pUsers[ulCount].tWarned = UTIL_StringTo64( vecQuery[11].c_str( ) );
			
			if( !vecQuery[12].empty( ) )
				pUsers[ulCount].strInviter = vecQuery[12];
			
			if( !vecQuery[13].empty( ) )
				pUsers[ulCount].strInviterID = vecQuery[13];

			ulCount++;
		
			vecQuery = pQuery->nextRow( );
		}
	
		delete pQuery;
		
		bResetUsers = false;
		
		ucSortUsers = SORTU_DCREATED;
	}
}


void CCache :: ResetUsers( )
{
	bResetUsers = true;
	bResortUsers = true;
}

struct user_t *CCache :: getCacheUsers( )
{
	resetCacheUsers( );
	
	return pUsers;
}

unsigned long CCache :: getSizeUsers( )
{
	resetCacheUsers( );
	
	if( pUsers )
		return ulSizeUsers;
	else
		return 0;
}

void CCache :: sortUsers( const unsigned char cucSort )
{
	if( cucSort == ucSortUsers && !bResortUsers )
		return;
	
	resetCacheUsers( );

	ucSortUsers = cucSort;
	bResortUsers = false;
	
	switch ( cucSort )
	{
	case SORTU_ALOGIN:
		qsort( pUsers, ulSizeUsers, sizeof( struct user_t ), asortuByLogin );
		break;
	case SORTU_AACCESS:
		qsort( pUsers, ulSizeUsers, sizeof( struct user_t ), asortuByAccess );
		break;
	case SORTU_AGROUP:
		qsort( pUsers, ulSizeUsers, sizeof( struct user_t ), asortuByGroup );
		break;
	case SORTU_AINVITER:
		qsort( pUsers, ulSizeUsers, sizeof( struct user_t ), asortuByInviter );
		break;
	case SORTU_AEMAIL:
		qsort( pUsers, ulSizeUsers, sizeof( struct user_t ), asortuByMail );
		break;
	case SORTU_ACREATED:
		qsort( pUsers, ulSizeUsers, sizeof( struct user_t ), asortuByCreated );
		break;
	case SORTU_ALAST:
		qsort( pUsers, ulSizeUsers, sizeof( struct user_t ), asortuByLast );
		break;
	case SORTU_AWARNED:
		qsort( pUsers, ulSizeUsers, sizeof( struct user_t ), asortuByWarned );
		break;
	case SORTU_ASHARERATIO:
		qsort( pUsers, ulSizeUsers, sizeof( struct user_t ), asortuByShareRatio );
		break;
	case SORTU_AUPPED:
		qsort( pUsers, ulSizeUsers, sizeof( struct user_t ), asortuByUpped );
		break;
	case SORTU_ADOWNED:
		qsort( pUsers, ulSizeUsers, sizeof( struct user_t ), asortuByDowned );
		break;
	case SORTU_ABONUS:
		qsort( pUsers, ulSizeUsers, sizeof( struct user_t ), asortuByBonus );
		break;
	case SORTU_ASEEDBONUS:
		qsort( pUsers, ulSizeUsers, sizeof( struct user_t ), asortuBySeedBonus );
		break;
	case SORTU_DLOGIN:
		qsort( pUsers, ulSizeUsers, sizeof( struct user_t ), dsortuByLogin );
		break;
	case SORTU_DACCESS:
		qsort( pUsers, ulSizeUsers, sizeof( struct user_t ), dsortuByAccess );
		break;
	case SORTU_DGROUP:
		qsort( pUsers, ulSizeUsers, sizeof( struct user_t ), dsortuByGroup );
		break;
	case SORTU_DINVITER:
		qsort( pUsers, ulSizeUsers, sizeof( struct user_t ), dsortuByInviter );
		break;
	case SORTU_DEMAIL:
		qsort( pUsers, ulSizeUsers, sizeof( struct user_t ), dsortuByMail );
		break;
	case SORTU_DCREATED:
		qsort( pUsers, ulSizeUsers, sizeof( struct user_t ), dsortuByCreated );
		break;
	case SORTU_DLAST:
		qsort( pUsers, ulSizeUsers, sizeof( struct user_t ), dsortuByLast );
		break;
	case SORTU_DWARNED:
		qsort( pUsers, ulSizeUsers, sizeof( struct user_t ), dsortuByWarned );
		break;
	case SORTU_DSHARERATIO:
		qsort( pUsers, ulSizeUsers, sizeof( struct user_t ), dsortuByShareRatio );
		break;
	case SORTU_DUPPED:
		qsort( pUsers, ulSizeUsers, sizeof( struct user_t ), dsortuByUpped );
		break;
	case SORTU_DDOWNED:
		qsort( pUsers, ulSizeUsers, sizeof( struct user_t ), dsortuByDowned );
		break;
	case SORTU_DBONUS:
		qsort( pUsers, ulSizeUsers, sizeof( struct user_t ), dsortuByBonus );
		break;
	case SORTU_DSEEDBONUS:
		qsort( pUsers, ulSizeUsers, sizeof( struct user_t ), dsortuBySeedBonus );
		break;
	default:
		qsort( pUsers, ulSizeUsers, sizeof( struct user_t ), dsortuByCreated );
	}
}

void CCache :: setRowUsers( const string &cstrUID )
{
	resetCacheUsers( );
	
	for( unsigned long ulKey = 0; ulKey < ulSizeUsers; ulKey++ )
	{
		if( pUsers[ulKey].strUID == cstrUID )
		{
			CMySQLQuery *pQuery = new CMySQLQuery( "SELECT buid,busername,bcreated,bemail,baccess,bgroup,buploaded,bdownloaded,bbonus,bseedbonus,UNIX_TIMESTAMP(blast),UNIX_TIMESTAMP(bwarned),binviter,binviterid FROM users WHERE buid=" + cstrUID );
		
			vector<string> vecQuery;

			vecQuery.reserve(14);

			vecQuery = pQuery->nextRow( );
			
			delete pQuery;

			if( vecQuery.size( ) == 14 )
			{
//				pUsers[ulKey].strUID = vecQuery[0];
//				pUsers[ulKey].strLogin = vecQuery[1];
//				pUsers[ulKey].strLowerLogin = UTIL_ToLower( pUsers[ulKey].strLogin );
				pUsers[ulKey].ucAccess = 0;
				pUsers[ulKey].ucGroup = 0;
		
//				if( !vecQuery[2].empty( ) )
//					pUsers[ulKey].strCreated = vecQuery[2];

				if( !vecQuery[3].empty( ) )
				{
					pUsers[ulKey].strMail = vecQuery[3];
					pUsers[ulKey].strLowerMail = UTIL_ToLower( vecQuery[3] );
				}

				if( !vecQuery[4].empty( ) )
					pUsers[ulKey].ucAccess = (unsigned char)atoi( vecQuery[4].c_str( ) );
		
				if( !vecQuery[5].empty( ) )
					pUsers[ulKey].ucGroup = (unsigned char)atoi( vecQuery[5].c_str( ) );

				if( !vecQuery[6].empty( ) )
					pUsers[ulKey].ulUploaded = UTIL_StringTo64( vecQuery[6].c_str( ) );
		
				if( !vecQuery[7].empty( ) )
					pUsers[ulKey].ulDownloaded =UTIL_StringTo64( vecQuery[7].c_str( ) );
		
				if( pUsers[ulKey].ulDownloaded == 0 )
				{
					if( pUsers[ulKey].ulUploaded == 0 )
						pUsers[ulKey].flShareRatio = 0;
					else
						pUsers[ulKey].flShareRatio = -1;
				}
				else
					pUsers[ulKey].flShareRatio = (float)pUsers[ulKey].ulUploaded / (float)pUsers[ulKey].ulDownloaded;
		
				if( !vecQuery[8].empty( ) )
					pUsers[ulKey].ulBonus = UTIL_StringTo64( vecQuery[8].c_str( ) );
		
//				if( !vecQuery[9].empty( ) )
//					pUsers[ulKey].flSeedBonus = atof( vecQuery[9].c_str( ) );

//				if( !vecQuery[10].empty( ) )
//					pUsers[ulKey].tLast = UTIL_StringTo64( vecQuery[10].c_str( ) );
		
				if( !vecQuery[11].empty( ) )
					pUsers[ulKey].tWarned = UTIL_StringTo64( vecQuery[11].c_str( ) );
			
//				if( !vecQuery[12].empty( ) )
//					pUsers[ulKey].strInviter = vecQuery[12];
//			
//				if( !vecQuery[13].empty( ) )
//					pUsers[ulKey].strInviterID = vecQuery[13];
			}
			
			break;
		}
	}
	bResortUsers = true;
}

void CCache :: setUserData( const string &cstrUID, int64 ulUploaded, int64 ulDownloaded, int64 ulBonus )
{
	resetCacheUsers( );
	
	for( unsigned long ulKey = 0; ulKey < ulSizeUsers; ulKey++ )
	{
		if( pUsers[ulKey].strUID == cstrUID )
		{
			pUsers[ulKey].ulUploaded += ulUploaded;
			pUsers[ulKey].ulDownloaded += ulDownloaded;
			pUsers[ulKey].ulBonus += ulBonus;
			if( pUsers[ulKey].ulDownloaded == 0 )
			{
				if( pUsers[ulKey].ulUploaded == 0 )
					pUsers[ulKey].flShareRatio = 0;
				else
					pUsers[ulKey].flShareRatio = -1;
			}
			else
				pUsers[ulKey].flShareRatio = (float)pUsers[ulKey].ulUploaded / (float)pUsers[ulKey].ulDownloaded;
			break;
		}
	}
	bResortUsers = true;
}

//void CCache :: setUserStatus( const string &cstrUID, const unsigned char cucOpt )
//{
//	resetCacheUsers( );
//	
//	for( unsigned long ulKey = 0; ulKey < ulSizeUsers; ulKey++ )
//	{
//		if( pUsers[ulKey].strUID == cstrUID )
//		{
//			switch( cucOpt )
//			{
//			case SET_USER_SEEDING_ADD:
//				pUsers[ulKey].ulSeeding++;
//				break;
//			case SET_USER_SEEDING_MINUS:
//				if( pUsers[ulKey].ulSeeding > 0 )
//					pUsers[ulKey].ulSeeding--;
//				break;
//			case SET_USER_LEECHING_ADD:
//				pUsers[ulKey].ulLeeching++;
//				break;
//			case SET_USER_LEECHING_MINUS:
//				if( pUsers[ulKey].ulLeeching > 0 )
//					pUsers[ulKey].ulLeeching--;
//				break;
//			case SET_USER_SEEDING_A_LEECHING_M:
//				if( pUsers[ulKey].ulLeeching > 0 )
//				{
//					pUsers[ulKey].ulSeeding++;
//					pUsers[ulKey].ulLeeching--;
//				}
//			}
//			break;
//		}
//	}
//}

void CCache :: setLast( const string &cstrUID )

{
	resetCacheUsers( );
	
	for( unsigned long ulKey = 0; ulKey < ulSizeUsers; ulKey++ )
	{
		if( pUsers[ulKey].strUID == cstrUID )
		{
			pUsers[ulKey].tLast = GetTime( );
			break;
		}
	}
	bResortUsers = true;
}
