#include "../include/rrc_rasp.hpp"
#include <cerrno>
#include <cstdio>

// Funzione di mappatura di un valore da un intervallo all'altro
int map(int x, int in_min, int in_max, int out_min, int out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void setupGPIO() {
    wiringPiSetup();
    pinMode(SERVO_PIN, PWM_OUTPUT);
    pinMode(MOTOR_PIN, PWM_OUTPUT);
    
    pwmSetMode(PWM_MODE_MS);
    pwmSetRange(1024);  // Imposta il range per il PWM (0-1024)
    pwmSetClock(192);   // Imposta la frequenza del PWM
}

void initializeControlSystems() {
    // Inizializza il servo motore (sterzo) e il motore (acceleratore/freno) con i valori di base
    std::cout << "Inizializzazione del sistema di controllo..." << std::endl;

    // Impostiamo il servo a una posizione neutra (ad esempio, 1500 microsecondi)
    pwmWrite(SERVO_PIN, 1500); // posizione neutra per il servo
    delay(500); // Attesa per stabilizzare il servo

    // Impostiamo il motore a velocità zero (stop)
    pwmWrite(MOTOR_PIN, 0); // Il motore è fermo
    delay(500); // Attesa per stabilizzare il motore

    std::cout << "Sistema di controllo inizializzato." << std::endl;
}

void handleCommand(int server_fd) {
    char buffer[1024];
    struct sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    struct sockaddr_in last_stream_addr{};
    bool stream_active = false;

    initializeControlSystems();

    while (true) {
        client_len = sizeof(client_addr);
        int valread = recvfrom(server_fd, buffer, sizeof(buffer) - 1, 0,
                               reinterpret_cast<struct sockaddr *>(&client_addr), &client_len);
        if (valread < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("recvfrom failed");
            continue;
        }

        buffer[valread] = '\0';

        if (!stream_active || client_addr.sin_addr.s_addr != last_stream_addr.sin_addr.s_addr) {
            if (stream_active) {
                stopVideoStream();
            }
            startVideoStream(client_addr);
            last_stream_addr = client_addr;
            stream_active = true;

            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
            std::cout << "Datagram dal client IP: " << client_ip << std::endl;
        }

        int steering = 0;
        int accelerator = 0;
        int brake = 0;
        int paddle = 0;
        if (sscanf(buffer, "%d %d %d %d", &steering, &accelerator, &brake, &paddle) != 4) {
            std::cerr << "Formato dei dati non valido: " << buffer << std::endl;
            continue;
        }

        std::cout << "Sterzo: " << steering << ", Acceleratore: " << accelerator
                  << ", Freno: " << brake << ", Paddle: " << paddle << std::endl;

        int steeringPWM = map(steering, 0, 1999, 500, 2500);
        int motorPWM = map(accelerator, 0, 1999, 0, 1024);
        int brakePWM = map(brake, 0, 1999, 0, 1024);

        if (paddle == 1) {
            currentMode = DRIVE;
            std::cout << "Modalità: DRIVE" << std::endl;
        } else if (paddle == -1) {
            currentMode = REVERSE;
            std::cout << "Modalità: REVERSE" << std::endl;
        }

        pwmWrite(SERVO_PIN, steeringPWM);

        if (brake > 0) {
            pwmWrite(MOTOR_PIN, brakePWM);
        } else {
            if (currentMode == DRIVE) {
                pwmWrite(MOTOR_PIN, motorPWM);
            } else if (currentMode == REVERSE) {
                pwmWrite(MOTOR_PIN, 1024 - motorPWM);
            }
        }
    }
}
