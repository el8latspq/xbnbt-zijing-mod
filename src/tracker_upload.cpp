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
#include "client.h"
#include "config.h"
#include "html.h"
#include "md5.h"
#include "sha1.h"
#include "tracker.h"
#include "util.h"

void CTracker :: serverResponseFileUploadGET( struct request_t *pRequest, struct response_t *pResponse )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
		if( IsIPBanned( pRequest, pResponse, btv, gmapLANG_CFG["upload_page"], string( CSS_UPLOAD ), NOT_INDEX ) )
			return;
		
	if( !pRequest->user.strUID.empty( ) && ( ( pRequest->user.ucAccess & m_ucAccessUploadTorrents ) || ( !m_strOfferDir.empty( ) && ( pRequest->user.ucAccess & m_ucAccessUploadOffers ) ) || ( pRequest->user.ucAccess & m_ucAccessEditOwn ) ) )
	{
		const string cstrImage( pRequest->mapParams["image"] );
		const string cstrArchieve( pRequest->mapParams["archieve"] );
//		const string cstrGUID( pRequest->mapParams["guid"] );
		const string cstrName( pRequest->mapParams["name"] );

		// define doctype
		if( !STR_DOC_TYPE.empty( ) )
			pResponse->strContent += STR_DOC_TYPE;
		else
			UTIL_LogPrint( "CTracker: HTML_Common_Begin: doctype!\n" );

		// open html and head
		pResponse->strContent += "<html lang=\"zh\">\n\n<head>\n";
		
		pResponse->strContent += "<style type=\"" + gmapMime[".css"] + "\" title=\"internal\">\n";
		pResponse->strContent += "<!--\n";
		pResponse->strContent += "body{padding: 0px 0px 0px 0px}\n";
		pResponse->strContent += "body{margin: 0px 0px 0px 0px}\n";
		pResponse->strContent += "body{background-color: #efefef}\n";
		pResponse->strContent += "input{font-family: \"tahoma\", \"arial\", \"helvetica\", \"sans-serif\";}\n";
		pResponse->strContent += "input{font-size:9pt}\n";
		pResponse->strContent += "input{overflow:hidden}\n";
		pResponse->strContent += "span{font-family: \"tahoma\", \"arial\", \"helvetica\", \"sans-serif\";}\n";
		pResponse->strContent += "span{font-size:9pt}\n";
		pResponse->strContent += "-->\n";
		pResponse->strContent += "</style>\n\n";
		
		// Content-Type
		if( !gstrCharSet.empty( ) )
			pResponse->strContent += "<META http-equiv=\"Content-Type\" content=\"" + gmapMime[".html"] + "; charset=" + gstrCharSet + "\">\n";

		// generator
		if( !XBNBT_VER.empty( ) )
			pResponse->strContent += "<META name=\"generator\" content=\"" + XBNBT_VER + "\">\n";

		// Author
		if( !m_strTitle.empty( ) )
			pResponse->strContent += "<META http-equiv=\"Author\" content=\"" + m_strTitle + "\">\n";
		
		// script-type
		pResponse->strContent += "<META http-equiv=\"Content-Script-Type\" content=\"text/javascript\">\n";
		
		// close head and start body
		pResponse->strContent += "</head>\n\n";

		pResponse->strContent += "<body>\n\n";
		if( !cstrImage.empty( ) || !cstrArchieve.empty( ) )
		{
			pResponse->strContent += "<script type=\"text/javascript\">parent.doInsert(\'";
			if( !cstrImage.empty( ) )
				pResponse->strContent += "[img]" + cstrImage + "[/img]";
			else if( !cstrArchieve.empty( ) )
			{
				if( !cstrName.empty( ) )
					pResponse->strContent += "[url=" + cstrArchieve + "]" + cstrName + "[/url]";
				else
					pResponse->strContent += "[url]" + cstrArchieve + "[/url]";
			}
			pResponse->strContent += "\')</script>\n\n";
		}
		pResponse->strContent += "<form name=\"fileupload\" method=\"post\" action=\"http://nvidia.njuftp.org/upload2.aspx\" enctype=\"multipart/form-data\">\n";
		pResponse->strContent += "<input name=\"inputFile\" id=\"inputFile\" type=file size=50>";
//		pResponse->strContent += "<input name=\"name\" id=\"inputName\" type=text size=30>";
		pResponse->strContent += Button_Submit( "submit_fileupload", string( gmapLANG_CFG["upload"] ) );
		pResponse->strContent += "<br><span>" + gmapLANG_CFG["upload_attachment_note"] + "</span>";
		pResponse->strContent += "</form>\n";
		pResponse->strContent += "</body>\n";
	}
}

void CTracker :: serverResponseSubUploadGET( struct request_t *pRequest, struct response_t *pResponse )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
		if( IsIPBanned( pRequest, pResponse, btv, gmapLANG_CFG["upload_page"], string( CSS_UPLOAD ), NOT_INDEX ) )
			return;
		
	if( !pRequest->user.strUID.empty( ) && ( pRequest->user.ucAccess & m_ucAccessUploadOffers ) )
	{
		const string cstrSub( pRequest->mapParams["sub"] );
		const string cstrID( pRequest->mapParams["id"] );
		const string cstrFilename( pRequest->mapParams["filename"] );
		const string cstrName( pRequest->mapParams["name"] );

		// define doctype
		if( !STR_DOC_TYPE.empty( ) )
			pResponse->strContent += STR_DOC_TYPE;
		else
			UTIL_LogPrint( "CTracker: HTML_Common_Begin: doctype!\n" );

		// open html and head
		pResponse->strContent += "<html lang=\"zh\">\n\n<head>\n";
		
		pResponse->strContent += "<style type=\"" + gmapMime[".css"] + "\" title=\"internal\">\n";
		pResponse->strContent += "<!--\n";
		pResponse->strContent += "body{padding: 0px 0px 0px 0px}\n";
		pResponse->strContent += "body{margin: 0px 0px 0px 0px}\n";
		pResponse->strContent += "body{background-color: #efefef}\n";
		pResponse->strContent += "input{font-family: \"tahoma\", \"arial\", \"helvetica\", \"sans-serif\";}\n";
		pResponse->strContent += "input{font-size:9pt}\n";
		pResponse->strContent += "input{overflow:hidden}\n";
		pResponse->strContent += "span{font-family: \"tahoma\", \"arial\", \"helvetica\", \"sans-serif\";}\n";
		pResponse->strContent += "span{font-size:9pt}\n";
		pResponse->strContent += "-->\n";
		pResponse->strContent += "</style>\n\n";
		
		// Content-Type
		if( !gstrCharSet.empty( ) )
			pResponse->strContent += "<META http-equiv=\"Content-Type\" content=\"" + gmapMime[".html"] + "; charset=" + gstrCharSet + "\">\n";

		// generator
		if( !XBNBT_VER.empty( ) )
			pResponse->strContent += "<META name=\"generator\" content=\"" + XBNBT_VER + "\">\n";

		// Author
		if( !m_strTitle.empty( ) )
			pResponse->strContent += "<META http-equiv=\"Author\" content=\"" + m_strTitle + "\">\n";
		
		// script-type
		pResponse->strContent += "<META http-equiv=\"Content-Script-Type\" content=\"text/javascript\">\n";
		
		// close head and start body
		pResponse->strContent += "</head>\n\n";

		pResponse->strContent += "<body>\n\n";
		
		if( !cstrID.empty( ) && !cstrSub.empty( ) && !cstrFilename.empty( ) )
		{
			CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bid,bimdbid from allowed WHERE bid=" + cstrID );

			vector<string> vecQuery;
		
			vecQuery.reserve(2);
			
			vecQuery = pQuery->nextRow( );

			delete pQuery;

			if( vecQuery.size( ) == 2 )
			{
				CMySQLQuery mq01( "INSERT INTO subs (btid,buid,busername,bsub,bfilename,bname,bimdbid,buploadtime) VALUES(" + cstrID + "," + pRequest->user.strUID + ",\'" + UTIL_StringToMySQL( pRequest->user.strLogin ) + "\',\'" + UTIL_StringToMySQL( cstrSub ) + "\',\'" + UTIL_StringToMySQL( cstrFilename ) + "\',\'" + UTIL_StringToMySQL( cstrName ) + "\',\'" + UTIL_StringToMySQL( vecQuery[1] ) + "\',NOW())" );
				CMySQLQuery mq02( "UPDATE allowed SET bsubs=bsubs+1 WHERE bid=" + cstrID );
				m_pCache->setSubs( cstrID, SET_SUBS_ADD );
			}
			
			pResponse->strContent += "<script type=\"text/javascript\">parent.refresh(\'subs\')</script>\n\n";
		}
		
		pResponse->strContent += "<form name=\"subupload\" method=\"post\" action=\"http://nvidia.njuftp.org/upload2.aspx\" enctype=\"multipart/form-data\">\n";
		pResponse->strContent += "<input name=\"id\" id=\"inputID\" type=hidden value=\"" + cstrID + "\">\n";
		pResponse->strContent += "<input name=\"inputFile\" id=\"inputFile\" type=file size=25>\n";
		pResponse->strContent += "<span>" + gmapLANG_CFG["upload_sub_name"] + "</span>";
		pResponse->strContent += "<input name=\"name\" id=\"inputName\" type=text size=25>\n";
		pResponse->strContent += Button_Submit( "submit_subupload", string( gmapLANG_CFG["upload"] ) );
		pResponse->strContent += "<br><span style=\"padding: 0px 0px 0px 0px;\">" + gmapLANG_CFG["upload_sub_note"] + "</span>";
		pResponse->strContent += "</form>\n";
		pResponse->strContent += "</body>\n";
	}
}

