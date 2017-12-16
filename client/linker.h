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


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include "utils.h"

#define BUFFER_LENGTH 4000 // max number of bytes we can get at once 
#define TIMEOUT 10
#define HEADER_SIZE 6
#define TYPEREQUEST "Please enter your answer and hit <enter> to commit: "
