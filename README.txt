


Code Description : The project implements the bittorrent client as per the 
specification given. The client has n seeder/n leecher capabilities.

Running steps:
untar bt_client
cd btclient 
compile : make
run seeder : ./bt_client torrent_file
run leecher :./bt_client -p peer1 -p peer2 torrent_file
The logs indicate the progress of the file download/upload.
The file is written to filename_recv (filename taken from the torrent file)

Tasks Accomplished:

Interpreting bencoded torrent file
abstracting the functionality to classes(seeder, leecher, message)
implementing handshake protocol
bitfield calculation using bit operators
1S/1L case with big file testing and random piece selection.
1S/NL case with dictionary storage of leecher data to handle dynamic leecher exits
nS/nL case with changing the command line arguments to take multiple peer details.
effective usage of git for co-ordinating team development.

Files:

bt_client.cpp : The entry point of the binary, does the connection management,
handshake and creates seeder and leecher objects where the individual management
is done.

parse_torrent.cpp : does the bencode interpretation.

handshake.cpp : handles handshake of peers.

bt_seeder.cpp : all functions specific to the seeder.

bt_leecher.cpp : all functions specific to the leecher.

bt_message.cpp : handles the message creation and sending required by both 
seeder and leecher.

bt_lib.cpp : functions from the skeleton code.

bt_setup.cpp : fucntion to parse the arguments.

Makefile : builds the code.

