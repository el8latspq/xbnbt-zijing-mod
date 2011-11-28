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

#include "bnbt.h"
#include "bnbt_mysql.h"
#include "atom.h"
#include "bencode.h"
#include "config.h"
#include "html.h"
#include "link.h"
#include "server.h"
#include "tracker.h"
#include "util.h"

void CTracker :: serverResponseAdmin( struct request_t *pRequest, struct response_t *pResponse )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0)
		if( IsIPBanned( pRequest, pResponse, btv, gmapLANG_CFG["admin_page"], string( CSS_ADMIN ), NOT_INDEX ) )
			return;

	// User must have administration rights
	if( !pRequest->user.strUID.empty( ) && ( pRequest->user.ucAccess & m_ucAccessAdmin ) )
	{
		if( pRequest->mapParams["submit_free_button"] == gmapLANG_CFG["Submit"] )
		{
			const string cstrFreeDownGlobal = pRequest->mapParams["free_down_global"];
			const string cstrFreeUpGlobal = pRequest->mapParams["free_up_global"];
			bool bNum = true;
			for( int i = 0; i < cstrFreeDownGlobal.length( ) && bNum; i++ )
				if( !isdigit( cstrFreeDownGlobal[i] ) )
					bNum  = false;
			for( int i = 0; i < cstrFreeUpGlobal.length( ) && bNum; i++ )
				if( !isdigit( cstrFreeUpGlobal[i] ) )
					bNum  = false;

			if( bNum )
			{
				m_iFreeDownGlobal = atoi( cstrFreeDownGlobal.c_str( ) );
				m_iFreeUpGlobal = atoi( cstrFreeUpGlobal.c_str( ) );
				CFG_SetString( "bnbt_free_down_global", cstrFreeDownGlobal );
				CFG_SetString( "bnbt_free_up_global", cstrFreeUpGlobal );
				if( pRequest->mapParams.find( "free_global" ) != pRequest->mapParams.end( ) && pRequest->mapParams["free_global"] == "on" )
				{
					m_bFreeGlobal = true;
					CFG_SetInt( "bnbt_free_global", 1 );
				}
				else
				{
					m_bFreeGlobal = false;
					CFG_SetInt( "bnbt_free_global", 0 );
				}
				CFG_Close( CFG_FILE );
				m_pCache->Reset( );
			}
			
			return JS_ReturnToPage( pRequest, pResponse, ADMIN_HTML );
		}
		
		if( pRequest->mapParams["submit_edit_rule_button"] == gmapLANG_CFG["Submit"] )
		{
			const string cstrFreeRuleIndex = pRequest->mapParams["free_rule_index"];
			if( !cstrFreeRuleIndex.empty( ) )
			{
				string strFreeRuleTag = pRequest->mapParams["free_rule_tag"];
				const string cstrFreeRuleTags = pRequest->mapParams["free_rule_tags"];
				const string cstrFreeRuleKeywordOr = pRequest->mapParams["free_rule_keyword_or"];
				const string cstrFreeRuleKeywordAnd = pRequest->mapParams["free_rule_keyword_and"];
				const string cstrFreeRuleSize = pRequest->mapParams["free_rule_size"];
				const string cstrFreeRuleDownDefault = pRequest->mapParams["free_rule_down_default"];
				const string cstrFreeRuleUpDefault = pRequest->mapParams["free_rule_up_default"];
				const string cstrFreeRuleDown = pRequest->mapParams["free_rule_down"];
				const string cstrFreeRuleUp = pRequest->mapParams["free_rule_up"];
				const string cstrFreeRuleTime = pRequest->mapParams["free_rule_time"];
			
				bool bNum = true;
				for( int i = 0; i < cstrFreeRuleSize.length( ) && bNum; i++ )
					if( !isdigit( cstrFreeRuleSize[i] ) )
						bNum  = false;
				for( int i = 0; i < cstrFreeRuleDownDefault.length( ) && bNum; i++ )
					if( !isdigit( cstrFreeRuleDownDefault[i] ) )
						bNum  = false;
				for( int i = 0; i < cstrFreeRuleUpDefault.length( ) && bNum; i++ )
					if( !isdigit( cstrFreeRuleUpDefault[i] ) )
						bNum  = false;
				for( int i = 0; i < cstrFreeRuleDown.length( ) && bNum; i++ )
					if( !isdigit( cstrFreeRuleDown[i] ) )
						bNum  = false;
				for( int i = 0; i < cstrFreeRuleUp.length( ) && bNum; i++ )
					if( !isdigit( cstrFreeRuleUp[i] ) )
						bNum  = false;
				for( int i = 0; i < cstrFreeRuleTime.length( ) && bNum; i++ )
					if( !isdigit( cstrFreeRuleTime[i] ) )
						bNum  = false;

				if( strFreeRuleTag == "000" )
					strFreeRuleTag = cstrFreeRuleTags;
				
				if( bNum && !strFreeRuleTag.empty( ) )
				{
					CFG_SetString( "bnbt_free_rule_" + cstrFreeRuleIndex, strFreeRuleTag + "|" + cstrFreeRuleKeywordOr + "|" + cstrFreeRuleKeywordAnd + "|" + cstrFreeRuleSize + "|" + cstrFreeRuleDownDefault + "|" + cstrFreeRuleUpDefault + "|" + cstrFreeRuleDown + "|" + cstrFreeRuleUp + "|" + cstrFreeRuleTime );
					CFG_Close( CFG_FILE );
				}
			}
			return JS_ReturnToPage( pRequest, pResponse, ADMIN_HTML );
		}
		
		const string cstrDelRule( pRequest->mapParams["ap_delrule"] );

		if( !cstrDelRule.empty( ) )
		{
			const unsigned char cucDelRule = (unsigned char)atoi( cstrDelRule.c_str( ) );
			unsigned char ucRule_Index = cucDelRule;
			unsigned char ucRulePrevious = ucRule_Index - 1;

			string strName = "bnbt_free_rule_" + cstrDelRule;
			string strRule = CFG_GetString( strName, string( ) );

			while( !strRule.empty( ) )
			{
				if( ucRule_Index == cucDelRule )
					CFG_Delete( strName );

				if( ucRule_Index > cucDelRule )
				{
					strName = "bnbt_free_rule_" + CAtomInt( ucRulePrevious ).toString( );
					CFG_SetString( strName, strRule );
				}
				
				ucRulePrevious = ucRule_Index;

				strName = "bnbt_free_rule_" + CAtomInt( ++ucRule_Index ).toString( );
				strRule = CFG_GetString( strName, string( ) );
			}

			strName = "bnbt_free_rule_" + CAtomInt( ucRulePrevious ).toString( );
			CFG_Delete( strName );
			CFG_Close( CFG_FILE );

			return JS_ReturnToPage( pRequest, pResponse, ADMIN_HTML );
		}
		
		if( pRequest->mapParams["submit_add_rule_button"] == gmapLANG_CFG["Submit"] )
		{
			unsigned char ucRule = 1;

			string strName = "bnbt_free_rule_" + CAtomInt( ucRule ).toString( );
			string strRule = CFG_GetString( strName, string( ) );

			while( !strRule.empty( ) )
			{
				strName = "bnbt_free_rule_" + CAtomInt( ++ucRule ).toString( );
				strRule = CFG_GetString( strName, string( ) );
			}
	
			string strFreeRuleTag = pRequest->mapParams["free_rule_tag"];
			const string cstrFreeRuleTags = pRequest->mapParams["free_rule_tags"];
			const string cstrFreeRuleKeywordOr = pRequest->mapParams["free_rule_keyword_or"];
			const string cstrFreeRuleKeywordAnd = pRequest->mapParams["free_rule_keyword_and"];
			const string cstrFreeRuleSize = pRequest->mapParams["free_rule_size"];
			const string cstrFreeRuleDownDefault = pRequest->mapParams["free_rule_down_default"];
			const string cstrFreeRuleUpDefault = pRequest->mapParams["free_rule_up_default"];
			const string cstrFreeRuleDown = pRequest->mapParams["free_rule_down"];
			const string cstrFreeRuleUp = pRequest->mapParams["free_rule_up"];
			const string cstrFreeRuleTime = pRequest->mapParams["free_rule_time"];
			
			bool bNum = true;
			for( int i = 0; i < cstrFreeRuleSize.length( ) && bNum; i++ )
				if( !isdigit( cstrFreeRuleSize[i] ) )
					bNum  = false;
			for( int i = 0; i < cstrFreeRuleDownDefault.length( ) && bNum; i++ )
				if( !isdigit( cstrFreeRuleDownDefault[i] ) )
					bNum  = false;
			for( int i = 0; i < cstrFreeRuleUpDefault.length( ) && bNum; i++ )
				if( !isdigit( cstrFreeRuleUpDefault[i] ) )
					bNum  = false;
			for( int i = 0; i < cstrFreeRuleDown.length( ) && bNum; i++ )
				if( !isdigit( cstrFreeRuleDown[i] ) )
					bNum  = false;
			for( int i = 0; i < cstrFreeRuleUp.length( ) && bNum; i++ )
				if( !isdigit( cstrFreeRuleUp[i] ) )
					bNum  = false;
			for( int i = 0; i < cstrFreeRuleTime.length( ) && bNum; i++ )
				if( !isdigit( cstrFreeRuleTime[i] ) )
					bNum  = false;
			
			if( strFreeRuleTag == "000" )
				strFreeRuleTag = cstrFreeRuleTags;
			
			if( bNum && !strFreeRuleTag.empty( ) )
			{
				strRule = strFreeRuleTag + "|" + cstrFreeRuleKeywordOr + "|" + cstrFreeRuleKeywordAnd + "|" + cstrFreeRuleSize + "|" + cstrFreeRuleDownDefault + "|" + cstrFreeRuleUpDefault + "|" + cstrFreeRuleDown + "|" + cstrFreeRuleUp + "|" + cstrFreeRuleTime;
				CFG_SetString( strName, strRule );
				CFG_Close( CFG_FILE );
			}

			return JS_ReturnToPage( pRequest, pResponse, ADMIN_HTML );
		}
		
		if( pRequest->mapParams["submit_access_button"] == gmapLANG_CFG["Submit"] )
		{
			for( map<string, string> :: iterator it = pRequest->mapParams.begin( ); it != pRequest->mapParams.end( ); it++ )
			{
				if( it->first == "submit_access_button" )
					continue;
				CFG_SetString( "bnbt_access_" + it->first, it->second );
			}
			CFG_Close( CFG_FILE );
			loadAccess( );
			return JS_ReturnToPage( pRequest, pResponse, ADMIN_HTML + "?func=access" );
		}
		
		if( pRequest->mapParams["submit_school_button"] == gmapLANG_CFG["Submit"] )
		{
			const string cstrSchoolMail = pRequest->mapParams["school_mail"];
			const string cstrSchoolName = pRequest->mapParams["school_name"];
			unsigned char ucIndex = 1;
			string strSchoolMail = gmapLANG_CFG["signup_type"+CAtomInt( ucIndex ).toString( )];
			while( !strSchoolMail.empty( ) )
			{
				strSchoolMail = gmapLANG_CFG["signup_type"+CAtomInt( ++ucIndex ).toString( )];
			}
			LANG_CFG_SetString( "signup_mail"+CAtomInt( ucIndex ).toString( ), cstrSchoolMail );
			LANG_CFG_SetString( "signup_type"+CAtomInt( ucIndex ).toString( ), cstrSchoolName );
			LANG_CFG_Close( LANG_CFG_FILE );
			return JS_ReturnToPage( pRequest, pResponse, ADMIN_HTML + "?func=config" );
		}

		if( pRequest->mapParams["submit_invite_button"] == gmapLANG_CFG["Submit"] )
		{
			if( pRequest->mapParams.find( "invite" ) != pRequest->mapParams.end( ) && pRequest->mapParams["invite"] == "on" )
				CFG_SetInt( "bnbt_invite_enable", 1 );
			else
				CFG_SetInt( "bnbt_invite_enable", 0 );
			CFG_Close( CFG_FILE );
			
			return JS_ReturnToPage( pRequest, pResponse, ADMIN_HTML + "?func=config" );
		}
		
		if( pRequest->mapParams["submit_bonus_trade_button"] == gmapLANG_CFG["Submit"] )
		{
			const string cstrBonusTradeUploadRate = pRequest->mapParams["trade_upload_rate"];
			const string cstrBonusTradeInviteRate = pRequest->mapParams["trade_invite_rate"];
			bool bNum = true;
			for( int i = 0; i < cstrBonusTradeUploadRate.length( ) && bNum; i++ )
				if( !isdigit( cstrBonusTradeUploadRate[i] ) )
					bNum  = false;
			for( int i = 0; i < cstrBonusTradeInviteRate.length( ) && bNum; i++ )
				if( !isdigit( cstrBonusTradeInviteRate[i] ) )
					bNum  = false;

			if( bNum )
			{
				CFG_SetString( "bnbt_bonus_trade_upload_rate", cstrBonusTradeUploadRate );
				CFG_SetString( "bnbt_bonus_trade_invite_rate", cstrBonusTradeInviteRate );
				if( pRequest->mapParams.find( "trade" ) != pRequest->mapParams.end( ) && pRequest->mapParams["trade"] == "on" )
					CFG_SetInt( "bnbt_bonus_trade_enable", 1 );
				else
					CFG_SetInt( "bnbt_bonus_trade_enable", 0 );
				CFG_Close( CFG_FILE );
			}
			
			return JS_ReturnToPage( pRequest, pResponse, ADMIN_HTML + "?func=config" );
		}
		
		if( pRequest->mapParams["submit_add_gift_button"] == gmapLANG_CFG["Submit"] )
		{
			const string cstrAddUploaded = pRequest->mapParams["add_uploaded"];
			const string cstrAddDownloaded = pRequest->mapParams["add_downloaded"];
			const string cstrAddBonus = pRequest->mapParams["add_bonus"];
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
				int64 iAddUploaded = 0, iAddDownloaded = 0, iAddBonus = 0;
				
				if( !cstrAddUploaded.empty( ) )
					iAddUploaded = UTIL_StringTo64( cstrAddUploaded.c_str( ) ) * 1024 * 1024 * 1024;
				if( !cstrAddDownloaded.empty( ) )
					iAddDownloaded = UTIL_StringTo64( cstrAddDownloaded.c_str( ) ) * 1024 * 1024 * 1024;
				if( !cstrAddBonus.empty( ) )
					iAddBonus = UTIL_StringTo64( cstrAddBonus.c_str( ) ) * 100;
					
				CMySQLQuery mq01( "UPDATE users SET buploaded=buploaded+" + CAtomLong( iAddUploaded ).toString( ) + ",bdownloaded=bdownloaded+" + CAtomLong( iAddDownloaded ).toString( ) + ",bbonus=bbonus+" + CAtomLong( iAddBonus ).toString( ) );
				
				m_pCache->ResetUsers( );

				UTIL_LogPrint( "editUser: %s edit all users\n", pRequest->user.strLogin.c_str( ) );
				UTIL_LogPrint( "editUser: Add Uploaded: %s GB\n", cstrAddUploaded.c_str( ) );
				UTIL_LogPrint( "editUser: Add Downloaded: %s GB\n", cstrAddDownloaded.c_str( ) );
				UTIL_LogPrint( "editUser: Add Bonus: %s\n", cstrAddBonus.c_str( ) );
			
				UTIL_LogPrint( "editUser: %s edited all users complete\n", pRequest->user.strLogin.c_str( ) );

			}
			return JS_ReturnToPage( pRequest, pResponse, ADMIN_HTML + "?func=config" );
		}
		
		if( pRequest->mapParams["submit_new_user_gift_button"] == gmapLANG_CFG["Submit"] )
		{
			const string cstrGiftRule = pRequest->mapParams["gift_rule"];
			const string cstrGiftUploaded = pRequest->mapParams["gift_uploaded"];
			const string cstrGiftDownloaded = pRequest->mapParams["gift_downloaded"];
			const string cstrGiftBonus = pRequest->mapParams["gift_bonus"];
			bool bNum = true;
			for( int i = 0; i < cstrGiftUploaded.length( ) && bNum ; i++ )
				if( !isdigit( cstrGiftUploaded[i] ) )
					bNum  = false;
			for( int i = 0; i < cstrGiftDownloaded.length( ) && bNum ; i++ )
				if( !isdigit( cstrGiftDownloaded[i] ) )
					bNum  = false;
			for( int i = 0; i < cstrGiftBonus.length( ) && bNum ; i++ )
				if( !isdigit( cstrGiftBonus[i] ) )
					bNum  = false;
			if( pRequest->mapParams.find( "gift" ) != pRequest->mapParams.end( ) && pRequest->mapParams["gift"] == "on" )
				CFG_SetInt( "bnbt_new_user_gift_enable", 1 );
			else
				CFG_SetInt( "bnbt_new_user_gift_enable", 0 );
			CFG_Close( CFG_FILE );
			if( bNum )
			{
				CFG_SetString( "bnbt_new_user_gift_rule", cstrGiftRule );
				CFG_SetString( "bnbt_new_user_gift_uploaded", cstrGiftUploaded );
				CFG_SetString( "bnbt_new_user_gift_downloaded", cstrGiftDownloaded );
				CFG_SetString( "bnbt_new_user_gift_bonus", cstrGiftBonus );
				CFG_Close( CFG_FILE );
			}
			return JS_ReturnToPage( pRequest, pResponse, ADMIN_HTML + "?func=config" );
		}
		//
		// kill tracker
		//

// 		if( pRequest->mapParams["ap_kill"] == "1" )
// 		{
// 			UTIL_LogPrint( "Admin: Kill Tracker\n" );
// 
// 			gpServer->Kill( );
// 
// 			return;
// 		}

		//
		// count unique peers
		//

		if( m_bCountUniquePeers && pRequest->mapParams["ap_recount"] == "1" )
		{
			UTIL_LogPrint( "Admin: Count Unique Peers\n" );

			gpServer->getTracker( )->CountUniquePeers( );
			
//			CMySQLQuery *pQueryIP = new CMySQLQuery( "SELECT bip from ips" );
//				
//			UTIL_LogPrint( "Admin: %i peers\n", pQueryIP->numRows( ) );
//			
//			delete pQueryIP;

			return JS_ReturnToPage( pRequest, pResponse, ADMIN_HTML + "?func=stat" );
		}
		
		//
		// refresh static files
		//

		if( pRequest->mapParams["ap_refresh_static"] == "1" )
		{
			UTIL_LogPrint( "Admin: Refresh Static Files\n" );

			gpServer->getTracker( )->RefreshStatic( );

                        gmapLANG_CFG.clear( );
			LANG_CFG_Init( LANG_CFG_FILE );

			CFG_Open( CFG_FILE );
			CFG_SetDefaults( );
			CFG_Close( CFG_FILE );
			initTags( );

			return JS_ReturnToPage( pRequest, pResponse, ADMIN_HTML + "?func=stat" );
		}
		
		//
		// cbtt parse list
		//

		if( pRequest->mapParams["ap_cbtt_parse_list"] == "1" )
		{
			UTIL_LogPrint( "Admin: CBTT Parse List\n" );

			gpServer->getTracker( )->CBTTParseList( );

			return JS_ReturnToPage( pRequest, pResponse, ADMIN_HTML + "?func=stat" );
		}

		//
		// reset tracker link
		//

// 		if( pRequest->mapParams["ap_relink"] == "1" )
// 		{
// 			UTIL_LogPrint( "Admin: Reset Tracker Link\n" );
// 
// 			if( gpLink )
// 			{
// 				// Resetting tracker link
// 				gpLink->Kill( );
// 	
// 				return JS_ReturnToPage( pRequest, pResponse, ADMIN_HTML );
// 			}
// 			else
// 			{
// 				// This tracker does not own a tracker link (it might be a tracker hub)
// 				m_bFlagNotOwnLinkAlert = true;
// 
// 				UTIL_LogPrint( "Admin: This tracker does not own a tracker link\n" );
// 
// 				return JS_ReturnToPage( pRequest, pResponse, ADMIN_HTML );
// 			}
// 		}  

		//
		// reset tracker hub link
		//

// 		if( pRequest->mapParams["ap_relinkhub"] == "1" )
// 		{
// 			UTIL_LogPrint( "Admin: Reset Tracker Hub Link\n" );
// 
// 			if( gpHUBLink )
// 			{
// 				// Resetting tracker link
// 				gpHUBLink->Kill( );
// 	
// 				return JS_ReturnToPage( pRequest, pResponse, ADMIN_HTML );
// 			}
// 			else
// 			{
// 				// This tracker does not own a tracker hub link (it might be a tracker hub)
// 				m_bFlagNotOwnHUBLinkAlert = true;
// 
// 				UTIL_LogPrint( "Admin: This tracker does not own a tracker hub link\n" );
// 
// 				return JS_ReturnToPage( pRequest, pResponse, ADMIN_HTML );
// 			}
// 		}  
			
		//
		// Dump XML
		//

// 		if( !xmldump.strName.empty( ) && pRequest->mapParams["ap_xml"] == "1" )
// 		{
// 			UTIL_LogPrint( "Admin: Update XML\n" );
// 
// 			saveXML( );
// 
// 			m_ulDumpXMLNext = GetTime( ) + m_uiDumpXMLInterval;
// 
// 			m_bFlagXMLAlert = true;
// 
// 			return JS_ReturnToPage( pRequest, pResponse, ADMIN_HTML );
// 		}

// 		if( !xmldump.strName.empty( ) )
// 		{
// 			if( !xmldump.strDir.empty( ) )
// 				xmldump.strFile = UTIL_ReadFile( ( xmldump.strDir + RESPONSE_STR_SEPERATOR + xmldump.strName ).c_str( ) );
// 			else
// 				xmldump.strFile = UTIL_ReadFile( xmldump.strName.c_str( ) );
// 		}
// 		else
// 			UTIL_LogPrint( "Admin: Update XML - file name not set\n" );

		//
		// Dump Dynstat
		//

#if defined ( XBNBT_GD )
		if( m_bDynstatGenerate && !m_strDynstatFontFile.empty( ) && !m_strDynstatLinkURL.empty( ) && pRequest->mapParams["ap_dynstat"] == "1" )
		{
			UTIL_LogPrint( "Admin: Update Dynstat Images\n" );
			
			runGenerateDynstat( );

			return JS_ReturnToPage( pRequest, pResponse, ADMIN_HTML );
		}
#endif

		//
		// display the admin page
		//

		// Output common HTML head
		HTML_Common_Begin( pRequest, pResponse, gmapLANG_CFG["admin_page"], string( CSS_ADMIN ), string( ), NOT_INDEX, CODE_200 );

		// JavaScript
		pResponse->strContent += "<script type=\"text/javascript\">\n";
		pResponse->strContent += "<!--\n";

		if( m_bCountUniquePeers )
		{
			pResponse->strContent += "function admin_count() {\n";
			pResponse->strContent += "window.location=\"" + RESPONSE_STR_ADMIN_HTML + "?ap_recount=1\"\n";
			pResponse->strContent += "}\n\n";
		}
		
		pResponse->strContent += "function admin_refresh_static() {\n";
		pResponse->strContent += "window.location=\"" + RESPONSE_STR_ADMIN_HTML + "?ap_refresh_static=1\"\n";
		pResponse->strContent += "}\n\n";
		
		pResponse->strContent += "function admin_cbtt_parse_list() {\n";
		pResponse->strContent += "window.location=\"" + RESPONSE_STR_ADMIN_HTML + "?ap_cbtt_parse_list=1\"\n";
		pResponse->strContent += "}\n\n";

// 		if( !rssdump.strName.empty( ) )
// 		{
// 			pResponse->strContent += "function admin_rss() {\n";
// 			pResponse->strContent += "window.location=\"" + RESPONSE_STR_ADMIN_HTML + "?ap_rss=1\"\n";
// 			pResponse->strContent += "}\n\n";
// 		}

// 		if( !xmldump.strName.empty( ) )
// 		{
// 			pResponse->strContent += "function admin_xml() {\n";
// 			pResponse->strContent += "window.location=\"" + RESPONSE_STR_ADMIN_HTML + "?ap_xml=1\"\n";
// 			pResponse->strContent += "}\n\n";
// 		}

