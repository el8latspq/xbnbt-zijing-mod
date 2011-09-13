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
#include "tracker.h"
#include "util.h"

void CTracker :: serverResponseCommentsGET( struct request_t *pRequest, struct response_t *pResponse )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
		if( IsIPBanned( pRequest, pResponse, btv, gmapLANG_CFG["comments_page"], string( CSS_COMMENTS ), NOT_INDEX ) )
			return;

	if( !m_bAllowComments )
	{
		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["comments_page"], string( CSS_COMMENTS ), string( ), NOT_INDEX, CODE_403 );

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_COMMENTS ) );

		return;
	}

	if( !pRequest->user.strUID.empty( ) && ( pRequest->user.ucAccess & m_ucAccessView ) )
	{
		const string cstrReturnPage( pRequest->mapParams["return"] );
			
		string cstrID = string( );
		string strIDName = string( );
		bool bOffer = false;
		
		if( pRequest->mapParams.find( "id" ) != pRequest->mapParams.end( ) )
		{
			cstrID = pRequest->mapParams["id"];
			strIDName = "id";
		}
		else if( pRequest->mapParams.find( "oid" ) != pRequest->mapParams.end( ) )
		{
			cstrID = pRequest->mapParams["oid"];
			strIDName = "oid";
			bOffer = true;
		}
		
		if( cstrID.find_first_not_of( "1234567890" ) != string :: npos )
			cstrID.erase( );
		
		vector< pair< string, string > > vecParams;
		vecParams.reserve(64);
		string strJoined = string( );

		vecParams.push_back( pair<string, string>( strIDName, cstrID ) );
		vecParams.push_back( pair<string, string>( string( "return" ), cstrReturnPage ) );
		
		strJoined = UTIL_RemoveHTML( UTIL_HTMLJoin( vecParams, string( "?" ), string( "&" ), string( "=" ) ) );
		
		string strDatabase = string( );
		string strIDKey = string( );
		if( bOffer )
		{
			strIDKey = "boid";
			strDatabase = "offer";
		}
		else
		{
			strIDKey = "btid";
			strDatabase = "allowed";
		}
		
		CMySQLQuery *pQuery = 0;
		
		vector<string> vecQuery;
		
		if( !cstrID.empty( ) )
		{
			if( bOffer )
			{
				pQuery = new CMySQLQuery( "SELECT bid,bfilename,bname,badded,bsize,bfiles,bcomment,btitle FROM offer WHERE bid=" + cstrID );
				
				vecQuery.reserve(8);
			}
			else
			{
				pQuery = new CMySQLQuery( "SELECT bid,bfilename,bname,badded,bsize,bfiles,bcomment,btitle,bnocomment FROM allowed WHERE bid=" + cstrID );
				
				vecQuery.reserve(9);
			}
			
			vecQuery = pQuery->nextRow( );
		
			delete pQuery;
		}
		
		if( vecQuery.size( ) == 0 || cstrID.empty( ) )
		{
			// Output common HTML head
			HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["comments_page"], string( CSS_COMMENTS ), string( ), NOT_INDEX, CODE_200 );
			
			pResponse->strContent += "<p class=\"not_exist\">" + UTIL_Xsprintf( gmapLANG_CFG["torrent_not_exist"].c_str( ), cstrID.c_str( ) );

			// Output common HTML tail
			HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_COMMENTS ) );

			return;
		}
		
		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["comments_page"], string( CSS_COMMENTS ), string( ), NOT_INDEX, CODE_200 );

		// assorted scripts (thanks SA)
		pResponse->strContent += "<script type=\"text/javascript\">\n";
		pResponse->strContent += "<!--\n";

		// validate
		pResponse->strContent += "function validate( theform ) {\n";
		pResponse->strContent += "if( theform.comment.value == \"\" ) {\n";
		pResponse->strContent += "  alert( \"" + gmapLANG_CFG["js_fill_fields"] + "\" );\n";
		pResponse->strContent += "  return false; }\n";
		pResponse->strContent += "if( theform.comment.value.getBytes() > " + CAtomInt( m_uiCommentLength ).toString( ) + " ) {\n";
		pResponse->strContent += "  alert( \"" + gmapLANG_CFG["js_too_long"] + "\\n" + m_strJSLength + "\\n" + m_strJSReduce + "\" );\n";
		pResponse->strContent += "  return false; }\n";
		pResponse->strContent += "else { return true; }\n";
		pResponse->strContent += "}\n\n";
		
		pResponse->strContent += "String.prototype.getBytes = function() {\n";
		pResponse->strContent += "  return this.length;\n";
//		pResponse->strContent += "  var cArr = this.match(/[^\\x00-\\xff]/ig);\n";
//		pResponse->strContent += "  return this.length + (cArr == null ? 0 : cArr.length);\n";
		pResponse->strContent += "}\n\n";
		
		pResponse->strContent += "String.prototype.stripX = function() {\n";
		pResponse->strContent += "  var str = this;\n";
		pResponse->strContent += "  str = str.replace(/\\&nbsp;/g,' ');\n";
		pResponse->strContent += "  str = str.replace(/\\&quot;/g,'\"');\n";
		pResponse->strContent += "  str = str.replace(/\\&amp;/g,'&');\n";
		pResponse->strContent += "  str = str.replace(/\\&gt;/g,'>');\n";
		pResponse->strContent += "  str = str.replace(/\\&lt;/g,'<');\n";
		pResponse->strContent += "  return str;\n";
		pResponse->strContent += "}\n\n";

		// checklength
		pResponse->strContent += "function checklength( theform ) {\n";
		pResponse->strContent += "  alert( \"" + UTIL_Xsprintf( gmapLANG_CFG["js_comment_length"].c_str( ), "\" + theform.comment.value.getBytes() + \"" ) + "\" );\n";
		pResponse->strContent += "}\n\n";
		
		// key post
		pResponse->strContent += "function keypost( evt, id ) {\n";
		pResponse->strContent += "  var key=evt.keyCode;\n";
		pResponse->strContent += "  if( (key==13) && (evt.ctrlKey) )\n";
		pResponse->strContent += "    document.getElementById( id ).click();\n";
		pResponse->strContent += "}\n\n";
		
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
		pResponse->strContent += "}\n\n";
		
		pResponse->strContent += "var posted;\n\n";
		
		// post
		pResponse->strContent += "function post(formId,textareaId,submitId) {\n";
		pResponse->strContent += "  var the_timeout;\n";
		pResponse->strContent += "  if (validate( document.postacomment )) {\n";
		pResponse->strContent += "    var postForm = document.getElementById( formId );\n";
		pResponse->strContent += "    var postTextarea = document.getElementById( textareaId );\n";
		pResponse->strContent += "    var postSubmit = document.getElementById( submitId );\n";
		pResponse->strContent += "    var post_data = '';\n";
		pResponse->strContent += "    for (var i = 0; i < postForm.elements.length; i++) {\n";
		pResponse->strContent += "      if (postForm.elements[i].name != 'submit_comment_button') {\n";
		pResponse->strContent += "        if ( postForm.elements[i].name == 'message' && postForm.elements[i].checked == false )\n";
		pResponse->strContent += "          continue;\n";
		pResponse->strContent += "        else {\n";
		pResponse->strContent += "          if( post_data != '' )\n";
		pResponse->strContent += "            post_data = post_data + '&';\n";
		pResponse->strContent += "          post_data = post_data + postForm.elements[i].name + '=' + encodeURIComponent(postForm.elements[i].value); }\n";
		pResponse->strContent += "      }\n";
		pResponse->strContent += "    }\n";
		pResponse->strContent += "    xmlhttp.onreadystatechange=function() {\n";
		pResponse->strContent += "      if (xmlhttp.readyState==4 && xmlhttp.status==200) {\n";
		pResponse->strContent += "          clearTimeout( the_timeout );\n";
		pResponse->strContent += "          postTextarea.value = '';\n";
