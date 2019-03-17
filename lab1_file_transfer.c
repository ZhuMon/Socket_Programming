#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

}
