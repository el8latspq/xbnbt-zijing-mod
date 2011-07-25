//
// Copyright (C) 2003-2004 Trevor Hogan
//

// =Xotic= Modified Source File

#ifndef TRACKER_H
 #define TRACKER_H

// XBNBT For lauching links in a new browser
#define STR_TARGET_REL string( "external" )
#define STR_TARGET_BLANK string( "_blank" )

// XBNBT HTML 4.01 Strict standard
#define STR_DOC_TYPE string( "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">\n" )

// XBNBT Which CSS entries to use
#define CSS_LOGIN "login"
#define CSS_INDEX "index"
#define CSS_ADMIN "admin"
#define CSS_LOG "log"
#define CSS_SIGNUP "signup"
#define CSS_USERS "users"
#define CSS_INFO "info"
#define CSS_RULES "rules"
#define CSS_FAQ "faq"
#define CSS_STAFF "staff"
#define CSS_RANK "rank"
#define CSS_MESSAGES "messages"
#define CSS_TALK "talk"
#define CSS_STATS "stats"
#define CSS_OFFER "offer"
#define CSS_UPLOAD "upload"
#define CSS_COMMENTS "comments"

// XBNBT Some common Javascript
#define JS_BACK "javascript:history.back();"
#define JS_CHECKLENGTH "javascript:checklength( document.form );"
// #define JS_CHECKLENGTH "javascript: checklength( document.getElementsByTagName( 'form' )[0] );"

// XBNBT Codes for calling the common head and tail of the HTML pages generated
#define IS_INDEX true
#define NOT_INDEX false
#define CODE_200 200
#define CODE_400 400
#define CODE_401 401
#define CODE_403 403
#define CODE_404 404

// XBNBT Server response codes
#define RESPONSE_INIT		0
#define RESPONSE_ANNOUNCE	1
#define RESPONSE_SCRAPE		2
#define RESPONSE_FAVICON	3
#define RESPONSE_CSS		4
#define RESPONSE_RSS		5
#define RESPONSE_XML		6
#define RESPONSE_ROBOTS		7
#define RESPONSE_INDEX		8
#define RESPONSE_STATS		9
#define RESPONSE_LOGIN		10
#define RESPONSE_SIGNUP		11
#define RESPONSE_UPLOAD		12
#define RESPONSE_INFO		13
#define RESPONSE_ADMIN		14
#define RESPONSE_USERS		15
#define RESPONSE_COMMENTS	16
#define RESPONSE_TORRENTS	17
#define RESPONSE_FILES		18
#define RESPONSE_IMAGEFILL	19
#define RESPONSE_IMAGETRANS	20
#define RESPONSE_TAGS		21
#define RESPONSE_LANGUAGE	22
#define RESPONSE_XSTATS		23
#define RESPONSE_XTORRENT	24
#define RESPONSE_BENCODE	25
#define RESPONSE_RSSXSL		26
#define RESPONSE_FAQ		27
#define RESPONSE_SIGNUP_SCHOOL	28
#define RESPONSE_MESSAGES	29
#define RESPONSE_TALK		30
#define RESPONSE_OFFER		31
#define RESPONSE_RULES		32
#define RESPONSE_LOG		33
#define RESPONSE_OFFERS		34
#define RESPONSE_STAFF		35
#define RESPONSE_FILE_UPLOAD	36
#define RESPONSE_SUB_UPLOAD	37
#define RESPONSE_INVITE		38
#define RESPONSE_RECOVER	39
#define RESPONSE_ANNOUNCEMENTS	40
#define RESPONSE_VOTES		41
#define RESPONSE_USERBAR	42
#define RESPONSE_RANK		43
#define RESPONSE_QUERY		44


// Common strings
#define STR_ANNOUNCE string( "announce" )
#define STR_SCRAPE string( "scrape" )
#define STR_TORRENTS string( "torrents" )
#define STR_OFFERS string( "offers" )
#define STR_FILES string( "files" )
#define STR_USERBAR string( "userbar" )
#define LOGIN_HTML string( "login.html" )
#define INDEX_HTML string( "index.html" )
#define STATS_HTML string( "stats.html" )
#define LOG_HTML string( "log.html" )
#define ADMIN_HTML string( "admin.html" )
#define USERS_HTML string( "users.html" )
#define TAGS_HTML string( "tags.html" )
#define LANGUAGE_HTML string( "language.html" )
#define XSTATS_HTML string( "xstats.html" )
#define XTORRENT_HTML string( "xtorrent.html" )
#define OFFER_HTML string( "offer.html" )
#define UPLOAD_HTML string( "upload.html" )
#define FILE_UPLOAD_HTML string( "fileupload.html" )
#define SUB_UPLOAD_HTML string( "subupload.html" )
#define INFO_HTML string( "info.html" )
#define RULES_HTML string( "rules.html" )
#define FAQ_HTML string( "faq.html" )
#define STAFF_HTML string( "staff.html" )
#define RANK_HTML string( "rank.html" )
#define MESSAGES_HTML string( "messages.html" )
#define TALK_HTML string( "talk.html" )
#define COMMENTS_HTML string( "comments.html" )
#define SIGNUP_HTML string( "signup.html" )
#define INVITE_HTML string( "invite.html" )
#define RECOVER_HTML string( "recover.html" )
#define SIGNUP_SCHOOL_HTML string( "signupschool.html" )
#define ANNOUNCEMENTS_HTML string( "announcements.html" )
#define VOTES_HTML string( "votes.html" )
#define ROBOTS_TXT string( "robots.txt" )
#define FAVICON_ICO string( "favicon.ico" )
#define BENCODE_INFO string( "info.bencode" )
#define QUERY_HTML string( "query.html" )
#define RSS_HTML string( "rss.html" )
#define RSS_XSL string( "rss.xsl" )

