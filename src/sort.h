//
// Copyright (C) 2003-2004 Trevor Hogan
//

// =Xotic= Modified Source File

#ifndef SORT_H
 #define SORT_H

#define SORT_ANAME				1
#define SORT_ACOMPLETE			2
#define SORT_AINCOMPLETE		3
#define SORT_AADDED				4
#define SORT_ASIZE				5
#define SORT_AFILES				6
#define SORT_ACOMMENTS			7
#define SORT_AAVGLEFT			8
#define SORT_ACOMPLETED			9
#define SORT_ATRANSFERRED		10
#define SORT_DNAME				11
#define SORT_DCOMPLETE			12
#define SORT_DINCOMPLETE		13
#define SORT_DADDED				14
#define SORT_DSIZE				15
#define SORT_DFILES				16
#define SORT_DCOMMENTS			17
#define SORT_DAVGLEFT			18
#define SORT_DCOMPLETED			19
#define SORT_DTRANSFERRED		20
#define SORT_ATAG				21
#define SORT_DTAG				22
#define SORT_AUPLOADER			23
#define SORT_DUPLOADER			24
#define SORT_AIP				25
#define SORT_DIP				26

#define SORTP_AUPPED			1
#define SORTP_ADOWNED			2
#define SORTP_ALEFT			3
#define SORTP_ACONNECTED		4
#define SORTP_ASHARERATIO		5
#define SORTP_DUPPED			6
#define SORTP_DDOWNED			7
#define SORTP_DLEFT			8
#define SORTP_DCONNECTED		9
#define SORTP_DSHARERATIO		10
#define SORTP_ACLIENT			11
#define SORTP_DCLIENT			12
#define SORTP_AIP				13
#define SORTP_DIP				14
#define SORTP_AAUR			15
#define SORTP_DAUR			16
#define SORTP_AADR			17
#define SORTP_DADR			18
#define SORTP_AUR				19
#define SORTP_DUR				20
#define SORTP_ADR				21
#define SORTP_DDR				22

#define SORTU_ALOGIN			1
#define SORTU_AACCESS			2
#define SORTU_AEMAIL			3
#define SORTU_ACREATED			4
#define SORTU_DLOGIN			5
#define SORTU_DACCESS			6
#define SORTU_DEMAIL			7
#define SORTU_DCREATED			8
#define SORTU_ALAST				9
#define SORTU_DLAST				10
#define SORTU_AWARNED			11
#define SORTU_DWARNED			12
#define SORTU_ASHARERATIO		13
#define SORTU_DSHARERATIO		14
#define SORTU_AUPPED			15
#define SORTU_DUPPED			16
#define SORTU_ADOWNED			17
#define SORTU_DDOWNED			18
#define SORTU_ABONUS			19
#define SORTU_DBONUS			20
#define SORTU_ASEEDBONUS		21
#define SORTU_DSEEDBONUS		22
#define SORTU_AGROUP			23
#define SORTU_DGROUP			24
#define SORTU_AINVITER			25
#define SORTU_DINVITER			26

#define SORTSTR_ANAME			string( "1" )
#define SORTSTR_ACOMPLETE		string( "2" )
#define SORTSTR_AINCOMPLETE		string( "3" )
#define SORTSTR_AADDED			string( "4" )
#define SORTSTR_ASIZE			string( "5" )
#define SORTSTR_AFILES			string( "6" )
#define SORTSTR_ACOMMENTS		string( "7" )
#define SORTSTR_AAVGLEFT		string( "8" )
#define SORTSTR_ACOMPLETED		string( "9" )
#define SORTSTR_ATRANSFERRED	string( "10" )
#define SORTSTR_DNAME			string( "11" )
#define SORTSTR_DCOMPLETE		string( "12" )
#define SORTSTR_DINCOMPLETE		string( "13" )
#define SORTSTR_DADDED			string( "14" )
#define SORTSTR_DSIZE			string( "15" )
#define SORTSTR_DFILES			string( "16" )
#define SORTSTR_DCOMMENTS		string( "17" )
#define SORTSTR_DAVGLEFT		string( "18" )
#define SORTSTR_DCOMPLETED		string( "19" )
#define SORTSTR_DTRANSFERRED	string( "20" )
#define SORTSTR_ATAG			string( "21" )
#define SORTSTR_DTAG			string( "22" )
#define SORTSTR_AUPLOADER		string( "23" )
#define SORTSTR_DUPLOADER		string( "24" )
#define SORTSTR_AIP				string( "25" )
#define SORTSTR_DIP				string( "26" )

