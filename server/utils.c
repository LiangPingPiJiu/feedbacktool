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


void sigchld_handler(int s)
{
	// waitpid() might overwrite errno, so we save and restore it:
	int saved_errno = errno;

	/*
		suspends execution of the calling process until any child process has changed state.
		By default, waitpid() waits only for terminated
		children, but this behavior is modifiable via the options argument, as described below.
		WNOHANG - return immediately if no child has exited.
	*/
	while(waitpid(-1, NULL, WNOHANG) > 0);

	errno = saved_errno;
}



// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
	{
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}



int setClientSocketOpt(int sockfd)
{
	//first set socket to unblocking:
//	fcntl(sockfd, F_SETFL, O_NONBLOCK);

	//then set timeout
	struct timeval tv;
	tv.tv_sec = TIMEOUT;
	tv.tv_usec = 0;

	if(setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(struct timeval)) < 0)
	{
		perror("setsockopt SO_SNDTIMEO");
		exit(1);
	}
/*	
	if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(struct timeval)) < 0)
	{
		perror("setsockopt SO_RCVTIMEO");
		exit(1);
	}
*/
	return 0;
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


//prints client IP and Port:
void printIpAndPort(struct sockaddr_storage their_addr)
{
	char ipStr[INET6_ADDRSTRLEN];

	//print client IP and Port:
	uint16_t incomingPort;

	// deal with both IPv4 and IPv6:
	if(their_addr.ss_family == AF_INET)
	{
	    struct sockaddr_in *s = (struct sockaddr_in *)&their_addr;
	    incomingPort = ntohs(s->sin_port);
	    inet_ntop(AF_INET, &s->sin_addr, ipStr, sizeof ipStr);
	}
	else // AF_INET6
	{
	    struct sockaddr_in6 *s = (struct sockaddr_in6 *)&their_addr;
	    incomingPort = ntohs(s->sin6_port);
	    inet_ntop(AF_INET6, &s->sin6_addr, ipStr, sizeof ipStr);
	}
	printf("\n\nserver: got connection from %s, on port: %u\n", ipStr, incomingPort);

}


void readFile(struct questionsFD *questionsFile, char *filePath)
{
	FILE *fp;
	fp = fopen(filePath, "r");
	if(fp == NULL)
	{
		perror("error while reading questions file");
		exit(0);	//kill the process
	}
	else
	{
		for(int i = 0; i < MAX_LINES; i++)
		{
			questionsFile->fileBuf[i] = (char *)malloc(BUFFER_LENGTH);

			if(fgets(questionsFile->fileBuf[i], BUFFER_LENGTH, fp) == NULL)
			{
				//end of file
				questionsFile->numberOfQuestions = i;
				break;
			}
			else
			{
				questionsFile->lineLength[i] = strlen(questionsFile->fileBuf[i]);

				//convert "\n" to '\n'
				char *newline = strstr(questionsFile->fileBuf[i], "\\n");
				while(newline != NULL)
				{
					*newline = ' ';	
					newline[1] = '\n';
					newline = strstr(newline, "\\n");
				}
			}
		}	
		fclose(fp);
		printf("number of lines (two more than questions): %u\n", questionsFile->numberOfQuestions);
	}
}


void writeOutputFile(char *buf, uint16_t bufLen)
{
	FILE *outputFD;

	//uses current date + _feedback.csv as file name
	char outputFile[FEEDBACK_PATH_LENGTH];
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	sprintf(outputFile, "%s%d_%d_%d_feedback.csv", FEEDBACK_PATH, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);

	//Open for appending (writing at end of file).  The file is created if it does not exist.  The stream is positioned at the end of the file.
	outputFD = fopen(outputFile, "a");
	if(outputFD == NULL)
	{
		perror(outputFile);
		exit(1);
	}

	int bytes_written = 0;


	bytes_written = fprintf(outputFD, "%-s", buf);
	if(bytes_written != bufLen)
	{
		perror("Error during write");
		printf("written bytes: %d\n", bytes_written);
		fclose(outputFD);
		exit(1);
	}
	fclose(outputFD);
}






