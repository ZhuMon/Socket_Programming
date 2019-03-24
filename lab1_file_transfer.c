#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>

int portno;   //use port number
char *file_name; //store file name
struct hostent *server; //store server ip
FILE *file;   //the file want to transfer
struct stat f_state; //store state of the file


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
    n = read(newsockfd, buffer, 256);
    
    if (n < 0) error("ERROR reading from socket");

    ////parse first line in buffer
    char file_name[256]; //store file name in first line
    int line_pointer = 0; //store position of '\n' in buffer
    
    for(int i = 0; i < 256; i++){   //let first line of buffer store in file_name
        if(buffer[i] == '\n'){
            line_pointer = i;
            break;
        }
        file_name[i] = buffer[i];
    }

    if(line_pointer == 0){
        error("ERROR, first packet format eroor");
    }

    printf("File name: %s\n", file_name);
    FILE *outfile;
    outfile = fopen(file_name, "ab");
    
    ////parse second line in buffer
    char file_size_c[256]; //store the file_size in character
    int file_size_i; //store the file size in integer

    for(int i = 0; i < 256; i++){  //let second line of buffer store in file_size_c
        if(buffer[i+line_pointer+1] == '\n'){
            line_pointer = i+line_pointer+1;
            break;
        }
        file_size_c[i] = buffer[i+line_pointer+1];
    }
    
    if(line_pointer == 0){
        error("ERROR, first packet format eroor");
    }

    file_size_i = atoi(file_size_c); //change file_size from char to int

    printf("File size: %d\n", file_size_i);

    ////parse third line in buffer
    char p_num_c[256]; //store packet number in character
    int p_num_i; //store packet number in integer
    
    for(int i = 0; i < 256; i++){  //let third line of buffer store in p_num_c
        if(buffer[i+line_pointer+1] == '\n'){
            line_pointer = i+line_pointer+1;
            break;
        }
        p_num_c[i] = buffer[i+line_pointer+1];
    }

    if(line_pointer == 0){
        error("ERROR, first packet format eroor");
    }

    p_num_i = atoi(p_num_c);

    printf("packet number: %d\n", p_num_i);

    for(int i = 0; i < p_num_i; i++){
        /*newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    
        if (newsockfd < 0)
            error("ERROR on accept");
        */
        bzero(buffer, 256);
        if(i == p_num_i-1){
            n = read(newsockfd, buffer, file_size_i%256);
            fwrite(buffer, 1, file_size_i%256, outfile);
        } else {
            n = read(newsockfd, buffer, 256);
            fwrite(buffer, 1, 256, outfile);
        }
    }


    //fwrite(buffer, 1, sizeof(buffer), outfile);

    fclose(outfile);

    //printf("Here is the message: \n%s\n", buffer);
    n = write(newsockfd, "server : I got your message", 27);

    if (n < 0) error("ERROR writing to socket");

    close(newsockfd);
    close(sockfd);
    
    
    return;

}

void tcp_c(){
    int sockfd, n;
    struct sockaddr_in serv_addr;

    //char buffer[25600];
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
    
    //pass a packet which store file name, file size, and # of packets 
    char f_buffer[256]; //first buffer
    bzero(f_buffer, 256);
    strcat(f_buffer, file_name); //store file name in first line

    strcat(f_buffer, "\n");   

    char file_size[256];
    sprintf(file_size, "%lld", f_state.st_size);  //change file_size from int to char
    strcat(f_buffer, file_size); //store file size in second line
    strcat(f_buffer, "\n");

    char p_num[256]; //the number of packet that the file will spilt into 
    sprintf(p_num, "%lld", f_state.st_size/256 +1); //change packet number from int to char
    strcat(f_buffer, p_num);
    printf("f_buffer: %s\n", f_buffer);
    strcat(f_buffer, "\n");

    n = write(sockfd, f_buffer, 256);
    if (n < 0) 
         error("ERROR writing to socket. (first packet)");

    //buffer store the transfered file
    char buffer[f_state.st_size+1]; //make buffer equal to size of file
    bzero(buffer, f_state.st_size+1); //clean buffer
    fread(buffer, f_state.st_size, 1, file); //read file to buffer
    
    char s_buffer[256]; //small buffer for each packet
    for(int i = 0; i < f_state.st_size/256 + 1; i++){
        bzero(s_buffer, 256);  

        if(i == f_state.st_size/256){ //the last packet or file size < 256
            for(int j = 0; j < f_state.st_size%256;j++){
                s_buffer[j] = buffer[i*256+j];
            }
            n = write(sockfd,s_buffer, f_state.st_size%256);
        } else {
            for(int j = 0; j < 256; j++){  
                s_buffer[j] = buffer[i*256+j];
            }
            n = write(sockfd,s_buffer,256);
        }

        if (n < 0) 
             error("ERROR writing to socket. (file packet)");
    }
    
    /*
    n = write(sockfd,buffer,strlen(buffer));

    if (n < 0) 
         error("ERROR writing to socket");
    */
    bzero(s_buffer,256);
    n = read(sockfd,s_buffer,256);
    if (n < 0) 
         error("ERROR reading from socket");
    printf("%s\n",s_buffer);
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
        file = fopen(argv[5], "rb");
        if (file == NULL ){
            fprintf(stderr, "ERROR, open file failure" );
            exit(0);
        }
        stat(argv[5], &f_state);
        file_name = argv[5];

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
