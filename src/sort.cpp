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
#include "sort.h"
#include "tracker.h"
#include "util.h"

int asortByTop( const void *elem1, const void *elem2 )
{
	if( ( (const struct torrent_t *)elem1 )->ucTop != ( (const struct torrent_t *)elem2 )->ucTop )
	{
		return ( (const struct torrent_t *)elem2 )->ucTop - ( (const struct torrent_t *)elem1 )->ucTop;
//		if( ( (const struct torrent_t *)elem1 )->bTop )
//			return -1;
//		else
//			return 1;
	}
	else
		return 0;

}

int asortByName( const void *elem1, const void *elem2 )
{
	if( asortByTop( elem1, elem2 ) != 0 )
		return asortByTop( elem1, elem2 );
	else
//		return ( (const struct torrent_t *)elem1 )->strName.compare( ( (const struct torrent_t *)elem2 )->strName );
		return ( (const struct torrent_t *)elem1 )->strLowerName.compare( ( (const struct torrent_t *)elem2 )->strLowerName );
}

int asortByComplete( const void *elem1, const void *elem2 )
{
	if( asortByTop( elem1, elem2 ) != 0 )
		return asortByTop( elem1, elem2 );
	else
		return ( (const struct torrent_t *)elem1 )->uiSeeders - ( (const struct torrent_t *)elem2 )->uiSeeders;
}

int asortByDL( const void *elem1, const void *elem2 )
{
	if( asortByTop( elem1, elem2 ) != 0 )
		return asortByTop( elem1, elem2 );
	else
		return ( (const struct torrent_t *)elem1 )->uiLeechers - ( (const struct torrent_t *)elem2 )->uiLeechers;
}

int asortByAdded( const void *elem1, const void *elem2 )
{
	if( asortByTop( elem1, elem2 ) != 0 )
		return asortByTop( elem1, elem2 );
	else
		return ( (const struct torrent_t *)elem1 )->strAdded.compare( ( (const struct torrent_t *)elem2 )->strAdded );
}

int asortBySize( const void *elem1, const void *elem2 )
{
	// int64's will overflow, force a compare

	const struct torrent_t *tor1( (const struct torrent_t *)elem1 );
	const struct torrent_t *tor2( (const struct torrent_t *)elem2 );
	
	if( asortByTop( elem1, elem2 ) != 0 )
		return asortByTop( elem1, elem2 );
	else
		if( tor1->iSize < tor2->iSize )
			return -1;
		else if( tor1->iSize > tor2->iSize )
			return 1;
		else
			return 0;
}

int asortByFiles( const void *elem1, const void *elem2 )
{
	if( asortByTop( elem1, elem2 ) != 0 )
		return asortByTop( elem1, elem2 );
	else
		return ( (const struct torrent_t *)elem1 )->uiFiles - ( (const struct torrent_t *)elem2 )->uiFiles;
}

int asortByComments( const void *elem1, const void *elem2 )
{
	if( asortByTop( elem1, elem2 ) != 0 )
		return asortByTop( elem1, elem2 );
	else
		return ( (const struct torrent_t *)elem1 )->uiComments - ( (const struct torrent_t *)elem2 )->uiComments;
}

// int asortByAvgLeft( const void *elem1, const void *elem2 )
// {
// 	// int64's will overflow, force a compare
// 
// 	const struct torrent_t *tor1( (const struct torrent_t *)elem1 );
// 	const struct torrent_t *tor2( (const struct torrent_t *)elem2 );
// 	
// 	if( asortByTop( elem1, elem2 ) != 0 )
// 		return asortByTop( elem1, elem2 );
// 	else
// 		if( tor1->iAverageLeft < tor2->iAverageLeft )
// 			return -1;
// 		else if( tor1->iAverageLeft > tor2->iAverageLeft )
// 			return 1;
// 		else
// 			return 0;
// }

// int asortByAvgLeftPercent( const void *elem1, const void *elem2 )
// {
// 	if( asortByTop( elem1, elem2 ) != 0 )
// 		return asortByTop( elem1, elem2 );
// 	else
// 		return ( (const struct torrent_t *)elem1 )->ucAverageLeftPercent - ( (const struct torrent_t *)elem2 )->ucAverageLeftPercent;
// }

int asortByCompleted( const void *elem1, const void *elem2 )
{
	if( asortByTop( elem1, elem2 ) != 0 )
		return asortByTop( elem1, elem2 );
	else
		return ( (const struct torrent_t *)elem1 )->ulCompleted - ( (const struct torrent_t *)elem2 )->ulCompleted;
}

