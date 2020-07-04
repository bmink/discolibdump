P = discolibdump
OBJS = main.o hiredis_helper.o cJSON.o cJSON_helper.o
CFLAGS = -g -Wall -Wstrict-prototypes -Werror
LDLIBS = -lb -lhiredis -lcurl

$(P): $(OBJS)
	$(CC) -o $(P) $(LDFLAGS) $(OBJS) $(LDLIBS)

clean:
	rm -f *o; rm -f $(P)
