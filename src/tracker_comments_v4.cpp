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
#include "atom.h"
#include "config.h"
#include "html.h"
#include "tracker.h"
#include "util.h"

void CTracker :: serverResponseComments( struct request_t *pRequest, struct response_t *pResponse )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
		if( IsIPBanned( pRequest, pResponse, btv, gmapLANG_CFG["comments_page"], string( CSS_COMMENTS ), NOT_INDEX ) )
			return;

	if( !m_pAllowed || !m_bAllowComments )
	{
		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["comments_page"], string( CSS_COMMENTS ), string( ), NOT_INDEX, CODE_403 );

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_COMMENTS ) );

		return;
	}

	if( pRequest->user.ucAccess & ACCESS_VIEW )
	{
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
		pResponse->strContent += "if( theform.comment.value.length > " + CAtomInt( m_uiCommentLength ).toString( ) + " ) {\n";
		pResponse->strContent += "  alert( \"" + gmapLANG_CFG["js_message_too_long"] + "\\n" + m_strJSReduce + "\\n" + m_strJSLength + "\" );\n";
		pResponse->strContent += "  return false; }\n";
		pResponse->strContent += "else { return true; }\n";
		pResponse->strContent += "}\n\n";

		// checklength
		pResponse->strContent += "function checklength( theform ) {\n";
		pResponse->strContent += "  alert( \"" + m_strJSLength + "\" );\n";
		pResponse->strContent += "}\n";
		pResponse->strContent += "//-->\n";
		pResponse->strContent += "</script>\n\n";

		const string cstrHashString( pRequest->mapParams["info_hash"] );
		// const string cstrName( pRequest->mapParams["name"] );
		const string cstrHash( UTIL_StringToHash( cstrHashString ) );

		if( !cstrHash.empty( ) )
		{
			//
			// delete comment
			//

			if( pRequest->user.ucAccess & ACCESS_EDIT )
			{
				const string cstrDelAll( pRequest->mapParams["delall"] );
				const string cstrDel( pRequest->mapParams["del"] );

				if( cstrDelAll == "1" )
				{
					m_pComments->delItem( cstrHash );

					// Save the comments
					saveComments( );

					// Deleted all comments.
					pResponse->strContent += "<p class=\"deleted\">" + UTIL_Xsprintf( gmapLANG_CFG["comments_deleted_all"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["comments"] + "\" href=\"" + RESPONSE_STR_COMMENTS_HTML + "?info_hash=" + cstrHashString + "\">" ).c_str( ), "</a>" ) + "</p>\n";

					// Output common HTML tail
					HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_COMMENTS ) );

					return;
				}
				else if( !cstrDel.empty( ) )
				{
					const unsigned int cuiDel( (unsigned int)( atoi( cstrDel.c_str( ) ) - 1 ) );

					CAtom *pComments = m_pComments->getItem( cstrHash );

					if( pComments && dynamic_cast<CAtomList *>( pComments ) )
					{
						vector<CAtom *> vecComments;
						vecComments.reserve(8);
						vecComments = dynamic_cast<CAtomList *>( pComments )->getValue( );

						if( cuiDel < vecComments.size( ) )
						{
							dynamic_cast<CAtomList *>( pComments )->delItem( vecComments[cuiDel] );

							saveComments( );

							// Deleted comment
							pResponse->strContent += "<p class=\"deleted\">" + UTIL_Xsprintf( gmapLANG_CFG["comments_deleted"].c_str( ), cstrDel.c_str( ), string( "<a title=\"" + gmapLANG_CFG["comments"] + "\" href=\"" + RESPONSE_STR_COMMENTS_HTML + "?info_hash=" + cstrHashString + "\">" ).c_str( ), "</a>" ) + "</p>\n";

							// Output common HTML tail
							HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_COMMENTS ) );

							return;
						}
						else
						{
							// Unable to delete comment
							pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["comments_unable_delete"].c_str( ), cstrDel.c_str( ), string( "<a title=\"" + gmapLANG_CFG["comments"] + "\" href=\"" + RESPONSE_STR_COMMENTS_HTML + "?info_hash=" + cstrHashString + "\">" ).c_str( ), "</a>" ) + "</p>\n";

							// Output common HTML tail
							HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_COMMENTS ) );

							return;
						}
					}
				}
			}

			// display torrent information list

			if( m_pAllowed )
			{
				CAtom *pTorrent = m_pAllowed->getItem( cstrHash );
				
				CAtom *pDicti = 0;

				if( m_pTags )
					pDicti = m_pTags->getItem( cstrHash );

				string strOldName = string( );

				if( pDicti && pDicti->isDicti( ) )
				{
					CAtom *pOldName = ( (CAtomDicti *)pDicti )->getItem( "name" );

					if( pOldName )
						strOldName = pOldName->toString( );
				}

				if( pTorrent && dynamic_cast<CAtomList *>( pTorrent ) )
				{
					vector<CAtom *> vecTorrent;
					vecTorrent.reserve( 6 );
					vecTorrent = dynamic_cast<CAtomList *>( pTorrent )->getValue( );

					if( vecTorrent.size( ) == 6 )
					{
						CAtom *pName = vecTorrent[1];
						CAtom *pAdded = vecTorrent[2];
						CAtom *pSize = vecTorrent[3];
						CAtom *pFiles = vecTorrent[4];
						CAtom *pComment = vecTorrent[5];

						pResponse->strContent += "<div class=\"comments_info_table\">\n";
						pResponse->strContent += "<p>" + gmapLANG_CFG["comments_file_information"] + "</p>\n";
						pResponse->strContent += "<p><a title=\"" + gmapLANG_CFG["name"] + ": " + UTIL_RemoveHTML( strOldName ) + "\" href=\"" + RESPONSE_STR_STATS_HTML + "?info_hash=" + cstrHashString + "\">";
						pResponse->strContent += UTIL_RemoveHTML( strOldName ) + "</a></p>\n";
					//	pResponse->strContent += "<p>" + UTIL_RemoveHTML( strOldName ) + "</p>\n";
						pResponse->strContent += "<table class=\"comments_info_table\" summary=\"file information\">\n";
						pResponse->strContent += "<tr class=\"comments_info_table\">\n";
						pResponse->strContent += "<td class=\"comments_info_table\">\n";
						pResponse->strContent += "<ul>\n";

						if( pName )
							pResponse->strContent += "<li><strong>" + gmapLANG_CFG["name"] + ":</strong> " + UTIL_RemoveHTML( pName->toString( ) ) + "</li>\n";

						if( !cstrHashString.empty( ) )
							pResponse->strContent += "<li><strong>" + gmapLANG_CFG["info_hash"] + ":</strong> " + cstrHashString + "</li>\n";

						if( pAdded )
							pResponse->strContent += "<li><strong>" + gmapLANG_CFG["added"] + ":</strong> " + pAdded->toString( ) + "</li>\n";

						if( pSize && dynamic_cast<CAtomLong *>( pSize ) )
							pResponse->strContent += "<li><strong>" + gmapLANG_CFG["size"] + ":</strong> " + UTIL_BytesToString( dynamic_cast<CAtomLong *>( pSize )->getValue( ) ) + "</li>\n";

						if( pFiles && dynamic_cast<CAtomInt *>( pFiles ) )
							pResponse->strContent += "<li><strong>" + gmapLANG_CFG["files"] + ":</strong> " + pFiles->toString( ) + "</li>\n";

						pResponse->strContent += "</ul>\n";
						pResponse->strContent += "</td>\n";
						pResponse->strContent += "</tr>\n";
						pResponse->strContent += "</table>\n";
						pResponse->strContent += "</div>\n";

						if( pComment )
						{
							if( m_bShowFileComment )
							{
								pResponse->strContent += "<div class=\"comments_table\">\n";
								pResponse->strContent += "<p>" + gmapLANG_CFG["file_comment"] + "</p>\n";
								pResponse->strContent += "<table  class=\"comments_table\" summary=\"file comment\">\n";
								pResponse->strContent += "<tr class=\"comments_table\"><td  class=\"comments_table\"><code>" + UTIL_RemoveHTML( pComment->toString( ) ) + "</code></td></tr>\n";
								pResponse->strContent += "</table>\n";
								pResponse->strContent += "</div>\n";
							}
						}
					}
				}
			}

			if( !m_pComments->getItem( cstrHash ) )
				m_pComments->setItem( cstrHash, new CAtomList( ) );

			CAtom *pComments = m_pComments->getItem( cstrHash );

			if( pComments && dynamic_cast<CAtomList *>( pComments ) )
			{
				if( pRequest->user.ucAccess & ACCESS_COMMENTS )
				{
					if( pRequest->mapParams.find( "comment" ) != pRequest->mapParams.end( ) )
					{
						string strComment = pRequest->mapParams["comment"].substr( 0, m_uiCommentLength );

						if( strComment.empty( ) )
						{
							//You must fill in all the fields.
							pResponse->strContent += "<p class=\"fill_all\">" + UTIL_Xsprintf( gmapLANG_CFG["comments_fill_warning"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["comments"] + "\" href=\"" + RESPONSE_STR_COMMENTS_HTML + "?info_hash=" + cstrHashString + "\">" ).c_str( ), "</a>" ) + "</p>\n";

							// Output common HTML tail
							HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_COMMENTS ) );

							return;
						}

						CAtomDicti *pNew = new CAtomDicti( );

						pNew->setItem( "ip", new CAtomString( inet_ntoa( pRequest->sin.sin_addr ) ) );

						if( !pRequest->user.strLogin.empty( ) )
							pNew->setItem( "name", new CAtomString( pRequest->user.strLogin ) );

						pNew->setItem( "comment", new CAtomString( strComment ) );

						time_t tNow = time( 0 );
						char *szTime = asctime( localtime( &tNow ) );
						szTime[strlen( szTime ) - 1] = TERM_CHAR;

						pNew->setItem( "time", new CAtomString( szTime ) );

						dynamic_cast<CAtomList *>( pComments )->addItem( pNew );

						// Save the comments
						saveComments( );

						// Your comment has been posted.
						pResponse->strContent += "<p class=\"comments_posted\">" + UTIL_Xsprintf( gmapLANG_CFG["comments_posted"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["comments"] + "\" href=\"" + RESPONSE_STR_STATS_HTML + "?info_hash=" + cstrHashString + "\">" ).c_str( ), "</a>" ) + "</p>\n";

						// Output common HTML tail
						HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_COMMENTS ) );

						return;
					}
				}

				vector<CAtom *> *pvecList = dynamic_cast<CAtomList *>( pComments )->getValuePtr( );

				bool bFound = false;

				unsigned long ulCount = 0;

				CAtomDicti *pCommentDicti = 0;

				CAtom *pIP = 0;
				CAtom *pName = 0;
				CAtom *pComText = 0;
				CAtom *pTime = 0;

				string strIP = string( );
				string strName = string( );
				string strComText = string( );
				string strTime = string( );

				string :: size_type iStart = 0;

				for( vector<CAtom *> :: iterator it = pvecList->begin( ); it != pvecList->end( ); it++ )
				{
					if( (*it)->isDicti( ) )
					{
						pCommentDicti = (CAtomDicti *)(*it);

						pIP = pCommentDicti->getItem( "ip" );
						pName = pCommentDicti->getItem( "name" );
						pComText = pCommentDicti->getItem( "comment" );
						pTime = pCommentDicti->getItem( "time" );

						if( pIP && pComText && pTime )
						{
							if( !bFound )
							{
								pResponse->strContent += "<div class=\"comments_table_posted\">";
								pResponse->strContent += "<p class=\"comments_table_posted\">" + gmapLANG_CFG["comments"];

								if( pRequest->user.ucAccess & ACCESS_EDIT )
									pResponse->strContent += " [<a title=\"" + gmapLANG_CFG["delete_all"] + "\" href=\"" + RESPONSE_STR_COMMENTS_HTML + "?info_hash=" + cstrHashString + "&amp;delall=1\">" + gmapLANG_CFG["delete_all"] + "</a>]";

								pResponse->strContent += "</p>\n";
								pResponse->strContent += "<table  class=\"comments_table_posted\" summary=\"comments\">\n";

								bFound = true;
							}

							strIP = pIP->toString( );
							strName = string( );

							if( pName )
								strName = pName->toString( );

							strComText = pComText->toString( );
							strTime = pTime->toString( );

							if( strName.empty( ) )
							{
								// strip ip

								iStart = strIP.rfind( "." );

								if( iStart != string :: npos )
								{
									// don't strip ip for mods

									if( !( pRequest->user.ucAccess & ACCESS_EDIT ) )
										strIP = strIP.substr( 0, iStart + 1 ) + "xxx";
								}
							}
							else
							{
								if( !( pRequest->user.ucAccess & ACCESS_EDIT ) )
									strIP = gmapLANG_CFG["comment_ip_hidded"];
							}

							//
							// header
							//

							// Comment by
							if( strName.empty( ) )
								strName = gmapLANG_CFG["unknown"];

							if( strTime.empty( ) )
								strTime = gmapLANG_CFG["unknown"];
	
							if( strIP.empty( ) )
								strIP = gmapLANG_CFG["unknown"];

							pResponse->strContent += "<tr class=\"com_header\"><td class=\"com_header\"><code>" + UTIL_Xsprintf( gmapLANG_CFG["comments_posted_by"].c_str( ), CAtomInt( ulCount + 1 ).toString( ).c_str( ) , UTIL_RemoveHTML( strName ).c_str( ) , strIP.c_str( ), strTime.c_str( ) );

							if( pRequest->user.ucAccess & ACCESS_EDIT )
								pResponse->strContent += " [<a title=\"" + gmapLANG_CFG["delete"] + "\" href=\"" + RESPONSE_STR_COMMENTS_HTML + "?info_hash=" + cstrHashString + "&amp;del=" + CAtomInt( ulCount + 1 ).toString( ) + "\">" + gmapLANG_CFG["delete"] + "</a>]";

							pResponse->strContent += "</code></td></tr>\n";

							//
							// body
							//

							pResponse->strContent += "<tr class=\"com_body\"><td class=\"com_body\"><code>" + UTIL_RemoveHTML( strComText ) + "</code></td></tr>\n";
						}
					}

					ulCount++;
				}

				if( bFound )
				{
					pResponse->strContent += "</table>\n";
					pResponse->strContent += "</div>\n";
				}
				else
					pResponse->strContent += "<p class=\"comments_table_posted\">" + gmapLANG_CFG["comments_no_comment"] + "</p>\n";
			}

			if( pRequest->user.ucAccess & ACCESS_COMMENTS )
			{
				pResponse->strContent += "<div class=\"comments_table_post\">\n";
				pResponse->strContent += "<p class=\"comments_table_post\">" + gmapLANG_CFG["comments_post_comment"] + "</p>\n";
				pResponse->strContent += "<table class=\"comments_table_post\">\n";
				pResponse->strContent += "<tr class=\"comments_table_post\">\n";
				pResponse->strContent += "<td class=\"comments_table_post\">\n";
				pResponse->strContent += "<form method=\"get\" action=\"" + RESPONSE_STR_COMMENTS_HTML + "\" name=\"postacomment\" onSubmit=\"return validate( this )\">\n";
				pResponse->strContent += "<ul>\n";

				// Comments must be less than
				pResponse->strContent += "<li>" + UTIL_Xsprintf( gmapLANG_CFG["comments_length_info"].c_str( ), CAtomInt( m_uiCommentLength ).toString( ).c_str( ) ) + "</li>\n";
				pResponse->strContent += "<li>" + gmapLANG_CFG["no_html"] + "</li>\n";
				pResponse->strContent += "</ul>\n";
				pResponse->strContent += "<p><input name=\"info_hash\" type=hidden value=\"" + cstrHashString + "\"></p>\n";
				// pResponse->strContent += "<p><input name=\"name\" type=hidden value=\"" + cstrName + "\"></p>\n";
				pResponse->strContent += "<p><span style=\"display:none\"><label for=\"commentarea\">" + gmapLANG_CFG["comments_post_comment"] + "</label></span><textarea id=\"commentarea\" name=\"comment\" rows=8 cols=64></textarea></p>\n";
				pResponse->strContent += "<p class=\"comments_table_post\">[<a title=\"" + gmapLANG_CFG["comment_check_length"] + "\" href=\"" + string( JS_CHECKLENGTH ) + "\">" + gmapLANG_CFG["comment_check_length"] + "</a>]</p>\n";
				pResponse->strContent += "<div class=\"comments_table_post_button\">\n";
				pResponse->strContent += Button_Submit( "submit_comment", string( gmapLANG_CFG["Submit"] ) );
				pResponse->strContent += "</div>\n";
				pResponse->strContent += "</form>\n";
				pResponse->strContent += "</td>\n";
				pResponse->strContent += "</tr>\n";
				pResponse->strContent += "</table>\n";
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
