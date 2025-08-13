#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>

int main() {
    struct sockaddr_in clientInfo = {0}; 
    int clientSize = 0;
    struct sockaddr_in serverInfo = {0};

    serverInfo.sin_family = AF_INET;
    serverInfo.sin_addr.s_addr = 0;
    serverInfo.sin_port = htons(5555);
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    if(fd == -1) {
        perror("socket");
        return -1;
    }

    printf("%d\n", fd);

    // bind
    if (bind(fd, (struct sockaddr*)&serverInfo, sizeof(serverInfo)) == -1 ) {
        perror("bind");
        close(fd);
        return -1;
    }
    // listen
    if(listen( fd, 0) == -1 ) {
        perror("listen");
        close(fd);
        return -1;
    }
    // accept 
    int cfd = accept(fd, (struct sockaddr*)&clientInfo, &clientSize);
    if(cfd == -1) {
        perror("accept");
        close(fd);
        return -1;
    }

    close(cfd);
}