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
 #include <fcntl.h>
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
	m_strIntrDir = CFG_GetString( "bnbt_intr_dir", string( ) );
	m_strTopDir = CFG_GetString( "bnbt_top_dir", string( ) );
	m_strReqDir = CFG_GetString( "bnbt_req_dir", string( ) );
	m_strHLDir = CFG_GetString( "bnbt_hl_dir", string( ) );
	m_strPostDir = CFG_GetString( "bnbt_post_dir", string( ) );
	m_strUploadDir = CFG_GetString( "bnbt_upload_dir", string( ) );
	m_strExternalTorrentDir = CFG_GetString( "bnbt_external_torrent_dir", string( ) );
	m_strArchiveDir = CFG_GetString( "bnbt_archive_dir", string( ) );
	m_strFileDir = CFG_GetString( "bnbt_file_dir", string( ) );

	if( !m_strAllowedDir.empty( ) && m_strAllowedDir[m_strAllowedDir.size( ) - 1] != PATH_SEP )
		m_strAllowedDir += PATH_SEP;

	if( !m_strIntrDir.empty( ) && m_strIntrDir[m_strIntrDir.size( ) - 1] != PATH_SEP )
		m_strIntrDir += PATH_SEP;
	
	if( !m_strTopDir.empty( ) && m_strTopDir[m_strTopDir.size( ) - 1] != PATH_SEP )
		m_strTopDir += PATH_SEP;
	
	if( !m_strReqDir.empty( ) && m_strReqDir[m_strReqDir.size( ) - 1] != PATH_SEP )
		m_strReqDir += PATH_SEP;
	
	if( !m_strHLDir.empty( ) && m_strHLDir[m_strHLDir.size( ) - 1] != PATH_SEP )
		m_strHLDir += PATH_SEP;
	
	if( !m_strPostDir.empty( ) && m_strPostDir[m_strPostDir.size( ) - 1] != PATH_SEP )
		m_strPostDir += PATH_SEP;

	if( !m_strUploadDir.empty( ) && m_strUploadDir[m_strUploadDir.size( ) - 1] != PATH_SEP )
		m_strUploadDir += PATH_SEP;

	// DWK - Added support for external torrent.php and similar scripts that end with a http get paramater check
	if( !m_strExternalTorrentDir.empty( ) && m_strExternalTorrentDir[m_strExternalTorrentDir.size( ) - 1] != CHAR_FS && m_strExternalTorrentDir[m_strExternalTorrentDir.size( ) - 1] != '=' )
		m_strExternalTorrentDir += CHAR_FS;

	if( !m_strArchiveDir.empty( ) && m_strArchiveDir[m_strArchiveDir.size( ) - 1] != PATH_SEP )
		m_strArchiveDir += PATH_SEP;

	if( !m_strFileDir.empty( ) && m_strFileDir[m_strFileDir.size( ) - 1] != PATH_SEP )
		m_strFileDir += PATH_SEP;

	m_strDFile = CFG_GetString( "dfile", "dstate.bnbt" );
	m_strCommentsFile = CFG_GetString( "bnbt_comments_file", "comments.bnbt" );
	m_strTagFile = CFG_GetString( "bnbt_tag_file", "tags.bnbt" );
	m_strIPStateFile = CFG_GetString( "bnbt_ipstate_file", "ipstate.bnbt" );
	m_strUsersFile = CFG_GetString( "bnbt_users_file", "users.bnbt" );
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
	m_uiSaveDFileInterval = CFG_GetInt( "save_dfile_interval", 300 );
	m_uiDownloaderTimeOutInterval = CFG_GetInt( "downloader_timeout_interval", 2700 );
	m_uiRefreshStaticInterval = CFG_GetInt( "bnbt_refresh_static_interval", 10 );
	m_uiDumpRSSInterval = CFG_GetInt( "bnbt_rss_interval", 30 );
	m_uiMySQLRefreshAllowedInterval = CFG_GetInt( "mysql_refresh_allowed_interval", 300 );
	m_uiMySQLRefreshStatsInterval = CFG_GetInt( "mysql_refresh_stats_interval", 600 );
	m_uiRefreshFastCacheInterval = CFG_GetInt( "bnbt_refresh_fast_cache_interval", 30 );
	m_ulParseAllowedNext = GetTime( ) + m_uiParseAllowedInterval * 60;
	m_ulSaveDFileNext = GetTime( ) + m_uiSaveDFileInterval;
	m_ulPrevTime = 1;
	m_ulDownloaderTimeOutNext = GetTime( ) + m_uiDownloaderTimeOutInterval;
	m_ulRefreshStaticNext = 0;  
	m_ulDumpRSSNext = 0;
	m_bMySQLRefreshAllowedOnce = true;
	m_ulMySQLRefreshAllowedNext = 0;
	m_ulMySQLRefreshStatsNext = 0;
	m_ulRefreshFastCacheNext = 0;
	m_uiAnnounceInterval = CFG_GetInt( "announce_interval", 1800 );
	m_uiMinRequestInterval = CFG_GetInt( "min_request_interval", 18000 );
	m_uiResponseSize = CFG_GetInt( "response_size", 50 );
	m_uiMaxGive = CFG_GetInt( "max_give", 200 );
	m_bKeepDead = CFG_GetInt( "keep_dead", 0 ) == 0 ? false : true;
	m_bAllowScrape = CFG_GetInt( "bnbt_allow_scrape", 1 ) == 0 ? false : true;
	m_bCountUniquePeers = CFG_GetInt( "bnbt_count_unique_peers", 1 ) == 0 ? false : true;
	m_bDeleteInvalid = CFG_GetInt( "bnbt_delete_invalid", 0 ) == 0 ? false : true;
	m_bParseOnUpload = CFG_GetInt( "bnbt_parse_on_upload", 1 ) == 0 ? false : true;
	m_uiMaxTorrents = CFG_GetInt( "bnbt_max_torrents", 0 );
	m_bShowInfoHash = CFG_GetInt( "bnbt_show_info_hash", 0 ) == 0 ? false : true;
	m_bShowNames = CFG_GetInt( "show_names", 1 ) == 0 ? false : true;
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
	m_bAllowInfoLink = CFG_GetInt( "bnbt_allow_info_link", 0 ) == 0 ? false : true;
	m_bSearch = CFG_GetInt( "bnbt_allow_search", 1 ) == 0 ? false : true;
	m_bSort = CFG_GetInt( "bnbt_allow_sort", 1 ) == 0 ? false : true;
	m_bShowFileComment = CFG_GetInt( "bnbt_show_file_comment", 1 ) == 0 ? false : true;
	m_bShowFileContents = CFG_GetInt( "bnbt_show_file_contents", 0 ) == 0 ? false : true;
	m_bShowShareRatios = CFG_GetInt( "bnbt_show_share_ratios", 1 ) == 0 ? false : true;
	m_bShowAvgDLRate = CFG_GetInt( "bnbt_show_average_dl_rate", 0 ) == 0 ? false : true;
	m_bShowAvgULRate = CFG_GetInt( "bnbt_show_average_ul_rate", 0 ) == 0 ? false : true;
	m_bDeleteOwnTorrents = CFG_GetInt( "bnbt_delete_own_torrents", 1 ) == 0 ? false : true;
	m_bGen = CFG_GetInt( "bnbt_show_gen_time", 1 ) == 0 ? false : true;
	m_bMySQLOverrideDState = CFG_GetInt( "mysql_override_dstate", 0 ) == 0 ? false : true;
	m_uiPerPage = CFG_GetInt( "bnbt_per_page", 20 );
	m_uiUsersPerPage = CFG_GetInt( "bnbt_users_per_page", 50 );
	m_uiMaxPeersDisplay = CFG_GetInt( "bnbt_max_peers_display", 500 );
	m_ucGuestAccess = (unsigned char)CFG_GetInt( "bnbt_guest_access", ACCESS_VIEW | ACCESS_DL | ACCESS_SIGNUP );
	m_ucMemberAccess =(unsigned char)CFG_GetInt( "bnbt_member_access", ACCESS_VIEW | ACCESS_DL | ACCESS_COMMENTS | ACCESS_UPLOAD | ACCESS_SIGNUP );
	m_uiFileExpires = CFG_GetInt( "bnbt_file_expires", 180 );
	m_uiNameLength = CFG_GetInt( "bnbt_name_length", 32 );
	m_uiCommentLength = CFG_GetInt( "bnbt_comment_length", 800 );
	m_pAllowed = 0;
	m_pTimeDicti = new CAtomDicti( );
	m_pCached = new CAtomDicti( );
	m_pIPs = new CAtomDicti( );
	m_pFastCache = new CAtomDicti( );

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
	m_bTorrentTraderCompatibility = CFG_GetInt( "mysql_cbtt_ttrader_support", 0 ) == 0 ? false : true;
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
	m_strKeywords = CFG_GetString( "bnbt_tracker_keywords", string( ) );
	m_strLanguage = CFG_GetString( "bnbt_language", "en" );
	m_strRating = CFG_GetString( "bnbt_rating", string( ) );
	m_strWebmaster = CFG_GetString( "bnbt_webmaster", string( ) );
	// Announce 'key' support
	m_bAnnounceKeySupport = CFG_GetInt( "bnbt_use_announce_key", 1 ) == 0 ? false : true;
	// make public
	m_ucMakePublic = (unsigned char)CFG_GetInt( "bnbt_public_option", 0 );
	m_strPublicUploadDir = CFG_GetString( "bnbt_public_upload_dir", string( ) );

	if( !m_strPublicUploadDir.empty( ) && m_strPublicUploadDir[m_strPublicUploadDir.size( ) - 1] != PATH_SEP )
		m_strPublicUploadDir += PATH_SEP;

	// Announce & Scrape & Info Authentication Support
	m_ucAuthAnnounceAccess = (unsigned char)CFG_GetInt( "bnbt_announce_access_required", 0 );
	m_ucAuthScrapeAccess = (unsigned char)CFG_GetInt( "bnbt_scrape_access_required", 0 );
	m_ucInfoAccess = (unsigned char)CFG_GetInt( "bnbt_info_access_required", 0 );
	// Custom Announce and Scrape
	m_strCustomAnnounce = CFG_GetString( "bnbt_custom_announce", "/announce.php" );
	m_strCustomScrape = CFG_GetString( "bnbt_custom_scrape", "/scrape.php" );
	// Retrieves the "bnbt_use_mouseovers" value (thanks Trinity)
	m_bUseMouseovers = CFG_GetInt( "bnbt_use_mouseovers", 0 ) == 0 ? false : true;
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
	favicon.strExt = getFileExt( favicon.strName );

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
	m_strLogin1 = UTIL_Xsprintf( gmapLANG_CFG["login1"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["login"] + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "\">" ).c_str( ), "</a>" );
	// validate1=validate RSS without images
	m_strValidate1 = UTIL_Xsprintf( "document.write( \"<a rel=\'%s\' title=\'%s\' href=\'http://www.feedvalidator.org/check?url=http://\" + parent.location.host + \"/%s\'>%s<\\/a>\" );", STR_TARGET_REL.c_str( ), gmapLANG_CFG["valid_rss"].c_str( ), rssdump.strName.c_str( ), gmapLANG_CFG["valid_rss"].c_str( ) );
	// validate2=validate RSS with images
	m_strValidate2 = UTIL_Xsprintf( "document.write( \"<a rel=\'%s\' title=\'%s\' href=\'http://www.feedvalidator.org/check?url=http://\" + parent.location.host + \"/%s\'><img src=\'%s\' alt=\'%s\' height=\'31\' width=\'88\'><\\/a>\" );", STR_TARGET_REL.c_str( ), gmapLANG_CFG["valid_rss"].c_str( ), rssdump.strName.c_str( ), m_strRSSValidImage.c_str( ), gmapLANG_CFG["valid_rss"].c_str( ) );
	// JS Reduce Characters - JS_Valid_Check( )
	m_strJSReduce = UTIL_Xsprintf( gmapLANG_CFG["js_reduce_characters"].c_str( ), CAtomInt( m_uiCommentLength ).toString( ).c_str( ) );
	// JS Message Length - JS_Valid_Check( )
	m_strJSLength = UTIL_Xsprintf( gmapLANG_CFG["js_message_length"].c_str( ), "\" + theform.comment.value.length + \"" );
	// RSS local link for info.html
	m_strRSSLocalLink = UTIL_Xsprintf( "document.write( \"<a rel=\'%s\' title=\'%s\' href=\'http://\" + parent.location.host + \"/%s\'>http://\" + parent.location.host + \"/%s<\\/a>\" );", STR_TARGET_REL.c_str( ), gmapLANG_CFG["navbar_rss"].c_str( ), rssdump.strName.c_str( ), rssdump.strName.c_str( ) );

	// XBNBT initialise tags for internal mouseover
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "CTracker: Initialise XBNBT Tags\n" );

	initTags( );

#if defined ( XBNBT_MYSQL )
	// Initialise XBNBT MySQL Users variables
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "CTracker: Initialise XBNBT MySQL Users variables\n" );

	m_strForumLink = CFG_GetString( "xbnbt_mysqlusers_forums_link", string( ) );    
	m_uiMySQLUsersLoadInterval = CFG_GetInt( "xbnbt_mysqlusers_interval", 10 );  
	m_strMySQLUsersID = CFG_GetString( "xbnbt_mysqlusers_table_id", string( ) );
	m_strMySQLUsersGroup = CFG_GetString( "xbnbt_mysqlusers_table_group", string( ) );
	m_strMySQLUsersPassword = CFG_GetString( "xbnbt_mysqlusers_table_password", string( ) );
	m_strMySQLUsersEmail = CFG_GetString( "xbnbt_mysqlusers_table_email", string( ) );
	m_strMySQLUsersJoined = CFG_GetString( "xbnbt_mysqlusers_table_joined", string( ) );
	m_strMySQLUsersIgnoreGroup1 = CFG_GetString( "xbnbt_mysqlusers_ignore_group1", string( ) );
	m_strMySQLUsersIgnoreGroup2 = CFG_GetString( "xbnbt_mysqlusers_ignore_group2", string( ) );
	m_strMySQLUsersIgnoreGroup3 = CFG_GetString( "xbnbt_mysqlusers_ignore_group3", string( ) );
	m_ucMySQLUsersMode = (unsigned char)CFG_GetInt( "xbnbt_mysqlusers_mode", 0 );
#endif

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

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "CTracker: Decode dfile\n" );

	CAtom *pState = DecodeFile( m_strDFile.c_str( ) );

	if( pState && pState->isDicti( ) )
	{
		m_pState = (CAtomDicti *)pState;

		if( m_pState->getItem( "peers" ) && m_pState->getItem( "completed" ) )
		{
			CAtom *pDFile = m_pState->getItem( "peers" );

			if( pDFile && pDFile->isDicti( ) )
				m_pDFile = (CAtomDicti *)pDFile;

			CAtom *pCompleted = m_pState->getItem( "completed" );

			if( pCompleted && pCompleted->isDicti( ) )
				m_pCompleted = (CAtomDicti *)pCompleted;
		}
		else
		{
			if( pState )
				delete pState;

			m_pState = new CAtomDicti( );
			m_pDFile = new CAtomDicti( );
			m_pCompleted = new CAtomDicti( );

			m_pState->setItem( "peers", m_pDFile );
			m_pState->setItem( "completed", m_pCompleted );
		}

		// populate time dicti

		map<string, CAtom *> *pmapDicti = m_pDFile->getValuePtr( );

		CAtomDicti *pTS = 0;
		CAtomDicti *pPeersDicti = 0;
		CAtomDicti *pPeerDicti = 0;

		for( map<string, CAtom *> :: iterator it = pmapDicti->begin( ); it != pmapDicti->end( ); it++ )
		{
			pTS = new CAtomDicti( );

			if( (*it).second->isDicti( ) )
			{
				pPeersDicti = (CAtomDicti *)(*it).second;

				map<string, CAtom *> *pmapPeersDicti = pPeersDicti->getValuePtr( );

				for( map<string, CAtom *> :: iterator it2 = pmapPeersDicti->begin( ); it2 != pmapPeersDicti->end( ); it2++ )
					pTS->setItem( (*it2).first, new CAtomLong( 0 ) );

				// reset connected times

				for( map<string, CAtom *> :: iterator it2 = pmapPeersDicti->begin( ); it2 != pmapPeersDicti->end( ); it2++ )
				{
					if( (*it2).second->isDicti( ) )
					{
						pPeerDicti = (CAtomDicti *)(*it2).second;

						if( pPeerDicti )
							pPeerDicti->setItem( "connected", new CAtomLong( 0 ) );
					}
				}
			}

			m_pTimeDicti->setItem( (*it).first, pTS );
		}
	}
	else
	{
		if( pState )
			delete pState;

		m_pState = new CAtomDicti( );
		m_pDFile = new CAtomDicti( );
		m_pCompleted = new CAtomDicti( );

		m_pState->setItem( "peers", m_pDFile );
		m_pState->setItem( "completed", m_pCompleted );
	}

	// decode comments file

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "CTracker: Decode comments file\n" );

	if( m_strCommentsFile.empty( ) )
		m_pComments = new CAtomDicti( );
	else
	{
		CAtom *pComments = DecodeFile( m_strCommentsFile.c_str( ) );

		if( pComments && pComments->isDicti( ) )
			m_pComments = (CAtomDicti *)pComments;
		else
		{
			if( pComments )
				delete pComments;

			m_pComments = new CAtomDicti( );
		}
	}

	// decode tag file

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "CTracker: Decode tag file\n" );

	if( m_strTagFile.empty( ) )
		m_pTags = new CAtomDicti( );
	else
	{
		CAtom *pTags = DecodeFile( m_strTagFile.c_str( ) );

		if( pTags && pTags->isDicti( ) )
			m_pTags = (CAtomDicti *)pTags;
		else
		{
			if( pTags )
				delete pTags;

			m_pTags = new CAtomDicti( );
		}
	}
	
	// decode IPstate file

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "CTracker: Decode IPstate file\n" );

	if( m_strIPStateFile.empty( ) )
		m_pIPState = new CAtomDicti( );
	else
	{
		CAtom *pIPState = DecodeFile( m_strIPStateFile.c_str( ) );

		if( pIPState && pIPState->isDicti( ) )
			m_pIPState = (CAtomDicti *)pIPState;
		else
		{
			if( pIPState )
				delete pIPState;

			m_pIPState = new CAtomDicti( );
		}
	}

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

#if defined ( XBNBT_MYSQL )
	// XBNBT MySQL Users Integration
	if( gbMySQLUsersOverrideUsers )
	{
		if( gbDebug )
			if( gucDebugLevel & DEBUG_TRACKER )
				UTIL_LogPrint( "MySQL Users override is set: loading the database\n" );

		m_pUsers = new CAtomDicti( );
		m_pMySQLUsersUsers = new CAtomDicti( );
		m_bBoot = true;
		runLoadMySQLUsers( );
	}
	else
	{
#endif
		if( gbDebug )
			if( gucDebugLevel & DEBUG_TRACKER )
				UTIL_LogPrint( "CTracker: Decode users file\n" );

		// Original code
		// decode users file
		if( m_strUsersFile.empty( ) )
			m_pUsers = new CAtomDicti( );
		else
		{
			CAtom *pUsers = DecodeFile( m_strUsersFile.c_str( ) );

			if( pUsers && pUsers->isDicti( ) )
				m_pUsers = (CAtomDicti *)pUsers;
			else
			{
				if( pUsers )
					delete pUsers;

				m_pUsers = new CAtomDicti( );
			}
		}
#if defined ( XBNBT_MYSQL )
		// XBNBT MySQL Users Integration
	}
