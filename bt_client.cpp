#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <netinet/ip.h> //ip hdeader library (must come before ip_icmp.h)
#include <netinet/ip_icmp.h> //icmp header
#include <arpa/inet.h> //internet address library
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>


#include "bt_setup.hpp"
#include "parse_torrent.hpp"
#include "bt_seeder.hpp"
#include "bt_leecher.hpp"
#include "handshake.hpp"
#include "bt_message.hpp"

#define ThrowError(x)\
    {perror(x); exit(0);}

using namespace std;

ofstream logFilePtr;
bool verboseOut = false;

void printIds(char * send, char * recv){
    DBG(<<"send id"<<endl);
    for(int i=0;i<20;i++)
        printf("%02x ", send[i]);
    DBG(<<endl);
    DBG(<<"recv id"<<endl);
    for(int i=0;i<20;i++)
        printf("%02x ", recv[i]);
    DBG(<<endl);
}

int main (int argc, char * argv[]){
    int sockfd, newfd;
    struct sockaddr_in clientaddr; //client address (required for server part)
    unsigned int client_add_len;
	long uploadedSize = 0;
    bt_args_t bt_args;
    int i, optval=1, maxFd;
    fd_set prime;
    fd_set update;
    int index=0, begin=0, length=0;
    parse_args(&bt_args, argc, argv);

    ParseTorrent torDetails;
    torDetails.scanTorrentFile(bt_args.torrent_file);

    if(bt_args.verbose){

        printf("Args:\n");
        printf("verbose: %d\n",bt_args.verbose);
        printf("save_file: %s\n",bt_args.save_file);
        printf("torrent_file: %s\n", bt_args.torrent_file);
        printf("No of peer: %d\n", bt_args.n_peer);
    }

    logFilePtr.open(bt_args.logFile);

    if(bt_args.verbose)
        verboseOut = true;

    //read and parse the torrent file here

    if (bt_args.verbose){
        // print out the torrent file arguments here
        torDetails.printTorrentInfo();
    }
    FD_ZERO(&prime);
    FD_ZERO(&update);


    /* Establish the TCP connection to the peer */
    if (bt_args.seed){//if it is a seeder start and wait for connections

		if((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
			ThrowError("Socket creation failed");

		setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

        memset(&bt_args.ownaddr, 0, sizeof(bt_args.ownaddr)); 		// Zero out structure
        bt_args.ownaddr.sin_family = AF_INET; 						// Internet Adress Family
        bt_args.ownaddr.sin_addr.s_addr = htonl(INADDR_ANY); 		// Any incoming interface

        // Try binding to an available port
        int port;
        for (port = INIT_PORT; port <= MAX_PORT; port++){
            bt_args.ownaddr.sin_port = htons(port);
            /* Bind to its own address */
            if (bind(sockfd, (struct sockaddr *)&(bt_args.ownaddr), sizeof(bt_args.ownaddr)) < 0){
                if (bt_args.verbose)
                    DBG( << "bind() failed with port " << port << ", trying with next port" << endl);
            }
            else{
                if (bt_args.verbose)
                    DBG( << "Binded successfully to port: " << port << endl);
                break;
            }
        }


        if (port > MAX_PORT){
            ThrowError("Bind() failed for all ports");
        }

        /* Mark the socket so it will listen for incoming connections */
        if (listen(sockfd, MAX_CONNECTIONS) < 0)
            ThrowError("listen() failed");
        FD_SET(sockfd, &prime);
        maxFd = sockfd;
        peer_t * seeder = new peer_t();

        if(bt_args.verbose)
            DBG(<<"INSERTING "<<sockfd<<endl);
        bt_args.peers[sockfd] = seeder;

        typedef map<int, peer_t*>::iterator sock_iter;
        client_add_len = sizeof(clientaddr);
        ifstream pFp;
        pFp.open(torDetails.getFileName().c_str(), ios::in| ios::binary);
        int del=0;
        while(1){
            update = prime;
            if(bt_args.verbose)
			    DBG( << "Waiting for any socket read in select()" << endl);
            if(select(maxFd+1, &update, NULL, NULL, NULL) < 0){
                DBG(<<"select error"<<endl);
                exit(1);
            }
            if(del>0){
                delete(bt_args.peers[del]);
                bt_args.peers.erase(del);
                del = 0;
            }
            for(sock_iter iter=bt_args.peers.begin(); iter != bt_args.peers.end(); iter++){
                int currentSock = iter->first;
                if (FD_ISSET(currentSock, &update)){
                    if(currentSock == sockfd){
                        if(bt_args.n_peer+1 > MAX_CONNECTIONS){
                            DBG( << "ERROR: Can only support " << MAX_CONNECTIONS << " initial peers" << endl);
                            continue;
                        }
                        /* Wait for a client to connect */
                        if ((newfd = accept(sockfd, (struct sockaddr *) &clientaddr,
                                        &client_add_len)) < 0)
                            ThrowError("accept() failed");
                        if (bt_args.verbose)
                            DBG( << "Accepted connection from peer: " << inet_ntoa(clientaddr.sin_addr) <<
                                ":" << ntohs(clientaddr.sin_port) << endl);
                        if(newfd<0){
                            DBG(<<"error accepting connection"<<endl);
                        }
                        else{
                            FD_SET(newfd, &prime);
                            if(newfd > maxFd)
                                maxFd = newfd;

							if(bt_args.n_peer == 0){
								//setting the peer id of seeder
								struct sockaddr_in local_address;
								int addr_size = sizeof(local_address);
								getsockname(newfd, (sockaddr *)&local_address, (socklen_t *)&addr_size);
								if (bt_args.verbose)
									DBG( << "Seeder connecting from : " << inet_ntoa(local_address.sin_addr) <<
										":" << ntohs(local_address.sin_port) << endl);

								// Make the id with SHA1 of IP:port
								unsigned char data[256];
								int idlen = snprintf((char *)data, 256, "%s%u", inet_ntoa(local_address.sin_addr), ntohs(local_address.sin_port));
								//id is just the SHA1 of the ip and port string
								//DBG(<<"sha calc "<<data<<endl);
								SHA1(data, idlen, bt_args.id);
							}
                        }
                        // Set the leecher as a peer
                        peer_t * leecher= new peer_t();
                        // Set the handshake false
                        leecher->handshake = false;


                        //get the peer id of leecher
                        char peer_id[50];
                        sprintf(peer_id, "%s:%d", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
                        __parse_peer(leecher, peer_id);
                        if(bt_args.verbose)
                            DBG(<<"INSERTING "<<newfd<<endl);
                        bt_args.peers[newfd] = leecher;
                        //increment the peer count
                        bt_args.n_peer = bt_args.n_peer + 1;

                    }
                    else{
                        if(iter->second->handshake == false){
                            if(bt_args.verbose)
                                DBG(<<"Working on handshake"<<endl);
                            Handshake hand_shake;
                            //printIds((char *)bt_args.id, (char *)bt_args.peers[currentSock]->id);
                            hand_shake.init(currentSock, torDetails.getInfoHash(),
                                    &bt_args, (char *)bt_args.id, (char *)bt_args.peers[currentSock]->id);
                            if (bt_args.verbose) DBG( << "Starting Main Loop" << endl);
                            while(1){
                                if (hand_shake.finished() == false){
                                    if (hand_shake.next_step() < 0){
                                        ThrowError("handshake failed");
                                    }
                                }
                                else{
                                    break;
                                }
                            }
                            if (bt_args.verbose)
                                DBG(<<"Handshake Successful"<<endl);
                            bitFieldSend(currentSock, torDetails.getNumPieces());
                            iter->second->handshake = true;
                        }
                        //expect data and transfer it
                        if(recvRequestPkt(currentSock, index, begin, length)){
                            if(bt_args.verbose)
							    DBG( << "Leecher is finished, closing socket" << endl);
                            close(currentSock);
                            FD_CLR(currentSock, &prime);
                            del = currentSock;
                            bt_args.n_peer--;
							continue;
                        }

						int result = sendPieces(currentSock, index, begin, length, pFp,
										torDetails.getFileSize(), uploadedSize, torDetails.getPieceSize());

                        if(result){
							if (result == 1){
								DBG( << "File: " << torDetails.getFileName() << ", Total Peers: " << bt_args.n_peer <<
									", Sending " << length << " Bytes To " << bt_args.peers[currentSock]->name <<
									", Total Uploaded: " << uploadedSize << " B" << endl);
							}
							else{
								DBG( << "Sending failed, closing socket" << endl);
								close(currentSock);
								FD_CLR(currentSock, &prime);
								del = currentSock;
								bt_args.n_peer--;
							}
                        }
						if (result == 0){
							DBG( << "File: " << torDetails.getFileName() << ", Total Peers: " << bt_args.n_peer <<
								", Sending " << length << " Bytes To " << bt_args.peers[currentSock]->name <<
								", Total Uploaded: " << uploadedSize << " Bytes" << endl);
						}
                    }
                }
            }
        }
    }
    else{
        int noOfSeedConnected = 0;
        do{
            for(int i = 0; i < bt_args.n_peer; ++i){

                // Create multiple socket
                if((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
                    ThrowError("Socket creation failed");
                bt_args.sockets[noOfSeedConnected] = sockfd;
                setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

                peer_t * seeder = new peer_t();
                __parse_peer(seeder, bt_args.peerArray[i]);
                bt_args.peers[sockfd] = seeder;
                if (bt_args.verbose)
                    DBG( << "Trying to a make a TCP with peer : " << bt_args.peerArray[i] << endl);
                if(connect(sockfd, (struct sockaddr *) &(bt_args.peers[sockfd]->sockaddr),
                            sizeof(bt_args.peers[sockfd]->sockaddr)) < 0){

                    if (bt_args.verbose)
                        DBG( << "Cannot made TCP with this peer, possibly the peer is not initiated yet" << endl);
                    DBG( << "Could not connect to peer : " << bt_args.peerArray[i] << endl);
                }
                else{
                    DBG( << "Connected to peer : " << bt_args.peerArray[i] << endl);
                    ++noOfSeedConnected;
                }
                if(noOfSeedConnected == 0){
                    if (bt_args.verbose)
                        DBG( <<"Could not connect to any peer, sleeping for 5 seconds.");
                    sleep(5);
                }
            }
        }while(noOfSeedConnected==0);
        //cout << "Crossed the loop"<<endl;
		bt_args.n_peer = noOfSeedConnected;

        // Connected to the peer, send the peer the first handshake message
		for(int i = 0; i < bt_args.n_peer; ++i){
			Handshake hand_shake;
			// Need to get own ip/port, use the socket to own ip/port
			sockfd = bt_args.sockets[i];
			struct sockaddr_in local_address;
			int addr_size = sizeof(local_address);
			char recvid[20] = {0};
			getsockname(sockfd, (sockaddr *)&local_address, (socklen_t *)&addr_size);
            if(bt_args.verbose)
			    DBG( << "Leecher connecting from : " << inet_ntoa(local_address.sin_addr) <<
												":" << ntohs(local_address.sin_port) << endl);

			// Make the id with SHA1 of IP:port
			unsigned char data[256];
			int idlen = snprintf((char *)data, 256, "%s%u", inet_ntoa(local_address.sin_addr), ntohs(local_address.sin_port));
			//id is just the SHA1 of the ip and port string
			SHA1(data, idlen, bt_args.id);


			//printIds((char *)bt_args.id, (char*)bt_args.peers[sockfd]->id);
			hand_shake.init(sockfd, torDetails.getInfoHash(), &bt_args,
					(char *)bt_args.id, (char *)bt_args.peers[sockfd]->id);
			if (bt_args.verbose) DBG( << "Starting Handshake for peer : " << bt_args.peerArray[i] << endl);
			while(1){
				if (hand_shake.finished() == false){
					if (hand_shake.next_step() < 0){
						ThrowError("handshake failed");
					}
				}
				else{
					if (bt_args.verbose) DBG( << "Handshake completed for peer : " << bt_args.peerArray[i] << endl);
					break;
				}
			}
		}
    }

    //main client loop
    if(bt_args.verbose)
        DBG(<<"Getting details"<<endl);
    int numPieces = torDetails.getNumPieces();
    int fileSize = torDetails.getFileSize();
    int pieceSize = torDetails.getPieceSize();
    Message msg;
    if(bt_args.seed){
        Seeder seeder;
        seeder.init(bt_args, numPieces, fileSize, pieceSize,
                torDetails.getFileName(), msg);
        seeder.start();
    }
    else {
        Leecher leecher;
        leecher.init(bt_args, numPieces, fileSize, pieceSize,
                torDetails.getFileName(), torDetails.getKeys(), msg);
        if(bt_args.verbose)
            DBG(<<"leecher start"<<endl);
        leecher.start();
        if(bt_args.verbose)
            DBG(<<"Leecher done, closing sockets" << endl);
		for(int i = 0; i < bt_args.n_peer; ++i)
			close(bt_args.sockets[i]);
    }
    logFilePtr.close();
    return 0;
}
