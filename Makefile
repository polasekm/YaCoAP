CFLAGS += -fPIC -std=c99 -Wall -Wextra -Werror -O2 -I.
LDFLAGS = -shared
DIRS = example tests
SRC = coap.c coap_dump.c coap_parse.c
OBJ = $(SRC:%.c=%.o)
DEPS = $(SRC:%.c=%.d)
TARGET_LIB = libyacoap.so # target lib

.PHONY: all
all: ${TARGET_LIB}

-include $(DEPS)

$(TARGET_LIB): $(OBJ)
	$(CC) ${LDFLAGS} -o $@ $^

%.o: %.c %.d
	@$(CC) -c $(CFLAGS) -o $@ $<

%.d: %.c
	@$(CC) -MM $(CFLAGS) $< > $@

clean:
	@$(RM) $(TARGET_LIB) $(OBJ) $(DEPS)
