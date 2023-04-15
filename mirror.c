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
#include <signal.h>
#include <sys/stat.h>
#include <ctype.h>
#include <fcntl.h>

#define SERVER_PORT 8080
#define MIRROR_PORT 8081
#define NULL_CH '\0'

int SOCKET_WAITING_QUEUE_SIZE = 3;
int IP_LENGTH = 25;
int BUFFER_LENGTH = 1024;
int MAX_WORDS = 128;
int MAX_WORD_LENGTH = 256;

char *MIRROR_ACK = "OK";
char *serverIpInstance = NULL;
char *CLIENT_ACK = "Hi";

char *ffbn = "#!/bin/bash\n"
             "# Create an empty array to store found files\n"
             "found_files=()\n"
             "# Loop through the input files and check if they exist in the directory tree rooted at ~\n"
             "for file in \"$@\"; do\n"
             "  file_path=$(find ~ -type f -name \"${file}\" -print -quit)\n"
             "  if [ -n \"${file_path}\" ]; then\n"
             "    found_files+=(\"${file_path}\")\n"
             "  fi\n"
             "done\n"
             "# If found files array is not empty, create temp.tar.gz containing the found files\n"
             "if [ ${#found_files[@]} -gt 0 ]; then\n"
             "  tar czf temp.tar.gz \"${found_files[@]}\"\n"
             "  echo \"temp.tar.gz created with the found files.\"\n"
             "else\n"
             "  echo \"No file found\"\n"
             "fi\n"
             "";

char *ffbe = "#!/bin/bash\n"
             "\n"
             "# Collect files with the specified extensions\n"
             "matching_files=()\n"
             "for ext in \"$@\"\n"
             "do\n"
             "  while IFS= read -r -d '' file; do\n"
             "    matching_files+=(\"$file\")\n"
             "  done < <(find ~ -type f -name \"*.${ext}\" -print0)\n"
             "done\n"
             "\n"
             "# Create the archive only if there are matching files\n"
             "if [ ${#matching_files[@]} -gt 0 ]; then\n"
             "  # Create an empty tar archive\n"
             "  tar -czf temp.tar.gz --files-from /dev/null\n"
             "\n"
             "  # Add the matching files to the archive\n"
             "  printf '%s\\0' \"${matching_files[@]}\" | tar -czvf temp.tar.gz --null -T -\n"
             "fi\n"
             "";

/**
 * General function to generate the file on file system with name @param nameOfFile and data of file being @param data
 * @param data
 * @param nameOfFile
 */
void generateFile(char *data, char *nameOfFile) {
    int fd = open(nameOfFile, O_TRUNC | O_CREAT | O_RDWR, 0700);
    if (fd < 0) {
        printf("Unable to open fd on the file %s\n", nameOfFile);
        exit(-1);
    }
    for (int i = 0; i < strlen(data); i++) {
        if (write(fd, &data[i], 1) != 1) {
            printf("failure while generating file %s\n", nameOfFile);
            close(fd);
            exit(-1);
        }
    }
    close(fd);
}

/**
 * Extract and return the current host IP which is required for registration onto server by mirror
 * @return extracted IP
 */
