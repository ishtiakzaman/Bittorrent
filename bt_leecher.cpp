#include "bt_leecher.hpp"

using namespace std;

int Leecher::init(bt_args_t _args, int _numPieces, int _fsize, int _psize,
        string _fName, unsigned char * _keys, Message _msg){
    args = _args;
    numPieces = _numPieces;
    msg = _msg;
    pSize = _psize;
    fSize = _fsize;
    fName = _fName;
    keys = _keys;
	downloadedSize = 0;
	blockSize = 1024;
	blockPerPiece = pSize / blockSize;
	totalBlocks = (fSize % blockSize == 0? fSize / blockSize : fSize / blockSize + 1);
	blockStatus = new int[totalBlocks];
	for(int i = 0; i < totalBlocks; ++i)
		blockStatus[i] = 0;
    return 0;
}

int Leecher::start(){
    //get the bitfield
    int bitSize = ceil(numPieces/8.0);
    int len=0, recLen=0;
    char * bits = (char *)malloc(bitSize*sizeof(char));
    if(args.verbose)
        DBG(<<"Leecher start: Waiting for bitfield"<<endl);
	for(int i = 0; i < args.n_peer; ++i)
		recvBitField(args.sockets[i], bits, bitSize);
    fileOpen(fName);
    requestPieces();
    pFp.close();
    return 0;
}

int Leecher::fileOpen(string name){
    recvName = args.save_file + name + "_recv";
	pFp.open(recvName.c_str(), ios::out|ios::binary);
}

int Leecher::writeToFile(unsigned char * buf, int num, int len, fstream& file){
    //DBG(<<"writing at offset "<<num*pSize <<" len "<<len<<endl);
    file.seekp(num*blockSize);
    file.write((char *)buf, len);
	return 0;
}

int Leecher::recvBitField(int sock, char * bits, int bitSize){
    char buf[1456] = {0};
    int recLen=0, len=0;
    do{
		if((len = recv(sock, buf, sizeof(buf), 0))<0){
			perror("error in recv");
			exit(1);
		}
		memcpy(&recLen, buf, sizeof(int));
    }while(len<recLen);
    char type = 1;
    if(memcmp(&buf[4], &type, 1)){
        DBG(<<"wrong recv"<<endl);
        exit(1);
    }
    memcpy(bits, &buf[5], bitSize);
    //DBG(<<"leecher recv bitfield "<<(bitset<8>)*bits<<endl);
}

int Leecher::requestPieces(){
	while(1){
		int blockIndex = rand() % totalBlocks;
		int i, j;
		i = blockIndex;
		for(j = 0; j < 100; ++j){			
			if (blockStatus[i] == 0){
				if (getBlock(i) == 1) // getBlock returns 1 when the full file has been downloaded
					return 0;
				break;
			}
			i = (i+1)%totalBlocks;
		}
	}
}

int Leecher::getBlock(int blockIndex){
	// getBlock returns 1 when the full file has been downloaded
    int recvLen, pieceIndex;
    unsigned char *blockData = new unsigned char[1456];
	//DBG( << "get block " << blockIndex << endl);
	if(blockIndex*blockSize + blockSize <= fSize){
		recvLen = blockSize;
	}
	else{
		recvLen = fSize - blockIndex*blockSize;
	}

	pieceIndex = blockIndex/blockPerPiece;

	// Select a seeder randomly
	int seeder = rand()%args.n_peer;
	msg.createRequest(args.sockets[seeder], pieceIndex, (blockIndex%blockPerPiece)*blockSize, recvLen);
	recvPiece(args.sockets[seeder], blockData);
	writeToFile(blockData, blockIndex, recvLen, pFp);
	downloadedSize += recvLen;
	cout.precision(4);
	DBG( << "File: " << fName << ", Total Peers: " << args.n_peer << ", Getting " << recvLen << " Bytes From "
		<< args.peerArray[seeder] <<", Total Downloaded: " << downloadedSize << " Bytes, "
		<< "Completed: " << (downloadedSize * 1.0 / (1.0 * fSize)) * 100.0 << "%" << endl);
	blockStatus[blockIndex] = 1;

    //check sha
	if (isPieceComplete(pieceIndex) == 1){
        if(args.verbose)
		    DBG(<<"Piece "<<pieceIndex<<" fully received"<<endl);
		if(checkPieceSha(pieceIndex)){
			DBG(<<"SHA for piece "<<pieceIndex<<" failed"<<endl);
			exit(1);
		}
		if (fileComplete() == true)
			return 1;
	}

    free(blockData);
	return 0;
}

int Leecher::checkPieceSha(int index){
	int len = pSize;
    unsigned char dig[30] = {0};
    unsigned char * buf = new unsigned char[pSize+1];
    ifstream file;
    file.open(recvName.c_str(), ios::in|ios::binary);
    file.seekg(index*pSize);
    //DBG(<<"sha from " << recvName.c_str() << ":" <<index*pSize<<" to "<<index*pSize+len<<endl);
	if ( (index+1)*pSize > fSize ){ // last piece
		len = fSize % pSize;
	}
    file.read((char *)buf, len);
	//buf[len] = 0;
	//DBG( << buf << endl);
    SHA1((unsigned char*)buf, len, dig);
    /*for(int i=0;i<20;i++)
        printf("%02x ", dig[i]);
    DBG(<<endl);
    for(int i=0;i<20;i++)
        printf("%02x ", keys[num*20+i]);
	DBG(<<endl);*/
	if(memcmp(dig, &keys[index*20], 20)){
		DBG(<<"wrong sha digest received"<<endl);
		return -1;
    }
	else{
        if(args.verbose)
		    DBG(<<"SHA passed for piece index: " << index <<endl);
	}
    file.close();
    free(buf);
    return 0;
}

int Leecher::fileComplete(){
	for(int i = 0; i < totalBlocks; ++i){
		if (blockStatus[i] == 0){
			return 0;
		}
	}
	return 1;
}

int Leecher::isPieceComplete(int index){
	for(int i = index*blockPerPiece; i < totalBlocks && i < (index+1)*blockPerPiece; ++i){
		if (blockStatus[i] == 0){
			return false;
		}
	}
	return true;
}

int Leecher::recvPiece(int sock, unsigned char * piece){
    int len=0, totLen=0, recvLen;
    char buf[1456];
    int check=0;
    int index=0, begin = 0;
    do{
        if((len = recv(sock, &buf[len], sizeof(buf), 0)) < 0){
            DBG(<<"recv error "<<__func__<<endl);
            exit(1);
        }
		//DBG(<<__func__<<" len "<<len<<endl);
		if(!check){
			memcpy(&recvLen, buf, sizeof(int));
			//DBG(<<__func__<<" recvlen "<<recvLen<<endl);
			check = 1;
		}
		totLen += len;
    }while(totLen < recvLen);
    //DBG(<<"passed the dreaded while"<<endl);
    char type = 3;
    if(memcmp(&buf[4], &type, 1)){
        DBG(<<"wrong recv"<<endl);
        exit(1);
    }
    //DBG(<<__func__<<" recv type check pass"<<endl);
    memcpy(&index, &buf[5], sizeof(int));
    memcpy(&begin, &buf[9], sizeof(int));
    int writeLen = recvLen - 13;
    //DBG(<<"copyting at "<<*piece<<" "<<writeLen<<endl);
    memcpy(piece, &buf[13], writeLen);
	return writeLen;
}
