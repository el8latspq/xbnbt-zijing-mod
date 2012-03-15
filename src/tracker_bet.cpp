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
#include "tracker.h"
#include "util.h"
void CTracker :: serverResponseBetGET( struct request_t *pRequest, struct response_t *pResponse )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
		if( IsIPBanned( pRequest, pResponse, btv, gmapLANG_CFG["bet_page"], string( CSS_BET ), NOT_INDEX ) )
			return;

//	if( !pRequest->user.strUID.empty( ) && ( m_ucInfoAccess == 0 || ( pRequest->user.ucAccess & m_ucInfoAccess ) ) )
	if( !pRequest->user.strUID.empty( ) && ( pRequest->user.ucAccess & m_ucAccessView ) )
	{
		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["bet_page"], string( CSS_BET ), string( ), NOT_INDEX, CODE_200 );

		pResponse->strContent += "<script type=\"text/javascript\">\n";
		pResponse->strContent += "<!--\n";

		pResponse->strContent += "function validatebonus(theform,min,max,bonus) {\n";
		pResponse->strContent += "  var element0 = document.getElementById( 'bet' + theform.bet_for.value + '_option0' );\n";
		pResponse->strContent += "  var betbonus = theform.bet_bonus.value;\n";
		pResponse->strContent += "  if( min > 0 && max > 0 ) {\n";
		pResponse->strContent += "    if( ( Math.floor( betbonus ) == betbonus && betbonus >= min && betbonus <= max && betbonus * 100 <= bonus ) || element0.checked )\n";
		pResponse->strContent += "      return true;\n";
		pResponse->strContent += "    else {\n";
		pResponse->strContent += "      alert('" + gmapLANG_CFG["bet_bonus_notvalid"] + "');\n";
		pResponse->strContent += "      return false; }\n";
		pResponse->strContent += "  }\n";
		pResponse->strContent += "  else\n";
		pResponse->strContent += "    return true;\n";
		pResponse->strContent += "}\n";
		
		pResponse->strContent += "//-->\n";
		pResponse->strContent += "</script>\n\n";

//		CMySQLQuery mq00( "UPDATE bets SET bclosed=NOW(),bautoclose=0 WHERE bopen!=0 AND bclosed=0 AND bdealed=0 AND bautoclose>0 AND bautoclose<NOW()" );
		time_t now_t = time( 0 );

		pResponse->strContent += "<div class=\"bet_table\">\n";
		pResponse->strContent += "<table class=\"bet_table\" summary=\"tbet\">\n";
		pResponse->strContent += "<tr><th colspan=2>" + gmapLANG_CFG["bet"] + "</th></tr>\n";
	
		pResponse->strContent += "<tr class=\"betheader\">\n";
		pResponse->strContent += "<td class=\"admin\" colspan=2>[<a class=\"black\" href=\"" + RESPONSE_STR_BETS_HTML + "\">" + gmapLANG_CFG["bet_all"] + "</a>]</td>";
		pResponse->strContent += "</tr>\n";
	
		CMySQLQuery *pQueryBets = new CMySQLQuery( "SELECT bid,btitle,bopen,bclosed,bbetbonus_min,bbetbonus_max,bbetnote,bautoclose,bbetcount FROM bets WHERE bopen!=0 AND bdealed=0 ORDER BY bopen DESC" );
	
		vector<string> vecQueryBets;

		vecQueryBets.reserve(9);
	
		vecQueryBets = pQueryBets->nextRow( );
	
//		if( vecQueryBets.size( ) == 0 )
//		{
//			delete pQueryBets;
//		
//			pQueryBets = new CMySQLQuery( "SELECT bid,btitle,bopen,bclosed,UNIX_TIMESTAMP(bclosed),bbetbonus_min,bbetbonus_max,bbetcount FROM bets WHERE bopen!=0 ORDER BY bopen DESC LIMIT 1" );
//		
//			vecQueryBets = pQueryBets->nextRow( );
//		}
	
		while( vecQueryBets.size( ) == 9 )
		{
			string strOption = string( );
			string strBonus = string( );
		
			CMySQLQuery *pQueryTicket = new CMySQLQuery( "SELECT boptionid,bbetbonus FROM betsticket WHERE bid=" + vecQueryBets[0] + " AND buid=" + pRequest->user.strUID + " ORDER BY boptionid" );

			vector<string> vecQueryTicket;

			vecQueryTicket.reserve(2);

			vecQueryTicket = pQueryTicket->nextRow( );
	
			delete pQueryTicket;
	
			if( vecQueryTicket.size( ) == 2 )
			{
				strOption = vecQueryTicket[0];
				strBonus = vecQueryTicket[1];
			}
		
//			pResponse->strContent += "<tr class=\"betheader\">\n";
//			pResponse->strContent += "<th class=\"bettime\">" + gmapLANG_CFG["bet_time_open"] + "</th>\n";
//			pResponse->strContent += "<td class=\"bettime\">" + vecQueryBets[2] + "</th>\n";
//			pResponse->strContent += "<th class=\"bettime\">" + gmapLANG_CFG["bet_time_closed"] + "</th>\n";
//			pResponse->strContent += "<td class=\"bettime\">" + vecQueryBets[3] + "</th>\n";
		
//			pResponse->strContent += "</tr>\n";
		
			CMySQLQuery *pQueryBet = new CMySQLQuery( "SELECT boptionid,boption,bbetrate,bbetcount FROM betsoption WHERE bid=" + vecQueryBets[0] + " ORDER BY boptionid" );
	
			vector<string> vecQueryBet;

			vecQueryBet.reserve(4);

			vecQueryBet = pQueryBet->nextRow( );
		
			if( vecQueryBet.size( ) == 4 )
			{
				bool bClosed = false;
				string strSkip = string( );
				unsigned long ulBonusTotal = 0;
				bool bChecked = false;
			
				if( vecQueryBets[3] != m_strMySQLTimeZero )
					bClosed = true;
				else if( vecQueryBets[7] != m_strMySQLTimeZero )
				{
					struct tm time_tm;
					int64 year, month, day, hour, minute, second;
					sscanf( vecQueryBets[7].c_str( ), "%d-%d-%d %d:%d:%d",&year,&month,&day,&hour,&minute,&second );
					time_tm.tm_year = year-1900;
					time_tm.tm_mon = month-1;
					time_tm.tm_mday = day;
					time_tm.tm_hour = hour;
					time_tm.tm_min = minute;
					time_tm.tm_sec = second;

					if( now_t > mktime(&time_tm) )
					{
						CMySQLQuery mq00( "UPDATE bets SET bclosed=NOW(),bautoclose=0 WHERE bid=" + vecQueryBets[0] );
						bClosed = true;
					}
				}

				if( strOption.empty( ) && !bClosed )
				{
					pResponse->strContent += "<form name=\"bet\" method=\"post\" action=\"" + RESPONSE_STR_BETS_HTML + "\" onSubmit=\"return validatebonus(this," + vecQueryBets[4] + "," + vecQueryBets[5] + "," + CAtomLong( pRequest->user.ulBonus ).toString( ) + ")\" enctype=\"multipart/form-data\">\n";
					pResponse->strContent += "<input name=\"bet_for\" type=hidden value=\"" + vecQueryBets[0] + "\">\n";
				}
				pResponse->strContent += "<tr class=\"bet\">";
				pResponse->strContent += "<td class=\"bet\" colspan=2>";
				pResponse->strContent += "<p class=\"bettitle\">" + UTIL_RemoveHTML2( vecQueryBets[1] );
				if( bClosed )
					pResponse->strContent += "<br><span class=\"red\">" + UTIL_RemoveHTML( gmapLANG_CFG["bet_closed"] ) + "</span>";
				else if( vecQueryBets[7] != m_strMySQLTimeZero )
					pResponse->strContent += "<br><span class=\"green\">" + UTIL_Xsprintf( gmapLANG_CFG["bet_autoclose_at"].c_str( ), vecQueryBets[7].c_str( ) ) + "</span>";
				pResponse->strContent += "</p>";
				pResponse->strContent += "<p class=\"bettitle\">[<a target=\"_blank\" href=\"" + RESPONSE_STR_TALK_HTML + "?tag=" + UTIL_StringToEscaped( gmapLANG_CFG["bet_match"] + vecQueryBets[0] ) + "&amp;tochannel=" + UTIL_StringToEscaped( gmapLANG_CFG["talk_channel_bets"] ) + "\">" + gmapLANG_CFG["bet_talk"] + "</a>]</p>";

				pResponse->strContent += "<table class=\"betoption\">\n";
				pResponse->strContent += "<tr class=\"betoption\">";
				pResponse->strContent += "<th class=\"betheader\">" + gmapLANG_CFG["bet_option"] + "</th>";
				pResponse->strContent += "<th class=\"betheader\">" + gmapLANG_CFG["bet_rate"] + "</th>";
				if( !strOption.empty( ) || bClosed )
				{
					pResponse->strContent += "<th class=\"betheader\">" + gmapLANG_CFG["bet_count"] + "</th>";
					pResponse->strContent += "<th class=\"betheader\">" + gmapLANG_CFG["bet_bonus_option"] + "</th>";
				}
				pResponse->strContent += "</th></tr>";

				while( vecQueryBet.size( ) == 4 )
				{
					if( vecQueryBet[0] != "0" )
					{
						pResponse->strContent += "<tr class=\"betoption";
						if( strOption.empty( ) && vecQueryBets[3] == m_strMySQLTimeZero )
						{
							pResponse->strContent += "\">";
							pResponse->strContent += "<td class=\"betinput\"><input name=\"bet_select\" type=radio value=\"" + vecQueryBet[0] + "\"";
							if( !bChecked )
							{
								pResponse->strContent += " checked";
								bChecked = true;
							}
							pResponse->strContent += ">" + UTIL_RemoveHTML( vecQueryBet[1] ) + "</td>";
							pResponse->strContent += "<td class=\"betoption\">";
							if( vecQueryBet[2] == "0.00" )
								pResponse->strContent += gmapLANG_CFG["bet_rate_dynamic"];
							else
								pResponse->strContent += vecQueryBet[2];
							pResponse->strContent += "</td>";
						}
						else
						{
							if( vecQueryBet[0] == strOption )
								pResponse->strContent += "ed";
							pResponse->strContent += "\">";
							pResponse->strContent += "<td class=\"betoption\">";
							pResponse->strContent += UTIL_RemoveHTML( vecQueryBet[1] ) + "</td>";
							pResponse->strContent += "<td class=\"betoption\">";
							if( vecQueryBet[2] == "0.00" )
								pResponse->strContent += gmapLANG_CFG["bet_rate_dynamic"];
							else
								pResponse->strContent += vecQueryBet[2];
							pResponse->strContent += "</td>";
							pResponse->strContent += "<td class=\"betcount\">" + vecQueryBet[3] + "</td>";

							CMySQLQuery *pQueryBonus = new CMySQLQuery( "SELECT boptionid,SUM(bbetbonus) FROM betsticket WHERE bid=" + vecQueryBets[0] + " AND boptionid=" + vecQueryBet[0] + " GROUP BY boptionid" );

							vector<string> vecQueryBonus;

							vecQueryBonus.reserve(2);

							vecQueryBonus = pQueryBonus->nextRow( );
					
							delete pQueryBonus;

							pResponse->strContent += "<td class=\"betbonus\">";

							if( vecQueryBet[0] == strOption )
								pResponse->strContent += strBonus + "/";
							if( vecQueryBonus.size( ) == 2 )
							{
								pResponse->strContent += vecQueryBonus[1];
								ulBonusTotal += UTIL_StringTo64( vecQueryBonus[1].c_str( ) );
							}
							else
								pResponse->strContent += "0";

							pResponse->strContent += "</td>";
						}
						pResponse->strContent += "</tr>";
					}
					else
					{
						if( strOption.empty( ) && !bClosed )
							strSkip = "<tr class=\"betoption\"><td class=\"betoption\" colspan=3><input id=\"bet" + vecQueryBets[0] + "_option0\" name=\"bet_select\" type=radio value=\"" + vecQueryBet[0] + "\">" + UTIL_RemoveHTML( vecQueryBet[1] ) + "</td></tr>\n";
					}
			
					vecQueryBet = pQueryBet->nextRow( );
				}
				if( strOption.empty( ) && !bClosed )
				{
					pResponse->strContent += strSkip;
					pResponse->strContent += "</table>\n";
					pResponse->strContent += "<p class=\"betnote\">" + UTIL_RemoveHTML2( vecQueryBets[6] ) + "</p>";
					if( vecQueryBets[4] != "0" && vecQueryBets[5] != "0" )
					{
						pResponse->strContent += "<p class=\"betbonus\">";
						pResponse->strContent += gmapLANG_CFG["bet_bonus"] + "<input name=\"bet_bonus\" type=text size=5 value=\"" + vecQueryBets[4] + "\"><br>";
						pResponse->strContent += UTIL_Xsprintf( gmapLANG_CFG["bet_bonus_tip"].c_str( ), vecQueryBets[4].c_str( ), vecQueryBets[5].c_str( ) ) + "</p>";
						pResponse->strContent += "<p class=\"betbutton\">" + Button_Submit( "submit_bet", string( gmapLANG_CFG["bet"] ) ) + "</p>";
					}
				}
				else
				{
					pResponse->strContent += "</table>\n";
					pResponse->strContent += "<p class=\"betnote\">" + UTIL_RemoveHTML2( vecQueryBets[6] ) + "</p>";
					pResponse->strContent += "<p class=\"betbonus\">" + UTIL_Xsprintf( gmapLANG_CFG["bet_bonus_tip"].c_str( ), vecQueryBets[4].c_str( ), vecQueryBets[5].c_str( ) ) + "</p>";
					pResponse->strContent += "<p class=\"bettotal\">" + UTIL_Xsprintf( gmapLANG_CFG["bet_total"].c_str( ), vecQueryBets[8].c_str( ), CAtomLong( ulBonusTotal ).toString( ).c_str( ) ) + "</p>";
				}
				pResponse->strContent += "</td>\n</tr>\n";
				if( strOption.empty( ) && !bClosed )
					pResponse->strContent += "</form>\n";
			}
		
			delete pQueryBet;
		
			vecQueryBets = pQueryBets->nextRow( );
		}
	
		delete pQueryBets;
	
		pResponse->strContent += "</table>\n</div>\n";

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_BET ) );
	}
	else
	{
		// Not authorised

		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["bet_page"], string( CSS_BET ), string( ), NOT_INDEX, CODE_401 );

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_BET ) );
	}
}

