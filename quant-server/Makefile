CXX       ?= g++
CXXFLAGS  = -std=c++17 -O2 -Wall -Wextra
INCLUDES  = -I../Quant/cpp-httplib-master -I../Quant
TARGET    = quant-server

# httplib threading
LDFLAGS   = -lpthread

# Uncomment for TLS support (requires OpenSSL)
# CXXFLAGS += -DCPPHTTPLIB_OPENSSL_SUPPORT
# LDFLAGS  += -lssl -lcrypto

all: $(TARGET)

$(TARGET): main.cpp SyncStore.h TicketSystem.h ../Quant/UserManager.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $(TARGET) main.cpp $(LDFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