//int asortByTransferred( const void *elem1, const void *elem2 )
//{
//	// int64's will overflow, force a compare

//	const struct torrent_t *tor1( (const struct torrent_t *)elem1 );
//	const struct torrent_t *tor2( (const struct torrent_t *)elem2 );
//	
//	if( asortByTop( elem1, elem2 ) != 0 )
//		return asortByTop( elem1, elem2 );
//	else
//		if( tor1->iTransferred < tor2->iTransferred )
//			return -1;
//		else if( tor1->iTransferred > tor2->iTransferred )
//			return 1;
//		else
//			return 0;
//}

int asortByTag( const void *elem1, const void *elem2 )
{
	if( asortByTop( elem1, elem2 ) != 0 )
		return asortByTop( elem1, elem2 );
	else
		return ( (const struct torrent_t *)elem1 )->strTag.compare( ( (const struct torrent_t *)elem2 )->strTag );
} 

int asortByUploader( const void *elem1, const void *elem2 )
{
	if( asortByTop( elem1, elem2 ) != 0 )
		return asortByTop( elem1, elem2 );
	else
		return ( (const struct torrent_t *)elem1 )->strUploader.compare( ( (const struct torrent_t *)elem2 )->strUploader );
}

int dsortByName( const void *elem1, const void *elem2 )
{
	if( asortByTop( elem1, elem2 ) != 0 )
		return asortByTop( elem1, elem2 );
	else
//		return ( (const struct torrent_t *)elem2 )->strName.compare( ( (const struct torrent_t *)elem1 )->strName );
		return ( (const struct torrent_t *)elem2 )->strLowerName.compare( ( (const struct torrent_t *)elem1 )->strLowerName );
}

int dsortByComplete( const void *elem1, const void *elem2 )
{
	if( asortByTop( elem1, elem2 ) != 0 )
		return asortByTop( elem1, elem2 );
	else
		return ( (const struct torrent_t *)elem2 )->uiSeeders - ( (const struct torrent_t *)elem1 )->uiSeeders;
}

int dsortByDL( const void *elem1, const void *elem2 )
{
	if( asortByTop( elem1, elem2 ) != 0 )
		return asortByTop( elem1, elem2 );
	else
		return ( (const struct torrent_t *)elem2 )->uiLeechers - ( (const struct torrent_t *)elem1 )->uiLeechers;
}

int dsortByAdded( const void *elem1, const void *elem2 )
{
	if( asortByTop( elem1, elem2 ) != 0 )
		return asortByTop( elem1, elem2 );
	else
		return ( (const struct torrent_t *)elem2 )->strAdded.compare( ( (const struct torrent_t *)elem1 )->strAdded );
}

int dsortBySize( const void *elem1, const void *elem2 )
{
	// int64's will overflow, force a compare

	const struct torrent_t *tor1( (const struct torrent_t *)elem1 );
	const struct torrent_t *tor2( (const struct torrent_t *)elem2 );
	
	if( asortByTop( elem1, elem2 ) != 0 )
		return asortByTop( elem1, elem2 );
	else
		if( tor1->iSize < tor2->iSize )
			return 1;
		else if( tor1->iSize > tor2->iSize )
			return -1;
		else
			return 0;
}

int dsortByFiles( const void *elem1, const void *elem2 )
{
	if( asortByTop( elem1, elem2 ) != 0 )
		return asortByTop( elem1, elem2 );
	else
		return ( (const struct torrent_t *)elem2 )->uiFiles - ( (const struct torrent_t *)elem1 )->uiFiles;
}

int dsortByComments( const void *elem1, const void *elem2 )
{
	if( asortByTop( elem1, elem2 ) != 0 )
		return asortByTop( elem1, elem2 );
	else
		return ( (const struct torrent_t *)elem2 )->uiComments - ( (const struct torrent_t *)elem1 )->uiComments;
}

// int dsortByAvgLeft( const void *elem1, const void *elem2 )
// {
// 	// int64's will overflow, force a compare
// 
// 	const struct torrent_t *tor1( (const struct torrent_t *)elem1 );
// 	const struct torrent_t *tor2( (const struct torrent_t *)elem2 );
// 
// 	if( asortByTop( elem1, elem2 ) != 0 )
// 		return asortByTop( elem1, elem2 );
// 	else
// 		if( tor1->iAverageLeft < tor2->iAverageLeft )
// 			return 1;
// 		else if( tor1->iAverageLeft > tor2->iAverageLeft )
// 			return -1;
// 		else
// 			return 0;
// }

