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
#include "bencode.h"
#include "config.h"
#include "html.h"
#include "tracker.h"
#include "util.h"

void CTracker :: serverResponseMessagesGET( struct request_t *pRequest, struct response_t *pResponse )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
		if( IsIPBanned( pRequest, pResponse, btv, gmapLANG_CFG["messages_page"], string( CSS_MESSAGES ), NOT_INDEX ) )
			return;

	if( pRequest->user.strUID.empty( ) )
	{
		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["messages_page"], string( CSS_MESSAGES ), string( ), NOT_INDEX, CODE_403 );

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_MESSAGES ) );

		return;
	}

	if( pRequest->user.ucAccess & m_ucAccessMessages )
	{
		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["messages_page"], string( CSS_MESSAGES ), string( ), NOT_INDEX, CODE_200 );

		// assorted scripts (thanks SA)
		pResponse->strContent += "<script type=\"text/javascript\">\n";
		pResponse->strContent += "<!--\n";
		
		pResponse->strContent += "var checkflag = \"false\";\n";
		pResponse->strContent += "function check(field,checkall_name,uncheckall_name) {\n";
		pResponse->strContent += "if (checkflag == \"false\") {\n";
		pResponse->strContent += "  for (i = 0; i < field.length; i++) {\n";
		pResponse->strContent += "    field[i].checked = true;}\n";
		pResponse->strContent += "  checkflag = \"true\";\n";
		pResponse->strContent += "  return uncheckall_name; }\n";
		pResponse->strContent += "else {\n";
		pResponse->strContent += "  for (i = 0; i < field.length; i++) {\n";
		pResponse->strContent += "    field[i].checked = false; }\n";
		pResponse->strContent += "  checkflag = \"false\";\n";
		pResponse->strContent += "  return checkall_name; }\n";
		pResponse->strContent += "}\n\n";

		pResponse->strContent += "function delete_message_confirm( theform ) {\n";
		pResponse->strContent += "  return confirm(\"" + gmapLANG_CFG["messages_delete_q"] + "\");\n";
		pResponse->strContent += "}\n\n";

		// validate
		pResponse->strContent += "function validate( theform ) {\n";
		pResponse->strContent += "if( theform.message.value == \"\" ) {\n";
		pResponse->strContent += "  alert( \"" + gmapLANG_CFG["js_fill_fields"] + "\" );\n";
		pResponse->strContent += "  return false; }\n";
		pResponse->strContent += "if( theform.message.value.getBytes() > " + CAtomInt( m_uiMessageLength ).toString( ) + " ) {\n";
		pResponse->strContent += "  alert( \"" + gmapLANG_CFG["js_too_long"] + "\\n" + m_strJSMsgLength + "\\n" + m_strJSMsgReduce + "\" );\n";
		pResponse->strContent += "  return false; }\n";
		pResponse->strContent += "else { return true; }\n";
		pResponse->strContent += "}\n\n";
		
		pResponse->strContent += "String.prototype.getBytes = function() {\n";
		pResponse->strContent += "  return this.length;\n";
