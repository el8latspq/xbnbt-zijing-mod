//
// Copyright (C) 2003-2004 Trevor Hogan
//

// =Xotic= Modified Source File

#ifndef SORT_H
 #define SORT_H

#define SORT_ANAME			1
#define SORT_ACOMPLETE			2
#define SORT_AINCOMPLETE		3
#define SORT_AADDED			4
#define SORT_ASIZE			5
#define SORT_AFILES			6
#define SORT_ACOMMENTS			7
#define SORT_AAVGLEFT			8
#define SORT_ACOMPLETED			9
#define SORT_ATRANSFERRED		10
#define SORT_ATAG			11
#define SORT_AUPLOADER			12
#define SORT_AIP			13
#define SORT_ADEFAULT			14

#define SORT_DNAME			21
#define SORT_DCOMPLETE			22
#define SORT_DINCOMPLETE		23
#define SORT_DADDED			24
#define SORT_DSIZE			25
#define SORT_DFILES			26
#define SORT_DCOMMENTS			27
#define SORT_DAVGLEFT			28
#define SORT_DCOMPLETED			29
#define SORT_DTRANSFERRED		30
#define SORT_DTAG			31
#define SORT_DUPLOADER			32
#define SORT_DIP			33
#define SORT_DDEFAULT			34

#define SORTP_AUPPED			1
#define SORTP_ADOWNED			2
#define SORTP_ALEFT			3
#define SORTP_ACONNECTED		4
#define SORTP_ASHARERATIO		5
#define SORTP_ACLIENT			6
#define SORTP_AIP			7
#define SORTP_AAUR			8
#define SORTP_AADR			9
#define SORTP_AUR			10
#define SORTP_ADR			11

#define SORTP_DUPPED			21
#define SORTP_DDOWNED			22
#define SORTP_DLEFT			23
#define SORTP_DCONNECTED		24
#define SORTP_DSHARERATIO		25
#define SORTP_DCLIENT			26
#define SORTP_DIP			27
#define SORTP_DAUR			28
#define SORTP_DADR			29
#define SORTP_DUR			30
#define SORTP_DDR			31

#define SORTU_ALOGIN			1
#define SORTU_AACCESS			2
#define SORTU_AEMAIL			3
#define SORTU_ACREATED			4
#define SORTU_ALAST			5
#define SORTU_AWARNED			6
#define SORTU_ASHARERATIO		7
#define SORTU_AUPPED			8
#define SORTU_ADOWNED			9
#define SORTU_ABONUS			10
#define SORTU_ASEEDBONUS		11
#define SORTU_AGROUP			12
#define SORTU_AINVITER			13

#define SORTU_DLOGIN			21
#define SORTU_DACCESS			22
#define SORTU_DEMAIL			23
#define SORTU_DCREATED			24
#define SORTU_DLAST			25
#define SORTU_DWARNED			26
#define SORTU_DSHARERATIO		27
#define SORTU_DUPPED			28
#define SORTU_DDOWNED			29
#define SORTU_DBONUS			30
#define SORTU_DSEEDBONUS		31
#define SORTU_DGROUP			32
#define SORTU_DINVITER			33

#define SORTSTR_ANAME			string( "1" )
#define SORTSTR_ACOMPLETE		string( "2" )
#define SORTSTR_AINCOMPLETE		string( "3" )
#define SORTSTR_AADDED			string( "4" )
#define SORTSTR_ASIZE			string( "5" )
#define SORTSTR_AFILES			string( "6" )
#define SORTSTR_ACOMMENTS		string( "7" )
#define SORTSTR_AAVGLEFT		string( "8" )
#define SORTSTR_ACOMPLETED		string( "9" )
#define SORTSTR_ATRANSFERRED		string( "10" )
#define SORTSTR_ATAG			string( "11" )
#define SORTSTR_AUPLOADER		string( "12" )
#define SORTSTR_AIP			string( "13" )
#define SORTSTR_ADEFAULT		string( "14" )

