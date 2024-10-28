#include <iostream>
#include <wiringPi.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SERVO_PIN 1  // GPIO pin per il servo (sterzo)
#define MOTOR_PIN 2  // GPIO pin per la centralina (acceleratore/freno/retromarcia)
#define PORT 8080    // Porta per la connessione socket

// Modalità di guida
enum Mode { DRIVE, REVERSE };
Mode currentMode = DRIVE;  // Modalità iniziale

void setupGPIO() {
    wiringPiSetup();
    pinMode(SERVO_PIN, PWM_OUTPUT);
    pinMode(MOTOR_PIN, PWM_OUTPUT);
    
    pwmSetMode(PWM_MODE_MS);
    pwmSetRange(2000);
    pwmSetClock(192);  // Imposta la frequenza del PWM
}

void handleCommand(int client_socket) {
    char buffer[1024] = {0};
    int valread = read(client_socket, buffer, 1024);

    if (valread == 0) {
        std::cout << "Client disconnected!" << std::endl;
        return;
    }

    // Parse dei valori ricevuti (sterzo, acceleratore, freno, paddle)
    int steering, accelerator, brake, paddle;
    sscanf(buffer, "%d %d %d %d", &steering, &accelerator, &brake, &paddle);

    // Cambia modalità in base al paddle
    if (paddle == 1) {
        currentMode = DRIVE;
        std::cout << "Modalità: DRIVE" << std::endl;
    } else if (paddle == -1) {
        currentMode = REVERSE;
        std::cout << "Modalità: REVERSE" << std::endl;
    }

    // Controllo del servo (sterzo)
    pwmWrite(SERVO_PIN, steering);

    // Controllo del motore (acceleratore/freno/retromarcia)
    if (brake > 0) {
        // Se viene premuto il freno, invia il segnale PWM per frenare
        pwmWrite(MOTOR_PIN, 1000);  // PWM per il freno (puoi regolare il valore)
    } else {
        if (currentMode == DRIVE) {
            // Se siamo in modalità DRIVE, invia il segnale per accelerare in avanti
            pwmWrite(MOTOR_PIN, accelerator);  // Imposta la velocità del motore in avanti
        } else if (currentMode == REVERSE) {
            // Se siamo in modalità REVERSE, invia il segnale per andare indietro
            pwmWrite(MOTOR_PIN, 2000 - accelerator);  // Imposta la velocità in retromarcia
        }
    }

    // Stampa i dati ricevuti (sterzo, acceleratore, freno, paddle)
    std::cout << "Steering: " << steering << " | Accelerator: " << accelerator
              << " | Brake: " << brake << " | Paddle: " << paddle 
              << " | Mode: " << (currentMode == DRIVE ? "Drive" : "Reverse") << std::endl;
}

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Impostazione del socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Binding del socket alla porta
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Ascolto delle connessioni in arrivo
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    std::cout << "Server listening on port " << PORT << std::endl;

    setupGPIO();  // Imposta i pin GPIO

    while (true) {
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            continue;
        }

        // Stampa l'indirizzo IP del client connesso
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(address.sin_addr), client_ip, INET_ADDRSTRLEN);
        std::cout << "Client connected from IP: " << client_ip << std::endl;

        handleCommand(client_socket);  // Gestisce il comando del client

        close(client_socket);
    }

    close(server_fd);
    return 0;
}