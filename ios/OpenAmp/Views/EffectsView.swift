import SwiftUI

struct EffectsView: View {
    @EnvironmentObject var audioEngine: AudioEngine
    
    var body: some View {
        NavigationView {
            ScrollView {
                VStack(spacing: 20) {
                    // Master Effects Toggle
                    HStack {
                        Text("Effects Chain")
                            .font(.headline)
                        Spacer()
                        Toggle("", isOn: $audioEngine.effectsEnabled)
                            .labelsHidden()
                    }
                    .padding(.horizontal)
                    .padding(.top)
                    
                    // Effect Order
                    VStack(alignment: .leading, spacing: 8) {
                        Text("Signal Chain Order")
                            .font(.subheadline)
                            .foregroundColor(.secondary)
                        
                        HStack {
                            Text(audioEngine.delayFirst ? "Delay → Reverb" : "Reverb → Delay")
                                .font(.caption)
                            Spacer()
                            Button("Swap") {
                                audioEngine.delayFirst.toggle()
                            }
                            .font(.caption)
                            .buttonStyle(.bordered)
                        }
                    }
                    .padding(.horizontal)
                    
                    Divider()
                    
                    // Distortion
                    EffectSection(title: "Distortion", isEnabled: $audioEngine.distortionEnabled) {
                        VStack(spacing: 12) {
                            Picker("Type", selection: $audioEngine.distortionType) {
                                Text("Overdrive").tag(0)
                                Text("Fuzz").tag(1)
                                Text("Tube").tag(2)
                                Text("Hard Clip").tag(3)
                            }
                            .pickerStyle(.segmented)
                            
                            HStack(spacing: 16) {
                                ParameterKnob(label: "Drive", value: $audioEngine.distortionDrive, range: 0...1)
                                ParameterKnob(label: "Tone", value: $audioEngine.distortionTone, range: 0...1)
                                ParameterKnob(label: "Level", value: $audioEngine.distortionLevel, range: 0...1)
                            }
                        }
                    }
                    
                    Divider()
                    
                    // Delay
                    EffectSection(title: "Delay", isEnabled: $audioEngine.delayEnabled) {
                        VStack(spacing: 12) {
                            HStack(spacing: 16) {
                                ParameterSlider(label: "Time", value: $audioEngine.delayTimeMs, range: 1...1000, unit: "ms")
                                ParameterSlider(label: "Feedback", value: $audioEngine.delayFeedback, range: 0...0.95, unit: "%", percentage: true)
                            }
                            ParameterSlider(label: "Mix", value: $audioEngine.delayMix, range: 0...1, unit: "%", percentage: true)
                        }
                    }
                    
                    Divider()
                    
                    // Reverb
                    EffectSection(title: "Reverb", isEnabled: $audioEngine.reverbEnabled) {
                        VStack(spacing: 12) {
                            HStack(spacing: 16) {
                                ParameterSlider(label: "Room", value: $audioEngine.reverbRoomSize, range: 0...1, unit: "%", percentage: true)
                                ParameterSlider(label: "Damp", value: $audioEngine.reverbDamping, range: 0...1, unit: "%", percentage: true)
                            }
                            ParameterSlider(label: "Mix", value: $audioEngine.reverbMix, range: 0...1, unit: "%", percentage: true)
                        }
                    }
                    
                    Divider()
                    
                    // Noise Gate
                    EffectSection(title: "Noise Gate", isEnabled: $audioEngine.noiseGateEnabled) {
                        VStack(spacing: 12) {
                            HStack(spacing: 16) {
                                ParameterSlider(label: "Threshold", value: $audioEngine.noiseGateThreshold, range: -80...0, unit: "dB")
                                ParameterSlider(label: "Range", value: $audioEngine.noiseGateRange, range: -80...0, unit: "dB")
                            }
                            HStack(spacing: 16) {
                                ParameterSlider(label: "Attack", value: $audioEngine.noiseGateAttack, range: 0.1...50, unit: "ms")
                                ParameterSlider(label: "Release", value: $audioEngine.noiseGateRelease, range: 10...1000, unit: "ms")
                            }
                        }
                    }
                    
                    Divider()
                    
                    // Compressor
                    EffectSection(title: "Compressor", isEnabled: $audioEngine.compressorEnabled) {
                        VStack(spacing: 12) {
                            HStack(spacing: 16) {
                                ParameterSlider(label: "Threshold", value: $audioEngine.compressorThreshold, range: -60...0, unit: "dB")
                                ParameterSlider(label: "Ratio", value: $audioEngine.compressorRatio, range: 1...20, unit: ":1")
                            }
                            HStack(spacing: 16) {
                                ParameterSlider(label: "Attack", value: $audioEngine.compressorAttack, range: 0.1...100, unit: "ms")
                                ParameterSlider(label: "Release", value: $audioEngine.compressorRelease, range: 10...1000, unit: "ms")
                            }
                            ParameterSlider(label: "Makeup", value: $audioEngine.compressorMakeup, range: 0...24, unit: "dB")
                            
                            // Gain Reduction Meter
                            HStack {
                                Text("Gain Reduction")
                                    .font(.caption)
                                    .foregroundColor(.secondary)
                                Spacer()
                                Text("\(audioEngine.gainReduction, specifier: "%.1f") dB")
                                    .font(.caption)
                                    .foregroundColor(.orange)
                            }
                        }
                    }
                    
                    Divider()
                    
                    // EQ
                    EffectSection(title: "EQ (10-Band)", isEnabled: $audioEngine.eqEnabled) {
                        VStack(spacing: 8) {
                            let freqs = ["31", "63", "125", "250", "500", "1K", "2K", "4K", "8K", "16K"]
                            
                            HStack(spacing: 4) {
                                ForEach(0..<10, id: \.self) { i in
                                    VStack(spacing: 4) {
                                        Slider(value: $audioEngine.eqBands[i], in: -12...12)
                                            .rotationEffect(.degrees(-90))
                                            .frame(width: 80, height: 20)
                                        
                                        Text(freqs[i])
                                            .font(.system(size: 8))
                                            .foregroundColor(.secondary)
                                    }
                                    .frame(width: 32)
                                }
                            }
                            .frame(height: 100)
                        }
                    }
                    
                    Divider()
                    
                    // Modulation
                    EffectSection(title: "Modulation", isEnabled: $audioEngine.modulationEnabled) {
                        VStack(spacing: 12) {
                            Picker("Type", selection: $audioEngine.modulationType) {
                                Text("Chorus").tag(0)
                                Text("Flanger").tag(1)
                                Text("Phaser").tag(2)
                                Text("Tremolo").tag(3)
                                Text("Vibrato").tag(4)
                            }
                            .pickerStyle(.segmented)
                            
                            HStack(spacing: 16) {
                                ParameterKnob(label: "Rate", value: $audioEngine.modulationRate, range: 0.1...20)
                                ParameterKnob(label: "Depth", value: $audioEngine.modulationDepth, range: 0...1)
                                ParameterKnob(label: "Mix", value: $audioEngine.modulationMix, range: 0...1)
                            }
                        }
                    }
                    
                    Divider()
                    
                    // Wah
                    EffectSection(title: "Wah", isEnabled: $audioEngine.wahEnabled) {
                        VStack(spacing: 12) {
                            Picker("Mode", selection: $audioEngine.wahMode) {
                                Text("Manual").tag(0)
                                Text("Auto").tag(1)
                                Text("Touch").tag(2)
                            }
                            .pickerStyle(.segmented)
                            
                            HStack(spacing: 16) {
                                ParameterKnob(label: "Position", value: $audioEngine.wahPosition, range: 0...1)
                                ParameterKnob(label: "Q", value: $audioEngine.wahQ, range: 0.5...20)
                            }
                            
                            if audioEngine.wahMode == 0 {
                                Text("Tip: Tilt device or use MIDI controller for manual control")
                                    .font(.caption2)
                                    .foregroundColor(.secondary)
                            }
                        }
                    }
                }
                .padding(.bottom, 20)
            }
            .navigationTitle("Effects")
        }
    }
}

