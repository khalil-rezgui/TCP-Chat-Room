# TCP Chat Room Server

This is a simple TCP chat room server implemented in C++. It allows multiple clients to connect and communicate with each other over a network.

## Features
- Handles multiple client connections simultaneously.
- Supports message broadcasting to all connected clients.
- Ensures thread safety using mutex locks.

## Requirements
- C++ compiler
- Unix-like operating system (e.g., Linux)

## Usage
1. Compile the server code using a C++ compiler.
2. Run the compiled executable.
3. Clients can connect to the server using a TCP client application.

## How to Connect
- Clients can connect to the server using its IP address and port number (default port: 8080).
- Once connected, clients can send messages that will be broadcasted to all other connected clients.

## Example Client
- You can use any TCP client application to connect to the server.
- For testing purposes, you can use the `telnet` command in your terminal:



## Author
- Khalil REZGUI

## License
This project is licensed under the [MIT License](LICENSE).
