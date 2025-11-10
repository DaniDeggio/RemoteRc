// LINUX

#include <arpa/inet.h>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <mutex>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <cerrno>
#include <algorithm>
#include <cmath>

#include <SDL2/SDL.h>

#define PORT 8080       // Porta utilizzata
#define VIDEO_PORT 1234 // Porta per il flusso video

constexpr int AXIS_STEERING = 0;
constexpr int AXIS_ACCELERATOR = 1;
constexpr int AXIS_BRAKE = 2;
constexpr int AXIS_MAX_VALUE = 2000;
constexpr double RAW_AXIS_FULL_RANGE = 65535.0;
constexpr double RAW_AXIS_HALF_RANGE = 32768.0;

int normalizeAxis(int raw) {
    const double scaled = (static_cast<double>(raw) + RAW_AXIS_HALF_RANGE) * AXIS_MAX_VALUE / RAW_AXIS_FULL_RANGE;
    const int value = static_cast<int>(std::lround(scaled));
    return std::clamp(value, 0, AXIS_MAX_VALUE);
}

std::mutex commandMutex;
bool running = true;

// Avvia ffplay per ricevere il flusso video dal Raspberry Pi
void streamVideo(const std::string &raspberry_ip) {
    std::string command = "ffplay -fflags nobuffer -flags low_delay -framedrop udp://" + raspberry_ip +
                          ":" + std::to_string(VIDEO_PORT) + " >/dev/null 2>&1";
    FILE *result = popen(command.c_str(), "r");

    if (!result) {
        std::cerr << "Errore nell'esecuzione del comando ffplay." << std::endl;
    }
}

// Legge il Logitech G29 e invia i comandi al Raspberry Pi
void handleCommands(int sock, SDL_Joystick *g29) {
    int steering, accelerator, brake, paddle;
    while (running) {
        std::lock_guard<std::mutex> lock(commandMutex);
        SDL_JoystickUpdate();

    steering = SDL_JoystickGetAxis(g29, AXIS_STEERING);
        steering = (steering + 32767) / 32.767; // Normalizza tra 0 e 2000

        accelerator = SDL_JoystickGetAxis(g29, AXIS_ACCELERATOR);
        accelerator = AXIS_MAX_VALUE - normalizeAxis(accelerator);

        brake = SDL_JoystickGetAxis(g29, AXIS_BRAKE);
        brake = AXIS_MAX_VALUE - normalizeAxis(brake);

        paddle = 0;
        if (SDL_JoystickGetButton(g29, 4)) {
            paddle = -1;
        } else if (SDL_JoystickGetButton(g29, 5)) {
            paddle = 1;
        }

        if (sock >= 0) {
            std::string command = std::to_string(steering) + " " + std::to_string(accelerator) + " " +
                                  std::to_string(brake) + " " + std::to_string(paddle);
            std::cout << command << std::endl;
            send(sock, command.c_str(), command.length(), 0);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int main() {
    std::string raspberry_ip;
    std::cout << "Inserisci l'indirizzo IP del Raspberry Pi: ";
    std::cin >> raspberry_ip;

    if (SDL_Init(SDL_INIT_JOYSTICK) < 0) {
        std::cerr << "Impossibile inizializzare SDL: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_Joystick *g29 = SDL_JoystickOpen(0);
    if (!g29) {
        std::cerr << "Impossibile aprire il joystick: " << SDL_GetError() << std::endl;
        return -1;
    }

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        std::cerr << "Errore nella creazione del socket." << std::endl;
        return -1;
    }

    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, raspberry_ip.c_str(), &serv_addr.sin_addr) <= 0) {
        std::cerr << "Indirizzo IP non valido o non supportato." << std::endl;
        return -1;
    }

    if (connect(sock, reinterpret_cast<sockaddr *>(&serv_addr), sizeof(serv_addr)) < 0) {
        std::cerr << "Impossibile configurare il socket UDP: " << strerror(errno) << std::endl;
        close(sock);
        SDL_JoystickClose(g29);
        SDL_Quit();
        return -1;
    }

    std::cout << "Pronto a inviare datagrammi a " << raspberry_ip << std::endl;

    std::thread commandThread(handleCommands, sock, g29);
    std::thread videoThread(streamVideo, raspberry_ip);

    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = false;
            } else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
                std::cout << "Tasto ESC premuto, interrompendo il programma..." << std::endl;
                running = false;
            }
        }
    }

    running = false;

    if (commandThread.joinable()) {
        commandThread.join();
    }
    if (videoThread.joinable()) {
        videoThread.join();
    }

    SDL_JoystickClose(g29);
    close(sock);
    SDL_Quit();
    return 0;
}
