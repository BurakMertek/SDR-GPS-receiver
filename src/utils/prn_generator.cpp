#include "utils/prn_generator.h"
#include <stdexcept>

namespace gps {

// GPS satellite PRN code phase assignments (G2 delay)
static const int PRN_G2_DELAY[32] = {
    5, 6, 7, 8, 17, 18, 139, 140, 141, 251,
    252, 254, 255, 256, 257, 258, 469, 470, 471, 472,
    473, 474, 509, 512, 513, 514, 515, 516, 859, 860,
    861, 862
};

PRNGenerator::PRNGenerator() {
    // Initialize G1 and G2 registers
    g1_register_.fill(1);
    g2_register_.fill(1);
}

std::vector<int> PRNGenerator::generateCode(int prn) {
    if (prn < 1 || prn > 32) {
        throw std::invalid_argument("PRN must be between 1 and 32");
    }
    
    std::vector<int> code;
    code.reserve(GPS_CA_CODE_LENGTH);
    
    // Reset shift registers
    g1_register_.fill(1);
    g2_register_.fill(1);
    
    // Generate 1023 chips
    for (int i = 0; i < GPS_CA_CODE_LENGTH; ++i) {
        // Get G1 output
        int g1_out = g1_register_[9];  // Tap at position 10
        
        // Get G2 output with satellite-specific phase
        int g2_out = getG2Output(prn);
        
        // C/A code = G1 XOR G2
        code.push_back(g1_out ^ g2_out);
        
        // Shift registers
        shiftG1();
        shiftG2();
    }
    
    return code;
}

std::vector<float> PRNGenerator::generateCodeFloat(int prn) {
    auto binary_code = generateCode(prn);
    std::vector<float> float_code;
    float_code.reserve(binary_code.size());
    
    // Convert binary (0,1) to bipolar (-1,+1)
    for (int bit : binary_code) {
        float_code.push_back(bit ? 1.0f : -1.0f);
    }
    
    return float_code;
}

void PRNGenerator::generateAllCodes(std::vector<std::vector<int>>& codes) {
    codes.clear();
    codes.resize(32);
    
    for (int prn = 1; prn <= 32; ++prn) {
        codes[prn - 1] = generateCode(prn);
    }
}

void PRNGenerator::shiftG1() {
    // G1 feedback taps: 3 and 10
    int feedback = g1_register_[2] ^ g1_register_[9];
    
    // Shift right
    for (int i = 9; i > 0; --i) {
        g1_register_[i] = g1_register_[i - 1];
    }
    g1_register_[0] = feedback;
}

void PRNGenerator::shiftG2() {
    // G2 feedback taps: 2, 3, 6, 8, 9, 10
    int feedback = g2_register_[1] ^ g2_register_[2] ^ 
                  g2_register_[5] ^ g2_register_[7] ^ 
                  g2_register_[8] ^ g2_register_[9];
    
    // Shift right
    for (int i = 9; i > 0; --i) {
        g2_register_[i] = g2_register_[i - 1];
    }
    g2_register_[0] = feedback;
}

int PRNGenerator::getG2Output(int prn) {
    // Get the delay for this PRN
    int delay = PRN_G2_DELAY[prn - 1];
    
    // Determine tap positions from delay
    int tap1 = 9;  // Always tap 10
    int tap2 = (tap1 + delay) % 10;
    
    // For some satellites, use different tap combinations
    switch (prn) {
        case 1:  return g2_register_[1] ^ g2_register_[5];   
        case 2:  return g2_register_[2] ^ g2_register_[6];   
        case 3:  return g2_register_[3] ^ g2_register_[7];   
        case 4:  return g2_register_[4] ^ g2_register_[8];   
        case 5:  return g2_register_[0] ^ g2_register_[8];   
        case 6:  return g2_register_[1] ^ g2_register_[9];   
        case 7:  return g2_register_[0] ^ g2_register_[7];   
        case 8:  return g2_register_[1] ^ g2_register_[8];   
        case 9:  return g2_register_[2] ^ g2_register_[9];   
        case 10: return g2_register_[1] ^ g2_register_[2];   
        case 11: return g2_register_[2] ^ g2_register_[3];   
        case 12: return g2_register_[4] ^ g2_register_[5];   
        case 13: return g2_register_[5] ^ g2_register_[6];   
        case 14: return g2_register_[6] ^ g2_register_[7];   
        case 15: return g2_register_[7] ^ g2_register_[8];   
        case 16: return g2_register_[8] ^ g2_register_[9];   
        case 17: return g2_register_[0] ^ g2_register_[3];   
        case 18: return g2_register_[1] ^ g2_register_[4];   
        case 19: return g2_register_[2] ^ g2_register_[5];   
        case 20: return g2_register_[3] ^ g2_register_[6];   
        case 21: return g2_register_[4] ^ g2_register_[7];   
        case 22: return g2_register_[5] ^ g2_register_[8];   
        case 23: return g2_register_[0] ^ g2_register_[2];   
        case 24: return g2_register_[3] ^ g2_register_[5];   
        case 25: return g2_register_[4] ^ g2_register_[6];   
        case 26: return g2_register_[5] ^ g2_register_[7];   
        case 27: return g2_register_[6] ^ g2_register_[8];   
        case 28: return g2_register_[7] ^ g2_register_[9];   
        case 29: return g2_register_[0] ^ g2_register_[5];   
        case 30: return g2_register_[1] ^ g2_register_[6];   
        case 31: return g2_register_[2] ^ g2_register_[7];   
        case 32: return g2_register_[3] ^ g2_register_[8];   
        default: return 0;
    }
}

// Optimized code generation for correlation
void PRNGenerator::generateCodeSampled(int prn, double sample_rate, 
                                      size_t num_samples, 
                                      std::vector<float>& sampled_code) {
    // Generate base PRN code
    auto base_code = generateCodeFloat(prn);
    
    sampled_code.clear();
    sampled_code.reserve(num_samples);
    
    // Code chip rate
    const double chip_rate = 1.023e6;  // 1.023 MHz
    const double samples_per_chip = sample_rate / chip_rate;
    
    // Generate sampled code
    for (size_t i = 0; i < num_samples; ++i) {
        double chip_index = i / samples_per_chip;
        int code_index = static_cast<int>(chip_index) % GPS_CA_CODE_LENGTH;
        sampled_code.push_back(base_code[code_index]);
    }
}

} 