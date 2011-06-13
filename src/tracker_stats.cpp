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

#include <fcntl.h>

#include "bnbt.h"
#include "bnbt_mysql.h"
#include "atom.h"
#include "bencode.h"
#include "config.h"
#include "html.h"
#include "sort.h"
#include "tracker.h"
#include "util.h"

// Convert a HEX char to a DEC char, return -1 if error
static inline const char UTIL_HexToDec( const unsigned char &cucHex )
{
	switch( cucHex )
	{
	case '0':
		return 0;
	case '1':
		return 1;
	case '2':
		return 2;
	case '3':
		return 3;
	case '4':
		return 4;
	case '5':
		return 5;
	case '6':
		return 6;
	case '7':
		return 7;
	case '8':
		return 8;
	case '9':
		return 9;
	case 'A':
		return 10;
	case 'B':
		return 11;
	case 'C':
		return 12;
	case 'D':
		return 13;
	case 'E':
		return 14;
	case 'F':
		return 15;
	default:
		return -1;
	}
}

inline void UTIL_GetClientIdentity( const string &cstrPeerID, const string &cstrUserAgent, string &strClientType, bool &bClientTypeIdentified )
{
	// Structure to hold the client types
	struct clienttype_t
	{
		char cPeerIDChar[10];
		unsigned char ucVersionMethod;
		string strUserAgent;
		string strClientType;
		string strVersionMask;
	};

	// Initialise the structure with the known client masks
	const clienttype_t KnownClientMask[9] =
	{
		{ '-', 'A', 'Z',  -1,  -1,  -1,  -1, '-',  -1,  -1, VERSION_METHOD_1, "Azureus ", "Azureus", "%c.%c.%c.%c" },
		{ '-', 'U', 'T',  -1,  -1,  -1,  -1, '-',  -1,  -1, VERSION_METHOD_1, "uTorrent", "uTorrent", "%c.%c.%c.%c" },
		{ '-', 'U', 'M',  -1,  -1,  -1,  -1, '-',  -1,  -1, VERSION_METHOD_1, "uTorrentMac", "uTorrent Mac", "%c.%c.%c.%c" },
		{ '-', 'T', 'R',  -1,  -1,  -1,  -1, '-',  -1,  -1, VERSION_METHOD_1, "Transmission", "Transmission", "%c.%c.%c.%c" },
		{ '-', 'K', 'T',  -1,  -1,  -1,  -1, '-',  -1,  -1, VERSION_METHOD_1, "KTorrent/", "KTorrent", "%c.%c.%c.%c" },
		{ '-', 'D', 'E',  -1,  -1,  -1,  -1, '-',  -1,  -1, VERSION_METHOD_1, "Deluge", "Deluge", "%c.%c.%c.%c" },
		{ '-', 'L', 'T',  -1,  -1,  -1,  -1, '-',  -1,  -1, VERSION_METHOD_1, "", "libtorrent", "%c.%c.%c.%c" },
		{ '-', 'l', 't',  -1,  -1,  -1,  -1, '-',  -1,  -1, VERSION_METHOD_8, "rtorrent", "", "" },
		{   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, VERSION_METHOD_8, "", "Not Identifiable", "" },
	};

	// Signal a match
	bool bMatch = false;

	// Loop through the client masks
	for( char cKnownClientMaskCount = 0; cKnownClientMaskCount < (char)( sizeof( KnownClientMask ) / sizeof(clienttype_t) ); cKnownClientMaskCount++ )
	{
		//  If the user-agent matches the mask
		if( KnownClientMask[cKnownClientMaskCount].strUserAgent == cstrUserAgent.substr( 0, KnownClientMask[cKnownClientMaskCount].strUserAgent.size( ) ) )
		{
			bMatch = true;

			// Loop through the first 10 characters of the peer id
			for( char cLoopPeerIDChar = 0; cLoopPeerIDChar < (char)( sizeof(KnownClientMask->cPeerIDChar ) / sizeof(char) ); cLoopPeerIDChar++ )
			{
				// If this character is not a control character
				if( KnownClientMask[cKnownClientMaskCount].cPeerIDChar[cLoopPeerIDChar] != -1 )
				{
					// If the character does not match the mask
					if( KnownClientMask[cKnownClientMaskCount].cPeerIDChar[cLoopPeerIDChar] != cstrPeerID[cLoopPeerIDChar] )
					{
						bMatch = false;

						break;
					}
				}
			}
		}

		// If we matched then stop checking
		if( bMatch )
		{
			strClientType = KnownClientMask[cKnownClientMaskCount].strClientType + " ";

			switch( KnownClientMask[cKnownClientMaskCount].ucVersionMethod )
			{
			case VERSION_METHOD_1:
				strClientType += UTIL_Xsprintf( KnownClientMask[cKnownClientMaskCount].strVersionMask.c_str( ), cstrPeerID[3], cstrPeerID[4], cstrPeerID[5], cstrPeerID[6] );

				break;
			case VERSION_METHOD_2:
				strClientType += UTIL_Xsprintf( KnownClientMask[cKnownClientMaskCount].strVersionMask.c_str( ), cstrPeerID[1], cstrPeerID[2], cstrPeerID[3] );

				break;
			case VERSION_METHOD_3:
				strClientType += UTIL_Xsprintf( KnownClientMask[cKnownClientMaskCount].strVersionMask.c_str( ), cstrPeerID[3], UTIL_HexToDec( cstrPeerID[4] ), cstrPeerID[5] );
//				strClientType += UTIL_Xsprintf( KnownClientMask[cKnownClientMaskCount].strVersionMask.c_str( ), cstrPeerID[1], cstrPeerID[2], UTIL_HexToDec( cstrPeerID[3] ) );

				break;
			case VERSION_METHOD_4:
				strClientType += UTIL_Xsprintf( KnownClientMask[cKnownClientMaskCount].strVersionMask.c_str( ), cstrPeerID[5] );

				break;
			case VERSION_METHOD_5:
				strClientType += UTIL_Xsprintf( KnownClientMask[cKnownClientMaskCount].strVersionMask.c_str( ), cstrPeerID[1], cstrPeerID[3], cstrPeerID[5] );

				break;
			case VERSION_METHOD_6:
				strClientType += UTIL_Xsprintf( KnownClientMask[cKnownClientMaskCount].strVersionMask.c_str( ), cstrPeerID[1], cstrPeerID[2], cstrPeerID[3], cstrPeerID[4] );

				break;
			case VERSION_METHOD_7:
				strClientType += UTIL_Xsprintf( KnownClientMask[cKnownClientMaskCount].strVersionMask.c_str( ), cstrPeerID[3], cstrPeerID[4], cstrPeerID[5] );

				break;
			case VERSION_METHOD_8:
				strClientType += cstrUserAgent;

				break;
			case VERSION_METHOD_9:
				strClientType +=  UTIL_Xsprintf( KnownClientMask[cKnownClientMaskCount].strVersionMask.c_str( ), cstrPeerID[5], cstrPeerID[7], cstrPeerID[9] );

				break;
			case VERSION_METHOD_10:
				strClientType +=  UTIL_Xsprintf( KnownClientMask[cKnownClientMaskCount].strVersionMask.c_str( ), cstrPeerID[0] / 10, cstrPeerID[0] % 10 );

				break;
			case VERSION_METHOD_11:
				strClientType += UTIL_Xsprintf( KnownClientMask[cKnownClientMaskCount].strVersionMask.c_str( ), cstrPeerID[2], cstrPeerID[3], cstrPeerID[4], cstrPeerID[5] );

				break;
			case VERSION_METHOD_12:
				strClientType += UTIL_Xsprintf( KnownClientMask[cKnownClientMaskCount].strVersionMask.c_str( ), cstrPeerID[4], cstrPeerID[5], cstrPeerID[6] );

				break;
			default:
				strClientType = "Unknown";
			}

			break;
		}
	}

	// Signal that the peer has had it's client type identified
	bClientTypeIdentified = true;
}

void CTracker :: serverResponseStatsGET( struct request_t *pRequest, struct response_t *pResponse )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
		if( IsIPBanned( pRequest, pResponse, btv, gmapLANG_CFG["stats_page"], string( CSS_STATS ), NOT_INDEX ) )
			return;

	if( !m_bShowStats )
	{
		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["stats_page"], string( CSS_STATS ), string( ), NOT_INDEX, CODE_403 );

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_STATS ) );

		return;
	}
	
	if( pRequest->user.ucAccess & m_ucAccessReq )
	{
		if( pRequest->mapParams.find( "req" ) != pRequest->mapParams.end( ) )
		{
			string strReqID( pRequest->mapParams["req"] );
			
			if( strReqID.find( " " ) != string :: npos )
				strReqID.erase( );
			
			CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bid,btitle,buploaderid,breq FROM allowed WHERE bid=" + strReqID );
		
			vector<string> vecQuery;
		
			vecQuery.reserve(4);

			vecQuery = pQuery->nextRow( );
			
			delete pQuery;
			
			if( vecQuery.size( ) == 4 && !vecQuery[0].empty( ) )
			{
				if( vecQuery[3] == "0" )
				{
					m_pCache->setStatus( strReqID, SET_STATUS_REQ );
					
					CMySQLQuery mq01( "UPDATE allowed SET breq=1 WHERE bid=" + strReqID );

					string cstrName = vecQuery[1];
					string cstrUploaderID = vecQuery[2];

					CMySQLQuery *pQueryUsers = new CMySQLQuery( "SELECT buid FROM users WHERE buid=" + cstrUploaderID );

					vector<string> vecQueryUsers;
				
					vecQueryUsers.reserve(1);

					vecQueryUsers = pQueryUsers->nextRow( );
					
					delete pQueryUsers;

					if( vecQueryUsers.size( ) == 1 && !vecQueryUsers[0].empty( ) )
					{
						if( !pRequest->user.strUID.empty( ) )
						{
							string strTitle = gmapLANG_CFG["section_reqseeders"];
							string strMessage = UTIL_Xsprintf( gmapLANG_CFG["req_seeders_message"].c_str( ), pRequest->user.strLogin.c_str( ), strReqID.c_str( ), cstrName.c_str( ) );
							
							sendMessage( pRequest->user.strLogin, pRequest->user.strUID, cstrUploaderID, pRequest->strIP, strTitle, strMessage );
						}
					}
				}

				// Output common HTML head
				HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["index_page"], string( CSS_INDEX ), string( ), NOT_INDEX, CODE_200 );

				// Deleted the torrent
				pResponse->strContent += "<p class=\"deleted\">" + UTIL_Xsprintf( gmapLANG_CFG["index_req_torrent"].c_str( ), strReqID.c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_index"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?section=req\">" ).c_str( ), "</a>" ) + "</p>\n";

				// Output common HTML tail
				HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INDEX ) );
				
				return;
			}
			else
			{
				// Output common HTML head
				HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["index_page"], string( CSS_INDEX ), string( ), NOT_INDEX, CODE_200 );

				pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["index_invalid_hash"].c_str( ), strReqID.c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_index"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";

				// Output common HTML tail
				HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INDEX ) );

				return;
			}
		}
	}
	
	if( pRequest->user.ucAccess & m_ucAccessReq )
	{
		if( pRequest->mapParams.find( "noreq" ) != pRequest->mapParams.end( ) )
		{
			string strNoReqID( pRequest->mapParams["noreq"] );
			
			if( strNoReqID.find( " " ) != string :: npos )
				strNoReqID.erase( );
			
			CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bid FROM allowed WHERE bid=" + strNoReqID );
		
			vector<string> vecQuery;
		
			vecQuery.reserve(1);

			vecQuery = pQuery->nextRow( );
			
			delete pQuery;
			
			if( vecQuery.size( ) == 1 && !vecQuery[0].empty( ) )
			{
				m_pCache->setStatus( strNoReqID, SET_STATUS_NOREQ );
				
				CMySQLQuery mq01( "UPDATE allowed SET breq=0 WHERE bid=" + strNoReqID );

				// Output common HTML head
				HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["index_page"], string( CSS_INDEX ), string( ), NOT_INDEX, CODE_200 );

				// Deleted the torrent
				pResponse->strContent += "<p class=\"deleted\">" + UTIL_Xsprintf( gmapLANG_CFG["index_req_cancel_torrent"].c_str( ), strNoReqID.c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_index"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "?section=req\">" ).c_str( ), "</a>" ) + "</p>\n";

				// Output common HTML tail
				HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INDEX ) );
				
				return;
			}
			else
			{
				// Output common HTML head
				HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["index_page"], string( CSS_INDEX ), string( ), NOT_INDEX, CODE_200 );

				pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["index_invalid_hash"].c_str( ), strNoReqID.c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_index"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";

				// Output common HTML tail
				HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INDEX ) );

				return;
			}
		}
	}
	
	const string cstrSub( pRequest->mapParams["sub"] );
	
	if( !cstrSub.empty( ) )
	{
		const string cstrID( pRequest->mapParams["id"] );
		const string cstrDo( pRequest->mapParams["do"] );
		const string cstrName( pRequest->mapParams["name"] );
		
		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["stats_page"], string( CSS_STATS ), string( ), NOT_INDEX, CODE_200 );
		
		if( !cstrID.empty( ) )
		{
			if( cstrDo == "delete" )
			{
				CMySQLQuery *pQuerySub = new CMySQLQuery( "SELECT btid,buid FROM subs WHERE bid=" + cstrSub );
	
				vector<string> vecQuerySub;

				vecQuerySub.reserve(2);

				vecQuerySub = pQuerySub->nextRow( );
			
				delete pQuerySub;
	
				if( vecQuerySub.size( ) == 2 && !vecQuerySub[0].empty( ) && vecQuerySub[0] == cstrID )
				{
					if( ( vecQuerySub[1] == pRequest->user.strUID && ( pRequest->user.ucAccess & m_ucAccessDelOwn ) ) || ( pRequest->user.ucAccess & m_ucAccessEditTorrents ) )
					{
						CMySQLQuery mq01( "DELETE FROM subs WHERE bid=" + cstrSub );
					
						// Deleted the sub
						pResponse->strContent += "<p class=\"deleted\">" + UTIL_Xsprintf( gmapLANG_CFG["stats_sub_deleted"].c_str( ), cstrSub.c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_stats"] + "\" href=\"" + RESPONSE_STR_STATS_HTML + "?id=" + cstrID + "\">" ).c_str( ), "</a>" ) + "</p>\n";
					
						// Output common HTML tail
						HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_STATS ) );

						return;
					}
				}
				else
				{
					pResponse->strContent += "<p class=\"deleted\">" + UTIL_Xsprintf( gmapLANG_CFG["stats_sub_delete_failed"].c_str( ), cstrSub.c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_stats"] + "\" href=\"" + RESPONSE_STR_STATS_HTML + "?id=" + cstrID + "\">" ).c_str( ), "</a>" ) + "</p>\n";
		
					// Output common HTML tail
					HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_STATS ) );
				
					return;
				}
			}
			else if( cstrDo == "edit" )
			{
				CMySQLQuery *pQuerySub = new CMySQLQuery( "SELECT btid,buid FROM subs WHERE bid=" + cstrSub );
	
				vector<string> vecQuerySub;

				vecQuerySub.reserve(2);

				vecQuerySub = pQuerySub->nextRow( );
			
				delete pQuerySub;
	
				if( vecQuerySub.size( ) == 2 && !vecQuerySub[0].empty( ) && vecQuerySub[0] == cstrID && !cstrName.empty( ) )
				{
					if( ( vecQuerySub[1] == pRequest->user.strUID && ( pRequest->user.ucAccess & m_ucAccessEditOwn ) ) || ( pRequest->user.ucAccess & m_ucAccessEditTorrents ) )
					{
						CMySQLQuery mq01( "UPDATE subs SET bname=\'" + UTIL_StringToMySQL( cstrName ) + "\' WHERE bid=" + cstrSub );
					
						// Deleted the sub
						pResponse->strContent += "<p class=\"deleted\">" + UTIL_Xsprintf( gmapLANG_CFG["stats_sub_edited"].c_str( ), cstrSub.c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_stats"] + "\" href=\"" + RESPONSE_STR_STATS_HTML + "?id=" + cstrID + "\">" ).c_str( ), "</a>" ) + "</p>\n";
					
						// Output common HTML tail
						HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_STATS ) );

						return;
					}
				}
				else
				{
					pResponse->strContent += "<p class=\"deleted\">" + UTIL_Xsprintf( gmapLANG_CFG["stats_sub_edit_failed"].c_str( ), cstrSub.c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_stats"] + "\" href=\"" + RESPONSE_STR_STATS_HTML + "?id=" + cstrID + "\">" ).c_str( ), "</a>" ) + "</p>\n";
		
					// Output common HTML tail
					HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_STATS ) );
				
					return;
				}
			}
		}
		
		pResponse->strContent += "<p class=\"deleted\">" + UTIL_Xsprintf( gmapLANG_CFG["stats_sub_action_failed"].c_str( ), cstrSub.c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_index"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";
		
		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_STATS ) );
		
		return;
	}

	if( !pRequest->user.strUID.empty( ) && ( pRequest->user.ucAccess & m_ucAccessViewStats ) )
	{
		// get the info hash from the parameter
		const string cstrReturnPage( pRequest->mapParams["return"] );
		const string cstrThanks( pRequest->mapParams["thanks"] );
		const string cstrAction( pRequest->mapParams["action"] );
		string cstrID = string( );

		string cstrHash = string( );
		string strFileName = string( );
		string strIDName = string( );
		string strIDKey = string( );

		bool bOffer = false;
		bool bPost = false;
		
		if( pRequest->mapParams.find( "id" ) != pRequest->mapParams.end( ) )
		{
			cstrID = pRequest->mapParams["id"];
			strIDName = "id";
			strIDKey = "btid";
		}
		else if( pRequest->mapParams.find( "oid" ) != pRequest->mapParams.end( ) )
		{
			cstrID = pRequest->mapParams["oid"];
			strIDName = "oid";
			strIDKey = "boid";
			bOffer = true;
		}

		
		if( cstrID.find( " " ) != string :: npos )
			cstrID.erase( );

		vector< pair< string, string > > vecParams;
		vecParams.reserve(64);
		string strJoined = string( );

		vecParams.push_back( pair<string, string>( strIDName, cstrID ) );
		vecParams.push_back( pair<string, string>( string( "return" ), cstrReturnPage ) );
		
		strJoined = UTIL_RemoveHTML( UTIL_HTMLJoin( vecParams, string( "?" ), string( "&" ), string( "=" ) ) );
		
		string strDatabase = string( );
		if( bOffer )
			strDatabase = "offer";
		else
			strDatabase = "allowed";
		
		if( pRequest->mapParams["thanks"] == "1" && pRequest->mapParams.find( "submit" ) != pRequest->mapParams.end( ) )
		{
			string strPageParameters = STATS_HTML;
			
			if( !cstrID.empty( ) )
				strPageParameters += "?id=" + cstrID;
			
			strPageParameters += "&thanks=1";
			
			return JS_ReturnToPage( pRequest, pResponse, strPageParameters );
		}
		
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["stats_page"], string( CSS_STATS ), string( ), NOT_INDEX, CODE_200 );
		
		// assorted scripts (thanks SA)
		pResponse->strContent += "<script type=\"text/javascript\">\n";
		pResponse->strContent += "<!--\n";
		
		pResponse->strContent += UTIL_JS_Edit_Tool_Bar( "torrentstats.intr" );
		
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
//		pResponse->strContent += "var obj_ta = document.torrentstats.intr;\n";
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
//		pResponse->strContent += "var obj_ta = document.torrentstats.intr;\n";
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
		pResponse->strContent += "function validate(theform) {\n";
		pResponse->strContent += "if( theform.comment.value == \"\" ) {\n";
		pResponse->strContent += "  alert( \"" + gmapLANG_CFG["js_fill_fields"] + "\" );\n";
		pResponse->strContent += "  return false; }\n";
		pResponse->strContent += "if( theform.comment.value.getBytes() > " + CAtomInt( m_uiCommentLength ).toString( ) + " ) {\n";
		pResponse->strContent += "  alert( \"" + gmapLANG_CFG["js_too_long"] + "\\n" + m_strJSLength + "\\n" + m_strJSReduce + "\" );\n";
		pResponse->strContent += "  return false; }\n";
		pResponse->strContent += "else { return true; }\n";
		pResponse->strContent += "}\n\n";
		
		pResponse->strContent += "String.prototype.getBytes = function() {\n";
		pResponse->strContent += "  var cArr = this.match(/[^\\x00-\\xff]/ig);\n";
		pResponse->strContent += "  return this.length + (cArr == null ? 0 : cArr.length);\n";
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
		pResponse->strContent += "function checklength(theform) {\n";
		pResponse->strContent += "  alert( \"" + UTIL_Xsprintf( gmapLANG_CFG["js_comment_length"].c_str( ), "\" + theform.comment.value.getBytes() + \"" ) + "\" );\n";
		pResponse->strContent += "}\n";
		
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
		pResponse->strContent += "        if( post_data != '' )\n";
		pResponse->strContent += "          post_data = post_data + '&';\n";
		pResponse->strContent += "        post_data = post_data + postForm.elements[i].name + '=' + encodeURIComponent(postForm.elements[i].value); }\n";
		pResponse->strContent += "    }\n";
		pResponse->strContent += "    xmlhttp.onreadystatechange=function() {\n";
		pResponse->strContent += "      if (xmlhttp.readyState==4 && xmlhttp.status==200) {\n";
		pResponse->strContent += "          clearTimeout( the_timeout );\n";
		pResponse->strContent += "          postTextarea.value = '';\n";
