#include "midi_handler.h"
#include <alsa/asoundlib.h>
#include <thread>
#include <atomic>
#include <unordered_map>

namespace openamp {

class MidiHandler::Impl {
public:
    snd_seq_t* seq = nullptr;
    int inputPort = -1;
    int outputPort = -1;
    int clientId = -1;

    std::string currentInputDevice;
    std::string currentOutputDevice;

    std::atomic<bool> running{false};
    std::thread pollThread;
    MidiCallback callback;

    std::unordered_map<int, std::function<void(float)>> ccMap;

    void pollLoop() {
        struct pollfd pfd;
        snd_seq_poll_descriptors(seq, &pfd, 1, POLLIN);

        while (running) {
            if (poll(&pfd, 1, 100) > 0) {
                snd_seq_event_t* event = nullptr;
                while (snd_seq_event_input(seq, &event) >= 0) {
                    if (event && callback) {
                        MidiMessage msg;
                        msg.timestamp = static_cast<double>(event->time.time.tv_sec) +
                                       static_cast<double>(event->time.time.tv_nsec) / 1e9;

                        switch (event->type) {
                            case SND_SEQ_EVENT_NOTEON:
                                msg.status = 0x90 | event->data.note.channel;
                                msg.data1 = event->data.note.note;
                                msg.data2 = event->data.note.velocity;
                                break;

                            case SND_SEQ_EVENT_NOTEOFF:
                                msg.status = 0x80 | event->data.note.channel;
                                msg.data1 = event->data.note.note;
                                msg.data2 = event->data.note.velocity;
                                break;

                            case SND_SEQ_EVENT_CONTROLLER:
                                msg.status = 0xB0 | event->data.control.channel;
                                msg.data1 = event->data.control.param;
                                msg.data2 = event->data.control.value;
                                handleCCInternal(msg.data1, msg.data2);
                                break;

                            case SND_SEQ_EVENT_PGMCHANGE:
                                msg.status = 0xC0 | event->data.control.channel;
                                msg.data1 = event->data.control.value;
                                msg.data2 = 0;
                                break;

                            case SND_SEQ_EVENT_PITCHBEND:
                                msg.status = 0xE0 | event->data.control.channel;
                                msg.data1 = event->data.control.value & 0x7F;
                                msg.data2 = (event->data.control.value >> 7) & 0x7F;
                                break;

                            default:
                                snd_seq_free_event(event);
                                continue;
                        }

                        callback(msg);
                    }
                    if (event) snd_seq_free_event(event);
                }
            }
        }
    }

