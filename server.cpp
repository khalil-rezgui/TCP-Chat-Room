#include <iostream>
#include <cstring>
#include <cerrno>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

/* how many pending connections queue will hold */
#define BACKLOG 10 //irrelevant
#define MAXDATASIZE 300
/* my address information, address where I run this program */
char bCont = 'O';
#define BACKLOG 10
#define MAXDATASIZE 300
#define MAXSESSIONS 10

static void *ReceiveDataEar(void *p_data);
typedef struct {
    int fd;
    int pos;
    char AdresseIP[16];
    pthread_t thread_start;
} tSession;

tSession sessions[MAXSESSIONS];
int iNbSession;

void initSessions() {
    for (int i = 0; i < MAXSESSIONS; i++) {
        sessions[i].fd = 0;
        sessions[i].pos = -1;
    }
}

int getNextPos() {
    for (int p = 0; p < MAXSESSIONS; p++) {
        if (sessions[p].pos == -1) {
            return p;
        }
    }
    return -1;
}

int init_server(int uPort, int *sockfd) {
    /* my address information, address where I run this program */
    struct sockaddr_in my_addr;
    /* remote address information */
    std::cout << "Server-The local port is: " << uPort << std::endl;
    *sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (*sockfd == -1) {
        perror("socket() error !");
        return -1;
    } else std::cout << "socket() is OK..." << std::endl;
    int yes = 1;
    if (setsockopt(*sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("Server-setsockopt() error lol!");
        return -1;
    } else std::cout << "Server-setsockopt is OK..." << std::endl;
    my_addr.sin_family = AF_INET; //Stack IPv4
    /* short, network byte order */
    my_addr.sin_port = htons(uPort);
    /* auto-fill with my IP */
    my_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(my_addr.sin_zero), 0, 8);
    if (bind(*sockfd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr)) == -1) {
        perror("bind() error lol!");
        return -1;
    } else
        std::cout << "bind() on " << uPort << " port is OK..." << std::endl;
    if (listen(*sockfd, BACKLOG) == -1) {
        perror("listen() error lol!");
        return -1;
    } else
        std::cout << "SERVER: listen() is OK..." << std::endl;
    return 0;
}

void *ReceiveConnections(void *p_data) {
    char message[200];
    int sockfd = *(int *) p_data;
    int new_fd;
    struct sockaddr_in their_addr;
    unsigned int sin_size;

    std::cout << "accept connection on " << sockfd << " socket" << std::endl;

    do {
        sin_size = sizeof(struct sockaddr_in); // Initialize sin_size
        new_fd = accept(sockfd, (struct sockaddr *) &their_addr, &sin_size);

        if (new_fd == -1) {
            perror("SERVER : accept() unlocked !");
            return NULL;
        } else {
            std::cout << "SERVER : accept a new connection from [" << inet_ntoa(their_addr.sin_addr) << "]" << std::endl;
            sprintf(message, "You are welcome...");

            if (send(new_fd, message, strlen(message), 0) == -1)
                perror("Server-send() error !");

            int iPos = getNextPos();

            if (iPos >= 0) {
                sprintf(message, "SETID %d", iPos + 1);

                if (send(new_fd, message, strlen(message), 0) == -1)
                    perror("Server-send() error !");

                std::cout << "SERVER : welcome message sent...." << std::endl;

                // Mutex section...
                sessions[iPos].fd = new_fd;
                sessions[iPos].pos = iPos + 1;
                strcpy(sessions[iPos].AdresseIP, inet_ntoa(their_addr.sin_addr));

                // Dedicate a thread for client session after welcome message
                pthread_create(&sessions[iPos].thread_start, NULL, ReceiveDataEar, (void *) &sessions[iPos]);
                std::cout << "SERVER : dedicated ear thread launched...." << std::endl;
            }
        }
    } while (bCont);

    return NULL;
}

static void *ReceiveDataEar(void *p_data) {
    int new_fd;
    if (p_data != NULL)/* retrieve application context */
        new_fd = *(int *) p_data;

    int numbytes;
    char buf[MAXDATASIZE];
    do {
        if ((numbytes = recv(new_fd, buf, MAXDATASIZE - 1, 0)) == -1) {
            perror("recv()");
            exit(1);
        }

        if (numbytes > 0) {
            buf[numbytes] = '\0'; // Ensure null-termination
            std::cout << "\n[Client]: " << buf << std::endl;

            if (strcmp(buf, "stop") == 0)
                bCont = 'N';
        } else if (numbytes == 0) {
            bCont = 'n';
        }
    } while ((bCont != 'n') && (bCont != 'N'));

    close(new_fd);
    std::cout << "ReceiveDataEar thread exiting...press a char to exit!" << std::endl;

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Server-Usage: " << argv[0] << " the_port" << std::endl;
        exit(1);
    } // get the host info
    unsigned int uPort = atoi(argv[1]);
    initSessions();
    int sockfd;
    int ret = init_server(uPort, &sockfd);
    pthread_t tThreadAccept;
    if (ret < 0) { //request for closing from main thread
        perror("SERVER : init Failure !");
        exit(1);
    } else { //success case !
        pthread_create(&tThreadAccept, NULL, ReceiveConnections, (void *) &sockfd);
        std::cout << "Waiting for thread to accept connection..." << std::endl;
    }
    char message[200];
    do {
        std::cout << "\n[Me]:";
        scanf("%s", message);
        if (strlen(message) == 0)
            continue;
        if (bCont == 'N')
            break;
        std::cout << "Message to send on broadcast[" << MAXSESSIONS << "] :" << message << std::endl;
        for (int i = 0; i < MAXSESSIONS; i++) {
            if (sessions[i].fd != 0)
                if (send(sessions[i].fd, message, strlen(message), 0) == -1)
                    perror("sending() error !");
        }
        if (strcmp(message, "stop") == 0)
            bCont = 'N';
    } while ((bCont != 'n') && (bCont != 'N'));
    std::cout << "Closing sockets..." << std::endl;
    close(sockfd);
    for (int i = 0; i < MAXSESSIONS; i++) {
        if (sessions[i].fd != 0)
            pthread_join(sessions[i].thread_start, NULL);
    }
    pthread_join(tThreadAccept, NULL);
    std::cout << "Server exited properly :)" << std::endl;
    return 0;
}
