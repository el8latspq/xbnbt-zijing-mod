SHELL = /bin/sh
SYSTEM = $(shell uname)
C++ = g++

# Debug Flags
DFLAGS =
# Warnings Flags (GCC)
WFLAGS = -w
# Warnings Pre Processor Flags (G++)
WPPFLAGS = 
# Remove the -O3 or reduce to -O2,-O1 or -O if anything fails to compile, i.e. tracker_index.html
OFLAGS =
# Library paths for linker 
LFLAGS = -L. -lz  -L/usr/local/lib/mysql/ -L/usr/lib/mysql/ -L/usr/local/lib/ -L/usr/lib/
# Set your machine architecture eg -march=i686
CFLAGS =
# Include paths for compiler
IFLAGS = -I. -I/usr/local/include/mysql/ -I/usr/include/mysql/ -I/usr/local/include/ -I/usr/include/ 

ifeq ($(SYSTEM),Linux)
DFLAGS += -D__LINUX__
WFLAGS +=
WPPFLAGS +=
OFLAGS +=
LFLAGS +=
CFLAGS +=
IFLAGS +=
endif

ifeq ($(SYSTEM),FreeBSD)
DFLAGS += -D__FREEBSD__
WFLAGS += 
WPPFLAGS += 
OFLAGS +=
LFLAGS +=
CFLAGS +=
IFLAGS += 
endif

ifeq ($(SYSTEM),OpenBSD)
DFLAGS += -D__OPENBSD__
WFLAGS += 
WPPFLAGS += 
OFLAGS +=
LFLAGS +=
CFLAGS +=
IFLAGS +=
endif

ifeq ($(SYSTEM),NetBSD)
DFLAGS += -D__NETBSD__
WFLAGS += 
WPPFLAGS += 
OFLAGS +=
LFLAGS +=
CFLAGS +=
IFLAGS +=
endif

ifeq ($(SYSTEM),SunOS)
DFLAGS += -D__SOLARIS__
WFLAGS += 
WPPFLAGS += 
OFLAGS +=
LFLAGS += -lresolv -lsocket -lnsl -lpthread
CFLAGS +=
IFLAGS +=
endif

ifeq ($(SYSTEM),Darwin)
DFLAGS += -D__APPLE__
WFLAGS += 
WPPFLAGS += 
OFLAGS += -flat_namespace
LFLAGS += -lpthread
CFLAGS +=
else
DFLAGS +=
WFLAGS += 
WPPFLAGS += 
OFLAGS +=
LFLAGS += -pthread
CFLAGS += -pthread
IFLAGS +=
endif

CFLAGS += $(OFLAGS) $(DFLAGS) $(WFLAGS) $(WPPFLAGS) $(IFLAGS) -I.

# OBJS = atom.o base64.o bencode.o client.o config.o link.o md5.o server.o sha1.o sort.o tracker_log.o tracker_file.o tracker_rules.o tracker_faq.o  util.o
OBJS = atom.o base64.o bencode.o client.o config.o md5.o server.o sha1.o sort.o tracker_log.o tracker_rules.o tracker_faq.o  util.o
OBJS_BNBT = bnbt.o bnbt_mysql.o tracker.o tracker_announce.o tracker_scrape.o
OBJS_BNBTMYSQL = bnbt.mysql.o bnbt_mysql.mysql.o tracker.mysql.o tracker_admin.mysql.o tracker_comments.mysql.o tracker_talk.mysql.o tracker_index.mysql.o tracker_info.mysql.o tracker_login.mysql.o tracker_messages.mysql.o tracker_offer.mysql.o tracker_stats.mysql.o tracker_upload.mysql.o tracker_announce.mysql.o tracker_scrape.mysql.o tracker_signup.mysql.o tracker_staff.mysql.o tracker_torrent.mysql.o tracker_file.mysql.o tracker_users.mysql.o

OBJS_GD = atom.gd.o base64.gd.o bencode.gd.o client.gd.o config.gd.o link.gd.o md5.gd.o server.gd.o sha1.gd.o sort.gd.o tracker_admin.gd.o tracker_comments.gd.o tracker_file.gd.o tracker_index.gd.o tracker_info.gd.o tracker_login.gd.o tracker_signup.gd.o tracker_stats.gd.o tracker_torrent.gd.o tracker_upload.gd.o tracker_users.gd.o util.gd.o
OBJS_XBNBTGD = bnbt.gd.o bnbt_mysql.gd.o tracker.gd.o tracker_announce.gd.o tracker_scrape.gd.o
OBJS_XBNBTMYSQLGD = bnbt.gdmysql.o bnbt_mysql.gdmysql.o tracker.gdmysql.o tracker_announce.gdmysql.o tracker_scrape.gdmysql.o

