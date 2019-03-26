#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>

int portno;                 //use port number
struct hostent *server;     //store server ip
FILE *file;                 //the file want to transfer
char file_name[256];            //store file name
struct stat f_state;        //store state of the file
struct timeval my_t;        //record arrival time in usec & sec
struct tm *p;               //record time in year, month ...

void print_time(int percent){
    gettimeofday(&my_t, NULL); //get sec & usec from 1970/1/1 0:00:00
    p = localtime(&my_t.tv_sec); //turn second to year, month, day, ...
    printf("%3d%% %02d/%02d/%02d %02d:%02d:%02d.%06d\n", percent, (1900+p->tm_year), (1+p->tm_mon), p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec, my_t.tv_usec);

}

void error(const char *msg){
    perror(msg);
    exit(1);
}

void tcp_s(){
    int sockfd, newsockfd;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    int n;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);  //build socket for tcp
    if (sockfd < 0) //check socket whether open successfully
        error("ERROR opening socket");

    bzero((char *) &serv_addr, sizeof(serv_addr)); //set serv_addr to zero

    serv_addr.sin_family = AF_INET; //transfer to internet
    serv_addr.sin_addr.s_addr = INADDR_ANY; //允許任何IP連接server 
    serv_addr.sin_port = htons(portno);  //use portno as port number & make endian same
    
    if (bind(sockfd, (struct sockaddr *) &serv_addr, 
                sizeof(serv_addr)) <0)
        error("ERROR on binding");

    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    
    if (newsockfd < 0)
        error("ERROR on accept");

    char buffer[256];   //to store data of first packet
    
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
    outfile = fopen(file_name, "wb");
    
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

    int fp_size = (int)(file_size_i/20.0+0.5); //size of packet is file_size/20 四捨五入
    int p_size; //packet size
    
    //if 5% file >= 256 bytes, after transfer, file will crash. So, let size of packet limited to 256 bytes
    if(fp_size >= 256){
        p_size = 256;
    } else {
        p_size = fp_size; //each packet size = 5% file size
    }
    
    
    char p_buffer[p_size]; //buffer for each packet
    
    int fp_num; //packet number = 20(the last packet will be big)
               //             or 21(the last packet will be small) 
    if(fp_size == (int) file_size_i/20){
        fp_num = 21;
    } else if(fp_size == (int) file_size_i/20 + 1){
        fp_num = 20;
    } else {
        error("Error, packet_size error");
    }

    if(file_size_i % 20 == 0){ //如果20個packet可以平分完file，就只使用20個packet
        fp_num = 20;
    }

    if(p_size == fp_size){    // if 5% file <= 256 bytes (only 20 or 21 packets)
        for(int i = 0; i < fp_num; i++){
            bzero(p_buffer, p_size);

            if(i == fp_num-1){
                n = read(newsockfd, p_buffer, file_size_i % p_size);
                fwrite(p_buffer, 1, file_size_i % p_size, outfile);
                print_time(100); //print 100%
            } else {
                n = read(newsockfd, p_buffer, p_size);
                fwrite(p_buffer, 1, p_size, outfile);
                if(i != 19){
                    print_time(5*(i+1));
                }
            }
        }
    } else { //each packet team has more than 1 packet
        int tp_num; // X packets/team. Record packet num of each team (except the last) 
        int tp_num_l; // X packets/team. Record packet number of the last team
        
        //分析每個team有多少個packets
        //無條件進位
        if(fp_size % 256 != 0){
            tp_num = fp_size/256 + 1;
        } else {
            tp_num = fp_size/256;
        }

        //最後一個team的packet 個數
        if(file_size_i % 20 == 0){ //若20個team可以平分file，最後一個team的packet個數就會與其他的一樣
            tp_num_l = tp_num;
        }else{
            if(file_size_i % fp_size % 256 != 0){
                tp_num_l = file_size_i % fp_size /256 + 1;
            } else {
                tp_num_l = file_size_i % fp_size /256;
            }
        }

         //每個i代表1個team
        for(int i = 0; i < fp_num; i++){

            if(i == fp_num -1 && file_size_i % 20 != 0){ //the last team whose packets different than others
                for(int j = 0; j < tp_num_l; j++){
                    bzero(p_buffer, 256);
                    if(j == tp_num_l -1){
                        n = read(newsockfd, p_buffer, file_size_i%fp_size%256);
                        fwrite(p_buffer, 1, file_size_i%fp_size%256, outfile);
                        print_time(100);
                     } else {
                        n = read(newsockfd, p_buffer,256);
                        fwrite(p_buffer, 1, 256, outfile);
                     }
                 }
            } else { //the other team
                //每個j代表team中的1個packet
                for(int j = 0; j < tp_num; j++){
                    bzero(p_buffer, 256);
                    if(j == tp_num - 1){  //the last packet
                        n = read(newsockfd, p_buffer, fp_size%256);
                        fwrite(p_buffer, 1, fp_size%256, outfile);
                        if(i != 19){
                            print_time((i+1)*5);
                        }
                    } else{
                        //每個k代表buffer的一個byte
                        n = read(newsockfd, p_buffer, 256);
                        fwrite(p_buffer, 1, 256, outfile);
                    }
                }
            }
        }

    }

    //fwrite(buffer, 1, sizeof(buffer), outfile);

    fclose(outfile);

    //printf("Here is the message: \n%s\n", buffer);
    n = write(newsockfd, "完成", 6);

    if (n < 0) error("ERROR writing to socket");

    close(newsockfd);
    close(sockfd);
    
    
    return;

}

