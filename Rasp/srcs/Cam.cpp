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

void startVideoStream(int sock, struct sockaddr_in &client_addr) {
    std::lock_guard<std::mutex> lock(stream_mutex);

    // Initialize OpenCV video capture (using the Raspberry Pi Camera)
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "Error opening camera!" << std::endl;
        return;
    }

    // Set camera properties
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    cap.set(cv::CAP_PROP_FPS, 30);

    // Initialize x264 encoder
    x264_param_t param;
    x264_param_default_preset(&param, "ultrafast", "zerolatency");
    param.i_threads = 1;
    param.i_width = 640;
    param.i_height = 480;
    param.i_fps_num = 30;
    param.i_fps_den = 1;
    param.i_keyint_max = 30;
    param.b_intra_refresh = 1;
    param.rc.i_rc_method = X264_RC_CRF;
    param.rc.f_rf_constant = 25;
    param.rc.f_rf_constant_max = 35;
    param.i_sps_id = 7;
    param.b_repeat_headers = 1;
    param.b_annexb = 1;

    x264_t *encoder = x264_encoder_open(&param);
    x264_picture_t pic_in;
    x264_picture_alloc(&pic_in, X264_CSP_I420, 640, 480);

    cv::Mat frame;

    while (true) {
        // Capture a new frame
        cap >> frame;
        if (frame.empty()) {
            std::cerr << "Failed to capture frame!" << std::endl;
            break;
        }

        // Encode and send the frame via UDP
        encodeAndSendFrame(frame, encoder, &pic_in, sock, client_addr);
    }

    // Cleanup
    x264_picture_clean(&pic_in);
    x264_encoder_close(encoder);
}

void stopVideoStream() {
    // Logic to stop video stream if needed (not implemented in this version)
}

void signalHandler(int signum) {
    std::cout << "Interrupt signal (" << signum << ") received.\n";
    stopVideoStream();
    exit(signum);
}
