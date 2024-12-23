#include "../include/rrc_rasp.hpp"

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

void handleCommand(int client_socket) {
    char buffer[1024] = {0};

    // Inizializza i sistemi di controllo prima di accettare i comandi
    initializeControlSystems();

    while (true) {
        int valread = read(client_socket, buffer, 1024);
        if (valread <= 0) {
            std::cout << "Client disconnected!" << std::endl;
            stopVideoStream();
            break;
        }

        // Parse dei valori ricevuti (sterzo, acceleratore, freno, paddle)
        int steering, accelerator, brake, paddle;
        sscanf(buffer, "%d %d %d %d", &steering, &accelerator, &brake, &paddle);

        // Stampa dei valori ricevuti per debug
        std::cout << "Sterzo: " << steering << ", Acceleratore: " << accelerator 
                  << ", Freno: " << brake << ", Paddle: " << paddle << std::endl;

        // Mappatura dei valori ricevuti per il PWM
        int steeringPWM = map(steering, 0, 1999, 500, 2500);
        int motorPWM = map(accelerator, 0, 1999, 0, 1024);
        int brakePWM = map(brake, 0, 1999, 0, 1024);

        // Cambia modalità in base al paddle
        if (paddle == 1) {
            currentMode = DRIVE;
            std::cout << "Modalità: DRIVE" << std::endl;
        } else if (paddle == -1) {
            currentMode = REVERSE;
            std::cout << "Modalità: REVERSE" << std::endl;
        }

        // Controllo del servo (sterzo)
        pwmWrite(SERVO_PIN, steeringPWM);

        // Controllo del motore (acceleratore/freno/retromarcia)
        if (brake > 0) {
            pwmWrite(MOTOR_PIN, brakePWM);  // Se freno, applica il PWM per frenare
        } else {
            if (currentMode == DRIVE) {
                pwmWrite(MOTOR_PIN, motorPWM);  // Accelerazione in modalità DRIVE
            } else if (currentMode == REVERSE) {
                pwmWrite(MOTOR_PIN, 1024 - motorPWM);  // Accelerazione in modalità REVERSE
            }
        }
    }

    close(client_socket); // Chiudi la connessione con il client dopo la disconnessione
}