void tcp_c(){
    int sockfd, n;
    struct sockaddr_in serv_addr;

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
    
    //pass a packet which store file name, and file size 
    char f_buffer[256]; //first buffer
    bzero(f_buffer, 256);
    strcat(f_buffer, file_name); //store file name in first line
    strcat(f_buffer, "\n");   

    char file_size_c[256];  //store file_size in character
    sprintf(file_size_c, "%lld", f_state.st_size);  //change file size from int to char
    strcat(f_buffer, file_size_c); //store file size in second line
    strcat(f_buffer, "\n");

    n = write(sockfd, f_buffer, 256);
    if (n < 0) 
         error("ERROR writing to socket. (first packet)");
    
    //calculate size of packets
    // 一個packet team 的大小 = 5%的file大小四捨五入
    int fp_size = f_state.st_size/20.0 + 0.5; 
    int p_size; //packet size;
    
    // if 5% file >= 256 bytes, after transfer, file will crash.
    // So, let size of packet limited to 256 bytes
    if(fp_size >= 256){  
        p_size = 256;
    } else {
        p_size = fp_size; //each packet size = 5% file size
    }

    char p_buffer[p_size]; //buffer for each packet

    
    // 5% packet number = 20(the last packet will be big)
    //                 or 21(the last packet will be small) 
    // in order to make each packet team equal to 5% file
    // a packet team consist of 
    //     many 256B_packets & (one <256B_packet or none)
    // no team:
    //   681 = 20*34 + 1*1      -> 21 packets
    //   699 = 19*35 + 1*34     -> 20 packets
    // team:
    //   20519 = 19*(4*256+1*2)+1*(4*256+1*1) -> 20 packet teams -> 100 packets

    int fp_num;
    if(fp_size == f_state.st_size/20){
        fp_num = 21;
    } else if(fp_size == f_state.st_size/20 + 1){
        fp_num = 20;
    }
    if(f_state.st_size % 20 == 0){ //如果20個packet可以平分完file，就只使用20個packet
        fp_num = 20;
    }

    //read file
    char buffer[f_state.st_size+1]; //make buffer equal to size of file
    bzero(buffer, f_state.st_size+1); //clean buffer
    fread(buffer, f_state.st_size, 1, file); //read file to buffer
    


    //pass packets split from file
    if(p_size == fp_size){    // if 5% file <= 256 bytes (only 20 or 21 packets)
        for(int i = 0; i < fp_num ; i++){
            bzero(p_buffer, p_size);  
            if(i == fp_num-1){ //the last packet
                //若20個packet可以平分file，最後一個packet還是使用 p_size
                if(fp_size == f_state.st_size/20){ 
                    for(int j = 0; j < p_size; j++){  
                        p_buffer[j] = buffer[i*p_size+j];
                    }
                    n = write(sockfd,p_buffer,p_size);
                    break;
                } 
                //最後一個packet 不會是滿的，所以j只到f_state.st_size%p_size
                for(int j = 0; j < f_state.st_size%p_size; j++){
                    p_buffer[j] = buffer[i*p_size+j];
                }
                n = write(sockfd,p_buffer, f_state.st_size%p_size);
            } else { 
                //spilt file into 19 or 20 packets and transfer them
                for(int j = 0; j < p_size; j++){  
                    p_buffer[j] = buffer[i*p_size+j];
                }
                n = write(sockfd,p_buffer,p_size);
            }

            if (n < 0) error("ERROR writing to socket. (file packet)");
        }
    } else { //each packet team has more than 1 packet
        int tp_num; // X packets/team. Record packet num of each team (except the last) 
        int tp_num_l; // X packets/team. Record packet number of the last team
        
        //分析每個team有多少個packets
        //無條件進位
        if(fp_size % 256 != 0){
            tp_num = fp_size/256 + 1;
        } else {
            tp_num = fp_size/256;
        }

        //最後一個team的packet 個數
        if(f_state.st_size % 20 == 0){ 
            //若20個team可以平分file，最後一個team的packet個數就會與其他的一樣
            tp_num_l = tp_num;
        }else{
            if(f_state.st_size % fp_size % 256 != 0){
                tp_num_l = f_state.st_size % fp_size /256 + 1;
            } else {
                tp_num_l = f_state.st_size % fp_size /256;
            }
        }

        //每個i代表1個team
        for(int i = 0; i < fp_num; i++){
            if(i == fp_num -1 && f_state.st_size % 20 != 0){ 
                //the last team whose packets different than others
                 for(int j = 0; j < tp_num_l; j++){
                     bzero(p_buffer, 256);
                     if(j == tp_num_l -1){
                         for(int k = 0; k < f_state.st_size%fp_size%256; k++){
                             p_buffer[k] = buffer[i*fp_size+j*256+k];
                         }
                         n = write(sockfd, p_buffer, f_state.st_size%fp_size%256);

                     } else {
                        for(int k = 0; k < 256; k++){
                            p_buffer[k] = buffer[i*fp_size+j*256+k];
                        }
                        n = write(sockfd, p_buffer, 256);
                     }
                 }
            } else { //the other team
                //每個j代表team中的1個packet
                for(int j = 0; j < tp_num; j++){
                    bzero(p_buffer, 256);
                    if(j == tp_num - 1){  //the last packet
                        for(int k = 0; k < fp_size%256;k++){
                            p_buffer[k] = buffer[i*fp_size + j*256 + k];
                        }
                        n = write(sockfd, p_buffer, fp_size%256);
                    } else{
                        //每個k代表buffer的一個byte
                        for(int k = 0; k < 256; k++){  
                            p_buffer[k] = buffer[i*fp_size+j*256+k];
                        }
                        n = write(sockfd,p_buffer,256);
                    }
                }
            }
        }
    }
    
    bzero(f_buffer,256);
    n = read(sockfd,f_buffer,6);
    if (n < 0) 
         error("ERROR reading from socket");
    printf("%s\n",f_buffer);
    close(sockfd);
    return;
}

