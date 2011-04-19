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
	const clienttype_t KnownClientMask[63] =
	{
		{ '-', 'A', 'Z',  -1,  -1,  -1,  -1, '-',  -1,  -1, VERSION_METHOD_1, "Azureus ", "Azureus", "%c.%c.%c.%c" },
		{ '-', 'U', 'T',  -1,  -1,  -1,  -1, '-',  -1,  -1, VERSION_METHOD_1, "uTorrent", "uTorrent", "%c.%c.%c.%c" },
		{ '-', 'T', 'R',  -1,  -1,  -1,  -1, '-',  -1,  -1, VERSION_METHOD_1, "Transmission", "Transmission", "%c.%c.%c.%c" },
		{ '-', 'A', 'Z',  -1,  -1,  -1,  -1, '-',  -1,  -1, VERSION_METHOD_1, "Java/", "Azureus", "%c.%c.%c.%c" },
		{ '-', 'A', 'Z',  -1,  -1,  -1,  -1, '-',  -1,  -1, VERSION_METHOD_1, "BitTorrent/", "Azureus", "%c.%c.%c.%c" },
		{ '-', 'A', 'Z',  -1,  -1,  -1,  -1, '-',  -1,  -1, VERSION_METHOD_1, "BitTorrent PRO", "BitTorrent PRO", "%c.%c.%c.%c" },
		{ '-', 'A', 'Z',  -1,  -1,  -1,  -1, '-',  -1,  -1, VERSION_METHOD_1, "FastTorrent", "FastTorrent", "%c.%c.%c.%c" },
		{ '-', 'A', 'R',  -1,  -1,  -1,  -1, '-',  -1,  -1, VERSION_METHOD_1, "BitTorrent PRO", "Arctic", "%c.%c.%c.%c" },
		{ '-', 'B', 'B',  -1,  -1,  -1,  -1, '-',  -1,  -1, VERSION_METHOD_1, "", "BitBuddy", "%c.%c.%c.%c" },
		{ '-', 'B', 'C',  -1,  -1,  -1,  -1, '-',  -1,  -1, VERSION_METHOD_1, "", "BitComet", "%c.%c.%c.%c" },
		{ '-', 'B', 'S',  -1,  -1,  -1,  -1, '-',  -1,  -1, VERSION_METHOD_1, "", "BTSlave", "%c.%c.%c.%c" },
		{ '-', 'B', 'X',  -1,  -1,  -1,  -1, '-',  -1,  -1, VERSION_METHOD_1, "", "Bittorrent X", "%c.%c.%c.%c" },
		{ '-', 'C', 'T',  -1,  -1,  -1,  -1, '-',  -1,  -1, VERSION_METHOD_1, "", "CTorrent", "%c.%c.%c%c" },
		{ '-', 'L', 'T',  -1,  -1,  -1,  -1, '-',  -1,  -1, VERSION_METHOD_8, "ed2k_plugin", "", "" },
		{ '-', 'L', 'T',  -1,  -1,  -1,  -1, '-',  -1,  -1, VERSION_METHOD_1, "", "libtorrent", "%c.%c.%c.%c" },
		{ '-', 'M', 'P',  -1,  -1,  -1,  -1, '-',  -1,  -1, VERSION_METHOD_1, "", "MooPolice", "%c.%c.%c.%c" },
		{ '-', 'M', 'T',  -1,  -1,  -1,  -1, '-',  -1,  -1, VERSION_METHOD_1, "", "MoonlightTorrent", "%c.%c.%c.%c" },
		{ '-', 'S', 'B',  -1,  -1,  -1,  -1, '-',  -1,  -1, VERSION_METHOD_1, "", "Swiftbit", "%c.%c.%c.%c" },
		{ '-', 'S', 'S',  -1,  -1,  -1,  -1, '-',  -1,  -1, VERSION_METHOD_1, "", "SwarmScope", "%c.%c.%c.%c" },
		{ '-', 'T', 'N',  -1,  -1,  -1,  -1, '-',  -1,  -1, VERSION_METHOD_1, "", "TorrentDotNET", "%c.%c.%c.%c" },
		{ '-', 'T', 'S',  -1,  -1,  -1,  -1, '-',  -1,  -1, VERSION_METHOD_1, "BitTorrent/T-", "TorrentStorm T", "%c.%c.%c.%c" },
		{ '-', 'T', 'S',  -1,  -1,  -1,  -1, '-',  -1,  -1, VERSION_METHOD_1, "BitTorrent/S-", "TorrentStorm S", "%c.%c.%c.%c" },
		{ '-', 'X', 'T',  -1,  -1,  -1,  -1, '-',  -1,  -1, VERSION_METHOD_1, "", "XanTorrent", "%c.%c.%c.%c" },
		{ '-', 'Z', 'T',  -1,  -1,  -1,  -1, '-',  -1,  -1, VERSION_METHOD_1, "", "ZipTorrent", "%c.%c.%c.%c" },
		{ 'A',  -1,  -1,  -1, '-', '-', '-', '-',  -1,  -1, VERSION_METHOD_2, "BitTorrent/ABC", "ABC", "%c.%c.%c" },
		{ 'A',  -1,  -1,  -1, '-', '-', '-', '-',  -1,  -1, VERSION_METHOD_2, "ABC 3.", "ABC", "%c.%c.%c" },
		{ 'U',  -1,  -1,  -1, '-', '-', '-', '-',  -1,  -1, VERSION_METHOD_2, "BitTorrent/U-", "UPnP NAT Bit Torrent", "%c.%c.%c" },
		{ '-', 'Q', 't', '-',  -1,  -1,  -1, '-',  -1,  -1, VERSION_METHOD_12, "", "Qt", "%c.%c.%c" },
		{ 'T',  -1,  -1,  -1, '-', '-', '-', '-',  -1,  -1, VERSION_METHOD_3, "BitTorrent/T-", "BitTornado", "%c.%c.%d" },
		{ 'T',  -1,  -1,  -1, '-', '-', '-', '-',  -1,  -1, VERSION_METHOD_3, "BitTornado/T-", "BitTornado", "%c.%c.%d" },
		{ 'T',  -1,  -1,  -1,  -1, '-', '-', '-',  -1,  -1, VERSION_METHOD_3, "BitTornado/T-", "BitTornado", "%c.%c.%d.b" },
		{ 'T',  -1,  -1,  -1, '-', '-',  -1,  -1,  -1,  -1, VERSION_METHOD_3, "BitTornado/T-", "BitTornado", "%c.%c.%d" },
		{ 'S',  -1,  -1,  -1, '-', '-', '-', '-',  -1,  -1, VERSION_METHOD_3, "BitTorrent/S-", "SHAD0W's experimental", "%c.%c.%d" },
		{ 'S',  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, VERSION_METHOD_8, "BitTorrent/S-", "", "" },
		{ 'O',  'P', -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, VERSION_METHOD_11, "", "Opera 8 previews", "%c.%c.%c.%c" },
		{ 'O',  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, VERSION_METHOD_8, "", "Osprey Permaseed", "" },
		{ '3', '4', '6', '-', '-', '-', '-', '-',  -1,  -1, VERSION_METHOD_8, "BitTorrent/", "346", "" },
		{ 'e', 'x', 'b', 'c',  -1,  -1,  -1,  -1,  -1,  -1, VERSION_METHOD_4, "BitTorrent/S-", "BitComet S", "0.%d" },
		{ 'e', 'x', 'b', 'c',  -1,  -1,  -1,  -1,  -1,  -1, VERSION_METHOD_4, "BitTorrent/", "BitComet ", "0.%d" },
		{ 'e', 'x', 'b', 'c',  -1,  -1,  -1,  -1,  -1,  -1, VERSION_METHOD_4, "Mozilla/4.0", "BitComet (Mozilla/4.0)", "0.%d" },
		{ 'M',  -1, '-',  -1, '-',  -1, '-', '-',  -1,  -1, VERSION_METHOD_5, "BitTorrent/", "MainLine", "%c.%c.%c" },
		{ 'M', 'b', 'r', 's', 't',  -1, '-',  -1, '-',  -1, VERSION_METHOD_9, "BitTorrent/brst", "Burst", "%c.%c.%c" },
		{ 'M', 'b', 'r', 's', 't',  -1, '-',  -1, '-',  -1, VERSION_METHOD_9, "BitTorrent/", "Burst", "%c.%c.%c" },
		{ 'X', 'B', 'T',  -1,  -1,  -1, '-', '-',  -1,  -1, VERSION_METHOD_7, "XBT", "XBT", "%c.%c.%c" },
		{ 'X', 'B', 'T',  -1,  -1,  -1, '-', '-',  -1,  -1, VERSION_METHOD_7, "", "XBT", "%c.%c.%c" },
		{ 'e', 'X',  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, VERSION_METHOD_8, "eXeem ", "", "" },
		{ 'e', 'X',  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, VERSION_METHOD_8, "ExLite", "", "" },
		{ 'U', 'p', 'n', 'p',  -1,  -1,  -1,  -1,  -1,  -1, VERSION_METHOD_8, "RAZA ", "Upnp", "" },
		{ '-', 'B', 'O', 'W', 'P', '0', '3', '-',  -1,  -1, VERSION_METHOD_8, "", "", "" },
		{ '-', 'G', '3',  -1,  -1,  -1,  -1,  -1,  -1,  -1, VERSION_METHOD_8, "BitTorrent/", "G3 Torrent", "" },
		{ '-', 'G', '3',  -1,  -1,  -1,  -1,  -1,  -1,  -1, VERSION_METHOD_8, "Python-urllib/", "G3 Torrent", "" },
		{  -1,  -1, 'B', 'S',  -1,  -1,  -1,  -1,  -1,  -1, VERSION_METHOD_8, "BitTorrent/BitSpirit", "BitSpirit", "" },
		{  -1,  -1, 'B', 'S',  -1,  -1,  -1,  -1,  -1,  -1, VERSION_METHOD_8, "BitTorrent/", "BitSpirit", "" },
		{  -1,  -1, 'B', 'S',  -1,  -1,  -1,  -1,  -1,  -1, VERSION_METHOD_8, "BitTorrent/3.4.2 WebWasher 3.3", "", "" },
		{ 'F', 'U', 'T', 'B',  -1,  -1,  -1,  -1,  -1,  -1, VERSION_METHOD_8, "BitTorrent/", "FUTB", "" },
		{  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, VERSION_METHOD_8, "Python-urllib/", "", "" },
		{  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, VERSION_METHOD_8, "RAZA ", "Shareeza", "" },
		{  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, VERSION_METHOD_8, "Etomi", "", "" },
		{  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, VERSION_METHOD_8, "Mozilla/3.0 (compatible; Indy Library)", "", "" },
		{  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, VERSION_METHOD_8, "MLdonkey/", "", "" },
		{  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, VERSION_METHOD_8, "MLdonkey", "", "" },
		{  -1, 'R',  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, VERSION_METHOD_10, "Rufus/", "Rufus", "0.%d.%d" },
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
				strClientType += UTIL_Xsprintf( KnownClientMask[cKnownClientMaskCount].strVersionMask.c_str( ), cstrPeerID[1], cstrPeerID[2], UTIL_HexToDec( cstrPeerID[3] ) );

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

	//string strIdentity = UTIL_Xsprintf( "%d.%d.%d.%d.%d.%d.%d.%d", cstrPeerID[0], cstrPeerID[1], cstrPeerID[2], cstrPeerID[3], cstrPeerID[4], cstrPeerID[5], cstrPeerID[6], cstrPeerID[7] );

	// Signal that the peer has had it's client type identified
	bClientTypeIdentified = true;
}

