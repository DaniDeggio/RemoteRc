#include <iostream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <SDL2/SDL.h>

#define SERVER_IP "192.168.1.100"  // IP del Raspberry Pi
#define PORT 8080                  // Porta utilizzata

int main() {
    // Inizializzazione SDL per leggere i comandi del Logitech G29
    if (SDL_Init(SDL_INIT_JOYSTICK) < 0) {
        std::cerr << "Impossibile inizializzare SDL: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_Joystick *g29 = SDL_JoystickOpen(0);
    if (!g29) {
        std::cerr << "Impossibile aprire il joystick: " << SDL_GetError() << std::endl;
        return -1;
    }

    int sock = 0;
    struct sockaddr_in serv_addr;
    
    // Creazione del socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Errore nella creazione del socket." << std::endl;
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Conversione dell'IP dell'host in binario
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        std::cerr << "Indirizzo IP non valido o non supportato." << std::endl;
        return -1;
    }

    // Connessione al server (Raspberry Pi)
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connessione fallita." << std::endl;
        return -1;
    }

    // Ciclo principale per leggere i comandi del volante
    while (true) {
        SDL_JoystickUpdate();

        // Lettura dello sterzo (asse 0)
        int steering = SDL_JoystickGetAxis(g29, 0);  
        steering = (steering + 32767) / 32.767;  // Normalizza i valori tra 0 e 2000

        // Lettura dell'acceleratore (pedale destro, asse 2)
        int accelerator = SDL_JoystickGetAxis(g29, 2);
        accelerator = (accelerator + 32767) / 32.767;  // Normalizza i valori tra 0 e 2000

        // Lettura del freno (pedale sinistro, asse 3)
        int brake = SDL_JoystickGetAxis(g29, 3);
        brake = (brake + 32767) / 32.767;  // Normalizza i valori tra 0 e 2000

        // Aggiungi la gestione dei paddle (cambio modalità)
        int paddle = 0;
        if (SDL_JoystickGetButton(g29, 4)) {  // Paddle sinistro (pulsante 4)
            paddle = -1;  // Modalità Reverse
        } else if (SDL_JoystickGetButton(g29, 5)) {  // Paddle destro (pulsante 5)
            paddle = 1;  // Modalità Drive
        }

        // Preparazione del messaggio da inviare
        std::string command = std::to_string(steering) + " " + std::to_string(accelerator) + " " + std::to_string(brake) + " " + std::to_string(paddle);
        
        // Invia il comando al Raspberry Pi
        send(sock, command.c_str(), command.length(), 0);

        // Debug
        std::cout << "Sterzo: " << steering << " | Acceleratore: " << accelerator 
                  << " | Freno: " << brake << " | Paddle: " << paddle << std::endl;

        // Aggiungi un piccolo ritardo per non sovraccaricare la CPU
        SDL_Delay(100);  // 100 ms di ritardo
    }

    // Chiudi le risorse
    SDL_JoystickClose(g29);
    close(sock);
    SDL_Quit();

    return 0;
}
