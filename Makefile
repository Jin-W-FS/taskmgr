CFLAGS=-g $(USER_CFLAGS)
LDFLAGS=-lpthread -lrt $(USER_LDFLAGS)

TARGET=taskmgr
SOURCES=taskmgr.c
OBJECTS=$(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET):$(OBJECTS)
	$(CC) $^ $(LDFLAGS) -o $@

include $(SOURCES:.c=.d)
%.d: %.c
	@set -e; rm -f $@; \
	$(CC) -MM $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

.PHONY:all clean

clean:
	-rm $(TARGET) $(OBJECTS)
