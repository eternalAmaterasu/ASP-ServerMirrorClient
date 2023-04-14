#define _XOPEN_SOURCE

#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

#define SERVER_PORT 8080
#define MIRROR_PORT 8081
#define NULL_CH '\0'

int IP_LENGTH = 25;
int BUFFER_LENGTH = 1024;
char *mirrorRegistrationStartingMessage = "Mir=";
char *CLIENT_ACK = "Hi";
char *REJECT_CLIENT = "No";

void receive_file(int sockfd) {
    char buffer[BUFFER_LENGTH];

    // Receive the file size from the server
    size_t file_size;
    recv(sockfd, &file_size, sizeof(file_size), 0);
    system("rm -f temp_client.tar.gz");
    FILE *file = fopen("temp_client.tar.gz", "wb");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    ssize_t bytes_received;
    size_t total_received = 0;
    while (total_received < file_size && (bytes_received = recv(sockfd, buffer, BUFFER_LENGTH, 0)) > 0) {
        fwrite(buffer, 1, bytes_received, file);
        total_received += bytes_received;
    }

    fclose(file);
}


bool is_valid_filename(const char *filename) {
    return strlen(filename) > 0;
}

bool is_valid_extension(const char *extension) {
    return strlen(extension) > 0;
}

bool parse_date(const char *date_str, struct tm *date) {
    char *result = strptime(date_str, "%Y-%m-%d", date);
    return result != NULL && *result == '\0';
}

bool is_valid_number(const char *number) {
    for (int i = 0; number[i]; ++i) {
        if (!isdigit(number[i])) {
            return false;
        }
    }
    return true;
}

void trim(char *str) {
    char *start, *end;
    for (start = str; *start && isspace(*start); ++start);
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
    for (end = str + strlen(str) - 1; end > str && isspace(*end); --end);
    *(end + 1) = '\0';
}

int check_findfile(char input_string[]) // returns 1 if invalid syntax, returns 0 if syntax is valid
{
    char command[1024];
    char filename[1024];
    int pos = 0;
    int token_start = 0;
    for (int i = 0; i <= strlen(input_string); ++i) {
        if (input_string[i] == ' ' || input_string[i] == '\0') {
            if (pos == 0) {
                strncpy(command, input_string + token_start, i - token_start);
                command[i - token_start] = '\0';
                if (strcmp(command, "findfile") != 0) {
                    printf("\nInvalid command.\n");
                    return 1;
                }
            } else if (pos == 1) {
                strncpy(filename, input_string + token_start, i - token_start);
                filename[i - token_start] = '\0';
                if (!is_valid_filename(filename)) {
                    printf("\nInvalid filename.\n");
                    return 1;
                }
            } else {
                printf("\nInvalid syntax.\n");
                return 1;
            }
            token_start = i + 1;
            pos++;
        }
    }
    if (pos != 2) {
        printf("\nInvalid syntax.\n");
        return 1;
    }
    return 0;
}

int check_sgetfiles(char input_string[]) // returns 1 if invalid syntax, returns 0 if syntax is valid and there's -u option and returns 2 if syntax is valid and there's no -u option
{
    char command[1024];
    char size1[1024], size2[1024];
    bool has_u_flag = false;
    int num_tokens = 0;
    int pos = 0;
    int token_start = 0;
    for (int i = 0; i <= strlen(input_string); ++i) {
        if (input_string[i] == ' ' || input_string[i] == '\0') {
            if (pos == 0) {
                strncpy(command, input_string + token_start, i - token_start);
                command[i - token_start] = '\0';
                if (strcmp(command, "sgetfiles") != 0) {
                    printf("\nInvalid command.\n");
                    return 1;
                }
            } else if (pos == 1) {
                strncpy(size1, input_string + token_start, i - token_start);
                size1[i - token_start] = '\0';
                if (!is_valid_number(size1)) {
                    printf("\nInvalid size1.\n");
                    return 1;
                }
            } else if (pos == 2) {
                strncpy(size2, input_string + token_start, i - token_start);
                size2[i - token_start] = '\0';
                if (!is_valid_number(size2)) {
                    printf("\nInvalid size2.\n");
                    return 1;
                }
            } else if (pos == 3) {
                if (strcmp(input_string + token_start, "-u") == 0) {
                    has_u_flag = true;
                } else {
                    printf("\nInvalid flag.\n");
                    return 1;
                }
            } else {
                printf("\nToo many arguments.\n");
                return 1;
            }
            token_start = i + 1;
            pos++;
        }
    }
    long size1_value = atol(size1);
    long size2_value = atol(size2);
    if (size1_value > size2_value || size1_value < 0 || size2_value < 0) {
        printf("\nInvalid size values.\n");
        return 1;
    }
    if (!has_u_flag) {
        return 2;
    }
    return 0;
}

