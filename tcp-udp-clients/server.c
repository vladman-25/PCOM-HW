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

struct client *clients;
int max;
fd_set file_descr;
#define MAX_CLIENTS_NO 100
#define FD_START 4

int find_client(char buffer[PACKLEN]) {
	for(int i = FD_START; i <= max; i++) {
		if(strcmp(clients[i].id, buffer) == 0) {
			return i;
		}
	}
	return -1;
}


int add_client(int socket, char buffer[PACKLEN]) {
	strcpy(clients[socket].id, buffer);
	clients[socket].socket = socket;
	clients[socket].online = ONLINE;

	if(socket > max) {
		max = socket;
	}

}

int print_client_status(int socket, char status, struct sockaddr_in* new_tcp) {
	if(status == 'c') {
		printf("New client %s connected from %s:%d\n", 
			clients[socket].id,
			inet_ntoa(new_tcp->sin_addr), 
			ntohs(new_tcp->sin_port));
		return 0;
	}
	if (status == 'a') {
		printf("Client %s already connected.\n", clients[socket].id);
		return 0;
	}
	if (status == 'd') {
		printf("Client %s disconnected.\n", clients[socket].id);
	}

}

int client_send_unsent(int socket) {
	for(int i = 0; i < clients[socket].nr_unsent; i++){
		int ret = send(clients[socket].socket, 
						&clients[socket].unsent[i],
						sizeof(struct tcp_msg), 
						0);
		DIE(ret < 0, "send");
	}
	clients[socket].nr_unsent = 0;
}
////////////////////////////////////////////////////////////////////
#define NEGATIVE 1

struct tcp_msg tcp_INT(struct udp_msg *udp_msg, struct tcp_msg tcp_msg) {
	strcpy(tcp_msg.data_type, "INT");
	uint32_t x = ntohl(*(uint32_t *)(udp_msg->content + 1));
	if(udp_msg->content[0] == NEGATIVE) {
		x *= -1;
	}
	sprintf(tcp_msg.content, "%d", x);
	return tcp_msg;
}

struct tcp_msg tcp_SHORT(struct udp_msg *udp_msg, struct tcp_msg tcp_msg) {
	strcpy(tcp_msg.data_type, "SHORT_REAL");
	float x = ntohs(*(uint16_t *)(udp_msg->content));
	if (x < 0.0) {
		x *= - 1;
	}
	x /= 100;
	sprintf(tcp_msg.content, "%.2f", x);
	return tcp_msg;
}

struct tcp_msg tcp_FLOAT(struct udp_msg *udp_msg, struct tcp_msg tcp_msg) {
	strcpy(tcp_msg.data_type, "FLOAT");
	double x = ntohl(*(uint32_t *)(udp_msg->content + 1));
	for(int i = 0; i < udp_msg->content[5]; i++) {
		x /= 10;
	}
	if(udp_msg->content[0] == NEGATIVE) {
		x *= -1;
	}
	sprintf(tcp_msg.content, "%lf", x);
	return tcp_msg;
}	

struct tcp_msg tcp_STRING(struct udp_msg *udp_msg, struct tcp_msg tcp_msg) {
	strcpy(tcp_msg.data_type, "STRING");
	strcpy(tcp_msg.content, udp_msg->content);
	return tcp_msg;
}


struct tcp_msg create_tcp_pack(char buffer[PACKLEN],struct sockaddr_in* udp_addr) {
	
	struct tcp_msg tcp_msg;
	memset(&tcp_msg, 0, sizeof(struct tcp_msg));
	tcp_msg.port = htons(udp_addr->sin_port);
	strcpy(tcp_msg.ip, inet_ntoa(udp_addr->sin_addr));

	struct udp_msg *udp_msg;
	udp_msg = (struct udp_msg *)buffer;

	strcpy(tcp_msg.topic, udp_msg->topic);
	tcp_msg.topic[50] = 0;



	if (udp_msg->data_type == 0) {
		return tcp_INT(udp_msg,tcp_msg);
	}
	if (udp_msg->data_type == 1) {
		return tcp_SHORT(udp_msg,tcp_msg);
	}
	if (udp_msg->data_type == 2) {
		return tcp_FLOAT(udp_msg,tcp_msg);
	}
	if (udp_msg->data_type == 3) {
		return tcp_STRING(udp_msg,tcp_msg);
	}
}

int send_tcp_pack(struct tcp_msg tcp_msg) {
	for(int i = FD_START; i <= max; i++) {
		for(int j = 0; j < clients[i].nr_topics; j++) {
			if (strcmp(clients[i].topics[j].name, tcp_msg.topic) == 0) {
				if(clients[i].online == ONLINE){
					int ret = send(clients[i].socket, 
									&tcp_msg,
									sizeof(struct tcp_msg), 
									0);
					DIE(ret < 0, "send");
				} else if (clients[i].topics[j].SF == 1) {
					clients[i].unsent[clients[i].nr_unsent++] = tcp_msg;
				}
				break;
			}
		}
	}
}

client* get_client(int idx) {
	return &clients[idx];
}

