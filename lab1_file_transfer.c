#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

int portno;   //use port number
struct hostent *server; //store server ip
FILE *file;   //the file want to transfer




void error(const char *msg){
    perror(msg);
    exit(1);
}

void tcp_s(){
    int sockfd, newsockfd;
    socklen_t clilen;
    char buffer[256];   //to store received data
    struct sockaddr_in serv_addr, cli_addr;
    int n;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);  //build socket for tcp
    if (sockfd < 0) //check socket whether open successfully
        error("ERROR opening socket");

    bzero((char *) &serv_addr, sizeof(serv_addr)); //set serv_addr to zero

    serv_addr.sin_family = AF_INET; //transfer to internet
    serv_addr.sin_addr.s_addr = INADDR_ANY; //set ip of s_addr as 0.0.0.0
    serv_addr.sin_port = htons(portno);  //use portno as port number & make endian same
    
    if (bind(sockfd, (struct sockaddr *) &serv_addr, 
                sizeof(serv_addr)) <0)
        error("ERROR on binding");

    listen(sockfd, 5);
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    
    if (newsockfd < 0)
        error("ERROR on accept");

    bzero(buffer, 256);
    
    n = read(newsockfd, buffer, 255);
    
    if (n < 0) error("ERROR reading from socket");

    printf("Here is the message: %s\n", buffer);
    n = write(newsockfd, "server : I got your message", 18);

    if (n < 0) error("ERROR writing to socket");

    close(newsockfd);
    close(sockfd);
    
    
    return;

}

void tcp_c(){
    int sockfd, n;
    struct sockaddr_in serv_addr;

    char buffer[256];
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) 
        error("ERROR opening socket");

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");
    
    printf("Please enter the message: ");
    

    bzero(buffer, 256);
    fread(buffer, 255, 1, file);

    n = write(sockfd,buffer,strlen(buffer));
    if (n < 0) 
         error("ERROR writing to socket");
    bzero(buffer,256);
    n = read(sockfd,buffer,255);
    if (n < 0) 
         error("ERROR reading from socket");
    printf("%s\n",buffer);
    close(sockfd);
    return;

}

void udp_s(){

}

void udp_c(){

}

int main(int argc, char *argv[]){
    if (argc < 5) {
        fprintf(stderr, "ERROR, lose arguments");
        exit(1);
    } else if (strcmp(argv[1], "tcp") != 0 && strcmp(argv[1], "udp") != 0) {  //check second argument whether is tcp or udp
        fprintf(stderr, "ERROR, only support TCP and UDP. Please pass \"tcp\" or \"udp\"");
        exit(1);
    } else if (strcmp(argv[2], "send") != 0 && strcmp(argv[2], "recv") != 0) {  //check third argument whether is send or recv
        fprintf(stderr, "ERROR, please choose a method in \"send\" and \"recv\"");
        exit(1);
    }

    if (strcmp(argv[2], "send") == 0 && argc < 6){ //check sixth argument whether exist
        fprintf(stderr, "ERROR, please input a file name in fifth argument");
        exit(1);
    } else if (strcmp(argv[2], "send") == 0) {
        file = fopen(argv[5], "r");
        if (file == NULL ){
            fprintf(stderr, "ERROR, open file failure" );
            exit(0);
        }
    }


    server = gethostbyname(argv[3]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }

    portno = atoi(argv[4]);
    

    if (strcmp(argv[1], "tcp") == 0){
        if (strcmp(argv[2], "send") == 0){        //as TCP client
            tcp_c();
        } else if(strcmp(argv[2], "recv") == 0){  //as TCP server
            tcp_s();
        }
    } else if (strcmp(argv[1], "udp") == 0){     
        if (strcmp(argv[2], "send") == 0){        //as UDP client
            tcp_c();
        } else if(strcmp(argv[2], "recv") == 0){  //as UDP server
            tcp_s();
        }
    }



    return 0;
}