    void handleCCInternal(int cc, int value) {
        auto it = ccMap.find(cc);
        if (it != ccMap.end()) {
            float normalized = static_cast<float>(value) / 127.0f;
            it->second(normalized);
        }
    }
};

MidiHandler::MidiHandler() : impl_(std::make_unique<Impl>()) {}

MidiHandler::~MidiHandler() {
    shutdown();
}

bool MidiHandler::initialize() {
    if (snd_seq_open(&impl_->seq, "default", SND_SEQ_OPEN_DUPLEX, 0) < 0) {
        return false;
    }

    snd_seq_set_client_name(impl_->seq, "OpenAmp");
    impl_->clientId = snd_seq_client_id(impl_->seq);

    // Create input port
    impl_->inputPort = snd_seq_create_simple_port(
        impl_->seq,
        "Input",
        SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
        SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION
    );

    // Create output port
    impl_->outputPort = snd_seq_create_simple_port(
        impl_->seq,
        "Output",
        SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ,
        SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION
    );

    if (impl_->inputPort < 0 || impl_->outputPort < 0) {
        shutdown();
        return false;
    }

    return true;
}

void MidiHandler::shutdown() {
    impl_->running = false;
    if (impl_->pollThread.joinable()) {
        impl_->pollThread.join();
    }

    if (impl_->seq) {
        snd_seq_close(impl_->seq);
        impl_->seq = nullptr;
    }
}

bool MidiHandler::isInitialized() const {
    return impl_->seq != nullptr;
}

std::vector<MidiDevice> MidiHandler::getInputDevices() {
    std::vector<MidiDevice> devices;

    snd_seq_client_info_t* clientInfo;
    snd_seq_port_info_t* portInfo;

    snd_seq_client_info_alloca(&clientInfo);
    snd_seq_port_info_alloca(&portInfo);

    snd_seq_client_info_set_client(clientInfo, -1);

    while (snd_seq_query_next_client(impl_->seq, clientInfo) >= 0) {
        int clientId = snd_seq_client_info_get_client(clientInfo);

        if (clientId == impl_->clientId) continue;

        snd_seq_port_info_set_client(portInfo, clientId);
        snd_seq_port_info_set_port(portInfo, -1);

        while (snd_seq_query_next_port(impl_->seq, portInfo) >= 0) {
            unsigned int caps = snd_seq_port_info_get_capability(portInfo);
            unsigned int type = snd_seq_port_info_get_type(portInfo);

            if ((caps & SND_SEQ_PORT_CAP_READ) &&
                (caps & SND_SEQ_PORT_CAP_SUBS_READ) &&
                (type & SND_SEQ_PORT_TYPE_MIDI_GENERIC)) {

                MidiDevice dev;
                dev.id = std::to_string(clientId) + ":" +
                        std::to_string(snd_seq_port_info_get_port(portInfo));
                dev.name = std::string(snd_seq_client_info_get_name(clientInfo)) +
                          " " + snd_seq_port_info_get_name(portInfo);
                dev.isInput = true;
                dev.isOutput = false;
                devices.push_back(dev);
            }
        }
    }

    return devices;
}

std::vector<MidiDevice> MidiHandler::getOutputDevices() {
    std::vector<MidiDevice> devices;

    snd_seq_client_info_t* clientInfo;
    snd_seq_port_info_t* portInfo;

    snd_seq_client_info_alloca(&clientInfo);
    snd_seq_port_info_alloca(&portInfo);

    snd_seq_client_info_set_client(clientInfo, -1);

    while (snd_seq_query_next_client(impl_->seq, clientInfo) >= 0) {
        int clientId = snd_seq_client_info_get_client(clientInfo);

        if (clientId == impl_->clientId) continue;

        snd_seq_port_info_set_client(portInfo, clientId);
        snd_seq_port_info_set_port(portInfo, -1);

        while (snd_seq_query_next_port(impl_->seq, portInfo) >= 0) {
            unsigned int caps = snd_seq_port_info_get_capability(portInfo);
            unsigned int type = snd_seq_port_info_get_type(portInfo);

            if ((caps & SND_SEQ_PORT_CAP_WRITE) &&
                (caps & SND_SEQ_PORT_CAP_SUBS_WRITE) &&
                (type & SND_SEQ_PORT_TYPE_MIDI_GENERIC)) {

                MidiDevice dev;
                dev.id = std::to_string(clientId) + ":" +
                        std::to_string(snd_seq_port_info_get_port(portInfo));
                dev.name = std::string(snd_seq_client_info_get_name(clientInfo)) +
                          " " + snd_seq_port_info_get_name(portInfo);
                dev.isInput = false;
                dev.isOutput = true;
                devices.push_back(dev);
            }
        }
    }

    return devices;
}

bool MidiHandler::openInput(const std::string& deviceId) {
    size_t colon = deviceId.find(':');
    if (colon == std::string::npos) return false;

    int srcClient = std::stoi(deviceId.substr(0, colon));
    int srcPort = std::stoi(deviceId.substr(colon + 1));

    snd_seq_addr_t src, dest;
    src.client = srcClient;
    src.port = srcPort;
    dest.client = impl_->clientId;
    dest.port = impl_->inputPort;

    snd_seq_port_subscribe_t* subs;
    snd_seq_port_subscribe_alloca(&subs);
    snd_seq_port_subscribe_set_sender(subs, &src);
    snd_seq_port_subscribe_set_dest(subs, &dest);

    if (snd_seq_subscribe_port(impl_->seq, subs) < 0) {
        return false;
    }

    impl_->currentInputDevice = deviceId;

    // Start polling if not already running
    if (!impl_->running) {
        impl_->running = true;
        impl_->pollThread = std::thread(&MidiHandler::Impl::pollLoop, impl_.get());
    }

    return true;
}

bool MidiHandler::openOutput(const std::string& deviceId) {
    size_t colon = deviceId.find(':');
    if (colon == std::string::npos) return false;

    int destClient = std::stoi(deviceId.substr(0, colon));
    int destPort = std::stoi(deviceId.substr(colon + 1));

    snd_seq_addr_t src, dest;
    src.client = impl_->clientId;
    src.port = impl_->outputPort;
    dest.client = destClient;
    dest.port = destPort;

    snd_seq_port_subscribe_t* subs;
    snd_seq_port_subscribe_alloca(&subs);
    snd_seq_port_subscribe_set_sender(subs, &src);
    snd_seq_port_subscribe_set_dest(subs, &dest);

    if (snd_seq_subscribe_port(impl_->seq, subs) < 0) {
        return false;
    }

    impl_->currentOutputDevice = deviceId;
    return true;
}

void MidiHandler::closeInput() {
    impl_->running = false;
    if (impl_->pollThread.joinable()) {
        impl_->pollThread.join();
    }
    impl_->currentInputDevice.clear();
}

void MidiHandler::closeOutput() {
    impl_->currentOutputDevice.clear();
}

void MidiHandler::setCallback(MidiCallback callback) {
    impl_->callback = std::move(callback);
}

void MidiHandler::sendMessage(const MidiMessage& msg) {
    snd_seq_event_t event;
    snd_seq_ev_clear(&event);

    event.source.port = impl_->outputPort;
    event.dest.client = SND_SEQ_ADDRESS_SUBSCRIBERS;
    event.dest.port = SND_SEQ_ADDRESS_UNKNOWN;

    switch (msg.status & 0xF0) {
        case 0x80: // Note Off
            event.type = SND_SEQ_EVENT_NOTEOFF;
            event.data.note.channel = msg.status & 0x0F;
            event.data.note.note = msg.data1;
            event.data.note.velocity = msg.data2;
            break;

        case 0x90: // Note On
            event.type = SND_SEQ_EVENT_NOTEON;
            event.data.note.channel = msg.status & 0x0F;
            event.data.note.note = msg.data1;
            event.data.note.velocity = msg.data2;
            break;

        case 0xB0: // Control Change
            event.type = SND_SEQ_EVENT_CONTROLLER;
            event.data.control.channel = msg.status & 0x0F;
            event.data.control.param = msg.data1;
            event.data.control.value = msg.data2;
            break;

        case 0xC0: // Program Change
            event.type = SND_SEQ_EVENT_PGMCHANGE;
            event.data.control.channel = msg.status & 0x0F;
            event.data.control.value = msg.data1;
            break;

        case 0xE0: // Pitch Bend
            event.type = SND_SEQ_EVENT_PITCHBEND;
            event.data.control.channel = msg.status & 0x0F;
            event.data.control.value = (msg.data2 << 7) | msg.data1;
            break;

        default:
            return;
    }

    snd_seq_ev_set_direct(&event);
    snd_seq_event_output(impl_->seq, &event);
    snd_seq_drain_output(impl_->seq);
}

void MidiHandler::mapCC(int cc, std::function<void(float)> callback) {
    impl_->ccMap[cc] = std::move(callback);
}

void MidiHandler::handleCC(int cc, int value) {
    impl_->handleCCInternal(cc, value);
}

} // namespace openamp
