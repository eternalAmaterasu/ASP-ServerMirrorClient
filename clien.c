//
// Created by v2 on 11/04/23.
//
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>

#define SERVER_PORT 8080
#define MIRROR_PORT 8081
#define NULL_CH '\0'

int IP_LENGTH = 25;
int BUFFER_LENGTH = 1024;
char *mirrorRegistrationStartingMessage = "Mir=";

int connectAndGetFdClient(char *serverIp, int port) {
    int fdClient;
    long int bytesReceived;
    struct sockaddr_in serv_addr;

    if ((fdClient = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        exit(-1);
    }
    printf("Socket creation complete for client...\n");

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, serverIp, &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        exit(-1);
    }

    printf("Attempting contact now...\n");
    if ((connect(fdClient, (struct sockaddr *) &serv_addr, sizeof(serv_addr))) < 0) {
        printf("\nConnection Failed \n");
        exit(-1);
    }
    return fdClient;
}

void cleanBuffer(char data[]) {
    for (int i = 0; i < BUFFER_LENGTH; i++) data[i] = NULL_CH;
}

int main(int argc, char const *argv[]) {
    if (argc != 2) {
        printf("Cannot start mirror. Need server's IP address as the 1st parameter...\n");
        exit(0);
    }
    char *serverIp = argv[1];
    //int *fdClient = NULL;
    int fdClient;
    char *mirrorIp = malloc(IP_LENGTH * sizeof(char));
    fdClient = connectAndGetFdClient(serverIp, SERVER_PORT);

    char *buffer = malloc(BUFFER_LENGTH * sizeof(char));
    cleanBuffer(buffer);
    long int bytesReceived;
    char *initMsg = "Hello from the client..\n";

    printf("Initiating server contact, with fdclient as %d...\n", fdClient);
    send(fdClient, initMsg, strlen(initMsg), 0);
    bytesReceived = read(fdClient, buffer, 1024);

    if (bytesReceived > 10 && bytesReceived < 25 && strncmp(mirrorRegistrationStartingMessage, buffer, 4) == 0) {
        close(fdClient);

        for (int i = 0; i < IP_LENGTH && buffer[i + 4] != ';'; i++)
            mirrorIp[i] = buffer[i + 4];

        printf("Rerouting to connect to the mirror...\n");
        fdClient = connectAndGetFdClient(mirrorIp, MIRROR_PORT);

        send(fdClient, initMsg, strlen(initMsg), 0);
        cleanBuffer(buffer);
        bytesReceived = read(fdClient, buffer, 1024);
    }
    printf("client received %ld bytes with data being '%s'..\n", bytesReceived, buffer);

    sleep(5);
    printf("client bye\n");
    // closing the connected socket
    close(fdClient);
    return 0;
}