// int dsortByAvgLeftPercent( const void *elem1, const void *elem2 )
// {
// 	if( asortByTop( elem1, elem2 ) != 0 )
// 		return asortByTop( elem1, elem2 );
// 	else
// 		return ( (const struct torrent_t *)elem2 )->ucAverageLeftPercent - ( (const struct torrent_t *)elem1 )->ucAverageLeftPercent;
// }

int dsortByCompleted( const void *elem1, const void *elem2 )
{
	if( asortByTop( elem1, elem2 ) != 0 )
		return asortByTop( elem1, elem2 );
	else
		return ( (const struct torrent_t *)elem2 )->ulCompleted - ( (const struct torrent_t *)elem1 )->ulCompleted;
}

//int dsortByTransferred( const void *elem1, const void *elem2 )
//{
//	// int64's will overflow, force a compare

//	const struct torrent_t *tor1( (const struct torrent_t *)elem1 );
//	const struct torrent_t *tor2( (const struct torrent_t *)elem2 );

//	if( asortByTop( elem1, elem2 ) != 0 )
//		return asortByTop( elem1, elem2 );
//	else
//		if( tor1->iTransferred < tor2->iTransferred )
//			return 1;
//		else if( tor1->iTransferred > tor2->iTransferred )
//			return -1;
//		else
//			return 0;
//}  

int dsortByTag( const void *elem1, const void *elem2 )
{
	if( asortByTop( elem1, elem2 ) != 0 )
		return asortByTop( elem1, elem2 );
	else
		return ( (const struct torrent_t *)elem2 )->strTag.compare( ( (const struct torrent_t *)elem1 )->strTag );
}

int dsortByUploader( const void *elem1, const void *elem2 )
{
	if( asortByTop( elem1, elem2 ) != 0 )
		return asortByTop( elem1, elem2 );
	else
		return ( (const struct torrent_t *)elem2 )->strUploader.compare( ( (const struct torrent_t *)elem1 )->strUploader );
}

int asortByIP( const void *elem1, const void *elem2 )
{
	if( asortByTop( elem1, elem2 ) != 0 )
		return asortByTop( elem1, elem2 );
	else
		return ( (const struct torrent_t *)elem1 )->strIP.compare( ( (const struct torrent_t *)elem2 )->strIP );
}

int dsortByIP( const void *elem1, const void *elem2 )
{
	if( asortByTop( elem1, elem2 ) != 0 )
		return asortByTop( elem1, elem2 );
	else
		return ( (const struct torrent_t *)elem2 )->strIP.compare( ( (const struct torrent_t *)elem1 )->strIP );
}

//sortByNoTop Start
int asortByNameNoTop( const void *elem1, const void *elem2 )
{
//	return ( (const struct torrent_t *)elem1 )->strName.compare( ( (const struct torrent_t *)elem2 )->strName );
	return ( (const struct torrent_t *)elem1 )->strLowerName.compare( ( (const struct torrent_t *)elem2 )->strLowerName );
}

int asortByCompleteNoTop( const void *elem1, const void *elem2 )
{
	return ( (const struct torrent_t *)elem1 )->uiSeeders - ( (const struct torrent_t *)elem2 )->uiSeeders;
}

int asortByDLNoTop( const void *elem1, const void *elem2 )
{
	return ( (const struct torrent_t *)elem1 )->uiLeechers - ( (const struct torrent_t *)elem2 )->uiLeechers;
}

int asortByAddedNoTop( const void *elem1, const void *elem2 )
{
	return ( (const struct torrent_t *)elem1 )->strAdded.compare( ( (const struct torrent_t *)elem2 )->strAdded );
}

int asortBySizeNoTop( const void *elem1, const void *elem2 )
{
	// int64's will overflow, force a compare

	const struct torrent_t *tor1( (const struct torrent_t *)elem1 );
	const struct torrent_t *tor2( (const struct torrent_t *)elem2 );
	
	if( tor1->iSize < tor2->iSize )
		return -1;
	else if( tor1->iSize > tor2->iSize )
		return 1;
	else
		return 0;
}

int asortByFilesNoTop( const void *elem1, const void *elem2 )
{
	return ( (const struct torrent_t *)elem1 )->uiFiles - ( (const struct torrent_t *)elem2 )->uiFiles;
}

int asortByCommentsNoTop( const void *elem1, const void *elem2 )
{
	return ( (const struct torrent_t *)elem1 )->uiComments - ( (const struct torrent_t *)elem2 )->uiComments;
}