#define SORTPSTR_AUPPED			string( "1" )
#define SORTPSTR_ADOWNED		string( "2" )
#define SORTPSTR_ALEFT			string( "3" )
#define SORTPSTR_ACONNECTED		string( "4" )
#define SORTPSTR_ASHARERATIO	string( "5" )
#define SORTPSTR_DUPPED			string( "6" )
#define SORTPSTR_DDOWNED		string( "7" )
#define SORTPSTR_DLEFT			string( "8" )
#define SORTPSTR_DCONNECTED		string( "9" )
#define SORTPSTR_DSHARERATIO	string( "10" )
#define SORTPSTR_ACLIENT		string( "11" )
#define SORTPSTR_DCLIENT		string( "12" )
#define SORTPSTR_AIP			string( "13" )
#define SORTPSTR_DIP			string( "14" )
#define SORTPSTR_AAUR			string( "15" )
#define SORTPSTR_DAUR			string( "16" )
#define SORTPSTR_AADR			string( "17" )
#define SORTPSTR_DADR			string( "18" )
#define SORTPSTR_AUR			string( "19" )
#define SORTPSTR_DUR			string( "20" )
#define SORTPSTR_ADR			string( "21" )
#define SORTPSTR_DDR			string( "22" )

#define SORTUSTR_ALOGIN			string( "1" )
#define SORTUSTR_AACCESS			string( "2" )
#define SORTUSTR_AEMAIL			string( "3" )
#define SORTUSTR_ACREATED		string( "4" )
#define SORTUSTR_DLOGIN			string( "5" )
#define SORTUSTR_DACCESS			string( "6" )
#define SORTUSTR_DEMAIL			string( "7" )
#define SORTUSTR_DCREATED		string( "8" )
#define SORTUSTR_ALAST			string( "9" )
#define SORTUSTR_DLAST			string( "10" )
#define SORTUSTR_AWARNED		string( "11" )
#define SORTUSTR_DWARNED		string( "12" )
#define SORTUSTR_ASHARERATIO		string( "13" )
#define SORTUSTR_DSHARERATIO		string( "14" )
#define SORTUSTR_AUPPED			string( "15" )
#define SORTUSTR_DUPPED			string( "16" )
#define SORTUSTR_ADOWNED		string( "17" )
#define SORTUSTR_DDOWNED		string( "18" )
#define SORTUSTR_ABONUS			string( "19" )
#define SORTUSTR_DBONUS			string( "20" )
#define SORTUSTR_ASEEDBONUS		string( "21" )
#define SORTUSTR_DSEEDBONUS		string( "22" )
#define SORTUSTR_AGROUP			string( "23" )
#define SORTUSTR_DGROUP			string( "24" )
#define SORTUSTR_AINVITER		string( "25" )
#define SORTUSTR_DINVITER		string( "26" )

int asortByName( const void *elem1, const void *elem2 );
int asortByComplete( const void *elem1, const void *elem2 );
int asortByDL( const void *elem1, const void *elem2 );
int asortByAdded( const void *elem1, const void *elem2 );
int asortBySize( const void *elem1, const void *elem2 );
int asortByFiles( const void *elem1, const void *elem2 );
int asortByComments( const void *elem1, const void *elem2 );
// int asortByAvgLeft( const void *elem1, const void *elem2 );
// int asortByAvgLeftPercent( const void *elem1, const void *elem2 );
int asortByCompleted( const void *elem1, const void *elem2 );
//int asortByTransferred( const void *elem1, const void *elem2 );
int asortByTag( const void *elem1, const void *elem2 );
int asortByUploader( const void *elem1, const void *elem2 );
int dsortByName( const void *elem1, const void *elem2 );
int dsortByComplete( const void *elem1, const void *elem2 );
int dsortByDL( const void *elem1, const void *elem2 );
int dsortByAdded( const void *elem1, const void *elem2 );
int dsortBySize( const void *elem1, const void *elem2 );
int dsortByFiles( const void *elem1, const void *elem2 );
int dsortByComments( const void *elem1, const void *elem2 );
// int dsortByAvgLeft( const void *elem1, const void *elem2 );
// int dsortByAvgLeftPercent( const void *elem1, const void *elem2 );
int dsortByCompleted( const void *elem1, const void *elem2 );
//int dsortByTransferred( const void *elem1, const void *elem2 );
int dsortByTag( const void *elem1, const void *elem2 );
int dsortByUploader( const void *elem1, const void *elem2 );