#endif

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

	// Count the unique peers
	if( m_bCountUniquePeers )
	{
		if( gbDebug )
			if( gucDebugLevel & DEBUG_TRACKER )
				UTIL_LogPrint( "CTracker: Counting the unique peers\n" );

		CountUniquePeers( );
	}

	// parse the allowed dir

	if( !m_strAllowedDir.empty( ) )
	{
		if( gbDebug )
			if( gucDebugLevel & DEBUG_TRACKER )
				UTIL_LogPrint( "CTracker: Parsing the allowed dir\n" );

		parseTorrents( m_strAllowedDir.c_str( ) );
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

	// CBTT parse the client ban list
	if( !m_strClientBanFile.empty( ) )
	{
		if( gbDebug )
			if( gucDebugLevel & DEBUG_TRACKER )
				UTIL_LogPrint( "CTracker: CBTT client ban parse list\n" );

		m_pClientBannedList = new CAtomList( );
		m_pClientBannedList->clear( ); 

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

		m_pIPBannedList = new CAtomList( );
		m_pIPBannedList->clear( ); 

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

				CAtom *piLogin = ( (CAtomDicti *)pXPage )->getItem( "iLogin" );
				if( piLogin )
					gtXStats.page.iLogin = dynamic_cast<CAtomLong *>( piLogin )->getValue( );
				else
					gtXStats.page.iLogin = 0;

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
	if( !xmldump.strName.empty( ) )
	{
		if( gbDebug )
			if( gucDebugLevel & DEBUG_TRACKER )
				UTIL_LogPrint( "Initial save of the XML file\n" );

		saveXML( );
	}

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

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "~CTracker: saving the RSS file\n" );

	saveRSS( );

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "~CTracker: saving the XML file\n" );

	saveXML( );

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "~CTracker: saving the XStats file\n" );

	saveXStats( );

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "~CTracker: saving the dfile file\n" );

	saveDFile( );

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

#if defined ( XBNBT_MYSQL )
	// XBNBT MySQL Users Integration
	if( gbMySQLUsersOverrideUsers )
	{
		if( m_pMySQLUsersUsers )
			delete m_pMySQLUsersUsers;

		m_pMySQLUsersUsers = 0;   
	}
#endif

	if( m_pAllowed )
		delete m_pAllowed;
	
	if( m_pState )
		delete m_pState;
	
	if( m_pTimeDicti )
		delete m_pTimeDicti;
	
	if( m_pCached )
		delete m_pCached;
	
	if( m_pComments )
		delete m_pComments;
	
	if( m_pTags )
		delete m_pTags;
	
	if( m_pIPState )
		delete m_pIPState;
	
	if( m_pUsers )
		delete m_pUsers;
	
	if( m_pIPs )
		delete m_pIPs;  
	
	if( m_pFastCache )
		delete m_pFastCache;

	m_pAllowed = 0;
	m_pState = 0;
	m_pDFile = 0;        /* don't delete */
	m_pCompleted = 0;    /* don't delete */
	m_pTimeDicti = 0;
	m_pCached = 0;
	m_pComments = 0;
	m_pTags = 0;
	m_pIPState =0;
	m_pUsers = 0;
	m_pIPs = 0;   
	m_pFastCache = 0;

	// Threads
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "~CTracker: destroying the MUTEX announce queue\n" );

	m_mtxQueued.Destroy( );

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "~CTracker: Destructor completed\n" );
}

// Save the dfile - Python standard
void CTracker :: saveDFile( )
{
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "saveDFile: started\n" );

	const string strData( Encode( m_pState ) );

	FILE *pFile = FILE_ERROR;

	pFile = fopen( m_strDFile.c_str( ), "wb" ) ;

	if( pFile == FILE_ERROR )
	{
		UTIL_LogPrint( ( gmapLANG_CFG["unable_to_write_file"] + "\n" ).c_str( ), m_strDFile.c_str( ) );

		if( gbDebug )
			if( gucDebugLevel & DEBUG_TRACKER )
				UTIL_LogPrint( "saveDFile: completed\n" );

		return;
	}

	fwrite( strData.c_str( ), sizeof( char ), strData.size( ), pFile );
	fclose( pFile );

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "saveDFile: completed\n" );
}

// Save the comments database
void CTracker :: saveComments( )
{
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "saveComments: started\n" );

	if( m_strCommentsFile.empty( ) )
	{
		if( gbDebug )
			if( gucDebugLevel & DEBUG_TRACKER )
				UTIL_LogPrint( "saveComments: file empty - completed\n" );

		return;
	}

	const string strData( Encode( m_pComments ) );

	FILE *pFile = FILE_ERROR;

	pFile = fopen( m_strCommentsFile.c_str( ), "wb" );

	if( pFile == FILE_ERROR )
	{
		UTIL_LogPrint( string( gmapLANG_CFG["unable_to_write_file"] + "\n" ).c_str( ), m_strCommentsFile.c_str( ) );

		if( gbDebug )
			if( gucDebugLevel & DEBUG_TRACKER )
				UTIL_LogPrint( "saveComments: completed\n" );

		return;
	}

	fwrite( strData.c_str( ), sizeof( char ), strData.size( ), pFile );
	fclose( pFile );

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "saveComments: completed\n" );
}

// Save other information related to the torrent
void CTracker :: saveTags( )
{
	if( gbDebug && ( gucDebugLevel & DEBUG_TRACKER ) )
		UTIL_LogPrint( "saveTags: started\n" );

	const string strData( Encode( m_pTags ) );

	FILE *pFile = FILE_ERROR;

	pFile = fopen( m_strTagFile.c_str( ), "wb" );

	if( pFile == FILE_ERROR )
	{
		UTIL_LogPrint( string( gmapLANG_CFG["unable_to_write_file"] + "\n" ).c_str( ), m_strTagFile.c_str( ) );

		if( gbDebug && ( gucDebugLevel & DEBUG_TRACKER ) )
			UTIL_LogPrint( "saveTags: completed\n" );

		return;
	}

	fwrite( strData.c_str( ), sizeof( char ), strData.size( ), pFile );
	fclose( pFile );

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "saveTags: completed\n" );
}

// Save ipstate
void CTracker :: saveIPState( )
{
	if( gbDebug && ( gucDebugLevel & DEBUG_TRACKER ) )
		UTIL_LogPrint( "saveIPState: started\n" );

	const string strData( Encode( m_pIPState ) );

	FILE *pFile = FILE_ERROR;

	pFile = fopen( m_strIPStateFile.c_str( ), "wb" );

	if( pFile == FILE_ERROR )
	{
		UTIL_LogPrint( string( gmapLANG_CFG["unable_to_write_file"] + "\n" ).c_str( ), m_strIPStateFile.c_str( ) );

		if( gbDebug && ( gucDebugLevel & DEBUG_TRACKER ) )
			UTIL_LogPrint( "saveIPState: completed\n" );

		return;
	}

	fwrite( strData.c_str( ), sizeof( char ), strData.size( ), pFile );
	fclose( pFile );

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "saveIPState: completed\n" );
}

// Save the users database
void CTracker :: saveUsers( )
{
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "saveUsers: started\n" );

	const string strData( Encode( m_pUsers ) );

	FILE *pFile = FILE_ERROR;

	pFile = fopen( m_strUsersFile.c_str( ), "wb" );

	if( pFile == FILE_ERROR )
	{
		UTIL_LogPrint( string( gmapLANG_CFG["unable_to_write_file"] + "\n" ).c_str( ), m_strUsersFile.c_str( ) );

		if( gbDebug )
			if( gucDebugLevel & DEBUG_TRACKER )
				UTIL_LogPrint( "saveUsers: completed\n" );

		return;
	}

	fwrite( strData.c_str( ), sizeof( char ), strData.size( ), pFile );
	fclose( pFile );

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "saveUsers: completed\n" );
}

// Save the RSS file
void CTracker :: saveRSS( const string &strChannelTag )
{
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "saveRSS: started\n" );

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
		{
			// channel
			strData += "<channel>\n";
			{
				// title
				strData += "<title>" + m_strTitle + ( !strChannelTag.empty( ) ? " - " + strChannelTag : string( ) ) + "</title>\n";

				// link
				if( !rssdump.strURL.empty( ) )
					strData += "<link>" + rssdump.strURL + "</link>\n";
				else if( m_bServeLocal )
					strData += "<link>/</link>\n";

				// description
				strData += "<description>" + m_strDescription + "</description>\n";

				// category
				if( !strChannelTag.empty( ) )
					strData += "<category domain=\"" + rssdump.strURL + INDEX_HTML + "?filter=" + strChannelTag + "\" >" + strChannelTag + "</category>\n";
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

#ifdef BNBT_MYSQL
				if( !m_bMySQLOverrideDState )
				{
#endif
					// retrieve torrent data
					if( m_pDFile )
					{
						map<string, CAtom *> *pmapDicti = m_pDFile->getValuePtr( );

						const unsigned long culKeySize( (unsigned long)pmapDicti->size( ) );

						// add the torrents into this structure one by one and sort it afterwards
						struct torrent_t *pTorrents = new struct torrent_t[culKeySize];

						unsigned long ulCount = 0;

						CAtom *pList = 0;
						CAtom *pFileName = 0;
						CAtom *pName = 0;
						CAtom *pAdded = 0;
						CAtom *pSize = 0;
						CAtom *pFiles = 0;
						CAtom *pTag = 0;
						CAtom *pUploader = 0;
						CAtom *pInfoLink = 0;
						CAtom *pFastCache = 0;
						CAtom *pMakePublic = 0;

						vector<CAtom *> vecTorrent;
						vecTorrent.reserve( 6 );

						for( map<string, CAtom *> :: iterator it = pmapDicti->begin( ); it != pmapDicti->end( ); it++ )
						{
							pTorrents[ulCount].strInfoHash = UTIL_HashToString( (*it).first );
							pTorrents[ulCount].strName = string( );
							pTorrents[ulCount].strLowerName = string( );
							pTorrents[ulCount].uiSeeders = 0;
							pTorrents[ulCount].uiLeechers = 0;
							pTorrents[ulCount].ulCompleted = 0;
							pTorrents[ulCount].iSize = 0;
							pTorrents[ulCount].uiFiles = 0;

							if( m_pAllowed )
							{
								pList = m_pAllowed->getItem( (*it).first );

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
											pTorrents[ulCount].strFileName = UTIL_RemoveHTML( pFileName->toString( ) );

										if( pName )
											pTorrents[ulCount].strName = UTIL_RemoveHTML( pName->toString( ) );

										if( pAdded )
											pTorrents[ulCount].strAdded = pAdded->toString( );

										if( pSize )
											pTorrents[ulCount].iSize = dynamic_cast<CAtomLong *>( pSize )->getValue( );

										if( pFiles )
											pTorrents[ulCount].uiFiles = (unsigned int)dynamic_cast<CAtomInt *>( pFiles )->getValue( );
									}
								}
							}

							if( m_pTags )
							{
								CAtom *pDicti = m_pTags->getItem( (*it).first );

								if( pDicti && pDicti->isDicti( ) )
								{
									pTag = ( (CAtomDicti *)pDicti )->getItem( "tag" );
									pName = ( (CAtomDicti *)pDicti )->getItem( "name" );
									pUploader = ( (CAtomDicti *)pDicti )->getItem( "uploader" );
									pInfoLink = ( (CAtomDicti *)pDicti )->getItem( "infolink" );
									pMakePublic = ( (CAtomDicti *)pDicti )->getItem( "makepublic" );

									if( pTag )
										pTorrents[ulCount].strTag = UTIL_RemoveHTML( pTag->toString( ) );

									if( pName )
										pTorrents[ulCount].strName = UTIL_RemoveHTML( pName->toString( ) );

									if( pUploader )
										pTorrents[ulCount].strUploader = UTIL_RemoveHTML( pUploader->toString( ) );

									if( pInfoLink )
										pTorrents[ulCount].strInfoLink = UTIL_RemoveHTML( pInfoLink->toString( ) );

									if( pMakePublic )
										pTorrents[ulCount].strMakePublic = UTIL_RemoveHTML( pMakePublic->toString( ) );
								}
							}

							// Seeders, Leechers and completed - from fastcache
							if( m_pFastCache )
							{
								pFastCache = m_pFastCache->getItem( (*it).first );

								if( pFastCache && dynamic_cast<CAtomList *>( pFastCache ) )
								{
									vecTorrent = dynamic_cast<CAtomList *>( pFastCache )->getValue( );

									pTorrents[ulCount].uiSeeders = dynamic_cast<CAtomInt *>( vecTorrent[0] )->getValue( );
									pTorrents[ulCount].uiLeechers = dynamic_cast<CAtomInt *>( vecTorrent[1] )->getValue( );
									pTorrents[ulCount].ulCompleted = dynamic_cast<CAtomInt *>( vecTorrent[2] )->getValue( );
								}
							}

							ulCount++;
						}

						qsort( pTorrents, culKeySize, sizeof( struct torrent_t ), dsortByAdded );

						unsigned long ulLimit = 0;

						if( m_uiDumpRSSLimit == 0 || (unsigned long)m_uiDumpRSSLimit > culKeySize )
							ulLimit = culKeySize;
						else
							ulLimit = m_uiDumpRSSLimit;

						string strTorrentLink = string( );
						unsigned long ulFileSize = 0;
						string strInfoLink = string( );

						for( unsigned long ulLoop = 0; ulLoop < ulLimit; ulLoop++ )
						{
							if( ( !strChannelTag.empty( ) && strChannelTag != pTorrents[ulLoop].strTag ) || ( m_ucMakePublic != 0 && pTorrents[ulLoop].strMakePublic != "on" ) )
								continue;

							// item
							strData += "<item>\n";
							{
								// title
								if( !pTorrents[ulLoop].strName.empty( ) )
									strData += "<title>" + pTorrents[ulLoop].strName + "</title>\n";
								else
									UTIL_LogPrint( "saveRSS: missing required elements for RSS 2.0 item title\n" );

								// link
								if( m_bShowStats )
								{
									if( ( !rssdump.strURL.empty( ) || m_bServeLocal ) && !pTorrents[ulLoop].strInfoHash.empty( ) )
										strData += "<link>" + rssdump.strURL + STATS_HTML + "?info_hash=" + pTorrents[ulLoop].strInfoHash + "</link>\n";
									else
										UTIL_LogPrint( "saveRSS: missing required elements for RSS 2.0 item link\n" );
								}

								// description
								strData += "<description><![CDATA[\n";
								{
									// tag
									if( !pTorrents[ulLoop].strTag.empty( ) )
										strData += gmapLANG_CFG["tag"] + ": " + pTorrents[ulLoop].strTag + "<br>\n";

									// info hash
									strData += gmapLANG_CFG["info_hash"] + ": " + pTorrents[ulLoop].strInfoHash + "<br>\n";

									// size
									strData += gmapLANG_CFG["size"] + ": " + UTIL_BytesToString( pTorrents[ulLoop].iSize ) + "<br>\n";

									// files
									strData += gmapLANG_CFG["files"] + ": " + CAtomInt( pTorrents[ulLoop].uiFiles ).toString( ) + "<br>\n";

									// info link
									if( !pTorrents[ulLoop].strInfoLink.empty( ) ) 
										strData += gmapLANG_CFG["info"] + ": <a href=\"" + pTorrents[ulLoop].strInfoLink + "\" title=\"" + pTorrents[ulLoop].strInfoLink + "\">" + gmapLANG_CFG["info"] + "</a>\n";

								}
								strData += "]]></description>\n";

								// author
								if( m_bShowUploader )
								{
									CAtom *pUser = 0;

									if( !pTorrents[ulLoop].strUploader.empty( ) )
										pUser = m_pUsers->getItem( pTorrents[ulLoop].strUploader );

									if( pUser && pUser->isDicti( ) && !pTorrents[ulLoop].strUploader.empty( ) )
										strData += "<author>" + ( (CAtomDicti *)pUser )->getItem( "email" )->toString( ) +  " (" + pTorrents[ulLoop].strUploader + ")</author>\n";
									else
										strData += "<author>(Unknown)</author>\n";
									//UTIL_LogPrint( "saveRSS: missing required elements for RSS 2.0 item author\n" );
								}
								else
									strData += "<author>(Unknown)</author>\n";

								// category
								if( ( !rssdump.strURL.empty( ) || m_bServeLocal ) && !pTorrents[ulLoop].strTag.empty( ) )
									strData += "<category domain=\"" + rssdump.strURL + INDEX_HTML + "?filter=" + UTIL_StringToEscaped( pTorrents[ulLoop].strTag ) + "\">" + pTorrents[ulLoop].strTag + "</category>\n";
								else if( m_bServeLocal )
									strData += "<category domain=\"/\">Unknown</category>\n";
								else
									UTIL_LogPrint( "saveRSS: missing required elements for RSS 2.0 item category\n" );

								// comments
								if( m_bAllowComments )
								{
									if( ( !rssdump.strURL.empty( ) || m_bServeLocal ) && !pTorrents[ulLoop].strInfoHash.empty( ) )
										strData += "<comments>" + rssdump.strURL + COMMENTS_HTML + "?info_hash=" + pTorrents[ulLoop].strInfoHash + "</comments>\n";
									else
										UTIL_LogPrint( "saveRSS: missing required elements for RSS 2.0 item comments\n" );
								}

								// enclosure
								if( m_bAllowTorrentDownloads )
								{
									strTorrentLink = string( );

									if( m_strExternalTorrentDir.empty( ) )
									{
										if( ( !rssdump.strURL.empty( ) || m_bServeLocal ) && !pTorrents[ulLoop].strInfoHash.empty( ) )
											strTorrentLink = rssdump.strURL + STR_TORRENTS + CHAR_FS + pTorrents[ulLoop].strInfoHash + ".torrent";
									}
									else
									{
										if( !pTorrents[ulLoop].strFileName.empty( ) )
											strTorrentLink = m_strExternalTorrentDir + UTIL_StringToEscapedStrict( pTorrents[ulLoop].strFileName );
									}

									ulFileSize = UTIL_SizeFile( string(m_strAllowedDir + pTorrents[ulLoop].strFileName ).c_str( ) );

									if( !strTorrentLink.empty( ) && !m_strAllowedDir.empty( ) && !pTorrents[ulLoop].strFileName.empty( ) && ulFileSize > 0 && !gmapMime[".torrent"].empty( ) )
										strData += "<enclosure url=\"" + strTorrentLink + "\" length=\"" + CAtomLong( ulFileSize ).toString( ) + "\" type=\"" + gmapMime[".torrent"] + "\" />\n";
									else
										UTIL_LogPrint( "saveRSS: missing required elements for RSS 2.0 item enclosure (%s)(%s)\n", pTorrents[ulLoop].strFileName.c_str( ), CAtomLong( ulFileSize ).toString( ).c_str( ) );
								}

								// guid
								if( !pTorrents[ulLoop].strInfoHash.empty( ) )
									strData += "<guid isPermaLink=\"false\">" + pTorrents[ulLoop].strInfoHash + "</guid>\n";
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
								{
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

									// infolink
									if( m_bAllowInfoLink )
									{
										if( !pTorrents[ulLoop].strInfoLink.empty( ) && !gmapLANG_CFG["info"].empty( ) ) 
											strData += "<torrentItem:infolink title=\"" + gmapLANG_CFG["info"] + "\">" + pTorrents[ulLoop].strInfoLink + "</torrentItem:infolink>\n";
									}
								}
							}
							strData += "</item>\n";
						}

						// free the memory
						delete [] pTorrents;
					}
					else
						UTIL_LogPrint( "saveRSS: no dfile information\n" );
#ifdef BNBT_MYSQL
				}
				else
					UTIL_LogPrint( "saveRSS: no dfile information\n" );
