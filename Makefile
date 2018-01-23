ifeq ($(OS),Windows_NT)
OBJECTS += async.o gerror.o
OBJECTS += $(patsubst %.c,%.o,$(wildcard src/windows/*.c))
else
OBJECTS += async.o
OBJECTS += $(patsubst %.c,%.o,$(wildcard src/posix/*.c))
endif

CPPFLAGS += -Iinclude -I. -I../
CFLAGS += -fPIC

LDFLAGS += -L../gimxlog
LDLIBS += -lgimxlog

include Makedefs

ifeq ($(OS),Windows_NT)
async.o: ../gimxcommon/src/windows/async.c
	$(COMPILE.c) $(OUTPUT_OPTION) $<

gerror.o: ../gimxcommon/src/windows/gerror.c
	$(COMPILE.c) $(OUTPUT_OPTION) $<
else
async.o: ../gimxcommon/src/posix/async.c
	$(COMPILE.c) $(OUTPUT_OPTION) $<
endif