//		pResponse->strContent += "          charleft(document.postatalk,'talk_left','submit_talk');\n";
		pResponse->strContent += "          document.postacomment.comment.focus();\n";
		pResponse->strContent += "          postSubmit.disabled = false;\n";
		pResponse->strContent += "          load('div','divComment',postForm.action+'?'+post_data); }\n";
		pResponse->strContent += "    }\n";
		pResponse->strContent += "    xmlhttp.open(\"POST\",postForm.action,true);\n";
//		pResponse->strContent += "    xmlhttp.setRequestHeader(\"Content-Length\", post_data.length);\n";
		pResponse->strContent += "    xmlhttp.setRequestHeader(\"Content-Type\", \"application/x-www-form-urlencoded\");\n";
//		pResponse->strContent += "    xmlhttp.setRequestHeader(\"Connection\", \"close\");\n";
		pResponse->strContent += "    xmlhttp.send(post_data);\n";
		pResponse->strContent += "    postSubmit.disabled = true;\n";
		pResponse->strContent += "    posted = false;\n";
		pResponse->strContent += "    var funcTimeout = call_timeout('submit_comment');\n";
		pResponse->strContent += "    the_timeout= setTimeout( funcTimeout, 15000 );\n";
		pResponse->strContent += "  }\n";
		pResponse->strContent += "}\n\n";
		
		// call_timeout
		pResponse->strContent += "function call_timeout(id) {\n";
		pResponse->strContent += "  return (function() {\n";
		pResponse->strContent += "    timeout(id); })\n";
		pResponse->strContent += "}\n\n";
		
		// post timeout
		pResponse->strContent += "function timeout(id) {\n";
		pResponse->strContent += "  var element = document.getElementById( id );\n";
		pResponse->strContent += "  if( element.disabled == true ) {\n";
		pResponse->strContent += "    alert('" + gmapLANG_CFG["talk_timeout"] + "');\n";
//		pResponse->strContent += "    clearAll();\n";
		pResponse->strContent += "    element.disabled = false; }\n";
		pResponse->strContent += "}\n\n";
		
		// reply
		pResponse->strContent += "function reply(replyId,textareaId) {\n";
		pResponse->strContent += "  var replySpan = document.getElementById( 'reply'+replyId );\n";
		pResponse->strContent += "  var replyData = replySpan.innerHTML;\n";
		pResponse->strContent += "  var replytoSpan = document.getElementById( 'replyto'+replyId );\n";
		pResponse->strContent += "  var replytoData = replytoSpan.innerHTML;\n";
		pResponse->strContent += "  var textarea = document.getElementById( textareaId );\n";
		pResponse->strContent += "  var textData = textarea.value;\n";
		pResponse->strContent += "  window.location.hash = \"commentarea\";\n";
		pResponse->strContent += "  document.postacomment.comment.focus();\n";
		pResponse->strContent += "  textarea.value = '[quote=' + replytoData + ']' + replyData.stripX( ) + '[/quote]\\n';\n";
		pResponse->strContent += "}\n\n";
//		pResponse->strContent += "function reply(replyId,formId,formName) {\n";
//		pResponse->strContent += "  var replyForm = document.getElementById( formId );\n";
//		pResponse->strContent += "  var get_data = '?reply=' + replyId;\n";
//		pResponse->strContent += "  for (var i = 0; i < replyForm.elements.length; i++) {\n";
//		pResponse->strContent += "    if ((replyForm.elements[i].name == 'submit') &&\n";
//		pResponse->strContent += "    (replyForm.elements[i].disabled)) {\n";
//		pResponse->strContent += "      return; }\n";
////		pResponse->strContent += "    if (replyForm.elements[i].name == 'id')\n";
//		pResponse->strContent += "      get_data = get_data + '&' + replyForm.elements[i].name + '=' + replyForm.elements[i].value;\n";
//		pResponse->strContent += "  }\n";
//		pResponse->strContent += "  xmlhttp.onreadystatechange=function() {\n";
//		pResponse->strContent += "    if (xmlhttp.readyState==4 && xmlhttp.status==200) {\n";
//		pResponse->strContent += "      var e = document.createElement('div');\n";
//                pResponse->strContent += "      e.innerHTML = xmlhttp.responseText;\n";
//                pResponse->strContent += "      var forms = e.getElementsByTagName('form');\n";
//		pResponse->strContent += "      for (var i = 0; i < forms.length; i++) {\n";
//		pResponse->strContent += "        if (forms[i].name == formName) {\n";
//		pResponse->strContent += "          replyForm.parentNode.innerHTML = forms[i].parentNode.innerHTML;\n";
//		pResponse->strContent += "  		window.location.hash = \"commentarea\";\n";
////		pResponse->strContent += "          document.postacomment.comment.focus();\n";
//		pResponse->strContent += "          break; }\n";
//		pResponse->strContent += "      }\n";
//		pResponse->strContent += "    }\n";
//		pResponse->strContent += "  }\n";
//		pResponse->strContent += "  xmlhttp.open(\"GET\",replyForm.action + get_data,true);\n";
//		pResponse->strContent += "  xmlhttp.send();\n";
//		pResponse->strContent += "}\n";
		
		// reply