#endif

			}
			strData += "</channel>\n";

		}
		strData += "</rss>\n";

		// write the file
		if( !rssdump.strName.empty( ) )
		{
			string strRSSFile = string( );

			if( !rssdump.strDir.empty( ) )
				strRSSFile = rssdump.strDir + PATH_SEP + rssdump.strName;
			else
				strRSSFile = rssdump.strName;

			if( !strChannelTag.empty( ) && !rssdump.strExt.empty( ) )
				strRSSFile = strRSSFile.substr( 0, strRSSFile.length( ) - rssdump.strExt.length( ) ) + "-" + strChannelTag + rssdump.strExt;
			else
			{
				if( rssdump.strExt.empty( ) )
				{
					UTIL_LogPrint( "saveRSS: missing RSS file extension\n" );

					if( gbDebug )
						if( gucDebugLevel & DEBUG_TRACKER )
							UTIL_LogPrint( "saveRSS: completed\n" );

					return;
				}
			}

			FILE *pFile = FILE_ERROR;

			pFile = fopen( strRSSFile.c_str( ), "wb" );

			if( pFile == FILE_ERROR )
			{
				UTIL_LogPrint( string( gmapLANG_CFG["unable_to_write_file"] + "\n" ).c_str( ), strRSSFile.c_str( ) );

				if( gbDebug )
					if( gucDebugLevel & DEBUG_TRACKER )
						UTIL_LogPrint( "saveRSS: completed\n" );

				return;
			}

			fwrite( strData.c_str( ), sizeof( char ), strData.size( ), pFile );
			fclose( pFile );
		}
		else
			UTIL_LogPrint( "saveRSS: missing RSS file name\n" );
	}
	else
		UTIL_LogPrint( "saveRSS: missing required elements for RSS 2.0\n" );

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "saveRSS: completed\n" );
}
// end addition

// Expire downloaders 
void CTracker :: expireDownloaders( )
{
#ifdef BNBT_MYSQL
	if( m_bMySQLOverrideDState )
	{
		if( gbDebug )
			if( gucDebugLevel & DEBUG_TRACKER )
				UTIL_LogPrint( "expireDownloaders: MySQL overridden started\n" );

		const string strQuery( "DELETE FROM dstate WHERE btime<NOW()-INTERVAL " + CAtomInt( m_uiDownloaderTimeOutInterval ).toString( ) + " SECOND" );

		CMySQLQuery mq01( strQuery );

		if( gbDebug )
			if( gucDebugLevel & DEBUG_TRACKER )
				UTIL_LogPrint( "expireDownloaders: MySQL overridden completed\n" );

		return;
	}
#endif

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "expireDownloaders: started\n" );

	if( m_pTimeDicti )
	{
		map<string, CAtom *> *pmapTimeDicti = m_pTimeDicti->getValuePtr( );
		map<string, CAtom *> *pmapPeersDicti;
		CAtom *pPeers = 0;

		for( map<string, CAtom *> :: iterator it = pmapTimeDicti->begin( ); it != pmapTimeDicti->end( ); it++ )
		{
			if( (*it).second->isDicti( ) )
			{
				pmapPeersDicti = ( (CAtomDicti *)(*it).second )->getValuePtr( );

				for( map<string, CAtom *> :: iterator it2 = pmapPeersDicti->begin( ); it2 != pmapPeersDicti->end( ); )
				{
					if( dynamic_cast<CAtomLong *>( (*it2).second ) && dynamic_cast<CAtomLong *>( (*it2).second )->getValue( ) < m_ulPrevTime )
					{
						if( m_pDFile )
						{
							pPeers = m_pDFile->getItem( (*it).first );

							if( pPeers && pPeers->isDicti( ) )
								( (CAtomDicti *)pPeers )->delItem( (*it2).first );
						}

						delete (*it2).second;

						pmapPeersDicti->erase( it2++ );
					}
					else
						it2++;
				}
			}
		}

		CountUniquePeers( );

		m_ulPrevTime = GetTime( );

		if( m_bKeepDead )
		{
			if( gbDebug )
				if( gucDebugLevel & DEBUG_TRACKER )
					UTIL_LogPrint( "expireDownloaders: completed (keep dead)\n" );

			return;
		}

		// delete empty hashes

		if( m_pDFile )
		{
			map<string, CAtom *> *pmapDicti = m_pDFile->getValuePtr( );

			for( map<string, CAtom *> :: iterator it = pmapDicti->begin( ); it != pmapDicti->end( ); )
			{
				if( (*it).second->isDicti( ) && ( (CAtomDicti *)(*it).second )->isEmpty( ) )
				{
					m_pTimeDicti->delItem( (*it).first );

					delete (*it).second;

					pmapDicti->erase( it++ );
				}
				else
					it++;
			}
		}
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

	UTIL_LogPrint( "CTracker: Parsing torrents (%s)\n", m_strAllowedDir.c_str( ) );

#ifdef BNBT_MYSQL
	if( m_bMySQLOverrideDState )
	{
		const string cstrQuery3( "TRUNCATE TABLE allowed" );

		mysql_real_query( gpMySQL, cstrQuery3.c_str( ), ( unsigned long )cstrQuery3.size( ) );
		mysql_store_result( gpMySQL );

		if( mysql_errno( gpMySQL ) )
			UTIL_LogPrint( "mysql error - %s\n", mysql_error( gpMySQL ) );

		const string cstrQuery4( "TRUNCATE TABLE allowed_ex" );

		mysql_real_query( gpMySQL, cstrQuery4.c_str( ), ( unsigned long )cstrQuery4.size( ) );
		mysql_store_result( gpMySQL );

		if( mysql_errno( gpMySQL ) )
			UTIL_LogPrint( "mysql error - %s\n", mysql_error( gpMySQL ) );
	}
#endif

	CAtomDicti *pAllowed = new CAtomDicti( );

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
#ifdef BNBT_MYSQL
						if( m_bMySQLOverrideDState )
						{
							const string cstrQuery( "INSERT INTO allowed VALUES('" + strHash + "','" + pName->toString( ) + "')" );
							mysql_real_query( gpMySQL, cstrQuery.c_str( ), ( unsigned long )cstrQuery.size( ) );
							mysql_store_result( gpMySQL );

							if( mysql_errno( gpMySQL ) )
								UTIL_LogPrint( "mysql error - %s\n", mysql_error( gpMySQL ) );
						}
#endif
						pList = new CAtomList( );

						//
						// filename
						//

						pList->addItem( new CAtomString( strFileName ) );

						//
						// name
						//

						pList->addItem( new CAtomString( pName->toString( ) ) );

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

						pList->addItem( new CAtomString( pTime ) );

						//
						// file size
						//

						if( pLen )
							pList->addItem( new CAtomLong( *dynamic_cast<CAtomLong *>( pLen ) ) );
						else
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

							pList->addItem( new CAtomLong( iSize ) );
						}

						//
						// number of files
						//

						if( pLen )
						{
							pList->addItem( new CAtomInt( 1 ) );
#ifdef BNBT_MYSQL
							if( m_bMySQLOverrideDState )
							{
								const string cstrQuery2( UTIL_Xsprintf( "INSERT INTO allowed_ex VALUES('%s',NOW(),%lu,1)", strHash.c_str( ), CAtomLong( *dynamic_cast<CAtomLong *>( pLen ) ).getValue( ) ) );
								mysql_real_query( gpMySQL, cstrQuery2.c_str( ), ( unsigned long )cstrQuery2.size( ) );
								mysql_store_result( gpMySQL );

								if( mysql_errno( gpMySQL ) )
									UTIL_LogPrint( "mysql error - %s\n", mysql_error( gpMySQL ) );
							}
#endif
						}
						else
						{
							pList->addItem( new CAtomInt( (int)dynamic_cast<CAtomList *>( pFiles )->getValuePtr( )->size( ) ) );
#ifdef BNBT_MYSQL
							if( m_bMySQLOverrideDState )
							{
								const string cstrQuery2( UTIL_Xsprintf( "INSERT INTO allowed_ex VALUES('%s',NOW(),%lu,%lu)", strHash.c_str( ), iSize, (int)dynamic_cast<CAtomList *>( pFiles )->getValuePtr( )->size( ) ) );
								mysql_real_query( gpMySQL, cstrQuery2.c_str( ), ( unsigned long )cstrQuery2.size( ) );
								mysql_store_result( gpMySQL );

								if( mysql_errno( gpMySQL ) )
									UTIL_LogPrint( "mysql error - %s\n", mysql_error( gpMySQL ) );
							}
#endif
						}

						//
						// file comment
						//

						pComment = ( (CAtomDicti *)pFile )->getItem( "comment" );

						if( pComment )
							pList->addItem( new CAtomString( pComment->toString( ) ) );
						else
							pList->addItem( new CAtomString( ) );

						pAllowed->setItem( strHash, pList );

						if( m_pDFile && m_bKeepDead )
						{
							if( !m_pDFile->getItem( strHash ) )
								m_pDFile->setItem( strHash, new CAtomDicti( ) );
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

		closedir( pDir );
#endif
		UTIL_LogPrint( "Tracker Info - Parsed %s torrents in %s seconds\n", CAtomInt( uiParseCount ).toString( ).c_str( ), UTIL_ElapsedTimeStr( btv, UTIL_CurrentTime( ) ).c_str( ) );
	}
	else
		UTIL_LogPrint( string( gmapLANG_CFG["parsing_torrents_opening"] + "\n" ).c_str( ), szDir );

	if( m_pAllowed )
		delete m_pAllowed;

	m_pAllowed = pAllowed;

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "parseTorrents: completed\n" );
}

// Parse a single torrent
void CTracker :: parseTorrent( const char *szFile )
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
#ifdef BNBT_MYSQL
				if( m_bMySQLOverrideDState )
				{
					const string cstrQuery( "INSERT INTO allowed VALUES('" + cstrHash + "','" + pName->toString( ) + "')" );
					mysql_real_query( gpMySQL, cstrQuery.c_str( ), ( unsigned long )cstrQuery.size( ) );
					mysql_store_result( gpMySQL );

					if( mysql_errno( gpMySQL ) )
						UTIL_LogPrint( "mysql error - %s\n", mysql_error( gpMySQL ) );
				}
#endif
				CAtomList *pList = new CAtomList( );

				//
				// filename
				//

				pList->addItem( new CAtomString( UTIL_StripPath( szFile ).c_str( ) ) );

				//
				// name
				//

				pList->addItem( new CAtomString( pName->toString( ) ) );

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

				pList->addItem( new CAtomString( pTime ) );

				//
				// file size
				//

				if( pLen )
					pList->addItem( new CAtomLong( *dynamic_cast<CAtomLong *>( pLen ) ) );
				else
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

					pList->addItem( new CAtomLong( iSize ) );
				}

				//
				// number of files
				//

				if( pLen )
				{
					pList->addItem( new CAtomInt( 1 ) );
#ifdef BNBT_MYSQL
					if( m_bMySQLOverrideDState )
					{
						const string cstrQuery2( UTIL_Xsprintf( "INSERT INTO allowed_ex VALUES('%s',NOW(),%lu,1)", cstrHash.c_str( ), CAtomLong( *dynamic_cast<CAtomLong *>( pLen ) ).getValue( ) ) );
						mysql_real_query( gpMySQL, cstrQuery2.c_str( ), ( unsigned long )cstrQuery2.size( ) );
						mysql_store_result( gpMySQL );

						if( mysql_errno( gpMySQL ) )
							UTIL_LogPrint( "mysql error - %s\n", mysql_error( gpMySQL ) );
					}
#endif
				}
				else
				{
					pList->addItem( new CAtomInt( (int)dynamic_cast<CAtomList *>( pFiles )->getValuePtr( )->size( ) ) );
#ifdef BNBT_MYSQL
					if( m_bMySQLOverrideDState )
					{
						const string cstrQuery2( UTIL_Xsprintf( "INSERT INTO allowed_ex VALUES('%s',NOW(),%lu,%lu)", cstrHash.c_str( ), iSize, (int)dynamic_cast<CAtomList *>( pFiles )->getValuePtr( )->size( ) ) );
						mysql_real_query( gpMySQL, cstrQuery2.c_str( ), ( unsigned long )cstrQuery2.size( ) );
						mysql_store_result( gpMySQL );

						if( mysql_errno( gpMySQL ) )
							UTIL_LogPrint( "mysql error - %s\n", mysql_error( gpMySQL ) );
					}
#endif
				}

				//
				// file comment
				//

				CAtom *pComment = ( (CAtomDicti *)pTorrent )->getItem( "comment" );

				if( pComment )
					pList->addItem( new CAtomString( pComment->toString( ) ) );
				else
					pList->addItem( new CAtomString( ) );

				m_pAllowed->setItem( cstrHash, pList );

				if( m_pDFile && m_bKeepDead )
				{
					if( !m_pDFile->getItem( cstrHash ) )
						m_pDFile->setItem( cstrHash, new CAtomDicti( ) );
				}
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
}

// Check a torrent associated entry
const bool CTracker :: checkTag( const string &strTag )
{
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "checkTag: checking\n" );

	if( m_vecTags.empty( ) )
		return true;

	unsigned char ucTags = 1;
	for( vector< pair< string, string > > :: iterator it = m_vecTags.begin( ); it != m_vecTags.end( ); it++ )
	{
//		if( (*it).first == strTag )
		if( CAtomInt( ucTags ).toString( ) == strTag )
		{
			if( gbDebug )
				if( gucDebugLevel & DEBUG_TRACKER )
					UTIL_LogPrint( "checkTag: passed\n" );

			return true;
		}
		ucTags++;
	}

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "checkTag: failed\n" );

	return false;
}

// Add a torrent associated entry
void CTracker :: addTag( const string &strInfoHash, const string &strTag, const string &strName, const string &strUploader, const string &strInfoLink, const string &strIP, const string &strMakePublic, const string &strRatio, const string &strAllow )
{
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "addTag: started\n" );

	// Does the .torrent tag exist for the info hash? If not create one.
	if( !m_pTags->getItem( strInfoHash ) )
		m_pTags->setItem( strInfoHash, new CAtomDicti( ) );

	// Get the .torrent tag for the info hash.
	CAtom *pTag = m_pTags->getItem( strInfoHash );

	// Do we have a valid .torrent tag?
	if( pTag && pTag->isDicti( ) )
	{
		// set the .torrent tag category tag
		( (CAtomDicti *)pTag )->setItem( "tag", new CAtomString( strTag ) );

		// set the .torrent tag name
		if( !strName.empty( ) )
			( (CAtomDicti *)pTag )->setItem( "name", new CAtomString( strName ) );
		
		// set the .torrent tag uploader
		// The Trinity Edition 7.5r3 - Addition Begins
		// The following code allows the removal of the Uploader's name for any 
		// given torrent by setting this value to REMOVE and saving changes
		// This code is brought to you by courtesy of user ConfusedFish :)

		if( !strUploader.empty( ) )
			( (CAtomDicti *)pTag )->setItem( "uploader", new CAtomString( strUploader ) );
	
		if( ( !strUploader.empty( ) ) && ( strUploader!="REMOVE" ) )
			( (CAtomDicti *)pTag )->setItem( "uploader" , new CAtomString( strUploader ) );

		if( strUploader == "REMOVE" )
			( (CAtomDicti *)pTag )->delItem( "uploader" );

		// set the .torrent tag infolink
		// The Trinity Edition 7.5r3 - Addition Begins
		// The following code allows the removal of the InfoLink for any 
		// given torrent by setting this value to REMOVE and saving changes
		// This code is brought to you by courtesy of user ConfusedFish :)

		if( !strInfoLink.empty( ) )
			( (CAtomDicti *)pTag )->setItem( "infolink", new CAtomString( strInfoLink ) );

		if( ( !strInfoLink.empty( ) ) && ( strInfoLink != "REMOVE" ) )
			( (CAtomDicti *)pTag )->setItem( "infolink" , new CAtomString( strInfoLink ) );

		if( strInfoLink == "REMOVE" )
			( (CAtomDicti *)pTag )->delItem( "infolink" );

		// set the .torrent tag IP - Needs work
		if( !strIP.empty( ) )
			( (CAtomDicti *)pTag )->setItem( "ip", new CAtomString( strIP ) );

		// set the .torrent tag makepublic
		if( m_ucMakePublic != 0 )
		{
			if( strMakePublic == "on" )
				( (CAtomDicti *)pTag )->setItem( "makepublic", new CAtomString( strMakePublic ) );
			else
				( (CAtomDicti *)pTag )->delItem( "makepublic" );
		}
		
		if( !strRatio.empty( ) )
			( (CAtomDicti *)pTag )->setItem( "ratio", new CAtomString( strRatio ) );
		
		if( !strAllow.empty( ) )
			( (CAtomDicti *)pTag )->setItem( "allow", new CAtomString( strAllow ) );
	}

	// Save the .torrent tags
	saveTags( );

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "addTag: completed\n" );
}

// Delete other torrent associated entry
void CTracker :: deleteTag( const string &strInfoHash )
{
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "deleteTag: started\n" );

	m_pTags->delItem( strInfoHash );

	// Save the .torrent tags
	saveTags( );

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "deleteTag: completed\n" );
}

// Add a ipstate entry
void CTracker :: addIPState( const string &strInfoHash, const string &strIP, int64 Uploaded, int64 Downloaded )
{
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "addIPState: started\n" );

	// Does the .torrent tag exist for the info hash? If not create one.
	
	if( !m_pIPState->getItem( strInfoHash ) )
		m_pIPState->setItem( strInfoHash, new CAtomDicti( ) );
	
	CAtom *pInfoHashState = m_pIPState->getItem( strInfoHash );
	
// 	if( !m_pIPState->getItem( strIP ) )
// 		m_pIPState->setItem( strIP, new CAtomDicti( ) );

	// Get the .torrent tag for the info hash.
// 	CAtom *pIPState = m_pIPState->getItem( strIP );

	// Do we have a valid .torrent tag?
	if( pInfoHashState && pInfoHashState->isDicti( ) )
	{
		if( !( (CAtomDicti *)pInfoHashState )->getItem( strIP ) )
			( (CAtomDicti *)pInfoHashState )->setItem( strIP, new CAtomDicti( ) );
	
		CAtom *pIPState = ( (CAtomDicti *)pInfoHashState )->getItem( strIP );
		
// 		if( !strUploaded.empty( ) )
// 			( (CAtomDicti *)pIPState )->setItem( "uploaded", new CAtomLong( UTIL_StringTo64( strUploaded.c_str( ) ) ) );
			( (CAtomDicti *)pIPState )->setItem( "uploaded", new CAtomLong( Uploaded ) );
		
// 		if( !strDownloaded.empty( ) )
// 			( (CAtomDicti *)pIPState )->setItem( "downloaded", new CAtomLong( UTIL_StringTo64( strDownloaded.c_str( ) ) ) );
			( (CAtomDicti *)pIPState )->setItem( "downloaded", new CAtomLong( Downloaded ) );
	}

	// Save the .torrent tags
	saveIPState( );

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "addIPState: completed\n" );
}