// Response strings
#define RESPONSE_STR_SEPERATOR string( "/" )
#define RESPONSE_STR_ANNOUNCE RESPONSE_STR_SEPERATOR + STR_ANNOUNCE
#define RESPONSE_STR_SCRAPE RESPONSE_STR_SEPERATOR + STR_SCRAPE
#define RESPONSE_STR_TORRENTS RESPONSE_STR_SEPERATOR + STR_TORRENTS + RESPONSE_STR_SEPERATOR
#define RESPONSE_STR_OFFERS RESPONSE_STR_SEPERATOR + STR_OFFERS + RESPONSE_STR_SEPERATOR
#define RESPONSE_STR_FILES RESPONSE_STR_SEPERATOR + STR_FILES + RESPONSE_STR_SEPERATOR
#define RESPONSE_STR_USERBAR RESPONSE_STR_SEPERATOR + STR_USERBAR + RESPONSE_STR_SEPERATOR
#define RESPONSE_STR_LOGIN_HTML RESPONSE_STR_SEPERATOR + LOGIN_HTML
#define RESPONSE_STR_INDEX_HTML RESPONSE_STR_SEPERATOR + INDEX_HTML
#define RESPONSE_STR_STATS_HTML RESPONSE_STR_SEPERATOR + STATS_HTML
#define RESPONSE_STR_LOG_HTML RESPONSE_STR_SEPERATOR + LOG_HTML
#define RESPONSE_STR_ADMIN_HTML RESPONSE_STR_SEPERATOR + ADMIN_HTML
#define RESPONSE_STR_USERS_HTML RESPONSE_STR_SEPERATOR + USERS_HTML
#define RESPONSE_STR_TAGS_HTML RESPONSE_STR_SEPERATOR + TAGS_HTML
#define RESPONSE_STR_LANGUAGE_HTML RESPONSE_STR_SEPERATOR + LANGUAGE_HTML
#define RESPONSE_STR_XSTATS_HTML RESPONSE_STR_SEPERATOR + XSTATS_HTML
#define RESPONSE_STR_XTORRENT_HTML RESPONSE_STR_SEPERATOR + XTORRENT_HTML
#define RESPONSE_STR_OFFER_HTML RESPONSE_STR_SEPERATOR + OFFER_HTML
#define RESPONSE_STR_UPLOAD_HTML RESPONSE_STR_SEPERATOR + UPLOAD_HTML
#define RESPONSE_STR_FILE_UPLOAD_HTML RESPONSE_STR_SEPERATOR + FILE_UPLOAD_HTML
#define RESPONSE_STR_SUB_UPLOAD_HTML RESPONSE_STR_SEPERATOR + SUB_UPLOAD_HTML
#define RESPONSE_STR_INFO_HTML RESPONSE_STR_SEPERATOR + INFO_HTML
#define RESPONSE_STR_RULES_HTML RESPONSE_STR_SEPERATOR + RULES_HTML
#define RESPONSE_STR_FAQ_HTML RESPONSE_STR_SEPERATOR + FAQ_HTML
#define RESPONSE_STR_STAFF_HTML RESPONSE_STR_SEPERATOR + STAFF_HTML
#define RESPONSE_STR_RANK_HTML RESPONSE_STR_SEPERATOR + RANK_HTML
#define RESPONSE_STR_MESSAGES_HTML RESPONSE_STR_SEPERATOR + MESSAGES_HTML
#define RESPONSE_STR_TALK_HTML RESPONSE_STR_SEPERATOR + TALK_HTML
#define RESPONSE_STR_COMMENTS_HTML RESPONSE_STR_SEPERATOR + COMMENTS_HTML
#define RESPONSE_STR_SIGNUP_HTML RESPONSE_STR_SEPERATOR + SIGNUP_HTML
#define RESPONSE_STR_INVITE_HTML RESPONSE_STR_SEPERATOR + INVITE_HTML
#define RESPONSE_STR_RECOVER_HTML RESPONSE_STR_SEPERATOR + RECOVER_HTML
#define RESPONSE_STR_SIGNUP_SCHOOL_HTML RESPONSE_STR_SEPERATOR + SIGNUP_SCHOOL_HTML
#define RESPONSE_STR_ANNOUNCEMENTS_HTML RESPONSE_STR_SEPERATOR + ANNOUNCEMENTS_HTML
#define RESPONSE_STR_VOTES_HTML RESPONSE_STR_SEPERATOR + VOTES_HTML
#define RESPONSE_STR_ROBOTS_TXT RESPONSE_STR_SEPERATOR + ROBOTS_TXT
#define RESPONSE_STR_FAVICON_ICO RESPONSE_STR_SEPERATOR + FAVICON_ICO
#define RESPONSE_STR_BENCODE_INFO RESPONSE_STR_SEPERATOR + BENCODE_INFO
#define RESPONSE_STR_QUERY_HTML RESPONSE_STR_SEPERATOR + QUERY_HTML
#define RESPONSE_STR_RSS_HTML RESPONSE_STR_SEPERATOR + RSS_HTML
#define RESPONSE_STR_RSS_XSL RESPONSE_STR_SEPERATOR + RSS_XSL

// Button strings
#define STR_SUBMIT "Submit"

// XBNBT Dynstat
// Which file format?
#define PNG_FORMAT	0
#define JPG_FORMAT	1
#define GIF_FORMAT	2
#define UNK_FORMAT	3
// Which image file naming system?
#define IMAGE_BY_INFOHASH 0
#define IMAGE_BY_FILENAME 1
// Which image error was returned?
#define IMAGE_ERROR_OK 0
#define IMAGE_ERROR_NAME 1
#define IMAGE_ERROR_TYPE 2
#define IMAGE_ERROR_EXIST 3

