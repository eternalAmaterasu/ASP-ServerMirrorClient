//
// Created by v2 on 11/04/23.
//
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char const *argv[]) {

    int server_fd, new_socket;
    long int bytesRead;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
    char *hello = "OK";

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) { //AF_INET for IPv4, SOCK_STREAM -- TCP
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) { //limiting to 3 requests in the queue
        perror("listen");
        exit(EXIT_FAILURE);
    }
    printf("Listen up... \n");
    if ((new_socket = accept(server_fd, (struct sockaddr *) &address, (socklen_t *) &addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    printf("Waiting for accepting..\n");
    int root = getpid();
    printf("root pid: %d\n", root);
    int cid = fork();
    if (cid == 0) {
        bytesRead = read(new_socket, buffer, 1024);
        printf("%d => %ld bytes read. String => '%s'\n", getpid(), bytesRead, buffer);
        send(new_socket, hello, strlen(hello), 0);
        printf("Hello message sent\n");

        // closing the connected socket
        close(new_socket);
        exit(0);
    } else {
        close(new_socket);
        printf("from the root process.. nothing else to do?\n");
        sleep(10);
        printf("sleep over..\n");
    }
    // closing the listening socket
    shutdown(server_fd, SHUT_RDWR);
    return 0;
}
