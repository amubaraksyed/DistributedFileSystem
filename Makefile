all: gunrock_web mkfs ds3ls ds3cat ds3bits test_lfs

CC = g++
CFLAGS = -g -Werror -Wall -I include -I shared/include -I/opt/homebrew/opt/openssl@3/include
CXXFLAGS = -g -Werror -Wall -std=c++11 -I include -I shared/include -I/opt/homebrew/opt/openssl@3/include
LDFLAGS = -L/opt/homebrew/opt/openssl@3/lib -lssl -lcrypto -pthread
VPATH = shared

OBJS = gunrock.o MyServerSocket.o MySocket.o HTTPRequest.o HTTPResponse.o http_parser.o HTTP.o HttpService.o HttpUtils.o FileService.o dthread.o WwwFormEncodedDict.o StringUtils.o Base64.o HttpClient.o HTTPClientResponse.o MySslSocket.o DistributedFileSystemService.o LocalFileSystem.o Disk.o

DSUTIL_OBJS = Disk.o LocalFileSystem.o

-include $(OBJS:.o=.d)

gunrock_web: $(OBJS)
	$(CC) -o $@ $(OBJS) $(CFLAGS) $(LDFLAGS)

mkfs: mkfs.o
	gcc -o $@ mkfs.o $(CFLAGS) $(LDFLAGS)

ds3ls: ds3ls.o $(DSUTIL_OBJS)
	$(CC) -o $@ ds3ls.o $(DSUTIL_OBJS) $(CXXFLAGS) $(LDFLAGS)

ds3cat: ds3cat.o $(DSUTIL_OBJS)
	$(CC) -o $@ ds3cat.o $(DSUTIL_OBJS) $(CXXFLAGS) $(LDFLAGS)

ds3bits: ds3bits.o $(DSUTIL_OBJS)
	$(CC) -o $@ ds3bits.o $(DSUTIL_OBJS) $(CXXFLAGS) $(LDFLAGS)

test_lfs: test_lfs.o $(DSUTIL_OBJS)
	$(CC) -o $@ test_lfs.o $(DSUTIL_OBJS) $(CXXFLAGS) $(LDFLAGS)

%.d: %.c
	@set -e; gcc -MM $(CFLAGS) $< \
		| sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@;
	@[ -s $@ ] || rm -f $@

%.d: %.cpp
	@set -e; $(CC) -MM $(CXXFLAGS) $< \
		| sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@;
	@[ -s $@ ] || rm -f $@

%.o: %.cpp
	$(CC) $(CXXFLAGS) -c $< -o $@

%.o: %.c
	gcc $(CFLAGS) -c $< -o $@

clean:
	rm -f gunrock_web mkfs ds3ls ds3cat ds3bits test_lfs *.o *~ core.* *.d
