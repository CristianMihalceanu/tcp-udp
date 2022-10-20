#ifndef _HELPERS_H
#define _HELPERS_H

#include <stdio.h>
#include <stdlib.h>


#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)

#define BUFLENUDP		1553		// dimensiunea primita de la UDP
#define BUFLEN			1501		// dimensiunea maxima a calupului de date
#define MAX_CLIENTS		2147483647	// numarul maxim de clienti in asteptare = INT_MAX
#define DUPLICATE_ID 	"42"		// cod client deja conectat
#define DEBUG 0						// flag pentru afisari aditionale

// structura trimisa de tcp la server
struct __attribute__((packed)) tcp_message {
	char buffer[BUFLEN];
	bool SF;
};

// structura trimisa de server la tcp
struct __attribute__((packed)) server_message {
	char udp_ip[16];
    uint16_t udp_port;
    char topic[51];
    char type[16];
    char message[BUFLEN];
}; 

#endif
