//
// Created by v2 on 11/04/23.
//
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <malloc.h>
#include <stdlib.h>

#define SERVER_PORT 8080
#define MIRROR_PORT 8081
#define NULL_CH '\0'

char *getCurrentHostIp() {
    int IP_LENGTH = 25;
    char hostName[256];
    struct hostent *hostData;
    char *ip = malloc(IP_LENGTH * sizeof(char));

    if (gethostname(hostName, sizeof(hostName)) >= 0) {
        hostData = gethostbyname(hostName);
        char *data = inet_ntoa(*((struct in_addr *) hostData->h_addr_list[0]));
        for (int i = 0; i < IP_LENGTH; i++) ip[i] = NULL_CH;
        strcpy(ip, data);
    }
    return ip;
}

int registerOntoServer(char *hostIp, char *serverIp) {
    int ipLength = (int) strlen(hostIp);
    char initMsg[31] = {NULL_CH};
    strncpy(initMsg, "Mir=", 4);
    int i = 0;
    for (; i < ipLength; i++) initMsg[i + 4] = hostIp[i];
    initMsg[i + 4] = ';';
    printf("initmsg => %s, length: %lu\n", initMsg, strlen(initMsg));

    int fd_mirror_client;
    long int bytesReceived;
    struct sockaddr_in serv_addr;

    char buffer[1024] = {0};
    if ((fd_mirror_client = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        exit(-1);
    }
    printf("Socket creation complete for server registration...\n");

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, serverIp, &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        exit(-1);
    }

    printf("Attempting server registration now...\n");
    if ((connect(fd_mirror_client, (struct sockaddr *) &serv_addr, sizeof(serv_addr))) < 0) {
        printf("\nConnection Failed \n");
        exit(-1);
    }

    printf("Initiating server registration...\n");
    send(fd_mirror_client, initMsg, strlen(initMsg), 0);
    bytesReceived = read(fd_mirror_client, buffer, 1024);
    close(fd_mirror_client);

    if (strcmp(buffer, "OK") == 0) {
        printf("Server registration completed, ready for handling client requests now...\n");
        return 0;
    }
    printf("Server responded with %ld bytes of msg: '%s'... Registration failed..\n", bytesReceived, buffer);
    return -1;
}

int main(int argc, char const *argv[]) {
    if (argc != 2) {
        printf("Cannot start mirror. Need server's IP address as the 1st parameter...\n");
        exit(0);
    }
    char *serverIp = argv[1];

    char *ip = getCurrentHostIp();
    printf("The current mirror host ip is: %s\n", ip);
    if (registerOntoServer(ip, serverIp) < 0) {
        exit(-1);
    }


    return 0;
}