//		pResponse->strContent += "          charleft(document.postatalk,'talk_left','submit_talk');\n";
		pResponse->strContent += "          document.postacomment.comment.focus();\n";
		pResponse->strContent += "          postSubmit.disabled = false;\n";
		pResponse->strContent += "          load('div','divComment','" + RESPONSE_STR_STATS_HTML + "'+'?'+post_data); }\n";
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
//		pResponse->strContent += "      e.innerHTML = xmlhttp.responseText;\n";
//		pResponse->strContent += "      var forms = e.getElementsByTagName('form');\n";
//		pResponse->strContent += "      for (var i = 0; i < forms.length; i++) {\n";
//		pResponse->strContent += "        if (forms[i].name == formName) {\n";
//		pResponse->strContent += "          replyForm.parentNode.innerHTML = forms[i].parentNode.innerHTML;\n";
//		pResponse->strContent += "  	    window.location.hash = \"commentarea\";\n";
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
//		pResponse->strContent += "  var comment = document.getElementById( \"comment\" + commentid );\n";
//		pResponse->strContent += "  var commenter = document.getElementById( \"commenter\" + commentid );\n";
//		pResponse->strContent += "  var anchor = document.getElementById( \"commentarea\" );\n";
//		pResponse->strContent += "  anchor.value = \"[quote=\" + commenter.innerHTML + \"]\" + comment.innerHTML + \"[/quote]\";\n";
//		pResponse->strContent += "  window.location.hash = \"commentarea\";\n";
//		pResponse->strContent += "  anchor.focus();\n";
//		pResponse->strContent += "}\n";
		
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
		
		pResponse->strContent += "function change(id,show_name,hide_name) {\n";
		pResponse->strContent += "  var element = document.getElementById( id );\n";
		pResponse->strContent += "  if (element.style.display == \"\") {\n";
		pResponse->strContent += "    hide( id );\n";
		pResponse->strContent += "    return '<span class=\"blue\">' + show_name + '</span>'; }\n";
		pResponse->strContent += "  else {\n";
		pResponse->strContent += "    display( id );\n";
		pResponse->strContent += "    return '<span class=\"red\">' + hide_name + '</span>'; }\n";
		pResponse->strContent += "}\n";
		
		// thanks
		pResponse->strContent += "function saythanks(id,name) {\n";
		pResponse->strContent += "  var thanksForm = document.getElementById( id );\n";
		pResponse->strContent += "  var get_data = '?';\n";
		pResponse->strContent += "  for (var i = 0; i < thanksForm.elements.length; i++) {\n";
		pResponse->strContent += "    if ((thanksForm.elements[i].name == 'submit') &&\n";
		pResponse->strContent += "    (thanksForm.elements[i].disabled)) {\n";
		pResponse->strContent += "      return; }\n";
		pResponse->strContent += "    if (thanksForm.elements[i].name != 'submit')\n";
		pResponse->strContent += "      get_data = get_data + '&' + thanksForm.elements[i].name + '=' + thanksForm.elements[i].value;\n";
		pResponse->strContent += "  }\n";
		pResponse->strContent += "  xmlhttp.onreadystatechange=function() {\n";
		pResponse->strContent += "    if (xmlhttp.readyState==4 && xmlhttp.status==200) {\n";
		pResponse->strContent += "      var e = document.createElement('div');\n";
		pResponse->strContent += "      e.innerHTML = xmlhttp.responseText;\n";
		pResponse->strContent += "      var forms = e.getElementsByTagName('form');\n";
		pResponse->strContent += "      for (var i = 0; i < forms.length; i++) {\n";
		pResponse->strContent += "        if (forms[i].name == name) {\n";
		pResponse->strContent += "          thanksForm.parentNode.innerHTML = forms[i].parentNode.innerHTML;\n";
		pResponse->strContent += "          break; }\n";
		pResponse->strContent += "      }\n";
		pResponse->strContent += "    }\n";
		pResponse->strContent += "  }\n";
		pResponse->strContent += "  xmlhttp.open(\"GET\",thanksForm.action + get_data,true);\n";
		pResponse->strContent += "  xmlhttp.send();\n";
		pResponse->strContent += "}\n";
		
		// bookmark
		pResponse->strContent += "function bookmark(id,bookmark_link,nobookmark_link) {\n";
		pResponse->strContent += "  var bookmarkLink = document.getElementById( 'bookmark'+id );\n";
		pResponse->strContent += "  xmlhttp.onreadystatechange=function() {\n";
		pResponse->strContent += "    if (xmlhttp.readyState==4) {\n";
		pResponse->strContent += "      if (xmlhttp.status==200) {\n";
		pResponse->strContent += "        if (bookmarkLink.innerHTML == bookmark_link)\n";
		pResponse->strContent += "          bookmarkLink.innerHTML = nobookmark_link;\n";
		pResponse->strContent += "        else\n";
		pResponse->strContent += "          bookmarkLink.innerHTML = bookmark_link;\n";
		pResponse->strContent += "      }\n";
		pResponse->strContent += "    }\n";
		pResponse->strContent += "  }\n";
		pResponse->strContent += "  if (bookmarkLink.innerHTML == bookmark_link) {\n";
		pResponse->strContent += "    xmlhttp.open(\"GET\",\"" + RESPONSE_STR_LOGIN_HTML + "?bookmark=\" + id,true);\n";
		pResponse->strContent += "    xmlhttp.send(); }\n";
		pResponse->strContent += "  else {\n";
		pResponse->strContent += "    xmlhttp.open(\"GET\",\"" + RESPONSE_STR_LOGIN_HTML + "?nobookmark=\" + id,true);\n";
		pResponse->strContent += "    xmlhttp.send(); }\n";
		pResponse->strContent += "}\n";
		
		// request
		pResponse->strContent += "function request(id,req,success_link) {\n";
		pResponse->strContent += "  var requestLink = document.getElementById( 'request'+id );\n";
		pResponse->strContent += "  xmlhttp.onreadystatechange=function() {\n";
		pResponse->strContent += "    if (xmlhttp.readyState==4) {\n";
		pResponse->strContent += "      if (xmlhttp.status==200) {\n";
		pResponse->strContent += "        requestLink.innerHTML = success_link; }\n";
		pResponse->strContent += "    }\n";
		pResponse->strContent += "  }\n";
		pResponse->strContent += "  if (requestLink.innerHTML != success_link) {\n";
		pResponse->strContent += "    if (req == 'true') {\n";
		pResponse->strContent += "      xmlhttp.open(\"GET\",\"" + RESPONSE_STR_STATS_HTML + "?req=\" + id,true);\n";
		pResponse->strContent += "      xmlhttp.send(); }\n";
		pResponse->strContent += "    else {\n";
		pResponse->strContent += "      xmlhttp.open(\"GET\",\"" + RESPONSE_STR_STATS_HTML + "?noreq=\" + id,true);\n";
		pResponse->strContent += "      xmlhttp.send();  }\n";
		pResponse->strContent += "  }\n";
		pResponse->strContent += "}\n";
		
		pResponse->strContent += "function refresh(id) {\n";
		pResponse->strContent += "  var subsDiv = document.getElementById( id );\n";
		pResponse->strContent += "  xmlhttp.onreadystatechange=function() {\n";
		pResponse->strContent += "    if (xmlhttp.readyState==4 && xmlhttp.status==200) {\n";
		pResponse->strContent += "      var e = document.createElement('div');\n";
		pResponse->strContent += "      e.innerHTML = xmlhttp.responseText;\n";
		pResponse->strContent += "      var divs = e.getElementsByTagName('div');\n";
		pResponse->strContent += "      for (var i = 0; i < divs.length; i++) {\n";
		pResponse->strContent += "        if (divs[i].id == id) {\n";
		pResponse->strContent += "          subsDiv.parentNode.innerHTML = divs[i].parentNode.innerHTML;\n";
		pResponse->strContent += "          break; }\n";
		pResponse->strContent += "      }\n";
		pResponse->strContent += "    }\n";
		pResponse->strContent += "  }\n";
		pResponse->strContent += "  xmlhttp.open(\"GET\",window.location,true);\n";
		pResponse->strContent += "  xmlhttp.send();\n";
		pResponse->strContent += "}\n";
		
		pResponse->strContent += "function edit_sub_confirm(id,sub,promptName,promptError) {\n";
		pResponse->strContent += "  var subElement = document.getElementById( 'sub'+sub );\n";
		pResponse->strContent += "  var subName = subElement.innerHTML;\n";
		pResponse->strContent += "  subName = subName.replace(/&lt;/g,\"<\");\n";
		pResponse->strContent += "  subName = subName.replace(/&gt;/g,\">\");\n";
		pResponse->strContent += "  subName = subName.replace(/&amp;/g,\"&\");\n";
		pResponse->strContent += "  subName = subName.replace(/&quot;/g,\"\\\"\");\n";
		pResponse->strContent += "  subName = subName.replace(/&nbsp;/g,\" \");\n";
		pResponse->strContent += "  var enterName = prompt(promptName, subName);\n";
		pResponse->strContent += "  if (!enterName) {\n";
		pResponse->strContent += "    alert(promptError + \" \" + promptName);\n";
		pResponse->strContent += "    return; }\n";
		pResponse->strContent += "  window.location = \'" + RESPONSE_STR_STATS_HTML + "?id=\' + id + \'&sub=\' + sub + \'&do=edit&name=\' + encodeURIComponent(enterName);\n";
		pResponse->strContent += "}\n\n";
		
		pResponse->strContent += "function delete_sub_confirm(id,sub) {\n";
		pResponse->strContent += "  if( confirm(\"" + gmapLANG_CFG["stats_sub_delete_q"] + "\") ) {\n";
		pResponse->strContent += "    window.location = \'" + RESPONSE_STR_STATS_HTML + "?id=\' + id + \'&sub=\' + sub + \'&do=delete\'; }\n";
		pResponse->strContent += "}\n\n";
		
		pResponse->strContent += "//-->\n";


		pResponse->strContent += "</script>\n\n";
		
		CMySQLQuery *pQuery = 0;
		
		vector<string> vecQuery;
		
		if( !cstrID.empty( ) )
		{
			if( bOffer )
			{
				pQuery = new CMySQLQuery( "SELECT bhash,bfilename,bname,badded,bsize,bfiles,bcomment,btag,btitle,bintr,buploader,buploaderid,bimdb,bimdbid,bimdbupdated FROM " + strDatabase + " WHERE bid=" + cstrID );
		
				vecQuery.reserve(15);
			}
			else
			{
				pQuery = new CMySQLQuery( "SELECT bhash,bfilename,bname,badded,bsize,bfiles,bcomment,btag,btitle,bintr,buploader,buploaderid,bimdb,bimdbid,bimdbupdated,bdefault_down,bdefault_up,bfree_down,bfree_up,bfree_time,UNIX_TIMESTAMP(bfree_to),btop,bclassic,breq,bnodownload,bnocomment,bseeders,bleechers,bcompleted,bupdated,bpost FROM " + strDatabase + " WHERE bid=" + cstrID );
		
				vecQuery.reserve(31);
			}

			vecQuery = pQuery->nextRow( );
			
			delete pQuery;
		}
		else
		{
			pResponse->strContent += "<p class=\"not_exist\">" + UTIL_Xsprintf( gmapLANG_CFG["torrent_not_exist"].c_str( ), cstrID.c_str( ) );
			HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_STATS ) );
			return;
		}
		
		if( vecQuery.size( ) == 15 || vecQuery.size( ) == 31 )
			cstrHash = vecQuery[0];
		
		if( vecQuery.size( ) == 31 && vecQuery[30] == "1" )
			bPost = true;
		
		if( !cstrHash.empty( ) )
		{
			const string cstrHashString( UTIL_HashToString( cstrHash ) );
			//
			// admin
			//
			
			string strCur = string( );
			string strOldName = string( );
			string strOldUploader = string( );
			string strOldUploaderID = string( );
			string strOldIMDb = string( );
			string strOldIMDbID = string( );
			string strOldIMDbUpdated = string( );
			string strOldIntr = string( );
			string strOldFreeDown = string( "100" );
			string strOldFreeUp = string( "100");
			string strOldFreeTime = string( "0" );
			string strOldDefaultDown = string( "100" );
			string strOldDefaultUp = string( "100");

			strCur = vecQuery[7];
			strOldName = vecQuery[8];
			strOldIntr = vecQuery[9];
			strOldUploader = vecQuery[10];
			strOldUploaderID = vecQuery[11];
			strOldIMDb = vecQuery[12];
			strOldIMDbID = vecQuery[13];
			strOldIMDbUpdated = vecQuery[14];
			if( !bOffer )
			{
				strOldDefaultDown = vecQuery[15];
				strOldDefaultUp = vecQuery[16];
				strOldFreeDown = vecQuery[17];
				strOldFreeUp = vecQuery[18];
				strOldFreeTime = vecQuery[19];
			}

			const string cstrReturnPage( pRequest->mapParams["return"] );
			const string cstrReturnPageResp( UTIL_StringToEscaped( cstrReturnPage ) );
			// needs work

			if( !vecQuery[1].empty( ) )
				strFileName = vecQuery[1];
			
			if( ( !bOffer && ( pRequest->user.ucAccess & m_ucAccessEditTorrents ) ) || ( bOffer && ( pRequest->user.ucAccess & m_ucAccessEditOffers ) ) || ( ( pRequest->user.ucAccess & m_ucAccessEditOwn ) && !pRequest->user.strUID.empty( ) && ( pRequest->user.strUID == strOldUploaderID ) ) )
			{
				// The Trinity Edition - Modification Begins
				// Removed "(blank values mean no change)" from each field; This information now appears once after the form
				if( pRequest->mapParams.find( "action" ) != pRequest->mapParams.end( ) )
				{
					if( cstrAction == "edit" )
					{
						pResponse->strContent += "<div class=\"change_info\">\n";
						pResponse->strContent += "<p class=\"change_info\">" + gmapLANG_CFG["stats_change_info"] + "</p>\n";
						if( strOldName.empty( ) )
							pResponse->strContent += "<h3>" + UTIL_RemoveHTML( vecQuery[2] ) + "</h3>\n";
						else
							pResponse->strContent += "<h3>" + UTIL_RemoveHTML( strOldName ) + "</h3>\n";
						pResponse->strContent += "<table class=\"change_info\">\n";
						pResponse->strContent += "<form name=\"torrentstats\" method=\"post\" action=\"" + RESPONSE_STR_STATS_HTML + "\" enctype=\"multipart/form-data\">\n";
						pResponse->strContent += "<input name=\"";
						if( bOffer )
							pResponse->strContent += "oid";
						else
							pResponse->strContent += "id";
						pResponse->strContent += "\" type=hidden value=\"" + cstrID + "\">\n";

						if( bPost )
							pResponse->strContent += "<input name=\"post\" type=hidden value=\"1\">\n";
//						if( bOffer )
//							pResponse->strContent += "<input name=\"offer\" type=hidden value=\"1\">\n";
						if( !cstrReturnPage.empty( ) )
							pResponse->strContent += "<input name=\"return\" type=hidden value=\"" + cstrReturnPage + "\">\n";
						pResponse->strContent += "<tr class=\"change_info\">\n<th class=\"change_info\">" + gmapLANG_CFG["stats_new_name"] + " (&dagger;)</th>\n";
						pResponse->strContent += "<td class=\"change_info\"><input id=\"name\" name=\"name\" alt=\"[" + gmapLANG_CFG["stats_new_name"] + "]\" type=text size=96 maxlength=" + CAtomInt( MAX_FILENAME_LEN ).toString( ) + " value=\"" + UTIL_RemoveHTML( strOldName ) + "\"></td>\n</tr>\n";

						if( !bPost )
						{
							pResponse->strContent += "<tr class=\"change_info\">\n<th class=\"change_info\">" + gmapLANG_CFG["stats_new_imdb"] + "</th>\n";
							pResponse->strContent += "<td class=\"change_info\"><input name=\"imdb\" alt=\"[" + gmapLANG_CFG["imdb"] + "]\" type=text size=12 maxlength=16 value=\"" + strOldIMDbID + "\">" + gmapLANG_CFG["imdb_note"] + "</td>\n</tr>\n";
						}
						pResponse->strContent += "<tr class=\"change_info\">\n<th class=\"change_info\">" + gmapLANG_CFG["upload_attachment"] + "</th>\n";
						pResponse->strContent += "<td class=\"change_info\"><iframe src=\"" + RESPONSE_STR_FILE_UPLOAD_HTML + "\" frameborder=\"0\" scrolling=\"no\"></iframe>";
						pResponse->strContent += "</td>\n</tr>\n";
						pResponse->strContent += "<tr class=\"change_info\">\n<th class=\"change_info\">" + gmapLANG_CFG["stats_new_intr"] + "</th>\n";
						pResponse->strContent += "<td class=\"change_info\">";
						
						pResponse->strContent += UTIL_Edit_Tool_Bar( );
//						pResponse->strContent += "<input style=\"font-weight: bold\" type=\"button\" name=\"B\" value=\"" + gmapLANG_CFG["insert_b"] + "\" onclick=\"javascript: doInsertSelect('[b]', '[/b]')\">\n";
//						pResponse->strContent += "<input style=\"font-style: italic\" type=\"button\" name=\"I\" value=\"" + gmapLANG_CFG["insert_i"] + "\" onclick=\"javascript: doInsertSelect('[i]', '[/i]')\">\n";
//						pResponse->strContent += "<input style=\"text-decoration: underline\" type=\"button\" name=\"U\" value=\"" + gmapLANG_CFG["insert_u"] + "\" onclick=\"javascript: doInsertSelect('[u]', '[/u]')\">\n";
//						pResponse->strContent += "<select name=\"size\" onchange=\"insertFont(this.options[this.selectedIndex].value, 'size')\">\n";
//						pResponse->strContent += "<option value=\"0\">" + gmapLANG_CFG["insert_fontsize"] + "</option>\n";
//						pResponse->strContent += "<option value=\"1\">1</option>\n";
//						pResponse->strContent += "<option value=\"2\">2</option>\n";
//						pResponse->strContent += "<option value=\"3\">3</option>\n";
//						pResponse->strContent += "<option value=\"4\">4</option>\n";
//						pResponse->strContent += "<option value=\"5\">5</option>\n";
//						pResponse->strContent += "<option value=\"6\">6</option>\n";
//						pResponse->strContent += "<option value=\"7\">7</option>\n";
//						pResponse->strContent += "</select>\n";
//						pResponse->strContent += "<select name=\"color\" onchange=\"insertFont(this.options[this.selectedIndex].value, 'color')\">\n";
//						pResponse->strContent += "<option value=\"0\">" + gmapLANG_CFG["insert_fontcolor"] + "</option>\n";
//						pResponse->strContent += "<option style=\"background-color: black\" value=\"Black\">Black</option>\n";
//						pResponse->strContent += "<option style=\"background-color: sienna\" value=\"Sienna\">Sienna</option>\n";
//						pResponse->strContent += "<option style=\"background-color: darkolivegreen\" value=\"DarkOliveGreen\">Dark Olive Green</option>\n";
//						pResponse->strContent += "<option style=\"background-color: darkgreen\" value=\"DarkGreen\">Dark Green</option>\n";
//						pResponse->strContent += "<option style=\"background-color: darkslateblue\" value=\"DarkSlateBlue\">Dark Slate Blue</option>\n";
//						pResponse->strContent += "<option style=\"background-color: navy\" value=\"Navy\">Navy</option>\n";
//						pResponse->strContent += "<option style=\"background-color: indigo\" value=\"Indigo\">Indigo</option>\n";
//						pResponse->strContent += "<option style=\"background-color: darkslategray\" value=\"DarkSlateGray\">Dark Slate Gray</option>\n";
//						pResponse->strContent += "<option style=\"background-color: darkred\" value=\"DarkRed\">Dark Red</option>\n";
//						pResponse->strContent += "<option style=\"background-color: darkorange\" value=\"DarkOrange\">Dark Orange</option>\n";
//						pResponse->strContent += "<option style=\"background-color: olive\" value=\"Olive\">Olive</option>\n";
//						pResponse->strContent += "<option style=\"background-color: green\" value=\"Green\">Green</option>\n";
//						pResponse->strContent += "<option style=\"background-color: teal\" value=\"Teal\">Teal</option>\n";
//						pResponse->strContent += "<option style=\"background-color: blue\" value=\"Blue\">Blue</option>\n";
//						pResponse->strContent += "<option style=\"background-color: slategray\" value=\"SlateGray\">Slate Gray</option>\n";
//						pResponse->strContent += "<option style=\"background-color: dimgray\" value=\"DimGray\">Dim Gray</option>\n";
//						pResponse->strContent += "<option style=\"background-color: red\" value=\"Red\">Red</option>\n";
//						pResponse->strContent += "<option style=\"background-color: sandybrown\" value=\"SandyBrown\">Sandy Brown</option>\n";
//						pResponse->strContent += "<option style=\"background-color: yellowgreen\" value=\"YellowGreen\">Yellow Green</option>\n";
//						pResponse->strContent += "<option style=\"background-color: seagreen\" value=\"SeaGreen\">Sea Green</option>\n";
//						pResponse->strContent += "<option style=\"background-color: mediumturquoise\" value=\"MediumTurquoise\">Medium Turquoise</option>\n";
//						pResponse->strContent += "<option style=\"background-color: royalblue\" value=\"RoyalBlue\">Royal Blue</option>\n";
//						pResponse->strContent += "<option style=\"background-color: purple\" value=\"Purple\">Purple</option>\n";
//						pResponse->strContent += "<option style=\"background-color: gray\" value=\"Gray\">Gray</option>\n";
//						pResponse->strContent += "<option style=\"background-color: magenta\" value=\"Magenta\">Magenta</option>\n";
//						pResponse->strContent += "<option style=\"background-color: orange\" value=\"Orange\">Orange</option>\n";
//						pResponse->strContent += "<option style=\"background-color: yellow\" value=\"Yellow\">Yellow</option>\n";
//						pResponse->strContent += "<option style=\"background-color: lime\" value=\"Lime\">Lime</option>\n";
//						pResponse->strContent += "<option style=\"background-color: cyan\" value=\"Cyan\">Cyan</option>\n";
//						pResponse->strContent += "<option style=\"background-color: deepskyblue\" value=\"DeepSkyBlue\">Deep Sky Blue</option>\n";
//						pResponse->strContent += "<option style=\"background-color: darkorchid\" value=\"DarkOrchid\">Dark Orchid</option>\n";
//						pResponse->strContent += "<option style=\"background-color: silver\" value=\"Silver\">Silver</option>\n";
//						pResponse->strContent += "<option style=\"background-color: pink\" value=\"Pink\">Pink</option>\n";
//						pResponse->strContent += "<option style=\"background-color: wheat\" value=\"Wheat\">Wheat</option>\n";
//						pResponse->strContent += "<option style=\"background-color: lemonchiffon\" value=\"LemonChiffon\">Lemon Chiffon</option>\n";
//						pResponse->strContent += "<option style=\"background-color: palegreen\" value=\"PaleGreen\">Pale Green</option>\n";
//						pResponse->strContent += "<option style=\"background-color: paleturquoise\" value=\"PaleTurquoise\">Pale Turquoise</option>\n";
//						pResponse->strContent += "<option style=\"background-color: lightblue\" value=\"LightBlue\">Light Blue</option>\n";
//						pResponse->strContent += "<option style=\"background-color: plum\" value=\"Plum\">Plum</option>\n"; 
//						pResponse->strContent += "<option style=\"background-color: white\" value=\"White\">White</option>\n";
//						pResponse->strContent += "</select>\n";
//						pResponse->strContent += "<input type=\"button\" name=\"IMG\" value=\"" + gmapLANG_CFG["insert_img"] + "\" onclick=\"javascript: tag_image('" + gmapLANG_CFG["insert_img_fill"] + "','" + gmapLANG_CFG["insert_error"] + "')\">\n";

//						pResponse->strContent += "<input type=\"button\" name=\"URL\" value=\"" + gmapLANG_CFG["insert_url"] + "\" onclick=\"javascript: tag_url('" + gmapLANG_CFG["insert_url_fill"] + "','" + gmapLANG_CFG["insert_url_title"] + "','" + gmapLANG_CFG["insert_error"] +"')\">\n";
//						pResponse->strContent += "<input type=\"button\" name=\"QUOTE\" value=\"" + gmapLANG_CFG["insert_quote"] + "\" onclick=\"javascript: doInsertSelect('[quote]', '[/quote]')\"><br>";
						pResponse->strContent += "<br>";
						pResponse->strContent += "<textarea id=\"intr\" name=\"intr\" rows=10 cols=96>" + UTIL_RemoveHTML3( strOldIntr ) + "</textarea></td>\n</tr>\n";

						if( !m_vecTags.empty( ) )
						{
							pResponse->strContent += "<tr class=\"change_info\">\n<th class=\"change_info\">" + gmapLANG_CFG["stats_new_tag"] + "</th>\n";
							pResponse->strContent += "<td class=\"change_info\"><select id=\"tag\" name=\"tag\">\n";
						
							string strNameIndex = string( );
							string strTag = string( );
							
							for( vector< pair< string, string > > :: iterator it = m_vecTags.begin( ); it != m_vecTags.end( ); it++ )
							{
								strNameIndex = (*it).first;
								strTag = (*it).second;
								
								pResponse->strContent += "<option value=\""  + strNameIndex + "\"";

								if( strNameIndex == strCur )
									pResponse->strContent += " selected";

								pResponse->strContent += ">" + strTag + "\n";

							}
							pResponse->strContent += "</select></td>\n</tr>\n";
						}
						
						if( ( pRequest->user.ucAccess & m_ucAccessEditTorrents ) && !bOffer )
						{
							if( !bPost )
							{
								pResponse->strContent += "<tr class=\"change_info\">\n<th class=\"change_info\">" + gmapLANG_CFG["stats_new_free_default"] + "</th>\n";
								pResponse->strContent += "<td class=\"change_info\">";
								pResponse->strContent += "<span class=\"blue\">" + gmapLANG_CFG["stats_default_down"] + "</span><input name=\"default_down\" type=text size=5 maxlength=3 value=\"" + strOldDefaultDown + "\">% ";
								pResponse->strContent += "<span class=\"green\">" + gmapLANG_CFG["stats_default_up"] + "</span><input name=\"default_up\" type=text size=5 maxlength=3 value=\"" + strOldDefaultUp + "\">%</td>\n</tr>\n";
								pResponse->strContent += "<tr class=\"change_info\">\n<th class=\"change_info\">" + gmapLANG_CFG["stats_new_free"] + "</th>\n";
								pResponse->strContent += "<td class=\"change_info\">";
								pResponse->strContent += "<input id=\"id_free_remain\" name=\"free_status\" alt=\"[" + gmapLANG_CFG["stats_free_remain"] + "]\" type=radio value=\"remain\" onclick=\"javascript: hide('id_free_time_change')\" checked>";
								pResponse->strContent += "<label for=\"id_free_remain\">" + gmapLANG_CFG["stats_free_remain"] + "</label>";
								pResponse->strContent += "<input id=\"id_free_change\" name=\"free_status\" alt=\"[" + gmapLANG_CFG["stats_free_change"] + "]\" type=radio value=\"change\" onclick=\"javascript: display('id_free_time_change')\">";
								pResponse->strContent += "<label for=\"id_free_change\">";
								pResponse->strContent += gmapLANG_CFG["stats_free_change"] + "</label>";
								pResponse->strContent += "<span id=\"id_free_time_change\" style=\"display: none\"><br><br><span class=\"blue\">" + gmapLANG_CFG["stats_free_down"] + "</span><input name=\"free_down\" type=text size=5 maxlength=3 value=\"" + strOldFreeDown + "\">%   ";
								pResponse->strContent += "<span class=\"green\">" + gmapLANG_CFG["stats_free_up"] + "</span><input name=\"free_up\" type=text size=5 maxlength=3 value=\"" + strOldFreeUp + "\">% ";
								pResponse->strContent += "<select name=\"free_from\">\n";
								pResponse->strContent += "<option value=\"added\" selected>" + gmapLANG_CFG["stats_free_from_added"] + "\n";
								pResponse->strContent += "<option value=\"now\">" + gmapLANG_CFG["stats_free_from_now"] + "\n";
								pResponse->strContent += "</select>\n";
								pResponse->strContent += gmapLANG_CFG["stats_free_time"] + "<input name=\"free_time\" type=text size=5 maxlength=3 value=\"" + strOldFreeTime + "\">" + gmapLANG_CFG["stats_free_hours"] + "</span>";
								pResponse->strContent += "</td>\n</tr>\n";
							}
							pResponse->strContent += "<tr class=\"change_info\">\n<th class=\"change_info\">" + gmapLANG_CFG["stats_new_status"] + "</th>\n";
							pResponse->strContent += "<td class=\"change_info\">\n";
							pResponse->strContent += gmapLANG_CFG["top"];
							pResponse->strContent += ": <select name=\"top\" alt=\"[" + gmapLANG_CFG["top"] + "]\">\n";
							for( unsigned char ucLoop = 0; ucLoop < 4; ucLoop++ )
							{
								pResponse->strContent += "<option value=\"" + CAtomInt( ucLoop ).toString( ) + "\"";
								if( CAtomInt( ucLoop ).toString( ) == vecQuery[21] )
									pResponse->strContent += " selected";
								pResponse->strContent += ">" + gmapLANG_CFG["top_level_"+CAtomInt( ucLoop ).toString( )] + "\n";
							}
							pResponse->strContent += "</select>\n";
//							pResponse->strContent += "<input id=\"id_top\" name=\"top\" alt=\"[" + gmapLANG_CFG["top"] + "]\" type=checkbox";
//							if( vecQuery[20] == "1" )
//								pResponse->strContent += " checked";
//							pResponse->strContent += "> <label for=\"id_top\">" + gmapLANG_CFG["top"] + "</label> \n";
							pResponse->strContent += gmapLANG_CFG["classic"];
							pResponse->strContent += ": <select name=\"classic\" alt=\"[" + gmapLANG_CFG["classic"] + "]\">\n";
							for( unsigned char ucLoop = 0; ucLoop < 4; ucLoop++ )
							{
								pResponse->strContent += "<option value=\"" + CAtomInt( ucLoop ).toString( ) + "\"";
								if( CAtomInt( ucLoop ).toString( ) == vecQuery[22] )
									pResponse->strContent += " selected";
								pResponse->strContent += ">" + gmapLANG_CFG["classic_level_"+CAtomInt( ucLoop ).toString( )] + "\n";
							}
							pResponse->strContent += "</select>\n";
//							pResponse->strContent += "<input id=\"id_hl\" name=\"hl\" alt=\"[" + gmapLANG_CFG["hl"] + "]\" type=checkbox";
//							if( vecQuery[21] == "1" )
//								pResponse->strContent += " checked";
//							pResponse->strContent += "> <label for=\"id_hl\">" + gmapLANG_CFG["hl"] + "</label> \n";
//							pResponse->strContent += "<input id=\"id_classic\" name=\"classic\" alt=\"[" + gmapLANG_CFG["classic"] + "]\" type=checkbox";
//							if( vecQuery[22] == "1" )
//								pResponse->strContent += " checked";
//							pResponse->strContent += "> <label for=\"id_classic\">" + gmapLANG_CFG["classic"] + "</label> \n";
							if( !bPost )
							{
								pResponse->strContent += "<input id=\"id_req\" name=\"req\" alt=\"[" + gmapLANG_CFG["section_reqseeders"] + "]\" type=checkbox";
								if( vecQuery[23] == "1" )
									pResponse->strContent += " checked";
								pResponse->strContent += "> <label for=\"id_req\">" + gmapLANG_CFG["section_reqseeders"] + "</label> \n";
								pResponse->strContent += "<input id=\"id_nodownload\" name=\"nodownload\" alt=\"[" + gmapLANG_CFG["no_download"] + "]\" type=checkbox";
								if( vecQuery[24] == "1" )
									pResponse->strContent += " checked";
								pResponse->strContent += "> <label for=\"id_nodownload\">" + gmapLANG_CFG["no_download"] + "</label> \n";
							}
							pResponse->strContent += "<input id=\"id_nocomment\" name=\"nocomment\" alt=\"[" + gmapLANG_CFG["no_comment"] + "]\" type=checkbox";
							if( vecQuery[25] == "1" )
								pResponse->strContent += " checked";
							pResponse->strContent += "> <label for=\"id_nocomment\">" + gmapLANG_CFG["no_comment"] + "</label> \n";
							pResponse->strContent += "</td>\n</tr>\n";
						}
						pResponse->strContent += "<tr class=\"change_info\">\n<td class=\"change_info\" colspan=\"2\">\n";
						pResponse->strContent += "<div class=\"change_info_button\">\n";
						pResponse->strContent += Button_Submit( "submit_change", string( gmapLANG_CFG["stats_change_info"] ) );
						pResponse->strContent += Button_Back( "cancel_change", string( gmapLANG_CFG["cancel"] ) );
						pResponse->strContent += "\n</div>\n</td>\n</tr>\n";
						// The Trinity Edition - Addition Begins
						// The following explains how to leave values unchanged and how to remove values from the database
						pResponse->strContent += "<tr class=\"change_info\">\n<td class=\"change_info\" colspan=\"2\">" + gmapLANG_CFG["stats_info_blank"] + "</td>\n</tr>\n";
						pResponse->strContent += "</form>\n</table>\n</div>\n\n<hr class=\"stats_hr\">\n\n";
					}
					else if( cstrAction == "edited" && !cstrReturnPage.empty( ) )
					{
						if( bOffer )
							pResponse->strContent += "<p class=\"changed_stats\">" + UTIL_Xsprintf( gmapLANG_CFG["stats_return_offer_edited"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_offer"] + "\" href=\"" + cstrReturnPage + "\">").c_str( ), "</a>" ) + "</p>\n";
						else
							pResponse->strContent += "<p class=\"changed_stats\">" + UTIL_Xsprintf( gmapLANG_CFG["stats_return_edited"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_index"] + "\" href=\"" + cstrReturnPage + "\">").c_str( ), "</a>" ) + "</p>\n";
					}
				}
			}

			// display torrent information list
			
// 			const string cstrReturnPage( pRequest->mapParams["return"] );

			int64 iSize = 0;
			unsigned int uiFiles = 1;
			
			if( strOldName.empty( ) )
				strOldName = vecQuery[2];

			pResponse->strContent += "<div class=\"stats_table\">\n";
			
			if( !pRequest->user.strUID.empty( ) && !bPost && !bOffer && cstrThanks == "1" )
			{
				CMySQLQuery *pQuery = new CMySQLQuery( "SELECT buploaderid FROM allowed WHERE bid=" + cstrID );

				vector<string> vecQuery;

				vecQuery.reserve(1);

				vecQuery = pQuery->nextRow( );
	
				delete pQuery;
	
				if( vecQuery.size( ) == 1 && vecQuery[0] != pRequest->user.strUID ) 
				{
					unsigned int uiThanksBonus = CFG_GetInt( "bnbt_thanks_bonus", 3 );
					CMySQLQuery *pQueryThanks = new CMySQLQuery( "SELECT bid,bthankerid FROM thanks WHERE bid=" + cstrID + " AND bthankerid=" + pRequest->user.strUID );

					vector<string> vecQueryThanks;

					vecQueryThanks.reserve(2);

					vecQueryThanks = pQueryThanks->nextRow( );
	
					delete pQueryThanks;
		
					if( vecQueryThanks.size( ) == 0 )
					{
						CMySQLQuery mq01( "INSERT INTO thanks (bid,bthankerid,bthanker,bthanktime) VALUES(" + cstrID + "," + pRequest->user.strUID + ",\'" + UTIL_StringToMySQL( pRequest->user.strLogin ) + "\',NOW())" );
//						m_pCache->setUserData( vecQuery[0], 0, 0, uiThanksBonus * 100 );
						CMySQLQuery mq02( "UPDATE users SET bbonus=bbonus+" + CAtomInt( uiThanksBonus * 100 ).toString( ) + " WHERE buid=" + vecQuery[0] );
						pResponse->strContent += "<p class=\"file_info\">" + gmapLANG_CFG["thanks_successful"] + "</p>";
					}
					else if( vecQueryThanks.size( ) == 2 )
						pResponse->strContent += "<p class=\"file_info\">" + gmapLANG_CFG["thanks_again"] + "</p>";
				}
				else
					pResponse->strContent += "<p class=\"file_info\">" + gmapLANG_CFG["thanks_failed"] + "</p>";
			}
			
			pResponse->strContent += "<p class=\"file_info\">" + gmapLANG_CFG["stats_file_info"] + "</p>\n";
// 			pResponse->strContent += "<p class=\"file_info\">" + UTIL_RemoveHTML( strOldName ) + "</p>\n";
			pResponse->strContent += "<h3>" + UTIL_RemoveHTML( strOldName ) + "</h3>\n";
			
			time_t now_t = time( 0 );
			
			bool bFreeGlobal = CFG_GetInt( "bnbt_free_global", 0 ) == 0 ? false : true;

			int64 iFreeDownGlobal = CFG_GetInt( "bnbt_free_down_global", 100 );
			int64 iFreeUpGlobal = CFG_GetInt( "bnbt_free_up_global", 100 );
			
			if( !bOffer )
			{
				int64 iDefaultDown = 100;
				int64 iDefaultUp = 100;
				int64 iFreeDown = 100;
				int64 iFreeUp = 100;
				int64 iFreeTo = 0;

				iDefaultDown = atoi( vecQuery[15].c_str( ) );
				iDefaultUp = atoi( vecQuery[16].c_str( ) );
				iFreeDown = atoi( vecQuery[17].c_str( ) );
				iFreeUp = atoi( vecQuery[18].c_str( ) );
				iFreeTo = UTIL_StringTo64( vecQuery[20].c_str( ) );
			
				if( bFreeGlobal )
				{
					if( iFreeDownGlobal < iDefaultDown )
						iDefaultDown = iFreeDownGlobal;
					if( iFreeUpGlobal > iDefaultUp )
						iDefaultUp = iFreeUpGlobal;
				}
				
				int64 day_left = -1, hour_left = -1, minute_left = -1;
				
				time_t tTimeFree = iFreeTo - now_t;
				
				if( tTimeFree > 0 )
				{
					tTimeFree += 60;
					tTimeFree /= 60;
					minute_left =  tTimeFree % 60;
					tTimeFree /= 60;
					hour_left =  tTimeFree % 24;
					tTimeFree /= 24;
					day_left =  tTimeFree;
				}

//				if( iFreeTo > now_t )
//				{
//					day_left = ( iFreeTo - now_t + 60 ) / 86400;
//					hour_left = ( iFreeTo - now_t + 60 ) % 86400 / 3600;
//					minute_left = ( iFreeTo - now_t + 60 ) % 3600 / 60;
//				}
				if( iFreeTo > 0 && day_left >= 0 )
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
				if( iFreeDown != 100 || iFreeUp != 100 )
				{
					pResponse->strContent += "<p>";
					if( iFreeDown != 100 )
					{
						if( iFreeDown == 0 )
							pResponse->strContent += "<span class=\"blue\">" + gmapLANG_CFG["free_down_free"] + "</span>";
						else
							pResponse->strContent += "<span class=\"blue\">" + UTIL_Xsprintf( gmapLANG_CFG["free_down"].c_str( ), CAtomInt( iFreeDown ).toString( ).c_str( ) )+ "</span>";
					}
					if( iFreeUp != 100 )
						pResponse->strContent += "<span class=\"green\">" + UTIL_Xsprintf( gmapLANG_CFG["free_up"].c_str( ), CAtomInt( iFreeUp ).toString( ).c_str( ) )+ "</span>";
					if( day_left >= 0 && ( iDefaultDown > iFreeDown || iDefaultUp < iFreeUp ) )
					{
						pResponse->strContent += "<span class=\"free_recover\" title=\"" + gmapLANG_CFG["free_recover"];
						pResponse->strContent += UTIL_Xsprintf( gmapLANG_CFG["free_down"].c_str( ), CAtomInt( iDefaultDown ).toString( ).c_str( ) ) + UTIL_Xsprintf( gmapLANG_CFG["free_up"].c_str( ), CAtomInt( iDefaultUp ).toString( ).c_str( ) ) + "\">";
							if( day_left > 0 )
								pResponse->strContent += UTIL_Xsprintf( gmapLANG_CFG["free_day_left"].c_str( ), CAtomInt( day_left ).toString( ).c_str( ), CAtomInt( hour_left ).toString( ).c_str( ) );
							else if( hour_left > 0 )
								pResponse->strContent += UTIL_Xsprintf( gmapLANG_CFG["free_hour_left"].c_str( ), CAtomInt( hour_left ).toString( ).c_str( ), CAtomInt( minute_left ).toString( ).c_str( ) );
							else if( minute_left >= 0 )
								pResponse->strContent += UTIL_Xsprintf( gmapLANG_CFG["free_minute_left"].c_str( ), CAtomInt( minute_left ).toString( ).c_str( ) );
						pResponse->strContent += "</span>";
					}
					pResponse->strContent += "</p>";
				}
			}

			pResponse->strContent += "<table class=\"file_info\" summary=\"file info\">\n";
			
			if( !pRequest->user.strUID.empty( ) )
			{
				pResponse->strContent += "<tr class=\"file_info\">";

				pResponse->strContent += "<th class=\"file_info\">" + gmapLANG_CFG["stats_operation"] + ":</th>\n";
				pResponse->strContent += "<td class=\"file_info\">";
				bool bNoDownload = false;
				if( !bOffer )
				{
					if( ( !vecQuery[24].empty( ) && vecQuery[24] == "1" ) || bPost )
						bNoDownload = true;
				}
				string strFunction = string( );
				string strAdmin = string( );
				if( !bOffer )
				{
					if( !bNoDownload && m_bAllowTorrentDownloads && ( pRequest->user.ucAccess & m_ucAccessDownTorrents ) )
					{
						strFunction += "<a title=\"" + strFileName + "\" class=\"download_link\" href=\"";
						strFunction += RESPONSE_STR_TORRENTS + cstrID + ".torrent\">" + gmapLANG_CFG["stats_download_torrent"] + "</a>";
						
						struct tm time_tm;
						int64 year, month, day, hour, minute, second;
						int64 passed;
						sscanf( vecQuery[3].c_str( ), "%d-%d-%d %d:%d:%d",&year,&month,&day,&hour,&minute,&second );
						time_tm.tm_year = year-1900;
						time_tm.tm_mon = month-1;
						time_tm.tm_mday = day;
						time_tm.tm_hour = hour;
						time_tm.tm_min = minute;
						time_tm.tm_sec = second;
						passed = (int64)difftime(now_t, mktime(&time_tm));

						if( ( pRequest->user.ucAccess & m_ucAccessReq ) && passed > m_uiDownloaderTimeOutInterval )
						{
							if( !vecQuery[23].empty( ) && vecQuery[23] == "1" )
							{
								if( atoi( vecQuery[26].c_str( ) ) > 0 )
								{
									strFunction += "<span class=\"pipe\"> | </span>";
									strFunction += "<a id=\"request" + cstrID + "\" class=\"noreq\" href=\"javascript: ;\" onclick=\"javascript: request('" + cstrID + "','false','" + gmapLANG_CFG["cancel"] + gmapLANG_CFG["section_reqseeders_success"] + "');\">";
									strFunction += gmapLANG_CFG["cancel"] + gmapLANG_CFG["section_reqseeders"];
									strFunction += "</a>";
									
								}
							}
							else
							{
								if( atoi( vecQuery[26].c_str( ) ) == 0 && strOldUploaderID != pRequest->user.strUID )
								{
									strFunction += "<span class=\"pipe\"> | </span>";
									strFunction += "<a id=\"request" + cstrID + "\" class=\"req\" href=\"javascript: ;\" onclick=\"javascript: request('" + cstrID + "','true','" + gmapLANG_CFG["section_reqseeders_success"] + "');\">";
									strFunction += gmapLANG_CFG["section_reqseeders"];
									strFunction += "</a>";
								}
							}
						}
					}
					if( pRequest->user.ucAccess & m_ucAccessBookmark )
					{
						if( !strFunction.empty( ) )
							strFunction += "<span class=\"pipe\"> | </span>";
						strFunction += "<a id=\"bookmark" + cstrID + "\" class=\"bookmark\" href=\"javascript: ;\" onclick=\"javascript: bookmark('" + cstrID + "','" + gmapLANG_CFG["stats_bookmark"] + "','" + gmapLANG_CFG["stats_no_bookmark"] + "');\">";
						CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bid FROM bookmarks WHERE buid=" +  pRequest->user.strUID + " AND bid=" + cstrID );
						if( pQuery->nextRow( ).size( ) == 1 )
							strFunction += gmapLANG_CFG["stats_no_bookmark"] + "</a>";
						else
							strFunction += gmapLANG_CFG["stats_bookmark"] + "</a>";
						delete pQuery;
					}
					if( pRequest->user.ucAccess & m_ucAccessComments )
					{
						if( !strFunction.empty( ) )
							strFunction += "<span class=\"pipe\"> | </span>";
						strFunction += "<a class=\"share\" target=\"_blank\" href=\"" + RESPONSE_STR_TALK_HTML + "?talk=" + UTIL_StringToEscaped( gmapLANG_CFG["share"] + "#" + gmapLANG_CFG["torrent"] + cstrID + "#" ) + "\">" + gmapLANG_CFG["share_to_talk"] + "</a>";
					}
				}
				else if( ( pRequest->user.ucAccess & m_ucAccessAllowOffers ) || ( ( pRequest->user.ucAccess & m_ucAccessUploadOffers ) && !pRequest->user.strUID.empty( ) && ( pRequest->user.strUID == strOldUploaderID ) ) )
				{
					strFunction += "<a title=\"" + strFileName + ".torrent" + "\" class=\"download_link\" href=\"";
					strFunction += RESPONSE_STR_OFFERS + cstrID + ".torrent";
					strFunction += "\">" + gmapLANG_CFG["stats_download_torrent"] + "</a>";
				}
				
				if( bOffer && ( pRequest->user.ucAccess & m_ucAccessAllowOffers ) )
				{
					strAdmin += "<a title=\"" + gmapLANG_CFG["allow"] + ": " + UTIL_RemoveHTML( strOldName ) + "\" class=\"black\" href=\"" + RESPONSE_STR_OFFER_HTML + "?allow=" + cstrID;
					if( !cstrReturnPage.empty( ) )
						strAdmin += "&amp;return=" + cstrReturnPageResp;
					strAdmin += "\">" + gmapLANG_CFG["stats_allow_offer"] + "</a>";
				}
				if( ( !bOffer && ( pRequest->user.ucAccess & m_ucAccessEditTorrents ) ) || ( bOffer && ( pRequest->user.ucAccess & m_ucAccessEditOffers ) ) || ( ( pRequest->user.ucAccess & m_ucAccessEditOwn ) && !pRequest->user.strUID.empty( ) && ( pRequest->user.strUID == strOldUploaderID ) ) )
				{
					if( !strAdmin.empty( ) )
						strAdmin += "<span class=\"pipe\"> | </span>";
					strAdmin += "<a title=\"" + gmapLANG_CFG["edit"] + ": " + UTIL_RemoveHTML( strOldName ) + "\" class=\"black\" href=\"" + RESPONSE_STR_STATS_HTML + strJoined + "&amp;action=edit";
					if( bOffer )
						strAdmin += "&amp;show=contents\">" + gmapLANG_CFG["stats_edit_offer"] + "</a>";
					else
						strAdmin += "&amp;show=contents\">" + gmapLANG_CFG["stats_edit_torrent"] + "</a>";
				}
				if( ( !bOffer && ( pRequest->user.ucAccess & m_ucAccessDelTorrents ) ) || ( bOffer && ( pRequest->user.ucAccess & m_ucAccessDelOffers ) ) || ( ( pRequest->user.ucAccess & m_ucAccessDelOwn ) && !pRequest->user.strUID.empty( ) && ( pRequest->user.strUID == strOldUploaderID ) ) )
				{
					if( !strAdmin.empty( ) )
						strAdmin += "<span class=\"pipe\"> | </span>";
					strAdmin += "<a title=\"" + gmapLANG_CFG["delete"] + ": " + UTIL_RemoveHTML( strOldName ) + "\" class=\"red\" href=\"";
					if( ( pRequest->user.ucAccess & m_ucAccessDelTorrents ) || ( pRequest->user.ucAccess & m_ucAccessDelOffers ) )
					{
						if( bOffer && ( pRequest->user.ucAccess & m_ucAccessDelOffers ) )
							strAdmin += RESPONSE_STR_OFFER_HTML + "?del=" + cstrID;
						else if( !bOffer && ( pRequest->user.ucAccess & m_ucAccessDelTorrents ) )
							strAdmin += RESPONSE_STR_INDEX_HTML + "?del=" + cstrID;
					}
					else if( pRequest->user.ucAccess & m_ucAccessDelOwn )
					{
						strAdmin += RESPONSE_STR_LOGIN_HTML;
						if( bOffer )
							strAdmin += "?odel=" + cstrID;
						else
							strAdmin += "?del=" + cstrID;
					}
					if( !cstrReturnPage.empty( ) )
						strAdmin += "&amp;return=" + cstrReturnPageResp;
					if( bOffer )
						strAdmin += "\">" + gmapLANG_CFG["stats_delete_offer"] + "</a>\n";
					else
						strAdmin += "\">" + gmapLANG_CFG["stats_delete_torrent"] + "</a>\n";
				}
				pResponse->strContent += strFunction;
				if( !strFunction.empty( ) && !strAdmin.empty( ) )
					pResponse->strContent += "<span class=\"pipe\"> | </span>";
				if( !strAdmin.empty( ) )
					pResponse->strContent += "( " + strAdmin + " )";
				pResponse->strContent += "</td>\n</tr>\n";
			}

			// needs work
			if( !bPost )
			{
				if( !strFileName.empty( ) )
				{
					pResponse->strContent += "<tr class=\"file_info\">";

					pResponse->strContent += "<th class=\"file_info\">" + gmapLANG_CFG["filename"] + ":</th>\n";
					pResponse->strContent += "<td class=\"file_info\">" + UTIL_RemoveHTML( strFileName ) + "</td>\n</tr>\n";
				}

				if( !vecQuery[2].empty( ) )
				{
					pResponse->strContent += "<tr class=\"file_info\">";

					pResponse->strContent += "<th class=\"file_info\">" + gmapLANG_CFG["name"] + ":</th>\n";
					pResponse->strContent += "<td class=\"file_info\">" + UTIL_RemoveHTML( vecQuery[2] ) + "</td>\n</tr>\n";
				}
				
				pResponse->strContent += "<tr class=\"file_info\">";
				pResponse->strContent += "<th class=\"file_info\">" + gmapLANG_CFG["file_info"] + ": </th>\n";
				pResponse->strContent += "<td class=\"file_info\">";

				if( !vecQuery[4].empty( ) )
				{
					// cache iSize
					iSize = UTIL_StringTo64( vecQuery[4].c_str( ) );
					
					pResponse->strContent += "<span class=\"file_info\">" + gmapLANG_CFG["size"] + ": </span>\n";
					pResponse->strContent += UTIL_BytesToString( iSize );
				}

				if( !vecQuery[5].empty( ) )
				{
					// cache uiFiles	 
					uiFiles = atoi( vecQuery[5].c_str( ) );

					pResponse->strContent += "<span class=\"pipe\"> | </span>";
					pResponse->strContent += "<span class=\"file_info\">" + gmapLANG_CFG["files"] + ": </span>\n";
					if( m_bShowFileContents && !pRequest->user.strUID.empty( ) )
					{
						pResponse->strContent += "<a class=\"contents\" href=\"" + RESPONSE_STR_STATS_HTML + strJoined;
						if( !cstrAction.empty( ) )
							pResponse->strContent += "&amp;action=" + cstrAction;
						pResponse->strContent += "&amp;show=contents#contents\">" + vecQuery[5] + "</a>";
					}
					else
						pResponse->strContent += vecQuery[5];
				}
				
				if( !cstrHashString.empty( ) )
				{
					pResponse->strContent += "<span class=\"pipe\"> | </span>";
					pResponse->strContent += "<span class=\"file_info\">" + gmapLANG_CFG["info_hash"] + ": </span>\n";
					pResponse->strContent += cstrHashString;
				}
				
				pResponse->strContent += "</td>\n</tr>\n";
			}
			
			pResponse->strContent += "<tr class=\"file_info\">";
			pResponse->strContent += "<th class=\"file_info\">" + gmapLANG_CFG["tag"] + ": </th>\n";
			pResponse->strContent += "<td class=\"file_info\">";

			if( !m_vecTags.empty( ) )
			{
				string strNameIndex = string( );
				string strTag = string( );
				for( vector< pair< string, string > > :: iterator it2 = m_vecTags.begin( ); it2 != m_vecTags.end( ); it2++ )
				{
					strNameIndex = (*it2).first;
					strTag = (*it2).second;
					if( strNameIndex == strCur )
					{
						if( bOffer )
							pResponse->strContent += "<a class=\"filter_by\" href=\"" + RESPONSE_STR_OFFER_HTML + "?tag=" + strNameIndex + "\">" + strTag + "</a>";
						else
							pResponse->strContent += "<a class=\"filter_by\" href=\"" + RESPONSE_STR_INDEX_HTML + "?tag=" + strNameIndex + "\">" + strTag + "</a>";
						break;
					}
				}
			}
			
			if( !vecQuery[3].empty( ) )
			{
				pResponse->strContent += "<span class=\"pipe\"> | </span>";
				pResponse->strContent += UTIL_Xsprintf( gmapLANG_CFG["added_by"].c_str( ), getUserLinkFull( strOldUploaderID, strOldUploader ).c_str( ), vecQuery[3].c_str( ) );
			}

			pResponse->strContent += "</td>\n</tr>\n";
//			pResponse->strContent += "<tr class=\"file_info\">";
//			pResponse->strContent += "<th class=\"file_info\">" + gmapLANG_CFG["uploader"] + ":</th>\n";
//			pResponse->strContent += "<td class=\"file_info\">";
//			
//			pResponse->strContent += getUserLinkFull( strOldUploaderID, strOldUploader );

//			pResponse->strContent += "</td>\n</tr>\n";
			if( !bOffer && !bPost )
			{
				pResponse->strContent += "<tr class=\"file_info\">";
				pResponse->strContent += "<th class=\"file_info\">" + gmapLANG_CFG["stats_live"] + ":</th>\n";
				pResponse->strContent += "<td class=\"file_info\">";
				if( ( atoi( vecQuery[26].c_str( ) ) > 0 || atoi( vecQuery[27].c_str( ) ) > 0 ) && !pRequest->user.strUID.empty( ) )
				{
					pResponse->strContent += "<a class=\"active\" href=\"" + RESPONSE_STR_STATS_HTML + strJoined;
					pResponse->strContent += "&amp;show=active#seeders\">";
				}
				pResponse->strContent += gmapLANG_CFG["stats_active_seeders"] + "(" + vecQuery[26] + ")/" + gmapLANG_CFG["stats_active_leechers"] + "(" + vecQuery[27] + ")";
				if( ( atoi( vecQuery[26].c_str( ) ) > 0 || atoi( vecQuery[27].c_str( ) ) > 0 ) && !pRequest->user.strUID.empty( ) )
					pResponse->strContent += "</a>";
				pResponse->strContent += "<span class=\"pipe\"> | </span>";
				
				if( atoi( vecQuery[28].c_str( ) ) > 0 && !pRequest->user.strUID.empty( ) )
				{
					pResponse->strContent += "<a href=\"" + RESPONSE_STR_STATS_HTML + strJoined;
					pResponse->strContent += "&amp;show=completes#completes\">";
				}
				pResponse->strContent += gmapLANG_CFG["stats_completes"] + " (" + vecQuery[28] + ")";
				if( atoi( vecQuery[28].c_str( ) ) > 0 && !pRequest->user.strUID.empty( ) )
					pResponse->strContent += "</a>";
				if( !pRequest->user.strUID.empty( ) )
					pResponse->strContent += " <span class=\"italic\">" + UTIL_Xsprintf( gmapLANG_CFG["stats_last_active"].c_str( ), vecQuery[29].c_str( ) ) + "</span>";

				pResponse->strContent += "</td>\n</tr>\n";
			}
			
			if( !bPost )
			{
				if( !strOldIMDbID.empty( ) )
				{
					pResponse->strContent += "<tr class=\"file_info\">";
					pResponse->strContent += "<th class=\"file_info\">" + gmapLANG_CFG["imdb"] + ":</th>\n";
					pResponse->strContent += "<td class=\"file_info\">";
					pResponse->strContent += "<a target=\"_blank\" href=\"" + gmapLANG_CFG["imdb_url"] + strOldIMDbID + "/\">";
					if( !strOldIMDb.empty( ) )
						pResponse->strContent += strOldIMDb;
					else
						pResponse->strContent += gmapLANG_CFG["na"];
					pResponse->strContent += "</a>";
					pResponse->strContent += " <span class=\"italic\">" + UTIL_Xsprintf( gmapLANG_CFG["stats_last_imdb_updated"].c_str( ), strOldIMDbUpdated.c_str( ) ) + "</span>";
					pResponse->strContent += "</td>\n</tr>\n";
					
					pResponse->strContent += "<tr class=\"file_info\">";
					pResponse->strContent += "<th class=\"file_info\">" + gmapLANG_CFG["version"] + ":</th>\n";
					pResponse->strContent += "<td class=\"file_info_version\">";
//					CMySQLQuery *pQueryIMDb = new CMySQLQuery( "SELECT bid,bname,badded,bsize,btag,btitle,bdefault_down,bdefault_up,bfree_down,bfree_up,UNIX_TIMESTAMP(bfree_to),bseeders,bleechers FROM allowed_cache WHERE bimdbid=\'" + UTIL_StringToMySQL( strOldIMDbID ) + "\' AND bid!=" + cstrID + " ORDER BY badded" );
					CMySQLQuery *pQueryIMDb = new CMySQLQuery( "SELECT bid,bname,badded,bsize,btag,btitle,bdefault_down,bdefault_up,bfree_down,bfree_up,UNIX_TIMESTAMP(bfree_to),bseeders,bleechers FROM allowed WHERE bimdbid=\'" + UTIL_StringToMySQL( strOldIMDbID ) + "\' AND bid!=" + cstrID + " ORDER BY badded" );
					
					vector<string> vecQueryIMDb;
			
					vecQueryIMDb.reserve(13);

					vecQueryIMDb = pQueryIMDb->nextRow( );
					
					if( vecQueryIMDb.size( ) == 13 )
					{
						pResponse->strContent += UTIL_Xsprintf( gmapLANG_CFG["result_x_found"].c_str( ), CAtomInt( pQueryIMDb->numRows( ) ).toString( ).c_str( ) );
						pResponse->strContent += " <a class=\"index_filter\" href=\"javascript: ;\" onclick=\"javascript: this.innerHTML=change('stats_version','" + gmapLANG_CFG["version_show"] + "','" + gmapLANG_CFG["version_hide"] + "')\">";
						pResponse->strContent += "<span class=\"blue\">" + gmapLANG_CFG["version_show"] + "</span></a>";
						pResponse->strContent += "<table class=\"stats_version\" id=\"stats_version\" style=\"display: none\">\n";
						
						pResponse->strContent += "<tr>\n";
						
						if( !m_vecTags.empty( ) )
						{
							pResponse->strContent += "<th class=\"tag\" id=\"tagheader\">" + gmapLANG_CFG["tag"];
							pResponse->strContent += "</th>\n";
						}
						
						// Name
						pResponse->strContent += "<th class=\"name\" id=\"nameheader\">" + gmapLANG_CFG["name"];

						pResponse->strContent += "</th>\n";
			
						// <th> seeders

						pResponse->strContent += "<th class=\"number\" id=\"seedersheader\">" + gmapLANG_CFG["seeders"];

						pResponse->strContent += "</th>\n";

						// <th> leechers

						pResponse->strContent += "<th class=\"number\" id=\"leechersheader\">" + gmapLANG_CFG["leechers"];

						pResponse->strContent += "</th>\n";
						
						
						// <th> added

						pResponse->strContent += "<th class=\"date\" id=\"addedheader\">" + gmapLANG_CFG["added"];

						pResponse->strContent += "</th>\n";

						// <th> size

						pResponse->strContent += "<th class=\"bytes\" id=\"sizeheader\">" + gmapLANG_CFG["size"];

						pResponse->strContent += "</th>\n</tr>\n";
						
						string strName = string( );
						string strEngName = string( );
						string strChiName = string( );
						
						int64 iDefaultDown = 100;
						int64 iDefaultUp = 100;
						int64 iFreeDown = 100;
						int64 iFreeUp = 100;
						int64 iFreeTo = 0;
						
						while( vecQueryIMDb.size( ) == 13 )
						{
							pResponse->strContent += "<tr>";
						
							// <td> tag
							
							vector< pair< string, string > > :: iterator it2 = m_vecTagsMouse.begin( );
							if( !m_vecTags.empty( ) )
							{
								pResponse->strContent += "<td class=\"tag\">\n";

								string strNameIndex = string( );
								string strTag = string( );

								for( vector< pair< string, string > > :: iterator it1 = m_vecTags.begin( ); it1 != m_vecTags.end( ); it1++ )
								{
									strNameIndex = (*it1).first;
									strTag = (*it1).second;
									if( strNameIndex == vecQueryIMDb[4] )
									{

										if( !(*it2).second.empty( ) )

											// Sets a TITLE parameter for each Tag Image, so that a TOOLTIP box will popup when
											// the user's mouse pointer hovers over the Tag Image.

											pResponse->strContent += "<img src=\"" + (*it2).second + "\" alt=\"[" + strTag + "]\" title=\"" + strTag + "\" name=\"" + strTag + "\">";

										break;
									}
									it2++;

								}
							}

							pResponse->strContent += "</td>\n";

							iDefaultDown = atoi( vecQueryIMDb[6].c_str( ) );
							iDefaultUp = atoi( vecQueryIMDb[7].c_str( ) );
							iFreeDown = atoi( vecQueryIMDb[8].c_str( ) );
							iFreeUp = atoi( vecQueryIMDb[9].c_str( ) );
							iFreeTo = UTIL_StringTo64( vecQueryIMDb[10].c_str( ) );
			
							if( bFreeGlobal )
							{
								if( iFreeDownGlobal < iDefaultDown )
									iDefaultDown = iFreeDownGlobal;
								if( iFreeUpGlobal > iDefaultUp )
									iDefaultUp = iFreeUpGlobal;
							}
				
							int64 day_left = -1, hour_left = -1, minute_left = -1;
							
							time_t tTimeFree = iFreeTo - now_t;
				
							if( tTimeFree > 0 )
							{
								tTimeFree += 60;
								tTimeFree /= 60;
								minute_left =  tTimeFree % 60;
								tTimeFree /= 60;
								hour_left =  tTimeFree % 24;
								tTimeFree /= 24;
								day_left =  tTimeFree;
							}

//							if( iFreeTo > now_t )
//							{
//								day_left = ( iFreeTo - now_t + 60 ) / 86400;
//								hour_left = ( iFreeTo - now_t + 60 ) % 86400 / 3600;
//								minute_left = ( iFreeTo - now_t + 60 ) % 3600 / 60;
//							}
							if( iFreeTo > 0 && day_left >= 0 )
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
							
							// <td> name
							strName = vecQueryIMDb[1];
							if( !vecQueryIMDb[5].empty( ) )
								strName = vecQueryIMDb[5];
							pResponse->strContent += "<td class=\"torrent_name_version\">\n";
							strEngName.erase( );
							strChiName.erase( );
							UTIL_StripName( strName.c_str( ), strEngName, strChiName );
							if( iFreeDown == 0 )
								pResponse->strContent += "<a class=\"stats_free\" title=\"";
							else
								pResponse->strContent += "<a class=\"stats\" title=\"";
							pResponse->strContent += gmapLANG_CFG["name"] + ": " + UTIL_RemoveHTML( strName )+ "\" href=\"" + RESPONSE_STR_STATS_HTML + "?id=" + vecQueryIMDb[0] + "\">";
							pResponse->strContent += UTIL_RemoveHTML( strEngName );
							if( !strChiName.empty( ) )
								pResponse->strContent += "<br>" + UTIL_RemoveHTML( strChiName );
							pResponse->strContent += "</a>";
							
							if( iFreeDown != 100 || iFreeUp != 100 )
							{
								if( iFreeDown != 100 )
								{
									if( iDefaultDown == 0 && !( bFreeGlobal && iFreeDownGlobal == 0 ) )
										pResponse->strContent += "<span class=\"free_down_free\" title=\"" + gmapLANG_CFG["free_down_free"] + "\">" + gmapLANG_CFG["free_down_free_short"] + "</span>";
									else if( iFreeDown > 0 )
										pResponse->strContent += "<span class=\"free_down\" title=\"" + UTIL_Xsprintf( gmapLANG_CFG["free_down"].c_str( ), CAtomInt( iFreeDown ).toString( ).c_str( ) ) + "\">" + UTIL_Xsprintf( gmapLANG_CFG["free_down_short"].c_str( ), CAtomInt( iFreeDown ).toString( ).c_str( ) )+ "</span>";
								}
								if( iFreeUp != 100 )
									pResponse->strContent += "<span class=\"free_up\" title=\"" + UTIL_Xsprintf( gmapLANG_CFG["free_up"].c_str( ), CAtomInt( iFreeUp ).toString( ).c_str( ) ) + "\"> " + UTIL_Xsprintf( gmapLANG_CFG["free_up_short"].c_str( ), CAtomInt( iFreeUp ).toString( ).c_str( ) )+ "</span>";
								if( day_left >= 0 && ( iDefaultDown > iFreeDown || iDefaultUp < iFreeUp ) )
								{
									pResponse->strContent += "<span class=\"free_recover\" title=\"" + gmapLANG_CFG["free_recover"];
									pResponse->strContent += UTIL_Xsprintf( gmapLANG_CFG["free_down"].c_str( ), CAtomInt( iDefaultDown ).toString( ).c_str( ) ) + UTIL_Xsprintf( gmapLANG_CFG["free_up"].c_str( ), CAtomInt( iDefaultUp ).toString( ).c_str( ) ) + "\">";
										if( day_left > 0 )
											pResponse->strContent += UTIL_Xsprintf( gmapLANG_CFG["free_day_left"].c_str( ), CAtomInt( day_left ).toString( ).c_str( ), CAtomInt( hour_left ).toString( ).c_str( ) );
										else if( hour_left > 0 )
											pResponse->strContent += UTIL_Xsprintf( gmapLANG_CFG["free_hour_left"].c_str( ), CAtomInt( hour_left ).toString( ).c_str( ), CAtomInt( minute_left ).toString( ).c_str( ) );
										else if( minute_left >= 0 )
											pResponse->strContent += UTIL_Xsprintf( gmapLANG_CFG["free_minute_left"].c_str( ), CAtomInt( minute_left ).toString( ).c_str( ) );
									pResponse->strContent += "</span>";
								}
							}
							
							pResponse->strContent += "</td>\n";
							
							// <td> seeders
							
							pResponse->strContent += "<td class=\"number\">";
							pResponse->strContent += vecQueryIMDb[11];
							pResponse->strContent += "</td>\n";
							
							// <td> leechers
							
							pResponse->strContent += "<td class=\"number\">";
							pResponse->strContent += vecQueryIMDb[12];
							pResponse->strContent += "</td>\n";
							
							// <td> added

	// 						if( m_bShowAdded )
	// 						{
								pResponse->strContent += "<td class=\"date\">";

								if( !vecQueryIMDb[2].empty( ) )
								{
									const string :: size_type br = vecQueryIMDb[2].find( ' ' );
									pResponse->strContent += vecQueryIMDb[2].substr( 0, br ) + "<br>" +  vecQueryIMDb[2].substr( br + 1 );
									// strip year and seconds from time
	// 								pResponse->strContent += pTorrents[ulKey].strAdded.substr( 5, pTorrents[ulKey].strAdded.size( ) - 8 );
								}

								pResponse->strContent += "</td>\n";
	// 						}

							// <td> size

							if( m_bShowSize )
							{
								const string :: size_type br = UTIL_BytesToString( UTIL_StringTo64( vecQueryIMDb[3].c_str( ) ) ).find( ' ' );
								pResponse->strContent += "<td class=\"bytes\">" + UTIL_BytesToString( UTIL_StringTo64( vecQueryIMDb[3].c_str( ) ) ).substr( 0, br ) + "<br>" + UTIL_BytesToString( UTIL_StringTo64( vecQueryIMDb[3].c_str( ) ) ).substr( br + 1 ) + "</td>\n";
	// 							pResponse->strContent += "<td class=\"bytes\">" + UTIL_BytesToString( pTorrents[ulKey].iSize ) + "</td>\n";
							}
							pResponse->strContent += "</tr>\n";

							vecQueryIMDb = pQueryIMDb->nextRow( );
						}
						pResponse->strContent += "</table>\n";
					}
					else
						pResponse->strContent += gmapLANG_CFG["result_none_found"];
					delete pQueryIMDb;
					pResponse->strContent += "</td>\n</tr>\n";
				}
			}
			
			if( !bOffer && !bPost && !pRequest->user.strUID.empty( ) )
			{
				pResponse->strContent += "<div id=\"subs\">";
				CMySQLQuery *pQuerySub = new CMySQLQuery( "SELECT bid,buid,busername,bsub,bfilename,bname FROM subs WHERE btid=" + cstrID + " ORDER BY buploadtime" );
				
				vector<string> vecQuerySub;
		
				vecQuerySub.reserve(6);

				vecQuerySub = pQuerySub->nextRow( );
				
				if( vecQuerySub.size( ) == 6 )
				{
					pResponse->strContent += "<tr class=\"file_info\">";
					pResponse->strContent += "<th class=\"file_info\">" + gmapLANG_CFG["subs"] + ":</th>\n";
					pResponse->strContent += "<td class=\"file_info_subs\">";
					pResponse->strContent += "<table class=\"subs\">\n";
					
					while( vecQuerySub.size( ) == 6 )
					{
						pResponse->strContent += "<tr class=\"subs\">\n<td class=\"subs\">\n";
						if( !vecQuerySub[5].empty( ) )
							pResponse->strContent += "<span id=\"sub" + vecQuerySub[0] + "\">" + UTIL_RemoveHTML( vecQuerySub[5] ) + "</span>: ";
						pResponse->strContent += "<a href=\"" + vecQuerySub[3] + "\">" + UTIL_RemoveHTML( vecQuerySub[4] ) + "</a> " + getUserLink( vecQuerySub[1], vecQuerySub[2] );
						if( ( vecQuerySub[1] == pRequest->user.strUID && ( pRequest->user.ucAccess & m_ucAccessEditOwn ) ) || ( pRequest->user.ucAccess & m_ucAccessEditTorrents ) )
							pResponse->strContent += " [<a href=\"javascript: edit_sub_confirm(\'" + cstrID + "\',\'" + vecQuerySub[0] + "\',\'" + gmapLANG_CFG["stats_sub_edit"] + "\',\'" + gmapLANG_CFG["stats_sub_edit_error"] + "\');\">" +  gmapLANG_CFG["edit"] + "</a>]";
						if( ( vecQuerySub[1] == pRequest->user.strUID && ( pRequest->user.ucAccess & m_ucAccessDelOwn ) ) || ( pRequest->user.ucAccess & m_ucAccessEditTorrents ) )
							pResponse->strContent += " [<a href=\"javascript: delete_sub_confirm(\'" + cstrID + "\',\'" + vecQuerySub[0] + "\');\">" +  gmapLANG_CFG["delete"] + "</a>]";
						pResponse->strContent += "</td></tr>";
						vecQuerySub = pQuerySub->nextRow( );
					}

					pResponse->strContent += "</table>\n</td>\n</tr>\n";
				}
				
				delete pQuerySub;
				
				pResponse->strContent += "</div>";
				
				if( pRequest->user.ucAccess & m_ucAccessUploadOffers )
				{


					pResponse->strContent += "<tr class=\"file_info\">";
					pResponse->strContent += "<th class=\"file_info\">" + gmapLANG_CFG["upload_sub"] + ":</th>\n";
					pResponse->strContent += "<td class=\"file_info\">";
					pResponse->strContent += "<iframe src=\"" + RESPONSE_STR_SUB_UPLOAD_HTML + "?id=" + cstrID + "\" frameborder=\"0\" scrolling=\"no\"></iframe>";
					pResponse->strContent += "</td>\n</tr>\n";
				}
			}
			
			pResponse->strContent += "<tr class=\"file_info\">";
			pResponse->strContent += "<th class=\"file_info\">" + gmapLANG_CFG["intr"] + ":</th>\n";
			pResponse->strContent += "<td class=\"file_info\">";

			if( !strOldIntr.empty( ) )
				pResponse->strContent += UTIL_RemoveHTML2( strOldIntr ) + "</td>\n</tr>\n";
			else
				pResponse->strContent += UTIL_RemoveHTML( strOldName ) + "</td>\n</tr>\n";
				
			if( !pRequest->user.strUID.empty( ) && !bPost && !bOffer )
			{
				CMySQLQuery *pQuery = new CMySQLQuery( "SELECT buploaderid FROM allowed WHERE bid=" + cstrID );

				vector<string> vecQuery;

				vecQuery.reserve(1);

				vecQuery = pQuery->nextRow( );
	
				delete pQuery;
				

				pResponse->strContent += "<tr class=\"file_info\">";
				pResponse->strContent += "<th class=\"file_info\">" + gmapLANG_CFG["thanker"] + ":</th>\n";
				pResponse->strContent += "<td class=\"file_info\">";
				
				CMySQLQuery *pQueryThanks = new CMySQLQuery( "SELECT bthankerid,bthanker FROM thanks WHERE bid=" + cstrID + " ORDER BY bthanktime DESC" );

				vector<string> vecQueryThanks;

				vecQueryThanks.reserve(2);

				vecQueryThanks = pQueryThanks->nextRow( );
				
				unsigned int uiCount = 0;
				bool bThanked = false;
				
				while( vecQueryThanks.size( ) == 2 )
				{
					if( !vecQueryThanks[0].empty( ) && vecQueryThanks[0] == pRequest->user.strUID )
						bThanked = true;
						
					if( uiCount > 0 )
						pResponse->strContent += ", ";
					pResponse->strContent += getUserLink( vecQueryThanks[0], vecQueryThanks[1] );
					
					vecQueryThanks = pQueryThanks->nextRow( );
					uiCount++;
				}

				delete pQueryThanks;
				
				pResponse->strContent += "\n<form name=\"thanks\" id=\"thanksForm\" method=\"get\" action=\"" + string( RESPONSE_STR_STATS_HTML ) + "\">\n";
				pResponse->strContent += "<input name=\"id\" type=hidden value=\"" + cstrID + "\">\n";
				pResponse->strContent += "<input name=\"thanks\" type=hidden value=\"1\">\n";
				pResponse->strContent += "<input name=\"submit\" type=button onclick=\"javascript: saythanks( 'thanksForm', 'thanks' );\" value=\"";
				if( vecQuery.size( ) == 1 && vecQuery[0] == pRequest->user.strUID )
				{
					pResponse->strContent += gmapLANG_CFG["thanks_yourself"];
					pResponse->strContent += "\" disabled";
				}
				else if( bThanked )
				{
					pResponse->strContent += gmapLANG_CFG["thanks_again"];
					pResponse->strContent += "\" disabled";
				}
				else
					pResponse->strContent += UTIL_Xsprintf( gmapLANG_CFG["thanks"].c_str( ), CFG_GetInt( "bnbt_thanks_bonus", 3 ) ) + "\"";
				pResponse->strContent += ">\n";
				if( uiCount > 0 )
					pResponse->strContent += UTIL_Xsprintf( gmapLANG_CFG["thanks_count"].c_str( ), uiCount );
				else
					pResponse->strContent += gmapLANG_CFG["thanks_no"];
				pResponse->strContent += "</form>\n</td>\n</tr>\n";
			}

			pResponse->strContent += "</table>\n</div>\n\n";

			if( m_bShowFileComment && !bPost )
			{
				const string cstrFileComment( vecQuery[5] );

				if( !cstrFileComment.empty( ) )
				{
					pResponse->strContent += "<div class=\"stats_table\">\n";
					pResponse->strContent += "<p class=\"file_comment\">" + gmapLANG_CFG["file_comment"] + "</p>\n";
					pResponse->strContent += "<table summary=\"file comment\">\n";
					pResponse->strContent += "<tr class=\"file_comment\">\n<td class=\"file_comment\">\n" + cstrFileComment;
					pResponse->strContent += "</td>\n</tr>\n</table>\n</div>\n\n";
				}
			}

#if defined ( XBNBT_GD )
			// Dynstat Image Link
			// Show the link if user has Uploader or Edit privaledge
			if( m_bDynstatShowLink && m_bDynstatGenerate && !m_strDynstatFontFile.empty( ) && !m_strDynstatLinkURL.empty( ) && ( pRequest->user.ucAccess & ACCESS_UPLOAD || pRequest->user.ucAccess & ACCESS_EDIT ) )
			{
				if( m_ucDynstatSaveMode == IMAGE_BY_FILENAME )
				{
					pResponse->strContent += "<span class=\"pipe\">|</span><a title=\"" + strFileName + m_strImageOutExt + "\" class=\"dynstat_link\" href=\"";
					pResponse->strContent += m_strDynstatLinkURL + RESPONSE_STR_SEPERATOR + UTIL_StringToEscapedStrict( strFileName ) + m_strImageOutExt;
				}
				else
				{
					// by info hash
					pResponse->strContent += "<span class=\"pipe\">|</span><a title=\"" + cstrHashString  +  m_strImageOutExt + "\" class=\"dynstat_link\" href=\"";
					pResponse->strContent += m_strDynstatLinkURL + RESPONSE_STR_SEPERATOR + cstrHashString  +  m_strImageOutExt;
				}

				pResponse->strContent += "\">" + gmapLANG_CFG["stats_dynstat_link"] + "</a>\n";
			}
#endif

			const string cstrShow( pRequest->mapParams["show"] );

// 			if( m_bShowFileContents && uiFiles > 1 && !pRequest->user.strLogin.empty( ) )
			if( m_bShowFileContents && !pRequest->user.strUID.empty( ) && !bPost )
			{
				if( !cstrShow.empty( ) && cstrShow == "contents" )
				{
					CAtom *pDecoded = 0;
					if( bOffer )
						pDecoded = DecodeFile( ( m_strOfferDir + strFileName ).c_str( ) );
					else
						pDecoded = DecodeFile( ( m_strAllowedDir + strFileName ).c_str( ) );

					if( pDecoded && pDecoded->isDicti( ) )
					{
						CAtom *pInfo = ( (CAtomDicti *)pDecoded )->getItem( "info" );

						if( pInfo && pInfo->isDicti( ) )
						{
							CAtom *pFiles = ( (CAtomDicti *)pInfo )->getItem( "files" );

							if( pFiles && dynamic_cast<CAtomList *>( pFiles ) )
							{
								bool bFound = false;

								unsigned int uiAdded = 0;


								vector<CAtom *> *pvecFiles = dynamic_cast<CAtomList *>( pFiles )->getValuePtr( );

								CAtom *pPath = 0;
								CAtom *pLen = 0;

								string strPath = string( );

								vector<CAtom *> *pvecPath = 0;

								for( vector<CAtom *> :: iterator it = pvecFiles->begin( ); it != pvecFiles->end( ); it++ )
								{
									if( (*it)->isDicti( ) )
									{
										pPath = ( (CAtomDicti *)(*it) )->getItem( "path" );
										pLen = ( (CAtomDicti *)(*it) )->getItem( "length" );

										if( pPath && dynamic_cast<CAtomList *>( pPath ) )
										{
											if( !bFound )
											{
												pResponse->strContent += "<div class=\"stats_table\">\n";
												pResponse->strContent += "<p class=\"contents\" id=\"contents\">" + gmapLANG_CFG["contents"] + "</p>\n";
												pResponse->strContent += "<table summary=\"contents\">\n<tr>\n";
												pResponse->strContent += "<th class=\"path\">" + gmapLANG_CFG["file"] + "</th>\n";

												if( pLen && dynamic_cast<CAtomLong *>( pLen ) )
													pResponse->strContent += "<th class=\"bytes\">" + gmapLANG_CFG["size"] + "</th>\n";

												pResponse->strContent += "</tr>\n";

												bFound = true;
											}
	
											strPath = string( );

											pvecPath = dynamic_cast<CAtomList *>( pPath )->getValuePtr( );

											for( vector<CAtom *> :: iterator it2 = pvecPath->begin( ); it2 != pvecPath->end( ); it2++ )
											{
												if( it2 != pvecPath->begin( ) )
													strPath += '/';

												strPath += (*it2)->toString( );
											}

											if( !strPath.empty( ) )
											{
//												if( uiAdded % 2 )
//													pResponse->strContent += "<tr class=\"even\">\n";
//												else
													pResponse->strContent += "<tr class=\"odd\">\n";

												pResponse->strContent += "<td class=\"path\">" + UTIL_RemoveHTML( strPath ) + "</td>\n";

												if( pLen && dynamic_cast<CAtomLong *>( pLen ) )
													pResponse->strContent += "<td class=\"bytes\">" + UTIL_BytesToString( dynamic_cast<CAtomLong *>( pLen )->getValue( ) ) + "</td>";

												pResponse->strContent += "</tr>\n";

												uiAdded++;
											}
										}
									}
								}

								if( bFound )
									pResponse->strContent += "</table>\n</div>\n\n";
							}
							else
							{
								CAtom *pName = 0;
								CAtom *pLen = 0;
								
								pName = ( (CAtomDicti *)pInfo )->getItem( "name" );
								pLen = ( (CAtomDicti *)pInfo )->getItem( "length" );
								
								if( pName )
								{
									pResponse->strContent += "<div class=\"stats_table\">\n";
									pResponse->strContent += "<p class=\"contents\" id=\"contents\">" + gmapLANG_CFG["contents"] + "</p>\n";
									pResponse->strContent += "<table summary=\"contents\">\n<tr>\n";
									pResponse->strContent += "<th class=\"path\">" + gmapLANG_CFG["file"] + "</th>\n";
									
									if( pLen && dynamic_cast<CAtomLong *>( pLen ) )
										pResponse->strContent += "<th class=\"bytes\">" + gmapLANG_CFG["size"] + "</th>\n";

									pResponse->strContent += "</tr>\n";
									
									pResponse->strContent += "<tr class=\"odd\">\n";
									
									pResponse->strContent += "<td class=\"path\">" + UTIL_RemoveHTML( pName->toString( ) ) + "</td>\n";

									if( pLen && dynamic_cast<CAtomLong *>( pLen ) )
										pResponse->strContent += "<td class=\"bytes\">" + UTIL_BytesToString( dynamic_cast<CAtomLong *>( pLen )->getValue( ) ) + "</td>";

									pResponse->strContent += "</tr>\n";
									
									pResponse->strContent += "</table>\n</div>\n\n";
								}
							}
						}
					}

					if( pDecoded )
						delete pDecoded;
				}
				else
				{
					pResponse->strContent += "<div class=\"stats_table\">\n";
					pResponse->strContent += "<p><a class=\"contents\" href=\"" + RESPONSE_STR_STATS_HTML + strJoined;
					pResponse->strContent += "&amp;show=contents#contents\">" + gmapLANG_CFG["contents"] + "</a></p></div>\n";
				}
					
			}

			bool bNoComment = false;
			if( !bOffer )
			{
				if( !vecQuery[25].empty( ) && vecQuery[25] == "1" )
					bNoComment = true;
			}
			
			pResponse->strContent += "<div id=\"divComment\" class=\"comments_table_posted\">\n";
			
			CMySQLQuery *pQueryComment = new CMySQLQuery( "SELECT bid,busername,buid,bip,bposted,bcomment FROM comments WHERE " + strIDKey + "=" + cstrID + " ORDER BY bposted" );
			
			vector<string> vecQueryComment;
		
			vecQueryComment.reserve(6);

			vecQueryComment = pQueryComment->nextRow( );
			
			unsigned long ulCount = 0;

			if( vecQueryComment.size( ) == 6 )
			{
				pResponse->strContent += "<p class=\"comments_table_posted\"><a title=\"" + gmapLANG_CFG["comments_page"] + "\" href=\"" + RESPONSE_STR_COMMENTS_HTML;
				pResponse->strContent += strJoined;
				pResponse->strContent += "\">" + gmapLANG_CFG["comments_page"];
				pResponse->strContent += "</a></p>\n";
				pResponse->strContent += "<p>" + gmapLANG_CFG["comments"] + "</p>\n";
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

					iStart = strIP.rfind( "." );

					if( iStart != string :: npos )
					{
						// don't strip ip for mods

						if( !( pRequest->user.ucAccess & m_ucAccessShowIP ) && pRequest->user.strUID != strNameID )
							strIP = strIP.substr( 0, iStart + 1 ) + "xxx";
					}
					else
					{
						iStart = strIP.rfind( ":" );
						if( iStart != string :: npos )
						{
							// don't strip ip for mods

							if( !( pRequest->user.ucAccess & m_ucAccessShowIP ) && pRequest->user.strUID != strNameID )
								strIP = strIP.substr( 0, iStart + 1 ) + "xxxx";
						}
					}

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
					pResponse->strContent += UTIL_Xsprintf( gmapLANG_CFG["comments_posted_by"].c_str( ), strID.c_str( ), strUserLink.c_str( ), strIP.c_str( ), strTime.c_str( ) );
				
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
				pResponse->strContent += "\" type=hidden value=\"" + cstrID+ "\">\n";
				if( !cstrReturnPage.empty( ) )
					pResponse->strContent += "<input name=\"return\" type=hidden value=\"" + cstrReturnPage + "\">\n";
//				if( bOffer )
//					pResponse->strContent += "<input name=\"offer\" type=hidden value=\"1\">\n";

				pResponse->strContent += "<tr class=\"comments_table_post\">\n";
				pResponse->strContent += "<td class=\"comments_table_post\">\n";
				pResponse->strContent += "<textarea id=\"commentarea\" name=\"comment\" rows=8 cols=64 onKeyDown=\"javascript: keypost(event,'submit_comment');\"></textarea>";
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
						pResponse->strContent += " checked";
					pResponse->strContent += "> <label for=\"id_message\">" + gmapLANG_CFG["comments_send_message"] + "</label> \n";
				}
				pResponse->strContent += "</div>\n";
				pResponse->strContent += "</td>\n</tr>\n";
				pResponse->strContent += "</table>\n";
				pResponse->strContent += "</form>\n";
				
				pResponse->strContent += "</div>\n";
			}
			else
				pResponse->strContent += "<p class=\"denied\">" + gmapLANG_CFG["comments_post_disallowed"] + "</p>\n";


			// Get a list of peers for the info hash
			CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bpeerid,buseragent,busername,buid,bip,bip6,bport,buploaded,bdownloaded,bleft,UNIX_TIMESTAMP(bconnected),UNIX_TIMESTAMP(bupdated),bupspeed,bdownspeed FROM dstate WHERE bid=" +  cstrID );
			
			vector<string> vecQuery;
			
			vecQuery.reserve(14);

			vecQuery = pQuery->nextRow( );
			
			unsigned long ulKeySize = (unsigned long)pQuery->numRows( );

			if( !cstrShow.empty( ) && cstrShow == "active" && !pRequest->user.strUID.empty( ) )
			{
				// add the peers into this structure one by one and sort it afterwards
				
				struct peer_t *pPeersT = new struct peer_t[pQuery->numRows( )];

				unsigned long ulCount = 0;

				// Loop through the peers map to obtain the list of peers and their information by peerid
				while( vecQuery.size( ) == 14 )
				{
					// Initialise peer entry variables
					pPeersT[ulCount].iUpped = 0;
					pPeersT[ulCount].iDowned = 0;
					pPeersT[ulCount].iLeft = 0;
					pPeersT[ulCount].ulConnected = 0;
					pPeersT[ulCount].ulUpdated = 0;
					pPeersT[ulCount].ulUpSpeed = 0;
					pPeersT[ulCount].ulDownSpeed = 0;
					pPeersT[ulCount].flShareRatio = 0.0;
					pPeersT[ulCount].strIP = string( );
					pPeersT[ulCount].strIP6 = string( );
					pPeersT[ulCount].strUserAgent = string( );
					pPeersT[ulCount].strClientType = string( );
					pPeersT[ulCount].bClientTypeIdentified = false;

					// Get the peer entry values

					// PeerID
					pPeersT[ulCount].strPeerID = vecQuery[0];

					if( m_ucShowPeerInfo != 0 )
					{
						// User-Agent
						if( !vecQuery[1].empty( ) )
							pPeersT[ulCount].strUserAgent = vecQuery[1];

						// Set the peer identification
						if( !pPeersT[ulCount].bClientTypeIdentified )
						{
							UTIL_GetClientIdentity( pPeersT[ulCount].strPeerID, pPeersT[ulCount].strUserAgent, pPeersT[ulCount].strClientType, pPeersT[ulCount].bClientTypeIdentified );
						}
					}
					
					if( !vecQuery[2].empty( ) )
						pPeersT[ulCount].strUsername = vecQuery[2];
						
					if( !vecQuery[3].empty( ) )
						pPeersT[ulCount].strUID = vecQuery[3];

					// Get the peer IP address
					if( !vecQuery[4].empty( ) )
					{
						pPeersT[ulCount].strIP += vecQuery[4];
						
						// cstrIP ip

						string :: size_type iStart = 0;
						iStart = pPeersT[ulCount].strIP.rfind( "." );

						if( iStart != string :: npos )
						{
							// don't cstrIP ip for mods

							if( !( pRequest->user.ucAccess & m_ucAccessShowIP ) && pRequest->user.strUID != pPeersT[ulCount].strUID )
								pPeersT[ulCount].strIP = pPeersT[ulCount].strIP.substr( 0, iStart + 1 ) + "xxx"; 
						}
					}
						
					if( !vecQuery[5].empty( ) )
					{
						pPeersT[ulCount].strIP6 += vecQuery[5];

						// cstrIP ip

						string :: size_type iStart = 0;
						iStart = pPeersT[ulCount].strIP6.rfind( ":" );
						if( iStart != string :: npos )
						{
							// don't cstrIP ip for mods

							if( !( pRequest->user.ucAccess & m_ucAccessShowIP ) && pRequest->user.strUID != pPeersT[ulCount].strUID )
								pPeersT[ulCount].strIP6 = pPeersT[ulCount].strIP6.substr( 0, iStart + 1 ) + "xxxx";
						}
					}

					// Uploaded
					if( !vecQuery[7].empty( ) )
						pPeersT[ulCount].iUpped = UTIL_StringTo64( vecQuery[7].c_str( ) );

					// Downloaded
					if( !vecQuery[8].empty( ) )
					{
						pPeersT[ulCount].iDowned = UTIL_StringTo64( vecQuery[8].c_str( ) );

						if( m_bShowShareRatios )
						{
							if( pPeersT[ulCount].iDowned > 0 )
								pPeersT[ulCount].flShareRatio = (float)pPeersT[ulCount].iUpped / (float)pPeersT[ulCount].iDowned;
							else if( pPeersT[ulCount].iUpped == 0 )
								pPeersT[ulCount].flShareRatio = 0.0;
							else
								pPeersT[ulCount].flShareRatio = -1.0;
						}
					}

					// Left
					if( !vecQuery[9].empty( ) )
						pPeersT[ulCount].iLeft = UTIL_StringTo64( vecQuery[9].c_str( ) );

					// Connected
					if( !vecQuery[10].empty( ) )
						pPeersT[ulCount].ulConnected = GetTime( ) - (unsigned long)UTIL_StringTo64( vecQuery[10].c_str( ) );
					
					// Updated
					if( !vecQuery[11].empty( ) )
						pPeersT[ulCount].ulUpdated = GetTime( ) - (unsigned long)UTIL_StringTo64( vecQuery[11].c_str( ) );

					


					// Upload speed
					if( !vecQuery[12].empty( ) )
						pPeersT[ulCount].ulUpSpeed = UTIL_StringTo64( vecQuery[12].c_str( ) );
					
					// Download speed
					if( !vecQuery[13].empty( ) )
						pPeersT[ulCount].ulDownSpeed = UTIL_StringTo64( vecQuery[13].c_str( ) );

					ulCount++;
					
					vecQuery = pQuery->nextRow( );
				}

				// Were we requested to sort the information?
				const string strSort( pRequest->mapParams["sort"] );

				if( m_bSort )
				{
					if( !strSort.empty( ) )
					{
						const unsigned char cucSort( (unsigned char)atoi( strSort.c_str( ) ) );

						switch( cucSort )
						{
						case SORTP_AUPPED:
							qsort( pPeersT, ulKeySize, sizeof( struct peer_t ), asortpByUpped );
							break;
						case SORTP_ADOWNED:
							qsort( pPeersT, ulKeySize, sizeof( struct peer_t ), asortpByDowned );
							break;
						case SORTP_ALEFT:
							qsort( pPeersT, ulKeySize, sizeof( struct peer_t ), asortpByLeft );
							break;
						case SORTP_ACONNECTED:
							qsort( pPeersT, ulKeySize, sizeof( struct peer_t ), asortpByConnected );
							break;

						case SORTP_ASHARERATIO:
							if( m_bShowShareRatios )
								qsort( pPeersT, ulKeySize, sizeof( struct peer_t ), asortpByShareRatio );
							break;
						case SORTP_ACLIENT:
							if( m_ucShowPeerInfo )
								qsort( pPeersT, ulKeySize, sizeof( struct peer_t ), asortpByClient );
							break;
						case SORTP_AIP:
							if( m_ucShowPeerInfo )
								qsort( pPeersT, ulKeySize, sizeof( struct peer_t ), asortpByIP );
							break;
						case SORTP_AUR:
							if( m_ucShowPeerInfo )
								qsort( pPeersT, ulKeySize, sizeof( struct peer_t ), asortpByUR );
							break;
						case SORTP_ADR:
							if( m_ucShowPeerInfo )
								qsort( pPeersT, ulKeySize, sizeof( struct peer_t ), asortpByDR );
							break;
//						case SORTP_DUPPED:
//							qsort( pPeersT, ulKeySize, sizeof( struct peer_t ), dsortpByUpped );
//							break;
						case SORTP_DDOWNED:
							qsort( pPeersT, ulKeySize, sizeof( struct peer_t ), dsortpByDowned );
							break;
						case SORTP_DLEFT:
							qsort( pPeersT, ulKeySize, sizeof( struct peer_t ), dsortpByLeft );
							break;
						case SORTP_DCONNECTED:
							qsort( pPeersT, ulKeySize, sizeof( struct peer_t ), dsortpByConnected );
							break;
						case SORTP_DSHARERATIO:
							if( m_bShowShareRatios )
								qsort( pPeersT, ulKeySize, sizeof( struct peer_t ), dsortpByShareRatio );
							break;
						case SORTP_DCLIENT:
							if( m_ucShowPeerInfo )
								qsort( pPeersT, ulKeySize, sizeof( struct peer_t ), dsortpByClient );
							break;
						case SORTP_DIP:
							if( m_ucShowPeerInfo )
								qsort( pPeersT, ulKeySize, sizeof( struct peer_t ), dsortpByIP );
							break;
						case SORTP_DUR:
							if( m_ucShowPeerInfo )
								qsort( pPeersT, ulKeySize, sizeof( struct peer_t ), dsortpByUR );
							break;
						case SORTP_DDR:
							if( m_ucShowPeerInfo )
								qsort( pPeersT, ulKeySize, sizeof( struct peer_t ), dsortpByDR );
							break;
						case SORTP_DUPPED:
						default:
							qsort( pPeersT, ulKeySize, sizeof( struct peer_t ), dsortpByUpped );
// 							qsort( pPeersT, ulKeySize, sizeof( struct peer_t ), asortpByLeft );
						}
					}
					else
					{
						// default action is to sort by left

						qsort( pPeersT, ulKeySize, sizeof( struct peer_t ), dsortpByUpped );
// 						qsort( pPeersT, ulKeySize, sizeof( struct peer_t ), asortpByLeft );
					}
				}
				else
				{
					// sort is disabled, but default action is to sort by left

					qsort( pPeersT, ulKeySize, sizeof( struct peer_t ), dsortpByUpped );
// 					qsort( pPeersT, ulKeySize, sizeof( struct peer_t ), asortpByLeft );
				}

				bool bFound = false;

				unsigned int uiAdded = 0;
				
				string :: size_type iCountGoesHere = string :: npos;
				//
				// seeders
				//
				
				pResponse->strContent += "<div class=\"stats_table\">\n";
				
				pResponse->strContent += "<p class=\"seeders\" id=\"seeders\">" + gmapLANG_CFG["stats_seeders"] + "</p>\n";

				// to save on calculations, we're going to insert the number of seeders later, keep track of where

				iCountGoesHere = pResponse->strContent.size( ) - ( sizeof( "</p>\n" ) - 1 );

				char szFloat[16];

				for( unsigned long it = 0; it < ulKeySize; it++ )
				{
					if( uiAdded >= m_uiMaxPeersDisplay )
						break;

					if( pPeersT[it].iLeft == 0 )
					{
						if( !bFound )
						{
							vector< pair< string, string > > vecParams;
							vecParams.reserve(64);
							string strJoined = string( );
			
							vecParams.push_back( pair<string, string>( string( "return" ), cstrReturnPage ) );
							vecParams.push_back( pair<string, string>( string( "show" ), string( "active" ) ) );
					
							strJoined = UTIL_RemoveHTML( UTIL_HTMLJoin( vecParams, string( "&" ), string( "&" ), string( "=" ) ) ) + "#seeders";
							// output table headers

							pResponse->strContent += "<table summary=\"seeders\">\n";
							pResponse->strContent += "<tr><th class=\"uploader\">\n" + gmapLANG_CFG["user_name"] + "</th>";;
							pResponse->strContent += "<th class=\"ip\">";

							if( m_bSort )
							{
								pResponse->strContent += "<a class=\"table_header\" href=\"" + RESPONSE_STR_STATS_HTML + "?id=" + cstrID;

								pResponse->strContent += "&amp;sort=";
								if( strSort == SORTPSTR_AIP )
									pResponse->strContent += SORTPSTR_DIP;
								else
									pResponse->strContent += SORTPSTR_AIP;
								pResponse->strContent += strJoined;
								pResponse->strContent += "\">" + gmapLANG_CFG["peer_ip"] + "</a>";
							}

							pResponse->strContent += "</th>\n";

							if( m_ucShowPeerInfo != 0 )
							{
								pResponse->strContent += "<th class=\"client\">";

								if( m_bSort )
								{
									pResponse->strContent += "<a class=\"table_header\" href=\"" + RESPONSE_STR_STATS_HTML + "?id=" + cstrID;

									pResponse->strContent += "&amp;sort=";
									if( strSort == SORTPSTR_ACLIENT )
										pResponse->strContent += SORTPSTR_DCLIENT;
									else
										pResponse->strContent += SORTPSTR_ACLIENT;
									pResponse->strContent += strJoined;
									pResponse->strContent += "\">" + gmapLANG_CFG["client"] + "</a>";
								}

								pResponse->strContent += "</th>\n";
							}

							pResponse->strContent += "<th class=\"bytes\">";

							if( m_bSort )
							{
								pResponse->strContent += "<a class=\"table_header\" href=\"" + RESPONSE_STR_STATS_HTML + "?id=" + cstrID;

								pResponse->strContent += "&amp;sort=";
								if( strSort == SORTPSTR_DUPPED || strSort.empty( ) )
									pResponse->strContent += SORTPSTR_AUPPED;
								else
									pResponse->strContent += SORTPSTR_DUPPED;
								pResponse->strContent += strJoined;
								pResponse->strContent += "\">" + gmapLANG_CFG["uploaded"] + "</a>";
							}

							pResponse->strContent += "</th>\n";
							pResponse->strContent += "<th class=\"bytes\">";

							if( m_bSort )
							{
								pResponse->strContent += "<a class=\"table_header\" href=\"" + RESPONSE_STR_STATS_HTML + "?id=" + cstrID;

								pResponse->strContent += "&amp;sort=";
								if( strSort == SORTPSTR_DDOWNED )
									pResponse->strContent += SORTPSTR_ADOWNED;
								else
									pResponse->strContent += SORTPSTR_DDOWNED;
								pResponse->strContent += strJoined;
								pResponse->strContent += "\">" + gmapLANG_CFG["downloaded"] + "</a>";
							}

							pResponse->strContent += "</th>\n";
							pResponse->strContent += "<th class=\"connected\">";

							if( m_bSort )
							{
								pResponse->strContent += "<a class=\"table_header\" href=\"" + RESPONSE_STR_STATS_HTML + "?id=" + cstrID;

								pResponse->strContent += "&amp;sort=";
								if( strSort == SORTPSTR_DCONNECTED )
									pResponse->strContent += SORTPSTR_ACONNECTED;
								else
									pResponse->strContent += SORTPSTR_DCONNECTED;
								pResponse->strContent += strJoined;
								pResponse->strContent += "\">" + gmapLANG_CFG["connected"] + "</a>";
							}
						
							pResponse->strContent += "<th class=\"updated\">" + gmapLANG_CFG["updated"];

							pResponse->strContent += "</th>\n";

							if( m_bShowShareRatios )
							{
								pResponse->strContent += "<th class=\"number\">";

								if( m_bSort )
								{
									pResponse->strContent += "<a class=\"table_header\" href=\"" + RESPONSE_STR_STATS_HTML + "?id=" + cstrID;

									pResponse->strContent += "&amp;sort=";
									if( strSort == SORTPSTR_DSHARERATIO )
										pResponse->strContent += SORTPSTR_ASHARERATIO;
									else
										pResponse->strContent += SORTPSTR_DSHARERATIO;
									pResponse->strContent += strJoined;
									pResponse->strContent += "\">" + gmapLANG_CFG["share_ratio"] + "</a>";
								}

								pResponse->strContent += "</th>\n";
							}

							if( m_bShowAvgULRate )
							{
								pResponse->strContent += "<th class=\"bytes\">";

								if( m_bSort )
								{
									pResponse->strContent += "<a class=\"table_header\" href=\"" + RESPONSE_STR_STATS_HTML + "?id=" + cstrID;

									pResponse->strContent += "&amp;sort=";
									if( strSort == SORTPSTR_DUR )
										pResponse->strContent += SORTPSTR_AUR;
									else
										pResponse->strContent += SORTPSTR_DUR;
									pResponse->strContent += strJoined;
									pResponse->strContent += "\">" + gmapLANG_CFG["avg_up_rate"] + "</a>";
								}

								pResponse->strContent += "</th>\n";
							}

							pResponse->strContent += "</tr>\n";

							bFound = true;
						}

						// output table rows

//						if( uiAdded % 2 )
						if( pPeersT[it].strUID == pRequest->user.strUID )
							pResponse->strContent += "<tr class=\"own\">\n";
						else if( !pPeersT[it].strIP6.empty( ) )
							pResponse->strContent += "<tr class=\"even\">\n";
						else
							pResponse->strContent += "<tr class=\"odd\">\n";
						
//						if( pPeersT[it].strUID == pRequest->user.strUID )
//							pResponse->strContent += "<td class=\"uploader\"><a class=\"username\"";
//						else
						pResponse->strContent += "<td class=\"uploader\">";
						pResponse->strContent += getUserLink( pPeersT[it].strUID, pPeersT[it].strUsername );

						pResponse->strContent += "</td>\n";
						pResponse->strContent += "<td class=\"ip\">";
						if( !pPeersT[it].strIP.empty( ) )
						{
							pResponse->strContent += pPeersT[it].strIP;
							if( !pPeersT[it].strIP6.empty( ) )
								pResponse->strContent += " | ";
						}
						if( !pPeersT[it].strIP6.empty( ) )
							pResponse->strContent += pPeersT[it].strIP6;
						pResponse->strContent += "</td>\n";

						// Peer client identification
						if( m_ucShowPeerInfo != 0 )
						{
							pResponse->strContent += "<td class=\"client\">\n";

							if( m_ucShowPeerInfo != 0 && !pPeersT[it].strClientType.empty( ) )
								pResponse->strContent += pPeersT[it].strClientType + "\n" ;

							if( m_ucShowPeerInfo == 2 || ( m_ucShowPeerInfo == 1 && pPeersT[it].strClientType.empty( ) ) )
							{
								// Add a <br>
								if( !pPeersT[it].strUserAgent.empty( ) && !pPeersT[it].strClientType.empty( ) )
									pResponse->strContent += "<br>\n";

								// User-Agent
								if( !pPeersT[it].strUserAgent.empty( ) )
									pResponse->strContent += "Agent: " + UTIL_StringToDisplay( pPeersT[it].strUserAgent + "\n" );

								// Add a <br>
								if( !pPeersT[it].strUserAgent.empty( ) && !pPeersT[it].strPeerID.empty( ) )
									pResponse->strContent += "<br>\n";

								// Peer_ID
								if( !pPeersT[it].strPeerID.empty( ) )
									pResponse->strContent += "ID: " + UTIL_StringToDisplay( pPeersT[it].strPeerID + "\n" );
							}

							pResponse->strContent += "</td>\n";
						}

						pResponse->strContent += "<td class=\"bytes\">" + UTIL_BytesToString( pPeersT[it].iUpped ) + "</td>\n";
						pResponse->strContent += "<td class=\"bytes\">" + UTIL_BytesToString( pPeersT[it].iDowned ) + "</td>\n";
						pResponse->strContent += "<td class=\"connected\">" + UTIL_SecondsToString( pPeersT[it].ulConnected ) + "</td>\n";
						pResponse->strContent += "<td class=\"updated\">" + UTIL_SecondsToString( pPeersT[it].ulUpdated ) + "</td>\n";

						if( m_bShowShareRatios )
						{
							pResponse->strContent += "<td class=\"number_";

							if( ( -1.001 < pPeersT[it].flShareRatio ) && ( pPeersT[it].flShareRatio < -0.999 ) )
								pResponse->strContent += "blue\">";
							else if( pPeersT[it].flShareRatio < 0.800 )
								pResponse->strContent += "red\">";
							else if( pPeersT[it].flShareRatio < 1.200 )
								pResponse->strContent += "green\">";
							else
								pResponse->strContent += "blue\">";


							// turn the share ratio into a string

							if( ( -1.001 < pPeersT[it].flShareRatio ) && ( pPeersT[it].flShareRatio < -0.999 ) )
								pResponse->strContent += gmapLANG_CFG["perfect"];
							else
							{
								memset( szFloat, 0, sizeof( szFloat ) / sizeof( char ) );
								snprintf( szFloat, sizeof( szFloat ) / sizeof( char ), "%0.3f", pPeersT[it].flShareRatio );

								pResponse->strContent += szFloat;
							}

							pResponse->strContent += "</td>\n";
						}
						
						// Average upload
						if( m_bShowAvgULRate )
						{
							pResponse->strContent += "<td class=\"bytes\">";

							if( pPeersT[it].ulConnected > 0 )
								pResponse->strContent += UTIL_BytesToString( pPeersT[it].ulUpSpeed ) + RESPONSE_STR_SEPERATOR + gmapLANG_CFG["sec"];
							else
								pResponse->strContent += gmapLANG_CFG["na"];

							pResponse->strContent += "</td>\n";
						}

//						if( m_bShowAvgULRate )
//						{
//							pResponse->strContent += "<td class=\"bytes\">";

//							if( pPeersT[it].ulConnected > 0 )
//								pResponse->strContent += UTIL_BytesToString( pPeersT[it].iUpped / pPeersT[it].ulConnected ) + RESPONSE_STR_SEPERATOR + gmapLANG_CFG["sec"];
//							else
//								pResponse->strContent += gmapLANG_CFG["na"];

//							pResponse->strContent += "</td>\n";
//						}

						pResponse->strContent += "</tr>\n";

						uiAdded++;
					}
				}

				// insert the number of seeders
				string strTemp = " (" + CAtomInt( uiAdded ).toString( ) + ")";

				if( bFound )
				{
					pResponse->strContent += "</table>\n";
					pResponse->strContent += "</div>\n\n";
				}

				if( iCountGoesHere != string :: npos )
					pResponse->strContent.insert( iCountGoesHere, strTemp );

				iCountGoesHere = string :: npos;

				bFound = false;


				uiAdded = 0;

				//
				// leechers
				//
				
				if( ( m_bSort && strSort.empty( ) ) || !m_bSort )
					qsort( pPeersT, ulKeySize, sizeof( struct peer_t ), asortpByLeft );

				unsigned char ucPercent = 0;
				
				pResponse->strContent += "<div class=\"stats_table\">\n";
				pResponse->strContent += "<p class=\"leechers\" id=\"leechers\">" + gmapLANG_CFG["stats_leechers"] + "</p>\n";

				// to save on calculations, we're going to insert the number of leechers later, keep track of where

				iCountGoesHere = pResponse->strContent.size( ) - ( sizeof( "</p>\n" ) - 1 );

				for( unsigned long it = 0; it < ulKeySize; it++ )
				{
					if( uiAdded >= m_uiMaxPeersDisplay )
						break;

					if( pPeersT[it].iLeft != 0 )
					{
						if( !bFound )
						{
							vector< pair< string, string > > vecParams;
							vecParams.reserve(64);
							string strJoined = string( );
			
							vecParams.push_back( pair<string, string>( string( "return" ), cstrReturnPage ) );
							vecParams.push_back( pair<string, string>( string( "show" ), string( "active" ) ) );
					
							strJoined = UTIL_RemoveHTML( UTIL_HTMLJoin( vecParams, string( "&" ), string( "&" ), string( "=" ) ) ) + "#leechers";
							// output table headers

							// Leechers
							pResponse->strContent += "<table summary=\"leechers\">\n";
							pResponse->strContent += "<tr>\n";
							pResponse->strContent += "<th class=\"uploader\">" + gmapLANG_CFG["user_name"] + "</th>";
							pResponse->strContent += "<th class=\"ip\">";

							if( m_bSort )
							{
								pResponse->strContent += "<a class=\"table_header\" href=\"" + RESPONSE_STR_STATS_HTML + "?id=" + cstrID;

								pResponse->strContent += "&amp;sort=";
								if( strSort == SORTPSTR_AIP )
									pResponse->strContent += SORTPSTR_DIP;
								else
									pResponse->strContent += SORTPSTR_AIP;
								pResponse->strContent += strJoined;
								pResponse->strContent += "\">" + gmapLANG_CFG["peer_ip"] + "</a>";
							}

							pResponse->strContent += "</th>\n";

							// Client information
							if( m_ucShowPeerInfo != 0 )
							{
								pResponse->strContent += "<th class=\"client\">";

								if( m_bSort )
								{
									pResponse->strContent += "<a class=\"table_header\" href=\"" + RESPONSE_STR_STATS_HTML + "?id=" + cstrID;

									pResponse->strContent += "&amp;sort=";
									if( strSort == SORTPSTR_ACLIENT )
										pResponse->strContent += SORTPSTR_DCLIENT;
									else
										pResponse->strContent += SORTPSTR_ACLIENT;
									pResponse->strContent += strJoined;
									pResponse->strContent += "\">" + gmapLANG_CFG["client"] + "</a>";
								}

								pResponse->strContent += "</th>\n";
							}

							// Uploaded
							pResponse->strContent += "<th class=\"bytes\">";

							if( m_bSort )
							{
								pResponse->strContent += "<a class=\"table_header\" href=\"" + RESPONSE_STR_STATS_HTML + "?id=" + cstrID;

								pResponse->strContent += "&amp;sort=";
								if( strSort == SORTPSTR_DUPPED )
									pResponse->strContent += SORTPSTR_AUPPED;
								else
									pResponse->strContent += SORTPSTR_DUPPED;
								pResponse->strContent += strJoined;
								pResponse->strContent += "\">" + gmapLANG_CFG["uploaded"] + "</a>";
							}

							// Downloaded
							pResponse->strContent += "</th>\n";
							pResponse->strContent += "<th class=\"bytes\">";

							if( m_bSort )
							{
								pResponse->strContent += "<a class=\"table_header\" href=\"" + RESPONSE_STR_STATS_HTML + "?id=" + cstrID;

								pResponse->strContent += "&amp;sort=";
								if( strSort == SORTPSTR_DDOWNED )
									pResponse->strContent += SORTPSTR_ADOWNED;
								else
									pResponse->strContent += SORTPSTR_DDOWNED;
								pResponse->strContent += strJoined;
								pResponse->strContent += "\">" + gmapLANG_CFG["downloaded"] + "</a>";
							}

							// Left as progress indicator
							if( m_bShowLeftAsProgress )
							{
								pResponse->strContent += "</th>\n";
								pResponse->strContent += "<th class=\"percent\">";
// 								pResponse->strContent += "<th class=\"bytes\">" + gmapLANG_CFG["progress"];
							}
							else
							{
								pResponse->strContent += "</th>\n";
								pResponse->strContent += "<th class=\"bytes\">";
// 								pResponse->strContent += "<th class=\"bytes\">" + gmapLANG_CFG["left"];
							}

							if( m_bSort )
							{
								pResponse->strContent += "<a class=\"table_header\" href=\"" + RESPONSE_STR_STATS_HTML + "?id=" + cstrID;

								pResponse->strContent += "&amp;sort=";
								if( strSort == SORTPSTR_ALEFT || strSort.empty( ) )
									pResponse->strContent += SORTPSTR_DLEFT;
								else
									pResponse->strContent += SORTPSTR_ALEFT;
								pResponse->strContent += strJoined;
								pResponse->strContent += "\">";
								if( m_bShowLeftAsProgress )
									pResponse->strContent += gmapLANG_CFG["progress"];
								else
									pResponse->strContent += gmapLANG_CFG["left"];
								pResponse->strContent += "</a>";
							}

							// Connected time
							pResponse->strContent += "</th>\n";
							pResponse->strContent += "<th class=\"connected\">";

							if( m_bSort )
							{
								pResponse->strContent += "<a class=\"table_header\" href=\"" + RESPONSE_STR_STATS_HTML + "?id=" + cstrID;

								pResponse->strContent += "&amp;sort=";
								if( strSort == SORTPSTR_DCONNECTED )
									pResponse->strContent += SORTPSTR_ACONNECTED;
								else
									pResponse->strContent += SORTPSTR_DCONNECTED;
								pResponse->strContent += strJoined;
								pResponse->strContent += "\">" + gmapLANG_CFG["connected"] + "</a>";
							}

							pResponse->strContent += "</th>\n";
							pResponse->strContent += "<th class=\"updated\">" + gmapLANG_CFG["updated"];
							pResponse->strContent += "</th>\n";

							// Share ratios
							if( m_bShowShareRatios )
							{
								pResponse->strContent += "<th class=\"number\">";

								if( m_bSort )
								{
									pResponse->strContent += "<a class=\"table_header\" href=\"" + RESPONSE_STR_STATS_HTML + "?id=" + cstrID;

									pResponse->strContent += "&amp;sort=";
									if( strSort == SORTPSTR_DSHARERATIO )
										pResponse->strContent += SORTPSTR_ASHARERATIO;
									else
										pResponse->strContent += SORTPSTR_DSHARERATIO;
									pResponse->strContent += strJoined;
									pResponse->strContent += "\">" + gmapLANG_CFG["share_ratio"] + "</a>";
								}

								pResponse->strContent += "</th>\n";
							}

							// Average upload rate
							if( m_bShowAvgULRate )
							{
								pResponse->strContent += "<th class=\"bytes\">";

								if( m_bSort )
								{
									pResponse->strContent += "<a class=\"table_header\" href=\"" + RESPONSE_STR_STATS_HTML + "?id=" + cstrID;

									pResponse->strContent += "&amp;sort=";
									if( strSort == SORTPSTR_DUR )
										pResponse->strContent += SORTPSTR_AUR;
									else
										pResponse->strContent += SORTPSTR_DUR;
									pResponse->strContent += strJoined;
									pResponse->strContent += "\">" + gmapLANG_CFG["avg_up_rate"] + "</a>";
								}

								pResponse->strContent += "</th>\n";
							}

							// Average download rate
							if( m_bShowAvgDLRate )
							{
								pResponse->strContent += "<th class=\"bytes\">";

								if( m_bSort )
								{
									pResponse->strContent += "<a class=\"table_header\" href=\"" + RESPONSE_STR_STATS_HTML + "?id=" + cstrID;

									pResponse->strContent += "&amp;sort=";
									if( strSort == SORTPSTR_DDR )
										pResponse->strContent += SORTPSTR_ADR;
									else
										pResponse->strContent += SORTPSTR_DDR;
									pResponse->strContent += strJoined;
									pResponse->strContent += "\">" + gmapLANG_CFG["avg_down_rate"] + "</a>";
								}

								pResponse->strContent += "</th>\n";
							}

							pResponse->strContent += "</tr>\n";

							bFound = true;
						}
						
						// output table rows

//						if( uiAdded % 2 )
						if( pPeersT[it].strUID == pRequest->user.strUID )
							pResponse->strContent += "<tr class=\"own\">\n";
						else if( !pPeersT[it].strIP6.empty( ) )
							pResponse->strContent += "<tr class=\"even\">\n";
						else
							pResponse->strContent += "<tr class=\"odd\">\n";
						
//						if( pPeersT[it].strUID == pRequest->user.strUID )
//							pResponse->strContent += "<td class=\"uploader\"><a class=\"username\"";
//						else
						pResponse->strContent += "<td class=\"uploader\">";
						pResponse->strContent += getUserLink( pPeersT[it].strUID, pPeersT[it].strUsername );

						pResponse->strContent += "</td>\n";

						// IP addresses
						pResponse->strContent += "<td class=\"ip\">";
						if( !pPeersT[it].strIP.empty( ) )
						{
							pResponse->strContent += pPeersT[it].strIP;
							if( !pPeersT[it].strIP6.empty( ) )
								pResponse->strContent += " | ";
						}

						if( !pPeersT[it].strIP6.empty( ) )
							pResponse->strContent += pPeersT[it].strIP6;

						// Peer client identification
						if( m_ucShowPeerInfo != 0 )
						{
							pResponse->strContent += "<td class=\"client\">\n";

							if( m_ucShowPeerInfo != 0 && !pPeersT[it].strClientType.empty( ) )
								pResponse->strContent += pPeersT[it].strClientType + "\n" ;

							if( m_ucShowPeerInfo == 2 || ( m_ucShowPeerInfo == 1 && pPeersT[it].strClientType.empty( ) ) )
							{
								// Add a <br>
								if( !pPeersT[it].strUserAgent.empty( ) && !pPeersT[it].strClientType.empty( ) )
									pResponse->strContent += "<br>\n";

								// User-Agent
								if( !pPeersT[it].strUserAgent.empty( ) )
									pResponse->strContent += "Agent: " + UTIL_StringToDisplay( pPeersT[it].strUserAgent + "\n" );

								// Add a <br>
								if( !pPeersT[it].strUserAgent.empty( ) && !pPeersT[it].strPeerID.empty( ) )
									pResponse->strContent += "<br>\n";

								// Peer_ID
								if( !pPeersT[it].strPeerID.empty( ) )
									pResponse->strContent += "ID: " + UTIL_StringToDisplay( pPeersT[it].strPeerID + "\n" );
							}

							pResponse->strContent += "</td>\n";
						}

						// Uploaded
						pResponse->strContent += "<td class=\"bytes\">" + UTIL_BytesToString( pPeersT[it].iUpped ) + "</td>\n";

						// Downloaded
						pResponse->strContent += "<td class=\"bytes\">" + UTIL_BytesToString( pPeersT[it].iDowned ) + "</td>\n";

						// Percentage done/left
						pResponse->strContent += "<td class=\"percent\">";

						if( m_bShowLeftAsProgress )
							pResponse->strContent += UTIL_BytesToString( iSize - pPeersT[it].iLeft );
						else
							pResponse->strContent += UTIL_BytesToString( pPeersT[it].iLeft );

						pResponse->strContent += " (";

						ucPercent = 0;

						if( iSize > 0 )
						{
							if( m_bShowLeftAsProgress )
								ucPercent = (unsigned char)( 100 - (unsigned char)( ( (float)pPeersT[it].iLeft / iSize ) * 100 ) );
							else
								ucPercent = (unsigned char)( ( (float)pPeersT[it].iLeft / iSize ) * 100 );
						}

						pResponse->strContent += CAtomInt( ucPercent ).toString( ) + "%)";

						if( !imagefill.strFile.empty( ) && !imagetrans.strFile.empty( ) )
						{
							pResponse->strContent += "<br>\n";

							string strFillFileName = imagefill.strName;

							if( strFillFileName.empty( ) )
								strFillFileName = "imagebarfill.png";

							if( ucPercent > 0 )
							{
								if( !imagefill.strURL.empty( ) )
									pResponse->strContent += "<img class=\"percent\" src=\"" + imagefill.strURL + strFillFileName + "\" width=" + CAtomInt( ucPercent ).toString( ) + " height=8 alt=\"[" + gmapLANG_CFG["completed"] + "]\" name=\"Completed\" title=\"" + gmapLANG_CFG["completed"] + "\">";
								else if( m_bServeLocal )
									pResponse->strContent += "<img class=\"percent\" src=\"" + strFillFileName + "\" width=" + CAtomInt( ucPercent ).toString( ) + " height=8 alt=\"[" + gmapLANG_CFG["completed"] + "]\" name=\"Completed\" title=\"" + gmapLANG_CFG["completed"] + "\">";
							}

							string strTransFileName = imagetrans.strName;

							if( strTransFileName.empty( ) )
								strTransFileName = "imagebartrans.png";

							if( ucPercent < 100 )
							{
								if( !imagetrans.strURL.empty( ) )
									pResponse->strContent += "<img class=\"percent\" src=\"" + imagetrans.strURL + strTransFileName + "\" width=" + CAtomInt( 100 - ucPercent ).toString( ) + " height=8 alt=\"[" + gmapLANG_CFG["remaining"] + "]\" name=\"Remaining\" title=\"" + gmapLANG_CFG["remaining"] + "\">";
								else if( m_bServeLocal )
									pResponse->strContent += "<img class=\"percent\" src=\"" + strTransFileName + "\" width=" + CAtomInt( 100 - ucPercent ).toString( ) + " height=8 alt=\"[" + gmapLANG_CFG["remaining"] + "]\" name=\"Remaining\" title=\"" + gmapLANG_CFG["remaining"] + "\">";
							}
						}


						pResponse->strContent += "</td>\n";

						// Connected
						pResponse->strContent += "<td class=\"connected\">" + UTIL_SecondsToString( pPeersT[it].ulConnected ) + "</td>\n";
						
						// Updated
						pResponse->strContent += "<td class=\"updated\">" + UTIL_SecondsToString( pPeersT[it].ulUpdated ) + "</td>\n";

						// Share ratios
						if( m_bShowShareRatios )
						{
							pResponse->strContent += "<td class=\"number_";

							if( ( -1.001 < pPeersT[it].flShareRatio ) && ( pPeersT[it].flShareRatio < -0.999 ) )
								pResponse->strContent += "blue\">";
							else if( pPeersT[it].flShareRatio < 0.800 )
								pResponse->strContent += "red\">";
							else if( pPeersT[it].flShareRatio < 1.200 )
								pResponse->strContent += "green\">";
							else
								pResponse->strContent += "blue\">";

							// turn the share ratio into a string

							if( ( -1.001 < pPeersT[it].flShareRatio ) && ( pPeersT[it].flShareRatio < -0.999 ) )
								pResponse->strContent += gmapLANG_CFG["perfect"];
							else
							{
								memset( szFloat, 0, sizeof( szFloat ) / sizeof( char ) );
								snprintf( szFloat, sizeof( szFloat ) / sizeof( char ), "%0.3f", pPeersT[it].flShareRatio );

								pResponse->strContent += szFloat;
							}

							pResponse->strContent += "</td>\n";
						}

						// Average upload
						if( m_bShowAvgULRate )
						{
							pResponse->strContent += "<td class=\"bytes\">";

							if( pPeersT[it].ulConnected > 0 )
								pResponse->strContent += UTIL_BytesToString( pPeersT[it].ulUpSpeed ) + RESPONSE_STR_SEPERATOR + gmapLANG_CFG["sec"];
							else
								pResponse->strContent += gmapLANG_CFG["na"];

							pResponse->strContent += "</td>\n";
						}

						// Average download
						if( m_bShowAvgDLRate )
						{
							pResponse->strContent += "<td class=\"bytes\">";

							if( pPeersT[it].ulConnected > 0 )
								pResponse->strContent += UTIL_BytesToString( pPeersT[it].ulDownSpeed ) + RESPONSE_STR_SEPERATOR + gmapLANG_CFG["sec"];
							else
								pResponse->strContent += gmapLANG_CFG["na"];

							pResponse->strContent += "</td>\n";
						}

						pResponse->strContent += "</tr>\n";

						uiAdded++;
					}
				}

				// insert the number of leechers
				
				strTemp = " (" + CAtomInt( uiAdded ).toString( ) + ")";

				if( bFound )
				{
					pResponse->strContent += "</table>\n";
					pResponse->strContent += "</div>\n\n";
				}

				if( iCountGoesHere != string :: npos )
					pResponse->strContent.insert( iCountGoesHere, strTemp );

				delete [] pPeersT;
			}
			delete pQuery;

			CMySQLQuery *pQueryCompleted = new CMySQLQuery( "SELECT busername,buid,bcompleted FROM peers WHERE bid=" +  cstrID + " AND bcompleted!=0 ORDER BY bcompleted DESC" );

			vector<string> vecQueryCompleted;
			
			vecQueryCompleted.reserve(3);

			vecQueryCompleted = pQueryCompleted->nextRow( );
			
			if( vecQueryCompleted.size( ) == 3 )
			{
				if( !cstrShow.empty( ) && cstrShow == "completes" && !pRequest->user.strUID.empty( ) )
				{
					pResponse->strContent += "<div class=\"stats_table\">\n";
					pResponse->strContent += "<p class=\"completes\" id=\"completes\">" + gmapLANG_CFG["stats_completes"] + " (" + CAtomInt( pQueryCompleted->numRows( ) ).toString( ) + ")</p>\n";
					
					pResponse->strContent += "<table summary=\"completes\">\n";
					pResponse->strContent += "<tr>\n";
// 					pResponse->strContent += "<th class=\"ip\">" + gmapLANG_CFG["peer_ip"];
					pResponse->strContent += "<th class=\"uploader\">" + gmapLANG_CFG["user_name"] + "</th>";
					pResponse->strContent += "<th class=\"date\">" + gmapLANG_CFG["user_completed"] +"</th>";
					pResponse->strContent += "</tr>\n";
					
					unsigned int uiAdded = 0;
					
					while( vecQueryCompleted.size( ) == 3 )
					{
//						if( uiAdded % 2 )
//							pResponse->strContent += "<tr class=\"even\">\n";
//						else
						string strUsername = vecQueryCompleted[0];
						string strUID = vecQueryCompleted[1];
						string strDate = vecQueryCompleted[2];
						
						if( strUID == pRequest->user.strUID )
							pResponse->strContent += "<tr class=\"own\">\n";
						else
							pResponse->strContent += "<tr class=\"odd\">\n";
						
// 							iStart = strCompletedIP.rfind( "." );
// 
// 							if( iStart != string :: npos )
// 							{
// 								// don't strip ip for mods
// 								if( !( pRequest->user.ucAccess & ACCESS_EDIT ) )
// 									strCompletedIP = strCompletedIP.substr( 0, iStart + 1 ) + "xxx";
// 							}
// 							else
// 							{
// 								iStart = strCompletedIP.rfind( ":" );
// 								if( iStart != string :: npos )
// 								{
// 									// don't strip ip for mods
// 
// 									if( !( pRequest->user.ucAccess & ACCESS_EDIT ) )
// 										strCompletedIP = strCompletedIP.substr( 0, iStart + 1 ) + "xxxx";
// 								}
// 							}

						if( !strUID.empty( ) )
						{
//							if( strUID == pRequest->user.strUID )
//								pResponse->strContent += "<td class=\"uploader\"><a class=\"username\"";
//							else
							pResponse->strContent += "<td class=\"uploader\">";
							pResponse->strContent += getUserLink( strUID, strUsername );
							pResponse->strContent += "</td>\n";
						}
						else
							pResponse->strContent += "<td class=\"uploader\">" + UTIL_RemoveHTML( strUsername ) + "</td>\n";

						pResponse->strContent += "</td>";
						pResponse->strContent += "<td class=\"date\">" + strDate + "</td></tr>";
						uiAdded++;
						
						vecQueryCompleted = pQueryCompleted->nextRow( );
					}
					pResponse->strContent += "</table>";
					pResponse->strContent += "</div>\n";
				}
			}
			delete pQueryCompleted;
		}
		else
		{
			pResponse->strContent += "<p class=\"not_exist\">" + UTIL_Xsprintf( gmapLANG_CFG["torrent_not_exist"].c_str( ), cstrID.c_str( ) );
			HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_STATS ) );
			return;
		}

		// Output common HTML ending
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_STATS ) );
	}
	else
	{
		// Not Authorised

		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["stats_page"], string( CSS_STATS ), string( ), NOT_INDEX, CODE_401 );

		// Output common HTML ending
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_STATS ) );
	}
}

