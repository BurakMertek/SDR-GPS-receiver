#ifndef NAV_DECODER_H
#define NAV_DECODER_H

#include <vector>
#include <map>
#include <memory>
#include <bitset>
#include "utils/gps_constants.h"

namespace gps {

/**
 * @brief GPS navigation message decoder
 * 
 * Decodes GPS L1 C/A navigation messages including:
 * - Ephemeris parameters
 * - Almanac data  
 * - Time parameters
 * - Ionospheric corrections
 */
class NavigationDecoder {
public:
    NavigationDecoder();
    ~NavigationDecoder() = default;
    
    /**
     * @brief Process navigation data bits
     * @param prn Satellite PRN number
     * @param nav_data Navigation data structure with subframes
     * @return True if new ephemeris data was decoded
     */
    bool processNavigationData(int prn, const NavigationData& nav_data);
    
    /**
     * @brief Add navigation bit to buffer
     * @param prn Satellite PRN number
     * @param bit Navigation bit value (0 or 1)
     * @param timestamp Bit timestamp
     */
    void addNavigationBit(int prn, bool bit, double timestamp);
    
    /**
     * @brief Get ephemeris data for a satellite
     * @param prn Satellite PRN number
     * @param ephemeris Output ephemeris data
     * @return True if valid ephemeris available
     */
    bool getEphemeris(int prn, EphemerisData& ephemeris) const;
    
    /**
     * @brief Check if ephemeris is valid and current
     * @param prn Satellite PRN number
     * @return True if ephemeris is valid
     */
    bool hasValidEphemeris(int prn) const;
    
    /**
     * @brief Get time of week from navigation data
     * @param prn Satellite PRN number
     * @return Time of week in seconds
     */
    double getTimeOfWeek(int prn) const;

private:
    
    struct Subframe {
        std::bitset<300> bits;  
        int id;                 
        bool valid;
        double tow;             
    };
    
    
    struct SatelliteData {
        std::vector<bool> bit_buffer;
        std::map<int, Subframe> subframes;
        EphemerisData ephemeris;
        bool ephemeris_valid;
        double last_update_time;
    };
    
    
    bool decodeSubframe1(const Subframe& sf, EphemerisData& eph);
    bool decodeSubframe2(const Subframe& sf, EphemerisData& eph);
    bool decodeSubframe3(const Subframe& sf, EphemerisData& eph);
    
    
    bool checkParity(const std::bitset<30>& word);
    uint32_t extractBits(const std::bitset<300>& bits, int start, int length);
    double extractSigned(const std::bitset<300>& bits, int start, int length, double scale);
    
    
    bool findPreamble(const std::vector<bool>& bits, size_t& preamble_pos);
    bool extractSubframe(const std::vector<bool>& bits, size_t start_pos, Subframe& sf);
    
    
    std::map<int, SatelliteData> satellite_data_;
    
    
    static constexpr uint32_t PREAMBLE = 0x8B;  
    static constexpr int WORD_SIZE = 30;
    static constexpr int SUBFRAME_SIZE = 300;
};

} 

#endif 