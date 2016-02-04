//literally this assignment was the death of my self esteem. 
//Help from online sources, Github, Youtube, Setting up VM's is still a pain
//code added from previous assignments and updated.
#include <errno.h>
#include <arpa/inet.h>
#include <signal.h>
#include <dirent.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <time.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>


int BUFFER_SIZE = 2048;

/*
    Process the request that a client requests
    Example:
        GET http://www.google.com HTTP/1.0
*/


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

void process_request(int sock){
    char buf[BUFFER_SIZE];
    /*char *token;
    char command[64], full_url[256], http_version[64], url[128], service[64];    
    //this is pulled from my second programming assignment 
    //reading in given information and use our handlers to serve the requests*/
    int server_sock;
    int read_size = 0;
    int len = 0;
    time_t time_date;

    // int server_fd;
    struct sockaddr_in server_socket_setup,server_addr,client_addr,destination_addr;
    socklen_t s_addrlen, client_addrlen, destination_addrlen; 
    s_addrlen = sizeof(struct sockaddr_in);
    client_addrlen = sizeof(struct sockaddr_in);         
    destination_addrlen = sizeof(struct sockaddr_in);

    server_sock = socket(AF_INET , SOCK_STREAM , 0);
    if (server_sock == -1)
    {
        printf("Could not create socket\n");
    }

    server_socket_setup.sin_family = AF_INET;
    server_socket_setup.sin_port = 8080;
    server_socket_setup.sin_addr.s_addr = INADDR_ANY; 
    bzero(&server_socket_setup.sin_zero, 8);


    //handy little stuff handles stuff

    //binding to a port
    if((bind(server_sock, (struct sockaddr *) &server_socket_setup, sizeof(server_socket_setup))) < 0){
        perror("bind failed. Error");
        exit(-1);
    }
    //server information 
    if((getsockname(server_sock, (struct sockaddr *) &server_addr, &s_addrlen)) < 0){
        perror("getsockname failed. Error");
        exit(-1);       
    }
    //client information
    if((getpeername(sock, (struct sockaddr *) &client_addr, &client_addrlen)) < 0){
        perror("getpeername: ");
        exit(-1);       
    }
    //original dest
    if((getsockopt(sock, SOL_IP, 80, (struct sockaddr *) &destination_addr, &destination_addrlen)) < 0){
        perror("getsockopt: ");
        exit(-1);
    }

    //then we need to write out to the table
    char snatRules[BUFFER_SIZE];
    sprintf(snatRules, "iptables -t nat -A POSTROUTING -p tcp -j SNAT --sport %d --to-source %s", ntohs(server_addr.sin_port), inet_ntoa(client_addr.sin_addr));
    printf("%s\n", snatRules);  
    system(snatRules);
    
    char *log_file;
    char stuff_to_log[BUFFER_SIZE];
    time_date = time(NULL);





    log_file = ctime(&time_date);
    log_file[strlen(log_file)-1] = ' ';
    strcat(log_file, inet_ntoa(client_addr.sin_addr));
    sprintf(stuff_to_log, " %d ", ntohs(client_addr.sin_port));
    strcat(log_file, stuff_to_log);
    strcat(log_file, inet_ntoa(destination_addr.sin_addr));
    memset(stuff_to_log, 0, BUFFER_SIZE);
    sprintf(stuff_to_log, " %d ", ntohs(destination_addr.sin_port));
    strcat(log_file, stuff_to_log);

    FILE * file_to_log = fopen("logFile.txt", "a+");
    fprintf(file_to_log, "%s\n", log_file);
    fclose(file_to_log);

    while ((read_size = recv(sock, &buf[len], (BUFFER_SIZE-len), 0)) > 0)
    { 
        char line[read_size];
        strncpy(line, &buf[len], sizeof(line));
        len += read_size;
        line[read_size] = '\0';

        printf("Found:  %s\n", line);
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


int start_port(){
    int sockfd , client_sock, c, *new_sock;
    struct sockaddr_in client;
    char dnat_rules[BUFFER_SIZE];
    char s[INET6_ADDRSTRLEN];
    int file_desc;
    struct ifreq ifr;

    char *port = NULL;

    port = "8080"; 
    //Create socket
    sockfd = socket(AF_INET , SOCK_STREAM , 0);
    if (sockfd == -1)
    {
        printf("Could not create socket\n");
    }
    printf("Socket created\n");
     
    //Prepare the sockaddr_in structure
    client.sin_family = AF_INET;
    client.sin_addr.s_addr = INADDR_ANY;
    client.sin_port = htons(atoi(port));
    bzero(&client.sin_zero, 8);
     
    //Bind socket to address
    if( bind(sockfd,(struct sockaddr *)&client , sizeof(client)) < 0)
    {
        perror("bind failed. Error");
        return 1;
    }
    printf("Binding Complete!\n");
     
    //Listen on socket
    listen(sockfd , 5);

    file_desc = socket(AF_INET, SOCK_DGRAM, 0);
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, "eth1", IFNAMSIZ-1);
    ioctl(file_desc, SIOCGIFADDR, &ifr);
    close(file_desc);
    //spit out rules 
    sprintf(dnat_rules, "iptables -t nat -A PREROUTING -p tcp -i eth1 -j DNAT --to %s:%d", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr), ntohs(client.sin_port));
    printf("DNAT RULES: %s\n", dnat_rules);
    system(dnat_rules);
    printf("\nserver port %d: waiting for connections...\n", ntohs(client.sin_port));
     
    //take in connections 
    printf("Waiting for incoming connections..\n");
    c = sizeof(struct sockaddr_in);
    while( (client_sock = accept(sockfd, (struct sockaddr *)&client, (socklen_t*)&c)) )
    {

        printf("server port %s: got connected from %s\n", port, s);

        new_sock = malloc(1);
        *new_sock = client_sock;
         
        //handles multiple connections
        if(fork() == 0){
            printf("Connected! %s\n", port);         
            process_request(client_sock);    
            exit(0);        
        }
    }
    if (client_sock < 0)
    {
        perror("accept failed");
        return 1;
    }
    return 0;
}


/*
    Main function
*/
int main(int argc, char *argv[]) {

    start_port();

    return EXIT_SUCCESS;

}