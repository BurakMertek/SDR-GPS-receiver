#ifndef GPS_CONSTANTS_H
#define GPS_CONSTANTS_H

#include <cstdint>
#include <complex>
#include <vector>

namespace gps {

// GPS L1 C/A signal parameters
constexpr double GPS_L1_FREQ_HZ = 1575.42e6;  // L1 frequency in Hz
constexpr double GPS_CA_CODE_FREQ_HZ = 1.023e6;  // C/A code frequency
constexpr int GPS_CA_CODE_LENGTH = 1023;  // C/A code length in chips
constexpr int GPS_DATA_RATE_BPS = 50;  // Navigation data rate
constexpr int GPS_MAX_SATELLITES = 32;  // Maximum number of GPS satellites

// Sampling parameters
constexpr double DEFAULT_SAMPLE_RATE = 2.048e6;  
constexpr double DEFAULT_IF_FREQ = 0.0;  

// Acquisition parameters
constexpr double ACQUISITION_THRESHOLD = 2.5;  
constexpr int DOPPLER_SEARCH_RANGE = 5000;  
constexpr int DOPPLER_SEARCH_STEP = 500;  

// Tracking parameters
constexpr double PLL_BANDWIDTH = 18.0;  
constexpr double DLL_BANDWIDTH = 2.0;  
constexpr double TRACKING_INTEGRATION_TIME = 0.001;  


using IQSample = std::complex<float>;
using IQBuffer = std::vector<IQSample>;

// Satellite structure
struct SatelliteInfo {
    int prn;
    double doppler_shift;
    double code_phase;
    double carrier_phase;
    double cn0;  
    bool is_tracked;
    bool has_ephemeris;
};


struct NavigationData {
    uint32_t subframe[5][10];  
    bool subframe_valid[5];
    double tow;  
};

// Ephemeris data structure
struct EphemerisData {
    int prn;
    double toe;  // Time of ephemeris
    double sqrt_a;  // Square root of semi-major axis
    double ecc;  // Eccentricity
    double i0;  // Inclination at reference time
    double omega0;  // Longitude of ascending node
    double w;  // Argument of perigee
    double m0;  // Mean anomaly at reference time
    // Add more parameters as needed
};

} 

#endif 