void CTracker :: serverResponseStatsPOST( struct request_t *pRequest, struct response_t *pResponse, CAtomList *pPost )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
		if( IsIPBanned( pRequest, pResponse, btv, gmapLANG_CFG["stats_page"], string( CSS_STATS ), NOT_INDEX ) )
			return;

	if( !m_bShowStats )
	{
		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["stats_page"], string( CSS_STATS ), string( ), NOT_INDEX, CODE_403 );

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_STATS ) );

		return;
	}
	
	string cstrName = string( );
	string cstrTag = string( );
	string cstrID = string( );
	string cstrPost = string( );
	string cstrOffer = string( );
//		string cstrUploader = string( );
	string cstrIMDbID = string( );
	string cstrIntr = string( );
	string cstrIntrImg = string( );
	string cstrFreeStatus = string( "remain" );
	string cstrFreeDown = string( "100" );
	string cstrFreeUp = string( "100" );
	string cstrFreeTime = string( "0" );
	string cstrFreeFrom = string( "added" );
	string cstrDefaultDown = string( );
	string cstrDefaultUp = string( );
	string cstrReturnPage = string( );
	
	string cstrTop = string( "0" );
	string cstrClassic = string( "0" );
//	bool bHL = false;
//	bool bClassic = false;
	bool bOffer = false;
	bool bReq = false;
	bool bNoDownload = false;
	bool bNoComment = false;
	
	// needs work

	const string cstrIP( pRequest->strIP );

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
						string strName = pName->toString( );
						
						// Does the content name indicate tag data?
						if( strName == "tag" )
							// Get the tag data
							cstrTag = pData->toString( );
						else if( strName == "id" )
							// Get the id data
							cstrID = pData->toString( );
						else if( strName == "oid" )
						{
							// Get the id data
							cstrID = pData->toString( );
							bOffer = true;
						}