void CTracker :: serverResponseStats( struct request_t *pRequest, struct response_t *pResponse )
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

	if( pRequest->user.ucAccess & ACCESS_VIEW )
	{
		HTML_Common_Begin(  pRequest, pResponse, gmapLANG_CFG["stats_page"], string( CSS_STATS ), string( ), NOT_INDEX, CODE_200 );
		
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

		// get the info hash from the parameter
		// const string cstrOldName( pRequest->mapParams["oldname"] );
		// const string cstrName( pRequest->mapParams["name"] );
		const string cstrHashString( pRequest->mapParams["info_hash"] );
		const string cstrHash( UTIL_StringToHash( cstrHashString ) );
		const string cstrPostNum( pRequest->mapParams["post"] );
		string strFileName = string( );

		if( !cstrHash.empty( ) )
		{
			//
			// admin
			//
			CAtom *pDicti = 0;

			if( m_pTags )
				pDicti = m_pTags->getItem( cstrHash );
			
			const string strRatio[4] = { "5%", "10%", "15%", "20%" };

			string strOldName = string( );
			string strOldUploader = string( );
			string strOldInfoLink = string( );
			string strOldIntr = string( );
			string strOldIntrImg = string( );
			string strOldRatio = string( );
			string strOldAllow = string( );

			if( pDicti && pDicti->isDicti( ) )
			{
				CAtom *pOldName = ( (CAtomDicti *)pDicti )->getItem( "name" );
				if( pOldName )
					strOldName = pOldName->toString( );
				CAtom *pOldUploader = ( (CAtomDicti *)pDicti )->getItem( "uploader" );
				if( pOldUploader )
					strOldUploader = pOldUploader->toString( );
				CAtom *pOldInfoLink = ( (CAtomDicti *)pDicti )->getItem( "infolink" );
				if( pOldInfoLink )
					strOldInfoLink = pOldInfoLink->toString( );
				CAtom *pOldRatio = ( (CAtomDicti *)pDicti )->getItem( "ratio" );
				strOldRatio = strRatio[1];
				if( pOldRatio )
					strOldRatio = pOldRatio->toString( );
				CAtom *pOldAllow = ( (CAtomDicti *)pDicti )->getItem( "allow" );
				strOldAllow = "1";
				if( pOldAllow )
					strOldAllow = pOldAllow->toString( );
				if ( UTIL_CheckFile( string( m_strIntrDir + cstrHashString ).c_str( ) ) )
					strOldIntr = UTIL_ReadFile( string( m_strIntrDir + cstrHashString ).c_str( ) );
				if ( UTIL_CheckFile( string( m_strIntrDir + cstrHashString + ".img" ).c_str( ) ) )
					strOldIntrImg = UTIL_ReadFile( string( m_strIntrDir + cstrHashString + ".img" ).c_str( ) );


			}

// 			if( pRequest->user.ucAccess & ACCESS_EDIT )
			if( pRequest->user.ucAccess & ACCESS_VIEW )
			{
				if( m_pTags )
				{
					const string cstrName( pRequest->mapParams["name"] );
					const string cstrTag( pRequest->mapParams["tag"] );
					const string cstrUploader( pRequest->mapParams["uploader"] );
					const string cstrInfoLink( pRequest->mapParams["infolink"] );
					const string cstrIntr( pRequest->mapParams["intr"] );
					const string cstrIntrImg( pRequest->mapParams["intrimg"] );
					const string cstrOK( pRequest->mapParams["ok"] );
					const string cstrRatio( pRequest->mapParams["ratio"] );

					// needs work
					const string cstrIP( (string)inet_ntoa( pRequest->sin.sin_addr ) );
					// needs work
					string strMakePublic = string( );

					if( m_ucMakePublic != 0 && pRequest->mapParams["makepublic"] == "on" )
						strMakePublic = pRequest->mapParams["makepublic"];

					if( m_pAllowed )
					{
						CAtom *pTorrent = m_pAllowed->getItem( cstrHash );

						if( pTorrent && dynamic_cast<CAtomList *>( pTorrent ) )
						{
							vector<CAtom *> vecTorrent;
							vecTorrent.reserve( 6 );
							vecTorrent = dynamic_cast<CAtomList *>( pTorrent )->getValue( );

							if( vecTorrent.size( ) == 6 )
							{
								CAtom *pFileName = vecTorrent[0];

								if( pFileName && !pFileName->toString( ).empty( ) )
									strFileName = UTIL_RemoveHTML( pFileName->toString( ) );
								else
									UTIL_LogPrint( "Stats: public upload: pFileName failed!\n" );
							}
							else
								UTIL_LogPrint( "Stats: public upload: vecTorrent.size failed!\n" );
						}
						else
							UTIL_LogPrint( "Stats: public upload: pTorrent failed!\n" );
					}
					else
						UTIL_LogPrint( "Stats: public upload: m_pAllowed failed!\n" );
					
					if( ( pRequest->user.ucAccess & ACCESS_EDIT ) || ( pRequest->user.ucAccess && ACCESS_UPLOAD && !pRequest->user.strLogin.empty( ) && ( pRequest->user.strLogin == strOldUploader ) ) )
					{
						// needs work pTorrent
						if( cstrOK == "1" )
						{
							if( pRequest->user.ucAccess && ACCESS_UPLOAD && !pRequest->user.strLogin.empty( ) && ( pRequest->user.strLogin == strOldUploader ) )
								addTag( cstrHash, cstrTag, cstrName, strOldUploader, cstrInfoLink, cstrIP, strMakePublic, cstrRatio, "1" );
							else
								addTag( cstrHash, cstrTag, cstrName, cstrUploader, cstrInfoLink, cstrIP, strMakePublic, cstrRatio, "1" );
							UTIL_MakeFile( string( m_strIntrDir + cstrHashString ).c_str( ), cstrIntr );
							UTIL_MakeFile( string( m_strIntrDir + cstrHashString + ".img" ).c_str( ), cstrIntrImg );

							if( m_ucMakePublic != 0 )
							{
								if( gbDebug )
									UTIL_LogPrint( "Stats: public upload: make public enabled\n" );

								if( m_pAllowed && !m_strPublicUploadDir.empty( ) && !m_strAllowedDir.empty( ) && !strFileName.empty( ) )
								{
									if( !strMakePublic.empty( ) )
									{
										CAtom *pDecoded = DecodeFile( string( m_strAllowedDir + strFileName ).c_str( ) );

										if( pDecoded && pDecoded->isDicti( ) )
										{
											if( gbDebug )
												UTIL_LogPrint( "Stats: public upload: adding file\n" );

											UTIL_MakeFile( string( m_strPublicUploadDir + strFileName ).c_str( ), Encode( pDecoded ) );
										}
										else
											UTIL_LogPrint( "Stats: public upload: pDecoded failed!\n" );
									}
									else
									{
										if( gbDebug )
											UTIL_LogPrint( "Stats: public upload: removing file\n" );

										UTIL_DeleteFile( string( m_strPublicUploadDir + strFileName ).c_str( ) );
									}
								}
								else
									UTIL_LogPrint( "Stats: public upload: requirement failed!\n" );
							}
							else if( gbDebug )
								UTIL_LogPrint( "Stats: public upload: make public disabled\n" );

							pResponse->strContent += "<div class=\"changed_stats\">\n";
							pResponse->strContent += "<table class=\"changed_stats\">\n";
							pResponse->strContent += "<tr>\n<td>\n<ul>\n";

							// Changed name to
							if( !cstrName.empty( ) )
								pResponse->strContent += "<li class=\"changed_stats\">" + UTIL_Xsprintf( gmapLANG_CFG["stats_changed_name"].c_str( ), UTIL_RemoveHTML( cstrName ).c_str( ) ) + "</li>\n";

							// Changed tag to
							if( !cstrTag.empty( ) )
								pResponse->strContent += "<li class=\"changed_stats\">" + UTIL_Xsprintf( gmapLANG_CFG["stats_changed_tag"].c_str( ), UTIL_RemoveHTML( cstrTag ).c_str( ) ) + "</li>\n";

							// Changed uploader to
							if( !cstrUploader.empty( ) && ( pRequest->user.ucAccess & ACCESS_EDIT ) )
								pResponse->strContent += "<li class=\"changed_stats\">" + UTIL_Xsprintf( gmapLANG_CFG["stats_changed_uploader"].c_str( ), UTIL_RemoveHTML( cstrUploader ).c_str( ) ) + "</li>\n";

							// Changed info link to
							if( !cstrInfoLink.empty( ) )
								pResponse->strContent += "<li class=\"changed_stats\">" + UTIL_Xsprintf( gmapLANG_CFG["stats_changed_infolink"].c_str( ), UTIL_RemoveHTML( cstrInfoLink ).c_str( ) ) + "</li>\n";

							// Changed IP to -  Needs work
							if( !cstrIP.empty( ) )
								pResponse->strContent += "<li class=\"changed_stats\">" + UTIL_Xsprintf( "IP: %s", UTIL_RemoveHTML( cstrIP ).c_str( ) ) + "</li>\n";

							// Changed make public to -  Needs work
							if( m_ucMakePublic != 0 )
								pResponse->strContent += "<li class=\"changed_stats\">" + UTIL_Xsprintf( "Public: %s", UTIL_RemoveHTML( strMakePublic ).c_str( ) ) + "</li>\n";

							pResponse->strContent += "</ul>\n</td>\n</tr>\n</table>\n</div>\n";

							// Return to stats page
							pResponse->strContent += "<p class=\"changed_stats\">" + UTIL_Xsprintf( gmapLANG_CFG["stats_return"].c_str( ), string( "<a title=\"" + cstrHashString + "\" href=\"" + RESPONSE_STR_STATS_HTML + "?info_hash=" + cstrHashString + "\">").c_str( ), "</a>" ) + "</p>\n";

							// Output common HTML tail
							HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_STATS ) );

							return;
						}
						// needs work
						else if( !cstrTag.empty( ) )
						{
							// The Trinity Edition - Modification Begins
							// The following replaces the OK response with a YES | NO option when CHANGING A TORRENT'S INFORMATION - name/uploader/infolink
							pResponse->strContent += "<p class=\"delete\">" + gmapLANG_CFG["stats_change_q"] + "</p>\n";
							pResponse->strContent += "<p class=\"delete\">\n<a title=\"" + gmapLANG_CFG["yes"] + "\" href=\"" + RESPONSE_STR_STATS_HTML + "?info_hash=" + cstrHashString;

							// name
							if( !cstrName.empty( ) )
								pResponse->strContent += "&amp;name=" + UTIL_StringToEscaped( cstrName );
							// else
							//	pResponse->strContent += "&amp;name=" + UTIL_StringToEscaped( cstrOldName );

							// uploader
							if( !cstrUploader.empty( ) )
								pResponse->strContent += "&amp;uploader=" + UTIL_StringToEscaped( cstrUploader );

							// infolink
							if( !cstrInfoLink.empty( ) )
								pResponse->strContent += "&amp;infolink=" + UTIL_StringToEscaped( cstrInfoLink );
							
							// tag - needs work
							if( !cstrTag.empty( ) )
								pResponse->strContent += "&amp;tag=" + UTIL_StringToEscaped( cstrTag );
							
							if( !cstrRatio.empty( ) )
								pResponse->strContent += "&amp;ratio=" + UTIL_StringToEscaped( cstrRatio );

							if( !cstrIntrImg.empty( ) )
								pResponse->strContent += "&amp;intrimg=" + UTIL_StringToEscaped( cstrIntrImg );
							if( !cstrIntr.empty( ) )
								pResponse->strContent += "&amp;intr=" + UTIL_StringToEscaped( cstrIntr );
				

							// Public/Private
							if( m_ucMakePublic != 0 && strMakePublic == "on" )
								pResponse->strContent += "&amp;makepublic=" + UTIL_StringToEscaped( strMakePublic );

										
							// ok - yes - no
							pResponse->strContent += "&amp;ok=1\">" + gmapLANG_CFG["yes"] + "</a> | <a title=\"" + gmapLANG_CFG["no"] + "\" href=\"" + string( JS_BACK ) + "\">" + gmapLANG_CFG["no"] + "</a></p>\n";

							// Output common HTML tail
							HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_STATS ) );

							return;
						}

						// The Trinity Edition - Modification Begins
						// Removed "(blank values mean no change)" from each field; This information now appears once after the form
						if( pRequest->mapParams.find( "edit" ) != pRequest->mapParams.end( ) )
						{
							const string strEdit( pRequest->mapParams["edit"] );
							if( strEdit == "1" )
							{
						pResponse->strContent += "<div class=\"change_info\">\n";
						pResponse->strContent += "<p class=\"change_info\">" + gmapLANG_CFG["stats_change_info"] + "</p>\n";
						pResponse->strContent += "<p class=\"change_info\">" + UTIL_RemoveHTML( strOldName ) + "</p>\n";
						pResponse->strContent += "<table class=\"change_info\">\n<tr>\n<td>\n";
						pResponse->strContent += "<form method=\"get\" action=\"" + RESPONSE_STR_STATS_HTML + "\">\n";
						pResponse->strContent += "<p class=\"change_info_input\"><input name=\"info_hash\" type=hidden value=\"" + cstrHashString + "\"></p>\n";
						// pResponse->strContent += "<p class=\"change_info_input\"><input name=\"oldname\" type=hidden value=\"" + cstrName + "\"></p>\n";
						pResponse->strContent += "<p class=\"change_info_input\"><input id=\"name\" name=\"name\" alt=\"[" + gmapLANG_CFG["stats_new_name"] + "]\" type=text size=96 maxlength=" + CAtomInt( MAX_FILENAME_LEN ).toString( ) + " value=\"" + UTIL_RemoveHTML( strOldName ) + "\"> <label for=\"name\">" + gmapLANG_CFG["stats_new_name"] + "</label> (&dagger;)</p>\n";
						if( pRequest->user.ucAccess & ACCESS_EDIT )
							pResponse->strContent += "<p class=\"change_info_input\"><input id=\"uploader\" name=\"uploader\" alt=\"[" + gmapLANG_CFG["stats_new_uploader"] + "]\" type=text size=64 maxlength=" + CAtomInt( m_uiNameLength ).toString( ) + " value=\"" + UTIL_RemoveHTML( strOldUploader ) + "\"> <label for=\"uploader\">" + gmapLANG_CFG["stats_new_uploader"] + "</label> (&dagger;&Dagger;)</p>\n";				
						else
							pResponse->strContent += "<p class=\"change_info_input\"><input name=\"uploader\" type=hidden value=\"" + UTIL_RemoveHTML( strOldUploader ) + "\"></p>\n";
						pResponse->strContent += "<p class=\"change_info_input\"><input id=\"infolink\" name=\"infolink\" type=hidden value=\"" + UTIL_RemoveHTML( strOldInfoLink ) + "\"> <label for=\"infolink\">" + "</label></p>\n";
						pResponse->strContent += "<p class=\"change_info_input\"><input id=\"intrimg\" name=\"intrimg\" type=text size=96 maxlength=" + CAtomInt( MAX_INFO_LINK_LEN ).toString( ) + " value=\"" + UTIL_RemoveHTML ( strOldIntrImg ) + "\"> <label for=\"intrimg\">" + gmapLANG_CFG["stats_new_intrimg"] + "</label></p>\n";
						pResponse->strContent += "<p class=\"change_info_input\"><textarea id=\"intr\" name=\"intr\" rows=10 cols=96>" + strOldIntr + "</textarea> <label for=\"intr\"></label></p>\n";

						CAtom *pTagInfo = 0;

						if( m_pTags )
							pTagInfo = m_pTags->getItem( cstrHash );

						string strCur = string( );

						if( pTagInfo && pTagInfo->isDicti( ) )
						{
							CAtom *pCur = ( (CAtomDicti *)pTagInfo )->getItem( "tag" );

							if( pCur )
								strCur = pCur->toString( );
						}

						if( !m_vecTags.empty( ) )
							pResponse->strContent += "<p class=\"change_info_tag\"><select id=\"tag\" name=\"tag\">\n";
						
						unsigned char ucTag = 1;

						for( vector< pair< string, string > > :: iterator it = m_vecTags.begin( ); it != m_vecTags.end( ); it++ )
						{
							pResponse->strContent += "<option value=\""  + CAtomInt( ucTag ).toString( ) + "\"";

// 							if( (*it).first == strCur )
							if( CAtomInt( ucTag ).toString( ) == strCur )
								pResponse->strContent += " selected";

							pResponse->strContent += ">" + UTIL_RemoveHTML( (*it).first ) + "\n";
							
							ucTag++;
						}

						if( !m_vecTags.empty( ) )
							pResponse->strContent += "</select> <label for=\"tag\">" + UTIL_RemoveHTML( gmapLANG_CFG["stats_new_tag"] ) + "</label><br></p>\n";
						
						pResponse->strContent += "<p class=\"change_info_tag\"><select id=\"ratio\" name=\"ratio\">\n";
						
						for( int iratio = 0; iratio < 4; iratio++ )
						{
							pResponse->strContent += "<option";
							if( strRatio[iratio] == strOldRatio )
								pResponse->strContent += " selected";
							pResponse->strContent += ">" + strRatio[iratio] + "\n";
						}
						
						pResponse->strContent += "</select> <label for=\"ratio\">" + UTIL_RemoveHTML( gmapLANG_CFG["stats_new_ratio"] ) + "</label><br></p>";

						// Make Public/Private
						if( m_ucMakePublic == 0 )
						{
							if( gbDebug )
								UTIL_LogPrint( "Stats: public upload: m_ucMakePublic disabled\n" );
						}
						else
						{
							if( gbDebug )
								UTIL_LogPrint( "Stats: public upload: m_ucMakePublic enabled\n" );

							pResponse->strContent += "<p class=\"change_info_tag\"><label for=\"id_makepublic\">Make Public</label>\n";
							pResponse->strContent += "<input name=\"makepublic\" id=\"id_makepublic\" type=checkbox";

							CAtom *pMakePublic = 0;

							if( pTagInfo && pTagInfo->isDicti( ) )
								pMakePublic = ( (CAtomDicti *)pTagInfo )->getItem( "makepublic" );

							if( pMakePublic && pMakePublic->toString( ) == "on" )
								pResponse->strContent += " checked";

							pResponse->strContent += ">\n</p>\n";

							if( m_strPublicUploadDir.empty( ) )
							{
								if( gbDebug )
									UTIL_LogPrint( "Stats: public upload: m_strPublicUploadDir disabled\n" );
							}
							else
							{
								if( gbDebug )
									UTIL_LogPrint( "Stats: public upload: m_strPublicUploadDir enabled\n" );

								if( strFileName.empty( ) )
									UTIL_LogPrint( "Stats: public upload: strFileName missing!\n" );

								if( !strFileName.empty( ) )
								{
									if( UTIL_CheckFile( string( m_strPublicUploadDir + strFileName ).c_str( ) ) )
									{
										if( gbDebug )
											UTIL_LogPrint( "Stats: public upload: .torrent exists\n" );
									}
									else
									{
										if( gbDebug )
											UTIL_LogPrint( "Stats: public upload: .torrent missing\n" );

										CAtom *pDecoded = 0;

										if( !m_strAllowedDir.empty( ) )
										{
											if( UTIL_CheckFile( string( m_strAllowedDir + strFileName ).c_str( ) ) )
												UTIL_LogPrint( "Stats: public upload: .torrent missing in allowed dir\n" );
											else
											{
												if( gbDebug )
													UTIL_LogPrint( "Stats: public upload: decoding .torrent from allowed dir\n" );

												pDecoded = DecodeFile( string( m_strAllowedDir + strFileName ).c_str( ) );
											}
										}

										if( !m_strUploadDir.empty( ) && pDecoded && pDecoded->isDicti( ) )
										{
											if( UTIL_CheckFile( string( m_strUploadDir + strFileName ).c_str( ) ) )
												UTIL_LogPrint( "Stats: public upload: .torrent missing in upload dir\n" );
											else
											{
												if( gbDebug )
													UTIL_LogPrint( "Stats: public upload: .torrent missing in upload dir\n" );

												pDecoded = DecodeFile( string( m_strUploadDir + strFileName ).c_str( ) );
											}
										}

										if( pDecoded && pDecoded->isDicti( ) )
										{
											if( gbDebug )
												UTIL_LogPrint( "Stats: public upload: making encoded file\n" );

											UTIL_MakeFile( string( m_strPublicUploadDir + strFileName ).c_str( ), Encode( pDecoded ) );
										}
										else
											UTIL_LogPrint( "Stats: public upload: invalid decoded contents\n" );
									}
								}
							}
						}

						pResponse->strContent += "<div class=\"change_info_button\">\n";
						pResponse->strContent += Button_Submit( "submit_change", string( gmapLANG_CFG["stats_change_info"] ) );
						pResponse->strContent += "\n</div>\n";
						// The Trinity Edition - Addition Begins
						// The following explains how to leave values unchanged and how to remove values from the database
						pResponse->strContent += "<p class=\"change_info_instruction\">" + gmapLANG_CFG["stats_info_blank"] + "<br>\n";
						// Type REMOVE to clear this value from the database.
						pResponse->strContent += UTIL_Xsprintf( gmapLANG_CFG["stats_info_remove"].c_str( ), "<span class=\"underlined\">", "</span>" ) + "</p>\n";
						pResponse->strContent += "</form>\n</td>\n</tr>\n</table>\n</div>\n\n<hr class=\"stats_hr\">\n\n";
							}
						}
					}
				}
			}

			// display torrent information list

			int64 iSize = 0;
			unsigned int uiFiles = 1;

			if( m_pAllowed )
			{
				// const string cstrName( pRequest->mapParams["name"] );
				CAtom *pTorrent = m_pAllowed->getItem( cstrHash );
// 				CAtom *pDicti = 0;
// 
// 				if( m_pTags )
// 					pDicti = m_pTags->getItem( cstrHash );
// 
// 				string strOldName = string( );
// 
// 				if( pDicti && pDicti->isDicti( ) )
// 				{
// 					CAtom *pOldName = ( (CAtomDicti *)pDicti )->getItem( "name" );
// 
// 					if( pOldName )
// 						strOldName = pOldName->toString( );
// 				}

				if( pTorrent && dynamic_cast<CAtomList *>( pTorrent ) )
				{
					vector<CAtom *> vecTorrent;
					vecTorrent.reserve( 6 );
					vecTorrent = dynamic_cast<CAtomList *>( pTorrent )->getValue( );

					if( vecTorrent.size( ) == 6 )
					{
						//CAtom *pFileName = vecTorrent[0];
						CAtom *pName = vecTorrent[1];
						CAtom *pAdded = vecTorrent[2];
						CAtom *pSize = vecTorrent[3];
						CAtom *pFiles = vecTorrent[4];
						CAtom *pComment = vecTorrent[5];
												
						//CAtom *pDicti = 0;
						//CAtom *pOldName = 0;
						//string strOldName;

						unsigned int uiAdded = 0;
						
						//if( m_pTags )
						//{
						//	pDicti = m_pTags->getItem( cstrHash );
						//	if( pDicti && pDicti->isDicti( ) )
						//		pOldName = ( (CAtomDicti *)pDicti )->getItem( "name" );
						//}
						//if( pName )
							//strOldName = pOldName->toString;
						// if( strOldName.empty( ) )
						//	strOldName = pName->toString( );

						pResponse->strContent += "<div class=\"stats_table\">\n";
						pResponse->strContent += "<p class=\"file_info\">" + gmapLANG_CFG["stats_file_info"] + "</p>\n";
						pResponse->strContent += "<p class=\"file_info\">" + UTIL_RemoveHTML( strOldName ) + "</p>\n";
						
						pResponse->strContent += "<p class=\"stats_menu\">\n";

						if( m_pAllowed && m_bAllowTorrentDownloads && ( pRequest->user.ucAccess & ACCESS_DL ) )
						{
							if( strOldAllow == "1" || ! pRequest->user.strLogin.empty( ) )
							{
								if( m_strExternalTorrentDir.empty( ) )
								{
									pResponse->strContent += "<a title=\"" + cstrHashString + ".torrent" + "\" class=\"download_link\" href=\"";
									pResponse->strContent += RESPONSE_STR_TORRENTS + cstrHashString + ".torrent";
								}
								else
								{
									pResponse->strContent += "<a title=\"" + strFileName + "\" class=\"download_link\" href=\"";
									pResponse->strContent += m_strExternalTorrentDir + UTIL_StringToEscapedStrict( strFileName );
								}

								pResponse->strContent += "\">" + gmapLANG_CFG["stats_download_torrent"] + "</a>\n";
							}
						if( ( pRequest->user.ucAccess & ACCESS_EDIT ) )
						{
							pResponse->strContent += "<span class=\"pipe\">|</span><a title=\"" + gmapLANG_CFG["edit"] + ": " + cstrHashString + "\" class=\"delete_link\" href=\"" + RESPONSE_STR_STATS_HTML + "?info_hash=" + cstrHashString + "&amp;edit=1\">" + gmapLANG_CFG["stats_edit_torrent"] + "</a>";
							pResponse->strContent += "<span class=\"pipe\">|</span><a title=\"" + gmapLANG_CFG["delete"] + ": " + cstrHashString + "\" class=\"delete_link\" href=\"" + RESPONSE_STR_INDEX_HTML + "?del=" + cstrHashString + "\">" + gmapLANG_CFG["stats_delete_torrent"] + "</a>\n";
						}
						else
							if( pRequest->user.ucAccess && ACCESS_UPLOAD && !pRequest->user.strLogin.empty( ) && ( pRequest->user.strLogin == strOldUploader ) )
							{
								pResponse->strContent += "<span class=\"pipe\">|</span><a title=\"" + gmapLANG_CFG["edit"] + ": " + cstrHashString + "\" class=\"delete_link\" href=\"" + RESPONSE_STR_STATS_HTML + "?info_hash=" + cstrHashString + "&amp;edit=1\">" + gmapLANG_CFG["stats_edit_torrent"] + "</a>";
	                                                        pResponse->strContent += "<span class=\"pipe\">|</span><a title=\"" + gmapLANG_CFG["delete"] + ": " + cstrHashString + "\" class=\"delete_link\" href=\"" + RESPONSE_STR_LOGIN_HTML + "?del=" + cstrHashString + "\">" + gmapLANG_CFG["stats_delete_torrent"] + "</a>\n";
							}

						pResponse->strContent += "</p>\n\n";


						pResponse->strContent += "<table class=\"file_info\" summary=\"file info\">\n";

						// needs work
						if( !strFileName.empty( ) )
						{
							uiAdded++;

							if( uiAdded % 2 )
								pResponse->strContent += "<tr class=\"even\">\n";
							else
								pResponse->strContent += "<tr class=\"odd\">\n";

							pResponse->strContent += "<th class=\"file_info\">" + gmapLANG_CFG["filename"] + ":</th>\n";
							pResponse->strContent += "<td class=\"file_info\">" + strFileName + "</td>\n</tr>\n";
						}

						if( pName )
						{
							uiAdded++;

							if( uiAdded % 2 )
								pResponse->strContent += "<tr class=\"even\">\n";
							else
								pResponse->strContent += "<tr class=\"odd\">\n";

							pResponse->strContent += "<th class=\"file_info\">" + gmapLANG_CFG["name"] + ":</th>\n";
							pResponse->strContent += "<td class=\"file_info\">" + UTIL_RemoveHTML( pName->toString( ) ) + "</td>\n</tr>\n";
						}

						if( !cstrHashString.empty( ) )
						{
							uiAdded++;

							if( uiAdded % 2 )
								pResponse->strContent += "<tr class=\"even\">\n";
							else
								pResponse->strContent += "<tr class=\"odd\">\n";

							pResponse->strContent += "<th class=\"file_info\">" + gmapLANG_CFG["info_hash"] + ":</th>\n";
							pResponse->strContent += "<td class=\"file_info\">" + cstrHashString + "</td>\n</tr>\n";
						}

						if( pAdded )
						{
							uiAdded++;

							if( uiAdded % 2 )
								pResponse->strContent += "<tr class=\"even\">\n";
							else
								pResponse->strContent += "<tr class=\"odd\">\n";

							pResponse->strContent += "<th class=\"file_info\">" + gmapLANG_CFG["added"] + ":</th>\n";
							pResponse->strContent += "<td class=\"file_info\">" + pAdded->toString( ) + "</td>\n</tr>\n";
						}

						if( pSize && dynamic_cast<CAtomLong *>( pSize ) )
						{
							uiAdded++;

							if( uiAdded % 2 )
								pResponse->strContent += "<tr class=\"even\">\n";
							else
								pResponse->strContent += "<tr class=\"odd\">\n";

							// cache iSize
							iSize = dynamic_cast<CAtomLong *>( pSize )->getValue( );

							pResponse->strContent += "<th class=\"file_info\">" + gmapLANG_CFG["size"] + ":</th>\n";
							pResponse->strContent += "<td class=\"file_info\">" + UTIL_BytesToString( iSize ) + "</td>\n</tr>\n";
						}

						if( pFiles && dynamic_cast<CAtomInt *>( pFiles ) )
						{
							uiAdded++;

							if( uiAdded % 2 )
								pResponse->strContent += "<tr class=\"even\">\n";
							else
								pResponse->strContent += "<tr class=\"odd\">\n";

							// cache uiFiles	 
							uiFiles = dynamic_cast<CAtomInt *>( pFiles )->getValue( );

							pResponse->strContent += "<th class=\"file_info\">" + gmapLANG_CFG["files"] + ":</th>\n";
							pResponse->strContent += "<td class=\"file_info\">" + pFiles->toString( ) + "</td>\n</tr>\n";
						}
						if( !strOldIntr.empty( ) )
						{
							uiAdded++;

							if( uiAdded % 2 )
								pResponse->strContent += "<tr class=\"even\">\n";
							else
								pResponse->strContent += "<tr class=\"odd\">\n";

							pResponse->strContent += "<th class=\"file_info\">" + gmapLANG_CFG["intr"] + ":</th>\n";
							pResponse->strContent += "<td class=\"file_info\"><img border=0 src=\"" + UTIL_RemoveHTML( strOldIntrImg ) + "\"><br><code>" + UTIL_RemoveHTML( strOldIntr ) + "</code></td>\n</tr>\n";
						}
						else
						{
							uiAdded++;

							if( uiAdded % 2 )
								pResponse->strContent += "<tr class=\"even\">\n";
							else
								pResponse->strContent += "<tr class=\"odd\">\n";

							pResponse->strContent += "<th class=\"file_info\">" + gmapLANG_CFG["intr"] + ":</th>\n";
							pResponse->strContent += "<td class=\"file_info\"><img border=0 src=\"" + UTIL_RemoveHTML( strOldIntrImg ) + "\"><br><code>" + UTIL_RemoveHTML( pName->toString( ) ) + "</code></td>\n</tr>\n";
						}


						pResponse->strContent += "</table>\n</div>\n\n";

						if( m_bShowFileComment )
						{
							if( pComment )
							{
								const string cstrFileComment( UTIL_RemoveHTML( pComment->toString( ) ) );

								if( !cstrFileComment.empty( ) )
								{
									pResponse->strContent += "<div class=\"stats_table\">\n";
									pResponse->strContent += "<p class=\"file_comment\">" + gmapLANG_CFG["file_comment"] + "</p>\n";
									pResponse->strContent += "<table summary=\"file comment\">\n";
									pResponse->strContent += "<tr class=\"file_comment\">\n<td class=\"file_comment\">\n<code>" + cstrFileComment;
									pResponse->strContent += "</code>\n</td>\n</tr>\n</table>\n</div>\n\n";
								}
							}
						}
					}
				}
			}

			// pResponse->strContent += "<p class=\"stats_menu\">\n";

			// if( m_pAllowed && m_bAllowTorrentDownloads && ( pRequest->user.ucAccess & ACCESS_DL ) )
			// {
				// if( m_strExternalTorrentDir.empty( ) )
				// {
					// pResponse->strContent += "<a title=\"" + cstrHashString + ".torrent" + "\" class=\"download_link\" href=\"";
					// pResponse->strContent += RESPONSE_STR_TORRENTS + cstrHashString + ".torrent";
				// }
				// else
				// {
					// pResponse->strContent += "<a title=\"" + strFileName + "\" class=\"download_link\" href=\"";
					// pResponse->strContent += m_strExternalTorrentDir + UTIL_StringToEscapedStrict( strFileName );
				// }

				// pResponse->strContent += "\">" + gmapLANG_CFG["stats_download_torrent"] + "</a>\n";
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

			// The Trinity Edition - Addition Begins
			// The following adds a link to DELETE TORRENT when viewing the TORRENT'S STATS page
			// This link is ONLY DISPLAYED when the user had EDIT ACCESS - Moderators and Admins

			// if( pRequest->user.ucAccess & ACCESS_EDIT )
			//	pResponse->strContent += "<span class=\"pipe\">|</span><a title=\"" + gmapLANG_CFG["delete"] + ": " + cstrHashString + "\" class=\"delete_link\" href=\"" + RESPONSE_STR_INDEX_HTML + "?del=" + cstrHashString + "\">" + gmapLANG_CFG["stats_delete_torrent"] + "</a>\n";

			// pResponse->strContent += "</p>\n\n";

			if( m_bShowFileContents && uiFiles > 1 )
			{
				CAtom *pDecoded = DecodeFile( ( m_strAllowedDir + strFileName ).c_str( ) );

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
											pResponse->strContent += "<p class=\"contents\">" + gmapLANG_CFG["contents"] + "</p>\n";
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
											if( uiAdded % 2 )
												pResponse->strContent += "<tr class=\"even\">\n";
											else
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
					}
				}

				if( pDecoded )
					delete pDecoded;
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
							pResponse->strContent += "<p class=\"fill_all\">" + UTIL_Xsprintf( gmapLANG_CFG["comments_fill_warning"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["comments"] + "\" href=\"" + RESPONSE_STR_STATS_HTML + "?info_hash=" + cstrHashString + "\">" ).c_str( ), "</a>" ) + "</p>\n";

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
						pResponse->strContent += "<p class=\"comments_posted\">" + UTIL_Xsprintf( gmapLANG_CFG["comments_posted"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["comments"] + "\" href=\"" +RESPONSE_STR_STATS_HTML + "?info_hash=" + cstrHashString + "\">" ).c_str( ), "</a>" ) + "</p>\n";

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
								pResponse->strContent += "<div class=\"comments_table_posted\">\n";
								pResponse->strContent += "<p class=\"comments_table_posted\"><a title=\"" + gmapLANG_CFG["comments_page"] + "\" href=\"" + RESPONSE_STR_COMMENTS_HTML + "?info_hash=" + cstrHashString + "\">" + gmapLANG_CFG["comments_page"] + "</a></p>\n";
								pResponse->strContent += "<p>" + gmapLANG_CFG["comments"] + "</p>\n";
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

