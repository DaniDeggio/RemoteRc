#include "../include/rrc_rasp.hpp"

void startVideoStream(struct sockaddr_in &client_addr) {
    std::lock_guard<std::mutex> lock(stream_mutex);

    // Prepara il comando per avviare rpicam-vid con streaming su UDP
    std::string command = "rpicam-vid -t 0 --inline -o udp://" + 
                          std::string(inet_ntoa(client_addr.sin_addr)) + ":1234";

    // Crea un processo figlio per eseguire rpicam-vid
    stream_pid = fork();
    if (stream_pid == 0) {
        // Codice del processo figlio: esegue il comando rpicam-vid
        execl("/bin/sh", "sh", "-c", command.c_str(), (char *)nullptr);
        exit(0);  // Termina il processo figlio quando il comando termina
    } else if (stream_pid < 0) {
        std::cerr << "Errore nella creazione del processo per rpicam-vid!" << std::endl;
    }

    // Controlla continuamente lo stato del flag di stop_streaming
    while (!stop_streaming.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));  // Attendi 1000 ms
    }

    // Interrompi lo streaming fermando il processo figlio
    if (stream_pid > 0) {
        kill(stream_pid, SIGTERM);  // Invia segnale di terminazione al processo figlio
        waitpid(stream_pid, nullptr, 0);  // Attendi la terminazione del processo figlio
        stream_pid = -1;  // Reset del PID
        std::cout << "Streaming terminato." << std::endl;
    }
}

void stopVideoStream() {
    stop_streaming.store(true);  // Imposta il flag di stop a true
}

void signalHandler(int signum) {
    std::cout << "Interrupt signal (" << signum << ") received.\n";
    stopVideoStream();  // Ferma lo streaming
    exit(signum);
}
