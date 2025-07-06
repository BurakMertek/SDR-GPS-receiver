#ifndef SDR_RECEIVER_H
#define SDR_RECEIVER_H

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <memory>
#include "utils/gps_constants.h"

namespace gps {

class SDRReceiver {
public:
    SDRReceiver();
    ~SDRReceiver();

    
    bool initializeDevice(int device_index = 0, 
                         double sample_rate = DEFAULT_SAMPLE_RATE,
                         double center_freq = GPS_L1_FREQ_HZ);

    
    bool startCapture();
    void stopCapture();

    
    bool getSamples(IQBuffer& buffer, size_t num_samples);

    

    double getSampleRate() const { return sample_rate_; }
    double getCenterFrequency() const { return center_freq_; }

    
    bool setGain(int gain_db);
    bool setAutoGain(bool enable);

private:
    
    static void rtlsdrCallback(unsigned char* buf, uint32_t len, void* ctx);
    void processRawData(unsigned char* buf, uint32_t len);

    

    
    double sample_rate_;
    double center_freq_;
    int gain_;

    
    std::thread capture_thread_;
    std::atomic<bool> is_running_;
    
    // Circular buffer for samples
    std::queue<IQSample> sample_buffer_;
    std::mutex buffer_mutex_;
    std::condition_variable buffer_cv_;
    static constexpr size_t MAX_BUFFER_SIZE = 1024 * 1024;  

    // Convert 8-bit unsigned to float IQ
    void convertToIQ(const unsigned char* raw_data, size_t len, IQBuffer& iq_data);
};

} 

#endif 