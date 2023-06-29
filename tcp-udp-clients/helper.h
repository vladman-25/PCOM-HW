#ifndef _HELPER_H
#define _HELPER_H 1

//content of helper
#define DIE(condition, message) \
	do { \
		if ((condition)) { \
			fprintf(stderr, "[(%s:%d)]: %s\n", __FILE__, __LINE__, (message)); \
			perror(""); \
			exit(1); \
		} \
	} while (0)
#define TOPIC_LEN 50
#define CONTENT_LEN 1500
#define IP_LEN 16


typedef struct udp_msg {
    char topic[TOPIC_LEN];
    uint8_t data_type;
    char content[CONTENT_LEN];
} udp_msg;

#define SUBSCRIBE 1
#define UNSUBSCRIBE 2
#define DISCONNECT 3

#define ONLINE 1
#define OFFLINE 0


typedef struct msg {
	char data_type;

	char topic[TOPIC_LEN + 1];
	uint8_t type; 
	char content[CONTENT_LEN + 1];

	char ip[IP_LEN];
	uint16_t port;
	int SF;
} msg;

typedef struct tcp_msg {
	char ip[IP_LEN];
	uint16_t port;
	char data_type[11];
	char topic[TOPIC_LEN + 1];
	
	char content[CONTENT_LEN + 1];
} tcp_msg;

typedef struct topic{
	char name[TOPIC_LEN + 1];
	int SF;
} topic;

typedef struct client{
	char id[10];
	int socket;
	int nr_topics;
	int nr_unsent;
	struct tcp_msg unsent[100];
	struct topic topics[100];
	int online; 
} client;

#define PACKLEN sizeof(struct msg)

#endif