// XBNBT Which validator display mode?
#define VALID_ADMIN	0
#define VALID_TEXT	1
#define VALID_IMAGE 2

// XBNBT Which mySQL users integration mode?
#define ORIGINAL_MODE 0
#define VBULLITEN3_MODE 1
#define IPB2_MODE 2

// XBNBT CBTT Which IP banning mode?
#define IP_BANNING_OFF 0
#define IP_BLACKLIST 1
#define IP_VIPLIST 2

// XBNBT CBTT Which ID/User-Agent banning mode?
#define ID_BANNING_OFF 0
#define ID_BLACKLIST 1
#define ID_VIPLIST 2

// XBNBT For controlling search results
// How many results found?
#define RESULTS_ZERO 0
#define RESULTS_ONE 1
// How many per page results?
#define DISPLAY_ALL 0

// Show peer info
#define SHOW_NO_INFO 0
#define SHOW_USER_AGENT 1
#define SHOW_PEER_ID 2

// Announce event string
#define EVENT_STR_STARTED "started"
#define EVENT_STR_COMPLETED "completed"
#define EVENT_STR_STOPPED "stopped"

// Announce event codes
#define EVENT_UNKNOWN 0
#define EVENT_UPDATE 1
#define EVENT_STARTED 2
#define EVENT_COMPLETED 3
#define EVENT_STOPPED 4

#define USER_STATUS_NONE 0
#define USER_STATUS_SHARERATIO 1
#define USER_STATUS_WARNED 2

#define SET_SEEDER_ADD 9
#define SET_SEEDER_ADD_V6 10
#define SET_SEEDER_ADD_BOTH 11
#define SET_SEEDER_MINUS 3
#define SET_SEEDER_MINUS_BOTH 2
#define SET_LEECHER_ADD 6
#define SET_LEECHER_ADD_V6 7
#define SET_LEECHER_ADD_BOTH 8
#define SET_LEECHER_MINUS 1
#define SET_LEECHER_MINUS_BOTH 0
#define SET_SEEDER_A_LEECHER_M 4
#define SET_SEEDER_A_LEECHER_M_BOTH 5

#define SET_USER_SEEDING_ADD 4
#define SET_USER_SEEDING_MINUS 1
#define SET_USER_LEECHING_ADD 3
#define SET_USER_LEECHING_MINUS 0
#define SET_USER_SEEDING_A_LEECHING_M 2

#define SET_COMPLETED_ADD 2
#define SET_COMPLETED_MINUS 1

#define SET_STATUS_REQ 1
#define SET_STATUS_NOREQ 0

#define SET_SEEDED_SEEDED 1
#define SET_SEEDED_UNSEEDED 0

#define SET_COMMENT_ADD 2
#define SET_COMMENT_MINUS 1
#define SET_COMMENT_CLEAR 0

#define SET_THANKS_ADD 2
#define SET_THANKS_MINUS 1

#define SET_SHARES_ADD 2
#define SET_SHARES_MINUS 1

#define SET_SUBS_ADD 2
#define SET_SUBS_MINUS 1

#define MATCH_METHOD_NONCASE_AND 0
#define MATCH_METHOD_NONCASE_OR 1
#define MATCH_METHOD_NONCASE_EQ 2
#define MATCH_METHOD_AND 3
#define MATCH_METHOD_OR 4
#define MATCH_METHOD_EQ 5

// XBNBT Structure for holding file details and data
struct localfile_t
{
	string strName;
	string strExt;
	string strDir;
	string strFile;
	string strURL;
};

// XBNBT Structure for holding text in Dynstat
struct dynstat_t
{
	string strTitle;
	string strSize;
	string strFiles;
	string strTag;
	string strHash;
	string strFilename;
	string strSeeders;
	string strLeechers;
	string strStats;
	string strPeers;
	string strLogo;
	string strImageFileName;
	string strImageFullPath;
};

// XBNBT Structure for holding announce stats
struct announcestats_t
{
	int64 iAnnounce;
	int64 iAnnMissing;
	int64 iAnnNotAuth;
	int64 iCompact;
	int64 iNopeerid;
	int64 iRegular;
	int64 iIPBanned;
	int64 iIPNotCleared;
	int64 iPeerSpoofRestrict;
	int64 iClientBanned;
	int64 iInvalidEvent;
	int64 iPortMissing;
	int64 iUploadedMissing;
	int64 iDownloadedMissing;
	int64 iLeftMissing;
	int64 iPeerIDLength;
	int64 iPortBlacklisted;
	int64 iDownloadedInvalid;
	int64 iUploadedInvalid;
	int64 iNotAuthorized;
	string sLastReset;
};

// XBNBT Structure for holding scrape stats
struct scrapestats_t
{
	int64 iScrape;
	int64 iDisallowed;
	int64 iSingle;
	int64 iMultiple;
	int64 iFull;
	int64 iNotAuthorized;
	string sLastReset;
};

// XBNBT Structure for holding Dynstat stats
struct dynstatstats_t
{
	int64 iRun;
	int64 iFrozen;
	int64 iProcessed;
	int64 iTotalProcessed;
	string sLastRunTime;
	string sElapsedTime;
	string sLastReset;
};

// XBNBT Structure for holding served file stats
struct filestats_t
{
	int64 iFavicon;
	int64 iCSS;
	int64 iRSS;
	int64 iRSSXSL;
	int64 iXML;
	int64 iRobots;
	int64 uiFiles;
	int64 iBarFill;
	int64 iBarTrans;
	int64 iTorrent;
	string sLastReset;
};

