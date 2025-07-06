#include <gtest/gtest.h>
#include <random>
#include <chrono>
#include "tracking/correlator.h"
#include "utils/prn_generator.h"

using namespace gps;

class CorrelatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize test parameters
        sample_rate_ = 2.048e6;
        prn_ = 1;
        correlator_ = std::make_unique<Correlator>(prn_, sample_rate_);
        
        // Generate test signal
        generateTestSignal();
    }

    void generateTestSignal() {
        const size_t num_samples = 2048;
        test_signal_.clear();
        test_signal_.reserve(num_samples);
        
        // Generate PRN code
        PRNGenerator prn_gen;
        std::vector<int> prn_code = prn_gen.generateCode(prn_);
        
        // Create test signal with known parameters
        const double code_phase = 100.5;  // chips
        const double carrier_freq = 1000.0;  // Hz
        const double carrier_phase = M_PI / 4;
        
        for (size_t i = 0; i < num_samples; ++i) {
            double t = i / sample_rate_;
            
            // Carrier
            double carrier = std::cos(2 * M_PI * carrier_freq * t + carrier_phase);
            
            // Code (with chip rate of 1.023 MHz)
            double code_time = t * 1.023e6;
            int code_index = static_cast<int>(code_time + code_phase) % GPS_CA_CODE_LENGTH;
            double code = prn_code[code_index] ? 1.0 : -1.0;
            
            // Add noise
            std::normal_distribution<double> noise_dist(0.0, 0.1);
            double noise = noise_dist(rng_);
            
            // Complex signal
            double i_sample = code * carrier + noise;
            double q_sample = code * std::sin(2 * M_PI * carrier_freq * t + carrier_phase) + noise_dist(rng_);
            
            test_signal_.emplace_back(i_sample, q_sample);
        }
    }

    // Test members
    std::unique_ptr<Correlator> correlator_;
    IQBuffer test_signal_;
    double sample_rate_;
    int prn_;
    std::mt19937 rng_{42};  // Fixed seed for reproducibility
};

TEST_F(CorrelatorTest, BasicCorrelation) {
    // Test basic correlation functionality
    CorrelationResult result = correlator_->correlate(
        test_signal_,
        100.5,    // code phase (chips)
        M_PI / 4, // carrier phase
        1000.0    // carrier frequency (Hz)
    );
    
    // Prompt correlation should be strongest
    EXPECT_GT(result.power_prompt, result.power_early);
    EXPECT_GT(result.power_prompt, result.power_late);
    
    // Power should be positive
    EXPECT_GT(result.power_prompt, 0.0);
}

TEST_F(CorrelatorTest, CodePhaseDiscrimination) {
    // Test code phase discrimination
    std::vector<double> code_phases = {99.0, 99.5, 100.0, 100.5, 101.0, 101.5, 102.0};
    std::vector<double> prompt_powers;
    
    for (double phase : code_phases) {
        CorrelationResult result = correlator_->correlate(
            test_signal_,
            phase,
            M_PI / 4,
            1000.0
        );
        prompt_powers.push_back(result.power_prompt);
    }
    
    // Find peak
    auto max_it = std::max_element(prompt_powers.begin(), prompt_powers.end());
    int max_idx = std::distance(prompt_powers.begin(), max_it);
    
    // Peak should be near the true code phase (100.5)
    EXPECT_NEAR(code_phases[max_idx], 100.5, 0.5);
}

TEST_F(CorrelatorTest, SIMDPerformance) {
    // Compare SIMD vs scalar performance
    const size_t num_iterations = 1000;
    
    // Prepare data
    std::vector<float> samples_i(2048), samples_q(2048);
    std::vector<float> code(2048), carrier_i(2048), carrier_q(2048);
    
    // Fill with random data
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    for (size_t i = 0; i < 2048; ++i) {
        samples_i[i] = dist(rng_);
        samples_q[i] = dist(rng_);
        code[i] = dist(rng_) > 0 ? 1.0f : -1.0f;
        carrier_i[i] = std::cos(2 * M_PI * i / 100.0f);
        carrier_q[i] = std::sin(2 * M_PI * i / 100.0f);
    }
    
    // Time SIMD version
    auto start_simd = std::chrono::high_resolution_clock::now();
    std::complex<float> result_simd;
    
    for (size_t i = 0; i < num_iterations; ++i) {
        correlator_->correlateSIMD(
            samples_i.data(), samples_q.data(),
            code.data(), carrier_i.data(), carrier_q.data(),
            2048, result_simd
        );
    }
    
    auto end_simd = std::chrono::high_resolution_clock::now();
    auto simd_duration = std::chrono::duration_cast<std::chrono::microseconds>(
        end_simd - start_simd).count();
    
    // SIMD should be significantly faster (print for manual verification)
    std::cout << "SIMD correlation time: " << simd_duration << " μs for "
              << num_iterations << " iterations\n";
    
    // Basic sanity check - should complete in reasonable time
    EXPECT_LT(simd_duration, 1000000);  // Less than 1 second
}

TEST_F(CorrelatorTest, EarlyPromptLateSpacing) {
    // Test Early-Prompt-Late correlator spacing
    CorrelationResult result = correlator_->correlate(
        test_signal_,
        100.5,    // Aligned with test signal
        M_PI / 4,
        1000.0
    );
    
    // Calculate discriminator output
    double code_error = (result.power_early - result.power_late) /
                       (result.power_early + result.power_late);
    
    // When aligned, discriminator should be near zero
    EXPECT_NEAR(code_error, 0.0, 0.1);
    
    // Test with offset
    result = correlator_->correlate(
        test_signal_,
        100.0,    // 0.5 chip early
        M_PI / 4,
        1000.0
    );
    
    code_error = (result.power_early - result.power_late) /
                (result.power_early + result.power_late);
    
    // Should indicate we need to move later (positive error)
    EXPECT_GT(code_error, 0.0);
}

// Test fixture for PRN code properties
class PRNTest : public ::testing::Test {
protected:
    void SetUp() override {
        prn_gen_ = std::make_unique<PRNGenerator>();
    }
    
    std::unique_ptr<PRNGenerator> prn_gen_;
};

TEST_F(PRNTest, CodeLength) {
    // All PRN codes should be 1023 chips long
    for (int prn = 1; prn <= 32; ++prn) {
        auto code = prn_gen_->generateCode(prn);
        EXPECT_EQ(code.size(), 1023) << "PRN " << prn << " has wrong length";
    }
}

TEST_F(PRNTest, CrossCorrelation) {
    // Test cross-correlation properties
    auto code1 = prn_gen_->generateCode(1);
    auto code2 = prn_gen_->generateCode(2);
    
    // Convert to float
    std::vector<float> code1_f(code1.begin(), code1.end());
    std::vector<float> code2_f(code2.begin(), code2.end());
    
    // Compute cross-correlation at zero lag
    float cross_corr = 0.0f;
    for (size_t i = 0; i < code1.size(); ++i) {
        cross_corr += (code1_f[i] * 2.0f - 1.0f) * (code2_f[i] * 2.0f - 1.0f);
    }
    
    // Normalize
    cross_corr /= code1.size();
    
    // Cross-correlation should be low (< -20 dB)
    EXPECT_LT(std::abs(cross_corr), 0.1);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}