int asortByNameNoTop( const void *elem1, const void *elem2 );
int asortByCompleteNoTop( const void *elem1, const void *elem2 );
int asortByDLNoTop( const void *elem1, const void *elem2 );
int asortByAddedNoTop( const void *elem1, const void *elem2 );
int asortBySizeNoTop( const void *elem1, const void *elem2 );
int asortByFilesNoTop( const void *elem1, const void *elem2 );
int asortByCommentsNoTop( const void *elem1, const void *elem2 );
int asortByCompletedNoTop( const void *elem1, const void *elem2 );
int asortByTagNoTop( const void *elem1, const void *elem2 );
int asortByUploaderNoTop( const void *elem1, const void *elem2 );
int dsortByNameNoTop( const void *elem1, const void *elem2 );
int dsortByCompleteNoTop( const void *elem1, const void *elem2 );
int dsortByDLNoTop( const void *elem1, const void *elem2 );
int dsortByAddedNoTop( const void *elem1, const void *elem2 );
int dsortBySizeNoTop( const void *elem1, const void *elem2 );
int dsortByFilesNoTop( const void *elem1, const void *elem2 );
int dsortByCommentsNoTop( const void *elem1, const void *elem2 );
int dsortByCompletedNoTop( const void *elem1, const void *elem2 );
int dsortByTagNoTop( const void *elem1, const void *elem2 );
int dsortByUploaderNoTop( const void *elem1, const void *elem2 );

int asortByIP( const void *elem1, const void *elem2 );
int dsortByIP( const void *elem1, const void *elem2 );

int asortpByUpped( const void *elem1, const void *elem2 );
int asortpByDowned( const void *elem1, const void *elem2 );
int asortpByLeft( const void *elem1, const void *elem2 );
int asortpByConnected( const void *elem1, const void *elem2 );
int asortpByShareRatio( const void *elem1, const void *elem2 );
int dsortpByUpped( const void *elem1, const void *elem2 );
int dsortpByDowned( const void *elem1, const void *elem2 );
int dsortpByLeft( const void *elem1, const void *elem2 );
int dsortpByConnected( const void *elem1, const void *elem2 );
int dsortpByShareRatio( const void *elem1, const void *elem2 );
int asortpByClient( const void *elem1, const void *elem2 );
int dsortpByClient( const void *elem1, const void *elem2 );

int asortpByIP( const void *elem1, const void *elem2 );
int dsortpByIP( const void *elem1, const void *elem2 );
int asortpByAUR( const void *elem1, const void *elem2 );
int dsortpByAUR( const void *elem1, const void *elem2 );
int asortpByADR( const void *elem1, const void *elem2 );
int dsortpByADR( const void *elem1, const void *elem2 );
int asortpByUR( const void *elem1, const void *elem2 );
int dsortpByUR( const void *elem1, const void *elem2 );
int asortpByDR( const void *elem1, const void *elem2 );
int dsortpByDR( const void *elem1, const void *elem2 );

int asortuByLogin( const void *elem1, const void *elem2 );
int asortuByAccess( const void *elem1, const void *elem2 );
int asortuByGroup( const void *elem1, const void *elem2 );
int asortuByInviter( const void *elem1, const void *elem2 );
int asortuByMail( const void *elem1, const void *elem2 );
int asortuByCreated( const void *elem1, const void *elem2 );
int asortuByLast( const void *elem1, const void *elem2 );
int asortuByWarned( const void *elem1, const void *elem2 );
int asortuByShareRatio( const void *elem1, const void *elem2 );
int asortuByUpped( const void *elem1, const void *elem2 );
int asortuByDowned( const void *elem1, const void *elem2 );
int asortuByBonus( const void *elem1, const void *elem2 );
int asortuBySeedBonus( const void *elem1, const void *elem2 );
int dsortuByLogin( const void *elem1, const void *elem2 );
int dsortuByAccess( const void *elem1, const void *elem2 );
int dsortuByGroup( const void *elem1, const void *elem2 );
int dsortuByInviter( const void *elem1, const void *elem2 );
int dsortuByMail( const void *elem1, const void *elem2 );
int dsortuByCreated( const void *elem1, const void *elem2 );
int dsortuByLast( const void *elem1, const void *elem2 );
int dsortuByWarned( const void *elem1, const void *elem2 );
int dsortuByShareRatio( const void *elem1, const void *elem2 );
int dsortuByUpped( const void *elem1, const void *elem2 );
int dsortuByDowned( const void *elem1, const void *elem2 );
int dsortuByBonus( const void *elem1, const void *elem2 );
int dsortuBySeedBonus( const void *elem1, const void *elem2 );

#endif