#if defined ( XBNBT_GD )
		if( m_bDynstatGenerate && !m_strDynstatFontFile.empty( ) && !m_strDynstatLinkURL.empty( ) )
		{
			pResponse->strContent += "function admin_dynstat() {\n";
			pResponse->strContent += "window.location=\"" + RESPONSE_STR_ADMIN_HTML + "?ap_dynstat=1\"\n";
			pResponse->strContent += "}\n\n";
		}
#endif

// 		pResponse->strContent += "function kill_confirm()\n";
// 		pResponse->strContent += "{\n";
// 		pResponse->strContent += "var name=confirm(\"" + gmapLANG_CFG["js_kill_the_tracker_q"] + "\")\n";
// 		pResponse->strContent += "if (name==true)\n";
// 		pResponse->strContent += "{\n";
// 		pResponse->strContent += "window.location=\"" + RESPONSE_STR_ADMIN_HTML + "?ap_kill=1\"\n";
// 		pResponse->strContent += "}\n";
// 		pResponse->strContent += "}\n\n";

// 		pResponse->strContent += "function relink_confirm()\n";
// 		pResponse->strContent += "{\n";
// 		pResponse->strContent += "var name=confirm(\"" + gmapLANG_CFG["js_reset_tracker_link_q"] + "\")\n";
// 		pResponse->strContent += "if (name==true)\n";
// 		pResponse->strContent += "{\n";
// 		pResponse->strContent += "window.location=\"" + RESPONSE_STR_ADMIN_HTML + "?ap_relink=1\"\n";
// 		pResponse->strContent += "}\n";
// 		pResponse->strContent += "}\n\n";
// 
// 		pResponse->strContent += "function relinkhub_confirm()\n";
// 		pResponse->strContent += "{\n";
// 		pResponse->strContent += "var name=confirm(\"" + gmapLANG_CFG["js_reset_tracker_hub_link_q"] + "\")\n";
// 		pResponse->strContent += "if (name==true)\n";
// 		pResponse->strContent += "{\n";
// 		pResponse->strContent += "window.location=\"" + RESPONSE_STR_ADMIN_HTML + "?ap_relinkhub=1\"\n";
// 		pResponse->strContent += "}\n";
// 		pResponse->strContent += "}\n\n";

		pResponse->strContent += "function delete_rule_confirm( RULE )\n";
		pResponse->strContent += "{\n";
		pResponse->strContent += "var name=confirm(\"" + gmapLANG_CFG["delete"] + " bnbt_free_rule_\" + RULE )\n";
		pResponse->strContent += "if (name==true)\n";
		pResponse->strContent += "{\n";
		pResponse->strContent += "window.location=\"" + RESPONSE_STR_ADMIN_HTML + "?ap_delrule=\" + RULE\n";
		pResponse->strContent += "}\n";
		pResponse->strContent += "}\n\n";

		pResponse->strContent += "//-->\n";
		pResponse->strContent += "</script>\n\n";
		
		pResponse->strContent += "<table class=\"admin_functions\">\n<tr class=\"admin_functions\">\n";

		pResponse->strContent += "<td class=\"admin_functions\"><a class=\"admin_functions\" href=\"" + RESPONSE_STR_ADMIN_HTML + "?func=free\">" + gmapLANG_CFG["admin_free"] + "</td>\n";
		pResponse->strContent += "<td class=\"admin_functions\"><a class=\"admin_functions\" href=\"" + RESPONSE_STR_ADMIN_HTML + "?func=access\">" + gmapLANG_CFG["admin_access"] + "</td>\n";
		pResponse->strContent += "<td class=\"admin_functions\"><a class=\"admin_functions\" href=\"" + RESPONSE_STR_ADMIN_HTML + "?func=config\">" + gmapLANG_CFG["admin_config"] + "</td>\n";
		pResponse->strContent += "<td class=\"admin_functions\"><a class=\"admin_functions\" href=\"" + RESPONSE_STR_ADMIN_HTML + "?func=recycle\">" + gmapLANG_CFG["admin_recycle"] + "</td>\n";
		pResponse->strContent += "<td class=\"admin_functions\"><a class=\"admin_functions\" href=\"" + RESPONSE_STR_ADMIN_HTML + "?func=stat\">" + gmapLANG_CFG["admin_stat"] + "</td>\n";
		
		pResponse->strContent += "</tr></table>\n<p>\n";
		
		const string cstrAdminFunction( pRequest->mapParams["func"] );
		
		if( cstrAdminFunction.empty( ) || cstrAdminFunction == "free" )
		{
			string strFreeDownGlobal = CFG_GetString( "bnbt_free_down_global", "100" );
			string strFreeUpGlobal = CFG_GetString( "bnbt_free_up_global", "100" );
			
			pResponse->strContent += "<table class=\"admin_function\">\n";
			pResponse->strContent += "<tr class=\"admin_function\">\n";
			pResponse->strContent += "<td class=\"admin_function\">\n";
			pResponse->strContent += "<div class=\"admin_free\">\n";
			pResponse->strContent += "<form method=\"get\" action=\"" + RESPONSE_STR_ADMIN_HTML + "\">\n";
			pResponse->strContent += "<p class=\"admin_free\"><input id=\"id_free_global\" name=\"free_global\" type=checkbox";
		
			if( CFG_GetInt( "bnbt_free_global", 0 ) == 0 ? false : true )
				pResponse->strContent += " checked";
			
			pResponse->strContent += "><label for=\"id_free_global\">" + gmapLANG_CFG["admin_free_enable"] + "</label>";
			pResponse->strContent += Button_Submit( "submit_free", string( gmapLANG_CFG["Submit"] ) );
			pResponse->strContent += "</p>";
			pResponse->strContent += "<p class=\"admin_free\"><span class=\"blue\">" + gmapLANG_CFG["stats_free_down"] + "</span><input name=\"free_down_global\" type=text size=5 maxlength=3 value=\"" + strFreeDownGlobal + "\">%   ";
			pResponse->strContent += "<span class=\"green\">" + gmapLANG_CFG["stats_free_up"] + "</span><input name=\"free_up_global\" type=text size=5 maxlength=3 value=\"" + strFreeUpGlobal + "\">%";
			pResponse->strContent += "</p></form>";
			pResponse->strContent += "</div></td></tr>\n";
			
			string strNameIndex = string( );
			string strRule = string( );
			string strTag = string( );
			string strRuleSize = CFG_GetString( "bnbt_free_rule_size_default", "0" );
			string strRuleDownDefault = CFG_GetString( "bnbt_free_rule_down_default", "100" );
			string strRuleUpDefault = CFG_GetString( "bnbt_free_rule_up_default", "100" );
			string strRuleDown = CFG_GetString( "bnbt_free_rule_down_default", "100" );
			string strRuleUp = CFG_GetString( "bnbt_free_rule_up_default", "100" );
			string strRuleTime = CFG_GetString( "bnbt_free_rule_time_default", "0" );
			string :: size_type iSplit = 0;
			string :: size_type iSplitTime = 0;
					
			pResponse->strContent += "<tr class=\"admin_function\">\n";
			pResponse->strContent += "<td class=\"admin_function\">\n";
			pResponse->strContent += "<div class=\"admin_free_rule\">\n";
			pResponse->strContent += "<p>" + gmapLANG_CFG["admin_free_rule"] + "</p>\n" ;
			pResponse->strContent += "<table>\n";
			pResponse->strContent += "<th>" + gmapLANG_CFG["rule"] + "</th>";
			pResponse->strContent += "<th>" + gmapLANG_CFG["tag"] + "</th>";
			pResponse->strContent += "<th>" + gmapLANG_CFG["keyword"] + "</th>";
			pResponse->strContent += "<th>" + gmapLANG_CFG["size"] + "</th>";
			pResponse->strContent += "<th>" + gmapLANG_CFG["stats_new_free_default"] + "</th>";
			pResponse->strContent += "<th>" + gmapLANG_CFG["stats_new_free"] + "</th>";
			pResponse->strContent += "<th colspan=2>" + gmapLANG_CFG["submit"] + "</th>\n</tr>";
			
			unsigned char ucRule = 1;
			strRule = CFG_GetString( "bnbt_free_rule_" + CAtomInt( ucRule ).toString( ), string( ) );
			vector<string> vecRule;
			vecRule.reserve(9);
			vector<string> vecTags;
			vecTags.reserve(64);
			
			vecRule = UTIL_SplitToVectorStrict( strRule, "|" );
			
			while( !vecRule.empty( ) && vecRule.size( ) == 9 )
			{
				pResponse->strContent += "<form method=\"get\" action=\"" + RESPONSE_STR_ADMIN_HTML + "\">\n";
				pResponse->strContent += "<tr>\n";
				pResponse->strContent += "<input name=\"free_rule_index\" type=hidden value=\"" + CAtomInt( ucRule ).toString( ) + "\">";
				pResponse->strContent += "<td class=\"admin_free_rule\">bnbt_free_rule_" + CAtomInt( ucRule ).toString( ) + "</td>";
				pResponse->strContent += "<td class=\"admin_free_rule\"><select name=\"free_rule_tag\">";
				pResponse->strContent += "<option value=\"000\">" + gmapLANG_CFG["choose_a_tag"] + "\n";
				vecTags = UTIL_SplitToVectorStrict( vecRule[0], " " );
				for( vector< pair< string, string > > :: iterator it = m_vecTags.begin( ); it != m_vecTags.end( ); it++ )
				{
					strNameIndex = (*it).first;
					strTag = (*it).second;
					
					pResponse->strContent += "<option value=\""  + strNameIndex + "\"";

					if( vecTags.size( ) == 1 && strNameIndex == vecTags[0] )
						pResponse->strContent += " selected";

					pResponse->strContent += ">" + strTag + "\n";

				}
				pResponse->strContent += "</select>";
				pResponse->strContent += "<input name=\"free_rule_tags\" type=text size=15 value=\"";
				if( vecTags.size( ) > 1 )
					pResponse->strContent += vecRule[0];
				pResponse->strContent += "\"></td>";
				pResponse->strContent += "<td class=\"admin_free_rule\"><input name=\"free_rule_keyword_or\" type=text size=15 value=\"" + vecRule[1] + "\">";
				pResponse->strContent += "<input name=\"free_rule_keyword_and\" type=text size=15 value=\"" + vecRule[2] + "\"></td>";
				pResponse->strContent += "<td class=\"admin_free_rule\"><input name=\"free_rule_size\" type=text size=3 maxlength=5 value=\"" + vecRule[3] + "\">G</td>";
				pResponse->strContent += "<td class=\"admin_free_rule\"><span class=\"blue\">" + gmapLANG_CFG["stats_default_down"] + "</span><input name=\"free_rule_down_default\" type=text size=3 maxlength=3 value=\"" + vecRule[4] + "\">% ";
				pResponse->strContent += "<span class=\"green\">" + gmapLANG_CFG["stats_default_up"] + "</span><input name=\"free_rule_up_default\" type=text size=3 maxlength=3 value=\"" + vecRule[5] + "\">%</td>";
				pResponse->strContent += "<td class=\"admin_free_rule\"><span class=\"blue\">" + gmapLANG_CFG["stats_free_down"] + "</span><input name=\"free_rule_down\" type=text size=3 maxlength=3 value=\"" + vecRule[6] + "\">% ";
				pResponse->strContent += "<span class=\"green\">" + gmapLANG_CFG["stats_free_up"] + "</span><input name=\"free_rule_up\" type=text size=3 maxlength=3 value=\"" + vecRule[7] + "\">% ";
				pResponse->strContent += gmapLANG_CFG["stats_free_time"] + "<input name=\"free_rule_time\" type=text size=3 maxlength=3 value=\"" + vecRule[8] + "\"></td>";
				pResponse->strContent += "<td class=\"admin_free_rule\">" + Button_Submit( "submit_edit_rule", string( gmapLANG_CFG["Submit"] ) ) + "</td>";
				pResponse->strContent += "<td class=\"admin_free_rule\">";
				pResponse->strContent += Button_JS_Link( "bnbt_free_rule_" + CAtomInt( ucRule ).toString( ), gmapLANG_CFG["delete"], "delete_rule_confirm(" + CAtomInt( ucRule ).toString( ) + ")" );
				pResponse->strContent += "</td>";
				pResponse->strContent += "</tr></form>\n";
				
				strRule = CFG_GetString( "bnbt_free_rule_" + CAtomInt( ++ucRule ).toString( ), string( ) );

				vecRule = UTIL_SplitToVectorStrict( strRule, "|" );
			}
			pResponse->strContent += "<form method=\"get\" action=\"" + RESPONSE_STR_ADMIN_HTML + "\">\n";
			pResponse->strContent += "<tr>\n";
			pResponse->strContent += "<td class=\"admin_free_rule\">*bnbt_free_rule_" + CAtomInt( ucRule ).toString( ) + "</td>";
			pResponse->strContent += "<td class=\"admin_free_rule\"><select name=\"free_rule_tag\">";
			pResponse->strContent += "<option value=\"000\">" + gmapLANG_CFG["choose_a_tag"] + "\n";
			for( vector< pair< string, string > > :: iterator it = m_vecTags.begin( ); it != m_vecTags.end( ); it++ )
			{
				strNameIndex = (*it).first;
				strTag = (*it).second;
				
				pResponse->strContent += "<option value=\""  + strNameIndex + "\"";
				pResponse->strContent += ">" + strTag + "\n";

			}
			pResponse->strContent += "</select>";
			pResponse->strContent += "<input name=\"free_rule_tags\" type=text size=15 value=\"\">";
			pResponse->strContent += "</td>";
			pResponse->strContent += "<td class=\"admin_free_rule\"><input name=\"free_rule_keyword_or\" type=text size=15 value=\"\">";
			pResponse->strContent += "<input name=\"free_rule_keyword_and\" type=text size=15 value=\"\"></td>";
			pResponse->strContent += "<td class=\"admin_free_rule\"><input name=\"free_rule_size\" type=text size=3 maxlength=5 value=\"" + strRuleSize + "\">G</td>";
			pResponse->strContent += "<td class=\"admin_free_rule\"><span class=\"blue\">" + gmapLANG_CFG["stats_default_down"] + "</span><input name=\"free_rule_down_default\" type=text size=3 maxlength=3 value=\"" + strRuleDownDefault + "\">% ";
			pResponse->strContent += "<span class=\"green\">" + gmapLANG_CFG["stats_default_up"] + "</span><input name=\"free_rule_up_default\" type=text size=3 maxlength=3 value=\"" + strRuleUpDefault + "\">%</td>";
			pResponse->strContent += "<td class=\"admin_free_rule\"><span class=\"blue\">" + gmapLANG_CFG["stats_free_down"] + "</span><input name=\"free_rule_down\" type=text size=3 maxlength=3 value=\"" + strRuleDown + "\">% ";
			pResponse->strContent += "<span class=\"green\">" + gmapLANG_CFG["stats_free_up"] + "</span><input name=\"free_rule_up\" type=text size=3 maxlength=3 value=\"" + strRuleUp + "\">% ";
			pResponse->strContent += gmapLANG_CFG["stats_free_time"] + "<input name=\"free_rule_time\" type=text size=3 maxlength=3 value=\"" + strRuleTime + "\"></td>";
			pResponse->strContent += "<td class=\"admin_free_rule\">" + Button_Submit( "submit_add_rule", string( gmapLANG_CFG["Submit"] ) ) + "</td>";
			pResponse->strContent += "<td class=\"admin_free_rule\"></td>";
			pResponse->strContent += "</tr></form>\n";
					
//			for( vector< pair< string, string > > :: iterator ulTagKey = m_vecTags.begin( ); ulTagKey != m_vecTags.end( ); ulTagKey++ )
//			{
//				strNameIndex = (*ulTagKey).first;
//				strTag = (*ulTagKey).second;
//				strRuleDefault = CFG_GetString( "bnbt_free_rule_" + strNameIndex + "_default", string( ) );
//				strRule = CFG_GetString( "bnbt_free_rule_" + strNameIndex, string( ) );
//				
//				strRuleDownDefault = CFG_GetString( "bnbt_free_rule_down_default", "100" );
//				strRuleUpDefault = CFG_GetString( "bnbt_free_rule_up_default", "100" );
//				if( !strRuleDefault.empty( ) )
//				{
//					iSplit = strRuleDefault.find( "|" );
//					if( iSplit == string :: npos ) 
//						strRuleDownDefault = strRuleDefault.substr( 0, iSplit );
//					else
//					{
//						strRuleDownDefault = strRuleDefault.substr( 0, iSplit );
//						strRuleUpDefault = strRuleDefault.substr( iSplit + 1 );
//					}
//				}

//				strRuleDown = CFG_GetString( "bnbt_free_rule_down_default", "100" );
//				strRuleUp = CFG_GetString( "bnbt_free_rule_up_default", "100" );
//				strRuleTime = CFG_GetString( "bnbt_free_rule_time_default", "0" );
//				if( !strRule.empty( ) )
//				{
//					iSplit = strRule.find( "|" );
//					iSplitTime = strRule.find( "|", iSplit + 1 );
//					if( iSplit == string :: npos ) 
//					{
//						if( iSplitTime == string :: npos )
//							strRuleDown = strRule.substr( 0, iSplit );
//					}
//					else
//					{
//						if( iSplitTime == string :: npos )
//						{
//							strRuleDown = strRule.substr( 0, iSplit );
//							strRuleUp = strRule.substr( iSplit + 1 );
//						}
//						else
//						{
//							strRuleDown = strRule.substr( 0, iSplit );
//							strRuleUp = strRule.substr( iSplit + 1, iSplitTime - iSplit - 1 );
//							strRuleTime = strRule.substr( iSplitTime + 1 );
//						}
//					}
//				}
//				
//			}
			pResponse->strContent += "</table>";
			pResponse->strContent += "</td></tr>\n";
			pResponse->strContent += "</table><p>\n";
		}
		else if( cstrAdminFunction == "access" )
		{
			unsigned char ucAccess = m_ucAccessAdmin;
			
			pResponse->strContent += "<div class=\"admin_access\">\n";
			pResponse->strContent += "<form method=\"get\" action=\"" + RESPONSE_STR_ADMIN_HTML + "\">\n";
			pResponse->strContent += "<table class=\"admin_access\">\n";
			ucAccess = m_ucAccessAdmin;
			pResponse->strContent += "<tr class=\"admin_access\"><th class=\"admin_access\">" + gmapLANG_CFG["set_access_view"] + "</th><td><select name=\"view\">\n";
			while( ucAccess > 0 )
			{
				pResponse->strContent += "<option value=\"" + CAtomInt( ucAccess ).toString( ) + "\"";
				if( ucAccess == m_ucAccessView )
					pResponse->strContent += " selected";
				pResponse->strContent += ">" + UTIL_AccessToText( ucAccess ) + "</option>\n";
				ucAccess = ucAccess >> 1;
			}
			pResponse->strContent += "</select>\n</td>\n</tr>\n";
			ucAccess = m_ucAccessAdmin;
			pResponse->strContent += "<tr class=\"admin_access\"><th class=\"admin_access\">" + gmapLANG_CFG["set_access_view_torrents"] + "</th><td><select name=\"view_torrents\">\n";
			while( ucAccess > 0 )
			{
				pResponse->strContent += "<option value=\"" + CAtomInt( ucAccess ).toString( ) + "\"";
				if( ucAccess == m_ucAccessViewTorrents )
					pResponse->strContent += " selected";
				pResponse->strContent += ">" + UTIL_AccessToText( ucAccess ) + "</option>\n";
				ucAccess = ucAccess >> 1;
			}
			pResponse->strContent += "</select>\n</td>\n</tr>\n";
			ucAccess = m_ucAccessAdmin;
			pResponse->strContent += "<tr class=\"admin_access\"><th class=\"admin_access\">" + gmapLANG_CFG["set_access_view_stats"] + "</th><td><select name=\"view_stats\">\n";
			while( ucAccess > 0 )
			{
				pResponse->strContent += "<option value=\"" + CAtomInt( ucAccess ).toString( ) + "\"";
				if( ucAccess == m_ucAccessViewStats )
					pResponse->strContent += " selected";
				pResponse->strContent += ">" + UTIL_AccessToText( ucAccess ) + "</option>\n";
				ucAccess = ucAccess >> 1;
			}
			pResponse->strContent += "</select>\n</td>\n</tr>\n";
			ucAccess = m_ucAccessAdmin;
			pResponse->strContent += "<tr class=\"admin_access\"><th class=\"admin_access\">" + gmapLANG_CFG["set_access_req"] + "</th><td><select name=\"req\">\n";
			while( ucAccess > 0 )
			{
				pResponse->strContent += "<option value=\"" + CAtomInt( ucAccess ).toString( ) + "\"";
				if( ucAccess == m_ucAccessReq )
					pResponse->strContent += " selected";
				pResponse->strContent += ">" + UTIL_AccessToText( ucAccess ) + "</option>\n";
				ucAccess = ucAccess >> 1;
			}
			pResponse->strContent += "</select>\n</td>\n</tr>\n";
			ucAccess = m_ucAccessAdmin;
			pResponse->strContent += "<tr class=\"admin_access\"><th class=\"admin_access\">" + gmapLANG_CFG["set_access_bookmark"] + "</th><td><select name=\"bookmark\">\n";
			while( ucAccess > 0 )
			{
				pResponse->strContent += "<option value=\"" + CAtomInt( ucAccess ).toString( ) + "\"";
				if( ucAccess == m_ucAccessBookmark )
					pResponse->strContent += " selected";
				pResponse->strContent += ">" + UTIL_AccessToText( ucAccess ) + "</option>\n";
				ucAccess = ucAccess >> 1;
			}
			pResponse->strContent += "</select>\n</td>\n</tr>\n";
			ucAccess = m_ucAccessAdmin;
			pResponse->strContent += "<tr class=\"admin_access\"><th class=\"admin_access\">" + gmapLANG_CFG["set_access_down_torrents"] + "</th><td><select name=\"down_torrents\">\n";
			while( ucAccess > 0 )
			{
				pResponse->strContent += "<option value=\"" + CAtomInt( ucAccess ).toString( ) + "\"";
				if( ucAccess == m_ucAccessDownTorrents )
					pResponse->strContent += " selected";
				pResponse->strContent += ">" + UTIL_AccessToText( ucAccess ) + "</option>\n";
				ucAccess = ucAccess >> 1;
			}
			pResponse->strContent += "</select>\n</td>\n</tr>\n";
			ucAccess = m_ucAccessAdmin;
			pResponse->strContent += "<tr class=\"admin_access\"><th class=\"admin_access\">" + gmapLANG_CFG["set_access_down_announce"] + "</th><td><select name=\"down_announce\">\n";
			while( ucAccess > 0 )
			{
				pResponse->strContent += "<option value=\"" + CAtomInt( ucAccess ).toString( ) + "\"";
				if( ucAccess == m_ucAccessDownAnnounce )
					pResponse->strContent += " selected";
				pResponse->strContent += ">" + UTIL_AccessToText( ucAccess ) + "</option>\n";
				ucAccess = ucAccess >> 1;
			}
			pResponse->strContent += "</select>\n</td>\n</tr>\n";
			ucAccess = m_ucAccessAdmin;
			pResponse->strContent += "<tr class=\"admin_access\"><th class=\"admin_access\">" + gmapLANG_CFG["set_access_upload_torrents"] + "</th><td><select name=\"upload_torrents\">\n";
			while( ucAccess > 0 )
			{
				pResponse->strContent += "<option value=\"" + CAtomInt( ucAccess ).toString( ) + "\"";
				if( ucAccess == m_ucAccessUploadTorrents )
					pResponse->strContent += " selected";
				pResponse->strContent += ">" + UTIL_AccessToText( ucAccess ) + "</option>\n";
				ucAccess = ucAccess >> 1;
			}
			pResponse->strContent += "</select>\n</td>\n</tr>\n";
			ucAccess = m_ucAccessAdmin;
			pResponse->strContent += "<tr class=\"admin_access\"><th class=\"admin_access\">" + gmapLANG_CFG["set_access_upload_posts"] + "</th><td><select name=\"upload_posts\">\n";
			while( ucAccess > 0 )
			{
				pResponse->strContent += "<option value=\"" + CAtomInt( ucAccess ).toString( ) + "\"";
				if( ucAccess == m_ucAccessUploadPosts )
					pResponse->strContent += " selected";
				pResponse->strContent += ">" + UTIL_AccessToText( ucAccess ) + "</option>\n";
				ucAccess = ucAccess >> 1;
			}
			pResponse->strContent += "</select>\n</td>\n</tr>\n";
			ucAccess = m_ucAccessAdmin;
			pResponse->strContent += "<tr class=\"admin_access\"><th class=\"admin_access\">" + gmapLANG_CFG["set_access_edit_torrents"] + "</th><td><select name=\"edit_torrents\">\n";
			while( ucAccess > 0 )
			{
				pResponse->strContent += "<option value=\"" + CAtomInt( ucAccess ).toString( ) + "\"";
				if( ucAccess == m_ucAccessEditTorrents )
					pResponse->strContent += " selected";
				pResponse->strContent += ">" + UTIL_AccessToText( ucAccess ) + "</option>\n";
				ucAccess = ucAccess >> 1;
			}
			pResponse->strContent += "</select>\n</td>\n</tr>\n";
			ucAccess = m_ucAccessAdmin;
			pResponse->strContent += "<tr class=\"admin_access\"><th class=\"admin_access\">" + gmapLANG_CFG["set_access_del_torrents"] + "</th><td><select name=\"del_torrents\">\n";
			while( ucAccess > 0 )
			{
				pResponse->strContent += "<option value=\"" + CAtomInt( ucAccess ).toString( ) + "\"";
				if( ucAccess == m_ucAccessDelTorrents )
					pResponse->strContent += " selected";
				pResponse->strContent += ">" + UTIL_AccessToText( ucAccess ) + "</option>\n";
				ucAccess = ucAccess >> 1;
			}
			pResponse->strContent += "</select>\n</td>\n</tr>\n";
			ucAccess = m_ucAccessAdmin;
			pResponse->strContent += "<tr class=\"admin_access\"><th class=\"admin_access\">" + gmapLANG_CFG["set_access_view_offers"] + "</th><td><select name=\"view_offers\">\n";
			while( ucAccess > 0 )
			{
				pResponse->strContent += "<option value=\"" + CAtomInt( ucAccess ).toString( ) + "\"";
				if( ucAccess == m_ucAccessViewOffers )
					pResponse->strContent += " selected";
				pResponse->strContent += ">" + UTIL_AccessToText( ucAccess ) + "</option>\n";
				ucAccess = ucAccess >> 1;
			}
			pResponse->strContent += "</select>\n</td>\n</tr>\n";
			ucAccess = m_ucAccessAdmin;
			pResponse->strContent += "<tr class=\"admin_access\"><th class=\"admin_access\">" + gmapLANG_CFG["set_access_upload_offers"] + "</th><td><select name=\"upload_offers\">\n";
			while( ucAccess > 0 )
			{
				pResponse->strContent += "<option value=\"" + CAtomInt( ucAccess ).toString( ) + "\"";
				if( ucAccess == m_ucAccessUploadOffers )
					pResponse->strContent += " selected";
				pResponse->strContent += ">" + UTIL_AccessToText( ucAccess ) + "</option>\n";
				ucAccess = ucAccess >> 1;
			}
			pResponse->strContent += "</select>\n</td>\n</tr>\n";
			ucAccess = m_ucAccessAdmin;
			pResponse->strContent += "<tr class=\"admin_access\"><th class=\"admin_access\">" + gmapLANG_CFG["set_access_allow_offers"] + "</th><td><select name=\"allow_offers\">\n";
			while( ucAccess > 0 )
			{
				pResponse->strContent += "<option value=\"" + CAtomInt( ucAccess ).toString( ) + "\"";
				if( ucAccess == m_ucAccessAllowOffers )
					pResponse->strContent += " selected";
				pResponse->strContent += ">" + UTIL_AccessToText( ucAccess ) + "</option>\n";
				ucAccess = ucAccess >> 1;
			}
			pResponse->strContent += "</select>\n</td>\n</tr>\n";
			ucAccess = m_ucAccessAdmin;
			pResponse->strContent += "<tr class=\"admin_access\"><th class=\"admin_access\">" + gmapLANG_CFG["set_access_edit_offers"] + "</th><td><select name=\"edit_offers\">\n";
			while( ucAccess > 0 )
			{
				pResponse->strContent += "<option value=\"" + CAtomInt( ucAccess ).toString( ) + "\"";
				if( ucAccess == m_ucAccessEditOffers )
					pResponse->strContent += " selected";
				pResponse->strContent += ">" + UTIL_AccessToText( ucAccess ) + "</option>\n";
				ucAccess = ucAccess >> 1;
			}
			pResponse->strContent += "</select>\n</td>\n</tr>\n";
			ucAccess = m_ucAccessAdmin;
			pResponse->strContent += "<tr class=\"admin_access\"><th class=\"admin_access\">" + gmapLANG_CFG["set_access_del_offers"] + "</th><td><select name=\"del_offers\">\n";
			while( ucAccess > 0 )
			{
				pResponse->strContent += "<option value=\"" + CAtomInt( ucAccess ).toString( ) + "\"";
				if( ucAccess == m_ucAccessDelOffers )
					pResponse->strContent += " selected";
				pResponse->strContent += ">" + UTIL_AccessToText( ucAccess ) + "</option>\n";
				ucAccess = ucAccess >> 1;
			}
			pResponse->strContent += "</select>\n</td>\n</tr>\n";
			ucAccess = m_ucAccessAdmin;
			pResponse->strContent += "<tr class=\"admin_access\"><th class=\"admin_access\">" + gmapLANG_CFG["set_access_edit_own"] + "</th><td><select name=\"edit_own\">\n";
			while( ucAccess > 0 )
			{
				pResponse->strContent += "<option value=\"" + CAtomInt( ucAccess ).toString( ) + "\"";
				if( ucAccess == m_ucAccessEditOwn )
					pResponse->strContent += " selected";
				pResponse->strContent += ">" + UTIL_AccessToText( ucAccess ) + "</option>\n";
				ucAccess = ucAccess >> 1;
			}
			pResponse->strContent += "</select>\n</td>\n</tr>\n";
			ucAccess = m_ucAccessAdmin;
			pResponse->strContent += "<tr class=\"admin_access\"><th class=\"admin_access\">" + gmapLANG_CFG["set_access_del_own"] + "</th><td><select name=\"del_own\">\n";
			while( ucAccess > 0 )
			{
				pResponse->strContent += "<option value=\"" + CAtomInt( ucAccess ).toString( ) + "\"";
				if( ucAccess == m_ucAccessDelOwn )
					pResponse->strContent += " selected";
				pResponse->strContent += ">" + UTIL_AccessToText( ucAccess ) + "</option>\n";
				ucAccess = ucAccess >> 1;
			}
			pResponse->strContent += "</select>\n</td>\n</tr>\n";
			ucAccess = m_ucAccessAdmin;
			pResponse->strContent += "<tr class=\"admin_access\"><th class=\"admin_access\">" + gmapLANG_CFG["set_access_comments"] + "</th><td><select name=\"comments\">\n";
			while( ucAccess > 0 )
			{
				pResponse->strContent += "<option value=\"" + CAtomInt( ucAccess ).toString( ) + "\"";
				if( ucAccess == m_ucAccessComments )
					pResponse->strContent += " selected";
				pResponse->strContent += ">" + UTIL_AccessToText( ucAccess ) + "</option>\n";
				ucAccess = ucAccess >> 1;
			}
			pResponse->strContent += "</select>\n</td>\n</tr>\n";
			ucAccess = m_ucAccessAdmin;
			pResponse->strContent += "<tr class=\"admin_access\"><th class=\"admin_access\">" + gmapLANG_CFG["set_access_comments_always"] + "</th><td><select name=\"comments_always\">\n";
			while( ucAccess > 0 )
			{
				pResponse->strContent += "<option value=\"" + CAtomInt( ucAccess ).toString( ) + "\"";
				if( ucAccess == m_ucAccessCommentsAlways )
					pResponse->strContent += " selected";
				pResponse->strContent += ">" + UTIL_AccessToText( ucAccess ) + "</option>\n";
				ucAccess = ucAccess >> 1;
			}
			pResponse->strContent += "</select>\n</td>\n</tr>\n";
			ucAccess = m_ucAccessAdmin;
			pResponse->strContent += "<tr class=\"admin_access\"><th class=\"admin_access\">" + gmapLANG_CFG["set_access_edit_comments"] + "</th><td><select name=\"edit_comments\">\n";
			while( ucAccess > 0 )
			{
				pResponse->strContent += "<option value=\"" + CAtomInt( ucAccess ).toString( ) + "\"";
				if( ucAccess == m_ucAccessEditComments )
					pResponse->strContent += " selected";
				pResponse->strContent += ">" + UTIL_AccessToText( ucAccess ) + "</option>\n";
				ucAccess = ucAccess >> 1;
			}
			pResponse->strContent += "</select>\n</td>\n</tr>\n";
			ucAccess = m_ucAccessAdmin;
			pResponse->strContent += "<tr class=\"admin_access\"><th class=\"admin_access\">" + gmapLANG_CFG["set_access_del_comments"] + "</th><td><select name=\"del_comments\">\n";
			while( ucAccess > 0 )
			{
				pResponse->strContent += "<option value=\"" + CAtomInt( ucAccess ).toString( ) + "\"";
				if( ucAccess == m_ucAccessDelComments )
					pResponse->strContent += " selected";
				pResponse->strContent += ">" + UTIL_AccessToText( ucAccess ) + "</option>\n";
				ucAccess = ucAccess >> 1;
			}
			pResponse->strContent += "</select>\n</td>\n</tr>\n";
			ucAccess = m_ucAccessAdmin;
			pResponse->strContent += "<tr class=\"admin_access\"><th class=\"admin_access\">" + gmapLANG_CFG["set_access_comments_to_message"] + "</th><td><select name=\"comments_to_message\">\n";
			while( ucAccess > 0 )
			{
				pResponse->strContent += "<option value=\"" + CAtomInt( ucAccess ).toString( ) + "\"";
				if( ucAccess == m_ucAccessCommentsToMessage )
					pResponse->strContent += " selected";
				pResponse->strContent += ">" + UTIL_AccessToText( ucAccess ) + "</option>\n";
				ucAccess = ucAccess >> 1;
			}
			pResponse->strContent += "</select>\n</td>\n</tr>\n";
			
			
			ucAccess = m_ucAccessAdmin;
			pResponse->strContent += "<tr class=\"admin_access\"><th class=\"admin_access\">" + gmapLANG_CFG["set_access_view_users"] + "</th><td><select name=\"view_users\">\n";
			while( ucAccess > 0 )
			{
				pResponse->strContent += "<option value=\"" + CAtomInt( ucAccess ).toString( ) + "\"";
				if( ucAccess == m_ucAccessViewUsers )
					pResponse->strContent += " selected";
				pResponse->strContent += ">" + UTIL_AccessToText( ucAccess ) + "</option>\n";
				ucAccess = ucAccess >> 1;
			}
			pResponse->strContent += "</select>\n</td>\n</tr>\n";
			ucAccess = m_ucAccessAdmin;
			pResponse->strContent += "<tr class=\"admin_access\"><th class=\"admin_access\">" + gmapLANG_CFG["set_access_user_detail"] + "</th><td><select name=\"user_detail\">\n";
			while( ucAccess > 0 )
			{
				pResponse->strContent += "<option value=\"" + CAtomInt( ucAccess ).toString( ) + "\"";
				if( ucAccess == m_ucAccessUserDetails )
					pResponse->strContent += " selected";
				pResponse->strContent += ">" + UTIL_AccessToText( ucAccess ) + "</option>\n";
				ucAccess = ucAccess >> 1;
			}
			pResponse->strContent += "</select>\n</td>\n</tr>\n";
			ucAccess = m_ucAccessAdmin;
			pResponse->strContent += "<tr class=\"admin_access\"><th class=\"admin_access\">" + gmapLANG_CFG["set_access_create_users"] + "</th><td><select name=\"create_users\">\n";
			while( ucAccess > 0 )
			{
				pResponse->strContent += "<option value=\"" + CAtomInt( ucAccess ).toString( ) + "\"";
				if( ucAccess == m_ucAccessCreateUsers )
					pResponse->strContent += " selected";
				pResponse->strContent += ">" + UTIL_AccessToText( ucAccess ) + "</option>\n";
				ucAccess = ucAccess >> 1;
			}
			pResponse->strContent += "</select>\n</td>\n</tr>\n";
			if( pRequest->user.ucAccess & m_ucAccessEditAdmins )
			{
				ucAccess = ACCESS_LEADER;
				pResponse->strContent += "<tr class=\"admin_access\"><th class=\"admin_access\">" + gmapLANG_CFG["set_access_edit_users"] + "</th><td><select name=\"edit_users\">\n";
				while( ucAccess > 0 )
				{
					pResponse->strContent += "<option value=\"" + CAtomInt( ucAccess ).toString( ) + "\"";
					if( ucAccess == m_ucAccessEditUsers )
						pResponse->strContent += " selected";
					pResponse->strContent += ">" + UTIL_AccessToText( ucAccess ) + "</option>\n";
					ucAccess = ucAccess >> 1;
				}
				pResponse->strContent += "</select>\n</td>\n</tr>\n";
			}
			if( pRequest->user.ucAccess & m_ucAccessEditAdmins )
			{
				ucAccess = ACCESS_LEADER;
				pResponse->strContent += "<tr class=\"admin_access\"><th class=\"admin_access\">" + gmapLANG_CFG["set_access_del_users"] + "</th><td><select name=\"del_users\">\n";
				while( ucAccess > 0 )
				{
					pResponse->strContent += "<option value=\"" + CAtomInt( ucAccess ).toString( ) + "\"";
					if( ucAccess == m_ucAccessDelUsers )
						pResponse->strContent += " selected";
					pResponse->strContent += ">" + UTIL_AccessToText( ucAccess ) + "</option>\n";
					ucAccess = ucAccess >> 1;
				}
				pResponse->strContent += "</select>\n</td>\n</tr>\n";
			}
			ucAccess = m_ucAccessAdmin;
			pResponse->strContent += "<tr class=\"admin_access\"><th class=\"admin_access\">" + gmapLANG_CFG["set_access_invites"] + "</th><td><select name=\"invites\">\n";
			while( ucAccess > 0 )
			{
				pResponse->strContent += "<option value=\"" + CAtomInt( ucAccess ).toString( ) + "\"";
				if( ucAccess == m_ucAccessInvites )
					pResponse->strContent += " selected";
				pResponse->strContent += ">" + UTIL_AccessToText( ucAccess ) + "</option>\n";
				ucAccess = ucAccess >> 1;
			}
			pResponse->strContent += "</select>\n</td>\n</tr>\n";
			ucAccess = m_ucAccessAdmin;
			pResponse->strContent += "<tr class=\"admin_access\"><th class=\"admin_access\">" + gmapLANG_CFG["set_access_trade_invites"] + "</th><td><select name=\"trade_invites\">\n";
			while( ucAccess > 0 )
			{
				pResponse->strContent += "<option value=\"" + CAtomInt( ucAccess ).toString( ) + "\"";
				if( ucAccess == m_ucAccessTradeInvites )
					pResponse->strContent += " selected";
				pResponse->strContent += ">" + UTIL_AccessToText( ucAccess ) + "</option>\n";
				ucAccess = ucAccess >> 1;
			}
			pResponse->strContent += "</select>\n</td>\n</tr>\n";
			ucAccess = m_ucAccessAdmin;
			pResponse->strContent += "<tr class=\"admin_access\"><th class=\"admin_access\">" + gmapLANG_CFG["set_access_messages"] + "</th><td><select name=\"messages\">\n";
			while( ucAccess > 0 )
			{
				pResponse->strContent += "<option value=\"" + CAtomInt( ucAccess ).toString( ) + "\"";
				if( ucAccess == m_ucAccessMessages )
					pResponse->strContent += " selected";
				pResponse->strContent += ">" + UTIL_AccessToText( ucAccess ) + "</option>\n";
				ucAccess = ucAccess >> 1;
			}
			pResponse->strContent += "</select>\n</td>\n</tr>\n";
			ucAccess = m_ucAccessAdmin;
			pResponse->strContent += "<tr class=\"admin_access\"><th class=\"admin_access\">" + gmapLANG_CFG["set_access_view_log"] + "</th><td><select name=\"view_log\">\n";
			while( ucAccess > 0 )
			{
				pResponse->strContent += "<option value=\"" + CAtomInt( ucAccess ).toString( ) + "\"";
				if( ucAccess == m_ucAccessViewLog )
					pResponse->strContent += " selected";
				pResponse->strContent += ">" + UTIL_AccessToText( ucAccess ) + "</option>\n";
				ucAccess = ucAccess >> 1;
			}
			pResponse->strContent += "</select>\n</td>\n</tr>\n";
			ucAccess = m_ucAccessAdmin;
			pResponse->strContent += "<tr class=\"admin_access\"><th class=\"admin_access\">" + gmapLANG_CFG["set_access_view_xstates"] + "</th><td><select name=\"view_xstates\">\n";
			while( ucAccess > 0 )
			{
				pResponse->strContent += "<option value=\"" + CAtomInt( ucAccess ).toString( ) + "\"";
				if( ucAccess == m_ucAccessViewXStates )
					pResponse->strContent += " selected";
				pResponse->strContent += ">" + UTIL_AccessToText( ucAccess ) + "</option>\n";
				ucAccess = ucAccess >> 1;
			}
			pResponse->strContent += "</select>\n</td>\n</tr>\n";
			ucAccess = m_ucAccessAdmin;
			pResponse->strContent += "<tr class=\"admin_access\"><th class=\"admin_access\">" + gmapLANG_CFG["set_access_rss"] + "</th><td><select name=\"rss\">\n";
			while( ucAccess > 0 )
			{
				pResponse->strContent += "<option value=\"" + CAtomInt( ucAccess ).toString( ) + "\"";
				if( ucAccess == m_ucAccessRSS )
					pResponse->strContent += " selected";
				pResponse->strContent += ">" + UTIL_AccessToText( ucAccess ) + "</option>\n";
				ucAccess = ucAccess >> 1;
			}
			pResponse->strContent += "</select>\n</td>\n</tr>\n";
			ucAccess = m_ucAccessAdmin;
			pResponse->strContent += "<tr class=\"admin_access\"><th class=\"admin_access\">" + gmapLANG_CFG["set_access_dump_xml"] + "</th><td><select name=\"dump_xml\">\n";
			while( ucAccess > 0 )
			{
				pResponse->strContent += "<option value=\"" + CAtomInt( ucAccess ).toString( ) + "\"";
				if( ucAccess == m_ucAccessDumpXML )
					pResponse->strContent += " selected";
				pResponse->strContent += ">" + UTIL_AccessToText( ucAccess ) + "</option>\n";
				ucAccess = ucAccess >> 1;
			}
			pResponse->strContent += "</select>\n</td>\n</tr>\n";
			ucAccess = m_ucAccessAdmin;
			pResponse->strContent += "<tr class=\"admin_access\"><th class=\"admin_access\">" + gmapLANG_CFG["set_access_sort_ip"] + "</th><td><select name=\"sort_ip\">\n";
			while( ucAccess > 0 )
			{
				pResponse->strContent += "<option value=\"" + CAtomInt( ucAccess ).toString( ) + "\"";
				if( ucAccess == m_ucAccessSortIP )
					pResponse->strContent += " selected";
				pResponse->strContent += ">" + UTIL_AccessToText( ucAccess ) + "</option>\n";
				ucAccess = ucAccess >> 1;
			}
			pResponse->strContent += "</select>\n</td>\n</tr>\n";
			ucAccess = m_ucAccessAdmin;
			pResponse->strContent += "<tr class=\"admin_access\"><th class=\"admin_access\">" + gmapLANG_CFG["set_access_show_ip"] + "</th><td><select name=\"show_ip\">\n";
			while( ucAccess > 0 )
			{
				pResponse->strContent += "<option value=\"" + CAtomInt( ucAccess ).toString( ) + "\"";
				if( ucAccess == m_ucAccessShowIP )
					pResponse->strContent += " selected";
				pResponse->strContent += ">" + UTIL_AccessToText( ucAccess ) + "</option>\n";
				ucAccess = ucAccess >> 1;
			}
			pResponse->strContent += "</select>\n</td>\n</tr>\n";
			if( pRequest->user.ucAccess & m_ucAccessEditAdmins )
			{
				ucAccess = ACCESS_LEADER;
				pResponse->strContent += "<tr class=\"admin_access\"><th class=\"admin_access\">" + gmapLANG_CFG["set_access_admin"] + "</th><td><select name=\"admin\">\n";
				while( ucAccess > 0 )
				{
					pResponse->strContent += "<option value=\"" + CAtomInt( ucAccess ).toString( ) + "\"";
					if( ucAccess == m_ucAccessAdmin )
						pResponse->strContent += " selected";
					pResponse->strContent += ">" + UTIL_AccessToText( ucAccess ) + "</option>\n";
					ucAccess = ucAccess >> 1;
				}
				pResponse->strContent += "</select>\n</td>\n</tr>\n";
			}
			if( pRequest->user.ucAccess & m_ucAccessEditAdmins )
			{
				ucAccess = ACCESS_LEADER;
				pResponse->strContent += "<tr class=\"admin_access\"><th class=\"admin_access\">" + gmapLANG_CFG["set_access_edit_admins"] + "</th><td><select name=\"edit_admins\">\n";
				while( ucAccess > 0 )
				{
					pResponse->strContent += "<option value=\"" + CAtomInt( ucAccess ).toString( ) + "\"";
					if( ucAccess == m_ucAccessEditAdmins )
						pResponse->strContent += " selected";
					pResponse->strContent += ">" + UTIL_AccessToText( ucAccess ) + "</option>\n";
					ucAccess = ucAccess >> 1;
				}
				pResponse->strContent += "</select>\n</td>\n</tr>\n";
			}
			if( pRequest->user.ucAccess & m_ucAccessEditAdmins )
			{
				ucAccess = ACCESS_LEADER;
				pResponse->strContent += "<tr class=\"admin_access\"><th class=\"admin_access\">" + gmapLANG_CFG["set_access_signup"] + "</th><td><select name=\"signup\">\n";
				while( ucAccess > 0 )
				{
					pResponse->strContent += "<option value=\"" + CAtomInt( ucAccess ).toString( ) + "\"";
					if( ucAccess == m_ucAccessSignup )
						pResponse->strContent += " selected";
					pResponse->strContent += ">" + UTIL_AccessToText( ucAccess ) + "</option>\n";
					ucAccess = ucAccess >> 1;
				}
				pResponse->strContent += "</select>\n</td>\n</tr>\n";
			}
			if( pRequest->user.ucAccess & m_ucAccessEditAdmins )
			{
				ucAccess = ACCESS_LEADER;
				pResponse->strContent += "<tr class=\"admin_access\"><th class=\"admin_access\">" + gmapLANG_CFG["set_access_signup_direct"] + "</th><td><select name=\"signup_direct\">\n";
				while( ucAccess > 0 )
				{
					pResponse->strContent += "<option value=\"" + CAtomInt( ucAccess ).toString( ) + "\"";
					if( ucAccess == m_ucAccessSignupDirect )
						pResponse->strContent += " selected";
					pResponse->strContent += ">" + UTIL_AccessToText( ucAccess ) + "</option>\n";
					ucAccess = ucAccess >> 1;
				}
				pResponse->strContent += "</select>\n</td>\n</tr>\n";
			}
			pResponse->strContent += "</table>\n";
			pResponse->strContent += Button_Submit( "submit_access", string( gmapLANG_CFG["Submit"] ) );
			pResponse->strContent += "</form></div>";
		}
		else if( cstrAdminFunction == "config" )
		{
			pResponse->strContent += "<table class=\"admin_function\">\n";

			pResponse->strContent += "<tr class=\"admin_function\">\n";
			pResponse->strContent += "<td class=\"admin_function\">\n";
			pResponse->strContent += "<div class=\"admin_invite\">\n";
			pResponse->strContent += "<form method=\"get\" action=\"" + RESPONSE_STR_ADMIN_HTML + "\">\n";
			pResponse->strContent += "<p>" + gmapLANG_CFG["admin_new_school"] + "</p>\n\n" ;
			pResponse->strContent += "<p class=\"admin_school\">" + gmapLANG_CFG["admin_new_school_mail"] + "<input name=\"school_mail\" type=text size=15 value=\"\"></p>";
			pResponse->strContent += "<p class=\"admin_school\">" + gmapLANG_CFG["admin_new_school_name"] + "<input name=\"school_name\" type=text size=15 value=\"\"></p>";
			pResponse->strContent += Button_Submit( "submit_school", string( gmapLANG_CFG["Submit"] ) );
			pResponse->strContent += "</form></div>\n";
			pResponse->strContent += "</form></td></tr>\n";
			
			pResponse->strContent += "<tr class=\"admin_function\">\n";
			pResponse->strContent += "<td class=\"admin_function\">\n";
			pResponse->strContent += "<div class=\"admin_invite\">\n";
			pResponse->strContent += "<form method=\"get\" action=\"" + RESPONSE_STR_ADMIN_HTML + "\">\n";
			pResponse->strContent += "<p class=\"admin_invite\"><input id=\"id_invite\" name=\"invite\" type=checkbox";
		
			if( CFG_GetInt( "bnbt_invite_enable", 0 ) == 0 ? false : true )
				pResponse->strContent += " checked";
			
			pResponse->strContent += "><label for=\"id_invite\">" + gmapLANG_CFG["admin_invite_enable"] + "</label>";
			pResponse->strContent += Button_Submit( "submit_invite", string( gmapLANG_CFG["Submit"] ) );
			pResponse->strContent += "</p>";
			pResponse->strContent += "</form></div>\n";
			pResponse->strContent += "</form></td></tr>\n";
			
			string strBonusTradeUploadRate = CFG_GetString( "bnbt_bonus_trade_upload_rate", "500" );
			string strBonusTradeInviteRate = CFG_GetString( "bnbt_bonus_trade_invite_rate", "50000" );
			
			pResponse->strContent += "<tr class=\"admin_function\">\n";
			pResponse->strContent += "<td class=\"admin_function\">\n";
			pResponse->strContent += "<div class=\"admin_trade\">\n";
			pResponse->strContent += "<form method=\"get\" action=\"" + RESPONSE_STR_ADMIN_HTML + "\">\n";
			pResponse->strContent += "<p class=\"admin_trade\"><input id=\"id_trade\" name=\"trade\" type=checkbox";
		
			if( CFG_GetInt( "bnbt_bonus_trade_enable", 0 ) == 0 ? false : true )
				pResponse->strContent += " checked";
			
			pResponse->strContent += "><label for=\"id_trade\">" + gmapLANG_CFG["admin_trade_enable"] + "</label>";
			pResponse->strContent += Button_Submit( "submit_bonus_trade", string( gmapLANG_CFG["Submit"] ) );
			pResponse->strContent += "</p>";
			pResponse->strContent += "<p class=\"admin_trade\"><input name=\"trade_upload_rate\" type=text size=5 maxlength=3 value=\"" + strBonusTradeUploadRate + "\"> / GB</p>";
			pResponse->strContent += "<p class=\"admin_trade\"><input name=\"trade_invite_rate\" type=text size=5 maxlength=5 value=\"" + strBonusTradeInviteRate + "\"> / Invite</p>";
			pResponse->strContent += "</form></div>\n";
			pResponse->strContent += "</form></td></tr>\n";
			
			string strGiftRule = CFG_GetString( "bnbt_new_user_gift_rule", string( ) );
			string strGiftUploaded = CFG_GetString( "bnbt_new_user_gift_uploaded", "0" );
			string strGiftDownloaded = CFG_GetString( "bnbt_new_user_gift_downloaded", "0" );
			string strGiftBonus = CFG_GetString( "bnbt_new_user_gift_bonus", "0" );
			
			pResponse->strContent += "<tr class=\"admin_function\">\n";
			pResponse->strContent += "<td class=\"admin_function\">\n";
			pResponse->strContent += "<div class=\"admin_new_user_gift\">\n";
			pResponse->strContent += "<form method=\"get\" action=\"" + RESPONSE_STR_ADMIN_HTML + "\">\n";
			pResponse->strContent += "<p>" + gmapLANG_CFG["admin_new_user_gift"] + "</p>\n\n" ;
			pResponse->strContent += "<p><input id=\"id_gift_enable\" name=\"gift\" type=checkbox";
		
			if( CFG_GetInt( "bnbt_new_user_gift_enable", 0 ) == 0 ? false : true )
				pResponse->strContent += " checked";
			
			pResponse->strContent += "><label for=\"id_gift_enable\">" + gmapLANG_CFG["admin_new_user_gift_enable"] + "</label></p>";
			pResponse->strContent += "<p class=\"admin_add_gift\">" + gmapLANG_CFG["admin_new_user_gift_rule"] + "<input name=\"gift_rule\" type=text size=15 value=\"" + strGiftRule + "\"></p>";
			pResponse->strContent += "<p class=\"admin_add_gift\"><input id=\"id_gift_uploaded\" name=\"gift_uploaded\" type=text size=20 value=\"";
			pResponse->strContent += strGiftUploaded + "\">(GB)" + gmapLANG_CFG["user_uploaded"] + "</p>\n";
			pResponse->strContent += "<p class=\"admin_add_gift\"><input id=\"id_gift_downloaded\" name=\"gift_downloaded\" type=text size=20 value=\"";
			pResponse->strContent += strGiftDownloaded + "\">(GB)" + gmapLANG_CFG["user_downloaded"] + "</p>\n";
			pResponse->strContent += "<p class=\"admin_add_gift\"><input id=\"id_gift_bonus\" name=\"gift_bonus\" type=text size=20 value=\"";
			pResponse->strContent += strGiftBonus + "\">" + gmapLANG_CFG["user_bonus"] + "</p>\n";
			pResponse->strContent += Button_Submit( "submit_new_user_gift", string( gmapLANG_CFG["Submit"] ) );
			pResponse->strContent += "</form>\n";
			pResponse->strContent += "\n</div>\n";
			pResponse->strContent += "</td></tr>\n";
			
			pResponse->strContent += "<tr class=\"admin_function\">\n";
			pResponse->strContent += "<td class=\"admin_function\">\n";
			pResponse->strContent += "<div class=\"admin_add_gift\">\n";
			pResponse->strContent += "<form method=\"get\" action=\"" + RESPONSE_STR_ADMIN_HTML + "\">\n";
			pResponse->strContent += "<p>" + gmapLANG_CFG["admin_add_gift"] + "</p>\n\n" ;
			pResponse->strContent += "<p class=\"admin_add_gift\">" + gmapLANG_CFG["users_editing_user_add"] + "<input id=\"id_add_uploaded\" name=\"add_uploaded\" type=text size=20";
			pResponse->strContent += " value=\"0\">(GB)" + gmapLANG_CFG["user_uploaded"] + "</p>\n";
			pResponse->strContent += "<p class=\"admin_add_gift\">" + gmapLANG_CFG["users_editing_user_add"] + "<input id=\"id_add_downloaded\" name=\"add_downloaded\" type=text size=20";
			pResponse->strContent += " value=\"0\">(GB)" + gmapLANG_CFG["user_downloaded"] + "</p>\n";
			pResponse->strContent += "<p class=\"admin_add_gift\">" + gmapLANG_CFG["users_editing_user_add"] + "<input id=\"id_add_bonus\" name=\"add_bonus\" type=text size=20";
			pResponse->strContent += " value=\"0\">" + gmapLANG_CFG["user_bonus"] + "</p>\n";
			pResponse->strContent += Button_Submit( "submit_add_gift", string( gmapLANG_CFG["Submit"] ) );
			pResponse->strContent += "</form>\n";
			pResponse->strContent += "\n</div>\n";
			pResponse->strContent += "</td></tr>\n";
			
			pResponse->strContent += "</table>\n";
		}
		else if( cstrAdminFunction == "recycle" )
		{
			const string cstrRecycleAction( pRequest->mapParams["action"] );
			const string cstrRecycleID( pRequest->mapParams["id"] );
			const string cstrOK( pRequest->mapParams["ok"] );

			const string cstrPerPage( pRequest->mapParams["per_page"] );
			unsigned long ulStart = 0;
			unsigned int uiOverridePerPage = 0;

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

			const string cstrPage( pRequest->mapParams["page"] );

			if( !cstrPage.empty( ) )
				ulStart = (unsigned long)atoi( cstrPage.c_str( ) ) * uiOverridePerPage;

			if( cstrRecycleAction.empty( ) )
			{
				unsigned long ulKeySize = 0;
				unsigned long ulAdded = 0;
				unsigned long ulSkipped = 0;

				vector< pair< string, string > > vecParams;
				vecParams.reserve(64);
				string strJoined = string( );
				string strReturn = string( );
				
				vecParams.push_back( pair<string, string>( string( "func" ), cstrAdminFunction ) );
				vecParams.push_back( pair<string, string>( string( "per_page" ), cstrPerPage ) );

				strJoined = UTIL_RemoveHTML( UTIL_HTMLJoin( vecParams, string( "&" ), string( "&" ), string( "=" ) ) );

				vecParams.push_back( pair<string, string>( string( "page" ), cstrPage ) );
				strReturn = RESPONSE_STR_ADMIN_HTML + UTIL_HTMLJoin( vecParams, string( "?" ), string( "&" ), string( "=" ) );

				string strEngName = string( );
				string strChiName = string( );

				CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bid,bname,badded,bsize,btag,btitle,buploader,buploaderid FROM allowed_archive ORDER BY badded DESC" );
				
				vector<string> vecQuery;
				vecQuery.reserve(8);
				
				vecQuery = pQuery->nextRow( );

				ulKeySize = (unsigned long)pQuery->numRows( );

				if( vecQuery.size( ) == 8 )
				{
					pResponse->strContent += UTIL_PageBar( ulKeySize, cstrPage, uiOverridePerPage, RESPONSE_STR_ADMIN_HTML, strJoined, true );

					pResponse->strContent += "<table class=\"user_detail_table\" id=\"user_seeding\">\n";

					pResponse->strContent += "<tr>\n";

					if( !m_vecTags.empty( ) )
					{
						pResponse->strContent += "<th class=\"tag\">" + gmapLANG_CFG["tag"];
						pResponse->strContent += "</th>\n";
					}
					
					// Name
					pResponse->strContent += "<th class=\"name\">" + gmapLANG_CFG["name"];

					pResponse->strContent += "</th>\n";
					
					// <th> added

					pResponse->strContent += "<th class=\"date\">" + gmapLANG_CFG["added"];

					pResponse->strContent += "</th>\n";

					// <th> size

					pResponse->strContent += "<th class=\"bytes\">" + gmapLANG_CFG["size"];

					pResponse->strContent += "</th>\n";
					
					// <th> size

					pResponse->strContent += "<th class=\"uploader\">" + gmapLANG_CFG["uploader"];

					pResponse->strContent += "</th>\n";

					// Admin
					pResponse->strContent += "<th class=\"admin\">" + gmapLANG_CFG["admin"] + "</th>\n";

					pResponse->strContent += "</tr>\n";

					while( vecQuery.size( ) == 8 )
					{
						if( ulAdded < uiOverridePerPage )
						{
							if( ulSkipped == ulStart )
							{
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

										if( strNameIndex == vecQuery[4] )
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

								pResponse->strContent += "<td class=\"name\">";
								strEngName.erase( );
								strChiName.erase( );
								UTIL_StripName( vecQuery[5].c_str( ), strEngName, strChiName );
								pResponse->strContent += "<span class=\"stats_bold\">" + UTIL_RemoveHTML( strEngName ) + "</span>";
								if( !strChiName.empty( ) )
									pResponse->strContent += "<br><span class=\"stats\">" + UTIL_RemoveHTML( strChiName ) + "</span>";
								pResponse->strContent += "</td>\n";
										
								pResponse->strContent += "<td class=\"date\">";

								if( !vecQuery[2].empty( ) )
								{
									const string :: size_type br = vecQuery[2].find( ' ' );
									pResponse->strContent += vecQuery[2].substr( 0, br );
									if( br != string :: npos )
										pResponse->strContent += "<br>" + vecQuery[2].substr( br + 1 );
									// strip year and seconds from time
								}

								pResponse->strContent += "</td>\n";

								int64 iSize = UTIL_StringTo64( vecQuery[3].c_str( ) );

								const string :: size_type br = UTIL_BytesToString( iSize ).find( ' ' );
								pResponse->strContent += "<td class=\"bytes\">" + UTIL_BytesToString( iSize ).substr( 0, br );
								if( br != string :: npos )
									pResponse->strContent += "<br>" + UTIL_BytesToString( iSize ).substr( br + 1 );
								pResponse->strContent += "</td>\n";
								pResponse->strContent += "<td class=\"uploader\">" + getUserLink( vecQuery[7], vecQuery[6] ) + "</td>\n";
								pResponse->strContent += "<td class=\"admin\">";
								pResponse->strContent += "[<a class=\"blue\" href=\"" + RESPONSE_STR_ADMIN_HTML + "?func=recycle&amp;action=recover&amp;id=" + vecQuery[0];
								pResponse->strContent += "&amp;return=" + UTIL_RemoveHTML( UTIL_StringToEscaped( strReturn ) );
								pResponse->strContent += "\">" + gmapLANG_CFG["recycle_recover"] + "</a>]<br>";
								pResponse->strContent += "[<a class=\"red\" href=\"" + RESPONSE_STR_ADMIN_HTML + "?func=recycle&amp;action=delete&amp;id=" + vecQuery[0];
								pResponse->strContent += "&amp;return=" + UTIL_RemoveHTML( UTIL_StringToEscaped( strReturn ) );
								pResponse->strContent += "\">" + gmapLANG_CFG["recycle_delete"] + "</a>]</td>\n";

								pResponse->strContent += "</tr>\n";
								ulAdded++;
							}
							else
								ulSkipped++;
						}
						else
							break;

						vecQuery = pQuery->nextRow( );
					}
					pResponse->strContent += "</table>\n";

					pResponse->strContent += UTIL_PageBar( ulKeySize, cstrPage, uiOverridePerPage, RESPONSE_STR_ADMIN_HTML, strJoined, false );
				}

				delete pQuery;
			}
			else if( !cstrRecycleID.empty( ) )
			{
				string strReturnPage( pRequest->mapParams["return"] );
				
				if( strReturnPage.empty( ) )
					strReturnPage = RESPONSE_STR_ADMIN_HTML + "?func=recycle";

				if( cstrOK == "1" )
				{
					bool bSuccess = false;

					CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bid,bfilename FROM allowed_archive WHERE bid=" + cstrRecycleID );
				
					vector<string> vecQuery;
				
					vecQuery.reserve(2);

					vecQuery = pQuery->nextRow( );
					
					delete pQuery;
					
					if( vecQuery.size( ) == 2 && !vecQuery[1].empty( ) )
					{
						string strFileName = vecQuery[1];

						if( cstrRecycleAction == "recover" )
						{
							if( !strFileName.empty( ) )
							{
								if( !m_strArchiveDir.empty( ) && UTIL_CheckDir( m_strArchiveDir.c_str( ) ) )
								{
									if( UTIL_CheckFile( string( m_strArchiveDir + cstrRecycleID + ".torrent" ).c_str( ) ) && !UTIL_CheckFile( string( m_strAllowedDir + strFileName ).c_str( ) ) )
									{
										UTIL_MoveFile( string( m_strArchiveDir + cstrRecycleID + ".torrent" ).c_str( ), string( m_strAllowedDir + strFileName ).c_str( ) );
										CMySQLQuery mq00( "INSERT INTO allowed (SELECT * FROM allowed_archive WHERE bid=" + cstrRecycleID + ")" );
										CMySQLQuery mq01( "DELETE FROM allowed_archive WHERE bid=" + cstrRecycleID );

										m_pCache->addRow( cstrRecycleID, false );

										UTIL_LogFilePrint( "recycleTorrent: %s recovered torrent %s\n", pRequest->user.strLogin.c_str( ), strFileName.c_str( ) );

										bSuccess = true;
									}
								}
							}
						}
						else if( cstrRecycleAction == "delete" )
						{
							if( !m_strArchiveDir.empty( ) && UTIL_CheckDir( m_strArchiveDir.c_str( ) ) )
							{
								if( UTIL_CheckFile( string( m_strArchiveDir + cstrRecycleID + ".torrent" ).c_str( ) ) )
								{
									UTIL_DeleteFile( string( m_strArchiveDir + cstrRecycleID + ".torrent" ).c_str( ) );

									CMySQLQuery mq01( "DELETE FROM allowed_archive WHERE bid=" + cstrRecycleID );
									CMySQLQuery mq02( "DELETE FROM dstate WHERE bid=" + cstrRecycleID );
									CMySQLQuery mq03( "DELETE FROM dstate_store WHERE bid=" + cstrRecycleID );
									CMySQLQuery mq04( "DELETE FROM peers WHERE bid=" + cstrRecycleID );
									CMySQLQuery mq05( "DELETE FROM statistics WHERE bid=" + cstrRecycleID );
									CMySQLQuery mq06( "DELETE FROM bookmarks WHERE bid=" + cstrRecycleID );
									CMySQLQuery mq07( "DELETE FROM thanks WHERE bid=" + cstrRecycleID );
									CMySQLQuery mq08( "DELETE FROM talktorrent WHERE btid=" + cstrRecycleID );
									CMySQLQuery mq09( "DELETE FROM talkrequest WHERE btid=" + cstrRecycleID );
									CMySQLQuery mq10( "DELETE FROM comments WHERE btid=" + cstrRecycleID );

									UTIL_LogFilePrint( "recycleTorrent: %s permanently deleted torrent %s\n", pRequest->user.strLogin.c_str( ), strFileName.c_str( ) );

									bSuccess = true;
								}
							}
						}
					}

					if( bSuccess )
						pResponse->strContent += "<p class=\"deleted\">" + UTIL_Xsprintf( gmapLANG_CFG["recycle_action_succeed"].c_str( ), cstrRecycleID.c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_admin"] + "\" href=\"" + strReturnPage + "\">" ).c_str( ), "</a>" ) + "</p>\n";
					else
						pResponse->strContent += "<p class=\"deleted\">" + UTIL_Xsprintf( gmapLANG_CFG["recycle_action_failed"].c_str( ), cstrRecycleID.c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_admin"] + "\" href=\"" + strReturnPage + "\">" ).c_str( ), "</a>" ) + "</p>\n";
				}
				else
				{
					if( cstrRecycleAction == "recover" )
					{
						pResponse->strContent += "<form name=\"recovertorrent\" method=\"get\" action=\"" + string( RESPONSE_STR_ADMIN_HTML ) + "\" onSubmit=\"return validate( this )\">";
						pResponse->strContent += "<div class=\"torrent_delete\">\n";
						pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["recover_torrent_q"].c_str( ), cstrRecycleID.c_str( ) ) + "</p>\n";
						pResponse->strContent += "<p class=\"delete\"><input name=\"func\" type=hidden value=\"recycle\"></p>\n";
						pResponse->strContent += "<p class=\"delete\"><input name=\"action\" type=hidden value=\"" + cstrRecycleAction + "\"></p>\n";
						pResponse->strContent += "<p class=\"delete\"><input name=\"id\" type=hidden value=\"" + cstrRecycleID + "\"></p>\n";
						pResponse->strContent += "<p class=\"delete\"><input name=\"ok\" type=hidden value=\"1\"></p>\n";
						pResponse->strContent += "<p class=\"delete\"><input name=\"return\" type=hidden value=\"" + strReturnPage + "\"></p>\n";
						pResponse->strContent += "<div>\n";
						pResponse->strContent += Button_Submit( "submit_recover", string( gmapLANG_CFG["yes"] ) );
						pResponse->strContent += Button_Back( "cancel_recover", string( gmapLANG_CFG["no"] ) );
						pResponse->strContent += "\n</div>\n</div></form>\n";
					}
					else if( cstrRecycleAction == "delete" ) 
					{
						pResponse->strContent += "<form name=\"deletetorrent\" method=\"get\" action=\"" + string( RESPONSE_STR_ADMIN_HTML ) + "\" onSubmit=\"return validate( this )\">";
						pResponse->strContent += "<div class=\"torrent_delete\">\n";
						pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["delete_torrent_q"].c_str( ), cstrRecycleID.c_str( ) ) + "</p>\n";
						pResponse->strContent += "<p class=\"delete\"><input name=\"func\" type=hidden value=\"recycle\"></p>\n";
						pResponse->strContent += "<p class=\"delete\"><input name=\"action\" type=hidden value=\"" + cstrRecycleAction + "\"></p>\n";
						pResponse->strContent += "<p class=\"delete\"><input name=\"id\" type=hidden value=\"" + cstrRecycleID + "\"></p>\n";
						pResponse->strContent += "<p class=\"delete\"><input name=\"ok\" type=hidden value=\"1\"></p>\n";
						pResponse->strContent += "<p class=\"delete\"><input name=\"return\" type=hidden value=\"" + strReturnPage + "\"></p>\n";
						pResponse->strContent += "<div>\n";
						pResponse->strContent += Button_Submit( "submit_delete", string( gmapLANG_CFG["yes"] ) );
						pResponse->strContent += Button_Back( "cancel_delete", string( gmapLANG_CFG["no"] ) );
						pResponse->strContent += "\n</div>\n</div></form>\n";
					}
				}
			}
		}
		else if( cstrAdminFunction == "stat" )
		{

			//
			// tracker links
			//

	// 		pResponse->strContent += "<div class=\"admin_table\">\n";
	// 		pResponse->strContent += "<table summary=\"tlink\">\n";
	// 		
	// 		if( gpLink )
	// 		{
	// 			pResponse->strContent += "<tr><th colspan=1>" + gmapLANG_CFG["admin_tracker_links"] + "</th>\n";
	// 			pResponse->strContent += "<th colspan=1>";
	// 			pResponse->strContent += Button_JS_Link( "confirm_relink", gmapLANG_CFG["reset"], "relink_confirm()" );
	// 			pResponse->strContent += "</th></tr>\n";
	// 			pResponse->strContent += "<tr><th>" + gmapLANG_CFG["established"] + "</th>";
	// 			pResponse->strContent += "<td class=\"tracker_connection\">" + gtXStats.date.sLinkEstablished + "</td></tr>\n";
	// 		}
	// 		else
	// 		{
	// 			pResponse->strContent += "<tr><th colspan=2>" + gmapLANG_CFG["admin_tracker_links"] + "</th></tr>\n";
	// 		}
	// 
	// 		pResponse->strContent += "<tr><th>" + gmapLANG_CFG["type"] + "</th>";
	// 
	// 		if( gpLinkServer )
	// 		{
	// 			pResponse->strContent += "<td class=\"tracker_link_type\">" + gmapLANG_CFG["admin_primary_tracker"] + "</td></tr>\n";
	// 			pResponse->strContent += "<tr><th>" + gmapLANG_CFG["admin_connections"] + "</th><td class=\"tracker_connection\">";
	// 
	// 			gpLinkServer->m_mtxLinks.Claim( );
	// 
	// 			for( vector<CLinkClient *> :: iterator it = gpLinkServer->m_vecLinks.begin( ); it != gpLinkServer->m_vecLinks.end( ); it++ )
	// 			{
	// 				pResponse->strContent += (*it)->getName( );
	// 
	// 				if( (*it)->m_bActive )
	// 					pResponse->strContent += " " + gmapLANG_CFG["admin_active"];
	// 				else
	// 					pResponse->strContent += " " + gmapLANG_CFG["admin_not_active"];
	// 
	// 				if( it + 1 != gpLinkServer->m_vecLinks.end( ) )
	// 					pResponse->strContent += "<br>";
	// 			}
	// 
	// 			gpLinkServer->m_mtxLinks.Release( );
	// 
	// 			pResponse->strContent += "</td></tr>\n";
	// 		}
	// 		else if( gpLink )
	// 		{
	// 			pResponse->strContent += "<td class=\"tracker_link_type\">" + gmapLANG_CFG["admin_secondary_tracker"] + "</td></tr>\n";
	// 			pResponse->strContent += "<tr><th>" + gmapLANG_CFG["admin_connections"] + "</th><td class=\"tracker_connection\">" + gpLink->getName( ) + "</td></tr>\n";
	// 		}
	// 		else
	// 			pResponse->strContent += "<td class=\"tracker_connection\">" + gmapLANG_CFG["admin_no_link"] + "</td></tr>\n";
	// 
	// 		pResponse->strContent += "</table>\n</div>\n";

			//
			// tracker hub links
			//

	// 		pResponse->strContent += "<div class=\"admin_table\">\n";
	// 		pResponse->strContent += "<table summary=\"tlink\">\n";
	// 		
	// 		if( gpHUBLink )
	// 		{
	// 			pResponse->strContent += "<tr><th colspan=1>" + gmapLANG_CFG["admin_tracker_hub_links"] + "</th>\n";
	// 			pResponse->strContent += "<th colspan=1>";
	// 			pResponse->strContent += Button_JS_Link( "confirm_relink", gmapLANG_CFG["reset"], "relinkhub_confirm()" );
	// 			pResponse->strContent += "</th></tr>\n";
	// 
	// 			pResponse->strContent += "<tr><th>" + gmapLANG_CFG["established"] + "</th>";
	// 			pResponse->strContent += "<td class=\"tracker_connection\">" + gtXStats.date.sHubLinkEstablished + "</td></tr>\n";
	// 		}
	// 		else
	// 		{
	// 			pResponse->strContent += "<tr><th colspan=2>" + gmapLANG_CFG["admin_tracker_hub_links"] + "</th></tr>\n";
	// 		}
	// 
	// 		pResponse->strContent += "<tr><th>" + gmapLANG_CFG["type"] + "</th>";
	// 
	// 		if( gpHUBLinkServer )
	// 		{
	// 			pResponse->strContent += "<td class=\"tracker_link_type\">" + gmapLANG_CFG["admin_primary_tracker"] + "</td></tr>\n";
	// 			pResponse->strContent += "<tr><th>" + gmapLANG_CFG["admin_connections"] + "</th><td class=\"tracker_connection\">";
	// 
	// 			gpHUBLinkServer->m_mtxLinks.Claim( );
	// 
	// 			for( vector<CHUBLinkClient *> :: iterator it = gpHUBLinkServer->m_vecLinks.begin( ); it != gpHUBLinkServer->m_vecLinks.end( ); it++ )
	// 			{
	// 				pResponse->strContent += (*it)->getName( );
	// 
	// 				if( (*it)->m_bActive )
	// 					pResponse->strContent += " " + gmapLANG_CFG["admin_active"];
	// 				else
	// 					pResponse->strContent += " " + gmapLANG_CFG["admin_not_active"];
	// 
	// 				if( it + 1 != gpHUBLinkServer->m_vecLinks.end( ) )
	// 					pResponse->strContent += "<br>";
	// 			}
	// 
	// 			gpHUBLinkServer->m_mtxLinks.Release( );
	// 
	// 			pResponse->strContent += "</td></tr>\n";
	// 		}
	// 		else if( gpHUBLink )
	// 		{
	// 			pResponse->strContent += "<td class=\"tracker_link_type\">" + gmapLANG_CFG["admin_secondary_tracker"] + "</td></tr>\n";
	// 			pResponse->strContent += "<tr><th>" + gmapLANG_CFG["admin_connections"] + "</th><td class=\"tracker_connection\">" + gpHUBLink->getName( ) + "</td></tr>\n";
	// 		}
	// 		else
	// 			pResponse->strContent += "<td class=\"tracker_connection\">" + gmapLANG_CFG["admin_no_link"] + "</td></tr>\n";
	// 
	// 		pResponse->strContent += "</table>\n</div>\n";

			//
			// clients
			//

			const unsigned int cuiOnline( ( unsigned int )gpServer->m_vecClients.size( ) );

			pResponse->strContent += "<p class=\"users_online\">" + gmapLANG_CFG["admin_serving"] + " " + CAtomInt( cuiOnline ).toString( ) + " ";

// 			if ( cuiOnline > 1 )
				pResponse->strContent += gmapLANG_CFG["admin_serving_clients"];
// 			else
// 				pResponse->strContent += gmapLANG_CFG["admin_serving_client"];

			pResponse->strContent += "</p>\n";

			// Button List
			pResponse->strContent += "<p class=\"button_list\">";

			// count unique peers
			if( m_bCountUniquePeers )
			{
				CMySQLQuery *pQueryIP = new CMySQLQuery( "SELECT COUNT(*) FROM ips WHERE bcount>0" );
				
				vector<string> vecQueryIP;
	
				vecQueryIP.reserve(1);
		
				vecQueryIP = pQueryIP->nextRow( );
			
				delete pQueryIP;
				
				if( vecQueryIP.size( ) == 1 )
					pResponse->strContent += Button_JS_Link( "link_count", gmapLANG_CFG["admin_count_peers"] + " (" + vecQueryIP[0] + ")", "admin_count()" );
				else
					pResponse->strContent += Button_JS_Link( "link_count", gmapLANG_CFG["admin_count_peers"] + " (0)", "admin_count()" );
			}
			
			pResponse->strContent += Button_JS_Link( "link_refresh_static", gmapLANG_CFG["admin_refresh_static"], "admin_refresh_static()" );
			
			pResponse->strContent += Button_JS_Link( "link_cbtt_parse_list", gmapLANG_CFG["admin_cbtt_parse_list"], "admin_cbtt_parse_list()" );

			// RSS
			// addition by labarks
	// 		if( !rssdump.strName.empty( ) )
	// 			pResponse->strContent += Button_JS_Link( "link_rss", gmapLANG_CFG["admin_update_rss"], "admin_rss()" );

			// XML
	// 		if( !rssdump.strName.empty( ) )
	// 			pResponse->strContent += Button_JS_Link( "link_xml", gmapLANG_CFG["admin_update_xml"], "admin_xml()" );

			// Dynstat
	#if defined ( XBNBT_GD )
			if( m_bDynstatGenerate && !m_strDynstatFontFile.empty( ) && !m_strDynstatLinkURL.empty( ) )
				pResponse->strContent += Button_JS_Link( "link_dynstat", gmapLANG_CFG["admin_update_dynstat"], "admin_dynstat()" );
	#endif

			pResponse->strContent += "</p>\n";

			//
			// kill tracker
			//  

	// 		pResponse->strContent += "<p class=\"tracker_kill\">";
	// 		pResponse->strContent += Button_JS_Link( "confirm_kill", gmapLANG_CFG["admin_kill_tracker"], "kill_confirm()" );
	// 		pResponse->strContent += "</p>\n<p class=\"tracker_kill_warning\">" + gmapLANG_CFG["admin_kill_warning"] + "</p>\n";

		}

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_ADMIN ) );
	}
	else
	{
		// not authorized

		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["admin_page"], string( CSS_ADMIN ), string( ), NOT_INDEX, CODE_401 );

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_ADMIN ) );
	}
}

