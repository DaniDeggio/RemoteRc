#include <iostream>
#include <wiringPi.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <chrono>
#include <sys/wait.h>
#include <signal.h>
#include <thread>
#include <mutex>

#define SERVO_PIN 1
#define MOTOR_PIN 2
#define PORT 8080

// Modalità di guida
enum Mode { DRIVE, REVERSE };
Mode currentMode = DRIVE;

pid_t stream_pid = -1;  // Variabile per memorizzare il PID del processo di streaming
std::mutex stream_mutex; // Mutex per gestire l'accesso al processo di streaming

void setupGPIO() {
    wiringPiSetup();
    pinMode(SERVO_PIN, PWM_OUTPUT);
    pinMode(MOTOR_PIN, PWM_OUTPUT);
    
    pwmSetMode(PWM_MODE_MS);
    pwmSetRange(2000);
    pwmSetClock(192);
}

void startVideoStream() {
    std::lock_guard<std::mutex> lock(stream_mutex); // Assicura l'accesso sicuro al processo di streaming
    stream_pid = fork();
    if (stream_pid == 0) {
        execlp("ffmpeg", "ffmpeg", "-f", "v4l2", "-i", "/dev/video0", "-f", "mpegts", "udp://192.168.1.25:1234", NULL);
        perror("Failed to start video stream");
        exit(EXIT_FAILURE);
    } else if (stream_pid < 0) {
        perror("Failed to fork process for video stream");
    } else {
        std::cout << "Video stream started with PID: " << stream_pid << std::endl;
    }
}

void stopVideoStream() {
    std::lock_guard<std::mutex> lock(stream_mutex);
    if (stream_pid > 0) {
        kill(stream_pid, SIGTERM);
        waitpid(stream_pid, NULL, 0);
        std::cout << "Video stream stopped." << std::endl;
        stream_pid = -1;
    }
}

void signalHandler(int signum) {
    std::cout << "Interrupt signal (" << signum << ") received.\n";
    stopVideoStream();
    exit(signum);
}

void handleCommand(int client_socket) {
    char buffer[1024] = {0};

    while (true) {
        int valread = read(client_socket, buffer, 1024);
        if (valread <= 0) {
            std::cout << "Client disconnected!" << std::endl;
            break; // Esci dal loop ma non fermare lo streaming
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

    close(client_socket); // Chiudi la connessione con il client dopo la disconnessione
}

int main() {
    signal(SIGINT, signalHandler);

    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    std::cout << "Server listening on port " << PORT << std::endl;

    setupGPIO();

    startVideoStream();  // Avvia lo streaming video all'inizio

    while (true) {
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(address.sin_addr), client_ip, INET_ADDRSTRLEN);
        std::cout << "Client connected from IP: " << client_ip << std::endl;

        // Crea un thread per gestire il client
        std::thread client_thread(handleCommand, client_socket);
        client_thread.detach(); // Scollega il thread per continuare ad accettare nuovi client
    }

    stopVideoStream();  // Ferma lo streaming alla fine
    close(server_fd);
    return 0;
}
