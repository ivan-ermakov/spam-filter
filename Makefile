include config.mk

BIN_DIR = bin
OBJS_DIR = obj

INCLUDE = -I.
LIBS = -luv

LIB_HEADERS = $(INCLUDE)/sf.h $(INCLUDE)/buf.h
LIB_OBJS = $(OBJS_DIR)/lib/sf.o $(OBJS_DIR)/lib/buf.o
CLIENT_HEADERS = client/client.h
CLIENT_OBJS = $(OBJS_DIR)/client/main.o $(OBJS_DIR)/client/client.o
SERVER_HEADERS = server/server.h server/client.h server/spam_filter.h
SERVER_OBJS = $(OBJS_DIR)/server/main.o $(OBJS_DIR)/server/server.o $(OBJS_DIR)/server/client.o $(OBJS_DIR)/server/spam_filter.o

.PHONY: all clean

$(OBJS_DIR)/%.o: %.c
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<

all: dir $(SHARED_LIB) $(STATIC_LIB) $(CLIENT) $(CLIENT_STATIC) $(SERVER) $(SERVER_STATIC)

dir:
	[ -d $(OBJS_DIR) ] || mkdir $(OBJS_DIR)
	[ -d $(OBJS_DIR)/lib ] || mkdir $(OBJS_DIR)/lib
	[ -d $(OBJS_DIR)/client ] || mkdir $(OBJS_DIR)/client
	[ -d $(OBJS_DIR)/server ] || mkdir $(OBJS_DIR)/server
	[ -d $(BIN_DIR) ] || mkdir $(BIN_DIR)

$(SHARED_LIB): $(LIB_OBJS)
	$(CC) -shared -Wl,-soname,$(BIN_DIR)/$(SONAME) -o $(BIN_DIR)/$(SHARED_LIB) $(LIB_OBJS)
	ln -fs $(SHARED_LIB) $(BIN_DIR)/$(SONAME)

$(STATIC_LIB): $(LIB_OBJS)
	$(AR) $(ARFLAGS) $(BIN_DIR)/$(STATIC_LIB) $(LIB_OBJS)

$(CLIENT): $(CLIENT_OBJS) $(CLIENT_HEADERS) $(BIN_DIR)/$(SHARED_LIB)
	$(CC) $(CFLAGS) $(INCLUDE) -o $(BIN_DIR)/$@ $^ $(LIBS)

$(CLIENT_STATIC): $(CLIENT_OBJS) $(CLIENT_HEADERS) $(BIN_DIR)/$(STATIC_LIB)
	$(CC) $(CFLAGS) $(INCLUDE) -o $(BIN_DIR)/$@ $^ $(LIBS)

$(SERVER): $(SERVER_OBJS) $(SERVER_HEADERS) $(BIN_DIR)/$(SHARED_LIB)
	$(CC) $(CFLAGS) $(INCLUDE) -o $(BIN_DIR)/$@ $^ $(LIBS) -lpcre2-8

$(SERVER_STATIC): $(SERVER_OBJS) $(SERVER_HEADERS) $(BIN_DIR)/$(STATIC_LIB)
	$(CC) $(CFLAGS) $(INCLUDE) -o $(BIN_DIR)/$@ $^ $(LIBS) -lpcre2-8

clean: cleanobj
	rm -f $(BIN_DIR)/$(CLIENT) $(BIN_DIR)/$(CLIENT_STATIC) $(BIN_DIR)/$(SERVER) $(BIN_DIR)/$(SERVER_STATIC) $(BIN_DIR)/*.so* $(BIN_DIR)/*.a

cleanobj:
	rm -f $(OBJS_DIR)/*/*.o
