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

void handleClient(int);
void* handler(void*);

int main() {
	// open stream socket and bind to port 30124
	
	int listenSock = Np4d::streamSocket();
	if (listenSock < 0) fatal("can't create socket");
	ipa_t myIP = Np4d::myIpAddress();
	if (!Np4d::bind4d(listenSock, myIP, 30124)) fatal("can't bind socket");

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

void* handler(void* sockp) {

	int sock = *((int *) sockp); delete ((int *) sockp);
	string fileName;
	NetBuffer buf(sock, 1024);
	string s1;

	if (!buf.readAlphas(s1) || s1 != "getPhoto") {
		Np4d::sendString(sock,"1unrecognized input\noverAndOut\n");
		close(sock); return NULL;
	}
	if (buf.verify(':')) {
		string s2;
		if (buf.readAlphas(s2) && s2 != ""){
			
			fileName = string("clientPhotos/") + s2 + string(".jpg");
			std::cout << fileName << std::endl;

			//std::cout << "opening" << s2 << std::endl;
			ifstream pFile (fileName.c_str(), ios::in | ios::binary | ios::ate);
			
			ifstream::pos_type size;
			char *memblock;
			
			if(pFile.is_open())
			{ 
				std::cout << "file opened" << std::endl;
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
				echo += '/n';
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
			else{
				std::cout << "didn't open" << std::endl;
				Np4d::sendString(sock,"failure:00404\n");
				close(sock); return NULL;
			}
		}
	 	else{
			std::cout << "hmmmm" << std::endl;				
			Np4d::sendString(sock,"2unrecognized input\n"
					      "overAndOut\n");
			close(sock); return NULL;		
		}
	}
	else{
			Np4d::sendString(sock,"3unrecognized input\n"
					     "overAndOut\n");
			close(sock); return NULL;
	} 
	
	close(sock); return NULL;
}