//		pResponse->strContent += "function reply( commentid ) {\n";
//		pResponse->strContent += "var comment = document.getElementById( \"comment\" + commentid );\n";
//		pResponse->strContent += "var commenter = document.getElementById( \"commenter\" + commentid );\n";
//		pResponse->strContent += "var anchor = document.getElementById( \"commentarea\" );\n";
//		pResponse->strContent += "anchor.value = \"[quote=\" + commenter.innerHTML + \"]\" + comment.innerHTML + \"[/quote]\";\n";
//		pResponse->strContent += "window.location.hash = \"commentarea\";\n";
//		pResponse->strContent += "anchor.focus();\n";
//		pResponse->strContent += "}\n";
//
		pResponse->strContent += "function delete_comment_confirm(del,link) {\n";
		pResponse->strContent += "  link = link.replace(/&amp;/g,\"&\");\n";
		pResponse->strContent += "  if( confirm(\"" + gmapLANG_CFG["comments_delete_q"] + "\") ) {\n";
		pResponse->strContent += "    window.location = \'" + RESPONSE_STR_COMMENTS_HTML + "\' + link + \'&del=\' + del; }\n";
		pResponse->strContent += "}\n\n";
		pResponse->strContent += "//-->\n";

		pResponse->strContent += "function delete_all_comments_confirm(link) {\n";
		pResponse->strContent += "  link = link.replace(/&amp;/g,\"&\");\n";
		pResponse->strContent += "  if( confirm(\"" + gmapLANG_CFG["comments_delete_all_q"] + "\") ) {\n";
		pResponse->strContent += "    window.location = \'" + RESPONSE_STR_COMMENTS_HTML + "\' + link + \'&delall=1\'; }\n";
		pResponse->strContent += "}\n\n";
		pResponse->strContent += "</script>\n\n";

		if( vecQuery.size( ) == 8 || vecQuery.size( ) == 9 )
		{
			//
			// delete comment
			//

			if( pRequest->user.ucAccess & m_ucAccessDelComments )
			{
				const string cstrDelAll( pRequest->mapParams["delall"] );
				string cstrDel( pRequest->mapParams["del"] );
				
				if( cstrDel.find_first_not_of( "1234567890" ) != string :: npos )
					cstrDel.erase( );

				if( cstrDelAll == "1" )
				{
					m_pCache->setComment( cstrID, SET_COMMENT_CLEAR, bOffer );
					
					CMySQLQuery mq01( "DELETE FROM comments WHERE " + strIDKey + "=" + cstrID );
					CMySQLQuery mq02( "UPDATE " + strDatabase + " SET bcomments=0 WHERE bid=" + cstrID );

					// Deleted all comments.
					pResponse->strContent += "<p class=\"deleted\">" + UTIL_Xsprintf( gmapLANG_CFG["comments_deleted_all"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["comments"] + "\" href=\"" + RESPONSE_STR_COMMENTS_HTML + strJoined + "\">" ).c_str( ), "</a>" ) + "</p>\n";

					// Output common HTML tail
					HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_COMMENTS ) );

					return;
				}
				else if( !cstrDel.empty( ) )
				{
					CMySQLQuery *pQueryComment = new CMySQLQuery( "SELECT " + strIDKey + " FROM comments WHERE bid=" + cstrDel );
				
					vector<string> vecQueryComment;
				
					vecQueryComment.reserve(1);

					vecQueryComment = pQueryComment->nextRow( );
					
					delete pQueryComment;

					if( vecQueryComment.size( ) == 1 )
					{
						if( vecQueryComment[0] == cstrID )
						{
							CMySQLQuery *pQuery = new CMySQLQuery( "DELETE FROM comments WHERE bid=" + cstrDel );
							
							if( pQuery )
							{
								m_pCache->setComment( cstrID, SET_COMMENT_MINUS, bOffer );
								CMySQLQuery mq01( "UPDATE " + strDatabase + " SET bcomments=bcomments-1 WHERE bid=" + cstrID );
							}
							
							delete pQuery;
							

							// Deleted comment
							pResponse->strContent += "<p class=\"deleted\">" + UTIL_Xsprintf( gmapLANG_CFG["comments_deleted"].c_str( ), cstrDel.c_str( ), string( "<a title=\"" + gmapLANG_CFG["comments"] + "\" href=\"" + RESPONSE_STR_COMMENTS_HTML + strJoined + "\">" ).c_str( ), "</a>" ) + "</p>\n";

							// Output common HTML tail
							HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_COMMENTS ) );

							return;
						}
					}
					
					// Unable to delete comment
					pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["comments_unable_delete"].c_str( ), cstrDel.c_str( ), string( "<a title=\"" + gmapLANG_CFG["comments"] + "\" href=\"" + RESPONSE_STR_COMMENTS_HTML + strJoined + "\">" ).c_str( ), "</a>" ) + "</p>\n";

					// Output common HTML tail
					HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_COMMENTS ) );

					return;
				}
			}
			
			if( pRequest->user.ucAccess & m_ucAccessEditComments )
			{
				string cstrEdit( pRequest->mapParams["edit"] );
				if( cstrEdit.find_first_not_of( "1234567890" ) != string :: npos )
					cstrEdit.erase( );
				if( !cstrEdit.empty( ) )
				{
					CMySQLQuery *pQueryComment = new CMySQLQuery( "SELECT " + strIDKey + ",busername,buid,bip,bposted,bcomment FROM comments WHERE bid=" + cstrEdit );
				
					vector<string> vecQueryComment;
				
					vecQueryComment.reserve(6);

					vecQueryComment = pQueryComment->nextRow( );
					
					delete pQueryComment;
					
					if( vecQueryComment.size( ) == 6 )
					{
						if( vecQueryComment[0] == cstrID )
						{
							string strName = string( );
							string strNameID = string( );
							string strIP = string( );
							string strTime = string( );
							string strComText = string( );

							if( !vecQueryComment[1].empty( ) )
								strName = vecQueryComment[1];
							if( !vecQueryComment[2].empty( ) )
								strNameID = vecQueryComment[2];
							strIP = vecQueryComment[3];
							strTime = vecQueryComment[4];
							strComText = vecQueryComment[5];
									
							// Comment by
							string strUserLink = getUserLinkFull( strNameID, strName );
							string strHeader = string( );
							
							if( strUserLink.empty( ) )
								strUserLink = gmapLANG_CFG["unknown"];
							
							if( strIP.empty( ) )
								strIP = gmapLANG_CFG["unknown"];
							
							if( strTime.empty( ) )
								strTime = gmapLANG_CFG["unknown"];
							
//							strHeader = UTIL_Xsprintf( gmapLANG_CFG["comments_posted_by"].c_str( ), cstrEdit.c_str( ), strUserLink.c_str( ), strIP.c_str( ), strTime.c_str( ) );
							strHeader = UTIL_Xsprintf( gmapLANG_CFG["comments_posted_by"].c_str( ), cstrEdit.c_str( ), strUserLink.c_str( ), strTime.c_str( ) );
//							if( !strName.empty( ) )
//							{
//								CMySQLQuery *pQueryUser = new CMySQLQuery( "SELECT buid,baccess,bgroup,btitle FROM users WHERE buid=" + strNameID );
//	
//								vector<string> vecQueryUser;
//							
//								vecQueryUser.reserve(4);

//								vecQueryUser = pQueryUser->nextRow( );
//								
//								delete pQueryUser;
//								
//								if( vecQueryUser.size( ) == 4 && !vecQueryUser[0].empty( ) )
//								{
//									if( vecQueryUser[2] == "0" )
//										strAccess = UTIL_AccessToString( (unsigned char)atoi( vecQueryUser[1].c_str( ) ) );
//									else
//										strAccess = UTIL_GroupToString( (unsigned char)atoi( vecQueryUser[2].c_str( ) ) );
//									if( strAccess.empty( ) )
//										strAccess = gmapLANG_CFG["unknown"];
//									if( !vecQueryUser[3].empty( ) )
//										strAccess += " <span class=\"" + UTIL_UserClass( (unsigned char)atoi( vecQueryUser[1].c_str( ) ), (unsigned char)atoi( vecQueryUser[2].c_str( ) ) ) + "\">" + UTIL_RemoveHTML( vecQueryUser[3] ) + "</span>";
//									strIcon += getUserLink( strNameID, strName );
//									strHeader += UTIL_Xsprintf( gmapLANG_CFG["comments_posted_by"].c_str( ), cstrEdit.c_str( ), cstrEdit.c_str( ), strIcon.c_str( ), strAccess.c_str( ), strIP.c_str( ), strTime.c_str( ) );
//								}
//								else
//									strHeader += UTIL_Xsprintf( gmapLANG_CFG["comments_posted_by"].c_str( ), cstrEdit.c_str( ), cstrEdit.c_str( ), UTIL_RemoveHTML( strName ).c_str( ), strAccess.c_str( ), strIP.c_str( ), strTime.c_str( ) );
//							}
//							else
//								strHeader += UTIL_Xsprintf( gmapLANG_CFG["comments_posted_by"].c_str( ), cstrEdit.c_str( ), cstrEdit.c_str( ), gmapLANG_CFG["unknown"].c_str( ), gmapLANG_CFG["unknown"].c_str( ), strIP.c_str( ), strTime.c_str( ) );

							pResponse->strContent += "<div class=\"comments_table_post\">\n";
							pResponse->strContent += "<p class=\"comments_table_post\">" + gmapLANG_CFG["comments_edit_comment"] + "</p>\n";
							pResponse->strContent += "<p class=\"comments_table_post\">" + strHeader + "</p>\n";
							pResponse->strContent += "<table class=\"comments_table_post\">\n";
							
							pResponse->strContent += "<form id=\"editacomment\" method=\"post\" action=\"" + RESPONSE_STR_COMMENTS_HTML + "\" name=\"postacomment\" onSubmit=\"return validate( this )\" enctype=\"multipart/form-data\">\n";
							
							pResponse->strContent += "<input name=\"edit\" type=hidden value=\"" + cstrEdit + "\">\n";
							pResponse->strContent += "<input name=\"";
							if( bOffer )
								pResponse->strContent += "oid";
							else
								pResponse->strContent += "id";
							pResponse->strContent += "\" type=hidden value=\"" + cstrID+ "\">\n";
							if( !cstrReturnPage.empty( ) )
								pResponse->strContent += "<input name=\"return\" type=hidden value=\"" + cstrReturnPage + "\">\n";
//							if( bOffer )
//								pResponse->strContent += "<input name=\"offer\" type=hidden value=\"1\">\n";
							
							pResponse->strContent += "<tr class=\"comments_table_post\">\n";
							pResponse->strContent += "<td class=\"comments_table_post\">\n";
							pResponse->strContent += "<textarea id=\"commentarea\" name=\"comment\" rows=8 cols=64 onKeyDown=\"javascript: keypost(event,'submit_comment');\">";
							string :: size_type iStart = strComText.find( gmapLANG_CFG["comments_edited_by"].substr( 0, 6 ) );
							if( iStart != string :: npos )
								pResponse->strContent += UTIL_RemoveHTML3( strComText.substr( 0, iStart - 1 ) );
							else
								pResponse->strContent += UTIL_RemoveHTML3( strComText );
							pResponse->strContent += "</textarea>\n";
							pResponse->strContent += "</td>\n</tr>\n";
							
							pResponse->strContent += "<tr class=\"comments_table_post\">\n";
							pResponse->strContent += "<td class=\"comments_table_post\">\n";
							pResponse->strContent += "<ul>\n";

							// Comments must be less than
							pResponse->strContent += "<li>" + UTIL_Xsprintf( gmapLANG_CFG["comments_length_info"].c_str( ), CAtomInt( m_uiCommentLength ).toString( ).c_str( ) );
							pResponse->strContent += " [<a title=\"" + gmapLANG_CFG["comment_check_length"] + "\" href=\"javascript: checklength( document.postacomment );\">" + gmapLANG_CFG["comment_check_length"] + "</a>]\n</li>\n";
							pResponse->strContent += "<li>" + gmapLANG_CFG["no_html"] + "</li>\n";
							pResponse->strContent += "</ul>\n";
							pResponse->strContent += "</td>\n</tr>\n";
							
							pResponse->strContent += "<tr class=\"comments_table_post\">\n";
							pResponse->strContent += "<td class=\"comments_table_post\">\n";
							pResponse->strContent += "<div class=\"comments_table_post_button\">\n";
							pResponse->strContent += Button_Submit( "submit_comment", string( gmapLANG_CFG["Submit"] ) );
							pResponse->strContent += "</div>\n";
							pResponse->strContent += "</td>\n</tr>\n";
							pResponse->strContent += "</form>\n";
							pResponse->strContent += "</table>\n";
							pResponse->strContent += "</div>\n";
							
							HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_COMMENTS ) );
									
							return;
						}
					}

					// Unable to edit comment
					pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["comments_unable_edit"].c_str( ), cstrEdit.c_str( ), string( "<a title=\"" + gmapLANG_CFG["comments"] + "\" href=\"" + RESPONSE_STR_COMMENTS_HTML + strIDName + cstrID + "\">" ).c_str( ), "</a>" ) + "</p>\n";

					// Output common HTML tail
					HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_COMMENTS ) );

					return;
				}
			}
			
			if( vecQuery.size( ) == 8 || vecQuery.size( ) == 9 )
			{
				string strOldName = string( );
				if( !vecQuery[7].empty( ) )
					strOldName = vecQuery[7];
				if( strOldName.empty( ) )
					strOldName = vecQuery[2];
					
				pResponse->strContent += "<div class=\"comments_info_table\">\n";
				pResponse->strContent += "<h3>" + gmapLANG_CFG["comments"] + "</h3>\n";
				pResponse->strContent += "<p><a class=\"stats\" title=\"" + gmapLANG_CFG["name"] + ": " + UTIL_RemoveHTML( strOldName ) + "\" href=\"" + RESPONSE_STR_STATS_HTML + strJoined;
				pResponse->strContent += "\">";
				pResponse->strContent += UTIL_RemoveHTML( strOldName ) + "</a></p>\n";
				pResponse->strContent += "</div>\n";
			}

			// display torrent information list
			
