#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <limits.h>
#include "helper.h"

#define BUFLEN 100

int main(int argc, char* argv[]) {

    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	DIE(argc < 4, "arguments");

	int tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
	DIE(tcp_sock < 0, "socket");

	struct sockaddr_in server;
	int port = atoi(argv[3]);

	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	inet_aton(argv[2], &server.sin_addr);

	fd_set file_descr;
	FD_ZERO(&file_descr);

	FD_SET(tcp_sock, &file_descr);
	FD_SET(STDIN_FILENO, &file_descr);

	int	connection = connect(tcp_sock, (struct sockaddr *)&server, sizeof(server));
	DIE(connection < 0, "connect");

	int ret = send(tcp_sock, argv[1], 10, 0);
	DIE(ret < 0, "send");

	struct msg pack;

	int flag = 1;
	setsockopt(tcp_sock, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
	int on = 1;

	while(on) {
		fd_set aux_set = file_descr;

		int sel = select(tcp_sock + 1, &aux_set, NULL, NULL, NULL);
		DIE(sel < 0, "select");

		if (FD_ISSET(STDIN_FILENO, &aux_set)) {
			char buffer[BUFLEN];
			memset(buffer, 0, BUFLEN);
			fgets(buffer, BUFLEN, stdin);
			memset(&pack, 0, PACKLEN);
			
			int command = 0;
			if (strncmp(buffer, "exit", 4) == 0) {
				command = DISCONNECT;
			}
			if (strncmp(buffer, "subscribe", 9) == 0) {
				command = SUBSCRIBE;
			}
			if (strncmp(buffer, "unsubscribe", 11) == 0) {
				command = UNSUBSCRIBE;
			}
			
			if(command) {
				pack.type = command;
				if (command != DISCONNECT) {
					char *token = strtok(buffer, " ");
					token = strtok(NULL, " ");
					strcpy(pack.topic, token);
					if (command == SUBSCRIBE) {
						token = strtok(NULL, " ");
						pack.data_type = token[0] - '0';
						printf("Subscribed to topic.\n");
					} else {
						printf("Unsubscribed from topic.\n");
					}
				}
				int sending = send(tcp_sock, &pack, PACKLEN, 0);
				DIE (sending < 0, "send");
				if(command == DISCONNECT) {
					on = 0;
				}
			} else {
				printf("Invalid cmd.\n");
			}
		}

		if(FD_ISSET(tcp_sock, &aux_set)) {

			char buffer[sizeof(struct tcp_msg)];
			memset(buffer, 0, sizeof(struct tcp_msg));

			int ret = recv(tcp_sock, buffer, sizeof(struct tcp_msg), 0);
			DIE(ret < 0, "receive");

			if(ret != 0) {
				struct tcp_msg *msg_recv = (struct tcp_msg *)buffer;
				printf("%s:%u - %s - %s - %s\n", 
					msg_recv->ip,
					msg_recv->port,
					msg_recv->topic,
					msg_recv->data_type, 
					msg_recv->content);
			} else {
				on = 0;
			}
		}
	}

	close(tcp_sock);
	return 0;
}