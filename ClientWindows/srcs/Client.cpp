#define SDL_MAIN_HANDLED
#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080  // Porta utilizzata
#define VIDEO_PORT 1234  // Porta per il flusso video

// Funzione per gestire lo streaming video
void streamVideo(const std::string& raspberry_ip) {
    std::string command = "ffmpeg -i udp://" + raspberry_ip + ":" + std::to_string(VIDEO_PORT) +
                          " -f rawvideo -pix_fmt rgb24 pipe:1";
    FILE* pipe = _popen(command.c_str(), "rb");

    if (!pipe) {
        std::cerr << "Errore nell'aprire la pipe." << std::endl;
        return;
    }

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Video Stream", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, 0);
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, 640, 480);

    uint8_t* buffer = new uint8_t[640 * 480 * 3]; // Buffer per RGB (3 byte per pixel)

    while (true) {
        size_t bytesRead = fread(buffer, 1, 640 * 480 * 3, pipe);  // Legge un frame

        if (bytesRead != 640 * 480 * 3) {
            std::cerr << "Errore nella lettura del frame o fine dello stream." << std::endl;
            break;
        }

        SDL_UpdateTexture(texture, nullptr, buffer, 640 * 3);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
        // SDL_Delay(30); // Aggiorna il frame a circa 33 fps
    }

    delete[] buffer;
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    _pclose(pipe);
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

    // Avvia il thread per lo streaming video
    std::thread videoThread(streamVideo, raspberry_ip);
    videoThread.detach();

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

        if (connected) {
            std::string command = std::to_string(steering) + " " + std::to_string(accelerator) + " " +
                                  std::to_string(brake) + " " + std::to_string(paddle);
            send(sock, command.c_str(), command.length(), 0);
        }

        // std::cout << "Sterzo: " << steering << " | Acceleratore: " << accelerator
        //           << " | Freno: " << brake << " | Paddle: " << paddle
        //           << " | Connesso: " << (connected ? "Sì" : "No") << std::endl;

		// SDL_Delay(5);  // 30 ms di ritardo	
        // SDL_Delay(100);  // 100 ms di ritardo
    }

    SDL_JoystickClose(g29);
    closesocket(sock);
    SDL_Quit();
    WSACleanup();
    return 0;
}
