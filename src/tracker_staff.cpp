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
#include "html.h"
#include "config.h"
#include "sort.h"
#include "tracker.h"
#include "util.h"

void CTracker :: serverResponseStaff( struct request_t *pRequest, struct response_t *pResponse )
{
	// Set the start time
	const struct bnbttv btv( UTIL_CurrentTime( ) );

	// Verify that the IP is permitted to access the tracker
	if( m_ucIPBanMode != 0 )
		if( IsIPBanned( pRequest, pResponse, btv, gmapLANG_CFG["staff_page"], string( CSS_STAFF ), NOT_INDEX ) )
			return;
		
//	CMySQLQuery *pQuery = new CMySQLQuery( "SELECT buid,busername,bcreated,baccess,bgroup FROM users" );
//		
//	vector<string> vecQuery;

//	vecQuery.reserve(5);

//	vecQuery = pQuery->nextRow( );
	
//	if( vecQuery.size( ) == 5 )
//	{
		// Populate the users structure for display

//		const unsigned long culKeySize( (unsigned long)pQuery->numRows( ) );

		// add the users into this structure one by one and sort it afterwards

//		struct user_t *pUsersT = new struct user_t[culKeySize];

		unsigned long culKeySize = 0;
			
		struct user_t *pUsersT = 0;
		
		if( m_pCache )
		{
//			m_pCache->ResetUsers( );
			pUsersT = m_pCache->getCacheUsers( );
			culKeySize = m_pCache->getSizeUsers( );
		}

		unsigned long ulCount = 0;
		
		m_pCache->sortUsers( SORTU_ACREATED );
		
//		qsort( pUsersT, culKeySize, sizeof( struct user_t ), asortuByCreated );
		
		unsigned char ucAccess = ACCESS_ADMIN;
		unsigned char ucGroup = GROUP_VIP;
		
		if( !pRequest->user.strUID.empty( ) && ( pRequest->user.ucAccess & m_ucAccessView ) )
		{
			HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["staff_page"], string( CSS_STAFF ), string( ), NOT_INDEX, CODE_200 );
			while( ucAccess > ACCESS_COMMENTS )
			{
// 				pResponse->strContent += "<table class=\"staff_main\"><tr><td class=\"staff_title\">\n";
// 				pResponse->strContent += "<p class=\"stafftitle\" id=\"" + UTIL_AccessToString( ucAccess ) + "\">" + UTIL_AccessToString( ucAccess );
// 				pResponse->strContent += "</p></td></tr>\n";
// 				pResponse->strContent += "<tr><td class=\"staff_table\"><table class=\"staff_table\">\n";
// 				pResponse->strContent += "<tr class=\"staff\">";
				
				ulCount = 0;
				bool bFound = false;
				
				for( unsigned long ulKey = 0; ulKey < culKeySize; ulKey++ )
				{
					if( ( pUsersT[ulKey].ucAccess & ucAccess ) && pUsersT[ulKey].ucGroup == 0 )
					{
						if( !bFound )
						{
							pResponse->strContent += "<table class=\"staff_main\"><tr><td class=\"staff_title\">\n";
							pResponse->strContent += "<p class=\"stafftitle\" id=\"" + UTIL_AccessToString( ucAccess ) + "\">" + UTIL_AccessToString( ucAccess );
							pResponse->strContent += "</p></td></tr>\n";
							pResponse->strContent += "<tr><td class=\"staff_table\"><table class=\"staff_table\">\n";
							pResponse->strContent += "<tr class=\"staff\">";
						}
						bFound = true;
						if( ulCount > 0 && ( ulCount % 3 == 0 ) )
							pResponse->strContent += "</tr>\n<tr class=\"staff\">";
						pResponse->strContent += "<td class=\"staff\">";
						pResponse->strContent += getUserLink( pUsersT[ulKey].strUID, pUsersT[ulKey].strLogin );
						pResponse->strContent += "</td>";
						pUsersT[ulKey].ucAccess = pUsersT[ulKey].ucAccess & ucAccess;
						ulCount++;
					}
				}
				if( bFound )
				{
					for( unsigned long ulKey = 0; ulKey < ( 3 - ulCount % 3 ) % 3; ulKey++ )
						pResponse->strContent += "<td class=\"staff\"></td>";
					pResponse->strContent += "</tr>\n";
					pResponse->strContent += "</table></td></tr>\n";
					pResponse->strContent += "</table>\n\n";
				}
				
				ucAccess = ucAccess >> 1;
			}
			
			while( ucGroup > 0 )
			{
// 				pResponse->strContent += "<table class=\"staff_main\"><tr><td class=\"staff_title\">\n";
// 				pResponse->strContent += "<p class=\"stafftitle\" id=\"" + UTIL_GroupToString( ucGroup ) + "\">" + UTIL_GroupToString( ucGroup );
// 				pResponse->strContent += "</p></td></tr>\n";
// 				pResponse->strContent += "<tr><td class=\"staff_table\"><table class=\"staff_table\">\n";
// 				pResponse->strContent += "<tr class=\"staff\">";
				
				bool bFound = false;
				ulCount = 0;
				
				for( unsigned long ulKey = 0; ulKey < culKeySize; ulKey++ )
				{
					if( pUsersT[ulKey].ucGroup & ucGroup )
					{
						if( !bFound )
						{
							pResponse->strContent += "<table class=\"staff_main\"><tr><td class=\"staff_title\">\n";
							pResponse->strContent += "<p class=\"stafftitle\" id=\"" + UTIL_GroupToString( ucGroup ) + "\">" + UTIL_GroupToString( ucGroup );
							pResponse->strContent += "</p></td></tr>\n";
							pResponse->strContent += "<tr><td class=\"staff_table\"><table class=\"staff_table\">\n";
							pResponse->strContent += "<tr class=\"staff\">";
						}
						bFound = true;
						if( ulCount > 0 && ( ulCount % 3 == 0 ) )
							pResponse->strContent += "</tr>\n<tr class=\"staff\">";
						pResponse->strContent += "<td class=\"staff\">";
						pResponse->strContent += getUserLink( pUsersT[ulKey].strUID, pUsersT[ulKey].strLogin );
						pResponse->strContent += "</td>";
// 						pUsersT[ulKey].ucAccess = pUsersT[ulKey].ucAccess & ucAccess;
						ulCount++;
					}
				}
				if( bFound )
				{
					for( unsigned long ulKey = 0; ulKey < ( 3 - ulCount % 3 ) % 3; ulKey++ )
						pResponse->strContent += "<td class=\"staff\"></td>";
					pResponse->strContent += "</tr>\n";
					pResponse->strContent += "</table></td></tr>\n";
					pResponse->strContent += "</table>\n\n";
				}
				
				ucGroup = ucGroup >> 1;
			}
			// Output common HTML tail
			HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_STAFF ) );
		}
		else
		{
			// Not authorised

			// Output common HTML head
			HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["staff_page"], string( CSS_STAFF ), string( ), NOT_INDEX, CODE_401 );

			// Output common HTML tail
			HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_STAFF ) );
		}
//		delete [] pUsersT;
//	}
//	delete pQuery;
}
