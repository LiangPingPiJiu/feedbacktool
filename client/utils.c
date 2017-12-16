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

#include "linker.h"


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
	{
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


int sendAll(int s, char *buf, uint16_t len)
{
	uint16_t total = 0;			// how many bytes we've sent
	uint16_t bytesleft = len;	// how many we have left to send
	int n;

	while(total < len)
	{
		n = send(s, buf+total, bytesleft, 0);
		//printf("n: %u\n", n);
		if(n == -1)
		{
			perror("send");
			printf("only %d bytes have been sent because of an error!\nthread is terminating\n", total);
			close(s);
			exit(0);	//kill the process
		}

		else if(n == 0)
		{
			perror("send");
			printf("connection to client has been closed\nthread is terminating\n");
			close(s);
			exit(0);	//kill the process
		}
		else
		{
			total += n;
			bytesleft -= n;
		}
	}
	return total;		// return number actually sent here
}


//recv's all in a loop
int recvAll(int s, char *buf, uint16_t maxBytes)
{
	
	int recvBytes = 0;
	int recvResult;
	while(maxBytes > 0)
	{
		recvResult = recv(s, buf, maxBytes, 0);
		maxBytes -= recvResult;

		if(recvResult == -1)		//nothing to receive (anymore)
		{
			printf("recvResult: %d\n", recvResult);
			break;
		}
		else if(recvResult == 0)		//connection closed
		{
			perror("recv");
			printf("The connection has been closed. :(\nProgram is terminating...\n");
			close(s);
			exit(1);
		}
		else //recvResult > 0
		{
			recvBytes += recvResult;
		}
	}

	if(recvBytes == 0)	//nothing received
	{
		perror("recv");
		printf("Did not receive anything. :(\nProgram is terminating...\n");
		close(s);
		exit(1);
	}

	buf[recvBytes] = '\0';
	return recvBytes;
}

