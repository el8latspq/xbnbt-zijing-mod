/***
*
* BNBT Beta 8.5 - A C++ BitTorrent Tracker
* Copyright (C) 2003-2005 Trevor Hogan
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
#include "util.h"

string EncodeInt( const CAtomInt &x )
{
	char pBuf[128];

	memset( pBuf, 0, sizeof( pBuf ) / sizeof( char ) );

	snprintf( pBuf, sizeof( pBuf ) / sizeof( char ), "%d", x.getValue( ) );

	string strDest;

	strDest += "i";
	strDest += pBuf;
	strDest += "e";

	return strDest;
}

string EncodeLong( const CAtomLong &x )
{
	char pBuf[128];

	memset( pBuf, 0, sizeof( pBuf ) / sizeof( char ) );

#if defined( WIN32 )
	snprintf( pBuf, sizeof( pBuf ) / sizeof( char ), "%I64d", x.getValue( ) );
#elif defined( __FREEBSD__ ) || defined( __OPENBSD__ ) || defined( __NETBSD__ )
	snprintf( pBuf, sizeof( pBuf ) / sizeof( char ), "%qd", x.getValue( ) );
#else
	snprintf( pBuf, sizeof( pBuf ) / sizeof( char ), "%lld", x.getValue( ) );
#endif

	string strDest;

	strDest += "i";
	strDest += pBuf;
	strDest += "e";

	return strDest;
}

string EncodeString( const CAtomString &x )
{
	char pBuf[128];

	memset( pBuf, 0, sizeof( pBuf ) / sizeof( char ) );

	snprintf( pBuf, sizeof( pBuf ) / sizeof( char ), "%u", (unsigned int)x.getValue( ).size( ) );

	string strDest;

	strDest += pBuf;
	strDest += ":";
	strDest += x.getValue( );

	return strDest;
}

string EncodeList( const CAtomList &x )
{
	vector<CAtom *> *pv = x.getValuePtr( );

	string strDest;

	strDest += "l";

	for( vector<CAtom *> :: iterator i = pv->begin( ); i != pv->end( ); i++ )
	{
		if( dynamic_cast<CAtomInt *>( *i ) )
			strDest += EncodeInt( *dynamic_cast<CAtomInt *>( *i ) );
		else if( dynamic_cast<CAtomLong *>( *i ) )
			strDest += EncodeLong( *dynamic_cast<CAtomLong *>( *i ) );
		else if( dynamic_cast<CAtomString *>( *i ) )
			strDest += EncodeString( *dynamic_cast<CAtomString *>( *i ) );
		else if( dynamic_cast<CAtomList *>( *i ) )
			strDest += EncodeList( *dynamic_cast<CAtomList *>( *i ) );
		else if( dynamic_cast<CAtomDicti *>( *i ) )
			strDest += EncodeDicti( *dynamic_cast<CAtomDicti *>( *i ) );
	}

	strDest += "e";

	return strDest;
}

string EncodeDicti( const CAtomDicti &x )
{
	map<string, CAtom *> *pmapDicti = x.getValuePtr( );

	string strDest;

	strDest += "d";

	for( map<string, CAtom *> :: iterator i = pmapDicti->begin( ); i != pmapDicti->end( ); i++ )
	{
		strDest += EncodeString( CAtomString( (*i).first ) );

		if( dynamic_cast<CAtomInt *>( (*i).second ) )
			strDest += EncodeInt( *dynamic_cast<CAtomInt *>( (*i).second ) );
		else if( dynamic_cast<CAtomLong *>( (*i).second ) )
			strDest += EncodeLong( *dynamic_cast<CAtomLong *>( (*i).second ) );
		else if( dynamic_cast<CAtomString *>( (*i).second ) )
			strDest += EncodeString( *dynamic_cast<CAtomString *>( (*i).second ) );
		else if( dynamic_cast<CAtomList *>( (*i).second ) )
			strDest += EncodeList( *dynamic_cast<CAtomList *>( (*i).second ) );
		else if( dynamic_cast<CAtomDicti *>( (*i).second ) )
			strDest += EncodeDicti( *dynamic_cast<CAtomDicti *>( (*i).second ) );
	}

	strDest += "e";

	return strDest;
}

string Encode( CAtom *pAtom )
{
	if( dynamic_cast<CAtomInt *>( pAtom ) )
		return EncodeInt( *dynamic_cast<CAtomInt *>( pAtom ) );
	else if( dynamic_cast<CAtomLong *>( pAtom ) )
		return EncodeLong( *dynamic_cast<CAtomLong *>( pAtom ) );
	else if( dynamic_cast<CAtomString *>( pAtom ) )
		return EncodeString( *dynamic_cast<CAtomString *>( pAtom ) );
	else if( dynamic_cast<CAtomList *>( pAtom ) )
		return EncodeList( *dynamic_cast<CAtomList *>( pAtom ) );
	else if( dynamic_cast<CAtomDicti *>( pAtom ) )
		return EncodeDicti( *dynamic_cast<CAtomDicti *>( pAtom ) );

	return string( );
}

/*

CAtomInt *DecodeInt( const string &x, unsigned long iStart )
{
	string :: size_type iEnd = x.find( "e" );

	if( iEnd == string :: npos )
	{
		UTIL_LogPrint( "error decoding int - couldn't find \"e\", halting decode\n" );

		return 0;
	}

	return new CAtomInt( atoi( x.substr( iStart + 1, iEnd - iStart - 1 ).c_str( ) ) );
}

*/

