#include "Np4d.h"
#include "NetBuffer.h"
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <pthread.h>

#define CHUNK 1024
using namespace forest;

/*
 * usage: AvatarServer
 * This script handles requests from users to save and download avatar files.
 * 09/25: download done, separated send_file
 */

void handleClient(int);
void* handler(void*);

int main() {
	// open stream socket and bind to port 30124
	
	int listenSock = Np4d::streamSocket();
	if (listenSock < 0) fatal("can't create socket");
	ipa_t myIP = Np4d::myIpAddress();
	if (!Np4d::bind4d(listenSock, INADDR_ANY, 30125)) fatal("can't bind socket");

	// prepare to accept connections
	if (!Np4d::listen4d(listenSock)) fatal("error on listen");
	while (true) {
		// wait for incoming connection request and create new socket
		int connSock = Np4d::accept4d(listenSock); //
		std::cout << connSock << "connected" << std::endl;
		if (connSock < 0) fatal("error on accept"); //include? for fatal
		
		// start a separate thread to handle this socket
		handleClient(connSock);
	}
}

void handleClient(int sock) {
	pthread_t thisThread;
	int* sockp = new int;
	*sockp = sock;

	pthread_attr_t attr; pthread_attr_init(&attr);
        pthread_attr_setstacksize(&attr,4*PTHREAD_STACK_MIN);
        pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
	if (pthread_create(&thisThread, &attr, &(handler), (void *) sockp) != 0)
		fatal("cannot create thread");
}

void sendFile(string fileName, int sock)
{
	ifstream pFile (fileName.c_str(), ios::in | ios::binary | ios::ate);
			
	ifstream::pos_type size;
	char *memblock;
	
	if(pFile.is_open())
	{ 
		//std::cout << "file opened" << std::endl;
		size = pFile.tellg();
		memblock = new char [size];
		pFile.seekg (0, ios::beg);
		pFile.read(memblock, size); 
		//complete file in its binary form is now in memory
		pFile.close();
		int bufSize = (int)size;
		//std::cout << bufSize << std::endl;
		stringstream ss;
		ss << "success:" << bufSize;
		string echo = ss.str();
		while (echo.size() < 14)
		{
			echo += " ";
		}
		echo += '\n';
		Np4d::sendString(sock, echo);
		char * ap = &memblock[0];
		
		//send from memblock 1024 bytes at a time
		while(bufSize >= CHUNK)
		{
			Np4d::sendBufBlock(sock, ap, CHUNK);
			ap += CHUNK;
			bufSize -= CHUNK;
		}
		// if last chunk less than CHUNK bytes (1024 for now)
		if(bufSize > 0)
		{
			Np4d::sendBufBlock(sock, ap, bufSize);
		}				
		
		//de-allocate memory
		delete[] memblock;
	}
	else {
		//std::cout << "didn't open" << std::endl;
		Np4d::sendString(sock,"failure:00404\n");
		close(sock); return;
	}


}

void* handler(void* sockp) {

	int sock = *((int *) sockp); delete ((int *) sockp);
	string fileName;
	NetBuffer buf(sock, 1024);
	string s1;
	string pkt_type;

	if (!buf.readAlphas(s1) || s1 != "") {
		if (s1 == "getAvatar") { pkt_type = "get"; }
		else if (s1 == "uploadAvatar") { pkt_type = "upload"; }
		else
		{
			Np4d::sendString(sock,"unrecognized input, should've been a getAvatar request.\noverAndOut\n");
			close(sock); return NULL;
		}
	}

	//process a getAvatar request by looking up files and send them
	if (pkt_type == "get") 
	{
		if (buf.verify(':')) 
		{
			string s2;
			if (buf.readAlphas(s2) && s2 != ""){				
				// get the egg files first
				fileName = string("clientAvatars/") + s2 + string(".zip");
				std::cout << fileName << std::endl;
				sendFile(fileName, sock);

				//then, send texture file
				if(buf.verify(':'))
				{
					string s3;
					string tex_file_type;
					if (buf.readAlphas(s3) && s3 != "")
					{
						switch (s3.at(0)){
							case 'H': tex_file_type = string(".png");
							case 'M': tex_file_type = string(".jpg");
							case 'L': tex_file_type = string("_lo.jpg");
							default: tex_file_type = string(".jpg");
						}
					}
					string tex_to_send = string("clientAvatars/") + s2 + tex_file_type;
					sendFile(tex_to_send, sock);
				}
				else
	 			{				
					Np4d::sendString(sock,"unrecognized input, missing : (colon)\n"
					      "overAndOut\n");
					close(sock); return NULL;		
				}
			}
	 		else
	 		{
				//std::cout << "hmmmm" << std::endl;				
				Np4d::sendString(sock,"unrecognized input, missing : (username)\n"
					      "overAndOut\n");
				close(sock); return NULL;		
			}			
		}
	 	else
	 	{
			//std::cout << "hmmmm" << std::endl;				
			Np4d::sendString(sock,"unrecognized input, missing : (colon)\n"
					      "overAndOut\n");
			close(sock); return NULL;		
		}
	}
	else
	{
		//it's an upload request

	}
		
	close(sock); return NULL;
}
