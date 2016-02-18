#include "bt_seeder.hpp"
//TODO : remove len in piecepkt
using namespace std;

int Seeder::init(bt_args_t _args, int _numPieces, int _fileSize, int _pieceSize, string _fName, Message _msg){
    args = _args;
    numPieces = _numPieces;
    msg = _msg;
    fName = _fName;
    fileSize = _fileSize;
    pieceSize = _pieceSize;
    return 0;
}

int Seeder::start(){
    int sock = args.sockets[0];
    //As handshake is done, send bitfield
    //msg.bitFieldSend(numPieces);
    transferData(sock);
    return 0;
}

int Seeder::transferData(int sock){
    int index=0, begin=0, length=0, pSize;
	long uploadedSize;
    ifstream file;
    file.open(fName.c_str(), ios::in| ios::binary);
    while(1){
        if(args.verbose)
		    DBG(<<"seeder waitin for req"<<endl);
		recvRequestPkt(sock, index, begin, length);
        if(args.verbose)
		    DBG(<<"seeder request recv"<<index<<" "<<begin<<" "<<length<<" "<<endl);
		if(sendPieces(sock, index, begin, length, file, 0, uploadedSize, pSize))
			break;
    }
    if(args.verbose)
        DBG(<<"Seeder Done"<<endl);
    file.close();
}

int sendPieces(int sock, int &index, int &begin, int &length, ifstream& file,
        int fileSize, long &uploadedSize, int pSize){
    char buf[1456] = {0};
    int offset = index*pSize + begin;
    file.seekg(offset);
    if(verboseOut)
        DBG(<<"length is "<<length<<endl);
    file.read(buf, length);

    if(createPiece(sock, index, begin, length, buf))
        return -1;
	uploadedSize += length;
    if(begin+length==fileSize){
		//DBG(<<begin+length<<" "<<fileSize;
        return 1;
    }
    return 0;
}
int recvRequestPkt(int sock, int &index, int &begin, int &length){
    char buf[1456] = {0};
    int recLen=0, len=0;
    do{
		if((len = recv(sock, buf, sizeof(buf), 0))<0){
			DBG(<<"error in recv"<<endl);
			return -1;
		}
		if (len == 0){ // The leecher is closing this socket
            if(verboseOut)
			    DBG( <<  "This socket has been closed by the leecher" << endl);
			return 1;
		}
		memcpy(&recLen, buf, sizeof(int));
		//DBG(<<"reclen is "<<recLen<<endl);
    }while(len<recLen);
    char type = 2;
    //DBG(<<buf[4]<<endl);
    if(memcmp(&buf[4], &type, 1)){
        DBG(<<"wrong recv got "<<hex<<buf[4]<<endl);
        return -1;
    }
    memcpy(&index, &buf[5], sizeof(int));
    memcpy(&begin, &buf[9], sizeof(int));
    memcpy(&length, &buf[13], sizeof(int));
    return 0;
    //DBG(<<"seeder recv req for "<< index <<" "<<begin<<" "<< length<<endl);
}
