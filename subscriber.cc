#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include "helpers.h"

void usage(char* file) {
    fprintf(stderr, "Usage: %s <ID_CLIENT> <IP_SERVER> <PORT_SERVER>\n", file);
    exit(0);
}

int main(int argc, char* argv[]) {
    // dezactivare buffering
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    char buffer[BUFLEN], copy[BUFLEN], *p;
    int sockfd, ret, s, fdmax, enable, r, i, n;
    bool shouldsend;
    struct sockaddr_in serv_address;


    bzero(buffer, BUFLEN);
    bzero(copy, BUFLEN);

    tcp_message tcp_message;

    // setul de file descriptori
    fd_set read_fds;
    fd_set tmp_fds;

    // verific numarul de argumente
    if (argc != 4) {
        usage(argv[0]);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockfd < 0, "Error socket");

    serv_address.sin_family = AF_INET;
    serv_address.sin_port = htons(atoi(argv[3]));

    ret = connect(sockfd, (struct sockaddr*)&serv_address, sizeof(serv_address));
    DIE(ret < 0, "Error connection");

    ret = inet_aton(argv[2], &serv_address.sin_addr);
    DIE(ret == 0, "inet_aton");

    // trimitere ID catre server
    sprintf(buffer, "%s\n", argv[1]);

    s = send(sockfd, buffer, strlen(buffer) + 1, 0);
    DIE(s == -1, "Error sending");

    FD_ZERO(&read_fds);

    // adaugam socketul in multimea de file descriptori
    FD_SET(sockfd, &read_fds);

    // adaug stdin in multimea de file descriptori
    FD_SET(STDIN_FILENO, &read_fds);

    fdmax = sockfd;

    // dezactivez algoritmul Neagle
    enable = 1;
    setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int));

    while (1) {
        tmp_fds = read_fds;

        r = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
        DIE(r < 0, "select");

        for (i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &tmp_fds)) {
                if (i == STDIN_FILENO) {  
                    // STDIN

                    shouldsend = false;

                    bzero(tcp_message.buffer, BUFLEN);
                    fgets(tcp_message.buffer, BUFLEN, stdin);

                    if (!strcmp(tcp_message.buffer, "exit\n")) {
                        // EXIT
                        exit(0);
                    }

                    tcp_message.SF = false;
                    strcpy(copy, tcp_message.buffer);
                    p = strtok(copy, " ");

                    if (!strcmp(p, "unsubscribe")) {
                        // UNSUBSCRIBE
                        p = strtok(NULL, " ");
                        if (strlen(p) <= 50 && p != nullptr) {
                            if (!strtok(NULL, " ")) {
                                shouldsend = true;
                                printf("Unsubscribed from topic.\n");
                            }
                        }
                    }
                    if (!strcmp(p, "subscribe")) {
                        // SUBSCRIBE
                        p = strtok(NULL, " ");
                        if (strlen(p) <= 50 && p != nullptr) {
                            p = strtok(NULL, " ");
                            if ((atoi(p) == 0 && p != nullptr || atoi(p) == 1) && !strtok(NULL, " ")) {
                                tcp_message.SF = atoi(p);
                                shouldsend = true;
                                printf("Subscribed to topic.\n");
                            }
                        }
                    }

                    if (shouldsend) {
                        r = send(sockfd, (char*)&tcp_message, sizeof(tcp_message), 0);
                        DIE(r == -1, "send");
                    }
                    continue;
                }

                bzero(buffer, sizeof(server_message));

                n = recv(i, buffer, sizeof(server_message), 0);
                DIE(n < 0, "recv");

                server_message* serv_message = (server_message*)buffer;

                if (!strcmp(serv_message->message, DUPLICATE_ID) || n == 0) {
                    // este conectat
                    exit(0);
                }
                printf("%s:%hu - %s - %s - %s\n",
                       serv_message->udp_ip,
                       serv_message->udp_port,
                       serv_message->topic,
                       serv_message->type,
                       serv_message->message);
            }
        }
    }
}