void CTracker :: serverResponseLanguage( struct request_t *pRequest, struct response_t *pResponse )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0)
		if( IsIPBanned( pRequest, pResponse, btv, gmapLANG_CFG["admin_page"], string( CSS_ADMIN ), NOT_INDEX ) )
			return;

	if( !pRequest->user.strUID.empty( ) && ( pRequest->user.ucAccess & m_ucAccessAdmin ) )
	{
		// submit?
		if( pRequest->mapParams["ap_submit_field"] == "1" )
		{
			if( !pRequest->mapParams["map"].empty( ) )
				LANG_CFG_SetString( pRequest->mapParams["map"], UTIL_EscapedToString( pRequest->mapParams["msg"] ) );
			
			LANG_CFG_Close( LANG_CFG_FILE );

			return JS_ReturnToPage( pRequest, pResponse, LANGUAGE_HTML );
		}
		
		// default?
		if( pRequest->mapParams["ap_default_field"] == "1" )
		{
			if( !pRequest->mapParams["map"].empty( ) )
				LANG_CFG_Delete( pRequest->mapParams["map"] );
			
			LANG_CFG_Close( LANG_CFG_FILE );
			gmapLANG_CFG.clear( );

			LANG_CFG_Init( LANG_CFG_FILE );

			return JS_ReturnToPage( pRequest, pResponse, LANGUAGE_HTML );
		}

		//
		// display the language page
		//

		// Output common HTML head
		HTML_Common_Begin( pRequest, pResponse, gmapLANG_CFG["admin_page"], string( CSS_ADMIN ), string( ), NOT_INDEX, CODE_200 );

		// Javascript
		pResponse->strContent += "<script type=\"text/javascript\">\n";
		pResponse->strContent += "<!--\n";
		pResponse->strContent += "function submit_language_confirm( ID, MAP )\n";
		pResponse->strContent += "{\n";
		pResponse->strContent += "var MSG=document.languageedit.elements[ID].value\n";
		pResponse->strContent += "var name=confirm(\"" + gmapLANG_CFG["Submit"] + "\\n\" + MAP + \"\\n\" + MSG)\n";
		pResponse->strContent += "if (name==true)\n";
		pResponse->strContent += "{\n";
	//	pResponse->strContent += "window.location=\"" + RESPONSE_STR_LANGUAGE_HTML + "?ap_submit_field=1&map=\" + MAP + \"&msg=\" + escape(MSG)\n";;
		pResponse->strContent += "window.location=\"" + RESPONSE_STR_LANGUAGE_HTML + "?ap_submit_field=1&map=\" + MAP + \"&msg=\" + MSG\n";;
		pResponse->strContent += "}\n";
		pResponse->strContent += "}\n\n";
		pResponse->strContent += "function default_language_confirm( MAP )\n";
		pResponse->strContent += "{\n";
		pResponse->strContent += "var name=confirm(\"Default\\n\" + MAP)\n";
		pResponse->strContent += "if (name==true)\n";
		pResponse->strContent += "{\n";
		pResponse->strContent += "window.location=\"" + RESPONSE_STR_LANGUAGE_HTML + "?ap_default_field=1&map=\" + MAP\n";;
		pResponse->strContent += "}\n";
		pResponse->strContent += "}\n\n";
		pResponse->strContent += "//-->\n";
		pResponse->strContent += "</script>\n\n";

		// table
		pResponse->strContent += "<div class=\"language_table\">\n";
		pResponse->strContent += "<form method=\"get\" name=\"languageedit\" onSubmit=\"return confirm(\'" + gmapLANG_CFG["Submit"] + "\')\" action=\"" + RESPONSE_STR_LANGUAGE_HTML + "\">\n";
		pResponse->strContent += "<table summary=\"xadminlanguage\">\n";

		pResponse->strContent += "<tr>\n";
		pResponse->strContent += "<th colspan=3>Admin Language</th>";
		pResponse->strContent += "</tr>\n";

		pResponse->strContent += "<tr>\n";
		pResponse->strContent += "<th colspan=1>Message ID</th>";
		pResponse->strContent += "<th colspan=1>Message</th>";
		pResponse->strContent += "<th colspan=1>Action</th>";
		pResponse->strContent += "</tr>\n";

		unsigned int uiRow = 0;
		unsigned int uiElement = 0;

		for( map<string, string>::iterator pos = gmapLANG_CFG.begin(); pos != gmapLANG_CFG.end(); pos++)
		{
			if( ( ++uiRow ) % 2 )
				pResponse->strContent += "<tr class=\"even\">\n";
			else
				pResponse->strContent += "<tr class=\"odd\">\n";

			pResponse->strContent += "<th>" + (*pos).first + "</th>\n";
			pResponse->strContent += "<td><span style=\"display:none\"><label for=\"id_" + (*pos).first + "\">" + (*pos).first + "</label></span><input name=\"name_" + (*pos).first + "\" id=\"id_" + (*pos).first + "\" type=text size=140 value=\"" + UTIL_RemoveHTML( (*pos).second ) + "\"></td>\n";

			pResponse->strContent += "<td>";
			pResponse->strContent += Button_JS_Link( (*pos).first + "_submit", gmapLANG_CFG["Submit"], "submit_language_confirm( " + CAtomInt( uiElement ).toString( ) + ", \'" + (*pos).first + "\' )" );
			pResponse->strContent += Button_Reset( (*pos).first + "_reset", gmapLANG_CFG["reset"] );
			pResponse->strContent += Button_JS_Link( (*pos).first + "_default", "Default", "default_language_confirm( \'" + (*pos).first + "\' )" );
			pResponse->strContent += "</td>\n";

			pResponse->strContent += "</tr>\n";

			uiElement += 4;
		}

		pResponse->strContent += "</table>\n</form>\n</div>\n\n";

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_ADMIN ) );
	}
	else
	{
		// not authorized

		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["admin_page"], string( CSS_ADMIN ), string( ), NOT_INDEX, CODE_401 );

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_ADMIN ) );
	}
}

