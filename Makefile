
CCFLAGS = 
RESP ?= TCP
CLOSING_DELAY ?= 1

ifeq ($(OS),Windows_NT)
	CCFLAGS += -lws2_32
	OUTPUT_EXT = .exe
else
	OUTPUT_EXT = 
endif

ifeq ($(RESP),TCP)
	CCFLAGS += -DRESP_TCP
else ifeq ($(RESP),HTTP)
	CCFLAGS += -DRESP_HTTP
endif

ifeq ($(CLOSING_DELAY),0)
	CCFLAGS += -DNO_CLOSING_DELAY
endif

all: heartbeat.cpp
	${CXX} -o heartbeat${OUTPUT_EXT} heartbeat.cpp ${CCFLAGS}


clean:
	rm *.exe *.o *.obj heartbeat