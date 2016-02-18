#include <iostream>
#include <fstream>
#include <vector>
#include <stdlib.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "bt_lib.hpp"

class Handshake{
        int sockfd;
		unsigned char *input, *output;
		int len;
		int max_buf_len;
		bool is_finished;
		int step;			// There would be 8 steps 0 to 7
		unsigned char *infoHash;
		bt_args_t *bt_args;
		peer_t *peer;
        char *sendid, *recvid;

		int first_step_send();
		int first_step_recv();
		int second_step_send();
		int second_step_recv();
		int third_step_send();
		int third_step_recv();
		int fourth_step_send();
		int fourth_step_recv();
    public:
		bool finished();
        int init(int _sockfd, unsigned char *_infoHash, bt_args_t *_bt_args, char*, char*);
		int next_step();
};