void CTracker :: serverResponseUploadGET( struct request_t *pRequest, struct response_t *pResponse )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
		if( IsIPBanned( pRequest, pResponse, btv, gmapLANG_CFG["upload_page"], string( CSS_UPLOAD ), NOT_INDEX ) )
			return;

	// Does the user have upload access?
	if( !pRequest->user.strUID.empty( ) && ( ( pRequest->user.ucAccess & m_ucAccessUploadTorrents ) || ( !m_strOfferDir.empty( ) && ( pRequest->user.ucAccess & m_ucAccessUploadOffers ) ) ) )
	{
		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["upload_page"], string( CSS_UPLOAD ), string( ), NOT_INDEX, CODE_200 );

		// Does the tracker allow uploads?
		if( m_strAllowedDir.empty( ) && m_strOfferDir.empty( ) )
		{
			// This tracker does not allow file uploads.
			pResponse->strContent += "<p class=\"denied\">" + UTIL_Xsprintf( gmapLANG_CFG["upload_nofile_uploads"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_upload"] + "\" href=\"" + RESPONSE_STR_UPLOAD_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";

			// Output common HTML tail
			HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_UPLOAD ) );

			return;
		}

		// Has the tracker had the maximum torrent limit set?
		if( m_uiMaxTorrents != 0 )
		{
			CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bfilename FROM allowed" );
			
			// Has the tracker reached it's maximum torrent file limit?
			if( pQuery->nextRow( ).size( ) == 1 && pQuery->numRows( ) >= m_uiMaxTorrents )
			{
				// This tracker has reached its torrent limit.
				pResponse->strContent += "<p class=\"denied\">" + UTIL_Xsprintf( gmapLANG_CFG["upload_torrent_limit"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_index"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";

				// Output common HTML tail
				HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_UPLOAD ) );
				
				delete pQuery;

				return;
			}
			delete pQuery;
		}
		
		// The Trinity Edition - Modification Begins
		// The following changes have been made:
		// 1. Field Descriptions now appear above the input fields
		// 2. Information regarding Optional input has been moved to a list below
		// 3. Modified the Tag field descriptor to read "Tag/Category"
		// 4. When FORCE_ANNOUNCE_URL is 0/empty, the tracker's Announce URL will be displayed
		//    a CSS class "announce" is also set for this string, which can be used to HIDE the 
		//    "Announce URL" by using the following CSS command:
		//    .announce{display:none}
		// 5. Added a CANCEL button after the UPLOAD button.
		
		// assorted scripts (thanks SA)
		pResponse->strContent += "<script type=\"text/javascript\">\n";
		pResponse->strContent += "<!--\n";
		
		pResponse->strContent += UTIL_JS_Edit_Tool_Bar( "torrentupload.intr" );
		
