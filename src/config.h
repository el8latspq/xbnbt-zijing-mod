//
// Copyright (C) 2003-2004 Trevor Hogan
//

#ifndef CONFIG_H
 #define CONFIG_H

#define CFG_FILE "bnbt.cfg"

extern map<string, string> gmapCFG;

void CFG_Open( const char *szFile );
void CFG_SetInt( const string &cstrKey, const int &ciX );
void CFG_SetString( const string &cstrKey, const string &csX );
const int CFG_GetInt( const string &cstrKey, const int &ciX );
const string CFG_GetString( const string &cstrKey, const string &csX );
void CFG_Delete( const string &cstrKey );
void CFG_Close( const char *szFile );
void CFG_SetDefaults( );

/***
*
* XBNBT Beta 81b.3.5 - A C++ BitTorrent Tracker
* Copyright (C) 2003-2005 =Xotic=
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

#define LANG_CFG_FILE "lang.cfg"

extern map<string, string> gmapLANG_CFG;

void LANG_CFG_Init( const char *szFile );
void LANG_CFG_Open( const char *szFile );
void LANG_CFG_SetString( const string &cstrKey, const string &csX );
const string LANG_CFG_GetString( const string &cstrKey, const string &csX );
void LANG_CFG_Delete( const string &cstrKey );
void LANG_CFG_Close( const char *szFile );
void LANG_CFG_SetDefaultsSort( );
void LANG_CFG_SetDefaultsErrorLog( );
void LANG_CFG_SetDefaultsXbnbt( );
void LANG_CFG_SetDefaults( );

#endif
