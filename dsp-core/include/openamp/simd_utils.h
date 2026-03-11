#pragma once

// SIMD utilities for audio processing
// Supports SSE, AVX, and NEON

#include <cstdint>
#include <cmath>

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #include <immintrin.h>
    #define HAS_SSE 1
    #if defined(__AVX__)
        #define HAS_AVX 1
    #endif
#elif defined(__aarch64__) || defined(__arm__)
    #include <arm_neon.h>
    #define HAS_NEON 1
#endif

namespace openamp {
namespace simd {

// ============== SIMD Types ==============

#if defined(HAS_AVX)
    using v8f = __m256;
    using v4f = __m128;
    constexpr size_t V8F_SIZE = 8;
    constexpr size_t V4F_SIZE = 4;
#elif defined(HAS_SSE)
    using v4f = __m128;
    constexpr size_t V4F_SIZE = 4;
#elif defined(HAS_NEON)
    using v4f = float32x4_t;
    constexpr size_t V4F_SIZE = 4;
#endif

// ============== Load/Store ==============

#if defined(HAS_SSE) || defined(HAS_AVX)
    inline v4f load(const float* p) { return _mm_loadu_ps(p); }
    inline void store(float* p, v4f v) { _mm_storeu_ps(p, v); }
    inline v4f set1(float f) { return _mm_set1_ps(f); }
    inline v4f setzero() { return _mm_setzero_ps(); }
    
    inline v4f add(v4f a, v4f b) { return _mm_add_ps(a, b); }
    inline v4f sub(v4f a, v4f b) { return _mm_sub_ps(a, b); }
    inline v4f mul(v4f a, v4f b) { return _mm_mul_ps(a, b); }
    inline v4f div(v4f a, v4f b) { return _mm_div_ps(a, b); }
    
    inline v4f max(v4f a, v4f b) { return _mm_max_ps(a, b); }
    inline v4f min(v4f a, v4f b) { return _mm_min_ps(a, b); }
    
    // Approximate reciprocal
    inline v4f rcp(v4f v) { return _mm_rcp_ps(v); }
    
    // Approximate reciprocal square root
    inline v4f rsqrt(v4f v) { return _mm_rsqrt_ps(v); }
    
    // Square root
    inline v4f sqrt(v4f v) { return _mm_sqrt_ps(v); }
    
    // Fused multiply-add (AVX2 or FMA)
    #if defined(__FMA__)
        inline v4f fmadd(v4f a, v4f b, v4f c) { return _mm_fmadd_ps(a, b, c); }
    #else
        inline v4f fmadd(v4f a, v4f b, v4f c) { return _mm_add_ps(_mm_mul_ps(a, b), c); }
    #endif
    
#elif defined(HAS_NEON)
    inline v4f load(const float* p) { return vld1q_f32(p); }
    inline void store(float* p, v4f v) { vst1q_f32(p, v); }
    inline v4f set1(float f) { return vdupq_n_f32(f); }
    inline v4f setzero() { return vdupq_n_f32(0.0f); }
    
    inline v4f add(v4f a, v4f b) { return vaddq_f32(a, b); }
    inline v4f sub(v4f a, v4f b) { return vsubq_f32(a, b); }
    inline v4f mul(v4f a, v4f b) { return vmulq_f32(a, b); }
    inline v4f div(v4f a, v4f b) { 
        // NEON doesn't have div, use reciprocal approximation
        float32x4_t recip = vrecpeq_f32(b);
        recip = vmulq_f32(recip, vrecpsq_f32(b, recip));
        return vmulq_f32(a, recip);
    }
    
    inline v4f max(v4f a, v4f b) { return vmaxq_f32(a, b); }
    inline v4f min(v4f a, v4f b) { return vminq_f32(a, b); }
    
    inline v4f rcp(v4f v) { return vrecpeq_f32(v); }
    inline v4f rsqrt(v4f v) { return vrsqrteq_f32(v); }
    
    // sqrt - vsqrtq_f32 only available on ARM64
    #if defined(__aarch64__)
        inline v4f sqrt(v4f v) { return vsqrtq_f32(v); }
    #else
        inline v4f sqrt(v4f v) { 
            // Fallback: use rsqrt and multiply for 32-bit ARM
            float32x4_t rsq = vrsqrteq_f32(v);
            // Newton-Raphson refinement
            rsq = vmulq_f32(rsq, vrsqrtsq_f32(vmulq_f32(v, rsq), rsq));
            return vmulq_f32(v, rsq);
        }
    #endif
    
