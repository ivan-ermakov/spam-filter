include config.mk

BIN_DIR = bin
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
	[ -d $(BIN_DIR) ] || mkdir $(BIN_DIR)

$(SHARED_LIB): $(LIB_OBJS)
	$(CC) -shared -Wl,-soname,$(SONAME) -o $(BIN_DIR)/$(SHARED_LIB) $(LIB_OBJS)
	ln -fs $(BIN_DIR)/$(SHARED_LIB) $(SONAME)

$(STATIC_LIB): $(LIB_OBJS)
	  $(AR) $(ARFLAGS) $(BIN_DIR)/$(STATIC_LIB) $(LIB_OBJS)

$(CLIENT): $(CLIENT_OBJS) $(BIN_DIR)/$(SHARED_LIB)
	$(CC) $(CFLAGS) $(INCLUDE) $(LIBS) -o $(BIN_DIR)/$@ $^

$(CLIENT_STATIC): $(CLIENT_OBJS) $(BIN_DIR)/$(STATIC_LIB)
	$(CC) $(CFLAGS) $(INCLUDE) $(LIBS) -o $(BIN_DIR)/$@ $^

$(SERVER): $(SERVER_OBJS) $(BIN_DIR)/$(SHARED_LIB)
	$(CC) $(CFLAGS) $(INCLUDE) $(LIBS) -lpcre2-8 -o $(BIN_DIR)/$@ $^

$(SERVER_STATIC): $(SERVER_OBJS) $(BIN_DIR)/$(STATIC_LIB)
	$(CC) $(CFLAGS) $(INCLUDE) $(LIBS) -lpcre2-8 -o $(BIN_DIR)/$@ $^

clean: cleanobj
	rm -f $(CLIENT) $(CLIENT_STATIC) $(SERVER) $(SERVER_STATIC) *.so* *.a

cleanobj:
	rm -f $(OBJS_DIR)/*/*.o
