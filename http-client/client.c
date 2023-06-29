#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"
#include "parson.h"

#define IP_SERVER "34.241.4.235"
#define PORT_SERVER 8080
char** cookies;
char *token;

int register_command(char user[BUFLEN], char pass[BUFLEN]) {
    char *message;
    char *response;
    int sockfd;

    char** body_data = (char**)malloc(2*sizeof(char*));
    

    JSON_Value *root_value;
    JSON_Object *root_object;
    char *serialized_string = NULL;
    sockfd = open_connection(IP_SERVER, PORT_SERVER, AF_INET, SOCK_STREAM, 0);

    root_value = json_value_init_object();
    root_object = json_value_get_object(root_value);
    json_object_set_string(root_object, "username", user);
    json_object_set_string(root_object, "password", pass);
    serialized_string = json_serialize_to_string_pretty(root_value);

    body_data[0] = malloc(100);
    strcpy(body_data[0],serialized_string);

    message = compute_post_request(IP_SERVER,
                "/api/v1/tema/auth/register",
                "application/json",
                body_data,
                1,
                NULL,
                0,
                NULL,
                0);
    send_to_server(sockfd, message);

    response = receive_from_server(sockfd);
    //printf("Am primit:\n%s\n", response);
    //printf("\n===============\n============\n");
    if(strstr(response,"Created")) {
        printf("200 - OK - Utilizator inregistrat cu succes!\n");
    }
    if(strstr(response,"Bad Request")) {
        printf("400 - ERROR - Utilizator deja exista!\n");
    }
    json_free_serialized_string(serialized_string);
    json_value_free(root_value);
    free(body_data[0]);
    close(sockfd);
    return 1;
}
int login_command(char user[BUFLEN], char pass[BUFLEN]) {
    char *message;
    char *response;
    int sockfd;

    char** body_data = (char**)malloc(2*sizeof(char*));
    

    JSON_Value *root_value;
    JSON_Object *root_object;
    char *serialized_string = NULL;
    root_value = json_value_init_object();
    root_object = json_value_get_object(root_value);
    json_object_set_string(root_object, "username", user);
    json_object_set_string(root_object, "password", pass);
    serialized_string = json_serialize_to_string_pretty(root_value);

    body_data[0] = malloc(100);
    strcpy(body_data[0],serialized_string);

    sockfd = open_connection(IP_SERVER, PORT_SERVER, AF_INET, SOCK_STREAM, 0);
    message = compute_post_request(IP_SERVER,
                "/api/v1/tema/auth/login",
                "application/json",
                body_data,
                1,
                NULL,
                0,
                NULL,
                0);
    send_to_server(sockfd, message);

    response = receive_from_server(sockfd);

    if(strstr(response,"OK")) {
        printf("200 - OK - Bun venit!\n");
    }
    if(strstr(response,"Credentials")) {
        printf("400 - ERROR - Parola incorecta!\n");
        return 0;
    }
    if(strstr(response,"username")) {
        printf("400 - ERROR - Utilizator nu exista!\n");
        return 0;
    }

    json_free_serialized_string(serialized_string);
    json_value_free(root_value);
    free(body_data[0]);

    char* aux = strstr(response,"connect");
    char* aux2 = strstr(aux,"Date: ");
    aux2[0] = '\0';
    aux[strlen(aux)-2] = '\0';


    cookies[0] = calloc(1000,1);
    strcpy(cookies[0],aux);

    close(sockfd);
    return 1;
}