void udp_s(){
    int sock;
    sock = socket(PF_INET, SOCK_DGRAM, 0);
    if(sock < 0){
        error("socket error");
    }

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(portno);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) <0)
        error("bind error");

    struct sockaddr_in peeraddr;
    socklen_t peerlen;
    int n; //for record error message

    char buffer[256];  //to store data of first packet

    while(1){
        bzero(buffer, 256);
        peerlen = sizeof(peeraddr);
        n = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&peeraddr, &peerlen);
        if(n == -1){
            if(errno == EINTR)
                continue;
            error("recvfrom error");
        } else if (n > 0){
            sendto(sock, buffer, n, 0,
                   (struct sockaddr *)&peeraddr, peerlen);
        }
        break;
    }
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
    outfile = fopen(file_name, "wb");
    
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

    int fp_size = (int)(file_size_i/20.0+0.5); //size of packet is file_size/20 四捨五入
    int p_size; //packet size
    
    //if 5% file >= 256 bytes, after transfer, file will crash. So, let size of packet limited to 256 bytes
    if(fp_size >= 256){
        p_size = 256;
    } else {
        p_size = fp_size; //each packet size = 5% file size
    }
    
    
    char p_buffer[p_size]; //buffer for each packet
    
    int fp_num; //packet number = 20(the last packet will be big)
               //             or 21(the last packet will be small) 
    if(fp_size == (int) file_size_i/20){
        fp_num = 21;
    } else if(fp_size == (int) file_size_i/20 + 1){
        fp_num = 20;
    } else {
        error("Error, packet_size error");
    }

    if(file_size_i % 20 == 0){ //如果20個packet可以平分完file，就只使用20個packet
        fp_num = 20;
    }

    if(p_size == fp_size){    // if 5% file <= 256 bytes (only 20 or 21 packets)
        for(int i = 0; i < fp_num; i++){
            //bzero(p_buffer, p_size);

            if(i == fp_num-1){
                //n = read(newsockfd, p_buffer, file_size_i % p_size);
                while(1){
                    bzero(p_buffer, file_size_i % p_size);
                    peerlen = sizeof(peeraddr);
                    n = recvfrom(sock, p_buffer, file_size_i % p_size, 0, (struct sockaddr *)&peeraddr, &peerlen);
                    if(n == -1){
                        if(errno == EINTR)
                            continue;
                        error("recvfrom error");
                    } else if (n > 0){
                        sendto(sock, p_buffer, n, 0,
                               (struct sockaddr *)&peeraddr, peerlen);
                    }
                    break;
                }

                fwrite(p_buffer, 1, file_size_i % p_size, outfile);
                print_time(100); //print 100%
            } else {
                //n = read(newsockfd, p_buffer, p_size);
                while(1){
                    bzero(p_buffer, p_size);
                    peerlen = sizeof(peeraddr);
                    n = recvfrom(sock, p_buffer, p_size, 0, (struct sockaddr *)&peeraddr, &peerlen);
                    if(n == -1){
                        if(errno == EINTR)
                            continue;
                        error("recvfrom error");
                    } else if (n > 0){
                        sendto(sock, buffer, n, 0,
                               (struct sockaddr *)&peeraddr, peerlen);
                    }
                    break;
                }
                fwrite(p_buffer, 1, p_size, outfile);
                if(i != 19){
                    print_time(5*(i+1));
                }
            }
        }
    } else { //each packet team has more than 1 packet
        int tp_num; // X packets/team. Record packet num of each team (except the last) 
        int tp_num_l; // X packets/team. Record packet number of the last team
        
        //分析每個team有多少個packets
        //無條件進位
        if(fp_size % 256 != 0){
            tp_num = fp_size/256 + 1;
        } else {
            tp_num = fp_size/256;
        }

        //最後一個team的packet 個數
        if(file_size_i % 20 == 0){ //若20個team可以平分file，最後一個team的packet個數就會與其他的一樣
            tp_num_l = tp_num;
        }else{
            if(file_size_i % fp_size % 256 != 0){
                tp_num_l = file_size_i % fp_size /256 + 1;
            } else {
                tp_num_l = file_size_i % fp_size /256;
            }
        }

         //每個i代表1個team
        for(int i = 0; i < fp_num; i++){
            bzero(p_buffer, 256);

            if(i == fp_num -1 && file_size_i % 20 != 0){ //the last team whose packets different than others
                for(int j = 0; j < tp_num_l; j++){
                    if(j == tp_num_l -1){
                        //n = read(newsockfd, p_buffer, file_size_i%fp_size%256);
                        while(1){
                            bzero(p_buffer, file_size_i % fp_size % 256);
                            peerlen = sizeof(peeraddr);
                            n = recvfrom(sock, p_buffer, file_size_i %fp_size %256, 0, (struct sockaddr *)&peeraddr, &peerlen);
                            if(n == -1){
                                if(errno == EINTR)
                                    continue;
                                error("recvfrom error");
                            } else if (n > 0){
                                sendto(sock, buffer, n, 0,
                                       (struct sockaddr *)&peeraddr, peerlen);
                            }
                            break;
                        }
                        fwrite(p_buffer, 1, file_size_i%fp_size%256, outfile);
                        print_time(100);
                     } else {
                        //n = read(newsockfd, p_buffer,256);
                        while(1){
                            bzero(p_buffer, 256);
                            peerlen = sizeof(peeraddr);
                            n = recvfrom(sock, p_buffer, 256, 0, (struct sockaddr *)&peeraddr, &peerlen);
                            if(n == -1){
                                if(errno == EINTR)
                                    continue;
                                error("recvfrom error");
                            } else if (n > 0){
                                sendto(sock, buffer, n, 0,
                                       (struct sockaddr *)&peeraddr, peerlen);
                            }
                            break;
                        }
                        fwrite(p_buffer, 1, 256, outfile);
                     }
                 }
            } else { //the other team
                //每個j代表team中的1個packet
                for(int j = 0; j < tp_num; j++){
                    if(j == tp_num - 1){  //the last packet
                        //n = read(newsockfd, p_buffer, fp_size%256);
                        while(1){
                            bzero(p_buffer, fp_size % 256);
                            peerlen = sizeof(peeraddr);
                            n = recvfrom(sock, p_buffer, fp_size % 256, 0, (struct sockaddr *)&peeraddr, &peerlen);
                            if(n == -1){
                                if(errno == EINTR)
                                    continue;
                                error("recvfrom error");
                            } else if (n > 0){
                                sendto(sock, buffer, n, 0,
                                       (struct sockaddr *)&peeraddr, peerlen);
                            }
                            break;
                        }
                        fwrite(p_buffer, 1, fp_size%256, outfile);
                        if(i != 19){
                            print_time((i+1)*5);
                        }
                    } else{
                        //n = read(newsockfd, p_buffer, 256);
                        while(1){
                            bzero(p_buffer, 256);
                            peerlen = sizeof(peeraddr);
                            n = recvfrom(sock, p_buffer, 256, 0, (struct sockaddr *)&peeraddr, &peerlen);
                            if(n == -1){
                                if(errno == EINTR)
                                    continue;
                                error("recvfrom error");
                            } else if (n > 0){
                                sendto(sock, buffer, n, 0,
                                       (struct sockaddr *)&peeraddr, peerlen);
                            }
                            break;
                        }
                        fwrite(p_buffer, 1, 256, outfile);
                    }
                }
            }
        }

    }

    fclose(outfile);

    sendto(sock, "完成", 6, 0,
           (struct sockaddr *)&peeraddr, peerlen);

    close(sock);
}

