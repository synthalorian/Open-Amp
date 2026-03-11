#include "cabinet.h"
#include <cmath>
#include <algorithm>

namespace openamp {

CabinetSimulator::CabinetSimulator() = default;

CabinetSimulator::~CabinetSimulator() = default;

void CabinetSimulator::prepare(double sampleRate, uint32_t maxBlockSize) {
    sampleRate_ = sampleRate;
    
    // Initialize input buffer for convolution
    inputBuffer_.resize(maxBlockSize * 4);  // Ring buffer
    std::fill(inputBuffer_.begin(), inputBuffer_.end(), 0.0f);
    inputWritePos_ = 0;
    
    // Setup filters
    lowCutFilter_.setCutoff(lowCutHz_, static_cast<float>(sampleRate));
    highCutFilter_.setCutoff(highCutHz_, static_cast<float>(sampleRate));
    
    // Generate default IR if none loaded
    if (!irLoaded_) {
        generateBuiltinIR(currentType_);
    }
    
    tempBuffer_.resize(maxBlockSize * 2);
}

void CabinetSimulator::reset() {
    std::fill(inputBuffer_.begin(), inputBuffer_.end(), 0.0f);
    inputWritePos_ = 0;
    lowCutFilter_.reset();
    highCutFilter_.reset();
}

void CabinetSimulator::process(AudioBuffer& buffer) {
    if (buffer.numChannels == 0 || buffer.numFrames == 0 || !irLoaded_) return;
    
    for (uint32_t ch = 0; ch < buffer.numChannels; ++ch) {
        float* channelData = buffer.data + ch * buffer.numFrames;
        
        // Apply pre-IR low cut
        for (uint32_t i = 0; i < buffer.numFrames; ++i) {
            channelData[i] = lowCutFilter_.process(channelData[i]);
        }
        
        // Convolution
        processConvolution(channelData, buffer.numFrames, 1);
        
        // Apply post-IR high cut and output gain
        for (uint32_t i = 0; i < buffer.numFrames; ++i) {
            channelData[i] = highCutFilter_.process(channelData[i]);
            channelData[i] *= outputGainLinear_;
        }
    }
}

void CabinetSimulator::processConvolution(float* data, uint32_t numFrames, uint32_t) {
    if (irData_.empty() || inputBuffer_.empty()) return;
    
    for (uint32_t i = 0; i < numFrames; ++i) {
        // Store input in ring buffer
        inputBuffer_[inputWritePos_] = data[i];
        
        // Perform convolution
        float sum = 0.0f;
        uint32_t bufSize = static_cast<uint32_t>(inputBuffer_.size());
        
        for (uint32_t j = 0; j < irLength_ && j < bufSize; ++j) {
            uint32_t readPos = (inputWritePos_ + bufSize - j) % bufSize;
            sum += inputBuffer_[readPos] * irData_[j];
        }
        
        // Advance write position
        inputWritePos_ = (inputWritePos_ + 1) % bufSize;
        
        // Mix dry/wet
        data[i] = data[i] * (1.0f - mix_) + sum * mix_;
    }
}

void CabinetSimulator::generateBuiltinIR(CabinetType type) {
    // Generate synthetic impulse responses for built-in cabinets
    // These are simplified approximations of real cabinet characteristics
    
    irLength_ = static_cast<uint32_t>(sampleRate_ * 0.05);  // 50ms IR length
    irData_.resize(irLength_);
    
    switch (type) {
        case CabinetType::Marshall4x12: {
            // Classic Marshall: aggressive midrange, tight low end
            for (uint32_t i = 0; i < irLength_; ++i) {
                float t = static_cast<float>(i) / sampleRate_;
                float env = std::exp(-t * 40.0f);
                
                // Main impulse
                float sample = env;
                
                // Add some early reflections
                if (t > 0.001f && t < 0.003f) {
                    sample += env * 0.3f * std::sin(t * 2000.0f * 3.14159f);
                }
                
                // Mid boost characteristic
                sample *= (1.0f + 0.5f * std::sin(t * 500.0f));
                
                irData_[i] = sample;
            }
            irName_ = "Marshall 4x12";
            break;
        }
        
        case CabinetType::Fender2x12: {
            // Fender: scooped mids, bright, clean
            for (uint32_t i = 0; i < irLength_; ++i) {
                float t = static_cast<float>(i) / sampleRate_;
                float env = std::exp(-t * 30.0f);
                float sample = env;
                
                // Bright characteristic
                if (t < 0.002f) {
                    sample += env * 0.4f;
                }
                
                irData_[i] = sample;
            }
            irName_ = "Fender 2x12";
            break;
        }
        
        case CabinetType::VoxAC30: {
            // Vox: chimey, bright, jangly
            for (uint32_t i = 0; i < irLength_; ++i) {
                float t = static_cast<float>(i) / sampleRate_;
                float env = std::exp(-t * 35.0f);
                float sample = env;
                
                // Add chime
                sample += env * 0.2f * std::sin(t * 3000.0f * 3.14159f);
                
                irData_[i] = sample;
            }
            irName_ = "Vox AC30";
            break;
        }
        
        case CabinetType::MesaBoogie4x12: {
            // Mesa: thick, high gain, tight
            for (uint32_t i = 0; i < irLength_; ++i) {
                float t = static_cast<float>(i) / sampleRate_;
                float env = std::exp(-t * 50.0f);
                float sample = env;
                
                // Thick low-mid
                sample *= (1.0f + 0.4f * std::sin(t * 300.0f));
                
                irData_[i] = sample;
            }
            irName_ = "Mesa Boogie 4x12";
            break;
        }
        
        case CabinetType::Orange4x12: {
            // Orange: modern, thick, heavy
            for (uint32_t i = 0; i < irLength_; ++i) {
                float t = static_cast<float>(i) / sampleRate_;
                float env = std::exp(-t * 45.0f);
                float sample = env * 1.2f;
                
                irData_[i] = sample;
            }
            irName_ = "Orange 4x12";
            break;
        }
        
        case CabinetType::TwinReverb: {
            // Fender Twin: very clean, lots of headroom
            for (uint32_t i = 0; i < irLength_; ++i) {
                float t = static_cast<float>(i) / sampleRate_;
                float env = std::exp(-t * 25.0f);
                float sample = env;
                
                // Bright cap simulation
                if (t < 0.003f) {
                    sample += env * 0.3f * (1.0f - t / 0.003f);
                }
                
                irData_[i] = sample;
            }
            irName_ = "Fender Twin Reverb";
            break;
        }
        
        case CabinetType::Greenback: {
            // Celestion Greenback: warm, classic rock
            for (uint32_t i = 0; i < irLength_; ++i) {
                float t = static_cast<float>(i) / sampleRate_;
                float env = std::exp(-t * 35.0f);
                float sample = env;
                
                // Warmth
                sample *= (1.0f - 0.1f * std::sin(t * 200.0f));
                
                irData_[i] = sample;
            }
            irName_ = "Greenback 4x12";
            break;
        }
        
        case CabinetType::Vintage30: {
            // Celestion V30: modern standard, balanced
            for (uint32_t i = 0; i < irLength_; ++i) {
                float t = static_cast<float>(i) / sampleRate_;
                float env = std::exp(-t * 40.0f);
                float sample = env;
                
                // Presence peak
                if (t > 0.0005f && t < 0.004f) {
                    sample += env * 0.15f * std::sin(t * 4000.0f * 3.14159f);
                }
                
                irData_[i] = sample;
            }
            irName_ = "Vintage 30 4x12";
            break;
        }
        
        case CabinetType::AcousticSim: {
            // Acoustic simulation: simulates piezo to magnetic conversion
            for (uint32_t i = 0; i < irLength_; ++i) {
                float t = static_cast<float>(i) / sampleRate_;
                float env = std::exp(-t * 60.0f);
                float sample = env;
                
                // Remove piezo quack
                sample *= (1.0f - 0.3f * std::sin(t * 1000.0f));
                
                // Add warmth
                sample += env * 0.2f * (1.0f - t / 0.05f);
                
                irData_[i] = sample;
            }
            irName_ = "Acoustic Sim";
            break;
        }
        
        default:
            // Default: flat response
            for (uint32_t i = 0; i < irLength_; ++i) {
                float t = static_cast<float>(i) / sampleRate_;
                irData_[i] = std::exp(-t * 50.0f);
            }
            irName_ = "Default";
            break;
    }
    
    // Normalize IR
    float maxAbs = 0.001f;
    for (float sample : irData_) {
        maxAbs = std::max(maxAbs, std::abs(sample));
    }
    for (float& sample : irData_) {
        sample /= maxAbs;
    }
    
    irLoaded_ = true;
    currentType_ = type;
}

bool CabinetSimulator::loadIR(const std::string& path) {
    // TODO: Implement WAV file loading for custom IRs
    // For now, just mark as custom and use generated
    (void)path;
    currentType_ = CabinetType::Custom;
    irName_ = "Custom IR";
    return false;  // Not yet implemented
}

void CabinetSimulator::setCabinetType(CabinetType type) {
    if (type != currentType_) {
        generateBuiltinIR(type);
    }
}

void CabinetSimulator::setMix(float amount) {
    mix_ = std::clamp(amount, 0.0f, 1.0f);
}

void CabinetSimulator::setLowCut(float hz) {
    lowCutHz_ = std::clamp(hz, 20.0f, 500.0f);
    lowCutFilter_.setCutoff(lowCutHz_, static_cast<float>(sampleRate_));
}

void CabinetSimulator::setHighCut(float hz) {
    highCutHz_ = std::clamp(hz, 2000.0f, 16000.0f);
    highCutFilter_.setCutoff(highCutHz_, static_cast<float>(sampleRate_));
}

void CabinetSimulator::setOutputGain(float db) {
    outputGainDb_ = db;
    outputGainLinear_ = std::pow(10.0f, db / 20.0f);
}

} // namespace openamp
