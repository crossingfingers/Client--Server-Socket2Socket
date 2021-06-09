//
// Created by Ofek on 6/9/2021.
//

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <assert.h>
#include <stdint.h>

uint32_t pcc_total[126];

// MINIMAL ERROR HANDLING FOR EASE OF READING
void sendPrintedCharCount(int connfd, uint32_t num_prints) {
    int not_written = sizeof(uint32_t);
    int bytes_sent = 0;
    uint32_t num_prints_for_net = htonl(num_prints);

    while (not_written > 0) {
        bytes_sent = write(connfd, &num_prints_for_net, not_written);
        assert(bytes_sent >= 0);
        printf("server wrote %d Bytes\n", bytes_sent);
        not_written -= bytes_sent;
    }
    not_written -= bytes_sent;
}

uint32_t printAndCount(char *buffer, uint32_t N) {
    int i;
    char c;
    uint32_t counter = 0;
    printf("printing bytes: ");
    for (i = 0; i < N; i++) {
        c = buffer[i];
        if ((32 <= c) && (c <= 126)) {
            pcc_total[c]++;
            printf("%c", c);
            counter++;
        }
    }

    printf("\n");
    return counter;
}


int main(int argc, char *argv[]) {
    int listenfd = -1;
    int connfd = -1;
    char *buffer;
    char *server_port;
    int bytes_received = 0;
    uint32_t N;
    uint32_t num_prints_int;
    struct sockaddr_in serv_addr;
    struct sockaddr_in my_addr;
    struct sockaddr_in peer_addr;
    socklen_t addrsize = sizeof(struct sockaddr_in);
    if (argc != 2) {
        fprintf(stderr, "Error, Arguments, number %s", strerror((errno)));
        exit(1);
    }
    server_port = argv[1];

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, 0, addrsize);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(server_port));

    if (0 != bind(listenfd, (struct sockaddr *) &serv_addr, addrsize)) {
        printf("\n Error : Bind Failed. %s \n", strerror(errno));
        return 1;
    }

    if (0 != listen(listenfd, 10)) {
        printf("\n Error : Listen Failed. %s \n", strerror(errno));
        return 1;
    }

    while (1) {
        connfd = accept(listenfd, (struct sockaddr *) &peer_addr, &addrsize);
        if (connfd < 0) {
            printf("\n Error : Accept Failed. %s \n", strerror(errno));
            return 1;
        }

        getsockname(connfd, (struct sockaddr *) &my_addr, &addrsize);
        getpeername(connfd, (struct sockaddr *) &peer_addr, &addrsize);
        printf("Server: Client connected.\n""\t\tClient IP: %s Client Port: %d\n", inet_ntoa(peer_addr.sin_addr),
               ntohs(peer_addr.sin_port));
        printf("\t\tServer IP: %s Server Port: %d\n", inet_ntoa(my_addr.sin_addr), ntohs(my_addr.sin_port));

        while (1) {
            bytes_received = read(connfd, &N, sizeof(uint32_t));
            if (bytes_received == sizeof(uint32_t))
                break;
        }
        N = ntohl(N);
        buffer = (char *) malloc(N * sizeof(char));
        printf("Server recieved N=%lu\n", N);
        printf("Server waiting for read of %d bytes\n", N);
        bytes_received = 0;
        while (1) {
            bytes_received = read(connfd, buffer, N);
            if (bytes_received == N)
                break;
        }
        printf("Server received %d bytes\n", bytes_received);
        num_prints_int = printAndCount(buffer, N);
        sendPrintedCharCount(connfd, num_prints_int);
        close(connfd);
    }
}