OBJS_USERS = atom.users.o base64.users.o bencode.users.o client.users.o config.users.o link.users.o md5.users.o server.users.o sha1.users.o sort.users.o tracker_admin.users.o tracker_comments.users.o tracker_file.users.o tracker_index.users.o tracker_info.users.o tracker_login.users.o tracker_signup.users.o tracker_stats.users.o tracker_torrent.users.o tracker_upload.users.o tracker_users.users.o util.users.o
OBJS_XBNBTUSERS = bnbt.users.o bnbt_mysql.users.o tracker.users.o tracker_announce.users.o tracker_scrape.users.o
OBJS_XBNBTMYSQLUSERS = bnbt.umysql.o bnbt_mysql.umysql.o tracker.umysql.o tracker_announce.umysql.o tracker_scrape.umysql.o

OBJS_X = atom.x.o base64.x.o bencode.x.o client.x.o config.x.o link.x.o md5.x.o server.x.o sha1.x.o sort.x.o tracker_admin.x.o tracker_comments.x.o tracker_file.x.o tracker_index.x.o tracker_info.x.o tracker_login.x.o tracker_signup.x.o tracker_stats.x.o tracker_torrent.x.o tracker_upload.x.o tracker_users.x.o util.x.o
OBJS_XBNBT = bnbt.x.o bnbt_mysql.x.o tracker.x.o tracker_announce.x.o tracker_scrape.x.o
OBJS_XBNBTMYSQL = bnbt.xmysql.o bnbt_mysql.xmysql.o tracker.xmysql.o tracker_announce.xmysql.o tracker_scrape.xmysql.o

PROGS = ./bnbt ./bnbtmysql ./xbnbtgd ./xbnbtmysqlgd ./xbnbtusers ./xbnbtmysqlusers ./xbnbt ./xbnbtmysql

all: $(OBJS) $(OBJS_BNBT) $(OBJS_BNBTMYSQL) $(OBJS_GD) $(OBJS_XBNBTGD) $(OBJS_XBNBTMYSQLGD) $(OBJS_USERS) $(OBJS_XBNBTUSERS) $(OBJS_XBNBTMYSQLUSERS) $(OBJS_X) $(OBJS_XBNBT) $(OBJS_XBNBTMYSQL) $(PROGS)

./bnbt: $(OBJS) $(OBJS_BNBT)
	$(C++) -o ./bnbt $(OBJS) $(OBJS_BNBT) $(LFLAGS) 
	
./bnbtmysql: $(OBJS) $(OBJS_BNBTMYSQL)
	$(C++) -o ./bnbtmysql $(OBJS) $(OBJS_BNBTMYSQL) $(LFLAGS) -lgd -lmysqlclient

./xbnbtgd: $(OBJS_GD) $(OBJS_XBNBTGD)
	$(C++) -o ./xbnbtgd $(OBJS_GD) $(OBJS_XBNBTGD) $(LFLAGS) -lgd
	
./xbnbtmysqlgd: $(OBJS_GD) $(OBJS_XBNBTMYSQLGD)
	$(C++) -o ./xbnbtmysqlgd $(OBJS_GD) $(OBJS_XBNBTMYSQLGD) $(LFLAGS) -lgd -lmysqlclient	

./xbnbtusers: $(OBJS_USERS) $(OBJS_XBNBTUSERS)
	$(C++) -o ./xbnbtusers $(OBJS_USERS) $(OBJS_XBNBTUSERS) $(LFLAGS) -lmysqlclient
	
./xbnbtmysqlusers: $(OBJS_USERS) $(OBJS_XBNBTMYSQLUSERS)
	$(C++) -o ./xbnbtmysqlusers $(OBJS_USERS) $(OBJS_XBNBTMYSQLUSERS) $(LFLAGS) -lmysqlclient	

./xbnbt: $(OBJS_X) $(OBJS_XBNBT)
	$(C++) -o ./xbnbt $(OBJS_X) $(OBJS_XBNBT) $(LFLAGS) -lgd -lmysqlclient
	
./xbnbtmysql: $(OBJS_X) $(OBJS_XBNBTMYSQL)
	$(C++) -o ./xbnbtmysql $(OBJS_X) $(OBJS_XBNBTMYSQL) $(LFLAGS) -lgd -lmysqlclient		
		