#if defined ( XBNBT_GD )
// XBNBT Reset Dynstats Statistics
void CTracker :: resetXStatsDynstat( )
{
	gtXStats.dynstat.iFrozen = 0;
	gtXStats.dynstat.iProcessed = 0;
	gtXStats.dynstat.iRun = 0;
	gtXStats.dynstat.iTotalProcessed = 0;
	gtXStats.dynstat.sElapsedTime = string( );
	gtXStats.dynstat.sLastRunTime = string( );
	gtXStats.dynstat.sLastReset = UTIL_Date( );
}
#endif

// XBNBT Reset Announce Statistics
void CTracker :: resetXStatsAnnounce( )
{
	gtXStats.announce.iAnnounce = 0;
	gtXStats.announce.iAnnMissing = 0;
	gtXStats.announce.iAnnNotAuth = 0;
	gtXStats.announce.iCompact = 0;
	gtXStats.announce.iNopeerid = 0;
	gtXStats.announce.iRegular = 0;
	gtXStats.announce.iIPBanned = 0;
	gtXStats.announce.iIPNotCleared = 0;
	//gtXStats.announce.iIPNATBlocked = 0;
	//gtXStats.announce.iIPLocalOnly = 0;
	gtXStats.announce.iPeerSpoofRestrict = 0;
	gtXStats.announce.iClientBanned = 0;
	gtXStats.announce.iInvalidEvent = 0;
	gtXStats.announce.iPortMissing = 0;
	gtXStats.announce.iUploadedMissing = 0;
	gtXStats.announce.iDownloadedMissing = 0;
	gtXStats.announce.iLeftMissing = 0;
	gtXStats.announce.iPeerIDLength = 0;
	gtXStats.announce.iPortBlacklisted = 0;
	gtXStats.announce.iDownloadedInvalid = 0;
	gtXStats.announce.iUploadedInvalid = 0;
	gtXStats.announce.iNotAuthorized = 0;
	gtXStats.announce.sLastReset = UTIL_Date( );
}

