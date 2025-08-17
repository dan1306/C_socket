#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/* Ensure the protocol header has no padding */
#pragma pack(push, 1)
typedef enum {
    PROTO_HELLO = 0,  // Only message type defined
} proto_type_e;

typedef struct {
    proto_type_e type;  // 4-byte message type
    unsigned short len; // 2-byte length field
} proto_hdr_t;
#pragma pack(pop)

/**
 * Handles communication with the server
 * @param fd Connected socket file descriptor
 */
void handle_client(int fd) {
    char buf[4096] = {0};  // Buffer for incoming data
    
    /* 1. Receive exactly header + data from server */
    ssize_t bytes = read(fd, buf, sizeof(proto_hdr_t) + sizeof(int));
    if (bytes != sizeof(proto_hdr_t) + sizeof(int)) {
        perror("Failed to read complete message");
        return;
    }

    /* 2. Parse the TLV message */
    proto_hdr_t *hdr = (proto_hdr_t *)buf;
    hdr->type = ntohl(hdr->type);  // Convert from network byte order
    hdr->len = ntohs(hdr->len);    // Convert length

    int *data = (int *)(buf + sizeof(proto_hdr_t));  // Correct pointer math
    *data = ntohl(*data);          // Convert data

    /* 3. Validate protocol */
    if (hdr->type != PROTO_HELLO) {
        fprintf(stderr, "Protocol mismatch: Expected %d, got %d\n", 
                PROTO_HELLO, hdr->type);
        return;
    }

    if (*data != 1) {
        fprintf(stderr, "Version mismatch: Expected 1, got %d\n", *data);
        return;
    }

    /* 4. Success case */
    printf("Success! Received:\n");
    printf("- Message Type: %d (PROTO_HELLO)\n", hdr->type);
    printf("- Data Length: %d bytes\n", hdr->len);
    printf("- Payload Value: %d\n", *data);
}

int main(int argc, char *argv[]) {
    /* 1. Verify command line arguments */
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <server_ip>\n", argv[0]);
        return EXIT_FAILURE;
    }

    /* 2. Configure server address */
    struct sockaddr_in serverInfo = {0};
    serverInfo.sin_family = AF_INET;
    serverInfo.sin_port = htons(5555);  // Server port
    
    /* Convert IP address (with error checking) */
    if (inet_pton(AF_INET, argv[1], &serverInfo.sin_addr) <= 0) {
        perror("Invalid server address");
        return EXIT_FAILURE;
    }

    /* 3. Create TCP socket */
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("Socket creation failed");
        return EXIT_FAILURE;
    }

    /* 4. Connect to server */
    if (connect(fd, (struct sockaddr*)&serverInfo, sizeof(serverInfo)) == -1) {
        perror("Connection failed");
        close(fd);
        return EXIT_FAILURE;
    }

    printf("Connected to %s. Waiting for server message...\n", argv[1]);

    /* 5. Handle server response */
    handle_client(fd);

    /* 6. Cleanup */
    close(fd);
    return EXIT_SUCCESS;
} 