void CTracker :: serverResponseBetsGET( struct request_t *pRequest, struct response_t *pResponse )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
		if( IsIPBanned( pRequest, pResponse, btv, gmapLANG_CFG["bet_page"], string( CSS_BET ), NOT_INDEX ) )
			return;

//	if( !pRequest->user.strUID.empty( ) && ( m_ucInfoAccess == 0 || ( pRequest->user.ucAccess & m_ucInfoAccess ) ) )
	if( !pRequest->user.strUID.empty( ) && ( pRequest->user.ucAccess & m_ucAccessView ) )
	{
		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["bet_page"], string( CSS_BET ), string( ), NOT_INDEX, CODE_200 );
		
		const string cstrAction( pRequest->mapParams["action"] );
		const string cstrShow( pRequest->mapParams["show"] );
		const string cstrCount( pRequest->mapParams["count"] );
		string cstrBet( pRequest->mapParams["bet"] );
		
//		pResponse->strContent += "<script type=\"text/javascript\">\n";
//		pResponse->strContent += "<!--\n";
//		
//		pResponse->strContent += UTIL_JS_Edit_Tool_Bar( "post_content.post_content" );

		// hide
//		pResponse->strContent += "function hide(id) {\n";
//		pResponse->strContent += "  var element = document.getElementById( id );\n";
//		pResponse->strContent += "  element.style.display=\"none\";\n";
//		pResponse->strContent += "}\n";
		
		// display
//		pResponse->strContent += "function display(id) {\n";
//		pResponse->strContent += "  var element = document.getElementById( id );\n";
//		pResponse->strContent += "  element.style.display=\"\";\n";
//		pResponse->strContent += "}\n";
		
