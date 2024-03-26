XCORNERS := xcorners

X11_CFLAGS = $(shell pkg-config --cflags x11 xfixes cairo)
X11_LIBS = $(shell pkg-config --libs x11 xfixes cairo) 

CFLAGS += -Wall -Wextra -pedantic -std=gnu99 $(X11_CFLAGS) 
LDFLAGS += $(X11_LIBS)

PREFIX ?= /usr
EXEC_PREFIX ?= $(PREFIX)/bin

.PHONY: all
all: $(XCORNERS)

$(XCORNERS): *.c
	$(CC) $(CFLAGS) -MMD -MP $^ -o $@ $(LDFLAGS) 

.PHONY: install
install: $(XCORNERS)
	install -v -D -m 755 $^ $(EXEC_PREFIX)/$(XCORNERS)

.PHONY: uninstall
uninstall:
	rm -f $(EXEC_PREFIX)/$(XCORNERS)

.PHONY: clean
clean:
	rm $(XCORNERS) *.d