// XBNBT Structure for holding page hit count stats
struct pagestats_t
{
	int64 iIndex;
	int64 iStats;
	int64 iLogin;
	int64 iSignup;
	int64 iOffer;
	int64 iUpload;
	int64 iInfo;
	int64 iRules;
	int64 iFAQ;
	int64 iMessages;
	int64 iAdmin;
	int64 iUsers;
	int64 uiComments;
	int64 iTorrents;
	int64 iTags;
	int64 iLanguage;
	int64 iXStats;
	int64 iXTorrent;
	int64 iError404;
	string sLastReset;
};

// XBNBT Structure for holding xtorrent configuration
struct xtorrent_t
{	
	bool bForceAnnounceDL;
	bool bForceAnnounceUL;
	bool bEnableAnnounceList;
};

// XBNBT Structure for holding date stats
struct datestats_t
{
	string sLinkEstablished;
	string sHubLinkEstablished;
	string sRSSPublish;
	string sXMLPublish;
};

// XBNBT Structure for holding TCP stats
struct tcpstats_t
{
	int64 iRecv;
	int64 iSend;
	int64 iRecvHub;
	int64 iSendHub;
	int64 iRecvLink;
	int64 iSendLink;
	string sLastReset;
};

// XBNBT Structure for holding peer stats
struct peerstats_t
{
	int64 iGreatest;
	int64 iGreatestSeeds;
	int64 iGreatestLeechers;
	int64 iGreatestUnique;
	string sLastReset;
};

// XBNBT Structure for holding all system stats
struct xbnbtstats_t
{
	// dynstat
	struct dynstatstats_t dynstat;
	// announce
	struct announcestats_t announce;
	// scrape
	struct scrapestats_t scrape;
	// files served
	struct filestats_t file;
	// html pages
	struct pagestats_t page;
	// date stamps
	struct datestats_t date;
	// TCP stats
	struct tcpstats_t tcp;
	// peer stats
	struct peerstats_t peer;
};

// Structure for holding announces
struct announce_t
{
//	string strInfoHash;
	string strID;
	string strIP;
	string strEvent;
	unsigned int uiPort;
	int64 iUploaded;
	int64 iDownloaded;
	int64 iLeft;
	string strPeerID;
	string strKey;
	string strUserAgent;
	string strUID;
	string strUsername;
};

// Structure for holding torrents
struct torrent_t
{
	string strInfoHash;
	string strName;
	string strLowerName;
	string strFileName;
	string strAdded;
	string strID;
	int uiSeeders;
	int uiSeeders6;
	int uiLeechers;
	int uiLeechers6;
	int64 ulCompleted;
//	int64 iTransferred;
	int64 iSize;
	unsigned int uiFiles;
	int uiComments;
	int uiThanks;
	int uiShares;
	int uiSubs;
// 	int64 iAverageLeft;
// 	unsigned char ucAverageLeftPercent;
// 	int64 iMinLeft;
// 	int64 iMaxiLeft;
	bool bPost;
	bool bAllow;
	bool bNoComment;
	unsigned char ucTop;
	unsigned char ucClassic;
//	bool bHL;
//	bool bClassic;
	bool bReq;
	int iFreeDown;
	int iFreeUp;
	int iTimeDown;
	int iTimeUp;
	int64 iFreeTo;
	int iDefaultDown;
	int iDefaultUp;
	string strTag;
	string strUploader;
	string strUploaderID;
	string strIMDb;
	string strIMDbID;
	string strIP;
// 	string strIgnore;
// 	string strIgnored;
};

// Structure for holding peers
struct peer_t
{
	string strUsername;
	string strUID;
	string strIP;
	string strIP6;
	int64 iUpped;
	int64 iDowned;
	int64 iLeft;
	unsigned long ulConnected;
	unsigned long ulUpdated;
	int64 ulUpSpeed;
	int64 ulDownSpeed;
	float flShareRatio;
	string strPeerID;
	string strKey;
	string strUserAgent;
	string strClientType;
	bool bClientTypeIdentified;
};

// The maximum file name length on upload
#define MAX_FILENAME_LEN	256
// The maximum info link length on upload
#define MAX_INFO_LINK_LEN	256

// Get a file extension
static inline const string getFileExt( const string &cstrFilename )
{
	string strExt = string( );

	if( !cstrFilename.empty( ) )
	{
		const string :: size_type ciExt( cstrFilename.rfind( "." ) );

		if( ciExt != string :: npos )
			strExt = cstrFilename.substr( ciExt );
	}

	return strExt;
};

//
// CTracker
//

class CTracker
{
public:
	CTracker( );
	virtual ~CTracker( );
	void loadAccess( );
	void sendMessage( const string &strLogin, const string &strUID, const string &strSendToID, const string &strIP, const string &strTitle, const string &strMessage, const bool bSaveSent = false );

// 	void saveRSS( const string &strChannelTag = string( ) );	
	
