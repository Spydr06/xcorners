XCORNERS := xcorners

CFLAGS += -Wall -Wextra -pedantic -std=c99 $(shell pkg-config --cflags x11 xfixes xcomposite cairo)
LDFLAGS += $(shell pkg-config --libs x11 xfixes xcomposite cairo)

.PHONY: all
all: $(XCORNERS)

$(XCORNERS): *.c
	$(CC) $(CFLAGS) $(LDFLAGS) -MMD -MP $^ -o $@
