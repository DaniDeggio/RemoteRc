#include "../include/rrc_rasp.hpp"
#include <atomic>
#include <csignal>
#include <cstdlib>
#include <iostream>

namespace {
std::atomic<bool> keepRunning{true};

void handleSignal(int) {
    keepRunning = false;
}

void configurePWM() {
    if (wiringPiSetup() < 0) {
        std::cerr << "Impossibile inizializzare wiringPi" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    pinMode(SERVO_PIN, PWM_OUTPUT);
    pwmSetMode(PWM_MODE_MS);
    pwmSetRange(20000); // ~50Hz, 1µs resolution (clock=19)
    pwmSetClock(19);
}

void writeServo(int value) {
    pwmWrite(SERVO_PIN, value);
    delay(20);
}
}

int main() {
    signal(SIGINT, handleSignal);
    signal(SIGTERM, handleSignal);

    configurePWM();

    constexpr int SERVO_LEFT = 1000;
    constexpr int SERVO_CENTER = 1500;
    constexpr int SERVO_RIGHT = 2000;
    constexpr int STEP = 10; // 10µs per step

    std::cout << "Test sterzo: CTRL+C per interrompere" << std::endl;

    writeServo(SERVO_CENTER);
    delay(500);

    while (keepRunning.load()) {
        for (int pos = SERVO_CENTER; pos <= SERVO_RIGHT && keepRunning.load(); pos += STEP) {
            writeServo(pos);
        }
        for (int pos = SERVO_RIGHT; pos >= SERVO_LEFT && keepRunning.load(); pos -= STEP) {
            writeServo(pos);
        }
        for (int pos = SERVO_LEFT; pos <= SERVO_CENTER && keepRunning.load(); pos += STEP) {
            writeServo(pos);
        }
    }

    writeServo(SERVO_CENTER);
    delay(250);

    std::cout << "Test sterzo terminato" << std::endl;
    return 0;
}
