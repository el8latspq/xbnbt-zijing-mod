//
// Copyright (C) 2003-2005 Trevor Hogan
//

#ifndef BENCODE_H
 #define BENCODE_H

string EncodeInt( const CAtomInt &x );
string EncodeLong( const CAtomLong &x );
string EncodeString( const CAtomString &x );
string EncodeList( const CAtomList &x );
string EncodeDicti( const CAtomDicti &x );
string Encode( CAtom *pAtom );

// the decode functions allocate memory, so be SURE to delete it

// CAtomInt *DecodeInt( const string &x, unsigned long iStart = 0 );
CAtomLong *DecodeLong( const string &x, unsigned long iStart = 0 );
CAtomString *DecodeString( const string &x, unsigned long iStart = 0 );
CAtomList *DecodeList( const string &x, unsigned long iStart = 0 );
CAtomDicti *DecodeDicti( const string &x, unsigned long iStart = 0 );
CAtom *Decode( const string &x, unsigned long iStart = 0 );

CAtom *DecodeFile( const char *szFile );

#endif
