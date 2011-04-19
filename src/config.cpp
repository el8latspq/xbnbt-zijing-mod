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

#include <fstream>

#include "bnbt.h"
#include "atom.h"
#include "config.h"
#include "tracker.h"
#include "util.h"

map<string, string> gmapCFG;

void CFG_Open( const char *szFile )
{
	gmapCFG.clear( );

	ifstream in;

	in.open( szFile );

	if( in.fail( ) )
	{
		UTIL_LogPrint( "config warning - unable to open %s for reading\n", szFile );

		return;
	}

	char pBuf[1024];

	string strTemp = string( );

	string :: size_type iSplit = 0;

	string :: size_type iKeyStart = 0;
	string :: size_type iKeyEnd = 0;
	string :: size_type iValueStart = 0;
	string :: size_type iValueEnd = 0;

	while( !in.eof( ) )
	{
		memset( pBuf, 0, sizeof(pBuf) / sizeof(char) );

		in.getline( pBuf, sizeof(pBuf) - 1 );

		strTemp = pBuf;

		// ignore blank lines and comments

		if( strTemp.empty( ) || strTemp[0] == '#' )
			continue;

		iSplit = strTemp.find( "=" );

		if( iSplit == string :: npos )
			continue;

		iKeyStart = strTemp.find_first_not_of( " " );
		iKeyEnd = strTemp.find( " ", iKeyStart );
		iValueStart = strTemp.find_first_not_of( " ", iSplit + 1 );
		iValueEnd = strTemp.size( );

		if( iValueStart != string :: npos )
			gmapCFG[strTemp.substr( iKeyStart, iKeyEnd - iKeyStart )] = strTemp.substr( iValueStart, iValueEnd - iValueStart );
	}

	in.close( );
}

void CFG_SetInt( const string &cstrKey, const int &ciX )
{
	gmapCFG[cstrKey] = CAtomInt( ciX ).toString( );
}

void CFG_SetString( const string &cstrKey, const string &csX )
{
	gmapCFG[cstrKey] = csX;
}

const int CFG_GetInt( const string &cstrKey, const int &ciX )
{
	if( gmapCFG.find( cstrKey ) == gmapCFG.end( ) )
		return ciX;
	else
		return atoi( gmapCFG[cstrKey].c_str( ) );
}

const string CFG_GetString( const string &cstrKey, const string &csX )
{
	if( gmapCFG.find( cstrKey ) == gmapCFG.end( ) )
		return csX;
	else
		return gmapCFG[cstrKey];
}

void CFG_Delete( const string &cstrKey )
{
	gmapCFG.erase( cstrKey );
}

void CFG_Close( const char *szFile )
{
	ofstream out;

	out.open( szFile );

	if( out.fail( ) )
	{
		UTIL_LogPrint( "config warning - unable to open %s for writing\n", szFile );

		return;
	}

	for( map<string, string> :: iterator it = gmapCFG.begin( ); it != gmapCFG.end( ); it++ )
		out << (*it).first.c_str( ) << " = " << (*it).second.c_str( ) << endl;

	out.close( );
}