//		pResponse->strContent += "var myAgent = navigator.userAgent.toLowerCase();\n";
//		pResponse->strContent += "var myVersion = parseInt(navigator.appVersion);\n";
//		pResponse->strContent += "var is_ie = ((myAgent.indexOf(\"msie\") != -1) && (myAgent.indexOf(\"opera\") == -1));\n";
//		pResponse->strContent += "var is_nav = ((myAgent.indexOf(\"mozilla\")!=-1) && (myAgent.indexOf(\"spoofer\")==-1)\n";
//		pResponse->strContent += "&& (myAgent.indexOf(\"compatible\") == -1) && (myAgent.indexOf(\"opera\")==-1)\n";
//		pResponse->strContent += "&& (myAgent.indexOf(\"webtv\") ==-1) && (myAgent.indexOf(\"hotjava\")==-1));\n";
//		
//		pResponse->strContent += "var is_win = ((myAgent.indexOf(\"win\")!=-1) || (myAgent.indexOf(\"16bit\")!=-1));\n";
//		pResponse->strContent += "var is_mac = (myAgent.indexOf(\"mac\")!=-1);\n";
//		
//		pResponse->strContent += "function doInsert(ibTag) {\n";
//		pResponse->strContent += "var isClose = false;\n";
//		pResponse->strContent += "var obj_ta = document.torrentupload.intr;\n";
//		pResponse->strContent += "if ( (myVersion >= 4) && is_ie && is_win) {\n";
//		pResponse->strContent += "  if(obj_ta.isTextEdit) {\n";
//		pResponse->strContent += "    obj_ta.focus();\n";
//		pResponse->strContent += "    var sel = document.selection;\n";
//		pResponse->strContent += "    var rng = sel.createRange();\n";
//		pResponse->strContent += "    rng.collapse;\n";
//		pResponse->strContent += "    if((sel.type == \"Text\" || sel.type == \"None\") && rng != null)\n";
//		pResponse->strContent += "      rng.text = ibTag; }\n";
//		pResponse->strContent += "  else\n";
//		pResponse->strContent += "    obj_ta.value += ibTag; }\n";
//		pResponse->strContent += "else if (obj_ta.selectionStart || obj_ta.selectionStart == '0') {\n";
//		pResponse->strContent += "  var startPos = obj_ta.selectionStart;\n";
//		pResponse->strContent += "  var endPos = obj_ta.selectionEnd;\n";
//		pResponse->strContent += "  obj_ta.value = obj_ta.value.substring(0, startPos) + ibTag + obj_ta.value.substring(endPos, obj_ta.value.length);\n";
//		pResponse->strContent += "  obj_ta.selectionEnd = startPos + ibTag.length; }\n";
//		pResponse->strContent += "else\n";
//		pResponse->strContent += "  obj_ta.value += ibTag;\n";
//		pResponse->strContent += "obj_ta.focus();\n";
//		pResponse->strContent += "return isClose;\n";
//		pResponse->strContent += "}\n\n";
//		
//		pResponse->strContent += "function doInsertSelect(ibTag,ibClsTag) {\n";
//		pResponse->strContent += "var isClose = false;\n";
//		pResponse->strContent += "var obj_ta = document.torrentupload.intr;\n";
//		pResponse->strContent += "if ( (myVersion >= 4) && is_ie && is_win) {\n";
//		pResponse->strContent += "  if(obj_ta.isTextEdit) {\n";
//		pResponse->strContent += "    obj_ta.focus();\n";
//		pResponse->strContent += "    var sel = document.selection;\n";
//		pResponse->strContent += "    var rng = sel.createRange();\n";
//		pResponse->strContent += "    rng.collapse;\n";
//		pResponse->strContent += "    if((sel.type == \"Text\" || sel.type == \"None\") && rng != null)\n";
//		pResponse->strContent += "      var length = rng.text.length;\n";
//		pResponse->strContent += "      rng.text = ibTag + rng.text + ibClsTag;\n";
//		pResponse->strContent += "      rng.moveStart(\"character\", -length - ibClsTag.length);\n";
//		pResponse->strContent += "      rng.moveEnd(\"character\", -ibClsTag.length);\n";
//		pResponse->strContent += "      rng.select(); }\n";
//		pResponse->strContent += "  else\n";
//		pResponse->strContent += "    obj_ta.value += ibTag; }\n";
//		pResponse->strContent += "else if (obj_ta.selectionStart || obj_ta.selectionStart == '0') {\n";
//		pResponse->strContent += "  var startPos = obj_ta.selectionStart;\n";
//		pResponse->strContent += "  var endPos = obj_ta.selectionEnd;\n";
//		pResponse->strContent += "  obj_ta.value = obj_ta.value.substring(0, startPos) + ibTag + obj_ta.value.substring(startPos, endPos) + ibClsTag + obj_ta.value.substring(endPos, obj_ta.value.length);\n";
//		pResponse->strContent += "  obj_ta.selectionStart = startPos + ibTag.length;\n";
//		pResponse->strContent += "  obj_ta.selectionEnd = endPos + ibTag.length; }\n";
//		pResponse->strContent += "else\n";
//		pResponse->strContent += "  obj_ta.value += ibTag + ibClsTag;\n";
//		pResponse->strContent += "obj_ta.focus();\n";
//		pResponse->strContent += "return isClose;\n";
//		pResponse->strContent += "}\n\n";
//		
//		pResponse->strContent += "function insertFont(theval,thetag) {\n";
//		pResponse->strContent += "if (theval == 0) return;\n";
//		pResponse->strContent += "doInsertSelect(\"[\" + thetag + \"=\" + theval + \"]\",\"[/\" + thetag + \"]\");\n";
//		pResponse->strContent += "}\n\n";
//		
//		pResponse->strContent += "function tag_image(PromptImageURL, PromptError) {\n";
//		pResponse->strContent += "var FoundErrors = '';\n";
//		pResponse->strContent += "var enterURL = prompt(PromptImageURL, \"http://\");\n";
//		pResponse->strContent += "if (!enterURL || enterURL==\"http://\") {\n";
//		pResponse->strContent += "  alert(PromptError + \" \" + PromptImageURL);\n";
//		pResponse->strContent += "  return; }\n";
//		pResponse->strContent += "doInsert(\"[img]\"+enterURL+\"[/img]\");\n";
//		pResponse->strContent += "}\n\n";
//		
//		pResponse->strContent += "function tag_url(PromptURL, PromptTitle, PromptError) {\n";
//		pResponse->strContent += "var FoundErrors = '';\n";
//		pResponse->strContent += "var enterURL = prompt(PromptURL, \"http://\");\n";
//		pResponse->strContent += "var enterTITLE = prompt(PromptTitle, \"\");\n";
//		pResponse->strContent += "if (!enterURL || enterURL==\"\" || enterURL==\"http://\") {FoundErrors += \" \" + PromptURL;}\n";
//		pResponse->strContent += "if (FoundErrors) {alert(PromptError+FoundErrors);return;}\n";
//		pResponse->strContent += "if (!enterTITLE || enterTITLE==\"\") {doInsert(\"[url]\"+enterURL+\"[/url]\");return;}\n";
//		pResponse->strContent += "doInsert(\"[url=\"+enterURL+\"]\"+enterTITLE+\"[/url]\");\n";
//		pResponse->strContent += "}\n\n";

		// validate
		pResponse->strContent += "function validate( theform ) {\n";
		pResponse->strContent += "var anchor = document.getElementById( \"uploadname\" );\n";
		pResponse->strContent += "var file = document.getElementById( \"uploadfile\" );\n";
		pResponse->strContent += "var template = document.getElementById( \"template\" + theform.tag.value );\n";
		pResponse->strContent += "if( theform.tag.value == \"000\" ) {\n";
		pResponse->strContent += "  alert( \"" + gmapLANG_CFG["js_choose_a_tag"] + "\" );\n";
		pResponse->strContent += "  return false; }\n";
		pResponse->strContent += "else if( anchor.value == template.innerHTML ) {\n";
		pResponse->strContent += "  alert( \"" + gmapLANG_CFG["js_fill_template"] + "\" );\n";
		pResponse->strContent += "  return false; }\n";
		pResponse->strContent += "else {\n";
		pResponse->strContent += "  var postCheck = document.getElementById( 'id_post' );\n";
		pResponse->strContent += "  var regTorrent = /(.*)\\.torrent$/i;\n";
		pResponse->strContent += "  if( regTorrent.test(file.value) || ( postCheck && postCheck.checked == true ) )\n";
		pResponse->strContent += "    return true;\n";
		pResponse->strContent += "  else {\n";
		pResponse->strContent += "    alert( \"" + gmapLANG_CFG["js_upload_torrent"] + "\" );\n";
		pResponse->strContent += "    return false; }\n";
		pResponse->strContent += "}\n";
		pResponse->strContent += "}\n\n";
		
		pResponse->strContent += "function sample( tag ) {\n";
		pResponse->strContent += "var anchor = document.getElementById( \"uploadname\" );\n";
		pResponse->strContent += "var template = document.getElementById( \"template\" + tag );\n";
		pResponse->strContent += "var sample = document.getElementById( \"sample\" + tag );\n";
		pResponse->strContent += "var samplename = document.getElementById( \"sample\" );\n";
		pResponse->strContent += "  anchor.value = template.innerHTML;\n";
		pResponse->strContent += "  samplename.innerHTML = sample.innerHTML;\n";
		pResponse->strContent += "}\n\n";
		
		// hide
		pResponse->strContent += "function hide( id ) {\n";
		pResponse->strContent += "  var element = document.getElementById( id );\n";
		pResponse->strContent += "  element.style.display=\"none\";\n";
		pResponse->strContent += "}\n";
		
		// display
		pResponse->strContent += "function display( id ) {\n";
		pResponse->strContent += "  var element = document.getElementById( id );\n";
		pResponse->strContent += "  element.style.display=\"\";\n";
		pResponse->strContent += "}\n";
		
		pResponse->strContent += "function checkPost( ) {\n";
		pResponse->strContent += "  var anchor = document.getElementById( 'id_post' );\n";
		pResponse->strContent += "  if( anchor.checked == true ) {\n";
		pResponse->strContent += "    hide('id_torrent');\n";
		pResponse->strContent += "    hide('id_imdb');\n";
		pResponse->strContent += "  }\n";
		pResponse->strContent += "  else {\n";
		pResponse->strContent += "    display('id_torrent');\n";
		pResponse->strContent += "    display('id_imdb');\n";
		pResponse->strContent += "  }\n";
		pResponse->strContent += "}\n";
		
		pResponse->strContent += "//-->\n";
		pResponse->strContent += "</script>\n\n";

		pResponse->strContent += "<div class=\"torrent_upload\">\n";
		if( !( pRequest->user.ucAccess & m_ucAccessUploadTorrents ) )
			pResponse->strContent += "<p class=\"failed\">" + gmapLANG_CFG["offer_upload_offer"] + "</p>";
		pResponse->strContent += "<p class=\"failed\">" + UTIL_Xsprintf( gmapLANG_CFG["upload_rule"].c_str(), string( "<a class=\"failed\" target=\"_blank\" href=\"" + RESPONSE_STR_RULES_HTML + "#rules2\">" ).c_str( ), string( "</a>" ).c_str( ) ) + "</p>";
		pResponse->strContent += "<table class=\"torrent_upload\">\n";
		pResponse->strContent += "<form name=\"torrentupload\" method=\"post\" action=\"" + string( RESPONSE_STR_UPLOAD_HTML ) + "\" onSubmit=\"return validate( this )\" enctype=\"multipart/form-data\">\n";
		if( ( pRequest->user.ucAccess & m_ucAccessUploadTorrents ) && ( pRequest->user.ucAccess & m_ucAccessUploadPosts ) )
		{
			pResponse->strContent += "<tr class=\"torrent_upload\">\n<th class=\"torrent_upload\">" + gmapLANG_CFG["category"] + "</th>\n";
			pResponse->strContent += "<td class=\"torrent_upload\"><input id=\"id_post\" name=\"post\" alt=\"[" + gmapLANG_CFG["top"] + "]\" type=checkbox onclick=\"javascript: checkPost()\">" + gmapLANG_CFG["post"] + "</td>\n</tr>\n";
		}
		pResponse->strContent += "<tr class=\"torrent_upload\" id=\"id_torrent\">\n<th class=\"torrent_upload\">" + gmapLANG_CFG["upload_file"] + "</th>\n"; 
		pResponse->strContent += "<td class=\"torrent_upload\"><input name=\"torrent\" id=\"uploadfile\" alt=\"[" + gmapLANG_CFG["torrent"] + "]\" type=file size=50\"></td>\n</tr>\n";    
		
		// Category Tag and Public/Private option

		// If the torrent categories have been set then enable the pull down selection filled with our categories
		if( m_vecTags.size( ) != 0 && !pRequest->user.strUID.empty( ) )
		{
			string strNameIndex = string( );
			string strTag = string( );
			pResponse->strContent += "<tr class=\"torrent_upload\">\n<th class=\"torrent_upload\">" + gmapLANG_CFG["upload_tag"] + "</th>\n";
			pResponse->strContent += "<td class=\"torrent_upload\"><select id=\"uploadtag\" name=\"tag\" onchange=\"sample(this.options[this.options.selectedIndex].value)\">\n";
			
			pResponse->strContent += "<option value=\"000\">" + gmapLANG_CFG["choose_a_tag"] + "\n";
			
			for( vector< pair< string, string > > :: iterator ulCount = m_vecTags.begin( ); ulCount != m_vecTags.end( ); ulCount++ )
			{
				strNameIndex = (*ulCount).first;
				strTag = (*ulCount).second;
				pResponse->strContent += "<option value=\"" + strNameIndex + "\">" + UTIL_RemoveHTML( strTag ) + "\n";
			}
			pResponse->strContent += "</select>\n" + gmapLANG_CFG["upload_tag_note"] + "</td>\n</tr>\n";
			for( vector< pair< string, string > > :: iterator ulCount = m_vecTags.begin( ); ulCount != m_vecTags.end( ); ulCount++ )
			{
				strNameIndex = (*ulCount).first;
				strTag = (*ulCount).second;
				pResponse->strContent += "<span id=\"template" + strNameIndex + "\" style=\"display: none\">" + gmapLANG_CFG["upload_name_template_"+strNameIndex] + "</span>\n";
				pResponse->strContent += "<span id=\"sample" + strNameIndex + "\" style=\"display: none\">" + gmapLANG_CFG["upload_name_sample_"+strNameIndex] + "</span>\n";
			}				
		}
		
		pResponse->strContent += "<tr class=\"torrent_upload\">\n<th class=\"torrent_upload\">" + gmapLANG_CFG["upload_name"] + "</th>\n";
		pResponse->strContent += "<td class=\"torrent_upload\"><input name=\"name\" id=\"uploadname\" alt=\"[" + gmapLANG_CFG["name"] + "]\" type=text size=96 maxlength=" + CAtomInt( MAX_FILENAME_LEN ).toString( ) + "><br>";
		pResponse->strContent += gmapLANG_CFG["upload_name_sample"] + "<span class=\"blue\" id=\"sample\"></span></td>\n</tr>\n";
		pResponse->strContent += "<tr class=\"torrent_upload\" id=\"id_imdb\">\n<th class=\"torrent_upload\">" + gmapLANG_CFG["imdb"] + "</th>\n";
		pResponse->strContent += "<td class=\"torrent_upload\"><input name=\"imdb\" alt=\"[" + gmapLANG_CFG["imdb"] + "]\" type=text size=12 maxlength=16>" + gmapLANG_CFG["imdb_note"] + "</td>\n</tr>\n";
