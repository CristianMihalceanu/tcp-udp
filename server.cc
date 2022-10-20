#include <algorithm>
#include <cmath>
#include <iostream>
#include <string.h>
#include <unordered_map>
#include <unordered_set>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include "helpers.h"

using namespace std;

#define MAX(a,b) (((a)>(b))?(a):(b))

void usage(char *file) {
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}

bool eraseVector(vector<string> &v, string id) {
	for(auto it = v.begin(); it != v.end(); it++) {
		if (*it == id) {
			v.erase(it);
			return true;
		}
	}
	return false;
}

int main(int argc, char *argv[]) {
	// dezactivez buffering ul
    setvbuf(stdout, NULL, _IONBF, BUFSIZ); // se dezactiveaza bufferingul la afisare

	// map pentru a retine clientii conectati la un topic
	unordered_map<string, vector<string>> clients;
	// map pentru a retine id ul clientilor in functie de socket
    unordered_map<int, string> idmap;

	fd_set read_fds;
    fd_set tmp_fds;

	struct sockaddr_in udp_address, serv_address, cli_address;
	char buffer[BUFLEN], buffer_udp[BUFLENUDP];
	server_message message;
	bool online;
	int udpSocket, tcpSocket, port_no, enable, ret, n, fdmax, i, newsockfd, s, tip;

	// verfic numarul de argumente
    if (argc != 2) {
        usage(argv[0]);
    }

	// extrag portul
    port_no = atoi(argv[1]);

    // socket UDP
    socklen_t udpSockLen = sizeof(sockaddr); 
    udpSocket = socket(PF_INET, SOCK_DGRAM, 0);
    DIE(udpSocket < 0, "Error socket UDP"); 

	// completez datele serverului
	bzero((char *) &udp_address, sizeof(udp_address));
	udp_address.sin_port = htons(port_no);
    udp_address.sin_family = AF_INET;
    udp_address.sin_addr.s_addr = INADDR_ANY;

    // socket TCP
    tcpSocket = socket(AF_INET, SOCK_STREAM, 0);
    DIE(tcpSocket < 0, "Error socket TCP");

    // se dezactiveaza algoritmul Nagle
    enable = 1;
	setsockopt(tcpSocket, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int));

	bzero((char *) &serv_address, sizeof(serv_address));
	serv_address.sin_family = AF_INET;
	serv_address.sin_port = htons(port_no);
	serv_address.sin_addr.s_addr = INADDR_ANY;

	ret = bind(tcpSocket, (struct sockaddr *) &serv_address, sizeof(struct sockaddr));
	DIE(ret < 0, "Error binding");

    ret = listen(tcpSocket, MAX_CLIENTS);
	DIE(ret < 0, "Error listen");

    ret = bind(udpSocket, (struct sockaddr *) &udp_address, sizeof(struct sockaddr));
    DIE(ret < 0, "Error binding");

    FD_ZERO(&read_fds);

	// adaug socketul TCP in multimea de descriptori
	FD_SET(tcpSocket, &read_fds);

	// adaug socketul UDP in multimea de descriptori
	FD_SET(udpSocket, &read_fds);

	// adaug stdin in multimea de descriptori
	FD_SET(STDIN_FILENO, &read_fds);

    fdmax = MAX(tcpSocket, udpSocket);

    while(1) {
        tmp_fds = read_fds;

        ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "Error select");

		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == tcpSocket) {
					socklen_t clilen = sizeof(cli_address);
					newsockfd = accept(tcpSocket, (struct sockaddr*) &cli_address, &clilen);
					DIE(newsockfd < 0, "accept");

                    FD_SET(newsockfd, &read_fds);

					fdmax = MAX(fdmax, newsockfd);

					bzero(buffer, BUFLEN);

					n = recv(newsockfd, buffer, sizeof(buffer), 0);
					DIE(n < 0, "recv");

					buffer[strlen(buffer)-1] = '\0';

				 online = false;
					for (auto x : idmap) {
      					if (x.second == string(buffer)) {
      						online = true;
      					}
					}

					if (!online) { 
						// client nou
						idmap[newsockfd] = buffer;
						printf("New client %s connected from %s:%d.\n", buffer, inet_ntoa(cli_address.sin_addr), ntohs(cli_address.sin_port));
					} else {
						 // client existent
						bzero(message.message, BUFLEN);
						strcpy(message.message, DUPLICATE_ID);

						s = send(newsockfd, (char*) &message, sizeof(server_message), 0);
						DIE(s < 0, "Error send");

						printf("Client %s already connected.\n", buffer);
					}
				} else if (i == STDIN_FILENO) {
					bzero(buffer, BUFLEN);
    				fgets(buffer, BUFLEN, stdin);

					// inchidem socketii
					if (string(buffer) == "exit\n") {
						for (i = 1; i <= fdmax; i++) {
					        if (FD_ISSET(i, &tmp_fds)) {
					            close(i);
					        }
					    }
    					exit(0);
    				}
				} else if (i == udpSocket) {
					bzero(buffer_udp, BUFLENUDP);

					n = recvfrom(udpSocket, buffer_udp, BUFLENUDP, 0, (sockaddr*) &udp_address, &udpSockLen);
					DIE(n < 0, "recv");

					strcpy(message.udp_ip, inet_ntoa(udp_address.sin_addr));
					message.udp_port = ntohs(udp_address.sin_port);
					memcpy(message.topic, buffer_udp, 50);

					// extrag tipul datelor
					tip = buffer_udp[50];

					char udpData[BUFLEN];
					bzero(udpData, BUFLEN);
					memcpy(udpData, buffer_udp + 51, 1500);

					switch (tip) {
						case 0:
							{
								strcpy(message.type, "INT");
								long long integerValue = ntohl(*(uint32_t*)(udpData + 1));
								integerValue = udpData[0] ? -integerValue : integerValue;
								sprintf(message.message, "%lld", integerValue);
							}
							break;
						case 1:
							{
								strcpy(message.type, "SHORT_REAL");
								double shortRealValue = ntohs(*(uint16_t*)(udpData));
								shortRealValue /= 100;
								sprintf(message.message, "%.2f", shortRealValue);
							}
							break;
						case 2:
							{
								strcpy(message.type, "FLOAT");
								double floatValue = ntohl(*(uint32_t*)(udpData + 1));
								floatValue /= pow(10, udpData[5]);
								floatValue = udpData[0] ? -floatValue : floatValue;
								sprintf(message.message, "%lf", floatValue);
							}
							break;
						case 3:
							{
								strcpy(message.type, "STRING");
								strcpy(message.message, udpData);
							}
							break;
					}

					string topic(message.topic);

					// trimit mesajul tuturor abonatilor
					for (auto id : clients[topic]) {
						for (auto const &pair: idmap) {
					    	if (pair.second.compare(id) == 0) {
					    		 s = send(pair.first, (char*) &message, sizeof(server_message), 0);
								DIE(s < 0, "send");
					    	}
					    }
					}
				} else {
					bzero(buffer, BUFLEN);
					n = recv(i, buffer, sizeof(tcp_message), 0);
					DIE(n < 0, "recv");

					tcp_message* tmp_message = (tcp_message *)buffer;

					// conexiunea s-a inchis
					if (n == 0) {
						if (idmap[i].size()) {
							cout<<"Client "<<idmap[i]<<" disconnected.\n";
							idmap.erase(i);
						}

						FD_CLR(i, &read_fds);
						close(i);
					} else {
						char*p = strtok(tmp_message->buffer, " ");
;
						if (strstr(tmp_message->buffer, "subscribe") != NULL) {
							p = strtok(NULL, " ");
							if (p != nullptr && strlen(p) <= 50) {
								string topic(p);
								clients[topic].push_back(idmap[i]);
							}
						} else if (strstr(tmp_message->buffer, "unsubscribe") != NULL) {
							p = strtok(NULL, " ");
							if (p != nullptr && strlen(p) <= 50) {
								p[strlen(p)-1] = '\0';
								string topic(p);
								eraseVector(clients[topic], idmap[i]);
							}
						}
					}
				}
			}
		}
    }

    return 0;
}