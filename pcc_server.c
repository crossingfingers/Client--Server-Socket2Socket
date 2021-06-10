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
#include <signal.h>

//TODO: ADD SIGNAL
//TODO : add error checks
//TODO : ADD MEM CHECKS
/*global variable to keep printable character count*/
uint32_t pcc_total[127] = {0};
int shutdown_sig = 0;
int connfd = -1;

void shutdownServer() {
    int i;
    for (i = 32; i < 127; i++) {
        printf("char '%c' : %u times\n", i, pcc_total[i]);
    }
    exit(0);
}

void SIGUSR1_handler() {
    if (connfd < 0) { shutdownServer(); }
    else { shutdown_sig = 1; }
}


uint32_t receiveFileByteSize() {
    uint32_t N;
    int bytes_received = 0;
    int bytes_written = 0;
    while (bytes_written < sizeof(N)) {
        bytes_received = read(connfd, &N + bytes_written, sizeof(N) - bytes_written);
        if (bytes_received < 0) {
            fprintf(stderr, "Error receiving from client bytes: %s\n", strerror(errno));
            exit(1);
        }
        bytes_written += bytes_received;
    }
    N = ntohl(N);
    return N;
}

int receiveFileBytes(unsigned char *buffer, uint32_t N) {
    int bytes_received = 0;
    int bytes_written = 0;

    while (bytes_written < N) {
        bytes_received = read(connfd, buffer + bytes_written, N - bytes_written);
        if (bytes_received < 0) {
            fprintf(stderr, "Error receiving from client bytes: %s\n", strerror(errno));
            exit(1);
        }
        bytes_written += bytes_received;
    }
    return bytes_written;

}

void sendPrintedCharCount(uint32_t num_prints) {
    uint32_t num_prints_for_net = htonl(num_prints);
    int bytes_written = 0;
    int bytes_sent = 0;
    printf("sending %u\n", num_prints_for_net);
    while (bytes_written < sizeof(uint32_t)) {
        bytes_sent = write(connfd, &num_prints_for_net + bytes_written, 4 - bytes_written);
        assert(bytes_sent >= 0);
        printf("Server wrote %d Bytes\n", bytes_sent);
        bytes_written += bytes_sent;
    }
}

void updatePCC(unsigned char *buffer, uint32_t N) {
    int i;
    char c;
    printf("printing bytes: ");
    for (i = 0; i < N; i++) {
        c = buffer[i];
        if ((32 <= c) && (c <= 126)) {
            pcc_total[(uint32_t)c]++;
            printf("%c", c);
        }
    }
    printf("\n");
}


uint32_t countPrintable(unsigned char *buffer, uint32_t N) {
    int i;
    char c;
    uint32_t counter = 0;
    printf("printing bytes: ");
    for (i = 0; i < N; i++) {
        c = buffer[i];
        if ((32 <= c) && (c <= 126)) {
            printf("%c", c);
            counter++;
        }
    }
    printf("\n");
    return counter;
}


int main(int argc, char *argv[]) {
    int listenfd = -1;
    unsigned char *buffer;
    char *server_port;
    int bytes_received = 0;
    int res;
    uint32_t N;
    uint32_t num_prints_int;
    struct sockaddr_in serv_addr;
    struct sockaddr_in my_addr; //TODO: REMOVE
    struct sockaddr_in peer_addr; //TODO : REMOVE
    struct sigaction sigusr1;
    socklen_t addrsize = sizeof(struct sockaddr_in);

    /*read inputs*/
    if (argc != 2) {
        fprintf(stderr, "Error, Arguments, number %s", strerror((errno)));
        exit(1);
    } else { server_port = argv[1]; }


    /*set SIGUSR1 */ //TODO : CHECK
    memset(&sigusr1, 0, sizeof(sigusr1));
    sigusr1.sa_handler = &SIGUSR1_handler;
    sigemptyset(&sigusr1.sa_mask);
    sigusr1.sa_flags = SA_RESTART;
    if (sigaction(SIGUSR1, &sigusr1, NULL) != 0) {
        fprintf(stderr, "SIGUSR1 signal handle registration failed, %s\n", strerror(errno));
        exit(1);
    }

    /*set server to listen on socket*/
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < -1) {
        fprintf(stderr, "error listening to socket: %s\n", strerror(errno));
        exit(1);
    }

    /*set reuse address*/
    res = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
    if( res< 0){
        fprintf(stderr, "Error in setsockopt, %s\n", strerror(errno));
        exit(1);
    }

    /*configure server connection*/
    memset(&serv_addr, 0, addrsize);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(server_port));
    if (0 != bind(listenfd, (struct sockaddr *) &serv_addr, addrsize)) {
        fprintf(stderr, "\n Error : Bind Failed. %s \n", strerror(errno));
        exit(1);
    }
    if (0 != listen(listenfd, 10)) {
        fprintf(stderr, "\n Error : Listen Failed. %s \n", strerror(errno));
        exit(1);
    }

    /*start receiving connections*/
    while (1) {

        /*if received server shutdown signal (SIGUSR1) then shutdown*/
        if (shutdown_sig == 1) { shutdownServer(); }

        /*receive connection*/
        connfd = accept(listenfd, (struct sockaddr *) &peer_addr, &addrsize);
        if (connfd < 0) {
            fprintf(stderr, "\n Error : Accept Failed. %s \n", strerror(errno));
            return 1;
        }

        getsockname(connfd, (struct sockaddr *) &my_addr, &addrsize); //TODO: REMOVE
        getpeername(connfd, (struct sockaddr *) &peer_addr, &addrsize); //TODO: REMOVE
        printf("Server: Client connected.\n""\t\tClient IP: %s Client Port: %d\n", inet_ntoa(peer_addr.sin_addr),
               ntohs(peer_addr.sin_port)); //TODO: REMOVE
        printf("\t\tServer IP: %s Server Port: %d\n", inet_ntoa(my_addr.sin_addr),
               ntohs(my_addr.sin_port));//TODO: REMOVE

        N = receiveFileByteSize(); //TODO : ADD ERROR CHECK
        buffer = (unsigned char *) malloc(N * sizeof(char));

        printf("Server recieved N=%u\n", N); //TODO: REMOVE
        printf("Server waiting for read of %d bytes\n", N);//TODO: REMOVE

        bytes_received = receiveFileBytes(buffer, N); //TODO : ADD ERROR CHECK

        printf("Server received %d bytes\n", bytes_received); //TODO: REMOVE
        num_prints_int = countPrintable(buffer, N);  //TODO : ADD ERROR CHECK
        sendPrintedCharCount(num_prints_int); //TODO : ADD ERROR CHECK

        updatePCC(buffer, N);
        free(buffer);
        close(connfd);
        connfd=-1;
    }
    exit(0);
}