int asortByCompletedNoTop( const void *elem1, const void *elem2 )
{
	return ( (const struct torrent_t *)elem1 )->ulCompleted - ( (const struct torrent_t *)elem2 )->ulCompleted;
}

int asortByTagNoTop( const void *elem1, const void *elem2 )
{
	return ( (const struct torrent_t *)elem1 )->strTag.compare( ( (const struct torrent_t *)elem2 )->strTag );
} 

int asortByUploaderNoTop( const void *elem1, const void *elem2 )
{
	return ( (const struct torrent_t *)elem1 )->strUploader.compare( ( (const struct torrent_t *)elem2 )->strUploader );
}

int dsortByNameNoTop( const void *elem1, const void *elem2 )
{
//	return ( (const struct torrent_t *)elem2 )->strName.compare( ( (const struct torrent_t *)elem1 )->strName );
	return ( (const struct torrent_t *)elem2 )->strLowerName.compare( ( (const struct torrent_t *)elem1 )->strLowerName );
}

int dsortByCompleteNoTop( const void *elem1, const void *elem2 )
{
	return ( (const struct torrent_t *)elem2 )->uiSeeders - ( (const struct torrent_t *)elem1 )->uiSeeders;
}

int dsortByDLNoTop( const void *elem1, const void *elem2 )
{
	return ( (const struct torrent_t *)elem2 )->uiLeechers - ( (const struct torrent_t *)elem1 )->uiLeechers;
}

int dsortByAddedNoTop( const void *elem1, const void *elem2 )
{
	return ( (const struct torrent_t *)elem2 )->strAdded.compare( ( (const struct torrent_t *)elem1 )->strAdded );
}

int dsortBySizeNoTop( const void *elem1, const void *elem2 )
{
	// int64's will overflow, force a compare

	const struct torrent_t *tor1( (const struct torrent_t *)elem1 );
	const struct torrent_t *tor2( (const struct torrent_t *)elem2 );
	
	if( tor1->iSize < tor2->iSize )
		return 1;
	else if( tor1->iSize > tor2->iSize )
		return -1;
	else
		return 0;
}

int dsortByFilesNoTop( const void *elem1, const void *elem2 )
{
	return ( (const struct torrent_t *)elem2 )->uiFiles - ( (const struct torrent_t *)elem1 )->uiFiles;
}

int dsortByCommentsNoTop( const void *elem1, const void *elem2 )
{
	return ( (const struct torrent_t *)elem2 )->uiComments - ( (const struct torrent_t *)elem1 )->uiComments;
}

int dsortByCompletedNoTop( const void *elem1, const void *elem2 )
{
	return ( (const struct torrent_t *)elem2 )->ulCompleted - ( (const struct torrent_t *)elem1 )->ulCompleted;
}

int dsortByTagNoTop( const void *elem1, const void *elem2 )
{
	return ( (const struct torrent_t *)elem2 )->strTag.compare( ( (const struct torrent_t *)elem1 )->strTag );
}

int dsortByUploaderNoTop( const void *elem1, const void *elem2 )
{
	return ( (const struct torrent_t *)elem2 )->strUploader.compare( ( (const struct torrent_t *)elem1 )->strUploader );
}
//sortByNoTop Complete

int asortpByUpped( const void *elem1, const void *elem2 )
{
	// int64's will overflow, force a compare

	const struct peer_t *peer1( (const struct peer_t *)elem1 );
	const struct peer_t *peer2( (const struct peer_t *)elem2 );

	if( peer1->iUpped < peer2->iUpped )
		return -1;
	else if( peer1->iUpped > peer2->iUpped )
		return 1;
	else
		return 0;
}

int asortpByDowned( const void *elem1, const void *elem2 )
{
	// int64's will overflow, force a compare

	const struct peer_t *peer1( (const struct peer_t *)elem1 );
	const struct peer_t *peer2( (const struct peer_t *)elem2 );

	if( peer1->iDowned < peer2->iDowned )
		return -1;
	else if( peer1->iDowned > peer2->iDowned )
		return 1;
	else
		return 0;
}

int asortpByLeft( const void *elem1, const void *elem2 )
{
	// int64's will overflow, force a compare

	const struct peer_t *peer1( (const struct peer_t *)elem1 );
	const struct peer_t *peer2( (const struct peer_t *)elem2 );

	if( peer1->iLeft < peer2->iLeft )
		return -1;
	else if( peer1->iLeft > peer2->iLeft )
		return 1;
	else
		return 0;
}

int asortpByConnected( const void *elem1, const void *elem2 )
{
	return ( (const struct peer_t *)elem1 )->ulConnected - ( (const struct peer_t *)elem2 )->ulConnected;
}

