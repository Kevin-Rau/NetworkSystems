


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

#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>

int BUFFER_SIZE = 1024;


//this will be storing all the information we need form the config file
struct Config {
    int    port;
    char    DocumentRoot[256];
    char    DirectoryIndex[256];
    char    content_type[100];
    int     content_num;
} config;


struct Request {
    char    method[256];
    char    url[256];
    char    httpver[256];
    int     keep_alive;
} request; 


void requests(int);
void send_file(int, const char *);
int get_line(int, char *, int);
void header(int , const char *);
void error(int, const char *, int );
int start_server(int);

void serveFile(int client, const char *filename){
    char *send_buffer;
    FILE *requested_file;
    long fileLength;
    printf("Received request for file: %s on socket %d\n", filename + 1, client);
  
    requested_file = fopen(filename, "rb");

    if (requested_file == NULL){
        error(client, filename, 404);
    }
    if(strstr(filename, ".html") != NULL){
        FILE *resource = NULL;
        int numchars = 1;
        char buf[BUFFER_SIZE];
        char buf1[BUFFER_SIZE];

        buf[0] = 'A'; buf[1] = '\0';
        while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers */
            numchars = get_line(client, buf, sizeof(buf));

        resource = fopen(filename, "r");
        header(client, filename);

        fgets(buf1, sizeof(buf1), resource);
        while (!feof(resource)){
            send(client, buf1, strlen(buf1), 0);
            fgets(buf1, sizeof(buf1), resource);
        }
        fclose(resource);
    } else {
        //find the requested file, then send the size of the file 
        fseek(requested_file, 0, SEEK_END);
        fileLength = ftell(requested_file);
        rewind(requested_file);
        send_buffer = (char*) malloc(sizeof(char)*fileLength);
        size_t result = fread(send_buffer, 1, fileLength, requested_file);
    
        if (result > 0) {
            header(client, filename);
            send(client, send_buffer, result, 0);   
        }   
        else { error(client, NULL, 500); }
    }

    fclose(requested_file);
}
//like in the httpc.c file, we are going to send the headers to print to the socket 
void header(int client, const char *filename){
    //this will read file and send back the header information as seen in tinyurl
    char buf[BUFFER_SIZE];
    (void)filename; 
    const char* filetype;
    struct stat st; 
    off_t size;

    if (stat(filename, &st) == 0)
        size = st.st_size;
//taken from TinyHttp & Zhued for getting the file info
    const char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) {
        filetype = "";
    } else {
        filetype = dot + 1;
    }
    //based on  that is passed we will return these sets of information
    if(strcmp(filetype, "html") == 0)
    {
        strcpy(buf, "HTTP/1.1 200 OK\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "Content-Length: %zd \r\n", size);
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "Content-Type: text/html\r\n");
        send(client, buf, strlen(buf), 0);
        strcpy(buf, "\r\n");
        send(client, buf, strlen(buf), 0);
    }

    else if(strcmp(filetype, "txt") == 0)
    {
        strcpy(buf, "HTTP/1.1 200 OK\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "Content-Type: text/plain\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "Content-Length: %zd \r\n", size);
        send(client, buf, strlen(buf), 0);
        strcpy(buf, "\r\n");
        send(client, buf, strlen(buf), 0);
    }
    else if (strcmp(filetype, "png") == 0)
    {
        strcpy(buf, "HTTP/1.1 200 OK\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "Content-Type: image/png\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "Content-Length: %zd \r\n", size);
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "Content-Transfer-Encoding: binary\r\n");
        send(client, buf, strlen(buf), 0);
        strcpy(buf, "\r\n");
        send(client, buf, strlen(buf), 0);
    }

    else if (strcmp(filetype, "gif") == 0)
    {
        strcpy(buf, "HTTP/1.1 200 OK\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "Content-Type: image/gif\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "Content-Length: %zd \r\n", size);
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "Content-Transfer-Encoding: binary\r\n");
        send(client, buf, strlen(buf), 0);
        strcpy(buf, "\r\n");
        send(client, buf, strlen(buf), 0);
    }
}