struct EffectSection<Content: View>: View {
    let title: String
    @Binding var isEnabled: Bool
    @ViewBuilder let content: () -> Content
    
    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            HStack {
                Text(title)
                    .font(.subheadline)
                    .fontWeight(.medium)
                Spacer()
                Toggle("", isOn: $isEnabled)
                    .labelsHidden()
            }
            
            if isEnabled {
                content()
            }
        }
        .padding(.horizontal)
    }
}

struct ParameterSlider: View {
    let label: String
    @Binding var value: Float
    let range: ClosedRange<Float>
    let unit: String
    var percentage: Bool = false
    
    var body: some View {
        VStack(spacing: 4) {
            Text(label)
                .font(.caption)
                .foregroundColor(.secondary)
            
            Slider(value: $value, in: range)
                .frame(width: 100)
            
            Text(displayValue)
                .font(.caption2)
                .foregroundColor(.secondary)
        }
    }
    
    private var displayValue: String {
        if percentage {
            return "\(Int(value * 100))\(unit)"
        }
        return "\(value, specifier: "%.1f") \(unit)"
    }
}

struct ParameterKnob: View {
    let label: String
    @Binding var value: Float
    let range: ClosedRange<Float>
    
    var body: some View {
        KnobView(
            label: label,
            value: $value,
            range: range,
            unit: "%",
            percentage: true
        )
    }
}

#Preview {
    EffectsView()
        .environmentObject(AudioEngine())
}
