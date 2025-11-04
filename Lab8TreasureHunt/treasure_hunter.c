// Replace PUT_USERID_HERE with your actual BYU CS user id, which you can find
// by running `id -u` on a CS lab machine.
#define USERID 1823704790	//added my userID
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include "sockhelper.h"
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h> 


int verbose = 0;

void print_bytes(unsigned char *bytes, int byteslen);

#include <stdint.h>  // Add this include at the top

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
		
		// printf("Server: %s\n", server);
		// printf("Port: %d\n", port);
		// printf("Level: %d\n", level);
		// printf("Seed: %d\n", seed);

		unsigned char message[8];
		
		message[0] = 0;
		message[1] = level;
		uint32_t user_id = htonl(USERID);
		memcpy(&message[2], &user_id, 4);
		uint16_t user_seed = htons(seed);
		memcpy(&message[6], &user_seed, 2); 

		// print_bytes(message, 8);

		struct addrinfo hints;
		memset(&hints, 0, sizeof(struct addrinfo));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_DGRAM;
		hints.ai_protocol = IPPROTO_UDP;
		hints.ai_flags = 0;
		struct addrinfo *servinfo;
		if (getaddrinfo(server, port_string, &hints, &servinfo) == 0){

			struct sockaddr_storage remote_addr_ss;
			struct sockaddr_storage local_addr_ss;
			struct sockaddr *remote_addr = (struct sockaddr *)&remote_addr_ss;
			struct sockaddr *local_addr = (struct sockaddr *)&local_addr_ss;
			char remote_ip[INET6_ADDRSTRLEN];
			char local_ip[INET6_ADDRSTRLEN];
			unsigned short remote_port;
			unsigned short local_port;
			int addr_fam = AF_INET;
			memcpy(remote_addr, servinfo->ai_addr, sizeof(struct sockaddr_storage));
			parse_sockaddr(remote_addr, remote_ip, &remote_port);

			int sock_fd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
			if (sock_fd == -1) {
				perror("socket failed\n");
				freeaddrinfo(servinfo);
				return -1;
			}
			socklen_t addr_len = sizeof(struct sockaddr_storage);
			if (getsockname(sock_fd, local_addr, &addr_len) < 0) {
				perror("getsockname failed");
			}
			parse_sockaddr(local_addr, local_ip, &local_port);
			int bytes_sent = sendto(sock_fd, message, 8, 0, servinfo->ai_addr, servinfo->ai_addrlen);
			if (bytes_sent < 8) {
				perror("sendto failed\n");
				close(sock_fd);
				freeaddrinfo(servinfo);
				return -1;
			}
			unsigned char response[256];

			char treasure[1024] = {0};
			int treasure_index = 0;
			
			while (1) {
				int bytes_received = recvfrom(sock_fd, response, 256, 0, NULL, NULL);
				if (bytes_received == -1) {
					perror("recvfrom failed");
					close(sock_fd);
					freeaddrinfo(servinfo);
					return -1;
				}
				
				// printf("Received %d bytes:\n", bytes_received);
				// print_bytes(response, bytes_received);

				if (bytes_received < 1) {
					printf("Response too short\n");
					close(sock_fd);
					freeaddrinfo(servinfo);
					return -1;
				}
				
				unsigned char chunklen = response[0];

				// ADD BREAK CONDITION
				if (chunklen == 0) {
					break;
				}

				char chunk[128];
				if (chunklen > 0 && chunklen <= 127) {
					if (bytes_received < (1 + chunklen + 7)) {
						printf("Response too short for chunklen %d\n", chunklen);
						close(sock_fd);
						freeaddrinfo(servinfo);
						return -1;
					}
					memcpy(chunk, &response[1], chunklen);
					chunk[chunklen] = '\0';
					
					// ACCUMULATE TREASURE
					if (treasure_index + chunklen < sizeof(treasure)) {
						memcpy(treasure + treasure_index, chunk, chunklen);
						treasure_index += chunklen;
						treasure[treasure_index] = '\0';
					}
				} else {
					chunk[0] = '\0';
				}

				if (bytes_received >= (chunklen + 6)) {
					unsigned char opcode = response[chunklen + 1];
					unsigned short opparam;
					memcpy(&opparam, &response[chunklen + 2], 2);
					opparam = ntohs(opparam);
					unsigned int nonce;
					memcpy(&nonce, &response[chunklen + 4], 4);
					nonce = ntohl(nonce);
					
					// printf("chunklen: %u\n", chunklen);
					// printf("chunk: %s\n", chunk);
					// printf("opcode: %u\n", opcode);
					// printf("opparam: %x\n", opparam);
					// printf("nonce: %x\n", nonce);

					if (opcode == 1) {
						remote_port = opparam;  // Use the op-param as new remote port
						populate_sockaddr(remote_addr, addr_fam, remote_ip, remote_port);
						// printf("Changed remote port to: %d\n", remote_port);
					}
					else if (opcode == 2) {
						local_port = opparam;
						close(sock_fd);
						sock_fd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
						if (sock_fd == -1) {
							perror("socket failed");
							freeaddrinfo(servinfo);
							return -1;
						}
						populate_sockaddr(local_addr, addr_fam, NULL, local_port);
						if (bind(sock_fd, local_addr, sizeof(struct sockaddr_storage)) < 0) {
							perror("bind() failed");
							close(sock_fd);
							freeaddrinfo(servinfo);
							return -1;
						}
						
						// printf("Changed local port to: %d\n", local_port);
					}
					else if (opcode == 3) {
						unsigned short m = opparam;
						unsigned int port_sum = 0;
						
						// printf("Opcode 3: Receiving %d datagrams\n", m);
						
						for (int i = 0; i < m; i++) {
							struct sockaddr_storage temp_addr_ss;
							struct sockaddr *temp_addr = (struct sockaddr *)&temp_addr_ss;
							char temp_ip[INET6_ADDRSTRLEN];
							unsigned short temp_port;
							
							unsigned char temp_buf[256];
							socklen_t addr_len = sizeof(struct sockaddr_storage);
							
							int bytes_received = recvfrom(sock_fd, temp_buf, 256, 0, temp_addr, &addr_len);
							if (bytes_received >= 0) {
								parse_sockaddr(temp_addr, temp_ip, &temp_port);
								port_sum += temp_port;
								// printf("Received datagram from port: %d\n", temp_port);
							}
						}
						// printf("Port sum: %u\n", port_sum);
						nonce = port_sum;
					}
					if (chunklen != 0) {
						unsigned int next_nonce = nonce + 1;
						uint32_t network_nonce = htonl(next_nonce);
						unsigned char followup_msg[4];
						memcpy(followup_msg, &network_nonce, 4);

						// printf("Follow-up request:\n");
						// print_bytes(followup_msg, 4);
						sendto(sock_fd, followup_msg, 4, 0, remote_addr, sizeof(struct sockaddr_storage));
					}
				} else {
					printf("Response too short to parse opcode/opparam/nonce\n");
					break;
				}
			}
			printf("%s\n", treasure);

			close(sock_fd);
			freeaddrinfo(servinfo);
		} else {
			perror("getaddrinfo failed");
			return -1;
		}
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