// Check the users login and password
user_t CTracker :: checkUser( const string &strLogin, const string &cstrMD5 )
{
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "checkUser: started\n" );

	user_t user;

	user.ucAccess = m_ucGuestAccess;

	string strMD5 = cstrMD5;

	// if no users exist, grant full access
	if( m_pUsers->isEmpty( ) )
	{
		if( gbDebug )
			if( gucDebugLevel & DEBUG_TRACKER )
				UTIL_LogPrint( "checkUser: no users, full permission granted\n" );

		user.ucAccess = (unsigned char)~0;
	}

	CAtom *pUser = m_pUsers->getItem( strLogin );

	if( pUser && pUser->isDicti( ) )
	{
		CAtom *pMD5 = ( (CAtomDicti *)pUser )->getItem( "md5" );
		CAtom *pAccess = ( (CAtomDicti *)pUser )->getItem( "access" );
		CAtom *pMail = ( (CAtomDicti *)pUser )->getItem( "email" );
		CAtom *pCreated = ( (CAtomDicti *)pUser )->getItem( "created" );


		// XBNBT mySQL users integration
#if defined ( XBNBT_MYSQL )

		CMD5 md5salt;
		CMD5 md5plain;

		CAtom *pSalt = 0;

		switch( m_ucMySQLUsersMode )
		{
		case VBULLITEN3_MODE:
			// Vbulletin 3
			// strMD5 now contains md5(md5(password)+salt), which is how vb stores it,
			// which matches the format in the bdictionary 
			if( gbDebug )
				if( gucDebugLevel & DEBUG_TRACKER )
					UTIL_LogPrint( "checkUser: Vbulletin 3\n" );

			// get the salt
			pSalt = ( (CAtomDicti *)pUser )->getItem( "salt" );

			// prepare the salt hash against the users md5'd password as entered
			md5salt = ( const char* )( cstrMD5 + pSalt->toString( ) ).c_str( );
			strMD5 = md5salt.getMD5Digest( );

			break;
		case IPB2_MODE:
			// IPB2
			// converge_pass_hash = md5( md5( converge_pass_salt ) . md5( plain_text_password ) );
			// http://docs.invisionpower.com/kb/article.php?id=194
			if( gbDebug )
				if( gucDebugLevel & DEBUG_TRACKER )
					UTIL_LogPrint( "checkUser: IPB 2\n" );

			// get the salt
			pSalt = ( (CAtomDicti *)pUser )->getItem( "salt" );

			// prepare the salt hash
			md5salt = ( const char* )pSalt->toString( ).c_str( );

			// prepare plain_text_password
			md5plain = ( const char* )( string( md5salt.getMD5Digest( ) ) + cstrMD5 ).c_str( );

			strMD5 = md5plain.getMD5Digest( );
		default:
			// Use the original method ORIGINAL_MODE
			if( gbDebug )
				if( gucDebugLevel & DEBUG_TRACKER )
					UTIL_LogPrint( "checkUser: original\n" );
		}
#endif

		if( pMD5 && pAccess && dynamic_cast<CAtomLong *>( pAccess ) && pMail )
		{
			// check hash

			if( strMD5 == pMD5->toString( ) )
			{
				user.strLogin = strLogin;
				user.strLowerLogin = UTIL_ToLower( user.strLogin );
				user.strMD5 = strMD5;
				user.strMail = pMail->toString( );
				user.strLowerMail = UTIL_ToLower( user.strMail );
				user.ucAccess = (unsigned char)dynamic_cast<CAtomLong *>( pAccess )->getValue( );
				user.strCreated = pCreated->toString( );
			}
		}
	}

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "checkUser: completed\n" );

	return user;
}

// Add a user
void CTracker :: addUser( const string &strLogin, const string &strPass, const unsigned char &ucAccess, const string &strMail )
{
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "addUser: started\n" );

	CAtomDicti *pUser = new CAtomDicti( );

	// calculate md5 hash of A1
	const string cstrA1( strLogin + ":" + gstrRealm + ":" + strPass );

	unsigned char szMD5[16];
	memset( szMD5, 0, sizeof( szMD5 ) / sizeof( unsigned char ) );

	MD5_CTX md5;

	MD5Init( &md5 );
	MD5Update( &md5, (const unsigned char *)cstrA1.c_str( ), (unsigned int)cstrA1.size( ) );
	MD5Final( szMD5, &md5 );

	pUser->setItem( "md5", new CAtomString( string( (char *)szMD5, sizeof( szMD5 ) / sizeof( unsigned char ) ) ) );
	pUser->setItem( "access", new CAtomLong( ucAccess ) );
	pUser->setItem( "email", new CAtomString( strMail ) );

	time_t tNow = time( 0 );

	char pTime[256];
	memset( pTime, 0, sizeof( pTime ) / sizeof( char ) );
	strftime( pTime, sizeof( pTime ) / sizeof( char ), "%Y %m/%d %H:%M:%S", localtime( &tNow ) );

	pUser->setItem( "created", new CAtomString( pTime ) );

	m_pUsers->setItem( strLogin, pUser );

	saveUsers( );

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "addUser: completed\n" );
}

// Delete a user
void CTracker :: deleteUser( const string &strLogin )
{
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "deleteUser: started\n" );

	m_pUsers->delItem( strLogin );

	saveUsers( );

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "deleteUser: completed\n" );
}

// Count the unique peers
void CTracker :: CountUniquePeers( )
{
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "CountUniquePeers: started\n" );

	delete m_pIPs;

	m_pIPs = new CAtomDicti( );

	map<string, CAtom *> *pmapDicti = m_pDFile->getValuePtr( );
	map<string, CAtom *> *pmapPeersDicti;
	CAtom *pIP = 0;

	for( map<string, CAtom *> :: iterator it = pmapDicti->begin( ); it != pmapDicti->end( ); it++ )
	{
		if( (*it).second->isDicti( ) )
		{
			pmapPeersDicti = ( (CAtomDicti *)(*it).second )->getValuePtr( );

			for( map<string, CAtom *> :: iterator it2 = pmapPeersDicti->begin( ); it2 != pmapPeersDicti->end( ); it2++ )
			{
				if( (*it2).second->isDicti( ) )
				{
					pIP = ( (CAtomDicti *)(*it2).second )->getItem( "ip" );

					if( pIP )
						AddUniquePeer( pIP->toString( ) );
				}
			}
		}
	}

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "CountUniquePeers: completed (%i)\n", m_pIPs->getValuePtr( )->size( ) );
}

// Add a unique peer to the count
void CTracker :: AddUniquePeer( const string &strIP )
{
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "AddUniquePeer: started (%s)\n", strIP.c_str( ) );

	// increment unique count for this ip

	CAtom *pNum = m_pIPs->getItem( strIP );

	unsigned int uiNum = 1;

	if( pNum && dynamic_cast<CAtomInt *>( pNum ) )
		uiNum = dynamic_cast<CAtomInt *>( pNum )->getValue( ) + 1;

	m_pIPs->setItem( strIP, new CAtomInt( uiNum ) );

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

	CAtom *pNum = m_pIPs->getItem( strIP );

	unsigned int uiNum = 0;

	if( pNum && dynamic_cast<CAtomInt *>( pNum ) )
		uiNum = dynamic_cast<CAtomInt *>( pNum )->getValue( ) - 1;

	if( uiNum > 0 )
		m_pIPs->setItem( strIP, new CAtomInt( uiNum ) );
	else
		m_pIPs->delItem( strIP );

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "RemoveUniquePeer: completed\n" );
}

// Act upon the received announce
void CTracker :: Announce( const struct announce_t &ann, bool &bRespond )
{
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "Announce: event (%s)\n", ann.strEvent.c_str( ) );

	bool bPeerFound = false;

#ifdef BNBT_MYSQL
	if( m_bMySQLOverrideDState )
	{
		if( gbDebug )
			if( gucDebugLevel & DEBUG_TRACKER )
				UTIL_LogPrint( "Announce: MySQL override started\n" );

		string strQuery = string( );

		if( ann.strEvent != EVENT_STR_STOPPED )
		{
			strQuery = "REPLACE INTO dstate (bhash,bid,bkey,bip,bport,buploaded,bdownloaded,bleft,btime) VALUES(\'";
			strQuery += UTIL_StringToMySQL( ann.strInfoHash );
			strQuery += "\',\'";
			strQuery += UTIL_StringToMySQL( ann.strPeerID );
			strQuery += "\',\'";
			strQuery += UTIL_StringToMySQL( ann.strKey );
			strQuery += "\',\'";
			strQuery += UTIL_StringToMySQL( ann.strIP.substr( 0, 15 ) );
			strQuery += "\',";
			strQuery += CAtomInt( ann.uiPort ).toString( );
			strQuery += ",";
			strQuery += CAtomLong( ann.iUploaded ).toString( );
			strQuery += ",";
			strQuery += CAtomLong( ann.iDownloaded ).toString( );
			strQuery += ",";
			strQuery += CAtomLong( ann.iLeft ).toString( );
			strQuery += ",NOW())";

			CMySQLQuery mq01( strQuery );

			if( ann.strEvent == EVENT_STR_COMPLETED )
			{
				CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bcompleted FROM completed WHERE bhash=\'" + UTIL_StringToMySQL( ann.strInfoHash ) + "\'" );

				strQuery.erase( );

				if( pQuery->nextRow( ).size( ) == 0 )
				{
					strQuery = "INSERT INTO completed (bhash,bcompleted) VALUES(\'";
					strQuery += UTIL_StringToMySQL( ann.strInfoHash );
					strQuery += "\',1)";
				}
				else
				{
					strQuery = "UPDATE completed SET bcompleted=bcompleted+1 WHERE bhash=\'";
					strQuery += UTIL_StringToMySQL( ann.strInfoHash );
					strQuery += "\'";
				}

				CMySQLQuery mq02( strQuery );

				delete pQuery;
			}
		}
		else if( ann.strEvent == EVENT_STR_STOPPED )
		{
			strQuery = "DELETE FROM dstate WHERE bhash=\'";
			strQuery += UTIL_StringToMySQL( ann.strInfoHash );
			strQuery += "\' AND bid=\'";
			strQuery += UTIL_StringToMySQL( ann.strPeerID );

			if( m_bAnnounceKeySupport && !ann.strKey.empty( ) )
			{
				if( gbDebug && ( gucDebugLevel & DEBUG_TRACKER ) )
					UTIL_LogPrint( "Announce: key support\n" );

				strQuery += "\' AND bkey=\'";
				strQuery += UTIL_StringToMySQL( ann.strKey );
			}
			else
			{
				if( gbDebug && ( gucDebugLevel & DEBUG_TRACKER ) )
					UTIL_LogPrint( "Announce: no key support\n" );

				strQuery += "\' AND bip=\'";
				strQuery += UTIL_StringToMySQL( ann.strIP );
			}

			strQuery += "\'";

			CMySQLQuery mq01( strQuery );
		}
		else
			UTIL_LogPrint( "Tracker Warning - Announce event unknown (%s)/n", ann.strEvent.c_str( ) );

		if( gbDebug )
			if( gucDebugLevel & DEBUG_TRACKER )
				UTIL_LogPrint( "Announce: MySQL override completed\n" );

		return;
	}
