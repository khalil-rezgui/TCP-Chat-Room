#include <iostream>
#include <algorithm>
#include <string>
#include <vector>
#include <thread>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <mutex>

// Constants
const int PORT = 8080;
const int MAX_CLIENTS = 10;

// Global variables
std::vector<int> client_sockets; // Vector to store client socket descriptors
std::mutex mtx; // Mutex for thread safety

// Function to send a message to a client
void send_message(int client_socket, const std::string& message) {
    send(client_socket, message.c_str(), message.size(), 0);
}

// Function to receive a message from a client
std::string receive_message(int client_socket) {
    char buffer[1024] = {0};
    recv(client_socket, buffer, 1024, 0);
    return std::string(buffer);
}

// Function to handle a client connection
void handle_client(int client_socket) {
    while (true) {
        // Receive message from client
        std::string message = receive_message(client_socket);
        if (message.empty()) {
            // If message is empty, client disconnected
            std::lock_guard<std::mutex> lock(mtx);
            // Remove client socket from the vector
			auto it = std::find(std::begin(client_sockets), std::end(client_sockets), client_socket);
            if (it != client_sockets.end()) {
                client_sockets.erase(it);
            }
            close(client_socket); // Close client socket
            std::cout << "Client disconnected" << std::endl;
            break;
        }
        std::cout << "Received: " << message << std::endl;

        // Broadcast the message to all other clients
        {
            std::lock_guard<std::mutex> lock(mtx);
            for (int socket : client_sockets) {
                if (socket != client_socket) {
                    send_message(socket, message);
                }
            }
        }
    }
}

// Main server function
void server() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    int opt = 1;
    int addrlen = sizeof(server_addr);

    // Create server socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind socket to port
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr))<0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_socket, MAX_CLIENTS) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    std::cout << "Server listening on port " << PORT << std::endl;

    // Accept incoming connections
    while (true) {
        if ((client_socket = accept(server_socket, (struct sockaddr *)&client_addr, (socklen_t*)&addrlen))<0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        std::cout << "New client connected" << std::endl;

        // Add client socket to the vector
        {
            std::lock_guard<std::mutex> lock(mtx);
            client_sockets.push_back(client_socket);
        }

        // Handle client in a new thread
        std::thread client_thread(handle_client, client_socket);
        client_thread.detach();
    }
}

// Main function
int main() {
    server(); // Start the server
    return 0;
}