	void expireDownloaders( );
	void parseTorrents( const char *szFile );
	const string parseTorrent( const char *szDir );
	const bool checkTag( const string &strTag );
	const string addTag( const string &strInfoHash, const string &strTag, const string &strName, const string &strIntr, const string &strUploader, const string &strUploaderID, const string &strIP, const string &strDefaultDown, const string &strDefaultUp, const string &strFreeDown, const string &strFreeUp, const string &strFreeTime, const string &strComments, const bool bFromNow, const bool bOffer );
	void modifyTag( const string &strID, const string &strTag, const string &strName, const string &strIntr, const string &strUploader, const string &strUploaderID, const string &strIP, const string &strDefaultDown, const string &strDefaultUp, const string &strFreeDown, const string &strFreeUp, const string &strFreeTime, const string &strComments, const bool bFromNow, const bool bOffer );
	void deleteTag( const string &strInfoHash, const bool bOffer );
	void addBonus( const string &strID, const string &strUID );
	const string checkUserMD5( const string &strUID, const string &cstrMD5 );
	user_t checkUser( const string &strLogin, const string &cstrMD5 );
	user_t getUser( const string &strUID, const string &strMyUID, const unsigned char ucAccess );
	const string getUserLogin( const string &strLogin );
	const string addUser( const string &strLogin, const string &strPass, const unsigned char ucAccess, const string &strMail );
//	const bool checkWarned( const string &strUID );
	const bool checkShareRatio( int64 iDownloaded, float flShareRatio );
//	const unsigned char checkUserStatus( const string &strUID );
	const string getUserLink( const string &strUID, const string &strUsername );
	const string getUserLinkFull( const string &strUID, const string &strUsername );
	const string getUserLinkTalk( const string &strUID, const string &strUsername );
	const string TransferMentions( const string &cstrTalk, const string &cstrTalkID );
	const string GenerateTalk( const vector<string> &vecQuery, const unsigned char cucAccess, const string &cstrUID, const string &cstrTextareaID, const string &cstrJoined, bool bTalker = true, bool bFunc = true, bool bHistory = false, bool bReplys = false );
	void InitPasskey( const string &strLogin );
	void deleteUser( const string &strUID );
	void CountUniquePeers( );
	void AddUniquePeer( const string &strIP );
	void RemoveUniquePeer( const string &strIP );
	const string GetIMDb( const string &cstrIMDbID );
	void GetIMDbLoop( );
	void CBTTParseList( );
	void RefreshStatic( );
	void Announce( const struct announce_t &ann, bool &bRespond );
	void RefreshFastCache( );
	void UpdateUserState( );

	void serverResponseGET( struct request_t *pRequest, struct response_t *pResponse );
	void serverResponsePOST( struct request_t *pRequest, struct response_t *pResponse, CAtomList *pPost );
	void serverResponseIndex( struct request_t *pRequest, struct response_t *pResponse );
	void serverResponseAnnounce( struct request_t *pRequest, struct response_t *pResponse );
	void serverResponseScrape( struct request_t *pRequest, struct response_t *pResponse );
	void serverResponseStatsGET( struct request_t *pRequest, struct response_t *pResponse );
	void serverResponseStatsPOST( struct request_t *pRequest, struct response_t *pResponse, CAtomList *pPost );
	void serverResponseTorrent( struct request_t *pRequest, struct response_t *pResponse );
	void serverResponseOffer( struct request_t *pRequest, struct response_t *pResponse );
	void serverResponseFile( struct request_t *pRequest, struct response_t *pResponse );
	void serverResponseUserbar( struct request_t *pRequest, struct response_t *pResponse );


	void serverResponseRobots( struct request_t *pRequest, struct response_t *pResponse );
	void serverResponseLoginGET( struct request_t *pRequest, struct response_t *pResponse );
	void serverResponseLoginPOST( struct request_t *pRequest, struct response_t *pResponse, CAtomList *pPost );
	void serverResponseSignupGET( struct request_t *pRequest, struct response_t *pResponse );
	void serverResponseSignupPOST( struct request_t *pRequest, struct response_t *pResponse, CAtomList *pPost );
	void serverResponseInviteGET( struct request_t *pRequest, struct response_t *pResponse );
	void serverResponseInvitePOST( struct request_t *pRequest, struct response_t *pResponse, CAtomList *pPost );
	void serverResponseRecoverGET( struct request_t *pRequest, struct response_t *pResponse );
	void serverResponseRecoverPOST( struct request_t *pRequest, struct response_t *pResponse, CAtomList *pPost );
	void serverResponseSignupSchoolGET( struct request_t *pRequest, struct response_t *pResponse );
	void serverResponseSignupSchoolPOST( struct request_t *pRequest, struct response_t *pResponse, CAtomList *pPost );
	void serverResponseOfferGET( struct request_t *pRequest, struct response_t *pResponse );
	void serverResponseUploadGET( struct request_t *pRequest, struct response_t *pResponse );
	void serverResponseUploadPOST( struct request_t *pRequest, struct response_t *pResponse, CAtomList *pPost );
	void serverResponseFileUploadGET( struct request_t *pRequest, struct response_t *pResponse );
	void serverResponseSubUploadGET( struct request_t *pRequest, struct response_t *pResponse );
	void serverResponseInfoGET( struct request_t *pRequest, struct response_t *pResponse );
//	void serverResponseInfoPOST( struct request_t *pRequest, struct response_t *pResponse, CAtomList *pPost );
	void serverResponseRulesGET( struct request_t *pRequest, struct response_t *pResponse );
	void serverResponseRulesPOST( struct request_t *pRequest, struct response_t *pResponse, CAtomList *pPost );
	void serverResponseFAQGET( struct request_t *pRequest, struct response_t *pResponse );
	void serverResponseFAQPOST( struct request_t *pRequest, struct response_t *pResponse, CAtomList *pPost );
	void serverResponseStaff( struct request_t *pRequest, struct response_t *pResponse );
	void serverResponseRank( struct request_t *pRequest, struct response_t *pResponse );
	void serverResponseMessagesGET( struct request_t *pRequest, struct response_t *pResponse );
	void serverResponseMessagesPOST( struct request_t *pRequest, struct response_t *pResponse, CAtomList *pPost );
	void serverResponseLog( struct request_t *pRequest, struct response_t *pResponse );
	void serverResponseAdmin( struct request_t *pRequest, struct response_t *pResponse );
	void serverResponseUsersGET( struct request_t *pRequest, struct response_t *pResponse );
	void serverResponseUsersPOST( struct request_t *pRequest, struct response_t *pResponse, CAtomList *pPost );
	void serverResponseCommentsGET( struct request_t *pRequest, struct response_t *pResponse );
	void serverResponseCommentsPOST( struct request_t *pRequest, struct response_t *pResponse, CAtomList *pPost );
	void serverResponseTalkGET( struct request_t *pRequest, struct response_t *pResponse );
	void serverResponseTalkPOST( struct request_t *pRequest, struct response_t *pResponse, CAtomList *pPost );
	void serverResponseAnnouncementsGET( struct request_t *pRequest, struct response_t *pResponse );
	void serverResponseAnnouncementsPOST( struct request_t *pRequest, struct response_t *pResponse, CAtomList *pPost );
	void serverResponseVotesGET( struct request_t *pRequest, struct response_t *pResponse );
	void serverResponseVotesPOST( struct request_t *pRequest, struct response_t *pResponse, CAtomList *pPost );
	void serverResponseXML( struct request_t *pRequest, struct response_t *pResponse );
	void serverResponseQuery( struct request_t *pRequest, struct response_t *pResponse );
	void serverResponseRSS( struct request_t *pRequest, struct response_t *pResponse );
	void serverResponseRSSXSL( struct request_t *pRequest, struct response_t *pResponse );
	void serverResponseCSS( struct request_t *pRequest, struct response_t *pResponse );
	void serverResponseImagefill( struct request_t *pRequest, struct response_t *pResponse );
	void serverResponseImagetrans( struct request_t *pRequest, struct response_t *pResponse );
	void serverResponseLanguage( struct request_t *pRequest, struct response_t *pResponse );
	void serverResponseTags( struct request_t *pRequest, struct response_t *pResponse );
	void serverResponseXStats( struct request_t *pRequest, struct response_t *pResponse );
	void serverResponseXTorrent( struct request_t *pRequest, struct response_t *pResponse );
	void serverResponseIcon( struct request_t *pRequest, struct response_t *pResponse );
	void serverResponseBencodeInfo( struct request_t *pRequest, struct response_t *pResponse );

