#include "../include/rrc_rasp.hpp"

void startVideoStream(struct sockaddr_in &client_addr) {
    std::lock_guard<std::mutex> lock(stream_mutex);

    // Prepara il comando per avviare rpicam-vid con streaming su UDP
    std::string command = "rpicam-vid -t 0 --inline -o udp://" + 
                          std::string(inet_ntoa(client_addr.sin_addr)) + ":1234";
    std::cout << "Address: " << inet_ntoa(client_addr.sin_addr) << std::endl;
    std::cout << "Port: " << ntohs(client_addr.sin_port) << std::endl;

    // Esegui il comando con popen()
    stream_proc = popen(command.c_str(), "r");
    if (!stream_proc) {
        std::cerr << "Errore nell'esecuzione del comando rpicam-vid!" << std::endl;
        return;
    }

    // Loop per continuare lo streaming finché stop_streaming non diventa true
    while (!stop_streaming.load()) {
        // Aggiungi logica per gestire lo streaming (se necessario)
        // Potresti monitorare eventuali output del comando qui se necessario
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Attesa per simulare il ciclo di controllo
    }

    // Chiudi il processo di streaming
    if (stream_proc) {
        std::cout << "Terminazione del processo di streaming..." << std::endl;
        pclose(stream_proc);  // Termina il processo avviato con popen()
        stream_proc = nullptr;
    }

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