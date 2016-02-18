#ifndef MESSAGE_H
#define MESSAGE_H

#include <fstream>
#include <iostream>

extern std::ofstream logFilePtr;
extern bool verboseOut;

#define DBG(x) do{cout x; logFilePtr x;} while(0)

void bitFieldCalc(char *, int);
int bitFieldSend(int, int);
int createPiece(int, int, int , int, char *);
class Message{
        int sock;
    public:
        int createRequest(int, int, int , int);
};
#endif
