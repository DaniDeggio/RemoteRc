#include "../include/rrc_rasp.hpp"

void startVideoStream(struct sockaddr_in &client_addr) {
    std::lock_guard<std::mutex> lock(stream_mutex);

    // Prepara il comando per avviare rpicam-vid con streaming su UDP
    std::string command = "rpicam-vid -t 0 --inline -o udp://" + 
                          std::string(inet_ntoa(client_addr.sin_addr)) + ":1234" + 
                          " > /dev/null 2>&1";  // Redirige sia stdout che stderr a /dev/null
    std::cout << "Address: " << inet_ntoa(client_addr.sin_addr) << std::endl;
    std::cout << "Port: " << ntohs(client_addr.sin_port) << std::endl;

    // Crea un nuovo processo per lo streaming
    stream_pid = fork();

    if (stream_pid == -1) {
        // Se fork fallisce
        std::cerr << "Errore nella creazione del processo di streaming!" << std::endl;
        return;
    } else if (stream_pid == 0) {
        // Codice del processo figlio (streaming)
        // Usa execlp per eseguire il comando di streaming
        execlp("sh", "sh", "-c", command.c_str(), (char*)NULL);

        // Se execlp fallisce
        std::cerr << "Errore nell'esecuzione del comando di streaming!" << std::endl;
        exit(EXIT_FAILURE);
    } else {
        // Codice del processo padre (continua l'esecuzione normalmente)
        std::cout << "Streaming avviato con PID: " << stream_pid << std::endl;

        // Loop per controllare lo stato del flag di stop
        while (!stop_streaming.load()) {
            // Attesa per evitare un ciclo troppo rapido
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // Quando lo streaming deve essere interrotto
        stopVideoStream();  // Ferma lo streaming
    }
}

void stopVideoStream() {
    stop_streaming.store(true);  // Imposta il flag di stop a true

    // Se il processo di streaming è attivo, invia un segnale di terminazione
    if (stream_pid != -1) {
        std::cout << "Invio del segnale di terminazione al processo di streaming con PID: " << stream_pid << std::endl;
        kill(stream_pid, SIGKILL);  // Invia SIGTERM al processo di streaming
        waitpid(stream_pid, nullptr, 0);  // Attendi la terminazione del processo figlio
        stream_pid = -1;  // Resetta il PID del processo
        std::cout << "Streaming terminato." << std::endl;
    }
}

void signalHandler(int signum) {
    std::cout << "Interrupt signal (" << signum << ") received.\n";
    stopVideoStream();  // Ferma lo streaming
    exit(signum);
}
