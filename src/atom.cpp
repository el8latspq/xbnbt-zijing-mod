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

// =Xotic= Modified Source File

#include "bnbt.h"
#include "atom.h"
#include "config.h"
#include "util.h"

//
// CAtomInt
//

CAtomInt :: CAtomInt( )
{
	setValue( 0 );
}

CAtomInt :: CAtomInt( const int &iInt )
{
	setValue( iInt );
}

CAtomInt :: CAtomInt( const CAtomInt &c )
{
	// copy constructor

	setValue( c.getValue( ) );
}

CAtomInt :: ~CAtomInt( )
{

}

int CAtomInt :: EncodedLength( )
{
	return (int)toString( ).size( ) + 2;
}

int CAtomInt :: Length( )
{
	return (int)toString( ).size( );
}

string CAtomInt :: toString( )
{
	char pBuf[32];
	memset( pBuf, 0, sizeof( pBuf ) / sizeof( char ) );

	snprintf( pBuf, sizeof( pBuf ) / sizeof( char ), "%d", getValue( ) );

	return pBuf;
}

int CAtomInt :: getValue( ) const
{
	return m_iInt;
}

void CAtomInt :: setValue( const int &iInt )
{
	m_iInt = iInt;
}

//
// CAtomLong
//

CAtomLong :: CAtomLong( )
{
	setValue( 0 );
}

CAtomLong :: CAtomLong( const int64 &iLong )
{
	setValue( iLong );
}

CAtomLong :: CAtomLong( const CAtomLong &c )
{
	// copy constructor

	setValue( c.getValue( ) );
}

CAtomLong :: ~CAtomLong( )
{

}

int CAtomLong :: EncodedLength( )
{
	return (int)toString( ).size( ) + 2;
}

int CAtomLong :: Length( )
{
	return (int)toString( ).size( );
}

string CAtomLong :: toString( )
{
	char pBuf[32];
	memset( pBuf, 0, sizeof( pBuf ) / sizeof( char ) );

#if defined( WIN32 )
	snprintf( pBuf, sizeof( pBuf ) / sizeof( char ), "%I64d", getValue( ) );
#elif defined( __FREEBSD__ ) || defined( __OPENBSD__ ) || defined( __NETBSD__ )
	snprintf( pBuf, sizeof( pBuf ) / sizeof( char ), "%qd", getValue( ) );
#else
	snprintf( pBuf, sizeof( pBuf ) / sizeof( char ), "%lld", getValue( ) );
#endif

	return pBuf;
}

int64 CAtomLong :: getValue( ) const
{
	return m_iLong;
}

void CAtomLong :: setValue( const int64 &iLong )
{
	m_iLong = iLong;
}

//
// CAtomString
//

CAtomString :: CAtomString( )
{

}

CAtomString :: CAtomString( const string &strString )
{
	setValue( strString );
}

CAtomString :: CAtomString( const CAtomString &c )
{
	// copy constructor

	setValue( c.getValue( ) );
}

CAtomString :: ~CAtomString( )
{

}

int CAtomString :: EncodedLength( )
{
	const unsigned int cuiSize( (unsigned int)getValue( ).size( ) );

	char pBuf[32];
	memset( pBuf, 0, sizeof( pBuf ) / sizeof( char ) );

	snprintf( pBuf, sizeof( pBuf ) / sizeof( char ), "%u", cuiSize );

	return cuiSize + strlen( pBuf ) + 1;
}

int CAtomString :: Length( )
{
	return (int)getValue( ).size( );
}

string CAtomString :: toString( )
{
	return getValue( );
}

string CAtomString :: getValue( ) const
{
	return m_strString;
}

void CAtomString :: setValue( const string &strString )
{
	m_strString = strString;
}

//
// CAtomList
//

CAtomList :: CAtomList( )
{

}

CAtomList :: CAtomList( const vector<CAtom *> &vecList )
{
	setValue( vecList );
}

CAtomList :: CAtomList( const CAtomList &c )
{
	// copy constructor

	vector<CAtom *> *pvecList = c.getValuePtr( );

	for( vector<CAtom *> :: iterator itAtom = pvecList->begin( ); itAtom != pvecList->end( ); itAtom++ )
	{
		if( dynamic_cast<CAtomInt *>( *itAtom ) )
			addItem( new CAtomInt( *dynamic_cast<CAtomInt *>( *itAtom ) ) );
		else if( dynamic_cast<CAtomLong *>( *itAtom ) )
			addItem( new CAtomLong( *dynamic_cast<CAtomLong *>( *itAtom ) ) );
		else if( dynamic_cast<CAtomString *>( *itAtom ) )
			addItem( new CAtomString( *dynamic_cast<CAtomString *>( *itAtom ) ) );
		else if( dynamic_cast<CAtomList *>( *itAtom ) )
			addItem( new CAtomList( *dynamic_cast<CAtomList *>( *itAtom ) ) );
		else if( dynamic_cast<CAtomDicti *>( *itAtom ) )
			addItem( new CAtomDicti( *dynamic_cast<CAtomDicti *>( *itAtom ) ) );
		else
			UTIL_LogPrint( ( gmapLANG_CFG["atomlist_copy_warning"] + "\n" ).c_str( ) );
	}
}

CAtomList :: ~CAtomList( )
{
	clear( );
}

int CAtomList :: EncodedLength( )
{
	int iLen = 0;

	for( vector<CAtom *> :: iterator itAtom = m_vecList.begin( ); itAtom != m_vecList.end( ); itAtom++ )
		iLen += (*itAtom)->EncodedLength( );

	return iLen + 2;
}

int CAtomList :: Length( )
{
	// nobody cares about you

	return 0;
}

