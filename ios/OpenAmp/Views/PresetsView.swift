import SwiftUI

struct PresetsView: View {
    @EnvironmentObject var audioEngine: AudioEngine
    @State private var presets: [Preset] = []
    @State private var showingSaveSheet = false
    @State private var newPresetName = ""
    @State private var selectedPreset: Preset?
    
    // Factory presets
    let factoryPresets: [(name: String, configure: (inout AudioEngine) -> Void)] = [
        ("Clean Sparkle", { engine in
            engine.inputGainDb = 0; engine.outputGainDb = 0
            engine.ampEnabled = true; engine.effectsEnabled = true
            engine.distortionEnabled = false
            engine.delayEnabled = false; engine.reverbEnabled = true
            engine.reverbRoomSize = 0.35; engine.reverbMix = 0.25
            engine.ampGainDb = -6; engine.ampDrive = 0.35
            engine.ampBassDb = 2; engine.ampTrebleDb = 3; engine.ampPresenceDb = 2
        }),
        ("Edge Crunch", { engine in
            engine.inputGainDb = 2; engine.outputGainDb = 0
            engine.ampEnabled = true; engine.effectsEnabled = true
            engine.distortionEnabled = true; engine.distortionType = 0
            engine.distortionDrive = 0.45; engine.distortionLevel = 0.7
            engine.delayEnabled = false; engine.reverbEnabled = true
            engine.reverbRoomSize = 0.3; engine.reverbMix = 0.2
            engine.ampGainDb = 4; engine.ampDrive = 0.55
            engine.ampBassDb = 1; engine.ampMidDb = 2; engine.ampTrebleDb = 1
        }),
        ("Heavy Lead", { engine in
            engine.inputGainDb = 3; engine.outputGainDb = 0
            engine.ampEnabled = true; engine.effectsEnabled = true
            engine.distortionEnabled = true; engine.distortionType = 2
            engine.distortionDrive = 0.75; engine.distortionLevel = 0.8
            engine.delayEnabled = true; engine.reverbEnabled = true
            engine.delayTimeMs = 360; engine.delayMix = 0.22
            engine.reverbRoomSize = 0.4; engine.reverbMix = 0.22
            engine.ampGainDb = 8; engine.ampDrive = 0.7
            engine.ampBassDb = 3; engine.ampMidDb = 2; engine.ampTrebleDb = 1
        }),
        ("Ambient Clean", { engine in
            engine.inputGainDb = -2; engine.outputGainDb = 0
            engine.ampEnabled = true; engine.effectsEnabled = true
            engine.distortionEnabled = false
            engine.delayEnabled = true; engine.reverbEnabled = true
            engine.delayTimeMs = 480; engine.delayFeedback = 0.45; engine.delayMix = 0.3
            engine.reverbRoomSize = 0.55; engine.reverbMix = 0.35
            engine.ampGainDb = -4; engine.ampDrive = 0.3
            engine.ampBassDb = 1; engine.ampMidDb = -1; engine.ampTrebleDb = 3
        }),
        ("Fuzz Stomp", { engine in
            engine.inputGainDb = 2; engine.outputGainDb = 0
            engine.ampEnabled = true; engine.effectsEnabled = true
            engine.distortionEnabled = true; engine.distortionType = 1
            engine.distortionDrive = 0.85; engine.distortionTone = 0.45; engine.distortionLevel = 0.75
            engine.delayEnabled = false; engine.reverbEnabled = false
            engine.ampGainDb = 6; engine.ampDrive = 0.6
            engine.ampBassDb = 3; engine.ampTrebleDb = -1
        })
    ]
    
    var body: some View {
        NavigationView {
            List {
                // Factory Presets
                Section("Factory Presets") {
                    ForEach(factoryPresets, id: \.name) { preset in
                        Button {
                            applyFactoryPreset(preset)
                        } label: {
                            HStack {
                                Text(preset.name)
                                    .foregroundColor(.primary)
                                Spacer()
                                Image(systemName: "play.circle")
                                    .foregroundColor(.orange)
                            }
                        }
                    }
                }
                
                // User Presets
                Section("My Presets") {
                    ForEach(presets, id: \.name) { preset in
                        Button {
                            loadPreset(preset)
                        } label: {
                            HStack {
                                Text(preset.name)
                                    .foregroundColor(.primary)
                                Spacer()
                                if selectedPreset?.name == preset.name {
                                    Image(systemName: "checkmark")
                                        .foregroundColor(.green)
                                }
                            }
                        }
                        .swipeActions(edge: .trailing, allowsFullSwipe: true) {
                            Button(role: .destructive) {
                                deletePreset(preset)
                            } label: {
                                Label("Delete", systemImage: "trash")
                            }
                        }
                    }
                    
                    if presets.isEmpty {
                        Text("No saved presets")
                            .foregroundColor(.secondary)
                            .italic()
                    }
                }
            }
            .navigationTitle("Presets")
            .toolbar {
                ToolbarItem(placement: .navigationBarTrailing) {
                    Button {
                        showingSaveSheet = true
                    } label: {
                        Image(systemName: "plus")
                    }
                }
            }
            .sheet(isPresented: $showingSaveSheet) {
                NavigationView {
                    Form {
                        Section("Save Preset") {
                            TextField("Preset Name", text: $newPresetName)
                                .textContentType(.name)
                        }
                    }
                    .navigationTitle("New Preset")
                    .toolbar {
                        ToolbarItem(placement: .cancellationAction) {
                            Button("Cancel") {
                                showingSaveSheet = false
                                newPresetName = ""
                            }
                        }
                        ToolbarItem(placement: .confirmationAction) {
                            Button("Save") {
                                saveCurrentPreset()
                                showingSaveSheet = false
                                newPresetName = ""
                            }
                            .disabled(newPresetName.isEmpty)
                        }
                    }
                }
                .presentationDetents([.medium])
            }
        }
        .onAppear {
            loadPresets()
        }
    }
    
    private func loadPresets() {
        presets = PresetManager.shared.loadAllPresets()
    }
    
    private func loadPreset(_ preset: Preset) {
        _ = audioEngine.loadPreset(name: preset.name)
        selectedPreset = preset
    }
    
    private func saveCurrentPreset() {
        _ = audioEngine.savePreset(name: newPresetName)
        loadPresets()
    }
    
    private func deletePreset(_ preset: Preset) {
        PresetManager.shared.deletePreset(name: preset.name)
        loadPresets()
        if selectedPreset?.name == preset.name {
            selectedPreset = nil
        }
    }
    
    private func applyFactoryPreset(_ factoryPreset: (name: String, configure: (inout AudioEngine) -> Void)) {
        var engine = audioEngine
        factoryPreset.configure(&engine)
        selectedPreset = nil
    }
}

#Preview {
    PresetsView()
        .environmentObject(AudioEngine())
}