clean:
	rm -f $(OBJS) $(OBJS_BNBT) $(OBJS_BNBTMYSQL) $(OBJS_GD) $(OBJS_XBNBTGD) $(OBJS_XBNBTMYSQLGD) $(OBJS_USERS) $(OBJS_XBNBTUSERS) $(OBJS_XBNBTMYSQLUSERS) $(OBJS_X) $(OBJS_XBNBT) $(OBJS_XBNBTMYSQL) $(PROGS)

cleanbnbt:
	rm -f $(OBJS) $(OBJS_BNBT) ./bnbt

cleanbnbtmysql:
	rm -f $(OBJS) $(OBJS_BNBTMYSQL) ./bnbtmysql

cleanxbnbtgd:
	rm -f $(OBJS_GD) $(OBJS_XBNBTGD) ./xbnbtgd

cleanxbnbtmysqlgd:
	rm -f $(OBJS_GD) $(OBJS_XBNBTMYSQLGD) ./xbnbtmysqlgd

cleanxbnbtusers:
	rm -f $(OBJS_USERS) $(OBJS_XBNBTUSERS) ./xbnbtusers

cleanxbnbtmysqlusers:
	rm -f $(OBJS_USERS) $(OBJS_XBNBTMYSQLUSERS) ./xbnbtmysqlusers

cleanxbnbt:
	rm -f $(OBJS_X) $(OBJS_XBNBT) ./xbnbt

cleanxbnbtmysql:
	rm -f $(OBJS_X) $(OBJS_XBNBTMYSQL) ./xbnbtmysql

%.o: %.cpp
	$(C++) -o $@ $(CFLAGS) -c $<

%.mysql.o: %.cpp
	$(C++) -o $@ $(CFLAGS) -DBNBT_MYSQL -c $<

%.gd.o: %.cpp
	$(C++) -o $@ $(CFLAGS) -DXBNBT_GD -c $<

%.gdmysql.o: %.cpp
	$(C++) -o $@ $(CFLAGS) -DBNBT_MYSQL -DXBNBT_GD -c $<
	
%.users.o: %.cpp
	$(C++) -o $@ $(CFLAGS) -DXBNBT_MYSQL -c $<
	
%.umysql.o: %.cpp
	$(C++) -o $@ $(CFLAGS) -DBNBT_MYSQL -DXBNBT_MYSQL -c $<	
	
%.x.o: %.cpp
	$(C++) -o $@ $(CFLAGS) -DXBNBT_GD -DXBNBT_MYSQL -c $<
	
%.xmysql.o: %.cpp
	$(C++) -o $@ $(CFLAGS) -DBNBT_MYSQL -DXBNBT_GD -DXBNBT_MYSQL -c $<	

./bnbt: $(OBJS) $(OBJS_BNBT)

./bnbtmysql: $(OBJS) $(OBJS_BNBTMYSQL)

./xbnbtgd: $(OBJS_GD) $(OBJS_XBNBTGD)

./xbnbtmysqlgd: $(OBJS_GD) $(OBJS_XBNBTMYSQLGD)

./xbnbtusers: $(OBJS_USERS) $(OBJS_XBNBTUSERS)

./xbnbtmysqlusers: $(OBJS_USERS) $(OBJS_XBNBTMYSQLUSERS)

./xbnbt: $(OBJS_X) $(OBJS_XBNBT)

./xbnbtmysql: $(OBJS_X) $(OBJS_XBNBTMYSQL)

all: $(PROGS)