void udp_c(){
    int sock;
    sock = socket(PF_INET, SOCK_DGRAM, 0);
    if(sock < 0){
        error("socket error");
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    
    //pass a packet which store file name, and file size 
    char f_buffer[256]; //first buffer
    char r_buffer[256]; //receive buffer

    bzero(f_buffer, 256);
    strcat(f_buffer, file_name); //store file name in first line
    strcat(f_buffer, "\n");   

    char file_size_c[256];  //store file_size in character
    sprintf(file_size_c, "%lld", f_state.st_size);  //change file size from int to char
    strcat(f_buffer, file_size_c); //store file size in second line
    strcat(f_buffer, "\n");

    bzero(r_buffer, 256);
    int ret; // return value by recvfrom()
    while(strcmp(f_buffer, r_buffer) != 0){
        sendto(sock, f_buffer, sizeof(f_buffer), 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
        ret = recvfrom(sock, r_buffer, sizeof(f_buffer), 0, NULL, NULL);
        if(ret == -1){
            if(errno == EINTR)
                continue;
            error("ERROR, recvfrom() error");
        }
    }



    //calculate size of packets
    // 一個packet team 的大小 = 5%的file大小四捨五入
    int fp_size = f_state.st_size/20.0 + 0.5; 
    int p_size; //packet size;
    
    // if 5% file >= 256 bytes, after transfer, file will crash.
    // So, let size of packet limited to 256 bytes
    if(fp_size >= 256){  
        p_size = 256;
    } else {
        p_size = fp_size; //each packet size = 5% file size
    }

    char p_buffer[p_size]; //buffer for each packet

    
    // 5% packet number = 20(the last packet will be big)
    //                 or 21(the last packet will be small) 
    // in order to make each packet team equal to 5% file
    // a packet team consist of 
    //     many 256B_packets & (one <256B_packet or none)
    // no team:
    //   681 = 20*34 + 1*1      -> 21 packets
    //   699 = 19*35 + 1*34     -> 20 packets
    // team:
    //   20519 = 19*(4*256+1*2)+1*(4*256+1*1) -> 20 packet teams -> 100 packets

    int fp_num;
    if(fp_size == f_state.st_size/20){
        fp_num = 21;
    } else if(fp_size == f_state.st_size/20 + 1){
        fp_num = 20;
    }
    if(f_state.st_size % 20 == 0){ //如果20個packet可以平分完file，就只使用20個packet
        fp_num = 20;
    }

    //read file
    char buffer[f_state.st_size+1]; //make buffer equal to size of file
    bzero(buffer, f_state.st_size+1); //clean buffer
    fread(buffer, f_state.st_size, 1, file); //read file to buffer
    
    if(p_size == fp_size){    // if 5% file <= 256 bytes (only 20 or 21 packets)
        for(int i = 0; i < fp_num ; i++){
            bzero(p_buffer, p_size);  
            bzero(r_buffer, 256);
            if(i == fp_num-1){ //the last packet
                //若20個packet可以平分file，最後一個packet還是使用 p_size
                if(fp_size == f_state.st_size/20){ 
                    for(int j = 0; j < p_size; j++){  
                        p_buffer[j] = buffer[i*p_size+j];
                    }
                    while(strcmp(p_buffer, r_buffer) != 0){
                        sendto(sock, p_buffer, p_size, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
                        ret = recvfrom(sock, r_buffer, p_size, 0, NULL, NULL);
                        if(ret == -1){
                            if(errno == EINTR)
                                continue;
                            error("ERROR, recvfrom() error");
                        }
                    }
                    break;
                } 
                //最後一個packet 不會是滿的，所以j只到f_state.st_size%p_size
                for(int j = 0; j < f_state.st_size%p_size; j++){
                    p_buffer[j] = buffer[i*p_size+j];
                }
                while(strcmp(p_buffer, r_buffer) != 0){
                    sendto(sock, p_buffer, f_state.st_size%p_size, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
                    ret = recvfrom(sock, r_buffer, f_state.st_size%p_size, 0, NULL, NULL);
                    if(ret == -1){
                        if(errno == EINTR)
                            continue;
                        error("ERROR, recvfrom() error");
                    }
                }
            } else { 
                //spilt file into 19 or 20 packets and transfer them
                for(int j = 0; j < p_size; j++){  
                    p_buffer[j] = buffer[i*p_size+j];
                }
                while(strcmp(p_buffer, r_buffer) != 0){
                    sendto(sock, p_buffer, p_size, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
                    ret = recvfrom(sock, r_buffer, p_size, 0, NULL, NULL);
                    if(ret == -1){
                        if(errno == EINTR)
                            continue;
                        error("ERROR, recvfrom() error");
                    }
                }
            }

        }
    } else {
        //each packet team has more than 1 packet
        int tp_num; // X packets/team. Record packet num of each team (except the last)
        int tp_num_l; // X packets/team. Record packet number of the last team

        //分析每個team有多少個packets
        //無條件進位
        if(fp_size % 256 != 0){
            tp_num = fp_size/256 + 1;
        } else {
            tp_num = fp_size/256;
        }

        //最後一個team的packet 個數
        if(f_state.st_size % 20 == 0){
            //若20個team可以平分file，最後一個team的packet個數就會與其他的一樣
            tp_num_l = tp_num;
        }else{
            if(f_state.st_size % fp_size % 256 != 0){
                tp_num_l = f_state.st_size % fp_size /256 + 1;
            } else {
                tp_num_l = f_state.st_size % fp_size /256;
            }
        }

        //每個i代表1個team
        for(int i = 0; i < fp_num; i++){
            if(i == fp_num -1 && f_state.st_size % 20 != 0){
                //the last team whose packets different than others
                for(int j = 0; j < tp_num_l; j++){
                    bzero(p_buffer, 256);
                    bzero(r_buffer, 256);
                    if(j == tp_num_l -1){
                        for(int k = 0; k < f_state.st_size%fp_size%256; k++){
                            p_buffer[k] = buffer[i*fp_size+j*256+k];
                        }
                        while(strcmp(p_buffer, r_buffer) != 0){
                            sendto(sock, p_buffer, f_state.st_size%fp_size%256, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
                            ret = recvfrom(sock, r_buffer, f_state.st_size%fp_size%256, 0, NULL, NULL);
                            if(ret == -1){
                                if(errno == EINTR)
                                    continue;
                                error("ERROR, recvfrom() error");
                            }
                        }

                     } else {
                        for(int k = 0; k < 256; k++){
                            p_buffer[k] = buffer[i*fp_size+j*256+k];
                        }
                        while(strcmp(p_buffer, r_buffer) != 0){
                            sendto(sock, p_buffer, 256, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
                            ret = recvfrom(sock, r_buffer, 256, 0, NULL, NULL);
                            if(ret == -1){
                                if(errno == EINTR)
                                    continue;
                                error("ERROR, recvfrom() error");
                            }
                        }
                        
                     }
                 }
            } else { //the other team
                //每個j代表team中的1個packet
                for(int j = 0; j < tp_num; j++){
                    bzero(p_buffer, 256);
                    bzero(r_buffer, 256);
                    if(j == tp_num - 1){  //the last packet
                        for(int k = 0; k < fp_size%256;k++){
                            p_buffer[k] = buffer[i*fp_size + j*256 + k];
                        }
                        while(strcmp(p_buffer, r_buffer) != 0){
                            sendto(sock, p_buffer, fp_size%256, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
                            ret = recvfrom(sock, r_buffer, fp_size%256, 0, NULL, NULL);
                            if(ret == -1){
                                if(errno == EINTR)
                                    continue;
                                error("ERROR, recvfrom() error");
                            }
                        }
                        //n = write(sockfd, p_buffer, fp_size%256);
                    } else{
                        //每個k代表buffer的一個byte
                        for(int k = 0; k < 256; k++){
                            p_buffer[k] = buffer[i*fp_size+j*256+k];
                        }
                        while(strcmp(p_buffer, r_buffer) != 0){
                            sendto(sock, p_buffer, 256, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
                            ret = recvfrom(sock, r_buffer, 256, 0, NULL, NULL);
                            if(ret == -1){
                                if(errno == EINTR)
                                    continue;
                                error("ERROR, recvfrom() error");
                            }
                        }
                    }
                }
            }
        }

    }
    
    bzero(f_buffer, 256);
    while(1){
        bzero(f_buffer, 256);
        ret = recvfrom(sock, f_buffer, 256, 0, NULL, NULL);
        if(ret == -1){
            if(errno == EINTR)
                continue;
            error("recvfrom error");
        } else if (ret > 0){
            sendto(sock, f_buffer, 256, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
        }
        break;
    }
    printf("%s\n", f_buffer);
    close(sock);

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
        stat(argv[5], &f_state); // store state of file, argv[5], in f_state
        bzero(file_name, 256);
        strcat(file_name, argv[5]); // store file name in global variable

    }


    server = gethostbyname(argv[3]); //turn ip to host name and store in global variable "server" 
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }

    portno = atoi(argv[4]); //store port num(argv[4]) in portno
    

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
