#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
  #include <winsock2.h>
  #pragma comment(lib, "ws2_32.lib")
  typedef int socklen_t;

#else
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <unistd.h>
#endif

#define PORT 8080
#define BUFFER_SIZE 4096

int main() {
#ifdef _WIN32
        WSADATA wsa;
        WSAStartup(MAKEWORD(2,2), &wsa);
#endif
        int server_fd, new_socket;
        struct sockaddr_in address;
        int addrlen;
        char buffer[BUFFER_SIZE];

        addrlen = sizeof(address);

        // 1. Create socket
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd == -1) {
                perror("socket failed");
                return 1;
        }

        // 2. Bind to port
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY; // 0.0.0.0
        address.sin_port = htons(PORT);

        if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
                perror("bind failed");
                return 1;
        }

        // 3. Listen for incoming connections
        if (listen(server_fd, 10) < 0) {
                perror("listen");
                return 1;
        }

        printf("server running at http://localhost: %d...\n", PORT);
        printf("Press Ctrl+C to stop the server.\n");

        // 4. Accept one connection
        while (1) {
                new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
                if (new_socket < 0) {
                        perror("accept");
                        continue;
                }

                // 5. Read data from client
                memset(buffer, 0, BUFFER_SIZE);
                int valread = recv(new_socket, buffer, BUFFER_SIZE, 0);
                if(valread < 0) {
                        perror("recv");
                        close(new_socket);
                        continue;
                }

                printf("Received request:\n%s\n", buffer);

                // Parse HTTP method and path
                char method[8], path[1019]; // 1024 - 1 (for dot) - 1 (for null) - 3 extra buffer
                sscanf(buffer, "%s %s", method, path);

                // Default route ("/") becomes index.html
                if (strcmp(path, "/") == 0) {
                        strcpy(path, "/index.html");
                }

                // Remove leading slash from path
                char filepath[1024];
                snprintf(filepath, sizeof(filepath), ".%s", path);

                FILE *fp = fopen(filepath, "r");
                if (fp == NULL) {
                        const char *not_found = "HTTP/1.1 404 Not Found\r\nContent-Length: 13\r\n\r\n404 Not Found";
                        send(new_socket, not_found, strlen(not_found), 0);
                } else {
                        fseek(fp, 0, SEEK_END);
                        long fsize = ftell(fp);
                        rewind(fp);

                        char *file_buffer = malloc(fsize + 1);
                        fread(file_buffer, 1, fsize, fp);
                        file_buffer[fsize] = 0;
                        fclose(fp);

                        // 6. Send a basic http response
                        char header[256];
                        sprintf(header,
                                        "HTTP/1.1 200 OK\r\n"
                                        "Content-length: %ld\r\n"
                                        "Content-Type: text/html\r\n"
                                        "\r\n", fsize);
                        send(new_socket, header, strlen(header), 0);
                        send(new_socket, file_buffer, fsize, 0);
                        free(file_buffer);
                }

                // Close sockets and cleanup
#ifdef _WIN32
                closesocket(new_socket);
#else
                close(new_socket);
#endif
        }
#ifdef _WIN32
        closesocket(server_fd);
        WSACleanup();
#else
        close(server_fd);
#endif
        return 0;
}