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
#include "tracker.h"
#include "util.h"

void CTracker :: serverResponseFile( struct request_t *pRequest, struct response_t *pResponse )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
		if( IsIPBanned( pRequest, pResponse, btv, "File server", string( CSS_INDEX ), NOT_INDEX ) )
			return;
	
	// Forbidden requests
	if( m_strFileDir.empty( ) || pRequest->strURL.find( "..\\" ) != string :: npos || pRequest->strURL.find( "../" ) != string :: npos )
	{
		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, "File server: " + gmapLANG_CFG["server_response_403"], string( CSS_INDEX ), string( ), NOT_INDEX, CODE_403 );

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INDEX ) );

		return;
	}	
	
	// User requires a level of authorisation
	// if( !( pRequest->user.ucAccess & ACCESS_VIEW ) || !( pRequest->user.ucAccess & ACCESS_UPLOAD ) )
//	if( !( pRequest->user.ucAccess & m_ucAccessView ) )
//	{
//		pResponse->strCode = "401 " + gmapLANG_CFG["server_response_401"];
////		pResponse->mapHeaders.insert( pair<string, string>( "WWW-Authenticate", string( "Basic realm=\"" ) + gstrRealm + "\"" ) );

//		return;
//	}

	// Strip bnbt_file_dir from request
	string strFile = "";

	if( !m_strFileDir.empty( ) )
		strFile = UTIL_EscapedToString( pRequest->strURL.substr( (int)m_strFileDir.size( ) + 1 ) );
	else
		strFile = UTIL_EscapedToString( pRequest->strURL );

	// Replace the path seperators for OS dependancy
	for( unsigned char ucPos = 0; ucPos < strFile.size( ); ucPos++ )
	{
		if( strFile.substr( ucPos,1 ) == STR_PATH_SEP )
			strFile.replace( ucPos, 1, STR_PATH_SEP );
	}

	// Get the file extension from the request
	const string cstrExt( getFileExt( strFile ) );

	// Make the local path from the bnbt_file_dir and the request
	const string cstrPath( m_strFileDir + strFile );

	// Serve the file if it exists
	if( UTIL_CheckFile( cstrPath.c_str( ) ) )
	{
		pResponse->strCode = "200 " + gmapLANG_CFG["server_response_200"];

		pResponse->mapHeaders.insert( pair<string, string>( "Content-Type", gmapMime[cstrExt] ) );

		// cache for awhile

		time_t tNow = time( 0 ) + m_uiFileExpires * 60;
		char *szTime = asctime( gmtime( &tNow ) );
		szTime[strlen( szTime ) - 1] = TERM_CHAR;

		pResponse->mapHeaders.insert( pair<string, string>( "Expires", string( szTime ) + " GMT" ) );

		pResponse->bCompressOK = false;
		
		pResponse->strContent = UTIL_ReadFile( cstrPath.c_str( ) );
	}
	else
	{
		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, "File server: " + gmapLANG_CFG["server_response_404"], string( CSS_INDEX ), string( ), NOT_INDEX, CODE_404 );

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INDEX ) );
	}
}
