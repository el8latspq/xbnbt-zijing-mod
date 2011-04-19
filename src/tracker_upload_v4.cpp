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
#include "atom.h"
#include "bencode.h"
#include "client.h"
#include "config.h"
#include "html.h"
#include "tracker.h"
#include "util.h"

void CTracker :: serverResponseUploadGET( struct request_t *pRequest, struct response_t *pResponse )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
		if( IsIPBanned( pRequest, pResponse, btv, gmapLANG_CFG["upload_page"], string( CSS_UPLOAD ), NOT_INDEX ) )
			return;

	// Does the user have upload access?
	if( pRequest->user.ucAccess & ACCESS_UPLOAD )
	{
		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["upload_page"], string( CSS_UPLOAD ), string( ), NOT_INDEX, CODE_200 );

		// Does the tracker allow uploads?
		if( m_strUploadDir.empty( ) )
		{
			// This tracker does not allow file uploads.
			pResponse->strContent += "<p class=\"denied\">" + UTIL_Xsprintf( gmapLANG_CFG["upload_nofile_uploads"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_index"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";

			// Output common HTML tail
			HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_UPLOAD ) );

			return;
		}

		// Has the tracker had the maximum torrent limit set?
		if( m_uiMaxTorrents != 0 )
		{
			// Has the tracker reached it's maximum torrent file limit?
			if( m_pAllowed && m_pAllowed->getValuePtr( )->size( ) >= m_uiMaxTorrents )
			{
				// This tracker has reached its torrent limit.
				pResponse->strContent += "<p class=\"denied\">" + UTIL_Xsprintf( gmapLANG_CFG["upload_torrent_limit"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_index"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";

				// Output common HTML tail
				HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_UPLOAD ) );

				return;
			}
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

		pResponse->strContent += "<div class=\"torrent_upload\">\n";
		pResponse->strContent += "<table class=\"torrent_upload\">\n";
		pResponse->strContent += "<tr>\n<td>\n";
		pResponse->strContent += "<form name=\"torrentupload\" method=\"post\" action=\"" + string( RESPONSE_STR_UPLOAD_HTML ) + "\" enctype=\"multipart/form-data\">\n";
		pResponse->strContent += "<p class=\"torrent_upload\"><label for=\"uploadfile\">" + gmapLANG_CFG["upload_file"] + "</label><br>\n";
		pResponse->strContent += "<input name=\"torrent\" id=\"uploadfile\" alt=\"[" + gmapLANG_CFG["torrent"] + "]\" type=file size=50><br></p>\n";        
		pResponse->strContent += "<p class=\"torrent_upload\"><label for=\"uploadname\">" + gmapLANG_CFG["upload_name"] + "</label><br>\n";
		pResponse->strContent += "<input name=\"name\" id=\"uploadname\" alt=\"[" + gmapLANG_CFG["name"] + "]\" type=text size=96 maxlength=" + CAtomInt( MAX_FILENAME_LEN ).toString( ) + "><br></p>\n";             
		pResponse->strContent += "<p class=\"torrent_upload\"><label for=\"uploadintrimg\">" + gmapLANG_CFG["intrimg"] + "</label><br>\n";
		pResponse->strContent += "<input name=\"intrimg\" id=\"uploadintrimg\" type=text size=96 maxlength=" + CAtomInt( MAX_INFO_LINK_LEN ).toString( ) + "><br></p>\n";
		pResponse->strContent += "<p class=\"torrent_upload\"><label for=\"uploadintr\">" + gmapLANG_CFG["intr"] + "</label><br>\n";
		pResponse->strContent += "<textarea id=\"uploadintr\" name=\"intr\" rows=10 cols=96></textarea><br></p>\n";


		// Is the information link allowed?
		if( m_bAllowInfoLink ) 
		{
			pResponse->strContent += "<p class=\"torrent_upload\"><label for=\"uploadinfolink\"></label><br>\n";
			pResponse->strContent += "<input name=\"infolink\" id=\"uploadinfolink\" type=hidden alt=\"[" + gmapLANG_CFG["link"] + "]\" type=text size=64 maxlength=" + CAtomInt( MAX_INFO_LINK_LEN ).toString( ) + " value=\"\"><br></p>\n";
		}

		// Category Tag and Public/Private option
		if( m_vecTags.size( ) != 0 || m_ucMakePublic != 0 )
			pResponse->strContent += "<p class=\"torrent_upload_category\">";

		{
			// If the torrent categories have been set then enable the pull down selection filled with our categories
			if( m_vecTags.size( ) != 0 && !pRequest->user.strLogin.empty( ) )
			{
				unsigned char ucTag = 1;
				pResponse->strContent += "<label for=\"uploadtag\">" + UTIL_RemoveHTML( gmapLANG_CFG["upload_tag"] ) + "</label> <select id=\"uploadtag\" name=\"tag\">\n";
				
				for( vector< pair< string, string > > :: iterator ulCount = m_vecTags.begin( ); ulCount != m_vecTags.end( ); ulCount++ )
				{
					pResponse->strContent += "<option value=\"" + CAtomInt( ucTag ).toString( ) + "\">" + UTIL_RemoveHTML( (*ulCount).first ) + "\n";
					ucTag++;
				}

				pResponse->strContent += "</select>\n";
			}
			
			const string strRatioArray[4] = { "5%", "10%", "15%", "20%" };
			
			if( !pRequest->user.strLogin.empty( ) )
			{
				pResponse->strContent += "<p class=\"torrent_upload_category\"><label for=\"uploadratio\">" + UTIL_RemoveHTML( gmapLANG_CFG["upload_ratio"] ) + "</label> <select id=\"uploadratio\" name=\"ratio\">\n";
				
				for( int iratio = 0; iratio < 4; iratio++ )
				{
					pResponse->strContent += "<option";
					if( iratio == 1 )
						pResponse->strContent += " selected";
					pResponse->strContent += ">" + strRatioArray[iratio] + "\n";
				}
				
				pResponse->strContent += "</select></p>";
			}

			// Public/Private
			if( m_ucMakePublic != 0 )
			{
				pResponse->strContent += "<label for=\"id_makepublic\">Make Public</label><input name=\"makepublic\" id=\"id_makepublic\" type=checkbox";

				if( m_ucMakePublic == 2 )
					pResponse->strContent += " checked";

				pResponse->strContent += ">\n";
			}
		}

		if( m_vecTags.size( ) != 0 || m_ucMakePublic != 0 )
			pResponse->strContent += "</p>\n";

		// Upload note list
		pResponse->strContent += "<ul>\n";
		pResponse->strContent += "<li class=\"torrent_upload\">" + gmapLANG_CFG["upload_note_1"] + "</li>\n";
		pResponse->strContent += "<li class=\"torrent_upload\">" + UTIL_Xsprintf( gmapLANG_CFG["upload_note_2"].c_str( ), CAtomInt( MAX_FILENAME_LEN ).toString( ).c_str( ) ) + "</li>\n";
		pResponse->strContent += "<li class=\"torrent_upload\">" + gmapLANG_CFG["upload_note_3"] + "</li>\n";
		pResponse->strContent += "<li class=\"torrent_upload\"><strong>" + gmapLANG_CFG["upload_note_4"] + ":</strong> " + UTIL_BytesToString( guiMaxRecvSize ) + "</li>\n";

		if( !m_strForceAnnounceURL.empty( ) )
			pResponse->strContent += "<li class=\"announce\"><strong>" + gmapLANG_CFG["forced_announce_url"] + ":</strong> " + UTIL_RemoveHTML( m_strForceAnnounceURL ) + "</li>\n";
		else
		{
			pResponse->strContent += "<li class=\"announce\"><strong>" + gmapLANG_CFG["announce_url"] + ":</strong> http://";
			pResponse->strContent += "<script type=\"text/javascript\">document.write( parent.location.host );</script>";
			pResponse->strContent += RESPONSE_STR_ANNOUNCE + "</li>\n";
		}

		pResponse->strContent += "<li class=\"torrent_upload\">" + gmapLANG_CFG["no_html"] + "</li>\n";
		pResponse->strContent += "</ul>\n";
		// The button list
		pResponse->strContent += "<div>\n";
		pResponse->strContent += Button_Submit( "submit_upload", string( gmapLANG_CFG["upload"] ) );
		pResponse->strContent += Button_Back( "cancel_upload", string( gmapLANG_CFG["cancel"] ) );
		pResponse->strContent += "\n</div>\n";
		// finish
		pResponse->strContent += "</form>\n</td>\n</tr>\n</table>\n</div>\n";
	
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
	if( !( pRequest->user.ucAccess & ACCESS_UPLOAD ) )
	{
		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["upload_page"], string( CSS_UPLOAD ), string( ), NOT_INDEX, CODE_401 );

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_UPLOAD ) );

		UTIL_LogPrint( "Upload Denied - User is not authorised to upload (%s:%u:%s)\n", pRequest->user.strLogin.c_str( ), pRequest->user.ucAccess, inet_ntoa( pRequest->sin.sin_addr ) );
	
		return;
	}

	// If there is an not a path set or the path does not exist
	const bool cbDirectoryExists( UTIL_CheckDir( m_strUploadDir.c_str( ) ) );

	if( m_strUploadDir.empty( ) || !cbDirectoryExists )
	{
		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["upload_page"], string( CSS_UPLOAD ), string( ), NOT_INDEX, CODE_403 );

		// failed
		pResponse->strContent += "<p class=\"failed\">" + gmapLANG_CFG["failed"] + "</p>\n";
			
		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_UPLOAD ) );

		if( m_strUploadDir.empty( ) )
			UTIL_LogPrint( "Upload Error - Upload directory is not set\n" );
		else if( cbDirectoryExists )
			UTIL_LogPrint( "Upload Error - Upload directory does not exist (%s)\n", m_strUploadDir.c_str( ) );

		return;
	}

	// Has the tracker had the maximum torrent limit set?
	if( m_uiMaxTorrents != 0 )
	{
		// Has the tracker maximum torrent limit been exceeded?
		if( m_pAllowed && m_pAllowed->getValuePtr( )->size( ) >= m_uiMaxTorrents )
		{
			// Output common HTML head
			HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["upload_page"], string( CSS_UPLOAD ), string( ), NOT_INDEX, CODE_403 );

			// failed
			pResponse->strContent += "<p class=\"failed\">" + gmapLANG_CFG["failed"] + "</p>\n";

			// Output common HTML tail
			HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_UPLOAD ) );

			UTIL_LogPrint( "Upload Denied - Maximum torrent limit exceeded (max allowed %u)\n", m_uiMaxTorrents );

			return;
		}
	}
	
	const string strRatioArray[4] = { "5%", "10%", "15%", "20%" };

	// Initialise the tag variables
	string strFile = string( );
	string strTorrent = string( );
	string strTag = string( );
	string strPostedName = string( );
	string strPostedInfoLink = string( );
	string strIntr = string( );
	string strIntrImg = string( );
	string strIP = string( );
	string strName = string( );
	string strRatio = strRatioArray[1];
	string strMakePublic = string( );

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
								strFile = UTIL_RemoveHTML( UTIL_StripPath( pFile->toString( ) ) );
								// Get the torrent contents from the data
								strTorrent = pData->toString( );
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
									UTIL_LogPrint( "Upload Warning - Bad request (no file name)\n" );

								return;
							}
						}
						// Does the content name indicate tag data?
						else if( strName == "tag" )
							// Get the tag data
							strTag = pData->toString( );
						// Does the content name indicate the posted name data?
						else if( strName == "name" )
							// Get the posted name
							strPostedName = pData->toString( ).substr( 0, MAX_FILENAME_LEN );
						// Does the content name indicate informatation link data?
						else if( strName == "infolink" )
							// Get the information link data
							strPostedInfoLink = pData->toString( ).substr( 0, MAX_INFO_LINK_LEN );
						else if( strName == "intrimg" )
							strIntrImg = pData->toString( );
						else if( strName == "intr" )
							strIntr = pData->toString( );
						else if( strName == "ratio" )
							strRatio = pData->toString( );
						// Does the content name indicate public data?
						else if( m_ucMakePublic != 0 )
							if( strName == "makepublic" )
								// Get the public data
								strMakePublic = pData->toString( );
						if( pRequest->user.strLogin.empty( ) )
						{
							strTag = gmapLANG_CFG["public_area"];
							strRatio = strRatioArray[1];
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

	// Output common HTML head
	HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["upload_page"], string( CSS_UPLOAD ), string( ), NOT_INDEX, CODE_200 );
	
	// Check the uploaded file for validity

	// Set the path as the local upload directory plus the user uploaded file name 
	const string cstrPath( m_strUploadDir + gmapLANG_CFG["site_name"] + "." + strFile );

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
	else if( UTIL_CheckFile( cstrPath.c_str( ) ) )
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
			const string strSource = gmapLANG_CFG["source"];

			if( !strSource.empty( ) )
			{	
				if( !pRequest->user.strLogin.empty( ) )
				{
					
					CAtom *pInfo = ( (CAtomDicti *)pTorrent )->getItem( "info" );

					( (CAtomDicti *)pInfo )->setItem( "source" , new CAtomString( strSource ) );
				}
			}
			
			// Get the info hash from the dictionary
			const string cstrInfoHash( UTIL_InfoHash( pTorrent ) );

			// Is their an info hash?
			if( !cstrInfoHash.empty( ) )
			{
				// Does this torrent info hash already exist amoung the torrent files that were parsed?
				if( m_pDFile->getItem( cstrInfoHash ) )
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
					//	( (CAtomDicti *)pTorrent )->setItem( "comment", new CAtomString( strComment ) );



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
								pAnnounceList->addItem( new CAtomString( m_strForceAnnounceURL ) );

								// We got an announce list url so add it to the announce list and get another
								while( !strAnnounceUrl.empty( ) )
								{
									pAnnounceList->addItem( new CAtomString( strAnnounceUrl ) );
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

						// Make public
						if( m_ucMakePublic != 0 )
							if( !m_strPublicUploadDir.empty( ) && !strFile.empty( ) )
								UTIL_MakeFile( string( m_strPublicUploadDir + strFile ).c_str( ), cstrEncodedTorrent );
					}
					else
					{
						pResponse->strContent += "<p class=\"failed\">" + gmapLANG_CFG["failed"] + "</p>\n";

						UTIL_LogPrint( "Upload - cstrEncodedTorrent is empty!\n" );
					}

					// Get the uploaders IP - needs work
					strIP = string( inet_ntoa( pRequest->sin.sin_addr ) );

					// Add the torrents details to the tag database
					if( !strPostedName.empty( ) )
						addTag( cstrInfoHash, strTag, strPostedName, pRequest->user.strLogin, strPostedInfoLink, strIP, strMakePublic, strRatio, "1" );
					else
					{
						CAtom *pInfo = ( (CAtomDicti *)pTorrent )->getItem( "info" );
						CAtom *pName = ( (CAtomDicti *)pInfo )->getItem( "name" );
						addTag( cstrInfoHash, strTag, UTIL_RemoveHTML( pName->toString( ) ), pRequest->user.strLogin, strPostedInfoLink, strIP, strMakePublic, strRatio, "1" );
					}

					UTIL_MakeFile( string( m_strIntrDir + UTIL_HashToString( cstrInfoHash ) ).c_str( ), strIntr );
					UTIL_MakeFile( string( m_strIntrDir + UTIL_HashToString( cstrInfoHash ) + ".img" ).c_str( ), strIntrImg );

					pResponse->strContent += "<p class=\"sucess\">" + gmapLANG_CFG["successful"] + "</p>\n";

					// The Trinity Edition - Modification Begins
					// The following removes the multiple RTT links that appear based on parsing method used

					if( m_bParseOnUpload )
					{
						if( m_pAllowed )
						{
							parseTorrent( cstrPath.c_str( ) );

							pResponse->strContent += "<p class=\"body_upload\">" + gmapLANG_CFG["upload_ready_now"] + "</p>\n";
						}
						else
						{
							pResponse->strContent += "<p class=\"failed\">" + gmapLANG_CFG["failed"] + "</p>\n";

							UTIL_LogPrint( "Upload - m_pAllowed failed!\n" );
						}
					}
					else
					{
						// The uploaded file will be ready in
						if( m_uiParseAllowedInterval == 1 )
							pResponse->strContent += "<p class=\"body_upload\">" + UTIL_Xsprintf( gmapLANG_CFG["upload_ready_1min"].c_str( ), CAtomInt( m_uiParseAllowedInterval ).toString( ).c_str( ) ) + "</p>\n";
						else
							pResponse->strContent += "<p class=\"body_upload\">" + UTIL_Xsprintf( gmapLANG_CFG["upload_ready_xmins"].c_str( ), CAtomInt(  m_uiParseAllowedInterval ).toString( ).c_str( ) ) + "</p>\n";
					}

					// The Trinity Edition - Addition Begins
					// The following displays SEEDING INSTRUCTIONS after a user successfully uploads a torrent
					pResponse->strContent += "<div class=\"seeding_instructions\">\n";
					pResponse->strContent += "<table class=\"seeding_instructions\">\n";
					pResponse->strContent += "<tr>\n<td>\n";
					pResponse->strContent += "<p class=\"seeding_instructions_head\">" + gmapLANG_CFG["upload_seeding_instructions"] + "</p>\n";
					pResponse->strContent += "<p class=\"seeding_instructions_lead\">" + gmapLANG_CFG["upload_begin_seeding"] + "</p>\n";
					pResponse->strContent += "<ol>\n";
					pResponse->strContent += "<li class=\"seeding_instructions\">" + gmapLANG_CFG["upload_inst_1"] + "</li>\n";
					pResponse->strContent += "<li class=\"seeding_instructions\">" + gmapLANG_CFG["upload_inst_2"] + "</li>\n";
					pResponse->strContent += "<li class=\"seeding_instructions\">" + gmapLANG_CFG["upload_inst_3"] + "</li>\n";
					pResponse->strContent += "<li class=\"seeding_instructions\">" + gmapLANG_CFG["upload_inst_4"] + "</li>\n";
					pResponse->strContent += "</ol>\n";
					pResponse->strContent += "<p class=\"seeding_instructions\">" + gmapLANG_CFG["upload_inst_verify"] + "<br>\n";
					pResponse->strContent += gmapLANG_CFG["upload_inst_return"];
					pResponse->strContent += "</p>\n</td>\n</tr>\n</table>\n";
					// Instructions note
					pResponse->strContent += "<table class=\"seeding_instructions_note\">\n<tr>\n<td style=\"font-weight:normal; border:1px solid black; padding:10px\">\n";
					pResponse->strContent += "<p class=\"seeding_instructions\">&dagger; " + gmapLANG_CFG["upload_note_1"] + "<br>\n";
					pResponse->strContent += UTIL_Xsprintf( gmapLANG_CFG["upload_note_2"].c_str( ), CAtomInt( MAX_FILENAME_LEN ).toString( ).c_str( ) ) + "<br>\n";
					pResponse->strContent += gmapLANG_CFG["upload_note_3"];
					pResponse->strContent += "</p>\n</td>\n</tr>\n</table>\n</div>\n";

					// Who uploaded the torrent?
					UTIL_LogPrint( string( gmapLANG_CFG["user_uploaded_torrent"] + "\n" ).c_str( ), pRequest->user.strLogin.c_str( ), inet_ntoa( pRequest->sin.sin_addr ), strFile.c_str( ) );
				}
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
