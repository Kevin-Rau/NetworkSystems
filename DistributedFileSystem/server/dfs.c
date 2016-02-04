// Server Side File, code inspired from // http://www.binarytides.com/server-client-example-c-sockets-linux/
// as well as various Github repos and Operating Systems PA3 for reading files and threading requests out
//Taken code From PA1 as well. 
//Kevin Rau 
//Network Systems PA2

#include <stdio.h>
#include <sys/socket.h>
#include <sys/errno.h> 
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdarg.h>
#include <fcntl.h> 
#include <unistd.h> 
//#include <openssl/md5.h> 

 
#define MAX_BUFFER 1024
//store some information from our conf file
typedef struct user{
    char name[128];
    char password[128];
} user;

user *users;
user currUser;
char server_directory[256] = ".";
int server_port;
int userNum = 0;


void parse(const char *);
void processRequest(int);
void listServer(int, char * username);
void putServer(int sock, char *);
int countLines(const char *);
int listenPort(int);
int userAuthServer(int, char *, char *);
void checkFileCurrServ(int, char * filename);
int requestFileCheck(char * filename);
int handleError(const char *format, ...);
void processRequest(int socket);
int readline(int fd, char * buf, int maxlen);
void getServer(int sock, char * filename);


void parse(const char *filename)
{
    char * line = NULL;
    char * token;
    size_t len = 0;
    int read_len = 0;
    int i=0;

    // open config file for reading
    FILE *conf_file = fopen(filename, "r");
    if (conf_file == NULL)
        fprintf(stderr, "Unable to open given file");

    // get number of users
    userNum = countLines(filename);

    users = malloc(userNum * sizeof(user));

    // for each line in config, add new user struct
    while((read_len = getline(&line, &len, conf_file)) != -1) {
        token = strtok(line, " ");
        strcpy(users[i].name, token);

        token = strtok(NULL, " ");
        strcpy(users[i].password,token);
        i++;
    }
    fclose(conf_file);
}

int countLines(const char *filename)
{
    FILE *fp = fopen(filename,"r");
    int ch=0;
    int lines=1;

    while(!feof(fp))
    {
        ch = fgetc(fp);
        if(ch == '\n')
        {
        lines++;
        }
    }
    return lines;
}

int handleError(const char *format, ...) {
        va_list args;

        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);
        exit(1);
}

void processRequest(int socket)
{
    char buf[MAX_BUFFER];
    char arg[64];
    char *token;
    char username[32], passwd[32];
    int read_size    = 0;
    int len = 0;
    char command[256];

    while ((read_size = recv(socket, &buf[len], (MAX_BUFFER-len), 0)) > 0)
    { 
        char line[read_size];
        strncpy(line, &buf[len], sizeof(line));
        len += read_size;
        line[read_size] = '\0';

        printf("Found:  %s\n", line);

        // printf("Line: %s\n", command);
        if (strncmp(line, "LOGIN:", 6) == 0)
        {
            token = strtok(line, ": ");
            token = strtok(NULL, " ");
            if (token == NULL){
                write(socket, "No such command.\n", 42);
                close(socket);
                return;
            }
            strcpy(username, token);
            if (token == NULL){
                write(socket, "No such command.\n", 42);
                close(socket);
                return;
            }
            token = strtok(NULL, " ");
            strcpy(passwd, token);

            if (userAuthServer(socket, username, passwd) == 0){
                char *message = "Invalid Username/Password.\n";
                write(socket, message, strlen(message));
                close(socket);
                return;
            }
            char * auth = "User Authenticated.\n";
            write(socket, auth, strlen(auth));
        }

        sscanf(line, "%s %s", command, arg);

        if (strncmp(command, "GET", 3) == 0) {
            printf("GET Request\n");
            getServer(socket, arg);
        } else if(strncmp(command, "LIST", 4) == 0) {
            printf("LIST Request:\n");
            // send(socket, command , strlen(command), 0);
            listServer(socket, username);
        } else if(strncmp(command, "PUT", 3) == 0){
            printf("PUT Request\n");
            putServer(socket, arg);
        } else if(strncmp(command, "LOGIN", 5) == 0) {
            
        } else {
            printf("No such command: %s\n", command);
        }
    }
    if(read_size == 0)
    {
        puts("Client has been disconnected");
        fflush(stdout);
    }
    else if(read_size == -1)
    {
        perror("recv failed");
    }
     
    return;
}

int userAuthServer(int socket, char * username, char * password) 
{
    int i;
    char directory[4096];

    strcpy(directory, server_directory);
    strcat(directory, username);
    printf("Username: %s\n", username);
    printf("Password: %s\n", password);
    for (i = 0; i < userNum; i++){
        if (strncmp(users[i].name, username, strlen(username)) == 0){
            if(strncmp(users[i].password, password, strlen(password)) == 0)
            {
                currUser = users[i];
                if(!opendir(directory)) {
                    write(socket, "No such directory! Creating new directory...\n", 35);
                    mkdir(directory, 0770);
                }
                printf("Given User Authenticated.\n");
                return 1;
            }
        }
    }
    return 0;
}