//		pResponse->strContent += "var hided = \"true\";\n";
//		
//		pResponse->strContent += "function change(id) {\n";
//		pResponse->strContent += "  var element = document.getElementById( id );\n";
//		pResponse->strContent += "  if (element.style.display == \"\") {\n";
//		pResponse->strContent += "    hide( id ); }\n";
//		pResponse->strContent += "  else {\n";
//		pResponse->strContent += "    display( id ); }\n";
//		pResponse->strContent += "}\n";
//		
//		pResponse->strContent += "//-->\n";
//		pResponse->strContent += "</script>\n\n";
		
		if( pRequest->user.ucAccess & m_ucAccessAdminBets )
		{
			if( !cstrAction.empty( ) )
			{
				if( cstrAction == "new" )
				{
					unsigned int uiOptionMax = (unsigned int)CFG_GetInt( "bnbt_bet_option_max", 16 );
				
					pResponse->strContent += "<form name=\"bet_option\" method=\"post\" action=\"" + RESPONSE_STR_BETS_HTML + "\" enctype=\"multipart/form-data\">\n";
					pResponse->strContent += "<input name=\"action\" type=hidden value=\"new\">\n";
					pResponse->strContent += "<table class=\"change_bet\">\n";
					pResponse->strContent += "<tr class=\"change_bet\">\n";
					pResponse->strContent += "<th class=\"change_bet\">" + gmapLANG_CFG["bet_title"] + "</th>";
					pResponse->strContent += "<td class=\"change_bet\" colspan=2><input name=\"title\" type=text size=96 maxlength=" + CAtomInt( MAX_FILENAME_LEN ).toString( ) + " value=\"\"></td>\n</tr>\n";
					pResponse->strContent += "<tr class=\"change_bet\">\n";
					pResponse->strContent += "<th class=\"change_bet\">" + gmapLANG_CFG["bet_option_skip"] + "</th>";
					pResponse->strContent += "<td class=\"change_bet\"><input name=\"option0\" type=text size=96 maxlength=" + CAtomInt( MAX_FILENAME_LEN ).toString( ) + " value=\"" + gmapLANG_CFG["bet_option_skip_desc"] + "\"></td>\n";
					pResponse->strContent += "<td class=\"change_bet\">0.0<input type=hidden name=\"rate0\" value=\"0.0\"></td></tr>\n";
				
					for( unsigned int uiOption = 1; uiOption <= uiOptionMax; uiOption++ )
					{
						pResponse->strContent += "<tr class=\"change_bet\">\n";
						pResponse->strContent += "<th class=\"change_bet\">" + CAtomInt( uiOption ).toString( ) + "</th>";
						pResponse->strContent += "<td class=\"change_bet\"><input name=\"option" + CAtomInt( uiOption ).toString( ) + "\" type=text size=96 maxlength=" + CAtomInt( MAX_FILENAME_LEN ).toString( ) + " value=\"\"></td>\n";
						pResponse->strContent += "<td class=\"change_bet\"><input name=\"rate" + CAtomInt( uiOption ).toString( ) + "\" type=text size=8 value=\"\"></td></tr>\n";
					}
					pResponse->strContent += "<tr class=\"change_bet\">\n";
					pResponse->strContent += "<th class=\"change_bet\">" + gmapLANG_CFG["bet_bonus"] + "</th>\n";
					pResponse->strContent += "<td class=\"change_bet\" colspan=2>" + gmapLANG_CFG["bet_bonus_min"] + "<input name=\"bonus_min\" value=\"0\">";
					pResponse->strContent += gmapLANG_CFG["bet_bonus_max"] + "<input name=\"bonus_max\" value=\"0\">";
					pResponse->strContent += "</td></tr>\n";
					pResponse->strContent += "<tr class=\"change_bet\">\n";
					pResponse->strContent += "<td class=\"change_bet\" colspan=3>\n";
					pResponse->strContent += Button_Submit( "submit_bet_new", string( gmapLANG_CFG["bet_new"] ) );
					pResponse->strContent += "</td></tr>\n";
					pResponse->strContent += "</table>\n";
					pResponse->strContent += "</form>\n";
				
					HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_BET ) );
				
					return;
				}
				if( cstrAction == "del" )
				{
					if( !cstrBet.empty( ) )
					{
						CMySQLQuery *pQueryBets = new CMySQLQuery( "SELECT bid,btitle,UNIX_TIMESTAMP(bdealed) FROM bets WHERE bid=" + cstrBet );
			
						vector<string> vecQueryBets;
				
						vecQueryBets.reserve(3);

						vecQueryBets = pQueryBets->nextRow( );
					
						delete pQueryBets;
					
						if( vecQueryBets.size( ) == 3 )
						{
							CMySQLQuery mq01( "DELETE FROM bets WHERE bid=" + vecQueryBets[0] );
							CMySQLQuery mq02( "DELETE FROM betsoption WHERE bid=" + vecQueryBets[0] );

							if( vecQueryBets[2] == "0" )
							{
								CMySQLQuery *pQueryTickets = new CMySQLQuery( "SELECT buid,bbetbonus FROM betsticket WHERE bid=" + vecQueryBets[0] );
				
								vector<string> vecQueryTickets;
						
								vecQueryTickets.reserve(2);

								vecQueryTickets = pQueryTickets->nextRow( );
							
								while( vecQueryTickets.size( ) == 2 )
								{
									CMySQLQuery mq03( "UPDATE users SET bbonus=bbonus+" + CAtomLong( atoi( vecQueryTickets[1].c_str( ) ) * 100 ).toString( ) + " WHERE buid=" + vecQueryTickets[0] );

									string strTitle = gmapLANG_CFG["bet_message_delete_title"];
									string strMessage = UTIL_Xsprintf( gmapLANG_CFG["bet_message_delete"].c_str( ), vecQueryBets[1].c_str( ), vecQueryTickets[1].c_str( ) );
									sendMessage( "", "0", vecQueryTickets[0], "127.0.0.1", strTitle, strMessage );

									vecQueryTickets = pQueryTickets->nextRow( );
								}

								delete pQueryTickets;

								m_pCache->ResetUsers( );
							}
							CMySQLQuery mq04( "DELETE FROM betsticket WHERE bid=" + vecQueryBets[0] );
						}
	//					pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["bet_deleted"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_info"] + "\" href=\"" + RESPONSE_STR_BETS_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";
					
						pResponse->strContent += "<script type=\"text/javascript\">window.location=\"" + RESPONSE_STR_BETS_HTML + "\"</script>\n\n";
					
						// Output common HTML tail
						HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_BET ) );
					
						return;
					}
				}
				if( cstrAction == "edit" )
				{
					if( !cstrBet.empty( ) )
					{
						unsigned int uiOptionMax = (unsigned int)CFG_GetInt( "bnbt_bet_option_max", 16 );
					
						unsigned int uiOption = 0;
					
						CMySQLQuery *pQueryBets = new CMySQLQuery( "SELECT bid,btitle,bbetbonus_min,bbetbonus_max,bbetnote,bautoclose FROM bets WHERE bid=" + cstrBet + " AND bresult=0" );
		
						vector<string> vecQueryBets;
				
						vecQueryBets.reserve(6);

						vecQueryBets = pQueryBets->nextRow( );
					
						delete pQueryBets;
					
						if( vecQueryBets.size( ) == 6 )
						{
							CMySQLQuery *pQueryBet = new CMySQLQuery( "SELECT boptionid,boption,bbetrate,bbetcount FROM betsoption WHERE bid=" + vecQueryBets[0] + " ORDER BY boptionid" );
		
							vector<string> vecQueryBet;
	
							vecQueryBet.reserve(4);

							vecQueryBet = pQueryBet->nextRow( );
						
							pResponse->strContent += "<form name=\"bet_option\" method=\"post\" action=\"" + RESPONSE_STR_BETS_HTML + "\" enctype=\"multipart/form-data\">\n";
							pResponse->strContent += "<input name=\"bet\" type=hidden value=\"" + cstrBet + "\">\n";
							pResponse->strContent += "<input name=\"action\" type=hidden value=\"edit\">\n";
							pResponse->strContent += "<table class=\"change_bet\">\n";
							pResponse->strContent += "<tr class=\"change_bet\">\n";
							pResponse->strContent += "<th class=\"change_bet\">" + gmapLANG_CFG["bet_title"] + "</th>";
							pResponse->strContent += "<td class=\"change_bet\" colspan=2><input name=\"title\" type=text size=96 maxlength=" + CAtomInt( MAX_FILENAME_LEN ).toString( ) + " value=\"" + UTIL_RemoveHTML( vecQueryBets[1] ) + "\"></td>\n</tr>\n";
			
							while( vecQueryBet.size( ) == 4 || uiOption < uiOptionMax )
							{
								pResponse->strContent += "<tr class=\"change_bet\">\n";
								pResponse->strContent += "<th class=\"change_bet\">";

								if( uiOption == 0 )
									pResponse->strContent += gmapLANG_CFG["bet_option_skip"];
								else
									pResponse->strContent += CAtomInt( uiOption ).toString( );

								pResponse->strContent += "</th>";
								pResponse->strContent += "<td class=\"change_bet\"><input name=\"option" + CAtomInt( uiOption ).toString( ) + "\" type=text size=96 maxlength=" + CAtomInt( MAX_FILENAME_LEN ).toString( ) + " value=\"";
								if( vecQueryBet.size( ) == 4 && uiOption == UTIL_StringTo64( vecQueryBet[0].c_str( ) ) )
								{
									pResponse->strContent += UTIL_RemoveHTML( vecQueryBet[1] );
									pResponse->strContent += "\"></td>\n";
									pResponse->strContent += "<td class=\"change_bet\"><input name=\"rate" + CAtomInt( uiOption ).toString( ) + "\" type=text size=8 value=\"" + vecQueryBet[2] + "\"";
									if( uiOption == 0 )
										pResponse->strContent += " disabled=true";
									pResponse->strContent += "></td></tr>\n";
								
									vecQueryBet = pQueryBet->nextRow( );
								}
								else
								{
									pResponse->strContent += "\"></td>\n";
									pResponse->strContent += "<td class=\"change_bet\"><input name=\"rate" + CAtomInt( uiOption ).toString( ) + "\" type=text size=8 value=\"\"></td></tr>\n";
								}
							
								uiOption++;
							}
			
							delete pQueryBet;
						
							pResponse->strContent += "<tr class=\"change_bet\">\n";
							pResponse->strContent += "<th class=\"change_bet\">" + gmapLANG_CFG["bet_bonus"] + "</th>\n";
							pResponse->strContent += "<td class=\"change_bet\" colspan=2>" + gmapLANG_CFG["bet_bonus_min"] + "<input name=\"bonus_min\" value=\"" + vecQueryBets[2] + "\">";
							pResponse->strContent += gmapLANG_CFG["bet_bonus_max"] + "<input name=\"bonus_max\" value=\"" + vecQueryBets[3] + "\">";
							pResponse->strContent += "</td></tr>\n";
							pResponse->strContent += "<tr class=\"change_bet\">\n";
							pResponse->strContent += "<th class=\"change_bet\">" + gmapLANG_CFG["bet_note"] + "</th>\n";
							pResponse->strContent += "<td class=\"change_bet\" colspan=2><input name=\"note\" type=text size=96 maxlength=" + CAtomInt( MAX_FILENAME_LEN ).toString( ) + " value=\"" + vecQueryBets[4] + "\">";
							pResponse->strContent += "</td></tr>\n";
							pResponse->strContent += "<tr class=\"change_bet\">\n";
							pResponse->strContent += "<th class=\"change_bet\">" + gmapLANG_CFG["bet_autoclose"] + "</th>\n";
							pResponse->strContent += "<td class=\"change_bet\" colspan=2><input name=\"autoclose\" value=\"" + vecQueryBets[5] + "\">";
							pResponse->strContent += "</td></tr>\n";
							pResponse->strContent += "<tr class=\"change_bet\">\n";
							pResponse->strContent += "<td class=\"change_bet\" colspan=3>\n";
							pResponse->strContent += Button_Submit( "submit_bet_edit", string( gmapLANG_CFG["bet_edit"] ) );
							pResponse->strContent += "</td></tr>\n";
							pResponse->strContent += "</table>\n";
							pResponse->strContent += "</form>\n";
						
							// Output common HTML tail
							HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_BET ) );
						
							return;
						}
						pResponse->strContent += "<script type=\"text/javascript\">window.location=\"" + RESPONSE_STR_BETS_HTML + "\"</script>\n\n";
					
						// Output common HTML tail
						HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_BET ) );
					
						return;
					}
				}
				if( cstrAction == "open" )
				{
					if( !cstrBet.empty( ) )
					{
						CMySQLQuery *pQueryBets = new CMySQLQuery( "SELECT bid,UNIX_TIMESTAMP(bopen),UNIX_TIMESTAMP(bclosed) FROM bets WHERE bid=" + cstrBet + " AND bresult=0" );
		
						vector<string> vecQueryBets;
				
						vecQueryBets.reserve(3);

						vecQueryBets = pQueryBets->nextRow( );
					
						delete pQueryBets;
					
						if( vecQueryBets.size( ) == 3 )
						{
							if( vecQueryBets[1] == "0" && vecQueryBets[2] == "0" )
								CMySQLQuery mq01( "UPDATE bets SET bopen=NOW() WHERE bid=" + cstrBet );
							else if( vecQueryBets[1] != "0" && vecQueryBets[2] != "0" )
								CMySQLQuery mq02( "UPDATE bets SET bclosed=0 WHERE bid=" + cstrBet );
						}
					
//						pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["bet_deleted"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_info"] + "\" href=\"" + RESPONSE_STR_BETS_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";
				
						pResponse->strContent += "<script type=\"text/javascript\">window.location=\"" + RESPONSE_STR_BETS_HTML + "\"</script>\n\n";
				
						// Output common HTML tail
						HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_BET ) );
					
						return;
					}
				}
				if( cstrAction == "close" )
				{
					if( !cstrBet.empty( ) )
					{
						CMySQLQuery *pQueryBets = new CMySQLQuery( "SELECT bid,UNIX_TIMESTAMP(bopen),UNIX_TIMESTAMP(bclosed),UNIX_TIMESTAMP(bdealed) FROM bets WHERE bid=" + cstrBet );
		
						vector<string> vecQueryBets;
				
						vecQueryBets.reserve(4);

						vecQueryBets = pQueryBets->nextRow( );
					
						delete pQueryBets;
					
						if( vecQueryBets.size( ) == 4 && vecQueryBets[3] == "0" )
						{
							if( vecQueryBets[1] != "0" && vecQueryBets[2] == "0" )
								CMySQLQuery mq01( "UPDATE bets SET bclosed=NOW() WHERE bid=" + cstrBet );
						}
					
//						pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["bet_deleted"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_info"] + "\" href=\"" + RESPONSE_STR_BETS_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";
				
						pResponse->strContent += "<script type=\"text/javascript\">window.location=\"" + RESPONSE_STR_BETS_HTML + "\"</script>\n\n";
				
						// Output common HTML tail
						HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_BET ) );
					
						return;
					}
				}
				if( cstrAction == "result" )
				{
					if( !cstrBet.empty( ) )
					{
						string cstrHalf( pRequest->mapParams["half"] );
						string cstrOptionID( pRequest->mapParams["option"] );

						CMySQLQuery *pQueryBets = new CMySQLQuery( "SELECT bid,UNIX_TIMESTAMP(bclosed),UNIX_TIMESTAMP(bdealed),bpayback FROM bets WHERE bid=" + cstrBet );
		
						vector<string> vecQueryBets;
				
						vecQueryBets.reserve(4);

						vecQueryBets = pQueryBets->nextRow( );
					
						delete pQueryBets;
					
						if( vecQueryBets.size( ) == 4 && !cstrOptionID.empty( ) )
						{
							if( vecQueryBets[1] != "0" && vecQueryBets[2] == "0" )
							{
								if( cstrOptionID == "payback" )
								{
									if( vecQueryBets[3] == "0" )
									{
										CMySQLQuery mq01( "UPDATE bets SET bresult=0,bpayback=1 WHERE bid=" + cstrBet );
										CMySQLQuery mq02( "UPDATE betsoption SET bresult=0,bresult_half=0 WHERE bid=" + cstrBet );
									}
								}
								else
								{
									CMySQLQuery *pQueryBet = new CMySQLQuery( "SELECT bresult FROM betsoption WHERE bid=" + cstrBet + " AND boptionid=" + cstrOptionID );

									vector<string> vecQueryBet;
							
									vecQueryBet.reserve(1);

									vecQueryBet = pQueryBet->nextRow( );
								
									delete pQueryBet;

									if( vecQueryBet.size( ) == 1 )
									{
										CMySQLQuery mq01( "UPDATE bets SET bresult=" + cstrOptionID + ",bpayback=0 WHERE bid=" + cstrBet );
										if( cstrHalf == "1" )
										{
											CMySQLQuery mq02( "UPDATE betsoption SET bresult=0,bresult_half=0 WHERE bid=" + cstrBet );
											CMySQLQuery mq03( "UPDATE betsoption SET bresult_half=1 WHERE bid=" + cstrBet + " AND boptionid=" + cstrOptionID );
										}
										else
										{
											CMySQLQuery mq02( "UPDATE betsoption SET bresult=0,bresult_half=0 WHERE bid=" + cstrBet );
											CMySQLQuery mq03( "UPDATE betsoption SET bresult=1 WHERE bid=" + cstrBet + " AND boptionid=" + cstrOptionID );
										}

									}
								}
							}
						}
						pResponse->strContent += "<script type=\"text/javascript\">window.location=\"" + RESPONSE_STR_BETS_HTML + "\"</script>\n\n";
				
						// Output common HTML tail
						HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_BET ) );
					
						return;
					}
				}
				if( cstrAction == "deal" )
				{
					if( !cstrBet.empty( ) )
					{
						string cstrOptionID = string( );
						string cstrOption = string( );
						string cstrRate = string( );
						bool bPayback = false;

						CMySQLQuery *pQueryBets = new CMySQLQuery( "SELECT bid,btitle,UNIX_TIMESTAMP(bclosed),UNIX_TIMESTAMP(bdealed),bresult,bpayback FROM bets WHERE bid=" + cstrBet );
		
						vector<string> vecQueryBets;
				
						vecQueryBets.reserve(6);

						vecQueryBets = pQueryBets->nextRow( );
					
						delete pQueryBets;
					
						if( vecQueryBets.size( ) == 6 && !vecQueryBets[4].empty( ) && !vecQueryBets[5].empty( ) )
						{
							cstrOptionID = vecQueryBets[4];
							if( vecQueryBets[5] == "1" )
								bPayback = true;

							if( vecQueryBets[2] != "0" && vecQueryBets[3] == "0" )
							{
								if( !bPayback )
								{
									bool bDynamic = true;
									bool bWinHalf = false;

									CMySQLQuery *pQueryBet = new CMySQLQuery( "SELECT boptionid,boption,bbetrate,bresult,bresult_half FROM betsoption WHERE bid=" + cstrBet );

									vector<string> vecQueryBet;
							
									vecQueryBet.reserve(5);

									vecQueryBet = pQueryBet->nextRow( );

									while( vecQueryBet.size( ) == 5 )
									{
										if( vecQueryBet[2] != "0.00" )
											bDynamic = false;

										if( vecQueryBet[0] == cstrOptionID && ( vecQueryBet[3] == "1" || vecQueryBet[4] == "1" ) ) 
										{
											cstrOption = vecQueryBet[1];
											cstrRate = vecQueryBet[2];

											if( vecQueryBet[4] == "1" )
											{
												bWinHalf = true;
												cstrOption += gmapLANG_CFG["bet_win_half"];
											}
										}

										vecQueryBet = pQueryBet->nextRow( );
									}
								
									delete pQueryBet;

									if( !cstrRate.empty( ) )
									{
										float flRate = 0.0;

										CMySQLQuery mq01( "UPDATE bets SET bdealed=NOW() WHERE bid=" + cstrBet );

										if( bDynamic )
										{
											unsigned long ulAll = 0, ulWin = 0;

											CMySQLQuery *pQueryBonus = new CMySQLQuery( "SELECT boptionid,SUM(bbetbonus) FROM betsticket WHERE bid=" + cstrBet + " GROUP BY boptionid" );
							
											vector<string> vecQueryBonus;
									
											vecQueryBonus.reserve(2);

											vecQueryBonus = pQueryBonus->nextRow( );

											while( vecQueryBonus.size( ) == 2 )
											{
												if( vecQueryBonus[0] == cstrOptionID )
													ulWin = atoi( vecQueryBonus[1].c_str( ) );
												ulAll += atoi( vecQueryBonus[1].c_str( ) );

												vecQueryBonus = pQueryBonus->nextRow( );
											}

											delete pQueryBonus;

											flRate = ( (float)ulAll ) / ulWin;
										}
										else if( bWinHalf )
											flRate = ( 1 + atof( cstrRate.c_str( ) ) ) / 2;
										else
											flRate = atof( cstrRate.c_str( ) );

										CMySQLQuery *pQueryTickets = new CMySQLQuery( "SELECT buid,boptionid,bbetbonus FROM betsticket WHERE bid=" + cstrBet + " AND boptionid>0" );
						
										vector<string> vecQueryTickets;
								
										vecQueryTickets.reserve(3);

										vecQueryTickets = pQueryTickets->nextRow( );
									
										while( vecQueryTickets.size( ) == 3 )
										{
											if( vecQueryTickets[1] == cstrOptionID )
											{
												unsigned long ulBonus = atoi( vecQueryTickets[2].c_str( ) ) * flRate * 100 + 0.5;
												CMySQLQuery mq03( "UPDATE users SET bbonus=bbonus+" + CAtomLong( ulBonus ).toString( ) + " WHERE buid=" + vecQueryTickets[0] );

												string strTitle = gmapLANG_CFG["bet_message_deal_title"];
												string strMessage = UTIL_Xsprintf( gmapLANG_CFG["bet_message_deal"].c_str( ), vecQueryBets[1].c_str( ), cstrOption.c_str( ), string( CAtomLong( ulBonus / 100 ).toString( ) + "." + CAtomInt( ( ulBonus % 100 ) / 10 ).toString( ) + CAtomInt( ulBonus % 10 ).toString( ) ).c_str( ) );
												sendMessage( "", "0", vecQueryTickets[0], "127.0.0.1", strTitle, strMessage );
											}
											else if( bWinHalf )
											{
												unsigned long ulBonus = atoi( vecQueryTickets[2].c_str( ) ) * 0.5 * 100 + 0.5;
												CMySQLQuery mq03( "UPDATE users SET bbonus=bbonus+" + CAtomLong( ulBonus ).toString( ) + " WHERE buid=" + vecQueryTickets[0] );

												string strTitle = gmapLANG_CFG["bet_message_deal_title"];
												string strMessage = UTIL_Xsprintf( gmapLANG_CFG["bet_message_deal_lose_half"].c_str( ), vecQueryBets[1].c_str( ), cstrOption.c_str( ), string( CAtomLong( ulBonus / 100 ).toString( ) + "." + CAtomInt( ( ulBonus % 100 ) / 10 ).toString( ) + CAtomInt( ulBonus % 10 ).toString( ) ).c_str( ) );
												sendMessage( "", "0", vecQueryTickets[0], "127.0.0.1", strTitle, strMessage );
											}

											vecQueryTickets = pQueryTickets->nextRow( );
										}

										delete pQueryTickets;

										m_pCache->ResetUsers( );
									}
								}
								else
								{
									CMySQLQuery mq01( "UPDATE bets SET bdealed=NOW() WHERE bid=" + cstrBet );

									CMySQLQuery *pQueryTickets = new CMySQLQuery( "SELECT buid,bbetbonus FROM betsticket WHERE bid=" + vecQueryBets[0] + " AND boptionid>0" );
					
									vector<string> vecQueryTickets;
							
									vecQueryTickets.reserve(2);

									vecQueryTickets = pQueryTickets->nextRow( );
								
									while( vecQueryTickets.size( ) == 2 )
									{
										CMySQLQuery mq03( "UPDATE users SET bbonus=bbonus+" + CAtomLong( atoi( vecQueryTickets[1].c_str( ) ) * 100 ).toString( ) + " WHERE buid=" + vecQueryTickets[0] );

										string strTitle = gmapLANG_CFG["bet_message_deal_title"];
										string strMessage = UTIL_Xsprintf( gmapLANG_CFG["bet_message_deal_payback"].c_str( ), vecQueryBets[1].c_str( ), vecQueryTickets[1].c_str( ) );
										sendMessage( "", "0", vecQueryTickets[0], "127.0.0.1", strTitle, strMessage );

										vecQueryTickets = pQueryTickets->nextRow( );
									}

									delete pQueryTickets;

									m_pCache->ResetUsers( );
								}
							}
						}
					
//						pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["bet_deleted"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_info"] + "\" href=\"" + RESPONSE_STR_BETS_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";
				
						pResponse->strContent += "<script type=\"text/javascript\">window.location=\"" + RESPONSE_STR_BETS_HTML + "\"</script>\n\n";
				
						// Output common HTML tail
						HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_BET ) );
					
						return;
					}
				}
				if( cstrAction == "undeal" )
				{
					if( !cstrBet.empty( ) )
					{
						string cstrOptionID = string( );
						string cstrOption = string( );
						string cstrRate = string( );
						bool bPayback = false;

						CMySQLQuery *pQueryBets = new CMySQLQuery( "SELECT bid,btitle,UNIX_TIMESTAMP(bclosed),UNIX_TIMESTAMP(bdealed),bresult,bpayback FROM bets WHERE bid=" + cstrBet );
		
						vector<string> vecQueryBets;
				
						vecQueryBets.reserve(6);

						vecQueryBets = pQueryBets->nextRow( );
					
						delete pQueryBets;
					
						if( vecQueryBets.size( ) == 6 && !vecQueryBets[4].empty( ) && !vecQueryBets[5].empty( ) )
						{
							cstrOptionID = vecQueryBets[4];
							if( vecQueryBets[5] == "1" )
								bPayback = true;

							if( vecQueryBets[2] != "0" && vecQueryBets[3] != "0" )
							{
								if( !bPayback )
								{
									bool bDynamic = true;
									bool bWinHalf = false;

									CMySQLQuery *pQueryBet = new CMySQLQuery( "SELECT boptionid,boption,bbetrate,bresult,bresult_half FROM betsoption WHERE bid=" + cstrBet );

									vector<string> vecQueryBet;
							
									vecQueryBet.reserve(5);

									vecQueryBet = pQueryBet->nextRow( );

									while( vecQueryBet.size( ) == 5 )
									{
										if( vecQueryBet[2] != "0.00" )
											bDynamic = false;

										if( vecQueryBet[0] == cstrOptionID && ( vecQueryBet[3] == "1" || vecQueryBet[4] == "1" ) ) 
										{
											cstrOption = vecQueryBet[1];
											cstrRate = vecQueryBet[2];

											if( vecQueryBet[4] == "1" )
											{
												bWinHalf = true;
												cstrOption += gmapLANG_CFG["bet_win_half"];
											}
										}

										vecQueryBet = pQueryBet->nextRow( );
									}
								
									delete pQueryBet;

									if( !cstrRate.empty( ) )
									{
										float flRate = 0.0;

										CMySQLQuery mq01( "UPDATE bets SET bdealed=0 WHERE bid=" + cstrBet );

										if( bDynamic )
										{
											unsigned long ulAll = 0, ulWin = 0;

											CMySQLQuery *pQueryBonus = new CMySQLQuery( "SELECT boptionid,SUM(bbetbonus) FROM betsticket WHERE bid=" + cstrBet + " GROUP BY boptionid" );
							
											vector<string> vecQueryBonus;
									
											vecQueryBonus.reserve(2);

											vecQueryBonus = pQueryBonus->nextRow( );

											while( vecQueryBonus.size( ) == 2 )
											{
												if( vecQueryBonus[0] == cstrOptionID )
													ulWin = atoi( vecQueryBonus[1].c_str( ) );
												ulAll += atoi( vecQueryBonus[1].c_str( ) );

												vecQueryBonus = pQueryBonus->nextRow( );
											}

											delete pQueryBonus;

											flRate = ( (float)ulAll ) / ulWin;
										}
										else if( bWinHalf )
											flRate = ( 1 + atof( cstrRate.c_str( ) ) ) / 2;
										else
											flRate = atof( cstrRate.c_str( ) );

										CMySQLQuery *pQueryTickets = new CMySQLQuery( "SELECT buid,boptionid,bbetbonus FROM betsticket WHERE bid=" + cstrBet + " AND boptionid>0" );
						
										vector<string> vecQueryTickets;
								
										vecQueryTickets.reserve(3);

										vecQueryTickets = pQueryTickets->nextRow( );
									
										while( vecQueryTickets.size( ) == 3 )
										{
											if( vecQueryTickets[1] == cstrOptionID )
											{
												unsigned long ulBonus = atoi( vecQueryTickets[2].c_str( ) ) * flRate * 100 + 0.5;
												CMySQLQuery mq02( "UPDATE users SET bbonus=0 WHERE buid=" + vecQueryTickets[0] + " AND bbonus<" + CAtomLong( ulBonus ).toString( ) );
												CMySQLQuery mq03( "UPDATE users SET bbonus=bbonus-" + CAtomLong( ulBonus ).toString( ) + " WHERE buid=" + vecQueryTickets[0] + " AND bbonus>=" + CAtomLong( ulBonus ).toString( ) );

												string strTitle = gmapLANG_CFG["bet_message_undeal_title"];
												string strMessage = UTIL_Xsprintf( gmapLANG_CFG["bet_message_undeal"].c_str( ), vecQueryBets[1].c_str( ), string( CAtomLong( ulBonus / 100 ).toString( ) + "." + CAtomInt( ( ulBonus % 100 ) / 10 ).toString( ) + CAtomInt( ulBonus % 10 ).toString( ) ).c_str( ) );
												sendMessage( "", "0", vecQueryTickets[0], "127.0.0.1", strTitle, strMessage );
											}
											else if( bWinHalf )
											{
												unsigned long ulBonus = atoi( vecQueryTickets[2].c_str( ) ) * 0.5 * 100 + 0.5;
												CMySQLQuery mq02( "UPDATE users SET bbonus=0 WHERE buid=" + vecQueryTickets[0] + " AND bbonus<" + CAtomLong( ulBonus ).toString( ) );
												CMySQLQuery mq03( "UPDATE users SET bbonus=bbonus-" + CAtomLong( ulBonus ).toString( ) + " WHERE buid=" + vecQueryTickets[0] + " AND bbonus>=" + CAtomLong( ulBonus ).toString( ) );

												string strTitle = gmapLANG_CFG["bet_message_undeal_title"];
												string strMessage = UTIL_Xsprintf( gmapLANG_CFG["bet_message_undeal"].c_str( ), vecQueryBets[1].c_str( ), string( CAtomLong( ulBonus / 100 ).toString( ) + "." + CAtomInt( ( ulBonus % 100 ) / 10 ).toString( ) + CAtomInt( ulBonus % 10 ).toString( ) ).c_str( ) );
												sendMessage( "", "0", vecQueryTickets[0], "127.0.0.1", strTitle, strMessage );
											}

											vecQueryTickets = pQueryTickets->nextRow( );
										}

										delete pQueryTickets;

										m_pCache->ResetUsers( );
									}
								}
								else
								{
									CMySQLQuery mq01( "UPDATE bets SET bdealed=0 WHERE bid=" + cstrBet );

									CMySQLQuery *pQueryTickets = new CMySQLQuery( "SELECT buid,bbetbonus FROM betsticket WHERE bid=" + vecQueryBets[0] + " AND boptionid>0" );
					
									vector<string> vecQueryTickets;
							
									vecQueryTickets.reserve(2);

									vecQueryTickets = pQueryTickets->nextRow( );
								
									while( vecQueryTickets.size( ) == 2 )
									{
										unsigned long ulBonus = atoi( vecQueryTickets[1].c_str( ) ) * 100;
										CMySQLQuery mq02( "UPDATE users SET bbonus=0 WHERE buid=" + vecQueryTickets[0] + " AND bbonus<" + CAtomLong( ulBonus ).toString( ) );
										CMySQLQuery mq03( "UPDATE users SET bbonus=bbonus-" + CAtomLong( ulBonus ).toString( ) + " WHERE buid=" + vecQueryTickets[0] + " AND bbonus>=" + CAtomLong( ulBonus ).toString( ) );

										string strTitle = gmapLANG_CFG["bet_message_undeal_title"];
										string strMessage = UTIL_Xsprintf( gmapLANG_CFG["bet_message_undeal"].c_str( ), vecQueryBets[1].c_str( ), vecQueryTickets[1].c_str( ) );
										sendMessage( "", "0", vecQueryTickets[0], "127.0.0.1", strTitle, strMessage );

										vecQueryTickets = pQueryTickets->nextRow( );
									}

									delete pQueryTickets;

									m_pCache->ResetUsers( );
								}
							}
						}
					
//						pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["bet_deleted"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_info"] + "\" href=\"" + RESPONSE_STR_BETS_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";
				
						pResponse->strContent += "<script type=\"text/javascript\">window.location=\"" + RESPONSE_STR_BETS_HTML + "\"</script>\n\n";
				
						// Output common HTML tail
						HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_BET ) );
					
						return;
					}
				}
				pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["stats_invalid_operation"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_bet"] + "\" href=\"" + RESPONSE_STR_BETS_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";
				// Output common HTML tail
				HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_BET ) );
			
				return;
			}
		}
			
		pResponse->strContent += "<table class=\"bet_table_admin\">\n";
		
		if(  pRequest->user.ucAccess & m_ucAccessAdminBets )
		{
			pResponse->strContent += "<tr>\n";
			pResponse->strContent += "<td class=\"admin\" colspan=11>";
			if( cstrShow.empty( ) || cstrShow == "dealed" )
				pResponse->strContent += "<span class=\"blue\">" + gmapLANG_CFG["bet_show_dealed"] + "</span>";
			else
				pResponse->strContent += "<a class=\"black\" href=\"" + RESPONSE_STR_BETS_HTML + "?show=dealed\">" + gmapLANG_CFG["bet_show_dealed"] + "</a>";
			pResponse->strContent += "<span class=\"pipe\"> | </span>";
			if( cstrShow == "undealed" )
				pResponse->strContent += "<span class=\"blue\">" + gmapLANG_CFG["bet_show_undealed"] + "</span>";
			else
				pResponse->strContent += "<a class=\"black\" href=\"" + RESPONSE_STR_BETS_HTML + "?show=undealed\">" + gmapLANG_CFG["bet_show_undealed"] + "</a>";
			pResponse->strContent += "<span class=\"pipe\"> | </span>";
			if( cstrShow == "all" )
				pResponse->strContent += "<span class=\"blue\">" + gmapLANG_CFG["bet_show_all"] + "</span>";
			else
				pResponse->strContent += "<a class=\"black\" href=\"" + RESPONSE_STR_BETS_HTML + "?show=all\">" + gmapLANG_CFG["bet_show_all"] + "</a>";
			pResponse->strContent += "</td>\n";
			pResponse->strContent += "</tr>\n";
		}

		pResponse->strContent += "<tr>\n";
		pResponse->strContent += "<td class=\"admin\" colspan=11>";
		if( cstrCount != "all" )
		{
			pResponse->strContent += "[<a class=\"black\" href=\"" + RESPONSE_STR_BETS_HTML + "?count=all";
			if( !cstrShow.empty( ) )
				pResponse->strContent += "&amp;show=" + cstrShow;
			pResponse->strContent += "\">" + gmapLANG_CFG["bet_count_all"] + "</a>]";
		}
		pResponse->strContent += "[<a class=\"black\" href=\"" + RESPONSE_STR_BET_HTML + "\">" + gmapLANG_CFG["bet_return"] + "</a>]";
		if(  pRequest->user.ucAccess & m_ucAccessAdminBets )
		{
			pResponse->strContent += "[<a class=\"black\" href=\"" + RESPONSE_STR_BETS_HTML + "?action=new\">" + gmapLANG_CFG["bet_new"] + "</a>]";
		}
		pResponse->strContent += "</td>\n";
		pResponse->strContent += "</tr>\n";
			
		CMySQLQuery *pQueryBets = 0;

		if( cstrCount == "all" )
		{
			if( pRequest->user.ucAccess & m_ucAccessAdminBets )
			{
				if( cstrShow.empty( ) || cstrShow == "dealed" )
					pQueryBets = new CMySQLQuery( "SELECT bid,btitle,bcreated,bopen,bclosed,bdealed,bbetbonus_min,bbetbonus_max,bbetnote,bautoclose,bbetcount,bresult,bpayback FROM bets WHERE bdealed>0 ORDER BY bcreated DESC" );
				else if( cstrShow == "undealed" )
					pQueryBets = new CMySQLQuery( "SELECT bid,btitle,bcreated,bopen,bclosed,bdealed,bbetbonus_min,bbetbonus_max,bbetnote,bautoclose,bbetcount,bresult,bpayback FROM bets WHERE bdealed=0 ORDER BY bcreated DESC" );
				else if( cstrShow == "all" )
					pQueryBets = new CMySQLQuery( "SELECT bid,btitle,bcreated,bopen,bclosed,bdealed,bbetbonus_min,bbetbonus_max,bbetnote,bautoclose,bbetcount,bresult,bpayback FROM bets ORDER BY bcreated DESC" );
			}
			else
				pQueryBets = new CMySQLQuery( "SELECT bid,btitle,bcreated,bopen,bclosed,bdealed,bbetbonus_min,bbetbonus_max,bbetnote,bautoclose,bbetcount,bresult,bpayback FROM bets WHERE bdealed>0 ORDER BY bcreated DESC" );
		}
		else
		{
			if( pRequest->user.ucAccess & m_ucAccessAdminBets )
			{
				if( cstrShow.empty( ) || cstrShow == "dealed" )
					pQueryBets = new CMySQLQuery( "SELECT bid,btitle,bcreated,bopen,bclosed,bdealed,bbetbonus_min,bbetbonus_max,bbetnote,bautoclose,bbetcount,bresult,bpayback FROM bets WHERE bdealed>0 ORDER BY bcreated DESC LIMIT 50" );
				else if( cstrShow == "undealed" )
					pQueryBets = new CMySQLQuery( "SELECT bid,btitle,bcreated,bopen,bclosed,bdealed,bbetbonus_min,bbetbonus_max,bbetnote,bautoclose,bbetcount,bresult,bpayback FROM bets WHERE bdealed=0 ORDER BY bcreated DESC LIMIT 50" );
				else if( cstrShow == "all" )
					pQueryBets = new CMySQLQuery( "SELECT bid,btitle,bcreated,bopen,bclosed,bdealed,bbetbonus_min,bbetbonus_max,bbetnote,bautoclose,bbetcount,bresult,bpayback FROM bets ORDER BY bcreated DESC LIMIT 50" );
			}
			else
				pQueryBets = new CMySQLQuery( "SELECT bid,btitle,bcreated,bopen,bclosed,bdealed,bbetbonus_min,bbetbonus_max,bbetnote,bautoclose,bbetcount,bresult,bpayback FROM bets WHERE bdealed>0 ORDER BY bcreated DESC LIMIT 50" );
		}
		
		vector<string> vecQueryBets;
	
		vecQueryBets.reserve(13);

		vecQueryBets = pQueryBets->nextRow( );
		
		while( vecQueryBets.size( ) == 13 )
		{
			string strOption = string( );
			string strBonus = string( );
		
			CMySQLQuery *pQueryTicket = new CMySQLQuery( "SELECT boptionid,bbetbonus FROM betsticket WHERE bid=" + vecQueryBets[0] + " AND buid=" + pRequest->user.strUID + " ORDER BY boptionid" );

			vector<string> vecQueryTicket;

			vecQueryTicket.reserve(2);

			vecQueryTicket = pQueryTicket->nextRow( );
	
			delete pQueryTicket;
	
			if( vecQueryTicket.size( ) == 2 )
			{
				strOption = vecQueryTicket[0];
				strBonus = vecQueryTicket[1];
			}

			pResponse->strContent += "<tr class=\"betheader\">\n";
			pResponse->strContent += "<th class=\"bettime\">" + gmapLANG_CFG["bet_time_created"] + "</th>\n";
			pResponse->strContent += "<td class=\"bettime\">" + vecQueryBets[2] + "</td>\n";
			pResponse->strContent += "<th class=\"bettime\">" + gmapLANG_CFG["bet_time_open"] + "</th>\n";
			pResponse->strContent += "<td class=\"bettime\">" + vecQueryBets[3] + "</td>\n";
			pResponse->strContent += "<th class=\"bettime\">" + gmapLANG_CFG["bet_time_closed"] + "</th>\n";
			pResponse->strContent += "<td class=\"bettime\">" + vecQueryBets[4] + "</td>\n";
			if( pRequest->user.ucAccess & m_ucAccessAdminBets )
			{
				pResponse->strContent += "<th class=\"bettime\">" + gmapLANG_CFG["bet_autoclose"] + "</th>\n";
				pResponse->strContent += "<td class=\"bettime\">" + vecQueryBets[9] + "</td>\n";
			}
			pResponse->strContent += "<th class=\"bettime\">" + gmapLANG_CFG["bet_status"] + "</th>\n";
			pResponse->strContent += "<td class=\"bettime\">";
			if( vecQueryBets[3] != m_strMySQLTimeZero && vecQueryBets[4] == m_strMySQLTimeZero )
				pResponse->strContent += "<span class=\"green\">" + gmapLANG_CFG["bet_status_open"] + "</span>";
			else
			{
				if( vecQueryBets[3] != m_strMySQLTimeZero )
				{
					pResponse->strContent += "<span class=\"red\">" + gmapLANG_CFG["bet_status_closed"] + "</span>";
					if( vecQueryBets[5] != m_strMySQLTimeZero )
						pResponse->strContent += "/<span class=\"blue\">" + gmapLANG_CFG["bet_status_dealed"] + "</span>";
					else
						pResponse->strContent += "/<span class=\"green\">" + gmapLANG_CFG["bet_status_not_dealed"] + "</span>";
				}
				else
					pResponse->strContent += "<span class=\"red\">" + gmapLANG_CFG["bet_status_not_open"] + "</span>";
			}
			pResponse->strContent += "</td>\n";
			
			if( pRequest->user.ucAccess & m_ucAccessAdminBets )
			{
				pResponse->strContent += "<td class=\"betadmin\">";
				if( vecQueryBets[11] == "0" && vecQueryBets[12] == "0" )
				{
					pResponse->strContent += "[<a href=\"" + RESPONSE_STR_BETS_HTML + "?bet=" + vecQueryBets[0] + "&amp;action=edit\">" + gmapLANG_CFG["bet_edit"] + "</a>]";
					if( vecQueryBets[3] != m_strMySQLTimeZero && vecQueryBets[4] == m_strMySQLTimeZero )
						pResponse->strContent += "[<a href=\"" + RESPONSE_STR_BETS_HTML + "?bet=" + vecQueryBets[0] + "&amp;action=close\">" + gmapLANG_CFG["bet_close"] + "</a>]";
					else
						pResponse->strContent += "[<a href=\"" + RESPONSE_STR_BETS_HTML + "?bet=" + vecQueryBets[0] + "&amp;action=open\">" + gmapLANG_CFG["bet_open"] + "</a>]";
				}
				if( vecQueryBets[4] != m_strMySQLTimeZero && vecQueryBets[5] == m_strMySQLTimeZero && ( vecQueryBets[11] != "0" || vecQueryBets[12] != "0" ) )
					pResponse->strContent += "[<a href=\"javascript: ;\" onClick=\"javascript: if( confirm('" + gmapLANG_CFG["bet_deal_q"] + "') ) window.location='" + RESPONSE_STR_BETS_HTML + "?bet=" + vecQueryBets[0] + "&amp;action=deal'\">" + gmapLANG_CFG["bet_deal"] + "</a>]";
				if( vecQueryBets[3] == m_strMySQLTimeZero )
					pResponse->strContent += "[<a class=\"red\" href=\"javascript: ;\" onClick=\"javascript: if( confirm('" + gmapLANG_CFG["bet_delete_q"] + "') ) window.location='" + RESPONSE_STR_BETS_HTML + "?bet=" + vecQueryBets[0] + "&amp;action=del'\">" + gmapLANG_CFG["bet_del"] + "</a>]</td>";
			}
			
			pResponse->strContent += "</tr>\n";
			
			pResponse->strContent += "<tr class=\"bet\">";
			
			pResponse->strContent += "<td class=\"bet\" colspan=11>";
			
			pResponse->strContent += "<p class=\"bettitle\">" + UTIL_RemoveHTML2( vecQueryBets[1] ) + "</p>";
			pResponse->strContent += "<p class=\"bettitle\">[<a target=\"_blank\" href=\"" + RESPONSE_STR_TALK_HTML + "?tag=" + UTIL_StringToEscaped( gmapLANG_CFG["bet_match"] + vecQueryBets[0] ) + "&amp;tochannel=" + UTIL_StringToEscaped( gmapLANG_CFG["talk_channel_bets"] ) + "\">" + gmapLANG_CFG["bet_talk"] + "</a>]</p>";

			CMySQLQuery *pQueryBet = new CMySQLQuery( "SELECT boptionid,boption,bbetrate,bbetcount,bresult,bresult_half FROM betsoption WHERE bid=" + vecQueryBets[0] + " ORDER BY boptionid" );
		
			vector<string> vecQueryBet;
	
			vecQueryBet.reserve(6);

			vecQueryBet = pQueryBet->nextRow( );
			
			if( vecQueryBet.size( ) == 6 )
			{
				string strSkip = string( );
				unsigned long ulBonusTotal = 0;
				
				pResponse->strContent += "<table class=\"betoption\">\n";

				pResponse->strContent += "<tr class=\"betoption\">";
				pResponse->strContent += "<th class=\"betheader\">" + gmapLANG_CFG["bet_option"] + "</th>";
				pResponse->strContent += "<th class=\"betheader\">" + gmapLANG_CFG["bet_rate"] + "</th>";
				pResponse->strContent += "<th class=\"betheader\">" + gmapLANG_CFG["bet_count"] + "</th>";
				pResponse->strContent += "<th class=\"betheader\">" + gmapLANG_CFG["bet_bonus_option"] + "</th>";
				if( vecQueryBets[4] != m_strMySQLTimeZero )
				{
					pResponse->strContent += "<th class=\"betheader\">" + gmapLANG_CFG["bet_result"] + "</th>";
					pResponse->strContent += "<th class=\"betheader\">" + gmapLANG_CFG["bet_win_half"] + "</th>";
				}
				pResponse->strContent += "</tr>";
				while( vecQueryBet.size( ) == 6 )
				{
					if( vecQueryBet[0] != "0" )
					{
						pResponse->strContent += "<tr class=\"betoption";
					
						if( !strOption.empty( ) && vecQueryBet[0] == strOption )
							pResponse->strContent += "ed";
						pResponse->strContent += "\">";
						pResponse->strContent += "<td class=\"betoption\">" + UTIL_RemoveHTML( vecQueryBet[1] ) + "</td>";
						pResponse->strContent += "<td class=\"betoption\">";
						if( vecQueryBet[2] == "0.00" )
							pResponse->strContent += gmapLANG_CFG["bet_rate_dynamic"];
						else
							pResponse->strContent += vecQueryBet[2];
						pResponse->strContent += "</td>";
						pResponse->strContent += "<td class=\"betcount\">" + vecQueryBet[3] + "</td>";

						CMySQLQuery *pQueryBonus = new CMySQLQuery( "SELECT boptionid,SUM(bbetbonus) FROM betsticket WHERE bid=" + vecQueryBets[0] + " AND boptionid=" + vecQueryBet[0] + " GROUP BY boptionid" );

						vector<string> vecQueryBonus;

						vecQueryBonus.reserve(2);

						vecQueryBonus = pQueryBonus->nextRow( );
				
						delete pQueryBonus;

						pResponse->strContent += "<td class=\"betbonus\">";
						if( !strOption.empty( ) && vecQueryBet[0] == strOption )
							pResponse->strContent += strBonus + "/";
						if( vecQueryBonus.size( ) == 2 )
						{
							pResponse->strContent += vecQueryBonus[1];
							ulBonusTotal += UTIL_StringTo64( vecQueryBonus[1].c_str( ) );
						}
						else
							pResponse->strContent += "0";

						pResponse->strContent += "</td>\n";
						if( vecQueryBets[4] != m_strMySQLTimeZero )
						{
							pResponse->strContent += "<td class=\"betoption\">";
							if( vecQueryBet[4] == "1" )
								pResponse->strContent += "<span class=\"betresult\">" + gmapLANG_CFG["bet_result_yes"] + "</span>";
							else if( ( pRequest->user.ucAccess & m_ucAccessAdminBets ) && vecQueryBets[5] == m_strMySQLTimeZero )
								pResponse->strContent += "<a class=\"betresult\" href=\"" + RESPONSE_STR_BETS_HTML + "?bet=" + vecQueryBets[0] + "&amp;action=result&amp;option=" + vecQueryBet[0] + "\">" + gmapLANG_CFG["bet_result_set"] + "</a>";
							pResponse->strContent += "</td>\n";
							pResponse->strContent += "<td class=\"betoption\">";
							if( vecQueryBet[2] != "0.00" )
							{
								if( vecQueryBet[5] == "1" )
									pResponse->strContent += "<span class=\"betresult\">" + gmapLANG_CFG["bet_result_yes"] + "</span>";
								else if( ( pRequest->user.ucAccess & m_ucAccessAdminBets ) && vecQueryBets[5] == m_strMySQLTimeZero )
									pResponse->strContent += "<a class=\"betresult\" href=\"" + RESPONSE_STR_BETS_HTML + "?bet=" + vecQueryBets[0] + "&amp;action=result&amp;half=1&amp;option=" + vecQueryBet[0] + "\">" + gmapLANG_CFG["bet_result_set"] + "</a>";
							}
							pResponse->strContent += "</td>\n";
						}
					
						pResponse->strContent += "</tr>\n";
					}
					else
					{
						strSkip = "<p class=\"betskip";
						if( !strOption.empty( ) && vecQueryBet[0] == strOption )
							strSkip += "ed";
						strSkip += "\">" + UTIL_RemoveHTML( vecQueryBet[1] ) + ": " + vecQueryBet[3] + "</p>\n";
					}
					
					vecQueryBet = pQueryBet->nextRow( );
				}

				if( vecQueryBets[4] != m_strMySQLTimeZero )
				{
					pResponse->strContent += "<tr class=\"betpayback\">";
					pResponse->strContent += "<td class=\"betoption\">" + UTIL_RemoveHTML( gmapLANG_CFG["bet_payback"] ) + "</td>";
					pResponse->strContent += "<td class=\"betoption\" colspan=3>" + UTIL_RemoveHTML( gmapLANG_CFG["bet_payback_note"] ) + "</td>";
					pResponse->strContent += "<td class=\"betoption\" colspan=2>";
					if( vecQueryBets[12] == "1" )
						pResponse->strContent += "<span class=\"betresult\">" + gmapLANG_CFG["bet_result_yes"] + "</span>";
					else if( ( pRequest->user.ucAccess & m_ucAccessAdminBets ) && vecQueryBets[5] == m_strMySQLTimeZero )
						pResponse->strContent += "<a class=\"betresult\" href=\"" + RESPONSE_STR_BETS_HTML + "?bet=" + vecQueryBets[0] + "&amp;action=result&amp;option=payback\">" + gmapLANG_CFG["bet_result_set"] + "</a>";
					pResponse->strContent += "</td>\n";
					pResponse->strContent += "</tr>\n";
				}
				pResponse->strContent += "</table>";

				pResponse->strContent += "<p class=\"betnote\">" + UTIL_RemoveHTML2( vecQueryBets[8] ) + "</p>";
				pResponse->strContent += "<p class=\"betbonus\">" + UTIL_Xsprintf( gmapLANG_CFG["bet_bonus_tip"].c_str( ), vecQueryBets[6].c_str( ), vecQueryBets[7].c_str( ) ) + "</p>";
				pResponse->strContent += "<p class=\"bettotal\">" + UTIL_Xsprintf( gmapLANG_CFG["bet_total"].c_str( ), vecQueryBets[10].c_str( ), CAtomLong( ulBonusTotal ).toString( ).c_str( ) ) + "</p>";
				
				pResponse->strContent += strSkip;
				
			}
			
			delete pQueryBet;

			pResponse->strContent += "</td>\n</tr>";
			
			vecQueryBets = pQueryBets->nextRow( );
		}
		
		delete pQueryBets;
		
		pResponse->strContent += "</table>\n";
	
		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_BET ) );
	}
	else
	{
		// Not authorised

		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["bet_page"], string( CSS_BET ), string( ), NOT_INDEX, CODE_401 );

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_BET ) );
	}
}

