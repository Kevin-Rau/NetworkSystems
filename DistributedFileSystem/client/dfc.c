// Client Side File, code inspired from // http://www.binarytides.com/server-client-example-c-sockets-linux/
// as well as various Github repos and Operating Systems PA3 for reading files and threading requests out
//Taken code From PA1 as well. 
//Kevin Rau 
//Network Systems PA2

#include <stdio.h>
#include <sys/errno.h> 
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h> 

#define MAX_BUFFER 1024

int parse(const char *);
void readUserInput();
int socketConnection(int, const char *);
void list(char *);
int put(char *);
int get(char *);
void authUser(int, char *, char *);
int handleError(const char *format, ...);
void serverRespond(int sock);
int tryConnection();

//struct to hold our server information 
typedef struct server {
    char host[20];
    int port;
    int fd;
} server;
//config file information, location and what we are grabbing to set equal to
const char *FILE_DIR="./test";
char username[128];
char password[128];
server * servers; 
int serverNum;

//same parse function used in the first assignment. compliments of zhued repo. 
int parse(const char *filename)
{
    char *line;
    char * token;
    char head[64], tail[256], host[256];
    size_t len = 0;
    int read_len = 0;
    int i=0;
    FILE* conf_file = fopen(filename, "r");
    if (conf_file == NULL)
        fprintf(stderr, "Could not open config file.\n");
    servers = malloc(4 * sizeof(server));

    while((read_len = getline(&line, &len, conf_file)) != -1) {
        line[read_len+1] = '\0';
        if (line[0] == '#')
            continue;

        sscanf(line, "%s %s %s", head, tail, host);
        if (!strcmp(head, "Server")) {
            token = strtok(host, ":");
            strncpy(servers[i].host,token, 20);
            token = strtok(NULL, " ");
            servers[i].port= atoi(token);
            i++;
        } 
        if (!strcmp(head, "Username:")) {
            sscanf(tail, "%s", username);
        } 
        if (!strcmp(head, "Password:")) {
            sscanf(tail, "%s", password);
        }
    }

    fclose(conf_file);

    return i;
}
//error function to be called for cases where we error out
int handleError(const char *format, ...) {
        va_list args;

        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);
        exit(1);
}


//////////////////////////////Request Inputs///////////////////////////////////////////////
void readUserInput() {
    char *line = NULL;      /* Line read from STDIN */
    size_t len;
    ssize_t read;
    char command[8], arg[64];
    int status = 1;

    while (status) {
        printf("%s> ", username);
        read = getline(&line, &len, stdin);
        line[read-1] = '\0';

        sscanf(line, "%s %s", command, arg);

        if (!strncasecmp(command, "LIST", 4)) {
            // printf("the command you entered was: %s\n", command);
            // printf("sock num %d\n", sock);
            list(command);
        }
        else if (!strncasecmp(command, "GET", 3)) {
            if (strlen(line) <= 3)
                printf("Check your args.\n");
            else
                get(line);
        }
        else if (!strncasecmp(command, "PUT", 3)) {
            if (strlen(line) <= 3)
                printf("Check your args.\n");
            else
                put(line);
        }
        else{
            printf("Invalid command.\n");
        }
    }
    printf("Quiting...\n");
}

void serverRespond(int sock){
    char server_reply[MAX_BUFFER];
    if( recv(sock, server_reply, 2000, 0) < 0)
    {
        handleError("Error in recv: %s\n", strerror(errno));
    } else {
        puts(server_reply);
    }
}
//make sure user is vaild
void authUser(int sock, char * username, char * password)
{
    char *result = malloc(strlen(username)+strlen(password));
    char buf[MAX_BUFFER];
    int len;

    strcpy(result, "LOGIN:");
    strcat(result, username);
    strcat(result, " ");
    strcat(result, password);
    if (write(sock, result, strlen(result)) < 0){
        handleError("Authentication Error: %s\n", strerror(errno));
    }
    serverRespond(sock);
}
//want to give back the files that we have on the DFS
void list(char *command)
{

    int sock = tryConnection();
    if(write(sock, command, strlen(command)) < 0) {
        handleError("ERROR %s\n", strerror(errno));
    }
    serverRespond(sock);
}

