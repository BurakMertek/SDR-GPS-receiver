#ifndef SIGNAL_ACQUISITION_H
#define SIGNAL_ACQUISITION_H

#include <vector>
#include <complex>
#include <memory>
#include "utils/gps_constants.h"
#include "utils/prn_generator.h"

namespace gps {

// Acquisition result structure
struct AcquisitionResult {
    bool found;
    int prn;
    double code_phase;      // chips
    double doppler_shift;   // Hz
    double peak_ratio;      // Peak to second peak ratio
    double snr_estimate;    // Estimated SNR in dB
};

/**
 * @brief GPS signal acquisition engine
 * 
 * Performs parallel code phase search using FFT-based correlation
 * and serial frequency search over Doppler range.
 */
class SignalAcquisition {
public:
    SignalAcquisition(double sample_rate = DEFAULT_SAMPLE_RATE);
    ~SignalAcquisition();
    
    /**
     * @brief Search for a specific satellite
     * @param samples Input IQ samples
     * @param prn PRN number to search (1-32)
     * @param doppler_min Minimum Doppler frequency to search
     * @param doppler_max Maximum Doppler frequency to search
     * @param doppler_step Doppler search step size
     * @return Acquisition result
     */
    AcquisitionResult searchSatellite(const IQBuffer& samples,
                                     int prn,
                                     double doppler_min = -DOPPLER_SEARCH_RANGE,
                                     double doppler_max = DOPPLER_SEARCH_RANGE,
                                     double doppler_step = DOPPLER_SEARCH_STEP);
    
    /**
     * @brief Search for all visible satellites
     * @param samples Input IQ samples
     * @param prn_list List of PRNs to search
     * @return Vector of acquisition results
     */
    std::vector<AcquisitionResult> searchAllSatellites(
        const IQBuffer& samples,
        const std::vector<int>& prn_list);
    
    /**
     * @brief Set acquisition threshold
     * @param threshold Detection threshold (typical: 2.5)
     */
    void setThreshold(double threshold) { threshold_ = threshold; }
    
    /**
     * @brief 
     * @param enable 
     */
    void setParallelProcessing(bool enable) { use_parallel_ = enable; }

private:
    
    void performFFTCorrelation(const IQBuffer& samples,
                              const std::vector<float>& prn_code,
                              double doppler_shift,
                              std::vector<float>& correlation_result);
    
    
    void findPeak(const std::vector<float>& correlation,
                 double& peak_value,
                 size_t& peak_index,
                 double& peak_ratio);
    
    
    void generateCarrier(size_t length, double frequency, 
                        std::vector<std::complex<float>>& carrier);
    
    
    double sample_rate_;
    double threshold_;
    bool use_parallel_;
    
    
    std::unique_ptr<PRNGenerator> prn_generator_;
    
    
    void* fft_forward_plan_;
    void* fft_inverse_plan_;
    
    
    std::vector<std::complex<float>> fft_buffer_;
    std::vector<std::complex<float>> code_fft_;
    std::vector<std::complex<float>> carrier_buffer_;
};

} 

#endif 