	// The main update routine
	void Update( );

	//
	// XBNBT Main
	//

	// XBNBT Save files
// 	void saveXML( );
	void saveXStats( );

	// Queued announces for tracker links
	void QueueAnnounce( const struct announce_t &ann );

	// XBNBT IP Banning
	const bool IsIPBanned( struct request_t *pRequest, struct response_t *pResponse, const struct bnbttv &btv, const string &cstrPageLabel, const string &cstrCSSLabel, const bool &bIsIndex );

	// XBNBT Internal mouseovers
	void initTags( );
	void initShareRatio( );

	// XBNBT HTML 4.01 Strict
	void HTML_Nav_Bar( struct request_t *pRequest, struct response_t *pResponse, const string &cstrCSS, bool bNewIndex, bool bNewOffer );
	void HTML_Common_Begin( struct request_t *pRequest, struct response_t *pResponse, const string &cstrTitle, const string &cstrCSS, const string &cstrUrl, const bool &bIndex, const unsigned int &cuiCode );
	void HTML_Common_End( struct request_t *pRequest, struct response_t *pResponse, const struct bnbttv &btv, const bool &bIndex, const string &cstrCSS );
	// Javascript return to page routine
	void JS_ReturnToPage( struct request_t *pRequest, struct response_t *pResponse, const string &cstrPageParameters );
	// XStats - HTML tables
	void tableXStatsAnnounce( struct request_t *pRequest, struct response_t *pResponse );
	void tableXStatsScrape( struct request_t *pRequest, struct response_t *pResponse );
	void tableXStatsFile( struct request_t *pRequest, struct response_t *pResponse );
	void tableXStatsPage( struct request_t *pRequest, struct response_t *pResponse );
	void tableXStatsDate( struct request_t *pRequest, struct response_t *pResponse );
	void tableXStatsTCP( struct request_t *pRequest, struct response_t *pResponse );
	void tableXStatsPeer( struct request_t *pRequest, struct response_t *pResponse );
	
	// XBNBT XStats for reseting statistics
	void resetXStatsAnnounce( );
	void resetXStatsScrape( );
	void resetXStatsFile( );
	void resetXStatsPage( );
	void resetXStatsTCP( );
	void resetXStatsPeers( );
	void resetXStatsAll( );
	
#if defined ( XBNBT_GD )
	// XBNBT Dynstat for generating forum friendy images for a torrents peer and seed information
	void runGenerateDynstat( );

	// XBNBT Dynstat for reseting XStats statistics
	void resetXStatsDynstat( );

	// XBNBT Dynstat for XStats - HTML tables
	void tableXStatsDynstat( struct request_t *pRequest, struct response_t *pResponse );
#endif

private:
	string m_strAllowedDir;
	string m_strOfferDir;
	string m_strRulesDir;
	string m_strFAQDir;
	string m_strArchiveDir;
	string m_strFileDir;
	string m_strStaticHeaderFile;
	string m_strStaticHeader;
	string m_strStaticFooterFile;
	string m_strStaticFooter;

	// RSS (thanks labarks)
	unsigned char m_ucDumpRSSFileMode;
	unsigned int m_uiDumpRSSttl;
	string m_strDumpRSScategory;
	string m_strDumpRSSurl;
	unsigned int m_uiDumpRSSwidth;
	unsigned int m_uiDumpRSSheight;
	string m_strDumpRSScopyright;
	string m_strDumpRSSmanagingEditor;
	unsigned int m_uiDumpRSSLimit;

