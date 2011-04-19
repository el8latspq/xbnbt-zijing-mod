//
// Copyright (C) 2003-2005 Trevor Hogan
//

#ifndef ATOM_H
 #define ATOM_H

class CAtom
{
public:
	virtual ~CAtom( ) { }

	virtual bool isLong( )		{ return false; }
	virtual bool isList( )		{ return false; }
	virtual bool isDicti( )		{ return false; }

	virtual int EncodedLength( ) = 0;
	virtual int Length( ) = 0;
	virtual string toString( ) = 0;
};

class CAtomInt : public CAtom
{
public:
	CAtomInt( );
	CAtomInt( const int &iInt );
	CAtomInt( const CAtomInt &c );
	virtual ~CAtomInt( );

	virtual int EncodedLength( );
	virtual int Length( );
	virtual string toString( );

	int getValue( ) const;
	void setValue( const int &iInt );

private:
	int m_iInt;
};

class CAtomLong : public CAtom
{
public:
	CAtomLong( );
	CAtomLong( const int64 &iLong );
	CAtomLong( const CAtomLong &c );
	virtual ~CAtomLong( );

	virtual int EncodedLength( );
	virtual int Length( );
	virtual string toString( );

	int64 getValue( ) const;
	void setValue( const int64 &iLong );

private:
	int64 m_iLong;
};

class CAtomString : public CAtom
{
public:
	CAtomString( );
	CAtomString( const string &strString );
	CAtomString( const CAtomString &c );
	virtual ~CAtomString( );

	virtual int EncodedLength( );
	virtual int Length( );
	virtual string toString( );

	string getValue( ) const;
	void setValue( const string &strString );

private:
	string m_strString;
};

class CAtomList : public CAtom
{
public:
	CAtomList( );
	CAtomList( const vector<CAtom *> &vecList );
	CAtomList( const CAtomList &c );
	virtual ~CAtomList( );
	
	virtual bool isList( )		{ return true; }

	virtual int EncodedLength( );
	virtual int Length( );
	virtual string toString( );

	virtual bool isEmpty( );
	virtual void clear( );

	virtual void Randomize( );

	vector<CAtom *> getValue( ) const;
	vector<CAtom *> *getValuePtr( ) const;
	void setValue( const vector<CAtom *> &vecList );

	void delItem( CAtom *atmItem );
	void addItem( CAtom *atmItem );

private:
	vector<CAtom *> m_vecList;
};

class CAtomDicti : public CAtom
{
public:
	CAtomDicti( );
	CAtomDicti( const CAtomDicti &c );
	virtual ~CAtomDicti( );

	virtual bool isDicti( )		{ return true; }

	virtual int EncodedLength( );
	virtual int Length( );
	virtual string toString( );

	virtual bool isEmpty( );
	virtual void clear( );

	map<string, CAtom *> *getValuePtr( ) const;
	void setValue( const map<string, CAtom *> &mapDicti );

	void delItem( const string &strKey );
	void eraseItem( const string &strKey );
	CAtom *getItem( const string &strKey );
	CAtom *getItem( const string &strKey, CAtom *pReturn );
	void setItem( const string &strKey, CAtom *pValue );

private:
	map<string, CAtom *> m_mapDicti;
};

#endif