int enter_lib() {
    char *message;
    char *response;
    int sockfd;

    char** body_data = (char**)malloc(2*sizeof(char*));
    

    if(cookies[0] == NULL) {
        printf("400 - ERROR - Nu te-ai autentificat!\n");
        return 1;
    }
    sockfd = open_connection(IP_SERVER, PORT_SERVER, AF_INET, SOCK_STREAM, 0);
    message = compute_get_request(IP_SERVER,
                    "/api/v1/tema/library/access",
                    NULL,
                    cookies,
                    1,
                    NULL,
                    0);
    send_to_server(sockfd, message);

    response = receive_from_server(sockfd);

    if(strstr(response,"OK")) {
        printf("200 - OK - Ai intrat in librarie!\n");
    }

    close(sockfd);

    body_data[0] = basic_extract_json_response(response);
    body_data[0] = strstr(body_data[0], ":");
    body_data[0] = strstr(body_data[0], "\"");

    
    
    token = calloc(1000,1);
    token = strtok(body_data[0],"\"");
    return 1;
}
int get_book(int id) {
    char *message;
    char *response;
    int sockfd;

    char** body_data = (char**)malloc(2*sizeof(char*));
    

    char url[50];
    sprintf(url,"/api/v1/tema/library/books/%d",id);

    if(cookies[0] == NULL) {
        printf("400 - ERROR - Nu te-ai autentificat!\n");
        return 1;
    }
    if(token == NULL) {
        printf("400 - ERROR - Nu esti in librarie!\n");
        return 1;
    }
    sockfd = open_connection(IP_SERVER, PORT_SERVER, AF_INET, SOCK_STREAM, 0);
    body_data[0] = token;

    message = compute_get_request(IP_SERVER,
                    url,
                    NULL,
                    NULL,
                    0,
                    body_data,
                    1);
    send_to_server(sockfd, message);

    response = receive_from_server(sockfd);
    if(strstr(response,"404")) {
        printf("404 - ERROR - Cartea cu id=%d nu exista!\n",id);
    } else {
        printf("%s\n",basic_extract_json_response(response));
    }

    close(sockfd);
    return 1;
}
int add_book(char title[BUFLEN],char author[BUFLEN],
             char genre[BUFLEN],char publisher[BUFLEN],
            int page_count) {
    if(cookies[0] == NULL) {
        printf("400 - ERROR - Nu te-ai autentificat!\n");
        return 1;
    }
    if(token == NULL) {
        printf("400 - ERROR - Nu esti in librarie!\n");
        return 1;
    }
    char *message;
    char *response;
    int sockfd;

    char** body_data = (char**)malloc(2*sizeof(char*));
    char** auth = (char**)malloc(2*sizeof(char*));

    JSON_Value *root_value;
    JSON_Object *root_object;
    char *serialized_string = NULL;
    root_value = json_value_init_object();
    root_object = json_value_get_object(root_value);
    json_object_set_string(root_object, "title", title);
    json_object_set_string(root_object, "author", author);
    json_object_set_string(root_object, "genre", genre);
    json_object_set_string(root_object, "publisher", publisher);
    json_object_set_number(root_object, "page_count", page_count);
    serialized_string = json_serialize_to_string_pretty(root_value);
     
    
    auth[0] = token;


    body_data[0] = malloc(100);
    strcpy(body_data[0],serialized_string);

    sockfd = open_connection(IP_SERVER, PORT_SERVER, AF_INET, SOCK_STREAM, 0);
    message = compute_post_request(IP_SERVER,
                "/api/v1/tema/library/books",
                "application/json",
                body_data,
                1,
                NULL,
                0,
                auth,
                1);

    send_to_server(sockfd, message);
    

    response = receive_from_server(sockfd);
    printf("200 - OK - Cartea adaugata cu succes!\n");
  
    close(sockfd);
    return 1;
}
int get_books() {
    char *message;
    char *response;
    int sockfd;

    char** body_data = (char**)malloc(2*sizeof(char*));
    if(cookies[0] == NULL) {
        printf("400 - ERROR - Nu te-ai autentificat!\n");
        return 1;
    }
    if(token == NULL) {
        printf("400 - ERROR - Nu esti in librarie!\n");
        return 1;
    }

    body_data[0] = token;
    sockfd = open_connection(IP_SERVER, PORT_SERVER, AF_INET, SOCK_STREAM, 0);
    

    message = compute_get_request(IP_SERVER,
                    "/api/v1/tema/library/books",
                    NULL,
                    NULL,
                    0,
                    body_data,
                    1);
    send_to_server(sockfd, message);

    response = receive_from_server(sockfd);
    // printf("%s\n",response);
    if(basic_extract_json_response(response) == NULL) {
        printf("400 - ERROR - Nu exista carti!\n");
        return 0;
    }
    printf("%s\n",basic_extract_json_response(response));
    close(sockfd);
    return 1;
}
int delete_book(int id) {
    if(cookies[0] == NULL) {
        printf("400 - ERROR - Nu te-ai autentificat!\n");
        return 1;
    }
    if(token == NULL) {
        printf("400 - ERROR - Nu esti in librarie!\n");
        return 1;
    }
    char *message;
    char *response;
    int sockfd;

    char** auth = (char**)malloc(2*sizeof(char*));
    
    char url[50];
    sprintf(url,"/api/v1/tema/library/books/%d",id);
    auth[0] = token;
    sockfd = open_connection(IP_SERVER, PORT_SERVER, AF_INET, SOCK_STREAM, 0);
    message = compute_delete_request(IP_SERVER,
                    url,
                    NULL,
                    NULL,
                    0,
                    auth,
                    1);
    send_to_server(sockfd, message);

    response = receive_from_server(sockfd);
    if(strstr(response,"200 OK")) {
        printf("200 - OK - Cartea cu id=%d a fost stearsa!\n",id);
    }
    if(strstr(response,"404")) {
        printf("404 - ERROR - Cartea cu id=%d nu exista!\n",id);
    }
    
    close(sockfd);
    return 1;
}