//			if( vecQuery.size( ) == 8 || vecQuery.size( ) == 9 )
//			{
//				string strOldName = string( );
//				if( !vecQuery[7].empty( ) )
//					strOldName = vecQuery[7];
//				if( strOldName.empty( ) )
//					strOldName = vecQuery[2];

//				pResponse->strContent += "<div class=\"comments_info_table\">\n";
//				pResponse->strContent += "<p>" + gmapLANG_CFG["comments_file_information"] + "</p>\n";
//				pResponse->strContent += "<p><a class=\"stats\" title=\"" + gmapLANG_CFG["name"] + ": " + UTIL_RemoveHTML( strOldName ) + "\" href=\"" + RESPONSE_STR_STATS_HTML + strIDName + cstrID;
//				pResponse->strContent += "\">";
//				pResponse->strContent += UTIL_RemoveHTML( strOldName ) + "</a></p>\n";

//				pResponse->strContent += "<table class=\"comments_info_table\" summary=\"file information\">\n";
//				pResponse->strContent += "<tr class=\"comments_info_table\">\n";
//				pResponse->strContent += "<td class=\"comments_info_table\">\n";
//				pResponse->strContent += "<ul>\n";

//				if( !vecQuery[1].empty( ) )
//					pResponse->strContent += "<li><strong>" + gmapLANG_CFG["name"] + ":</strong> " + UTIL_RemoveHTML( vecQuery[1] ) + "</li>\n";

