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

uint32_t getPrintCount(int sockfd) {
    int bytes_received = 0;
    uint32_t num_prints_int32;

    while (1) {
        bytes_received = read(sockfd,
                              &num_prints_int32,
                              sizeof(uint32_t));
        if (bytes_received <= 0)
            break;
    }
    num_prints_int32 = ntohl(num_prints_int32);
    return num_prints_int32;

}

void sendFileLength(uint32_t file_len, int sockfd) {
    uint32_t N = htonl(file_len);
    printf("sending %lu\n", N);
    int not_written = sizeof(uint32_t);
    int bytes_sent = 0;
    while (not_written > 0) {
        bytes_sent = write(sockfd, &N, not_written);
        assert(bytes_sent >= 0);
        printf("Client wrote %d Bytes\n", bytes_sent);
        not_written -= bytes_sent;
    }
}

void sendBytes(uint32_t file_len, int sockfd, char *buffer) {
    long not_written = (long) file_len;
    int bytes_sent = 0;
    while (not_written > 0) {
        bytes_sent = write(sockfd, buffer, not_written);
        assert(bytes_sent >= 0);
        printf("Client wrote %d Bytes\n", bytes_sent);
        not_written -= bytes_sent;
    }
}

uint32_t readFile(FILE *fileptr, char *buffer) {
    uint32_t byte_count;
    long file_len;
    fseek(fileptr, 0, SEEK_END);
    file_len = ftell(fileptr);
    rewind(fileptr);
    buffer = (char *) malloc(file_len * sizeof(char));
    fread(buffer, file_len, 1, fileptr);
    fclose(fileptr);
    return (uint32_t) file_len;

}

int main(int argc, char *argv[]) {
    int sockfd = -1;
    int res = -1;
    char *buffer;
    uint32_t num_prints;
    uint32_t file_len;
    FILE *fileptr;
    char *server_IP;
    char *server_port;
    char *file_path;
    struct sockaddr_in serv_addr; // where we Want to get to
    struct sockaddr_in my_addr;   // where we actually connected through
    struct sockaddr_in peer_addr; // where we actually connected to
    socklen_t addrsize = sizeof(struct sockaddr_in);

    if (argc != 4) {
        fprintf(stderr, "Error, Arguments, number %s", strerror((errno)));
        exit(1);
    }
    server_IP = argv[1];
    server_port = argv[2];
    file_path = argv[3];
    fileptr = fopen(file_path, "rb");
    if (fileptr == NULL) {
        printf("\n Error : File to be read not found\n");
        return 1;
    }
    file_len = readFile(fileptr, buffer);
    printf("read from file %lu bytes\n", file_len);

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Error : Could not create socket \n");
        return 1;
    }
    getsockname(sockfd, (struct sockaddr *) &my_addr, &addrsize);
    printf("Client: socket created %s:%d\n", inet_ntoa((my_addr.sin_addr)), ntohs(my_addr.sin_port));
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(server_port));
    serv_addr.sin_addr.s_addr = inet_addr(server_IP);

    printf("Client: connecting...\n");

    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        printf("\n Error : Connect Failed. %s \n", strerror(errno));
        return 1;
    }

    getsockname(sockfd, (struct sockaddr *) &my_addr, &addrsize);
    getpeername(sockfd, (struct sockaddr *) &peer_addr, &addrsize);


    printf("Client: Connected. \n""\t\tSource IP: %s Source Port: %d\n", inet_ntoa((my_addr.sin_addr)),
           ntohs(my_addr.sin_port));
    printf("\t\tTarget IP: %s Target Port: %d\n", inet_ntoa((peer_addr.sin_addr)), ntohs(peer_addr.sin_port));
    sendFileLength(file_len, sockfd);
    printf("client waiting for server\n");
    sendBytes(file_len, sockfd, buffer);
    num_prints = getPrintCount(sockfd);
    printf("# of printable characters: %u\n", num_prints);
    close(sockfd);
    return 0;
}
