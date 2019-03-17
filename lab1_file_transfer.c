#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/type.h>
#include <sys/socket.h>
#include <netinet/in.h>

int portno;   //use port number

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
    if (sockfd < 0)
        error("ERROR opening socket");

    bzero((char *) &serv_addr, sizeof(serv_addr)); //set serv_addr to zero
    if (bind(sockfd, (struct sockaddr *) &serv_addr, 
                sizeof(serv_addr)) <0)
        error("ERROR on binding");
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    
    if (newsockfd < 0)
        error("ERROR on accept");

    bzero((char *) &serv_addr, sizeof(serv_addr));
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

}

void tcp_c(){

}

void udp_s(){

}

void udp_c(){

}

int main(int argc, char *argv[]){
    if (argc < 4) {
        fprintf(stderr, "ERROR, lose arguments");
        exit(1);
    } else if (strcmp(argv[1], "tcp") != 0 && strcmp(argv[1], "udp") != 0) {  //check second argument whether is tcp or udp
        fprintf(stderr, "ERROR, only support TCP and UDP. Please pass \"tcp\" or \"udp\"");
        exit(1);
    } else if (strcmp(argv[2], "send") != 0 && strcmp(argv[2], "recv") != 0) {  //check third argument whether is send or recv
        fprintf(stderr, "ERROR, please choose a method in \"send\" and \"recv\"");
        exit(1);
    }

    if (strcmp(argv[2], "send") == 0 && argc < 5){ //check fifth argument whether exist
        fprintf(stderr, "ERROR, please input a file name in fifth argument");
        exit(1);
    }

    portno = atoi(argv[3]);

    
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