int check_dgetfiles(char input_string[]) // returns 1 if invalid syntax, returns 0 if syntax is valid and there's -u option and returns 2 if syntax is valid and there's no -u option
{
    char command[1024];
    char date1_str[11];
    char date2_str[11];
    char optional_flag[4];
    if (sscanf(input_string, "%s %10s %10s %3s", command, date1_str, date2_str, optional_flag) >= 3) {
        if (strcmp(command, "dgetfiles") == 0) {
            struct tm date1 = {0};
            struct tm date2 = {0};
            if (parse_date(date1_str, &date1) && parse_date(date2_str, &date2)) {
                time_t date1_time = mktime(&date1);
                time_t date2_time = mktime(&date2);
                if (date1_time <= date2_time) {
                    if (strcmp(optional_flag, "-u") != 0) {
                        return 2;
                    } else if (strcmp(optional_flag, "-u") == 0) {
                        return 0;
                    } else if (optional_flag[0] != '\0') {
                        printf("Invalid optional flag.\n");
                        return 1;
                    }
                } else {
                    printf("Invalid date range: date1 should be less than or equal to date2.\n");
                    return 1;
                }
            } else {
                printf("Invalid date format. Use YYYY-MM-DD.\n");
                return 1;
            }
        } else {
            printf("Invalid command.\n");
            return 1;
        }
    } else {
        printf("Invalid syntax.\n");
        return 1;
    }
    return 0;
}


int check_getfiles(char input_string[]) // returns 1 if invalid syntax, returns 0 if syntax is valid and there's -u option and returns 2 if syntax is valid and there's no -u option
{
    char command[1024];
    char filenames[6][1024];
    bool has_u_flag = false;
    int num_filenames = 0;
    int pos = 0;
    int token_start = 0;
    for (int i = 0; i <= strlen(input_string); ++i) {
        if (input_string[i] == ' ' || input_string[i] == '\0') {
            if (pos == 0) {
                strncpy(command, input_string + token_start, i - token_start);
                command[i - token_start] = '\0';
                if (strcmp(command, "getfiles") != 0) {
                    printf("\nInvalid command.\n");
                    return 1;
                }
            } else {

                strncpy(filenames[num_filenames], input_string + token_start, i - token_start);
                filenames[num_filenames][i - token_start] = '\0';
                if (strcmp(filenames[num_filenames], "-u") == 0) {
                    has_u_flag = true;
                } else if (is_valid_filename(filenames[num_filenames])) {
                    num_filenames++;
                    if (num_filenames > 6 && has_u_flag == false) {
                        printf("\nToo many arguments.\n");
                        return 1;
                    }
                } else {
                    printf("\nInvalid filename.\n");
                    return 1;
                }
            }
            token_start = i + 1;
            pos++;
        }
    }
    if (num_filenames < 1) {
        printf("\nInvalid syntax.\n");
        return 1;
    }
    if (!has_u_flag) {
        return 2;
    }
    return 0;
}