// 		pResponse->strContent += "<span id=\"template1\" style=\"display: none\">" + gmapLANG_CFG["upload_name_template_1"] + "</span>\n";
// 		pResponse->strContent += "<span id=\"template2\" style=\"display: none\">" + gmapLANG_CFG["upload_name_template_2"] + "</span>\n";
// 		pResponse->strContent += "<span id=\"template3\" style=\"display: none\">" + gmapLANG_CFG["upload_name_template_3"] + "</span>\n";
// 		pResponse->strContent += "<span id=\"sample1\" style=\"display: none\">" + gmapLANG_CFG["upload_name_sample_1"] + "</span>\n";
// 		pResponse->strContent += "<span id=\"sample2\" style=\"display: none\">" + gmapLANG_CFG["upload_name_sample_2"] + "</span>\n";
// 		pResponse->strContent += "<span id=\"sample3\" style=\"display: none\">" + gmapLANG_CFG["upload_name_sample_3"] + "</span>\n";
// 		pResponse->strContent += "<p class=\"torrent_upload\"><label for=\"uploadintrimg\">" + gmapLANG_CFG["intrimg"] + "</label><br>\n";
// 		pResponse->strContent += "<input name=\"intrimg\" id=\"uploadintrimg\" type=text size=96 maxlength=" + CAtomInt( MAX_INFO_LINK_LEN ).toString( ) + "><br></p>\n";
		pResponse->strContent += "<tr class=\"torrent_upload\">\n<th class=\"torrent_upload\">" + gmapLANG_CFG["upload_attachment"] + "</th>\n";
		pResponse->strContent += "<td class=\"torrent_upload\"><iframe src=\"/fileupload.html\" frameborder=\"0\" scrolling=\"no\"></iframe>";
		pResponse->strContent += "</td>\n</tr>\n";
		pResponse->strContent += "<tr class=\"torrent_upload\">\n<th class=\"torrent_upload\">" + gmapLANG_CFG["intr"] + "</th>\n";
		pResponse->strContent += "<td class=\"torrent_upload\">";
		pResponse->strContent += UTIL_Edit_Tool_Bar( );
		
//		pResponse->strContent += "<input style=\"font-weight: bold\" type=\"button\" name=\"B\" value=\"" + gmapLANG_CFG["insert_b"] + "\" onclick=\"javascript: doInsertSelect('[b]', '[/b]')\">\n";
//		pResponse->strContent += "<input style=\"font-style: italic\" type=\"button\" name=\"I\" value=\"" + gmapLANG_CFG["insert_i"] + "\" onclick=\"javascript: doInsertSelect('[i]', '[/i]')\">\n";
//		pResponse->strContent += "<input style=\"text-decoration: underline\" type=\"button\" name=\"U\" value=\"" + gmapLANG_CFG["insert_u"] + "\" onclick=\"javascript: doInsertSelect('[u]', '[/u]')\">\n";
//		pResponse->strContent += "<select name='size' onchange=\"insertSize(this.options[this.selectedIndex].value,'" + gmapLANG_CFG["insert_here"] + "')\">\n";
//		pResponse->strContent += "<option value=\"0\">" + gmapLANG_CFG["insert_fontsize"] + "</option>\n";
//		pResponse->strContent += "<option value=\"1\">1</option>\n";
//		pResponse->strContent += "<option value=\"2\">2</option>\n";
//		pResponse->strContent += "<option value=\"3\">3</option>\n";
//		pResponse->strContent += "<option value=\"4\">4</option>\n";
//		pResponse->strContent += "<option value=\"5\">5</option>\n";
//		pResponse->strContent += "<option value=\"6\">6</option>\n";
//		pResponse->strContent += "<option value=\"7\">7</option>\n";
//		pResponse->strContent += "</select>\n";
//		pResponse->strContent += "<select name=\"color\" onchange=\"insertFont(this.options[this.selectedIndex].value, 'color')\">\n";
//		pResponse->strContent += "<option value=\"0\">" + gmapLANG_CFG["insert_fontcolor"] + "</option>\n";
//		pResponse->strContent += "<option style=\"background-color: black\" value=\"Black\">Black</option>\n";
//		pResponse->strContent += "<option style=\"background-color: sienna\" value=\"Sienna\">Sienna</option>\n";
//		pResponse->strContent += "<option style=\"background-color: darkolivegreen\" value=\"DarkOliveGreen\">Dark Olive Green</option>\n";
//		pResponse->strContent += "<option style=\"background-color: darkgreen\" value=\"DarkGreen\">Dark Green</option>\n";
//		pResponse->strContent += "<option style=\"background-color: darkslateblue\" value=\"DarkSlateBlue\">Dark Slate Blue</option>\n";
//		pResponse->strContent += "<option style=\"background-color: navy\" value=\"Navy\">Navy</option>\n";
//		pResponse->strContent += "<option style=\"background-color: indigo\" value=\"Indigo\">Indigo</option>\n";
//		pResponse->strContent += "<option style=\"background-color: darkslategray\" value=\"DarkSlateGray\">Dark Slate Gray</option>\n";
//		pResponse->strContent += "<option style=\"background-color: darkred\" value=\"DarkRed\">Dark Red</option>\n";
//		pResponse->strContent += "<option style=\"background-color: darkorange\" value=\"DarkOrange\">Dark Orange</option>\n";
//		pResponse->strContent += "<option style=\"background-color: olive\" value=\"Olive\">Olive</option>\n";
//		pResponse->strContent += "<option style=\"background-color: green\" value=\"Green\">Green</option>\n";
//		pResponse->strContent += "<option style=\"background-color: teal\" value=\"Teal\">Teal</option>\n";
//		pResponse->strContent += "<option style=\"background-color: blue\" value=\"Blue\">Blue</option>\n";
//		pResponse->strContent += "<option style=\"background-color: slategray\" value=\"SlateGray\">Slate Gray</option>\n";
//		pResponse->strContent += "<option style=\"background-color: dimgray\" value=\"DimGray\">Dim Gray</option>\n";
//		pResponse->strContent += "<option style=\"background-color: red\" value=\"Red\">Red</option>\n";
//		pResponse->strContent += "<option style=\"background-color: sandybrown\" value=\"SandyBrown\">Sandy Brown</option>\n";
//		pResponse->strContent += "<option style=\"background-color: yellowgreen\" value=\"YellowGreen\">Yellow Green</option>\n";
//		pResponse->strContent += "<option style=\"background-color: seagreen\" value=\"SeaGreen\">Sea Green</option>\n";
//		pResponse->strContent += "<option style=\"background-color: mediumturquoise\" value=\"MediumTurquoise\">Medium Turquoise</option>\n";
//		pResponse->strContent += "<option style=\"background-color: royalblue\" value=\"RoyalBlue\">Royal Blue</option>\n";
//		pResponse->strContent += "<option style=\"background-color: purple\" value=\"Purple\">Purple</option>\n";
//		pResponse->strContent += "<option style=\"background-color: gray\" value=\"Gray\">Gray</option>\n";
//		pResponse->strContent += "<option style=\"background-color: magenta\" value=\"Magenta\">Magenta</option>\n";
//		pResponse->strContent += "<option style=\"background-color: orange\" value=\"Orange\">Orange</option>\n";
//		pResponse->strContent += "<option style=\"background-color: yellow\" value=\"Yellow\">Yellow</option>\n";
//		pResponse->strContent += "<option style=\"background-color: lime\" value=\"Lime\">Lime</option>\n";
//		pResponse->strContent += "<option style=\"background-color: cyan\" value=\"Cyan\">Cyan</option>\n";
//		pResponse->strContent += "<option style=\"background-color: deepskyblue\" value=\"DeepSkyBlue\">Deep Sky Blue</option>\n";
//		pResponse->strContent += "<option style=\"background-color: darkorchid\" value=\"DarkOrchid\">Dark Orchid</option>\n";
//		pResponse->strContent += "<option style=\"background-color: silver\" value=\"Silver\">Silver</option>\n";
//		pResponse->strContent += "<option style=\"background-color: pink\" value=\"Pink\">Pink</option>\n";
//		pResponse->strContent += "<option style=\"background-color: wheat\" value=\"Wheat\">Wheat</option>\n";
//		pResponse->strContent += "<option style=\"background-color: lemonchiffon\" value=\"LemonChiffon\">Lemon Chiffon</option>\n";
//		pResponse->strContent += "<option style=\"background-color: palegreen\" value=\"PaleGreen\">Pale Green</option>\n";
//		pResponse->strContent += "<option style=\"background-color: paleturquoise\" value=\"PaleTurquoise\">Pale Turquoise</option>\n";
//		pResponse->strContent += "<option style=\"background-color: lightblue\" value=\"LightBlue\">Light Blue</option>\n";
//		pResponse->strContent += "<option style=\"background-color: plum\" value=\"Plum\">Plum</option>\n"; 
//		pResponse->strContent += "<option style=\"background-color: white\" value=\"White\">White</option>\n";
//		pResponse->strContent += "</select>\n";
//		pResponse->strContent += "<input type=\"button\" name=\"IMG\" value=\"" + gmapLANG_CFG["insert_img"] + "\" onclick=\"javascript: tag_image('" + gmapLANG_CFG["insert_img_fill"] + "','" + gmapLANG_CFG["insert_error"] + "')\">\n";
//		pResponse->strContent += "<input type=\"button\" name=\"URL\" value=\"" + gmapLANG_CFG["insert_url"] + "\" onclick=\"javascript: tag_url('" + gmapLANG_CFG["insert_url_fill"] + "','" + gmapLANG_CFG["insert_url_title"] + "','" + gmapLANG_CFG["insert_error"] +"')\">\n";
//		pResponse->strContent += "<input type=\"button\" name=\"QUOTE\" value=\"" + gmapLANG_CFG["insert_quote"] + "\" onclick=\"javascript: doInsertSelect('[quote]', '[/quote]')\"><br>";
		pResponse->strContent += "<br>";
		pResponse->strContent += gmapLANG_CFG["upload_note_intr"] + "<br>\n";

		pResponse->strContent += "<textarea id=\"uploadintr\" name=\"intr\" rows=10 cols=96></textarea></td>\n</tr>\n";

		// Upload note list
		pResponse->strContent += "<tr class=\"torrent_upload\">\n<td class=\"torrent_upload\" colspan=\"2\">\n";
		pResponse->strContent += "<ul>\n";
		pResponse->strContent += "<li class=\"torrent_upload\">" + gmapLANG_CFG["upload_note_1"] + "</li>\n";
		pResponse->strContent += "<li class=\"torrent_upload\">" + UTIL_Xsprintf( gmapLANG_CFG["upload_note_2"].c_str( ), CAtomInt( MAX_FILENAME_LEN ).toString( ).c_str( ) ) + "</li>\n";