int asortpByShareRatio( const void *elem1, const void *elem2 )
{
	const float fl1( ( (const struct peer_t *)elem1 )->flShareRatio );
	const float fl2( ( (const struct peer_t *)elem2 )->flShareRatio );

	// this is complicated because -1 means infinite and casting to ints won't work

	// Elandal: else is unnecessary because of return...
	// Elandal: floating point comparison using == or != is not recommended.
	//          It just might not work as expected. Don't have time to fix now.
	//          (fix I did against 80b-2 might not be any better)

	if( ( ( -0.0001 < ( fl1 - fl2 ) ) && ( ( fl1 - fl2 ) < 0.0001 ) ) ) return 0;
	if( ( -1.001 < fl1 ) && ( fl1 < -0.999 ) ) return 1;
	if( ( -1.001 < fl2 ) && ( fl2 < -0.999 ) ) return -1;
	if( ( ( fl1 - fl2 ) < -0.0001 ) )return -1;

	return 1;
}

int dsortpByUpped( const void *elem1, const void *elem2 )
{
	// int64's will overflow, force a compare

	const struct peer_t *peer1( (const struct peer_t *)elem1 );
	const struct peer_t *peer2( (const struct peer_t *)elem2 );

	if( peer1->iUpped < peer2->iUpped )
		return 1;
	else if( peer1->iUpped > peer2->iUpped )
		return -1;
	else
		return 0;
}

int dsortpByDowned( const void *elem1, const void *elem2 )
{
	// int64's will overflow, force a compare

	const struct peer_t *peer1( (const struct peer_t *)elem1 );
	const struct peer_t *peer2( (const struct peer_t *)elem2 );

	if( peer1->iDowned < peer2->iDowned )
		return 1;
	else if( peer1->iDowned > peer2->iDowned )
		return -1;
	else
		return 0;
}

int dsortpByLeft( const void *elem1, const void *elem2 )
{
	// int64's will overflow, force a compare

	const struct peer_t *peer1( (const struct peer_t *)elem1 );
	const struct peer_t *peer2( (const struct peer_t *)elem2 );

	if( peer1->iLeft < peer2->iLeft )
		return 1;
	else if( peer1->iLeft > peer2->iLeft )
		return -1;
	else
		return 0;
}

int dsortpByConnected( const void *elem1, const void *elem2 )
{
	return ( (const struct peer_t *)elem2 )->ulConnected - ( (const struct peer_t *)elem1 )->ulConnected;
}

int dsortpByShareRatio( const void *elem1, const void *elem2 )
{
	const float fl1( ( (const struct peer_t *)elem1 )->flShareRatio );
	const float fl2( ( (const struct peer_t *)elem2 )->flShareRatio );

	if( ( ( -0.0001 < ( fl1 - fl2 ) ) && ( ( fl1 - fl2 ) < 0.0001 ) ) ) return 0;
	if( ( -1.001 < fl1 ) && ( fl1 < -0.999 ) ) return -1;
	if( ( -1.001 < fl2 ) && ( fl2 < -0.999 ) ) return 1;
	if( ( ( fl1 - fl2 ) < -0.0001 ) )return 1;

	return -1;
}

int asortpByClient( const void *elem1, const void *elem2 )
{
	return ( (const struct peer_t *)elem1 )->strClientType.compare( ( (const struct peer_t *)elem2 )->strClientType );
}

int dsortpByClient( const void *elem1, const void *elem2 )
{
	return ( (const struct peer_t *)elem2 )->strClientType.compare( ( (const struct peer_t *)elem1 )->strClientType );
}

int asortpByIP( const void *elem1, const void *elem2 )
{
	return ( (const struct peer_t *)elem1 )->strIP.compare( ( (const struct peer_t *)elem2 )->strIP );
}

int dsortpByIP( const void *elem1, const void *elem2 )
{
	return ( (const struct peer_t *)elem2 )->strIP.compare( ( (const struct peer_t *)elem1 )->strIP );
}

int asortpByAUR( const void *elem1, const void *elem2 )
{
	
	const struct peer_t *peer1( (const struct peer_t *)elem1 );
	const struct peer_t *peer2( (const struct peer_t *)elem2 );
	
	if( peer1->ulConnected !=0 && peer2->ulConnected !=0 )
	{
		if( peer1->ulConnected && ( peer1->iUpped / peer1->ulConnected ) < ( peer2->iUpped / peer2->ulConnected ) )
			return 1;
		else if( ( peer1->iUpped / peer1->ulConnected ) > ( peer2->iUpped / peer2->ulConnected ) )
			return -1;
		else
			return 0;
	}
	else
		return 0;
}