string CAtomList :: toString( )
{
	return string( );
}

bool CAtomList :: isEmpty( )
{
	return m_vecList.empty( );
}

void CAtomList :: clear( )
{
	for( vector<CAtom *> :: iterator itAtom = m_vecList.begin( ); itAtom != m_vecList.end( ); itAtom++ )
		delete *itAtom;

	m_vecList.clear( );
}

void CAtomList :: Randomize( )
{
	random_shuffle( m_vecList.begin( ), m_vecList.end( ) );
}

vector<CAtom *> CAtomList :: getValue( ) const
{
	return m_vecList;
}

vector<CAtom *> *CAtomList :: getValuePtr( ) const
{
	return (vector<CAtom *> *)&m_vecList;
}

void CAtomList :: setValue( const vector<CAtom *> &vecList )
{
	m_vecList = vecList;
}

void CAtomList :: delItem( CAtom *atmItem )
{
	for( vector<CAtom *> :: iterator itAtom = m_vecList.begin( ); itAtom != m_vecList.end( ); itAtom++ )
	{
		if( *itAtom == atmItem )
		{
			delete *itAtom;

			m_vecList.erase( itAtom );

			return;
		}
	}
}

void CAtomList :: addItem( CAtom *atmItem )
{
	m_vecList.push_back( atmItem );
}

//
// CAtomDicti
//

CAtomDicti :: CAtomDicti( )
{

}

CAtomDicti :: CAtomDicti( const CAtomDicti &c )
{
	// copy constructor

	map<string, CAtom *> *pmapDicti = c.getValuePtr( );

	for( map<string, CAtom *> :: iterator itAtom = pmapDicti->begin( ); itAtom != pmapDicti->end( ); itAtom++ )
	{
		if( dynamic_cast<CAtomInt *>( (*itAtom).second ) )
			setItem( (*itAtom).first, new CAtomInt( *dynamic_cast<CAtomInt *>( (*itAtom).second ) ) );
		else if( dynamic_cast<CAtomLong *>( (*itAtom).second ) )
			setItem( (*itAtom).first, new CAtomLong( *dynamic_cast<CAtomLong *>( (*itAtom).second ) ) );
		else if( dynamic_cast<CAtomString *>( (*itAtom).second ) )
			setItem( (*itAtom).first, new CAtomString( *dynamic_cast<CAtomString *>( (*itAtom).second ) ) );
		else if( dynamic_cast<CAtomList *>( (*itAtom).second ) )
			setItem( (*itAtom).first, new CAtomList( *dynamic_cast<CAtomList *>( (*itAtom).second ) ) );
		else if( dynamic_cast<CAtomDicti *>( (*itAtom).second ) )
			setItem( (*itAtom).first, new CAtomDicti( *dynamic_cast<CAtomDicti *>( (*itAtom).second ) ) );
		else
			UTIL_LogPrint( ( gmapLANG_CFG["atomdicti_copy_warning"] + "\n" ).c_str( ) );
	}
}

CAtomDicti :: ~CAtomDicti( )
{
	clear( );
}

int CAtomDicti :: EncodedLength( )
{
	int iLen = 0;

	for( map<string, CAtom *> :: iterator itAtom = m_mapDicti.begin( ); itAtom != m_mapDicti.end( ); itAtom++ )
		iLen += CAtomString( (*itAtom).first ).EncodedLength( ) + (*itAtom).second->EncodedLength( );

	return iLen + 2;
}

int CAtomDicti :: Length( )
{
	// nobody cares about you

	return 0;
}

string CAtomDicti :: toString( )
{
	return string( );
}

bool CAtomDicti :: isEmpty( )
{
	return m_mapDicti.empty( );
}

void CAtomDicti :: clear( )
{
	for( map<string, CAtom *> :: iterator itAtom = m_mapDicti.begin( ); itAtom != m_mapDicti.end( ); itAtom++ )
		delete (*itAtom).second;

	m_mapDicti.clear( );
}

map<string, CAtom *> *CAtomDicti :: getValuePtr( ) const
{
	return (map<string, CAtom *> *)&m_mapDicti;
}

void CAtomDicti :: delItem( const string &strKey )
{
	const map<string, CAtom *> :: iterator itAtom( m_mapDicti.find( strKey ) );

	if( itAtom != m_mapDicti.end( ) )
	{
		delete (*itAtom).second;

		m_mapDicti.erase( itAtom );
	}
}

void CAtomDicti :: eraseItem( const string &strKey )
{
	const map<string, CAtom *> :: iterator itAtom( m_mapDicti.find( strKey ) );

	if( itAtom != m_mapDicti.end( ) )
	{
		
		m_mapDicti.erase( itAtom );
	}
}

CAtom *CAtomDicti :: getItem( const string &strKey )
{
	const map<string, CAtom *> :: iterator itAtom( m_mapDicti.find( strKey ) );

	if( itAtom == m_mapDicti.end( ) )
		return 0;
	else
		return (*itAtom).second;
}

CAtom *CAtomDicti :: getItem( const string &strKey, CAtom *pReturn )
{
	const map<string, CAtom *> :: iterator itAtom( m_mapDicti.find( strKey ) );

	if( itAtom == m_mapDicti.end( ) )
		return pReturn;
	else
		return (*itAtom).second;
}

void CAtomDicti :: setItem( const string &strKey, CAtom *pValue )
{
	const map<string, CAtom *> :: iterator itAtom( m_mapDicti.find( strKey ) );

	if( itAtom == m_mapDicti.end( ) )
		m_mapDicti.insert( pair<string, CAtom *>( strKey, pValue ) );
	else
	{
		delete (*itAtom).second;

		(*itAtom).second = pValue;
	}
}
