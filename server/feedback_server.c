//This program is the server-part of a simple feedback tool, developped
//to get feedback (e.g. for teaching) through this specific command line program,
//communicating over a simple TCP connection
//should run without problems under at least most of the x64 linux os
//
//Version 1, december 2017
//Copyright (C) 2017  Tobias Endrikat
//
//This program is free software; you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation; either version 3 of the License, or
//(at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software Foundation,
//Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
//
//some of the code is taken from http://beej.us/guide/bgnet/
//
// ------------------------------------------------------------------------
//



/*
feedback_server version 1, Copyright (C) 2017 Tobias Endrikat
feedback_server comes with ABSOLUTELY NO WARRANTY
This is free software, and you are welcome to redistribute it
licensed under the GPLv3
*/


#include "linker.h"


int main(int argc, char *argv[])
{
	int listeningSocket, clientSocket;  // listen on listeningSocket, new connection on clientSocket
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;

	int rv;

	if (argc != 3)
	{
		fprintf(stderr,"usage: %s ownPort, questions.txt\n", argv[0]);
		exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next)
	{
		if((listeningSocket = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
			perror("server: socket");
			continue;
		}

		if(setsockopt(listeningSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
		{
			perror("setsockopt SO_REUSEADDR");
			exit(1);
		}

		if(bind(listeningSocket, p->ai_addr, p->ai_addrlen) == -1)
		{
			close(listeningSocket);
			perror("server: bind");
			continue;
		}
		break;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (p == NULL)
	{
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}

	if (listen(listeningSocket, BACKLOG) == -1)
	{
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1)
	{
		perror("sigaction");
		exit(1);
	}

	printf("server: waiting for connections...\n");

	
	//reads file with questions
	struct questionsFD questionsFile;
	readFile(&questionsFile, argv[2]);

	while(1)
	{  // main accept() loop
		sin_size = sizeof their_addr;
		clientSocket = accept(listeningSocket, (struct sockaddr *)&their_addr, &sin_size);
		if(clientSocket == -1)
		{
			perror("error while accept()");
			continue;
		}

		//print client IP and Port:
		printIpAndPort(their_addr);


		if (!fork()) // this is the child process
		{

			close(listeningSocket); // child doesn't need the listener
			clientHandler(clientSocket, &questionsFile);

		}

		close(clientSocket);  // parent doesn't need this
	}

	return 0;
}