int dsortpByAUR( const void *elem1, const void *elem2 )
{
	const struct peer_t *peer1( (const struct peer_t *)elem1 );
	const struct peer_t *peer2( (const struct peer_t *)elem2 );

	if( peer1->ulConnected != 0 && peer2->ulConnected != 0 )
	{
		if( ( peer1->ulConnected && peer2->ulConnected ) && ( peer2->iUpped / peer2->ulConnected ) < ( peer1->iUpped / peer1->ulConnected ) )
			return 1;
		else if( ( peer2->iUpped / peer2->ulConnected ) > ( peer1->iUpped / peer1->ulConnected ) )
			return -1;
	else
		return 0;
	}
	else
		return 0;
}

int asortpByADR( const void *elem1, const void *elem2 )
{
	const struct peer_t *peer1( (const struct peer_t *)elem1 );
	const struct peer_t *peer2( (const struct peer_t *)elem2 );

	if( peer1->ulConnected !=0 && peer2->ulConnected !=0 )
	{
		if( ( peer1->iDowned / peer1->ulConnected ) < ( peer2->iDowned / peer2->ulConnected ) )
			return 1;
		else if( ( peer1->iDowned / peer1->ulConnected ) > ( peer2->iDowned / peer2->ulConnected ) )
			return -1;
	else
		return 0;
	}
	else
		return 0;
}

int dsortpByADR( const void *elem1, const void *elem2 )
{
	const struct peer_t *peer1( (const struct peer_t *)elem1 );
	const struct peer_t *peer2( (const struct peer_t *)elem2 );

	if( peer1->ulConnected !=0 && peer2->ulConnected !=0 )
	{
		if( ( peer2->iDowned / peer2->ulConnected ) < ( peer1->iDowned / peer1->ulConnected ) )
			return 1;
		else if( ( peer2->iDowned / peer2->ulConnected ) > ( peer1->iDowned / peer1->ulConnected ) )
			return -1;
		else
			return 0;
	}
	else
		return 0;
}

int asortpByUR( const void *elem1, const void *elem2 )
{
	
	const struct peer_t *peer1( (const struct peer_t *)elem1 );
	const struct peer_t *peer2( (const struct peer_t *)elem2 );
	
	if( peer1->ulUpSpeed < peer2->ulUpSpeed )
		return -1;
	else if( peer1->ulUpSpeed > peer2->ulUpSpeed )
		return 1;
	else
		return 0;
}

int dsortpByUR( const void *elem1, const void *elem2 )
{
	const struct peer_t *peer1( (const struct peer_t *)elem1 );
	const struct peer_t *peer2( (const struct peer_t *)elem2 );

	if( peer1->ulUpSpeed < peer2->ulUpSpeed )
		return 1;
	else if( peer1->ulUpSpeed > peer2->ulUpSpeed )
		return -1;
	else
		return 0;
}

int asortpByDR( const void *elem1, const void *elem2 )
{
	const struct peer_t *peer1( (const struct peer_t *)elem1 );
	const struct peer_t *peer2( (const struct peer_t *)elem2 );

	if( peer1->ulDownSpeed < peer2->ulDownSpeed )
		return -1;
	else if( peer1->ulDownSpeed > peer2->ulDownSpeed )
		return 1;
	else
		return 0;
}

int dsortpByDR( const void *elem1, const void *elem2 )
{
	const struct peer_t *peer1( (const struct peer_t *)elem1 );
	const struct peer_t *peer2( (const struct peer_t *)elem2 );

	if( peer1->ulDownSpeed < peer2->ulDownSpeed )
		return 1;
	else if( peer1->ulDownSpeed > peer2->ulDownSpeed )
		return -1;
	else
		return 0;
}

int asortuByLogin( const void *elem1, const void *elem2 )
{
//	return ( (const struct user_t *)elem1 )->strLogin.compare( ( (const struct user_t *)elem2 )->strLogin );
	return ( (const struct user_t *)elem1 )->strLowerLogin.compare( ( (const struct user_t *)elem2 )->strLowerLogin );
}

int asortuByAccess( const void *elem1, const void *elem2 )
{
	return ( (const struct user_t *)elem1 )->ucAccess - ( (const struct user_t *)elem2 )->ucAccess;
}

int asortuByGroup( const void *elem1, const void *elem2 )
{
	return ( (const struct user_t *)elem1 )->ucGroup - ( (const struct user_t *)elem2 )->ucGroup;
}

