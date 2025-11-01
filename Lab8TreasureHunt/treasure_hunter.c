// Replace PUT_USERID_HERE with your actual BYU CS user id, which you can find
// by running `id -u` on a CS lab machine.
#define USERID 1823704790	//added my userID
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include "sockhelper.h"
#include <string.h>

int verbose = 0;

void print_bytes(unsigned char *bytes, int byteslen);

int main(int argc, char *argv[]) {
	if (argc < 5) {
		printf("not enough commands");
		return 1;
	}
	else{
		char* server = argv[1];
		char* port_string = argv[2];
		int port = atoi(port_string);
		int level = atoi(argv[3]);
		int seed = atoi(argv[4]);
		
		printf("Server: %s\n", server);
		printf("Port: %d\n", port);
		printf("Level: %d\n", level);
		printf("Seed: %d\n", seed);

		unsigned char message[8];
		
		message[0] = 0;
		message[1] = level;
		uint32_t user_id = htonl(USERID);
		memcpy(&message[2], &user_id, 4);
		uint16_t user_seed = htons(seed);
		memcpy(&message[6], &user_seed, 2); 

		print_bytes(message, 8);

		struct addrinfo hints;
		memset(&hints, 0, sizeof(struct addrinfo));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_DGRAM;
	}
}

void print_bytes(unsigned char *bytes, int byteslen) {
	int i, j, byteslen_adjusted;

	if (byteslen % 8) {
		byteslen_adjusted = ((byteslen / 8) + 1) * 8;
	} else {
		byteslen_adjusted = byteslen;
	}
	for (i = 0; i < byteslen_adjusted + 1; i++) {
		if (!(i % 8)) {
			if (i > 0) {
				for (j = i - 8; j < i; j++) {
					if (j >= byteslen_adjusted) {
						printf("  ");
					} else if (j >= byteslen) {
						printf("  ");
					} else if (bytes[j] >= '!' && bytes[j] <= '~') {
						printf(" %c", bytes[j]);
					} else {
						printf(" .");
					}
				}
			}
			if (i < byteslen_adjusted) {
				printf("\n%02X: ", i);
			}
		} else if (!(i % 4)) {
			printf(" ");
		}
		if (i >= byteslen_adjusted) {
			continue;
		} else if (i >= byteslen) {
			printf("   ");
		} else {
			printf("%02X ", bytes[i]);
		}
	}
	printf("\n");
	fflush(stdout);
}
