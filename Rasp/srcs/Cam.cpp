#include "../include/rrc_rasp.hpp"

// Usa raspicam_cv per configurare la telecamera e acquisire i frame
void startVideoStream(int sock, struct sockaddr_in &client_addr) {
    std::lock_guard<std::mutex> lock(stream_mutex);

    // Prepara il comando per avviare rpicam-vid con streaming su UDP
    std::string command = "rpicam-vid -t 0 --inline -o udp://" + 
                          std::string(inet_ntoa(client_addr.sin_addr)) + ":" + 
                          std::to_string(ntohs(client_addr.sin_port));

    // Esegui il comando per avviare il flusso
    int result = system(command.c_str());
    if (result != 0) {
        std::cerr << "Errore nell'esecuzione del comando rpicam-vid!" << std::endl;
        return;
    }
}

void stopVideoStream() {
    // Logic to stop video stream if needed (not implemented in this version)
}

void signalHandler(int signum) {
    std::cout << "Interrupt signal (" << signum << ") received.\n";
    stopVideoStream();
    exit(signum);
}