int asortuByInviter( const void *elem1, const void *elem2 )
{
	return UTIL_ToLower( ( (const struct user_t *)elem1 )->strInviter ).compare( UTIL_ToLower( ( (const struct user_t *)elem2 )->strInviter ) );
}

int asortuByMail( const void *elem1, const void *elem2 )
{
//	return ( (const struct user_t *)elem1 )->strMail.compare( ( (const struct user_t *)elem2 )->strMail );
	return ( (const struct user_t *)elem1 )->strLowerMail.compare( ( (const struct user_t *)elem2 )->strLowerMail );
}

int asortuByCreated( const void *elem1, const void *elem2 )
{
	return ( (const struct user_t *)elem1 )->strCreated.compare( ( (const struct user_t *)elem2 )->strCreated );
}

int asortuByLast( const void *elem1, const void *elem2 )
{
	const struct user_t *user1( (const struct user_t *)elem1 );
	const struct user_t *user2( (const struct user_t *)elem2 );

	if( user1->tLast < user2->tLast )
		return -1;
	else if( user1->tLast > user2->tLast )
		return 1;
	else
		return 0;
}

int asortuByWarned( const void *elem1, const void *elem2 )
{
	const struct user_t *user1( (const struct user_t *)elem1 );
	const struct user_t *user2( (const struct user_t *)elem2 );

	if( user1->tWarned < user2->tWarned )
		return -1;
	else if( user1->tWarned > user2->tWarned )
		return 1;
	else
		return 0;
}

int asortuByShareRatio( const void *elem1, const void *elem2 )
{
	const float fl1( ( (const struct user_t *)elem1 )->flShareRatio );
	const float fl2( ( (const struct user_t *)elem2 )->flShareRatio );
	
	if( ( ( -0.0001 < ( fl1 - fl2 ) ) && ( ( fl1 - fl2 ) < 0.0001 ) ) ) return 0;
	if( ( -1.001 < fl1 ) && ( fl1 < -0.999 ) ) return 1;
	if( ( -1.001 < fl2 ) && ( fl2 < -0.999 ) ) return -1;
	if( ( ( fl1 - fl2 ) < -0.0001 ) )return -1;

	return 1;

}

int asortuByUpped( const void *elem1, const void *elem2 )
{
	// int64's will overflow, force a compare

	const struct user_t *user1( (const struct user_t *)elem1 );
	const struct user_t *user2( (const struct user_t *)elem2 );

	if( user1->ulUploaded < user2->ulUploaded )
		return -1;
	else if( user1->ulUploaded > user2->ulUploaded )
		return 1;
	else
		return 0;
}

int asortuByDowned( const void *elem1, const void *elem2 )
{
	// int64's will overflow, force a compare

	const struct user_t *user1( (const struct user_t *)elem1 );
	const struct user_t *user2( (const struct user_t *)elem2 );

	if( user1->ulDownloaded < user2->ulDownloaded )
		return -1;
	else if( user1->ulDownloaded > user2->ulDownloaded )
		return 1;
	else
		return 0;
}

int asortuByBonus( const void *elem1, const void *elem2 )
{
	// int64's will overflow, force a compare

	const struct user_t *user1( (const struct user_t *)elem1 );
	const struct user_t *user2( (const struct user_t *)elem2 );

	if( user1->ulBonus < user2->ulBonus )
		return -1;
	else if( user1->ulBonus > user2->ulBonus )
		return 1;
	else
		return 0;
}

int asortuBySeedBonus( const void *elem1, const void *elem2 )
{
	const float fl1( ( (const struct user_t *)elem1 )->flSeedBonus );
	const float fl2( ( (const struct user_t *)elem2 )->flSeedBonus );

	if( ( ( -0.001 < ( fl1 - fl2 ) ) && ( ( fl1 - fl2 ) < 0.001 ) ) ) return 0;
	if( ( -1.001 < fl1 ) && ( fl1 < -0.999 ) ) return 1;
	if( ( -1.001 < fl2 ) && ( fl2 < -0.999 ) ) return -1;
	if( ( ( fl1 - fl2 ) < -0.001 ) )return -1;

	return 1;
}

int dsortuByLogin( const void *elem1, const void *elem2 )
{
//	return ( (const struct user_t *)elem2 )->strLogin.compare( ( (const struct user_t *)elem1 )->strLogin );
	return ( (const struct user_t *)elem2 )->strLowerLogin.compare( ( (const struct user_t *)elem1 )->strLowerLogin );
}

