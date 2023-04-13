#define _XOPEN_SOURCE
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>

#define SERVER_PORT 8080
#define NULL_CH '\0'

int SOCKET_WAITING_QUEUE_SIZE = 3;
int BUFFER_LENGTH = 1024;
int MAX_WORDS = 128;
int MAX_WORD_LENGTH = 256;

void send_file(int sockfd)
{
    char buffer[BUFFER_LENGTH];

    FILE *file = fopen("temp.tar.gz", "rb");
    if (file == NULL)
    {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    // Get the file size
    struct stat file_stat;
    if (fstat(fileno(file), &file_stat) < 0)
    {
        perror("Error getting file size");
        exit(EXIT_FAILURE);
    }

    // Send the file size to the client
    size_t file_size = file_stat.st_size;
    send(sockfd, &file_size, sizeof(file_size), 0);

    // Send the file
    while (!feof(file))
    {
        size_t bytes_read = fread(buffer, 1, BUFFER_LENGTH, file);
        send(sockfd, buffer, bytes_read, 0);
    }

    fclose(file);
}


int split_words(char input_string[], char words[MAX_WORDS][MAX_WORD_LENGTH])
{
    int word_count = 0;
    char *ptr = input_string;
    while (*ptr != '\0') 
    {
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

void trim(char *str) 
{
    char *start, *end;
    for (start = str; *start && isspace(*start); ++start);
    if (start != str) 
    {
        memmove(str, start, strlen(start) + 1);
    }
    for (end = str + strlen(str) - 1; end > str && isspace(*end); --end);
    *(end + 1) = '\0';
}

void cleanBuffer(char data[]) 
{
    for (int i = 0; i < BUFFER_LENGTH; i++) data[i] = NULL_CH;
}

void process_command(int socket,char input_string[])
{
char output_message[2056];
trim(input_string);
char words[MAX_WORDS][MAX_WORD_LENGTH];
int word_count = split_words(input_string, words);
if(strcmp(words[0],"findfile")==0)
{
	char command[1024]={0};
	//printf("\nSearch Filename %s\n",words[1]);
	system("rm -f output.txt");
	snprintf(command,sizeof(command) ,"find ~ -type f -name \"%s\" -printf \"File: %%p\\nSize: %%s bytes\\nCreation date: %%Tc\\n\" -quit > output.txt", words[1]);
	FILE *pipe = popen(command, "r");
	if (pipe == NULL) 
		{
			printf("\nError: Could Not Execute find Command\n");
			char error_message[] = "Some error occurred while processing the request";
			strcpy(output_message, error_message);
			cleanBuffer(output_message);
			send(socket, output_message, strlen(output_message), 0);
		}
	pclose(pipe);
	int fd = open("output.txt",O_RDONLY);
	char buffer[2056]={0};
	int bytes_read = read(fd,buffer,2056);
	close(fd);
	//system("rm -f output.txt");
	cleanBuffer(output_message);
	if(bytes_read == 0)
		strcpy(output_message,"File not found");
	else	
		strcpy(output_message, buffer);
	send(socket, output_message, strlen(output_message), 0);
}
else if(strcmp(words[0],"sgetfiles")==0)
{
	char command[1024]={0};
	printf("\nSize1 %s Size2 %s\n",words[1],words[2]);
	int size1 = atoi(words[1]);
	int size2 = atoi(words[2]);
	system("rm -f temp.tar.gz");
	snprintf(command, sizeof(command), "find ~ -type f -size +%dc -a -size -%dc -print0 | tar -czvf temp.tar.gz --null -T -", size1, size2);
	FILE *pipe = popen(command, "r");
	if (pipe == NULL) 
		{
			printf("\nError: Could Not Execute find Command\n");
			char error_message[] = "Some error occurred while processing the request";
			cleanBuffer(output_message);
			strcpy(output_message, error_message);
		}
	pclose(pipe);
	send_file(socket);
	//system("rm -f temp.tar.gz");
}
else if(strcmp(words[0],"dgetfiles")==0)
{
	char command[1024]={0};
	printf("\nDate1 %s Date2 %s\n",words[1],words[2]);
	system("rm -f temp.tar.gz");
	snprintf(command, sizeof(command), "find ~ -type f -newermt \"%s\" ! -newermt \"%s\" -print0 | tar -czvf temp.tar.gz --null -T -",words[1],words[2]);
	FILE *pipe = popen(command, "r");
	if (pipe == NULL) 
		{
			printf("\nError: Could Not Execute find Command\n");
			char error_message[] = "Some error occurred while processing the request";
			strcpy(output_message, error_message);
		}
	pclose(pipe);
	send_file(socket);
	//system("rm -f temp.tar.gz");
}
else if(strcmp(words[0],"getfiles")==0)
{
	system("rm -f output.txt");
	system("chmod +x ./find_files_by_name.sh");
	char command[1024]={0};
	strcat(command,"./find_files_by_name.sh ");
	for (int i = 1; i < word_count; i++) 
	{
		if(strcmp(words[i],"-u") != 0)
		{
			strcat(command, words[i]);
			strcat(command, " ");
		}
    	}
    	strcat(command, " > output.txt");
	printf("\nCommand: %s\n",command);
	system("rm -f temp.tar.gz");
	FILE *pipe = popen(command, "r");
	if (pipe == NULL) 
		{
			printf("\nError: Could Not Execute find Command\n");
			char error_message[] = "Some error occurred while processing the request";
			strcpy(output_message, error_message);
		}
	pclose(pipe);
	int fd = open("output.txt",O_RDONLY);
	char buffer[2056]={0};
	int bytes_read = read(fd,buffer,2056);
	close(fd);
	cleanBuffer(output_message);
	trim(buffer);
	if(strcmp(buffer,"temp.tar.gz created with the found files.") == 0)
		{
		strcpy(output_message, "File found");
		send(socket, output_message, strlen(output_message), 0);
		send_file(socket);
		}
	else
		{	
		strcpy(output_message, "No file found");
		send(socket, output_message, strlen(output_message), 0);
		}
	//system("rm -f temp.tar.gz");
	//system("rm -f output.txt");
}
else if(strcmp(words[0],"gettargz")==0)
{
	system("rm -f output.txt");
	system("chmod +x ./find_files_by_extension.sh");
	char command[1024]={0};
	strcat(command,"./find_files_by_extension.sh ");
	for (int i = 1; i < word_count; i++) 
	{
		if(strcmp(words[i],"-u") != 0)
		{
			strcat(command, words[i]);
			strcat(command, " ");
		}
    	}
	printf("\nCommand: %s\n",command);
	system("rm -f temp.tar.gz");
	FILE *pipe = popen(command, "r");
	if (pipe == NULL) 
		{
			printf("\nError: Could Not Execute find Command\n");
			char error_message[] = "Some error occurred while processing the request";
			strcpy(output_message, error_message);
		}
	pclose(pipe);
	struct stat buffer;
	int file_exists = stat("./temp.tar.gz", &buffer) == 0;
	cleanBuffer(output_message);
	if (file_exists)
		{
		strcpy(output_message, "File found");
		trim(output_message);
		send(socket, output_message, strlen(output_message), 0);
		send_file(socket);
		}
	else
		{	
		strcpy(output_message, "No file found");
		send(socket, output_message, strlen(output_message), 0);
		}
	//system("rm -f temp.tar.gz");
	//system("rm -f output.txt");
}
}
void processClient(int socket) 
{
    int pid = fork();
    if (pid==0) 
    {
    	printf("\nSERVER(%d): Connected with client\n",getpid());
    	while(1)
    	{
    		char input_string[2056]={0};
    		int bytes_received = read(socket, input_string, 2056);
    		trim(input_string);
    		if(strcmp(input_string,"quit")==0)
		{
			exit(0);
		}
    		process_command(socket,input_string);
	}
    }
    else if(pid > 0)
    {
    	wait(NULL); // to prevent child from becoming defunct process and downgrading system performance
    	printf("\nSERVER(%d): Shutting down. Closing connection. \n",pid);
    }
    else
    {
    	perror("fork failed");
        exit(EXIT_FAILURE);
    }
    close(socket);
}

int main(int argc, char const *argv[]) 
{
    int root = getpid();
    printf("root pid: %d\n", root);
    int server_fd, socketConn;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    { 
    	//AF_INET for IPv4, SOCK_STREAM -- TCP
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) 
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(SERVER_PORT);
    if (bind(server_fd, (struct sockaddr *) &address, sizeof(address)) < 0) 
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, SOCKET_WAITING_QUEUE_SIZE) < 0) 
    { 
    	//limiting to 3 requests in the queue
        perror("listen");
        exit(EXIT_FAILURE);
    }
    printf("Entering listening state now...\n");
    while (1) 
    {
        printf("Waiting for accepting..\n");
        if ((socketConn = accept(server_fd, (struct sockaddr *) &address, (socklen_t *) &addrlen)) < 0) 
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }
	printf("Accepted a request from the queue..\n");
    	processClient(socketConn);

    } 
    // closing the listening socketConn
    shutdown(server_fd, SHUT_RDWR);
    return 0;
}
