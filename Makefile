include config.mk

OBJS_DIR = obj

INCLUDE = -Ilib
LIBS = -luv

CFLAGS = -fPIC $(CPPFLAGS) $(OPTIMIZE) $(INCLUDE) $(LIBS)

LIB_HEADERS = $(INCLUDE)/sf.h
LIB_OBJS = $(OBJS_DIR)/lib/sf.o
CLIENT_OBJS = $(OBJS_DIR)/client/main.o
SERVER_OBJS = $(OBJS_DIR)/server/main.o

.PHONY: all clean

$(OBJS_DIR)/%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

all: dir $(SHARED_LIB) $(STATIC_LIB) $(CLIENT) $(CLIENT_STATIC) $(SERVER) $(SERVER_STATIC)

dir:
	[ -d $(OBJS_DIR) ] || mkdir $(OBJS_DIR)
	[ -d $(OBJS_DIR)/lib ] || mkdir $(OBJS_DIR)/lib
	[ -d $(OBJS_DIR)/client ] || mkdir $(OBJS_DIR)/client
	[ -d $(OBJS_DIR)/server ] || mkdir $(OBJS_DIR)/server

$(SHARED_LIB): $(LIB_OBJS)
	$(CC) -shared -Wl,-soname,$(SONAME) -o $(SHARED_LIB) $(LIB_OBJS)
	ln -fs $(SHARED_LIB) $(SONAME)

$(STATIC_LIB): $(LIB_OBJS)
	  $(AR) $(ARFLAGS) $(STATIC_LIB) $(LIB_OBJS)

$(CLIENT): $(CLIENT_OBJS) $(SHARED_LIB)
	$(CC) $(CFLAGS) $(INCLUDE) $(LIBS) -o $@ $^

$(CLIENT_STATIC): $(CLIENT_OBJS) $(STATIC_LIB)
	$(CC) $(CFLAGS) $(INCLUDE) $(LIBS) -o $@ $^

$(SERVER): $(SERVER_OBJS) $(SHARED_LIB)
	$(CC) $(CFLAGS) $(INCLUDE) $(LIBS) -lpcre2-8 -o $@ $^

$(SERVER_STATIC): $(SERVER_OBJS) $(STATIC_LIB)
	$(CC) $(CFLAGS) $(INCLUDE) $(LIBS) -lpcre2-8 -o $@ $^

clean: cleanobj
	rm -f $(CLIENT) $(CLIENT_STATIC) $(SERVER) $(SERVER_STATIC) *.so* *.a

cleanobj:
	rm -f $(OBJS_DIR)/*/*.o
