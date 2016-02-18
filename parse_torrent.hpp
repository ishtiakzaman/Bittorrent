#include <iostream>
#include <fstream>
#include <vector>
#include <stdlib.h>
#include <cstring>
#include <openssl/sha.h> //hashing pieces

#define FILENAMESIZE 20
#define ANNOUNCELEN 8
#define NAMELEN 4
#define LENGTHLEN 6
#define CREATEDATELEN 13
#define PIECELEN 12

typedef enum error{
    ERROR = -1,
    OK
}retCode;

class ParseTorrent{
		std::string getInfo(std::string, int pos);
        long benIntegerDecode(std::string line, int pos);
        long getFileLength(std::string, int pos);
        int getDetailsTorrent(std::string);
        std::string getAnnounce(std::string, int);
        std::string getFileName(std::string, int);
        int benLengthGet(std::string line, int & pos);
        int digestStore(std::string, int);
		void printHexaData(unsigned char *data, int index, int len);
        long fileLength;
        long pieceLength;
        long creationDate;
        int numPieces;
        std::string announce;
		std::string info;
        std::string fileName;
        std::string torrentFileName;
        unsigned char *shaList;
		unsigned char infoHash[20];
        int totalKeyLen;
    public:
        int scanTorrentFile(char *);
        int printTorrentInfo();
		unsigned char *getInfoHash();
        int getNumPieces();
        int getFileSize();
        int getPieceSize();
        std::string getFileName();
		unsigned char *getKeys();
};
