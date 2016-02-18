#include<math.h>
#include<string.h>
#include<iostream>
#include<bitset>
#include<fstream>
#include <stdlib.h>
#include<openssl/sha.h>
#include "bt_lib.hpp"
#include "bt_message.hpp"

class Leecher{
    bt_args_t args;
    int numPieces;
    int fSize;
    int pSize;
    std::string fName;
    std::fstream pFp;
	long downloadedSize;
    unsigned char *keys;
	int blockSize;
	int totalBlocks;
	int blockPerPiece;
    Message msg;
	std::string recvName;
    int recvBitField(int, char *, int);
    int requestPieces();    
	int getBlock(int);
    int recvPiece(int, unsigned char*);
    int fileOpen(std::string);
    int writeToFile(unsigned char *, int, int, std::fstream&);    
	int checkPieceSha(int);
	int isPieceComplete(int);
	int fileComplete();
	int *blockStatus;
    public:
        int start();
        int init(bt_args_t, int, int, int, std::string, unsigned char *, Message);
};