//				if( !vecQuery[3].empty( ) )
//					pResponse->strContent += "<li><strong>" + gmapLANG_CFG["added"] + ":</strong> " + vecQuery[3] + "</li>\n";

//				if( !vecQuery[4].empty( ) )
//					pResponse->strContent += "<li><strong>" + gmapLANG_CFG["size"] + ":</strong> " + UTIL_BytesToString( UTIL_StringTo64( vecQuery[4].c_str( ) ) ) + "</li>\n";


//				if( !vecQuery[5].empty( ) )
//					pResponse->strContent += "<li><strong>" + gmapLANG_CFG["files"] + ":</strong> " + vecQuery[5] + "</li>\n";

//				pResponse->strContent += "</ul>\n";
//				pResponse->strContent += "</td>\n";
//				pResponse->strContent += "</tr>\n";
//				pResponse->strContent += "</table>\n";
//				pResponse->strContent += "</div>\n";

//				if( !vecQuery[5].empty( ) )
//				{
//					if( m_bShowFileComment )
//					{
//						pResponse->strContent += "<div class=\"comments_table\">\n";
//						pResponse->strContent += "<p>" + gmapLANG_CFG["file_comment"] + "</p>\n";
//						pResponse->strContent += "<table  class=\"comments_table\" summary=\"file comment\">\n";
//						pResponse->strContent += "<tr class=\"comments_table\"><td  class=\"comments_table\">" + UTIL_RemoveHTML( vecQuery[5] ) + "</td></tr>\n";
//						pResponse->strContent += "</table>\n";
//						pResponse->strContent += "</div>\n";
//					}
//				}
//			}
			
			string strReply( pRequest->mapParams["reply"] );
			if( strReply.find_first_not_of( "1234567890" ) != string :: npos )
				strReply.erase( );
			
			bool bNoComment = false;
			if( !bOffer )
			{
				if( !vecQuery[8].empty( ) && vecQuery[8] == "1" )
					bNoComment = true;
			}
			
			pResponse->strContent += "<div id=\"divComment\" class=\"comments_table_posted\">";
			
			if( strReply.empty( ) )
			{
				CMySQLQuery *pQueryComment = new CMySQLQuery( "SELECT bid,busername,buid,bip,bposted,bcomment FROM comments WHERE " + strIDKey + "=" + cstrID + " ORDER BY bposted" );
				
				vector<string> vecQueryComment;
		
				vecQueryComment.reserve(6);

				vecQueryComment = pQueryComment->nextRow( );
			
				unsigned long ulCount = 0;

				if( vecQueryComment.size( ) == 6 )
				{
					if( pRequest->user.ucAccess & m_ucAccessDelComments )
					{
						pResponse->strContent += "<p>[<a class=\"red\" title=\"" + gmapLANG_CFG["delete_all"] + "\" href=\"javascript: delete_all_comments_confirm('" + UTIL_StringToEscaped( strJoined ) + "');";
						pResponse->strContent += "\">" + gmapLANG_CFG["delete_all"] + "</a>]</p>\n";
					}

					pResponse->strContent += "<table class=\"comments_table_posted\" summary=\"comments\">\n";

					while( vecQueryComment.size( ) == 6 )
					{
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

//						iStart = strIP.rfind( "." );
//
//						if( iStart != string :: npos )
//						{
//							// don't strip ip for mods
//
//							if( !( pRequest->user.ucAccess & m_ucAccessShowIP ) && pRequest->user.strUID != strNameID )
//								strIP = strIP.substr( 0, iStart + 1 ) + "xxx";
//						}
//						else
//						{
//							iStart = strIP.rfind( ":" );
//							if( iStart != string :: npos )
//							{
//								// don't strip ip for mods
//
//								if( !( pRequest->user.ucAccess & m_ucAccessShowIP ) && pRequest->user.strUID != strNameID )
//									strIP = strIP.substr( 0, iStart + 1 ) + "xxxx";
//							}
//
//						}

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
				
						pResponse->strContent += "<tr class=\"com_header\">";
						pResponse->strContent += "<td class=\"com_header\">";
						pResponse->strContent += "<span style=\"display: none\" id=\"replyto" + strID + "\">" + UTIL_RemoveHTML( strName ) + "</span>\n";
//						pResponse->strContent += UTIL_Xsprintf( gmapLANG_CFG["comments_posted_by"].c_str( ), strID.c_str( ), strUserLink.c_str( ), strIP.c_str( ), strTime.c_str( ) );
						pResponse->strContent += UTIL_Xsprintf( gmapLANG_CFG["comments_posted_by"].c_str( ), strID.c_str( ), strUserLink.c_str( ), strTime.c_str( ) );
		//				if( !strName.empty( ) )
		//				{
		//					if( !strNameID.empty( ) )
		//					{
		//						if( vecQueryComment[7] == "0" )
		//							strAccess = UTIL_AccessToString( (unsigned char)atoi( vecQueryComment[6].c_str( ) ) );
		//						else
		//							strAccess = UTIL_GroupToString( (unsigned char)atoi( vecQueryComment[7].c_str( ) ) );
		//						if( strAccess.empty( ) )
		//							strAccess = gmapLANG_CFG["unknown"];
		//						if( !vecQueryComment[8].empty( ) )
		//							strAccess += " <span class=\"" + UTIL_UserClass( (unsigned char)atoi( vecQueryComment[6].c_str( ) ), (unsigned char)atoi( vecQueryComment[7].c_str( ) ) ) + "\">" + UTIL_RemoveHTML( vecQueryComment[8] ) + "</span>";
		//						strIcon += getUserLink( strNameID, strName );

		//						pResponse->strContent += "<tr class=\"com_header\"><td class=\"com_header\">" + UTIL_Xsprintf( gmapLANG_CFG["comments_posted_by"].c_str( ), strID.c_str( ), CAtomInt( ulCount + 1 ).toString( ).c_str( ), strIcon.c_str( ), strAccess.c_str( ), strIP.c_str( ), strTime.c_str( ) );
		//					}
		//					else
		//						pResponse->strContent += "<tr class=\"com_header\"><td class=\"com_header\">" + UTIL_Xsprintf( gmapLANG_CFG["comments_posted_by"].c_str( ), strID.c_str( ), CAtomInt( ulCount + 1 ).toString( ).c_str( ), UTIL_RemoveHTML( strName ).c_str( ), strAccess.c_str( ), strIP.c_str( ), strTime.c_str( ) );
		//				}
		//				else
		//					pResponse->strContent += "<tr class=\"com_header\"><td class=\"com_header\">" + UTIL_Xsprintf( gmapLANG_CFG["comments_posted_by"].c_str( ), strID.c_str( ), CAtomInt( ulCount + 1 ).toString( ).c_str( ), gmapLANG_CFG["unknown"].c_str( ), gmapLANG_CFG["unknown"].c_str( ), strIP.c_str( ), strTime.c_str( ) );
				
						if( ( pRequest->user.ucAccess & m_ucAccessComments ) && ( !bNoComment || pRequest->user.ucAccess & m_ucAccessCommentsAlways ) )
						{
							pResponse->strContent += " [<a class=\"black\" title=\"" + gmapLANG_CFG["comments_reply"] + "\" href=\"javascript:;\" onclick=\"javascript: reply( '" + strID + "', 'commentarea' );\">" + gmapLANG_CFG["comments_reply"] + "</a>]";
		//					pResponse->strContent += " [<a class=\"black\" title=\"" + gmapLANG_CFG["comments_reply"] + "\" href=\"" + RESPONSE_STR_COMMENTS_HTML + strJoined + "&amp;reply=" + strID;
		//					pResponse->strContent += "#commentarea\">" + gmapLANG_CFG["comments_reply"] + "</a>]";
						}
						if( pRequest->user.ucAccess & m_ucAccessEditComments )
						{
							pResponse->strContent += " [<a class=\"black\" title=\"" + gmapLANG_CFG["edit"] + "\" href=\"" + RESPONSE_STR_COMMENTS_HTML + strJoined + "&amp;edit=" + strID;
							pResponse->strContent += "\">" + gmapLANG_CFG["edit"] + "</a>]";
						}
						if( pRequest->user.ucAccess & m_ucAccessDelComments )
						{
							pResponse->strContent += " [<a class=\"red\" title=\"" + gmapLANG_CFG["delete"] + "\" href=\"javascript: delete_comment_confirm('" + strID + "','" + UTIL_StringToEscaped( strJoined ) + "');";
							pResponse->strContent += "\">" + gmapLANG_CFG["delete"] + "</a>]";
						}

						pResponse->strContent += "</td>\n";
						pResponse->strContent += "<td class=\"com_floor\">";
						pResponse->strContent += UTIL_Xsprintf( gmapLANG_CFG["comments_posted_floor"].c_str( ), CAtomInt( ulCount + 1 ).toString( ).c_str( ) );
						pResponse->strContent += "</td></tr>\n";

						//
						// body
						//
				
						pResponse->strContent += "<tr class=\"com_body\">";
						pResponse->strContent += "<td class=\"com_body\" colspan=2>";
						pResponse->strContent += "<span style=\"display: none\" id=\"reply" + strID + "\">";
						iStart = strComText.find( gmapLANG_CFG["comments_edited_by"].substr( 0, 6 ) );
						if( iStart != string :: npos )
							pResponse->strContent += UTIL_RemoveHTML3( strComText.substr( 0, iStart - 1 ) );
						else
							pResponse->strContent += UTIL_RemoveHTML3( strComText );
						pResponse->strContent += "</span>\n";
						pResponse->strContent += "<div class=\"comment\">" + UTIL_RemoveHTML2( strComText ) + "</div></td></tr>\n";

						ulCount++;
				
						vecQueryComment = pQueryComment->nextRow( );
					}
					pResponse->strContent += "</table>\n";
				}
				else
					pResponse->strContent += "<p class=\"comments_table_posted\">" + gmapLANG_CFG["comments_no_comment"] + "</p>\n";
			
				delete pQueryComment;
			}
			
			pResponse->strContent += "</div>\n";
			
			if( ( pRequest->user.ucAccess & m_ucAccessComments ) && ( !bNoComment || pRequest->user.ucAccess & m_ucAccessCommentsAlways ) )
			{
				pResponse->strContent += "<div class=\"comments_table_post\">\n";
				pResponse->strContent += "<p class=\"comments_table_post\">" + gmapLANG_CFG["comments_post_comment"] + "</p>\n";
				
				pResponse->strContent += "<form id=\"postForm\" method=\"post\" action=\"" + RESPONSE_STR_COMMENTS_HTML + "\" name=\"postacomment\" onSubmit=\"return validate( this )\" enctype=\"multipart/form-data\">\n";
				pResponse->strContent += "<table class=\"comments_table_post\">\n";
				
				pResponse->strContent += "<input name=\"";
				if( bOffer )
					pResponse->strContent += "oid";
				else
					pResponse->strContent += "id";
				pResponse->strContent += "\" type=hidden value=\"" + cstrID + "\">\n";
				if( !cstrReturnPage.empty( ) )
					pResponse->strContent += "<input name=\"return\" type=hidden value=\"" + cstrReturnPage + "\">\n";
				
				pResponse->strContent += "<tr class=\"comments_table_post\">\n";
				pResponse->strContent += "<td class=\"comments_table_post\">\n";
				pResponse->strContent += "<textarea id=\"commentarea\" name=\"comment\" rows=8 cols=64 onKeyDown=\"javascript: keypost(event,'submit_comment');\">";
				
				if( !strReply.empty( ) )
				{
					CMySQLQuery *pQueryComment = new CMySQLQuery( "SELECT busername,bcomment FROM comments WHERE bid=" + strReply );
				
					vector<string> vecQueryComment;
	
					vecQueryComment.reserve(2);

					vecQueryComment = pQueryComment->nextRow( );
					
					delete pQueryComment;

					if( vecQueryComment.size( ) == 2 )
					{
						string strReplyTo = vecQueryComment[0];
						string strReplyCom = vecQueryComment[1];
						
						pResponse->strContent += "[quote=" + UTIL_RemoveHTML( strReplyTo ) + "]";
						string :: size_type iStart = strReplyCom.find( gmapLANG_CFG["comments_edited_by"].substr( 0, 6 ) );
						if( iStart != string :: npos )
							pResponse->strContent += UTIL_RemoveHTML3( strReplyCom.substr( 0, iStart - 1 ) );
						else
							pResponse->strContent += UTIL_RemoveHTML3( strReplyCom );
						pResponse->strContent += "[/quote]\n";
					}
				}
				
				pResponse->strContent += "</textarea>\n";
				pResponse->strContent += "</td>\n</tr>\n";

				pResponse->strContent += "<tr class=\"comments_table_post\">\n";
				pResponse->strContent += "<td class=\"comments_table_post\">\n";
				pResponse->strContent += "<ul>\n";

				// Comments must be less than
				pResponse->strContent += "<li>" + UTIL_Xsprintf( gmapLANG_CFG["comments_length_info"].c_str( ), CAtomInt( m_uiCommentLength ).toString( ).c_str( ) );
				pResponse->strContent += " [<a title=\"" + gmapLANG_CFG["comment_check_length"] + "\" href=\"javascript: checklength( document.postacomment );\">" + gmapLANG_CFG["comment_check_length"] + "</a>]\n</li>\n";
				pResponse->strContent += "<li>" + gmapLANG_CFG["no_html"] + "</li>\n";
				pResponse->strContent += "</ul>\n";
				pResponse->strContent += "</td>\n</tr>\n";
				pResponse->strContent += "<tr class=\"comments_table_post\">\n";
				pResponse->strContent += "<td class=\"comments_table_post\">\n";
				pResponse->strContent += "<div class=\"comments_table_post_button\">\n";
				pResponse->strContent += "<input name=\"submit_comment_button\" id=\"submit_comment\" alt=\"" + gmapLANG_CFG["Submit"] + "\" type=button value=\"" + gmapLANG_CFG["Submit"] + "\" onClick=\"javascript: post('postForm','commentarea','submit_comment');\">\n";
//				pResponse->strContent += Button_Submit( "submit_comment", string( gmapLANG_CFG["Submit"] ) );
				if( pRequest->user.ucAccess & m_ucAccessCommentsToMessage )
				{
					pResponse->strContent += "<input id=\"id_message\" name=\"message\" alt=\"[" + gmapLANG_CFG["comments_send_message"] + "]\" type=checkbox";
					if( bOffer )
						pResponse->strContent += " checked=\"checked\"";
					pResponse->strContent += "> <label for=\"id_message\">" + gmapLANG_CFG["comments_send_message"] + "</label> \n";
				}
				pResponse->strContent += "</div>\n</td>\n</tr>\n";

				pResponse->strContent += "</table>\n";
				pResponse->strContent += "</form>\n";
				pResponse->strContent += "</div>\n";
			}
			else
				pResponse->strContent += "<p class=\"denied\">" + gmapLANG_CFG["comments_post_disallowed"] + "</p>\n";
		}

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_COMMENTS ) );
	}
	else
	{
		// Not authorised

		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["comments_page"], string( CSS_COMMENTS ), string( ), NOT_INDEX, CODE_401 );

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_COMMENTS ) );
	}
}

