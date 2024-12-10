#include "../include/rrc_rasp.hpp"

void startVideoStream(struct sockaddr_in &client_addr) {
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

    while (!stop_streaming.load()) {
        // Continuare lo streaming fino a quando il flag non è impostato su true
        // Puoi aggiungere qui logica aggiuntiva per gestire il loop dello streaming
    }

    // Codice per fermare lo streaming (se necessario)
    std::cout << "Streaming terminato." << std::endl;
}
void stopVideoStream() {
    stop_streaming.store(true);  // Imposta il flag di stop a true
}

void signalHandler(int signum) {
    std::cout << "Interrupt signal (" << signum << ") received.\n";
    stopVideoStream();  // Ferma lo streaming
    exit(signum);
}