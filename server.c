/* A simple server in the internet domain using TCP
   The port number is passed as an argument
   This version runs forever, forking off a separate
   process for each connection
*/
#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>  /* signal name macros, and the kill() prototype */

#define PORT 1025


char *getFilename(char *);
void respond(int, char *);
void write404(int);
void write200(int, FILE *);


void error(char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno; //listen on sock_fd, new connection new_fd
    int pid;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

    //if (argc < 2) {
    //   fprintf(stderr,"ERROR, no port provided\n");
    //   exit(1);
    //}

    sockfd = socket(AF_INET, SOCK_STREAM, 0);  // create socket
    if (sockfd < 0)
        error("ERROR opening socket");

    memset((char *) &serv_addr, 0, sizeof(serv_addr));   // reset memory

    // fill in address info
    //portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    listen(sockfd, 5);  // 5 simultaneous connection at most

    


    while(1){
        //accept connections
        clilen = sizeof(struct sockaddr_in);
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

        if (newsockfd < 0)
            error("ERROR on accept");

        pid = fork();

        if(pid<0)
            error("ERROR on fork");

        if(pid==0){
            
            close(sockfd);
            int n;
            char request_buffer[1024]; //increase the buffer size to print full request message

            memset(request_buffer, 0, 1024);  // reset memory


            //read client's message
            n = read(newsockfd, request_buffer, 1023);
            if (n < 0) error("ERROR reading from socket");
            printf("Here is the message: %s\n", request_buffer);

            //get requested filename from client
            char *file_name = getFilename(request_buffer);

            printf("\nFile name request: %s\n",file_name);
    
    
            //reply to client
            //n = write(newsockfd, "I got your message", 18);
            //if (n < 0) error("ERROR writing to socket");
        
            //printf("\nI got your message\n");
    
            respond(newsockfd, file_name);
            exit(0);
        }

        else
            close(newsockfd);  // close connection
            //close(sockfd);
        //close(sockfd);
    }

    return 0;
}


//update to handle cases when file name contains space
char *getFilename(char *request_buffer)
{
    //can't handle when file name contains spaces
    /*
    char *file_name;
    char *temp;
    char *found;
    int count = 0;

    temp= strdup(request_buffer);

    while(count<2 && (found = strsep(&temp," ")) != NULL){
        file_name = found;
        count = count + 1;
    }

    file_name = file_name + 1; //get rid of '/'


    return file_name;
    */

    //store the request buffer to a temp string
    char *temp = request_buffer;

    char *pattern1 = "GET /";
    char *pattern2 = " HTTP/1.1";

    char *filename = NULL;
    char *start;
    char *end;

    start = strstr(temp, pattern1);
    end = strstr(temp, pattern2);
    if(start!=NULL){
        start = start + 5;
    }
    if(end!=NULL){
        filename = (char *)malloc(end - start + 1);
        memcpy(filename, start, end-start);
    }

    //replace all "%20" with " "
    int i;
    int count = 0;
    for(i = 0; i<strlen(filename); i++){
        if(filename[i] == '%'){
            count++;
        }
    }
    //printf("total number of blank spaces : %d\n", count);
    char buffer[1024];

    for(i = 0; i < count; i++){
    //find space index
    char * space = strstr(filename, "%20");
    //copy to buffer the substring before space
    strncpy(buffer, filename, space-filename);
    //append the substring after space
    buffer[space-filename] = 0;
    sprintf(buffer + (space - filename), "%s%s", " ", space + 3);
 
    //update filename
    filename[0] = 0;
    strcpy(filename, buffer);
    }
    return filename;

    
}


//todo: search for filename, return error message if not found, else respond to client
void respond(int newsockfd, char *file_name){

    FILE *file = fopen(file_name, "r");
    
    //if file does not found, return error message
    if (!file) {
        // no file or file not found
        printf("404: File not found\n");
        write404(newsockfd);
        return;
    }

    //if file is found, show file in browser
    printf("found file!\n");
    write200(newsockfd, file);
    fclose(file);
    return;
}

void write404(int newsockfd) {
    write(newsockfd, "HTTP/1.1 ", 9);
    write(newsockfd, "404 Not Found\n", 14);
    write(newsockfd, "Content-Length: 0\n", 18);
    write(newsockfd, "Content-Language: en-US\n", 24);
    write(newsockfd, "Content-Type: text/html\n", 22);
    write(newsockfd, "Connection: close\n\n", 19);
}

void write200(int newsockfd, FILE *file) {
    // get the size of file
    fseek(file, 0L, SEEK_END);
    int filesize = (int) ftell(file);
    fseek(file, 0L, SEEK_SET);
    printf("Filesize: %d\n", filesize);
    // use buffer to
    char* filebuf = (char *) malloc(sizeof(char) * filesize);
    fread(filebuf, 1, filesize, file);

    write(newsockfd, "HTTP/1.1 ", 9);
    write(newsockfd, "200 OK\n", 5); // file found
    write(newsockfd, "Connection: keep-alive\n\n", 24);
    write(newsockfd, filebuf, filesize);
    if (ferror(file)) error("ERROR reading file");

    printf("Done with transfer!\n");
    free(filebuf);
}