// 		pResponse->strContent += "<li class=\"torrent_upload\">" + gmapLANG_CFG["upload_note_3"] + "</li>\n";
		pResponse->strContent += "<li class=\"torrent_upload\"><strong>" + gmapLANG_CFG["upload_note_4"] + ":</strong> " + UTIL_BytesToString( guiMaxRecvSize ) + "</li>\n";

		if( !m_strForceAnnounceURL.empty( ) )
			pResponse->strContent += "<li class=\"announce\"><strong>" + gmapLANG_CFG["forced_announce_url"] + ":</strong> " + UTIL_RemoveHTML( m_strForceAnnounceURL ) + "</li>\n";
		else
		{
			pResponse->strContent += "<li class=\"announce\"><strong>" + gmapLANG_CFG["announce_url"] + ":</strong> http://";
			pResponse->strContent += "<script type=\"text/javascript\">document.write( parent.location.host );</script>";
			pResponse->strContent += RESPONSE_STR_ANNOUNCE + "</li>\n";
		}

// 		pResponse->strContent += "<li class=\"torrent_upload\">" + gmapLANG_CFG["no_html"] + "</li>\n";
		pResponse->strContent += "</ul>\n</td>\n</tr>\n";
		// The button list
		pResponse->strContent += "<tr class=\"torrent_upload\">\n<td class=\"torrent_upload_button\" colspan=\"2\">\n";
		pResponse->strContent += Button_Submit( "submit_upload", string( gmapLANG_CFG["upload"] ) );
		pResponse->strContent += Button_Back( "cancel_upload", string( gmapLANG_CFG["cancel"] ) );
		pResponse->strContent += "\n</td>\n</tr>\n";
		// finish
		pResponse->strContent += "</form>\n</table>\n</div>\n";
	
		// Output common HTML ending
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_UPLOAD ) );
	}
	else
	{
		// Not Authorised

		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["upload_page"], string( CSS_UPLOAD ), string( ), NOT_INDEX, CODE_401 );

		// Output common HTML ending
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_UPLOAD ) );
	}
}

void CTracker :: serverResponseUploadPOST( struct request_t *pRequest, struct response_t *pResponse, CAtomList *pPost )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
		if( IsIPBanned( pRequest, pResponse, btv, gmapLANG_CFG["upload_page"], string( CSS_UPLOAD ), NOT_INDEX ) )
			return;

	// Is the user authorised?
	if( pRequest->user.strUID.empty( ) || ( !( pRequest->user.ucAccess & m_ucAccessUploadTorrents ) && ( m_strOfferDir.empty( ) || !( pRequest->user.ucAccess & m_ucAccessUploadOffers ) ) ) )