int check_gettargz(char input_string[]) // returns 1 if invalid syntax, returns 0 if syntax is valid and there's -u option and returns 2 if syntax is valid and there's no -u option
{
    char command[1024];
    char extensions[6][1024];
    bool has_u_flag = false;
    int num_extensions = 0;
    int pos = 0;
    int token_start = 0;
    for (int i = 0; i <= strlen(input_string); ++i) {
        if (input_string[i] == ' ' || input_string[i] == '\0') {
            if (pos == 0) {
                strncpy(command, input_string + token_start, i - token_start);
                command[i - token_start] = '\0';
                if (strcmp(command, "gettargz") != 0) {
                    printf("\nInvalid command.\n");
                    return 1;
                }
            } else {
                strncpy(extensions[num_extensions], input_string + token_start, i - token_start);
                extensions[num_extensions][i - token_start] = '\0';
                if (strcmp(extensions[num_extensions], "-u") == 0) {
                    has_u_flag = true;
                } else if (is_valid_extension(extensions[num_extensions])) {
                    num_extensions++;
                    if (num_extensions > 6 && has_u_flag == false) {
                        printf("\nToo many arguments.\n");
                        return 1;
                    }
                } else {
                    printf("\nInvalid extension.\n");
                    return 1;
                }
            }
            token_start = i + 1;
            pos++;
        }
    }
    if (num_extensions < 1) {
        printf("\nInvalid syntax.\n");
        return 1;
    }
    if (!has_u_flag) {
        return 2;
    }
    return 0;
}

char *check_command(char input_string[], char *command) // returns command name if it's valid else prints appropriate error message and returns NULL
{
    command = malloc(sizeof(char) * 1024);
    int i = 0;
    while (input_string[i] != ' ' && input_string[i] != '\0') {
        command[i] = input_string[i];
        i++;
    }
    command[i] = '\0';
    if (strcmp(command, "findfile") == 0) {
        if (check_findfile(input_string) != 0) {
            return NULL;
        }
    } else if (strcmp(command, "sgetfiles") == 0) {
        int ret_val = check_sgetfiles(input_string);
        if (ret_val == 1) {
            return NULL;
        } else if (ret_val == 0) {
            command[i] = 'u';
            command[i + 1] = '\0';
        }
    } else if (strcmp(command, "dgetfiles") == 0) {
        int ret_val = check_dgetfiles(input_string);
        if (ret_val == 1) {
            return NULL;
        } else if (ret_val == 0) {
            command[i] = 'u';
            command[i + 1] = '\0';
        }

    } else if (strcmp(command, "getfiles") == 0) {
        int ret_val = check_getfiles(input_string);
        if (ret_val == 1) {
            return NULL;
        } else if (ret_val == 0) {
            command[i] = 'u';
            command[i + 1] = '\0';
        }

    } else if (strcmp(command, "gettargz") == 0) {
        int ret_val = check_gettargz(input_string);
        if (ret_val == 1) {
            return NULL;
        } else if (ret_val == 0) {
            command[i] = 'u';
            command[i + 1] = '\0';
        }

    } else if (strcmp(command, "quit") == 0) {
        if (strcmp(input_string, "quit") != 0) {
            printf("\nInvalid Syntax\n");
            return NULL;
        }
    } else {
        printf("\nInvalid command\n");
        return NULL;
    }
    return (command);
}