int tryConnection()
{
    int sock;
    int i;

    for (i = 0; i < serverNum; ++i)
    {
        servers[i].fd = socketConnection(servers[i].port, servers[i].host);
        if(servers[i].fd != 1)
        {
            sock = servers[i].fd;
            return sock;
        }
    }
    return 0;
}

// http://stackoverflow.com/questions/2014033/send-and-receive-a-file-in-socket-programming-in-linux-with-c-c-gcc-g
int put(char *line)
{
    struct timespec tim;
    tim.tv_sec = 0;
    tim.tv_nsec = 100000000L; //timeout

    char file_loc[128];
    int fd;
    char file_size[256];
    int size;
    struct stat file_stat;
    char command[8], arg[64];
    char buffer[MAX_BUFFER];
    char server_reply[MAX_BUFFER];
    int sock = tryConnection();


    sscanf(line, "%s %s", command, arg);
    sprintf(file_loc, "%s/%s", FILE_DIR, arg);

    if(write(sock, line, strlen(line)) < 0) {
        handleError("Error in DFS List: %s\n", strerror(errno));
    }

    if ((fd = open(file_loc, O_RDONLY)) < 0)
        handleError("Failed opening file at: '%s' %s\n", file_loc, strerror(errno)); 

    if (fstat(fd, &file_stat) < 0)
        handleError("Error fstat file at: '%s' %s\n", file_loc, strerror(errno));

    size = file_stat.st_size;
    sprintf(file_size, "%d", size);
    
    if (write(sock, file_size, sizeof(file_size)) < 0)
        handleError("Echo write: %s\n", strerror(errno));


    while (1) {
        //reading into our buffer
        int bytes_read = read(fd, buffer, sizeof(buffer));
        if (bytes_read == 0) 
            break;
        //done reading 
        if (bytes_read < 0) {
            //errors
        }
        //writing case that there is a file
        void *p = buffer;
        while (bytes_read > 0) {
            int bytes_written = write(sock, p, bytes_read);
            if (bytes_written <= 0) {
                //errors
            }
            bytes_read -= bytes_written;
            p += bytes_written;
        }
    }
    return 0;  
}
//get request, want to grab the given file in the GET command, read and present to client
int get(char *line)
{
    char buf[MAX_BUFFER];
    int read_size    = 0;
    int len = 0;
    FILE *downloaded_file;
    char command[8], arg[64];
    char file_loc[128];
    int sock;
    int i;

    for (i = 0; i < serverNum; ++i)
    {
        servers[i].fd = socketConnection(servers[i].port, servers[i].host);
        if(servers[i].fd != 1)
        {
            sock = servers[i].fd;
            break; 
        }
    }

    sscanf(line, "%s %s", command, arg);
    sprintf(file_loc, "./%s", arg);

    if(write(sock, line, strlen(line)) < 0) {

        handleError("ERROR in List: %s\n", strerror(errno));
    }

    downloaded_file = fopen(file_loc, "w"); //writing
    if (downloaded_file == NULL) //no such file found, error out
    {
        printf("ERROR opening file!\n");
        exit(1);
    }


    while ((read_size = recv(sock, &buf[len], (MAX_BUFFER-len), 0)) > 0)
    { 
        char line[read_size];
        strncpy(line, &buf[len], sizeof(line));
        len += read_size;
        line[read_size] = '\0';

        printf("%s\n", line);
        fprintf(downloaded_file, "%s\n", line);
        fclose(downloaded_file);
        return 1;
    }

    return 0;
}

int socketConnection(int port, const char *hostname) // opening and connecting to the socket server
{
    int sock;
    struct sockaddr_in server;
     
    sock = socket(AF_INET , SOCK_STREAM , IPPROTO_TCP);
    if (sock == -1)
    {
        printf("Could not create socket");
    }

    server.sin_addr.s_addr = inet_addr(hostname);
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
 
    //Connect to remote server
    if (connect(sock, (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        return 1;
    }

    authUser(sock, username, password);

    return sock;
}

int main(int argc, char *argv[], char **envp)
{
    if(argv[1]) //have enough arguments, parse file and get ready to take in requests. 
    {
        serverNum = parse(argv[1]);
        printf("%d servers in given config file.\n", serverNum);
        readUserInput();
        return 1;
    }
    //User didnt not set a config file to use. 
    else
    {
        printf("Add confg file\n");
    }
}