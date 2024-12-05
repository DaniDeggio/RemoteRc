#include <iostream>
#include <wiringPi.h>
#include <opencv2/opencv.hpp>  // Aggiungi OpenCV per la codifica e gestione immagini
#include <raspicam/raspicam_cv.h>  // Libreria per la Pi Camera
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <mutex>
#include <signal.h>

#define SERVO_PIN 1
#define MOTOR_PIN 2
#define PORT 8080
#define VIDEO_PORT 1234  // Porta per lo streaming video
#define FRAME_WIDTH 640
#define FRAME_HEIGHT 480

// Modalità di guida
enum Mode { DRIVE, REVERSE };
Mode currentMode = DRIVE;

int sock = -1;  // Socket per inviare il video
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
    std::lock_guard<std::mutex> lock(stream_mutex);  // Assicura l'accesso sicuro al processo di streaming
    
    // Configurazione socket UDP
    sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(VIDEO_PORT);
    serv_addr.sin_addr.s_addr = inet_addr("192.168.1.25");

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Errore nella creazione del socket UDP");
        return;
    }

    // Utilizza raspicam_cv per aprire la Pi Camera
    raspicam::RaspiCam_Cv cap;
    cap.set(cv::CAP_PROP_FRAME_WIDTH, FRAME_WIDTH);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT);

    if (!cap.open()) {
        std::cerr << "Errore nell'apertura della Pi Camera" << std::endl;
        return;
    }

    cv::Mat frame;
    std::vector<uchar> buffer;

    while (true) {
        cap.grab();  // Cattura il frame
        cap.retrieve(frame);

        if (frame.empty()) {
            std::cerr << "Errore nella cattura del frame" << std::endl;
            break;
        }

        // Comprimi il frame in JPEG per risparmiare larghezza di banda
        cv::imencode(".jpg", frame, buffer);
        // Invia il frame tramite UDP
        sendto(sock, buffer.data(), buffer.size(), 0, (sockaddr*)&serv_addr, sizeof(serv_addr));

        std::this_thread::sleep_for(std::chrono::milliseconds(33));  // Approssima a 30 fps
    }
}

void stopVideoStream() {
    std::lock_guard<std::mutex> lock(stream_mutex);
    if (sock > 0) {
        close(sock);  // Chiude il socket
        std::cout << "Video stream stopped." << std::endl;
        sock = -1;
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
            break;
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

    close(client_socket);
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

        std::thread client_thread(handleCommand, client_socket);
        client_thread.detach();
    }

    stopVideoStream();
    close(server_fd);
    return 0;
}