//		pResponse->strContent += "  var cArr = this.match(/[^\\x00-\\xff]/ig);\n";
//		pResponse->strContent += "  return this.length + (cArr == null ? 0 : cArr.length);\n";
		pResponse->strContent += "}\n\n";

		// checklength
		pResponse->strContent += "function checklength( theform ) {\n";
		pResponse->strContent += "  alert( \"" + UTIL_Xsprintf( gmapLANG_CFG["js_message_length"].c_str( ), "\" + theform.message.value.getBytes() + \"" ) + "\" );\n";
		pResponse->strContent += "}\n";
		
		pResponse->strContent += "if (window.XMLHttpRequest)\n";
		pResponse->strContent += "{// code for IE7+, Firefox, Chrome, Opera, Safari\n";
		pResponse->strContent += "  xmlhttp=new XMLHttpRequest(); }\n";
		pResponse->strContent += "else\n";
		pResponse->strContent += "{// code for IE6, IE5\n";
		pResponse->strContent += "  xmlhttp=new ActiveXObject(\"Microsoft.XMLHTTP\"); }\n";
		
		// load
		pResponse->strContent += "function load(tag,id,url) {\n";
		pResponse->strContent += "  var loadElement = document.getElementById( id );\n";
		pResponse->strContent += "  xmlhttp.onreadystatechange=function() {\n";
		pResponse->strContent += "    if (xmlhttp.readyState==4 && xmlhttp.status==200) {\n";
		pResponse->strContent += "      var e = document.createElement('div');\n";
		pResponse->strContent += "      e.innerHTML = xmlhttp.responseText;\n";
		pResponse->strContent += "      var elements = e.getElementsByTagName(tag);\n";
		pResponse->strContent += "      for (var i = 0; i < elements.length; i++) {\n";
		pResponse->strContent += "        if (elements[i].id == id) {\n";
		pResponse->strContent += "          loadElement.innerHTML = elements[i].innerHTML;\n";
		pResponse->strContent += "          break; }\n";
		pResponse->strContent += "      }\n";
		pResponse->strContent += "    }\n";
		pResponse->strContent += "  }\n";
		pResponse->strContent += "  xmlhttp.open(\"GET\",url,true);\n";
		pResponse->strContent += "  xmlhttp.send();\n";
		pResponse->strContent += "}\n";
		
		pResponse->strContent += "//-->\n";
		pResponse->strContent += "</script>\n\n";

		if( !pRequest->user.strUID.empty( ) )
		{
			const string cstrPage( pRequest->mapParams["page"] );
			const string cstrPerPage( pRequest->mapParams["per_page"] );
			const string cstrMode( pRequest->mapParams["mode"] );
			bool bModeSent = false;
			if( !cstrMode.empty( ) && cstrMode == "sent" )
				bModeSent = true;
			
			//
			// delete message
			//

			string cstrDel( pRequest->mapParams["del"] );
			
			if( cstrDel.find( " " ) != string :: npos )
				cstrDel.erase( );

			if( !cstrDel.empty( ) )
			{
				vector< pair< string, string > > vecParams;
				vecParams.reserve(64);
				string strJoined = string( );
				
				vecParams.push_back( pair<string, string>( string( "per_page" ), cstrPerPage ) );
				vecParams.push_back( pair<string, string>( string( "mode" ), cstrMode ) );
				
				strJoined = UTIL_RemoveHTML( UTIL_HTMLJoin( vecParams, string( "?" ), string( "&" ), string( "=" ) ) );
				
				CMySQLQuery *pQueryMessage = 0;
				
				if( bModeSent )
					pQueryMessage = new CMySQLQuery( "SELECT bfromid FROM messages_sent WHERE bid=" + cstrDel );
				else
					pQueryMessage = new CMySQLQuery( "SELECT bsendtoid FROM messages WHERE bid=" + cstrDel );
			
				vector<string> vecQueryMessage;
			
				vecQueryMessage.reserve(1);

				vecQueryMessage = pQueryMessage->nextRow( );
				
				delete pQueryMessage;
				
				if( vecQueryMessage.size( ) == 1 )
				{
					if( vecQueryMessage[0] == pRequest->user.strUID )
					{
						if( bModeSent )
							CMySQLQuery mq01( "DELETE FROM messages_sent WHERE bid=" + cstrDel );
						else
							CMySQLQuery mq01( "DELETE FROM messages WHERE bid=" + cstrDel );

						// Deleted message
						pResponse->strContent += "<p class=\"deleted\">" + UTIL_Xsprintf( gmapLANG_CFG["messages_deleted"].c_str( ), cstrDel.c_str( ), string( "<a title=\"" + gmapLANG_CFG["messages"] + "\" href=\"" + RESPONSE_STR_MESSAGES_HTML + strJoined + "\">" ).c_str( ), "</a>" ) + "</p>\n";

						// Output common HTML tail
						HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_MESSAGES ) );

						return;
					}
				}
				// Unable to delete message
				pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["messages_unable_delete"].c_str( ), cstrDel.c_str( ), string( "<a title=\"" + gmapLANG_CFG["messages"] + "\" href=\"" + RESPONSE_STR_MESSAGES_HTML + strJoined + "\">" ).c_str( ), "</a>" ) + "</p>\n";

				// Output common HTML tail
				HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_MESSAGES ) );

				return;
			}
			
			string cstrRead( pRequest->mapParams["read"] );
			string cstrSendToID( pRequest->mapParams["sendto"] );
			string cstrReply( pRequest->mapParams["reply"] );
			string cstrSendTo = string( );
			
			if( cstrRead.find( " " ) != string :: npos )
				cstrRead.erase( );
			if( cstrSendToID.find( " " ) != string :: npos )
				cstrSendToID.erase( );
			if( cstrReply.find( " " ) != string :: npos )
				cstrReply.erase( );
			
			if( !cstrSendToID.empty( ) )
			{
				CMySQLQuery *pQuery = new CMySQLQuery( "SELECT busername FROM users WHERE buid=" + cstrSendToID );
				
				vector<string> vecQuery;
			
				vecQuery.reserve(1);

				vecQuery = pQuery->nextRow( );
				
				delete pQuery;
			
				if( vecQuery.size( ) == 1 )
					cstrSendTo = vecQuery[0];
			}
			
			if( cstrSendToID.empty( ) && cstrReply.empty( ) )
			{
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

				bool bModeSent = false;
				string strQuery = "SELECT bid,";

				if( !cstrMode.empty( ) && cstrMode == "sent" )
					bModeSent = true;
				
				if( bModeSent )
					strQuery += "bsendto,bsendtoid,bsent,btitle,bmessage,bread FROM messages_sent WHERE bfromid=" + pRequest->user.strUID;
				else
				{
					strQuery += "bfrom,bfromid,bsent,btitle,bmessage,bread FROM messages WHERE bsendtoid=" + pRequest->user.strUID;
					if( !cstrMode.empty( ) && cstrMode == "unread" )
						strQuery += " AND bread=0";
				}
				strQuery += " ORDER BY bsent DESC";
				
				CMySQLQuery *pQueryMessage = new CMySQLQuery( strQuery );
				
				vector<string> vecQueryMessage;
			
				vecQueryMessage.reserve(7);

				vecQueryMessage = pQueryMessage->nextRow( );

				unsigned long ulCount = pQueryMessage->numRows( );

				pResponse->strContent += "<div id=\"divMsg\" class=\"messages_table_posted\">";
				pResponse->strContent += "<h3>";
				if( !cstrMode.empty( ) )
				{
					if( cstrMode == "unread" || cstrMode == "sent" )
						pResponse->strContent += gmapLANG_CFG["messages_mode_"+cstrMode];
				}
				else
					pResponse->strContent += gmapLANG_CFG["messages_mode_all"];
				pResponse->strContent += gmapLANG_CFG["messages"] + "</h3>\n";
				pResponse->strContent += "<p class=\"subfilter\"><a href=\"" + RESPONSE_STR_MESSAGES_HTML + "\">" + gmapLANG_CFG["messages_mode_all"] + "</a>";
				pResponse->strContent += "<span class=\"pipe\"> | </span>";
				pResponse->strContent += "<a href=\"" + RESPONSE_STR_MESSAGES_HTML + "?mode=unread\">" + gmapLANG_CFG["messages_mode_unread"] + "</a>";
				pResponse->strContent += "<span class=\"pipe\"> | </span>";
				pResponse->strContent += "<a href=\"" + RESPONSE_STR_MESSAGES_HTML + "?mode=sent\">" + gmapLANG_CFG["messages_mode_sent"] + "</a>";
				pResponse->strContent += "</p>";
				
				vector< pair< string, string > > vecParams;
				vecParams.reserve(64);
				string strJoined = string( );
				
				vecParams.push_back( pair<string, string>( string( "per_page" ), cstrPerPage ) );
				vecParams.push_back( pair<string, string>( string( "mode" ), cstrMode ) );
				
				strJoined = UTIL_RemoveHTML( UTIL_HTMLJoin( vecParams, string( "&" ), string( "&" ), string( "=" ) ) );
			
				if( !cstrPage.empty( ) )
					ulStart = (unsigned long)atoi( cstrPage.c_str( ) ) * uiOverridePerPage;
				
				// page numbers
				
				pResponse->strContent += UTIL_PageBarAJAX( ulCount, cstrPage, uiOverridePerPage, RESPONSE_STR_MESSAGES_HTML, strJoined, "div", "divMsg", true );
				
				unsigned long ulAdded = 0;
				unsigned long ulSkipped = 0;

				if( vecQueryMessage.size( ) == 7 )
				{
					pResponse->strContent += "<form name=\"messagesFrm\" method=\"post\" action=\"" + RESPONSE_STR_MESSAGES_HTML + "\" onSubmit=\"return delete_message_confirm(this)\" enctype=\"multipart/form-data\">\n";
					pResponse->strContent += "<table class=\"messages_table_posted\" summary=\"messages\">\n";
					pResponse->strContent += "<tr class=\"messages_table_header\">\n";
					pResponse->strContent += "<th class=\"messages_table\">" + gmapLANG_CFG["messages_title"] + "</th>";
					pResponse->strContent += "<th class=\"messages_table\">";
					if( bModeSent )
						pResponse->strContent += gmapLANG_CFG["messages_sendto"];
					else
						pResponse->strContent += gmapLANG_CFG["messages_from"];
					pResponse->strContent += "</th>";
					pResponse->strContent += "<th class=\"messages_table\">" + gmapLANG_CFG["messages_time"] + "</th>";
					pResponse->strContent += "<th class=\"messages_table\">" + gmapLANG_CFG["messages_operation"] + "</th>\n";
					pResponse->strContent += "<th class=\"messages_table\">" + gmapLANG_CFG["messages_select"] + "</th></tr>\n";
					
					while( vecQueryMessage.size( ) == 7 )
					{
						if( uiOverridePerPage == 0 || ulAdded < uiOverridePerPage )
						{
							if( ulSkipped == ulStart )
							{
								vector< pair< string, string > > vecParams;
								vecParams.reserve(64);
								string strJoined = string( );
					
								vecParams.push_back( pair<string, string>( string( "page" ), cstrPage ) );
								vecParams.push_back( pair<string, string>( string( "per_page" ), cstrPerPage ) );
								vecParams.push_back( pair<string, string>( string( "mode" ), cstrMode ) );
					
								strJoined = UTIL_RemoveHTML( UTIL_HTMLJoin( vecParams, string( "&" ), string( "&" ), string( "=" ) ) );
						
								string strID = string( );
								string strName = string( );

								string strNameID = string( );
								string strTime = string( );
								string strMsgTitle = string( );
								string strMsgText = string( );
								string strRead = string( "0" );

								strID = vecQueryMessage[0];
								if( !vecQueryMessage[1].empty( ) )
									strName = vecQueryMessage[1];
								if( !vecQueryMessage[2].empty( ) )
									strNameID = vecQueryMessage[2];
								strTime = vecQueryMessage[3];
								strMsgTitle = vecQueryMessage[4];
								strMsgText = vecQueryMessage[5];
								strRead = vecQueryMessage[6];

								//
								// header
								//

								// Comment by
								if( strName.empty( ) )
									strName = gmapLANG_CFG["unknown"];
						
								if( strTime.empty( ) )
									strTime = gmapLANG_CFG["unknown"];
						
								if( strMsgTitle.empty( ) )
									strMsgTitle = gmapLANG_CFG["messages_no_title"];

								pResponse->strContent += "<tr class=\"messages";
								if( cstrRead == strID )
								{
									pResponse->strContent += "_read";
									if( !bModeSent && strRead == "0" )
										CMySQLQuery mq01( "UPDATE messages SET bread=1 WHERE bid=" + cstrRead );
								}
								pResponse->strContent += "\" id=\"" + strID + "\"><td class=\"messages_title\"><a class=\"messages\" href=\"" + RESPONSE_STR_MESSAGES_HTML + "?read=" + strID;
								pResponse->strContent += strJoined;
								pResponse->strContent += "#" + strID + "\">";
								pResponse->strContent += UTIL_RemoveHTML( strMsgTitle ) + "</a>";
								if( strRead == "0" && cstrRead != strID )
									pResponse->strContent += "<span class=\"hot\"> " + gmapLANG_CFG["messages_unread"] + "</span>";

								pResponse->strContent += "</td>\n";
						
								if( !strNameID.empty( ) && strNameID != "0" )
								{
									pResponse->strContent += "<td class=\"messages\">";
									pResponse->strContent += getUserLink( strNameID, strName );
									pResponse->strContent += "</td>\n";
								}
								else if( strNameID == "0" )
									pResponse->strContent += "<td class=\"messages\">" + UTIL_RemoveHTML( gmapLANG_CFG["system"] ) + "</td>\n";
								else
									pResponse->strContent += "<td class=\"messages\">" + UTIL_RemoveHTML( strName ) + "</td>\n";
							
								pResponse->strContent += "<td class=\"messages\">" + strTime + "</td>\n";
								pResponse->strContent += "<td class=\"messages\">";
								if( strNameID != "0" && !bModeSent )
									pResponse->strContent += "[<a class=\"black\" title=\"" + gmapLANG_CFG["messages_reply"] + "\" href=\"" + RESPONSE_STR_MESSAGES_HTML + "?reply=" + strID + strJoined + "\">" + gmapLANG_CFG["messages_reply"] + "</a>] ";
								pResponse->strContent += "[<a class=\"red\" title=\"" + gmapLANG_CFG["delete"] + "\" href=\"" + RESPONSE_STR_MESSAGES_HTML + "?del=" + strID + strJoined + "\">" + gmapLANG_CFG["delete"] + "</a>]</td>\n";
								pResponse->strContent += "<td class=\"messages\"><input name=\"msg" + strID + "\" type=checkbox value=\"" + strID + "\"></td>\n";
								pResponse->strContent += "</tr>\n";
						
								if( cstrRead == strID )
									pResponse->strContent += "<tr class=\"messages_body\"><td class=\"messages_body\" colspan=5>" + UTIL_RemoveHTML2( strMsgText ) + "</td></tr>\n";
								ulAdded++;
							}
							else
								ulSkipped++;
						}
						else
							break;
						vecQueryMessage = pQueryMessage->nextRow( );
					}
					pResponse->strContent += "<tr class=\"messages_delete\"><td class=\"messages_delete\" colspan=5>";
					if( !cstrPerPage.empty( ) )
						pResponse->strContent += "<input type=\"hidden\" name=\"per_page\" value=\"" + cstrPerPage + "\">";
					if( !cstrMode.empty( ) )
						pResponse->strContent += "<input type=\"hidden\" name=\"mode\" value=\"" + cstrMode + "\">";
					pResponse->strContent += "<input type=\"button\" value=\"" + gmapLANG_CFG["messages_select_all"] + "\" onClick=\"this.value=check(form,'" + gmapLANG_CFG["messages_select_all"] + "','" + gmapLANG_CFG["messages_select_none"] + "')\">";
					if( !bModeSent )
						pResponse->strContent += "<input name=\"submit_messages_mark_read_button\" id=\"submit_messages_mark_read\" alt=\"[" + gmapLANG_CFG["messages_mark_read"] + "]\" type=\"button\" value=\"" + gmapLANG_CFG["messages_mark_read"] + "\" onClick=\"this.form.submit()\">";
					pResponse->strContent += Button_Submit( "submit_messages_delete", string( gmapLANG_CFG["delete"] ) );
					pResponse->strContent += "</td></tr>";
					pResponse->strContent += "</table></form>\n";
				}
				else
					pResponse->strContent += "<p class=\"messages_table_posted\">" + gmapLANG_CFG["messages_no_message"] + "</p>\n";
					
				delete pQueryMessage;
				
				// page numbers
				
				pResponse->strContent += UTIL_PageBarAJAX( ulCount, cstrPage, uiOverridePerPage, RESPONSE_STR_MESSAGES_HTML, strJoined, "div", "divMsg", false );
				
				pResponse->strContent += "</div>\n";
			}
			else if( !cstrSendToID.empty( ) || !cstrReply.empty( ) )
			{
				if( !cstrSendToID.empty( ) )
				{
					// Does the user already exist?
					CMySQLQuery *pQuery = new CMySQLQuery( "SELECT buid FROM users WHERE buid=" + cstrSendToID );
			
					vector<string> vecQuery;
				
					vecQuery.reserve(1);

					vecQuery = pQuery->nextRow( );
					
					delete pQuery;
					
					if( vecQuery.size( ) == 0 )
					{
						pResponse->strContent += "<p class=\"not_exist\">" + UTIL_Xsprintf( gmapLANG_CFG["user_not_exist"].c_str( ), cstrSendToID.c_str( ) );
						HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_MESSAGES ) );
						return;
					}
				}
				
				string strID = string( );
				string strName = string( );
				string strNameID = string( );
				string strTime = string( );
				string strMsgTitle = string( );
				string strMsgText = string( );

				if( !cstrReply.empty( ) )
				{
					CMySQLQuery *pQueryMessage = new CMySQLQuery( "SELECT bsendtoid,bfrom,bfromid,btitle,bmessage FROM messages WHERE bid=" + cstrReply );
				
					vector<string> vecQueryMessage;
				
					vecQueryMessage.reserve(5);

					vecQueryMessage = pQueryMessage->nextRow( );
					
					delete pQueryMessage;
					
					strName = string( );

					if( vecQueryMessage.size( ) == 5 )
					{
						if( vecQueryMessage[0] == pRequest->user.strUID )
						{
							strName = vecQueryMessage[1];
							strNameID = vecQueryMessage[2];
							strMsgTitle = vecQueryMessage[3];
							strMsgText = vecQueryMessage[4];
						}
							
					}
				}

				if( !strName.empty( ) )
				{
					CMySQLQuery *pQueryUser = new CMySQLQuery( "SELECT buid FROM users WHERE buid=" + strNameID );
			
					vector<string> vecQueryUser;
				
					vecQueryUser.reserve(1);

					vecQueryUser = pQueryUser->nextRow( );
					
					delete pQueryUser;
					
					if( vecQueryUser.size( ) == 0 )
					{
						pResponse->strContent += "<p class=\"not_exist\">" + UTIL_Xsprintf( gmapLANG_CFG["user_not_exist"].c_str( ), strName.c_str( ) );
						HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_MESSAGES ) );
						return;
					}
				}

				pResponse->strContent += "<div class=\"messages_table_post\">\n";
				if( !cstrReply.empty( ) )
					pResponse->strContent += "<p class=\"messages_table_post\">" + gmapLANG_CFG["messages_reply"] + " " + UTIL_RemoveHTML( strName ) + "</p>\n";
				else
					pResponse->strContent += "<p class=\"messages_table_post\">" + gmapLANG_CFG["messages_post_message_to"] + " " + UTIL_RemoveHTML( cstrSendTo ) + "</p>\n";
				pResponse->strContent += "<table class=\"messages_table_post\">\n";
				pResponse->strContent += "<form method=\"post\" action=\"" + RESPONSE_STR_MESSAGES_HTML + "\" name=\"postamessage\" onSubmit=\"return validate( this )\" enctype=\"multipart/form-data\">\n";
				pResponse->strContent += "<tr class=\"messages_table_post\">\n";
				pResponse->strContent += "<th class=\"messages_table_post\">" + gmapLANG_CFG["messages_post_title"] + "</th>\n";
				pResponse->strContent += "<td class=\"messages_table_post\">\n";
				pResponse->strContent += "<input id=\"title\" name=\"title\" alt=\"[" + gmapLANG_CFG["messages_post_title"] + "]\" type=text size=64 maxlength=" + CAtomInt( MAX_FILENAME_LEN ).toString( );
				
				// Messages must be less than
				if( !cstrReply.empty( ) && !strName.empty( ) )
				{
					string :: size_type iStart = 0;
					iStart = strMsgTitle.rfind( gmapLANG_CFG["messages_reply"] + ": " );
					if( iStart != string :: npos )
					{
						iStart = strMsgTitle.find( ": ", iStart );
						if( iStart != string :: npos )
							strMsgTitle = strMsgTitle.substr( iStart + 2 );
					}
					pResponse->strContent += " value=\"" + gmapLANG_CFG["messages_reply"] + ": ";
					if( strMsgTitle.empty( ) )
						pResponse->strContent += gmapLANG_CFG["messages_no_title"] + "\">";
					else
						pResponse->strContent += strMsgTitle + "\">";
					pResponse->strContent += "</td>\n</tr>\n";
					pResponse->strContent += "<input name=\"sendto\" type=hidden value=\"" + strNameID + "\">\n";
					if( !cstrPage.empty( ) )
						pResponse->strContent += "<input type=\"hidden\" name=\"page\" value=\"" + cstrPage + "\">";
					if( !cstrPerPage.empty( ) )
						pResponse->strContent += "<input type=\"hidden\" name=\"per_page\" value=\"" + cstrPerPage + "\">";
					if( !cstrMode.empty( ) )
						pResponse->strContent += "<input type=\"hidden\" name=\"mode\" value=\"" + cstrMode + "\">";
					pResponse->strContent += "<tr class=\"messages_table_post\">\n";
					pResponse->strContent += "<th class=\"messages_table_post\">" + gmapLANG_CFG["messages_post_message"] + "</th>\n";
					pResponse->strContent += "<td class=\"messages_table_post\">\n<textarea id=\"messagearea\" name=\"message\" rows=8 cols=64>";
					pResponse->strContent += "[quote=[user]" + UTIL_RemoveHTML( strName ) + "[/user]]" + UTIL_RemoveHTML3( strMsgText ) + "[/quote]\n</textarea>\n</td>\n</tr>\n";
				}
				else
				{
					pResponse->strContent += " value=\"" + gmapLANG_CFG["messages_no_title"] + "\">\n";
					pResponse->strContent += "</td>\n</tr>\n";
					pResponse->strContent += "<input name=\"sendto\" type=hidden value=\"" + cstrSendToID + "\">\n";
					pResponse->strContent += "<tr class=\"messages_table_post\">\n";
					pResponse->strContent += "<th class=\"messages_table_post\">" + gmapLANG_CFG["messages_post_message"] + "</th>\n";
					pResponse->strContent += "<td class=\"messages_table_post\">\n<textarea id=\"messagearea\" name=\"message\" rows=8 cols=64>";
					pResponse->strContent += "</textarea>\n</td>\n</tr>\n";
				}
				pResponse->strContent += "<tr class=\"messages_table_post\">\n";
				pResponse->strContent += "<th class=\"messages_table_post\">" + gmapLANG_CFG["messages_save"] + "</th>\n";
				pResponse->strContent += "<td class=\"messages_table_post\">\n";
				pResponse->strContent += "<input id=\"id_save\" name=\"save\" type=checkbox";
				
				CMySQLQuery *pQueryPrefs = new CMySQLQuery( "SELECT bsavesent FROM users_prefs WHERE buid=" + pRequest->user.strUID );
				
				map<string, string> mapPrefs;

				mapPrefs = pQueryPrefs->nextRowMap( );
				
				delete pQueryPrefs;
				
				if( mapPrefs.size( ) == 1 && mapPrefs["bsavesent"] == "1" )
					pResponse->strContent += " checked=\"checked\"";
				pResponse->strContent += "> <label for=\"id_save\">" + gmapLANG_CFG["messages_save_sent"] + "</label>";
				pResponse->strContent += "</td>\n</tr>\n";
				pResponse->strContent += "<tr class=\"messages_table_post\">\n<td class=\"messages_table_post\" colspan=\"2\">";
				pResponse->strContent += "<ul>\n";
				pResponse->strContent += "<li>" + UTIL_Xsprintf( gmapLANG_CFG["messages_length_info"].c_str( ), CAtomInt( m_uiMessageLength ).toString( ).c_str( ) );
				pResponse->strContent += " [<a title=\"" + gmapLANG_CFG["message_check_length"] + "\" href=\"javascript: checklength( document.postamessage );\">" + gmapLANG_CFG["message_check_length"] + "</a>]\n";
				pResponse->strContent += "</li>\n";
				pResponse->strContent += "<li>" + gmapLANG_CFG["no_html"] + "</li>\n";
				pResponse->strContent += "</ul>\n</td>\n</tr>\n";
				pResponse->strContent += "<tr class=\"messages_table_post\">\n<td class=\"messages_table_post\" colspan=\"2\">";
				pResponse->strContent += "<div class=\"messages_table_post_button\">\n";
				pResponse->strContent += Button_Submit( "submit_message", string( gmapLANG_CFG["Submit"] ) );
				pResponse->strContent += "</div>\n";
				pResponse->strContent += "</td>\n";
				pResponse->strContent += "</tr>\n";
				pResponse->strContent += "</form>\n";
				pResponse->strContent += "</table>\n";
				pResponse->strContent += "</div>\n";
			}
			else
				pResponse->strContent += "<p class=\"denied\">" + gmapLANG_CFG["messages_post_disallowed"] + "</p>\n";
		}

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_MESSAGES ) );
	}
	else
	{
		// Not authorised

		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["messages_page"], string( CSS_MESSAGES ), string( ), NOT_INDEX, CODE_401 );

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_MESSAGES ) );
		
	}
}

