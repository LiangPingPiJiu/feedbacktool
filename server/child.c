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

void clientHandler(int clientSocket, struct questionsFD *questionsFile)
{

	char header[HEADER_SIZE];
	memset(header, 0, HEADER_SIZE);
	uint16_t questionNumber = 0;
	uint16_t networkOrderTmp;
	uint16_t answersLength;
	char *answers = malloc((questionsFile->numberOfQuestions-2)*(BUFFER_LENGTH+1));
	answersLength = 0;

	/*
	//first set socket to unblocking:
	fcntl(clientSocket, F_SETFL, O_NONBLOCK);

	FD_SET(clientSocket, &writefds);
	select(clientSocket+1, NULL, &writefds, NULL, &tv);
	*/


	while(questionNumber < questionsFile->numberOfQuestions) //child loop
	{
		/*
		tv.tv_sec = 2;
		tv.tv_usec = 0;

		FD_ZERO(&writefds);

		FD_SET(clientSocket, &writefds);
		select(clientSocket+1, NULL, &writefds, NULL, &tv);
		*/

		
		//copy to header in network byte order
		//the network byte order is big-endian, most significant byte first

		//numberOfQuestions
		networkOrderTmp = htons(questionsFile->numberOfQuestions);
		//copy MSB
		header[0] = networkOrderTmp;
		//copy LSB
		header[1] = networkOrderTmp>>8;


		//questionNumber
		networkOrderTmp = htons(questionNumber);
		//copy MSB
		header[2] = networkOrderTmp;
		//copy LSB
		header[3] = networkOrderTmp>>8;


		//questionLength
		networkOrderTmp = htons(questionsFile->lineLength[questionNumber]);
		//copy MSB
		header[4] = networkOrderTmp;
		//copy LSB
		header[5] = networkOrderTmp>>8;



		//@debug:
		/*
		for(int i = 0; i < 6; i++)
			printf("header[%d]: %u\n", i, header[i]);
		printf("\n");
		*/



		printf("questionNumber: %u\n", questionNumber);
		//printf("questionsFile->lineLength[questionNumber]: %u\n", questionsFile->lineLength[questionNumber]);
		setClientSocketOpt(clientSocket);

		/*
		if (FD_ISSET(clientSocket, &writefds))
			sendResult = sendAll(clientSocket, buf, &len);
		else
			printf("Timed out.\n");
		*/

		//send header
		sendAll(clientSocket, header, HEADER_SIZE);

		//send question (or statement)
		sendAll(clientSocket,  questionsFile->fileBuf[questionNumber], questionsFile->lineLength[questionNumber]);



		//recv answer from client, if last send was a question (and not the first or the final statement)
		if(questionNumber != 0 && questionNumber != questionsFile->numberOfQuestions -1)
		{
			//recv from client:
			//recv complete header from server:		
			recvAll(clientSocket, header, HEADER_SIZE);

			//convert from network byte order
			//the network byte order is big-endian, most significant byte first
			uint16_t networkOrderTmp;
			uint16_t tmpLen;

			//answerLength
			networkOrderTmp = 0;
			//copy MSB
			networkOrderTmp = header[4];
			//copy LSB
			networkOrderTmp |= header[5]<<8; // only '=' instead of |= overwrites 'networkOrderTmp = header[4];'
			tmpLen = ntohs(networkOrderTmp);


			//recv complete question or statement from server:
			recvAll(clientSocket, answers + answersLength, tmpLen);

			//convert ';' to ','
			char *semicolon = strstr(answers + answersLength, ";");
			while(semicolon != NULL)
			{
				*semicolon = ',';	
				semicolon = strstr(semicolon, ";");
			}

			printf("received answer: %s\n\n", answers + answersLength);

			answersLength += tmpLen;

			//convert '\n' at line end to ';' for every response exept the very last one
			if(questionNumber != questionsFile->numberOfQuestions -2)
			{
				answers[answersLength-1] = ';';
			}
		}

		questionNumber++;
	}

	writeOutputFile(answers, answersLength);
	free(answers);
	close(clientSocket);
	exit(0);
}