#endif

	// Do we have a golbal dfile?
	if( m_pDFile && m_pDFile->isDicti( ) )
	{
		if( gbDebug )
			if( gucDebugLevel & DEBUG_TRACKER )
				UTIL_LogPrint( "Announce: global dfile is good\n" );

		// Get the list of peers for this info hash
		CAtom *pPeers = m_pDFile->getItem( ann.strInfoHash );

		// If there is no info hash entry, create one
		if( !pPeers )
		{
			if( gbDebug )
				if( gucDebugLevel & DEBUG_TRACKER )
					UTIL_LogPrint( "Announce: info hash (%s) not found, creating\n", UTIL_HashToString( ann.strInfoHash ).c_str( ) );

			m_pDFile->setItem( ann.strInfoHash, new CAtomDicti( ) );

			// Get the list of peers for this info hash
			pPeers = m_pDFile->getItem( ann.strInfoHash );
		}

		// Do we have a list of peers?
		if( pPeers && pPeers->isDicti( ) )
		{
			if( gbDebug )
				if( gucDebugLevel & DEBUG_TRACKER )
					UTIL_LogPrint( "Announce: got list of peers\n" );

			// Get the peer from the list
			CAtom *pPeer = ( (CAtomDicti *)pPeers )->getItem( ann.strPeerID );
		
			if( pPeer && pPeer->isDicti( ) )
			{
				if( gbDebug )
					if( gucDebugLevel & DEBUG_TRACKER )
						UTIL_LogPrint( "Announce: got peer (%s)\n", ann.strPeerID.c_str( ) );

				// Signal we found the peer
				bPeerFound = true;

				// Announce key support
				if( m_bAnnounceKeySupport )
				{
					if( gbDebug )
						if( gucDebugLevel & DEBUG_TRACKER )
							UTIL_LogPrint( "Announce: key support (enabled)\n" );

					// Get the last key supplied by the peer
					CAtom *pKey = ( (CAtomDicti *)pPeer )->getItem( "key" );

					if( pKey )
					{
						if( gbDebug )
							if( gucDebugLevel & DEBUG_TRACKER )
								UTIL_LogPrint( "Announce: last key (%s)\n", pKey->toString( ).c_str( ) );

						// Get the last IP supplied by the peer
						CAtom *pPeerIP = ( (CAtomDicti *)pPeer )->getItem( "ip" );

						if( pPeerIP )
						{
							if( gbDebug )
								if( gucDebugLevel & DEBUG_TRACKER )
									UTIL_LogPrint( "Announce: last IP (%s)\n", ann.strIP.c_str( ) );

							// Does the last key match the current key?
							if( pKey->toString( ) == ann.strKey )
							{
								if( gbDebug )
									if( gucDebugLevel & DEBUG_TRACKER )
										UTIL_LogPrint( "Announce: key matched (%s)\n", ann.strKey.c_str( ) );

								// Are the last IP and the current IP the same?
								if( pPeerIP->toString( ) == ann.strIP )
								{
									if( gbDebug )
										if( gucDebugLevel & DEBUG_TRACKER )
											UTIL_LogPrint( "Announce: IP (%s) has not changed\n", ann.strIP.c_str( ) );
								}
								else
								{
									if( gbDebug )
										if( gucDebugLevel & DEBUG_TRACKER )
											UTIL_LogPrint( "Announce: IP (%s) change to (%s)\n", pPeerIP->toString( ).c_str( ), ann.strIP.c_str( ) );

									// Remove the old IP from the unique peers list
									if( m_bCountUniquePeers )
									{
										if( gbDebug )
											if( gucDebugLevel & DEBUG_TRACKER )
												UTIL_LogPrint( "Announce: remove unique peer (enabled)\n" );

										RemoveUniquePeer( pPeerIP->toString( ) );
									}

									// Updating peer IP from the announce
									( (CAtomDicti *)pPeer )->setItem( "ip", new CAtomString( ann.strIP ) );

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

								( (CAtomDicti *)pPeer )->setItem( "uploaded", new CAtomLong( ann.iUploaded ) );
								( (CAtomDicti *)pPeer )->setItem( "downloaded", new CAtomLong( ann.iDownloaded ) );
								( (CAtomDicti *)pPeer )->setItem( "left", new CAtomLong( ann.iLeft ) );
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

							( (CAtomDicti *)pPeer )->setItem( "ip", new CAtomString( ann.strIP ) );
						}

					}
					else
					{
						if( gbDebug )
							if( gucDebugLevel & DEBUG_TRACKER )
								UTIL_LogPrint( "Announce: last key not found\n" );

						( (CAtomDicti *)pPeer )->setItem( "key", new CAtomString( ann.strKey ) );
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
						
	/*				if( m_pIPState )
					{
						int64 Uploaded = 0;
						int64 Downloaded = 0;
					
						if( !m_pIPState->getItem( ann.strIP ) )
							m_pIPState->setItem( ann.strIP, new CAtomDicti( ) );
						CAtom *pIPState = m_pIPState->getItem( ann.strIP );
						
						if( pIPState && pIPState->isDicti( ) )
						{
							CAtomDicti *pIPStateDicti = dynamic_cast<CAtomDicti *>( pIPState );
							CAtom *pUploaded = 0;
							CAtom *pDownloaded = 0;
							
							pUploaded = ( (CAtomDicti *)( pIPStateDicti ) )->getItem( "uploaded" );
							pDownloaded = ( (CAtomDicti *)( pIPStateDicti ) )->getItem( "downloaded" );
							
							if( pUploaded )
								Uploaded = UTIL_StringTo64( pUploaded->toString( ).c_str( ) );
							if( pDownloaded )
								Downloaded = UTIL_StringTo64( pDownloaded->toString( ).c_str( ) );
							FILE *pFile;
							string cstrContents;

							pFile = fopen( "download.txt", "a+" );
							cstrContents=pDownloaded->toString( );

							fwrite( cstrContents.c_str( ), sizeof( char ), cstrContents.size( ), pFile );
							fclose( pFile );
	
							Uploaded += ann.iUploaded - UTIL_StringTo64( ( (CAtomDicti *)pPeer )->getItem( "uploaded" )->toString( ).c_str( ) );
							Downloaded += ann.iDownloaded - UTIL_StringTo64( ( (CAtomDicti *)pPeer )->getItem( "downloaded" )->toString( ).c_str( ) );
								printf( "%ld\n", Uploaded );
								printf( "%ld\n", Downloaded );
						}
						addIPState( ann.strIP, CAtomLong( Uploaded ).toString( ), CAtomLong( Downloaded ).toString( ) );
					}*/

					( (CAtomDicti *)pPeer )->setItem( "uploaded", new CAtomLong( ann.iUploaded ) );
					( (CAtomDicti *)pPeer )->setItem( "downloaded", new CAtomLong( ann.iDownloaded ) );
					( (CAtomDicti *)pPeer )->setItem( "left", new CAtomLong( ann.iLeft ) );
					
				}
				
				// Peer information support
				if( m_ucShowPeerInfo != 0 )
				{
					if( gbDebug )
						if( gucDebugLevel & DEBUG_TRACKER )
							UTIL_LogPrint( "Announce: updating useragent\n" );

					( (CAtomDicti *)pPeer )->setItem( "useragent", new CAtomString( ann.strUserAgent ) );
				}

				//
				// Set the peer in the global expiry time list
				//

				if( m_pTimeDicti && m_pTimeDicti->isDicti( ) )
				{
					if( gbDebug )
						if( gucDebugLevel & DEBUG_TRACKER )
							UTIL_LogPrint( "Announce: updating the global expiry list\n" );

					// Create an entry for this info hash if it does not exist
					if( !m_pTimeDicti->getItem( ann.strInfoHash ) )
						m_pTimeDicti->setItem( ann.strInfoHash, new CAtomDicti( ) );

					// Get the list of peers for this info hash
					CAtom *pTS = m_pTimeDicti->getItem( ann.strInfoHash );

					// If we got a list then update the peer entry
					if( pTS && pTS->isDicti( ) )
						( (CAtomDicti *)pTS )->setItem( ann.strPeerID, new CAtomLong( GetTime( ) ) );
					else
						UTIL_LogPrint( "Announce: peer time dictionary broken\n" );
				}
				else
					UTIL_LogPrint( "Announce: global time dictionary is broken\n" );

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
					CAtomDicti *pPeerDicti = new CAtomDicti( );

					// Set the new peer details
					pPeerDicti->setItem( "ip", new CAtomString( ann.strIP ) );
					pPeerDicti->setItem( "port", new CAtomLong( ann.uiPort ) );
					pPeerDicti->setItem( "uploaded", new CAtomLong( ann.iUploaded ) );
					pPeerDicti->setItem( "downloaded", new CAtomLong( ann.iDownloaded ) );
					pPeerDicti->setItem( "left", new CAtomLong( ann.iLeft ) );
					pPeerDicti->setItem( "connected", new CAtomLong( GetTime( ) ) );

					// Announce key support
					if( m_bAnnounceKeySupport )
					{
						if( gbDebug )
							if( gucDebugLevel & DEBUG_TRACKER )
								UTIL_LogPrint( "Announce: key support (enabled)\n" );

						pPeerDicti->setItem( "key", new CAtomString( ann.strKey ) );
					}

					// Peer information support
					if( m_ucShowPeerInfo != 0 )
					{
						if( gbDebug )
							if( gucDebugLevel & DEBUG_TRACKER )
								UTIL_LogPrint( "Announce: peer info (enabled)\n" );

						pPeerDicti->setItem( "useragent", new CAtomString( ann.strUserAgent ) );
					}

					// Add the new peer entry
					( (CAtomDicti *)pPeers )->setItem( ann.strPeerID, pPeerDicti );
					
/*					if( m_pIPState )
					{
						int64 Uploaded = 0;
						int64 Downloaded = 0;
						
						if( !m_pIPState->getItem( ann.strIP ) )
							m_pIPState->setItem( ann.strIP, new CAtomDicti( ) );
						CAtom *pIPState = m_pIPState->getItem( ann.strIP );
						
						if( pIPState && pIPState->isDicti( ) )
						{
							CAtomDicti *pIPStateDicti = dynamic_cast<CAtomDicti *>( pIPState );
							CAtom *pUploaded = 0;
							CAtom *pDownloaded = 0;
							
							pUploaded = ( (CAtomDicti *)( pIPStateDicti ) )->getItem( "uploaded" );
							pDownloaded = ( (CAtomDicti *)( pIPStateDicti ) )->getItem( "downloaded" );
							
							if( pUploaded )
								Uploaded = UTIL_StringTo64( pUploaded->toString( ).c_str( ) );
							if( pDownloaded )
								Downloaded = UTIL_StringTo64( pDownloaded->toString( ).c_str( ) );
							
							Uploaded += ann.iUploaded;
							Downloaded += ann.iDownloaded;
						}
						addIPState( ann.strIP, CAtomLong( Uploaded ).toString( ), CAtomLong( Downloaded ).toString( ) );

					}*/

					// Add to the unique peer list
					if( m_bCountUniquePeers )
					{
						if( gbDebug )
							if( gucDebugLevel & DEBUG_TRACKER )
								UTIL_LogPrint( "Announce: count unique peer (enabled)\n" );

						AddUniquePeer( ann.strIP );
					}

					//
					// Set the peer in the global expiry time list
					//

					if( m_pTimeDicti && m_pTimeDicti->isDicti( ) )
					{
						if( gbDebug )
							if( gucDebugLevel & DEBUG_TRACKER )
								UTIL_LogPrint( "Announce: updating the global expiry list\n" );

						// Create an entry for this info hash if it does not exist
						if( !m_pTimeDicti->getItem( ann.strInfoHash ) )
							m_pTimeDicti->setItem( ann.strInfoHash, new CAtomDicti( ) );

						// Get the list of peers for this info hash
						CAtom *pTS = m_pTimeDicti->getItem( ann.strInfoHash );

						// If we got a list then update the peer entry
						if( pTS && pTS->isDicti( ) )
							( (CAtomDicti *)pTS )->setItem( ann.strPeerID, new CAtomLong( GetTime( ) ) );
						else
							UTIL_LogPrint( "Announce: peer time dictionary broken\n" );
					}
					else
						UTIL_LogPrint( "Announce: global time dictionary is broken\n" );

				}

				break;
			case EVENT_COMPLETED:
				if( bPeerFound )
				{
					if( m_pCompleted && m_pCompleted->isDicti( ) )
					{
						if( gbDebug )
							if( gucDebugLevel & DEBUG_TRACKER )
								UTIL_LogPrint( "Announce: global completed list is good\n" );

						CAtom *pCompleted = m_pCompleted->getItem( ann.strInfoHash );

						int64 iCompleted = 0;

						if( pCompleted && dynamic_cast<CAtomLong *>( pCompleted ) )
							iCompleted = dynamic_cast<CAtomLong *>( pCompleted )->getValue( );
						else
						{
							if( gbDebug )
								if( gucDebugLevel & DEBUG_TRACKER )
									UTIL_LogPrint( "Announce: no prevoius count (%s)\n", UTIL_HashToString( ann.strInfoHash ).c_str( ) );
						}

						if( gbDebug )
						{
							if( gucDebugLevel & DEBUG_TRACKER )
							{
#if defined( WIN32 )
								UTIL_LogPrint( "Announce: updating completed (%I64d) count for (%s)\n", iCompleted + 1,  ann.strInfoHash.c_str( ) );
#elif defined( __FREEBSD__ ) || defined( __OPENBSD__ ) || defined( __NETBSD__ )
								UTIL_LogPrint( "Announce: updating completed (%qd) count for (%s)\n", iCompleted + 1,  ann.strInfoHash.c_str( ) );
#else
								UTIL_LogPrint( "Announce: updating completed (%lld) count for (%s)\n", iCompleted + 1,  ann.strInfoHash.c_str( ) );
#endif
							}
						}

						m_pCompleted->setItem( ann.strInfoHash, new CAtomLong( iCompleted + 1 ) );
						
						if( m_pIPState )
						{
// 							int64 Uploaded = 0;
// 							int64 Downloaded = 0;
							
							if( !m_pIPState->getItem( ann.strInfoHash ) )
								m_pIPState->setItem( ann.strInfoHash, new CAtomDicti( ) );
							CAtom *pIPState = m_pIPState->getItem( ann.strInfoHash );
							
							if( pIPState && pIPState->isDicti( ) )
							{
// 								CAtomDicti *pIPStateDicti = dynamic_cast<CAtomDicti *>( pIPState );
// 								CAtom *pUploaded = 0;
// 								CAtom *pDownloaded = 0;
								
// 								pIPStateDicti->setItem( "uploaded", new CAtomLong( ann.iUploaded ) );
// 								pIPStateDicti->setItem( "downloaded", new CAtomLong( ann.iDownloaded ) );
								
// 								if( pUploaded )
// 									Uploaded = UTIL_StringTo64( pUploaded->toString( ).c_str( ) );
// 								if( pDownloaded )
// 									Downloaded = UTIL_StringTo64( pDownloaded->toString( ).c_str( ) );
								
// 								Uploaded += ann.iUploaded;
// 								Downloaded += ann.iDownloaded;
								addIPState( ann.strInfoHash, ann.strIP, ann.iUploaded, ann.iDownloaded );
							}
// 							saveIPState( );
// 							addIPState( ann.strInfoHash, ann.strIP, ann.iUploaded, ann.iDownloaded );

						}
					}
					else
						UTIL_LogPrint( "Announce: global completed list is broken\n" );

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
					( (CAtomDicti *)pPeers )->delItem( ann.strPeerID );

					// Remove the IP from the unique peers list
					if( m_bCountUniquePeers )
					{
						if( gbDebug )
							if( gucDebugLevel & DEBUG_TRACKER )
								UTIL_LogPrint( "Announce: remove unique peer (enabled)\n" );

						RemoveUniquePeer( ann.strIP );
					}

					//
					// Set the peer in the global expiry time list
					//

					if( m_pTimeDicti && m_pTimeDicti->isDicti( ) )
					{
						if( gbDebug )
							if( gucDebugLevel & DEBUG_TRACKER )
								UTIL_LogPrint( "Announce: updating the global expiry list\n" );

						// Create an entry for this info hash if it does not exist
						if( !m_pTimeDicti->getItem( ann.strInfoHash ) )
							m_pTimeDicti->setItem( ann.strInfoHash, new CAtomDicti( ) );

						// Get the list of peers for this info hash
						CAtom *pTS = m_pTimeDicti->getItem( ann.strInfoHash );

						// If we got a list then update the peer entry
						if( pTS && pTS->isDicti( ) )
							( (CAtomDicti *)pTS )->delItem( ann.strPeerID );
						else
							UTIL_LogPrint( "Announce: peer time dictionary broken\n" );
					}
					else
						UTIL_LogPrint( "Announce: global time dictionary is broken\n" );

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

		}
		else
		{
			UTIL_LogPrint( "Announce: peer list is broken\n" );
		
			bRespond = false;

			return;
		}

	}
	else
	{
		UTIL_LogPrint( "Announce: global dfile is broken\n" );
		
		bRespond = false;

		return;
	}

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "Announce: event (%s) completed\n", ann.strEvent.c_str( ) );
}

// Refresh the fast cache
void CTracker :: RefreshFastCache( )
{
	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "RefreshFastCache: started\n" );

	delete m_pFastCache;

	m_pFastCache = new CAtomDicti( );

	m_ulSeedersTotal = 0;
	m_ulLeechersTotal = 0;
	m_ulPeers = 0;

#ifdef BNBT_MYSQL
	if( !m_bMySQLOverrideDState )
	{
#endif
		if( m_pDFile )
		{
			map<string, CAtom *> *pmapDicti = m_pDFile->getValuePtr( );

			unsigned long ulSeeders = 0;
			unsigned long ulLeechers = 0;
			unsigned long ulCompleted = 0;
			int64 iTotalLeft = 0;
			int64 iMinLeft = 0;
			int64 iMaxiLeft = 0;

			bool bFirst = true;

			map<string, CAtom *> *pmapPeersDicti;

			CAtom *pLeft = 0;
			CAtom *pCompleted = 0;
			CAtomList *pList = 0;

			for( map<string, CAtom *> :: iterator it = pmapDicti->begin( ); it != pmapDicti->end( ); it++ )
			{
				if( (*it).second->isDicti( ) )
				{
					pmapPeersDicti = ( (CAtomDicti *)(*it).second )->getValuePtr( );

					ulSeeders = 0;
					ulLeechers = 0;
					ulCompleted = 0;
					iTotalLeft = 0;
					iMinLeft = 0;
					iMaxiLeft = 0;

					for( map<string, CAtom *> :: iterator it2 = pmapPeersDicti->begin( ); it2 != pmapPeersDicti->end( ); it2++ )
					{
						if( (*it2).second->isDicti( ) )
						{
							pLeft = ( (CAtomDicti *)(*it2).second )->getItem( "left" );

							if( pLeft && dynamic_cast<CAtomLong *>( pLeft ) )
							{
								int64 iLeft = dynamic_cast<CAtomLong *>( pLeft )->getValue( );

								if( iLeft == 0 )
								{
									ulSeeders++;
									m_ulSeedersTotal++;
								}
								else
								{
									ulLeechers++;
									m_ulLeechersTotal++;

									// only calculate total / min / max on leechers

									if( m_bShowAverageLeft )
										iTotalLeft += iLeft;

									if( bFirst || iLeft < iMinLeft )
										iMinLeft = iLeft;

									if( bFirst || iLeft > iMaxiLeft )
										iMaxiLeft = iLeft;

									bFirst = false;
								}
							}
						}
					}

					if( m_pCompleted )
					{
						pCompleted = m_pCompleted->getItem( (*it).first );

						if( pCompleted && dynamic_cast<CAtomLong *>( pCompleted ) )
							ulCompleted = (unsigned long)dynamic_cast<CAtomLong *>( pCompleted )->getValue( );
					}

					pList = new CAtomList( );

					pList->addItem( new CAtomInt( ulSeeders ) );
					pList->addItem( new CAtomInt( ulLeechers ) );
					pList->addItem( new CAtomInt( ulCompleted ) );
					pList->addItem( new CAtomLong( iTotalLeft ) );
					pList->addItem( new CAtomLong( iMinLeft ) );
					pList->addItem( new CAtomLong( iMaxiLeft ) );

					m_pFastCache->setItem( (*it).first, pList );
				}
			}

			m_ulPeers = m_ulSeedersTotal + m_ulLeechersTotal;

			if( m_ulPeers > gtXStats.peer.iGreatest )
				gtXStats.peer.iGreatest = m_ulPeers;

			if( m_ulSeedersTotal > gtXStats.peer.iGreatestSeeds )
				gtXStats.peer.iGreatestSeeds = m_ulSeedersTotal;

			if( m_ulLeechersTotal > gtXStats.peer.iGreatestLeechers )
				gtXStats.peer.iGreatestLeechers = m_ulLeechersTotal;
		}

		if( gbDebug )
			if( gucDebugLevel & DEBUG_TRACKER )
				UTIL_LogPrint( "RefreshFastCache: completed\n" );
#ifdef BNBT_MYSQL
	}
	else
		UTIL_LogPrint( "RefreshFastCache: no dfile information\n" );
#endif
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
	else if( pRequest->strURL == RESPONSE_STR_ADMIN_HTML ) ucResponse = RESPONSE_ADMIN;
	else if( pRequest->strURL == RESPONSE_STR_USERS_HTML ) ucResponse = RESPONSE_USERS;
	else if( pRequest->strURL == RESPONSE_STR_TAGS_HTML ) ucResponse = RESPONSE_TAGS;
	else if( pRequest->strURL == RESPONSE_STR_LANGUAGE_HTML ) ucResponse = RESPONSE_LANGUAGE;
	else if( pRequest->strURL == RESPONSE_STR_XSTATS_HTML ) ucResponse = RESPONSE_XSTATS;
	else if( pRequest->strURL == RESPONSE_STR_XTORRENT_HTML ) ucResponse = RESPONSE_XTORRENT;
	else if( !style.strName.empty( ) && pRequest->strURL == RESPONSE_STR_SEPERATOR + style.strName ) ucResponse = RESPONSE_CSS;
	else if( pRequest->strURL == RESPONSE_STR_SEPERATOR || pRequest->strURL == RESPONSE_STR_INDEX_HTML || pRequest->strURL == "/index.htm") ucResponse = RESPONSE_INDEX;
	else if( pRequest->strURL.substr( 0, 10 ) == RESPONSE_STR_TORRENTS ) ucResponse = RESPONSE_TORRENTS;
	else if( pRequest->strURL.substr( 0, 7 ) == RESPONSE_STR_FILES ) ucResponse = RESPONSE_FILES;
	else if( !imagefill.strName.empty( ) && pRequest->strURL == RESPONSE_STR_SEPERATOR + imagefill.strName ) ucResponse = RESPONSE_IMAGEFILL;
	else if( imagefill.strName.empty( ) && pRequest->strURL == "/imagebarfill.png" ) ucResponse = RESPONSE_IMAGEFILL;
	else if( !imagetrans.strName.empty( ) && pRequest->strURL == RESPONSE_STR_SEPERATOR + imagetrans.strName ) ucResponse = RESPONSE_IMAGETRANS;
	else if( imagetrans.strName.empty( ) && pRequest->strURL == "/imagebartrans.png" ) ucResponse = RESPONSE_IMAGETRANS;
	else if( pRequest->strURL == RESPONSE_STR_UPLOAD_HTML ) ucResponse = RESPONSE_UPLOAD;
	else if( pRequest->strURL == RESPONSE_STR_STATS_HTML ) ucResponse = RESPONSE_STATS;
	else if( pRequest->strURL == RESPONSE_STR_INFO_HTML ) ucResponse = RESPONSE_INFO;
	else if( pRequest->strURL == RESPONSE_STR_COMMENTS_HTML ) ucResponse = RESPONSE_COMMENTS;
	else if( pRequest->strURL == RESPONSE_STR_SIGNUP_HTML ) ucResponse = RESPONSE_SIGNUP;
	else if( pRequest->strURL == RESPONSE_STR_ROBOTS_TXT ) ucResponse = RESPONSE_ROBOTS;
	else if( pRequest->strURL == RESPONSE_STR_FAVICON_ICO ) ucResponse = RESPONSE_FAVICON;
	else if( pRequest->strURL == RESPONSE_STR_BENCODE_INFO ) ucResponse = RESPONSE_BENCODE;
	else if( !rssdump.strName.empty( ) && pRequest->strURL == RESPONSE_STR_SEPERATOR + rssdump.strName ) ucResponse = RESPONSE_RSS;
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
		if( m_bServeLocal && !rssdump.strFile.empty( ) && ( pRequest->user.ucAccess & ACCESS_VIEW ) )
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
		if( m_bServeLocal && !rssdump.strFile.empty( ) && ( pRequest->user.ucAccess & ACCESS_VIEW ) )
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
		if( m_bServeLocal && !xmldump.strFile.empty( ) && ( pRequest->user.ucAccess & ACCESS_ADMIN ) )
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
		serverResponseStats( pRequest, pResponse );
		gtXStats.page.iStats++;

		break;
	case RESPONSE_LOGIN:
		serverResponseLogin( pRequest, pResponse );
		gtXStats.page.iLogin++;

		break;
	case RESPONSE_SIGNUP:
		serverResponseSignup( pRequest, pResponse );
		gtXStats.page.iSignup++;

		break;
	case RESPONSE_UPLOAD:
		serverResponseUploadGET( pRequest, pResponse );
		gtXStats.page.iUpload++;

		break;
	case RESPONSE_INFO:
		serverResponseInfo( pRequest, pResponse );
		gtXStats.page.iInfo++;

		break;
	case RESPONSE_ADMIN:
		serverResponseAdmin( pRequest, pResponse );
		gtXStats.page.iAdmin++;

		break;
	case RESPONSE_USERS:
		serverResponseUsers( pRequest, pResponse );
		gtXStats.page.iUsers++;

		break;
	case RESPONSE_COMMENTS:
		serverResponseComments( pRequest, pResponse );
		gtXStats.page.uiComments++;

		break;
	case RESPONSE_TORRENTS:
		serverResponseTorrent( pRequest, pResponse );
		gtXStats.page.iTorrents++;

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
	case RESPONSE_IMAGEFILL:
		if( m_bServeLocal && !imagefill.strFile.empty( ) && ( pRequest->user.ucAccess & ACCESS_VIEW ) )
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
		if( m_bServeLocal && !imagetrans.strFile.empty( ) && ( pRequest->user.ucAccess & ACCESS_VIEW ) )
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

	pResponse->strCode = "200 " + gmapLANG_CFG["server_response_200"];

	pResponse->mapHeaders.insert( pair<string, string>( "Content-Type", gmapMime[".rss"] ) );
	pResponse->mapHeaders.insert( pair<string, string>( "Content-Type", gmapMime[".xml"] ) );

	pResponse->strContent = rssdump.strFile;
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
	if( m_ucIPBanMode != 0 )
	{
		// Set the start time
		const struct bnbttv btv( UTIL_CurrentTime( ) );

		if( IsIPBanned( pRequest, pResponse, btv, "IP Banned", string( CSS_INDEX ), NOT_INDEX ) )
			return;
	}

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

#ifdef BNBT_MYSQL
	if( !m_bMySQLOverrideDState )
	{
#endif
		if( m_pDFile )
		{
			pData->setItem( "files", new CAtomLong( m_pDFile->getValuePtr( )->size( ) ) );
			unsigned long ulPeers = 0;

			map<string, CAtom *> *pmapDicti = m_pDFile->getValuePtr( );

			for( map<string, CAtom *> :: iterator it = pmapDicti->begin( ); it != pmapDicti->end( ); it++ )
			{
				if( (*it).second->isDicti( ) )
					ulPeers += (unsigned long)( (CAtomDicti *)(*it).second )->getValuePtr( )->size( );
			}

			pData->setItem( "peers", new CAtomLong( ulPeers ) );

			if( m_bCountUniquePeers )
				pData->setItem( "unique", new CAtomLong( m_pIPs->getValuePtr( )->size( ) ) );
		}
#ifdef BNBT_MYSQL
	}
	else
		UTIL_LogPrint( "serverResponseBencodeInfo: no dfile information\n" );
#endif

	pResponse->strContent = Encode( pData );
	pResponse->bCompressOK = false;

	delete pData;
}

// The main update routine
void CTracker :: Update( )
{
	if( !m_strAllowedDir.empty( ) )
	{
		if( m_uiParseAllowedInterval != 0 && GetTime( ) > m_ulParseAllowedNext )
		{
			// don't parse torrents again until we're done since parseTorrents calls gpServer->Update( ) which calls m_pTracker->Update( )

			m_ulParseAllowedNext = GetTime( ) + 9999;

			parseTorrents( m_strAllowedDir.c_str( ) );

			m_ulParseAllowedNext = GetTime( ) + m_uiParseAllowedInterval * 60;
		}
	}

#ifdef BNBT_MYSQL
	if( !m_strAllowedDir.empty( ) && m_uiMySQLRefreshAllowedInterval != 0 && GetTime( ) > m_ulMySQLRefreshAllowedNext )
	{
		if( gbDebug )
			if( gucDebugLevel & DEBUG_TRACKER )
				UTIL_LogPrint( "mysql - refreshing allowed\n" );

		if( m_pAllowed )
			delete m_pAllowed;

		m_pAllowed = new CAtomDicti( );

		CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bhash,bname FROM allowed" );

		vector<string> vecQuery;

		vecQuery.reserve(2);

		CAtomList *pList;

		vecQuery = pQuery->nextRow( );

		while( vecQuery.size( ) == 2 )
		{
			pList = new CAtomList( );

			pList->addItem( new CAtomString( ) );
			pList->addItem( new CAtomString( vecQuery[1] ) );
			pList->addItem( new CAtomString( ) );
			pList->addItem( new CAtomLong( ) );
			pList->addItem( new CAtomInt( ) );
			pList->addItem( new CAtomString( ) );

			m_pAllowed->setItem( vecQuery[0], pList );

			vecQuery = pQuery->nextRow( );
		}

		delete pQuery;

		m_ulMySQLRefreshAllowedNext = GetTime( ) + m_uiMySQLRefreshAllowedInterval;
	}

	if( m_uiMySQLRefreshStatsInterval != 0 && GetTime( ) > m_ulMySQLRefreshStatsNext )
	{
		if( gbDebug )
			if( gucDebugLevel & DEBUG_TRACKER )
				UTIL_LogPrint( "mysql - refreshing stats\n" );

		if( m_bMySQLOverrideDState )
		{
			// trucate tables
			CMySQLQuery mq01( "TRUNCATE TABLE hashes" );
			CMySQLQuery mq02( "TRUNCATE TABLE seeders" );
			CMySQLQuery mq03( "TRUNCATE TABLE leechers" );
			CMySQLQuery mq04( "TRUNCATE TABLE torrents" );

			// hashes
			if( !m_strAllowedDir.empty( ) )
			{
				if( m_bMySQLRefreshAllowedOnce || m_uiMySQLRefreshAllowedInterval > 0 )
				{
					CMySQLQuery mq05( "INSERT INTO hashes (bhash) SELECT bhash FROM allowed" );
					m_bMySQLRefreshAllowedOnce = false;
				}
				else
					CMySQLQuery mq05( "INSERT INTO hashes (bhash) SELECT DISTINCT bhash FROM dstate" );
			}
			else
				CMySQLQuery mq05( "INSERT INTO hashes (bhash) SELECT DISTINCT bhash FROM dstate" );

			// torrents
			CMySQLQuery mq06( "INSERT INTO torrents (bhash) SELECT bhash FROM hashes" );
			CMySQLQuery mq07( "REPLACE INTO torrents (bhash,bseeders) SELECT bhash,COUNT(*) FROM dstate WHERE bleft=0 GROUP BY bhash" );
			CMySQLQuery mq08( "REPLACE INTO torrents (bhash,bleechers) SELECT bhash,COUNT(*) FROM dstate WHERE bleft!=0 GROUP BY bhash" );
			CMySQLQuery mq09( "REPLACE INTO torrents (bhash,bcompleted) SELECT bhash,bcompleted FROM completed GROUP BY bhash" );

			// seeders
			CMySQLQuery mq10( "INSERT INTO seeders (bhash,bseeders) SELECT bhash,bseeders FROM torrents" );
			
			// leechers
			CMySQLQuery mq11( "INSERT INTO leechers (bhash,bleechers) SELECT bhash,bleechers FROM torrents" );
		}
		else
		{
			if( m_pDFile )
			{
				map<string, CAtom *> *pmapDicti = m_pDFile->getValuePtr( );

				string strQuery = string( );

				if( !pmapDicti->empty( ) )
					strQuery += "INSERT INTO torrents (bhash,bseeders,bleechers,bcompleted,badded) VALUES";

				unsigned long ulSeeders = 0;
				unsigned long ulLeechers = 0;
				unsigned long ulCompleted = 0;
				string sAdded = string( );

				map<string, CAtom *> *pmapPeersDicti;

				CAtom *pLeft = 0;
				CAtom *pCompleted = 0;

				for( map<string, CAtom *> :: iterator it = pmapDicti->begin( ); it != pmapDicti->end( ); )
				{
					if( (*it).second->isDicti( ) )
					{
						pmapPeersDicti = ( (CAtomDicti *)(*it).second )->getValuePtr( );

						ulSeeders = 0;
						ulLeechers = 0;
						ulCompleted = 0;

						for( map<string, CAtom *> :: iterator it2 = pmapPeersDicti->begin( ); it2 != pmapPeersDicti->end( ); it2++ )
						{
							if( (*it2).second->isDicti( ) )
							{
								pLeft = ( (CAtomDicti *)(*it2).second )->getItem( "left" );

								if( pLeft && dynamic_cast<CAtomLong *>( pLeft ) )
								{
									if( dynamic_cast<CAtomLong *>( pLeft )->getValue( ) == 0 )
										ulSeeders++;
									else
										ulLeechers++;
								}
							}
						}

						if( m_pCompleted )
						{
							pCompleted = m_pCompleted->getItem( (*it).first );

							if( pCompleted && dynamic_cast<CAtomLong *>( pCompleted ) )
								ulCompleted = (unsigned long)dynamic_cast<CAtomLong *>( pCompleted )->getValue( );
						}

						if( m_pAllowed )
						{
							CAtom *pList = m_pAllowed->getItem( (*it).first );
							CAtom *pAdded = 0;

							vector<CAtom *> vecTorrent;
							vecTorrent.reserve( 6 );

							if( pList && dynamic_cast<CAtomList *>( pList ) )
							{
								vecTorrent = dynamic_cast<CAtomList *>( pList )->getValue( );

								if( vecTorrent.size( ) == 6 )
								{
									pAdded = vecTorrent[2];

									if( pAdded )
										sAdded = pAdded->toString( );
								}
							}
						}

						strQuery += " (\'" + UTIL_StringToMySQL( (*it).first ) + "\'," + CAtomLong( ulSeeders ).toString( ) + "," + CAtomLong( ulLeechers ).toString( ) + "," + CAtomLong( ulCompleted ).toString( ) + ",'" + sAdded + "')";

						if( ++it != pmapDicti->end( ) )
							strQuery += ",";
					}
					else
						it++;
				}

				if( !strQuery.empty( ) )
				{
					if( m_bTorrentTraderCompatibility  )
						strQuery += " ON DUPLICATE KEY UPDATE bseeders=VALUES(bseeders), bleechers=VALUES(bleechers), bcompleted=VALUES(bcompleted) ;";
					else
						CMySQLQuery mq01( "TRUNCATE TABLE torrents" );

					CMySQLQuery mq02( strQuery );
				}
			}
		}

		m_ulMySQLRefreshStatsNext = GetTime( ) + m_uiMySQLRefreshStatsInterval;
	}

	if( !m_bMySQLOverrideDState )
	{
#endif
		// Should we save the dfile?
		if( GetTime( ) > m_ulSaveDFileNext )
		{
			if( gbDebug )
				if( gucDebugLevel & DEBUG_TRACKER )
					UTIL_LogPrint( "CTracker: saving dfile (%s)\n", m_strDFile.c_str( ) );

			saveDFile( );

			m_ulSaveDFileNext = GetTime( ) + m_uiSaveDFileInterval;
		}
#ifdef BNBT_MYSQL
	}
#endif

	// Should we refresh the static files?
	if( GetTime( ) > m_ulRefreshStaticNext )
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
			if( UTIL_CheckFile( favicon.strName.c_str( ) ) )
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

		// XBNBT - Serve HTTP interface requests using the internal file server
		if( m_bServeLocal )
		{
			// XML
			if( !xmldump.strName.empty( ) )
			{
				if( !xmldump.strDir.empty( ) )
					xmldump.strFile = UTIL_ReadFile( ( xmldump.strDir + RESPONSE_STR_SEPERATOR + xmldump.strName ).c_str( ) );
				else
					xmldump.strFile = UTIL_ReadFile( xmldump.strName.c_str( ) );
			}
			else if( gbDebug )
				if( gucDebugLevel & DEBUG_TRACKER )
					UTIL_LogPrint( "CTracker: XML file name not set\n" );

			// RSS
			if( !rssdump.strName.empty( ) )
			{
				if( !rssdump.strDir.empty( ) )
					rssdump.strFile = UTIL_ReadFile( ( rssdump.strDir + RESPONSE_STR_SEPERATOR + rssdump.strName ).c_str( ) );
				else
					rssdump.strFile = UTIL_ReadFile( rssdump.strName.c_str( ) );
			}
			else if( gbDebug )
				if( gucDebugLevel & DEBUG_TRACKER )
					UTIL_LogPrint( "CTracker: RSS file name not set\n" );

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

		m_ulRefreshStaticNext = GetTime( ) + m_uiRefreshStaticInterval * 60;
	}

	// Expire dounloaders
	if( GetTime( ) > m_ulDownloaderTimeOutNext )
	{
		if( gbDebug )
			if( gucDebugLevel & DEBUG_TRACKER )
				UTIL_LogPrint( ( gmapLANG_CFG["expiring_downloaders"] + "\n" ).c_str( ) );

		expireDownloaders( );

		m_ulDownloaderTimeOutNext = GetTime( ) + m_uiDownloaderTimeOutInterval;
	}

	// Dump RSS (thanks labarks)
	if( !rssdump.strName.empty( ) && m_uiDumpRSSInterval != 0 && GetTime( ) > m_ulDumpRSSNext )
	{
		if( m_ucDumpRSSFileMode == 0 || m_ucDumpRSSFileMode == 2 )
		{
			if( gbDebug )
				if( gucDebugLevel & DEBUG_TRACKER )
					UTIL_LogPrint( ( gmapLANG_CFG["dumping_rss"] + "\n" ).c_str( ) );

			saveRSS( );
		}

		if( m_ucDumpRSSFileMode == 1 || m_ucDumpRSSFileMode == 2 )
		{
			string strTag = string( );

			for( vector< pair<string, string> > :: iterator it = m_vecTags.begin( ); it != m_vecTags.end( ); it++ )
			{
				strTag = (string)(*it).first;

				if( gbDebug )
					if( gucDebugLevel & DEBUG_TRACKER )
						UTIL_LogPrint( ( gmapLANG_CFG["dumping_rss_for"] + "\n" ).c_str( ), strTag.c_str( ) );

				saveRSS( strTag );
			}

			if( !m_vecTags.size( ) && m_ucDumpRSSFileMode == 1 )
				UTIL_LogPrint( ( gmapLANG_CFG["dumping_rss_tags_warning"] + "\n" ).c_str( ) );
		}

		m_ulDumpRSSNext = GetTime( ) + m_uiDumpRSSInterval * 60;
	}

	// Dump XML
	if( !xmldump.strName.empty( ) && GetTime( ) > m_ulDumpXMLNext )
	{
		if( gbDebug )
			if( gucDebugLevel & DEBUG_TRACKER )
				UTIL_LogPrint( "CTracker: dumping xml\n" );

		saveXML( );

		m_ulDumpXMLNext = GetTime( ) + m_uiDumpXMLInterval;
	}

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
	if( gpLinkServer || gpLink || gpHUBLinkServer || gpHUBLink )
	{
		m_mtxQueued.Claim( );
		vector<struct announce_t> vecTemp;
		vecTemp.reserve(guiMaxConns);
		vecTemp = m_vecQueued;
		m_vecQueued.clear( );
		m_mtxQueued.Release( );
		bool bRespond = true;

		for( vector<struct announce_t> :: iterator it = vecTemp.begin( ); it != vecTemp.end( ); it++ )
		{
			if( m_pAllowed )
			{
				if( !m_pAllowed->getItem( (*it).strInfoHash ) )
					continue;
			}

			Announce( *it, bRespond );
		}
	}

#if defined ( XBNBT_MYSQL )
	// XBNBT MySQL Users Integration
	if( gbMySQLUsersOverrideUsers && m_uiMySQLUsersLoadInterval && GetTime( ) > m_ulMySQLUsersLoadNext )
		runLoadMySQLUsers( );
#endif

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
void CTracker :: saveXML( )
{
	string strData = string( );
	string tmpData = string( );
	unsigned int uiDL = 0;
	unsigned int uiComplete = 0;
	unsigned long ulCompleted = 0;

	//addition by labarks
	strData += "<?xml version=\"1.0\" encoding=\"" + gstrCharSet + "\"?>\n";

	strData += "<torrents>\n";

#ifdef BNBT_MYSQL
	if( !m_bMySQLOverrideDState )
	{
#endif
		if( m_pDFile )
		{
			map<string, CAtom *> *pmapDicti = m_pDFile->getValuePtr( );
			map<string, CAtom *> *pmapPeersDicti;

			vector<CAtom *> vecTorrent;
			vecTorrent.reserve( 6 );

			CAtom *pList = 0;
			CAtom *pFileName = 0;
			CAtom *pName = 0;
			CAtom *pAdded = 0;
			CAtom *pSize = 0;
			CAtom *pFiles = 0;
			CAtom *pCompleted = 0;
			CAtom *pDicti = 0;
			CAtom *pTag = 0;
			CAtom *pUploader = 0;
			CAtom *pIP = 0;
			CAtom *pUpped = 0;
			CAtom *pDowned = 0;
			CAtom *pLef = 0;
			CAtom *pConn = 0;
			CAtomDicti *pPeerDicti = 0;

			for( map<string, CAtom *> :: iterator it = pmapDicti->begin( ); it != pmapDicti->end( ); it++ )
			{
				strData += "<torrent hash=\"";
				strData += UTIL_HashToString( (*it).first );
				strData += "\"";

				if( m_pAllowed )
				{
					pList = m_pAllowed->getItem( (*it).first );

					if( pList && pList->isList( ) )
					{
						vecTorrent = ( (CAtomList *)pList )->getValue( );

						if( vecTorrent.size( ) == 6 )
						{
							pFileName = vecTorrent[0];
							pName = vecTorrent[1];
							pAdded = vecTorrent[2];
							pSize = vecTorrent[3];
							pFiles = vecTorrent[4];

							if( pFileName )
								strData += " filename=\"" + UTIL_RemoveHTML( pFileName->toString( ) ) + "\"";

							if( pName )
								strData += " name=\"" + UTIL_RemoveHTML( pName->toString( ) ) + "\"";

							if( pAdded )
								strData += " added=\"" + pAdded->toString( ) + "\"";

							if( pSize )
								strData += " size=\"" + pSize->toString( ) + "\"";

							if( pFiles )
								strData += " files=\"" + pFiles->toString( ) + "\"";
						}
					}
				}

				ulCompleted = 0;

				if( m_pCompleted )
				{
					pCompleted = m_pCompleted->getItem( (*it).first );

					if( pCompleted && pCompleted->isLong( ) )
						ulCompleted = (unsigned int)( (CAtomLong *)pCompleted )->getValue( );
				}
				strData += " completed=\"" + CAtomInt( ulCompleted ).toString( ) + "\"";

				if( m_pTags )
				{
					pDicti = m_pTags->getItem( (*it).first );

					if( pDicti && pDicti->isDicti( ) )
					{
						pTag = ( (CAtomDicti *)pDicti )->getItem( "tag" );
						pName = ( (CAtomDicti *)pDicti )->getItem( "name" );
						pUploader = ( (CAtomDicti *)pDicti )->getItem( "uploader" );

						if( pTag )
							strData += " tag=\"" + UTIL_RemoveHTML( pTag->toString( ) ) + "\"";

						if( pName )
							strData += " uploadname=\"" + UTIL_RemoveHTML( pName->toString( ) ) + "\"";

						if( m_bShowUploader )
						{
							if( pUploader )
								strData += " uploader=\"" + UTIL_RemoveHTML( pUploader->toString( ) ) + "\"";
						}
					}
				}

				tmpData = "";
				uiDL = 0;
				uiComplete = 0;

				if( (*it).second->isDicti( ) )
				{
					pmapPeersDicti = ( (CAtomDicti *)(*it).second )->getValuePtr( );

					if( m_bDumpXMLPeers )
					{
						if( !pmapPeersDicti->empty( ) )
						{
							tmpData += "<peers>\n";

							for( map<string, CAtom *> :: iterator it2 = pmapPeersDicti->begin( ); it2 != pmapPeersDicti->end( ); it2++ )
							{
								if( (*it2).second->isDicti( ) )
								{
									pPeerDicti = (CAtomDicti *)(*it2).second;

									pIP = pPeerDicti->getItem( "ip" );
									pUpped = pPeerDicti->getItem( "uploaded" );
									pDowned = pPeerDicti->getItem( "downloaded" );
									pLef = pPeerDicti->getItem( "left" );
									pConn = pPeerDicti->getItem( "connected" );

									if( ( (CAtomLong *)pLef )->getValue( ) == 0 )
										uiComplete++;
									else
										uiDL++;

									tmpData += "<peer id=\"";
									tmpData += UTIL_HashToString( (*it2).first );
									tmpData += "\"";

									if( pIP )
										tmpData += " ip=\"" + pIP->toString( ) + "\"";

									if( pUpped )
										tmpData += " uploaded=\"" + pUpped->toString( ) + "\"";

									if( pDowned )
										tmpData += " downloaded=\"" + pDowned->toString( ) + "\"";

									if( pLef && pLef->isLong( ) )
										tmpData += " left=\"" + pLef->toString( ) + "\"";

									if( pConn && pConn->isLong( ) )
										tmpData += " connected=\"" + CAtomLong( GetTime( ) - (unsigned long)( (CAtomLong *)pConn )->getValue( ) ).toString( ) + "\"";

									tmpData += " />\n";
								}
							}

							tmpData += "</peers>\n";
						}
					}
				}

				strData += " leecher=\""+ CAtomInt( uiDL ).toString( ) + "\"";
				strData += " seeder=\""+ CAtomInt( uiComplete ).toString( ) + "\"";
				strData += ">\n";

				if( m_bDumpXMLPeers )
					strData += tmpData;

				strData += "</torrent>\n";
			}
		}
#ifdef BNBT_MYSQL
	}
	else
		UTIL_LogPrint( "saveXML: no dfile information\n" );
#endif

	strData += "</torrents>\n";

	// write the file
	if( !xmldump.strName.empty( ) )
	{
		string strXMLFile = string( );

		if( !xmldump.strDir.empty( ) )
			strXMLFile = xmldump.strDir + PATH_SEP + xmldump.strName;
		else
			strXMLFile = xmldump.strName;

		FILE *pFile = FILE_ERROR;

		pFile = fopen( strXMLFile.c_str( ), "wb" );

		if( pFile == FILE_ERROR )
		{
			UTIL_LogPrint( string( gmapLANG_CFG["unable_to_write_file"] + "\n" ).c_str( ), strXMLFile.c_str( ) );

			if( gbDebug )
				if( gucDebugLevel & DEBUG_TRACKER )
					UTIL_LogPrint( "saveXML: completed\n" );

			return;
		}

		fwrite( strData.c_str( ), sizeof( char ), strData.size( ), pFile );
		fclose( pFile );
	}
}

// XBNBT IP Banning
const bool CTracker :: IsIPBanned( struct request_t *pRequest, struct response_t *pResponse, const struct bnbttv &btv, const string &cstrPageLabel, const string &cstrCSSLabel, const bool &bIsIndex )
{
	// retrieve ip
	const string cstrIP( inet_ntoa( pRequest->sin.sin_addr ) );

	switch( m_ucIPBanMode )
	{
	case IP_BLACKLIST:
		if( UTIL_IsIPBanList( cstrIP, m_pIPBannedList ) )
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
		if( UTIL_IsIPBanList( cstrIP, m_pIPBannedList ) )
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

	string strName = "bnbt_tag" + CAtomInt( ucTag ).toString( );
	string strTag = CFG_GetString( strName, string( ) );

	m_vecTags.reserve(16);
	m_vecTagsMouse.reserve(16);

	string :: size_type iSplit = 0;
	string :: size_type iSplitMouse = 0;

	while( !strTag.empty( ) )
	{
		iSplit = strTag.find( "|" );
		iSplitMouse = strTag.find( "|", iSplit + 1 );

		strName = "bnbt_tag" + CAtomInt( ++ucTag ).toString( );

		if( iSplit == string :: npos ) 
		{
			if( iSplitMouse == string :: npos )
			{
				m_vecTags.push_back( pair<string, string>( strTag, string( ) ) );
				m_vecTagsMouse.push_back( pair<string, string>( strTag, string( ) ) );
			}
			else
			{
				m_vecTags.push_back( pair<string, string>( strTag.substr( 0, iSplitMouse ), string( ) ) );
				m_vecTagsMouse.push_back( pair<string, string>( strTag.substr( 0, iSplitMouse ), strTag.substr( iSplitMouse + 1) ) );
			}
		}
		else
		{
			if( iSplitMouse == string :: npos )
			{
				m_vecTags.push_back( pair<string, string>( strTag.substr( 0, iSplit ), strTag.substr( iSplit + 1 ) ) );
				m_vecTagsMouse.push_back( pair<string, string>( strTag.substr( 0, iSplit ), string( ) ) );
			}
			else
			{
				m_vecTags.push_back( pair<string, string>( strTag.substr( 0, iSplit ), strTag.substr( iSplit + 1, iSplitMouse - iSplit - 1 ) ) );
				m_vecTagsMouse.push_back( pair<string, string>( strTag.substr( 0, iSplit ), strTag.substr( iSplitMouse + 1 ) ) );
			}
		}   
		strTag = CFG_GetString( strName, string( ) );
	}

	if( gbDebug )
		if( gucDebugLevel & DEBUG_TRACKER )
			UTIL_LogPrint( "initTags: completed\n" );
}

// Javascrpt

// Javascript for internalised mouseover feature
void CTracker :: JS_Mouseover_Head( struct response_t *pResponse )
{
	unsigned char ucImage = 1;

	pResponse->strContent += "<script type=\"text/javascript\">\n\n";
	pResponse->strContent += "<!--\n";

	if( !m_vecTags.empty( ) && !m_vecTagsMouse.empty( ) )
	{
		vector< pair< string, string > > :: iterator itTagsMouse = m_vecTagsMouse.begin( );

		unsigned char ucTag = 1;

		for( vector< pair< string, string > > :: iterator itTags = m_vecTags.begin( ); itTags != m_vecTags.end( ); itTags++ )
		{
			if( (*itTags).second.empty( ) || (*itTagsMouse).second.empty( ) )
			{
				ucTag++;
				itTagsMouse++;
				continue;
			}

			pResponse->strContent += "imagetag" + CAtomInt( ucImage ).toString( ) + " = new Image();\n";
			pResponse->strContent += "imagetag" + CAtomInt( ucImage ).toString( ) + ".src = \"" + (*itTagsMouse).second + "\";\n";
			pResponse->strContent += "function hoverOffTag" + CAtomInt( ucTag ).toString( ) + "() {\n";
			pResponse->strContent += "	document.bnbt_tag" + CAtomInt( ucTag ).toString( ) + ".src = \"" + (*itTagsMouse).second + "\";\n";
			pResponse->strContent += "}\n\n";

			ucImage++;

			pResponse->strContent += "imagetag" + CAtomInt( ucImage ).toString( ) + " = new Image();\n";
			pResponse->strContent += "imagetag" + CAtomInt( ucImage ).toString( ) + ".src = \"" + (*itTags).second + "\";\n";
			pResponse->strContent += "function hoverOnTag" + CAtomInt( ucTag ).toString( ) + "() {\n";
			pResponse->strContent += "	document.bnbt_tag" + CAtomInt( ucTag ).toString( ) + ".src = \"" + (*itTags).second + "\";\n";
			pResponse->strContent += "}\n\n";

			ucImage++;
			ucTag++;
			itTagsMouse++;
		}
	}

	pResponse->strContent += "//-->\n";
	pResponse->strContent += "</script>\n\n";
}

// Javascript for internalised mouseover feature
void CTracker :: JS_Mouseover_Tail( struct response_t *pResponse )
{
	pResponse->strContent += "<script type=\"text/javascript\">\n";
	pResponse->strContent += "<!--\n";

	if( !m_vecTags.empty( ) && !m_vecTagsMouse.empty( ) )
	{
		vector< pair< string, string > > :: iterator itTags = m_vecTags.begin( );

		unsigned char ucTag = 1;

		for( vector< pair< string, string > > :: iterator itTagsMouse = m_vecTagsMouse.begin( ); itTagsMouse != m_vecTagsMouse.end( ); itTagsMouse++ )
		{
			if( (*itTagsMouse).second.empty( ) || (*itTags).second.empty( ) )
			{
				ucTag++;
				itTags++;

				continue;
			}

			pResponse->strContent += "document.bnbt_tag" + CAtomInt( ucTag ).toString( ) + ".src = \"" + (*itTagsMouse).second + "\";\n";

			ucTag++;
			itTags++;
		}
	}

	pResponse->strContent += "//-->\n";
	pResponse->strContent += "</script>\n\n";
}

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
void CTracker :: HTML_Nav_Bar( struct request_t *pRequest, struct response_t *pResponse, const string &cstrCSS )
{
	if( !cstrCSS.empty( ) )
	{
		// This funtion builds a navbar
		// start navbar
		pResponse->strContent += "<div class=\"navbar_" + cstrCSS + "\">\n";

		// Index (RTT)
		if( pRequest->user.ucAccess & ACCESS_VIEW )
		{
			pResponse->strContent += "<span class=\"navbar_index_" + cstrCSS + "\"><a title=\"" + gmapLANG_CFG["navbar_index"] + "\" class=\"navbar_index_" + cstrCSS + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "\">" + gmapLANG_CFG["navbar_index"] + "</a></span>\n";
			pResponse->strContent += "<span class=\"navbar_pipe_login_" + cstrCSS + "\">|</span>";
		}

		// login & my torrents
		if( pRequest->user.strLogin.empty( ) )
			pResponse->strContent += "<span class=\"navbar_login_" + cstrCSS + "\"><a title=\"" + gmapLANG_CFG["navbar_login"] + "\" class=\"navbar_login_" + cstrCSS + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "\">" + gmapLANG_CFG["navbar_login"] + "</a></span>\n";
		else
			pResponse->strContent += "<span class=\"navbar_login_" + cstrCSS + "\"><a title=\"" + gmapLANG_CFG["navbar_my_torrents"] + "\" class=\"navbar_login_" + cstrCSS + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "\">" + gmapLANG_CFG["navbar_my_torrents"] + "</a></span>\n";

		// upload
		if( pRequest->user.ucAccess & ACCESS_UPLOAD )
			pResponse->strContent += "<span class=\"navbar_pipe_upload_" + cstrCSS + "\">|</span><span class=\"navbar_upload_" + cstrCSS + "\"><a title=\"" + gmapLANG_CFG["navbar_upload"] + "\" class=\"navbar_upload_" + cstrCSS + "\" href=\"" + RESPONSE_STR_UPLOAD_HTML + "\">" + gmapLANG_CFG["navbar_upload"] + "</a></span>\n";

		// logout
		if( !pRequest->user.strLogin.empty( ) )
			pResponse->strContent += "<span class=\"navbar_pipe_logout_" + cstrCSS + "\">|</span><span class=\"navbar_logout_" + cstrCSS + "\"><a title=\"" + gmapLANG_CFG["navbar_logout"] + "\" class=\"navbar_logout_" + cstrCSS + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "?logout=1\">" + gmapLANG_CFG["navbar_logout"] + "</a></span>\n";

		// signup
		if( pRequest->user.ucAccess & ACCESS_SIGNUP )
			pResponse->strContent += "<span class=\"navbar_pipe_signup_" + cstrCSS + "\">|</span><span class=\"navbar_signup_" + cstrCSS + "\"><a title=\"" + gmapLANG_CFG["navbar_sign_up"] + "\" class=\"navbar_signup_" + cstrCSS + "\" href=\"" + RESPONSE_STR_SIGNUP_HTML + "\">" + gmapLANG_CFG["navbar_sign_up"] + "</a></span>\n";

		// RSS
		if( ( pRequest->user.ucAccess & ACCESS_VIEW ) && !rssdump.strName.empty( ) )
		{
			if( !rssdump.strURL.empty( ) )
				pResponse->strContent += "<span class=\"navbar_pipe_rss_" + cstrCSS + "\">|</span><span class=\"navbar_rss_" + cstrCSS + "\"><a rel=\"" + STR_TARGET_REL + "\" title=\"" + gmapLANG_CFG["navbar_rss"] + "\" class=\"navbar_rss_" + cstrCSS + "\" href=\"" + rssdump.strURL + rssdump.strName + "\">" + gmapLANG_CFG["navbar_rss"] + "</a></span>\n";
			else if( m_bServeLocal )
				pResponse->strContent += "<span class=\"navbar_pipe_rss_" + cstrCSS + "\">|</span><span class=\"navbar_rss_" + cstrCSS + "\"><a rel=\"" + STR_TARGET_REL + "\" title=\"" + gmapLANG_CFG["navbar_rss"] + "\" class=\"navbar_rss_" + cstrCSS + "\" href=\"" + RESPONSE_STR_SEPERATOR + rssdump.strName + "\">" + gmapLANG_CFG["navbar_rss"] + "</a></span>\n";
		}

		// XML
		if( ( pRequest->user.ucAccess & ACCESS_ADMIN ) && !xmldump.strName.empty( ) )
		{
			if( !xmldump.strURL.empty( ) )
				pResponse->strContent += "<span class=\"navbar_pipe_xml_" + cstrCSS + "\">|</span><span class=\"navbar_xml_" + cstrCSS + "\"><a rel=\"" + STR_TARGET_REL + "\" title=\"" + gmapLANG_CFG["navbar_xml"] + "\" class=\"navbar_xml_" + cstrCSS + "\" href=\"" + xmldump.strURL + xmldump.strName + "\">" + gmapLANG_CFG["navbar_xml"] + "</a></span>\n";
			else if( m_bServeLocal )
				pResponse->strContent += "<span class=\"navbar_pipe_xml_" + cstrCSS + "\">|</span><span class=\"navbar_xml_" + cstrCSS + "\"><a rel=\"" + STR_TARGET_REL + "\" title=\"" + gmapLANG_CFG["navbar_xml"] + "\" class=\"navbar_xml_" + cstrCSS + "\" href=\"" + RESPONSE_STR_SEPERATOR + xmldump.strName + "\">" + gmapLANG_CFG["navbar_xml"] + "</a></span>\n";
		}

		// info
		if( ( pRequest->user.ucAccess & ACCESS_VIEW ) || ( pRequest->user.ucAccess & ACCESS_UPLOAD ) )
			pResponse->strContent += "<span class=\"navbar_pipe_info_" + cstrCSS + "\">|</span><span class=\"navbar_info_" + cstrCSS + "\"><a class=\"navbar_info_" + cstrCSS + "\" title=\"" + gmapLANG_CFG["navbar_info"] + "\" href=\"" + RESPONSE_STR_INFO_HTML + "\">" + gmapLANG_CFG["navbar_info"] + "</a></span>\n";

		// admin
		if( pRequest->user.ucAccess & ACCESS_ADMIN )
			pResponse->strContent += "<span class=\"navbar_pipe_admin_" + cstrCSS + "\">|</span><span class=\"navbar_admin_" + cstrCSS + "\"><a title=\"" + gmapLANG_CFG["navbar_admin"] + "\" class=\"navbar_admin_" + cstrCSS + "\" href=\"" + RESPONSE_STR_ADMIN_HTML + "\">" + gmapLANG_CFG["navbar_admin"] + "</a></span>\n";

		// users
		if( pRequest->user.ucAccess & ACCESS_ADMIN )
			pResponse->strContent += "<span class=\"navbar_pipe_users_" + cstrCSS + "\">|</span><span class=\"navbar_users_" + cstrCSS + "\"><a title=\"" + gmapLANG_CFG["navbar_users"] + "\" class=\"navbar_users_" + cstrCSS + "\" href=\"" + RESPONSE_STR_USERS_HTML + "\">" + gmapLANG_CFG["navbar_users"] + "</a></span>\n";

		// tags
		if( pRequest->user.ucAccess & ACCESS_ADMIN )
			pResponse->strContent += "<span class=\"navbar_pipe_tags_" + cstrCSS + "\">|</span><span class=\"navbar_tags_" + cstrCSS + "\"><a title=\"" + gmapLANG_CFG["navbar_tags"] + "\" class=\"navbar_tags_" + cstrCSS + "\" href=\"" + RESPONSE_STR_TAGS_HTML + "\">" + gmapLANG_CFG["navbar_tags"] + "</a></span>\n";

		// language
		if( pRequest->user.ucAccess & ACCESS_ADMIN )
			pResponse->strContent += "<span class=\"navbar_pipe_language_" + cstrCSS + "\">|</span><span class=\"navbar_language_" + cstrCSS + "\"><a title=\"" + gmapLANG_CFG["navbar_language"] + "\" class=\"navbar_language_" + cstrCSS + "\" href=\"" + RESPONSE_STR_LANGUAGE_HTML + "\">" + gmapLANG_CFG["navbar_language"] + "</a></span>\n";

		// xstats
		if( pRequest->user.ucAccess & ACCESS_ADMIN )
			pResponse->strContent += "<span class=\"navbar_pipe_xstats_" + cstrCSS + "\">|</span><span class=\"navbar_xstats_" + cstrCSS + "\"><a title=\"" + gmapLANG_CFG["navbar_xstats"] + "\" class=\"navbar_xstats_" + cstrCSS + "\" href=\"" + RESPONSE_STR_XSTATS_HTML + "\">" + gmapLANG_CFG["navbar_xstats"] + "</a></span>\n";

		// xtorrent
		if( pRequest->user.ucAccess & ACCESS_ADMIN )
			pResponse->strContent += "<span class=\"navbar_pipe_xtorrent_" + cstrCSS + "\">|</span><span class=\"navbar_xtorrent_" + cstrCSS + "\"><a title=\"" + gmapLANG_CFG["navbar_xtorrent"] + "\" class=\"navbar_xtorrent_" + cstrCSS + "\" href=\"" + RESPONSE_STR_XTORRENT_HTML + "\">" + gmapLANG_CFG["navbar_xtorrent"] + "</a></span>\n";

		// close navbar
		pResponse->strContent += "</div>\n\n";
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
	const string cstrIP( inet_ntoa( pRequest->sin.sin_addr ) );
	unsigned char ucExpireSecs = 0;

	if( cstrCSS == CSS_INDEX || cstrCSS == CSS_STATS || cstrCSS == CSS_INFO )
		ucExpireSecs = 15;

	time_t tExpires = time( 0 ) + ucExpireSecs;
	char *szExpires = asctime( gmtime( &tExpires ) );
	szExpires[strlen( szExpires ) - 1] = TERM_CHAR;

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
	if( !string( szExpires ).empty( ) )
		pResponse->mapHeaders.insert( pair<string, string>( "Expires", string( szExpires ) + " GMT" ) );	
	else
		UTIL_LogPrint( "CTracker: HTML_Common_Begin: Expires!\n" );

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
	if( bIndex && ( pRequest->user.ucAccess & ACCESS_VIEW ) && !rssdump.strName.empty( ) )
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
				const string cstrRSSByTag( rssdump.strName.substr( 0, rssdump.strName.length( ) - rssdump.strExt.length( ) ) + "-" );

				for( vector< pair< string, string > > :: iterator ulTagKey = m_vecTags.begin( ); ulTagKey != m_vecTags.end( ); ulTagKey++ )
				{
					pResponse->strContent += "<link rel=\"alternate\" type=\"" + gmapMime[".rss"] + "\" title=\"" + m_strTitle + ": " + gmapLANG_CFG["navbar_rss"] + " - ";

					if( !rssdump.strURL.empty( ) )
						pResponse->strContent += (*ulTagKey).first + "\" href=\"" + rssdump.strURL + cstrRSSByTag + (*ulTagKey).first + rssdump.strExt + "\">\n";
					else if( m_bServeLocal )
						pResponse->strContent += (*ulTagKey).first + "\" href=\"/" + cstrRSSByTag + (*ulTagKey).first + rssdump.strExt + "\">\n";
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
	if( !string( szExpires ).empty( ) )
		pResponse->strContent += "<META http-equiv=\"Expires\" content=\"" + string( szExpires ) + " GMT\">\n";

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

	// include JS_Mouseover_Head if enabled and is index
	if( !m_pDFile->isEmpty( ) )
		if( m_bUseMouseovers && bIndex && ( pRequest->user.ucAccess & ACCESS_VIEW ) )
			JS_Mouseover_Head( pResponse );

/*	int64 Uploaded = 0;
	int64 Downloaded = 0;
	
	
	if( m_pIPState )
	{
		
		if( !m_pIPState->getItem( cstrIP ) )
				m_pIPState->setItem( cstrIP, new CAtomDicti( ) );
		CAtom *pIPState = m_pIPState->getItem( cstrIP );
		
		if( pIPState && pIPState->isDicti( ) )
		{
			
			CAtomDicti *pIPStateDicti = dynamic_cast<CAtomDicti *>( pIPState );
			CAtom *pUploaded = 0;
			CAtom *pDownloaded = 0;
			
			pUploaded = ( (CAtomDicti *)( pIPStateDicti ) )->getItem( "uploaded" );
			pDownloaded = ( (CAtomDicti *)( pIPStateDicti ) )->getItem( "downloaded" );
			
			if( pUploaded )
				Uploaded = UTIL_StringTo64( pUploaded->toString( ).c_str( ) );
			if( pDownloaded )
				Downloaded = UTIL_StringTo64( pDownloaded->toString( ).c_str( ) );

		}
	}*/

	// display login status (login1=not logged in) (login2=logged in)
// 	pResponse->strContent += "<table class=\"top_bar\">";
	pResponse->strContent += "<table class=\"top_bar\"><tr class=\"top_bar\"><td class=\"avatar\">" + m_strTitle + "</td><td class=\"header\">" + cstrTitle + "</td>";
	
	pResponse->strContent += "<td class=\"top_login\">";
	
	if( pRequest->user.strLogin.empty( ) )
	//	pResponse->strContent += "<p class=\"login1_" + cstrCSS + "\">" + m_strLogin1 + " " + UTIL_Xsprintf( gmapLANG_CFG["show_ip"].c_str( ), string( "<span class=\"username\">" + cstrIP + "</span>" ).c_str( ) ) + " UP: " + UTIL_BytesToString( Uploaded ) + " DOWN: " + UTIL_BytesToString( Downloaded ) + "</p>\n";
// 		pResponse->strContent += "<p class=\"login1_" + cstrCSS + "\">" + m_strLogin1 + "<br>" + UTIL_Xsprintf( gmapLANG_CFG["show_ip"].c_str( ), string( "<span class=\"username\">" + cstrIP + "</span>" ).c_str( ) ) + "</p>\n";
		pResponse->strContent += "<span class=\"top_login1\">" + m_strLogin1 + "<br>" + UTIL_Xsprintf( gmapLANG_CFG["show_ip"].c_str( ), string( "<span class=\"username\">" + cstrIP + "</span>" ).c_str( ) ) + "</span>";
	else
//		pResponse->strContent += "<p class=\"login2_" + cstrCSS + "\">" + UTIL_Xsprintf( gmapLANG_CFG["login2"].c_str( ), string( "<span class=\"username\">" + UTIL_RemoveHTML( pRequest->user.strLogin ) + "</span>" ).c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_logout"] + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "?logout=1\">" ).c_str( ), "</a>" ) + " " + UTIL_Xsprintf( gmapLANG_CFG["show_ip"].c_str( ), string( "<span class=\"username\">" + cstrIP + "</span>" ).c_str( ) ) + " UP: " + UTIL_BytesToString( Uploaded ) + " DOWN: " + UTIL_BytesToString( Downloaded ) + "</p>\n";
// 		pResponse->strContent += "<p class=\"login2_" + cstrCSS + "\">" + UTIL_Xsprintf( gmapLANG_CFG["login2"].c_str( ), string( "<span class=\"username\">" + UTIL_RemoveHTML( pRequest->user.strLogin ) + "</span>" ).c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_logout"] + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "?logout=1\">" ).c_str( ), "</a>" ) + "<br>" + UTIL_Xsprintf( gmapLANG_CFG["show_ip"].c_str( ), string( "<span class=\"username\">" + cstrIP + "</span>" ).c_str( ) ) + "</p>\n";
		pResponse->strContent += "<span class=\"top_login2\">" + UTIL_Xsprintf( gmapLANG_CFG["login2"].c_str( ), string( "<span class=\"username\">" + UTIL_RemoveHTML( pRequest->user.strLogin ) + "</span>" ).c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_logout"] + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "?logout=1\">" ).c_str( ), "</a>" ) + "<br>" + UTIL_Xsprintf( gmapLANG_CFG["show_ip"].c_str( ), string( "<span class=\"username\">" + cstrIP + "</span>" ).c_str( ) ) + "</span>";
	
	pResponse->strContent += "</td></tr></table>\n<p>\n";
	// display tracker title
// 	if( bIndex && !m_strTitle.empty( ) )
// 		pResponse->strContent += "<h1 class=\"header_" + cstrCSS + "\">" + m_strTitle + "</h1>\n\n";

	// display page header
// 	if( !cstrTitle.empty( ) )
// 		pResponse->strContent += "<h3 class=\"header_" + cstrCSS + "\">" + cstrTitle + "</h3>\n\n";

	// Display static header
	if( ( ( bIndex && ( pRequest->user.ucAccess & ACCESS_VIEW ) ) || m_bStaticAll ) && !m_strStaticHeader.empty( ) )
		pResponse->strContent += "<div class=\"static_header\">\n" + m_strStaticHeader + "\n</div>\n\n";

	// Display navbar
	if( ( m_ucNavbar == 1 ) || ( m_ucNavbar == 3 ) )
		HTML_Nav_Bar( pRequest, pResponse, cstrCSS );

	// draw a line
	pResponse->strContent += "<hr class=\"header\">\n\n";

	// display redirect message if TRUE
	if( !cstrUrl.empty( ) )
		pResponse->strContent += "<p class=\"redirect\">" + UTIL_Xsprintf( gmapLANG_CFG["redirect"].c_str( ), string( "<a title=\"" + cstrUrl + "\" href=\"" + cstrUrl + "\">" ).c_str( ), "</a>" ) + "</p>\n\n";

	switch( cuiCode )
	{
	case CODE_401:
		pResponse->strContent += "<p class=\"unauthorized\">401 " + gmapLANG_CFG["server_response_401"] + "</p>\n" ;
		pResponse->strContent += "<p class=\"denied\">" + UTIL_Xsprintf( gmapLANG_CFG["view_not_authorized"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["login"] + "\" href=\"" + RESPONSE_STR_LOGIN_HTML + "\">" ).c_str( ), "</a>" );

		if( pRequest->user.ucAccess & ACCESS_SIGNUP )
			pResponse->strContent += " " + UTIL_Xsprintf( gmapLANG_CFG["click_to_signup"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_sign_up"] + "\" href=\"" + RESPONSE_STR_SIGNUP_HTML + "\">" ).c_str( ), "</a>" );

		pResponse->strContent += "</p>\n\n";

		break;
	case CODE_403:
		pResponse->strContent += "<p class=\"unauthorized\">403 " + gmapLANG_CFG["server_response_403"] + "</p>\n" ;
		pResponse->strContent += "<p class=\"denied\">" + gmapLANG_CFG["view_forbidden"] + "</p>\n\n";

		break;
	default:
		;
	}
}

// This function builds the HTML for the end of every display page and is HTML 4.01 Strict compliant
void CTracker :: HTML_Common_End( struct request_t *pRequest, struct response_t *pResponse, const struct bnbttv &btv, const bool &bIndex, const string &cstrCSS )
{
	// Display static footer
	if( ( ( bIndex && ( pRequest->user.ucAccess & ACCESS_VIEW ) ) || m_bStaticAll ) && !m_strStaticFooter.empty( ) )
		pResponse->strContent += "<div class=\"static_footer\">\n" + m_strStaticFooter + "\n</div>\n\n";

	// Draw line
	pResponse->strContent += "<hr class=\"footer\">\n\n";

	// include JS_Mouseover_Tail if enabled and is index
	if( !m_pDFile->isEmpty( ) )
		if( m_bUseMouseovers && bIndex && ( pRequest->user.ucAccess & ACCESS_VIEW ) )
			JS_Mouseover_Tail( pResponse );

	// Display navbar
	if( ( m_ucNavbar == 2 ) || ( m_ucNavbar == 3 ) )
		HTML_Nav_Bar( pRequest, pResponse, cstrCSS );

	// Display users online if enabled and is index
	if( m_bUsersOnline && bIndex && ( pRequest->user.ucAccess & ACCESS_VIEW ) )
	{
		const unsigned int cuiOnline( ( unsigned int )gpServer->m_vecClients.size( ) );

		if ( cuiOnline > 1 )
			// Users plural
			pResponse->strContent += "<p class=\"users_online\">" + UTIL_Xsprintf( gmapLANG_CFG["users_online"].c_str( ), CAtomInt( cuiOnline ).toString( ).c_str( ) ) + "</p>\n\n";
		else
			// User singular
			pResponse->strContent += "<p class=\"users_online\">" + UTIL_Xsprintf( gmapLANG_CFG["user_online"].c_str( ), CAtomInt( cuiOnline ).toString( ).c_str( ) ) + "</p>\n\n";
	}

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
	if( m_ucShowValidator != 0 || ( pRequest->user.ucAccess & ACCESS_ADMIN ) )
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
	if( m_bGen && ( pRequest->user.ucAccess & ACCESS_VIEW ) )
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

#if defined ( XBNBT_MYSQL )
// XBNBT MySQL Users Integration
void CTracker :: runLoadMySQLUsers( )
{
	if( gbMySQLUsersOverrideUsers )
	{
		if( gbDebug )
			if( gucDebugLevel & DEBUG_TRACKER )
				UTIL_LogPrint( string( gmapLANG_CFG["loading_mysql_users"] + "\n" ).c_str( ) );

		time_t tJoined;
		char pTime[256];
		memset( pTime, 0, sizeof( pTime ) / sizeof( char ) );

		CAtomDicti *pMySQLUsersUsers = new CAtomDicti( );

		string strQuery1 = string( );
		string strQuery2 = string( );
		unsigned long ulCountUsers = 0;

		vector<string> vecQuery;
		vecQuery.reserve(10000);

		CMySQLUsersQuery *pQuery = 0;

		CAtomDicti *pUser = 0;
		CMySQLUsersQuery *pQuery2 = 0;

		switch( m_ucMySQLUsersMode )
		{
		case VBULLITEN3_MODE:
			//added salt to the query 
			strQuery1 = "SELECT " + m_strMySQLUsersID + "," + gstrMySQLUsersName + "," + m_strMySQLUsersGroup + "," + m_strMySQLUsersPassword + ",xbnbt_perm," + m_strMySQLUsersEmail + "," + m_strMySQLUsersJoined + ",salt" + " FROM " + gstrMySQLUsersTable; 

			pQuery = new CMySQLUsersQuery( strQuery1 );

			vecQuery = pQuery->nextRow( );

			while( vecQuery.size( ) == 8 )
			{
				// Do not process if user group is guest(2), validating(1) or banned(5)
				// e.g. if( !( vecQuery[2] == "1" ) && !( vecQuery[2] == "2" ) && !( vecQuery[2] == "5" ) )
				if( ( vecQuery[2] != m_strMySQLUsersIgnoreGroup1 ) && ( vecQuery[2] != m_strMySQLUsersIgnoreGroup2 ) && ( vecQuery[2] != m_strMySQLUsersIgnoreGroup3 ) )
				{
					ulCountUsers++;

					tJoined = (time_t)atoi( vecQuery[6].c_str( ) );

					strftime( pTime, sizeof( pTime ) / sizeof( char ), "%Y-%m-%d %H:%M:%S", localtime( &tJoined ) );

					pUser = new CAtomDicti( );

					pUser->setItem( "md5", new CAtomString( vecQuery[3] ) );
					pUser->setItem( "access", new CAtomLong( UTIL_StringTo64( vecQuery[4].c_str( ) ) ) );
					pUser->setItem( "email", new CAtomString( vecQuery[5] ) );
					pUser->setItem( "created", new CAtomString( pTime ) );
					pUser->setItem( "salt", new CAtomString( vecQuery[7] ) );

					pMySQLUsersUsers->setItem( vecQuery[1], pUser );
				}

				vecQuery = pQuery->nextRow( );
			}

			delete pQuery;

			break;
		case IPB2_MODE:
			strQuery1 = "SELECT " + m_strMySQLUsersID + "," + gstrMySQLUsersName + "," + m_strMySQLUsersGroup + ",xbnbt_perm," + m_strMySQLUsersEmail + "," + m_strMySQLUsersJoined + " FROM " + gstrMySQLUsersTable;

			pQuery = new CMySQLUsersQuery( strQuery1 );

			vecQuery = pQuery->nextRow( );

			while( vecQuery.size( ) == 6 )
			{
				// Do not process if user group is guest(2), validating(1) or banned(5)
				// e.g. if( !( vecQuery[2] == "1" ) && !( vecQuery[2] == "2" ) && !( vecQuery[2] == "5" ) )
				if( ( vecQuery[2] != m_strMySQLUsersIgnoreGroup1 ) && ( vecQuery[2] != m_strMySQLUsersIgnoreGroup2 ) && ( vecQuery[2] != m_strMySQLUsersIgnoreGroup3 ) )
				{
					ulCountUsers++;

					tJoined = (time_t)atoi( vecQuery[5].c_str( ) );

					strftime( pTime, sizeof( pTime ) / sizeof( char ), "%Y-%m-%d %H:%M:%S", localtime( &tJoined ) );

					pUser = new CAtomDicti( );

					pUser->setItem( "access", new CAtomLong( UTIL_StringTo64( vecQuery[3].c_str( ) ) ) );
					pUser->setItem( "email", new CAtomString( vecQuery[4] ) );
					pUser->setItem( "created", new CAtomString( pTime ) );

					strQuery2 = "SELECT converge_pass_hash,converge_pass_salt,converge_email FROM ibf_members_converge WHERE converge_email = '" + vecQuery[4] +"'";

					pQuery2 = new CMySQLUsersQuery( strQuery2 );

					vector<string> vecQuery2;
					vecQuery2.reserve(10000);

					vecQuery2 = pQuery2->nextRow( );

					while( vecQuery2.size( ) == 3 )
					{
						pUser->setItem( "md5", new CAtomString( vecQuery2[0] ) );
						pUser->setItem( "salt", new CAtomString( vecQuery2[1] ) );

						vecQuery2 = pQuery2->nextRow( );
					}

					delete pQuery2;

					pMySQLUsersUsers->setItem( vecQuery[1], pUser );
				}

				vecQuery = pQuery->nextRow( );
			}

			delete pQuery;

			break;
		default:
			strQuery1 = "SELECT " + m_strMySQLUsersID + "," + gstrMySQLUsersName + "," + m_strMySQLUsersGroup + "," + m_strMySQLUsersPassword + ",xbnbt_perm," + m_strMySQLUsersEmail + "," + m_strMySQLUsersJoined + " FROM " + gstrMySQLUsersTable;

			pQuery = new CMySQLUsersQuery( strQuery1 );

			vecQuery = pQuery->nextRow( );

			while( vecQuery.size( ) == 7 )
			{
				// Do not process if user group is guest(2), validating(1) or banned(5)
				// e.g. if( !( vecQuery[2] == "1" ) && !( vecQuery[2] == "2" ) && !( vecQuery[2] == "5" ) )
				if( ( vecQuery[2] != m_strMySQLUsersIgnoreGroup1 ) && ( vecQuery[2] != m_strMySQLUsersIgnoreGroup2 ) && ( vecQuery[2] != m_strMySQLUsersIgnoreGroup3 ) )
				{
					ulCountUsers++;

					tJoined = (time_t)atoi( vecQuery[6].c_str( ) );

					strftime( pTime, sizeof( pTime ) / sizeof( char ), "%Y-%m-%d %H:%M:%S", localtime( &tJoined ) );

					pUser = new CAtomDicti( );

					pUser->setItem( "md5", new CAtomString( vecQuery[3] ) );
					pUser->setItem( "access", new CAtomLong( UTIL_StringTo64( vecQuery[4].c_str( ) ) ) );
					pUser->setItem( "email", new CAtomString( vecQuery[5] ) );
					pUser->setItem( "created", new CAtomString( pTime ) );

					pMySQLUsersUsers->setItem( vecQuery[1], pUser );
				}

				vecQuery = pQuery->nextRow( );
			}

			delete pQuery;
		}

		if( ( ulCountUsers != 0 ) && pMySQLUsersUsers && pMySQLUsersUsers->isDicti( ) )
		{
			delete m_pUsers;

			m_pUsers = (CAtomDicti *)pMySQLUsersUsers;
			m_strMySQLUsersLoadLast = UTIL_Date( );

			if( m_bBoot )
			{
				m_strMySQLUsersLoadLastStatus = gmapLANG_CFG["at_run_time"] ;
				m_bBoot = false;
			}
			else
				m_strMySQLUsersLoadLastStatus = gmapLANG_CFG["loaded"] ;

			UTIL_LogPrint( string( gmapLANG_CFG["loaded_mysql_users"] + "\n" ).c_str( ), ulCountUsers );
		}
		else
		{
			m_strMySQLUsersLoadLastStatus = gmapLANG_CFG["failed"] + ": " + UTIL_Date( );

			UTIL_LogPrint( string( gmapLANG_CFG["not_loaded_mysql_users"] + "\n" ).c_str( ) );

			if( pMySQLUsersUsers )
				delete pMySQLUsersUsers;
		}

		m_ulMySQLUsersLoadNext = GetTime( ) + m_uiMySQLUsersLoadInterval * 60;
	}
}
#endif

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

#ifdef BNBT_MYSQL
			if( !m_bMySQLOverrideDState )
			{
#endif
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
					CAtom *pFastCache = 0;
					CAtom *pDynstat = 0;
					CAtom *pIgnore = 0;
					CAtom *pIgnored = 0;

					for( map<string, CAtom *> :: iterator it = pmapDicti->begin( ); it != pmapDicti->end( ); it++ )
					{
						if( m_pAllowed )
						{
							pList = m_pAllowed->getItem( (*it).first );

							if( pList && dynamic_cast<CAtomList *>( pList ) )
							{
								vecTorrent = ( (CAtomList *)pList )->getValue( );

								if( vecTorrent.size( ) == 6 )
								{
									pFileName = vecTorrent[0];
									pName = vecTorrent[1];
									pAdded = vecTorrent[2];
									pSize = vecTorrent[3];
									pFiles = vecTorrent[4];

									pTorrents[ulKeyCount].strInfoHash = UTIL_HashToString( (*it).first );

									if( pFileName )
										pTorrents[ulKeyCount].strFileName = UTIL_RemoveHTML( pFileName->toString( ) );

									if( pName )
										pTorrents[ulKeyCount].strName = UTIL_RemoveHTML( pName->toString( ) );

									if( pAdded )
										pTorrents[ulKeyCount].strAdded = pAdded->toString( );

									if( pSize )
										pTorrents[ulKeyCount].iSize = ( (CAtomLong *)pSize )->getValue( );

									if( pFiles )
										pTorrents[ulKeyCount].uiFiles = ( (CAtomInt *)pFiles )->getValue( );
								}
							}
						}

						if( m_pTags )
						{
							pDicti = m_pTags->getItem( (*it).first );

							if( pDicti && pDicti->isDicti( ) )
							{
								pTag = ( (CAtomDicti *)pDicti )->getItem( "tag" );
								pName = ( (CAtomDicti *)pDicti )->getItem( "name" );

								if( pTag )
									pTorrents[ulKeyCount].strTag = UTIL_RemoveHTML( pTag->toString( ) );

								if( pName )
									pTorrents[ulKeyCount].strName = UTIL_RemoveHTML( pName->toString( ) );
							}
						}

						// Seeders and leecher using fastcache
						pFastCache = m_pFastCache->getItem( (*it).first );

						if( pFastCache && dynamic_cast<CAtomList *>( pFastCache ) )
						{
							vecTorrent = dynamic_cast<CAtomList *>( pFastCache )->getValue( );

							pTorrents[ulKeyCount].uiSeeders = CAtomInt( *dynamic_cast<CAtomInt *>( vecTorrent[0] ) ).getValue( );
							pTorrents[ulKeyCount].uiLeechers = CAtomInt( *dynamic_cast<CAtomInt *>( vecTorrent[1] ) ).getValue( );
						}
						else
						{
							pTorrents[ulKeyCount].uiSeeders = 0;
							pTorrents[ulKeyCount].uiLeechers = 0;
						}

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
#ifdef BNBT_MYSQL
			}
			else
				UTIL_LogPrint( "runGenerateDynstat: no dfile information\n" );
#endif

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