int dsortuByAccess( const void *elem1, const void *elem2 )
{
	return ( (const struct user_t *)elem2 )->ucAccess - ( (const struct user_t *)elem1 )->ucAccess;
}

int dsortuByGroup( const void *elem1, const void *elem2 )
{
	return ( (const struct user_t *)elem2 )->ucGroup - ( (const struct user_t *)elem1 )->ucGroup;
}

int dsortuByInviter( const void *elem1, const void *elem2 )
{
	return UTIL_ToLower( ( (const struct user_t *)elem2 )->strInviter ).compare( UTIL_ToLower( ( (const struct user_t *)elem1 )->strInviter ) );
}

int dsortuByMail( const void *elem1, const void *elem2 )
{
//	return ( (const struct user_t *)elem2 )->strMail.compare( ( (const struct user_t *)elem1 )->strMail );
	return ( (const struct user_t *)elem2 )->strLowerMail.compare( ( (const struct user_t *)elem1 )->strLowerMail );
}

int dsortuByCreated( const void *elem1, const void *elem2 )
{
	return ( (const struct user_t *)elem2 )->strCreated.compare( ( (const struct user_t *)elem1 )->strCreated );
}

int dsortuByLast( const void *elem1, const void *elem2 )
{
	const struct user_t *user1( (const struct user_t *)elem1 );
	const struct user_t *user2( (const struct user_t *)elem2 );

	if( user1->tLast < user2->tLast )
		return 1;
	else if( user1->tLast > user2->tLast )
		return -1;
	else
		return 0;
}

int dsortuByWarned( const void *elem1, const void *elem2 )
{
	const struct user_t *user1( (const struct user_t *)elem1 );
	const struct user_t *user2( (const struct user_t *)elem2 );

	if( user1->tWarned < user2->tWarned )
		return 1;
	else if( user1->tWarned > user2->tWarned )
		return -1;
	else
		return 0;
}

int dsortuByShareRatio( const void *elem1, const void *elem2 )
{
	const float fl1( ( (const struct user_t *)elem1 )->flShareRatio );
	const float fl2( ( (const struct user_t *)elem2 )->flShareRatio );
	
	if( ( ( -0.0001 < ( fl1 - fl2 ) ) && ( ( fl1 - fl2 ) < 0.0001 ) ) ) return 0;
	if( ( -1.001 < fl1 ) && ( fl1 < -0.999 ) ) return -1;
	if( ( -1.001 < fl2 ) && ( fl2 < -0.999 ) ) return 1;
	if( ( ( fl1 - fl2 ) < -0.0001 ) )return 1;

	return -1;
}

int dsortuByUpped( const void *elem1, const void *elem2 )
{
	// int64's will overflow, force a compare

	const struct user_t *user1( (const struct user_t *)elem1 );
	const struct user_t *user2( (const struct user_t *)elem2 );

	if( user1->ulUploaded < user2->ulUploaded )
		return 1;
	else if( user1->ulUploaded > user2->ulUploaded )
		return -1;
	else
		return 0;
}

int dsortuByDowned( const void *elem1, const void *elem2 )
{
	// int64's will overflow, force a compare

	const struct user_t *user1( (const struct user_t *)elem1 );
	const struct user_t *user2( (const struct user_t *)elem2 );

	if( user1->ulDownloaded < user2->ulDownloaded )
		return 1;
	else if( user1->ulDownloaded > user2->ulDownloaded )
		return -1;
	else
		return 0;
}

int dsortuByBonus( const void *elem1, const void *elem2 )
{
	// int64's will overflow, force a compare

	const struct user_t *user1( (const struct user_t *)elem1 );
	const struct user_t *user2( (const struct user_t *)elem2 );

	if( user1->ulBonus < user2->ulBonus )
		return 1;
	else if( user1->ulBonus > user2->ulBonus )
		return -1;
	else
		return 0;
}

int dsortuBySeedBonus( const void *elem1, const void *elem2 )
{
	const float fl1( ( (const struct user_t *)elem1 )->flSeedBonus );
	const float fl2( ( (const struct user_t *)elem2 )->flSeedBonus );

	if( ( ( -0.001 < ( fl1 - fl2 ) ) && ( ( fl1 - fl2 ) < 0.001 ) ) ) return 0;
	if( ( -1.001 < fl1 ) && ( fl1 < -0.999 ) ) return -1;
	if( ( -1.001 < fl2 ) && ( fl2 < -0.999 ) ) return 1;
	if( ( ( fl1 - fl2 ) < -0.001 ) )return 1;

	return -1;
}