// 	if( !( pRequest->user.ucAccess & ACCESS_UPLOAD ) )
	{
		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["upload_page"], string( CSS_UPLOAD ), string( ), NOT_INDEX, CODE_401 );

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_UPLOAD ) );

		UTIL_LogPrint( "Upload Denied - User is not authorised to upload (%s:%u:%s)\n", pRequest->user.strLogin.c_str( ), pRequest->user.ucAccess, pRequest->strIP.c_str( ) );
	
		return;
	}
	
	string strUploadDir = string( );
	bool bOffer = false;
	bool bPost = false;
	bool bTorrent = true;
	
	if( pRequest->user.ucAccess & m_ucAccessUploadTorrents )
		strUploadDir = m_strAllowedDir;
	else
	{
		strUploadDir = m_strOfferDir;
		bOffer = true;
	}

	// If there is an not a path set or the path does not exist
	const bool cbDirectoryExists( UTIL_CheckDir( strUploadDir.c_str( ) ) );

	if( strUploadDir.empty( ) || !cbDirectoryExists )
	{
		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["upload_page"], string( CSS_UPLOAD ), string( ), NOT_INDEX, CODE_403 );

		// failed
		pResponse->strContent += "<p class=\"failed\">" + gmapLANG_CFG["failed"] + "</p>\n";
			
		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_UPLOAD ) );

		if( strUploadDir.empty( ) )
			UTIL_LogPrint( "Upload Error - Upload directory is not set\n" );
		else if( !cbDirectoryExists )
			UTIL_LogPrint( "Upload Error - Upload directory does not exist (%s)\n", strUploadDir.c_str( ) );

		return;
	}

	// Has the tracker had the maximum torrent limit set?
	if( m_uiMaxTorrents != 0 )
	{
		CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bfilename FROM allowed" );
			
		// Has the tracker maximum torrent limit been exceeded?
		if( pQuery->nextRow( ).size( ) == 1 && pQuery->numRows( ) >= m_uiMaxTorrents )
		{
			// Output common HTML head
			HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["upload_page"], string( CSS_UPLOAD ), string( ), NOT_INDEX, CODE_403 );

			// failed
			pResponse->strContent += "<p class=\"failed\">" + gmapLANG_CFG["failed"] + "</p>\n";

			// Output common HTML tail
			HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_UPLOAD ) );

			UTIL_LogPrint( "Upload Denied - Maximum torrent limit exceeded (max allowed %u)\n", m_uiMaxTorrents );
			
			delete pQuery;

			return;
		}
		
		delete pQuery;
	}
	
	// Initialise the tag variables
	string strFile = string( );
	string strTorrent = string( );
	string strTag = string( );
	string strIMDbID = string( );
	string strPostedName = string( );
	string strIntr = string( );
	string strName = string( );

	// If we received a post
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
						strName = pName->toString( );

						// Does the content name indicate torrent data?
						if( strName == "torrent" )
						{
							// Get the file path and file name from the disposition
							pFile = ( (CAtomDicti *)pDisposition )->getItem( "filename" );

							// Did we get a file path and file name?
							if( pFile )
							{
								// The file path is local to the peer, we want the file name
								// Strip the unwanted file path off
//								strFile = UTIL_RemoveHTML( UTIL_StripPath( pFile->toString( ) ) );
								strFile = UTIL_StripPath( pFile->toString( ) );
								// Get the torrent contents from the data
								strTorrent = pData->toString( );
							}
							else
								bTorrent = false;
// 							{
// 								// Output common HTML head
// 								HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["upload_page"], string( CSS_UPLOAD ), string( ), NOT_INDEX, CODE_400 );
// 
// 								// failed
// 								pResponse->strContent += "<p class=\"failed\">" + gmapLANG_CFG["failed"] + "</p>\n";
// 								// Signal a bad request
// 								pResponse->strContent += "<p class=\"body_upload\">400 " + gmapLANG_CFG["server_response_400"] + "</p>\n";
// 
// 								// Output common HTML tail
// 								HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_UPLOAD ) );
// 
// 								if( gbDebug )
// 									UTIL_LogPrint( "Upload Warning - Bad request (no file name)\n" );
// 
// 								return;
// 							}
						}
						// Does the content name indicate tag data?
						else if( strName == "tag" )
							// Get the tag data
							strTag = pData->toString( );
						else if( strName == "post" && pData->toString( ) == "on" )
							// Get the tag data
							bPost = true;
						// Does the content name indicate the posted name data?
						else if( strName == "name" )
							// Get the posted name
							strPostedName = pData->toString( ).substr( 0, MAX_FILENAME_LEN );
						else if( strName == "imdb" )
							strIMDbID = pData->toString( );
						else if( strName == "intr" )
							strIntr = pData->toString( );
						
					}
					else
					{
						// Output common HTML head
						HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["upload_page"], string( CSS_UPLOAD ), string( ), NOT_INDEX, CODE_400 );

						// failed
						pResponse->strContent += "<p class=\"failed\">" + gmapLANG_CFG["failed"] + "</p>\n";
						// Signal a bad request
						pResponse->strContent += "<p class=\"body_upload\">400 " + gmapLANG_CFG["server_response_400"] + "</p>\n";

						// Output common HTML tail
						HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_UPLOAD ) );

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
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["upload_page"], string( CSS_UPLOAD ), string( ), NOT_INDEX, CODE_400 );

		// failed
		pResponse->strContent += "<p class=\"failed\">" + gmapLANG_CFG["failed"] + "</p>\n";
		// Signal a bad request
		pResponse->strContent += "<p class=\"body_upload\">400 " + gmapLANG_CFG["server_response_400"] + "</p>\n";

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_UPLOAD ) );

		if( gbDebug )
			UTIL_LogPrint( "Upload Warning - Bad request (no post received)\n" );

		return;
	}
	
	if( !bTorrent && !bPost )
	{
		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["upload_page"], string( CSS_UPLOAD ), string( ), NOT_INDEX, CODE_400 );

		// failed
		pResponse->strContent += "<p class=\"failed\">" + gmapLANG_CFG["failed"] + "</p>\n";
		// Signal a bad request
		pResponse->strContent += "<p class=\"body_upload\">400 " + gmapLANG_CFG["server_response_400"] + "</p>\n";

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_UPLOAD ) );

		if( gbDebug )
			UTIL_LogPrint( "Upload Warning - Bad request (no file name)\n" );

		return;
	}

	// Output common HTML head
	HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["upload_page"], string( CSS_UPLOAD ), string( ), NOT_INDEX, CODE_200 );
	
	if( bPost && ( pRequest->user.ucAccess & m_ucAccessUploadPosts ) )
	{
		unsigned char szMD5[20];
		memset( szMD5, 0, sizeof( szMD5 ) / sizeof( unsigned char ) );
		MD5_CTX md5;

		time_t tNow = time( 0 );
		char pTime[256];
		memset( pTime, 0, sizeof( pTime ) / sizeof( char ) );
		strftime( pTime, sizeof( pTime ) / sizeof( char ), "%Y-%m-%d %H:%M:%S", localtime( &tNow ) );
		const string cstrA1( pRequest->user.strLogin + ":" + gstrRealm + ":" + pTime );

		MD5Init( &md5 );
		MD5Update( &md5, (const unsigned char *)cstrA1.c_str( ), (unsigned int)cstrA1.size( ) );
		MD5Final( szMD5, &md5 );
		
		const string cstrData( (char *)szMD5, sizeof( szMD5 ) / sizeof( unsigned char ) );
		
		CSHA1 hasher;

		hasher.Update( (const unsigned char *)cstrData.c_str( ), (unsigned int)cstrData.size( ) );
		hasher.Final( );

		char szInfoHash[64];
		memset( szInfoHash, 0, sizeof( szInfoHash ) / sizeof( char ) );

		hasher.ReportHash( szInfoHash );

		const string cstrInfoHash( UTIL_StringToHash( szInfoHash ) );
		
		if( !cstrInfoHash.empty( ) )
		{
			// Get the uploaders IP - needs work

			CMySQLQuery *pQuery = new CMySQLQuery( "INSERT INTO allowed (bhash,badded,bnodownload,bpost) VALUES(\'" + UTIL_StringToMySQL( cstrInfoHash ) + "\',NOW(),1,1)" );

			pResponse->strContent += "<p class=\"body_upload\">" + gmapLANG_CFG["upload_ready_now"] + "</p>\n";

			// Add the torrents details to the tag database
			if( !strPostedName.empty( ) )
				addTag( cstrInfoHash, strTag, strPostedName, strIntr, pRequest->user.strLogin, pRequest->user.strUID, pRequest->strIP, string( ), string( ), string( ), string( ), string( ), string( ), false, false );
			else
				addTag( cstrInfoHash, strTag, UTIL_HashToString( cstrInfoHash ), strIntr, pRequest->user.strLogin, pRequest->user.strUID, pRequest->strIP, string( ), string( ), string( ), string( ), string( ), string( ), false, false );
				
			m_pCache->Reset( );

			pResponse->strContent += "<p class=\"success\">" + gmapLANG_CFG["successful"] + "</p>\n";

			// The Trinity Edition - Addition Begins
			// The following displays SEEDING INSTRUCTIONS after a user successfully uploads a torrent

			pResponse->strContent += "<div class=\"seeding_instructions\">\n";
			pResponse->strContent += "<table class=\"seeding_instructions\">\n";
			pResponse->strContent += "<tr>\n<td>\n";
			pResponse->strContent += "<p class=\"seeding_instructions_head\">" + gmapLANG_CFG["upload_seeding_instructions"] + "</p>\n";
			pResponse->strContent += "<p class=\"seeding_instructions\">" + gmapLANG_CFG["upload_inst_verify"] + "<br>\n";
			pResponse->strContent += gmapLANG_CFG["upload_inst_return"];
			pResponse->strContent += "</p>\n</td>\n</tr>\n</table>\n";
			pResponse->strContent += "</div>";

// 			UTIL_LogFilePrint( string( gmapLANG_CFG["user_uploaded_torrent"] + "\n" ).c_str( ), pRequest->user.strLogin.c_str( ), strIP.c_str( ), strFile.c_str( ) );
		}
		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_UPLOAD ) );
		
		return;
	}
	
	// Check the uploaded file for validity

	// Set the path as the local upload directory plus the user uploaded file name 
	
	const string cstrPath( strUploadDir + gmapLANG_CFG["site_name"] + "." + strFile );
	
	string cstrPath2 = string( );
	if( bOffer )
		cstrPath2 = m_strAllowedDir + gmapLANG_CFG["site_name"] + "." + strFile;
	else
		cstrPath2 = m_strOfferDir + gmapLANG_CFG["site_name"] + "." + strFile;

	// Get the file extension of the user uploaded file
	const string strExt( getFileExt( strFile ) );
	
	// Does the user uploaded file have a .torrent file extension?
	if( strExt != ".torrent" )
	{
		// failed
		pResponse->strContent += "<p class=\"failed\">" + gmapLANG_CFG["failed"] + "</p>\n";
		// The uploaded file is not a .torrent file.
		pResponse->strContent += "<p class=\"body_upload\">" + UTIL_Xsprintf( gmapLANG_CFG["upload_not_torrent"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_upload"] + "\" href=\"" + RESPONSE_STR_UPLOAD_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";
	}
	// Did we receive any data?
	else if( strTorrent.empty( ) )
	{
		// failed
		pResponse->strContent += "<p class=\"failed\">" + gmapLANG_CFG["failed"] + "</p>\n";
		// The uploaded file is corrupt or invalid.
		pResponse->strContent += "<p class=\"body_upload\">" + UTIL_Xsprintf( gmapLANG_CFG["upload_file_corrupt"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_upload"] + "\" href=\"" + RESPONSE_STR_UPLOAD_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";
	}
	// Has a category tag been set?
	else if( !checkTag( strTag ) )
	{
		// failed
		pResponse->strContent += "<p class=\"failed\">" + gmapLANG_CFG["failed"] + "</p>\n";
		// The file tag is invalid.
		pResponse->strContent += "<p class=\"body_upload\">" + UTIL_Xsprintf( gmapLANG_CFG["upload_filetag_invalid"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_upload"] + "\" href=\"" + RESPONSE_STR_UPLOAD_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";
	}
	// Does the file already exist on the file system?
	else if( UTIL_CheckFile( cstrPath.c_str( ) ) || UTIL_CheckFile( cstrPath2.c_str( ) ) )
	{
		// failed
		pResponse->strContent += "<p class=\"failed\">" + gmapLANG_CFG["failed"] + "</p>\n";
		//The uploaded file already exists.
		pResponse->strContent += "<p class=\"body_upload\">" + UTIL_Xsprintf( gmapLANG_CFG["upload_file_exists"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_upload"] + "\" href=\"" + RESPONSE_STR_UPLOAD_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";
	}
	else
	{
		// Decode the torrent
		CAtom *pTorrent = Decode( strTorrent );
		// Did we decode a dictionary?
		if( pTorrent && pTorrent->isDicti( ) )
		{
			if( !pRequest->user.strUID.empty( ) )
			{
				CAtom *pInfo = ( (CAtomDicti *)pTorrent )->getItem( "info" );
				const string strSource = gmapLANG_CFG["source"];
				if( !strSource.empty( ) )
				{	
					( (CAtomDicti *)pInfo )->setItem( "source" , new CAtomString( strSource ) );
				}
//				if( CFG_GetInt( "bnbt_allow_magnet_downloads", 1 ) )
//				{
//					( (CAtomDicti *)pInfo )->setItem( "private" , new CAtomInt( 0 ) );
//				}
//				else
//				{
					( (CAtomDicti *)pInfo )->setItem( "private" , new CAtomInt( 1 ) );
//				}
			}
			
			// Get the info hash from the dictionary
			const string cstrInfoHash( UTIL_InfoHash( pTorrent ) );

			// Is their an info hash?
			if( !cstrInfoHash.empty( ) )
			{
				CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bfilename FROM allowed WHERE bhash=\'" + UTIL_StringToMySQL( cstrInfoHash ) + "\'" );
			
				vector<string> vecQuery;
			
				vecQuery.reserve(1);
				
				vecQuery = pQuery->nextRow( );
				
				CMySQLQuery *pQueryOffer = new CMySQLQuery( "SELECT bfilename FROM offer WHERE bhash=\'" + UTIL_StringToMySQL( cstrInfoHash ) + "\'" );
			
				vector<string> vecQueryOffer;
			
				vecQueryOffer.reserve(1);
				
				vecQueryOffer = pQueryOffer->nextRow( );
				// Does this torrent info hash already exist amoung the torrent files that were parsed?

				if( vecQuery.size( ) == 1 || ( !( pRequest->user.ucAccess & m_ucAccessUploadTorrents ) && vecQueryOffer.size( ) == 1 ) )
				{
					// failed
					pResponse->strContent += "<p class=\"failed\">" + gmapLANG_CFG["failed"] + "</p>\n";
					// A file with the uploaded file's info hash already exists.
					pResponse->strContent += "<p class=\"body_upload\">" + UTIL_Xsprintf( gmapLANG_CFG["upload_hash_exists"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_upload"] + "\" href=\"" + RESPONSE_STR_UPLOAD_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";
				}
				else
				{
					// Do we force the announce address and has the address has been set?
					if( m_bForceAnnounceOnUL && !m_strForceAnnounceURL.empty( ) )
					{
						// Set the torrent's announce url 
											
						( (CAtomDicti *)pTorrent )->setItem( "announce", new CAtomString( m_strForceAnnounceURL ) );

						// Do we force the announce list?
						if( m_bEnableAnnounceList )
						{
							// Get the first announce list address
							unsigned char ucAnnounceList = 1;
							string strKey = "xbnbt_announce_list" + CAtomInt( ucAnnounceList ).toString( );
							string strAnnounceUrl = CFG_GetString( strKey, string( ) );

							// Did we get an announce list address?
							if( !strAnnounceUrl.empty( ) )
							{
								// Initialiste the announce list
								CAtomList *pAnnounceList = new CAtomList( );
								// Set the first announce list url as the force announce address url
								CAtomList *pAnnounceListItem = new CAtomList( );
								pAnnounceListItem->addItem( new CAtomString( m_strForceAnnounceURL ) );
								pAnnounceList->addItem( pAnnounceListItem );

								// We got an announce list url so add it to the announce list and get another
								while( !strAnnounceUrl.empty( ) )
								{
									CAtomList *pAnnounceListItem1 = new CAtomList( );
									pAnnounceListItem1->addItem( new CAtomString( strAnnounceUrl ) );
									pAnnounceList->addItem( pAnnounceListItem1 );
									strKey = "xbnbt_announce_list" + CAtomInt( ++ucAnnounceList ).toString( );
									strAnnounceUrl = CFG_GetString( strKey, string( ) );
								}

								// Set the torrents announce list
								( (CAtomDicti *)pTorrent )->setItem( "announce-list", new CAtomList( *pAnnounceList ) );

								// Free the memory
								delete pAnnounceList;
							}
						}
					}

					// Write the torrent to disk
					const string cstrEncodedTorrent( Encode( pTorrent ) );

					if( !cstrEncodedTorrent.empty( ) )
					{
						if( !cstrPath.empty( ) )
							UTIL_MakeFile( cstrPath.c_str( ), cstrEncodedTorrent );

					}
					else
					{
						pResponse->strContent += "<p class=\"failed\">" + gmapLANG_CFG["failed"] + "</p>\n";

						UTIL_LogPrint( "Upload - cstrEncodedTorrent is empty!\n" );
					}

						
//					string strDefaultRule = CFG_GetString( "bnbt_free_rule_" + strTag + "_default", string( ) );
//					string strRule = CFG_GetString( "bnbt_free_rule_" + strTag, string( ) );
					
					string strUploadedID = string( );

					strUploadedID = parseTorrent( cstrPath.c_str( ) );

					if( !strUploadedID.empty( ) )
					{
						// Add the torrents details to the tag database
						if( strPostedName.empty( ) )
						{
							CAtom *pInfo = ( (CAtomDicti *)pTorrent )->getItem( "info" );
							CAtom *pName = ( (CAtomDicti *)pInfo )->getItem( "name" );
							if( pName && !pName->toString( ).empty( ) )
								strPostedName = pName->toString( );
							else
								strPostedName = strFile;
						}
						
						string strDatabase = string( );
					
						if( !bOffer )
							strDatabase = "allowed";
						else
							strDatabase = "offer";
							
						CMySQLQuery *pQueryUploaded = new CMySQLQuery( "SELECT badded,bsize FROM " + strDatabase + " WHERE bid=" + strUploadedID );
						
						vector<string> vecQueryUploaded;

						vecQueryUploaded.reserve(2);
					
						vecQueryUploaded = pQueryUploaded->nextRow( );
					
						delete pQueryUploaded;
						
						string strFreeSize = CFG_GetString( "bnbt_free_rule_size_default", "0" );
						string strDefaultDown = CFG_GetString( "bnbt_free_rule_down_default", "100" );
						string strDefaultUp = CFG_GetString( "bnbt_free_rule_up_default", "100" );
						string strFreeDown = CFG_GetString( "bnbt_free_rule_down_default", "100" );
						string strFreeUp = CFG_GetString( "bnbt_free_rule_up_default", "100" );
						string strFreeTime = CFG_GetString( "bnbt_free_rule_time_default", "0" );
						
						if( !bOffer )
						{
							int64 iSize = 0;
						
							if( vecQueryUploaded.size( ) == 2 && !vecQueryUploaded[1].empty( ) )
								iSize = UTIL_StringTo64( vecQueryUploaded[1].c_str( ) );
						
							unsigned char ucRule = 1;
							string strRule = CFG_GetString( "bnbt_free_rule_" + CAtomInt( ucRule ).toString( ), string( ) );
							vector<string> vecRule;
							vecRule.reserve(9);
						
							vector<string> vecTags;
							vecTags.reserve(64);
							vector<string> vecKeyword;
							vecKeyword.reserve(64);
					
							vecRule = UTIL_SplitToVectorStrict( strRule, "|" );
			
							while( !vecRule.empty( ) && vecRule.size( ) == 9 )
							{
								vecTags = UTIL_SplitToVector( vecRule[0], " " );
								if( UTIL_MatchVector( strTag, vecTags, MATCH_METHOD_NONCASE_OR ) )
								{
									vecKeyword = UTIL_SplitToVector( vecRule[1], " " );
									if( UTIL_MatchVector( strPostedName, vecKeyword, MATCH_METHOD_NONCASE_OR ) )
									{
										vecKeyword = UTIL_SplitToVector( vecRule[2], " " );
										if( UTIL_MatchVector( strPostedName, vecKeyword, MATCH_METHOD_NONCASE_AND ) )
										{
											int64 iFreeSize = 0;
											if( !vecRule[3].empty( ) )
												iFreeSize = UTIL_StringTo64( vecRule[3].c_str( ) ) * 1024 * 1024 * 1024;
											if( iSize > iFreeSize )
											{
												strDefaultDown = vecRule[4];
												strDefaultUp = vecRule[5];
												strFreeDown = vecRule[6];
												strFreeUp = vecRule[7];
												strFreeTime = vecRule[8];
											}
										}
									}
								}
								strRule = CFG_GetString( "bnbt_free_rule_" + CAtomInt( ++ucRule ).toString( ), string( ) );
								vecRule = UTIL_SplitToVectorStrict( strRule, "|" );
							}
						}
						
						modifyTag( strUploadedID, strTag, strPostedName, strIntr, pRequest->user.strLogin, pRequest->user.strUID, pRequest->strIP, strDefaultDown, strDefaultUp, strFreeDown, strFreeUp, strFreeTime, string( ), false, bOffer );
					
						string strIMDb = string( );
					
						if( strIMDbID.find( "tt" ) != 0 || strIMDbID.find_first_not_of( "0123456789t" )  != string :: npos )
							strIMDbID.erase( );

						if( !strIMDbID.empty( ) )
						{
							CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bimdb,bimdbid,bimdbupdated from allowed WHERE bimdbid=\'" + UTIL_StringToMySQL( strIMDbID ) + "\' AND bimdbupdated>NOW()-interval 3 day GROUP BY bimdbid UNION SELECT bimdb,bimdbid,bimdbupdated from offer WHERE bimdbid=\'" + UTIL_StringToMySQL( strIMDbID ) + "\' AND bimdbupdated>NOW()-interval 3 day GROUP BY bimdbid" );
		
							vector<string> vecQuery;

							vecQuery.reserve(3);

							vecQuery = pQuery->nextRow( );
						
							delete pQuery;
						
							if( vecQuery.size( ) == 3 )
							{
								strIMDb = vecQuery[0];
								CMySQLQuery mq01( "UPDATE " + strDatabase + " SET bimdb=\'" + UTIL_StringToMySQL( strIMDb ) + "\',bimdbid=\'" + UTIL_StringToMySQL( strIMDbID ) + "\',bimdbupdated=\'" + vecQuery[2] + "\' WHERE bid=" + strUploadedID );
							}
							else
							{
								CMySQLQuery mq01( "UPDATE " + strDatabase + " SET bimdbid=\'" + UTIL_StringToMySQL( strIMDbID ) + "\' WHERE bid=" + strUploadedID );
								system( string( "./getimdbone.py \"" + strIMDbID + "\" &" ).c_str( ) );
							}
	// 							strIMDb = GetIMDb( strIMDbID );
						}
						else
							CMySQLQuery mq01( "UPDATE " + strDatabase + " SET bimdb=\'" + UTIL_StringToMySQL( strIMDb ) + "\',bimdbid=\'" + UTIL_StringToMySQL( strIMDbID ) + "\',bimdbupdated=NOW() WHERE bid=" + strUploadedID );
							
						if( vecQueryUploaded.size( ) == 2 && !vecQueryUploaded[0].empty( ) )
							m_pCache->setLatest( vecQueryUploaded[0], bOffer );
						m_pCache->Reset( bOffer );
						
						if( !bOffer )
						{
							CMySQLQuery *pQueryFriend = new CMySQLQuery( "SELECT buid FROM friends WHERE bfriendid=" + pRequest->user.strUID );
			
							vector<string> vecQueryFriend;
	
							vecQueryFriend.reserve(1);

							vecQueryFriend = pQueryFriend->nextRow( );
				
							while( vecQueryFriend.size( ) == 1 )
							{
								if( !vecQueryFriend[0].empty( ) )
								{
									CMySQLQuery mq01( "INSERT INTO talktorrent (buid,bfriendid,btid,bposted) VALUES(" + vecQueryFriend[0] + "," + pRequest->user.strUID + "," + strUploadedID + ",'" + UTIL_StringToMySQL( vecQueryUploaded[0] ) + "')" );
									CMySQLQuery mq02( "UPDATE users SET btalktorrent=btalktorrent+1 WHERE buid=" + vecQueryFriend[0] );
								}
						
								vecQueryFriend = pQueryFriend->nextRow( );
							}
		
							delete pQueryFriend;
						}
						
						if( bOffer )
						{
							pResponse->strContent += "<p class=\"body_upload\">" + gmapLANG_CFG["offer_ready_now"] + "</p>\n";
							pResponse->strContent += "<p class=\"success\">" + gmapLANG_CFG["successful_offer"] + "</p>\n";
						}
						else
						{
							pResponse->strContent += "<p class=\"body_upload\">" + gmapLANG_CFG["upload_ready_now"] + "</p>\n";
							pResponse->strContent += "<p class=\"success\">" + gmapLANG_CFG["successful"] + "</p>\n";
						}

						// The Trinity Edition - Modification Begins
						// The following removes the multiple RTT links that appear based on parsing method used

					//	if( m_bParseOnUpload )
					//	{

					//		parseTorrent( cstrPath.c_str( ) );
						
					//		if( bOffer )
					//			pResponse->strContent += "<p class=\"body_upload\">" + gmapLANG_CFG["offer_ready_now"] + "</p>\n";
					//		else
					//			pResponse->strContent += "<p class=\"body_upload\">" + gmapLANG_CFG["upload_ready_now"] + "</p>\n";
					//	}
					//	else
					//	{
							// The uploaded file will be ready in
					//		if( m_uiParseAllowedInterval == 1 )
					//			pResponse->strContent += "<p class=\"body_upload\">" + UTIL_Xsprintf( gmapLANG_CFG["upload_ready_1min"].c_str( ), CAtomInt( m_uiParseAllowedInterval ).toString( ).c_str( ) ) + "</p>\n";
					//		else
					//			pResponse->strContent += "<p class=\"body_upload\">" + UTIL_Xsprintf( gmapLANG_CFG["upload_ready_xmins"].c_str( ), CAtomInt(  m_uiParseAllowedInterval ).toString( ).c_str( ) ) + "</p>\n";
					//	}

						// The Trinity Edition - Addition Begins
						// The following displays SEEDING INSTRUCTIONS after a user successfully uploads a torrent
					
						pResponse->strContent += "<div class=\"seeding_instructions\">\n";
						pResponse->strContent += "<table class=\"seeding_instructions\">\n";
						pResponse->strContent += "<tr>\n<td>\n";
						pResponse->strContent += "<p class=\"seeding_instructions_head\">" + gmapLANG_CFG["upload_seeding_instructions"] + "</p>\n";
						pResponse->strContent += "<p class=\"seeding_instructions_lead\">" + gmapLANG_CFG["upload_begin_seeding"] + "</p>\n";
						pResponse->strContent += "<ol>\n";
	// 					pResponse->strContent += "<li class=\"seeding_instructions\">" + gmapLANG_CFG["upload_inst_1"] + "</li>\n";

						pResponse->strContent += "<li class=\"seeding_instructions\">" + gmapLANG_CFG["upload_inst_1"] + "<br>\n";
						pResponse->strContent += "<a class=\"hot\" href=\"";
						if( bOffer )
							pResponse->strContent += RESPONSE_STR_OFFERS;
						else
							pResponse->strContent += RESPONSE_STR_TORRENTS;
						pResponse->strContent += strUploadedID + ".torrent\">" + gmapLANG_CFG["stats_download_torrent"] + "</a></li>\n";

						pResponse->strContent += "<li class=\"seeding_instructions\">" + gmapLANG_CFG["upload_inst_2"] + "</li>\n";
						pResponse->strContent += "<li class=\"seeding_instructions\">" + gmapLANG_CFG["upload_inst_3"] + "</li>\n";
						pResponse->strContent += "<li class=\"seeding_instructions\">" + gmapLANG_CFG["upload_inst_4"] + "</li>\n";
						pResponse->strContent += "</ol>\n";
						if( bOffer )
						{
							pResponse->strContent += "<p class=\"seeding_instructions\">" + gmapLANG_CFG["upload_inst_verify_offer"] + "<br>\n";
							pResponse->strContent += gmapLANG_CFG["upload_inst_return_offer"];
						}
						else
						{
							pResponse->strContent += "<p class=\"seeding_instructions\">" + gmapLANG_CFG["upload_inst_verify"] + "<br>\n";
							pResponse->strContent += gmapLANG_CFG["upload_inst_return"];
						}
						pResponse->strContent += "</p>\n</td>\n</tr>\n</table>\n";
						pResponse->strContent += "</div>";

						// Instructions note
	// 					pResponse->strContent += "<table class=\"seeding_instructions_note\">\n<tr>\n<td style=\"font-weight:normal; border:1px solid black; padding:10px\">\n";
	// 					pResponse->strContent += "<table class=\"seeding_instructions_note\">\n<tr>\n<td>\n";
	// 					pResponse->strContent += "<p class=\"seeding_instructions\">&dagger; " + gmapLANG_CFG["upload_note_1"] + "<br>\n";
	// 					pResponse->strContent += UTIL_Xsprintf( gmapLANG_CFG["upload_note_2"].c_str( ), CAtomInt( MAX_FILENAME_LEN ).toString( ).c_str( ) ) + "<br>\n";
	// 					pResponse->strContent += gmapLANG_CFG["upload_note_3"];
	// 					pResponse->strContent += "</p>\n</td>\n</tr>\n</table>\n</div>\n";
						// Who uploaded the torrent?

						UTIL_LogFilePrint( string( gmapLANG_CFG["user_uploaded_torrent"] + "\n" ).c_str( ), pRequest->user.strLogin.c_str( ), pRequest->strIP.c_str( ), strFile.c_str( ) );
					}
					else
					{
						// failed
						pResponse->strContent += "<p class=\"failed\">" + gmapLANG_CFG["failed"] + "</p>\n";
					}
				}
				
				delete pQuery;
				delete pQueryOffer;
			}
			else
			{
				// failed
				pResponse->strContent += "<p class=\"failed\">" + gmapLANG_CFG["failed"] + "</p>\n";
				// The uploaded file is corrupt or invalid.
				pResponse->strContent += "<p class=\"body_upload\">" + UTIL_Xsprintf( gmapLANG_CFG["upload_file_corrupt"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_upload"] + "\" href=\"" + RESPONSE_STR_UPLOAD_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";
			}
		}
		else
		{
			// failed 
			pResponse->strContent += "<p class=\"failed\">" + gmapLANG_CFG["failed"] + "</p>\n";
			// The uploaded file is corrupt or invalid.
			pResponse->strContent += "<p class=\"body_upload\">" + UTIL_Xsprintf( gmapLANG_CFG["upload_file_corrupt"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_upload"] + "\" href=\"" + RESPONSE_STR_UPLOAD_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";
		}

		// Free the memory
		if( pTorrent )
			delete pTorrent;
	}
 
	// Output common HTML tail
	HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_UPLOAD ) );
}