CAtomLong *DecodeLong( const string &x, unsigned long iStart )
{
	string :: size_type iEnd = x.find( "e", iStart );

	if( iEnd == string :: npos )
	{
		UTIL_LogPrint( "error decoding long - couldn't find \"e\", halting decode\n" );

		return 0;
	}

	int64 i;

#if defined( WIN32 )
	sscanf( x.substr( iStart + 1, iEnd - iStart - 1 ).c_str( ), "%I64d", &i );
#elif defined( __FREEBSD__ ) || defined( __OPENBSD__ ) || defined( __NETBSD__ )
	sscanf( x.substr( iStart + 1, iEnd - iStart - 1 ).c_str( ), "%qd", &i );
#else
	sscanf( x.substr( iStart + 1, iEnd - iStart - 1 ).c_str( ), "%lld", &i );
#endif

	return new CAtomLong( i );
}

CAtomString *DecodeString( const string &x, unsigned long iStart )
{
	string :: size_type iSplit = x.find_first_not_of( "1234567890", iStart );

	if( iSplit == string :: npos )
	{
		UTIL_LogPrint( "error decoding string - couldn't find \":\", halting decode\n" );

		return 0;
	}

	return new CAtomString( x.substr( iSplit + 1, atoi( x.substr( iStart, iSplit - iStart ).c_str( ) ) ) );
}

CAtomList *DecodeList( const string &x, unsigned long iStart )
{
	unsigned long i = iStart + 1;

	CAtomList *pList = new CAtomList( );

	while( i < x.size( ) && x[i] != 'e' )
	{
		CAtom *pAtom = Decode( x, i );

		if( pAtom )
		{
			i += pAtom->EncodedLength( );

			pList->addItem( pAtom );
		}
		else
		{
			UTIL_LogPrint( "error decoding list - error decoding list item, discarding list\n" );

			delete pList;

			return 0;
		}
	}

	return pList;
}

CAtomDicti *DecodeDicti( const string &x, unsigned long iStart )
{
	unsigned long i = iStart + 1;

	CAtomDicti *pDicti = new CAtomDicti( );

	while( i < x.size( ) && x[i] != 'e' )
	{
		CAtom *pKey = Decode( x, i );

		if( pKey && dynamic_cast<CAtomString *>( pKey ) )
		{
			i += pKey->EncodedLength( );

			string strKey = pKey->toString( );

			delete pKey;

			if( i < x.size( ) )
			{
				CAtom *pValue = Decode( x, i );

				if( pValue )
				{
					i += pValue->EncodedLength( );

					pDicti->setItem( strKey, pValue );
				}
				else
				{
					UTIL_LogPrint( "error decoding dictionary - error decoding value, discarding dictionary\n" );

					delete pDicti;

					return 0;
				}
			}
		}
		else
		{
			UTIL_LogPrint( "error decoding dictionary - error decoding key, discarding dictionary\n" );

			delete pDicti;

			return 0;
		}
	}

	return pDicti;
}

CAtom *Decode( const string &x, unsigned long iStart )
{
	if( iStart < x.size( ) )
	{
		if( x[iStart] == 'i' )
			return DecodeLong( x, iStart );
		else if( isdigit( x[iStart] ) )
			return DecodeString( x, iStart );
		else if( x[iStart] == 'l' )
			return DecodeList( x, iStart );
		else if( x[iStart] == 'd' )
			return DecodeDicti( x, iStart );

		string temp = x.substr( iStart );

		UTIL_LogPrint( "error decoding - found unexpected character %u, halting decode\n", (unsigned char)x[iStart] );
	}
	else
		UTIL_LogPrint( "error decoding - out of range\n" );

	return 0;
}

CAtom *DecodeFile( const char *szFile )
{
	FILE *pFile = FILE_ERROR;

	pFile = fopen( szFile, "rb" );

	if( pFile == FILE_ERROR )
	{
		UTIL_LogPrint( "warning - unable to open %s for reading\n", szFile );

		return 0;
	}

	// Find the end of the file
	fseek( pFile, 0, SEEK_END );

	// Remember the file size for later
	const unsigned long culFileSize( ftell( pFile ) );

	// Reset to the start of the file
	fseek( pFile, 0, SEEK_SET );

	// Allocate memory for the data buffer
	char *pData = (char *)malloc( sizeof( char ) * culFileSize );

	// Read the data
	fread( pData, sizeof( char ), culFileSize, pFile );

	// Close the file
	fclose( pFile );

	// Place the data in a string
	const string cstrFile( pData, culFileSize );

	// Free the data buffer memory
	free( pData );

	// Return the decoded data
	return Decode( cstrFile );
}
