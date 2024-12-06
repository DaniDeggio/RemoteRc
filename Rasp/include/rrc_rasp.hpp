#ifndef RRC_RASP_HPP
#define RRC_RASP_HPP

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

void setupGPIO();
void handleCommand(int client_socket);
void startVideoStream();
void stopVideoStream();
void signalHandler(int signum);
void setupSocket(int &server_fd, struct sockaddr_in &address);
void acceptClientConnections(int server_fd, struct sockaddr_in &address);
void startServer();

#endif // RRC_RASP_HPP