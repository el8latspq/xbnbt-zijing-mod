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
#include "tracker.h"
#include "util.h"
#include "gd.h"

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

void CTracker :: serverResponseUserbar( struct request_t *pRequest, struct response_t *pResponse )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
		if( IsIPBanned( pRequest, pResponse, btv, "File server", string( CSS_INDEX ), NOT_INDEX ) )
			return;

	pResponse->strCode = "200 " + gmapLANG_CFG["server_response_200"];


	string strUserbar = pRequest->strURL.substr( 9 );
	const string :: size_type ciExt( strUserbar.rfind( "." ) );
	const string cstrExt( getFileExt( strUserbar ) );

	if( ciExt != string :: npos && UTIL_ToLower( cstrExt ) == ".png" )
	{
		string strUID = strUserbar.substr( 0, ciExt );

		if( strUID.find_first_not_of( "1234567890" ) != string :: npos )
			strUID.erase( );

		if( !strUID.empty( ) )
		{
			CMySQLQuery *pQueryUser = new CMySQLQuery( "SELECT busername,baccess,bgroup,buploaded,bdownloaded,bbonus FROM users WHERE buid=" + strUID );
			
			vector<string> vecQueryUser;
			
			vecQueryUser.reserve(6);

			vecQueryUser = pQueryUser->nextRow( );
			
			delete pQueryUser;

			if( vecQueryUser.size( ) == 6 )
			{
				int64 ulUploaded = 0;
				int64 ulDownloaded = 0;
				int64 ulBonus = 0;
				float flShareRatio = 0;

				unsigned char ucAccess = (unsigned char)atoi( vecQueryUser[1].c_str( ) );
				unsigned char ucGroup = (unsigned char)atoi( vecQueryUser[2].c_str( ) );
				ulUploaded = UTIL_StringTo64( vecQueryUser[3].c_str( ) );
				ulDownloaded = UTIL_StringTo64( vecQueryUser[4].c_str( ) );
				ulBonus = UTIL_StringTo64( vecQueryUser[5].c_str( ) );
				if( ulDownloaded == 0 )
				{
					if( ulUploaded == 0 )
						flShareRatio = 0;
					else
						flShareRatio = -1;
				}
				else
					flShareRatio = (float)ulUploaded / (float)ulDownloaded;
				
				char szFloat[16];
				string strShareRatio = string( );
				if( ( -1.001 < flShareRatio ) && ( flShareRatio < -0.999 ) )
					strShareRatio = gmapLANG_CFG["perfect"];
				else
				{
					memset( szFloat, 0, sizeof( szFloat ) / sizeof( char ) );
					snprintf( szFloat, sizeof( szFloat ) / sizeof( char ), "%0.3f", flShareRatio );
					strShareRatio = string( szFloat );
				}

				bool bShareRatioWarned = checkShareRatio( ulDownloaded, flShareRatio );
				string strClass = UTIL_UserClass( ucAccess, ucGroup );
				if( m_bRatioRestrict && bShareRatioWarned && strClass == gmapLANG_CFG["class_member"] )
					strClass = "share_warned";

				gdImagePtr imWorking = 0;
				gdImagePtr imMaster = 0;
				char *pData = 0;

				int brect[8];

				string strFontEN = CFG_GetString( "userbar_font_en", string( ) );
				string strFontCN = CFG_GetString( "userbar_font_cn", string( ) );

				int iSize = 0;

				unsigned int uiXSize, uiYSize;
				unsigned int uiBg = 0;
				unsigned int uiUsercolor = 0;
				unsigned int uiFontcolor = 0;

				string strBgColor = CFG_GetString( "userbar_bg_color", string( "204,204,255" ) );
				string strUserColor = CFG_GetString( "userbar_user_color_" + strClass, string( "0,0,0" ) );
				string strFontColor = CFG_GetString( "userbar_font_color", string( "0,0,0" ) );
				string strX =  CFG_GetString( "userbar_x", string( "24,150,240,354,472" ) );
				string strY =  CFG_GetString( "userbar_y", string( "14,14,14,14,14" ) );

				vector<string> vecBgColor;
				vecBgColor.reserve(3);
				vector<string> vecUserColor;
				vecUserColor.reserve(3);
				vector<string> vecFontColor;
				vecFontColor.reserve(3);

				vector<string> vecX;
				vecX.reserve(5);
				vector<string> vecY;
				vecY.reserve(5);

				vecBgColor = UTIL_SplitToVector( strBgColor, "," );
				vecUserColor = UTIL_SplitToVector( strUserColor, "," );
				vecFontColor = UTIL_SplitToVector( strFontColor, "," );
				vecX = UTIL_SplitToVector( strX, "," );
				vecY = UTIL_SplitToVector( strY, "," );

				int uiBgRed = atoi( vecBgColor[0].c_str( ) );
				int uiBgGreen = atoi( vecBgColor[1].c_str( ) );
				int uiBgBlue = atoi( vecBgColor[2].c_str( ) );
				int uiUserRed = atoi( vecUserColor[0].c_str( ) );
				int uiUserGreen = atoi( vecUserColor[1].c_str( ) );
				int uiUserBlue = atoi( vecUserColor[2].c_str( ) );
				int uiFontRed = atoi( vecFontColor[0].c_str( ) );
				int uiFontGreen = atoi( vecFontColor[1].c_str( ) );
				int uiFontBlue = atoi( vecFontColor[2].c_str( ) );
				int uiX1 = atoi( vecX[0].c_str( ) );
				int uiX2 = atoi( vecX[1].c_str( ) );
				int uiX3 = atoi( vecX[2].c_str( ) );
				int uiX4 = atoi( vecX[3].c_str( ) );
				int uiX5 = atoi( vecX[4].c_str( ) );
				int uiY1 = atoi( vecY[0].c_str( ) );
				int uiY2 = atoi( vecY[1].c_str( ) );
				int uiY3 = atoi( vecY[2].c_str( ) );
				int uiY4 = atoi( vecY[3].c_str( ) );
				int uiY5 = atoi( vecY[4].c_str( ) );

				if( !userbar.strFile.empty( ) )
				{
					imMaster = gdImageCreateFromPngPtr( userbar.strFile.size( ), (void *)userbar.strFile.c_str( ) );
				}
				else
					imMaster = gdImageCreate( 550, 20 );

				uiXSize = gdImageSX( imMaster );
				uiYSize = gdImageSY( imMaster );

				if( imMaster->trueColor != 0 )
					imWorking = gdImageCreateTrueColor( uiXSize, uiYSize );
				else
					imWorking = gdImageCreate( uiXSize, uiYSize );

				gdImageCopy(imWorking, imMaster, 0, 0, 0, 0, uiXSize, uiYSize );

				uiBg = gdImageColorResolve(imWorking, uiBgRed, uiBgGreen, uiBgBlue);  
				uiUsercolor = gdImageColorResolve(imWorking, uiUserRed, uiUserGreen, uiUserBlue);  

				uiFontcolor = gdImageColorResolve(imWorking, uiFontRed, uiFontGreen, uiFontBlue);  

				gdImageStringFT( imWorking, &brect[0], uiUsercolor, (char *)strFontCN.c_str( ), 10.0, 0.0, uiX1, uiY1, (char *)vecQueryUser[0].c_str( ) );
				gdImageStringFT( imWorking, &brect[0], uiFontcolor, (char *)strFontEN.c_str( ), 10.0, 0.0, uiX2, uiY2, (char *)UTIL_BytesToString( ulUploaded ).c_str( ) );
				gdImageStringFT( imWorking, &brect[0], uiFontcolor, (char *)strFontEN.c_str( ), 10.0, 0.0, uiX3, uiY3, (char *)UTIL_BytesToString( ulDownloaded ).c_str( ) );
				gdImageStringFT( imWorking, &brect[0], uiFontcolor, (char *)strFontEN.c_str( ), 10.0, 0.0, uiX4, uiY4, (char *)strShareRatio.c_str( ) );
				gdImageStringFT( imWorking, &brect[0], uiFontcolor, (char *)strFontEN.c_str( ), 10.0, 0.0, uiX5, uiY5, (char *)string( CAtomLong( ulBonus / 100 ).toString( ) + "." + CAtomInt( ( ulBonus % 100 ) / 10 ).toString( ) + CAtomInt( ulBonus % 10 ).toString( ) ).c_str( ) );

//				FILE *pImageoutFile = FILE_ERROR;
//				pImageoutFile = fopen( "test.png", "wb" );
//				gdImagePngEx( imWorking, pImageoutFile, 0 );
//				fclose( pImageoutFile );
//
				pData = (char *)gdImagePngPtrEx( imWorking, &iSize, 1 );

				pResponse->mapHeaders.insert( pair<string, string>( "Content-Type", gmapMime[cstrExt] ) );

				// cache for awhile

				time_t tNow = time( 0 ) + 1 * 60;
				char *szTime = asctime( gmtime( &tNow ) );
				szTime[strlen( szTime ) - 1] = TERM_CHAR;

				pResponse->mapHeaders.insert( pair<string, string>( "Expires", string( szTime ) + " GMT" ) );

				pResponse->bCompressOK = false;
				
				pResponse->strContent = string( pData, iSize );

				gdImageDestroy( imWorking );
				gdImageDestroy( imMaster );
				gdFree( pData );
			}
		}
	}

}