void CTracker :: serverResponseMessagesPOST( struct request_t *pRequest, struct response_t *pResponse, CAtomList *pPost )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
		if( IsIPBanned( pRequest, pResponse, btv, gmapLANG_CFG["messages_page"], string( CSS_MESSAGES ), NOT_INDEX ) )
			return;
	
	if( !pRequest->user.strUID.empty( ) && ( pRequest->user.ucAccess & m_ucAccessMessages ) )
	{
	
		string strTitle = string( );
		string strMessage = string( );
		string strSendTo = string( );
		bool bSaveSent = false;
		string strPage = string( );
		string strPerPage = string( );
		string strMode = string( );
// 		string cstrHashString = string( );
		vector<string> vecMessages;
		vecMessages.reserve( 64 );

		unsigned long ulKeySize = 0;
		bool bDelete = false;
//		bool bRead = false;

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
							if( strName == "title" )
								strTitle = pData->toString( );
							else if( strName == "message" )
								strMessage = pData->toString( );
							else if( strName == "sendto" )
								strSendTo = pData->toString( );
							else if( strName == "save" && pData->toString( ) == "on" )
								bSaveSent = true;
							else if( strName == "page" )
								strPage = pData->toString( );
							else if( strName == "per_page" )
								strPerPage = pData->toString( );
							else if( strName == "mode" )
								strMode = pData->toString( );
							else if( strName.substr( 0, 3 ) == "msg" )
								vecMessages.push_back( pData->toString( ) );
							else if( strName == "submit_messages_delete_button" )
								bDelete = true;
//							else if( strName == "submit_messages_mark_read_button" )
//								bRead = true;

//							CMySQLQuery *pQueryMessage = new CMySQLQuery( "SELECT bid FROM messages WHERE bsendtoid=" + pRequest->user.strUID );
//			
//							vector<string> vecQueryMessage;
//						
//							vecQueryMessage.reserve(1);

//							vecQueryMessage = pQueryMessage->nextRow( );
//				
//							while( vecQueryMessage.size( ) == 1 )
//							{
//								if( strName == "del" + vecQueryMessage[0] )
//									if( pData->toString( ) == "on" )
//									{
//										if( bDelete == false )
//											bDelete = true;
//										CMySQLQuery mq01( "DELETE FROM messages WHERE bid=" + vecQueryMessage[0] );
//									}
//								vecQueryMessage = pQueryMessage->nextRow( );
//							}
//							delete pQueryMessage;
						}
						else
						{
							// Output common HTML head
							HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["messages_page"], string( CSS_MESSAGES ), string( ), NOT_INDEX, CODE_400 );

							// failed
							pResponse->strContent += "<p class=\"failed\">" + gmapLANG_CFG["failed"] + "</p>\n";
							// Signal a bad request
							pResponse->strContent += "<p class=\"body_upload\">400 " + gmapLANG_CFG["server_response_400"] + "</p>\n";

							// Output common HTML tail
							HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_MESSAGES ) );

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
			HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["messages_page"], string( CSS_MESSAGES ), string( ), NOT_INDEX, CODE_400 );

			// failed
			pResponse->strContent += "<p class=\"failed\">" + gmapLANG_CFG["failed"] + "</p>\n";
			// Signal a bad request
			pResponse->strContent += "<p class=\"body_upload\">400 " + gmapLANG_CFG["server_response_400"] + "</p>\n";

			// Output common HTML tail
			HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_MESSAGES ) );

			if( gbDebug )
				UTIL_LogPrint( "Upload Warning - Bad request (no post received)\n" );

			return;
		}
		
		string strReturnPage = MESSAGES_HTML;
		
		vector< pair< string, string > > vecParams;
		vecParams.reserve(64);
		
		vecParams.push_back( pair<string, string>( string( "page" ), strPage ) );
		vecParams.push_back( pair<string, string>( string( "per_page" ), strPerPage ) );
		vecParams.push_back( pair<string, string>( string( "mode" ), strMode ) );
		
		strReturnPage += UTIL_HTMLJoin( vecParams, string( "?" ), string( "&" ), string( "=" ) );
		
		if( bDelete )
		{
			for( vector<string> :: iterator ulKey = vecMessages.begin( ); ulKey != vecMessages.end( ); ulKey++ )
			{
				if( !(*ulKey).empty( ) )
				{
					if( !strMode.empty( ) && strMode == "sent" )
						CMySQLQuery mq01( "DELETE FROM messages_sent WHERE bid=" + *ulKey + " AND bfromid=" + pRequest->user.strUID );
					else
						CMySQLQuery mq01( "DELETE FROM messages WHERE bid=" + *ulKey + " AND bsendtoid=" + pRequest->user.strUID );
				}
			}
			// Output common HTML head
			HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["messages_page"], string( CSS_MESSAGES ), strReturnPage, NOT_INDEX, CODE_400 );

			// Output common HTML tail
			HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_MESSAGES ) );

			return;
		}
		
		if( vecMessages.size( ) > 0 && !bDelete )
		{
			for( vector<string> :: iterator ulKey = vecMessages.begin( ); ulKey != vecMessages.end( ); ulKey++ )
			{
				if( !(*ulKey).empty( ) )
					CMySQLQuery mq01( "UPDATE messages SET bread=1 WHERE bid=" + *ulKey + " AND bsendtoid=" + pRequest->user.strUID );
			}
			// Output common HTML head
			HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["messages_page"], string( CSS_MESSAGES ), strReturnPage, NOT_INDEX, CODE_400 );

			// Output common HTML tail
			HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_MESSAGES ) );

			return;
		}
		
		if( !pRequest->user.strUID.empty( ) && !strSendTo.empty( ) )
		{
			if( strMessage.empty( ) )
			{
				HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["messages_page"], string( CSS_MESSAGES ), string( ), NOT_INDEX, CODE_200 );
				//You must fill in all the fields.
				pResponse->strContent += "<p class=\"fill_all\">" + UTIL_Xsprintf( gmapLANG_CFG["messages_fill_warning"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["messages"] + "\" href=\"" + RESPONSE_STR_MESSAGES_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";

				// Output common HTML tail
				HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_MESSAGES ) );

				return;
			}
			
			sendMessage( pRequest->user.strLogin, pRequest->user.strUID, strSendTo, pRequest->strIP, strTitle, strMessage, bSaveSent );
			
			// Your comment has been posted.
			
			HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["messages_page"], string( CSS_MESSAGES ), strReturnPage, NOT_INDEX, CODE_200 );
			
			pResponse->strContent += "<p class=\"messages_posted\">" + UTIL_Xsprintf( gmapLANG_CFG["messages_posted"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["messages"] + "\" href=\"" + RESPONSE_STR_MESSAGES_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";
			
// 			pResponse->strContent += "<script type=\"text/javascript\">window.location=\"" + RESPONSE_STR_STATS_HTML + "?info_hash=" + cstrHashString + "\"</script>\n\n";

			// Output common HTML tail
			HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_MESSAGES ) );
			
			return;
		}
		
		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["messages_page"], string( CSS_MESSAGES ), strReturnPage, NOT_INDEX, CODE_400 );

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_MESSAGES ) );
	}
}
