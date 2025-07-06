#ifndef GPS_TRACKER_H
#define GPS_TRACKER_H

#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include "utils/gps_constants.h"
#include "tracking/correlator.h"

namespace gps {

// Tracking channel state
enum class ChannelState {
    IDLE,
    ACQUIRING,
    TRACKING,
    LOST
};


class TrackingChannel {
public:
    TrackingChannel(int prn, double sample_rate);
    ~TrackingChannel();

    
    void startAcquisition(const IQBuffer& samples);
    void updateTracking(const IQBuffer& samples);
    
    
    ChannelState getState() const { return state_; }
    SatelliteInfo getSatelliteInfo() const { return sat_info_; }
    bool hasNavigationBit() const;
    bool getNavigationBit();

private:
    
    bool performAcquisition(const IQBuffer& samples);
    
    
    void updatePLL(double phase_error);
    void updateDLL(double code_error);
    
    
    double calculatePhaseError(std::complex<float> prompt);
    double calculateCodeError(std::complex<float> early, 
                            std::complex<float> prompt, 
                            std::complex<float> late);

    
    ChannelState state_;
    SatelliteInfo sat_info_;
    
    std::unique_ptr<Correlator> correlator_;
    
    
    double carrier_freq_;
    double carrier_phase_;
    double code_freq_;
    double code_phase_;
    
    
    double pll_nco_;
    double dll_nco_;
    
    
    std::vector<double> correlation_history_;
    int bit_sync_counter_;
    
    
    double sample_rate_;
    int prn_;
};


class GPSTracker {
public:
    GPSTracker(double sample_rate = DEFAULT_SAMPLE_RATE);
    ~GPSTracker();

    
    void initialize(const std::vector<int>& prn_list);
    
    
    void processSamples(const IQBuffer& samples);
    
    
    void startTracking();
    void stopTracking();
    
    
    std::vector<SatelliteInfo> getTrackedSatellites() const;
    NavigationData getNavigationData(int prn) const;

private:
    
    std::vector<std::unique_ptr<TrackingChannel>> channels_;
    
    
    std::vector<std::thread> channel_threads_;
    std::atomic<bool> is_running_;
    
   
    void distributesamples(const IQBuffer& samples);
    
   
    double sample_rate_;
};

} 

#endif 