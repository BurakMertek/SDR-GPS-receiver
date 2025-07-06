#include "acquisition/sdr_receiver.h"
#include <iostream>
#include <cstring>
#include <chrono>

namespace gps {

SDRReceiver::SDRReceiver() 
    : device_(nullptr)
    , sample_rate_(DEFAULT_SAMPLE_RATE)
    , center_freq_(GPS_L1_FREQ_HZ)
    , gain_(40)
    , is_running_(false) {
}

SDRReceiver::~SDRReceiver() {
    stopCapture();
    if (device_) {
        rtlsdr_close(device_);
    }
}

bool SDRReceiver::initializeDevice(int device_index, double sample_rate, double center_freq) {
    
    int device_count = rtlsdr_get_device_count();
    if (device_count == 0) {
        std::cerr << "No RTL-SDR devices found!" << std::endl;
        return false;
    }

    if (device_index >= device_count) {
        std::cerr << "Invalid device index!" << std::endl;
        return false;
    }

    
    const char* device_name = rtlsdr_get_device_name(device_index);
    std::cout << "Opening device: " << device_name << std::endl;

    
    int ret = rtlsdr_open(&device_, device_index);
    if (ret < 0) {
        std::cerr << "Failed to open RTL-SDR device!" << std::endl;
        return false;
    }

    
    ret = rtlsdr_set_sample_rate(device_, static_cast<uint32_t>(sample_rate));
    if (ret < 0) {
        std::cerr << "Failed to set sample rate!" << std::endl;
        rtlsdr_close(device_);
        device_ = nullptr;
        return false;
    }
    sample_rate_ = sample_rate;

    
    ret = rtlsdr_set_center_freq(device_, static_cast<uint32_t>(center_freq));
    if (ret < 0) {
        std::cerr << "Failed to set center frequency!" << std::endl;
        rtlsdr_close(device_);
        device_ = nullptr;
        return false;
    }
    center_freq_ = center_freq;

    
    ret = rtlsdr_set_agc_mode(device_, 1);
    if (ret < 0) {
        std::cerr << "Warning: Failed to enable AGC" << std::endl;
    }

    
    ret = rtlsdr_reset_buffer(device_);
    if (ret < 0) {
        std::cerr << "Warning: Failed to reset buffer" << std::endl;
    }

    std::cout << "SDR device initialized successfully!" << std::endl;
    std::cout << "Sample rate: " << sample_rate_ / 1e6 << " MHz" << std::endl;
    std::cout << "Center frequency: " << center_freq_ / 1e6 << " MHz" << std::endl;

    return true;
}

bool SDRReceiver::startCapture() {
    if (!device_ || is_running_) {
        return false;
    }

    is_running_ = true;
    
    
    capture_thread_ = std::thread([this]() {
        const int buffer_size = 256 * 1024;  
        int ret = rtlsdr_read_async(device_, rtlsdrCallback, this, 0, buffer_size);
        if (ret < 0) {
            std::cerr << "RTL-SDR async read failed!" << std::endl;
            is_running_ = false;
        }
    });

    return true;
}

void SDRReceiver::stopCapture() {
    if (!is_running_) {
        return;
    }

    is_running_ = false;
    
    if (device_) {
        rtlsdr_cancel_async(device_);
    }

    if (capture_thread_.joinable()) {
        capture_thread_.join();
    }

   
    {
        std::lock_guard<std::mutex> lock(buffer_mutex_);
        std::queue<IQSample> empty;
        std::swap(sample_buffer_, empty);
    }
}

bool SDRReceiver::getSamples(IQBuffer& buffer, size_t num_samples) {
    std::unique_lock<std::mutex> lock(buffer_mutex_);
    
    
    auto timeout = std::chrono::milliseconds(100);
    if (buffer_cv_.wait_for(lock, timeout, [this, num_samples]() {
        return sample_buffer_.size() >= num_samples || !is_running_;
    })) {
        
        if (sample_buffer_.size() >= num_samples) {
            buffer.clear();
            buffer.reserve(num_samples);
            
            for (size_t i = 0; i < num_samples; ++i) {
                buffer.push_back(sample_buffer_.front());
                sample_buffer_.pop();
            }
            return true;
        }
    }
    
    return false;
}

bool SDRReceiver::setGain(int gain_db) {
    if (!device_) {
        return false;
    }

    
    int ret = rtlsdr_set_agc_mode(device_, 0);
    if (ret < 0) {
        return false;
    }

    
    ret = rtlsdr_set_tuner_gain(device_, gain_db * 10);  
    if (ret < 0) {
        return false;
    }

    gain_ = gain_db;
    return true;
}

bool SDRReceiver::setAutoGain(bool enable) {
    if (!device_) {
        return false;
    }

    int ret = rtlsdr_set_agc_mode(device_, enable ? 1 : 0);
    return ret >= 0;
}

void SDRReceiver::rtlsdrCallback(unsigned char* buf, uint32_t len, void* ctx) {
    SDRReceiver* receiver = static_cast<SDRReceiver*>(ctx);
    receiver->processRawData(buf, len);
}

void SDRReceiver::processRawData(unsigned char* buf, uint32_t len) {
    IQBuffer iq_samples;
    convertToIQ(buf, len, iq_samples);

    std::lock_guard<std::mutex> lock(buffer_mutex_);
    
    for (const auto& sample : iq_samples) {
        
        if (sample_buffer_.size() >= MAX_BUFFER_SIZE) {
            sample_buffer_.pop();  
        }
        sample_buffer_.push(sample);
    }
    
    buffer_cv_.notify_all();
}

void SDRReceiver::convertToIQ(const unsigned char* raw_data, size_t len, IQBuffer& iq_data) {
    
    size_t num_samples = len / 2;
    iq_data.clear();
    iq_data.reserve(num_samples);

    for (size_t i = 0; i < len; i += 2) {
        
        float i_sample = (raw_data[i] - 127.5f) / 127.5f;
        float q_sample = (raw_data[i + 1] - 127.5f) / 127.5f;
        iq_data.emplace_back(i_sample, q_sample);
    }
}

} 