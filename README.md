# 490-Capstone

To compile server, non-gui client, and simple bots, run Make

GUI client requires fltk to compile, and the binary is thus included

# Running Program

The server and client take in two command line arguments.

The port (>=2000) and maximum number of players (1-10) for the server

The ip of the server and the port for the client

#Notes

For DEMONSTRATION PURPOSES ONLY I have included an example private and public key. Please do not use them and generate your own for anything serious. The keys just need to in the same location as the program when run (private for server and public for any client). For testing purposes, feel free to use the included keys to see how the program works, worse case scenario your UNO hand info is leaked lol.

You might also run into an error with openSSL versions, please run this to get the correct version if so (for Ubuntu):
wget http://nz2.archive.ubuntu.com/ubuntu/pool/main/o/openssl/libssl1.1_1.1.1f-1ubuntu2_amd64.deb
sudo dpkg -i libssl1.1_1.1.1f-1ubuntu2_amd64.deb