atom.o: bnbt.h atom.h util.h config.h
base64.o: bnbt.h base64.h
bencode.o: bnbt.h atom.h bencode.h util.h config.h
bnbt.o: bnbt.h atom.h config.h link.h server.h util.h bnbt_mysql.h
bnbt_mysql.o: bnbt.h bnbt_mysql.h util.h
client.o: bnbt.h atom.h base64.h client.h md5.h server.h tracker.h util.h
config.o: bnbt.h atom.h config.h util.h
# link.o: bnbt.h atom.h bencode.h config.h link.h md5.h server.h tracker.h util.h
md5.o: bnbt.h md5.h
server.o: bnbt.h client.h config.h server.h tracker.h util.h
sha1.o: bnbt.h sha1.h
sort.o: bnbt.h sort.h tracker.h util.h
tracker.o: bnbt.h atom.h bencode.h bnbt_mysql.h config.h html.h md5.h server.h sort.h tracker.h util.h
tracker_admin.o: bnbt.h atom.h bencode.h bnbt_mysql.h config.h html.h link.h tracker.h server.h util.h
tracker_announce.o: bnbt.h atom.h config.h bencode.h link.h tracker.h util.h bnbt_mysql.h
tracker_comments.o: bnbt.h atom.h config.h html.h tracker.h util.h
tracker_talk.o: bnbt.h atom.h config.h html.h tracker.h util.h
tracker_messages.o: bnbt.h atom.h bnbt_mysql.h config.h html.h tracker.h util.h
tracker_file.o: bnbt.h atom.h config.h tracker.h util.h
tracker_index.o: bnbt.h atom.h bnbt_mysql.h config.h html.h sort.h tracker.h util.h server.h
tracker_info.o: bnbt.h atom.h bnbt_mysql.h config.h tracker.h util.h
tracker_rules.o: bnbt.h atom.h config.h html.h tracker.h util.h
tracker_faq.o: bnbt.h atom.h config.h html.h tracker.h util.h
tracker_staff.o: bnbt.h atom.h bnbt_mysql.h config.h html.h tracker.h util.h
tracker_log.o: bnbt.h atom.h config.h html.h tracker.h util.h
tracker_login.o: bnbt.h atom.h config.h html.h tracker.h util.h
tracker_scrape.o: bnbt.h atom.h bencode.h bnbt_mysql.h config.h tracker.h util.h
tracker_signup.o: bnbt.h atom.h bnbt_mysql.h config.h html.h md5.h tracker.h util.h 
tracker_stats.o: bnbt.h atom.h bencode.h bnbt_mysql.h config.h html.h sort.h tracker.h util.h
tracker_torrent.o: bnbt.h atom.h bencode.h config.h tracker.h util.h
tracker_offer.o: bnbt.h atom.h config.h bnbt_mysql.h html.h sort.h tracker.h util.h server.h
tracker_upload.o: bnbt.h atom.h bencode.h bnbt_mysql.h client.h config.h html.h tracker.h util.h
tracker_users.o: bnbt.h atom.h config.h html.h md5.h sort.h tracker.h util.h
util.o: bnbt.h atom.h bencode.h sha1.h tracker.h util.h config.h

bnbt.mysql.o: bnbt.h bnbt_mysql.h config.h server.h util.h
bnbt_mysql.mysql.o: bnbt.h bnbt_mysql.h util.h
tracker.mysql.o: bnbt.h bnbt_mysql.h atom.h bencode.h config.h html.h md5.h server.h sort.h tracker.h util.h
tracker_announce.mysql.o: bnbt.h bnbt_mysql.h atom.h bencode.h config.h link.h tracker.h util.h
tracker_comments.mysql.o: bnbt.h bnbt_mysql.h atom.h config.h html.h tracker.h util.h
tracker_talk.mysql.o: bnbt.h bnbt_mysql.h atom.h config.h html.h tracker.h util.h
tracker_index.mysql.o: bnbt.h atom.h bnbt_mysql.h config.h html.h sort.h tracker.h util.h server.h
tracker_login.mysql.o: bnbt.h atom.h bnbt_mysql.h config.h html.h tracker.h util.h
tracker_scrape.mysql.o: bnbt.h bnbt_mysql.h atom.h bencode.h config.h tracker.h util.h
tracker_stats.mysql.o: bnbt.h atom.h bencode.h bnbt_mysql.h config.h html.h sort.h tracker.h util.h
tracker_signup.mysql.o: bnbt.h atom.h bnbt_mysql.h config.h html.h md5.h tracker.h util.h 
tracker_torrent.mysql.o: bnbt.h atom.h bencode.h bnbt_mysql.h config.h tracker.h util.h
tracker_upload.mysql.o: bnbt.h atom.h bencode.h bnbt_mysql.h client.h config.h html.h tracker.h util.h
tracker_users.mysql.o: bnbt.h atom.h bnbt_mysql.h config.h html.h md5.h sort.h tracker.h util.h

