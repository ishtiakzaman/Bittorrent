#include "parse_torrent.hpp"

using namespace std;

int ParseTorrent::scanTorrentFile(char * fileName){
    string content;
    torrentFileName = fileName;
    ifstream torrentFile (fileName);
    if(torrentFile.is_open()){
        for(string line; getline(torrentFile, line);){
            content.append(line);
                }
    }
    else{
        cout<<"error opening torrent file";
        exit(1);
    }
    torrentFile.close();
    getDetailsTorrent(content);
    return 0;
}

unsigned char *ParseTorrent::getInfoHash(){
	return infoHash;
}

int ParseTorrent::getDetailsTorrent(string line){
    int pos;
    retCode out = ERROR;
	if((pos = line.find(":info")) != string::npos){
        info = getInfo(line, pos);
		SHA1((unsigned char *) info.c_str(), info.length(), (unsigned char *) infoHash);
    }
    else
        goto done;
    if((pos = line.find(":announce")) != string::npos){
        announce = getAnnounce(line, pos+ANNOUNCELEN+1);
    }
    else
        goto done;
    if((pos = line.find(":name")) != string::npos){
        fileName = getFileName(line, pos+NAMELEN+1);
    }
    else
        goto done;
    if((pos = line.find(":length")) != string::npos){
        fileLength = benIntegerDecode(line, pos+LENGTHLEN+1);
    }
    else
        goto done;
    if((pos = line.find(":creation date")) != string::npos){
        creationDate = benIntegerDecode(line, pos+CREATEDATELEN+1);
    }
    else
        goto done;
    if((pos = line.find(":piece length")) != string::npos){
        pieceLength = benIntegerDecode(line, pos+PIECELEN+1);
    }
    else
        goto done;
    if((pos = line.find(":pieces")) != string::npos){
        digestStore(line, pos+LENGTHLEN+1);
    }
    else
        goto done;
    out = OK;
done:
    if(out == ERROR){
        cout << "Wrong bencode format\n";
        exit(1);
    }
    return out;
}

string ParseTorrent::getInfo(string line, int pos){
    string s;
    s = line.substr( pos, string::npos);
    return s;
}

string ParseTorrent::getAnnounce(string line, int pos){
    string s;
    s = line.substr( pos, benLengthGet(line, pos));
    return s;
}

string ParseTorrent::getFileName(string line, int pos){
    string s;
    s = line.substr( pos, benLengthGet(line, pos));
    return s;
}

int ParseTorrent::benLengthGet(string line, int & pos){
    int i = 0;
    char numDig[20]={0};
    //cout <<"start with "<<numDig<<endl;
    for (int j=pos;j<line.length();j++){
        if((line[j] > 47 && line[j]<58) || (line[j] == ':')){
            if(line[j] == ':'){
                pos++;
                break;
            }
            numDig[i] = line[j];
            //cout << "added "<<numDig[i]<<" to make "<<numDig<<endl;
            i++;
            pos++;
        }
        else{
            cout << "error parsing\n";
            exit(1);
        }
    }
    //cout <<"lenret "<<atoi(numDig);
    return atoi(numDig);
}

long ParseTorrent::benIntegerDecode(string line, int pos){
    char numDig[32]={0};
    int i=0;
    if(line[pos] != 'i'){
        cout << "Wrong bencode format\n";
        exit(1);
    }
    else {
        pos++;
    }
    for (int j=pos;j<line.length();j++){
        if((line[j] > 47 && line[j]<58) || (line[j] == 'e')){
            if(line[j] == 'e'){
                break;
            }
            numDig[i] = line[j];
            i++;
        }
        else{
            cout << "Wrong bencode format\n";
            exit(1);
        }
        numDig[i]='\0';
    }
    return atol(numDig);
}

int ParseTorrent::digestStore(string line, int pos){
    totalKeyLen = benLengthGet(line, pos);
    //cout<<"key length is "<<totalKeyLen<<endl;
    int numKeys = totalKeyLen/20;
    char key[20] = {0};
    if(totalKeyLen%20 != 0){
        cout<<"Error in digest info\n";
        exit(1);
    }
    cout<<"opening "<<torrentFileName.c_str()<<endl;
	ifstream torrentFile (torrentFileName.c_str());
    if(torrentFile.is_open()){
		torrentFile.seekg(pos, torrentFile.beg);
		shaList = (unsigned char *)malloc((totalKeyLen+1)*sizeof(unsigned char));
        torrentFile.read((char *)shaList, totalKeyLen);
    }
    else{
        cout<<"error opening torrent file";
        exit(1);
    }
    torrentFile.close();

    numPieces = numKeys;
    return 0;
}

int ParseTorrent::printTorrentInfo(){
    cout << "Torrent parsing result below:" << endl << "announce: " << announce << endl << "file name: "
		<< fileName << endl <<
        "file length: " << fileLength << endl << "creation date: " << creationDate << endl
        << "piece length: " << pieceLength << endl;
    cout << "Keys: " << endl;
    for(int i=0; i < totalKeyLen; i++){
		if (i%20==0) cout << "0x";
		printf("%02x", shaList[i]);
        if((i+1)%20==0)
            cout<<endl;
    }
    cout<<endl;
}

int ParseTorrent::getNumPieces(){
    return numPieces;
}

int ParseTorrent::getFileSize(){
    return fileLength;
}

int ParseTorrent::getPieceSize(){
    return pieceLength;
}

string ParseTorrent::getFileName(){
    return fileName;
}

unsigned char * ParseTorrent::getKeys(){
    return shaList;
}
