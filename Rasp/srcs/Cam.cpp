#include "../include/rrc_rasp.hpp"

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