	string m_strForceAnnounceURL;
	bool m_bForceAnnounceOnDL;
	unsigned int m_uiParseAllowedInterval;
	unsigned int m_uiGetSeedBonusInterval;
	unsigned int m_uiDownloaderTimeOutInterval;
	unsigned int m_uiRefreshConfigInterval;
	unsigned int m_uiRefreshCBTTListInterval;
	unsigned int m_uiRefreshIMDbInterval;
	unsigned int m_uiRefreshStaticInterval;
	unsigned int m_uiTimerInterval;	  
	unsigned int m_uiRefreshFastCacheInterval;
	unsigned long m_ulGetSeedBonusNext;
	unsigned long m_ulDownloaderTimeOutNext;
	unsigned long m_ulRefreshConfigNext;
	unsigned long m_ulRefreshCBTTListNext;
	unsigned long m_ulRefreshIMDbNext;
	unsigned long m_ulRefreshStaticNext;
	unsigned long m_ulTimerNext;
	unsigned long m_ulRefreshFastCacheNext;
	unsigned int m_uiAnnounceInterval;
	unsigned int m_uiMinRequestInterval;
	unsigned int m_uiResponseSize;
	unsigned int m_uiMaxGive;
	bool m_bRatioRestrict;
	bool m_bAllowScrape;
	bool m_bCountUniquePeers;
	bool m_bDeleteInvalid;
	bool m_bParseOnStart;
	bool m_bParseOnUpload;
	unsigned int m_uiMaxTorrents;
	bool m_bShowStats;
	bool m_bAllowTorrentDownloads;
	bool m_bAllowComments;
	bool m_bShowAdded;
	bool m_bShowAdded_Index;
	bool m_bShowSize;
	bool m_bShowNumFiles;
	bool m_bShowCompleted;
	bool m_bShowTransferred;
	bool m_bShowMinLeft;
	bool m_bShowAverageLeft;
	bool m_bShowMaxiLeft;
	bool m_bShowLeftAsProgress;
	bool m_bShowUploader;
	bool m_bSearch;
	bool m_bSort;
	bool m_bShowFileComment;
	bool m_bShowFileContents;
	bool m_bShowShareRatios;
	bool m_bShowAvgDLRate;
	bool m_bShowAvgULRate;
	bool m_bDeleteOwnTorrents;
	bool m_bGen;
	unsigned int m_uiPerPage;
	unsigned int m_uiPerPageMax;
	unsigned int m_uiUsersPerPage;
	unsigned int m_uiMaxPeersDisplay;
	unsigned char m_ucGuestAccess;
	unsigned char m_ucMemberAccess;
	unsigned int m_uiFileExpires;
	unsigned int m_uiTorrentExpires;
	unsigned int m_uiNameLength;
	unsigned int m_uiCommentLength;
	unsigned int m_uiMessageLength;
	unsigned int m_uiTalkLength;
	
	unsigned char m_ucAccessDownAnnounce;
	unsigned char m_ucAccessRSS;
	unsigned char m_ucAccessDumpXML;
	unsigned char m_ucAccessSortIP;
	unsigned char m_ucAccessShowIP;
	unsigned char m_ucAccessComments;
	unsigned char m_ucAccessCommentsAlways;
	unsigned char m_ucAccessEditComments;
	unsigned char m_ucAccessDelComments;
	unsigned char m_ucAccessCommentsToMessage;
	unsigned char m_ucAccessView;
	unsigned char m_ucAccessViewTorrents;
	unsigned char m_ucAccessDownTorrents;
	unsigned char m_ucAccessUploadTorrents;
	unsigned char m_ucAccessUploadPosts;
	unsigned char m_ucAccessReq;
	unsigned char m_ucAccessBookmark;
	unsigned char m_ucAccessEditTorrents;
	unsigned char m_ucAccessDelTorrents;
	unsigned char m_ucAccessViewOffers;
	unsigned char m_ucAccessAllowOffers;
	unsigned char m_ucAccessEditOffers;
	unsigned char m_ucAccessDelOffers;
	unsigned char m_ucAccessUploadOffers;
	unsigned char m_ucAccessViewStats;
	unsigned char m_ucAccessEditOwn;
	unsigned char m_ucAccessDelOwn;
	unsigned char m_ucAccessCreateUsers;
	unsigned char m_ucAccessEditUsers;
	unsigned char m_ucAccessDelUsers;
	unsigned char m_ucAccessEditAdmins;
	unsigned char m_ucAccessInvites;
	unsigned char m_ucAccessMessages;
	unsigned char m_ucAccessSignup;
	unsigned char m_ucAccessSignupDirect;
	unsigned char m_ucAccessViewLog;
	unsigned char m_ucAccessAdmin;
	unsigned char m_ucAccessViewUsers;
	unsigned char m_ucAccessUserDetails;
	unsigned char m_ucAccessViewXStates;
	
//	CAtomDicti *m_pCached;		// self.cached
	CCache *m_pCache;

	// Standard Tags
	vector< pair< string, string > > m_vecTags;
	vector< pair< string, string > > m_vecTagsMouse;
	
	vector< string > m_vecQualities;
	vector< string > m_vecMediums;
	vector< string > m_vecEncodes;
	
	int RequiredDown[6];
	float RequiredRatio[6];

	//
	// XBNBT Main
	//
	
	// CBTT - Declares new variables for the use with CBTT features
	string m_strClientBanFile;
	unsigned char m_ucBanMode;
	bool m_bPeerSpoofRestrict;
	bool m_bDontCompressTorrent;
	string m_strOverFlowLimit;
	bool m_bRestrictOverflow;
	unsigned char m_ucIPBanMode;
	string m_strIPBanFile;
	bool m_bBlockNATedIP;
	bool m_bLocalOnly;
	bool m_bBlacklistP2PPorts;
	unsigned char m_ucBlockSearchRobots;
	CAtomList *m_pClientBannedList;
	CAtomList *m_pIPBannedList;
	unsigned char m_ucPrivateTracker;

