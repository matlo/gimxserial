ifneq ($(DEBUG),yes)
CFLAGS += -Wall -Wextra -O3
CXXFLAGS += -Wall -Wextra -O3
else
ifneq ($(OS),Windows_NT)
CFLAGS += -Wall -Wextra -O0 -g -fsanitize=address -fno-omit-frame-pointer
CXXFLAGS += -Wall -Wextra -O0 -g -fsanitize=address -fno-omit-frame-pointer
else
CFLAGS += -Wall -Wextra -O0 -g -fno-omit-frame-pointer
CXXFLAGS += -Wall -Wextra -O0 -g -fno-omit-frame-pointer
endif
endif

CPPFLAGS = -I../..

LDFLAGS = -L../../gimxserial -L../../gimxpoll -L../../gimxlog -L../../gimxtime -L../../gimxtimer -L../../gimxprio
LDLIBS = -lgimxserial -lgimxpoll -lgimxlog -lgimxtime -lgimxtimer -lgimxprio

LDLIBS += -lm

ifeq ($(OS),Windows_NT)
CPPFLAGS += `sdl2-config --cflags`
LDLIBS += $(shell sdl2-config --libs) -lws2_32 -lpdcursesw -lintl
LDLIBS:=$(filter-out -mwindows,$(LDLIBS))
endif


BINS=gserial_test
ifneq ($(OS),Windows_NT)
OUT=$(BINS)
else
OUT=gserial_test.exe
endif

all: $(BINS)

clean:
	$(RM) $(OUT) *~
