#include "jack_backend.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <jack/metadata.h>

namespace openamp {

JackBackend::JackBackend() = default;

JackBackend::~JackBackend() {
    shutdown();
}

bool JackBackend::initialize() {
    jack_options_t options = JackNoStartServer;
    jack_status_t status;

    client_ = jack_client_open("OpenAmp", options, &status);
    if (!client_) {
        lastError_ = "Failed to open JACK client. Is PipeWire-JACK running?";
        return false;
    }

    jack_set_process_callback(client_, JackBackend::processCallback, this);
    jack_on_shutdown(client_, JackBackend::shutdownCallback, this);

    inputPort_ = jack_port_register(client_, "input", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
    outputPorts_[0] = jack_port_register(client_, "output_L", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    outputPorts_[1] = jack_port_register(client_, "output_R", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

    if (!inputPort_ || !outputPorts_[0] || !outputPorts_[1]) {
        lastError_ = "Failed to register JACK ports";
        jack_client_close(client_);
        client_ = nullptr;
        return false;
    }

    config_.sampleRate = jack_get_sample_rate(client_);
    config_.bufferSize = jack_get_buffer_size(client_);

    initialized_ = true;
    return true;
}

void JackBackend::shutdown() {
    stop();
    if (client_) {
        jack_client_close(client_);
        client_ = nullptr;
    }
    initialized_ = false;
}

bool JackBackend::isInitialized() const {
    return initialized_;
}

std::vector<AudioDevice> JackBackend::getInputDevices() {
    std::vector<AudioDevice> devices;
    if (!client_) return devices;

    // Get all capture ports (outputs of other nodes that we can capture from)
    const char** ports = jack_get_ports(client_, NULL, NULL, JackPortIsOutput);
    if (ports) {
        for (int i = 0; ports[i]; ++i) {
            AudioDevice dev;
            dev.id = ports[i];
            dev.name = ports[i];
            
            // In PipeWire-JACK, physical ports are usually system:capture_X
            // We want to show EVERYTHING so you can route from anywhere
            bool isPhysical = false;
            jack_port_t* p = jack_port_by_name(client_, ports[i]);
            if (p) {
                int flags = jack_port_flags(p);
                isPhysical = (flags & JackPortIsPhysical);
                
                jack_uuid_t uuid = jack_port_uuid(p);
                char* pretty_name = nullptr;
                jack_get_property(uuid, JACK_METADATA_PRETTY_NAME, &pretty_name, nullptr);
                if (pretty_name) {
                    dev.name = std::string(pretty_name) + " (" + ports[i] + ")";
                    free(pretty_name);
                }
            }

            dev.isInput = true;
            dev.isDefault = (std::string(ports[i]).find("system:capture_1") != std::string::npos);
            devices.push_back(dev);
        }
        free(ports);
    }
    return devices;
}

std::vector<AudioDevice> JackBackend::getOutputDevices() {
    std::vector<AudioDevice> devices;
    if (!client_) return devices;

    // Get all playback ports (inputs of other nodes we can play to)
    const char** ports = jack_get_ports(client_, NULL, NULL, JackPortIsInput);
    if (ports) {
        for (int i = 0; ports[i]; ++i) {
            // Filter out our own input port
            if (std::string(ports[i]).find("OpenAmp:input") != std::string::npos) continue;

            AudioDevice dev;
            dev.id = ports[i];
            dev.name = ports[i];
            
            jack_port_t* p = jack_port_by_name(client_, ports[i]);
            if (p) {
                jack_uuid_t uuid = jack_port_uuid(p);
                char* pretty_name = nullptr;
                jack_get_property(uuid, JACK_METADATA_PRETTY_NAME, &pretty_name, nullptr);
                if (pretty_name) {
                    dev.name = std::string(pretty_name) + " (" + ports[i] + ")";
                    free(pretty_name);
                }
            }

            dev.isInput = false;
            dev.isDefault = (std::string(ports[i]).find("system:playback_1") != std::string::npos);
            devices.push_back(dev);
        }
        free(ports);
    }
    return devices;
}

AudioDevice JackBackend::getDefaultInputDevice() { 
    auto devs = getInputDevices();
    for(const auto& d : devs) if(d.isDefault) return d;
    return devs.empty() ? AudioDevice{"", "None", true, false, 0, 0} : devs[0];
}

AudioDevice JackBackend::getDefaultOutputDevice() { 
    auto devs = getOutputDevices();
    for(const auto& d : devs) if(d.isDefault) return d;
    return devs.empty() ? AudioDevice{"", "None", false, false, 0, 0} : devs[0];
}

bool JackBackend::setConfig(const AudioConfig& config) {
    config_ = config;
    return true;
}

AudioConfig JackBackend::getConfig() const {
    return config_;
}

bool JackBackend::start(AudioCallback callback) {
    if (!client_) return false;
    callback_ = callback;
    
    if (jack_activate(client_)) {
        lastError_ = "Failed to activate JACK client";
        return false;
    }

    // Connect specified input or try default
    std::string targetIn = config_.inputDeviceId;
    if (!targetIn.empty()) {
        // Only attempt connection if it's not a generic placeholder
        if (targetIn != "jack_input") {
            jack_connect(client_, targetIn.c_str(), jack_port_name(inputPort_));
        }
    }

    // Connect specified output or try defaults
    std::string targetOut = config_.outputDeviceId;
    if (!targetOut.empty() && targetOut != "jack_output") {
        jack_connect(client_, jack_port_name(outputPorts_[0]), targetOut.c_str());
        
        // Logical neighbor for stereo
        std::string rightChannel = targetOut;
        if (rightChannel.find("FL") != std::string::npos) {
            size_t pos = rightChannel.find("FL");
            rightChannel.replace(pos, 2, "FR");
            jack_connect(client_, jack_port_name(outputPorts_[1]), rightChannel.c_str());
        } else if (rightChannel.find("playback_1") != std::string::npos) {
            size_t pos = rightChannel.find("playback_1");
            rightChannel.replace(pos, 10, "playback_2");
            jack_connect(client_, jack_port_name(outputPorts_[1]), rightChannel.c_str());
        } else {
            jack_connect(client_, jack_port_name(outputPorts_[1]), targetOut.c_str());
        }
    }

    running_ = true;
    return true;
}

void JackBackend::stop() {
    if (client_ && running_) {
        jack_deactivate(client_);
    }
    running_ = false;
}

bool JackBackend::isRunning() const {
    return running_;
}

double JackBackend::getInputLatency() const { return 0; }
double JackBackend::getOutputLatency() const { return 0; }

int JackBackend::processCallback(jack_nframes_t nframes, void* arg) {
    auto* backend = static_cast<JackBackend*>(arg);
    if (!backend || !backend->callback_) return 0;

    float* in = (float*)jack_port_get_buffer(backend->inputPort_, nframes);
    float* outL = (float*)jack_port_get_buffer(backend->outputPorts_[0], nframes);
    float* outR = (float*)jack_port_get_buffer(backend->outputPorts_[1], nframes);

    if (!in || !outL || !outR) return 0;

    static float stereoOut[16384]; 
    
    backend->callback_(in, stereoOut, nframes);

    float inMax = 0, outLMax = 0, outRMax = 0;
    for (uint32_t i = 0; i < nframes; ++i) {
        outL[i] = stereoOut[i * 2];
        outR[i] = stereoOut[i * 2 + 1];
        
        if (std::abs(in[i]) > inMax) inMax = std::abs(in[i]);
        if (std::abs(outL[i]) > outLMax) outLMax = std::abs(outL[i]);
        if (std::abs(outR[i]) > outRMax) outRMax = std::abs(outR[i]);
    }

    backend->inputLevel_.store(backend->linearToDb(inMax));
    backend->outputLevelLeft_.store(backend->linearToDb(outLMax));
    backend->outputLevelRight_.store(backend->linearToDb(outRMax));

    return 0;
}

void JackBackend::shutdownCallback(void* arg) {
    auto* backend = static_cast<JackBackend*>(arg);
    if(backend) backend->running_ = false;
}

float JackBackend::linearToDb(float linear) {
    if (linear <= 0.00001f) return -60.0f;
    return 20.0f * std::log10(linear);
}

MeterLevels JackBackend::getInputLevels() const {
    MeterLevels l;
    l.leftDb = inputLevel_.load();
    l.leftPeakDb = l.leftDb;
    return l;
}

MeterLevels JackBackend::getOutputLevels() const {
    MeterLevels l;
    l.leftDb = outputLevelLeft_.load();
    l.rightDb = outputLevelRight_.load();
    l.leftPeakDb = l.leftDb;
    l.rightPeakDb = l.rightDb;
    return l;
}

void JackBackend::resetPeakHold() {}

} // namespace openamp
