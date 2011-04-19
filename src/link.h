//
// Copyright (C) 2003-2004 Trevor Hogan
//

#ifndef LINK_H
 #define LINK_H

//
// CLink
//  - one instance created on the secondary tracker to connect to the primary tracker
//

class CLink
{
public:
	CLink( );
	virtual ~CLink( );

	void Kill( );
	void Go( );

	string getName( );

	void Queue( struct linkmsg_t lm );

private:
	bool m_bKill;

	string m_strIP;
	string m_strPass;

	SOCKET m_sckLink;

// 	struct sockaddr_in sin;
	struct sockaddr_in6 sin;

	string m_strReceiveBuf;
	string m_strSendBuf;

	void Send( struct linkmsg_t lm );
	struct linkmsg_t Receive( bool bBlock );
	struct linkmsg_t Parse( );

	CMutex m_mtxQueued;

	vector<struct linkmsg_t> m_vecQueued;
};

void StartLink( );

//
// CLinkClient
//  - one instance created on the primary tracker for each secondary tracker
//

class CLinkClient
{
public:
// 	CLinkClient( SOCKET sckLink, struct sockaddr_in sinAddress );
	CLinkClient( SOCKET sckLink, struct sockaddr_in6 sinAddress );
	virtual ~CLinkClient( );

	void Kill( );
	void Go( );

	string getName( );

	void Queue( struct linkmsg_t lm );

	bool m_bActive;

private:
	bool m_bKill;

	SOCKET m_sckLink;

// 	struct sockaddr_in sin;
	struct sockaddr_in6 sin;

	string m_strReceiveBuf;
	string m_strSendBuf;

	void Send( struct linkmsg_t lm );
	struct linkmsg_t Receive( bool bBlock );
	struct linkmsg_t Parse( );

	CMutex m_mtxQueued;

	vector<struct linkmsg_t> m_vecQueued;
};

void StartLinkClient( CLinkClient *pLinkClient );

//
// CLinkServer
//  - one instance created on the primary tracker
//

class CLinkServer
{
public:
	CLinkServer( );
	virtual ~CLinkServer( );

	void Update( );

	void Queue( struct linkmsg_t lm );
	void Queue( struct linkmsg_t lm, string strExclude );

	string m_strPass;

	CMutex m_mtxLinks;

	vector<CLinkClient *> m_vecLinks;

private:
	string m_strBind;

	SOCKET m_sckLinkServer;
};

/***
*
* XBNBT Beta 81b.3.5 - A C++ BitTorrent Tracker
* Copyright (C) 2003-2004 =Xotic=
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

//
// CHUBLink
//  - one instance created on the secondary tracker to connect to the primary tracker
//

class CHUBLink
{
public:
	CHUBLink( );
	virtual ~CHUBLink( );

	void Kill( );
	void Go( );

	string getName( );

	void Queue( struct linkmsg_t lm );

private:
	bool m_bKill;

	string m_strIP;
	string m_strPass;

	SOCKET m_sckLink;

// 	struct sockaddr_in sin;
	struct sockaddr_in6 sin;

	string m_strReceiveBuf;
	string m_strSendBuf;

	void Send( struct linkmsg_t lm );
	struct linkmsg_t Receive( bool bBlock );
	struct linkmsg_t Parse( );

	CMutex m_mtxQueued;

	vector<struct linkmsg_t> m_vecQueued;
};

void StartHUBLink( );

//
// CHUBLinkClient
//  - one instance created on the primary tracker for each secondary tracker
//

class CHUBLinkClient
{
public:
// 	CHUBLinkClient( SOCKET sckLink, struct sockaddr_in sinAddress );
	CHUBLinkClient( SOCKET sckLink, struct sockaddr_in6 sinAddress );
	virtual ~CHUBLinkClient( );

	void Kill( );
	void Go( );

	string getName( );

	void Queue( struct linkmsg_t lm );

	bool m_bActive;

private:
	bool m_bKill;

	SOCKET m_sckLink;

// 	struct sockaddr_in sin;
	struct sockaddr_in6 sin;

	string m_strReceiveBuf;
	string m_strSendBuf;

	void Send( struct linkmsg_t lm );
	struct linkmsg_t Receive( bool bBlock );
	struct linkmsg_t Parse( );

	CMutex m_mtxQueued;

	vector<struct linkmsg_t> m_vecQueued;
};

void StartHUBLinkClient( CHUBLinkClient *pLinkClient );

//
// CHUBLinkServer
//  - one instance created on the primary tracker
//

class CHUBLinkServer
{
public:
	CHUBLinkServer( );
	virtual ~CHUBLinkServer( );

	void Update( );

	void Queue( struct linkmsg_t lm );
	void Queue( struct linkmsg_t lm, string strExclude );

	string m_strPass;

	CMutex m_mtxLinks;

	vector<CHUBLinkClient *> m_vecLinks;

private:
	string m_strBind;

	SOCKET m_sckLinkServer;
};

#endif
