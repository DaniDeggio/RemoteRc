#include "../include/rrc_rasp.hpp"

void setupGPIO() {
    wiringPiSetup();
    pinMode(SERVO_PIN, PWM_OUTPUT);
    pinMode(MOTOR_PIN, PWM_OUTPUT);
    
    pwmSetMode(PWM_MODE_MS);
    pwmSetRange(2000);
    pwmSetClock(192);
}

void handleCommand(int client_socket) {
    char buffer[1024] = {0};

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

		       // Stampa dei valori ricevuti
        std::cout << "Sterzo: " << steering << ", Acceleratore: " << accelerator 
                  << ", Freno: " << brake << ", Paddle: " << paddle << std::endl;

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
            pwmWrite(MOTOR_PIN, 1000);  // Segnale PWM per frenare
        } else {
            if (currentMode == DRIVE) {
                pwmWrite(MOTOR_PIN, accelerator);  // Imposta la velocità del motore in avanti
            } else if (currentMode == REVERSE) {
                pwmWrite(MOTOR_PIN, 2000 - accelerator);  // Imposta la velocità in retromarcia
            }
        }
    }

    close(client_socket); // Chiudi la connessione con il client dopo la disconnessione
}