#include "../include/rrc_rasp.hpp"

// Definizione delle variabili globali
Mode currentMode = DRIVE;
pid_t stream_pid = -1;
std::mutex stream_mutex;

void setupSocket(int &server_fd, struct sockaddr_in &address) {
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
}

void acceptClientConnections(int server_fd, struct sockaddr_in &address) {
    int addrlen = sizeof(address);
    int client_socket;

    while (true) {
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(address.sin_addr), client_ip, INET_ADDRSTRLEN);
        std::cout << "Client connected from IP: " << client_ip << std::endl;

        // Avvia lo streaming video al client connesso
        std::thread stream_thread(startVideoStream, address);  // Passa l'indirizzo del client alla funzione di streaming
        stream_thread.detach();

        // Crea un thread per gestire il client
        std::thread client_thread(handleCommand, client_socket);
        client_thread.detach(); // Scollega il thread per continuare ad accettare nuovi client
    }
}

void startServer() {
    int server_fd;
    struct sockaddr_in address;

    setupSocket(server_fd, address);  // Impostazione del socket
    setupGPIO();  // Impostazione dei pin GPIO
    acceptClientConnections(server_fd, address);  // Accetta connessioni dai client
    close(server_fd);
}

int main() {
    signal(SIGINT, signalHandler);  // Gestisce l'interruzione del programma (CTRL+C)
    
    startServer();  // Avvia il server
    return 0;
}
