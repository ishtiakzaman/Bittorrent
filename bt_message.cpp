#include "bt_message.hpp"
#include <math.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <bitset>
#include <stdio.h>

//TODO : Generic bit field
//
using namespace std;

void bitFieldCalc(char * bits, int num){
    char * set = (char *)malloc(sizeof(char));
    int size = num;
    int tot = num;
    for(int i=0;i<ceil(tot/8.0);i++){
        num -= 8;
        memset(set, 0, sizeof(char));
        if(num>0){
            *set = ~(*set & 0);
            memcpy(&bits[i], set, sizeof(char));
        }
        else{
            num = abs(num);
            *set = ~((1<<num)-1);
            memcpy(&bits[i], set, 1);
        }
    }
	free(set);
}

int bitFieldSend(int sock, int numPieces){
    int bitSize = ceil(numPieces/8.0);
    if(verboseOut)
        DBG(<<"bitSize is "<<bitSize<<endl);
    int len = bitSize + 4 + 1;//length + type
    char * bits = (char *)malloc(bitSize*sizeof(char));
    memset(bits, 0, bitSize);
    char * sendBits = (char *)malloc(len*sizeof(char));
    memset(sendBits, 0, len);
    bitFieldCalc(bits, numPieces);
    memcpy(sendBits, &len, sizeof(int));
    memcpy(&sendBits[5], bits, bitSize);
    char type = 1;
    memcpy(&sendBits[4], &type, 1);
    if(verboseOut)
        DBG(<<"sending.."<<endl);
    if(send(sock, sendBits, bitSize+5, 0) != len){
        DBG(<<"send error"<<__func__<<endl);
        exit(1);
    }
    if(bits)
        free(bits);
    if(sendBits)
        free(sendBits);
}

int Message::createRequest(int sock, int index, int begin, int len){
    int packLen = 4*sizeof(int) + sizeof(char);
    char * sendBits = (char *)malloc(packLen);
    memset(sendBits, 0, packLen);
    memcpy(sendBits, &packLen, sizeof(int));
    char type = 2;
    memcpy(&sendBits[4], &type, sizeof(char));
    memcpy(&sendBits[5], &index, sizeof(int));
    memcpy(&sendBits[9], &begin, sizeof(int));
    memcpy(&sendBits[13], &len, sizeof(int));
    if(send(sock, sendBits, packLen, 0) != packLen){
        DBG(<<"send error"<<__func__<<endl);
        exit(1);
    }
    if(sendBits)
        free(sendBits);
}

int createPiece(int sock, int index, int begin, int length, char * buf){
    int packLen = 3*sizeof(int) + sizeof(char)+length;
    char * sendBits = (char *)malloc(packLen);
    memset(sendBits, 0, packLen);
    memcpy(sendBits, &packLen, sizeof(int));
    char type = 3;
    memcpy(&sendBits[4], &type, sizeof(char));
    memcpy(&sendBits[5], &index, sizeof(int));
    memcpy(&sendBits[9], &begin, sizeof(int));
    memcpy(&sendBits[13], buf, length*sizeof(char));
    if(verboseOut)
        DBG(<<"create piece "<<begin<<" "<<length<<endl);
    /*if(index >= 0){
    DBG(<<"sending "<< length<<endl);
    for(int j=13;j<packLen;j++){
        printf("%x", buf[j]);
    }
    DBG(<<endl);
    }*/
    if(send(sock, sendBits, packLen, 0) != packLen){
        DBG(<<"send error"<<__func__<<endl);
        return -1;
    }
    if(verboseOut)
        DBG(<<"seeder sent packet"<<" "<<packLen<<endl);
    if(sendBits)
        free(sendBits);
    return 0;
}
