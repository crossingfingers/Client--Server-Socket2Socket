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

//TODO : REMOVE  all prints

/**
 * Methods recieves printable character count from server, returns int value
 * @param sockfd - socket file descriptor
 * @return
 */
uint32_t getPrintCount(int sockfd) {
    int bytes_received = 0;
    int bytes_written=0;
    uint32_t num_prints_int32;
    while (bytes_written<sizeof(num_prints_int32)) {
        bytes_received = read(sockfd,
                              &num_prints_int32+bytes_written,
                              sizeof(uint32_t)-bytes_written);
        if(bytes_received < 0){
            fprintf(stderr, "Error receiving from server bytes: %s\n", strerror(errno));
            exit(1);
        }
        bytes_written+=bytes_received;
    }
    num_prints_int32 = ntohl(num_prints_int32);
    return num_prints_int32;

}

/**
 * Method sends file length (number of bytes) to server
 * @param file_len - in bytes
 * @param sockfd  - socket file descriptor
 */
void sendFileLength(uint32_t file_len, int sockfd) {
    uint32_t N = htonl(file_len);
    printf("sending %u\n", N); //TODO: REMOVE
    int bytes_written = 0;
    int bytes_sent = 0;
    while (bytes_written < sizeof(uint32_t)) {
        bytes_sent = write(sockfd, &N + bytes_written, 4 - bytes_written);
        if(bytes_sent < 0){
            fprintf(stderr, "Error sending to server bytes: %s\n", strerror(errno));
            exit(1);
        }
        bytes_written += bytes_sent;
    }
}

/**
 * sends bytes read from file to server
 * @param file_len - number of bytes read from file
 * @param sockfd  - socket file descriptor
 * @param buffer pointer to buffer (holds file bytes)
 */
void sendBytes(uint32_t file_len, int sockfd, char *buffer) {
    int bytes_sent = 0;
    int bytes_written = 0;
    while (bytes_written < file_len) {
        bytes_sent = write(sockfd, buffer + bytes_written, file_len - bytes_written);
        if(bytes_sent < 0){
            fprintf(stderr, "Error sending to server bytes: %s\n", strerror(errno));
            exit(1);
        }
        bytes_written += bytes_sent;
    }
    free(buffer);
}
/**
 * opens & reads file data
 * @param fileptr - pointer to file to be opened
 * @param buffer - buffer to save file bytes read
 * @return - number of bytes read
 */
uint32_t readFile(FILE *fileptr, char *buffer) {
    long file_len;
    unsigned long res;
    fseek(fileptr, 0, SEEK_END);
    file_len = ftell(fileptr);
    rewind(fileptr);
    free(buffer);
    buffer = (char *) malloc(file_len * sizeof(char));
    if (buffer == NULL) {
        fprintf(stderr, "Malloc Failed, Error %s\n", strerror(errno));
        exit(1);
    }
    res = fread(buffer, 1, file_len, fileptr);
    if (res != file_len) {
        fprintf(stderr, "Error reading %lu bytes from file, %s\n", file_len, strerror(errno));
        exit(1);
    }
    fclose(fileptr);
    return (uint32_t) file_len;

}

int main(int argc, char *argv[]) {
    int sockfd = -1;
    int res = -1;
    char *buffer=(char *) malloc(sizeof(char));
    uint32_t num_prints;
    uint32_t file_len;
    FILE *fileptr;
    char *server_IP;
    char *server_port;
    char *file_path;
    struct sockaddr_in serv_addr;
    struct sockaddr_in my_addr;

    struct sockaddr_in peer_addr; //TODO : REMOVE

    socklen_t addrsize = sizeof(struct sockaddr_in);

    if (argc != 4) {
        fprintf(stderr, "Error, Arguments, number %s", strerror((errno)));
        exit(1);
    } else {
        server_IP = argv[1];
        server_port = argv[2];
        file_path = argv[3];
    }

    /*reads file bytes*/
    fileptr = fopen(file_path, "rb");
    if (fileptr == NULL) {
        fprintf(stderr, "Error %s: File %s  cannot be opened or not found\n", strerror(errno), file_path);
        exit(1);
    }
    file_len = readFile(fileptr, buffer);
    printf("read from file %u bytes\n", file_len); //TODO : REMOVE

    /*creates socket*/
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "\n Error : Could not create socket: %s \n", strerror(errno));
        return 1;
    }

    getsockname(sockfd, (struct sockaddr *) &my_addr, &addrsize);//TODO : REMOVE
    printf("Client: socket created %s:%d\n", inet_ntoa((my_addr.sin_addr)), ntohs(my_addr.sin_port));//TODO : REMOVE

    /*defines socket configurations*/
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(server_port));
    res = inet_pton(AF_INET, server_IP, &serv_addr.sin_addr.s_addr);
    if (res <= 0) {
        if (res == 0)
            fprintf(stderr, "IP not in correct format");
        else
            fprintf(stderr, "error in inet_pton : %s", strerror((errno)));
        exit(1);
    }


    printf("Client: connecting...\n"); //TODO : REMOVE

    /*connects to server*/
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr, "\n Error : Connect Failed. %s \n", strerror(errno));
        exit(1);
    }

    getsockname(sockfd, (struct sockaddr *) &my_addr, &addrsize); //TODO : REMOVE
    getpeername(sockfd, (struct sockaddr *) &peer_addr, &addrsize);//TODO : REMOVE
    printf("Client: Connected. \n""\t\tSource IP: %s Source Port: %d\n", inet_ntoa((my_addr.sin_addr)),ntohs(my_addr.sin_port));//TODO : REMOVE
    printf("\t\tTarget IP: %s Target Port: %d\n", inet_ntoa((peer_addr.sin_addr)), ntohs(peer_addr.sin_port));//TODO : REMOVE

    /*sends file length N to server*/
    sendFileLength(file_len, sockfd);

    printf("client sent file length to server\n"); //TODO : REMOVE

    /*sends N bytes read from file to server*/
    sendBytes(file_len, sockfd, buffer);

    printf("client sent bytes to server\n"); //TODO : REMOVE

    /*receives number of printable bytes from server*/
    num_prints = getPrintCount(sockfd);

    printf("# of printable characters: %u\n", num_prints);
    close(sockfd);
    exit(0);
}
