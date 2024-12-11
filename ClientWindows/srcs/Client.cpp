#define SDL_MAIN_HANDLED
#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include <thread>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080  // Porta utilizzata
#define VIDEO_PORT 1234  // Porta per il flusso video

// Funzione per gestire lo streaming video tramite ffplay
pid_t ffplay_pid = -1;  // PID del processo ffplay

void streamVideo(const std::string& raspberry_ip) {
    std::string command = "ffplay -fflags nobuffer -flags low_delay -framedrop udp://" + raspberry_ip + ":" + std::to_string(VIDEO_PORT);

    // Fork per eseguire ffplay
    ffplay_pid = fork();
    if (ffplay_pid == -1) {
        std::cerr << "Errore nella creazione del processo ffplay." << std::endl;
        return;
    } else if (ffplay_pid == 0) {
        // Codice del processo figlio (streaming video con ffplay)
        int result = system(command.c_str());
        if (result == -1) {
            std::cerr << "Errore nell'esecuzione del comando ffplay." << std::endl;
        }
        exit(0);  // Termina il processo figlio dopo aver eseguito ffplay
    }
}

void stopVideoStream() {
    if (ffplay_pid != -1) {
        // Invia un segnale SIGTERM per fermare il processo figlio che esegue ffplay
        std::cout << "Terminando il processo ffplay con PID: " << ffplay_pid << std::endl;
        kill(ffplay_pid, SIGTERM);  // Invia SIGTERM al processo ffplay
        waitpid(ffplay_pid, nullptr, 0);  // Attendi la terminazione del processo ffplay
        ffplay_pid = -1;  // Reset del PID
        std::cout << "Streaming video terminato." << std::endl;
    }
}

int main() {
    std::string raspberry_ip;
    std::cout << "Inserisci l'indirizzo IP del Raspberry Pi: ";
    std::cin >> raspberry_ip;

    // Inizializza SDL per il joystick (Logitech G29)
    if (SDL_Init(SDL_INIT_JOYSTICK) < 0) {
        std::cerr << "Impossibile inizializzare SDL: " << SDL_GetError() << std::endl;
        return -1;
    }
    SDL_Joystick* g29 = SDL_JoystickOpen(0);
    if (!g29) {
        std::cerr << "Impossibile aprire il joystick: " << SDL_GetError() << std::endl;
        return -1;
    }

    // Inizializzazione di Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Errore nell'inizializzazione di Winsock." << std::endl;
        return -1;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    if (sock < 0) {
        std::cerr << "Errore nella creazione del socket." << std::endl;
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, raspberry_ip.c_str(), &serv_addr.sin_addr) <= 0) {
        std::cerr << "Indirizzo IP non valido o non supportato." << std::endl;
        return -1;
    }

    bool connected = false;
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connessione fallita." << std::endl;
    } else {
        connected = true;
        std::cout << "Connesso al Raspberry Pi all'indirizzo IP: " << raspberry_ip << std::endl;
    }

    // Avvia il thread per lo streaming video tramite ffplay
    streamVideo(raspberry_ip);

    // Ciclo principale per leggere i comandi del volante
    bool running = true;
    while (running) {
        SDL_JoystickUpdate();

        int steering = SDL_JoystickGetAxis(g29, 0);  // Asse dello sterzo
        steering = (steering + 32767) / 32.767;      // Normalizza tra 0 e 2000

        int accelerator = SDL_JoystickGetAxis(g29, 2);  // Acceleratore (pedale destro)
        accelerator = (accelerator + 32767) / 32.767;

        int brake = SDL_JoystickGetAxis(g29, 3);  // Freno (pedale sinistro)
        brake = (brake + 32767) / 32.767;

        int paddle = 0;
        if (SDL_JoystickGetButton(g29, 4)) {
            paddle = -1;  // Modalità Reverse
        } else if (SDL_JoystickGetButton(g29, 5)) {
            paddle = 1;  // Modalità Drive
        }

        if (SDL_JoystickGetButton(g29, 9)) {
            std::cout << "Pulsante Options premuto, interrompendo il programma..." << std::endl;
            running = false;
        }

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = false;  // Esci dal ciclo principale
            } else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
                std::cout << "Tasto ESC premuto, interrompendo il programma..." << std::endl;
                running = false;  // Esci dal ciclo principale
            }
        }

        if (connected) {
            std::string command = std::to_string(steering) + " " + std::to_string(accelerator) + " " +
                                  std::to_string(brake) + " " + std::to_string(paddle);
            send(sock, command.c_str(), command.length(), 0);
        }
    }

    // Ferma lo streaming quando il programma termina
    stopVideoStream();

    SDL_JoystickClose(g29);
    closesocket(sock);
    SDL_Quit();
    WSACleanup();
    return 0;
}
