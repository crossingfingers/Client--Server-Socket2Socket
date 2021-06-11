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


/*global variables to keep printable character count*/
uint32_t pcc_total[127] = {0};
/*shutdown signal to shutdown server after next client tcp connection*/
int shutdown_sig = 0;
/*connection file descriptor*/
int connfd = -1;

/**
 * Shutdown method, prints printable file count
 */
void shutdownServer() {
    int i;
    for (i = 32; i < 127; i++) {
        printf("char '%c' : %u times\n", i, pcc_total[i]);
    }
    exit(0);
}

/**
 * handler for SIGUSR1 signal
 * either shuts down server, or raises shutdown flag (shutdown after tcp connection end)
 */
void SIGUSR1_handler() {
    if (connfd < 0) { shutdownServer(); }
    else { shutdown_sig = 1; }
}

/**
 * receives file byte size method
 * @return size of file to be received (N)
 */
uint32_t receiveFileByteSize() {
    uint32_t N;
    int bytes_received = 0;
    int bytes_written = 0;
    while (bytes_written < sizeof(N)) {
        bytes_received = read(connfd, &N + bytes_written, sizeof(N) - bytes_written);
        if (bytes_received < 0) {
            if ((errno != ETIMEDOUT) && (errno != ECONNRESET) && (errno != EPIPE)) {
                fprintf(stderr, "Error receiving from client bytes: %s\n", strerror(errno));
                exit(1);
            } else {
                fprintf(stderr, "TCP ERROR while receiving from client: %s\n", strerror(errno));
                return -1;
            }
        }
        if (bytes_received == 0) { break; }
        bytes_written += bytes_received;
    }
    if (bytes_written != sizeof(uint32_t)) {
        fprintf(stderr, "client killed unexpectedly: %s\n", strerror(errno));
        return -1;
    }
    N = ntohl(N);
    return N;
}

/**
 * receives file bytes from client
 * @param buffer - to save data to
 * @param N  - number of bytes
 * @return  - bytes written or -1 if critical error
 */
int receiveFileBytes(char *buffer, uint32_t N) {
    int bytes_received = 0;
    int bytes_written = 0;

    while (bytes_written < N) {
        bytes_received = read(connfd, buffer + bytes_written, N - bytes_written);
        if (bytes_received < 0) {
            if ((errno != ETIMEDOUT) && (errno != ECONNRESET) && (errno != EPIPE)) {
                fprintf(stderr, "Error receiving from client bytes: %s\n", strerror(errno));
                exit(1);
            } else {
                fprintf(stderr, "TCP ERROR while receiving from client: %s\n", strerror(errno));
                return -1;
            }
        }
        if (bytes_received == 0) { break; }
        bytes_written += bytes_received;
    }
    if (bytes_written != N) {
        fprintf(stderr, "client killed unexpectedly: %s\n", strerror(errno));
        return -1;
    }
    return bytes_written;
}

/**
 * sends to client printable character count
 * @param num_prints - number of printable characters
 * @return
 */
int sendPrintedCharCount(uint32_t num_prints) {
    uint32_t num_prints_for_net = htonl(num_prints);
    int bytes_written = 0;
    int bytes_sent = 0;
    while (bytes_written < sizeof(uint32_t)) {
        bytes_sent = write(connfd, &num_prints_for_net + bytes_written, 4 - bytes_written);
        if (bytes_sent < 0) {
            if ((errno != ETIMEDOUT) && (errno != ECONNRESET) && (errno != EPIPE)) {
                fprintf(stderr, "Error sending to client bytes: %s\n", strerror(errno));
                exit(1);
            } else {
                fprintf(stderr, "TCP ERROR while sending to client: %s\n", strerror(errno));
                return -1;
            }
        }
        if (bytes_sent == 0) { break; }
        bytes_written += bytes_sent;
    }
    if (bytes_written != sizeof(uint32_t)) {
        fprintf(stderr, "client killed unexpectedly: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}


/**
 * updates PCC structure only if full tcp connection has finished correctly
 * @param buffer - contains received chars
 * @param N  - number of bytes
 */
void updatePCC(char *buffer, uint32_t N) {
    int i;
    char c;
    for (i = 0; i < N; i++) {
        c = buffer[i];
        if ((32 <= c) && (c <= 126)) {
            pcc_total[(uint32_t) c]++;
        }
    }
}

/**
 * counts number of printable characters
 * @param buffer  - contains received bytes
 * @param N  - number of bytes
 * @return  - number of printable characters
 */
uint32_t countPrintable(char *buffer, uint32_t N) {
    int i;
    char c;
    uint32_t counter = 0;
    for (i = 0; i < N; i++) {
        c = buffer[i];
        if ((32 <= c) && (c <= 126)) {
            counter++;
        }
    }
    return counter;
}


int main(int argc, char *argv[]) {
    int listenfd = -1;
    char *buffer;
    char *server_port;
    int bytes_received = 0;
    int res;
    uint32_t N;
    uint32_t num_prints_int;
    struct sockaddr_in serv_addr;
    struct sockaddr_in peer_addr;
    struct sigaction sigusr1;
    socklen_t addrsize = sizeof(struct sockaddr_in);

    /*reads input*/
    if (argc != 2) {
        fprintf(stderr, "Error, Arguments, number %s\n", strerror((errno)));
        exit(1);
    } else { server_port = argv[1]; }


    /*set SIGUSR1 */
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
    res = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int));
    if (res < 0) {
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

        /*receive file byte size, if encountered error (N=-1), finish connection & continue*/
        N = receiveFileByteSize();
        if (N < 0) {
            close(connfd);
            connfd = -1;
            continue;
        }

        /*allocate buffer size, (N bytes)*/
        buffer = (char *) malloc(N * sizeof(char));
        if (buffer == NULL) {
            fprintf(stderr, "Malloc Failed, Error %s\n", strerror(errno));
            exit(1);
        }

        /*receives bytes from client, if byte count equals -1, error occurred, finish connection & continue*/
        bytes_received = receiveFileBytes(buffer, N);
        if (bytes_received < 0) {
            close(connfd);
            connfd = -1;
            free(buffer);
            continue;
        }

        /*count number of printable characters in buffer*/
        num_prints_int = countPrintable(buffer, N);

        /*send to client PCC, if res =-1, error occurred, finish connection & continue*/
        res = sendPrintedCharCount(num_prints_int);
        if (res < 0) {
            close(connfd);
            connfd = -1;
            free(buffer);
            continue;
        }

        /*if all is OK, update PCC structure, free buffer and continue*/
        updatePCC(buffer, N);
        free(buffer);
        close(connfd);
        connfd = -1;
    }
    exit(0);
}