void CFG_SetDefaults( )
{
	if( gbDebug )
		UTIL_LogPrint( "config - setting defaults\n" );

	// bnbt.cpp

	if( CFG_GetInt( "bnbt_debug", -1 ) < 0 )
		CFG_SetInt( "bnbt_debug", 0 );

	if( CFG_GetInt( "bnbt_max_conns", 0 ) < 1 )
		CFG_SetInt( "bnbt_max_conns", 64 );

	if( CFG_GetString( "bnbt_style_sheet", string( ) ).empty( ) )
		CFG_SetString( "bnbt_style_sheet", string( ) );

	if( CFG_GetString( "bnbt_charset", string( ) ).empty( ) )
		CFG_SetString( "bnbt_charset", "utf-8" );

	if( CFG_GetString( "bnbt_realm", string( ) ).empty( ) )
		CFG_SetString( "bnbt_realm", "BNBT" );

	if( CFG_GetString( "bnbt_log_dir", string( ) ).empty( ) )
		CFG_SetString( "bnbt_log_dir", string( ) );

	if( CFG_GetString( "bnbt_error_log_dir", string( ) ).empty( ) )
		CFG_SetString( "bnbt_error_log_dir", string( ) );

	if( CFG_GetString( "bnbt_access_log_dir", string( ) ).empty( ) )
		CFG_SetString( "bnbt_access_log_dir", string( ) );

	if( CFG_GetInt( "bnbt_flush_interval", 0 ) < 1 )
		CFG_SetInt( "bnbt_flush_interval", 100 );

	// mysql

	if( CFG_GetString( "mysql_host", string( ) ).empty( ) )
		CFG_SetString( "mysql_host", string( ) );

	if( CFG_GetString( "mysql_database", string( ) ).empty( ) )
		CFG_SetString( "mysql_database", "bnbt" );

	if( CFG_GetString( "mysql_user", string( ) ).empty( ) )
		CFG_SetString( "mysql_user", string( ) );

	if( CFG_GetString( "mysql_password", string( ) ).empty( ) )
		CFG_SetString( "mysql_password", string( ) );

	if( CFG_GetInt( "mysql_port", -1 ) < 0 )
		CFG_SetInt( "mysql_port", 0 );

	if( CFG_GetInt( "mysql_refresh_allowed_interval", -1 ) < 0 )
		CFG_SetInt( "mysql_refresh_allowed_interval", 0 );

	if( CFG_GetInt( "mysql_refresh_stats_interval", 0 ) < 1 )
		CFG_SetInt( "mysql_refresh_stats_interval", 600 );

	if( CFG_GetInt( "mysql_override_dstate", -1 ) < 0 )
		CFG_SetInt( "mysql_override_dstate", 0 );

	// link.cpp

	if( CFG_GetInt( "bnbt_tlink_server", -1 ) < 0 )
		CFG_SetInt( "bnbt_tlink_server", 0 );

	if( CFG_GetString( "bnbt_tlink_connect", string( ) ).empty( ) )
		CFG_SetString( "bnbt_tlink_connect", string( ) );

	if( CFG_GetString( "bnbt_tlink_password", string( ) ).empty( ) )
		CFG_SetString( "bnbt_tlink_password", string( ) );

	if( CFG_GetString( "bnbt_tlink_bind", string( ) ).empty( ) )
		CFG_SetString( "bnbt_tlink_bind", string( ) );

	if( CFG_GetInt( "bnbt_tlink_port", 0 ) < 1 )
		CFG_SetInt( "bnbt_tlink_port", 5204 );

	// server.cpp

	if( CFG_GetInt( "socket_timeout", 0 ) < 1 )
		CFG_SetInt( "socket_timeout", 15 );

	if( CFG_GetInt( "bnbt_compression_level", -1 ) < 0 )
		CFG_SetInt( "bnbt_compression_level", 6 );

	if( CFG_GetString( "bind", string( ) ).empty( ) )
		CFG_SetString( "bind", string( ) );

	if( CFG_GetInt( "port", 0 ) < 1 )
		CFG_SetInt( "port", 6969 );

	// tracker.cpp

	if( CFG_GetString( "allowed_dir", string( ) ).empty( ) )
		CFG_SetString( "allowed_dir", string( ) );

	if( CFG_GetString( "bnbt_intr_dir", string( ) ).empty( ) )
		CFG_SetString( "bnbt_intr_dir", string( ) );

	if( CFG_GetString( "bnbt_upload_dir", string( ) ).empty( ) )
		CFG_SetString( "bnbt_upload_dir", string( ) );

	if( CFG_GetString( "bnbt_external_torrent_dir", string( ) ).empty( ) )
		CFG_SetString( "bnbt_external_torrent_dir", string( ) );

	if( CFG_GetString( "bnbt_archive_dir", string( ) ).empty( ) )
		CFG_SetString( "bnbt_archive_dir", string( ) );

	if( CFG_GetString( "bnbt_file_dir", string( ) ).empty( ) )
		CFG_SetString( "bnbt_file_dir", string( ) );

	if( CFG_GetString( "dfile", string( ) ).empty( ) )
		CFG_SetString( "dfile", "dstate.bnbt" );

	if( CFG_GetString( "bnbt_comments_file", string( ) ).empty( ) )
		CFG_SetString( "bnbt_comments_file", string( ) );

	if( CFG_GetString( "bnbt_tag_file", string( ) ).empty( ) )
		CFG_SetString( "bnbt_tag_file", "tags.bnbt" );

	if( CFG_GetString( "bnbt_users_file", string( ) ).empty( ) )
		CFG_SetString( "bnbt_users_file", "users.bnbt" );

	if( CFG_GetString( "bnbt_static_header", string( ) ).empty( ) )
		CFG_SetString( "bnbt_static_header", string( ) );

	if( CFG_GetString( "bnbt_static_footer", string( ) ).empty( ) )
		CFG_SetString( "bnbt_static_footer", string( ) );

	if( CFG_GetString( "bnbt_robots_txt", string( ) ).empty( ) )
		CFG_SetString( "bnbt_robots_txt", string( ) );

	// addition by labarks

	if( CFG_GetString( "bnbt_rss_file", string( ) ).empty( ) )
		CFG_SetString( "bnbt_rss_file", string( ) );

	if( CFG_GetString( "bnbt_rss_online_dir", string( ) ).empty( ) )
		CFG_SetString( "bnbt_rss_online_dir", string( ) );

	if( CFG_GetInt( "bnbt_rss_file_mode", -1 ) < 0 )
		CFG_SetInt( "bnbt_rss_file_mode", 0 );

	if( CFG_GetInt( "bnbt_rss_channel_ttl", -1 ) < 0 )
		CFG_SetInt( "bnbt_rss_channel_ttl", 60 );

	if( CFG_GetString( "bnbt_rss_channel_image_url", string( ) ).empty( ) )
		CFG_SetString( "bnbt_rss_channel_image_url", string( ) );

	if( CFG_GetInt( "bnbt_rss_channel_image_width", -1 ) < 0 )
		CFG_SetInt( "bnbt_rss_channel_image_width", 0 );

	if( CFG_GetInt( "bnbt_rss_channel_image_height", -1 ) < 0 )
		CFG_SetInt( "bnbt_rss_channel_image_height", 0 );

	if( CFG_GetString( "bnbt_rss_channel_copyright", string( ) ).empty( ) )
		CFG_SetString( "bnbt_rss_channel_copyright", string( ) );

	if( CFG_GetInt( "bnbt_rss_limit", -1 ) < 0 )
		CFG_SetInt( "bnbt_rss_limit", 25 );

	// end addition

	if( CFG_GetString( "image_bar_fill", string( ) ).empty( ) )
		CFG_SetString( "image_bar_fill", string( ) );

	if( CFG_GetString( "image_bar_trans", string( ) ).empty( ) )
		CFG_SetString( "image_bar_trans", string( ) );

	if( CFG_GetString( "bnbt_force_announce_url", string( ) ).empty( ) )
		CFG_SetString( "bnbt_force_announce_url", string( ) );

	if( CFG_GetInt( "bnbt_force_announce_on_download", -1 ) < 0 )
		CFG_SetInt( "bnbt_force_announce_on_download", 0 );

	if( CFG_GetInt( "parse_allowed_interval", -1 ) < 0 )
		CFG_SetInt( "parse_allowed_interval", 0 );

	if( CFG_GetInt( "save_dfile_interval", 0 ) < 1 )
		CFG_SetInt( "save_dfile_interval", 300 );

	if( CFG_GetInt( "downloader_timeout_interval", 0 ) < 1 )
		CFG_SetInt( "downloader_timeout_interval", 2700 );

	if( CFG_GetInt( "bnbt_refresh_static_interval", 0 ) < 1 )
		CFG_SetInt( "bnbt_refresh_static_interval", 10 );

  	if( CFG_GetInt( "bnbt_refresh_fast_cache_interval", -1 ) < 0 )
		CFG_SetInt( "bnbt_refresh_fast_cache_interval", 30 );

	// added by labarks

	if( CFG_GetInt( "bnbt_rss_interval", -1 ) < 0 )
		CFG_SetInt( "bnbt_rss_interval", 30 );

	if( CFG_GetInt( "announce_interval", -1 ) < 0 )
		CFG_SetInt( "announce_interval", 1800 );

	if( CFG_GetInt( "min_request_interval", -1 ) < 0 )
		CFG_SetInt( "min_request_interval", 3600 );

	if( CFG_GetInt( "response_size", -1 ) < 0 )
		CFG_SetInt( "response_size", 50 );

	if( CFG_GetInt( "max_give", -1 ) < 0 )
		CFG_SetInt( "max_give", 200 );

	// Modified by =Xotic=
	// CBTT Modification
	// The Trinity Edition - Modification Begins
	// Changes the default keep_dead value to 1
	if( CFG_GetInt( "keep_dead", -1 ) < 0 )
	{
		if( CFG_GetInt( "bnbt_display_all", -1) >= 0 )
		{
			if( CFG_GetInt( "bnbt_display_all", -1) == 0 )
				CFG_SetInt( "keep_dead", 0 );
			else if( CFG_GetInt( "bnbt_display_all", -1) == 1 )
				CFG_SetInt( "keep_dead", 1 );
		}
		else
			CFG_SetInt( "keep_dead", 1 );
	}

	if( CFG_GetInt( "bnbt_allow_scrape", -1 ) < 0 )
		CFG_SetInt( "bnbt_allow_scrape", 1 );

	if( CFG_GetInt( "bnbt_count_unique_peers", -1 ) < 0 )
		CFG_SetInt( "bnbt_count_unique_peers", 1 );

	if( CFG_GetInt( "bnbt_delete_invalid", -1 ) < 0 )
		CFG_SetInt( "bnbt_delete_invalid", 0 );

	if( CFG_GetInt( "bnbt_parse_on_upload", -1 ) < 0 )
		CFG_SetInt( "bnbt_parse_on_upload", 1 );

	if( CFG_GetInt( "bnbt_max_torrents", -1 ) < 0 )
		CFG_SetInt( "bnbt_max_torrents", 0 );

	if( CFG_GetInt( "bnbt_show_info_hash", -1 ) < 0 )
		CFG_SetInt( "bnbt_show_info_hash", 0 );

	if( CFG_GetInt( "show_names", -1 ) < 0 )
		CFG_SetInt( "show_names", 1 );

	if( CFG_GetInt( "bnbt_show_stats", -1 ) < 0 )
		CFG_SetInt( "bnbt_show_stats", 1 );

	if( CFG_GetInt( "bnbt_allow_torrent_downloads", -1 ) < 0 )
		CFG_SetInt( "bnbt_allow_torrent_downloads", 1 );

	if( CFG_GetInt( "bnbt_allow_comments", -1 ) < 0 )
		CFG_SetInt( "bnbt_allow_comments", 0 );

	if( CFG_GetInt( "bnbt_show_added", -1 ) < 0 )
		CFG_SetInt( "bnbt_show_added", 1 );

	if( CFG_GetInt( "bnbt_show_size", -1 ) < 0 )
		CFG_SetInt( "bnbt_show_size", 1 );

	if( CFG_GetInt( "bnbt_show_num_files", -1 ) < 0 )
		CFG_SetInt( "bnbt_show_num_files", 1 );

	if( CFG_GetInt( "bnbt_show_completed", -1 ) < 0 )
		CFG_SetInt( "bnbt_show_completed", 0 );

	if( CFG_GetInt( "bnbt_show_transferred", -1 ) < 0 )
		CFG_SetInt( "bnbt_show_transferred", 0 );

	if( CFG_GetInt( "bnbt_show_min_left", -1 ) < 0 )
		CFG_SetInt( "bnbt_show_min_left", 0 );

	if( CFG_GetInt( "bnbt_show_average_left", -1 ) < 0 )
		CFG_SetInt( "bnbt_show_average_left", 0 );

	if( CFG_GetInt( "bnbt_show_max_left", -1 ) < 0 )
		CFG_SetInt( "bnbt_show_max_left", 0 );

	if( CFG_GetInt( "bnbt_show_left_as_progress", -1 ) < 0 )
		CFG_SetInt( "bnbt_show_left_as_progress", 1 );

	if( CFG_GetInt( "bnbt_show_uploader", -1 ) < 0 )
		CFG_SetInt( "bnbt_show_uploader", 0 );

	if( CFG_GetInt( "bnbt_allow_info_link", -1 ) < 0 )
		CFG_SetInt( "bnbt_allow_info_link", 0 );

	if( CFG_GetInt( "bnbt_allow_search", -1 ) < 0 )
		CFG_SetInt( "bnbt_allow_search", 1 );

	if( CFG_GetInt( "bnbt_allow_sort", -1 ) < 0 )
		CFG_SetInt( "bnbt_allow_sort", 1 );

	if( CFG_GetInt( "bnbt_show_file_comment", -1 ) < 0 )
		CFG_SetInt( "bnbt_show_file_comment", 1 );

	if( CFG_GetInt( "bnbt_show_file_contents", -1 ) < 0 )
		CFG_SetInt( "bnbt_show_file_contents", 0 );

	if( CFG_GetInt( "bnbt_show_share_ratios", -1 ) < 0 )
		CFG_SetInt( "bnbt_show_share_ratios", 1 );

	if( CFG_GetInt( "bnbt_show_average_dl_rate", -1 ) < 0 )
		CFG_SetInt( "bnbt_show_average_dl_rate", 0 );

	if( CFG_GetInt( "bnbt_show_average_ul_rate", -1 ) < 0 )
		CFG_SetInt( "bnbt_show_average_ul_rate", 0 );

	if( CFG_GetInt( "bnbt_delete_own_torrents", -1 ) < 0 )
		CFG_SetInt( "bnbt_delete_own_torrents", 1 );

	if( CFG_GetInt( "bnbt_show_gen_time", -1 ) < 0 )
		CFG_SetInt( "bnbt_show_gen_time", 1 );

	if( CFG_GetInt( "bnbt_per_page", -1 ) < 0 )
		CFG_SetInt( "bnbt_per_page", 20 );

	if( CFG_GetInt( "bnbt_users_per_page", -1 ) < 0 )
		CFG_SetInt( "bnbt_users_per_page", 50 );

	if( CFG_GetInt( "bnbt_max_peers_display", -1 ) < 0 )
		CFG_SetInt( "bnbt_max_peers_display", 500 );

	if( CFG_GetInt( "bnbt_guest_access", -1 ) < 0 )
		CFG_SetInt( "bnbt_guest_access", ACCESS_VIEW );

	if( CFG_GetInt( "bnbt_member_access", -1 ) < 0 )
		CFG_SetInt( "bnbt_member_access", ACCESS_VIEW | ACCESS_DL | ACCESS_COMMENTS );

	if( CFG_GetInt( "bnbt_file_expires", -1 ) < 0 )
		CFG_SetInt( "bnbt_file_expires", 180 );

	if( CFG_GetInt( "bnbt_name_length", -1 ) < 0 )
		CFG_SetInt( "bnbt_name_length", 32 );

	if( CFG_GetInt( "bnbt_comment_length", -1 ) < 0 )
		CFG_SetInt( "bnbt_comment_length", 800 );

	if( CFG_GetInt( "bnbt_max_recv_size", -1 ) < 0 )
		CFG_SetInt( "bnbt_max_recv_size", 524288 );
	
	// Added by =Xotic
	
	// The Trinity Edition - Addition Begins
	// Sets the default value for "bnbt_use_mouseovers" to 0 (FALSE)

	// Internalised mouseover
	if( CFG_GetInt( "bnbt_use_mouseovers", -1 ) < 0 )
	{
		if( CFG_GetInt( "trinity_use_mouseovers", -1 ) < 1 )
		{
			CFG_SetInt( "bnbt_use_mouseovers", 0 );
			CFG_Delete( "trinity_use_mouseovers" );
		}
		else
		{
            CFG_SetInt( "bnbt_use_mouseovers", 1 );
			CFG_Delete( "trinity_use_mouseovers" );
		}
	}

	// CBTT values
	if( CFG_GetInt( "cbtt_ban_mode", -1 ) < 0 )
		CFG_SetInt( "cbtt_ban_mode", 0 );

	if( CFG_GetInt( "cbtt_restricted_peer_spoofing", -1 ) < 0 )
		CFG_SetInt( "cbtt_restricted_peer_spoofing", 0 );

	if( CFG_GetString( "cbtt_ban_file", string( ) ).empty( ) )
		CFG_SetString( "cbtt_ban_file", "clientbans.bnbt" );

	if( CFG_GetString( "cbtt_service_name", string( ) ).empty( ) )
	{
		if( !CFG_GetString( "trinity_nt_service_name",string( ) ).empty( ) )
			CFG_SetString( "cbtt_service_name", CFG_GetString( "trinity_nt_service_name",string( ) ) );
		else
			CFG_SetString( "cbtt_service_name", "BNBT Service" );
	}
		
	if( CFG_GetInt( "cbtt_ip_ban_mode", -1 ) < 0 )
		CFG_SetInt( "cbtt_ip_ban_mode", 0 );

	if( CFG_GetString( "cbtt_ipban_file", string( ) ).empty( ) )
		CFG_SetString( "cbtt_ipban_file", "bans.bnbt" );

	if( CFG_GetInt( "cbtt_dont_compress_torrents", -1 ) < 0 )
		CFG_SetInt( "cbtt_dont_compress_torrents", 1 );

	if( CFG_GetInt( "cbtt_restrict_overflow", -1 ) <0 )
		CFG_SetInt( "cbtt_restrict_overflow", 0 );

	if( CFG_GetString( "cbtt_restrict_overflow_limit", string( ) ).empty( ) )
		CFG_SetString( "cbtt_restrict_overflow_limit", "1099511627776" );

	if( CFG_GetInt( "cbtt_block_private_ip", -1 ) < 0 )
	{
		if( CFG_GetInt( "bnbt_block_private_ip", -1 ) >= 0 )
		{
            CFG_SetInt( "cbtt_block_private_ip", CFG_GetInt( "bnbt_block_private_ip", -1 ) );
			CFG_Delete( "bnbt_block_private_ip");
		}
		else
			CFG_SetInt( "cbtt_block_private_ip", 0 );
	}

	if( CFG_GetInt( "cbtt_blacklist_common_p2p_ports", -1) < 0)
		CFG_SetInt( "cbtt_blacklist_common_p2p_ports", 0);

	if( CFG_GetInt( "only_local_override_ip", -1 ) < 0 )
		CFG_SetInt( "only_local_override_ip", 0 );

	if( CFG_GetInt( "cbtt_block_search_robots", -1 ) < 0 )
		CFG_SetInt( "cbtt_block_search_robots", 0 );
  
	if( CFG_GetInt( "mysql_cbtt_ttrader_support", -1 ) < 0 )
        CFG_SetInt( "mysql_cbtt_ttrader_support", 0 );

	if( CFG_GetString( "favicon", string( ) ).empty() )
		CFG_SetString( "favicon", string( ) );

	if( CFG_GetString( "bnbt_error_log_file_pattern", string( ) ).empty( ) )
		CFG_SetString( "bnbt_error_log_file_pattern", "%Y-%m-%de.log" );

	if( CFG_GetString( "bnbt_access_log_file_pattern", string( ) ).empty( ) )
		CFG_SetString( "bnbt_access_log_file_pattern", "%Y-%m-%d.log" );

	if( CFG_GetInt( "bnbt_private_tracker_flag", -1 ) < 0 )
		 CFG_SetInt( "bnbt_private_tracker_flag", 0 );
	
	if( CFG_GetInt( "bnbt_private_tracker_flag", -1 ) > 1 )
		CFG_SetInt( "bnbt_private_tracker_flag", 1 );

	// XBNBT DynStat

	if( CFG_GetString( "xbnbt_dynstat_file", string( ) ).empty( ) )
		CFG_SetString( "xbnbt_dynstat_file", "dynstat.bnbt" );

	if( CFG_GetInt( "xbnbt_dynstat_generate", -1 ) < 0 )
		CFG_SetInt( "xbnbt_dynstat_generate", 0 );
	
	if( CFG_GetInt( "xbnbt_dynstat_savemode", -1 ) < 0 )
		CFG_SetInt( "xbnbt_dynstat_savemode", 0 );
	
	if( CFG_GetInt( "xbnbt_dynstat_output_type", -1 ) < 0 )
		CFG_SetInt( "xbnbt_dynstat_output_type", 0 );

	if( CFG_GetInt( "xbnbt_dynstat_output_type", 3 ) > 2 )
		CFG_SetInt( "xbnbt_dynstat_output_type", 0 );

	if( CFG_GetInt( "xbnbt_dynstat_interval", -1 ) < 0 )
		CFG_SetInt( "xbnbt_dynstat_interval", 10 );

	if( CFG_GetInt( "xbnbt_dynstat_background", -1 ) < 0 )
		CFG_SetInt( "xbnbt_dynstat_background", 0 );

	if( CFG_GetInt( "xbnbt_dynstat_background", 256 ) > 255 )
		CFG_SetInt( "xbnbt_dynstat_background", 255 );

	if( CFG_GetInt( "xbnbt_dynstat_x_size", -1 ) < 0 )
		CFG_SetInt( "xbnbt_dynstat_x_size", 350 );

	if( CFG_GetInt( "xbnbt_dynstat_y_size", -1 ) < 0 )
		CFG_SetInt( "xbnbt_dynstat_y_size", 80 );

	if( CFG_GetInt( "xbnbt_dynstat_font_red", -1 ) < 0 )
		CFG_SetInt( "xbnbt_dynstat_font_red", 255 );

	if( CFG_GetInt( "xbnbt_dynstat_font_red", 256 ) > 255 )
		CFG_SetInt( "xbnbt_dynstat_font_red", 255 );

	if( CFG_GetInt( "xbnbt_dynstat_font_green", -1 ) < 0 )
		CFG_SetInt( "xbnbt_dynstat_font_green", 255 );

	if( CFG_GetInt( "xbnbt_dynstat_font_green", 256 ) > 255 )
		CFG_SetInt( "xbnbt_dynstat_font_green", 255 );

	if( CFG_GetInt( "xbnbt_dynstat_font_blue", -1 ) < 0 )
		CFG_SetInt( "xbnbt_dynstat_font_blue", 255 );

	if( CFG_GetInt( "xbnbt_dynstat_font_blue", 256 ) > 255 )
		CFG_SetInt( "xbnbt_dynstat_font_blue", 255 );

	if( CFG_GetInt( "xbnbt_dynstat_png_compress", -2 ) < -1 )
		CFG_SetInt( "xbnbt_dynstat_png_compress", -1 );

	if( CFG_GetInt( "xbnbt_dynstat_png_compress", 10 ) > 9 )
		CFG_SetInt( "xbnbt_dynstat_png_compress", 9 );

	if( CFG_GetInt( "xbnbt_dynstat_jpg_quality", -2 ) < -1 )
		CFG_SetInt( "xbnbt_dynstat_jpg_quality", -1 );

	if( CFG_GetInt( "xbnbt_dynstat_jpg_quality", 96 ) > 95 )
		CFG_SetInt( "xbnbt_dynstat_jpg_quality", 95 );

	if( CFG_GetString( "xbnbt_dynstat_dir", string( ) ).empty( ) )
		CFG_SetString( "xbnbt_dynstat_dir", string( ) );

	if( CFG_GetString( "xbnbt_dynstat_font", string( ) ).empty( ) )
		CFG_SetString( "xbnbt_dynstat_font", string( ) );

	if( CFG_GetString( "xbnbt_dynstat_link", string( ) ).empty( ) )
		CFG_SetString( "xbnbt_dynstat_link", string( ) );

	if( CFG_GetInt( "xbnbt_dynstat_showlink", -1 ) < 0 )
		CFG_SetInt( "xbnbt_dynstat_showlink", 0 );

	if( CFG_GetString( "xbnbt_dynstat_skinfile", string( ) ).empty( ) )
		CFG_SetString( "xbnbt_dynstat_skinfile", string( ) );

	if( CFG_GetInt( "xbnbt_dynstat_skin", -1 ) < 0 )
		CFG_SetInt( "xbnbt_dynstat_skin", 0 );

	// XBNBT MySQL Users mysql

	if( CFG_GetString( "xbnbt_mysqlusers_host", string( ) ).empty( ) )
	{
		const string cstrTemp( CFG_GetString( "xbnbt_ipb_host", string( ) ) );

		if( cstrTemp.empty( ) )
			CFG_SetString( "xbnbt_mysqlusers_host", "localhost" );
		else
		{
			UTIL_LogPrint( "config - migrating xbnbt_ipb_host to xbnbt_mysqlusers_host\n" );

			CFG_SetString( "xbnbt_mysqlusers_host", cstrTemp );
		}

		if( !cstrTemp.empty( ) )
		{
			UTIL_LogPrint( "config - deleteing xbnbt_ipb_host\n" );
			CFG_Delete( "xbnbt_ipb_host" );
		}
	}

	if( CFG_GetString( "xbnbt_mysqlusers_database", string( ) ).empty( ) )
	{
		const string cstrTemp( CFG_GetString( "xbnbt_ipb_database", string( ) ) );

		if( cstrTemp.empty( ) )
			CFG_SetString( "xbnbt_mysqlusers_database", "forum" );
		else
		{
			UTIL_LogPrint( "config - migrating xbnbt_ipb_database to xbnbt_mysqlusers_database\n" );

			CFG_SetString( "xbnbt_mysqlusers_database", cstrTemp );
		}

		if( !cstrTemp.empty( ) )
		{
			UTIL_LogPrint( "config - deleteing xbnbt_ipb_database\n" );
			CFG_Delete( "xbnbt_ipb_database" );
		}
	}

	if( CFG_GetString( "xbnbt_mysqlusers_user", string( ) ).empty( ) )
	{
		const string cstrTemp( CFG_GetString( "xbnbt_ipb_user", string( ) ) );

		if( cstrTemp.empty( ) )
			CFG_SetString( "xbnbt_mysqlusers_user", string( ) );
		else
		{
			UTIL_LogPrint( "config - migrating xbnbt_ipb_user to xbnbt_mysqlusers_user\n" );

			CFG_SetString( "xbnbt_mysqlusers_user", cstrTemp );
		}

		if( !cstrTemp.empty( ) )
		{
			UTIL_LogPrint( "config - deleteing xbnbt_ipb_user\n" );
			CFG_Delete( "xbnbt_ipb_user" );
		}
	}

	if( CFG_GetString( "xbnbt_mysqlusers_password", string( ) ).empty( ) )
	{
		const string cstrTemp( CFG_GetString( "xbnbt_ipb_password", string( ) ) );

		if( cstrTemp.empty( ) )
			CFG_SetString( "xbnbt_mysqlusers_password", string( ) );
		else
		{
			UTIL_LogPrint( "config - migrating xbnbt_ipb_password to xbnbt_mysqlusers_password\n" );

			CFG_SetString( "xbnbt_mysqlusers_password", cstrTemp );
		}

		if( !cstrTemp.empty( ) )
		{
			UTIL_LogPrint( "config - deleteing xbnbt_ipb_password\n" );
			CFG_Delete( "xbnbt_ipb_password" );
		}
	}

	if( CFG_GetInt( "xbnbt_mysqlusers_port", -1) < 0 )
	{
		const int ciTemp( CFG_GetInt( "xbnbt_ipb_port", -1 ) );

		if( ciTemp < 0 )
			CFG_SetInt( "xbnbt_mysqlusers_port", 3306 );
		else
		{
			UTIL_LogPrint( "config - migrating xbnbt_ipb_port to xbnbt_mysqlusers_port\n" );

			CFG_SetInt( "xbnbt_mysqlusers_port", ciTemp );
		}

		if( ciTemp > -1 )
		{
			UTIL_LogPrint( "config - deleteing xbnbt_ipb_port\n" );
			CFG_Delete( "xbnbt_ipb_port" );
		}
	}

	if( CFG_GetInt( "xbnbt_mysqlusers_interval", -1) < 0 )
	{
		const int ciTemp( CFG_GetInt( "xbnbt_ipb_interval", -1 ) );

		if( ciTemp < 0 )
			CFG_SetInt( "xbnbt_mysqlusers_interval", 10 );
		else
		{
			UTIL_LogPrint( "config - migrating xbnbt_ipb_interval to xbnbt_mysqlusers_interval\n" );

			CFG_SetInt( "xbnbt_mysqlusers_interval", ciTemp );
		}

		if( ciTemp > -1 )
		{
			UTIL_LogPrint( "config - deleteing xbnbt_ipb_interval\n" );
			CFG_Delete( "xbnbt_ipb_interval" );
		}
	}

	if( CFG_GetInt( "xbnbt_mysqlusers_override_users", -1) < 0 )
	{
		const int ciTemp( CFG_GetInt( "xbnbt_ipb_override_users", -1 ) );

		if( ciTemp < 0 )
			CFG_SetInt( "xbnbt_mysqlusers_override_users", 0 );
		else
		{
			UTIL_LogPrint( "config - migrating xbnbt_ipb_override_users to xbnbt_mysqlusers_override_users\n" );

			CFG_SetInt( "xbnbt_mysqlusers_override_users", ciTemp );
		}

		if( ciTemp > -1 )
		{
			UTIL_LogPrint( "config - deleteing xbnbt_ipb_override_users\n" );
			CFG_Delete( "xbnbt_ipb_override_users" );
		}
	}

	if( CFG_GetString( "xbnbt_mysqlusers_forums_link", string( ) ).empty( ) )
	{
		const string cstrTemp( CFG_GetString( "xbnbt_ipb_forums_link", string( ) ) );

		if( cstrTemp.empty( ) )
			CFG_SetString( "xbnbt_mysqlusers_forums_link", "http://localhost/forums/index.php?act=Reg" );
		else
		{
			UTIL_LogPrint( "config - migrating xbnbt_ipb_forums_link to xbnbt_mysqlusers_forums_link\n" );

			CFG_SetString( "xbnbt_mysqlusers_forums_link", cstrTemp );
		}

		if( !cstrTemp.empty( ) )
		{
			UTIL_LogPrint( "config - deleteing xbnbt_ipb_forums_link\n" );
			CFG_Delete( "xbnbt_ipb_forums_link" );
		}
	}
	
	if( CFG_GetString( "xbnbt_mysqlusers_table", string( ) ).empty( ) )
	{
		const string cstrTemp( CFG_GetString( "xbnbt_ipb_forums_prefix", string( ) ) );
		
		if( cstrTemp.empty( ) )
			CFG_SetString( "xbnbt_mysqlusers_table", "ibf_members" );
		else
		{
			UTIL_LogPrint( "config - migrating xbnbt_ipb_forums_prefix to xbnbt_mysqlusers_table\n" );

			CFG_SetString( "xbnbt_mysqlusers_table", cstrTemp );
		}

		if( !cstrTemp.empty( ) )
		{
			UTIL_LogPrint( "config - deleteing xbnbt_ipb_forums_prefix\n" );
			CFG_Delete( "xbnbt_ipb_forums_prefix" );
		}
	}

	if( CFG_GetString( "xbnbt_mysqlusers_forums_admin", string( ) ).empty( ) )
	{
		const string cstrTemp( CFG_GetString( "xbnbt_ipb_forums_admin", string( ) ) );

		if( cstrTemp.empty( ) )
			CFG_SetString( "xbnbt_mysqlusers_forums_admin", "admin" );
		else
		{
			UTIL_LogPrint( "config - migrating xbnbt_ipb_forums_admin to xbnbt_mysqlusers_forums_admin\n" );

			CFG_SetString( "xbnbt_mysqlusers_forums_admin", cstrTemp );
		}

		if( !cstrTemp.empty( ) )
		{
			UTIL_LogPrint( "config - deleteing xbnbt_ipb_forums_admin\n" );
			CFG_Delete( "xbnbt_ipb_forums_admin" );
		}
	}

	if( CFG_GetString( "xbnbt_mysqlusers_table_id", string( ) ).empty( ) )
		CFG_SetString( "xbnbt_mysqlusers_table_id", "id" );

	if( CFG_GetString( "xbnbt_mysqlusers_table_name", string( ) ).empty( ) )
		CFG_SetString( "xbnbt_mysqlusers_table_name", "name" );

	if( CFG_GetString( "xbnbt_mysqlusers_table_group", string( ) ).empty( ) )
		CFG_SetString( "xbnbt_mysqlusers_table_group", "mgroup" );

	if( CFG_GetString( "xbnbt_mysqlusers_table_password", string( ) ).empty( ) )
		CFG_SetString( "xbnbt_mysqlusers_table_password", "password" );

	if( CFG_GetString( "xbnbt_mysqlusers_table_email", string( ) ).empty( ) )
		CFG_SetString( "xbnbt_mysqlusers_table_email", "email" );

	if( CFG_GetString( "xbnbt_mysqlusers_table_joined", string( ) ).empty( ) )
		CFG_SetString( "xbnbt_mysqlusers_table_joined", "joined" );

	if( CFG_GetString( "xbnbt_mysqlusers_ignore_group1", string( ) ).empty( ) )
		CFG_SetString( "xbnbt_mysqlusers_ignore_group1", "1" );

	if( CFG_GetString( "xbnbt_mysqlusers_ignore_group2", string( ) ).empty( ) )
		CFG_SetString( "xbnbt_mysqlusers_ignore_group2", "2" );

	if( CFG_GetString( "xbnbt_mysqlusers_ignore_group3", string( ) ).empty( ) )
		CFG_SetString( "xbnbt_mysqlusers_ignore_group3", "5" );

	if( CFG_GetInt( "xbnbt_mysqlusers_mode", -1 ) < 0 )
		CFG_SetInt( "xbnbt_mysqlusers_mode", 0 );

	// XBNBT Specific

	// Validators

	if( CFG_GetInt( "bnbt_show_validator", -1) < 0 )
	{
		const char ccTemp( (char)CFG_GetInt( "xbnbt_show_validator", -1 ) );

		if( ccTemp < 1 )
			CFG_SetInt( "bnbt_show_validator", 0 );
		else
		{
			UTIL_LogPrint( "config - migrating xbnbt_show_validator to bnbt_show_validator\n" );

			CFG_SetInt( "bnbt_show_validator", ccTemp );
		}

		if( ccTemp != -1 )
		{
			UTIL_LogPrint( "config - deleteing xbnbt_show_validator\n" );
			CFG_Delete( "xbnbt_show_validator" );
		}
	}

	if( CFG_GetString( "xbnbt_rss_valid_image", string( ) ).empty( ) )
		CFG_SetString( "xbnbt_rss_valid_image", string( ) );

	// Serve local

	if( CFG_GetInt( "bnbt_allow_serve_local", -1) < 0 )
	{
		const char ccTemp( (char)CFG_GetInt( "xbnbt_serve_local", -1 ) );

		if( ccTemp < 1 )
			CFG_SetInt( "bnbt_allow_serve_local", 0 );
		else
		{
			UTIL_LogPrint( "config - migrating xbnbt_serve_local to bnbt_allow_serve_local\n" );

			CFG_SetInt( "bnbt_allow_serve_local", ccTemp );
		}

		if( ccTemp != -1 )
		{
			UTIL_LogPrint( "config - deleteing xbnbt_serve_local\n" );
			CFG_Delete( "xbnbt_serve_local" );
		}
	}

	// Image bars

	if( CFG_GetString( "image_bar_url", string( ) ).empty( ) )
	{
		const string cstrTemp( CFG_GetString( "xbnbt_image_bar_url", string( ) ) );

		if( cstrTemp.empty( ) )
			CFG_SetString( "image_bar_url", string( ) );
		else
		{
			UTIL_LogPrint( "config - migrating xbnbt_image_bar_url to image_bar_url\n" );

			CFG_SetString( "image_bar_url", cstrTemp );
		}

		if( !cstrTemp.empty( ) )
		{
			UTIL_LogPrint( "config - deleteing xbnbt_image_bar_url\n" );
			CFG_Delete( "xbnbt_image_bar_url" );
		}
	}

	if( CFG_GetString( "image_bar_dir", string( ) ).empty( ) )
	{
		const string cstrTemp( CFG_GetString( "xbnbt_image_bar_dir", string( ) ) );

		if( cstrTemp.empty( ) )
			CFG_SetString( "image_bar_dir", string( ) );
		else
		{
			UTIL_LogPrint( "config - migrating xbnbt_image_bar_dir to image_bar_dir\n" );

			CFG_SetString( "image_bar_dir", cstrTemp );
		}

		if( !cstrTemp.empty( ) )
		{
			UTIL_LogPrint( "config - deleteing xbnbt_image_bar_dir\n" );
			CFG_Delete( "xbnbt_image_bar_dir" );
		}
	}

	// CSS Style sheet

	if( CFG_GetString( "bnbt_style_sheet_url", string( ) ).empty( ) )
	{
		const string cstrTemp( CFG_GetString( "xbnbt_style_sheet_url", string( ) ) );

		if( cstrTemp.empty( ) )
			CFG_SetString( "bnbt_style_sheet_url", string( ) );
		else
		{
			UTIL_LogPrint( "config - migrating xbnbt_style_sheet_url to bnbt_style_sheet_url\n" );

			CFG_SetString( "bnbt_style_sheet_url", cstrTemp );
		}

		if( !cstrTemp.empty( ) )
		{
			UTIL_LogPrint( "config - deleteing xbnbt_style_sheet_url\n" );
			CFG_Delete( "xbnbt_style_sheet_url" );
		}
	}

	if( CFG_GetString( "bnbt_style_sheet_dir", string( ) ).empty( ) )
	{
		const string cstrTemp( CFG_GetString( "xbnbt_style_sheet_dir", string( ) ) );

		if( cstrTemp.empty( ) )
			CFG_SetString( "bnbt_style_sheet_dir", string( ) );
		else
		{
			UTIL_LogPrint( "config - migrating xbnbt_style_sheet_dir to bnbt_style_sheet_dir\n" );

			CFG_SetString( "bnbt_style_sheet_dir", cstrTemp );
		}

		if( !cstrTemp.empty( ) )
		{
			UTIL_LogPrint( "config - deleteing xbnbt_style_sheet_dir\n" );
			CFG_Delete( "xbnbt_style_sheet_dir" );
		}
	}

	// RSS

	if( CFG_GetString( "bnbt_rss_directory", string( ) ).empty( ) )
	{
		const string cstrTemp( CFG_GetString( "xbnbt_rss_directory", string( ) ) );

		if( cstrTemp.empty( ) )
			CFG_SetString( "bnbt_rss_directory", string( ) );
		else
		{
			UTIL_LogPrint( "config - migrating xbnbt_rss_directory to bnbt_rss_directory\n" );

			CFG_SetString( "bnbt_rss_directory", cstrTemp );
		}

		if( !cstrTemp.empty( ) )
		{
			UTIL_LogPrint( "config - deleteing xbnbt_rss_directory\n" );
			CFG_Delete( "xbnbt_rss_directory" );
		}
	}

	if( CFG_GetString( "bnbt_rss_channel_managingeditor", string( ) ).empty( ) )
		CFG_SetString( "bnbt_rss_channel_managingeditor", string( ) );

	// Stats

	if( CFG_GetString( "bnbt_stats_file", string( ) ).empty( ) )
	{
		const string cstrTemp( CFG_GetString( "xbnbt_stats_file", string( ) ) );

		if( cstrTemp.empty( ) )
			CFG_SetString( "bnbt_stats_file", string( ) );
		else
		{
			UTIL_LogPrint( "config - migrating xbnbt_stats_file to bnbt_stats_file\n" );

			CFG_SetString( "bnbt_stats_file", cstrTemp );
		}

		if( !cstrTemp.empty( ) )
		{
			UTIL_LogPrint( "config - deleteing xbnbt_stats_file\n" );
			CFG_Delete( "xbnbt_stats_file" );
		}
	}

	if( CFG_GetInt( "bnbt_stats_dump_interval", -1) < 0 )
	{
		const char ccTemp( (char)CFG_GetInt( "xbnbt_dump_stats_interval", -1 ) );

		if( ccTemp < 1 )
			CFG_SetInt( "bnbt_stats_dump_interval", 600 );
		else
		{
			UTIL_LogPrint( "config - migrating xbnbt_dump_stats_interval to bnbt_stats_dump_interval\n" );

			CFG_SetInt( "bnbt_stats_dump_interval", ccTemp );
		}

		if( ccTemp != -1 )
		{
			UTIL_LogPrint( "config - deleteing xbnbt_dump_stats_interval\n" );
			CFG_Delete( "xbnbt_dump_stats_interval" );
		}
	}

	// Authentication

	// Announce and scrape authorisation

	if( CFG_GetInt( "bnbt_announce_access_required", -1) < 0 )
	{
		const char ccAuthAnn( (char)CFG_GetInt( "xbnbt_authenticate_announce", -1 ) );
		const char ccAuthAnnAccess( (char)CFG_GetInt( "xbnbt_authenticate_announce_access", -1 ) );

		if( ccAuthAnn < 1 )
			CFG_SetInt( "bnbt_announce_access_required", 0 );
		else
		{
			if( ccAuthAnnAccess > 0 )
			{
				UTIL_LogPrint( "config - migrating xbnbt_authenticate_announce and xbnbt_authenticate_announce_access to bnbt_announce_access_required\n" );

				CFG_SetInt( "bnbt_announce_access_required", ccAuthAnnAccess );
			}
			else
				CFG_SetInt( "bnbt_announce_access_required", 0 );
		}

		if( ccAuthAnn != -1 )
		{
			UTIL_LogPrint( "config - deleteing xbnbt_authenticate_announce and xbnbt_authenticate_announce_access\n" );
			CFG_Delete( "xbnbt_authenticate_announce" );
			CFG_Delete( "xbnbt_authenticate_announce_access" );
		}
	}

	if( CFG_GetInt( "bnbt_scrape_access_required", -1) < 0 )
	{
		const char ccAuthScrape( (unsigned char)CFG_GetInt( "xbnbt_authenticate_scrape", -1 ) );
		const char ccAuthScrapeAccess( (unsigned char)CFG_GetInt( "xbnbt_authenticate_scrape_access", -1 ) );

		if( ccAuthScrape < 1 )
			CFG_SetInt( "bnbt_scrape_access_required", 0 );
		else
		{
			if( ccAuthScrapeAccess > 0 )
			{
				UTIL_LogPrint( "config - migrating xbnbt_authenticate_scrape and xbnbt_authenticate_scrape_access to bnbt_scrape_access_required\n" );

				CFG_SetInt( "bnbt_scrape_access_required", ccAuthScrapeAccess );
			}
			else
				CFG_SetInt( "bnbt_scrape_access_required", 0 );
		}

		if( ccAuthScrape != -1 )
		{
			UTIL_LogPrint( "config - deleteing xbnbt_authenticate_scrape and xbnbt_authenticate_scrape_access\n" );
			CFG_Delete( "xbnbt_authenticate_scrape" );
			CFG_Delete( "xbnbt_authenticate_scrape_access" );
		}
	}

	// Tracker info authorisation level

	if( CFG_GetInt( "bnbt_info_access_required", -1) < 0 )
	{
		const char ccAuthInfo( (char)CFG_GetInt( "xbnbt_authenticate_info_access", -1 ) );

		if( ccAuthInfo < 1 )
			CFG_SetInt( "bnbt_info_access_required", 0 );
		else
		{
			UTIL_LogPrint( "config - migrating xbnbt_authenticate_info_access to bnbt_info_access_required\n" );

			CFG_SetInt( "bnbt_info_access_required", ccAuthInfo );
		}

		if( ccAuthInfo != -1 )
		{
			UTIL_LogPrint( "config - deleteing xbnbt_authenticate_info_access\n" );
			CFG_Delete( "xbnbt_authenticate_info_access" );
		}
	}

	// Custom scrape and announce

	if( CFG_GetString( "bnbt_custom_announce", string( ) ).empty( ) )
	{
		const string cstrTemp( CFG_GetString( "xbnbt_custom_announce", string( ) ) );

		if( cstrTemp.empty( ) )
		{
			CFG_SetString( "bnbt_custom_announce", string( ) );
			CFG_Delete( "xbnbt_custom_announce" );
		}
		else
		{
			CFG_SetString( "bnbt_custom_announce", cstrTemp );
			CFG_Delete( "xbnbt_custom_announce" );
		}
	}

	if( CFG_GetString( "bnbt_custom_scrape", string( ) ).empty( ) )
	{
		const string cstrTemp( CFG_GetString( "xbnbt_custom_scrape", string( ) ) );

		if( cstrTemp.empty( ) )
		{
			CFG_SetString( "bnbt_custom_scrape", string( ) );
			CFG_Delete( "xbnbt_custom_scrape" );
		}
		else
		{
			CFG_SetString( "bnbt_custom_scrape", cstrTemp );
			CFG_Delete( "xbnbt_custom_scrape" );
		}
	}
	
	// TCP

	if( CFG_GetInt( "xbnbt_so_recbuf", -1 ) < 0 )
		CFG_SetInt( "xbnbt_so_recbuf", 128 );

	if( CFG_GetInt( "xbnbt_so_sndbuf", -1 ) < 0 )
		CFG_SetInt( "xbnbt_so_sndbuf", 128 );

	if( CFG_GetInt( "xbnbt_tcp_nodelay", -1 ) < 0 )
		CFG_SetInt( "xbnbt_tcp_nodelay", 0 );

	// hublink.cpp

	if( CFG_GetInt( "xbnbt_thlink_server", -1 ) < 0 )
		CFG_SetInt( "xbnbt_thlink_server", 0 );

	if( CFG_GetString( "xbnbt_thlink_connect", string( ) ).empty( ) )
		CFG_SetString( "xbnbt_thlink_connect", string( ) );

	if( CFG_GetString( "xbnbt_thlink_password", string( ) ).empty( ) )
		CFG_SetString( "xbnbt_thlink_password", string( ) );

	if( CFG_GetString( "xbnbt_thlink_bind", string( ) ).empty( ) )
		CFG_SetString( "xbnbt_thlink_bind", string( ) );

	if( CFG_GetInt( "xbnbt_thlink_port", 0 ) < 1 )
		CFG_SetInt( "xbnbt_thlink_port", 5205 );

	// XML
	if( CFG_GetInt( "bnbt_dump_xml_interval", 0 ) < 1 )
		CFG_SetInt( "bnbt_dump_xml_interval", 600 );

	if( CFG_GetString( "bnbt_dump_xml_file", string( ) ).empty( ) )
		CFG_SetString( "bnbt_dump_xml_file", string( ) );

	if( CFG_GetString( "bnbt_dump_xml_dir", string( ) ).empty( ) )
		CFG_SetString( "bnbt_dump_xml_dir", string( ) );

	if( CFG_GetString( "bnbt_dump_xml_url", string( ) ).empty( ) )
		CFG_SetString( "bnbt_dump_xml_url", string( ) );

	if( CFG_GetInt( "bnbt_dump_xml_peers", -1 ) < 0 )
		CFG_SetInt( "bnbt_dump_xml_peers", 1 );


	// Fully qualified domain name

	if( CFG_GetString( "bnbt_tracker_fqdn", string( ) ).empty( ) )
	{
		const string cstrTemp( CFG_GetString( "xbnbt_tracker_fqdn", string( ) ) );

		if( cstrTemp.empty( ) )
			CFG_SetString( "bnbt_tracker_fqdn", string( ) );
		else
		{
			UTIL_LogPrint( "config - migrating xbnbt_tracker_fqdn to bnbt_tracker_fqdn\n" );

			CFG_SetString( "bnbt_tracker_fqdn", cstrTemp );
		}

		if( !cstrTemp.empty( ) )
		{
			UTIL_LogPrint( "config - deleteing xbnbt_tracker_fqdn\n" );
			CFG_Delete( "xbnbt_tracker_fqdn" );
		}
	}

	// Tracker description

	if( CFG_GetString( "bnbt_tracker_description", string( ) ).empty( ) )
	{
		const string cstrTemp( CFG_GetString( "xbnbt_tracker_description", string( ) ) );

		if( cstrTemp.empty( ) )
			CFG_SetString( "bnbt_tracker_description", "Powered by XBNBT" );
		else
		{
			UTIL_LogPrint( "config - migrating xbnbt_tracker_description to bnbt_tracker_description\n" );

			CFG_SetString( "bnbt_tracker_description", cstrTemp );
		}

		if( !cstrTemp.empty( ) )
		{
			UTIL_LogPrint( "config - deleteing xbnbt_tracker_description\n" );
			CFG_Delete( "xbnbt_tracker_description" );
		}
	}

	// Tracker title

	if( CFG_GetString( "bnbt_tracker_title", string( ) ).empty( ) )
	{
		const string cstrTemp( CFG_GetString( "xbnbt_tracker_title", string( ) ) );

		if( cstrTemp.empty( ) )
			CFG_SetString( "bnbt_tracker_title", "My XBNBT Tracker" );
		else
		{
			UTIL_LogPrint( "config - migrating xbnbt_tracker_title to bnbt_tracker_title\n" );

			CFG_SetString( "bnbt_tracker_title", cstrTemp );
		}

		if( !cstrTemp.empty( ) )
		{
			UTIL_LogPrint( "config - deleteing xbnbt_tracker_title\n" );
			CFG_Delete( "xbnbt_tracker_title" );
		}
	}

	// Keywords

	if( CFG_GetString( "bnbt_tracker_keywords", string( ) ).empty( ) )
	{
		const string cstrTemp( CFG_GetString( "xbnbt_tracker_keywords", string( ) ) );

		if( cstrTemp.empty( ) )
			CFG_SetString( "bnbt_tracker_keywords", "bittorrent, torrent, tracker, XBNBT" );
		else
		{
			UTIL_LogPrint( "config - migrating xbnbt_tracker_keywords to bnbt_tracker_keywords\n" );

			CFG_SetString( "bnbt_tracker_keywords", cstrTemp );
		}

		if( !cstrTemp.empty( ) )
		{
			UTIL_LogPrint( "config - deleteing xbnbt_tracker_keywords\n" );
			CFG_Delete( "xbnbt_tracker_keywords" );
		}
	}

	// Allow header and footer on all pages

	if( CFG_GetInt( "bnbt_show_header_footer", -1) < 0 )
	{
		const char ccTemp( (char)CFG_GetInt( "xbnbt_allow_header_footer", -1 ) );

		if( ccTemp < 1 )
			CFG_SetInt( "bnbt_show_header_footer", 0 );
		else
		{
			UTIL_LogPrint( "config - migrating xbnbt_allow_header_footer to bnbt_show_header_footer\n" );

			CFG_SetInt( "bnbt_show_header_footer", ccTemp );
		}

		if( ccTemp != -1 )
		{
			UTIL_LogPrint( "config - deleteing xbnbt_allow_header_footer\n" );
			CFG_Delete( "xbnbt_allow_header_footer" );
		}
	}

	// Allow uploader/modifier IP to be displayed to admin

	if( CFG_GetInt( "bnbt_show_uploader_ip", -1) < 0 )
	{
		const char ccTemp( (char)CFG_GetInt( "xbnbt_show_uploader_ip", -1 ) );

		if( ccTemp < 1 )
			CFG_SetInt( "bnbt_show_uploader_ip", 0 );
		else
		{
			UTIL_LogPrint( "config - migrating xbnbt_show_uploader_ip to bnbt_show_uploader_ip\n" );

			CFG_SetInt( "bnbt_show_uploader_ip", ccTemp );
		}

		if( ccTemp != -1 )
		{
			UTIL_LogPrint( "config - deleteing xbnbt_show_uploader_ip\n" );
			CFG_Delete( "xbnbt_show_uploader_ip" );
		}
	}

	// Currently connected count on index.html

	if( CFG_GetInt( "xbnbt_users_online", -1 ) < 0 )
		CFG_SetInt( "xbnbt_users_online", 0 );

	// Navbar

	if( CFG_GetInt( "bnbt_show_navbar", -1) < 0 )
	{
		const char ccTemp( (char)CFG_GetInt( "xbnbt_show_navbar", -1 ) );

		if( ccTemp < 1 )
			CFG_SetInt( "bnbt_show_navbar", 0 );
		else
		{
			UTIL_LogPrint( "config - migrating xbnbt_show_navbar to bnbt_show_navbar\n" );

			CFG_SetInt( "bnbt_show_navbar", ccTemp );
		}

		if( ccTemp != -1 )
		{
			UTIL_LogPrint( "config - deleteing xbnbt_show_navbar\n" );
			CFG_Delete( "xbnbt_show_navbar" );
		}
	}

	// Peer info on stats.html

	if( CFG_GetInt( "bnbt_show_peer_info", -1) < 0 )
	{
		const char ccTemp( (char)CFG_GetInt( "xbnbt_show_peer_info", -1 ) );

		if( ccTemp < 1 )
			CFG_SetInt( "bnbt_show_peer_info", 0 );
		else
		{
			UTIL_LogPrint( "config - migrating xbnbt_show_peer_info to bnbt_show_peer_info\n" );

			CFG_SetInt( "bnbt_show_peer_info", ccTemp );
		}

		if( ccTemp != -1 )
		{
			UTIL_LogPrint( "config - deleteing xbnbt_show_peer_info\n" );
			CFG_Delete( "xbnbt_show_peer_info" );
		}
	}

    // Enable torrent announce list

	if( CFG_GetInt( "bnbt_announce_list_enable", -1) < 0 )
	{
		const char ccTemp( (char)CFG_GetInt( "xbnbt_announce_list_enable", -1 ) );

		if( ccTemp < 1 )
			CFG_SetInt( "bnbt_announce_list_enable", 0 );
		else
		{
			UTIL_LogPrint( "config - migrating xbnbt_announce_list_enable to bnbt_announce_list_enable\n" );

			CFG_SetInt( "bnbt_announce_list_enable", ccTemp );
		}

		if( ccTemp != -1 )
		{
			UTIL_LogPrint( "config - deleteing xbnbt_announce_list_enable\n" );
			CFG_Delete( "xbnbt_announce_list_enable" );
		}
	}

    // Force announce on upload

	if( CFG_GetInt( "bnbt_force_announce_on_upload", -1) < 0 )
	{
		const char ccTemp( (char)CFG_GetInt( "xbnbt_force_announce_on_upload", -1 ) );

		if( ccTemp < 1 )
			CFG_SetInt( "bnbt_force_announce_on_upload", 0 );
		else
		{
			UTIL_LogPrint( "config - migrating xbnbt_force_announce_on_upload to bnbt_force_announce_on_upload\n" );

			CFG_SetInt( "bnbt_force_announce_on_upload", ccTemp );
		}

		if( ccTemp != -1 )
		{
			UTIL_LogPrint( "config - deleteing xbnbt_force_announce_on_upload\n" );
			CFG_Delete( "xbnbt_force_announce_on_upload" );
		}
	}

	// Process ID (PID) file
	if( CFG_GetString( "bnbt_pid_file", string( ) ).empty( ) )
		CFG_SetString( "bnbt_pid_file", string( ) );	

	// Debug level

	if( CFG_GetInt( "bnbt_debug_level", -1) < 0 )
	{
		const char ccTemp( (char)CFG_GetInt( "xbnbt_debug_level", -1 ) );

		if( ccTemp < 1 )
			CFG_SetInt( "bnbt_debug_level", 0 );
		else
		{
			UTIL_LogPrint( "config - migrating xbnbt_debug_level to bnbt_debug_level\n" );

			CFG_SetInt( "bnbt_debug_level", ccTemp );
		}

		if( ccTemp != -1 )
		{
			UTIL_LogPrint( "config - deleteing xbnbt_debug_level\n" );
			CFG_Delete( "xbnbt_debug_level" );
		}
	}

	// Use buttons
	if( CFG_GetInt( "xbnbt_use_buttons", -1 ) < 0 )
		CFG_SetInt( "xbnbt_use_buttons", 0 );	

	// Announce 'key' support
	if( CFG_GetInt( "bnbt_use_announce_key", -1 ) < 0 )
		CFG_SetInt( "bnbt_use_announce_key", 1 );	

	// BNBT language code
	if( CFG_GetString( "bnbt_language", string( ) ).empty( ) )
		CFG_SetString( "bnbt_language", "en" );

	// Rating
	if( CFG_GetString( "bnbt_rating", string( ) ).empty( ) )
		CFG_SetString( "bnbt_rating", string( ) );

	// webmaster
	if( CFG_GetString( "bnbt_webmaster", string( ) ).empty( ) )
		CFG_SetString( "bnbt_webmaster", string( ) );

	// make public
	if( CFG_GetInt( "bnbt_public_option", -1 ) < 0 )
		CFG_SetInt( "bnbt_public_option", 0 );	

	// public upload directory
	if( CFG_GetString( "bnbt_public_upload_dir", string( ) ).empty( ) )
		CFG_SetString( "bnbt_public_upload_dir", string( ) );
}