int logout() {
    if(cookies[0] == NULL) {
        printf("400 - ERROR - Nu te-ai autentificat!\n");
        return 1;
    }
    char *message;
    char *response;
    int sockfd;

    sockfd = open_connection(IP_SERVER, PORT_SERVER, AF_INET, SOCK_STREAM, 0);
    message = compute_get_request(IP_SERVER,
                    "/api/v1/tema/auth/logout",
                    NULL,
                    cookies,
                    1,
                    NULL,
                    0);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    if(strstr(response,"200 OK")) {
        printf("200 - OK - Ai iesit din cont!\n");
    } else {
        printf("Am primit:\n%s\n", response);
    }

    close(sockfd);
    return 1;

}

int main() {
    cookies = (char**)malloc(sizeof(char*));
    int online = 1;
    cookies[0] = NULL;
    token = NULL;
    while(online) {
        char buffer[BUFLEN];
        memset(buffer, 0, BUFLEN);
        fgets(buffer, BUFLEN, stdin);

        if (strncmp(buffer, "exit", 4) == 0) {
            online = 0;
        }
        if (strncmp(buffer, "register", 8) == 0) {
            printf("username=");
            char username[BUFLEN];
            memset(username, 0, BUFLEN);
            fgets(username, BUFLEN, stdin);
            username[strlen(username)-1] = '\0';
            
            char password[BUFLEN];
            printf("password=");
            memset(password, 0, BUFLEN);
            fgets(password, BUFLEN, stdin);
            password[strlen(password)-1] = '\0';
            
            int x = register_command(username,password);
            if (x == -1) {
                continue;
            }
        }
        if (strncmp(buffer, "login", 5) == 0) {
            printf("username=");
            char username[BUFLEN];
            memset(username, 0, BUFLEN);
            fgets(username, BUFLEN, stdin);
            username[strlen(username)-1] = '\0';
            
            char password[BUFLEN];
            printf("password=");
            memset(password, 0, BUFLEN);
            fgets(password, BUFLEN, stdin);
            password[strlen(password)-1] = '\0';
            
            login_command(username,password);
        }
        if (strncmp(buffer, "enter_library", 13) == 0) {
            enter_lib();
        }
        if (strncmp(buffer, "get_books", 9) == 0) {
            get_books();
            continue;
        }
        if (strncmp(buffer, "get_book", 8) == 0) {
            int id;
            printf("id=");
            scanf("%d", &id);
            get_book(id);
            continue;
        }
        if (strncmp(buffer, "add_book", 8) == 0) {
            printf("title=");
            char title[BUFLEN];
            memset(title, 0, BUFLEN);
            fgets(title, BUFLEN, stdin);
            title[strlen(title)-1] = '\0';
            
            char author[BUFLEN];
            printf("author=");
            memset(author, 0, BUFLEN);
            fgets(author, BUFLEN, stdin);
            author[strlen(author)-1] = '\0';

            printf("genre=");
            char genre[BUFLEN];
            memset(genre, 0, BUFLEN);
            fgets(genre, BUFLEN, stdin);
            genre[strlen(genre)-1] = '\0';
            
            char publisher[BUFLEN];
            printf("publisher=");
            memset(publisher, 0, BUFLEN);
            fgets(publisher, BUFLEN, stdin);
            publisher[strlen(publisher)-1] = '\0';

            int page_count;
            printf("page_count=");
            scanf("%d", &page_count);
            if(page_count <= 0) {
                printf("ERROR - Numarul paginilor nu este format int\n");
                continue;
            }

            add_book(title,author,genre,publisher,page_count);
        }
        if (strncmp(buffer, "delete_book", 11) == 0) {
            int id;
            printf("id=");
            scanf("%d", &id);
            delete_book(id);
            continue;
        }
        if (strncmp(buffer, "logout", 5) == 0) {
            logout();
            cookies[0] = NULL;
            token = NULL;
        }
    }
    return 0;
}