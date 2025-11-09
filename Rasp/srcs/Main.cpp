#include "../include/rrc_rasp.hpp"

// Definizione delle variabili globali
Mode currentMode = DRIVE;
pid_t stream_pid = -1;
std::mutex stream_mutex;
std::atomic<bool> stop_streaming(false);
FILE* stream_proc = nullptr;  // Pointer per popen()

void setupSocket(int &server_fd, struct sockaddr_in &address) {
    if ((server_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
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
    std::cout << "Server ready on UDP port " << PORT << std::endl;
}

void startServer() {
    int server_fd;
    struct sockaddr_in address;

    setupSocket(server_fd, address);  // Impostazione del socket
    setupGPIO();  // Impostazione dei pin GPIO
    handleCommand(server_fd);
    close(server_fd);
}

int main() {
    signal(SIGINT, signalHandler);  // Gestisce l'interruzione del programma (CTRL+C)
    
    startServer();  // Avvia il server
    return 0;
}