//						else if( strName == "offer" )
//							// Get the tag data
//							cstrOffer = pData->toString( );
						else if( strName == "post" )
							// Get the tag data
							cstrPost = pData->toString( );
						// Does the content name indicate the posted name data?
						else if( strName == "name" )
							// Get the posted name
							cstrName = pData->toString( ).substr( 0, MAX_FILENAME_LEN );
						else if( strName == "imdb" )
							cstrIMDbID = pData->toString( );
						else if( strName == "intr" )
							cstrIntr = pData->toString( );
//							else if( strName == "uploader" )
//								cstrUploader = pData->toString( );
						else if( strName == "return" )
							cstrReturnPage = pData->toString( );
						else if( strName == "free_status" )
							cstrFreeStatus = pData->toString( );
						else if( strName == "free_down" )
							cstrFreeDown = pData->toString( );
						else if( strName == "free_up" )
							cstrFreeUp = pData->toString( );
						else if( strName == "free_time" )
							cstrFreeTime = pData->toString( );
						else if( strName == "free_from" )
							cstrFreeFrom = pData->toString( );
						else if( strName == "default_down" )
							cstrDefaultDown = pData->toString( );
						else if( strName == "default_up" )
							cstrDefaultUp = pData->toString( );
						else if( strName == "top" )
							cstrTop = pData->toString( );
						else if( strName == "classic" )
							cstrClassic = pData->toString( );
