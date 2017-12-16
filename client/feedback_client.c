//This program is the client-part of a simple feedback tool, developped
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
feedback_client version 1, Copyright (C) 2017 Tobias Endrikat
feedback_client comes with ABSOLUTELY NO WARRANTY
This is free software, and you are welcome to redistribute it
licensed under the GPLv3
*/



#include "linker.h"


int main(int argc, char *argv[])
{
	int sockfd;  
	char *buf = malloc(BUFFER_LENGTH+1);
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	uint16_t numberOfQuestions;
	uint16_t questionNumber;
	uint16_t questionLength;
	uint16_t answerLength;

	struct timeval tv;
	tv.tv_sec = TIMEOUT;
	tv.tv_usec = 0;

	if (argc != 3)
	{
		fprintf(stderr,"usage: %s servername, serverport\n", argv[0]);
		exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if((rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
			perror("client: socket");
			continue;
		}

		/*
		if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(struct timeval)) == -1)
		{
			perror("setsockopt");
			exit(1);
		}
		*/

		if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(struct timeval)) < 0)
		{
			perror("setsockopt SO_RCVTIMEO");
			exit(1);
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
		{
			close(sockfd);
			//perror("client: connect");
			continue;
		}

		if(setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(struct timeval)) < 0)
		{
			perror("setsockopt SO_SNDTIMEO");
			exit(1);
		}

		break;
	}

	if (p == NULL)
	{
		fprintf(stderr, "Sorry, can't connect to server. Please check whether the server address is correct.\nterminating...\n\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
	//printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure



	char header[HEADER_SIZE];


	numberOfQuestions = 1;
	questionNumber = 0;
	questionLength = 0;

	while(questionNumber < numberOfQuestions)
	{
		memset(header, 0, HEADER_SIZE);

		//recv complete header from server:
		recvAll(sockfd, header, HEADER_SIZE);

		//@debug
		/*
		for(int i = 0; i < 6; i++)
			printf("header[%d]: %u\n", i, header[i]);
		printf("\n");
		*/


		//convert from network byte order
		//the network byte order is big-endian, most significant byte first
		uint16_t networkOrderTmp;

		//numberOfQuestions
		networkOrderTmp = 0;
		//copy MSB
		networkOrderTmp = header[0];
		//copy LSB
		networkOrderTmp |= header[1]<<8; // only '=' instead of |= overwrites 'networkOrderTmp = header[4];'
		numberOfQuestions = ntohs(networkOrderTmp);


		//questionNumber
		networkOrderTmp = 0;
		//copy MSB
		networkOrderTmp = header[2];
		//copy LSB
		networkOrderTmp |= header[3]<<8; // only '=' instead of |= overwrites 'networkOrderTmp = header[4];'
		questionNumber = ntohs(networkOrderTmp);


		//questionLength
		networkOrderTmp = 0;
		//copy MSB
		networkOrderTmp = header[4];
		//copy LSB
		networkOrderTmp |= header[5]<<8; // only '=' instead of |= overwrites 'networkOrderTmp = header[4];'
		questionLength = ntohs(networkOrderTmp);


		//recv complete question or statement from server:
		recvAll(sockfd, buf, questionLength);

		if(questionNumber == 0) //first statement
		{
			printf("\n\n##########################################################################################\n");
			printf("%s\n", buf);
			printf("There are %u questions in total.\n", numberOfQuestions-2);
			printf("##########################################################################################\n");
		}
		else if(questionNumber < numberOfQuestions-1) //question
		{
			printf("\n++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
			printf("question %u/%u:\n", questionNumber, numberOfQuestions-2);
			printf("%s\n", buf);
			printf(TYPEREQUEST);
			printf("\n++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n\n> ");
		}
		else //final statement
		{
			printf("\n\n##########################################################################################\n\n");
			printf("%s\n", buf);
			printf("##########################################################################################\n\n\n");
		}


		//send answer from command line, if it's a question (and not the first or the final statement)
		if(questionNumber != 0 && questionNumber != numberOfQuestions -1)
		{
			fgets(buf, BUFFER_LENGTH, stdin);
			answerLength = strlen(buf);

			memset(header, 0, HEADER_SIZE);

			//copy to header in network byte order
			//the network byte order is big-endian, most significant byte first
			networkOrderTmp = htons(answerLength);
			//copy MSB
			header[4] = networkOrderTmp;
			//copy LSB
			header[5] = networkOrderTmp>>8;


			//send header
			sendAll(sockfd, header, HEADER_SIZE);

			//send answer string
			sendAll(sockfd, buf, answerLength);
		}
		questionNumber++;
	}

	free(buf);
	//@debug:
	//printf("close socket and terminate\n");	
	close(sockfd);

	return 0;
}
