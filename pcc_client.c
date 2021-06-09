#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <errno.h>
#include <assert.h>
#include <stdint.h>


int main(int argc, char *argv[]) {
    int sockfd = -1;
    int bytes_received = 0;
    int bytes_sent = -1;
    char recv_buff[1024];
    int res = -1;
    char *buffer;
    unsigned char N;
    char num_prints_bytes[4];
    uint32_t num_prints_int;
    uint32_t byte_count = 0;
    long file_len;
    FILE *fileptr;
    char *server_IP = argv[1];
    char *server_port = argv[2];
    char *file_path = argv[3];
    int notwritten;

    struct sockaddr_in serv_addr; // where we Want to get to
    struct sockaddr_in my_addr;   // where we actually connected through
    struct sockaddr_in peer_addr; // where we actually connected to
    socklen_t addrsize = sizeof(struct sockaddr_in);

    if (argc != 4) {
        fprintf(stderr, "Error, Arguments, number %s", strerror((errno)));
        exit(1);
    }
    fileptr = fopen(file_path, "rb");
    if (fileptr == NULL) {
        printf("\n Error : File to be read not found\n");
        return 1;
    }
    fseek(fileptr, 0, SEEK_END);
    file_len = ftell(fileptr);
    rewind(fileptr);
    buffer = (char *) malloc(file_len * sizeof(char));
    byte_count = fread(buffer, file_len, 1, fileptr);
    fclose(fileptr);


    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Error : Could not create socket \n");
        return 1;
    }
    getsockname(sockfd,
                (struct sockaddr *) &my_addr,
                &addrsize);
    printf("Client: socket created %s:%d\n",
           inet_ntoa((my_addr.sin_addr)),
           ntohs(my_addr.sin_port));

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(server_port)); // Note: htons for endiannes
    serv_addr.sin_addr.s_addr = inet_addr(server_IP); // hardcoded...

    printf("Client: connecting...\n");
    // Note: what about the client port number?
    // connect socket to the target address
    if (connect(sockfd,
                (struct sockaddr *) &serv_addr,
                sizeof(serv_addr)) < 0) {
        printf("\n Error : Connect Failed. %s \n", strerror(errno));
        return 1;
    }

    // print socket details again
    getsockname(sockfd, (struct sockaddr *) &my_addr, &addrsize);
    getpeername(sockfd, (struct sockaddr *) &peer_addr, &addrsize);


    printf("Client: Connected. \n"
           "\t\tSource IP: %s Source Port: %d\n"
           "\t\tTarget IP: %s Target Port: %d\n",
           inet_ntoa((my_addr.sin_addr)), ntohs(my_addr.sin_port),
           inet_ntoa((peer_addr.sin_addr)), ntohs(peer_addr.sin_port));
    N =  htonl(num_prints_int);

    notwritten = sizeof(uint32_t);
    while (notwritten > 0) {
        bytes_sent = write(sockfd, &N, notwritten);
        assert(bytes_sent >= 0);
        printf("Client wrote %d Bytes\n", bytes_sent);
        notwritten -= bytes_sent;
    }

    notwritten = strlen(buffer);

    while (notwritten > 0) {
        bytes_sent = write(sockfd, buffer, notwritten);
        assert(bytes_sent >= 0);
        printf("Client wrote %d Bytes\n", bytes_sent);
        notwritten -= bytes_sent;
    }
    while (1) {
        bytes_received = read(sockfd,
                              &num_prints_int,
                              sizeof(uint32_t));
        if (bytes_received <= 0)
            break;
        recv_buff[bytes_received] = '\0';
    }
    num_prints_int = ntohs(num_prints_int);

    close(sockfd); // is socket really done here?
    printf("printed %d characters", num_prints_int);
    printf("Write after close returned \n");
    return 0;
}
