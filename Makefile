XCORNERS := xcorners

CFLAGS += -Wall -Wextra -pedantic -std=gnu99 $(shell pkg-config --cflags x11 xfixes xcomposite cairo) -O3
LDFLAGS += $(shell pkg-config --libs x11 xfixes xcomposite cairo)

.PHONY: all
all: $(XCORNERS)

$(XCORNERS): *.c
	$(CC) $(CFLAGS) -MMD -MP $^ -o $@ $(LDFLAGS) 

.PHONY: clean
clean:
	rm $(XCORNERS) *.d