// XBNBT Reset Scrape Statistics
void CTracker :: resetXStatsScrape( )
{
	gtXStats.scrape.iScrape = 0;
	gtXStats.scrape.iDisallowed = 0;
	gtXStats.scrape.iSingle = 0;
	gtXStats.scrape.iMultiple = 0;
	gtXStats.scrape.iFull = 0;
	gtXStats.scrape.iNotAuthorized = 0;
	gtXStats.scrape.sLastReset = UTIL_Date( );
}

// XBNBT Reset File Statistics
void CTracker :: resetXStatsFile( )
{
	gtXStats.file.iBarFill = 0;
	gtXStats.file.iBarTrans = 0;
	gtXStats.file.iCSS = 0;
	gtXStats.file.iFavicon = 0;
	gtXStats.file.uiFiles = 0;
	gtXStats.file.iRobots = 0;
	gtXStats.file.iRSS = 0;
	gtXStats.file.iRSSXSL = 0;
	gtXStats.file.iXML = 0;
	gtXStats.file.iTorrent = 0;
	gtXStats.file.sLastReset = UTIL_Date( );
}

// XBNBT Reset Page Statistics
void CTracker :: resetXStatsPage( )
{
	gtXStats.page.iAdmin = 0;
	gtXStats.page.uiComments = 0;
	gtXStats.page.iError404 = 0;
	gtXStats.page.iIndex = 0;
	gtXStats.page.iInfo = 0;
	gtXStats.page.iFAQ = 0;
	gtXStats.page.iRules = 0;
	gtXStats.page.iLogin = 0;
	gtXStats.page.iSignup = 0;
	gtXStats.page.iStats = 0;
	gtXStats.page.iTorrents = 0;
	gtXStats.page.iOffer = 0;
	gtXStats.page.iUpload = 0;
	gtXStats.page.iMessages = 0;
	gtXStats.page.iUsers = 0;
	gtXStats.page.iTags = 0;
	gtXStats.page.iLanguage = 0;
	gtXStats.page.iXStats = 0;
	gtXStats.page.sLastReset = UTIL_Date( );
}

// XBNBT Reset TCP Statistics
void CTracker :: resetXStatsTCP( )
{
	gtXStats.tcp.iRecv = 0;
	gtXStats.tcp.iSend = 0;
	gtXStats.tcp.iRecvHub = 0;
	gtXStats.tcp.iSendHub = 0;
	gtXStats.tcp.iRecvLink = 0;
	gtXStats.tcp.iSendLink = 0;
	gtXStats.tcp.sLastReset = UTIL_Date( );
}

// XBNBT Reset Peer Statistics
void CTracker :: resetXStatsPeers( )
{
	gtXStats.peer.iGreatest = 0;
	gtXStats.peer.iGreatestSeeds = 0;
	gtXStats.peer.iGreatestLeechers = 0;
	gtXStats.peer.iGreatestUnique = 0;
	gtXStats.peer.sLastReset = UTIL_Date( );
}

// XBNBT Reset All Statistics
void CTracker :: resetXStatsAll( )
{
#if defined ( XBNBT_GD )
	resetXStatsDynstat( );
#endif
	resetXStatsAnnounce( );
	resetXStatsScrape( );
	resetXStatsFile( );
	resetXStatsPage( );
	resetXStatsTCP( );
	resetXStatsPeers( );
}

// XBNBT Stats EXport Stats from struct to dictionary and save to file
void CTracker :: saveXStats( )
{
	CAtomDicti *pXStats = new CAtomDicti( );
	
	if( m_pXStats )
	{
		if( !pXStats->getItem( "announce" ) )
			pXStats->setItem( "announce", new CAtomDicti( ) );

		CAtom *pXAnnounce = pXStats->getItem( "announce" );

		if( pXAnnounce && pXAnnounce->isDicti( ) )
		{
			( (CAtomDicti *)pXAnnounce )->setItem( "iAnnounce", new CAtomLong( gtXStats.announce.iAnnounce ) );
			( (CAtomDicti *)pXAnnounce )->setItem( "iAnnMissing", new CAtomLong( gtXStats.announce.iAnnMissing ) );
			( (CAtomDicti *)pXAnnounce )->setItem( "iAnnNotAuth", new CAtomLong( gtXStats.announce.iAnnNotAuth ) );
			( (CAtomDicti *)pXAnnounce )->setItem( "iClientBanned", new CAtomLong( gtXStats.announce.iClientBanned ) );
			( (CAtomDicti *)pXAnnounce )->setItem( "iCompact", new CAtomLong( gtXStats.announce.iCompact ) );
			( (CAtomDicti *)pXAnnounce )->setItem( "iDownloadedInvalid", new CAtomLong( gtXStats.announce.iDownloadedInvalid) );
			( (CAtomDicti *)pXAnnounce )->setItem( "iDownloadedMissing", new CAtomLong( gtXStats.announce.iDownloadedMissing ) );
			( (CAtomDicti *)pXAnnounce )->setItem( "iInvalidEvent", new CAtomLong( gtXStats.announce.iInvalidEvent ) );
			( (CAtomDicti *)pXAnnounce )->setItem( "iIPBanned", new CAtomLong( gtXStats.announce.iIPBanned ) );
			( (CAtomDicti *)pXAnnounce )->setItem( "iIPNotCleared", new CAtomLong( gtXStats.announce.iIPNotCleared ) );
			( (CAtomDicti *)pXAnnounce )->setItem( "iLeftMissing", new CAtomLong( gtXStats.announce.iLeftMissing ) );
			( (CAtomDicti *)pXAnnounce )->setItem( "iNopeerid", new CAtomLong( gtXStats.announce.iNopeerid ) );
			( (CAtomDicti *)pXAnnounce )->setItem( "iPeerIDLength", new CAtomLong( gtXStats.announce.iPeerIDLength ) );
			( (CAtomDicti *)pXAnnounce )->setItem( "iPeerSpoofRestrict", new CAtomLong( gtXStats.announce.iPeerSpoofRestrict ) );
			( (CAtomDicti *)pXAnnounce )->setItem( "iPortBlacklisted", new CAtomLong( gtXStats.announce.iPortBlacklisted ) );
			( (CAtomDicti *)pXAnnounce )->setItem( "iPortMissing", new CAtomLong( gtXStats.announce.iPortMissing ) );
			( (CAtomDicti *)pXAnnounce )->setItem( "iRegular", new CAtomLong( gtXStats.announce.iRegular ) );
			( (CAtomDicti *)pXAnnounce )->setItem( "iUploadedInvalid", new CAtomLong( gtXStats.announce.iUploadedInvalid) );
			( (CAtomDicti *)pXAnnounce )->setItem( "iUploadedMissing", new CAtomLong( gtXStats.announce.iUploadedMissing ) );
			( (CAtomDicti *)pXAnnounce )->setItem( "iNotAuthorized", new CAtomLong( gtXStats.announce.iNotAuthorized ) );
			( (CAtomDicti *)pXAnnounce )->setItem( "sLastReset", new CAtomString( gtXStats.announce.sLastReset ) );
		}

#if defined ( XBNBT_GD )
		if( !pXStats->getItem( "dynstat" ) )
			pXStats->setItem( "dynstat", new CAtomDicti( ) );

		CAtom *pXDynstat = pXStats->getItem( "dynstat" );

		if( pXDynstat && pXDynstat->isDicti( ) )
		{
			( (CAtomDicti *)pXDynstat )->setItem( "iFrozen", new CAtomLong( gtXStats.dynstat.iFrozen ) );
			( (CAtomDicti *)pXDynstat )->setItem( "iProcessed", new CAtomLong( gtXStats.dynstat.iProcessed ) );
			( (CAtomDicti *)pXDynstat )->setItem( "iRun", new CAtomLong( gtXStats.dynstat.iRun ) );
			( (CAtomDicti *)pXDynstat )->setItem( "iTotalProcessed", new CAtomLong( gtXStats.dynstat.iTotalProcessed ) );
			( (CAtomDicti *)pXDynstat )->setItem( "sLastRunTime", new CAtomString( gtXStats.dynstat.sLastRunTime ) );
			( (CAtomDicti *)pXDynstat )->setItem( "sElapsedTime", new CAtomString( gtXStats.dynstat.sElapsedTime ) );
			( (CAtomDicti *)pXDynstat )->setItem( "sLastReset", new CAtomString( gtXStats.dynstat.sLastReset ) );
		}
#endif

		if( !pXStats->getItem( "file" ) )
			pXStats->setItem( "file", new CAtomDicti( ) );

		CAtom *pXFile = pXStats->getItem( "file" );

		if( pXFile && pXFile->isDicti( ) )
		{
			( (CAtomDicti *)pXFile )->setItem( "iBarFill", new CAtomLong( gtXStats.file.iBarFill ) );
			( (CAtomDicti *)pXFile )->setItem( "iBarTrans", new CAtomLong( gtXStats.file.iBarTrans ) );
			( (CAtomDicti *)pXFile )->setItem( "iCSS", new CAtomLong( gtXStats.file.iCSS ) );
			( (CAtomDicti *)pXFile )->setItem( "iFavicon", new CAtomLong( gtXStats.file.iFavicon ) );
			( (CAtomDicti *)pXFile )->setItem( "uiFiles", new CAtomLong( gtXStats.file.uiFiles ) );
			( (CAtomDicti *)pXFile )->setItem( "iRobots", new CAtomLong( gtXStats.file.iRobots ) );
			( (CAtomDicti *)pXFile )->setItem( "iRSS", new CAtomLong( gtXStats.file.iRSS ) );
			( (CAtomDicti *)pXFile )->setItem( "iRSSXSL", new CAtomLong( gtXStats.file.iRSSXSL ) );
			( (CAtomDicti *)pXFile )->setItem( "iXML", new CAtomLong( gtXStats.file.iXML ) );
			( (CAtomDicti *)pXFile )->setItem( "iTorrent", new CAtomLong( gtXStats.file.iTorrent ) );
			( (CAtomDicti *)pXFile )->setItem( "sLastReset", new CAtomString( gtXStats.file.sLastReset ) );
		}

		if( !pXStats->getItem( "page" ) )
			pXStats->setItem( "page", new CAtomDicti( ) );

		CAtom *pXPage = pXStats->getItem( "page" );

		if( pXPage && pXPage->isDicti( ) )
		{
			( (CAtomDicti *)pXPage )->setItem( "iAdmin", new CAtomLong( gtXStats.page.iAdmin ) );
			( (CAtomDicti *)pXPage )->setItem( "uiComments", new CAtomLong( gtXStats.page.uiComments ) );
			( (CAtomDicti *)pXPage )->setItem( "iError404", new CAtomLong( gtXStats.page.iError404 ) );
			( (CAtomDicti *)pXPage )->setItem( "iIndex", new CAtomLong( gtXStats.page.iIndex ) );
			( (CAtomDicti *)pXPage )->setItem( "iInfo", new CAtomLong( gtXStats.page.iInfo ) );
			( (CAtomDicti *)pXPage )->setItem( "iRules", new CAtomLong( gtXStats.page.iRules ) );
			( (CAtomDicti *)pXPage )->setItem( "iFAQ", new CAtomLong( gtXStats.page.iFAQ ) );
			( (CAtomDicti *)pXPage )->setItem( "iLogin", new CAtomLong( gtXStats.page.iLogin ) );
			( (CAtomDicti *)pXPage )->setItem( "iSignup", new CAtomLong( gtXStats.page.iSignup ) );
			( (CAtomDicti *)pXPage )->setItem( "iStats", new CAtomLong( gtXStats.page.iStats ) );
			( (CAtomDicti *)pXPage )->setItem( "iTorrents", new CAtomLong( gtXStats.page.iTorrents) );
			( (CAtomDicti *)pXPage )->setItem( "iOffer", new CAtomLong( gtXStats.page.iOffer ) );
			( (CAtomDicti *)pXPage )->setItem( "iUpload", new CAtomLong( gtXStats.page.iUpload ) );
			( (CAtomDicti *)pXPage )->setItem( "iMessages", new CAtomLong( gtXStats.page.iMessages ) );
			( (CAtomDicti *)pXPage )->setItem( "iUsers", new CAtomLong( gtXStats.page.iUsers ) );
			( (CAtomDicti *)pXPage )->setItem( "iTags", new CAtomLong( gtXStats.page.iTags ) );
			( (CAtomDicti *)pXPage )->setItem( "iLanguage", new CAtomLong( gtXStats.page.iLanguage ) );
			( (CAtomDicti *)pXPage )->setItem( "iXStats", new CAtomLong( gtXStats.page.iXStats ) );
			( (CAtomDicti *)pXPage )->setItem( "sLastReset", new CAtomString( gtXStats.page.sLastReset ) );
		}

		if( !pXStats->getItem( "scrape" ) )
			pXStats->setItem( "scrape", new CAtomDicti( ) );

		CAtom *pXScrape = pXStats->getItem( "scrape" );

		if( pXScrape && pXScrape->isDicti( ) )
		{
			( (CAtomDicti *)pXScrape )->setItem( "iScrape", new CAtomLong( gtXStats.scrape.iScrape ) );
			( (CAtomDicti *)pXScrape )->setItem( "iDisallowed", new CAtomLong( gtXStats.scrape.iDisallowed ) );
			( (CAtomDicti *)pXScrape )->setItem( "iSingle", new CAtomLong( gtXStats.scrape.iSingle ) );
			( (CAtomDicti *)pXScrape )->setItem( "iMultiple", new CAtomLong( gtXStats.scrape.iMultiple ) );
			( (CAtomDicti *)pXScrape )->setItem( "iFull", new CAtomLong( gtXStats.scrape.iFull ) );
			( (CAtomDicti *)pXScrape )->setItem( "sLastReset", new CAtomString( gtXStats.scrape.sLastReset ) );
		}

		if( !pXStats->getItem( "tcp" ) )
			pXStats->setItem( "tcp", new CAtomDicti( ) );

		CAtom *pXTCP = pXStats->getItem( "tcp" );

		if( pXTCP && pXTCP->isDicti( ) )
		{
			( (CAtomDicti *)pXTCP )->setItem( "iRecv", new CAtomLong( gtXStats.tcp.iRecv ) );
			( (CAtomDicti *)pXTCP )->setItem( "iSend", new CAtomLong( gtXStats.tcp.iSend ) );
			( (CAtomDicti *)pXTCP )->setItem( "iRecvHub", new CAtomLong( gtXStats.tcp.iRecvHub ) );
			( (CAtomDicti *)pXTCP )->setItem( "iSendHub", new CAtomLong( gtXStats.tcp.iSendHub ) );
			( (CAtomDicti *)pXTCP )->setItem( "iRecvLink", new CAtomLong( gtXStats.tcp.iRecvLink ) );
			( (CAtomDicti *)pXTCP )->setItem( "iSendLink", new CAtomLong( gtXStats.tcp.iSendLink ) );
			( (CAtomDicti *)pXTCP )->setItem( "sLastReset", new CAtomString( gtXStats.tcp.sLastReset ) );
		}

		if( !pXStats->getItem( "peer" ) )
			pXStats->setItem( "peer", new CAtomDicti( ) );

		CAtom *pXPeer = pXStats->getItem( "peer" );

		if( pXPeer && pXPeer->isDicti( ) )
		{
			( (CAtomDicti *)pXPeer )->setItem( "iGreatest", new CAtomLong( gtXStats.peer.iGreatest ) );
			( (CAtomDicti *)pXPeer )->setItem( "iGreatestSeeds", new CAtomLong( gtXStats.peer.iGreatestSeeds ) );
			( (CAtomDicti *)pXPeer )->setItem( "iGreatestLeechers", new CAtomLong( gtXStats.peer.iGreatestLeechers ) );
			( (CAtomDicti *)pXPeer )->setItem( "iGreatestUnique", new CAtomLong( gtXStats.peer.iGreatestUnique ) );
			( (CAtomDicti *)pXPeer )->setItem( "sLastReset", new CAtomString( gtXStats.peer.sLastReset ) );
		}
	}
	const string cstrData( Encode( pXStats ) );
	
	delete m_pXStats;
	
	m_pXStats = pXStats;

	FILE *pFile = FILE_ERROR;

	pFile = fopen( statsdump.strName.c_str( ), "wb" );

	if( pFile == FILE_ERROR )
	{
		UTIL_LogPrint( ( gmapLANG_CFG["unable_to_write_file"] + "\n" ).c_str( ), statsdump.strName.c_str( ) );

		return;
	}

	fwrite( cstrData.c_str( ), sizeof( char ), cstrData.size( ), pFile );
	fclose( pFile );
}