//						else if( strName == "hl" && pData->toString( ) == "on" )
//							bHL = true;
//						else if( strName == "classic" && pData->toString( ) == "on" )
//							bClassic = true;
						else if( strName == "req" && pData->toString( ) == "on" )
							bReq = true;
						else if( strName == "nodownload" && pData->toString( ) == "on" )
							bNoDownload = true;
						else if( strName == "nocomment" && pData->toString( ) == "on" )
							bNoComment = true;
					}
					else
					{
						// Output common HTML head
						HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["stats_page"], string( CSS_STATS ), string( ), NOT_INDEX, CODE_400 );

						// failed
						pResponse->strContent += "<p class=\"failed\">" + gmapLANG_CFG["failed"] + "</p>\n";
						// Signal a bad request
						pResponse->strContent += "<p class=\"body_upload\">400 " + gmapLANG_CFG["server_response_400"] + "</p>\n";

						// Output common HTML tail
						HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_STATS ) );

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
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["stats_page"], string( CSS_STATS ), string( ), NOT_INDEX, CODE_400 );

		// failed
		pResponse->strContent += "<p class=\"failed\">" + gmapLANG_CFG["failed"] + "</p>\n";
		// Signal a bad request
		pResponse->strContent += "<p class=\"body_upload\">400 " + gmapLANG_CFG["server_response_400"] + "</p>\n";

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_STATS ) );

		if( gbDebug )
			UTIL_LogPrint( "Upload Warning - Bad request (no post received)\n" );

		return;
	}
	
	if( ( pRequest->user.ucAccess & m_ucAccessEditTorrents ) || ( pRequest->user.ucAccess & m_ucAccessEditOffers ) || ( ( pRequest->user.ucAccess & m_ucAccessEditOwn ) && !pRequest->user.strUID.empty( ) ) )
	{
		if( cstrFreeStatus == "remain" )
		{
			cstrFreeDown.erase( );
			cstrFreeUp.erase( );
			cstrFreeTime.erase( );
		}
		
		bool bPost = false;
	
		if( cstrPost == "1" )
			bPost = true;
		if( cstrOffer == "1" )
			bOffer = true;


		bool bFromNow = false;
		
		if( cstrFreeFrom == "now" )
			bFromNow = true;
		
		string strDatabase = string( );
		string strIDName = string( );
		if( bOffer )
		{
			strDatabase = "offer";
			strIDName = "oid";
			if( cstrReturnPage.empty( ) )
				cstrReturnPage = RESPONSE_STR_OFFER_HTML;
		}
		else
		{
			strDatabase = "allowed";
			strIDName = "id";
			if( cstrReturnPage.empty( ) )
				cstrReturnPage = RESPONSE_STR_INDEX_HTML;
		}
			
		if( cstrID.find( " " ) != string :: npos )
			cstrID.erase( );
		
		vector< pair< string, string > > vecParams;
		vecParams.reserve(64);
		string strJoined = string( );
		string strJoinedHTML = string( );

		vecParams.push_back( pair<string, string>( strIDName, cstrID ) );
		vecParams.push_back( pair<string, string>( string( "return" ), cstrReturnPage ) );
		
		strJoined = UTIL_RemoveHTML( UTIL_HTMLJoin( vecParams, string( "?" ), string( "&" ), string( "=" ) ) );
		
		CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bid,buploaderid FROM " + strDatabase + " WHERE bid=" + cstrID );
		
		vector<string> vecQuery;

		vecQuery.reserve(2);

		vecQuery = pQuery->nextRow( );
				
		string strOldUploaderID = string( );
		
		if( vecQuery.size( ) > 0 )
			strOldUploaderID = vecQuery[1];
		
		delete pQuery;
		
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["stats_page"], string( CSS_STATS ), string( ), NOT_INDEX, CODE_200 );
		
		// assorted scripts (thanks SA)