    inline v4f fmadd(v4f a, v4f b, v4f c) { return vmlaq_f32(c, a, b); }
#endif

// ============== SIMD Operations ==============

// Tanh approximation for SIMD
inline v4f tanh_approx(v4f x) {
    // Fast tanh approximation: tanh(x) ≈ x * (27 + x²) / (27 + 9x²)
    v4f x2 = mul(x, x);
    v4f nine = set1(9.0f);
    v4f twentyseven = set1(27.0f);
    
    v4f num = mul(x, add(twentyseven, x2));
    v4f denom = add(twentyseven, mul(nine, x2));
    
    return div(num, denom);
}

// Soft clip for SIMD
inline v4f soft_clip(v4f x, float threshold) {
    v4f t = set1(threshold);
    v4f one = set1(1.0f);
    
    // tanh approximation for soft clipping
    v4f scaled = div(x, t);
    return mul(t, tanh_approx(scaled));
}

// Hard clip for SIMD
inline v4f hard_clip(v4f x, float threshold) {
    v4f t = set1(threshold);
    v4f negT = set1(-threshold);
    return max(min(x, t), negT);
}

// Absolute value
inline v4f abs(v4f x) {
#if defined(HAS_SSE) || defined(HAS_AVX)
    __m128 mask = _mm_castsi128_ps(_mm_set1_epi32(0x7FFFFFFF));
    return _mm_and_ps(x, mask);
#elif defined(HAS_NEON)
    return vabsq_f32(x);
#endif
}

// Sum all elements (horizontal sum)
inline float sum(v4f v) {
#if defined(HAS_SSE) || defined(HAS_AVX)
    // Extract and add
    float result[4];
    store(result, v);
    return result[0] + result[1] + result[2] + result[3];
#elif defined(HAS_NEON)
    float32x2_t sum = vadd_f32(vget_low_f32(v), vget_high_f32(v));
    return vget_lane_f32(sum, 0) + vget_lane_f32(sum, 1);
#endif
}

// ============== Process Blocks ==============

// Apply gain to a block of samples
inline void apply_gain(float* data, uint32_t numSamples, float gain) {
    v4f g = set1(gain);
    uint32_t i = 0;
    
    // Process 4 samples at a time
    for (; i + 4 <= numSamples; i += 4) {
        v4f samples = load(data + i);
        store(data + i, mul(samples, g));
    }
    
    // Process remaining samples
    for (; i < numSamples; ++i) {
        data[i] *= gain;
    }
}

// Mix two buffers: out = a * mixA + b * mixB
inline void mix(float* out, const float* a, const float* b, 
                uint32_t numSamples, float mixA, float mixB) {
    v4f ma = set1(mixA);
    v4f mb = set1(mixB);
    uint32_t i = 0;
    
    for (; i + 4 <= numSamples; i += 4) {
        v4f va = load(a + i);
        v4f vb = load(b + i);
        v4f mixed = add(mul(va, ma), mul(vb, mb));
        store(out + i, mixed);
    }
    
    for (; i < numSamples; ++i) {
        out[i] = a[i] * mixA + b[i] * mixB;
    }
}

// Calculate RMS level
inline float calculate_rms(const float* data, uint32_t numSamples) {
    if (numSamples == 0) return 0.0f;
    
    v4f sumVec = setzero();
    uint32_t i = 0;
    
    for (; i + 4 <= numSamples; i += 4) {
        v4f samples = load(data + i);
        sumVec = add(sumVec, mul(samples, samples));
    }
    
    float sumVal = sum(sumVec);
    
    // Remaining samples
    for (; i < numSamples; ++i) {
        sumVal += data[i] * data[i];
    }
    
    return std::sqrt(sumVal / numSamples);
}

// Soft clip an entire buffer
inline void soft_clip_buffer(float* data, uint32_t numSamples, float threshold = 1.0f) {
    v4f t = set1(threshold);
    uint32_t i = 0;
    
    for (; i + 4 <= numSamples; i += 4) {
        v4f samples = load(data + i);
        v4f clipped = soft_clip(samples, threshold);
        store(data + i, clipped);
    }
    
    for (; i < numSamples; ++i) {
        if (data[i] > threshold) {
            data[i] = threshold * std::tanh(data[i] / threshold);
        } else if (data[i] < -threshold) {
            data[i] = -threshold * std::tanh(-data[i] / threshold);
        }
    }
}

} // namespace simd
} // namespace openamp