void CTracker :: serverResponseBetsPOST( struct request_t *pRequest, struct response_t *pResponse, CAtomList *pPost )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
		if( IsIPBanned( pRequest, pResponse, btv, gmapLANG_CFG["bet_page"], string( CSS_BET ), NOT_INDEX ) )
			return;
	
	if( !pRequest->user.strUID.empty( ) && ( m_ucInfoAccess == 0 || ( pRequest->user.ucAccess & m_ucInfoAccess ) ) )
	{
		string strBet = string( );
		string strAction = string( );
		string strTitle = string( );
		string strOption = string( );
		string strBonusMin = string( );
		string strBonusMax = string( );
		string strNote = string( );
		string strAutoClose = string( );
		string strBonus = string( );
		string strSelected = string( );
		string strBetFor = string( );
		vector< pair<string, string> > vecOption;
		vector< pair<string, string> > vecOptionRate;
		
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
							
							if( strName == "bet" )
								strBet = pData->toString( );
							else if( strName.substr( 0, 6 ) == "option" )
							{
								strOption = pData->toString( );
								vecOption.push_back( pair<string, string>( strName.substr( 6 ), strOption ) );
							}
							else if( strName.substr( 0, 4 ) == "rate" )
							{
								strOption = pData->toString( );
								vecOptionRate.push_back( pair<string, string>( strName.substr( 4 ), strOption ) );
							}
							else if( strName == "action" )
								strAction = pData->toString( );
							else if( strName == "title" )
								strTitle = pData->toString( );
							else if( strName == "bonus_min" )
								strBonusMin = pData->toString( );
							else if( strName == "bonus_max" )
								strBonusMax = pData->toString( );
							else if( strName == "note" )
								strNote = pData->toString( );
							else if( strName == "autoclose" )
								strAutoClose = pData->toString( );
							else if( strName == "bet_bonus" )
								strBonus = pData->toString( );
							else if( strName == "bet_select" )
								strSelected = pData->toString( );
							else if( strName == "bet_for" )
								strBetFor = pData->toString( );
						}
						else
						{
							// Output common HTML head
							HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["bet_page"], string( CSS_BET ), string( ), NOT_INDEX, CODE_400 );

							// failed
							pResponse->strContent += "<p class=\"failed\">" + gmapLANG_CFG["failed"] + "</p>\n";
							// Signal a bad request
							pResponse->strContent += "<p class=\"body_upload\">400 " + gmapLANG_CFG["server_response_400"] + "</p>\n";

							// Output common HTML tail
							HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_BET ) );

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
			HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["bet_page"], string( CSS_BET ), string( ), NOT_INDEX, CODE_400 );

			// failed
			pResponse->strContent += "<p class=\"failed\">" + gmapLANG_CFG["failed"] + "</p>\n";
			// Signal a bad request
			pResponse->strContent += "<p class=\"body_upload\">400 " + gmapLANG_CFG["server_response_400"] + "</p>\n";

			// Output common HTML tail
			HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_BET ) );

			if( gbDebug )
				UTIL_LogPrint( "Upload Warning - Bad request (no post received)\n" );

			return;
		}
		
		time_t now_t = time( 0 );
