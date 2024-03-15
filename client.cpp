#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define MAXDATASIZE 300

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Client-Usage: " << argv[0] << " server_hostname server_port" << std::endl;
        exit(1);
    }

    char *server_hostname = argv[1];
    char *server_port = argv[2];

    int sockfd, numbytes;
    char buf[MAXDATASIZE];
    struct hostent *he;
    struct sockaddr_in their_addr;

    if ((he = gethostbyname(server_hostname)) == nullptr) {
        perror("gethostbyname");
        exit(1);
    }

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    their_addr.sin_family = AF_INET;
    their_addr.sin_port = htons(atoi(server_port));
    their_addr.sin_addr = *((struct in_addr *)he->h_addr);
    memset(&(their_addr.sin_zero), '\0', 8);

    if (connect(sockfd, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) == -1) {
        perror("connect");
        exit(1);
    }

    std::cout << "Connected to server. Type 'stop' to exit." << std::endl;

    while (1) {
        std::cout << "[Me]: ";
        std::cin.getline(buf, MAXDATASIZE); // Use getline to read input along with spaces
        // buf[strcspn(buf, "\n")] = '\0'; // No need to remove newline character for getline

        if (strcmp(buf, "stop") == 0) {
            std::cout << "Stopping client." << std::endl;
            break;
        }

        if (send(sockfd, buf, strlen(buf), 0) == -1) {
            perror("send");
            exit(1);
        }

        if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
            perror("recv");
            exit(1);
        }

        buf[numbytes] = '\0';
        std::cout << "[Server]: " << buf << std::endl;
    }

    close(sockfd);
    return 0;
}