//		pResponse->strContent += "<script type=\"text/javascript\">\n";
//		pResponse->strContent += "<!--\n";

		// validate
//		pResponse->strContent += "function validate( theform ) {\n";
//		pResponse->strContent += "if( theform.comment.value == \"\" ) {\n";
//		pResponse->strContent += "  alert( \"" + gmapLANG_CFG["js_fill_fields"] + "\" );\n";
//		pResponse->strContent += "  return false; }\n";
//		pResponse->strContent += "if( theform.comment.value.length > " + CAtomInt( m_uiCommentLength ).toString( ) + " ) {\n";
//		pResponse->strContent += "  alert( \"" + gmapLANG_CFG["js_too_long"] + "\\n" + m_strJSLength + "\\n" + m_strJSReduce + "\" );\n";
//		pResponse->strContent += "  return false; }\n";
//		pResponse->strContent += "else { return true; }\n";
//		pResponse->strContent += "}\n\n";

		// checklength
//		pResponse->strContent += "function checklength( theform ) {\n";
//		pResponse->strContent += "  alert( \"" + UTIL_Xsprintf( gmapLANG_CFG["js_comment_length"].c_str( ), "\" + theform.comment.value.getBytes() + \"" ) + "\" );\n";
//		pResponse->strContent += "}\n";
//		pResponse->strContent += "//-->\n";
//		pResponse->strContent += "</script>\n\n";

		if( vecQuery.size( ) > 0 )
		{
			if( ( !bOffer && ( pRequest->user.ucAccess & m_ucAccessEditTorrents ) ) || ( bOffer && ( pRequest->user.ucAccess & m_ucAccessEditOffers ) ) || ( ( pRequest->user.ucAccess & m_ucAccessEditOwn ) && !pRequest->user.strUID.empty( ) && ( pRequest->user.strUID == strOldUploaderID ) ) )
			{
			// needs work pTorrent
				bool bNum = true;
				for( int i = 0; i < cstrFreeDown.length( ) && bNum; i++ )
					if( !isdigit( cstrFreeDown[i] ) )
						bNum  = false;
				for( int i = 0; i < cstrFreeUp.length( ) && bNum; i++ )
					if( !isdigit( cstrFreeUp[i] ) )
						bNum  = false;
				for( int i = 0; i < cstrFreeTime.length( ) && bNum; i++ )
					if( !isdigit( cstrFreeTime[i] ) )
						bNum  = false;
				for( int i = 0; i < cstrDefaultDown.length( ) && bNum; i++ )
					if( !isdigit( cstrDefaultDown[i] ) )
						bNum  = false;
				for( int i = 0; i < cstrDefaultUp.length( ) && bNum; i++ )
					if( !isdigit( cstrDefaultUp[i] ) )
						bNum  = false;
				
				if( ( ( !bOffer && ( pRequest->user.ucAccess & m_ucAccessEditTorrents ) ) || ( bOffer && ( pRequest->user.ucAccess & m_ucAccessEditOffers ) ) ) && bNum )
					modifyTag( cstrID, cstrTag, cstrName, cstrIntr, string( ), string( ), cstrIP, cstrDefaultDown, cstrDefaultUp, cstrFreeDown, cstrFreeUp, cstrFreeTime, string( ), bFromNow, bOffer );
				else
					modifyTag( cstrID, cstrTag, cstrName, cstrIntr, string( ), string( ), cstrIP, string( ),  string( ), string( ),  string( ),  string( ), string( ), false, bOffer );
				
				string strIMDb = string( );
				
				if( cstrIMDbID.find( "tt" ) != 0 || cstrIMDbID.find_first_not_of( "0123456789t" )  != string :: npos )
					cstrIMDbID.erase( );
				
				if( !cstrIMDbID.empty( ) )
				{
					CMySQLQuery *pQuery = new CMySQLQuery( "SELECT bimdb,bimdbid,bimdbupdated from allowed WHERE bimdbid=\'" + UTIL_StringToMySQL( cstrIMDbID ) + "\' GROUP BY bimdbid UNION SELECT bimdb,bimdbid,bimdbupdated from offer WHERE bimdbid=\'" + UTIL_StringToMySQL( cstrIMDbID ) + "\' GROUP BY bimdbid" );
	
					vector<string> vecQuery;

					vecQuery.reserve(3);

					vecQuery = pQuery->nextRow( );
					
					delete pQuery;
					
					if( vecQuery.size( ) == 3 )
					{
						strIMDb = vecQuery[0];
						CMySQLQuery mq01( "UPDATE " + strDatabase + " SET bimdb=\'" + UTIL_StringToMySQL( strIMDb ) + "\',bimdbid=\'" + UTIL_StringToMySQL( cstrIMDbID ) + "\',bimdbupdated=\'" + vecQuery[2] + "\' WHERE bid=" + cstrID );
					}
					else
					{
						CMySQLQuery mq01( "UPDATE " + strDatabase + " SET bimdbid=\'" + UTIL_StringToMySQL( cstrIMDbID ) + "\' WHERE bid=" + cstrID );
						system( string( "./getimdbone.py \"" + cstrIMDbID + "\" &" ).c_str( ) );
					}
// 						strIMDb = GetIMDb( cstrIMDbID );
				}
				else
					CMySQLQuery mq01( "UPDATE " + strDatabase + " SET bimdb=\'" + UTIL_StringToMySQL( strIMDb ) + "\',bimdbid=\'" + UTIL_StringToMySQL( cstrIMDbID ) + "\',bimdbupdated=NOW() WHERE bid=" + cstrID );
				
				if( !bOffer && ( pRequest->user.ucAccess & m_ucAccessEditTorrents ) )
				{
					string strQuery = "UPDATE " + strDatabase + " SET ";
					strQuery += "btop=";
					if( isdigit( cstrTop[0] ) )
						strQuery += cstrTop;
					else
						strQuery += "0";
//					strQuery += ",bhl=";
//					if( bHL )
//						strQuery += "1";
//					else
//						strQuery += "0";
					strQuery += ",bclassic=";
					if( isdigit( cstrClassic[0] ) )
						strQuery += cstrClassic;
					else
						strQuery += "0";
					strQuery += ",breq=";
					if( bReq )
						strQuery += "1";
					else
						strQuery += "0";
					strQuery += ",bnodownload=";
					if( bNoDownload || bPost )
						strQuery += "1";
					else
						strQuery += "0";
					strQuery += ",bnocomment=";
					if( bNoComment )
						strQuery += "1";
					else
						strQuery += "0";
					strQuery += " WHERE bid=" + cstrID;
					CMySQLQuery mq01( strQuery );
				}
				
				m_pCache->setRow( cstrID, bOffer );
				
				if( !bOffer )
					UTIL_LogFilePrint( "editTorrent: %s edited torrent %s\n", pRequest->user.strLogin.c_str( ), cstrName.c_str( ) );
				else
					UTIL_LogFilePrint( "editOffer: %s edited offer %s\n", pRequest->user.strLogin.c_str( ), cstrName.c_str( ) );

				pResponse->strContent += "<div class=\"changed_stats\">\n";
				pResponse->strContent += "<table class=\"changed_stats\">\n";
				pResponse->strContent += "<tr>\n<td>\n<ul>\n";

				// Changed name to
				if( !cstrName.empty( ) )
					pResponse->strContent += "<li class=\"changed_stats\">" + UTIL_Xsprintf( gmapLANG_CFG["stats_changed_name"].c_str( ), UTIL_RemoveHTML( cstrName ).c_str( ) ) + "</li>\n";

				// Changed tag to
				if( !cstrTag.empty( ) )
					pResponse->strContent += "<li class=\"changed_stats\">" + UTIL_Xsprintf( gmapLANG_CFG["stats_changed_tag"].c_str( ), UTIL_RemoveHTML( cstrTag ).c_str( ) ) + "</li>\n";

// 				// Changed uploader to
// 				if( !cstrUploader.empty( ) && ( pRequest->user.ucAccess & ACCESS_EDIT ) )
// 					pResponse->strContent += "<li class=\"changed_stats\">" + UTIL_Xsprintf( gmapLANG_CFG["stats_changed_uploader"].c_str( ), UTIL_RemoveHTML( cstrUploader ).c_str( ) ) + "</li>\n";

				// Changed IP to -  Needs work
				if( !cstrIP.empty( ) )
					pResponse->strContent += "<li class=\"changed_stats\">" + UTIL_Xsprintf( "IP: %s", UTIL_RemoveHTML( cstrIP ).c_str( ) ) + "</li>\n";

				pResponse->strContent += "</ul>\n</td>\n</tr>\n</table>\n</div>\n";

				// Return to stats page
				if( bOffer )
					pResponse->strContent += "<p class=\"changed_stats\">" + UTIL_Xsprintf( gmapLANG_CFG["stats_return_offer"].c_str( ), string( "<a title=\"" + cstrName + "\" href=\"" + RESPONSE_STR_STATS_HTML + strJoined + "&amp;action=edited\">").c_str( ), "</a>", string( "<a title=\"" + gmapLANG_CFG["navbar_offer"] + "\" href=\"" + cstrReturnPage + "\">").c_str( ), "</a>" ) + "</p>\n";
				else
					pResponse->strContent += "<p class=\"changed_stats\">" + UTIL_Xsprintf( gmapLANG_CFG["stats_return"].c_str( ), string( "<a title=\"" + cstrName + "\" href=\"" + RESPONSE_STR_STATS_HTML + strJoined + "&amp;action=edited\">").c_str( ), "</a>", string( "<a title=\"" + gmapLANG_CFG["navbar_offer"] + "\" href=\"" + cstrReturnPage + "\">").c_str( ), "</a>" ) + "</p>\n";

				// Output common HTML tail
				HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_STATS ) );

				return;
			}
		}
		else
		{
			pResponse->strContent += "<p class=\"not_exist\">" + UTIL_Xsprintf( gmapLANG_CFG["torrent_not_exist"].c_str( ), cstrID.c_str( ) );
			HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_STATS ) );
			return;
		}
	}
}