	// XBNBT Specific
	bool m_bBoot;
	bool m_bUsersOnline;
	unsigned char m_ucNavbar;
	unsigned char m_ucShowPeerInfo;
	unsigned char m_ucShowValidator;
	string m_strRSSValidImage;
	bool m_bFlagRSSAlert;
	bool m_bFlagXMLAlert;
	bool m_bFlagNotOwnLinkAlert;
	bool m_bFlagNotOwnHUBLinkAlert;
	bool m_bStaticAll;
	bool m_bShowIP;
	unsigned char m_ucAuthAnnounceAccess;
	unsigned char m_ucAuthScrapeAccess;
	unsigned char m_ucInfoAccess;
	string m_strCustomAnnounce;
	string m_strCustomScrape;
	unsigned long m_ulSeedersTotal;
	unsigned long m_ulLeechersTotal;
	unsigned long m_ulPeers;
	bool m_bEnableAnnounceList;
	bool m_bForceAnnounceOnUL;
	unsigned int m_uiPort;
	string m_strTrackerFQDN;
	string m_strTrackerURL;
	string m_strDescription;
	string m_strTitle;
	string m_strSubTitle;
	string m_strKeywords;
	string m_strLanguage;
	bool m_bUseButtons;
	string m_strRating;
	string m_strWebmaster;

	// static messages
	string m_strValidate1;
	string m_strValidate2;
	string m_strJSReduce;
	string m_strJSMsgReduce;
	string m_strJSLength;
	string m_strJSMsgLength;
	string m_strJSTalkLength;
	string m_strRSSLocalLink;

	// queued announces
	CMutex m_mtxQueued;
	vector<struct announce_t> m_vecQueued;

	// XML
	unsigned int m_uiDumpXMLInterval;	   
	unsigned long m_ulDumpXMLNext;  
	bool m_bDumpXMLPeers;

	// Serve Local
	bool m_bServeLocal;
	// robots.txt
	struct localfile_t robots;
	// favicon.ico
	struct localfile_t favicon;
	// CSS
	struct localfile_t style;
	// XML
	struct localfile_t xmldump;
	// RSS
	struct localfile_t rssdump;
	struct localfile_t rssxsl;
	// image bar fill
	struct localfile_t imagefill;
	// image bar trans
	struct localfile_t imagetrans;
	// userbar.png
	struct localfile_t userbar;

	// XBNBT Stats
	struct localfile_t statsdump;
	CAtomDicti *m_pXStats;
	unsigned int m_uiXStatsInterval;	   
	unsigned long m_ulXStatsNext;  	

	// xtorrent settings
	struct xtorrent_t xtorrent;

	// Announce 'key' support
	bool m_bAnnounceKeySupport;

#if defined ( XBNBT_GD )
	// XBNBT Dynstat
	CAtomDicti *m_pDynstat;
	string m_strDynstatFile;
	bool m_bDynstatGenerate;
	unsigned char m_ucDynstatSaveMode;
	bool m_bDynstatShowLink;
	unsigned int m_uiDynstatDumpInterval;
	unsigned int m_uiDynstatBackground;
	unsigned int m_uiDynstatXSize;
	unsigned int m_uiDynstatYSize;

	unsigned char m_ucDynstatFontRed;
	unsigned char m_ucDynstatFontGreen;
	unsigned char m_ucDynstatFontBlue;
	char m_cDynstatPNGCompress;
	char m_cDynstatJPGQuality;
	unsigned char m_ucDynstatOutType;
	string m_strDynstatDumpFileDir;
	unsigned long m_ulDynstatDumpNext;
	string m_strDynstatFontFile;
	string m_strDynstatLinkURL;
	bool m_bDynstatSkin;
	string m_strDynstatSkinFile;
	string m_strImageOutExt;
	unsigned char m_ucDynstatInType;
#endif

};

class CCache
{
public:
	CCache( );
	virtual ~CCache( );

	void Reset( bool bOffer = false );
//	void addRow( const string strID, bool bOffer );
	void sort( const unsigned char cucSort, bool bNoTop, bool bOffer = false );
	void sortUsers( const unsigned char cucSort );
	void setRow( const string &cstrID, bool bOffer );
	void setActive( const string &cstrID, const unsigned char cucOpt );
	void setCompleted( const string &cstrID, const unsigned char cucOpt );
	void setComment( const string &cstrID, const unsigned char cucOpt, bool bOffer );
	void setThanks( const string &cstrID, const unsigned char cucOpt );
	void setShares( const string &cstrID, const unsigned char cucOpt );
	void setSubs( const string &cstrID, const unsigned char cucOpt );
	void setStatus( const string &cstrID, const unsigned char cucOpt );
	void setSeeded( const string &cstrID, const unsigned char cucOpt );
	void setFree( );
	void setLatest( const string &cstrAdded, bool bOffer = false );
	time_t getLatest( bool bOffer = false );
	unsigned long getSize( bool bOffer = false );
	struct torrent_t *getCache( bool bOffer = false );
	
	void ResetUsers( );
	void setRowUsers( const string &cstrUID );
	void setUserData( const string &cstrUID, int64 iUploaded, int64 iDownloaded, int64 iBonus );
//	void setUserStatus( const string &cstrUID, const unsigned char cucOpt );
	void setLast( const string &cstrUID );
	unsigned long getSizeUsers( );
	struct user_t *getCacheUsers( );

private:
	void resetCache( bool bOffer = false );
	void resetCacheUsers( );
	
	bool bReset;
	bool bResetOffers;
	bool bResetUsers;
	bool bResort;
	bool bResortOffers;
	bool bResortUsers;
	bool bSortNoTop;
	time_t tLatest;
	time_t tLatestOffer;
	unsigned char ucSort;
	unsigned char ucSortOffers;
	unsigned char ucSortUsers;
	unsigned long ulSize;
	unsigned long ulSizeOffers;
	unsigned long ulSizeUsers;
	struct torrent_t *pTorrents;
	struct torrent_t *pOffers;
	struct user_t *pUsers;
};

#endif
