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

#include <fcntl.h>

#if defined ( WIN32 )
 #include <time.h>
#else
 #include <sys/time.h>
#endif

#include "bnbt.h"
#include "bnbt_mysql.h"
#include "atom.h"
#include "html.h"
#include "config.h"
#include "tracker.h"
#include "util.h"

void CTracker :: serverResponseLog( struct request_t *pRequest, struct response_t *pResponse )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
		if( IsIPBanned( pRequest, pResponse, btv, gmapLANG_CFG["log_page"], string( CSS_LOG ), NOT_INDEX ) )
			return;
	
	if( !pRequest->user.strUID.empty( ) && ( pRequest->user.ucAccess & m_ucAccessViewLog ) )
	{
		if( !gstrLogDir.empty( ) && !gstrLogFilePattern.empty( ) )
		{
			unsigned int iShowLog = CFG_GetInt( "bnbt_log_show_day", 3 );
			
			time_t tNow = time( 0 );
			
			struct tm tmToday = *localtime( &tNow );

			
			string strLog = string( );

			// Output common HTML head
			HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["log_page"], string( CSS_LOG ), string( ), NOT_INDEX, CODE_200 );

			pResponse->strContent += "<table class=\"log_main\"><tr><td class=\"log_title\">";
			pResponse->strContent += "<p class=\"logtitle\" id=\"top\">" + gmapLANG_CFG["log_top"] + "\n</p></td></tr>";
			pResponse->strContent += "<tr><td class=\"log_top\"><table class=\"log_table\">\n";

			for( unsigned int i = 0; i < iShowLog; i++ )
			{
				
				mktime( &tmToday );
				char pTime[256];
				memset( pTime, 0, sizeof( pTime ) / sizeof( char ) );
				strftime( pTime, sizeof( pTime ) / sizeof( char ), gstrLogFilePattern.data( ), &tmToday );
				tmToday.tm_mday--;
				
				const string cstrFile( gstrLogDir + pTime );
				
				pResponse->strContent += "<tr>\n";
				pResponse->strContent += "<td class=\"log_name\"><a class=\"log\" href=\"" + RESPONSE_STR_LOG_HTML + "#log" + CAtomInt( i ).toString( ) + "\">" + cstrFile + "</a></td>";
				pResponse->strContent += "</tr>\n";
				
			}
			pResponse->strContent += "</table></td></tr></table>\n\n";
			
			tmToday = *localtime( &tNow );
			
			for( unsigned int i = 0; i < iShowLog; i++ )
			{
				mktime( &tmToday );
				char pTime[256];
				memset( pTime, 0, sizeof( pTime ) / sizeof( char ) );
				strftime( pTime, sizeof( pTime ) / sizeof( char ), gstrLogFilePattern.data( ), &tmToday );
				tmToday.tm_mday--;
				
				const string cstrFile( gstrLogDir + pTime );
				
				if( access( string( cstrFile ).c_str( ), 0 ) == 0 )
					strLog = UTIL_ReadFile( cstrFile.c_str( ) );
				else
					strLog = string( );
				
				pResponse->strContent += "<table class=\"log_main\"><tr><td class=\"log_title\">\n";
				pResponse->strContent += "<p class=\"logtitle\" id=\"log" + CAtomInt( i ).toString( ) + "\">" + UTIL_RemoveHTML( cstrFile );
				pResponse->strContent += " - <a class=\"log\" href=\"" + RESPONSE_STR_LOG_HTML + "#top" + "\">" + gmapLANG_CFG["log_top"] + "</a></p></td></tr>";
				if( !strLog.empty( ) )
				{
					pResponse->strContent += "<tr><td class=\"log_table\"><table class=\"log_table\" summary=\"file info\">\n";
					pResponse->strContent += "<tr class=\"log_content\">";
					pResponse->strContent += "<td class=\"log_content\">" + UTIL_RemoveHTML( strLog ) + "</td>\n</tr>\n";

					pResponse->strContent += "</table></td></tr>\n";
				}
				pResponse->strContent += "</table>\n\n";
			}
			// Output common HTML tail
			HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_LOG ) );
		}


		
	}
	else
	{
		// Not authorised

		// Output common HTML head
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["log_page"], string( CSS_LOG ), string( ), NOT_INDEX, CODE_401 );

		// Output common HTML tail
		HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_LOG ) );
	}
// 	}
}
