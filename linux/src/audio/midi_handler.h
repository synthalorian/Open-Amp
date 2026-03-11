#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <cstdint>

namespace openamp {

struct MidiDevice {
    std::string id;
    std::string name;
    bool isInput;
    bool isOutput;
};

struct MidiMessage {
    uint8_t status;
    uint8_t data1;
    uint8_t data2;
    double timestamp;
};

using MidiCallback = std::function<void(const MidiMessage&)>;

class MidiHandler {
public:
    MidiHandler();
    ~MidiHandler();

    bool initialize();
    void shutdown();
    bool isInitialized() const;

    std::vector<MidiDevice> getInputDevices();
    std::vector<MidiDevice> getOutputDevices();

    bool openInput(const std::string& deviceId);
    bool openOutput(const std::string& deviceId);
    void closeInput();
    void closeOutput();

    void setCallback(MidiCallback callback);
    void sendMessage(const MidiMessage& msg);

    // Common MIDI controls
    void mapCC(int cc, std::function<void(float)> callback);
    void handleCC(int cc, int value);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace openamp
