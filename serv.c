//
// Created by v2 on 11/04/23.
//
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERVER_PORT 8080
#define MIRROR_PORT 8081

int SOCKET_WAITING_QUEUE_SIZE = 3;
int IP_LENGTH = 25;
char *mirrorIp = NULL;
char *redirectMessageForClient = NULL;
char *mirrorRegistrationStartingMessage = "Mir=";
char *MIRROR_ACK = "OK";

int decideConnectionServer(int connectionNumber) {
    return connectionNumber <= 4 || (connectionNumber >= 9 && (connectionNumber & 1) == 1);
}

void processClient(int socket, char buffer[], long int bytesRead) {
    int cid = fork();
    if (cid == 0) {
        printf("%d => %ld bytes read. String => '%s'\n", getpid(), bytesRead, buffer);
        send(socket, MIRROR_ACK, strlen(MIRROR_ACK), 0);
        printf("Hello message sent\n");

        // closing the connected socket
        close(socket);
        exit(0);
    } else {
        printf("from the root process.. nothing else to do?\n");
    }
}

int main(int argc, char const *argv[]) {
    int root = getpid();
    printf("root pid: %d\n", root);

    mirrorIp = malloc(IP_LENGTH * sizeof(char));
    redirectMessageForClient = malloc((IP_LENGTH + 6) * sizeof(char));

    int server_fd, socketConn;
    long int bytesRead;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) { //AF_INET for IPv4, SOCK_STREAM -- TCP
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(SERVER_PORT);

    if (bind(server_fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, SOCKET_WAITING_QUEUE_SIZE) < 0) { //limiting to 3 requests in the queue
        perror("listen");
        exit(EXIT_FAILURE);
    }

    int connectionsServiced = 0;
    printf("Entering listening state now...\n");
    while (1) {
        printf("Waiting for accepting..\n");
        if ((socketConn = accept(server_fd, (struct sockaddr *) &address, (socklen_t *) &addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        char buffer[1024] = {0};

        bytesRead = read(socketConn, buffer, 1024);
        if (bytesRead == 18 && strncmp(mirrorRegistrationStartingMessage, buffer, 4) == 0) {
            strncpy(redirectMessageForClient, buffer, 18);
            //this msg was sent by the mirror, and shall not count as a client connection.
            for (int i = 0; i < IP_LENGTH && buffer[i + 4] != ';'; i++)
                mirrorIp[i] = buffer[i + 4];

            send(socketConn, MIRROR_ACK, strlen(MIRROR_ACK), 0);
            printf("Registered mirror ip as: %s, of length: %lu\n", mirrorIp, strlen(mirrorIp));
            continue;
        }

        connectionsServiced++;
        if (decideConnectionServer(connectionsServiced)) {
            //server will handle this request
            printf("Accepted a request from the queue..\n");
            processClient(socketConn, buffer, bytesRead);

        } else {
            //mirror will handle this request
            printf("Advising redirect to the client with the mirror address. Server will not service this request..\n");
            send(socketConn, redirectMessageForClient, strlen(redirectMessageForClient), 0);
            printf("Redirect command sent to client..\n");
        }
    }
    // closing the listening socketConn
    shutdown(server_fd, SHUT_RDWR);
    return 0;
}
