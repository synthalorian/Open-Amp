import SwiftUI

struct AmpView: View {
    @EnvironmentObject var audioEngine: AudioEngine
    @State private var selectedSlot: String? = nil
    
    let ampSlots = ["A", "B", "C"]
    
    var body: some View {
        NavigationView {
            ScrollView {
                VStack(spacing: 20) {
                    // Amp Enable Toggle
                    HStack {
                        Text("Amp Simulator")
                            .font(.headline)
                        Spacer()
                        Toggle("", isOn: $audioEngine.ampEnabled)
                            .labelsHidden()
                    }
                    .padding(.horizontal)
                    .padding(.top)
                    
                    // Amp Slots
                    HStack(spacing: 12) {
                        ForEach(ampSlots, id: \.self) { slot in
                            AmpSlotButton(
                                slot: slot,
                                isSelected: selectedSlot == slot
                            ) {
                                loadSlot(slot)
                            }
                            .onLongPressGesture {
                                saveSlot(slot)
                            }
                        }
                    }
                    .padding(.horizontal)
                    
                    Divider()
                    
                    // Amp Controls
                    VStack(spacing: 16) {
                        // Gain and Drive
                        HStack(spacing: 20) {
                            KnobView(
                                label: "Gain",
                                value: $audioEngine.ampGainDb,
                                range: -20...20,
                                unit: "dB"
                            )
                            
                            KnobView(
                                label: "Drive",
                                value: $audioEngine.ampDrive,
                                range: 0...1,
                                unit: "%",
                                percentage: true
                            )
                        }
                        
                        // EQ Section
                        VStack(spacing: 12) {
                            Text("Tone Stack")
                                .font(.subheadline)
                                .foregroundColor(.secondary)
                            
                            HStack(spacing: 16) {
                                KnobView(
                                    label: "Bass",
                                    value: $audioEngine.ampBassDb,
                                    range: -12...12,
                                    unit: "dB"
                                )
                                
                                KnobView(
                                    label: "Mid",
                                    value: $audioEngine.ampMidDb,
                                    range: -12...12,
                                    unit: "dB"
                                )
                                
                                KnobView(
                                    label: "Treble",
                                    value: $audioEngine.ampTrebleDb,
                                    range: -12...12,
                                    unit: "dB"
                                )
                            }
                            
                            HStack(spacing: 16) {
                                KnobView(
                                    label: "Presence",
                                    value: $audioEngine.ampPresenceDb,
                                    range: -12...12,
                                    unit: "dB"
                                )
                                
                                KnobView(
                                    label: "Master",
                                    value: $audioEngine.ampMasterDb,
                                    range: -10...10,
                                    unit: "dB"
                                )
                            }
                        }
                        
                        Divider()
                        
                        // Input/Output Gain
                        HStack(spacing: 20) {
                            VStack {
                                Text("Input")
                                    .font(.caption)
                                    .foregroundColor(.secondary)
                                Slider(value: $audioEngine.inputGainDb, in: -24...24)
                                    .frame(width: 120)
                                Text("\(audioEngine.inputGainDb, specifier: "%+.0f") dB")
                                    .font(.caption2)
                            }
                            
                            VStack {
                                Text("Output")
                                    .font(.caption)
                                    .foregroundColor(.secondary)
                                Slider(value: $audioEngine.outputGainDb, in: -24...24)
                                    .frame(width: 120)
                                Text("\(audioEngine.outputGainDb, specifier: "%+.0f") dB")
                                    .font(.caption2)
                            }
                        }
                    }
                    .padding()
                }
            }
            .navigationTitle("Amp")
        }
    }
    
    private func loadSlot(_ slot: String) {
        // Load preset from UserDefaults
        if let data = UserDefaults.standard.data(forKey: "ampSlot_\(slot)"),
           let preset = try? JSONDecoder().decode(Preset.self, from: data) {
            applyPreset(preset)
            selectedSlot = slot
        }
    }
    