#define SORTSTR_DNAME			string( "21" )
#define SORTSTR_DCOMPLETE		string( "22" )
#define SORTSTR_DINCOMPLETE		string( "23" )
#define SORTSTR_DADDED			string( "24" )
#define SORTSTR_DSIZE			string( "25" )
#define SORTSTR_DFILES			string( "26" )
#define SORTSTR_DCOMMENTS		string( "27" )
#define SORTSTR_DAVGLEFT		string( "28" )
#define SORTSTR_DCOMPLETED		string( "29" )
#define SORTSTR_DTRANSFERRED		string( "30" )
#define SORTSTR_DTAG			string( "31" )
#define SORTSTR_DUPLOADER		string( "32" )
#define SORTSTR_DIP			string( "33" )
#define SORTSTR_DDEFAULT		string( "34" )

#define SORTPSTR_AUPPED			string( "1" )
#define SORTPSTR_ADOWNED		string( "2" )
#define SORTPSTR_ALEFT			string( "3" )
#define SORTPSTR_ACONNECTED		string( "4" )
#define SORTPSTR_ASHARERATIO		string( "5" )
#define SORTPSTR_ACLIENT		string( "6" )
#define SORTPSTR_AIP			string( "7" )
#define SORTPSTR_AAUR			string( "8" )
#define SORTPSTR_AADR			string( "9" )
#define SORTPSTR_AUR			string( "10" )
#define SORTPSTR_ADR			string( "11" )

#define SORTPSTR_DUPPED			string( "21" )
#define SORTPSTR_DDOWNED		string( "22" )
#define SORTPSTR_DLEFT			string( "23" )
#define SORTPSTR_DCONNECTED		string( "24" )
#define SORTPSTR_DSHARERATIO		string( "25" )
#define SORTPSTR_DCLIENT		string( "26" )
#define SORTPSTR_DIP			string( "27" )
#define SORTPSTR_DAUR			string( "28" )
#define SORTPSTR_DADR			string( "29" )
#define SORTPSTR_DUR			string( "30" )
#define SORTPSTR_DDR			string( "31" )

#define SORTUSTR_ALOGIN			string( "1" )
#define SORTUSTR_AACCESS		string( "2" )
#define SORTUSTR_AEMAIL			string( "3" )
#define SORTUSTR_ACREATED		string( "4" )
#define SORTUSTR_ALAST			string( "5" )
#define SORTUSTR_AWARNED		string( "6" )
#define SORTUSTR_ASHARERATIO		string( "7" )
#define SORTUSTR_AUPPED			string( "8" )
#define SORTUSTR_ADOWNED		string( "9" )
#define SORTUSTR_ABONUS			string( "10" )
#define SORTUSTR_ASEEDBONUS		string( "11" )
#define SORTUSTR_AGROUP			string( "12" )
#define SORTUSTR_AINVITER		string( "13" )

#define SORTUSTR_DLOGIN			string( "21" )
#define SORTUSTR_DACCESS		string( "22" )
#define SORTUSTR_DEMAIL			string( "23" )
#define SORTUSTR_DCREATED		string( "24" )
#define SORTUSTR_DLAST			string( "25" )
#define SORTUSTR_DWARNED		string( "26" )
#define SORTUSTR_DSHARERATIO		string( "27" )
#define SORTUSTR_DUPPED			string( "28" )
#define SORTUSTR_DDOWNED		string( "29" )
#define SORTUSTR_DBONUS			string( "30" )
#define SORTUSTR_DSEEDBONUS		string( "31" )
#define SORTUSTR_DGROUP			string( "32" )
#define SORTUSTR_DINVITER		string( "33" )

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
int asortByDefault( const void *elem1, const void *elem2 );
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
int dsortByDefault( const void *elem1, const void *elem2 );

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
int asortByDefaultNoTop( const void *elem1, const void *elem2 );
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
int dsortByDefaultNoTop( const void *elem1, const void *elem2 );

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