void CTracker :: serverResponseCommentsPOST( struct request_t *pRequest, struct response_t *pResponse, CAtomList *pPost )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
		if( IsIPBanned( pRequest, pResponse, btv, gmapLANG_CFG["comments_page"], string( CSS_COMMENTS ), NOT_INDEX ) )
			return;
	
	if( pRequest->user.ucAccess & m_ucAccessComments )
	{
		string strComment = string( );
		string cstrEdit = string( );
		string cstrID = string( );
		string cstrReturnPage = string( );
		bool bOffer = false;
		bool bSendMessage = false;
		
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
							
							if( strName == "comment" )
								strComment = pData->toString( );
							else if( strName == "message" && pData->toString( ) == "on" && ( pRequest->user.ucAccess & m_ucAccessCommentsToMessage ) )
								bSendMessage = true;
							else if( strName == "edit" )
								cstrEdit = pData->toString( );
							else if( strName == "id" )
								cstrID = pData->toString( );
							else if( strName == "oid" )
							{
								cstrID = pData->toString( );
								bOffer = true;
							}
							else if( strName == "return" )
								cstrReturnPage = pData->toString( );
						}
						else
						{
							// Output common HTML head
							HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["comments_page"], string( CSS_COMMENTS ), string( ), NOT_INDEX, CODE_400 );

							// failed
							pResponse->strContent += "<p class=\"failed\">" + gmapLANG_CFG["failed"] + "</p>\n";
							// Signal a bad request
							pResponse->strContent += "<p class=\"body_upload\">400 " + gmapLANG_CFG["server_response_400"] + "</p>\n";

							// Output common HTML tail
							HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_COMMENTS ) );

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
			HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["comments_page"], string( CSS_COMMENTS ), string( ), NOT_INDEX, CODE_400 );

			// failed
			pResponse->strContent += "<p class=\"failed\">" + gmapLANG_CFG["failed"] + "</p>\n";
			// Signal a bad request
			pResponse->strContent += "<p class=\"body_upload\">400 " + gmapLANG_CFG["server_response_400"] + "</p>\n";

			// Output common HTML tail
			HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_COMMENTS ) );

			if( gbDebug )
				UTIL_LogPrint( "Upload Warning - Bad request (no post received)\n" );

			return;
		}
		
		string strDatabase = string( );
		string strIDKey = string( );
		string strIDName = string( );

		if( bOffer )
		{
			strIDKey = "boid";
			strDatabase = "offer";
			strIDName = "oid";
		}
		else
		{
			strIDKey = "btid";
			strDatabase = "allowed";
			strIDName = "id";
		}
		
		if( cstrID.find_first_not_of( "1234567890" ) != string :: npos )
			cstrID.erase( );
		if( cstrEdit.find_first_not_of( "1234567890" ) != string :: npos )
			cstrEdit.erase( );
		
		vector< pair< string, string > > vecParams;
		vecParams.reserve(64);
		string strJoined = string( );
		string strJoinedHTML = string( );

		vecParams.push_back( pair<string, string>( strIDName, cstrID ) );
		vecParams.push_back( pair<string, string>( string( "return" ), cstrReturnPage ) );
		
		strJoinedHTML = UTIL_HTMLJoin( vecParams, string( "?" ), string( "&" ), string( "=" ) );
		strJoined = UTIL_RemoveHTML( strJoinedHTML );
		
		CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bid,bname,btitle,buploaderid FROM " + strDatabase + " WHERE bid=" + cstrID );
			
		vector<string> vecQuery;
	
		vecQuery.reserve(4);

		vecQuery = pQuery->nextRow( );
		
		delete pQuery;
		
		if( vecQuery.size( ) == 4 && ( pRequest->user.ucAccess & m_ucAccessEditComments ) )
		{
			if( !cstrEdit.empty( ) )
			{
				CMySQLQuery *pQueryComment = new CMySQLQuery( "SELECT " + strIDKey + " FROM comments WHERE bid=" + cstrEdit );
				
				vector<string> vecQueryComment;
			
				vecQueryComment.reserve(1);

				vecQueryComment = pQueryComment->nextRow( );
				
				delete pQueryComment;
				
				if( vecQueryComment.size( ) == 1 )
				{
					if( vecQueryComment[0] == cstrID )
					{
// 						strComment = strComment.substr( 0, m_uiCommentLength );
						
						if( strComment.empty( ) )
						{
							
							HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["comments_page"], string( CSS_COMMENTS ), string( ), NOT_INDEX, CODE_200 );
							//You must fill in all the fields.
							pResponse->strContent += "<p class=\"fill_all\">" + UTIL_Xsprintf( gmapLANG_CFG["comments_fill_warning"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["comments"] + "\" href=\"" + RESPONSE_STR_COMMENTS_HTML + strJoined + "\">" ).c_str( ), "</a>" ) + "</p>\n";

							// Output common HTML tail
							HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_COMMENTS ) );

							return;
						}
						
						time_t tNow = time( 0 );
						char pTime[256];
						memset( pTime, 0, sizeof( pTime ) / sizeof( char ) );
						strftime( pTime, sizeof( pTime ) / sizeof( char ), "%Y-%m-%d %H:%M:%S", localtime( &tNow ) );
						
						strComment += "\n" + UTIL_Xsprintf( gmapLANG_CFG["comments_edited_by"].c_str( ), UTIL_AccessToString( pRequest->user.ucAccess ).c_str( ), pRequest->user.strLogin.c_str( ), pTime );
						CMySQLQuery mq01( "UPDATE comments SET bcomment=\'" + UTIL_StringToMySQL( strComment ) + "\' WHERE bid=" + cstrEdit );

						// Your comment has been edited.

						HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["comments_page"], string( CSS_COMMENTS ), string( STATS_HTML + strJoinedHTML ), NOT_INDEX, CODE_200 );
						pResponse->strContent += "<p class=\"comments_posted\">" + UTIL_Xsprintf( gmapLANG_CFG["comments_edited"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["comments"] + "\" href=\"" + RESPONSE_STR_STATS_HTML + strJoined + "\">" ).c_str( ), "</a>" ) + "</p>\n";
						
						// Output common HTML tail
						HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_COMMENTS ) );

						return;

					}
				}
				
				HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["comments_page"], string( CSS_COMMENTS ), string( ), NOT_INDEX, CODE_200 );
				// Unable to edit comment

				pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["comments_unable_edit"].c_str( ), cstrEdit.c_str( ), string( "<a title=\"" + gmapLANG_CFG["comments"] + "\" href=\"" + RESPONSE_STR_COMMENTS_HTML + strJoined + "\">" ).c_str( ), "</a>" ) + "</p>\n";

				// Output common HTML tail
				HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_COMMENTS ) );

				return;

			}
		}
		
		bool bNoComment = false;
		
		if( !bOffer )
		{
			CMySQLQuery *pQueryCommment = new CMySQLQuery( "SELECT bnocomment FROM allowed WHERE bid=" + cstrID );
				
			vector<string> vecQueryComment;
		
			vecQueryComment.reserve(1);
			
			vecQueryComment = pQueryCommment->nextRow( );
			
			delete pQueryCommment;
			
			if( vecQueryComment[0] == "1" )
				bNoComment = true;
		}
		
		if( vecQuery.size( ) == 4 && ( !bNoComment || pRequest->user.ucAccess & m_ucAccessCommentsAlways ) )
		{
// 			strComment = strComment.substr( 0, m_uiCommentLength );

			if( strComment.empty( ) )
			{
				
				HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["comments_page"], string( CSS_COMMENTS ), string( ), NOT_INDEX, CODE_200 );
				//You must fill in all the fields.

				pResponse->strContent += "<p class=\"fill_all\">" + UTIL_Xsprintf( gmapLANG_CFG["comments_fill_warning"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["comments"] + "\" href=\"" + RESPONSE_STR_COMMENTS_HTML + strJoined + "\">" ).c_str( ), "</a>" ) + "</p>\n";

				// Output common HTML tail
				HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_COMMENTS ) );

				return;
			}