    private func saveSlot(_ slot: String) {
        let preset = Preset(
            name: "Slot \(slot)",
            inputGainDb: audioEngine.inputGainDb,
            outputGainDb: audioEngine.outputGainDb,
            ampEnabled: audioEngine.ampEnabled,
            effectsEnabled: audioEngine.effectsEnabled,
            distortionEnabled: audioEngine.distortionEnabled,
            distortionType: audioEngine.distortionType,
            distortionDrive: audioEngine.distortionDrive,
            distortionTone: audioEngine.distortionTone,
            distortionLevel: audioEngine.distortionLevel,
            delayEnabled: audioEngine.delayEnabled,
            reverbEnabled: audioEngine.reverbEnabled,
            delayFirst: audioEngine.delayFirst,
            delayTimeMs: audioEngine.delayTimeMs,
            delayFeedback: audioEngine.delayFeedback,
            delayMix: audioEngine.delayMix,
            reverbRoomSize: audioEngine.reverbRoomSize,
            reverbDamping: audioEngine.reverbDamping,
            reverbMix: audioEngine.reverbMix,
            ampGainDb: audioEngine.ampGainDb,
            ampDrive: audioEngine.ampDrive,
            ampBassDb: audioEngine.ampBassDb,
            ampMidDb: audioEngine.ampMidDb,
            ampTrebleDb: audioEngine.ampTrebleDb,
            ampPresenceDb: audioEngine.ampPresenceDb,
            ampMasterDb: audioEngine.ampMasterDb,
            noiseGateEnabled: audioEngine.noiseGateEnabled,
            noiseGateThreshold: audioEngine.noiseGateThreshold,
            noiseGateAttack: audioEngine.noiseGateAttack,
            noiseGateHold: audioEngine.noiseGateHold,
            noiseGateRelease: audioEngine.noiseGateRelease,
            noiseGateRange: audioEngine.noiseGateRange,
            compressorEnabled: audioEngine.compressorEnabled,
            compressorThreshold: audioEngine.compressorThreshold,
            compressorRatio: audioEngine.compressorRatio,
            compressorAttack: audioEngine.compressorAttack,
            compressorRelease: audioEngine.compressorRelease,
            compressorMakeup: audioEngine.compressorMakeup,
            eqEnabled: audioEngine.eqEnabled,
            eqBands: audioEngine.eqBands,
            modulationEnabled: audioEngine.modulationEnabled,
            modulationType: audioEngine.modulationType,
            modulationRate: audioEngine.modulationRate,
            modulationDepth: audioEngine.modulationDepth,
            modulationMix: audioEngine.modulationMix,
            wahEnabled: audioEngine.wahEnabled,
            wahPosition: audioEngine.wahPosition,
            wahMode: audioEngine.wahMode,
            wahQ: audioEngine.wahQ
        )
        
        if let data = try? JSONEncoder().encode(preset) {
            UserDefaults.standard.set(data, forKey: "ampSlot_\(slot)")
        }
        
        selectedSlot = slot
    }
    
    private func applyPreset(_ preset: Preset) {
        audioEngine.inputGainDb = preset.inputGainDb
        audioEngine.outputGainDb = preset.outputGainDb
        audioEngine.ampEnabled = preset.ampEnabled
        audioEngine.effectsEnabled = preset.effectsEnabled
        audioEngine.ampGainDb = preset.ampGainDb
        audioEngine.ampDrive = preset.ampDrive
        audioEngine.ampBassDb = preset.ampBassDb
        audioEngine.ampMidDb = preset.ampMidDb
        audioEngine.ampTrebleDb = preset.ampTrebleDb
        audioEngine.ampPresenceDb = preset.ampPresenceDb
        audioEngine.ampMasterDb = preset.ampMasterDb
    }
}

struct AmpSlotButton: View {
    let slot: String
    let isSelected: Bool
    let action: () -> Void
    
    var body: some View {
        Button(action: action) {
            Text(slot)
                .font(.title2)
                .fontWeight(.bold)
                .frame(width: 60, height: 60)
                .background(isSelected ? Color.orange : Color.gray.opacity(0.3))
                .foregroundColor(isSelected ? .white : .primary)
                .cornerRadius(12)
        }
    }
}

#Preview {
    AmpView()
        .environmentObject(AudioEngine())
}
