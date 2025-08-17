#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>

/* ===================== PROTOCOL DEFINITION ===================== */
#pragma pack(push, 1)  // Disable struct padding for network compatibility
typedef enum {
    PROTO_HELLO = 0,    // Message type 0: Handshake
    PROTO_DATA = 1,     // Message type 1: Data transfer (example extension)
} proto_type_e;

typedef struct {
    proto_type_e type;  // 4-byte message identifier
    unsigned short len; // 2-byte payload length
} proto_hdr_t;
#pragma pack(pop)       // Restore default struct alignment
/* =============================================================== */

/**
 * Handles a connected client by sending protocol data
 * @param fd Client socket file descriptor
 */
void handle_client(int fd) {
    char buf[4096] = {0};  // Initialize send buffer (zeroed)
    
    /* ----- 1. Prepare Protocol Header ----- */
    proto_hdr_t *hdr = (proto_hdr_t *)buf;
    hdr->type = htonl(PROTO_HELLO);  // Convert to network byte order
    hdr->len = htons(sizeof(int));   // Our payload is 1 integer (4 bytes)

    /* ----- 2. Add Payload Data ----- */
    int *data = (int *)(buf + sizeof(proto_hdr_t));  // Pointer arithmetic
    *data = htonl(1);  // Sample data (value=1 in network byte order)
    sleep(5);
    /* ----- 3. Send TLV Message ----- */
    ssize_t sent = write(fd, buf, sizeof(proto_hdr_t) + sizeof(int));
    if (sent == -1) {
        perror("write failed");
    } else {
        printf("Sent %zd bytes (Header: %ldB, Data: %ldB)\n", 
               sent, sizeof(proto_hdr_t), sizeof(int));
    }
}

int main() {
    /* ================ 1. Socket Setup ================ */

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket creation failed");
        return EXIT_FAILURE;
    }
   

    /* ================ 2. Server Configuration ================ */
    struct sockaddr_in serverInfo = {
        .sin_family = AF_INET,          // IPv4
        .sin_addr.s_addr = INADDR_ANY,  // Listen on all interfaces
        .sin_port = htons(5555)         // Port 5555
    };

    /* ================ 3. Socket Binding ================ */
    if (bind(fd, (struct sockaddr*)&serverInfo, sizeof(serverInfo)) == -1) {
        perror("bind failed");
        close(fd);
        return EXIT_FAILURE;
    }

    /* ================ 4. Start Listening ================ */
    if (listen(fd, SOMAXCONN) == -1) {  // SOMAXCONN = system max queue
        perror("listen failed");
        close(fd);
        return EXIT_FAILURE;
    }

    printf("Server running on port 5555 (Ctrl+C to stop)...\n");

    struct pollfd fds[15]; // 14 clients + 1 server
    fds[0].fd = fd; 
    fds[0].events = POLLIN; // lookout for new connections

    // initialize all client to -1, meaning empty
    for(int i = 1; i < 15; i++){
        fds[i].fd = -1;
    }

    /* ================ 5. Client Handling Loop ================ */
    while (1) {
        // waiting till an event occurs
        int event_happened = poll(fds, 15, -1);
        if(event_happened < 0) {
            perror("poll failed");
            exit(1);
        }

        // check sockets
        for(int i = 0; i < 15; i++){
            if(fds[i].fd == -1) continue;

            // connetion waiting
            if(i == 0 && (fds[i].revents & POLLIN)) {
                /* -- Accept new connection -- */
                struct sockaddr_in clientInfo = {0};
                socklen_t clientSize = sizeof(clientInfo);
        
                int cfd = accept(fd, (struct sockaddr*)&clientInfo, &clientSize);
                if (cfd == -1) {
                    perror("accept failed");
                    continue;  // Don't exit on single failure
                }

                /* -- Log client info -- */
                char clientIP[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &clientInfo.sin_addr, clientIP, sizeof(clientIP));
                // printf("Client connected from %s:%d\n", 
                    // clientIP, ntohs(clientInfo.sin_port));

                for(int j = 1; j < 15; i++){
                    if(fds[j].fd == -1){
                        fds[j].fd = cfd;
                        fds[j].events = POLLIN; // lookout for data
                        printf("New client connected (%s=%d)\n", clientIP, ntohs(clientInfo.sin_port) );
                        break;
                    }
                }

            } else if(fds[i].revents & (POLLHUP | POLLERR)){ 

                // client disconnected or err has occured
                printf("Client (fd=%d) disconnected\n", fds[i].fd);
                close(fds[i].fd);
                fds[i].fd = -1;
            } else{
                /* -- Handle client -- */
                handle_client(fds[i].fd);
                printf("Closing client connection (fd=%d)\n", fds[i].fd);
                close(fds[i].fd);  // Close client socket
                fds[i].fd = -1;
            }
        }

  }

    close(fd);  // Never reached (would need signal handling)
    return EXIT_SUCCESS;
}