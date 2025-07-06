#ifndef PRN_GENERATOR_H
#define PRN_GENERATOR_H

#include <vector>
#include <array>
#include "gps_constants.h"

namespace gps {

/**
 * @brief GPS C/A PRN code generator
 * 
 * Generates GPS L1 C/A PRN codes using Linear Feedback Shift Registers (LFSR).
 * Implements the standard GPS PRN code generation algorithm with G1 and G2
 * generators as specified in the GPS ICD.
 */
class PRNGenerator {
public:
    PRNGenerator();
    ~PRNGenerator() = default;
    
    /**
     * @brief Generate PRN code for a specific satellite
     * @param prn Satellite PRN number (1-32)
     * @return Vector of 1023 binary values (0 or 1)
     */
    std::vector<int> generateCode(int prn);
    
    /**
     * @brief Generate PRN code as floating point values
     * @param prn Satellite PRN number (1-32)
     * @return Vector of 1023 float values (-1.0 or +1.0)
     */
    std::vector<float> generateCodeFloat(int prn);
    
    /**
     * @brief Generate codes for all GPS satellites
     * @param codes Output vector of PRN codes (indexed 0-31 for PRN 1-32)
     */
    void generateAllCodes(std::vector<std::vector<int>>& codes);
    
    /**
     * @brief Generate sampled PRN code at specific sample rate
     * @param prn Satellite PRN number
     * @param sample_rate Sampling rate in Hz
     * @param num_samples Number of samples to generate
     * @param sampled_code Output vector of sampled code values
     */
    void generateCodeSampled(int prn, double sample_rate, 
                           size_t num_samples, 
                           std::vector<float>& sampled_code);

private:
    // G1 and G2 shift registers (10 bits each)
    std::array<int, 10> g1_register_;
    std::array<int, 10> g2_register_;
    
    // Shift register operations
    void shiftG1();
    void shiftG2();
    int getG2Output(int prn);
};

} 

#endif 