#include "../include/rrc_rasp.hpp"

int main() {
    signal(SIGINT, signalHandler);

    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    std::cout << "Server listening on port " << PORT << std::endl;

    setupGPIO();

    startVideoStream();  // Avvia lo streaming video all'inizio

    while (true) {
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(address.sin_addr), client_ip, INET_ADDRSTRLEN);
        std::cout << "Client connected from IP: " << client_ip << std::endl;

        // Crea un thread per gestire il client
        std::thread client_thread(handleCommand, client_socket);
        client_thread.detach(); // Scollega il thread per continuare ad accettare nuovi client

        //handleCommand(client_socket);  // Gestisce il comando del client
        //close(client_socket);  // Chiudi la connessione con il client
    }

    stopVideoStream();  // Ferma lo streaming alla fine
    close(server_fd);
    return 0;
}