/***
*
* XBNBT Beta 81b.3.5 - A C++ BitTorrent Tracker
* Copyright (C) 2003-2005 =Xotic=
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

map<string, string> gmapLANG_CFG;

void LANG_CFG_Init( const char *szFile )
{
	LANG_CFG_Open( szFile );
	LANG_CFG_SetDefaults( );
	LANG_CFG_Close( szFile );
}

void LANG_CFG_Open( const char *szFile )
{
	gmapLANG_CFG.clear( );

	ifstream in;

	in.open( szFile );

	if( in.fail( ) )
	{
		UTIL_LogPrint( "XBNBT language warning - unable to open %s for reading\n", szFile );

		return;
	}

	char pBuf[1024];

	string strTemp = string( );

	string :: size_type iSplit = 0;

	string :: size_type iKeyStart = 0;
	string :: size_type iKeyEnd = 0;
	string :: size_type iValueStart = 0;
	string :: size_type iValueEnd = 0;

	while( !in.eof( ) )
	{
		memset( pBuf, 0, sizeof(pBuf) / sizeof(char) );

		in.getline( pBuf, sizeof(pBuf) - 1 );

		strTemp = pBuf;

		// ignore blank lines and comments

		if( strTemp.empty( ) || strTemp[0] == '#' )
			continue;

		iSplit = strTemp.find( "=" );

		if( iSplit == string :: npos )
			continue;

		iKeyStart = strTemp.find_first_not_of( " " );
		iKeyEnd = strTemp.find( " ", iKeyStart );
		iValueStart = strTemp.find_first_not_of( " ", iSplit + 1 );
		iValueEnd = strTemp.size( );

		if( iValueStart != string :: npos )
			gmapLANG_CFG[strTemp.substr( iKeyStart, iKeyEnd - iKeyStart )] = strTemp.substr( iValueStart, iValueEnd - iValueStart );
	}

	in.close( );
}

void LANG_CFG_SetString( const string &cstrKey, const string &csX )
{
	gmapLANG_CFG[cstrKey] = csX;
}

const string LANG_CFG_GetString( const string &cstrKey, const string &csX )
{
	if( gmapLANG_CFG.find( cstrKey ) == gmapLANG_CFG.end( ) )
		return csX;
	else
		return gmapLANG_CFG[cstrKey];
}

void LANG_CFG_Delete( const string &cstrKey )
{
	gmapLANG_CFG.erase( cstrKey );
}

void LANG_CFG_Close( const char *szFile )
{
	ofstream out;

	out.open( szFile );

	if( out.fail( ) )
	{
		UTIL_LogPrint( "XBNBT language warning - unable to open %s for writing\n", szFile );

		return;
	}

	for( map<string, string> :: iterator it = gmapLANG_CFG.begin( ); it != gmapLANG_CFG.end( ); it++ )
		out << (*it).first.c_str( ) << " = " << (*it).second.c_str( ) << endl;

	out.close( );
}

void LANG_CFG_SetDefaultsXbnbt( )
{
	// common
	if( LANG_CFG_GetString( "view_not_authorized", string( ) ).empty( ) )
		LANG_CFG_SetString( "view_not_authorized", "You are not authorized to view this page." );

	if( LANG_CFG_GetString( "view_forbidden", string( ) ).empty( ) )
		LANG_CFG_SetString( "view_forbidden", "You are forbidden to view this page." );

	if( LANG_CFG_GetString( "name", string( ) ).empty( ) )
		LANG_CFG_SetString( "name", "Name" );

	if( LANG_CFG_GetString( "filename", string( ) ).empty( ) )
		LANG_CFG_SetString( "filename", "File Name" );

	if( LANG_CFG_GetString( "info_hash", string( ) ).empty( ) )
		LANG_CFG_SetString( "info_hash", "Info Hash" );

	if( LANG_CFG_GetString( "added", string( ) ).empty( ) )
		LANG_CFG_SetString( "added", "Added" );

	if( LANG_CFG_GetString( "size", string( ) ).empty( ) )
		LANG_CFG_SetString( "size", "Size" );

	if( LANG_CFG_GetString( "files", string( ) ).empty( ) )
		LANG_CFG_SetString( "files", "Files" );

	if( LANG_CFG_GetString( "file_comment", string( ) ).empty( ) )
		LANG_CFG_SetString( "file_comment", "File Comment" );

	if( LANG_CFG_GetString( "delete_all", string( ) ).empty( ) )
		LANG_CFG_SetString( "delete_all", "Delete All" );

	if( LANG_CFG_GetString( "delete", string( ) ).empty( ) )
		LANG_CFG_SetString( "delete", "Delete" );

	if( LANG_CFG_GetString( "yes", string( ) ).empty( ) )
		LANG_CFG_SetString( "yes", "YES" );

	if( LANG_CFG_GetString( "no", string( ) ).empty( ) )
		LANG_CFG_SetString( "no", "NO" );

	if( LANG_CFG_GetString( "no_html", string( ) ).empty( ) )
		LANG_CFG_SetString( "no_html", "No HTML" );

	if( LANG_CFG_GetString( "tag", string( ) ).empty( ) )
		LANG_CFG_SetString( "tag", "Tag" );

	if( LANG_CFG_GetString( "torrent", string( ) ).empty( ) )
		LANG_CFG_SetString( "torrent", "Torrent" );

	if( LANG_CFG_GetString( "seeders", string( ) ).empty( ) )
		LANG_CFG_SetString( "seeders", "SDs" );

	if( LANG_CFG_GetString( "leechers", string( ) ).empty( ) )
		LANG_CFG_SetString( "leechers", "DLs" );

	if( LANG_CFG_GetString( "completed", string( ) ).empty( ) )
		LANG_CFG_SetString( "completed", "Completed" );

	if( LANG_CFG_GetString( "transferred", string( ) ).empty( ) )
		LANG_CFG_SetString( "transferred", "Transferred" );

	if( LANG_CFG_GetString( "min_progress", string( ) ).empty( ) )
		LANG_CFG_SetString( "min_progress", "Min Progress" );

	if( LANG_CFG_GetString( "min_left", string( ) ).empty( ) )
		LANG_CFG_SetString( "min_left", "Min Left" );

	if( LANG_CFG_GetString( "avg_progress", string( ) ).empty( ) )
		LANG_CFG_SetString( "avg_progress", "Average Progress" );

	if( LANG_CFG_GetString( "avg_left", string( ) ).empty( ) )
		LANG_CFG_SetString( "avg_left", "Average Left" );

	if( LANG_CFG_GetString( "max_progress", string( ) ).empty( ) )
		LANG_CFG_SetString( "max_progress", "Max Progress" );

	if( LANG_CFG_GetString( "max_left", string( ) ).empty( ) )
		LANG_CFG_SetString( "max_left", "Max Left" );

	if( LANG_CFG_GetString( "uploader", string( ) ).empty( ) )
		LANG_CFG_SetString( "uploader", "Uploader" );

	if( LANG_CFG_GetString( "ip", string( ) ).empty( ) )
		LANG_CFG_SetString( "ip", "IP" );

	if( LANG_CFG_GetString( "info", string( ) ).empty( ) )
		LANG_CFG_SetString( "info", "Info" );

	if( LANG_CFG_GetString( "admin", string( ) ).empty( ) )
		LANG_CFG_SetString( "admin", "Admin" );

	if( LANG_CFG_GetString( "type", string( ) ).empty( ) )
		LANG_CFG_SetString( "type", "Type" );

	if( LANG_CFG_GetString( "download", string( ) ).empty( ) )
		LANG_CFG_SetString( "download", "DL" );

	if( LANG_CFG_GetString( "na", string( ) ).empty( ) )
		LANG_CFG_SetString( "na", "N/A" );

	if( LANG_CFG_GetString( "remaining", string( ) ).empty( ) )
		LANG_CFG_SetString( "remaining", "Remaining" );

	if( LANG_CFG_GetString( "link", string( ) ).empty( ) )
		LANG_CFG_SetString( "link", "Link" );

	if( LANG_CFG_GetString( "search", string( ) ).empty( ) )
		LANG_CFG_SetString( "search", "Search" );

	// javascript
	if( LANG_CFG_GetString( "js_fill_fields", string( ) ).empty( ) )
		LANG_CFG_SetString( "js_fill_fields", "You must fill in all the fields." );

	if( LANG_CFG_GetString( "js_message_too_long", string( ) ).empty( ) )
		LANG_CFG_SetString( "js_message_too_long", "Your message is too long." );

	if( LANG_CFG_GetString( "js_reduce_characters", string( ) ).empty( ) )
		LANG_CFG_SetString( "js_reduce_characters", "Reduce your message to %s characters." );

	if( LANG_CFG_GetString( "js_message_length", string( ) ).empty( ) )
		LANG_CFG_SetString( "js_message_length", "Your message is %s characters long." );

	if( LANG_CFG_GetString( STR_SUBMIT, string( ) ).empty( ) )
		LANG_CFG_SetString( STR_SUBMIT, STR_SUBMIT );

	if( LANG_CFG_GetString( "js_delete_torrent_q", string( ) ).empty( ) )
		LANG_CFG_SetString( "js_delete_torrent_q", "Are you sure you want to delete the torrent %s?" );

	if( LANG_CFG_GetString( "js_search_results_for", string( ) ).empty( ) )
		LANG_CFG_SetString( "js_search_results_for", "Search results for: %s" );

	if( LANG_CFG_GetString( "js_kill_the_tracker_q", string( ) ).empty( ) )
		LANG_CFG_SetString( "js_kill_the_tracker_q", "WARNING: KILL the tracker?" );

	if( LANG_CFG_GetString( "js_reset_tracker_link_q", string( ) ).empty( ) )
		LANG_CFG_SetString( "js_reset_tracker_link_q", "Reset the tracker link?" );

	if( LANG_CFG_GetString( "js_reset_tracker_hub_link_q", string( ) ).empty( ) )
		LANG_CFG_SetString( "js_reset_tracker_hub_link_q", "Reset the tracker hub link?" );

	if( LANG_CFG_GetString( "js_reset_dynstat_stats_q", string( ) ).empty( ) )
		LANG_CFG_SetString( "js_reset_dynstat_stats_q", "Reset Dynstat Stats?" );

	if( LANG_CFG_GetString( "js_reset_announce_stats_q", string( ) ).empty( ) )
		LANG_CFG_SetString( "js_reset_announce_stats_q", "Reset Announce Stats?" );

	if( LANG_CFG_GetString( "js_reset_scrape_stats_q", string( ) ).empty( ) )
		LANG_CFG_SetString( "js_reset_scrape_stats_q", "Reset Scrape Stats?" );

	if( LANG_CFG_GetString( "js_reset_file_stats_q", string( ) ).empty( ) )
		LANG_CFG_SetString( "js_reset_file_stats_q", "Reset File Stats?" );

	if( LANG_CFG_GetString( "js_reset_page_stats_q", string( ) ).empty( ) )
		LANG_CFG_SetString( "js_reset_page_stats_q", "Reset Page Stats?" );

	if( LANG_CFG_GetString( "js_reset_all_stats_q", string( ) ).empty( ) )
		LANG_CFG_SetString( "js_reset_all_stats_q", "Reset ALL Stats?" );

	// navbar
	if( LANG_CFG_GetString( "navbar_index", string( ) ).empty( ) )
		LANG_CFG_SetString( "navbar_index", "Index" );

	if( LANG_CFG_GetString( "navbar_login", string( ) ).empty( ) )
		LANG_CFG_SetString( "navbar_login", "Login" );

	if( LANG_CFG_GetString( "navbar_my_torrents", string( ) ).empty( ) )
		LANG_CFG_SetString( "navbar_my_torrents", "My Torrents" );

	if( LANG_CFG_GetString( "navbar_upload", string( ) ).empty( ) )
		LANG_CFG_SetString( "navbar_upload", "Upload" );

	if( LANG_CFG_GetString( "navbar_logout", string( ) ).empty( ) )
		LANG_CFG_SetString( "navbar_logout", "Logout" );

	if( LANG_CFG_GetString( "navbar_sign_up", string( ) ).empty( ) )
		LANG_CFG_SetString( "navbar_sign_up", "Sign Up" );

	if( LANG_CFG_GetString( "navbar_rss", string( ) ).empty( ) )
		LANG_CFG_SetString( "navbar_rss", "RSS" );

	if( LANG_CFG_GetString( "navbar_xml", string( ) ).empty( ) )
		LANG_CFG_SetString( "navbar_xml", "XML" );

	if( LANG_CFG_GetString( "navbar_info", string( ) ).empty( ) )
		LANG_CFG_SetString( "navbar_info", "Info" );

	if( LANG_CFG_GetString( "navbar_admin", string( ) ).empty( ) )
		LANG_CFG_SetString( "navbar_admin", "Admin" );

	if( LANG_CFG_GetString( "navbar_users", string( ) ).empty( ) )
		LANG_CFG_SetString( "navbar_users", "Users" );

	if( LANG_CFG_GetString( "navbar_tags", string( ) ).empty( ) )
		LANG_CFG_SetString( "navbar_tags", "Tags" );

	if( LANG_CFG_GetString( "navbar_language", string( ) ).empty( ) )
		LANG_CFG_SetString( "navbar_language", "Language" );

	if( LANG_CFG_GetString( "navbar_xstats", string( ) ).empty( ) )
		LANG_CFG_SetString( "navbar_xstats", "XStats" );

	if( LANG_CFG_GetString( "navbar_xtorrent", string( ) ).empty( ) )
		LANG_CFG_SetString( "navbar_xtorrent", "XTorrent" );

	// header login message
	if( LANG_CFG_GetString( "login1", string( ) ).empty( ) )
		LANG_CFG_SetString( "login1", "You are not logged in. Click %shere%s to login." );

	if( LANG_CFG_GetString( "login2", string( ) ).empty( ) )
		LANG_CFG_SetString( "login2", "You are logged in as %s. Click %shere%s to logout." );

	// redirect
	if( LANG_CFG_GetString( "redirect", string( ) ).empty( ) )
		LANG_CFG_SetString( "redirect", "Click %shere%s if you are not redirected in 10 seconds" );

	// page headers
	if( LANG_CFG_GetString( "admin_page", string( ) ).empty( ) )
		LANG_CFG_SetString( "admin_page", "XBNBT Admin Panel" );

	if( LANG_CFG_GetString( "comments_page", string( ) ).empty( ) )
		LANG_CFG_SetString( "comments_page", "XBNBT Comments" );

	if( LANG_CFG_GetString( "index_page", string( ) ).empty( ) )
		LANG_CFG_SetString( "index_page", "XBNBT Tracker Index" );

	if( LANG_CFG_GetString( "info_page", string( ) ).empty( ) )
		LANG_CFG_SetString( "info_page", "XBNBT Tracker Info" );

	if( LANG_CFG_GetString( "login_page", string( ) ).empty( ) )
		LANG_CFG_SetString( "login_page", "XBNBT Login" );

	if( LANG_CFG_GetString( "signup_page", string( ) ).empty( ) )
		LANG_CFG_SetString( "signup_page", "XBNBT Signup" );

	if( LANG_CFG_GetString( "stats_page", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_page", "XBNBT File Info" );

	if( LANG_CFG_GetString( "upload_page", string( ) ).empty( ) )
		LANG_CFG_SetString( "upload_page", "XBNBT Torrent Uploader" );

	if( LANG_CFG_GetString( "users_page", string( ) ).empty( ) )
		LANG_CFG_SetString( "users_page", "XBNBT Users Info" );
}

void LANG_CFG_SetDefaultsSort( )
{
	// sort
	if( LANG_CFG_GetString( "sort_ascending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_ascending", "A" );

	if( LANG_CFG_GetString( "sort_descending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_descending", "Z" );

	if( LANG_CFG_GetString( "sort_tag_ascending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_tag_ascending", "Sort Tag Ascending" );

	if( LANG_CFG_GetString( "sort_tag_descending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_tag_descending", "Sort Tag Descending" );

	if( LANG_CFG_GetString( "sort_name_ascending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_name_ascending", "Sort Name Ascending" );

	if( LANG_CFG_GetString( "sort_name_descending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_name_descending", "Sort Name Descending" );

	if( LANG_CFG_GetString( "sort_comments_ascending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_comments_ascending", "Sort Comments Ascending" );

	if( LANG_CFG_GetString( "sort_comments_descending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_comments_descending", "Sort Comments Descending" );

	if( LANG_CFG_GetString( "sort_added_ascending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_added_ascending", "Sort Added Ascending" );

	if( LANG_CFG_GetString( "sort_added_descending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_added_descending", "Sort Added Descending" );

	if( LANG_CFG_GetString( "sort_size_ascending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_size_ascending", "Sort Size Ascending" );

	if( LANG_CFG_GetString( "sort_size_descending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_size_descending", "Sort Size Descending" );

	if( LANG_CFG_GetString( "sort_files_ascending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_files_ascending", "Sort Files Ascending" );

	if( LANG_CFG_GetString( "sort_files_descending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_files_descending", "Sort Files Descending" );

	if( LANG_CFG_GetString( "sort_seeders_ascending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_seeders_ascending", "Sort Seeders Ascending" );

	if( LANG_CFG_GetString( "sort_seeders_descending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_seeders_descending", "Sort Seeders Descending" );

	if( LANG_CFG_GetString( "sort_leechers_ascending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_leechers_ascending", "Sort Leechers Ascending" );

	if( LANG_CFG_GetString( "sort_leechers_descending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_leechers_descending", "Sort Leechers Descending" );

	if( LANG_CFG_GetString( "sort_completed_ascending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_completed_ascending", "Sort Completed Ascending" );

	if( LANG_CFG_GetString( "sort_completed_descending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_completed_descending", "Sort Completed Descending" );

	if( LANG_CFG_GetString( "sort_transferred_ascending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_transferred_ascending", "Sort Transferred Ascending" );

	if( LANG_CFG_GetString( "sort_transferred_descending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_transferred_descending", "Sort Transferred Descending" );

	if( LANG_CFG_GetString( "sort_avgleft_ascending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_avgleft_ascending", "Sort Average Left Ascending" );

	if( LANG_CFG_GetString( "sort_avgleft_descending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_avgleft_descending", "Sort Average Left Descending" );

	if( LANG_CFG_GetString( "sort_uploader_ascending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_uploader_ascending", "Sort Uploader Ascending" );

	if( LANG_CFG_GetString( "sort_uploader_descending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_uploader_descending", "Sort Uploader Descending" );

	if( LANG_CFG_GetString( "sort_ip_ascending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_ip_ascending", "Sort IP Ascending" );

	if( LANG_CFG_GetString( "sort_ip_descending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_ip_descending", "Sort IP Descending" );

	if( LANG_CFG_GetString( "sort_client_ascending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_client_ascending", "Sort Client Ascending" );

	if( LANG_CFG_GetString( "sort_client_descending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_client_descending", "Sort Client Descending" );

	if( LANG_CFG_GetString( "sort_uploaded_ascending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_uploaded_ascending", "Sort Uploaded Ascending" );

	if( LANG_CFG_GetString( "sort_uploaded_descending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_uploaded_descending", "Sort Uploaded Descending" );

	if( LANG_CFG_GetString( "sort_downloaded_ascending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_downloaded_ascending", "Sort Downloaded Ascending" );

	if( LANG_CFG_GetString( "sort_downloaded_descending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_downloaded_descending", "Sort Downloaded Descending" );

	if( LANG_CFG_GetString( "sort_connected_ascending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_connected_ascending", "Sort Connected Ascending" );

	if( LANG_CFG_GetString( "sort_connected_descending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_connected_descending", "Sort Connected Descending" );

	if( LANG_CFG_GetString( "sort_ratio_ascending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_ratio_ascending", "Sort Ratio Ascending" );

	if( LANG_CFG_GetString( "sort_ratio_descending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_ratio_descending", "Sort Ratio Descending" );

	if( LANG_CFG_GetString( "sort_aur_ascending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_aur_ascending", "Sort Average Upload Rate Ascending" );

	if( LANG_CFG_GetString( "sort_aur_descending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_aur_descending", "Sort Average Upload Rate Descending" );

	if( LANG_CFG_GetString( "sort_left_ascending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_left_ascending", "Sort Left Ascending" );

	if( LANG_CFG_GetString( "sort_left_descending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_left_descending", "Sort Left Descending" );

	if( LANG_CFG_GetString( "sort_adr_ascending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_adr_ascending", "Sort Average Download Rate Ascending" );

	if( LANG_CFG_GetString( "sort_adr_descending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_adr_descending", "Sort Average Download Rate Descending" );

	if( LANG_CFG_GetString( "sort_login_ascending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_login_ascending", "Sort Login Ascending" );

	if( LANG_CFG_GetString( "sort_login_descending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_login_descending", "Sort Login Descending" );

	if( LANG_CFG_GetString( "sort_access_ascending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_access_ascending", "Sort Access Ascending" );

	if( LANG_CFG_GetString( "sort_access_descending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_access_descending", "Sort Access Descending" );

	if( LANG_CFG_GetString( "sort_email_ascending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_email_ascending", "Sort Email Ascending" );

	if( LANG_CFG_GetString( "sort_email_descending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_email_descending", "Sort Email Descending" );

	if( LANG_CFG_GetString( "sort_created_ascending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_created_ascending", "Sort Created Ascending" );

	if( LANG_CFG_GetString( "sort_created_descending", string( ) ).empty( ) )
		LANG_CFG_SetString( "sort_created_descending", "Sort Created Descending" );
}

void LANG_CFG_SetDefaultsErrorLog( )
{
	if( LANG_CFG_GetString( "unable_to_write_file", string( ) ).empty( ) )
		LANG_CFG_SetString( "unable_to_write_file", "Tracker Warning - Unable to open %s for writing" );

	if( LANG_CFG_GetString( "parsing_torrents_infokey_incomplete", string( ) ).empty( ) )
		LANG_CFG_SetString( "parsing_torrents_infokey_incomplete", "Tracker Error - Parsing Torrent: %s has an incomplete or invalid info key, skipping" );

	if( LANG_CFG_GetString( "parsing_torrents_infokey_invalid", string( ) ).empty( ) )
		LANG_CFG_SetString( "parsing_torrents_infokey_invalid", "Tracker Error - Parsing Torrent: %s doesn't have an info key or info is not a valid bencoded dictionary, skipping" );

	if( LANG_CFG_GetString( "parsing_torrents_decode", string( ) ).empty( ) )
		LANG_CFG_SetString( "parsing_torrents_decode", "Tracker Error - Parsing Torrent: %s is not a valid bencoded dictionary or unable to decode, skipping" );
	
	if( LANG_CFG_GetString( "parsing_torrents_opening", string( ) ).empty( ) )
		LANG_CFG_SetString( "parsing_torrents_opening", "Tracker Error - Parsing Torrent: unable to open %s or no torrents found" );

	if( LANG_CFG_GetString( "parsing_torrents_reading", string( ) ).empty( ) )
		LANG_CFG_SetString( "parsing_torrents_reading", "Tracker Error - Parsing Torrent: unable to open %s for reading" );

	if( LANG_CFG_GetString( "expiring_downloaders", string( ) ).empty( ) )
		LANG_CFG_SetString( "expiring_downloaders", "Tracker Info - Expiring downloaders" );

	if( LANG_CFG_GetString( "dumping_rss", string( ) ).empty( ) )
		LANG_CFG_SetString( "dumping_rss", "Tracker Info - Dumping RSS" );

	if( LANG_CFG_GetString( "dumping_rss_for", string( ) ).empty( ) )
		LANG_CFG_SetString( "dumping_rss_for", "Tracker Info - Dumping RSS for %s" );

	if( LANG_CFG_GetString( "dumping_rss_tags_warning", string( ) ).empty( ) )
		LANG_CFG_SetString( "dumping_rss_tags_warning", "Tracker Warning - No tags to dump RSS files per category, try changing to mode 0 or 2" );

	if( LANG_CFG_GetString( "dumping_stats", string( ) ).empty( ) )
		LANG_CFG_SetString( "dumping_stats", "Tracker Info - Dumping Statistics" );

	if( LANG_CFG_GetString( "user_uploaded_torrent", string( ) ).empty( ) )
		LANG_CFG_SetString( "user_uploaded_torrent", "Tracker Info - %s( %s ) uploaded ( %s )" );

	if( LANG_CFG_GetString( "cbtt_client_parse", string( ) ).empty( ) )
		LANG_CFG_SetString( "cbtt_client_parse", "Tracker Info - Client Banlist: parse called" );

	if( LANG_CFG_GetString( "cbtt_ip_parse", string( ) ).empty( ) )
		LANG_CFG_SetString( "cbtt_ip_parse", "Tracker Info - IP Banlist: parse called" );

	if( LANG_CFG_GetString( "font_file_not_set", string( ) ).empty( ) )
		LANG_CFG_SetString( "font_file_not_set", "Tracker Warning - Dynstat Error: font file not set" );

	if( LANG_CFG_GetString( "font_file_not_found", string( ) ).empty( ) )
		LANG_CFG_SetString( "font_file_not_found", "Tracker Warning - Dynstat Error: font file not found ( %s )" );

	if( LANG_CFG_GetString( "skin_file_not_set", string( ) ).empty( ) )
		LANG_CFG_SetString( "skin_file_not_set", "Tracker Warning - Dynstat Warning: skin file no set" );

	if( LANG_CFG_GetString( "skin_file_not_found", string( ) ).empty( ) )
		LANG_CFG_SetString( "skin_file_not_found", "Tracker Warning - Dynstat Warning: skin file not found ( %s )" );

	if( LANG_CFG_GetString( "skin_format_not_known", string( ) ).empty( ) )
		LANG_CFG_SetString( "skin_format_not_known", "Tracker Warning - Dynstat Warning: skin format unknown ( %s )" );

	if( LANG_CFG_GetString( "skin_true_colour", string( ) ).empty( ) )
		LANG_CFG_SetString( "skin_true_colour", "Tracker Info - Dynstat Info: true colour skin image" );

	if( LANG_CFG_GetString( "skin_pallete_colour", string( ) ).empty( ) )
		LANG_CFG_SetString( "skin_pallete_colour", "Tracker Info - Dynstat Info: palette based skin image" );

	if( LANG_CFG_GetString( "image_dir_not_found", string( ) ).empty( ) )
		LANG_CFG_SetString( "image_dir_not_found", "Tracker Warning - Dynstat Error: image directory not found ( %s )" );

	if( LANG_CFG_GetString( "image_dir_not_set", string( ) ).empty( ) )
		LANG_CFG_SetString( "image_dir_not_set", "Tracker Warning - Dynstat Error: image directory not set" );

	if( LANG_CFG_GetString( "image_filename_empty", string( ) ).empty( ) )
		LANG_CFG_SetString( "image_filename_empty", "Tracker Warning - Dynstat Error: file name is empty" );

	if( LANG_CFG_GetString( "parsing_dynstat", string( ) ).empty( ) )
		LANG_CFG_SetString( "parsing_dynstat", "Tracker Info - Dynstat Info: parsing" );

	if( LANG_CFG_GetString( "access_log", string( ) ).empty( ) )
		LANG_CFG_SetString( "access_log", "Tracker Warning - Access Log Warning: unable to open %s for writing" );

	if( LANG_CFG_GetString( "escaped_string", string( ) ).empty( ) )
		LANG_CFG_SetString( "escaped_string", "Tracker Warning - Error Decoding Escaped String: possible truncation, halting decode" );

	if( LANG_CFG_GetString( "makefile", string( ) ).empty( ) )
		LANG_CFG_SetString( "makefile", "Tracker Warning - Make File: unable to open %s for writing" );

	if( LANG_CFG_GetString( "deletefile", string( ) ).empty( ) )
		LANG_CFG_SetString( "deletefile", "Tracker Info - Delete File: deleted \"%s\"" );

	if( LANG_CFG_GetString( "deletefile_error", string( ) ).empty( ) )
		LANG_CFG_SetString( "deletefile_error", "Tracker Warning - Delete File: error deleting \"%s\"" );

	if( LANG_CFG_GetString( "deletefile_error_stream", string( ) ).empty( ) )
		LANG_CFG_SetString( "deletefile_error_stream", "Tracker Warning - Delete File: error deleting \"%s\" - %s" );

	if( LANG_CFG_GetString( "movefile", string( ) ).empty( ) )
		LANG_CFG_SetString( "movefile", "Tracker Warning - Move File: error moving \"%s\"" );

	if( LANG_CFG_GetString( "readfile", string( ) ).empty( ) )
		LANG_CFG_SetString( "readfile", "Tracker Warning - Read File: unable to open %s for reading" );

	if( LANG_CFG_GetString( "sizefile", string( ) ).empty( ) )
		LANG_CFG_SetString( "sizefile", "Tracker Warning - Read File: unable to open %s for reading" );

	if( LANG_CFG_GetString( "decode_http_post_end", string( ) ).empty( ) )
		LANG_CFG_SetString( "decode_http_post_end", "Tracker Warning - Decode HTTP Post: error decoding HTTP POST request - unexpected end of request" );

	if( LANG_CFG_GetString( "decode_http_post_notfound", string( ) ).empty( ) )
		LANG_CFG_SetString( "decode_http_post_notfound", "Tracker Warning - Decode HTTP Post: error decoding HTTP POST request - couldn't find Content-Disposition" );

	if( LANG_CFG_GetString( "decode_http_post_disposition", string( ) ).empty( ) )
		LANG_CFG_SetString( "decode_http_post_disposition", "Tracker Warning - Decode HTTP Post: error decoding HTTP POST request - malformed Content-Disposition" );

	if( LANG_CFG_GetString( "decode_http_post_segment", string( ) ).empty( ) )
		LANG_CFG_SetString( "decode_http_post_segment", "Tracker Warning - Decode HTTP Post: error decoding HTTP POST request - malformed segment" );

	if( LANG_CFG_GetString( "init_mysql_users", string( ) ).empty( ) )
		LANG_CFG_SetString( "init_mysql_users", "Tracker Error - MySQL Users Error: unable to init mysql users database : %s" );

	if( LANG_CFG_GetString( "connect_mysql_users", string( ) ).empty( ) )
		LANG_CFG_SetString( "connect_mysql_users", "Tracker Error - MySQL Users Error: unable to connect mysql users database : %s" );

	if( LANG_CFG_GetString( "success_mysql_users", string( ) ).empty( ) )
		LANG_CFG_SetString( "success_mysql_users", "Tracker Info - MySQL Users Info: successfully connected mysql users database" );

	if( LANG_CFG_GetString( "no_mysql_users_column", string( ) ).empty( ) )
		LANG_CFG_SetString( "no_mysql_users_column", "Tracker Error - MySQL Users Error: no mysql users column : %s" );

	if( LANG_CFG_GetString( "found_mysql_users_column", string( ) ).empty( ) )
		LANG_CFG_SetString( "found_mysql_users_column", "Tracker Info - MySQL Users Info: successfully found mysql users column" );

	if( LANG_CFG_GetString( "not_set_default_access", string( ) ).empty( ) )
		LANG_CFG_SetString( "not_set_default_access", "Tracker Error - MySQL Users Error: not set mysql users member default access : %s" );

	if( LANG_CFG_GetString( "set_default_access", string( ) ).empty( ) )
		LANG_CFG_SetString( "set_default_access", "Tracker Info - MySQL Users Info: successfully set mysql users member default access" );

	if( LANG_CFG_GetString( "not_set_member_access", string( ) ).empty( ) )
		LANG_CFG_SetString( "not_set_member_access", "Tracker Error - MySQL Users Error: not set mysql users member access : %s" );

	if( LANG_CFG_GetString( "set_member_access", string( ) ).empty( ) )
		LANG_CFG_SetString( "set_member_access", "Tracker Info - MySQL Users Info: successfully set mysql users member access" );

	if( LANG_CFG_GetString( "not_created_access_column", string( ) ).empty( ) )
		LANG_CFG_SetString( "not_created_access_column", "Tracker Error - MySQL Users Error: unable to creating mysql users access column : %s" );

	if( LANG_CFG_GetString( "created_access_column", string( ) ).empty( ) )
		LANG_CFG_SetString( "created_access_column", "Tracker Info - MySQL Users Info: successfully created mysql users access column" );

	if( LANG_CFG_GetString( "not_closed_mysql_users", string( ) ).empty( ) )
		LANG_CFG_SetString( "not_closed_mysql_users", "Tracker Error - MySQL Users Error: unable to close mysql users database : %s" );

	if( LANG_CFG_GetString( "closed_mysql_users", string( ) ).empty( ) )
		LANG_CFG_SetString( "closed_mysql_users", "Tracker Info - MySQL Users Info: successfully closed mysql users database" );

	if( LANG_CFG_GetString( "mysql_users_query", string( ) ).empty( ) )
		LANG_CFG_SetString( "mysql_users_query", "Tracker Info - MySQL Users Info: query - %s" );

	if( LANG_CFG_GetString( "mysql_users_query_error", string( ) ).empty( ) )
		LANG_CFG_SetString( "mysql_users_query_error", "Tracker Warning - MySQL Users Error: query - %s" );

	if( LANG_CFG_GetString( "ping_error", string( ) ).empty( ) )
		LANG_CFG_SetString( "ping_error", "Tracker Warning - MySQL Users Error: ping database - %s" );

	if( LANG_CFG_GetString( "loaded_mysql_users", string( ) ).empty( ) )
		LANG_CFG_SetString( "loaded_mysql_users", "Tracker Info - MySQL Users Info: loaded %d MySQL Users" );

	if( LANG_CFG_GetString( "not_loaded_mysql_users", string( ) ).empty( ) )
		LANG_CFG_SetString( "not_loaded_mysql_users", "Tracker Warning - MySQL Users Error: load MySQL Users Failed" );

	if( LANG_CFG_GetString( "loading_mysql_users", string( ) ).empty( ) )
		LANG_CFG_SetString( "loading_mysql_users", "Tracker Info - MySQL Users Info: loading mysql users" );
    
	if( LANG_CFG_GetString( "catomlist", string( ) ).empty( ) )
		LANG_CFG_SetString( "catomlist", "Tracker Warning - Atom Error: copying list - found invalid atom, ignoring" );

	if( LANG_CFG_GetString( "catomdicti", string( ) ).empty( ) )
		LANG_CFG_SetString( "catomdicti", "Tracker Warning - Atom Error: copying dictionary - found invalid atom, ignoring" );

	if( LANG_CFG_GetString( "decodelong", string( ) ).empty( ) )
		LANG_CFG_SetString( "decodelong", "Tracker Warning - Bencode Error: decoding long - couldn't find \"e\", halting decode" );

	if( LANG_CFG_GetString( "decodestring", string( ) ).empty( ) )
		LANG_CFG_SetString( "decodestring", "Tracker Warning - Bencode Error: decoding string - couldn't find \":\", halting decode" );

	if( LANG_CFG_GetString( "decodelist", string( ) ).empty( ) )
		LANG_CFG_SetString( "decodelist", "Tracker Warning - Bencode Error: decoding list - error decoding list item, discarding list" );

	if( LANG_CFG_GetString( "decodedictiitem", string( ) ).empty( ) )
		LANG_CFG_SetString( "decodedictiitem", "Tracker Warning - Bencode Error: decoding dictionary - error decoding value, discarding dictionary" );

	if( LANG_CFG_GetString( "decodedictikey", string( ) ).empty( ) )
		LANG_CFG_SetString( "decodedictikey", "Tracker Warning - Bencode Error: decoding key, discarding dictionary" );

	if( LANG_CFG_GetString( "decode", string( ) ).empty( ) )
		LANG_CFG_SetString( "decode", "Tracker Warning - Bencode Error: found unexpected character %u, halting decode" );

	if( LANG_CFG_GetString( "decodefile", string( ) ).empty( ) )
		LANG_CFG_SetString( "decodefile", "Tracker Warning - Bencode Error: unable to open %s for reading" );

	if( LANG_CFG_GetString( "dumping_xml", string( ) ).empty( ) )
		LANG_CFG_SetString( "dumping_xml", "Tracker Info - Dumping XML" );

	if( LANG_CFG_GetString( "unable_to_bind", string( ) ).empty( ) )
		LANG_CFG_SetString( "unable_to_bind", "Server Error - Unable to bind to %s" );

	if( LANG_CFG_GetString( "binding_to_all", string( ) ).empty( ) )
		LANG_CFG_SetString( "binding_to_all", "Server Info - Binding to all available addresses" );

	if( LANG_CFG_GetString( "invalid_port", string( ) ).empty( ) )
		LANG_CFG_SetString( "invalid_port", "Server Warning - Invalid port %s (\"port\"), ignoring" );

	if( LANG_CFG_GetString( "unable_to_listen", string( ) ).empty( ) )
		LANG_CFG_SetString( "unable_to_listen", "Server Warning - Unable to add listener on port %s (\"port\")" );

	if( LANG_CFG_GetString( "listen_on_port", string( ) ).empty( ) )
		LANG_CFG_SetString( "listen_on_port", "Server Info - Listening on port %s (\"port\")" );

	if( LANG_CFG_GetString( "invalid_ports", string( ) ).empty( ) )
		LANG_CFG_SetString( "invalid_ports", "Server Warning - Invalid port %s (\"%s\"), ignoring" );

	if( LANG_CFG_GetString( "unable_to_listens", string( ) ).empty( ) )
		LANG_CFG_SetString( "unable_to_listens", "Server Warning - Unable to add listener on port %s (\"%s\")" );

	if( LANG_CFG_GetString( "listen_on_ports", string( ) ).empty( ) )
		LANG_CFG_SetString( "listen_on_ports", "Server Info - Listening on port %s (\"%s\")" );

	if( LANG_CFG_GetString( "not_listening", string( ) ).empty( ) )
		LANG_CFG_SetString( "not_listening", "Server Error - Not listening on any ports" );

	if( LANG_CFG_GetString( "select_error", string( ) ).empty( ) )
		LANG_CFG_SetString( "select_error", "Server Warning - Select error (error %s)" );

	if( LANG_CFG_GetString( "accept_error", string( ) ).empty( ) )
		LANG_CFG_SetString( "accept_error", "Server Warning - Accept error (error %s)" );

	if( LANG_CFG_GetString( "no_tcp_protocol", string( ) ).empty( ) )
		LANG_CFG_SetString( "no_tcp_protocol", "Server Error - Unable to get tcp protocol entry (error %s)" );

	if( LANG_CFG_GetString( "not_allocated_socket", string( ) ).empty( ) )
		LANG_CFG_SetString( "not_allocated_socket", "Server Error - Unable to allocate socket (error %s)" );

	if( LANG_CFG_GetString( "no_sndbuf", string( ) ).empty( ) )
		LANG_CFG_SetString( "no_sndbuf", "Server Warning - Setsockopt SO_SNDBUF (error %s" );

	if( LANG_CFG_GetString( "no_rcvbuf", string( ) ).empty( ) )
		LANG_CFG_SetString( "no_rcvbuf", "Server Warning - Setsockopt SO_RCVBUF (error %s)" );

	if( LANG_CFG_GetString( "no_reuseaddr", string( ) ).empty( ) )
		LANG_CFG_SetString( "no_reuseaddr", "Server Warning - Setsockopt SO_REUSEADDR (error %s)" );

	if( LANG_CFG_GetString( "no_nodelay", string( ) ).empty( ) )
		LANG_CFG_SetString( "no_nodelay", "Server Warning - Setsockopt TCP_NODELAY (error %s)" );

	if( LANG_CFG_GetString( "unable_to_bind_socket", string( ) ).empty( ) )
		LANG_CFG_SetString( "unable_to_bind_socket", "Server Error - Unable to bind socket (error %s)" );

	if( LANG_CFG_GetString( "no_nosigpipe", string( ) ).empty( ) )
		LANG_CFG_SetString( "no_nosigpipe", "Server Warning - Setsockopt SO_NOSIGPIPE (error %s)" );

	if( LANG_CFG_GetString( "unable_to_listen", string( ) ).empty( ) )
		LANG_CFG_SetString( "unable_to_listen", "Server Error - Unable to listen (error %s)" );

	if( LANG_CFG_GetString( "server_start", string( ) ).empty( ) )
		LANG_CFG_SetString( "server_start", "Server Info - Start" );

	if( LANG_CFG_GetString( "server_exit", string( ) ).empty( ) )
		LANG_CFG_SetString( "server_exit", "Server Info - Exit" );

	if( LANG_CFG_GetString( "unable_to_start_winsock", string( ) ).empty( ) )
		LANG_CFG_SetString( "unable_to_start_winsock", "XBNBT Error - Unable to start winsock (error %s)" );

	if( LANG_CFG_GetString( "mysql_users_connected", string( ) ).empty( ) )
		LANG_CFG_SetString( "mysql_users_connected", "XBNBT MySQL Users - Connected" );

	if( LANG_CFG_GetString( "bnbt_mysql_error", string( ) ).empty( ) )
		LANG_CFG_SetString( "bnbt_mysql_error", "BNBT MySQL Error - (error %s)" );

	if( LANG_CFG_GetString( "bnbt_mysql_connected", string( ) ).empty( ) )
		LANG_CFG_SetString( "bnbt_mysql_connected", "BNBT MySQL - Connected" );

	if( LANG_CFG_GetString( "unable_to_spawn_link_thread_win32", string( ) ).empty( ) )
		LANG_CFG_SetString( "unable_to_spawn_link_thread_win32", "XBNBT Error - Unable to spawn link thread" );

	if( LANG_CFG_GetString( "unable_to_spawn_link_thread", string( ) ).empty( ) )
		LANG_CFG_SetString( "unable_to_spawn_link_thread", "XBNBT Error - Unable to spawn link thread (error %s)" );

	if( LANG_CFG_GetString( "unable_to_spawn_hublink_thread_win32", string( ) ).empty( ) )
		LANG_CFG_SetString( "unable_to_spawn_hublink_thread_win32", "XBNBT Error - Unable to spawn HUB link thread" );

	if( LANG_CFG_GetString( "unable_to_spawn_hublink_thread", string( ) ).empty( ) )
		LANG_CFG_SetString( "unable_to_spawn_hublink_thread", "XBNBT Error - Unable to spawn HUB link thread (error %s)" );

	if( LANG_CFG_GetString( "wait_link_disconnect", string( ) ).empty( ) )
		LANG_CFG_SetString( "wait_link_disconnect", "XBNBT Info - Waiting for link to disconnect" );

	if( LANG_CFG_GetString( "waited_link_disconnect", string( ) ).empty( ) )
		LANG_CFG_SetString( "waited_link_disconnect", "XBNBT Warning - Waited 60 seconds for link to disconnect, exiting anyway" );

	if( LANG_CFG_GetString( "wait_hublink_disconnect", string( ) ).empty( ) )
		LANG_CFG_SetString( "wait_hublink_disconnect", "XBNBT Info - Waiting for HUB link to disconnect" );

	if( LANG_CFG_GetString( "waited_hublink_disconnect", string( ) ).empty( ) )
		LANG_CFG_SetString( "waited_hublink_disconnect", "XBNBT Warning - Waited 60 seconds for HUB link to disconnect, exiting anyway" );

	if( LANG_CFG_GetString( "atomdicti_copy_warning", string( ) ).empty( ) )
		LANG_CFG_SetString( "atomdicti_copy_warning", "AtomDicti Warning - Copying dictionary: found invalid atom, ignoring" );

	if( LANG_CFG_GetString( "atomlist_copy_warning", string( ) ).empty( ) )
		LANG_CFG_SetString( "atomlist_copy_warning", "AtomList Warning - Copying dictionary: found invalid atom, ignoring" );
}

void LANG_CFG_SetDefaults( )
{
	if( gbDebug )
		UTIL_LogPrint( "language - setting defaults\n" );
	
	LANG_CFG_SetDefaultsErrorLog( );
	LANG_CFG_SetDefaultsXbnbt( );
	LANG_CFG_SetDefaultsSort( );

	// admin stats
	if( LANG_CFG_GetString( "stats_dynstat", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_dynstat", "XBNBT Dynstat Stats<" );

	if( LANG_CFG_GetString( "stats_last_reset", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_last_reset", "Last Reset" );

	if( LANG_CFG_GetString( "stats_run_times", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_run_times", "Run Times" );

	if( LANG_CFG_GetString( "stats_last_run", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_last_run", "Last Run" );

	if( LANG_CFG_GetString( "stats_last_elapsed", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_last_elapsed", "Last Elapsed" );

	if( LANG_CFG_GetString( "stats_last_processed", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_last_processed", "Processed Last Run" );

	if( LANG_CFG_GetString( "stats_torrent_frozen", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_torrent_frozen", "Torrents Frozen" );

	if( LANG_CFG_GetString( "stats_processed_start", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_processed_start", "Processed Since Start" );

	if( LANG_CFG_GetString( "stats_announce", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_announce", "XBNBT Announce Stats" );

	if( LANG_CFG_GetString( "stats_announces", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_announces", "Announces" );

	if( LANG_CFG_GetString( "stats_announces_missing", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_announces_missing", "Hash Missing" );

	if( LANG_CFG_GetString( "stats_announces_notauth", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_announces_notauth", "Hash Not Matched" );

	if( LANG_CFG_GetString( "stats_compact", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_compact", "Compact" );

	if( LANG_CFG_GetString( "stats_no_peer_id", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_no_peer_id", "No Peer ID" );

	if( LANG_CFG_GetString( "stats_regular", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_regular", "Regular" );

	if( LANG_CFG_GetString( "stats_ip_banned", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_ip_banned", "IP Banned" );

	if( LANG_CFG_GetString( "stats_ip_not_cleared", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_ip_not_cleared", "IP Not Cleared" );

	if( LANG_CFG_GetString( "stats_peer_spoof_blocked", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_peer_spoof_blocked", "Peer Spoof Blocked" );

	if( LANG_CFG_GetString( "stats_client_banned", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_client_banned", "Client Banned" );

	if( LANG_CFG_GetString( "stats_invalid_event", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_invalid_event", "Invalid Event" );

	if( LANG_CFG_GetString( "stats_port_missing", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_port_missing", "Port Missing" );

	if( LANG_CFG_GetString( "stats_uploaded_missing", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_uploaded_missing", "Uploaded Missing" );

	if( LANG_CFG_GetString( "stats_downloaded_missing", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_downloaded_missing", "Downloaded Missing" );

	if( LANG_CFG_GetString( "stats_left_missing", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_left_missing", "Left Missing" );

	if( LANG_CFG_GetString( "stats_peer_id_length", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_peer_id_length", "Peer ID Length" );

	if( LANG_CFG_GetString( "stats_port_blacklisted", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_port_blacklisted", "Port Blacklisted" );

	if( LANG_CFG_GetString( "stats_download_invalid", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_download_invalid", "Downloaded Invalid" );

	if( LANG_CFG_GetString( "stats_upload_invalid", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_upload_invalid", "Uploaded Invalid" );

	if( LANG_CFG_GetString( "stats_scrape", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_scrape", "XBNBT Scrape Stats" );

	if( LANG_CFG_GetString( "stats_scrapes", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_scrapes", "Scrapes" );

	if( LANG_CFG_GetString( "stats_single", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_single", "Single" );

	if( LANG_CFG_GetString( "stats_multiple", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_multiple", "Multiple" );

	if( LANG_CFG_GetString( "stats_full", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_full", "Full" );

	if( LANG_CFG_GetString( "stats_file", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_file", "XBNBT File Stats" );

	if( LANG_CFG_GetString( "stats_favicon", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_favicon", RESPONSE_STR_FAVICON_ICO );

	if( LANG_CFG_GetString( "stats_ibfill", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_ibfill", "Image Bar Fill" );

	if( LANG_CFG_GetString( "stats_ibtrans", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_ibtrans", "Image Bar Trans" );

	if( LANG_CFG_GetString( "stats_css", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_css", "CSS" );

	if( LANG_CFG_GetString( "stats_rss", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_rss", "RSS" );

	if( LANG_CFG_GetString( "stats_xml", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_xml", "XML" );

	if( LANG_CFG_GetString( "stats_robots", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_robots", RESPONSE_STR_ROBOTS_TXT );

	if( LANG_CFG_GetString( "stats_other", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_other", "Other Files" );

	if( LANG_CFG_GetString( "stats_index", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_index", "Index" );

	if( LANG_CFG_GetString( "stats_stats", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_stats", "Stats" );

	if( LANG_CFG_GetString( "stats_comments", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_comments", "Comments" );

	if( LANG_CFG_GetString( "stats_info", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_info", "Info" );

	if( LANG_CFG_GetString( "stats_login", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_login", "Login" );

	if( LANG_CFG_GetString( "stats_signup", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_signup", "Signup" );

	if( LANG_CFG_GetString( "stats_upload", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_upload", "Upload" );

	if( LANG_CFG_GetString( "stats_torrents", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_torrents", "Torrents" );

	if( LANG_CFG_GetString( "stats_admin", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_admin", "Admin" );

	if( LANG_CFG_GetString( "stats_users", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_users", "Users" );

	if( LANG_CFG_GetString( "stats_error404", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_error404", "Error 404" );

	// admin
	if( LANG_CFG_GetString( "admin_dumping_xml", string( ) ).empty( ) )
		LANG_CFG_SetString( "admin_dumping_xml", "Updated XML file." );

	if( LANG_CFG_GetString( "admin_links_message", string( ) ).empty( ) )
		LANG_CFG_SetString( "admin_links_message", "Resetting tracker link. Click %shere%s to return to the admin page." );

	if( LANG_CFG_GetString( "admin_not_owner", string( ) ).empty( ) )
		LANG_CFG_SetString( "admin_not_owner", "This tracker does not own a tracker link (it might be a tracker hub)." );

	if( LANG_CFG_GetString( "admin_rss_message", string( ) ).empty( ) )
		LANG_CFG_SetString( "admin_rss_message", "Updated RSS file(s)." );

	if( LANG_CFG_GetString( "admin_serving", string( ) ).empty( ) )
		LANG_CFG_SetString( "admin_serving", "Currently serving" );

	if( LANG_CFG_GetString( "admin_serving_clients", string( ) ).empty( ) )
		LANG_CFG_SetString( "admin_serving_clients", "clients (including you)!" );

	if( LANG_CFG_GetString( "admin_serving_client", string( ) ).empty( ) )
		LANG_CFG_SetString( "admin_serving_client", "client (including you)!" );

	if( LANG_CFG_GetString( "admin_tracker_links", string( ) ).empty( ) )
		LANG_CFG_SetString( "admin_tracker_links", "Tracker Links" );

	if( LANG_CFG_GetString( "admin_tracker_hub_links", string( ) ).empty( ) )
		LANG_CFG_SetString( "admin_tracker_hub_links", "Tracker Hub Links" );

	if( LANG_CFG_GetString( "admin_primary_tracker", string( ) ).empty( ) )
		LANG_CFG_SetString( "admin_primary_tracker", "Primary Tracker" );

	if( LANG_CFG_GetString( "admin_connections", string( ) ).empty( ) )
		LANG_CFG_SetString( "admin_connections", "Connections" );

	if( LANG_CFG_GetString( "admin_active", string( ) ).empty( ) )
		LANG_CFG_SetString( "admin_active", "(ACTIVE)" );

	if( LANG_CFG_GetString( "admin_not_active", string( ) ).empty( ) )
		LANG_CFG_SetString( "admin_not_active", "(NOT ACTIVE)" );

	if( LANG_CFG_GetString( "admin_secondary_tracker", string( ) ).empty( ) )
		LANG_CFG_SetString( "admin_secondary_tracker", "Secondary Tracker" );

	if( LANG_CFG_GetString( "admin_reset_link", string( ) ).empty( ) )
		LANG_CFG_SetString( "admin_reset_link", "Reset Tracker Link" );

	if( LANG_CFG_GetString( "admin_reset_hub_link", string( ) ).empty( ) )
		LANG_CFG_SetString( "admin_reset_hub_link", "Reset Tracker Link" );

	if( LANG_CFG_GetString( "admin_no_link", string( ) ).empty( ) )
		LANG_CFG_SetString( "admin_no_link", "No Link" );

	if( LANG_CFG_GetString( "admin_kill_tracker", string( ) ).empty( ) )
		LANG_CFG_SetString( "admin_kill_tracker", "Kill Tracker" );

	if( LANG_CFG_GetString( "admin_kill_warning", string( ) ).empty( ) )
		LANG_CFG_SetString( "admin_kill_warning", "If you kill the tracker your connection will be dropped and no response will be sent to your browser." );

	if( LANG_CFG_GetString( "admin_count_peers", string( ) ).empty( ) )
		LANG_CFG_SetString( "admin_count_peers", "Count Unique Peers" );

	if( LANG_CFG_GetString( "admin_update_rss", string( ) ).empty( ) )
		LANG_CFG_SetString( "admin_update_rss", "Update RSS file(s)" );

	if( LANG_CFG_GetString( "admin_update_xml", string( ) ).empty( ) )
		LANG_CFG_SetString( "admin_update_xml", "Update XML file" );

	if( LANG_CFG_GetString( "admin_update_dynstat", string( ) ).empty( ) )
		LANG_CFG_SetString( "admin_update_dynstat", "Update Dynstat Images" );

	if( LANG_CFG_GetString( "reset", string( ) ).empty( ) )
		LANG_CFG_SetString( "reset", "Reset" );

	if( LANG_CFG_GetString( "reset_all_stats", string( ) ).empty( ) )
		LANG_CFG_SetString( "reset_all_stats", "Reset All Stats" );

	if( LANG_CFG_GetString( "established", string( ) ).empty( ) )
		LANG_CFG_SetString( "established", "Established" );

	// announce
	if( LANG_CFG_GetString( "announce_hash_missing", string( ) ).empty( ) )
		LANG_CFG_SetString( "announce_hash_missing", "info hash missing" );

	if( LANG_CFG_GetString( "announce_download_not_authorized", string( ) ).empty( ) )
		LANG_CFG_SetString( "announce_download_not_authorized", "requested download is not authorized for use with this tracker" );

	if( LANG_CFG_GetString( "announce_ip_banned", string( ) ).empty( ) )
		LANG_CFG_SetString( "announce_ip_banned", "Your IP has been banned from this tracker." );

	if( LANG_CFG_GetString( "announce_ip_not_cleared", string( ) ).empty( ) )
		LANG_CFG_SetString( "announce_ip_not_cleared", "Your IP has not been cleared for use on this tracker." );

	if( LANG_CFG_GetString( "announce_client_spoof", string( ) ).empty( ) )
		LANG_CFG_SetString( "announce_client_spoof", "This tracker does not allow client spoofing. Please disable client spoofing in your client configuration." );

	if( LANG_CFG_GetString( "announce_client_banned", string( ) ).empty( ) )
		LANG_CFG_SetString( "announce_client_banned", "Client Version is banned. Check with the tracker administrator" );

	if( LANG_CFG_GetString( "announce_invalid_event", string( ) ).empty( ) )
		LANG_CFG_SetString( "announce_invalid_event", "invalid event" );

	if( LANG_CFG_GetString( "announce_port_missing", string( ) ).empty( ) )
		LANG_CFG_SetString( "announce_port_missing", "port missing" );

	if( LANG_CFG_GetString( "announce_uploaded_missing", string( ) ).empty( ) )
		LANG_CFG_SetString( "announce_uploaded_missing", "uploaded missing" );

	if( LANG_CFG_GetString( "announce_downloaded_missing", string( ) ).empty( ) )
		LANG_CFG_SetString( "announce_downloaded_missing", "downloaded missing" );

	if( LANG_CFG_GetString( "announce_left_missing", string( ) ).empty( ) )
		LANG_CFG_SetString( "announce_left_missing", "left missing" );

	if( LANG_CFG_GetString( "announce_peerid_length", string( ) ).empty( ) )
		LANG_CFG_SetString( "announce_peerid_length", "peer id not of length 20" );

	if( LANG_CFG_GetString( "announce_blacklisted_port", string( ) ).empty( ) )
		LANG_CFG_SetString( "announce_blacklisted_port", "Listen port %s is blacklisted from this server, please adjust your settings" );

	if( LANG_CFG_GetString( "announce_downloaded_invalid", string( ) ).empty( ) )
		LANG_CFG_SetString( "announce_downloaded_invalid", "Amount reported downloaded is invaild" );

	if( LANG_CFG_GetString( "announce_uploaded_invalid", string( ) ).empty( ) )
		LANG_CFG_SetString( "announce_uploaded_invalid", "Amount reported uploaded is invaild" );

	// comments
	if( LANG_CFG_GetString( "comments_deleted_all", string( ) ).empty( ) )
		LANG_CFG_SetString( "comments_deleted_all", "Deleted all comments. Click %shere%s to return to the comments page." );

	if( LANG_CFG_GetString( "comments_deleted", string( ) ).empty( ) )
		LANG_CFG_SetString( "comments_deleted", "Deleted comment %s. Click %shere%s to return to the comments page." );

	if( LANG_CFG_GetString( "comments_unable_delete", string( ) ).empty( ) )
		LANG_CFG_SetString( "comments_unable_delete", "Unable to delete comment %s. The comment number is invalid. Click %shere%s to return to the comments page." );

	if( LANG_CFG_GetString( "comments_file_information", string( ) ).empty( ) )
		LANG_CFG_SetString( "comments_file_information", "File Information" );

	if( LANG_CFG_GetString( "comments_fill_warning", string( ) ).empty( ) )
		LANG_CFG_SetString( "comments_fill_warning", "You must fill in all the fields. Click %shere%s to return to the comments page." );

	if( LANG_CFG_GetString( "comments_posted", string( ) ).empty( ) )
		LANG_CFG_SetString( "comments_posted", "Your comment has been posted. DO NOT REFRESH THIS PAGE OR YOUR COMMENT WILL BE POSTED TWICE. Click %shere%s to return to the comments page." );

	if( LANG_CFG_GetString( "comments", string( ) ).empty( ) )
		LANG_CFG_SetString( "comments", "Comments" );

	if( LANG_CFG_GetString( "comments_posted_by", string( ) ).empty( ) )
		LANG_CFG_SetString( "comments_posted_by", "Comment %s posted by <strong>%s</strong> (%s) : %s" );

	if( LANG_CFG_GetString( "comments_no_comment", string( ) ).empty( ) )
		LANG_CFG_SetString( "comments_no_comment", "No Comments Posted" );

	if( LANG_CFG_GetString( "comments_post_comment", string( ) ).empty( ) )
		LANG_CFG_SetString( "comments_post_comment", "Post A Comment" );

	if( LANG_CFG_GetString( "comments_length_info", string( ) ).empty( ) )
		LANG_CFG_SetString( "comments_length_info", "Comments must be less than %s characters long" );

	if( LANG_CFG_GetString( "comment_check_length", string( ) ).empty( ) )
		LANG_CFG_SetString( "comment_check_length", "check message length" );

	if( LANG_CFG_GetString( "comments_post_disallowed", string( ) ).empty( ) )
		LANG_CFG_SetString( "comments_post_disallowed", "You are not authorized to post comments." );

	// index
	if( LANG_CFG_GetString( "index_invalid_hash", string( ) ).empty( ) )
		LANG_CFG_SetString( "index_invalid_hash", "Unable to delete torrent %s. The info hash is invalid. Click %shere%s to return to the tracker." );

	if( LANG_CFG_GetString( "index_deleted_torrent", string( ) ).empty( ) )
		LANG_CFG_SetString( "index_deleted_torrent", "Deleted torrent %s. Click %shere%s to return to the tracker." );

	if( LANG_CFG_GetString( "index_subfilter", string( ) ).empty( ) )
		LANG_CFG_SetString( "index_subfilter", "Sub-Filter" );

	if( LANG_CFG_GetString( "index_no_files", string( ) ).empty( ) )
		LANG_CFG_SetString( "index_no_files", "No files are being tracked yet!" );

	// filter
	if( LANG_CFG_GetString( "filter_by", string( ) ).empty( ) )
		LANG_CFG_SetString( "filter_by", "Filter by" );

	// sub-filter
	if( LANG_CFG_GetString( "subfilter_all", string( ) ).empty( ) )
		LANG_CFG_SetString( "subfilter_all", "All" );

	if( LANG_CFG_GetString( "subfilter_seeded", string( ) ).empty( ) )
		LANG_CFG_SetString( "subfilter_seeded", "Seeded" );

	if( LANG_CFG_GetString( "subfilter_unseeded", string( ) ).empty( ) )
		LANG_CFG_SetString( "subfilter_unseeded", "Unseeded" );

	if( LANG_CFG_GetString( "subfilter_mytorrents", string( ) ).empty( ) )
		LANG_CFG_SetString( "subfilter_mytorrents", "MyTorrents" );

	// results
	if( LANG_CFG_GetString( "result_search", string( ) ).empty( ) )
		LANG_CFG_SetString( "result_search", "Search results by" );

	if( LANG_CFG_GetString( "result_filter", string( ) ).empty( ) )
		LANG_CFG_SetString( "result_filter", "Filtered by" );

	if( LANG_CFG_GetString( "result_subfilter", string( ) ).empty( ) )
		LANG_CFG_SetString( "result_subfilter", "Sub-Filtered by" );

	if( LANG_CFG_GetString( "result_none_found", string( ) ).empty( ) )
		LANG_CFG_SetString( "result_none_found", "No Result Found" );

	if( LANG_CFG_GetString( "result_1_found", string( ) ).empty( ) )
		LANG_CFG_SetString( "result_1_found", "1 Result Found" );

	if( LANG_CFG_GetString( "result_x_found", string( ) ).empty( ) )
		LANG_CFG_SetString( "result_x_found", "%s Results Found" );

	if( LANG_CFG_GetString( "viewing_page_num", string( ) ).empty( ) )
		LANG_CFG_SetString( "viewing_page_num", "You are viewing page %s" );

	if( LANG_CFG_GetString( "index_jump", string( ) ).empty( ) )
		LANG_CFG_SetString( "index_jump", "Jump to Page Navigation and Torrent Search" );

	if( LANG_CFG_GetString( "torrent_search", string( ) ).empty( ) )
		LANG_CFG_SetString( "torrent_search", "Torrent Search" );

	if( LANG_CFG_GetString( "clear_filter_search", string( ) ).empty( ) )
		LANG_CFG_SetString( "clear_filter_search", "Clear Filter and Search Results" );

	if( LANG_CFG_GetString( "jump_to_page", string( ) ).empty( ) )
		LANG_CFG_SetString( "jump_to_page", "Jump to Page" );

	if( LANG_CFG_GetString( "click_to_signup", string( ) ).empty( ) )
		LANG_CFG_SetString( "click_to_signup", "Click %shere%s to sign up for an account!" );

	if( LANG_CFG_GetString( "info_tracker_info", string( ) ).empty( ) )
		LANG_CFG_SetString( "info_tracker_info", "Tracker Info" );

	if( LANG_CFG_GetString( "info_tracker_version", string( ) ).empty( ) )
		LANG_CFG_SetString( "info_tracker_version", "Tracker Version" );

	if( LANG_CFG_GetString( "info_server_time", string( ) ).empty( ) )
		LANG_CFG_SetString( "info_server_time", "Server Time" );

	if( LANG_CFG_GetString( "info_uptime", string( ) ).empty( ) )
		LANG_CFG_SetString( "info_uptime", "Uptime" );

	if( LANG_CFG_GetString( "info_tracker_title", string( ) ).empty( ) )
		LANG_CFG_SetString( "info_tracker_title", "Tracker Title" );

	if( LANG_CFG_GetString( "info_tracker_desc", string( ) ).empty( ) )
		LANG_CFG_SetString( "info_tracker_desc", "Tracker Description" );

	if( LANG_CFG_GetString( "forced_announce_url", string( ) ).empty( ) )
		LANG_CFG_SetString( "forced_announce_url", "Forced Announce URL" );

	if( LANG_CFG_GetString( "announce_url", string( ) ).empty( ) )
		LANG_CFG_SetString( "announce_url", "Announce URL" );

	if( LANG_CFG_GetString( "info_torrent_size", string( ) ).empty( ) )
		LANG_CFG_SetString( "info_torrent_size", "Max. Torrent Size" );

	if( LANG_CFG_GetString( "info_torrents", string( ) ).empty( ) )
		LANG_CFG_SetString( "info_torrents", "Torrents" );

	if( LANG_CFG_GetString( "info_rss", string( ) ).empty( ) )
		LANG_CFG_SetString( "info_rss", "RSS Feed" );

	if( LANG_CFG_GetString( "info_total_peers", string( ) ).empty( ) )
		LANG_CFG_SetString( "info_total_peers", "Total Peers ( Seeders / Leechers )" );

	if( LANG_CFG_GetString( "info_unique_peers", string( ) ).empty( ) )
		LANG_CFG_SetString( "info_unique_peers", "Unique Peers" );

	if( LANG_CFG_GetString( "info_reg_users", string( ) ).empty( ) )
		LANG_CFG_SetString( "info_reg_users", "Registered Users" );

	if( LANG_CFG_GetString( "logging_out", string( ) ).empty( ) )
		LANG_CFG_SetString( "logging_out", "Logging out... You may need to close your browser window to completely logout." );

	if( LANG_CFG_GetString( "login_invalid_hash", string( ) ).empty( ) )
		LANG_CFG_SetString( "login_invalid_hash", "Unable to delete torrent %s. The info hash is invalid. Click %shere%s to return to the login page." );

	if( LANG_CFG_GetString( "login_not_owned", string( ) ).empty( ) )
		LANG_CFG_SetString( "login_not_owned", "Unable to delete torrent %s. You didn't upload that torrent. Click %shere%s to return to the login page." );

	if( LANG_CFG_GetString( "login_deleted_torrent", string( ) ).empty( ) )
		LANG_CFG_SetString( "login_deleted_torrent", "Deleted torrent %s. Click %shere%s to return to the login page." );

	if( LANG_CFG_GetString( "login_signed_up", string( ) ).empty( ) )
		LANG_CFG_SetString( "login_signed_up", "You signed up on %s." );

	if( LANG_CFG_GetString( "login_your_torrents", string( ) ).empty( ) )
		LANG_CFG_SetString( "login_your_torrents", "Your Torrents" );

	if( LANG_CFG_GetString( "login_rows_per_page", string( ) ).empty( ) )
		LANG_CFG_SetString( "login_rows_per_page", "Rows Per Page" );

	if( LANG_CFG_GetString( "signup_fields_error", string( ) ).empty( ) )
		LANG_CFG_SetString( "signup_fields_error", "Unable to signup. You must fill in all the fields. %shere%s to return to the signup page." );

	if( LANG_CFG_GetString( "signup_name_error", string( ) ).empty( ) )
		LANG_CFG_SetString( "signup_name_error", "Unable to signup. Your name must be less than %s characters long and it must not start or end with spaces. Click %shere%s to return to the signup page." );

	if( LANG_CFG_GetString( "signup_email_error", string( ) ).empty( ) )
		LANG_CFG_SetString( "signup_email_error", "Unable to signup. Your e-mail address is invalid. Click %shere%s to return to the signup page." );

	if( LANG_CFG_GetString( "signup_exists_error", string( ) ).empty( ) )
		LANG_CFG_SetString( "signup_exists_error", "Unable to signup. The user \"%s\" already exists. Click %shere%s to return to the signup page." );

	if( LANG_CFG_GetString( "signup_success", string( ) ).empty( ) )
		LANG_CFG_SetString( "signup_success", "Thanks! You've successfully signed up! Click %shere%s to login." );

	if( LANG_CFG_GetString( "signup_password_error", string( ) ).empty( ) )
		LANG_CFG_SetString( "signup_password_error", "Unable to signup. The passwords did not match. Click %shere%s to return to the signup page." );

	if( LANG_CFG_GetString( "signup", string( ) ).empty( ) )
		LANG_CFG_SetString( "signup", "Signup" );

	if( LANG_CFG_GetString( "signup_info_name_length", string( ) ).empty( ) )
		LANG_CFG_SetString( "signup_info_name_length", "Names must be less than %s characters long" );

	if( LANG_CFG_GetString( "signup_info_case", string( ) ).empty( ) )
		LANG_CFG_SetString( "signup_info_case", "Names are case sensitive" );

	if( LANG_CFG_GetString( "login", string( ) ).empty( ) )
		LANG_CFG_SetString( "login", "Login" );

	if( LANG_CFG_GetString( "password", string( ) ).empty( ) )
		LANG_CFG_SetString( "password", "Password" );

	if( LANG_CFG_GetString( "verify_password", string( ) ).empty( ) )
		LANG_CFG_SetString( "verify_password", "Verify Password" );

	if( LANG_CFG_GetString( "email", string( ) ).empty( ) )
		LANG_CFG_SetString( "email", "E-Mail" );

	if( LANG_CFG_GetString( "stats_changed_name", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_changed_name", "Changed name to \"%s\" (blank values mean no change)." );

	if( LANG_CFG_GetString( "stats_changed_tag", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_changed_tag", "Changed tag to \"%s\" (blank values mean no change)." );

	if( LANG_CFG_GetString( "stats_changed_uploader", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_changed_uploader", "Changed uploader to \"%s\" (blank values mean no change)." );

	if( LANG_CFG_GetString( "stats_changed_infolink", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_changed_infolink", "Changed info link to \"%s\" (blank values mean no change)." );

	if( LANG_CFG_GetString( "stats_return", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_return", "Click %shere%s to return to the stats page." );

	if( LANG_CFG_GetString( "stats_change_q", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_change_q", "Are you sure you want to change the torrent's info?" );

	if( LANG_CFG_GetString( "stats_change_info", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_change_info", "Change Info" );

	if( LANG_CFG_GetString( "stats_new_name", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_new_name", "New Name" );

	if( LANG_CFG_GetString( "stats_new_uploader", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_new_uploader", "New Uploader" );

	if( LANG_CFG_GetString( "stats_new_infolink", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_new_infolink", "New Info Link" );

	if( LANG_CFG_GetString( "stats_new_tag", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_new_tag", "New Tag" );

	if( LANG_CFG_GetString( "stats_info_blank", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_info_blank", "&dagger; If this field is left blank, this value will remain unchanged." );

	if( LANG_CFG_GetString( "stats_info_remove", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_info_remove", "&Dagger; Type %sREMOVE%s to clear this value from the database." );

	if( LANG_CFG_GetString( "stats_file_info", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_file_info", "File Information" );

	if( LANG_CFG_GetString( "stats_download_torrent", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_download_torrent", "DOWNLOAD TORRENT" );

	if( LANG_CFG_GetString( "stats_dynstat_link", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_dynstat_link", "DYNAMIC STATS LINK" );

	if( LANG_CFG_GetString( "stats_delete_torrent", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_delete_torrent", "DELETE THIS TORRENT" );

	if( LANG_CFG_GetString( "contents", string( ) ).empty( ) )
		LANG_CFG_SetString( "contents", "Contents" );

	if( LANG_CFG_GetString( "file", string( ) ).empty( ) )
		LANG_CFG_SetString( "file", "File" );

	if( LANG_CFG_GetString( "stats_seeders", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_seeders", "Seeders" );

	if( LANG_CFG_GetString( "peer_ip", string( ) ).empty( ) )
		LANG_CFG_SetString( "peer_ip", "Peer IP" );

	if( LANG_CFG_GetString( "client", string( ) ).empty( ) )
		LANG_CFG_SetString( "client", "Client" );

	if( LANG_CFG_GetString( "uploaded", string( ) ).empty( ) )
		LANG_CFG_SetString( "uploaded", "Uploaded" );

	if( LANG_CFG_GetString( "downloaded", string( ) ).empty( ) )
		LANG_CFG_SetString( "downloaded", "Downloaded" );

	if( LANG_CFG_GetString( "connected", string( ) ).empty( ) )
		LANG_CFG_SetString( "connected", "Connected" );

	if( LANG_CFG_GetString( "share_ratio", string( ) ).empty( ) )
		LANG_CFG_SetString( "share_ratio", "Share Ratio" );

	if( LANG_CFG_GetString( "avg_up_rate", string( ) ).empty( ) )
		LANG_CFG_SetString( "avg_up_rate", "Average Upload Rate" );

	if( LANG_CFG_GetString( "perfect", string( ) ).empty( ) )
		LANG_CFG_SetString( "perfect", "Perfect" );

	if( LANG_CFG_GetString( "sec", string( ) ).empty( ) )
		LANG_CFG_SetString( "sec", "sec" );

	if( LANG_CFG_GetString( "stats_leechers", string( ) ).empty( ) )
		LANG_CFG_SetString( "stats_leechers", "Leechers" );

	if( LANG_CFG_GetString( "progress", string( ) ).empty( ) )
		LANG_CFG_SetString( "progress", "Progress" );

	if( LANG_CFG_GetString( "left", string( ) ).empty( ) )
		LANG_CFG_SetString( "left", "Left" );

	if( LANG_CFG_GetString( "avg_down_rate", string( ) ).empty( ) )
		LANG_CFG_SetString( "avg_down_rate", "Average Download Rate" );

	if( LANG_CFG_GetString( "upload_nofile_uploads", string( ) ).empty( ) )
		LANG_CFG_SetString( "upload_nofile_uploads", "This tracker does not allow file uploads. Click %shere%s to return to the tracker." );

	if( LANG_CFG_GetString( "upload_torrent_limit", string( ) ).empty( ) )
		LANG_CFG_SetString( "upload_torrent_limit", "This tracker has reached its torrent limit. Click %shere%s to return to the tracker." );

	if( LANG_CFG_GetString( "upload_file", string( ) ).empty( ) )
		LANG_CFG_SetString( "upload_file", "Torrent File to Upload" );

	if( LANG_CFG_GetString( "upload_name", string( ) ).empty( ) )
		LANG_CFG_SetString( "upload_name", "Name (Optional - See Note Below)" );

	if( LANG_CFG_GetString( "upload_infolink", string( ) ).empty( ) )
		LANG_CFG_SetString( "upload_infolink", "Informational  Link (Optional - See Note Below)" );

	if( LANG_CFG_GetString( "upload_tag", string( ) ).empty( ) )
		LANG_CFG_SetString( "upload_tag", "Tag/Category" );

	if( LANG_CFG_GetString( "upload_note_1", string( ) ).empty( ) )
		LANG_CFG_SetString( "upload_note_1", "If <b>Name</b> is left blank, the torrent filename will be used." );

	if( LANG_CFG_GetString( "upload_note_2", string( ) ).empty( ) )
		LANG_CFG_SetString( "upload_note_2", "Names must be less than %s characters long" );

	if( LANG_CFG_GetString( "upload_note_3", string( ) ).empty( ) )
		LANG_CFG_SetString( "upload_note_3", "Informational Links must start with <b>http://</b>" );

	if( LANG_CFG_GetString( "upload_note_4", string( ) ).empty( ) )
		LANG_CFG_SetString( "upload_note_4", "Max. File Size" );

	if( LANG_CFG_GetString( "upload", string( ) ).empty( ) )
		LANG_CFG_SetString( "upload", "Upload" );

	if( LANG_CFG_GetString( "cancel", string( ) ).empty( ) )
		LANG_CFG_SetString( "cancel", "Cancel" );

	if( LANG_CFG_GetString( "failed", string( ) ).empty( ) )
		LANG_CFG_SetString( "failed", "Failed" );

	if( LANG_CFG_GetString( "upload_file_corrupt", string( ) ).empty( ) )
		LANG_CFG_SetString( "upload_file_corrupt", "The uploaded file is corrupt or invalid. Click %shere%s to return to the upload page." );

	if( LANG_CFG_GetString( "upload_not_torrent", string( ) ).empty( ) )
		LANG_CFG_SetString( "upload_not_torrent", "The uploaded file is not a .torrent file. Click %shere%s to return to the upload page." );

	if( LANG_CFG_GetString( "upload_filetag_invalid", string( ) ).empty( ) )
		LANG_CFG_SetString( "upload_filetag_invalid", "The file tag is invalid. Click %shere%s to return to the upload page." );

	if( LANG_CFG_GetString( "upload_file_exists", string( ) ).empty( ) )
		LANG_CFG_SetString( "upload_file_exists", "The uploaded file already exists. Click %shere%s to return to the upload page." );

	if( LANG_CFG_GetString( "upload_hash_exists", string( ) ).empty( ) )
		LANG_CFG_SetString( "upload_hash_exists", "A file with the uploaded file's info hash already exists. Click %shere%s to return to the upload page." );

	if( LANG_CFG_GetString( "successful", string( ) ).empty( ) )
		LANG_CFG_SetString( "successful", "Successful" );

	if( LANG_CFG_GetString( "upload_another", string( ) ).empty( ) )
		LANG_CFG_SetString( "upload_another", "Upload Another Torrent" );

	if( LANG_CFG_GetString( "return_to_tracker", string( ) ).empty( ) )
		LANG_CFG_SetString( "return_to_tracker", "Return To Tracker" );

	if( LANG_CFG_GetString( "upload_ready_now", string( ) ).empty( ) )
		LANG_CFG_SetString( "upload_ready_now", "The uploaded file is ready. You should start seeding it now." );

	if( LANG_CFG_GetString( "upload_ready_1min", string( ) ).empty( ) )
		LANG_CFG_SetString( "upload_ready_1min", "The uploaded file will be ready in %s minute. You should start seeding it as soon as possible." );

	if( LANG_CFG_GetString( "upload_ready_xmins", string( ) ).empty( ) )
		LANG_CFG_SetString( "upload_ready_xmins", "The uploaded file will be ready in %s minutes. You should start seeding it as soon as possible." );

	if( LANG_CFG_GetString( "upload_seeding_instructions", string( ) ).empty( ) )
		LANG_CFG_SetString( "upload_seeding_instructions", "Seeding Instructions" );

	if( LANG_CFG_GetString( "upload_begin_seeding", string( ) ).empty( ) )
		LANG_CFG_SetString( "upload_begin_seeding", "To begin seeding your torrent..." );

	if( LANG_CFG_GetString( "upload_inst_1", string( ) ).empty( ) )
		LANG_CFG_SetString( "upload_inst_1", "Locate and double-click the torrent file on your hard drive.&dagger;" );

	if( LANG_CFG_GetString( "upload_inst_2", string( ) ).empty( ) )
		LANG_CFG_SetString( "upload_inst_2", "Choose the save location where the torrented files exist already." );

	if( LANG_CFG_GetString( "upload_inst_3", string( ) ).empty( ) )
		LANG_CFG_SetString( "upload_inst_3", "Your client will check the existing data and should then say <b>Download Complete</b>." );

	if( LANG_CFG_GetString( "upload_inst_4", string( ) ).empty( ) )
		LANG_CFG_SetString( "upload_inst_4", "Your client should then contact the tracker and begin seeding to others." );

	if( LANG_CFG_GetString( "upload_inst_verify", string( ) ).empty( ) )
		LANG_CFG_SetString( "upload_inst_verify", "To verify that you have correctly started to seed your torrent," );

	if( LANG_CFG_GetString( "upload_inst_return", string( ) ).empty( ) )
		LANG_CFG_SetString( "upload_inst_return", "return to the Tracker and examine the <u>SDs</u>(Seeders) column." );

	if( LANG_CFG_GetString( "upload_note_1", string( ) ).empty( ) )
		LANG_CFG_SetString( "upload_note_1", "Depending on this tracker's configuration, the torrent may not" );

	if( LANG_CFG_GetString( "upload_note_2", string( ) ).empty( ) )
		LANG_CFG_SetString( "upload_note_2", "show up until it has been seeded, and therefore cannot be" );

	if( LANG_CFG_GetString( "upload_note_3", string( ) ).empty( ) )
		LANG_CFG_SetString( "upload_note_3", "initially started using the [DL] link on the tracker's main page." );

	if( LANG_CFG_GetString( "users_load_success", string( ) ).empty( ) )
		LANG_CFG_SetString( "users_load_success", "Last MySQL Users load sucess: %s" );

	if( LANG_CFG_GetString( "users_load_status", string( ) ).empty( ) )
		LANG_CFG_SetString( "users_load_status", "Last MySQL Users load status: %s" );

	if( LANG_CFG_GetString( "users_load_interval", string( ) ).empty( ) )
		LANG_CFG_SetString( "users_load_interval", "Next MySQL Users load interval: %s minutes" );

	if( LANG_CFG_GetString( "users_fields_create_fail", string( ) ).empty( ) )
		LANG_CFG_SetString( "users_fields_create_fail", "Unable to create user. You must fill in all the fields." );

	if( LANG_CFG_GetString( "users_do_q", string( ) ).empty( ) )
		LANG_CFG_SetString( "users_do_q", "What would you like to do?" );

	if( LANG_CFG_GetString( "users_make_correct", string( ) ).empty( ) )
		LANG_CFG_SetString( "users_make_correct", "Make Corrections" );

	if( LANG_CFG_GetString( "users_start_over", string( ) ).empty( ) )
		LANG_CFG_SetString( "users_start_over", "Start Over" );

	if( LANG_CFG_GetString( "users_exists_create_fail", string( ) ).empty( ) )
		LANG_CFG_SetString( "users_exists_create_fail", "Unable to create user. The user \"%s\" already exists." );

	if( LANG_CFG_GetString( "users_created_user", string( ) ).empty( ) )
		LANG_CFG_SetString( "users_created_user", "Created user \"%s\". Click %shere%s to return to User Configuration." );

	if( LANG_CFG_GetString( "users_password_create_fail", string( ) ).empty( ) )
		LANG_CFG_SetString( "users_password_create_fail", "Unable to create user. The passwords did not match." );

	if( LANG_CFG_GetString( "users_password_edit_fail", string( ) ).empty( ) )
		LANG_CFG_SetString( "users_password_edit_fail", "Unable to edit user. The passwords did not match." );

	if( LANG_CFG_GetString( "users_edited_user", string( ) ).empty( ) )
		LANG_CFG_SetString( "users_edited_user", "Edited user \"%s\". Click %shere%s to return to the users page." );

	if( LANG_CFG_GetString( "users_editing_user", string( ) ).empty( ) )
		LANG_CFG_SetString( "users_editing_user", "<strong>Edit User \"%s\"</strong>" );

	if( LANG_CFG_GetString( "users_password", string( ) ).empty( ) )
		LANG_CFG_SetString( "users_password", "Password (optional)" );

	if( LANG_CFG_GetString( "users_verify_password", string( ) ).empty( ) )
		LANG_CFG_SetString( "users_verify_password", "Verify Password (optional)" );

	if( LANG_CFG_GetString( "users_view_access", string( ) ).empty( ) )
		LANG_CFG_SetString( "users_view_access", "View Access (Basic)" );

	if( LANG_CFG_GetString( "users_dl_access", string( ) ).empty( ) )
		LANG_CFG_SetString( "users_dl_access", "DL Access (Downloader)" );

	if( LANG_CFG_GetString( "users_comments_access", string( ) ).empty( ) )
		LANG_CFG_SetString( "users_comments_access", "Comments Access (Poster)" );

	if( LANG_CFG_GetString( "users_upload_access", string( ) ).empty( ) )
		LANG_CFG_SetString( "users_upload_access", "Upload Access (Uploader)" );

	if( LANG_CFG_GetString( "users_edit_access", string( ) ).empty( ) )
		LANG_CFG_SetString( "users_edit_access", "Edit Access (Moderator)" );

	if( LANG_CFG_GetString( "users_admin_access", string( ) ).empty( ) )
		LANG_CFG_SetString( "users_admin_access", "Admin Access (Admin)" );

	if( LANG_CFG_GetString( "users_signup_access", string( ) ).empty( ) )
		LANG_CFG_SetString( "users_signup_access", "Signup Access" );

	if( LANG_CFG_GetString( "edit_user", string( ) ).empty( ) )
		LANG_CFG_SetString( "edit_user", "Edit User" );

	if( LANG_CFG_GetString( "users_edit_nouser", string( ) ).empty( ) )
		LANG_CFG_SetString( "users_edit_nouser", "Unable to edit user. The user \"%s\" does not exist." );

	if( LANG_CFG_GetString( "previous_page", string( ) ).empty( ) )
		LANG_CFG_SetString( "previous_page", "Go Back to Previous Page" );

	if( LANG_CFG_GetString( "users_deleted_user", string( ) ).empty( ) )
		LANG_CFG_SetString( "users_deleted_user", "Deleted user \"%s\". Click %shere%s to return to the users page." );

	if( LANG_CFG_GetString( "users_delete_q", string( ) ).empty( ) )
		LANG_CFG_SetString( "users_delete_q", "Are you sure you want to delete the user \"%s\"? WARNING! If there are no admin users, you won't be able to administrate your tracker!" );

	if( LANG_CFG_GetString( "users_create_user_title", string( ) ).empty( ) )
		LANG_CFG_SetString( "users_create_user_title", "<strong>Create User</strong>" );

	if( LANG_CFG_GetString( "create_user", string( ) ).empty( ) )
		LANG_CFG_SetString( "create_user", "Create User" );

	if( LANG_CFG_GetString( "users_nousers_warning", string( ) ).empty( ) )
		LANG_CFG_SetString( "users_nousers_warning", "<strong>WARNING! Your tracker does not have any users. Guests will have full access until someone creates the first user.</strong>" );

	if( LANG_CFG_GetString( "user_search", string( ) ).empty( ) )
		LANG_CFG_SetString( "user_search", "User Search" );

	if( LANG_CFG_GetString( "access", string( ) ).empty( ) )
		LANG_CFG_SetString( "access", "Access" );

	if( LANG_CFG_GetString( "created", string( ) ).empty( ) )
		LANG_CFG_SetString( "created", "Created" );

	if( LANG_CFG_GetString( "edit", string( ) ).empty( ) )
		LANG_CFG_SetString( "edit", "Edit" );

	if( LANG_CFG_GetString( "loaded", string( ) ).empty( ) )
		LANG_CFG_SetString( "loaded", "Loaded" );

	if( LANG_CFG_GetString( "dynstat_logo", string( ) ).empty( ) )
		LANG_CFG_SetString( "dynstat_logo", "%s %s %s minute update" );

	if( LANG_CFG_GetString( "dynstat_logo_frozen", string( ) ).empty( ) )
		LANG_CFG_SetString( "dynstat_logo_frozen", "%s %s Status: Frozen" );

	if( LANG_CFG_GetString( "dynstat_title", string( ) ).empty( ) )
		LANG_CFG_SetString( "dynstat_title", "Title: %s" );

	if( LANG_CFG_GetString( "dynstat_size", string( ) ).empty( ) )
		LANG_CFG_SetString( "dynstat_size", "Torrent Size: %s" );

	if( LANG_CFG_GetString( "dynstat_files", string( ) ).empty( ) )
		LANG_CFG_SetString( "dynstat_files", "Number Of Files: %s" );

	if( LANG_CFG_GetString( "dynstat_tag", string( ) ).empty( ) )
		LANG_CFG_SetString( "dynstat_tag", "Tag: %s" );

	if( LANG_CFG_GetString( "dynstat_hash", string( ) ).empty( ) )
		LANG_CFG_SetString( "dynstat_hash", "Info Hash: %s" );

	if( LANG_CFG_GetString( "dynstat_filename", string( ) ).empty( ) )
		LANG_CFG_SetString( "dynstat_filename", "File Name: %s" );

	if( LANG_CFG_GetString( "dynstat_added", string( ) ).empty( ) )
		LANG_CFG_SetString( "dynstat_added", "Added: %s" );

	if( LANG_CFG_GetString( "dynstat_seeders", string( ) ).empty( ) )
		LANG_CFG_SetString( "dynstat_seeders", "Seeders: %s" );

	if( LANG_CFG_GetString( "dynstat_leechers", string( ) ).empty( ) )
		LANG_CFG_SetString( "dynstat_leechers", "Leechers: %s" );

	if( LANG_CFG_GetString( "js_enable_support", string( ) ).empty( ) )
		LANG_CFG_SetString( "js_enable_support", "Please enable JavaScript support or upgrade your browser." );

	if( LANG_CFG_GetString( "users_online", string( ) ).empty( ) )
		LANG_CFG_SetString( "users_online", "%s users online" );

	if( LANG_CFG_GetString( "user_online", string( ) ).empty( ) )
		LANG_CFG_SetString( "user_online", "%s user online" );

	if( LANG_CFG_GetString( "generated_time", string( ) ).empty( ) )
		LANG_CFG_SetString( "generated_time", "Generated in %s seconds." );

	if( LANG_CFG_GetString( "valid_html", string( ) ).empty( ) )
		LANG_CFG_SetString( "valid_html", "Valid HTML 4.01!" );

	if( LANG_CFG_GetString( "valid_css", string( ) ).empty( ) )
		LANG_CFG_SetString( "valid_css", "Valid CSS!" );

	if( LANG_CFG_GetString( "valid_rss", string( ) ).empty( ) )
		LANG_CFG_SetString( "valid_rss", "Valid RSS!" );

	if( LANG_CFG_GetString( "cynthia_tested", string( ) ).empty( ) )
		LANG_CFG_SetString( "cynthia_tested", "Cynthia Tested!" );

	if( LANG_CFG_GetString( "server_response_200", string( ) ).empty( ) )
		LANG_CFG_SetString( "server_response_200", "OK" );

	if( LANG_CFG_GetString( "server_response_400", string( ) ).empty( ) )
		LANG_CFG_SetString( "server_response_400", "Bad Request" );
	
	if( LANG_CFG_GetString( "server_response_401", string( ) ).empty( ) )
		LANG_CFG_SetString( "server_response_401", "Unauthorized" );
	
	if( LANG_CFG_GetString( "server_response_403", string( ) ).empty( ) )
		LANG_CFG_SetString( "server_response_403", "Forbidden" );

	if( LANG_CFG_GetString( "server_response_404", string( ) ).empty( ) )
		LANG_CFG_SetString( "server_response_404", "Not Found" );

	if( LANG_CFG_GetString( "server_response_500", string( ) ).empty( ) )
		LANG_CFG_SetString( "server_response_500", "Server Error" );

	if( LANG_CFG_GetString( "comment_ip_hidded", string( ) ).empty( ) )
		LANG_CFG_SetString( "comment_ip_hidded", "HIDDEN" );

	if( LANG_CFG_GetString( "at_run_time", string( ) ).empty( ) )
		LANG_CFG_SetString( "at_run_time", "At run time" );

	if( LANG_CFG_GetString( "unknown", string( ) ).empty( ) )
		LANG_CFG_SetString( "unknown", "Unknown" );
}
