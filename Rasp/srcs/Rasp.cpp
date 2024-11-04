#include <iostream>
#include <wiringPi.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <chrono>  // Per calcolare la latenza
#include <sys/wait.h>  // Per usare fork e gestire processi
#include <signal.h>    // Per inviare segnali ai processi

#define SERVO_PIN 1  // GPIO pin per il servo (sterzo)
#define MOTOR_PIN 2  // GPIO pin per la centralina (acceleratore/freno/retromarcia)
#define PORT 8080    // Porta per la connessione socket

// Modalità di guida
enum Mode { DRIVE, REVERSE };
Mode currentMode = DRIVE;  // Modalità iniziale

pid_t stream_pid = -1;  // Variabile per memorizzare il PID del processo di streaming

void setupGPIO() {
    wiringPiSetup();
    pinMode(SERVO_PIN, PWM_OUTPUT);
    pinMode(MOTOR_PIN, PWM_OUTPUT);
    
    pwmSetMode(PWM_MODE_MS);
    pwmSetRange(2000);
    pwmSetClock(192);  // Imposta la frequenza del PWM
}

// Funzione per avviare lo streaming video tramite ffmpeg
void startVideoStream() {
    stream_pid = fork();  // Crea un nuovo processo
    if (stream_pid == 0) {
        // Questo è il processo figlio che avvia lo streaming
        execlp("ffmpeg", "ffmpeg", "-f", "v4l2", "-i", "/dev/video0", "-f", "mpegts", "udp://192.168.1.25:1234", NULL);
        
        // Se execlp fallisce, termina il processo figlio
        perror("Failed to start video stream");
        exit(EXIT_FAILURE);
    } else if (stream_pid < 0) {
        perror("Failed to fork process for video stream");
    } else {
        std::cout << "Video stream started with PID: " << stream_pid << std::endl;
    }
}

// Funzione per fermare lo streaming video
void stopVideoStream() {
    if (stream_pid > 0) {
        kill(stream_pid, SIGTERM);  // Termina il processo di streaming
        waitpid(stream_pid, NULL, 0);  // Aspetta che il processo finisca
        std::cout << "Video stream stopped." << std::endl;
        stream_pid = -1;  // Resetta il PID
    }
}

// Gestisce l'interruzione del programma
void signalHandler(int signum) {
    std::cout << "Interrupt signal (" << signum << ") received.\n";
    stopVideoStream();  // Ferma lo streaming
    exit(signum);       // Esci dal programma
}

void handleCommand(int client_socket) {
    char buffer[1024] = {0};

    while (true) {
        int valread = read(client_socket, buffer, 1024);
        if (valread <= 0) {
            std::cout << "Client disconnected!" << std::endl;
            break;  // Esci dal loop ma non fermare lo streaming
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
            pwmWrite(MOTOR_PIN, 1000);  // Segnale PWM per frenare
        } else {
            if (currentMode == DRIVE) {
                pwmWrite(MOTOR_PIN, accelerator);  // Imposta la velocità del motore in avanti
            } else if (currentMode == REVERSE) {
                pwmWrite(MOTOR_PIN, 2000 - accelerator);  // Imposta la velocità in retromarcia
            }
        }
    }
}

int main() {
    signal(SIGINT, signalHandler);  // Registra il gestore di segnali per Ctrl+C

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

    startVideoStream();  // Avvia lo streaming video all'inizio

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

        close(client_socket);  // Chiudi la connessione con il client
    }

    stopVideoStream();  // Ferma lo streaming alla fine
    close(server_fd);
    return 0;
}