#ifdef BNBT_MYSQL
			if( !m_bMySQLOverrideDState )
			{
#endif
				// Do we have a global dfile?
				if( m_pDFile )
				{
					// Get a list of peers for the info hash
					CAtom *pPeers = m_pDFile->getItem( cstrHash );

					// Did we get a list of peers?
					if( pPeers && pPeers->isDicti( ) )
					{
						// The peer map to obtain the list of peers
						map<string, CAtom *> *pmapPeersDicti = ( (CAtomDicti *)pPeers )->getValuePtr( );

						// add the peers into this structure one by one and sort it afterwards
						struct peer_t *pPeersT = new struct peer_t[pmapPeersDicti->size( )];

						unsigned long ulCount = 0;

						CAtomDicti *pPeerDicti = 0;
						CAtom *pIP = 0;
						CAtom *pUpped = 0;
						CAtom *pDowned = 0;
						CAtom *pLeft = 0;
						CAtom *pConn = 0;
						CAtom *pUserAgent = 0;
						CAtom *pClientType = 0;
						CAtom *pClientIdentified = 0;

						string :: size_type iStart = 0;

						// Loop through the peers map to obtain the list of peers and their information by peerid
						for( map<string, CAtom *> :: iterator it = pmapPeersDicti->begin( ); it != pmapPeersDicti->end( ); it++ )
						{
							// Initialise peer entry variables
							pPeersT[ulCount].iUpped = 0;
							pPeersT[ulCount].iDowned = 0;
							pPeersT[ulCount].iLeft = 0;
							pPeersT[ulCount].ulConnected = 0;
							pPeersT[ulCount].flShareRatio = 0.0;
							pPeersT[ulCount].strUserAgent = string( );
							pPeersT[ulCount].strClientType = string( );
							pPeersT[ulCount].bClientTypeIdentified = false;

							// Do we have any information for this peer?
							if( (*it).second->isDicti( ) )
							{
								// Get the peer entry values
								pPeerDicti = (CAtomDicti *)(*it).second;
								pIP = pPeerDicti->getItem( "ip" );
								pUpped = pPeerDicti->getItem( "uploaded" );
								pDowned = pPeerDicti->getItem( "downloaded" );
								pLeft = pPeerDicti->getItem( "left" );
								pConn = pPeerDicti->getItem( "connected" );
								pUserAgent = pPeerDicti->getItem( "useragent" );
								pClientType = pPeerDicti->getItem( "clienttype" );
								pClientIdentified = pPeerDicti->getItem( "clientidentified" );

								// PeerID
								pPeersT[ulCount].strPeerID = (*it).first;

								if( m_ucShowPeerInfo != 0 )
								{
									// User-Agent
									if( pUserAgent && dynamic_cast<CAtomString *>( pUserAgent ) )
										pPeersT[ulCount].strUserAgent = dynamic_cast<CAtomString *>( pUserAgent )->toString( );

									// Has the client been identified?
									if( pClientIdentified && dynamic_cast<CAtomInt *>( pClientIdentified ) )
										pPeersT[ulCount].bClientTypeIdentified = dynamic_cast<CAtomInt *>( pClientIdentified )->getValue( ) == 0 ? false : true;

									// Get the client identification
									if( pClientType && dynamic_cast<CAtomString *>( pClientType ) && pPeersT[ulCount].bClientTypeIdentified )
										pPeersT[ulCount].strClientType = dynamic_cast<CAtomString *>( pClientType )->toString( );

									// Set the peer identification
									if( !pPeersT[ulCount].bClientTypeIdentified )
									{
										UTIL_GetClientIdentity( pPeersT[ulCount].strPeerID, pPeersT[ulCount].strUserAgent, pPeersT[ulCount].strClientType, pPeersT[ulCount].bClientTypeIdentified );

										pPeerDicti->setItem( "clienttype", new CAtomString( pPeersT[ulCount].strClientType ) );
										pPeerDicti->setItem( "clientidentified", new CAtomInt( pPeersT[ulCount].bClientTypeIdentified ) );
									}
								}

								// Get the peer IP address
								if( pIP && dynamic_cast<CAtomString *>( pIP ) )
								{
									pPeersT[ulCount].strIP = dynamic_cast<CAtomString *>( pIP )->toString( );

									// cstrIP ip

									iStart = pPeersT[ulCount].strIP.rfind( "." );

									if( iStart != string :: npos )
									{
										// don't cstrIP ip for mods

										if( !( pRequest->user.ucAccess & ACCESS_EDIT ) )
											pPeersT[ulCount].strIP = pPeersT[ulCount].strIP.substr( 0, iStart + 1 ) + "xxx"; 
									}
								}

								// Uploaded
								if( pUpped && dynamic_cast<CAtomLong *>( pUpped ) )
									pPeersT[ulCount].iUpped = dynamic_cast<CAtomLong *>( pUpped )->getValue( );

								// Downloaded
								if( pDowned && dynamic_cast<CAtomLong *>( pDowned ) )
								{
									pPeersT[ulCount].iDowned = dynamic_cast<CAtomLong *>( pDowned )->getValue( );

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
								if( pLeft && dynamic_cast<CAtomLong *>( pLeft ) )
									pPeersT[ulCount].iLeft = dynamic_cast<CAtomLong *>( pLeft )->getValue( );

								// Connected
								if( pConn && dynamic_cast<CAtomLong *>( pConn ) )
									pPeersT[ulCount].ulConnected = GetTime( ) - (unsigned long)dynamic_cast<CAtomLong *>( pConn )->getValue( );
							}

							ulCount++;
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
									qsort( pPeersT, pmapPeersDicti->size( ), sizeof( struct peer_t ), asortpByUpped );
									break;
								case SORTP_ADOWNED:
									qsort( pPeersT, pmapPeersDicti->size( ), sizeof( struct peer_t ), asortpByDowned );
									break;
								case SORTP_ACONNECTED:
									qsort( pPeersT, pmapPeersDicti->size( ), sizeof( struct peer_t ), asortpByConnected );
									break;
								case SORTP_ASHARERATIO:
									if( m_bShowShareRatios )
										qsort( pPeersT, pmapPeersDicti->size( ), sizeof( struct peer_t ), asortpByShareRatio );
									break;
								case SORTP_ACLIENT:
									if( m_ucShowPeerInfo )
										qsort( pPeersT, pmapPeersDicti->size( ), sizeof( struct peer_t ), asortpByClient );
									break;
								case SORTP_AIP:
									if( m_ucShowPeerInfo )
										qsort( pPeersT, pmapPeersDicti->size( ), sizeof( struct peer_t ), asortpByIP );
									break;
								case SORTP_AAUR:
									if( m_ucShowPeerInfo )
										qsort( pPeersT, pmapPeersDicti->size( ), sizeof( struct peer_t ), asortpByAUR );
									break;
								case SORTP_AADR:
									if( m_ucShowPeerInfo )
										qsort( pPeersT, pmapPeersDicti->size( ), sizeof( struct peer_t ), asortpByADR );
									break;
								case SORTP_DUPPED:
									qsort( pPeersT, pmapPeersDicti->size( ), sizeof( struct peer_t ), dsortpByUpped );
									break;
								case SORTP_DDOWNED:
									qsort( pPeersT, pmapPeersDicti->size( ), sizeof( struct peer_t ), dsortpByDowned );
									break;
								case SORTP_DLEFT:
									qsort( pPeersT, pmapPeersDicti->size( ), sizeof( struct peer_t ), dsortpByLeft );
									break;
								case SORTP_DCONNECTED:
									qsort( pPeersT, pmapPeersDicti->size( ), sizeof( struct peer_t ), dsortpByConnected );
									break;
								case SORTP_DSHARERATIO:
									if( m_bShowShareRatios )
										qsort( pPeersT, pmapPeersDicti->size( ), sizeof( struct peer_t ), dsortpByShareRatio );
									break;
								case SORTP_DCLIENT:
									if( m_ucShowPeerInfo )
										qsort( pPeersT, pmapPeersDicti->size( ), sizeof( struct peer_t ), dsortpByClient );
									break;
								case SORTP_DIP:
									if( m_ucShowPeerInfo )
										qsort( pPeersT, pmapPeersDicti->size( ), sizeof( struct peer_t ), dsortpByIP );
									break;
								case SORTP_DAUR:
									if( m_ucShowPeerInfo )
										qsort( pPeersT, pmapPeersDicti->size( ), sizeof( struct peer_t ), dsortpByAUR );
									break;
								case SORTP_DADR:
									if( m_ucShowPeerInfo )
										qsort( pPeersT, pmapPeersDicti->size( ), sizeof( struct peer_t ), dsortpByADR );
									break;
								case SORTP_ALEFT:
								default:
									qsort( pPeersT, pmapPeersDicti->size( ), sizeof( struct peer_t ), asortpByLeft );
								}
							}
							else
							{
								// default action is to sort by left

								qsort( pPeersT, pmapPeersDicti->size( ), sizeof( struct peer_t ), asortpByLeft );
							}
						}
						else
						{
							// sort is disabled, but default action is to sort by left

							qsort( pPeersT, pmapPeersDicti->size( ), sizeof( struct peer_t ), asortpByLeft );
						}

						bool bFound = false;

						unsigned int uiAdded = 0;

						//
						// seeders
						//

						string :: size_type iCountGoesHere = string :: npos;

						char szFloat[16];

						for( unsigned long it = 0; it < pmapPeersDicti->size( ); it++ )
						{
							if( uiAdded >= m_uiMaxPeersDisplay )
								break;

							if( pPeersT[it].iLeft == 0 )
							{
								if( !bFound )
								{
									// output table headers

									pResponse->strContent += "<div class=\"stats_table\">\n";
									pResponse->strContent += "<p class=\"seeders\">" + gmapLANG_CFG["stats_seeders"] + "</p>\n";

									// to save on calculations, we're going to insert the number of seeders later, keep track of where

									iCountGoesHere = pResponse->strContent.size( ) - ( sizeof( "</p>\n" ) - 1 );

									pResponse->strContent += "<table summary=\"seeders\">\n";
									pResponse->strContent += "<tr>\n";
									pResponse->strContent += "<th class=\"ip\">" + gmapLANG_CFG["peer_ip"];

									if( m_bSort )
									{
										pResponse->strContent += "<br>\n";
										pResponse->strContent += "<a title=\"" + gmapLANG_CFG["sort_ip_ascending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_STATS_HTML + "?info_hash=";
										pResponse->strContent += cstrHashString;
										pResponse->strContent += "&amp;sort=";
										pResponse->strContent += SORTPSTR_AIP;
										pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a title=\"" + gmapLANG_CFG["sort_ip_descending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_STATS_HTML + "?info_hash=";
										pResponse->strContent += cstrHashString;
										pResponse->strContent += "&amp;sort=";
										pResponse->strContent += SORTPSTR_DIP;
										pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a>";
									}

									pResponse->strContent += "</th>\n";

									if( m_ucShowPeerInfo != 0 )
									{
										pResponse->strContent += "<th class=\"client\">" + gmapLANG_CFG["client"];

										if( m_bSort )
										{
											pResponse->strContent += "<br>\n";
											pResponse->strContent += "<a title=\"" + gmapLANG_CFG["sort_client_ascending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_STATS_HTML + "?info_hash=";
											pResponse->strContent += cstrHashString;
											pResponse->strContent += "&amp;sort=";
											pResponse->strContent += SORTPSTR_ACLIENT;
											pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a title=\"" + gmapLANG_CFG["sort_client_descending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_STATS_HTML + "?info_hash=";
											pResponse->strContent += cstrHashString;
											pResponse->strContent += "&amp;sort=";
											pResponse->strContent += SORTPSTR_DCLIENT;
											pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a>";
										}

										pResponse->strContent += "</th>\n";
									}

									pResponse->strContent += "<th class=\"bytes\">" + gmapLANG_CFG["uploaded"];

									if( m_bSort )
									{
										pResponse->strContent += "<br>\n";
										pResponse->strContent += "<a title=\"" + gmapLANG_CFG["sort_uploaded_ascending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_STATS_HTML + "?info_hash=";
										pResponse->strContent += cstrHashString;
										pResponse->strContent += "&amp;sort=";
										pResponse->strContent += SORTPSTR_AUPPED;
										pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a title=\"" + gmapLANG_CFG["sort_uploaded_descending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_STATS_HTML + "?info_hash=";
										pResponse->strContent += cstrHashString;
										pResponse->strContent += "&amp;sort=";
										pResponse->strContent += SORTPSTR_DUPPED;
										pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a>";
									}

									pResponse->strContent += "</th>\n";
									pResponse->strContent += "<th class=\"bytes\">" + gmapLANG_CFG["downloaded"];

									if( m_bSort )
									{
										pResponse->strContent += "<br>\n";
										pResponse->strContent += "<a title=\"" + gmapLANG_CFG["sort_downloaded_ascending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_STATS_HTML + "?info_hash=";
										pResponse->strContent += cstrHashString;
										pResponse->strContent += "&amp;sort=";
										pResponse->strContent += SORTPSTR_ADOWNED;
										pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a title=\"" + gmapLANG_CFG["sort_downloaded_descending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_STATS_HTML + "?info_hash=";
										pResponse->strContent += cstrHashString;
										pResponse->strContent += "&amp;sort=";
										pResponse->strContent += SORTPSTR_DDOWNED;
										pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a>";
									}

									pResponse->strContent += "</th>\n";
									pResponse->strContent += "<th class=\"connected\">" + gmapLANG_CFG["connected"];

									if( m_bSort )
									{
										pResponse->strContent += "<br>\n";
										pResponse->strContent += "<a title=\"" + gmapLANG_CFG["sort_connected_ascending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_STATS_HTML + "?info_hash=";
										pResponse->strContent += cstrHashString;
										pResponse->strContent += "&amp;sort=";
										pResponse->strContent += SORTPSTR_ACONNECTED;
										pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a title=\"" + gmapLANG_CFG["sort_connected_descending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_STATS_HTML + "?info_hash=";
										pResponse->strContent += cstrHashString;
										pResponse->strContent += "&amp;sort=";
										pResponse->strContent += SORTPSTR_DCONNECTED;
										pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a>";
									}

									pResponse->strContent += "</th>\n";

									if( m_bShowShareRatios )
									{
										pResponse->strContent += "<th class=\"number\">" + gmapLANG_CFG["share_ratio"];

										if( m_bSort )
										{
											pResponse->strContent += "<br>\n";
											pResponse->strContent += "<a title=\"" + gmapLANG_CFG["sort_ratio_ascending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_STATS_HTML + "?info_hash=";
											pResponse->strContent += cstrHashString;
											pResponse->strContent += "&amp;sort=";
											pResponse->strContent += SORTPSTR_ASHARERATIO;
											pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a title=\"" + gmapLANG_CFG["sort_ratio_descending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_STATS_HTML + "?info_hash=";
											pResponse->strContent += cstrHashString;
											pResponse->strContent += "&amp;sort=";
											pResponse->strContent += SORTPSTR_DSHARERATIO;
											pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a>\n";
										}

										pResponse->strContent += "</th>\n";
									}

									if( m_bShowAvgULRate )
									{
										pResponse->strContent += "<th class=\"number\">" + gmapLANG_CFG["avg_up_rate"];

										if( m_bSort )
										{
											pResponse->strContent += "<br>\n";
											pResponse->strContent += "<a title=\"" + gmapLANG_CFG["sort_aur_ascending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_STATS_HTML + "?info_hash=";
											pResponse->strContent += cstrHashString;
											pResponse->strContent += "&amp;sort=";
											pResponse->strContent += SORTPSTR_AAUR;
											pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a title=\"" + gmapLANG_CFG["sort_aur_descending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_STATS_HTML + "?info_hash=";
											pResponse->strContent += cstrHashString;
											pResponse->strContent += "&amp;sort=";
											pResponse->strContent += SORTPSTR_DAUR;
											pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a>";
										}

										pResponse->strContent += "</th>\n";
									}

									pResponse->strContent += "</tr>\n";

									bFound = true;
								}

								// output table rows

								if( uiAdded % 2 )
									pResponse->strContent += "<tr class=\"even\">\n";
								else
									pResponse->strContent += "<tr class=\"odd\">\n";

								pResponse->strContent += "<td class=\"ip\">" + pPeersT[it].strIP + "</td>\n";

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

								if( m_bShowShareRatios )
								{
									pResponse->strContent += "<td class=\"number_";

									if( ( -1.001 < pPeersT[it].flShareRatio ) && ( pPeersT[it].flShareRatio < -0.999 ) )
										pResponse->strContent += "green\">";
									else if( pPeersT[it].flShareRatio < 0.800 )
										pResponse->strContent += "red\">";
									else if( pPeersT[it].flShareRatio < 1.200 )
										pResponse->strContent += "yellow\">";
									else
										pResponse->strContent += "green\">";

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

								if( m_bShowAvgULRate )
								{
									pResponse->strContent += "<td class=\"number\">";

									if( pPeersT[it].ulConnected > 0 )
										pResponse->strContent += UTIL_BytesToString( pPeersT[it].iUpped / pPeersT[it].ulConnected ) + RESPONSE_STR_SEPERATOR + gmapLANG_CFG["sec"];
									else
										pResponse->strContent += gmapLANG_CFG["na"];

									pResponse->strContent += "</td>\n";
								}

								pResponse->strContent += "</tr>\n";

								uiAdded++;
							}
						}

						// insert the number of seeders

						string strTemp = " (" + CAtomInt( uiAdded ).toString( ) + ")";

						if( iCountGoesHere != string :: npos )
							pResponse->strContent.insert( iCountGoesHere, strTemp );

						iCountGoesHere = string :: npos;

						if( bFound )
						{
							pResponse->strContent += "</table>\n";
							pResponse->strContent += "</div>\n\n";
						}

						bFound = false;

						uiAdded = 0;

						//
						// leechers
						//

						unsigned char ucPercent = 0;

						for( unsigned long it = 0; it < pmapPeersDicti->size( ); it++ )
						{
							if( uiAdded >= m_uiMaxPeersDisplay )
								break;

							if( pPeersT[it].iLeft != 0 )
							{
								if( !bFound )
								{
									// output table headers

									pResponse->strContent += "<div class=\"stats_table\">\n";
									pResponse->strContent += "<p class=\"leechers\">" + gmapLANG_CFG["stats_leechers"] + "</p>\n";

									// to save on calculations, we're going to insert the number of leechers later, keep track of where

									iCountGoesHere = pResponse->strContent.size( ) - ( sizeof( "</p>\n" ) - 1 );

									// Leechers
									pResponse->strContent += "<table summary=\"leechers\">\n";
									pResponse->strContent += "<tr>\n";
									pResponse->strContent += "<th class=\"ip\">" + gmapLANG_CFG["peer_ip"];

									if( m_bSort )
									{
										pResponse->strContent += "<br>\n";
										pResponse->strContent += "<a title=\"" + gmapLANG_CFG["sort_ip_ascending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_STATS_HTML + "?info_hash=";
										pResponse->strContent += cstrHashString;
										pResponse->strContent += "&amp;sort=";
										pResponse->strContent += SORTPSTR_AIP;
										pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a title=\"" + gmapLANG_CFG["sort_ip_descending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_STATS_HTML + "?info_hash=";
										pResponse->strContent += cstrHashString;
										pResponse->strContent += "&amp;sort=";
										pResponse->strContent += SORTPSTR_DIP;
										pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a>";
									}

									pResponse->strContent += "</th>\n";

									// Client information
									if( m_ucShowPeerInfo != 0 )
									{
										pResponse->strContent += "<th class=\"client\">" + gmapLANG_CFG["client"];

										if( m_bSort )
										{
											pResponse->strContent += "<br>\n";
											pResponse->strContent += "<a title=\"" + gmapLANG_CFG["sort_client_ascending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_STATS_HTML + "?info_hash=";
											pResponse->strContent += cstrHashString;
											pResponse->strContent += "&amp;sort=";
											pResponse->strContent += SORTPSTR_ACLIENT;
											pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a title=\"" + gmapLANG_CFG["sort_client_descending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_STATS_HTML + "?info_hash=";
											pResponse->strContent += cstrHashString;
											pResponse->strContent += "&amp;sort=";
											pResponse->strContent += SORTPSTR_DCLIENT;
											pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a>";
										}

										pResponse->strContent += "</th>\n";
									}

									// Uploaded
									pResponse->strContent += "<th class=\"bytes\">" + gmapLANG_CFG["uploaded"];

									if( m_bSort )
									{
										pResponse->strContent += "<br>\n";
										pResponse->strContent += "<a title=\"" + gmapLANG_CFG["sort_uploaded_ascending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_STATS_HTML + "?info_hash=";
										pResponse->strContent += cstrHashString;
										pResponse->strContent += "&amp;sort=";
										pResponse->strContent += SORTPSTR_AUPPED;
										pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a title=\"" + gmapLANG_CFG["sort_uploaded_descending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_STATS_HTML + "?info_hash=";
										pResponse->strContent += cstrHashString;
										pResponse->strContent += "&amp;sort=";
										pResponse->strContent += SORTPSTR_DUPPED;
										pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a>";
									}

									// Downloaded
									pResponse->strContent += "</th>\n";
									pResponse->strContent += "<th class=\"bytes\">" + gmapLANG_CFG["downloaded"];

									if( m_bSort )
									{
										pResponse->strContent += "<br>\n";
										pResponse->strContent += "<a title=\"" + gmapLANG_CFG["sort_downloaded_ascending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_STATS_HTML + "?info_hash=";
										pResponse->strContent += cstrHashString;
										pResponse->strContent += "&amp;sort=";
										pResponse->strContent += SORTPSTR_ADOWNED;
										pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a title=\"" + gmapLANG_CFG["sort_downloaded_descending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_STATS_HTML + "?info_hash=";
										pResponse->strContent += cstrHashString;
										pResponse->strContent += "&amp;sort=";
										pResponse->strContent += SORTPSTR_DDOWNED;
										pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a>";
									}

									// Left as progress indicator
									if( m_pAllowed && m_bShowLeftAsProgress )
									{
										pResponse->strContent += "</th>\n";
										pResponse->strContent += "<th class=\"bytes\">" + gmapLANG_CFG["progress"];
									}
									else
									{
										pResponse->strContent += "</th>\n";
										pResponse->strContent += "<th class=\"bytes\">" + gmapLANG_CFG["left"];
									}

									if( m_bSort )
									{
										pResponse->strContent += "<br>\n";
										pResponse->strContent += "<a title=\"" + gmapLANG_CFG["sort_left_ascending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_STATS_HTML + "?info_hash=";
										pResponse->strContent += cstrHashString;
										pResponse->strContent += "&amp;sort=";
										pResponse->strContent += SORTPSTR_ALEFT;
										pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a title=\"" + gmapLANG_CFG["sort_left_descending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_STATS_HTML + "?info_hash=";
										pResponse->strContent += cstrHashString;
										pResponse->strContent += "&amp;sort=";
										pResponse->strContent += SORTPSTR_DLEFT;
										pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a>";
									}

									// Connected time
									pResponse->strContent += "</th>\n";
									pResponse->strContent += "<th class=\"connected\">" + gmapLANG_CFG["connected"];

									if( m_bSort )
									{
										pResponse->strContent += "<br>\n";
										pResponse->strContent += "<a title=\"" + gmapLANG_CFG["sort_connected_ascending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_STATS_HTML + "?info_hash=";
										pResponse->strContent += cstrHashString;
										pResponse->strContent += "&amp;sort=";
										pResponse->strContent += SORTPSTR_ACONNECTED;
										pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a title=\"" + gmapLANG_CFG["sort_connected_descending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_STATS_HTML + "?info_hash=";
										pResponse->strContent += cstrHashString;
										pResponse->strContent += "&amp;sort=";
										pResponse->strContent += SORTPSTR_DCONNECTED;
										pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a>";
									}

									pResponse->strContent += "</th>\n";

									// Share ratios
									if( m_bShowShareRatios )
									{
										pResponse->strContent += "<th class=\"number\">" + gmapLANG_CFG["share_ratio"];

										if( m_bSort )
										{
											pResponse->strContent += "<br>\n";
											pResponse->strContent += "<a title=\"" + gmapLANG_CFG["sort_ratio_ascending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_STATS_HTML + "?info_hash=";
											pResponse->strContent += cstrHashString;
											pResponse->strContent += "&amp;sort=";
											pResponse->strContent += SORTPSTR_ASHARERATIO;
											pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a title=\"" + gmapLANG_CFG["sort_ratio_descending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_STATS_HTML + "?info_hash=";
											pResponse->strContent += cstrHashString;
											pResponse->strContent += "&amp;sort=";
											pResponse->strContent += SORTPSTR_DSHARERATIO;
											pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a>";
										}

										pResponse->strContent += "</th>\n";
									}

									// Average upload rate
									if( m_bShowAvgULRate )
									{
										pResponse->strContent += "<th class=\"number\">" + gmapLANG_CFG["avg_up_rate"];

										if( m_bSort )
										{
											pResponse->strContent += "<br>\n";
											pResponse->strContent += "<a title=\"" + gmapLANG_CFG["sort_aur_ascending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_STATS_HTML + "?info_hash=";
											pResponse->strContent += cstrHashString;
											pResponse->strContent += "&amp;sort=";
											pResponse->strContent += SORTPSTR_AAUR;
											pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a title=\"" + gmapLANG_CFG["sort_aur_descending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_STATS_HTML + "?info_hash=";
											pResponse->strContent += cstrHashString;
											pResponse->strContent += "&amp;sort=";
											pResponse->strContent += SORTPSTR_DAUR;
											pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a>";
										}

										pResponse->strContent += "</th>\n";
									}

									// Average download rate
									if( m_bShowAvgDLRate )
									{
										pResponse->strContent += "<th class=\"number\">" + gmapLANG_CFG["avg_down_rate"];

										if( m_bSort )
										{
											pResponse->strContent += "<br>\n";
											pResponse->strContent += "<a title=\"" + gmapLANG_CFG["sort_adr_ascending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_STATS_HTML + "?info_hash=";
											pResponse->strContent += cstrHashString;
											pResponse->strContent += "&amp;sort=";
											pResponse->strContent += SORTPSTR_AADR;
											pResponse->strContent += "\">" + gmapLANG_CFG["sort_ascending"] + "</a><span>-</span><a title=\"" + gmapLANG_CFG["sort_adr_descending"] + "\" class=\"sort\" href=\"" + RESPONSE_STR_STATS_HTML + "?info_hash=";
											pResponse->strContent += cstrHashString;
											pResponse->strContent += "&amp;sort=";
											pResponse->strContent += SORTPSTR_DADR;
											pResponse->strContent += "\">" + gmapLANG_CFG["sort_descending"] + "</a>";
										}

										pResponse->strContent += "</th>\n";
									}

									pResponse->strContent += "</tr>\n";

									bFound = true;
								}

								// output table rows

								if( uiAdded % 2 )
									pResponse->strContent += "<tr class=\"even\">\n";
								else
									pResponse->strContent += "<tr class=\"odd\">\n";

								// IP addresses
								pResponse->strContent += "<td class=\"ip\">" + pPeersT[it].strIP + "</td>\n";

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

								if( m_pAllowed && m_bShowLeftAsProgress )
									pResponse->strContent += UTIL_BytesToString( iSize - pPeersT[it].iLeft );
								else
									pResponse->strContent += UTIL_BytesToString( pPeersT[it].iLeft );

								if( m_pAllowed )
								{
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
										}								}
								}

								pResponse->strContent += "</td>\n";

								// Connected
								pResponse->strContent += "<td class=\"connected\">" + UTIL_SecondsToString( pPeersT[it].ulConnected ) + "</td>\n";

								// Share ratios
								if( m_bShowShareRatios )
								{
									pResponse->strContent += "<td class=\"number_";

									if( ( -1.001 < pPeersT[it].flShareRatio ) && ( pPeersT[it].flShareRatio < -0.999 ) )
										pResponse->strContent += "green\">";
									else if( pPeersT[it].flShareRatio < 0.800 )
										pResponse->strContent += "red\">";
									else if( pPeersT[it].flShareRatio < 1.200 )
										pResponse->strContent += "yellow\">";
									else
										pResponse->strContent += "green\">";

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
									pResponse->strContent += "<td class=\"number\">";

									if( pPeersT[it].ulConnected > 0 )
										pResponse->strContent += UTIL_BytesToString( pPeersT[it].iUpped / pPeersT[it].ulConnected ) + "/sec";
									else
										pResponse->strContent += gmapLANG_CFG["na"];

									pResponse->strContent += "</td>\n";
								}

								// Average download
								if( m_bShowAvgDLRate )
								{
									pResponse->strContent += "<td class=\"number\">";

									if( pPeersT[it].ulConnected > 0 )
										pResponse->strContent += UTIL_BytesToString( pPeersT[it].iDowned / pPeersT[it].ulConnected ) + "/sec";
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

						if( iCountGoesHere != string :: npos )
							pResponse->strContent.insert( iCountGoesHere, strTemp );

						delete [] pPeersT;

						if( bFound )
						{
							pResponse->strContent += "</table>\n";
							pResponse->strContent += "</div>\n\n";
						}
						
						if( m_pIPState )
						{
							CAtom *pIPState = m_pIPState->getItem( cstrHash );
							
							int64 Completed;

							// Did we get a list of peers?
							if( pIPState && pIPState->isDicti( ) )
							{
								// The peer map to obtain the list of peers
								map<string, CAtom *> *pmapIPStateDicti = ( (CAtomDicti *)pIPState )->getValuePtr( );
								Completed = pmapIPStateDicti->size( );
								const string cstrCompleted( pRequest->mapParams["completes"] );
// 
// 								unsigned long ulCount = 0;
// 
// 								string :: size_type iStart = 0;

								// Loop through the peers map to obtain the list of peers and their information by peerid
// 								for( map<string, CAtom *> :: iterator it = pmapIPStateDicti->begin( ); it != pmapIPStateDicti->end( ); it++ )
// 								{
									
								
								pResponse->strContent += "<div class=\"stats_table\">\n";
								pResponse->strContent += "<p class=\"seeders\">";
								if( cstrCompleted.empty( ) || cstrCompleted != "1" )
								{
									pResponse->strContent += "<a href=\"" + RESPONSE_STR_STATS_HTML + "?info_hash=" + cstrHashString + "&amp;completes=1\">";
									pResponse->strContent += gmapLANG_CFG["stats_completes"] + " (" + CAtomInt( Completed ).toString( ) + ")</a></p>\n";
								}
								else
								{
									pResponse->strContent += gmapLANG_CFG["stats_completes"] + " (" + CAtomInt( Completed ).toString( ) + ")</p>\n";
									
									pResponse->strContent += "<table summary=\"completes\">\n";
									pResponse->strContent += "<tr>\n";
									pResponse->strContent += "<th class=\"ip\">" + gmapLANG_CFG["peer_ip"];
									pResponse->strContent += "</th></tr>\n";
									
									uiAdded = 0;
									for( map<string, CAtom *> :: iterator it = pmapIPStateDicti->begin( ); it != pmapIPStateDicti->end( ); it++ )
									{
										if( uiAdded % 2 )
											pResponse->strContent += "<tr class=\"even\">\n";
										else
											pResponse->strContent += "<tr class=\"odd\">\n";
										
										
										string strCompletedIP = it->first;
										iStart = strCompletedIP.rfind( "." );

										if( iStart != string :: npos )
										{
											// don't strip ip for mods
											if( !( pRequest->user.ucAccess & ACCESS_EDIT ) )
												strCompletedIP = strCompletedIP.substr( 0, iStart + 1 ) + "xxx";
										}
										
										pResponse->strContent += "<td class=\"ip\">" + strCompletedIP + "</td></tr>";
										uiAdded++;
									}
									pResponse->strContent += "</table>";
								}
								pResponse->strContent += "</div>\n";
							
							}
							else
							{
								pResponse->strContent += "<div class=\"stats_table\">\n";
								pResponse->strContent += "<p class=\"seeders\">" + gmapLANG_CFG["stats_completes"] + " (0)" + "</p>\n";
								pResponse->strContent += "</div>\n";
							}
						}
					}
				}
#ifdef BNBT_MYSQL
			}
			else
				UTIL_LogPrint( "serverResponseStats: no dfile information\n" );
#endif
		}
		else
		{
			if( !cstrPostNum.empty( ) )
			{
				string postfile;
				string strPostName;
				string strPost;
				int64 uiAdded = 0;
				int64 postcount = 0;
				if( cstrPostNum == "new" )
				{
					postfile = "post0";
					while( access( string( m_strPostDir + postfile + ".name" ).c_str( ), 0 ) == 0 )
					{							
						postcount++;
						postfile = "post" + CAtomInt( postcount ).toString( );
					}
					UTIL_MakeFile( string( m_strPostDir + postfile + ".name" ).c_str( ), gmapLANG_CFG["new_post"] );
					UTIL_MakeFile( string( m_strPostDir + postfile ).c_str( ), gmapLANG_CFG["new_post_content"] );
					pResponse->strContent += "<p class=\"changed_post\">" + UTIL_Xsprintf( gmapLANG_CFG["stats_post_return"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_index"] + "\" href=\"" + RESPONSE_STR_STATS_HTML + "?post=" + CAtomInt( postcount ).toString( ) + "&amp;edit=1\">" ).c_str( ), "</a>" ) + "</p>\n";
					// Output common HTML tail
					HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_STATS ) );
					
					return;
				}
				if( cstrPostNum == "del" )
				{
					postfile = "post0";
					while( access( string( m_strPostDir + postfile + ".name" ).c_str( ), 0 ) == 0 )
					{							
						postcount++;
						postfile = "post" + CAtomInt( postcount ).toString( );
					}
					postcount--;
					postfile = "post" + CAtomInt( postcount ).toString( );
					UTIL_DeleteFile( string( m_strPostDir + postfile + ".name" ).c_str( ) );
					UTIL_DeleteFile( string( m_strPostDir + postfile ).c_str( ) );
					pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["stats_succeed_operation"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_index"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";
					// Output common HTML tail
					HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INDEX ) );
					
					return;
				}
					
				postfile = "post" + cstrPostNum;
				if( access( string( m_strPostDir + postfile + ".name" ).c_str( ), 0 ) == 0 )
				{
					strPostName = UTIL_ReadFile( string( m_strPostDir + postfile + ".name" ).c_str( ) );
					strPost = UTIL_ReadFile( string( m_strPostDir + postfile ).c_str( ) );
				}
				if( pRequest->mapParams.find( "edit" ) != pRequest->mapParams.end( ) )
				{
					if( pRequest->user.ucAccess & ACCESS_ADMIN )
					{
						const string strEdit( pRequest->mapParams["edit"] );
						if( strEdit == "1" )
						{
							pResponse->strContent += "<div class=\"change_post\">\n";
							pResponse->strContent += "<p class=\"change_post\">" + gmapLANG_CFG["stats_change_post"] + "</p>\n";
							pResponse->strContent += "<p class=\"change_post\">" + UTIL_RemoveHTML( strPostName ) + "</p>\n";
							pResponse->strContent += "<table class=\"change_post\">\n<tr>\n<td>\n";
							pResponse->strContent += "<form method=\"get\" action=\"" + RESPONSE_STR_STATS_HTML + "\">\n";
							pResponse->strContent += "<p class=\"change_post_input\"><input name=\"post\" type=hidden value=\"" + cstrPostNum + "\"></p>\n";
							pResponse->strContent += "<p class=\"change_post_input\"><input id=\"post_name\" name=\"post_name\" alt=\"[" + gmapLANG_CFG["stats_new_post_name"] + "]\" type=text size=96 maxlength=" + CAtomInt( MAX_FILENAME_LEN ).toString( ) + " value=\"" + UTIL_RemoveHTML( strPostName ) + "\"> <label for=\"name\">" + gmapLANG_CFG["stats_new_post_name"] + "</label></p>\n";
							pResponse->strContent += "<p class=\"change_post_input\"><textarea id=\"post_content\" name=\"post_content\" rows=10 cols=96>" + strPost + "</textarea> <label for=\"intr\"></label></p>\n";
							pResponse->strContent += "<div class=\"change_post_button\">\n";
							pResponse->strContent += Button_Submit( "submit_change", string( gmapLANG_CFG["stats_change_post"] ) );
							pResponse->strContent += "\n</div>\n";
							pResponse->strContent += "</form>\n</td>\n</tr>\n</table>\n</div>\n\n<hr class=\"stats_hr\">\n\n";
						}
						else
						{
							if( strEdit == "0" )
							{
								const string cstrPostUp = "post" + CAtomInt( atoi( cstrPostNum.c_str( ) ) - 1 ).toString( );
								UTIL_MoveFile( string( m_strPostDir + postfile ).c_str( ), string( m_strPostDir + postfile + ".temp" ).c_str( ) );
								UTIL_MoveFile( string( m_strPostDir + postfile + ".name" ).c_str( ), string( m_strPostDir + postfile + ".name.temp" ).c_str( ) );
								UTIL_MoveFile( string( m_strPostDir + cstrPostUp ).c_str( ), string( m_strPostDir + postfile ).c_str( ) );
								UTIL_MoveFile( string( m_strPostDir + cstrPostUp + ".name" ).c_str( ), string( m_strPostDir + postfile + ".name" ).c_str( ) );
								UTIL_MoveFile( string( m_strPostDir + postfile + ".temp" ).c_str( ), string( m_strPostDir + cstrPostUp ).c_str( ) );
								UTIL_MoveFile( string( m_strPostDir + postfile + ".name.temp" ).c_str( ), string( m_strPostDir + cstrPostUp + ".name" ).c_str( ) );
							}
							else
							{
								if( strEdit == "2" )
								{
									const string cstrPostDown = "post" + CAtomInt( atoi( cstrPostNum.c_str( ) ) + 1 ).toString( );
									UTIL_MoveFile( string( m_strPostDir + postfile ).c_str( ), string( m_strPostDir + postfile + ".temp" ).c_str( ) );
									UTIL_MoveFile( string( m_strPostDir + postfile + ".name" ).c_str( ), string( m_strPostDir + postfile + ".name.temp" ).c_str( ) );
									UTIL_MoveFile( string( m_strPostDir + cstrPostDown ).c_str( ), string( m_strPostDir + postfile ).c_str( ) );
									UTIL_MoveFile( string( m_strPostDir + cstrPostDown + ".name" ).c_str( ), string( m_strPostDir + postfile + ".name" ).c_str( ) );
									UTIL_MoveFile( string( m_strPostDir + postfile + ".temp" ).c_str( ), string( m_strPostDir + cstrPostDown ).c_str( ) );
									UTIL_MoveFile( string( m_strPostDir + postfile + ".name.temp" ).c_str( ), string( m_strPostDir + cstrPostDown + ".name" ).c_str( ) );
								}
								else
								{
									pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["stats_invalid_operation"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_index"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";
									// Output common HTML tail
									HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INDEX ) );
									
									return;
								}
								
							}
							pResponse->strContent += "<p class=\"delete\">" + UTIL_Xsprintf( gmapLANG_CFG["stats_succeed_operation"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_index"] + "\" href=\"" + RESPONSE_STR_INDEX_HTML + "\">" ).c_str( ), "</a>" ) + "</p>\n";
							// Output common HTML tail
							HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_INDEX ) );
							
							return;
						}
					}
				}
				else
					if( pRequest->mapParams.find( "post_name" ) != pRequest->mapParams.end( ) )
					{
						if( pRequest->user.ucAccess & ACCESS_ADMIN )
						{
							const string strNewPostName( pRequest->mapParams["post_name"] );
							const string strNewPost( pRequest->mapParams["post_content"] );
							if( !strNewPostName.empty( ) )
							{
								UTIL_MakeFile( ( string( m_strPostDir + postfile + ".name" ).c_str( ) ), strNewPostName );
								UTIL_MakeFile( ( string( m_strPostDir + postfile ).c_str( ) ), strNewPost );
								
								pResponse->strContent += "<div class=\"changed_post\">\n";
								pResponse->strContent += "<table class=\"changed_post\">\n";
								pResponse->strContent += "<tr>\n<td>\n<ul>\n";
								if( !strNewPostName.empty( ) )
									pResponse->strContent += "<li class=\"changed_post\">" + UTIL_Xsprintf( gmapLANG_CFG["stats_changed_post_name"].c_str( ), UTIL_RemoveHTML( strNewPostName ).c_str( ) ) + "</li>\n";
								pResponse->strContent += "</ul>\n</td>\n</tr>\n</table>\n</div>\n";
								
								pResponse->strContent += "<p class=\"changed_post\">" + UTIL_Xsprintf( gmapLANG_CFG["stats_post_return"].c_str( ), string( "<a title=\"" + gmapLANG_CFG["navbar_index"] + "\" href=\"" + RESPONSE_STR_STATS_HTML + "?post=" + cstrPostNum + "\">" ).c_str( ), "</a>" ) + "</p>\n";
								
								HTML_Common_End( pRequest, pResponse, btv, NOT_INDEX, string( CSS_STATS ) );
								
								return;
							}
						}
					}
				pResponse->strContent += "<div class=\"stats_table\">\n";
				pResponse->strContent += "<p class=\"file_info\">" + gmapLANG_CFG["stats_post_view"] + "</p>\n";
				
				if( pRequest->user.ucAccess & ACCESS_ADMIN )
					pResponse->strContent += "<p class=\"stats_menu\"><a href=\"" + RESPONSE_STR_STATS_HTML + "?post=" + cstrPostNum + "&amp;edit=1\">" + gmapLANG_CFG["stats_edit_post"] + "</a></p>\n";



				pResponse->strContent += "<table class=\"file_info\" summary=\"file info\">\n";

				// needs work
				if( !strPostName.empty( ) )
				{
					uiAdded++;

					if( uiAdded % 2 )
						pResponse->strContent += "<tr class=\"even\">\n";
					else
						pResponse->strContent += "<tr class=\"odd\">\n";

					pResponse->strContent += "<th class=\"file_info\">" + gmapLANG_CFG["post_header"] + ":</th>\n";
					pResponse->strContent += "<td class=\"file_info\">" + UTIL_RemoveHTML( strPostName ) + "</td>\n</tr>\n";
				}
				else
				{
					uiAdded++;

					if( uiAdded % 2 )
						pResponse->strContent += "<tr class=\"even\">\n";
					else
						pResponse->strContent += "<tr class=\"odd\">\n";

					pResponse->strContent += "<th class=\"file_info\">" + gmapLANG_CFG["post_header"] + ":</th>\n";
					pResponse->strContent += "<td class=\"file_info\">" + gmapLANG_CFG["new_post"] + "</td>\n</tr>\n";
				}


				if( !strPost.empty( ) )
				{
					uiAdded++;

					if( uiAdded % 2 )
						pResponse->strContent += "<tr class=\"even\">\n";
					else
						pResponse->strContent += "<tr class=\"odd\">\n";

					pResponse->strContent += "<th class=\"file_info\">" + gmapLANG_CFG["post_content"] + ":</th>\n";
					pResponse->strContent += "<td class=\"file_info\"><code>" + UTIL_RemoveHTML2( strPost ) + "</code></td>\n</tr>\n";
				}
				else
				{
					uiAdded++;

					if( uiAdded % 2 )
						pResponse->strContent += "<tr class=\"even\">\n";
					else
						pResponse->strContent += "<tr class=\"odd\">\n";

					pResponse->strContent += "<th class=\"file_info\">" + gmapLANG_CFG["post_content"] + ":</th>\n";
					pResponse->strContent += "<td class=\"file_info\"><code>" + gmapLANG_CFG["new_post_content"] + "</code></td>\n</tr>\n";
				}
				pResponse->strContent += "</table>\n</div>\n\n";
			}
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