// Connect to a certain socket
int connectSocket(int port, const char *hostname)
{
    int sock;
    struct sockaddr_in server;
     
    //Create socket
    sock = socket(AF_INET , SOCK_STREAM , IPPROTO_TCP);
    if (sock == -1)
    {
        printf("Unable to create socket!");
    }
     printf("port number: %d\n", port);
    server.sin_addr.s_addr = inet_addr(hostname);
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
 
    //Connecting to the remote server
    if (connect(sock, (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("ERROR: Connection Failed!");
        return 1;
    }

    printf("Socket %d connected onto port %d\n", sock, ntohs(server.sin_port));

    return sock;
}

int listenPort(int port) 
{
    int socket_desc , client_sock, c, *new_sock;
    struct sockaddr_in server , client;
     
    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Failed Creating Socket!");
    }
    puts("Socket created!");
     
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( port );
     
    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        perror("ERROR: Binding Failed");
        return 1;
    }
    puts("binding completed!");
     
    //Listen
    listen(socket_desc , 3);
     
    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
    while((client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)))
    {
        puts("Accepted Connection Request!");
        new_sock = malloc(1);
        *new_sock = client_sock;
         
        if(fork() == 0){
            printf("Connected to Socket! %d\n", server_port);         
            processRequest(client_sock);    
            exit(0);        
        }
         
        puts("Handlers assigned!");
    }
     
    if (client_sock < 0)
    {
        perror("failed connection!");
        return 1;
    }
    return 0;
}

void getServer(int sock, char * filename)
{
    char file_loc[128];
    int fd;
    char buffer[MAX_BUFFER];

    sprintf(file_loc, "%s%s/%s", server_directory, currUser.name, filename);

    if ((fd = open(file_loc, O_RDONLY)) < 0)
        handleError("Failed to open file at: '%s' %s\n", file_loc, strerror(errno)); 
    while (1) {
        int bytes_read = read(fd, buffer, sizeof(buffer));
        if (bytes_read == 0) // done reading file
            break;

        if (bytes_read < 0) {
            // handle errors
            handleError("Failed to read: %s\n", strerror(errno));
        }
        //writing the socket 
        void *p = buffer;
        printf("Writing into socket\n");

        while (bytes_read > 0) {
            int bytes_written = write(sock, p, bytes_read);
            if (bytes_written <= 0) {
            }
            bytes_read -= bytes_written;
            p += bytes_written;
        }
    }
}



void putServer(int sock, char * arg)
{
    char buf[MAX_BUFFER];
    char file_path[128];
    int file_size, remaining, len = 0;
    int len2;
    FILE *file;

    sprintf(file_path, "%s%s/", server_directory, currUser.name);
    strncat(file_path, arg, strlen(arg));
    printf("%s\n", file_path);

    printf("In File System");

    int read_size;
    while((read_size = recv(sock, &buf[len], (MAX_BUFFER-len), 0)) > 0)
    {
        char line[read_size];
        strncpy(line, &buf[len], sizeof(line));
        len += read_size;
        line[read_size] = '\0';
        file_size = atoi(buf);
        printf("%d\n", file_size);
        if (!(file = fopen(file_path, "w")))
            handleError("Failed to open file at: '%s' %s\n", file_path, strerror(errno)); 

        remaining = file_size;

        while (((len2 = recv(sock, buf, MAX_BUFFER, 0)) > 0) && (remaining > 0)) {
            printf("%s\n", buf);

            // write to file
            fprintf(file, "%s\n", buf);
            fclose(file);

            fwrite(buf, sizeof(char), len, file);
            remaining -= len;
            return;
        }
    }
}

void listServer(int sock, char * username){
    printf("Getting Files for: %s\n", username);

    DIR * d;
    struct dirent *dir;
    struct stat filedets;
    int status;
    int i = 0;

    char result[256];

    char path[MAX_BUFFER];
    char directory[4096];

    strcpy(directory, server_directory);
    strcat(directory, username);

    d = opendir(directory);
    int length = 0;

    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (strcmp(".", dir->d_name) == 0){
            } else if (strcmp("..", dir->d_name) == 0){
            } else {
                sprintf(path, "%s/%s", directory, dir->d_name);
                status = lstat(path, &filedets);
                if(S_ISDIR(filedets.st_mode)) {
                } else {
                    printf("%s\n", dir->d_name);
                    if(strncmp(dir->d_name, ".DS_Store", 9) != 0)
                    {
                        length += sprintf(result + length, "%d. %s\n", i, dir->d_name);
                        i++;
                    }
                }
            }
        }
        puts(result);
        write(sock, result, strlen(result));
        closedir(d);
    } 
}

int main(int argc, char *argv[], char **envp)
{
    if(argc != 3)
    {
        printf("Not enough arguments.");
        return -1;
    }

    // create a folder for server contents 
    strcat(server_directory, argv[1]);
    strcat(server_directory, "/");
    if(!opendir(server_directory))
    {
        mkdir(server_directory, 0770);      
    }

    server_port = atoi(argv[2]);

    parse("server/dfs.conf"); //grab what we need for the server

    listenPort(server_port); //start up the server

    return 1;
}