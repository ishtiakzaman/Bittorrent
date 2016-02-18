SRC = $(wildcard *.cpp)
CC = g++
CFLAGS = -g
LD_FLAGS = -lssl -lcrypto
bt_client: $(SRC)
	$(CC) -o $@ $^ $(CFLAGS) $(LD_FLAGS)

.PHONY :clean

clean :
	rm -f bt_client