int connectAndGetFd(char *serverIp, int port) {
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
        printf("Cannot start client. Need server's IP address as the 1st parameter...\n");
        exit(0);
    }
    char *serverIp = argv[1];
    //int *fdClient = NULL;
    int fdClient;
    char *mirrorIp = malloc(IP_LENGTH * sizeof(char));
    fdClient = connectAndGetFd(serverIp, SERVER_PORT);

    char *buffer = malloc(BUFFER_LENGTH * sizeof(char));
    cleanBuffer(buffer);
    long int bytesReceived;

    printf("Initiating server contact, with fdclient as %d...\n", fdClient);
    send(fdClient, CLIENT_ACK, strlen(CLIENT_ACK), 0);
    bytesReceived = read(fdClient, buffer, 1024);
    printf("client received %ld bytes with data being '%s'..\n", bytesReceived, buffer);

    if (bytesReceived > 10 && bytesReceived < 25 && strncmp(mirrorRegistrationStartingMessage, buffer, 4) == 0) {
        close(fdClient);

        for (int i = 0; i < IP_LENGTH && buffer[i + 4] != ';'; i++)
            mirrorIp[i] = buffer[i + 4];

        printf("Rerouting to connect to the mirror...\n");
        fdClient = connectAndGetFd(mirrorIp, MIRROR_PORT);

        send(fdClient, CLIENT_ACK, strlen(CLIENT_ACK), 0);
        cleanBuffer(buffer);
        bytesReceived = read(fdClient, buffer, 1024);
    } else if (strcmp(REJECT_CLIENT, buffer) == 0) {
        printf("Client cannot start operation as server is not ready yet to accept connections, due to lack of mirror...\n");
        exit(0);
    }

    char input_string[1024];
    while (1) {
        cleanBuffer(input_string);
        printf("\nCLIENT: Enter the command\n");
        fgets(input_string, sizeof(input_string), stdin);
        trim(input_string);
        char *command;
        command = check_command(input_string, command);
        //printf("\nCheck Command ----> %s\n",command);
        if (command != NULL) {
            if (strcmp(command, "findfile") == 0) {
                send(fdClient, input_string, strlen(input_string), 0);
                char buffer[2056] = {0};
                int bytesReceived = read(fdClient, buffer, 2056);
                printf("\nCLIENT: %s\n", buffer);
            } else if (strcmp(command, "sgetfiles") == 0 || strcmp(command, "sgetfilesu") == 0) {
                send(fdClient, input_string, strlen(input_string), 0);
                receive_file(fdClient);
                if (strcmp(command, "sgetfilesu") == 0) {
                    char unzip_command[1024] = {0};
                    printf("\nExtracting temp_client.tar.gz file\n");
                    snprintf(unzip_command, sizeof(unzip_command), "tar -xzf temp_client.tar.gz");
                    FILE *pipe = popen(unzip_command, "r");
                    if (pipe == NULL) {
                        printf("\nError: Could Not Unzip Files\n");
                    }
                    pclose(pipe);
                }

            } else if (strcmp(command, "dgetfiles") == 0 || strcmp(command, "dgetfilesu") == 0) {
                send(fdClient, input_string, strlen(input_string), 0);
                receive_file(fdClient);
                if (strcmp(command, "dgetfilesu") == 0) {
                    char unzip_command[1024] = {0};
                    printf("\nExtracting temp_client.tar.gz file\n");
                    snprintf(unzip_command, sizeof(unzip_command), "tar -xzf temp_client.tar.gz");
                    FILE *pipe = popen(unzip_command, "r");
                    if (pipe == NULL) {
                        printf("\nError: Could Not Unzip Files\n");
                    }
                    pclose(pipe);
                }
            } else if (strcmp(command, "getfiles") == 0 || strcmp(command, "getfilesu") == 0) {
                send(fdClient, input_string, strlen(input_string), 0);
                char buffer[2056] = {0};
                int bytesReceived = read(fdClient, buffer, 2056);
                trim(buffer);
                if (strcmp(buffer, "File found") == 0) {
                    receive_file(fdClient);
                    if (strcmp(command, "getfilesu") == 0) {
                        char unzip_command[1024] = {0};
                        printf("\nExtracting temp_client.tar.gz file\n");
                        snprintf(unzip_command, sizeof(unzip_command), "tar -xzf temp_client.tar.gz");
                        FILE *pipe = popen(unzip_command, "r");
                        if (pipe == NULL) {
                            printf("\nError: Could Not Unzip Files\n");
                        }
                        pclose(pipe);
                    }
                } else {
                    printf("\nCLIENT: %s\n", buffer);
                }

            } else if (strcmp(command, "gettargz") == 0 || strcmp(command, "gettargzu") == 0) {
                send(fdClient, input_string, strlen(input_string), 0);
                char buffer[2056] = {0};
                int bytesReceived = read(fdClient, buffer, 2056);
                trim(buffer);
                if (strcmp(buffer, "File found") == 0) {
                    receive_file(fdClient);
                    if (strcmp(command, "gettargzu") == 0) {
                        char unzip_command[1024] = {0};
                        printf("\nExtracting temp_client.tar.gz file\n");
                        snprintf(unzip_command, sizeof(unzip_command), "tar -xzf temp_client.tar.gz");
                        FILE *pipe = popen(unzip_command, "r");
                        if (pipe == NULL) {
                            printf("\nError: Could Not Unzip Files\n");
                        }
                        pclose(pipe);
                    }
                } else {
                    printf("\nCLIENT: %s\n", buffer);
                }
            } else if (strcmp(command, "quit") == 0) {
                send(fdClient, input_string, strlen(input_string), 0);
                break;
            }
        } else {
            printf("\nEnter Correct Command\n");
        }

    }
    // closing the connected socket
    close(fdClient);
    return 0;
}