//			time_t tNow = time( 0 );
//			char pTime[256];
//			memset( pTime, 0, sizeof( pTime ) / sizeof( char ) );
//			strftime( pTime, sizeof( pTime ) / sizeof( char ), "%Y-%m-%d %H:%M:%S", localtime( &tNow ) );
			
			string strQuery = "INSERT INTO comments (";
			strQuery += strIDKey;
			strQuery += ",busername,buid,bip,bposted,bcomment) VALUES(";
			strQuery += cstrID;
			strQuery += ",\'" + UTIL_StringToMySQL( pRequest->user.strLogin );
			strQuery += "\'," + pRequest->user.strUID;
			strQuery += ",\'" + UTIL_StringToMySQL( pRequest->strIP );
			strQuery += "\',NOW()";
			strQuery += ",\'" + UTIL_StringToMySQL( strComment );
			strQuery += "\')";
			
			CMySQLQuery *pQuery = new CMySQLQuery( strQuery );
			
			if( pQuery )
			{
				m_pCache->setComment( cstrID, SET_COMMENT_ADD, bOffer );
				
				CMySQLQuery mq01( "UPDATE " + strDatabase + " SET bcomments=bcomments+1 WHERE bid=" + cstrID );

				if( !vecQuery[3].empty( ) && vecQuery[3] != pRequest->user.strUID )
				{
					bool bMsgUploader = false;
					set<string> setMessage;

					CMySQLQuery *pQueryPrefs = new CMySQLQuery( "SELECT bmsgcomment FROM users_prefs WHERE buid=" + vecQuery[3] );
				
					map<string, string> mapPrefs;

					mapPrefs = pQueryPrefs->nextRowMap( );
				
					delete pQueryPrefs;
					
					if( bSendMessage || ( mapPrefs.size( ) == 1 && mapPrefs["bmsgcomment"] == "1" ) )
					{
						setMessage.insert( vecQuery[3] );
						bMsgUploader = true;
					}

					if( !bOffer )
					{
						CMySQLQuery *pQueryMessage = new CMySQLQuery( "SELECT bookmarks.buid,users_prefs.bmsgcommentbm FROM bookmarks LEFT JOIN users_prefs ON bookmarks.buid=users_prefs.buid WHERE bookmarks.bid=" + vecQuery[0] );
								
						vector<string> vecQueryMessage;
					
						vecQueryMessage.reserve(1);
						
						vecQueryMessage = pQueryMessage->nextRow( );

						while( vecQueryMessage.size( ) == 2 )
						{
							if( !vecQueryMessage[0].empty( ) && vecQueryMessage[1] == "1" && vecQueryMessage[0] != pRequest->user.strUID )
								setMessage.insert( vecQueryMessage[0] );

							vecQueryMessage = pQueryMessage->nextRow( );
						}
						
						delete pQueryMessage;
					}

					for ( set<string> :: iterator it = setMessage.begin( ); it != setMessage.end( ); it++ )
					{
						CMySQLQuery *pQueryUser = new CMySQLQuery( "SELECT buid FROM users WHERE buid=" + (*it) );
					
						if( pQueryUser->numRows( ) == 1 )
						{
							string strName = vecQuery[2];
							string strPage = RESPONSE_STR_STATS_HTML;
							if( bOffer )
								strPage += "?oid=";
							else
								strPage += "?id=";
							strPage += vecQuery[0];
							if( strName.empty( ) )
								strName = vecQuery[1];
					
							string strTitle = gmapLANG_CFG["admin_add_comment_title"];
							string strMessage = string( );

							if( (*it) == vecQuery[3] && bMsgUploader )
							{
								strMessage = UTIL_Xsprintf( gmapLANG_CFG["admin_add_comment"].c_str( ), UTIL_AccessToString( pRequest->user.ucAccess ).c_str( ), pRequest->user.strLogin.c_str( ), strPage.c_str( ), strName.c_str( ) );
					
								if( bSendMessage )
								{
									sendMessage( pRequest->user.strLogin, pRequest->user.strUID, (*it), pRequest->strIP, strTitle, strMessage );
								}
								else
									sendMessage( "", "0", (*it), "127.0.0.1", strTitle, strMessage );
							}
							else
							{
								strMessage = UTIL_Xsprintf( gmapLANG_CFG["admin_add_comment_bookmarked"].c_str( ), UTIL_AccessToString( pRequest->user.ucAccess ).c_str( ), pRequest->user.strLogin.c_str( ), strPage.c_str( ), strName.c_str( ) );

								sendMessage( "", "0", (*it), "127.0.0.1", strTitle, strMessage );
							}
						}
						
						delete pQueryUser;
					}
				}
			}
			
			delete pQuery;

			// Save the comments
// 			saveComments( );

			// Your comment has been posted.
			
			HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["comments_page"], string( CSS_COMMENTS ), string( STATS_HTML + strJoinedHTML ), NOT_INDEX, CODE_200 );
			pResponse->strContent += "<p class=\"comments_posted\">" + UTIL_Xsprintf( gmapLANG_CFG["comments_posted"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["comments"] + "\" href=\"" + RESPONSE_STR_STATS_HTML + strJoined + "\">" ).c_str( ), "</a>" ) + "</p>\n";

			// Output common HTML tail
			HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_COMMENTS ) );

			return;
		}
	}
}