void subscribe(client* client, struct msg *input) {
	for(int i = 0; i < client->nr_topics; i++) {
		if (strcmp(client->topics[i].name, input->topic) == 0) {
			return;
		}
	}

	strcpy(client->topics[client->nr_topics].name, input->topic);
	client->topics[client->nr_topics].SF = input->data_type;
	client->nr_topics++;

}
void unsubscribe(client* client, struct msg *input) {
	int idx = -1;
	for(int i = 0; i < client->nr_topics; i++) {
		if(strcmp(client->topics[i].name, input->topic) == 0) {
			idx = i;
			break;
		}
	}
	if (idx >= 0) {
		for(int i = idx; i < client->nr_topics; i++) {
			client->topics[i] = client->topics[i+1];
		}
		client->nr_topics--;
	}
}

void disconnect(int idx) {
	print_client_status(idx, 'd', NULL);
	clients[idx].online = 0;
	clients[idx].socket = -1;
	FD_CLR(idx, &file_descr);
	close(idx);
}
////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[]) {

	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
	clients = calloc(MAX_CLIENTS_NO, sizeof(struct client));

	int flag = 1;
	max = 0;
	fd_set read_fds;
	fd_set tmp_fds;
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

    int udp_sock = socket(PF_INET, SOCK_DGRAM, 0);
	DIE(udp_sock < 0, "socket");

	int tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
	DIE(tcp_sock < 0, "socket");

    struct sockaddr_in serv_addr;
	struct sockaddr_in udp_addr;
	struct sockaddr_in new_tcp;

	int port = atoi(argv[1]);

	memset((char *)&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	int tcp_bnd  = bind(tcp_sock, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr));
	DIE(tcp_bnd < 0, "bind tcp");

	udp_addr.sin_family = AF_INET;
	udp_addr.sin_port = htons(port);
	udp_addr.sin_addr.s_addr = INADDR_ANY;

	int udp_bnd = bind(udp_sock, (struct sockaddr *)&udp_addr, sizeof(struct sockaddr));
	DIE(udp_bnd < 0, "bind udp");

	setsockopt(tcp_sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));

	int tcp_lsn = listen(tcp_sock, INT_MAX);
	DIE(tcp_lsn < 0, "listen tcp");

	FD_ZERO(&file_descr);

	FD_SET(tcp_sock, &file_descr);
	FD_SET(udp_sock, &file_descr);

	FD_SET(STDIN_FILENO, &file_descr);

	socklen_t udp_len = sizeof(struct sockaddr);

	if(tcp_sock > udp_sock)
		max = tcp_sock;
	else max = udp_sock;

    int on = 1;

    while(on) {
        fd_set aux_set = file_descr;
		int sel = select(max + 1, &aux_set, NULL, NULL, NULL);
		DIE(sel < 0, "select");
		char buffer[PACKLEN];

		for (int i = 0; i <= max; i++) {

			if (FD_ISSET(i, &aux_set)) {
				memset(buffer, 0, PACKLEN);



				if (i == STDIN_FILENO) {
					fgets(buffer, PACKLEN, stdin);
					if(strncmp(buffer, "exit", 4) == 0) {
						on = 0;
						break;
					}



				} else if (i == tcp_sock) {
					int socket = accept(tcp_sock, 
										(struct sockaddr *) 
										&new_tcp, 
										&udp_len);
					DIE(socket < 0, "accept");

					int ret = recv(socket, buffer, 10, 0);
					DIE(ret < 0, "recv");

					int found = find_client(buffer);
					int online = OFFLINE;
					if (found != -1) {
						online = clients[found].online;
					}

					if(found == -1) {
						FD_SET(socket, &file_descr);

						add_client(socket, buffer);
						print_client_status(socket,'c',&new_tcp);
						break;
					}
					if((online == OFFLINE) && (found != -1)) {
						FD_SET(socket, &file_descr);

						clients[socket].socket = socket;
						clients[socket].online = ONLINE;
						print_client_status(socket,'c',&new_tcp);
						client_send_unsent(socket);
						break;
					}
					if ((online == ONLINE) && (found != -1)) {
						close(socket);
						print_client_status(found,'a',&new_tcp);
						break;
					}
					


				} else if (i == udp_sock) {
					
					int ret = recvfrom(udp_sock, 
										buffer, 
										PACKLEN, 
										0, 
										(struct sockaddr *)&udp_addr,
										&udp_len);
					DIE(ret < 0, "recvfrom");

					struct tcp_msg send_to_tcp = create_tcp_pack(buffer,&udp_addr);
					send_tcp_pack(send_to_tcp);

				} else {

					memset(buffer, 0, PACKLEN);
					int ret = recv(i, buffer, PACKLEN, 0);
					DIE(ret < 0, "recv");

					if(ret > 0) {
						struct msg *input = (struct msg *) buffer;
						client* client = get_client(i);

						if (input->type == SUBSCRIBE) {
							subscribe(client, input);
						}

						if (input->type == UNSUBSCRIBE) {
							unsubscribe(client, input);
						}

						if (input->type == DISCONNECT) {
							disconnect(i);
						}
					}
					if (ret == 0) {
						disconnect(i);
					}
				}
			}
		}
	}
	for(int i = FD_START; i <= max; i++) {
		if(FD_ISSET(i, &file_descr))
			close(i);
	}
	close(tcp_sock);
	close(udp_sock);

    return 0;
}