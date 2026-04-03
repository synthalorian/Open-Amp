#pragma once

#include <vector>
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace openamp {

/// Minimal radix-2 Cooley-Tukey FFT for overlap-add convolution.
/// Operates on interleaved real/imaginary pairs.
class FFTConvolver {
public:
    /// Forward FFT. Input: real signal of length n. Output: complex (2*n floats, interleaved re/im).
    /// n must be a power of 2.
    static void fft(const float* input, float* output, size_t n) {
        // Pack real input into complex
        for (size_t i = 0; i < n; ++i) {
            output[2 * i] = input[i];
            output[2 * i + 1] = 0.0f;
        }
        fftInPlace(output, n, false);
    }

    /// Inverse FFT. Input/output: complex (2*n floats). Result scaled by 1/n.
    static void ifft(float* data, size_t n) {
        fftInPlace(data, n, true);
        const float scale = 1.0f / static_cast<float>(n);
        for (size_t i = 0; i < 2 * n; ++i) {
            data[i] *= scale;
        }
    }

    /// Complex multiply-accumulate: acc += a * b (complex, interleaved, length 2*n)
    static void complexMultiplyAccumulate(const float* a, const float* b,
                                           float* acc, size_t n) {
        for (size_t i = 0; i < n; ++i) {
            float ar = a[2 * i], ai = a[2 * i + 1];
            float br = b[2 * i], bi = b[2 * i + 1];
            acc[2 * i] += ar * br - ai * bi;
            acc[2 * i + 1] += ar * bi + ai * br;
        }
    }

    /// Returns the smallest power of 2 >= n.
    static size_t nextPow2(size_t n) {
        size_t p = 1;
        while (p < n) p <<= 1;
        return p;
    }

private:
    /// In-place Cooley-Tukey radix-2 DIT FFT on interleaved complex data.
    static void fftInPlace(float* data, size_t n, bool inverse) {
        // Bit-reversal permutation
        for (size_t i = 1, j = 0; i < n; ++i) {
            size_t bit = n >> 1;
            while (j & bit) {
                j ^= bit;
                bit >>= 1;
            }
            j ^= bit;
            if (i < j) {
                std::swap(data[2 * i], data[2 * j]);
                std::swap(data[2 * i + 1], data[2 * j + 1]);
            }
        }

        // Butterfly stages
        const float sign = inverse ? 1.0f : -1.0f;
        for (size_t len = 2; len <= n; len <<= 1) {
            const float angle = sign * 2.0f * 3.14159265358979323846f / static_cast<float>(len);
            const float wRe = std::cos(angle);
            const float wIm = std::sin(angle);

            for (size_t i = 0; i < n; i += len) {
                float curRe = 1.0f, curIm = 0.0f;
                for (size_t j = 0; j < len / 2; ++j) {
                    size_t u = i + j;
                    size_t v = i + j + len / 2;
                    float tRe = curRe * data[2 * v] - curIm * data[2 * v + 1];
                    float tIm = curRe * data[2 * v + 1] + curIm * data[2 * v];
                    data[2 * v] = data[2 * u] - tRe;
                    data[2 * v + 1] = data[2 * u + 1] - tIm;
                    data[2 * u] += tRe;
                    data[2 * u + 1] += tIm;
                    float newRe = curRe * wRe - curIm * wIm;
                    curIm = curRe * wIm + curIm * wRe;
                    curRe = newRe;
                }
            }
        }
    }
};

/// Uniform partitioned overlap-add convolver.
/// Partitions the IR into blocks matching the audio buffer size,
/// then performs frequency-domain multiply-accumulate each block.
class PartitionedConvolver {
public:
    void prepare(const std::vector<float>& ir, size_t blockSize) {
        blockSize_ = blockSize;
        fftSize_ = FFTConvolver::nextPow2(2 * blockSize);

        // Partition IR into blocks of blockSize, FFT each
        numPartitions_ = (ir.size() + blockSize - 1) / blockSize;
        irPartitionsFreq_.resize(numPartitions_);

        std::vector<float> padded(fftSize_, 0.0f);
        for (size_t p = 0; p < numPartitions_; ++p) {
            std::fill(padded.begin(), padded.end(), 0.0f);
            size_t start = p * blockSize;
            size_t count = std::min(blockSize, ir.size() - start);
            std::copy(ir.begin() + start, ir.begin() + start + count, padded.begin());

            irPartitionsFreq_[p].resize(2 * fftSize_);
            FFTConvolver::fft(padded.data(), irPartitionsFreq_[p].data(), fftSize_);
        }

        // Allocate input and overlap buffers
        inputFreqHistory_.resize(numPartitions_);
        for (auto& buf : inputFreqHistory_) {
            buf.assign(2 * fftSize_, 0.0f);
        }
        inputHistoryIdx_ = 0;
        overlapBuffer_.assign(fftSize_, 0.0f);
        fftTemp_.resize(2 * fftSize_);
        accumTemp_.resize(2 * fftSize_);
        prepared_ = true;
    }

    void process(float* data, size_t numFrames) {
        if (!prepared_ || numFrames == 0) return;

        // Pad input to fftSize and FFT
        std::vector<float> padded(fftSize_, 0.0f);
        std::copy(data, data + std::min(numFrames, blockSize_), padded.begin());

        FFTConvolver::fft(padded.data(), inputFreqHistory_[inputHistoryIdx_].data(), fftSize_);

        // Frequency-domain multiply-accumulate across all partitions
        std::fill(accumTemp_.begin(), accumTemp_.end(), 0.0f);
        for (size_t p = 0; p < numPartitions_; ++p) {
            size_t histIdx = (inputHistoryIdx_ - p + numPartitions_) % numPartitions_;
            FFTConvolver::complexMultiplyAccumulate(
                inputFreqHistory_[histIdx].data(),
                irPartitionsFreq_[p].data(),
                accumTemp_.data(),
                fftSize_);
        }

        // IFFT
        FFTConvolver::ifft(accumTemp_.data(), fftSize_);

        // Extract real part and add overlap
        for (size_t i = 0; i < numFrames && i < blockSize_; ++i) {
            data[i] = accumTemp_[2 * i] + overlapBuffer_[i];
        }

        // Store new overlap (the tail beyond blockSize)
        std::fill(overlapBuffer_.begin(), overlapBuffer_.end(), 0.0f);
        for (size_t i = blockSize_; i < fftSize_; ++i) {
            overlapBuffer_[i - blockSize_] = accumTemp_[2 * i];
        }

        inputHistoryIdx_ = (inputHistoryIdx_ + 1) % numPartitions_;
    }

    void reset() {
        for (auto& buf : inputFreqHistory_) {
            std::fill(buf.begin(), buf.end(), 0.0f);
        }
        std::fill(overlapBuffer_.begin(), overlapBuffer_.end(), 0.0f);
        inputHistoryIdx_ = 0;
    }

    bool isPrepared() const { return prepared_; }

private:
    size_t blockSize_ = 0;
    size_t fftSize_ = 0;
    size_t numPartitions_ = 0;
    size_t inputHistoryIdx_ = 0;
    bool prepared_ = false;

    std::vector<std::vector<float>> irPartitionsFreq_;
    std::vector<std::vector<float>> inputFreqHistory_;
    std::vector<float> overlapBuffer_;
    std::vector<float> fftTemp_;
    std::vector<float> accumTemp_;
};

} // namespace openamp