atom.gd.o: bnbt.h atom.h util.h config.h
base64.gd.o: bnbt.h base64.h
bencode.gd.o: bnbt.h atom.h bencode.h util.h config.h
bnbt.gd.o: bnbt.h atom.h config.h link.h server.h util.h bnbt_mysql.h
bnbt_mysql.gd.o: bnbt.h bnbt_mysql.h util.h
client.gd.o: bnbt.h atom.h base64.h client.h md5.h server.h tracker.h util.h 
config.gd.o: bnbt.h atom.h config.h util.h
link.gd.o: bnbt.h atom.h bencode.h config.h link.h md5.h server.h tracker.h util.h
md5.gd.o: bnbt.h md5.h
server.gd.o: bnbt.h client.h config.h server.h tracker.h util.h
sha1.gd.o: bnbt.h sha1.h
sort.gd.o: bnbt.h sort.h tracker.h util.h
tracker.gd.o: bnbt.h atom.h bencode.h bnbt_mysql.h config.h html.h md5.h server.h sort.h tracker.h util.h
tracker_admin.gd.o: bnbt.h atom.h bencode.h config.h html.h link.h tracker.h server.h util.h
tracker_announce.gd.o: bnbt.h atom.h bencode.h bnbt_mysql.h config.h link.h tracker.h util.h
tracker_comments.gd.o: bnbt.h atom.h config.h html.h tracker.h util.h
tracker_talk.gd.o: bnbt.h atom.h config.h html.h tracker.h util.h
tracker_file.gd.o: bnbt.h atom.h config.h tracker.h util.h
tracker_index.gd.o: bnbt.h atom.h config.h html.h sort.h tracker.h util.h server.h
tracker_info.gd.o: bnbt.h atom.h config.h tracker.h util.h
tracker_login.gd.o: bnbt.h atom.h config.h html.h tracker.h util.h
tracker_scrape.gd.o: bnbt.h atom.h bencode.h bnbt_mysql.h config.h tracker.h util.h
tracker_signup.gd.o: bnbt.h atom.h config.h html.h tracker.h util.h
tracker_stats.gd.o: bnbt.h atom.h bencode.h config.h html.h sort.h tracker.h util.h
tracker_torrent.gd.o: bnbt.h atom.h bencode.h config.h tracker.h util.h
tracker_upload.gd.o: bnbt.h atom.h bencode.h client.h config.h html.h tracker.h util.h
tracker_users.gd.o: bnbt.h atom.h bnbt_mysql.h config.h html.h md5.h sort.h tracker.h util.h
util.gd.o: bnbt.h atom.h bencode.h config.h sha1.h tracker.h util.h

bnbt.gdmysql.o: bnbt.h bnbt_mysql.h config.h server.h util.h
bnbt_mysql.gdmysql.o: bnbt.h bnbt_mysql.h util.h
tracker.gdmysql.o: bnbt.h bnbt_mysql.h atom.h bencode.h config.h html.h md5.h server.h sort.h tracker.h util.h
tracker_announce.gdmysql.o: bnbt.h bnbt_mysql.h atom.h bencode.h link.h tracker.h util.h
tracker_scrape.gdmysql.o: bnbt.h bnbt_mysql.h atom.h bencode.h tracker.h util.h

atom.users.o: bnbt.h atom.h util.h config.h
base64.users.o: bnbt.h base64.h
bencode.users.o: bnbt.h atom.h bencode.h util.h config.h
bnbt.users.o: bnbt.h atom.h config.h link.h server.h util.h bnbt_mysql.h
bnbt_mysql.users.o: bnbt.h bnbt_mysql.h util.h
client.users.o: bnbt.h atom.h base64.h client.h md5.h server.h tracker.h util.h 
config.users.o: bnbt.h atom.h config.h util.h
link.users.o: bnbt.h atom.h bencode.h config.h link.h md5.h server.h tracker.h util.h
md5.users.o: bnbt.h md5.h
server.users.o: bnbt.h client.h config.h server.h tracker.h util.h
sha1.users.o: bnbt.h sha1.h
sort.users.o: bnbt.h sort.h tracker.h util.h
tracker.users.o: bnbt.h atom.h bencode.h bnbt_mysql.h config.h html.h md5.h server.h sort.h tracker.h util.h
tracker_admin.users.o: bnbt.h atom.h bencode.h config.h html.h link.h tracker.h server.h util.h
tracker_announce.users.o: bnbt.h atom.h bencode.h bnbt_mysql.h config.h link.h tracker.h util.h
tracker_comments.users.o: bnbt.h atom.h config.h html.h tracker.h util.h
tracker_talk.users.o: bnbt.h atom.h config.h html.h tracker.h util.h
tracker_file.users.o: bnbt.h atom.h config.h tracker.h util.h
tracker_index.users.o: bnbt.h atom.h config.h html.h sort.h tracker.h util.h server.h
tracker_info.users.o: bnbt.h atom.h config.h tracker.h util.h
tracker_login.users.o: bnbt.h atom.h config.h html.h tracker.h util.h
tracker_scrape.users.o: bnbt.h atom.h bencode.h bnbt_mysql.h config.h tracker.h util.h
tracker_signup.users.o: bnbt.h atom.h config.h html.h tracker.h util.h
tracker_stats.users.o: bnbt.h atom.h bencode.h config.h html.h sort.h tracker.h util.h
tracker_torrent.users.o: bnbt.h atom.h bencode.h config.h tracker.h util.h
tracker_upload.users.o: bnbt.h atom.h bencode.h client.h config.h html.h tracker.h util.h
tracker_users.users.o: bnbt.h atom.h bnbt_mysql.h config.h html.h md5.h sort.h tracker.h util.h
util.users.o: bnbt.h atom.h bencode.h config.h sha1.h tracker.h util.h

