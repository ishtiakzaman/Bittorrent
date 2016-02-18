#include "bt_message.hpp"
#include "bt_lib.hpp"
#include <iostream>
#include <fstream>

int recvRequestPkt(int, int&, int&, int&);
int sendPieces(int, int&, int&, int&, std::ifstream&, int, long &, int);
class Seeder{
    bt_args_t args;
    int numPieces;
    Message msg;
    int fileSize;
    int pieceSize;
    std::string fName;
    int transferData(int);
    public:
        int start();
        int init(bt_args_t _args, int _numPieces, int, int, std::string, Message _msg);
};
