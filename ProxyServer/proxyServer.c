///I hate everything

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <err.h>
#include <fcntl.h>
#include <sys/errno.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>

int BUFFER_SIZE = 2048;



//to start since i was getting some 404 but still works and pulls header information regardless?
/*
void error(int client, const char *filename, int error_num){
    char buf[BUFFER_SIZE];
    if (error_num == 404)
    {
        sprintf(buf, "HTTP/1.1 404 NOT FOUND\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "Content-Type: text/html\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "<HTML><TITLE>404 NOT FOUND</TITLE>\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "<BODY><P>HTTP/1.1 404 Not Found: %s \r\n", filename);
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "</BODY></HTML>\r\n");
        send(client, buf, strlen(buf), 0);
    }
}
*/
//http://www.beej.us/guide/bgnet/output/html/singlepage/bgnet.html#getaddrinfo

void get_request(int sock, char *url, char *http_version, char *request){
    // int status;
    struct hostent *server;
    struct sockaddr_in serveraddr;
    char response[2048];

    if(strncmp(http_version, "HTTP/1.0", 8) == 0){
        printf("HTTP is correct!\n");

        server = gethostbyname(url);

        if (server == NULL){
            printf("gethostbyname() failed\n");
        }

    printf("Host Name: %s\n", server->h_name);
    printf("Host IP address: %s\n", inet_ntoa(*(struct in_addr*)server->h_addr));


        // connect to the server
        bzero((char *) &serveraddr, server->h_length);

        serveraddr.sin_family = AF_INET;
        // server.sin_addr.s_addr = inet_addr(host);
        bcopy((char *)server->h_addr, (char *)&serveraddr.sin_addr.s_addr, server->h_length );
        serveraddr.sin_port = htons(80);

        int tcpSocket = socket(AF_INET, SOCK_STREAM, 0);

        if (connect(tcpSocket, (struct sockaddr *) &serveraddr, sizeof serveraddr ) < 0){
            printf("\nError with Connection");
        }

        strcat(request, "\r\n");
        // Send request to socket
        if (send(tcpSocket, request, strlen(request), 0) < 0){
            printf("Error with send()");
        }
        bzero(response, 1000);
        recv(tcpSocket, response, 999, 0);
        write(sock, response, strlen(response));

    } else {
        printf("HTTP is not correct!!\n");
    }
}

void process_request(int socket){
    char buf[BUFFER_SIZE];
    char *token;
    int read_size = 0;
    int len = 0;
    char command[64], full_url[256], http_version[64], url[128], service[64];    
    //this is pulled from my second programming assignment 
    //reading in given information and use our handlers to serve the requests
    while ((read_size = recv(socket, &buf[len], (BUFFER_SIZE-len), 0)) > 0)
    { 
        char line[read_size];
        strncpy(line, &buf[len], sizeof(line));
        len += read_size;
        line[read_size] = '\0';

        printf("Found:  %s\n", line);

      
        sscanf(line, "%s %s %s", command, full_url, http_version);
        if (strncmp(full_url, "http", 4) == 0){
            token = strtok(full_url, "://");
            strcpy(service, token);
            token = strtok(NULL, "//");
            strcpy(url, token);
        } else {
            printf("HTTP not found in url\n");
            strcpy(url, full_url);
        }

        // We only support GET
        if(strncmp(command, "GET", 3) == 0) {
            get_request(socket, url, http_version, line);
        } else {
            printf("No Such Command: %s\n", command);
        }
    } 

    if(read_size == 0) {
        puts("Client disconnected");
        fflush(stdout);
    } else if(read_size == -1){
        perror("recv failed");
    }
    return;
}

/*
int start_server(int port) {
    //basic TCP web server start up only difference is the forking while listening 
    int one = 1, client_request, sock;
    struct sockaddr_in server_address, client_address;
    socklen_t client_length = sizeof(client_address);
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) <= 0) {
        perror("Can't open socket");
        return -1;
    }
 
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) < 0) {
        perror("Setsockopt - SO_REUSEADDR failed");
        return -1;
    }
 
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port);
 
    // Attach the socket to the port given above
    if (bind(sock, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
        close(sock);
        perror("Bind failed");
        return -1;
    }
 
    listen(sock, 5);
    // we are waiting for clients here put in an infinite loop till we get a request
    while (1) {
    //create a new socket, fork a child process to handle that new socket
        client_request = accept(sock, (struct sockaddr *) &client_address, &client_length);
        if (client_request == -1) {
            perror("Can't accept");
        }
        if(fork() == 0){
            requests(client_request); //for out a child process to handle the request. 
            exit(0);
        }
        close(client_request);
    }
    close(sock);

    return 0;
}
*/
int start_port(int port){
    int sockfd , client_sock, c, *new_sock;
    struct sockaddr_in server , client;
     
    //Create socket
    sockfd = socket(AF_INET , SOCK_STREAM , IPPROTO_TCP);
    if (sockfd == -1)
    {
        printf("Could not create socket\n");
    }
    printf("Socket created\n");
     
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);

    // timeouts on socket
    struct timeval timeout;      
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    if (setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
        perror("setsockopt failed\n");

    if (setsockopt (sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
        perror("setsockopt failed\n");
     
    //Bind socket to address
    if( bind(sockfd,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("Bind Failed. Error");
        return 1;
    }
    printf("Binding Complete\n");
     
    //Listen on socket
    listen(sockfd , 5);
     
    //Accept and incoming connection
    printf("Waiting for incoming connections..\n");
    c = sizeof(struct sockaddr_in);
    while( (client_sock = accept(sockfd, (struct sockaddr *)&client, (socklen_t*)&c)) )
    {
        printf("Accepted Connection Request\n");
        new_sock = malloc(1);
        *new_sock = client_sock;
         
        // fork it so multiple connections can come in
        if(fork() == 0){
            printf("Connected! %d\n", port);         
            process_request(client_sock);    
            exit(0);        
        }
    }
    if (client_sock < 0)
    {
        perror("failed connection");
        return 1;
    }
    return 0;
}

/*
    Main functions
*/
int main(int argc, char *argv[]) {
    int port;

    if (argc != 2) {
        fprintf(stderr, "Aurguemtns Invalid! Enter a port number, please");
        exit(0);
    }

    port = atoi(argv[1]); //atoi converts a string inout and turns into a integer
    start_port(port); //start up the server


    return EXIT_SUCCESS;

}