bnbt.umysql.o: bnbt.h bnbt_mysql.h config.h server.h util.h
bnbt_mysql.umysql.o: bnbt.h bnbt_mysql.h util.h
tracker.umysql.o: bnbt.h bnbt_mysql.h atom.h bencode.h config.h html.h md5.h server.h sort.h tracker.h util.h
tracker_announce.umysql.o: bnbt.h bnbt_mysql.h atom.h bencode.h link.h tracker.h util.h
tracker_scrape.xumysql.o: bnbt.h bnbt_mysql.h atom.h bencode.h tracker.h util.h

atom.x.o: bnbt.h atom.h util.h config.h
base64.x.o: bnbt.h base64.h
bencode.x.o: bnbt.h atom.h bencode.h util.h config.h
bnbt.x.o: bnbt.h atom.h config.h link.h server.h util.h bnbt_mysql.h
bnbt_mysql.x.o: bnbt.h bnbt_mysql.h util.h
client.x.o: bnbt.h atom.h base64.h client.h md5.h server.h tracker.h util.h 
config.x.o: bnbt.h atom.h config.h util.h
link.x.o: bnbt.h atom.h bencode.h config.h link.h md5.h server.h tracker.h util.h
md5.x.o: bnbt.h md5.h
server.x.o: bnbt.h client.h config.h server.h tracker.h util.h
sha1.x.o: bnbt.h sha1.h
sort.x.o: bnbt.h sort.h tracker.h util.h
tracker.x.o: bnbt.h atom.h bencode.h bnbt_mysql.h config.h html.h md5.h server.h sort.h tracker.h util.h
tracker_admin.x.o: bnbt.h atom.h bencode.h config.h html.h link.h tracker.h server.h util.h
tracker_announce.x.o: bnbt.h atom.h bencode.h bnbt_mysql.h config.h link.h tracker.h util.h
tracker_comments.x.o: bnbt.h atom.h config.h html.h tracker.h util.h
tracker_talk.x.o: bnbt.h atom.h config.h html.h tracker.h util.h
tracker_file.x.o: bnbt.h atom.h config.h tracker.h util.h
tracker_index.x.o: bnbt.h atom.h config.h html.h sort.h tracker.h util.h server.h
tracker_info.x.o: bnbt.h atom.h config.h tracker.h util.h
tracker_login.x.o: bnbt.h atom.h config.h html.h tracker.h util.h
tracker_scrape.x.o: bnbt.h atom.h bencode.h bnbt_mysql.h config.h tracker.h util.h
tracker_signup.x.o: bnbt.h atom.h config.h html.h tracker.h util.h
tracker_stats.x.o: bnbt.h atom.h bencode.h config.h html.h sort.h tracker.h util.h
tracker_torrent.x.o: bnbt.h atom.h bencode.h config.h tracker.h util.h
tracker_upload.x.o: bnbt.h atom.h bencode.h client.h config.h html.h tracker.h util.h
tracker_users.x.o: bnbt.h atom.h bnbt_mysql.h config.h html.h md5.h sort.h tracker.h util.h
util.x.o: bnbt.h atom.h bencode.h config.h sha1.h tracker.h util.h

bnbt.xmysql.o: bnbt.h bnbt_mysql.h config.h server.h util.h
bnbt_mysql.xmysql.o: bnbt.h bnbt_mysql.h util.h
tracker.xmysql.o: bnbt.h bnbt_mysql.h atom.h bencode.h config.h html.h md5.h server.h sort.h tracker.h util.h
tracker_announce.xmysql.o: bnbt.h bnbt_mysql.h atom.h bencode.h link.h tracker.h util.h
tracker_scrape.xmysql.o: bnbt.h bnbt_mysql.h atom.h bencode.h tracker.h util.h
