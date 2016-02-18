#include "handshake.hpp"

using namespace std;

int Handshake::init(int _sockfd, unsigned char *_infoHash, bt_args_t *_bt_args,
        char * _sendid, char * _recvid){
	sockfd = _sockfd;
	infoHash = _infoHash;
	bt_args = _bt_args;
    is_finished = false;
	step = 0;
	max_buf_len = 1024;
	input = new unsigned char[max_buf_len];
	output = new unsigned char[max_buf_len];
    sendid = _sendid;
    recvid = _recvid;
    return 0;
}

bool Handshake::finished(){
	return is_finished;
}

int Handshake::next_step(){
	int result = -1;
	if (bt_args->seed == 0){
		if (step == 0)
			result = first_step_send();
		else if (step == 1)
			result = first_step_recv();
		else if (step == 2)
			result = second_step_send();
		else if (step == 3)
			result = second_step_recv();
		else if (step == 4)
			result = third_step_send();
		else if (step == 5)
			result = third_step_recv();
		else if (step == 6)
			result = fourth_step_send();
		else if (step == 7)
			result = fourth_step_recv();
	}
	else{
		if (step == 0)
			result = first_step_recv();
		else if (step == 1)
			result = first_step_send();
		else if (step == 2)
			result = second_step_recv();
		else if (step == 3)
			result = second_step_send();
		else if (step == 4)
			result = third_step_recv();
		else if (step == 5)
			result = third_step_send();
		else if (step == 6)
			result = fourth_step_recv();
		else if (step == 7)
			result = fourth_step_send();
	}
	step++;
	if (step == 8)
		is_finished = true;
	return result;
}

int Handshake::first_step_send(){
	len = 20;
	output[0] = 19;
	strcpy((char *)output+1, "BitTorrent Protocol");
	if (write(sockfd, output, len) != len){
		if (bt_args->verbose) cout << "Handshake: Step 1 send: write error" << endl;
		return -1;
	}
	if (bt_args->verbose) cout << "Handshake: Step 1 send: successful" << endl;
	return 0;
}

int Handshake::first_step_recv(){
	len = 20;
	int index = 0;
	while (index < len){
		if ((index += read(sockfd, input+index, max_buf_len)) < 0){
			if (bt_args->verbose) cout << "Handshake: Step 1 recv: read error" << endl;
			return -1;
		}
	}
	if (input[0] == 19 && strncmp((char *)input+1, "BitTorrent Protocol", 19) == 0){
		if (bt_args->verbose) cout << "Handshake: Step 1 recv: successful" << endl;
		return 0;
	}
	if (bt_args->verbose) cout << "Handshake: Step 1 recv: match error" << endl;
	return -1;
}

int Handshake::second_step_send(){
	len = 8;
	memset(output, 0, len);
	if (write(sockfd, output, len) != len){
		if (bt_args->verbose) cout << "Handshake: Step 2 send: write error" << endl;
		return -1;
	}
	if (bt_args->verbose) cout << "Handshake: Step 2 send: successful" << endl;
	return 0;
}

int Handshake::second_step_recv(){
	len = 8;
	int index = 0;
	while (index < len){
		if ((index += read(sockfd, input+index, max_buf_len)) < 0){
			if (bt_args->verbose) cout << "Handshake: Step 2 recv: read error" << endl;
			return -1;
		}
	}
	memset(output, 0, len);
	if (memcmp(input, output, len) == 0){
		// Marking this peer is interested
		//bt_args->peers[sockfd]->interested = 1;
		if (bt_args->verbose) cout << "Handshake: Step 2 recv: successful" << endl;
		return 0;
	}
	if (bt_args->verbose) cout << "Handshake: Step 2 recv: match error" << endl;
	return -1;
}

int Handshake::third_step_send(){
	len = 20;
	memcpy(output, infoHash, len);
	if (write(sockfd, output, len) != len){
		if (bt_args->verbose) cout << "Handshake: Step 3 send: write error" << endl;
		return -1;
	}
	if (bt_args->verbose) cout << "Handshake: Step 3 send: successful" << endl;
	return 0;
}

int Handshake::third_step_recv(){
	len = 20;
	int index = 0;
	while (index < len){
		if ((index += read(sockfd, input+index, max_buf_len)) < 0){
			if (bt_args->verbose) cout << "Handshake: Step 3 recv: read error" << endl;
			return -1;
		}
	}

	if (memcmp(input, infoHash, len) == 0){
		if (bt_args->verbose) cout << "Handshake: Step 3 recv: successful" << endl;
		return 0;
	}
	if (bt_args->verbose) cout << "Handshake: Step 3 recv: match error" << endl;
	return -1;
}


int Handshake::fourth_step_send(){
	len = 20;
	memcpy(output, sendid, len);
	if (write(sockfd, output, len) != len){
		if (bt_args->verbose) cout << "Handshake: Step 4 send: write error" << endl;
		return -1;
	}
	if (bt_args->verbose) cout << "Handshake: Step 4 send: successful" << endl;
	return 0;
}

int Handshake::fourth_step_recv(){
	len = 20;
	int index = 0;
	while (index < len){
		if ((index += read(sockfd, input+index, len-index)) < 0){
			if (bt_args->verbose) cout << "Handshake: Step 4 recv: read error" << endl;
			return -1;
		}
	}

	if (index == len){
		// verify the peer id
		if (memcmp(input, recvid, len) == 0){
			if (bt_args->verbose) cout << "Handshake: Step 4 recv: successful" << endl;
			return 0;
		}
	}
	if (bt_args->verbose) cout << "Handshake: Step 4 recv: match error" << endl;
	return 0;
}
