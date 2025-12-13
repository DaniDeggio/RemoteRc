#include "../include/rrc_rasp.hpp"
#include <cerrno>
#include <cstdio>
#include <algorithm>

// Funzione di mappatura di un valore da un intervallo all'altro
int map(int x, int in_min, int in_max, int out_min, int out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

constexpr int PWM_MIN_US = 1000;     // Retromarcia massima
constexpr int PWM_MAX_US = 2000;     // Avanti massima
constexpr int PWM_NEUTRAL_US = 1500; // Punto neutro centrale
constexpr int PWM_DEAD_LOW = 1485;   // Dead zone
constexpr int PWM_DEAD_HIGH = 1515;

void setupGPIO() {
    wiringPiSetup();
    pinMode(SERVO_PIN, PWM_OUTPUT);
    pinMode(MOTOR_PIN, PWM_OUTPUT);
    
    // Impostiamo il PWM a ~50Hz con risoluzione a microsecondi (range 0-20000).
    // 19.2MHz / (clock * range) = frequenza; clock=19, range=20000 -> ~50.5Hz.
    pwmSetMode(PWM_MODE_MS);
    pwmSetRange(20000);
    pwmSetClock(19);
}

void initializeControlSystems() {
    // Inizializza il servo motore (sterzo) e il motore (acceleratore/freno) con i valori di base
    std::cout << "Inizializzazione del sistema di controllo..." << std::endl;

    // Impostiamo il servo a una posizione neutra (1500µs) e il motore al neutro ESC
    pwmWrite(SERVO_PIN, PWM_NEUTRAL_US);
    delay(500); // Attesa per stabilizzare il servo

    pwmWrite(MOTOR_PIN, PWM_NEUTRAL_US); // ESC neutro
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
    // Mappiamo i valori joystick (0-1999) nei microsecondi richiesti dall'ESC/servo.
    int steeringPWM = std::clamp(map(steering, 0, 1999, 1000, 2000), PWM_MIN_US, PWM_MAX_US);
    int forwardPWM = std::clamp(map(accelerator, 0, 1999, PWM_NEUTRAL_US, PWM_MAX_US), PWM_NEUTRAL_US, PWM_MAX_US);
    int brakePWM = std::clamp(map(brake, 0, 1999, PWM_NEUTRAL_US, PWM_MIN_US), PWM_MIN_US, PWM_NEUTRAL_US);
    int reversePWM = std::clamp(map(accelerator, 0, 1999, PWM_NEUTRAL_US, PWM_MIN_US), PWM_MIN_US, PWM_NEUTRAL_US);

        if (paddle == 1) {
            currentMode = DRIVE;
            std::cout << "Modalità: DRIVE" << std::endl;
        } else if (paddle == -1) {
            currentMode = REVERSE;
            std::cout << "Modalità: REVERSE" << std::endl;
        }

        pwmWrite(SERVO_PIN, steeringPWM);

        int throttlePWM = PWM_NEUTRAL_US;

        // Logica ESC bidirezionale: 
        // - 1000µs: retromarcia massima
        // - 1500µs: neutro (dead zone ~1485-1515)
        // - 2000µs: avanti massima
        // Nota: passando da avanti a indietro l'ESC richiede due comandi sotto 1500µs (freno poi reverse).

        if (brake > 15) { // Piccola soglia per evitare rumore sui pedali
            throttlePWM = brakePWM; // freno / richiesta reverse (1° comando frena, 2° reverse)
        } else if (currentMode == DRIVE) {
            throttlePWM = forwardPWM;
        } else if (currentMode == REVERSE) {
            throttlePWM = reversePWM;
        } else {
            throttlePWM = PWM_NEUTRAL_US;
        }

        // Evita di uscire dalla deadzone se il comando è già neutro
        if (throttlePWM > PWM_DEAD_LOW && throttlePWM < PWM_DEAD_HIGH && brake <= 15 && accelerator <= 15) {
            throttlePWM = PWM_NEUTRAL_US;
        }

        pwmWrite(MOTOR_PIN, throttlePWM);
    }
}
