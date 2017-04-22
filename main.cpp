
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

#include <iostream>
#include <string>
#include <ctime>
#include <sstream>
#include <iomanip>

#include <arpa/inet.h>
#include <getopt.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

#include "commands.pb.h"

using namespace std;
using namespace pas;

#define WHERE		__FUNCTION__ << " " << __LINE__ << " "
#define MAX_DACS 	1
int listening_socket = -1;
bool keep_going = true;

string unknown_message;
string invalid_device;
string internal_error;

int port = 5077;

string TimeCode()
{
	time_t t = time(nullptr);
	tm * tx = localtime(&t);
	stringstream ss;
	ss << setfill('0') << setw(2) << tx->tm_hour << ":";
	ss << setw(2) << tx->tm_min << ":";
	ss << setw(2) << tx->tm_sec;
	return ss.str();
}

bool CheckDevice(int device)
{
	return device >= 0 && device < MAX_DACS;
}

void SIGINTHandler(int)
{
	cout << endl << WHERE << "signal caught - setting keep_going to false" << endl;
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

bool SendPB(string & s, int server_socket)
{
	size_t length = s.size();
	size_t ll = length;

	length = htonl(length);
	size_t bytes_sent;

	if ((bytes_sent  = send(server_socket, (const void *) &length, sizeof(length), 0)) != sizeof(length)) {
		cout << WHERE << "bad bytes_sent for length: " << strerror(errno) << endl;
		return false;
	}

	if ((bytes_sent = send(server_socket, (const void *) s.data(), ll, 0)) != ll) {
		cout << WHERE << "bad bytes_sent for message: " << strerror(errno) << endl;
		return false;
	}
	return true;
}

bool OneInteger_OneIntegerReply(string & in, int socket)
{
	bool rv = false;
	OneInteger o;
	OneInteger outi;
	OneString outs;
	string output;

	if (o.ParseFromString(in)) {
		switch (o.type())
		{
			case ARTIST_COUNT:
			case TRACK_COUNT:
				outi.set_type(ONE_INT);
				outi.set_value(1);
				if (outi.SerializeToString(&output)) {
					if (SendPB(output, socket)) {
						cout << WHERE << "sent 1 as count." << endl;
						rv = true;
					}
				}
				else {
					cout << WHERE << "failed to serialize response." << endl;
				}
				break;

			case WHO_DEVICE:
			case WHAT_DEVICE:
			case WHEN_DEVICE:
				if (!CheckDevice(o.value())) {
					if (SendPB(invalid_device, socket)) {
						cout << WHERE << "sent invalid device: " << o.value() << endl;
						rv = true;
						break;
					}	
				}
				outs.set_type(ONE_STRING);
				if (o.type() == WHO_DEVICE)
					outs.set_value("The Who");
				else if (o.type() == WHAT_DEVICE)
					outs.set_value("The What");
				else {
					outs.set_value(TimeCode());
				}
				if (!outs.SerializeToString(&output)) {
					cout << WHERE << "failed to serialize response." << endl;
					SendPB(internal_error, socket);
				}
				else if (SendPB(output, socket)) {
					cout << WHERE << "relating to device: " << o.value() << " sent " << outs.value() << endl;
					rv = true;
				}
				break;

			default:
				cout << WHERE << "should not get here ";
				break;
		}
		rv = true;		
	}
	return rv;

}

bool OneInteger_NoReply(string & in)
{
	bool rv = false;
	OneInteger o;
	if (o.ParseFromString(in)) {
		switch (o.type())
		{
			case CLEAR_DEVICE:
				cout << "Clearing device: ";
				break;

			case NEXT_DEVICE:
				cout << "Next track on device: ";
				break;

			case STOP_DEVICE:
				cout << "Stopping device: ";
				break;

			case RESUME_DEVICE:
				cout << "Resuming device: ";
				break;

			case PAUSE_DEVICE:
				cout << "Pausing device: ";
				break;

			default:
				cout << WHERE << " should not get here ";
				break;
		}
		cout << o.value() << endl;
		rv = true;		
	}
	return rv;
}

bool TwoInteger_NoReply(string & in)
{
	bool rv = false;
	TwoIntegers o;
	if (o.ParseFromString(in)) {
		switch (o.type())
		{
			case PLAY_TRACK_DEVICE:
				if (!CheckDevice(o.value_a())) {
					cout << WHERE << "received play on invalid device: " << o.value_a() << " but no reply is being sent." << endl;
				}
				else {
					cout << WHERE << "play track message received. Device: " << o.value_a() << " Track: " << o.value_b() << endl;
				}
				rv = true;
				break;

			default:
				cout << WHERE << "should not get here." << endl;
				break;
		}
		rv = true;
	}
	return rv;
}

bool DacInfoCommand(string & in, int socket)
{
	bool rv = true;
	string s;

	SelectResult sr;
	sr.set_type(SELECT_RESULT);
	Row * r = sr.add_row();
	r->set_type(ROW);
	google::protobuf::Map<string, string> * result = r->mutable_results();
	(*result)[string("index")] = "0";
	(*result)[string("name")] = "Make Believe DAC";
	(*result)[string("who")] = "The Who";
	(*result)[string("what")] = "The What";
	(*result)[string("when")] = TimeCode();
	if (!sr.SerializeToString(&s)) {
		cout << WHERE << "failed to serialize response." << endl;
		s = internal_error;
		rv = false;
	}
	else {
		cout << WHERE << "sent a DAC_INFO_COMMAND response" << endl;
	}
	SendPB(s, socket);
	return rv;
}

bool CommandProcessor(int socket, string & in)
{
	bool rv = false;
	GenericPB g;
	if (g.ParseFromString(in)) {
		cout << WHERE << "received type: " << g.type() << endl;
		switch (g.type())
		{
			// The OneIntegers that do not send a reply.
			case CLEAR_DEVICE:
			case NEXT_DEVICE:
			case STOP_DEVICE:
			case RESUME_DEVICE:
			case PAUSE_DEVICE:
				rv = OneInteger_NoReply(in);
				break;

			case TRACK_COUNT:
			case ARTIST_COUNT:
			case WHO_DEVICE:
			case WHEN_DEVICE:
			case WHAT_DEVICE:
				rv = OneInteger_OneIntegerReply(in, socket);
				break;

			case PLAY_TRACK_DEVICE:
				rv = TwoInteger_NoReply(in);
				break;

			case DAC_INFO_COMMAND:
				rv = DacInfoCommand(in, socket);
				break;

			default:
				if (SendPB(unknown_message, socket)) {
					cout << WHERE << "sent unknown message." << endl;
					rv = true;
				}
				break;
		}

	}
	else {
		cout << WHERE << "failed to parse incoming message." << endl;
	}
	return rv;
}

void HandleConnection(int socket)
{
	size_t length;
	size_t bytes_read;

	while (keep_going) {
		if ((bytes_read = recv(socket, (void*)&length, sizeof(length), 0)) == sizeof(length)) {
			length = ntohl(length);
			cout << WHERE << "length of next message: " << length << endl;
			string incoming;

			incoming.resize(length);
			if ((bytes_read = recv(socket, (void*)&incoming[0], length, 0)) == length) {
				cout << WHERE << "received message of length: " << bytes_read << endl;
				if (!CommandProcessor(socket, incoming)) {
					break;
				}
			}
			else {
				cout << WHERE << "failed to read message correctly: " << strerror(errno) << endl;
				break;
			}
		}
		else {
			cout << WHERE << "failed to read length correctly: " << strerror(errno) << endl;
			break;
		}
	}
}

void Serve()
{
	int incoming_socket;

	if ((listening_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		cout << WHERE << strerror(errno) << endl;
		return;
	}

	int optval = 1;
	if (setsockopt(listening_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval) < 0) {
		cout << WHERE << strerror(errno) << endl;
		return;
	}

	sockaddr_in listening_sockaddr;

	memset(&listening_sockaddr, 0, sizeof(sockaddr_in));
	listening_sockaddr.sin_family = AF_INET;
	listening_sockaddr.sin_port = htons(port);
	listening_sockaddr.sin_addr.s_addr = INADDR_ANY;

	if (bind(listening_socket, (sockaddr*)&listening_sockaddr, sizeof(sockaddr_in)) < 0) {
		cout << WHERE << " " << strerror(errno) << endl;
		return;
	}

	// Configure for handling at most one (1) client.
	if (listen(listening_socket, 1) != 0) {
		cout << WHERE << " " << strerror(errno) << endl;
		return;
	}

	sockaddr_in client_info;
	memset(&client_info, 0, sizeof(sockaddr_in));
	int c = sizeof(sockaddr_in);

	int connection_counter = 0;

	cout << WHERE << "monitoring network." << endl;
	while ((incoming_socket = accept(listening_socket, (sockaddr*)&client_info, (socklen_t*)&c)) > 0) {

		cout << WHERE << "Connection: " << connection_counter << " established." << endl;
		HandleConnection(incoming_socket);
		cout << WHERE << "Connection: " << connection_counter << " taken down." << endl;

		connection_counter++;

		if (!keep_going)
			break;
	}
}

bool InitializeErrorMessages()
{
	bool rv = true;

	OneInteger o;
	o.set_type(ERROR_MESSAGE);


	o.set_value(UNKNOWN_MESSAGE);
	if (!o.SerializeToString(&unknown_message))
		rv = false;

	o.set_value(INVALID_DEVICE);
	if (!o.SerializeToString(&invalid_device))
		rv = false;

	o.set_value(INTERNAL_ERROR);
	if (!o.SerializeToString(&internal_error))
		rv = false;
	
	return rv;
}

int main(int argc, char* argv[])
{
	signal(SIGINT, SIGINTHandler);
	siginterrupt(SIGINT, 1);

	if (!InitializeErrorMessages()) {
		cout << WHERE << "failed to initialize stock error messages." << endl;
		return 1;
	}

	GetOptions(argc, argv);
	Serve();

	if (listening_socket >= 0)
		close(listening_socket);

	return 0;
}