//		CMySQLQuery mq00( "UPDATE bets SET bclosed=NOW(),bautoclose=0 WHERE bopen!=0 AND bclosed=0 AND bdealed=0 AND bautoclose>0 AND bautoclose<NOW()" );
		
		if( strTitle.empty( ) )
			strTitle = gmapLANG_CFG["bet_title_new"];

		if( !strAction.empty( ) && ( pRequest->user.ucAccess & m_ucAccessAdminBets ) )
		{
			HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["bet_page"], string( CSS_BET ), string( BETS_HTML ), NOT_INDEX, CODE_200 );
			
			if( !strTitle.empty( ) && !vecOption.empty( ) )
			{
				int64 uiBonusMin = 0, uiBonusMax = 0;
				bool bNum = true;
				for( int i = 0; i < strBonusMin.length( ) && bNum; i++ )
					if( !isdigit( strBonusMin[i] ) )
						bNum  = false;
				for( int i = 0; i < strBonusMax.length( ) && bNum; i++ )
					if( !isdigit( strBonusMax[i] ) )
						bNum  = false;

				uiBonusMin = UTIL_StringTo64( strBonusMin.c_str( ) );
				uiBonusMax = UTIL_StringTo64( strBonusMax.c_str( ) );

				if( bNum && uiBonusMin > uiBonusMax )
					bNum = false;

				if( strAction == "new" )
				{
					CMySQLQuery *pQueryBet = 0;
					if( bNum )
						pQueryBet = new CMySQLQuery( "INSERT INTO bets (btitle,bcreated,bbetbonus_min,bbetbonus_max) VALUES('" + UTIL_StringToMySQL( strTitle ) + "',NOW()," + CAtomInt( uiBonusMin ).toString( ) + "," + CAtomInt( uiBonusMax ).toString( ) + ")" );
					else
						pQueryBet = new CMySQLQuery( "INSERT INTO bets (btitle,bcreated,bbetbonus_min,bbetbonus_max) VALUES('" + UTIL_StringToMySQL( strTitle ) + "',NOW(),0,0)" );
				
					unsigned long ulLast = pQueryBet->lastInsertID( );
				
					delete pQueryBet;
				
					if( ulLast > 0 )
					{
						for( vector< pair< string, string > > :: iterator ulOption = vecOption.begin( ); ulOption != vecOption.end( ); ulOption++ )
						{
							if( !(*ulOption).second.empty( ) )
							{
								for( vector< pair< string, string > > :: iterator ulOptionRate = vecOptionRate.begin( ); ulOptionRate != vecOptionRate.end( ); ulOptionRate++ )
								{
									if( (*ulOptionRate).first == (*ulOption).first && !(*ulOptionRate).second.empty( ) )
									{
										CMySQLQuery mq02( "INSERT INTO betsoption (bid,boptionid,boption,bbetrate) VALUES(" + CAtomInt( ulLast ).toString( ) + "," + (*ulOption).first + ",\'" + UTIL_StringToMySQL( (*ulOption).second ) + "\'," + (*ulOptionRate).second + ")" );
										break;
									}
								}
							}
						}
					}
				}
				else if( strAction == "edit" && !strBet.empty( ) )
				{
					if( bNum )
						CMySQLQuery mq01( "UPDATE bets SET btitle=\'" + UTIL_StringToMySQL( strTitle ) + "\',bbetbonus_min=" + CAtomInt( uiBonusMin ).toString( ) + ",bbetbonus_max=" + CAtomInt( uiBonusMax ).toString( ) + ",bbetnote=\'" + UTIL_StringToMySQL( strNote ) + "\',bautoclose=\'" + strAutoClose + "\' WHERE bid=" + strBet );
					else
						CMySQLQuery mq02( "UPDATE bets SET btitle=\'" + UTIL_StringToMySQL( strTitle ) + "\',bbetnote=\'" + UTIL_StringToMySQL( strNote ) + "\',bautoclose=\'" + strAutoClose + "\' WHERE bid=" + strBet );
					
					for( vector< pair< string, string > > :: iterator ulOption = vecOption.begin( ); ulOption != vecOption.end( ); ulOption++ )
					{
						if( !(*ulOption).second.empty( ) )
						{
							for( vector< pair< string, string > > :: iterator ulOptionRate = vecOptionRate.begin( ); ulOptionRate != vecOptionRate.end( ); ulOptionRate++ )
							{
								if( (*ulOptionRate).first == (*ulOption).first )
								{
									CMySQLQuery mq03( "INSERT INTO betsoption (bid,boptionid,boption,bbetrate) VALUES(" + strBet + "," + (*ulOption).first + ",'" + UTIL_StringToMySQL( (*ulOption).second ) + "'," + (*ulOptionRate).second + ") ON DUPLICATE KEY UPDATE boption='" + UTIL_StringToMySQL( (*ulOption).second ) + "',bbetrate=" + (*ulOptionRate).second );
									break;
								}
							}
						}
						else
							CMySQLQuery mq04( "DELETE FROM betsoption WHERE bid=" + strBet + " AND boptionid=" + (*ulOption).first );
					}
				}
				

//				pResponse->strContent += "<p class=\"changed_post\">" + UTIL_Xsprintf( gmapLANG_CFG["post_return"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_info"] + "\" href=\"" + RESPONSE_STR_ANNOUNCEMENTS_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";
			}

			HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_BET ) );

			return;
		}
		else if( !strSelected.empty( ) )
		{
			HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["bet_page"], string( CSS_BET ), string( BET_HTML ), NOT_INDEX, CODE_200 );

			CMySQLQuery *pQueryBets = new CMySQLQuery( "SELECT bid,bbetbonus_min,bbetbonus_max,bautoclose FROM bets WHERE bid=" + strBetFor + " AND bopen!=0 AND bclosed=0" );
		
			vector<string> vecQueryBets;

			vecQueryBets.reserve(4);

			vecQueryBets = pQueryBets->nextRow( );
			
			delete pQueryBets;

			bool bClosed = false;

			if( vecQueryBets.size( ) == 4 && vecQueryBets[3] != m_strMySQLTimeZero )
			{
				struct tm time_tm;
				int64 year, month, day, hour, minute, second;
				sscanf( vecQueryBets[3].c_str( ), "%d-%d-%d %d:%d:%d",&year,&month,&day,&hour,&minute,&second );
				time_tm.tm_year = year-1900;
				time_tm.tm_mon = month-1;
				time_tm.tm_mday = day;
				time_tm.tm_hour = hour;
				time_tm.tm_min = minute;
				time_tm.tm_sec = second;

				if( now_t > mktime(&time_tm) )
				{
					CMySQLQuery mq00( "UPDATE bets SET bclosed=NOW(),bautoclose=0 WHERE bid=" + vecQueryBets[0] );
					bClosed = true;
				}
			}
			
			if( vecQueryBets.size( ) == 4 && !bClosed )
			{
				CMySQLQuery *pQueryTicket = new CMySQLQuery( "SELECT boptionid FROM betsticket WHERE bid=" + strBetFor + " AND buid=" + pRequest->user.strUID + " ORDER BY boptionid" );
			
				vector<string> vecQueryTicket;

				vecQueryTicket.reserve(1);

				vecQueryTicket = pQueryTicket->nextRow( );
				
				delete pQueryTicket;
				
				if( vecQueryTicket.size( ) == 0 )
				{
					bool bNum = true;
					for( int i = 0; i < strBonus.length( ) && bNum; i++ )
						if( !isdigit( strBonus[i] ) )
							bNum  = false;
					if( atoi( strBonus.c_str( ) ) < atoi( vecQueryBets[1].c_str( ) ) || atoi( strBonus.c_str( ) ) > atoi( vecQueryBets[2].c_str( ) ) )
						bNum = false;

					CMySQLQuery *pQueryUser = new CMySQLQuery( "SELECT bbonus FROM users WHERE buid=" + pRequest->user.strUID );
			
					vector<string> vecQueryUser;
		
					vecQueryUser.reserve(1);

					vecQueryUser = pQueryUser->nextRow( );
					
					delete pQueryUser;

					int64 ulBonus = 0;

					if( vecQueryUser.size( ) == 1 && !vecQueryUser[0].empty( ) )
						ulBonus = UTIL_StringTo64( vecQueryUser[0].c_str( ) );

					CMySQLQuery *pQueryBet = new CMySQLQuery( "SELECT boptionid FROM betsoption WHERE bid=" + strBetFor + " AND boptionid=" + strSelected );
			
					vector<string> vecQueryBet;
		
					vecQueryBet.reserve(1);

					vecQueryBet = pQueryBet->nextRow( );
					
					delete pQueryBet;

					bool bBetted = false;
					
					if( vecQueryBet.size( ) == 1 )
					{
						if( !strBonus.empty( ) && bNum && strSelected != "0" )
						{
							if( ulBonus / 100 >= UTIL_StringTo64( strBonus.c_str( ) ) )
							{
								CMySQLQuery mq01( "INSERT INTO betsticket (bid,buid,boptionid,bbetbonus,bbettime) VALUES(" + strBetFor + "," + pRequest->user.strUID + "," + strSelected + "," + strBonus + ",NOW())" );
								CMySQLQuery mq02( "UPDATE users SET bbonus=bbonus-" + CAtomLong( UTIL_StringTo64( strBonus.c_str( ) ) * 100 ).toString( ) + " WHERE buid=" + pRequest->user.strUID );
								bBetted = true;
							}
						}
						else if( strSelected == "0" )
						{
							CMySQLQuery mq03( "INSERT INTO betsticket (bid,buid,boptionid,bbettime) VALUES(" + strBetFor + "," + pRequest->user.strUID + "," + strSelected + ",NOW())" );
							bBetted = true;
						}
						if( bBetted )
						{
							if( strSelected != "0" )
								CMySQLQuery mq04( "UPDATE bets SET bbetcount=bbetcount+1 WHERE bid=" + strBetFor );
							CMySQLQuery mq05( "UPDATE betsoption SET bbetcount=bbetcount+1 WHERE bid=" + strBetFor + " AND boptionid=" + strSelected );
						}
					}
				}
			}
			
			HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_BET ) );
				
			return;
		}
	}
	else
	{
		// Not authorised

		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["bet_page"], string( CSS_BET ), string( ), NOT_INDEX, CODE_401 );

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_BET ) );
	}
}
