//
// Copyright (C) 2003-2004 Trevor Hogan
//

#ifndef SERVER_H
 #define SERVER_H

class CServer
{
public:
	CServer( );
	virtual ~CServer( );

	void Kill( );
	bool isDying( );

	// returns true if the server should be killed

	bool Update( const bool &bBlock );

	CTracker *getTracker( );

	vector<CClient *> m_vecClients;

private:
	bool m_bKill;

	CTracker *m_pTracker;

	unsigned int m_uiSocketTimeOut;
	string m_strBind;
	char m_cCompression;

	// tphogan - the server is listening on each of these sockets
	// the vector container will handle memory allocation for the SOCKET object

	vector<SOCKET> m_vecListeners;

	// tphogan - helper function to add a new socket listener
	// returns true on success, false on error

//	bool AddListener( struct sockaddr_in sin );
	bool AddListener( struct sockaddr_in6 sin );
};

#endif