char *getCurrentHostIp() {
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

/**
 * General purpose function to connect and get the fd for @param serverIp and @param port -- for mirror registration and deregistration
 * @param serverIp
 * @param port
 * @return socket's client fd
 */
int connectAndGetFd(char *serverIp, int port) {
    int fdClient;
    struct sockaddr_in serv_addr;

    if ((fdClient = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        exit(-1);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, serverIp, &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        exit(-1);
    }

    if ((connect(fdClient, (struct sockaddr *) &serv_addr, sizeof(serv_addr))) < 0) {
        printf("\nConnection Failed \n");
        exit(-1);
    }
    return fdClient;
}

/**
 * Fill the @param data[] with '\0' upto @param length
 * @param data
 * @param length
 */
void cleanBufferWithLength(char data[], int length) {
    for (int i = 0; i < length; i++) data[i] = NULL_CH;
}

/**
 * Fill the @param data[] with '\0' upto BUFFER_LENGTH
 * @param data
 */
void cleanBuffer(char data[]) {
    cleanBufferWithLength(data, BUFFER_LENGTH);
}

/**
 * Register the mirror which has IP: @param hostIp onto the server which has IP: @param serverIp
 * @param hostIp
 * @param serverIp
 * @return 0 if server registration was successful, -1 if not
 */
int registerOntoServer(char *hostIp, char *serverIp) {
    int ipLength = (int) strlen(hostIp);
    char initMsg[31] = {NULL_CH};
    strncpy(initMsg, "Mir=", 4);
    int i = 0;
    for (; i < ipLength; i++) initMsg[i + 4] = hostIp[i];
    initMsg[i + 4] = ';';
    printf("\nRegistration data to send to server => '%s', length: %lu\n", initMsg, strlen(initMsg));

    int fd_mirror_client = connectAndGetFd(serverIp, SERVER_PORT);
    long int bytesReceived;
    char *buffer = malloc(BUFFER_LENGTH * sizeof(char));
    cleanBuffer(buffer);

    printf("Initiating server connection / registration...\n");
    send(fd_mirror_client, initMsg, strlen(initMsg), 0);
    bytesReceived = read(fd_mirror_client, buffer, BUFFER_LENGTH);
    close(fd_mirror_client);

    if (strcmp(buffer, MIRROR_ACK) == 0) {
        printf("Server registration completed, ready for handling client requests now...\n\n");
        return 0;
    }
    printf("Server responded with %ld bytes of msg: '%s'... Registration failed.\n\n", bytesReceived, buffer);
    return -1;
}

/**
 * Deregister the mirror from the server as the mirror process was interrupted by SIGINT
 */
void deregisterFromServer() {
    char lastMsg[20] = {NULL_CH};
    strncpy(lastMsg, "Mir=", 4);
    int i = 0;
    for (; i < 20 - 4 - 1; i++) lastMsg[i + 4] = '-';
    lastMsg[i + 4] = ';';
    printf("\nDeregistration data to send to server => '%s'\n", lastMsg);

    int fd_mirror_client = connectAndGetFd(serverIpInstance, SERVER_PORT);
    printf("Initiating mirror deregistration...\n");
    send(fd_mirror_client, lastMsg, strlen(lastMsg), 0);
    printf("Mirror deregistered...\n\n");
    close(fd_mirror_client);
}

/**
 * Send temp.tar.gz to the client via @param sockfd
 * @param sockfd
 */
void send_file(int sockfd) {
    char buffer[BUFFER_LENGTH];

    FILE *file = fopen("temp.tar.gz", "rb");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    // Get the file size
    struct stat file_stat;
    if (fstat(fileno(file), &file_stat) < 0) {
        perror("Error getting file size");
        exit(EXIT_FAILURE);
    }

    // Send the file size to the client
    size_t file_size = file_stat.st_size;
    send(sockfd, &file_size, sizeof(file_size), 0);

    // Send the file
    while (!feof(file)) {
        size_t bytes_read = fread(buffer, 1, BUFFER_LENGTH, file);
        send(sockfd, buffer, bytes_read, 0);
    }

    fclose(file);
}

/**
 *
 * @param input_string
 * @param words
 * @return
 */
int split_words(char input_string[], char words[MAX_WORDS][MAX_WORD_LENGTH]) {
    int word_count = 0;
    char *ptr = input_string;
    while (*ptr != '\0') {
        while (*ptr == ' ') ptr++;
        if (*ptr == '\0') break;
        char *word_start = ptr;
        while (*ptr != ' ' && *ptr != '\0') ptr++;
        strncpy(words[word_count], word_start, ptr - word_start);
        words[word_count][ptr - word_start] = '\0';
        word_count++;
        if (*ptr == ' ') ptr++;
    }
    return word_count;
}

/**
 *
 * @param str
 */
void trim(char *str) {
    char *start, *end;
    for (start = str; *start && isspace(*start); ++start);
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
    for (end = str + strlen(str) - 1; end > str && isspace(*end); --end);
    *(end + 1) = '\0';
}

/**
 *
 * @param socket
 * @param input_string
 */
void process_command(int socket, char input_string[]) {
    char output_message[2056];
    trim(input_string);
    printf("\nProcessing command => '%s'\n", input_string);
    char words[MAX_WORDS][MAX_WORD_LENGTH];
    int word_count = split_words(input_string, words);
    if (strcmp(words[0], "findfile") == 0) {
        char command[1024] = {0};
        //printf("\nSearch Filename %s\n",words[1]);
        system("rm -f output.txt");
        snprintf(command, sizeof(command), "find ~ -type f -name \"%s\" -printf \"File: %%p\\nSize: %%s bytes\\nCreation date: %%Tc\\n\" -quit > output.txt", words[1]);
        FILE *pipe = popen(command, "r");
        if (pipe == NULL) {
            printf("\nError: Could Not Execute find Command\n");
            char error_message[] = "Some error occurred while processing the request";
            strcpy(output_message, error_message);
            cleanBuffer(output_message);
            send(socket, output_message, strlen(output_message), 0);
        }
        pclose(pipe);
        int fd = open("output.txt", O_RDONLY);
        char buffer[2056] = {0};
        int bytes_read = read(fd, buffer, 2056);
        close(fd);
        //system("rm -f output.txt");
        cleanBuffer(output_message);
        if (bytes_read == 0)
            strcpy(output_message, "File not found");
        else
            strcpy(output_message, buffer);
        send(socket, output_message, strlen(output_message), 0);
    } else if (strcmp(words[0], "sgetfiles") == 0) {
        char command[1024] = {0};
        printf("\nSize1 %s Size2 %s\n", words[1], words[2]);
        int size1 = atoi(words[1]);
        int size2 = atoi(words[2]);
        system("rm -f temp.tar.gz");
        snprintf(command, sizeof(command), "find ~ -type f -size +%dc -a -size -%dc -print0 | tar -czvf temp.tar.gz --null -T -", size1, size2);
        FILE *pipe = popen(command, "r");
        if (pipe == NULL) {
            printf("\nError: Could Not Execute find Command\n");
            char error_message[] = "Some error occurred while processing the request";
            cleanBuffer(output_message);
            strcpy(output_message, error_message);
        }
        pclose(pipe);
        send_file(socket);
        //system("rm -f temp.tar.gz");
    } else if (strcmp(words[0], "dgetfiles") == 0) {
        char command[1024] = {0};
        printf("\nDate1 %s Date2 %s\n", words[1], words[2]);
        system("rm -f temp.tar.gz");
        snprintf(command, sizeof(command), "find ~ -type f -newermt \"%s\" ! -newermt \"%s\" -print0 | tar -czvf temp.tar.gz --null -T -", words[1], words[2]);
        FILE *pipe = popen(command, "r");
        if (pipe == NULL) {
            printf("\nError: Could Not Execute find Command\n");
            char error_message[] = "Some error occurred while processing the request";
            strcpy(output_message, error_message);
        }
        pclose(pipe);
        send_file(socket);
        //system("rm -f temp.tar.gz");
    } else if (strcmp(words[0], "getfiles") == 0) {
        system("rm -f output.txt");
        system("chmod +x ./find_files_by_name.sh");
        char command[1024] = {0};
        strcat(command, "./find_files_by_name.sh ");
        for (int i = 1; i < word_count; i++) {
            if (strcmp(words[i], "-u") != 0) {
                strcat(command, words[i]);
                strcat(command, " ");
            }
        }
        strcat(command, " > output.txt");
        printf("\nCommand: %s\n", command);
        system("rm -f temp.tar.gz");
        FILE *pipe = popen(command, "r");
        if (pipe == NULL) {
            printf("\nError: Could Not Execute find Command\n");
            char error_message[] = "Some error occurred while processing the request";
            strcpy(output_message, error_message);
        }
        pclose(pipe);
        int fd = open("output.txt", O_RDONLY);
        char buffer[2056] = {0};
        int bytes_read = read(fd, buffer, 2056);
        close(fd);
        cleanBuffer(output_message);
        trim(buffer);
        if (strcmp(buffer, "temp.tar.gz created with the found files.") == 0) {
            strcpy(output_message, "File found");
            send(socket, output_message, strlen(output_message), 0);
            send_file(socket);
        } else {
            strcpy(output_message, "No file found");
            send(socket, output_message, strlen(output_message), 0);
        }
        //system("rm -f temp.tar.gz");
        //system("rm -f output.txt");
    } else if (strcmp(words[0], "gettargz") == 0) {
        system("rm -f output.txt");
        system("chmod +x ./find_files_by_extension.sh");
        char command[1024] = {0};
        strcat(command, "./find_files_by_extension.sh ");
        for (int i = 1; i < word_count; i++) {
            if (strcmp(words[i], "-u") != 0) {
                strcat(command, words[i]);
                strcat(command, " ");
            }
        }
        printf("\nCommand: %s\n", command);
        system("rm -f temp.tar.gz");
        FILE *pipe = popen(command, "r");
        if (pipe == NULL) {
            printf("\nError: Could Not Execute find Command\n");
            char error_message[] = "Some error occurred while processing the request";
            strcpy(output_message, error_message);
        }
        pclose(pipe);
        struct stat buffer;
        int file_exists = stat("./temp.tar.gz", &buffer) == 0;
        cleanBuffer(output_message);
        if (file_exists) {
            strcpy(output_message, "File found");
            trim(output_message);
            send(socket, output_message, strlen(output_message), 0);
            send_file(socket);
        } else {
            strcpy(output_message, "No file found");
            send(socket, output_message, strlen(output_message), 0);
        }
        //system("rm -f temp.tar.gz");
        //system("rm -f output.txt");
    }
    printf("Processed command => '%s'\n", input_string);
}

/**
 * Handle client connection requests in a dedicated child process using @param socket
 * @param socket
 */
void processClient(int socket) {
    int pid = fork();
    if (pid == 0) {
        printf("\nConnected with client\n");
        //printf("%d => %ld bytes read. String => '%s'\n", getpid(), bytesRead, buffer);
        send(socket, CLIENT_ACK, strlen(CLIENT_ACK), 0);
        while (1) {
            char input_string[2056] = {0};
            long int bytes_received = read(socket, input_string, 2056);
            trim(input_string);
            if (strcmp(input_string, "quit") == 0) {
                close(socket); // alert - added here
                printf("\nShutting down. Closing connection. \n");
                exit(0);
            }
            process_command(socket, input_string);
        }
    } else if (pid < 0) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }

    // closing the connected socket
    close(socket);
}

/**
 * SIGINT handler
 */
void handleInterrupt() {
    deregisterFromServer();
    exit(0);
}

int main(int argc, char const *argv[]) {
    generateFile(ffbn, "find_files_by_name.sh");
    generateFile(ffbe, "find_files_by_extension.sh");

    if (argc != 2) {
        printf("Cannot start mirror. Need server's IP address as the 1st parameter...\n");
        exit(0);
    }

    printf("**** WELCOME TO MIRROR OF COMP-8567 PROJECT ****\n");
    printf("Mirror started with process id: %d\n", getpid());
    char *serverIp = argv[1];
    char *ip = getCurrentHostIp();
    printf("The current mirror host ip is: '%s'\n", ip);
    if (registerOntoServer(ip, serverIp) < 0) {
        exit(-1);
    }
    serverIpInstance = malloc(strlen(serverIp) * sizeof(char));
    strcpy(serverIpInstance, serverIp);
    signal(SIGINT, handleInterrupt); //register SIGINT handler

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
    address.sin_port = htons(MIRROR_PORT);

    if (bind(server_fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, SOCKET_WAITING_QUEUE_SIZE) < 0) { //limiting to 3 requests in the queue
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Entering listening state now...\n\n");
    while (1) {
        if ((socketConn = accept(server_fd, (struct sockaddr *) &address, (socklen_t *) &addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        char *buffer = malloc(BUFFER_LENGTH * sizeof(char));
        cleanBuffer(buffer);

        bytesRead = read(socketConn, buffer, BUFFER_LENGTH);
        processClient(socketConn);
    }
    return 0;
}