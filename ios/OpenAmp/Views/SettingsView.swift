import SwiftUI

struct SettingsView: View {
    @EnvironmentObject var audioEngine: AudioEngine
    @State private var showingAbout = false
    
    var body: some View {
        NavigationView {
            List {
                // Audio Settings
                Section("Audio") {
                    HStack {
                        Text("Sample Rate")
                        Spacer()
                        Text("48 kHz")
                            .foregroundColor(.secondary)
                    }
                    
                    HStack {
                        Text("Buffer Size")
                        Spacer()
                        Text("1024")
                            .foregroundColor(.secondary)
                    }
                    
                    Toggle("Low Latency Mode", isOn: .constant(true))
                }
                
                // Input Settings
                Section("Input") {
                    NavigationLink("Input Device") {
                        InputDeviceView()
                    }
                    
                    HStack {
                        Text("Input Level")
                        Spacer()
                        Text("\(audioEngine.inputLevel * 100, specifier: "%.0f")%")
                            .foregroundColor(.secondary)
                    }
                }
                
                // Output Settings
                Section("Output") {
                    NavigationLink("Output Device") {
                        OutputDeviceView()
                    }
                    
                    HStack {
                        Text("Output Level")
                        Spacer()
                        Text("\(audioEngine.outputLevel * 100, specifier: "%.0f")%")
                            .foregroundColor(.secondary)
                    }
                }
                
                // MIDI Settings
                Section("MIDI Control") {
                    NavigationLink("MIDI Devices") {
                        MidiDevicesView()
                    }
                    
                    Toggle("Enable MIDI Control", isOn: .constant(false))
                    
                    HStack {
                        Text("MIDI Channel")
                        Spacer()
                        Text("All")
                            .foregroundColor(.secondary)
                    }
                }
                
                // Recording
                Section("Recording") {
                    Toggle("Enable Recording", isOn: .constant(false))
                    
                    HStack {
                        Text("Recording Format")
                        Spacer()
                        Text("WAV 48kHz/24bit")
                            .foregroundColor(.secondary)
                    }
                    
                    NavigationLink("Recordings") {
                        RecordingsView()
                    }
                }
                
                // Appearance
                Section("Appearance") {
                    Picker("Theme", selection: .constant("system")) {
                        Text("System").tag("system")
                        Text("Light").tag("light")
                        Text("Dark").tag("dark")
                    }
                    
                    Toggle("Show Level Meters", isOn: .constant(true))
                    Toggle("Show Waveform", isOn: .constant(false))
                }
                
                // Data
                Section("Data") {
                    Button("Export Presets") {
                        exportPresets()
                    }
                    
                    Button("Import Presets") {
                        // Import logic
                    }
                    
                    Button("Reset All Settings", role: .destructive) {
                        // Reset logic
                    }
                }
                
                // About
                Section("About") {
                    HStack {
                        Text("Version")
                        Spacer()
                        Text("1.0.0")
                            .foregroundColor(.secondary)
                    }
                    
                    Button("About Guitar Amp") {
                        showingAbout = true
                    }
                    
                    Link("☕ Buy Me a Coffee", destination: URL(string: "https://buymeacoffee.com/synthalorian")!)
                    
                    Link("Privacy Policy", destination: URL(string: "https://example.com/privacy")!)
                    
                    Link("Support", destination: URL(string: "https://example.com/support")!)
                }
            }
            .navigationTitle("Settings")
            .sheet(isPresented: $showingAbout) {
                AboutView()
            }
        }
    }
    
    private func exportPresets() {
        let presets = PresetManager.shared.loadAllPresets()
        guard let data = try? JSONEncoder().encode(presets),
              let string = String(data: data, encoding: .utf8) else {
            return
        }
        
        // Copy to clipboard for now
        UIPasteboard.general.string = string
    }
}

struct InputDeviceView: View {
    var body: some View {
        List {
            Text("Built-in Microphone")
            Text("USB Audio Device")
        }
        .navigationTitle("Input Device")
    }
}

struct OutputDeviceView: View {
    var body: some View {
        List {
            Text("Built-in Speaker")
            Text("Headphones")
            Text("Bluetooth Speaker")
        }
        .navigationTitle("Output Device")
    }
}

struct MidiDevicesView: View {
    var body: some View {
        List {
            Text("No MIDI devices connected")
                .foregroundColor(.secondary)
        }
        .navigationTitle("MIDI Devices")
    }
}

struct RecordingsView: View {
    var body: some View {
        List {
            Text("No recordings yet")
                .foregroundColor(.secondary)
        }
        .navigationTitle("Recordings")
    }
}

struct AboutView: View {
    @Environment(\.dismiss) var dismiss
    
    var body: some View {
        NavigationView {
            ScrollView {
                VStack(spacing: 24) {
                    Image(systemName: "speaker.wave.3.fill")
                        .font(.system(size: 80))
                        .foregroundColor(.orange)
                        .padding(.top, 40)
                    
                    Text("Guitar Amp")
                        .font(.largeTitle)
                        .fontWeight(.bold)
                    
                    Text("Version 1.0.0")
                        .foregroundColor(.secondary)
                    
                    Text("A professional guitar amplifier and effects processor for iOS and Android.")
                        .font(.body)
                        .multilineTextAlignment(.center)
                        .padding(.horizontal, 40)
                    
                    Divider()
                        .padding(.vertical, 20)
                    
                    VStack(alignment: .leading, spacing: 16) {
                        FeatureRow(icon: "waveform", title: "Amp Simulation", description: "Realistic tube amp modeling")
                        FeatureRow(icon: "slider.horizontal.3", title: "Effects", description: "Delay, Reverb, Chorus, and more")
                        FeatureRow(icon: "music.note.list", title: "Presets", description: "Save and recall your favorite sounds")
                        FeatureRow(icon: "tuningfork", title: "Tuner", description: "Chromatic tuner built-in")
                        FeatureRow(icon: "repeat", title: "Looper", description: "Record and loop your playing")
                    }
                    .padding(.horizontal, 20)
                    
                    Spacer()
                    
                    Text("Made with ❤️ by Synthalorian")
                        .font(.caption)
                        .foregroundColor(.secondary)
                        .padding(.bottom, 20)
                    
                    Link(destination: URL(string: "https://buymeacoffee.com/synthalorian")!) {
                        HStack {
                            Text("☕ Buy Me a Coffee")
                                .fontWeight(.bold)
                            Image(systemName: "arrow.up.right.circle.fill")
                        }
                        .padding()
                        .background(Color.orange)
                        .foregroundColor(.white)
                        .cornerRadius(10)
                    }
                    .padding(.bottom, 40)
                }
            }
            .navigationTitle("About")
            .toolbar {
                ToolbarItem(placement: .confirmationAction) {
                    Button("Done") {
                        dismiss()
                    }
                }
            }
        }
    }
}

struct FeatureRow: View {
    let icon: String
    let title: String
    let description: String
    
    var body: some View {
        HStack(spacing: 16) {
            Image(systemName: icon)
                .font(.title2)
                .foregroundColor(.orange)
                .frame(width: 40)
            
            VStack(alignment: .leading, spacing: 2) {
                Text(title)
                    .font(.headline)
                Text(description)
                    .font(.caption)
                    .foregroundColor(.secondary)
            }
        }
    }
}

#Preview {
    SettingsView()
        .environmentObject(AudioEngine())
}
