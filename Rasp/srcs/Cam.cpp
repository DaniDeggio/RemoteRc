#include "../include/rrc_rasp.hpp"

void encodeAndSendFrame(cv::Mat &frame, x264_t *encoder, x264_picture_t *pic_in, int sock, struct sockaddr_in &client_addr) {
    x264_nal_t *nals;
    int i_nals;

    // Convert frame from BGR to YUV (I420)
    cv::Mat yuv_frame;
    cv::cvtColor(frame, yuv_frame, cv::COLOR_BGR2YUV_I420);

    // Fill x264_picture with YUV data
    pic_in->img.plane[0] = yuv_frame.data;
    pic_in->img.i_stride[0] = yuv_frame.step[0];

    // Encode the frame
    int frame_size = x264_encoder_encode(encoder, &nals, &i_nals, pic_in, NULL);

    if (frame_size > 0) {
        // Send each NAL unit via UDP
        for (int i = 0; i < i_nals; ++i) {
            sendto(sock, nals[i].p_payload, nals[i].i_payload, 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
        }
    }
}

// Usa raspicam_cv per configurare la telecamera e acquisire i frame
void startVideoStream(int sock, struct sockaddr_in &client_addr, x264_t *encoder, x264_picture_t *pic_in) {
    std::lock_guard<std::mutex> lock(stream_mutex);

    // Inizializza RaspiCam
    raspicam::RaspiCam_Cv camera;
    camera.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    camera.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    camera.set(cv::CAP_PROP_FORMAT, CV_8UC3);  // Formato BGR per OpenCV

    // Apri la telecamera
    if (!camera.open()) {
        std::cerr << "Errore nell'apertura della telecamera!" << std::endl;
        return;
    }

    cv::Mat frame;

    while (true) {
        // Cattura il frame dalla telecamera
        camera.grab();
        camera.retrieve(frame);

        // Codifica e invia il frame via UDP
        encodeAndSendFrame(frame, encoder, pic_in, sock, client_addr);
    }

    // Rilascia la telecamera
    camera.release();
}


void stopVideoStream() {
    // Logic to stop video stream if needed (not implemented in this version)
}

void signalHandler(int signum) {
    std::cout << "Interrupt signal (" << signum << ") received.\n";
    stopVideoStream();
    exit(signum);
}
