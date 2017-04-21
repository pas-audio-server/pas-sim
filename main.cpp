
/*	This file is part of pas.

    pas is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    pas is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with pas.  If not, see <http://www.gnu.org/licenses/>.
*/

/*  pas is Copyright 2017 by Perry Kivolowitz.
*/

#include <arpa/inet.h>
#include <getopt.h>
#include <iostream>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

using namespace std;

int listening_socket = -1;
bool keep_going = true;

int port = 5077;

void SIGINTHandler(int)
{
	keep_going = false;
}

void GetOptions(int argc, char* argv[])
{
	int opt;

	while ((opt = getopt(argc, argv, "p:")) != -1) {
		switch (opt) {

		case 'p':
			port = atoi(optarg);
			break;
		}
	}
}

void HandleConnection(int socket)
{
	unsigned int length;
	unsigned int bytes_read;

	while (keep_going) {
		if ((bytes_read = recv(socket, (void*)&length, sizeof(length), 0)) == sizeof(length)) {
			length = ntohl(length);
			string incoming;

			incoming.resize(length);
			if ((bytes_read = recv(socket, (void*)&incoming[0], length, 0)) == length) {
				cout << __FUNCTION__ << " " << __LINE__ << endl;
			}
		}
	}
}

void Serve()
{
	int incoming_socket;

	if ((listening_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		cout << __FUNCTION__ << " " << __LINE__ << " " << strerror(errno) << endl;
		return;
	}

	int optval = 1;
	if (setsockopt(listening_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval) < 0) {
		cout << __FUNCTION__ << " " << __LINE__ << " " << strerror(errno) << endl;
		return;
	}

	sockaddr_in listening_sockaddr;

	memset(&listening_sockaddr, 0, sizeof(sockaddr_in));
	listening_sockaddr.sin_family = AF_INET;
	listening_sockaddr.sin_port = htons(port);
	listening_sockaddr.sin_addr.s_addr = INADDR_ANY;

	if (bind(listening_socket, (sockaddr*)&listening_sockaddr, sizeof(sockaddr_in)) < 0) {
		cout << __FUNCTION__ << " " << __LINE__ << " " << strerror(errno) << endl;
		return;
	}

	// Configure for handling at most one (1) client.
	if (listen(listening_socket, 1) != 0) {
		cout << __FUNCTION__ << " " << __LINE__ << " " << strerror(errno) << endl;
		return;
	}

	sockaddr_in client_info;
	memset(&client_info, 0, sizeof(sockaddr_in));
	int c = sizeof(sockaddr_in);

	int connection_counter = 0;

	while ((incoming_socket = accept(listening_socket, (sockaddr*)&client_info, (socklen_t*)&c)) > 0) {
		cout << "Connection: " << connection_counter << " established." << endl;
		HandleConnection(incoming_socket);
		cout << "Connection: " << connection_counter << " taken down." << endl;

		connection_counter++;

		if (!keep_going)
			break;
	}
}

int main(int argc, char* argv[])
{
	signal(SIGINT, SIGINTHandler);
	siginterrupt(SIGINT, 1);

	GetOptions(argc, argv);
	Serve();

	if (listening_socket >= 0)
		close(listening_socket);

	return 0;
}