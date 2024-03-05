XCORNERS := xcorners

CFLAGS += -Wall -Wextra -pedantic -std=gnu99 $(shell pkg-config --cflags x11 xfixes cairo)
LDFLAGS += $(shell pkg-config --libs x11 xfixes cairo)

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

