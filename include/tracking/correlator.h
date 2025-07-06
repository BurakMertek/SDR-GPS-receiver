#ifndef CORRELATOR_H
#define CORRELATOR_H

#include <immintrin.h>
#include <complex>
#include <vector>
#include "utils/gps_constants.h"

namespace gps {


struct CorrelationResult {
    std::complex<float> early;
    std::complex<float> prompt;
    std::complex<float> late;
    float power_early;
    float power_prompt;
    float power_late;
};

class Correlator {
public:
    Correlator(int prn, double sample_rate);
    ~Correlator() = default;

    
    CorrelationResult correlate(const IQBuffer& samples,
                               double code_phase,
                               double carrier_phase,
                               double carrier_freq);

    // SIMD-optimized correlation
    void correlateSIMD(const float* samples_i,
                      const float* samples_q,
                      const float* code,
                      const float* carrier_i,
                      const float* carrier_q,
                      size_t length,
                      std::complex<float>& result);

private:
    
    void generatePRNCode(int prn, std::vector<float>& code);
    
    
    void generateCarrier(double phase, double freq, size_t length,
                        std::vector<float>& carrier_i,
                        std::vector<float>& carrier_q);

    
    int prn_;
    double sample_rate_;
    std::vector<float> prn_code_;
    
    // Pre-allocated buffers for performance
    std::vector<float> carrier_i_;
    std::vector<float> carrier_q_;
    std::vector<float> code_early_;
    std::vector<float> code_prompt_;
    std::vector<float> code_late_;
};

// Inline SIMD correlation function for maximum performance
inline void correlateAVX(const float* __restrict__ samples_i,
                        const float* __restrict__ samples_q,
                        const float* __restrict__ code,
                        const float* __restrict__ carrier_i,
                        const float* __restrict__ carrier_q,
                        size_t length,
                        float& corr_i,
                        float& corr_q) {
    __m256 sum_i = _mm256_setzero_ps();
    __m256 sum_q = _mm256_setzero_ps();

    
    size_t simd_length = length & ~7;  
    
    for (size_t i = 0; i < simd_length; i += 8) {
        
        __m256 samp_i = _mm256_loadu_ps(&samples_i[i]);
        __m256 samp_q = _mm256_loadu_ps(&samples_q[i]);
        
        
        __m256 code_vec = _mm256_loadu_ps(&code[i]);
        
        
        __m256 carr_i = _mm256_loadu_ps(&carrier_i[i]);
        __m256 carr_q = _mm256_loadu_ps(&carrier_q[i]);
        
        // Complex multiplication: (samp_i + j*samp_q) * (carr_i - j*carr_q) * code
        // Real part: (samp_i * carr_i + samp_q * carr_q) * code
        __m256 real = _mm256_mul_ps(samp_i, carr_i);
        real = _mm256_fmadd_ps(samp_q, carr_q, real);
        real = _mm256_mul_ps(real, code_vec);
        
        // Imaginary part: (samp_q * carr_i - samp_i * carr_q) * code
        __m256 imag = _mm256_mul_ps(samp_q, carr_i);
        imag = _mm256_fnmadd_ps(samp_i, carr_q, imag);
        imag = _mm256_mul_ps(imag, code_vec);
        
        // Accumulate
        sum_i = _mm256_add_ps(sum_i, real);
        sum_q = _mm256_add_ps(sum_q, imag);
    }
    
    // Horizontal sum
    __m256 temp = _mm256_hadd_ps(sum_i, sum_q);
    temp = _mm256_hadd_ps(temp, temp);
    __m128 hi = _mm256_extractf128_ps(temp, 1);
    __m128 lo = _mm256_castps256_ps128(temp);
    __m128 result = _mm_add_ps(hi, lo);
    
    corr_i = _mm_cvtss_f32(result);
    corr_q = _mm_cvtss_f32(_mm_shuffle_ps(result, result, 1));
    
    // Handle remaining samples
    for (size_t i = simd_length; i < length; ++i) {
        float mix_i = samples_i[i] * carrier_i[i] + samples_q[i] * carrier_q[i];
        float mix_q = samples_q[i] * carrier_i[i] - samples_i[i] * carrier_q[i];
        corr_i += mix_i * code[i];
        corr_q += mix_q * code[i];
    }
}

}

#endif 