//this will handle all the errors we need like in tinyurl just in a large function
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
    else if (error_num == 400)
    {
        send(client, "HTTP/1.1 400 BAD REQUEST\r\n", sizeof("HTTP/1.1 400 BAD REQUEST\r\n"), 0);
        send(client, "Content-type: text/html\r\n", sizeof("Content-type: text/html\r\n"), 0);
        send(client, "\r\n", sizeof("\r\n"), 0);
        if(strcmp(filename, "Invalid Method")){
            sprintf(buf, "<P>HTTP/1.1 400 Bad Request:  Invalid Method: %s \r\n", request.method);
            send(client, buf, strlen(buf), 0);
            // send(client, "<P>HTTP/1.1 400 Bad Request:  Invalid Method", sizeof("<P>HTTP/1.1 400 Bad Request:  Invalid Method"), 0);
        }
        if(strcmp(filename, "Invalid Version")){
            sprintf(buf, "<P>HTTP/1.1 400 Bad Request:  Invalid Version: %s \r\n", request.httpver);
            send(client, buf, strlen(buf), 0);
            // send(client, "<P>HTTP/1.1 400 Bad Request:  Invalid Version", sizeof("<P>HTTP/1.1 400 Bad Request:  Invalid Version"), 0);
        }
        if(strcmp(filename, "Invalid URI")){
            send(client, "<P>HTTP/1.1 400 Bad Request:  Invalid URI", sizeof("<P>HTTP/1.1 400 Bad Request:  Invalid URI"), 0);
        }
        send(client, "\r\n", sizeof("\r\n"), 0);
    }
    else if (error_num == 500) 
    {
        sprintf(buf, "HTTP/1.0 500 Internal Server Error\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "Content-type: text/html\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "HTTP/1.1 500  Internal  Server  Error:  cannot  allocate  memory\r\n");
        send(client, buf, strlen(buf), 0);

    }
    else if (error_num == 501) {
        sprintf(buf, "HTTP/1.1 501 Method Not Implemented\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "Content-Type: text/html\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "</TITLE></HEAD>\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "<BODY><P>HTTP/1.1 501  Not Implemented:  %s\r\n", filename);
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "</BODY></HTML>\r\n");
        send(client, buf, strlen(buf), 0);

    }

}

int get_line(int sock, char *buf, int size_buffer)
{ //getting the lines from the socket, this was found on tinyurl set buffer to header
    //will return the number of bytes stored
    int p = 0;
    char c = '\0';
    int n;

    while ((p < size_buffer - 1) && (c != '\n'))
    {
        n = recv(sock, &c, 1, 0);
        if (n > 0){

            if (c == '\r'){
                n = recv(sock, &c, 1, MSG_PEEK);
                if ((n > 0) && (c == '\n'))
                    recv(sock, &c, 1, 0);
                else
                    c = '\n';
            }
            buf[p] = c;
            p++;
        }

        else
            c = '\n';
    }
    buf[p] = '\0';

    return(p);
}

//param is socket connection to the client like accept_request. 
void requests(int client){
    char buf[BUFFER_SIZE];
    int numchars;
    char path[512];
    struct stat st;
    char *extension;
    char all_headers[1024];


    numchars = get_line(client, buf, sizeof(buf));

    sscanf(buf, "%s %s %s", request.method, request.url, request.httpver);

    request.method[strlen(request.method)+1] = '\0';
    request.url[strlen(request.url)+1] = '\0';
    request.httpver[strlen(request.httpver)+1] = '\0';

    while ((numchars > 0) && strcmp("\n", buf)){
        numchars = get_line(client, buf, sizeof(numchars));
        strcat(all_headers, buf);
    }


    extension = strrchr(request.url, '.');
    if (extension != NULL) {
        if(strstr(config.content_type, extension) == NULL){
            error(client, request.url, 501);
            return;
        }
    }
    if (strstr( request.url, " " ) || strstr( request.url, "\\" )){
        error(client, "Invalid URI", 400);
    }

    if (strcasecmp(request.method, "GET") != 0){
        error(client, "Invalid Method", 400);
    }

    if ((strcmp(request.httpver, "HTTP/1.1") != 0) && (strcmp(request.httpver, "HTTP/1.0") != 0) ){
        error(client, "Invalid Version", 400);
    }

    // Addes the url to the path
    sprintf(path, "%s%s", config.DocumentRoot, request.url);
    if (path[strlen(path) - 1] == '/')
        strcat(path, config.DirectoryIndex);

    if (stat(path, &st) == -1) {
        error(client, path, 404);
    } else {
        serveFile(client, path);
    }    

    close(client);
}

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


int main() {
 
    char *line;
    char head[64], tail[256];
    size_t len = 0;
    int read_len = 0;
    int p = -1;

    FILE* conf_file = fopen("ws.conf","r");
    while((read_len = getline(&line, &len, conf_file)) != -1) {

        line[read_len-1] = '\0';

        if (line[0] == '#')
            continue;
        sscanf(line, "%s %s", head, tail);

        if (!strcmp(head, "Listen")) {
            config.port = atoi(tail);
        } 
   
        if (!strncmp(head, "DocumentRoot", 12)) {
            sscanf(line, "%*s \"%s", config.DocumentRoot);
      
            config.DocumentRoot[strlen(config.DocumentRoot)-1] = '\0';
        } 

        if (!strcmp(head, "DirectoryIndex")) {
            sscanf(line, "%*s %s", config.DirectoryIndex);
            config.DocumentRoot[strlen(config.DirectoryIndex)-1] = '\0';
        }
        if (head[0] == '.') {
                strcat(config.content_type, head);
                strcat(config.content_type, " ");
        }
    }
    fclose(conf_file);
    //close up the file and start our webserver! we are ready to go. 
    p = start_server(config.port);
    return EXIT_SUCCESS;
}