#if defined ( XBNBT_GD )
// XBNBT XStats table for Dynstat
void CTracker :: tableXStatsDynstat( struct request_t *pRequest, struct response_t *pResponse )
{
	pResponse->strContent += "<div class=\"admin_table\">\n";
	pResponse->strContent += "<table summary=\"xdsstats\">\n";

	if( pRequest->user.ucAccess & m_ucAccessViewXStates )
	{
		pResponse->strContent += "<tr><th colspan=6>" + gmapLANG_CFG["stats_dynstat"] + "/th>\n";
		pResponse->strContent += "<th colspan=1>";
		pResponse->strContent += Button_JS_Link( "confirm_dynstat", gmapLANG_CFG["reset"], "dynstat_confirm()" );
		pResponse->strContent += "</th></tr>\n";
	}
	else
		pResponse->strContent += "<tr><th colspan=7>" + gmapLANG_CFG["stats_dynstat"] + "</th></tr>\n";

	pResponse->strContent += "<tr>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_last_reset"] + "</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_run_times"] + "</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_last_run"] + "</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_last_elapsed"] + "</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_last_processed"] + "</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_torrent_frozen"] + "</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_processed_start"] + "</th>\n";
	pResponse->strContent += "</tr>\n";

	pResponse->strContent += "<tr>\n";
	pResponse->strContent += "<td class=\"even\">" + gtXStats.dynstat.sLastReset + "</td>\n";
	pResponse->strContent += "<td class=\"odd\">" + CAtomLong( gtXStats.dynstat.iRun ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"even\">" + gtXStats.dynstat.sLastRunTime + "</td>\n";
	pResponse->strContent += "<td class=\"odd\">" + gtXStats.dynstat.sElapsedTime + "</td>\n";
	pResponse->strContent += "<td class=\"odd\">" + CAtomLong( gtXStats.dynstat.iProcessed ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"even\">" + CAtomLong( gtXStats.dynstat.iFrozen ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"odd\">" + CAtomLong( gtXStats.dynstat.iTotalProcessed ).toString( ) + "</td>\n";
	pResponse->strContent += "</tr>\n";

	pResponse->strContent += "</table>\n";
	pResponse->strContent += "</div>\n\n";
}
#endif

// XBNBT XStats table for Announce
void CTracker :: tableXStatsAnnounce( struct request_t *pRequest, struct response_t *pResponse )
{
	pResponse->strContent += "<div class=\"admin_table\">\n";
	pResponse->strContent += "<table summary=\"xannstats\">\n";

	if( pRequest->user.ucAccess & m_ucAccessViewXStates )
	{
		pResponse->strContent += "<tr><th colspan=6>" + gmapLANG_CFG["stats_announce"] + "</th>\n";
		pResponse->strContent += "<th colspan=1>";
		pResponse->strContent += Button_JS_Link( "confirm_announce", gmapLANG_CFG["reset"], "announce_confirm()" );
		pResponse->strContent += "</th></tr>\n";
	}
	else
		pResponse->strContent += "<tr><th colspan=7>" + gmapLANG_CFG["stats_announce"] + "</th></tr>\n";

	pResponse->strContent += "<tr>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_last_reset"] + "</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_announces"] + "</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_announces_missing"] + "</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_announces_notauth"] + "</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_compact"] + "</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_no_peer_id"] + "</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_regular"] + "</th>\n";
	pResponse->strContent += "</tr>\n";

	pResponse->strContent += "<tr>\n";
	pResponse->strContent += "<td class=\"even\">" + gtXStats.announce.sLastReset + "</td>\n";
	pResponse->strContent += "<td class=\"odd\">" + CAtomLong( gtXStats.announce.iAnnounce ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"odd\">" + CAtomLong( gtXStats.announce.iAnnMissing ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"odd\">" + CAtomLong( gtXStats.announce.iAnnNotAuth ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"even\">" + CAtomLong( gtXStats.announce.iCompact ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"odd\">" + CAtomLong( gtXStats.announce.iNopeerid ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"even\">" + CAtomLong( gtXStats.announce.iRegular ).toString( ) + "</td>\n";
	pResponse->strContent += "</tr>\n";

	pResponse->strContent += "<tr>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_ip_banned"] + "</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_ip_not_cleared"] + "</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_peer_spoof_blocked"] + "</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_client_banned"] + "</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_invalid_event"] + "</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_port_missing"] + "</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_uploaded_missing"] + "</th>\n";
	pResponse->strContent += "</tr>\n";

	pResponse->strContent += "<tr>\n";
	pResponse->strContent += "<td class=\"odd\">" + CAtomLong( gtXStats.announce.iIPBanned ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"even\">" + CAtomLong( gtXStats.announce.iIPNotCleared ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"odd\">" + CAtomLong( gtXStats.announce.iPeerSpoofRestrict ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"odd\">" + CAtomLong( gtXStats.announce.iClientBanned ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"even\">" + CAtomLong( gtXStats.announce.iInvalidEvent ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"odd\">" + CAtomLong( gtXStats.announce.iPortMissing ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"even\">" + CAtomLong( gtXStats.announce.iUploadedMissing ).toString( ) + "</td>\n";
	pResponse->strContent += "</tr>\n";

	pResponse->strContent += "<tr>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_downloaded_missing"] + "</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_left_missing"] + "</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_peer_id_length"] + "</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_port_blacklisted"] + "</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_download_invalid"] + "</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_upload_invalid"] + "</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["server_response_401"] + "</th>\n";
	pResponse->strContent += "</tr>\n";

	pResponse->strContent += "<tr>\n";
	pResponse->strContent += "<td class=\"even\">" + CAtomLong( gtXStats.announce.iDownloadedMissing ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"odd\">" + CAtomLong( gtXStats.announce.iLeftMissing ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"even\">" + CAtomLong( gtXStats.announce.iPeerIDLength ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"odd\">" + CAtomLong( gtXStats.announce.iPortBlacklisted ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"even\">" + CAtomLong( gtXStats.announce.iDownloadedInvalid ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"odd\">" + CAtomLong( gtXStats.announce.iUploadedInvalid ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"even\">" + CAtomLong( gtXStats.announce.iNotAuthorized ).toString( ) + "</td>\n";
	pResponse->strContent += "</tr>\n";

	pResponse->strContent += "</table>\n";
	pResponse->strContent += "</div>\n\n";
}

// XBNBT XStats table for Scrape
void CTracker :: tableXStatsScrape( struct request_t *pRequest, struct response_t *pResponse )
{
	pResponse->strContent += "<div class=\"admin_table\">\n";
	pResponse->strContent += "<table summary=\"xscrstats\">\n";

	if( pRequest->user.ucAccess & m_ucAccessViewXStates )
	{
		pResponse->strContent += "<tr>\n";
		pResponse->strContent += "<th colspan=5>";
		pResponse->strContent += gmapLANG_CFG["stats_scrape"] + "</th>\n";
		pResponse->strContent += "<th colspan=1>";
		pResponse->strContent += Button_JS_Link( "confirm_scrape", gmapLANG_CFG["reset"], "scrape_confirm()" );
		pResponse->strContent += "</th>\n</tr>\n";
	}
	else
	{
		pResponse->strContent += "<tr>\n";
		pResponse->strContent += "<th colspan=6>";
		pResponse->strContent += gmapLANG_CFG["stats_scrape"] + "</th>\n</tr>\n";
	}

	pResponse->strContent += "<tr>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_last_reset"] + "</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_scrapes"] + "</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_single"] + "</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_multiple"] + "</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_full"] + "</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["server_response_401"] + "</th>\n";
	pResponse->strContent += "</tr>\n";

	pResponse->strContent += "<tr>\n";
	pResponse->strContent += "<td class=\"even\">" + gtXStats.scrape.sLastReset + "</td>\n";
	pResponse->strContent += "<td class=\"odd\">" + CAtomLong( gtXStats.scrape.iScrape ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"even\">" + CAtomLong( gtXStats.scrape.iSingle ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"odd\">" + CAtomLong( gtXStats.scrape.iMultiple ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"even\">" + CAtomLong( gtXStats.scrape.iFull ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"odd\">" + CAtomLong( gtXStats.scrape.iDisallowed ).toString( ) + "</td>\n";
	pResponse->strContent += "</tr>\n";

	pResponse->strContent += "</table>\n";
	pResponse->strContent += "</div>\n\n";
}

// XBNBT XStats table for File
void CTracker :: tableXStatsFile( struct request_t *pRequest, struct response_t *pResponse )
{
	pResponse->strContent += "<div class=\"admin_table\">\n";
	pResponse->strContent += "<table summary=\"xfilestats\">\n";

	if( pRequest->user.ucAccess & m_ucAccessViewXStates )
	{
		pResponse->strContent += "<tr><th colspan=10>" + gmapLANG_CFG["stats_file"] + "</th>\n";
		pResponse->strContent += "<th colspan=1>";
		pResponse->strContent += Button_JS_Link( "confirm_file", gmapLANG_CFG["reset"], "file_confirm()" );
		pResponse->strContent += "</th></tr>\n";
	}
	else
	{
		pResponse->strContent += "<tr><th colspan=11>" + gmapLANG_CFG["stats_file"] + "</th></tr>\n";
	}

	pResponse->strContent += "<tr>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_last_reset"] + "</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_favicon"] + "</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_ibfill"] + "</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_ibtrans"] + "</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_css"] + "</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_rss"] + "</th>\n";
	pResponse->strContent += "<th>RSS.XSL</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_xml"] + "</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_robots"] + "</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_torrents"] + "</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_other"] + "</th>\n";
	pResponse->strContent += "</tr>\n";
	pResponse->strContent += "<tr>\n";
	pResponse->strContent += "<td class=\"even\">" + gtXStats.file.sLastReset + "</td>\n";
	pResponse->strContent += "<td class=\"odd\">" + CAtomLong( gtXStats.file.iFavicon ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"even\">" + CAtomLong( gtXStats.file.iBarFill ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"odd\">" + CAtomLong( gtXStats.file.iBarTrans ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"even\">" + CAtomLong( gtXStats.file.iCSS ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"odd\">" + CAtomLong( gtXStats.file.iRSS ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"even\">" + CAtomLong( gtXStats.file.iRSSXSL ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"odd\">" + CAtomLong( gtXStats.file.iXML ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"even\">" + CAtomLong( gtXStats.file.iRobots ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"odd\">" + CAtomLong( gtXStats.file.iTorrent ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"even\">" + CAtomLong( gtXStats.file.uiFiles ).toString( ) + "</td>\n";
	pResponse->strContent += "</tr>\n";

	pResponse->strContent += "</table>\n";
	pResponse->strContent += "</div>\n\n";
}

// XBNBT XStats table for Page
void CTracker :: tableXStatsPage( struct request_t *pRequest, struct response_t *pResponse )
{
	pResponse->strContent += "<div class=\"admin_table\">\n";
	pResponse->strContent += "<table summary=\"xpagestats\">\n";

	if( pRequest->user.ucAccess & m_ucAccessViewXStates )
	{
		pResponse->strContent += "<tr><th colspan=9>XBNBT HTML Page Stats</th>\n";
		pResponse->strContent += "<th colspan=1>";
		pResponse->strContent += Button_JS_Link( "confirm_page", gmapLANG_CFG["reset"], "page_confirm()" );
		pResponse->strContent += "</th></tr>\n";
	}
	else
	{
		pResponse->strContent += "<tr><th colspan=10>XBNBT HTML Page Stats</th></tr>\n";
	}

	pResponse->strContent += "<tr>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_last_reset"] + "</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_index"] + "</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_stats"] + "</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_comments"] + "</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_info"] + "</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_rules"] + "</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_faq"] + "</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_login"] + "</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_signup"] + "</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_offer"] + "</th>\n";
	pResponse->strContent += "</tr>\n";

	pResponse->strContent += "<tr>\n";
	pResponse->strContent += "<td class=\"even\">" + gtXStats.page.sLastReset + "</td>\n";
	pResponse->strContent += "<td class=\"odd\">" + CAtomLong( gtXStats.page.iIndex ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"even\">" + CAtomLong( gtXStats.page.iStats ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"odd\">" + CAtomLong( gtXStats.page.uiComments ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"even\">" + CAtomLong( gtXStats.page.iInfo ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"odd\">" + CAtomLong( gtXStats.page.iRules ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"even\">" + CAtomLong( gtXStats.page.iFAQ ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"odd\">" + CAtomLong( gtXStats.page.iLogin ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"even\">" + CAtomLong( gtXStats.page.iSignup ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"odd\">" + CAtomLong( gtXStats.page.iOffer ).toString( ) + "</td>\n";
	pResponse->strContent += "</tr>\n";

	pResponse->strContent += "<tr>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_upload"] + "</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_torrents"] + "</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_messages"] + "</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_admin"] + "</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_users"] + "</th>\n";
	pResponse->strContent += "<th>Tags</th>\n";
	pResponse->strContent += "<th>Language</th>\n";
	pResponse->strContent += "<th>XStats</th>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_error404"] + "</th>\n";
	pResponse->strContent += "</tr>\n";

	pResponse->strContent += "<tr>\n";
	pResponse->strContent += "<td class=\"even\">" + CAtomLong( gtXStats.page.iUpload ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"odd\">" + CAtomLong( gtXStats.page.iTorrents ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"even\">" + CAtomLong( gtXStats.page.iMessages ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"odd\">" + CAtomLong( gtXStats.page.iAdmin ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"even\">" + CAtomLong( gtXStats.page.iUsers ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"odd\">" + CAtomLong( gtXStats.page.iTags ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"even\">" + CAtomLong( gtXStats.page.iLanguage ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"odd\">" + CAtomLong( gtXStats.page.iXStats ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"even\">" + CAtomLong( gtXStats.page.iError404 ).toString( ) + "</td>\n";
	pResponse->strContent += "</tr>\n";

	pResponse->strContent += "</table>\n";
	pResponse->strContent += "</div>\n\n";
}

// XBNBT XStats table for TCP
void CTracker :: tableXStatsTCP( struct request_t *pRequest, struct response_t *pResponse )
{
	pResponse->strContent += "<div class=\"admin_table\">\n";
	pResponse->strContent += "<table summary=\"xtcpstats\">\n";

	if( pRequest->user.ucAccess & m_ucAccessViewXStates )
	{
		pResponse->strContent += "<tr>\n";
	
		pResponse->strContent += "<th colspan=8>TCP Stats</th>\n";

		pResponse->strContent += "<th colspan=1>";
		pResponse->strContent += Button_JS_Link( "confirm_tcp", gmapLANG_CFG["reset"], "tcp_confirm()" );
		pResponse->strContent += "</th>\n</tr>\n";
	}
	else
	{
		pResponse->strContent += "<tr>\n";

		pResponse->strContent += "<th colspan=9>TCP Stats</th>\n</tr>\n";
	}

	pResponse->strContent += "<tr>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_last_reset"] + "</th>\n";
	pResponse->strContent += "<th>Total Received</th>\n";
	pResponse->strContent += "<th>Total Sent</th>\n";
	pResponse->strContent += "<th>Received Client</th>\n";
	pResponse->strContent += "<th>Sent Client</th>\n";
	pResponse->strContent += "<th>Received Hub</th>\n";
	pResponse->strContent += "<th>Sent Hub</th>\n";
	pResponse->strContent += "<th>Received Link</th>\n";
	pResponse->strContent += "<th>Sent Link</th>\n";
	pResponse->strContent += "</tr>\n";

	pResponse->strContent += "<tr>\n";
	pResponse->strContent += "<td class=\"even\">" + gtXStats.tcp.sLastReset + "</td>\n";
	pResponse->strContent += "<td class=\"odd\">" + UTIL_BytesToString( gtXStats.tcp.iRecv + gtXStats.tcp.iRecvHub + gtXStats.tcp.iRecvLink ) + "</td>\n";
	pResponse->strContent += "<td class=\"even\">" + UTIL_BytesToString( gtXStats.tcp.iSend + gtXStats.tcp.iSendHub + gtXStats.tcp.iSendLink ) + "</td>\n";
	pResponse->strContent += "<td class=\"odd\">" + UTIL_BytesToString( gtXStats.tcp.iRecv ) + "</td>\n";
	pResponse->strContent += "<td class=\"even\">" + UTIL_BytesToString( gtXStats.tcp.iSend ) + "</td>\n";
	pResponse->strContent += "<td class=\"odd\">" + UTIL_BytesToString( gtXStats.tcp.iRecvHub ) + "</td>\n";
	pResponse->strContent += "<td class=\"even\">" + UTIL_BytesToString( gtXStats.tcp.iSendHub ) + "</td>\n";
	pResponse->strContent += "<td class=\"odd\">" + UTIL_BytesToString( gtXStats.tcp.iRecvLink ) + "</td>\n";
	pResponse->strContent += "<td class=\"even\">" + UTIL_BytesToString( gtXStats.tcp.iSendLink ) + "</td>\n";
	pResponse->strContent += "</tr>\n";

	pResponse->strContent += "</table>\n";
	pResponse->strContent += "</div>\n\n";
}

// XBNBT XStats table for Peers
void CTracker :: tableXStatsPeer( struct request_t *pRequest, struct response_t *pResponse )
{
	pResponse->strContent += "<div class=\"admin_table\">\n";
	pResponse->strContent += "<table summary=\"xpeerstats\">\n";

	if( pRequest->user.ucAccess & m_ucAccessViewXStates )
	{
		pResponse->strContent += "<tr>\n";
	
		pResponse->strContent += "<th colspan=8>Peer Stats</th>\n";

		pResponse->strContent += "<th colspan=1>";
		pResponse->strContent += Button_JS_Link( "confirm_peer", gmapLANG_CFG["reset"], "peer_confirm()" );
		pResponse->strContent += "</th>\n</tr>\n";
	}
	else
	{
		pResponse->strContent += "<tr>\n";

		pResponse->strContent += "<th colspan=9>Peer Stats</th>\n</tr>\n";
	}

	pResponse->strContent += "<tr>\n";
	pResponse->strContent += "<th>" + gmapLANG_CFG["stats_last_reset"] + "</th>\n";
	pResponse->strContent += "<th>Current Peers</th>\n";
	pResponse->strContent += "<th>Current Seeders</th>\n";
	pResponse->strContent += "<th>Current Leechers</th>\n";
	pResponse->strContent += "<th>Current Unique</th>\n";
	pResponse->strContent += "<th>Greatest Peers</th>\n";
	pResponse->strContent += "<th>Greatest Seeders</th>\n";
	pResponse->strContent += "<th>Greatest Leechers</th>\n";
	pResponse->strContent += "<th>Greatest Unique</th>\n";
	pResponse->strContent += "</tr>\n";

	// Refresh the fast cache
	if( GetTime( ) > m_ulRefreshFastCacheNext )
	{
		// Refresh
		RefreshFastCache( );

		// Set the next refresh time
		m_ulRefreshFastCacheNext = GetTime( ) + m_uiRefreshFastCacheInterval;
	}
	
	CMySQLQuery *pQueryIP = new CMySQLQuery( "SELECT COUNT(*) FROM ips WHERE bcount>0" );
			
	vector<string> vecQueryIP;

	vecQueryIP.reserve(1);

	vecQueryIP = pQueryIP->nextRow( );
	
	delete pQueryIP;
	
	int64 iGreatestUnique = 0;
	if( vecQueryIP.size( ) == 1 )
		iGreatestUnique = UTIL_StringTo64( vecQueryIP[0].c_str( ) );

	if( m_bCountUniquePeers )
	{
		if( iGreatestUnique > gtXStats.peer.iGreatestUnique )
			gtXStats.peer.iGreatestUnique = iGreatestUnique;
	}

	pResponse->strContent += "<tr>\n";
	pResponse->strContent += "<td class=\"even\">" + gtXStats.peer.sLastReset + "</td>\n";

	pResponse->strContent += "<td class=\"odd\">" + CAtomLong( m_ulPeers ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"even\">" + CAtomLong( m_ulSeedersTotal ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"odd\">" + CAtomLong( m_ulLeechersTotal ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"even\">" + vecQueryIP[0] + "</td>\n";
	pResponse->strContent += "<td class=\"odd\">" + CAtomLong( gtXStats.peer.iGreatest ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"even\">" + CAtomLong( gtXStats.peer.iGreatestSeeds ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"odd\">" + CAtomLong( gtXStats.peer.iGreatestLeechers ).toString( ) + "</td>\n";
	pResponse->strContent += "<td class=\"even\">" + CAtomLong( gtXStats.peer.iGreatestUnique ).toString( ) + "</td>\n";
	pResponse->strContent += "</tr>\n";

	pResponse->strContent += "</table>\n";
	pResponse->strContent += "</div>\n\n";
}

void CTracker :: serverResponseXStats( struct request_t *pRequest, struct response_t *pResponse )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0)
		if( IsIPBanned( pRequest, pResponse, btv, gmapLANG_CFG["admin_page"], string( CSS_ADMIN ), NOT_INDEX ) )
			return;

	// Does the user have admin access?
	if( !pRequest->user.strUID.empty( ) && ( pRequest->user.ucAccess & m_ucAccessAdmin ) )
	{

#if defined ( XBNBT_GD )
		if( pRequest->mapParams["ap_dynstat"] == "1" )
		{
			resetXStatsDynstat( );
			saveXStats( );

			return JS_ReturnToPage( pRequest, pResponse, XSTATS_HTML );
		}
#endif

		if( pRequest->mapParams["ap_announce"] == "1" )
		{
			resetXStatsAnnounce( );
			saveXStats( );

			return JS_ReturnToPage( pRequest, pResponse, XSTATS_HTML );
		}

		if( pRequest->mapParams["ap_scrape"] == "1" )
		{
			resetXStatsScrape( );
			saveXStats( );

			return JS_ReturnToPage( pRequest, pResponse, XSTATS_HTML );
		}

		if( pRequest->mapParams["ap_file"] == "1" )
		{
			resetXStatsFile( );
			saveXStats( );

			return JS_ReturnToPage( pRequest, pResponse, XSTATS_HTML );
		}

		if( pRequest->mapParams["ap_page"] == "1" )
		{
			resetXStatsPage( );
			saveXStats( );

			return JS_ReturnToPage( pRequest, pResponse, XSTATS_HTML );
		}

		if( pRequest->mapParams["ap_tcp"] == "1" )
		{
			resetXStatsTCP( );
			saveXStats( );

			return JS_ReturnToPage( pRequest, pResponse, XSTATS_HTML );
		}

		if( pRequest->mapParams["ap_peer"] == "1" )
		{
			resetXStatsPeers( );
			saveXStats( );

			return JS_ReturnToPage( pRequest, pResponse, XSTATS_HTML );
		}

		if( pRequest->mapParams["ap_all"] == "1" )
		{
			resetXStatsAll( );
			saveXStats( );

			return JS_ReturnToPage( pRequest, pResponse, XSTATS_HTML );
		}

		// display
		HTML_Common_Begin( pRequest, pResponse, gmapLANG_CFG["admin_page"], string( CSS_ADMIN ), string( ), NOT_INDEX, CODE_200 );
		
		// Javascript
        pResponse->strContent += "<script type=\"text/javascript\">\n";
		pResponse->strContent += "<!--\n";

		pResponse->strContent += "function dynstat_confirm()\n";
		pResponse->strContent += "{\n";
		pResponse->strContent += "var name=confirm(\"" + gmapLANG_CFG["js_reset_dynstat_stats_q"] + "\")\n";
		pResponse->strContent += "if (name==true)\n";
		pResponse->strContent += "{\n";
		pResponse->strContent += "window.location=\"" + RESPONSE_STR_XSTATS_HTML + "?ap_dynstat=1\"\n";
		pResponse->strContent += "}\n";
		pResponse->strContent += "}\n\n";

		pResponse->strContent += "function announce_confirm()\n";
		pResponse->strContent += "{\n";
		pResponse->strContent += "var name=confirm(\"" + gmapLANG_CFG["js_reset_announce_stats_q"] + "\")\n";
		pResponse->strContent += "if (name==true)\n";
		pResponse->strContent += "{\n";
		pResponse->strContent += "window.location=\"" + RESPONSE_STR_XSTATS_HTML + "?ap_announce=1\"\n";
		pResponse->strContent += "}\n";
		pResponse->strContent += "}\n\n";

		pResponse->strContent += "function scrape_confirm()\n";
		pResponse->strContent += "{\n";
		pResponse->strContent += "var name=confirm(\"" + gmapLANG_CFG["js_reset_scrape_stats_q"] + "\")\n";
		pResponse->strContent += "if (name==true)\n";
		pResponse->strContent += "{\n";
		pResponse->strContent += "window.location=\"" + RESPONSE_STR_XSTATS_HTML + "?ap_scrape=1\"\n";
		pResponse->strContent += "}\n";
		pResponse->strContent += "}\n\n";

		pResponse->strContent += "function file_confirm()\n";
		pResponse->strContent += "{\n";
		pResponse->strContent += "var name=confirm(\"" + gmapLANG_CFG["js_reset_file_stats_q"] + "\")\n";
		pResponse->strContent += "if (name==true)\n";
		pResponse->strContent += "{\n";
		pResponse->strContent += "window.location=\"" + RESPONSE_STR_XSTATS_HTML + "?ap_file=1\"\n";
		pResponse->strContent += "}\n";
		pResponse->strContent += "}\n\n";

		pResponse->strContent += "function page_confirm()\n";
		pResponse->strContent += "{\n";
		pResponse->strContent += "var name=confirm(\"" + gmapLANG_CFG["js_reset_page_stats_q"] + "\")\n";
		pResponse->strContent += "if (name==true)\n";
		pResponse->strContent += "{\n";
		pResponse->strContent += "window.location=\"" + RESPONSE_STR_XSTATS_HTML + "?ap_page=1\"\n";
		pResponse->strContent += "}\n";
		pResponse->strContent += "}\n\n";

		pResponse->strContent += "function tcp_confirm()\n";
		pResponse->strContent += "{\n";
		pResponse->strContent += "var name=confirm(\"Reset TCP Stats\")\n";
		pResponse->strContent += "if (name==true)\n";
		pResponse->strContent += "{\n";
		pResponse->strContent += "window.location=\"" + RESPONSE_STR_XSTATS_HTML + "?ap_tcp=1\"\n";
		pResponse->strContent += "}\n";
		pResponse->strContent += "}\n\n";

		pResponse->strContent += "function peer_confirm()\n";
		pResponse->strContent += "{\n";
		pResponse->strContent += "var name=confirm(\"Reset Peer Stats\")\n";
		pResponse->strContent += "if (name==true)\n";
		pResponse->strContent += "{\n";
		pResponse->strContent += "window.location=\"" + RESPONSE_STR_XSTATS_HTML + "?ap_peer=1\"\n";
		pResponse->strContent += "}\n";
		pResponse->strContent += "}\n\n";

		pResponse->strContent += "function all_confirm()\n";
		pResponse->strContent += "{\n";
		pResponse->strContent += "var name=confirm(\"" + gmapLANG_CFG["js_reset_all_stats_q"] + "\")\n";
		pResponse->strContent += "if (name==true)\n";
		pResponse->strContent += "{\n";
		pResponse->strContent += "window.location=\"" + RESPONSE_STR_XSTATS_HTML + "?ap_all=1\"\n";
		pResponse->strContent += "}\n";
		pResponse->strContent += "}\n\n";

		pResponse->strContent += "//-->\n";
		pResponse->strContent += "</script>\n\n";

		// Tables
#if defined ( XBNBT_GD )
		tableXStatsDynstat( pRequest, pResponse );
#endif
		tableXStatsAnnounce( pRequest, pResponse );
		tableXStatsScrape( pRequest, pResponse );
		tableXStatsFile( pRequest, pResponse );
		tableXStatsPage( pRequest, pResponse );
		tableXStatsTCP( pRequest, pResponse );
		tableXStatsPeer( pRequest, pResponse );

		// Button List
		pResponse->strContent += "<p class=\"button_list\">";

		// Reset all stats
		pResponse->strContent += Button_JS_Link( "confirm_all", gmapLANG_CFG["reset_all_stats"], "all_confirm()" );

		pResponse->strContent += "</p>\n";

		// Output common HTML ending
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_ADMIN ) );
	}
	else
	{
		// not authorized
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["admin_page"], string( CSS_ADMIN ), string( ), NOT_INDEX, CODE_401 );

		// Output common HTML ending
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_ADMIN ) );
	}
}

void CTracker :: serverResponseTags( struct request_t *pRequest, struct response_t *pResponse )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0)
		if( IsIPBanned( pRequest, pResponse, btv, gmapLANG_CFG["admin_page"], string( CSS_ADMIN ), NOT_INDEX ) )
			return;

	if( !pRequest->user.strUID.empty( ) && ( pRequest->user.ucAccess & m_ucAccessAdmin ) )
	{
// 		const string strSwapTag( pRequest->mapParams["ap_swaptag"] );
		const unsigned int cucSwapTag( (unsigned int)atoi( pRequest->mapParams["ap_swaptag"].c_str( ) ) );

		if( cucSwapTag != 0 )
		{
			unsigned char ucTag = 1;
			unsigned int ucTag_Index = ucTag * 100 + 1;
			string strNameNext = string( );
			string strTagNext = string( );

			string strName = "bnbt_tag" + CAtomInt( ucTag_Index ).toString( );
			string strTag = CFG_GetString( strName, string( ) );

			while( !strTag.empty( ) )
			{
				if( ucTag_Index == cucSwapTag )
				{
					strNameNext = "bnbt_tag" + CAtomInt( ucTag_Index + 1 ).toString( );
					strTagNext = CFG_GetString( strNameNext, string( ) );

					CFG_SetString( strName, strTagNext );
					CFG_SetString( strNameNext, strTag );

					ucTag_Index++;
				}
				else
				{
					strName = "bnbt_tag" + CAtomInt( ucTag_Index ).toString( );
					CFG_SetString( strName, strTag );
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

			CFG_Close( CFG_FILE );
//			m_vecTags.clear( );
//			m_vecTagsMouse.clear( );
			initTags( );

			return JS_ReturnToPage( pRequest, pResponse, TAGS_HTML );
		}

// 		const string strDelTag( pRequest->mapParams["ap_deltag"] );
		const unsigned int cucDelTag( (unsigned int)atoi( pRequest->mapParams["ap_deltag"].c_str( ) ) );

		if( cucDelTag != 0 )
		{
			unsigned char ucTag = (unsigned char)( cucDelTag / 100 );
			unsigned int ucTag_Index = ucTag * 100 + 1;
			unsigned int ucTagPrevious = ucTag * 100;

			string strName = "bnbt_tag" + CAtomInt( ucTag_Index ).toString( );
			string strTag = CFG_GetString( strName, string( ) );

			while( !strTag.empty( ) )
			{
				if( ucTag_Index == cucDelTag )
					CFG_Delete( strName );

				if( ucTag_Index > cucDelTag )
				{
					strName = "bnbt_tag" + CAtomInt( ucTagPrevious ).toString( );
					CFG_SetString( strName, strTag );
					UTIL_LogPrint( "%s = %s\n", strName.c_str( ), strTag.c_str( ) );
				}
				
					ucTagPrevious = ucTag_Index;
// 				ucTagPrevious = ucTag;

				strName = "bnbt_tag" + CAtomInt( ++ucTag_Index ).toString( );
				UTIL_LogPrint( "%s\n", strName.c_str( ) );
				strTag = CFG_GetString( strName, string( ) );
				if( strTag.empty( ) )
				{
					if( ucTagPrevious % 100 == 1 )
						ucTag_Index = ++ucTag * 100 + 1;
					strName = "bnbt_tag" + CAtomInt( ucTag_Index ).toString( );
					UTIL_LogPrint( "%s\n", strName.c_str( ) );
					strTag = CFG_GetString( strName, string( ) );
				}
			}

			strName = "bnbt_tag" + CAtomInt( ucTagPrevious ).toString( );
			CFG_Delete( strName );
			CFG_Close( CFG_FILE );
//			m_vecTags.clear( );
//			m_vecTagsMouse.clear( );
			initTags( );

			return JS_ReturnToPage( pRequest, pResponse, TAGS_HTML );
		}

		if( pRequest->mapParams["submit_tag_form_button"] == gmapLANG_CFG["Submit"] )
		{
			const unsigned int cucTagCount( (unsigned int)atoi( pRequest->mapParams["tag_count"].c_str( ) ) );

			unsigned char ucTagCountLimit = (unsigned char)( cucTagCount % 100 );
			unsigned int ucTagIndex = cucTagCount - ucTagCountLimit;

			string strName = string( );
			string strImageOff = string( );
	
			if( ucTagCountLimit == 0 )
				ucTagCountLimit = 1;

			for( unsigned char ucCount = 1; ucCount <= ucTagCountLimit; ucCount++ )
			{
				strName = pRequest->mapParams["bnbt_tag" + CAtomInt( ucTagIndex + ucCount ).toString( ) + "_name"];
				strImageOff = pRequest->mapParams["bnbt_tag" + CAtomInt( ucTagIndex + ucCount ).toString( ) + "_imageoff"];
				UTIL_LogPrint( string( "bnbt_tag" + CAtomInt( ucTagIndex + ucCount ).toString( ) + "_imageoff = %s\n" ).c_str( ), strImageOff.c_str( ) );

				if( strName.empty( ) )
					strName = "Tag" + CAtomInt( ucTagIndex + ucCount ).toString( );

				if( !strImageOff.empty( ) )
				{
					strImageOff = "|" + strImageOff;
				}

				CFG_SetString( "bnbt_tag" + CAtomInt( ucTagIndex + ucCount ).toString( ), strName + strImageOff );
			}

			strName = pRequest->mapParams["bnbt_tag_name"];

			if( !strName.empty( ) )
			{
				strImageOff = pRequest->mapParams["bnbt_tag_imageoff"];

				if( !strImageOff.empty( ) )
				{
					strImageOff = "|" + strImageOff;
				}

				CFG_SetString( "bnbt_tag" + CAtomInt( cucTagCount + 1 ).toString( ), strName + strImageOff );
			}

			CFG_Close( CFG_FILE );
//			m_vecTags.clear( );
//			m_vecTagsMouse.clear( );
			initTags( );

			return JS_ReturnToPage( pRequest, pResponse, TAGS_HTML );
		}

		// display
		HTML_Common_Begin( pRequest, pResponse, gmapLANG_CFG["admin_page"], string( CSS_ADMIN ), string( ), NOT_INDEX, CODE_200 );

		// javascript
		pResponse->strContent += "<script type=\"text/javascript\">\n";
		pResponse->strContent += "<!--\n";
		pResponse->strContent += "function delete_tag_confirm( TAG )\n";
		pResponse->strContent += "{\n";
		pResponse->strContent += "var name=confirm(\"" + gmapLANG_CFG["delete"] + " bnbt_tag\" + TAG )\n";
		pResponse->strContent += "if (name==true)\n";
		pResponse->strContent += "{\n";
		pResponse->strContent += "window.location=\"" + RESPONSE_STR_TAGS_HTML + "?ap_deltag=\" + TAG\n";
		pResponse->strContent += "}\n";
		pResponse->strContent += "}\n\n";
		pResponse->strContent += "function admin_swap_tag( TAG ) {\n";
		pResponse->strContent += "window.location=\"" + RESPONSE_STR_TAGS_HTML + "?ap_swaptag=\" + TAG\n";
		pResponse->strContent += "}\n\n";
		pResponse->strContent += "//-->\n";
		pResponse->strContent += "</script>\n\n";

		// table
		pResponse->strContent += "<div class=\"admin_table\">\n";
		pResponse->strContent += "<table summary=\"xadmintags\">\n";
		pResponse->strContent += "<tr>\n<th colspan=6>Admin Tags</th></tr>\n"; // NEW
		
		pResponse->strContent += "<form method=\"get\" name=\"admintags\" onSubmit=\"return confirm(\'" + gmapLANG_CFG["Submit"] + "\')\" action=\"" + RESPONSE_STR_TAGS_HTML + "\">\n";
		pResponse->strContent += "<tr>\n";
		pResponse->strContent += "<th>Key</th>\n";
		pResponse->strContent += "<th>Tag</th>\n";
		pResponse->strContent += "<th>Image Off</th>\n";
		pResponse->strContent += "<th>Image Off URL</th>\n";
		pResponse->strContent += "<th colspan=2>Action</th>\n";
		pResponse->strContent += "</tr>\n";

		unsigned char ucTag = 1;
		unsigned int ucTag_Index = ucTag * 100 + 1;
		unsigned int ucTagPrevious = 0;

		string strTagPrevious = string( );

		string strName = "bnbt_tag" + CAtomInt( ucTag_Index ).toString( );
		string strTag = CFG_GetString( strName, string( ) );

		string :: size_type iSplit = 0;
		string :: size_type iSplitMouse = 0;

		while( !strTag.empty( ) )
		{
		
			iSplit = strTag.find( "|" );
			iSplitMouse = strTag.find( "|", iSplit + 1 );

			ucTagPrevious = ucTag_Index;
			strTagPrevious = CAtomInt( ucTagPrevious ).toString( );

// 			strName = "bnbt_tag" + CAtomInt( ++ucTag ).toString( );
			strName = "bnbt_tag" + CAtomInt( ++ucTag_Index ).toString( );
			
			if( ucTag % 2 )
				pResponse->strContent += "<tr class=\"even\">\n";
			else
				pResponse->strContent += "<tr class=\"odd\">\n";

			if( iSplit == string :: npos ) 
			{
				if( iSplitMouse == string :: npos )
				{
					pResponse->strContent += "<td>bnbt_tag" + strTagPrevious + "</td>\n";
					pResponse->strContent += "<td><span style=\"display:none\"><label for=\"id_bnbt_tag" + strTagPrevious + "_name\">" + UTIL_RemoveHTML( strTag ) + " Name</label></span><input name=\"bnbt_tag" + strTagPrevious + "_name\" id=\"id_bnbt_tag" + strTagPrevious + "_name\" type=text size=25 value=\"" + strTag + "\"></td>\n";
					pResponse->strContent += "<td></td>\n";
					pResponse->strContent += "<td><span style=\"display:none\"><label for=\"id_bnbt_tag" + strTagPrevious + "_imageoff\">" + UTIL_RemoveHTML( strTag ) + " Image Off URL</label></span><input name=\"bnbt_tag" + strTagPrevious + "_imageoff\" id=\"id_bnbt_tag" + strTagPrevious + "_imageoff\" type=text size=40 value=\"\"></td>\n";


					pResponse->strContent += "<td>";

// 					if( ucTag > 2 )
					if( ( ucTag_Index % 100 ) > 2 )
					{
						pResponse->strContent += Button_JS_Link( "up_bnbt_tag" + strTagPrevious, "Move Up", "admin_swap_tag(" + CAtomInt( ucTag_Index - 2 ).toString( ) + ")" );
						pResponse->strContent += "<br>";
					}

					if( !CFG_GetString( "bnbt_tag" + CAtomInt( ucTag_Index ).toString( ), string( ) ).empty( ) )
						pResponse->strContent += Button_JS_Link( "down_bnbt_tag" + strTagPrevious, "Move Down", "admin_swap_tag(" + strTagPrevious + ")" );

					pResponse->strContent += "</td>\n";

					pResponse->strContent += "<td>";
					pResponse->strContent += Button_JS_Link( "bnbt_tag" + strTagPrevious, gmapLANG_CFG["delete"], "delete_tag_confirm(" + strTagPrevious +  ")" );
					pResponse->strContent += "</td>\n";
				}
				else
				{
					pResponse->strContent += "<td>bnbt_tag" + strTagPrevious + "</td>\n";
					pResponse->strContent += "<td><span style=\"display:none\"><label for=\"id_bnbt_tag" + strTagPrevious + "_name\">" + UTIL_RemoveHTML( strTag.substr( 0, iSplitMouse ) ) + " Name</label></span><input name=\"bnbt_tag" + strTagPrevious + "_name\" id=\"id_bnbt_tag" + strTagPrevious + "_name\" type=text size=25 value=\"" + UTIL_RemoveHTML( strTag.substr( 0, iSplitMouse ) ) + "\"></td>\n";
					pResponse->strContent += "<td><img title=\"" + strTag.substr( iSplitMouse + 1 ) + "\" name=\"name_bnbt_tag" + strTagPrevious + "_imageoff\" alt=\"[" + strTag.substr( iSplitMouse + 1 ) + "]\" src=\"" + strTag.substr( iSplitMouse + 1 ) + "\"></td>\n";

					pResponse->strContent += "<td>";

// 					if( ucTag > 2 )
					if( ( ucTag_Index % 100 ) > 2 )
					{
						pResponse->strContent += Button_JS_Link( "up_bnbt_tag" + strTagPrevious, "Move Up", "admin_swap_tag(" + CAtomInt( ucTag_Index - 2 ).toString( ) + ")" );
						pResponse->strContent += "<br>";
					}

					if( !CFG_GetString( "bnbt_tag" + CAtomInt( ucTag_Index ).toString( ), string( ) ).empty( ) )
						pResponse->strContent += Button_JS_Link( "down_bnbt_tag" + strTagPrevious, "Move Down", "admin_swap_tag(" + strTagPrevious + ")" );

					pResponse->strContent += "</td>\n";

					pResponse->strContent += "<td>";
					pResponse->strContent += Button_JS_Link( "bnbt_tag" + strTagPrevious, gmapLANG_CFG["delete"], "delete_tag_confirm(" + strTagPrevious +  ")" );
					pResponse->strContent += "</td>\n";
				}
			}
			else
			{
				if( iSplitMouse == string :: npos )
				{
					pResponse->strContent += "<td>bnbt_tag" + strTagPrevious + "</td>\n";
					pResponse->strContent += "<td><span style=\"display:none\"><label for=\"id_bnbt_tag" + strTagPrevious + "_name\">" + UTIL_RemoveHTML( strTag.substr( 0, iSplit ) ) + " Name</label></span><input name=\"bnbt_tag" + strTagPrevious + "_name\" id=\"id_bnbt_tag" + strTagPrevious + "_name\" type=text size=25 value=\"" + UTIL_RemoveHTML( strTag.substr( 0, iSplit ) ) + "\"></td>\n";
					pResponse->strContent += "<td><img title=\"" + strTag.substr( iSplit + 1 ) + "\" name=\"name_bnbt_tag" + strTagPrevious + "_imageoff\" alt=\"[" + strTag.substr( iSplit + 1 ) + "]\" src=\"" + strTag.substr( iSplit + 1 ) + "\"></td>\n";
					pResponse->strContent += "<td><span style=\"display:none\"><label for=\"id_bnbt_tag" + strTagPrevious + "_imageoff\">" + UTIL_RemoveHTML( strTag.substr( 0, iSplit ) ) + " Image Off URL</label></span><input name=\"bnbt_tag" + strTagPrevious + "_imageoff\" id=\"id_bnbt_tag" + strTagPrevious + "_imageoff\" type=text size=40 value=\"" + UTIL_RemoveHTML( strTag.substr( iSplit + 1 ) ) + "\"></td>\n";

					pResponse->strContent += "<td>";

// 					if( ucTag > 2 )
					if( ( ucTag_Index % 100 ) > 2 )
					{
						pResponse->strContent += Button_JS_Link( "up_bnbt_tag" + strTagPrevious, "Move Up", "admin_swap_tag(" + CAtomInt( ucTag_Index - 2 ).toString( ) + ")" );
						pResponse->strContent += "<br>";
					}

					if( !CFG_GetString( "bnbt_tag" + CAtomInt( ucTag_Index ).toString( ), string( ) ).empty( ) )
						pResponse->strContent += Button_JS_Link( "down_bnbt_tag" + strTagPrevious, "Move Down", "admin_swap_tag(" + strTagPrevious + ")" );

					pResponse->strContent += "</td>\n";

					pResponse->strContent += "<td>";
					pResponse->strContent += Button_JS_Link( "bnbt_tag" + strTagPrevious, gmapLANG_CFG["delete"], "delete_tag_confirm(" + strTagPrevious +  ")" );
					pResponse->strContent += "</td>\n";
				}
				else
				{
					pResponse->strContent += "<td>bnbt_tag" + strTagPrevious + "</td>\n";
					pResponse->strContent += "<td><span style=\"display:none\"><label for=\"id_bnbt_tag" + strTagPrevious + "_name\">" + UTIL_RemoveHTML( strTag.substr( 0, iSplit ) ) + " Name</label></span><input name=\"bnbt_tag" + strTagPrevious + "_name\" id=\"id_bnbt_tag" + strTagPrevious + "_name\" type=text size=25 value=\"" + UTIL_RemoveHTML( strTag.substr( 0, iSplit ) ) + "\"></td>\n";
					pResponse->strContent += "<td><img title=\"" + strTag.substr( iSplitMouse + 1 ) + "\" name=\"name_bnbt_tag" + strTagPrevious + "_imageoff\" alt=\"[" + strTag.substr( iSplitMouse + 1 ) + "]\" src=\"" + strTag.substr( iSplitMouse + 1 ) + "\"></td>\n";
					pResponse->strContent += "<td><span style=\"display:none\"><label for=\"id_bnbt_tag" + strTagPrevious + "_imageoff\">" + UTIL_RemoveHTML( strTag.substr( 0, iSplit ) ) + " Image Off URL</label></span><input name=\"bnbt_tag" + strTagPrevious + "_imageoff\" id=\"id_bnbt_tag" + strTagPrevious + "_imageoff\" type=text size=40 value=\"" + UTIL_RemoveHTML( strTag.substr( iSplitMouse + 1 ) ) + "\"></td>\n";

					pResponse->strContent += "<td>";

// 					if( ucTag > 2 )
					if( ( ucTag_Index % 100 ) > 2 )
					{
						pResponse->strContent += Button_JS_Link( "up_bnbt_tag" + strTagPrevious, "Move Up", "admin_swap_tag(" + CAtomInt( ucTag_Index - 2 ).toString( ) + ")" );
						pResponse->strContent += "<br>";
					}

					if( !CFG_GetString( "bnbt_tag" + CAtomInt( ucTag_Index ).toString( ), string( ) ).empty( ) )
						pResponse->strContent += Button_JS_Link( "down_bnbt_tag" + strTagPrevious, "Move Down", "admin_swap_tag(" + strTagPrevious + ")" );

					pResponse->strContent += "</td>\n";

					pResponse->strContent += "<td>";
					pResponse->strContent += Button_JS_Link( "bnbt_tag" + strTagPrevious, gmapLANG_CFG["delete"], "delete_tag_confirm(" + strTagPrevious + ")" );
					pResponse->strContent += "</td>\n";
				}
			}   

			pResponse->strContent += "</tr>\n";
			strTag = CFG_GetString( strName, string( ) );
			if( strTag.empty( ) )
			{
				if( ucTag % 2 )
					pResponse->strContent += "<tr class=\"even\">\n";
				else
					pResponse->strContent += "<tr class=\"odd\">\n";
				pResponse->strContent += "<td>*bnbt_tag" + CAtomInt( ucTag_Index ).toString( ) + "</td>\n";
				pResponse->strContent += "<td><span style=\"display:none\"><label for=\"id_bnbt_tag_name\">New Name</label></span><input name=\"bnbt_tag_name\" id=\"id_bnbt_tag_name\" type=text size=25></td>\n";
				pResponse->strContent += "<td></td>\n";
				pResponse->strContent += "<td><span style=\"display:none\"><label for=\"id_bnbt_tag_imageoff\">New Image Off URL</label></span><input name=\"bnbt_tag_imageoff\" id=\"id_bnbt_tag_imageoff\" type=text size=40></td>\n";

				pResponse->strContent += "<td class=\"number_red\">";
				pResponse->strContent += Button_Submit( "submit_tag_form", gmapLANG_CFG["Submit"] );
				pResponse->strContent += "</td>\n";
				
				pResponse->strContent += "<td class=\"number_red\">";
				pResponse->strContent += Button_Reset( "reset_tag_form", gmapLANG_CFG["reset"] );
				pResponse->strContent += "</td>\n";

				pResponse->strContent += "</tr>\n";
				
				pResponse->strContent += "<p><input name=\"tag_count\" type=hidden value=\"" + strTagPrevious + "\"></p>\n";
				pResponse->strContent += "</form>\n";
				
				pResponse->strContent += "<form method=\"get\" name=\"admintags\" onSubmit=\"return confirm(\'" + gmapLANG_CFG["Submit"] + "\')\" action=\"" + RESPONSE_STR_TAGS_HTML + "\">\n";
				pResponse->strContent += "<tr>\n";
				pResponse->strContent += "<th>Key</th>\n";
				pResponse->strContent += "<th>Tag</th>\n";
				pResponse->strContent += "<th>Image Off</th>\n";
				pResponse->strContent += "<th>Image Off URL</th>\n";
				pResponse->strContent += "<th colspan=2>Action</th>\n";
				pResponse->strContent += "</tr>\n";
				
				ucTag_Index = ++ucTag * 100 + 1;
				strName = "bnbt_tag" + CAtomInt( ucTag_Index ).toString( );
				strTag = CFG_GetString( strName, string( ) );
			}
		}
		
		if( ( ucTagPrevious ) % 2 )
			pResponse->strContent += "<tr class=\"even\">\n";
		else
			pResponse->strContent += "<tr class=\"odd\">\n";
		
		ucTag_Index = ucTag * 100 + 1;
		strTagPrevious = CAtomInt( ucTag_Index - 1 ).toString( );

		pResponse->strContent += "<td>*bnbt_tag" + CAtomInt( ucTag_Index ).toString( ) + "</td>\n";
		pResponse->strContent += "<td><span style=\"display:none\"><label for=\"id_bnbt_tag_name\">New Name</label></span><input name=\"bnbt_tag_name\" id=\"id_bnbt_tag_name\" type=text size=25></td>\n";
		pResponse->strContent += "<td></td>\n";
		pResponse->strContent += "<td><span style=\"display:none\"><label for=\"id_bnbt_tag_imageoff\">New Image Off URL</label></span><input name=\"bnbt_tag_imageoff\" id=\"id_bnbt_tag_imageoff\" type=text size=40></td>\n";

		pResponse->strContent += "<td class=\"number_red\">";
		pResponse->strContent += Button_Submit( "submit_tag_form", gmapLANG_CFG["Submit"] );
		pResponse->strContent += "</td>\n";

		pResponse->strContent += "<td class=\"number_red\">";
		pResponse->strContent += Button_Reset( "reset_tag_form", gmapLANG_CFG["reset"] );
		pResponse->strContent += "</td>\n";

		pResponse->strContent += "</tr>\n";

// 		pResponse->strContent += "</table>\n";
		pResponse->strContent += "<p><input name=\"tag_count\" type=hidden value=\"" + strTagPrevious + "\"></p>\n";
		pResponse->strContent += "</form>\n";
		pResponse->strContent += "</table>\n";
		pResponse->strContent += "</div>\n\n";

		// Output common HTML ending
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_ADMIN ) );
	}
	else
	{
		// not authorized
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["admin_page"], string( CSS_ADMIN ), string( ), NOT_INDEX, CODE_401 );

		// Output common HTML ending
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_ADMIN ) );
	}
}

void CTracker :: serverResponseXTorrent( struct request_t *pRequest, struct response_t *pResponse )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
		if( IsIPBanned( pRequest, pResponse, btv, gmapLANG_CFG["admin_page"], string( CSS_ADMIN ), NOT_INDEX ) )
			return;

	if( !pRequest->user.strUID.empty( ) && ( pRequest->user.ucAccess & m_ucAccessAdmin ) )
	{
		//
		// Swap announce address
		//

		const unsigned char cucSwapAnnounce( (unsigned char)atoi( pRequest->mapParams["ap_swapannounce"].c_str( ) ) );

		if( cucSwapAnnounce != 0 )
		{
			unsigned char ucAnnounceList = 1;
			string strKeyNext = string( );
			string strAnnounceNext = string( );

			string strKey = "xbnbt_announce_list" + CAtomInt( ucAnnounceList ).toString( );
			string strAnnounceUrl = CFG_GetString( strKey, string( ) );

			while( !strAnnounceUrl.empty( ) )
			{
				if( ucAnnounceList == cucSwapAnnounce )
				{
					strKeyNext = "xbnbt_announce_list" + CAtomInt( ucAnnounceList + 1 ).toString( );
					strAnnounceNext = CFG_GetString( strKeyNext, string( ) );

					CFG_SetString( strKey, strAnnounceNext );
					CFG_SetString( strKeyNext, strAnnounceUrl );

					ucAnnounceList++;
				}
				else
				{
					strKey = "xbnbt_announce_list" + CAtomInt( ucAnnounceList ).toString( );

					CFG_SetString( strKey, strAnnounceUrl );
				}

				strKey = "xbnbt_announce_list" + CAtomInt( ++ucAnnounceList ).toString( );

				strAnnounceUrl = CFG_GetString( strKey, string( ) );
			}

			CFG_Close( CFG_FILE );

			return JS_ReturnToPage( pRequest, pResponse, XTORRENT_HTML );
		}

		//
		// Delete announce address
		//

		const unsigned char cucDelAnnounce( (unsigned char)atoi( pRequest->mapParams["ap_delannounce"].c_str( ) ) );

		if( cucDelAnnounce != 0 )
		{
			unsigned char ucAnnounce = 1;
			unsigned char ucAnnouncePrevious = 0;

			string strName = "xbnbt_announce_list" + CAtomInt( ucAnnounce ).toString( );
			string strAnnounce = CFG_GetString( strName, string( ) );

			while( !strAnnounce.empty( ) )
			{
				if( ucAnnounce == cucDelAnnounce )
					CFG_Delete( strName );

				if( ucAnnounce > cucDelAnnounce )
				{
					strName = "xbnbt_announce_list" + CAtomInt( ucAnnouncePrevious ).toString( );
					CFG_SetString( strName, strAnnounce );
				}

				ucAnnouncePrevious = ucAnnounce;

				strName = "xbnbt_announce_list" + CAtomInt( ++ucAnnounce ).toString( );

				strAnnounce = CFG_GetString( strName, string( ) );
			}

			strName = "xbnbt_announce_list" + CAtomInt( ucAnnouncePrevious ).toString( );
			CFG_Delete( strName );
			CFG_Close( CFG_FILE );

			return JS_ReturnToPage( pRequest, pResponse, XTORRENT_HTML );
		}

		//
		// Submit the announce form
		//

// 		if( pRequest->mapParams["submit_announce_form_button"] == STR_SUBMIT )
		if( pRequest->mapParams["submit_announce_form_button"] == gmapLANG_CFG["Submit"] )
		{
			const unsigned char cucAnnounceCount( (unsigned char)atoi( pRequest->mapParams["announce_count"].c_str( ) ) );

			unsigned char ucAnnounceCountLimit = cucAnnounceCount;

			string strAnnounceURL = pRequest->mapParams["bnbt_force_announce_url"];
	
			CFG_SetString( "bnbt_force_announce_url", strAnnounceURL );

			m_strForceAnnounceURL = strAnnounceURL;
			
			if( ucAnnounceCountLimit == 0 )
				ucAnnounceCountLimit = 1;

			for( unsigned char ucCount = 1; ucCount <= ucAnnounceCountLimit; ucCount++ )
			{
				strAnnounceURL = pRequest->mapParams["xbnbt_announce_list" + CAtomInt( ucCount ).toString( ) + "_url"];

				CFG_SetString( "xbnbt_announce_list" + CAtomInt( ucCount ).toString( ), strAnnounceURL );
			}

			strAnnounceURL = pRequest->mapParams["xbnbt_announce_list_url"];

			if( !strAnnounceURL.empty( ) )
				CFG_SetString( "xbnbt_announce_list" + CAtomInt( cucAnnounceCount + 1 ).toString( ), strAnnounceURL );

			const string cstrForceAnnounceDL( pRequest->mapParams["bnbt_force_announce_on_download"] );

			if( cstrForceAnnounceDL == "on" || cstrForceAnnounceDL == "On" )
			{
				CFG_SetInt( "bnbt_force_announce_on_download", 1);
				m_bForceAnnounceOnDL = true;
			}
			else
			{
				CFG_SetInt( "bnbt_force_announce_on_download", 0);
				m_bForceAnnounceOnDL = false;
			}

			const string cstrEnableAnnounceList( pRequest->mapParams["bnbt_announce_list_enable"] );

			if( cstrEnableAnnounceList == "on" || cstrEnableAnnounceList == "On" )
			{
				CFG_SetInt( "bnbt_announce_list_enable", 1);
				m_bEnableAnnounceList = true;
			}
			else
			{
				CFG_SetInt( "bnbt_announce_list_enable", 0);
				m_bEnableAnnounceList = false;
			}

			const string cstrForceAnnounceUL( pRequest->mapParams["bnbt_force_announce_on_upload"] );

			if( cstrForceAnnounceUL == "on" || cstrForceAnnounceUL == "On" )
			{
				CFG_SetInt( "bnbt_force_announce_on_upload", 1);
				m_bForceAnnounceOnUL = true;
			}
			else
			{
				CFG_SetInt( "bnbt_force_announce_on_upload", 0);
				m_bForceAnnounceOnUL = false;
			}

			CFG_Close( CFG_FILE );

			return JS_ReturnToPage( pRequest, pResponse, XTORRENT_HTML );
		}

		//
		// display the announce page
		//

		xtorrent.bForceAnnounceDL = CFG_GetInt( "bnbt_force_announce_on_download", 0 ) == 0 ? false : true;
		xtorrent.bForceAnnounceUL = CFG_GetInt( "bnbt_force_announce_on_upload", 0 ) == 0 ? false : true;
		xtorrent.bEnableAnnounceList = CFG_GetInt( "bnbt_announce_list_enable", 0 ) == 0 ? false : true;

		// Output common HTML head
		HTML_Common_Begin( pRequest, pResponse, gmapLANG_CFG["admin_page"], string( CSS_ADMIN ), string( ), NOT_INDEX, CODE_200 );

		// javascript
		pResponse->strContent += "<script type=\"text/javascript\">\n";
		pResponse->strContent += "<!--\n";
		pResponse->strContent += "function delete_announce_confirm( ANNOUNCE )\n";
		pResponse->strContent += "{\n";
		pResponse->strContent += "var name=confirm(\"" + gmapLANG_CFG["delete"] + " xbnbt_announce_list\" + ANNOUNCE )\n";
		pResponse->strContent += "if (name==true)\n";
		pResponse->strContent += "{\n";
		pResponse->strContent += "window.location=\"" + RESPONSE_STR_XTORRENT_HTML + "?ap_delannounce=\" + ANNOUNCE\n";
		pResponse->strContent += "}\n";
		pResponse->strContent += "}\n\n";
		pResponse->strContent += "function admin_swap_announce( ANNOUNCE ) {\n";
		pResponse->strContent += "window.location=\"" + RESPONSE_STR_XTORRENT_HTML + "?ap_swapannounce=\" + ANNOUNCE\n";
		pResponse->strContent += "}\n\n";
		pResponse->strContent += "//-->\n";
		pResponse->strContent += "</script>\n\n";

		// table
		pResponse->strContent += "<div class=\"admin_table\">\n";
		pResponse->strContent += "<form method=\"get\" name=\"adminxtorrent\" onSubmit=\"return confirm(\'" + gmapLANG_CFG["Submit"] + "\')\" action=\"" + RESPONSE_STR_XTORRENT_HTML + "\">\n";
		pResponse->strContent += "<table summary=\"adminxtorrent\">\n";
		pResponse->strContent += "<tr>\n";
		pResponse->strContent += "<th colspan=2>Admin XTorrent</th>";
		pResponse->strContent += "<th colspan=1><label for=\"id_bnbt_force_announce_on_download\">Force Announce DL</label><input name=\"bnbt_force_announce_on_download\" id=\"id_bnbt_force_announce_on_download\" type=checkbox";

		if( xtorrent.bForceAnnounceDL )
			pResponse->strContent += " checked";

		pResponse->strContent += "></th>\n";
		pResponse->strContent += "<th colspan=1><label for=\"id_bnbt_force_announce_on_upload\">Force Announce UL</label><input name=\"bnbt_force_announce_on_upload\" id=\"id_bnbt_force_announce_on_upload\" type=checkbox";

		if( xtorrent.bForceAnnounceUL )
			pResponse->strContent += " checked";

		pResponse->strContent += "></th>\n";
		pResponse->strContent += "</tr>\n";
		pResponse->strContent += "<tr>\n";
		pResponse->strContent += "<th>Key</th>\n";
		pResponse->strContent += "<th>Announce URL</th>\n";
		pResponse->strContent += "<th colspan=2>Action</th>\n";
		pResponse->strContent += "</tr>\n";

		unsigned char ucAnnounce = 1;
		unsigned char ucAnnouncePrevious = 0;
		string strAnnouncePrevious = string( );
		string strKey = "xbnbt_announce_list" + CAtomInt( ucAnnounce ).toString( );
		string strAnnounceUrl = CFG_GetString( strKey, string( ) );
		string strForceAnnounceUrl = CFG_GetString( "bnbt_force_announce_url", string( ) );

		pResponse->strContent += "<tr class=\"even\">\n";
		pResponse->strContent += "<td>bnbt_force_announce_url</td>\n";
		pResponse->strContent += "<td><span style=\"display:none\"><label for=\"id_bnbt_force_announce_url\">Force Announce URL</label></span><input name=\"bnbt_force_announce_url\" id=\"id_bnbt_force_announce_url\" type=text size=60 value=\"" + UTIL_RemoveHTML( strForceAnnounceUrl ) + "\"></td>\n";
		pResponse->strContent += "<td></td>\n";
		pResponse->strContent += "<td></td>\n";
		pResponse->strContent += "</tr>\n";

		if( ucAnnounce % 2 )
			pResponse->strContent += "<tr class=\"even\">\n";
		else
			pResponse->strContent += "<tr class=\"odd\">\n";

		pResponse->strContent += "<th colspan=2></th>";
		pResponse->strContent += "<th colspan=2><label for=\"id_bnbt_announce_list_enable\">Announce List</label><input name=\"bnbt_announce_list_enable\" id=\"id_bnbt_announce_list_enable\" type=checkbox";

		if( xtorrent.bEnableAnnounceList )
			pResponse->strContent += " checked";

		pResponse->strContent += "></th>\n";
		pResponse->strContent += "</tr>\n";

		while( !strAnnounceUrl.empty( ) )
		{
			ucAnnouncePrevious = ucAnnounce;
			strAnnouncePrevious = CAtomInt( ucAnnouncePrevious ).toString( );

			strKey = "xbnbt_announce_list" + CAtomInt( ++ucAnnounce ).toString( );

			if( ucAnnounce % 2 )
				pResponse->strContent += "<tr class=\"even\">\n";
			else
				pResponse->strContent += "<tr class=\"odd\">\n";
 
			pResponse->strContent += "<td>xbnbt_announce_list" + strAnnouncePrevious + "</td>\n";
			pResponse->strContent += "<td><span style=\"display:none\"><label for=\"id_xbnbt_announce_list" + strAnnouncePrevious + "_url\">Announce URL " + strAnnouncePrevious + "</label></span><input name=\"xbnbt_announce_list" + strAnnouncePrevious + "_url\" id=\"id_xbnbt_announce_list" + strAnnouncePrevious + "_url\" type=text size=60 value=\"" + UTIL_RemoveHTML( strAnnounceUrl ) + "\"></td>\n";
			pResponse->strContent += "<td>";

			if( ucAnnounce > 2 )
			{
				pResponse->strContent += Button_JS_Link( "up_xbnbt_announce_list" + strAnnouncePrevious, "Move Up", "admin_swap_announce(" + CAtomInt( ucAnnounce - 2 ).toString( ) + ")" );
				pResponse->strContent += "<br>";
			}

			if( !CFG_GetString( "xbnbt_announce_list" + CAtomInt( ucAnnounce ).toString( ), string( ) ).empty( ) )
				pResponse->strContent += Button_JS_Link( "down_xbnbt_announce_list" + strAnnouncePrevious, "Move Down", "admin_swap_announce(" + strAnnouncePrevious + ")" );

			pResponse->strContent += "</td>\n";
			pResponse->strContent += "<td>";
			pResponse->strContent += Button_JS_Link( "xbnbt_announce_list" + strAnnouncePrevious, gmapLANG_CFG["delete"], "delete_announce_confirm(" + strAnnouncePrevious +  ")" );
			pResponse->strContent += "</td>\n";
			pResponse->strContent += "</tr>\n";

			strAnnounceUrl = CFG_GetString( strKey, string( ) );
		}

		if( ( ucAnnouncePrevious ) % 2 )
			pResponse->strContent += "<tr class=\"even\">\n";
		else
			pResponse->strContent += "<tr class=\"odd\">\n";

		pResponse->strContent += "<td>*xbnbt_announce_list" + CAtomInt( ucAnnounce ).toString( ) + "</td>\n";
		pResponse->strContent += "<td><span style=\"display:none\"><label for=\"id_xbnbt_announce_list_url\">New Announce URL</label></span><input name=\"xbnbt_announce_list_url\" id=\"id_xbnbt_announce_list_url\" type=text size=60></td>\n";
		pResponse->strContent += "<td class=\"number_red\">";
		pResponse->strContent += Button_Submit( "submit_announce_form", gmapLANG_CFG["Submit"] );
		pResponse->strContent += "</td>\n";
		pResponse->strContent += "<td class=\"number_red\">";
		pResponse->strContent += Button_Reset( "reset_announce_form", gmapLANG_CFG["reset"] );
		pResponse->strContent += "</td>\n";
		pResponse->strContent += "</tr>\n";
		pResponse->strContent += "</table>\n";
		pResponse->strContent += "<p><input name=\"announce_count\" type=hidden value=\"" + strAnnouncePrevious + "\"></p>\n";
		pResponse->strContent += "</form>\n";
		pResponse->strContent += "</div>\n\n";

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_ADMIN ) );
	}
	else
	{
		// not authorized

		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["admin_page"], string( CSS_ADMIN ), string( ), NOT_INDEX, CODE_401 );

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_ADMIN ) );
	}
}
