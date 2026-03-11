import Foundation
import AVFoundation

/// Swift wrapper for the C++ DSP core
class AudioEngine {
    private var audioEngine: AVAudioEngine?
    private var inputNode: AVAudioInputNode?
    private var outputNode: AVAudioOutputNode?
    
    // Effect states
    var ampEnabled: Bool = true
    var effectsEnabled: Bool = true
    var distortionEnabled: Bool = false
    var delayEnabled: Bool = true
    var reverbEnabled: Bool = true
    var noiseGateEnabled: Bool = false
    var compressorEnabled: Bool = false
    var eqEnabled: Bool = false
    var modulationEnabled: Bool = false
    var wahEnabled: Bool = false
    var cabinetEnabled: Bool = true
    var acousticSimEnabled: Bool = false
    var harmonizerEnabled: Bool = false
    
    // Effect parameters
    var inputGainDb: Float = 0.0
    var outputGainDb: Float = 0.0
    
    // Amp parameters
    var ampGainDb: Float = 0.0
    var ampDrive: Float = 0.5
    var ampBassDb: Float = 0.0
    var ampMidDb: Float = 0.0
    var ampTrebleDb: Float = 0.0
    var ampPresenceDb: Float = 0.0
    var ampMasterDb: Float = 0.0
    
    // Distortion parameters
    var distortionType: Int = 0  // 0=Overdrive, 1=Fuzz, 2=Tube, 3=HardClip
    var distortionDrive: Float = 0.5
    var distortionTone: Float = 0.5
    var distortionLevel: Float = 0.7
    
    // Delay parameters
    var delayTimeMs: Float = 350.0
    var delayFeedback: Float = 0.35
    var delayMix: Float = 0.25
    var delayFirst: Bool = true
    
    // Reverb parameters
    var reverbRoomSize: Float = 0.5
    var reverbDamping: Float = 0.3
    var reverbMix: Float = 0.25
    
    // Compressor parameters
    var compressorThreshold: Float = -20.0
    var compressorRatio: Float = 4.0
    var compressorAttack: Float = 10.0
    var compressorRelease: Float = 100.0
    var compressorMakeup: Float = 0.0
    
    // Noise Gate parameters
    var noiseGateThreshold: Float = -40.0
    var noiseGateAttack: Float = 1.0
    var noiseGateHold: Float = 50.0
    var noiseGateRelease: Float = 100.0
    var noiseGateRange: Float = -40.0
    
    // EQ parameters (10-band)
    var eqBands: [Float] = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
    var eqLowCut: Float = 20.0
    var eqHighCut: Float = 20000.0
    
    // Modulation parameters
    var modulationType: Int = 0  // 0=Chorus, 1=Flanger, 2=Phaser, 3=Tremolo, 4=Vibrato
    var modulationRate: Float = 1.5
    var modulationDepth: Float = 0.5
    var modulationMix: Float = 0.5
    
    // Wah parameters
    var wahPosition: Float = 0.5
    var wahMode: Int = 0  // 0=Manual, 1=Auto, 2=Touch
    var wahQ: Float = 5.0
    
    // Cabinet parameters
    var cabinetType: Int = 0  // 0=Marshall4x12, 1=Fender2x12, 2=VoxAC30, 3=MesaBoogie4x12, 4=Orange4x12, 5=TwinReverb, 6=Greenback, 7=Vintage30, 8=AcousticSim
    var cabinetMix: Float = 1.0
    
    // Acoustic Sim parameters
    var acousticAmount: Float = 0.5
    var acousticBodySize: Float = 0.5
    var acousticBrightness: Float = 0.5
    
    // Harmonizer parameters
    var harmonizerMode: Int = 0  // 0=OctaveDown, 1=OctaveUp, 2=PerfectFifth, 3=Thirds
    var harmonizerMix: Float = 0.5
    
    // Level meters
    private(set) var inputLevel: Float = 0.0
    private(set) var outputLevel: Float = 0.0
    private(set) var gainReduction: Float = 0.0
    
    // State
    private(set) var isRunning: Bool = false
    
    init() {
        setupAudioEngine()
    }
    
    private func setupAudioEngine() {
        audioEngine = AVAudioEngine()
        inputNode = audioEngine?.inputNode
        outputNode = audioEngine?.outputNode
        
        guard let inputNode = inputNode,
              let outputNode = outputNode else {
            return
        }
        
        let inputFormat = inputNode.outputFormat(forBus: 0)
        let outputFormat = outputNode.inputFormat(forBus: 0)
        
        // Install tap on input node to process audio
        inputNode.installTap(onBus: 0, bufferSize: 1024, format: inputFormat) { [weak self] buffer, time in
            self?.processAudio(buffer: buffer)
        }
        
        // Connect input to output through main mixer
        let mainMixer = audioEngine?.mainMixerNode
        inputNode.connect(to: mainMixer!, format: inputFormat)
    }
    
    private func processAudio(buffer: AVAudioPCMBuffer) {
        guard let channelData = buffer.floatChannelData?[0] else { return }
        let frameCount = Int(buffer.frameLength)
        
        // Process audio through DSP chain
        for i in 0..<frameCount {
            var sample = channelData[i]
            
            // Input gain
            let inputGain = pow(10.0, inputGainDb / 20.0)
            sample *= inputGain
            
            // Noise Gate
            if noiseGateEnabled {
                sample = applyNoiseGate(sample)
            }
            
            // Compressor
            if compressorEnabled {
                sample = applyCompressor(sample)
            }
            
            // EQ
            if eqEnabled {
                sample = applyEQ(sample)
            }
            
            // Wah
            if wahEnabled {
                sample = applyWah(sample)
            }
            
            // Distortion
            if distortionEnabled {
                sample = applyDistortion(sample)
            }
            
            // Amp simulator
            if ampEnabled {
                sample = applyAmp(sample)
            }
            
            // Cabinet simulation
            if cabinetEnabled {
                sample = applyCabinet(sample)
            }
            
            // Acoustic simulation
            if acousticSimEnabled {
                sample = applyAcousticSim(sample)
            }
            
            // Harmonizer
            if harmonizerEnabled {
                sample = applyHarmonizer(sample)
            }
            
            // Effects chain
            if effectsEnabled {
                if delayFirst {
                    if delayEnabled { sample = applyDelay(sample) }
                    if reverbEnabled { sample = applyReverb(sample) }
                } else {
                    if reverbEnabled { sample = applyReverb(sample) }
                    if delayEnabled { sample = applyDelay(sample) }
                }
                
                // Modulation
                if modulationEnabled {
                    sample = applyModulation(sample)
                }
            }
            
            // Output gain
            let outputGain = pow(10.0, outputGainDb / 20.0)
            sample *= outputGain
            
            // Soft clip output
            sample = tanh(sample)
            
            channelData[i] = sample
        }
        
        // Update level meters
        updateMeters(buffer: buffer)
    }
    
    // MARK: - Effect Processing (Simplified implementations - would use C++ DSP core in production)
    
    private var delayBuffer: [Float] = Array(repeating: 0, count: 192000)
    private var delayWriteIndex: Int = 0
    
    private func applyDistortion(_ sample: Float) -> Float {
        let drive = distortionDrive * 10.0 + 1.0
        var output: Float
        
        switch distortionType {
        case 0: // Overdrive
            output = tanh(sample * drive)
        case 1: // Fuzz
            let x = sample * drive
            output = sign(x) * min(abs(x), 1.0)
        case 2: // Tube
            let x = sample * drive
            output = x / (1.0 + abs(x))
        case 3: // Hard Clip
            output = max(-1.0, min(1.0, sample * drive))
        default:
            output = sample
        }
        
        return output * distortionLevel + sample * (1.0 - distortionLevel)
    }
    
    private var lastDelaySample: Float = 0.0
    
    private func applyDelay(_ sample: Float) -> Float {
        let sampleRate: Float = 48000.0
        let delaySamples = Int(delayTimeMs * sampleRate / 1000.0)
        
        let readIndex = (delayWriteIndex - delaySamples + delayBuffer.count) % delayBuffer.count
        let delayedSample = delayBuffer[readIndex]
        
        // Write to buffer with feedback
        delayBuffer[delayWriteIndex] = sample + delayedSample * delayFeedback
        delayWriteIndex = (delayWriteIndex + 1) % delayBuffer.count
        
        // Mix
        return sample * (1.0 - delayMix) + delayedSample * delayMix
    }
    
    private var reverbBuffer: [Float] = Array(repeating: 0, count: 48000)
    private var reverbIndex: Int = 0
    
    private func applyReverb(_ sample: Float) -> Float {
        // Simple reverb using multiple delay taps
        let sampleRate: Float = 48000.0
        let delays = [0.03, 0.05, 0.07, 0.11, 0.13, 0.17]
        var reverbOut: Float = 0.0
        
        for (i, delaySec) in delays.enumerated() {
            let delaySamples = Int(delaySec * sampleRate * (1.0 + reverbRoomSize))
            let readIdx = (reverbIndex - delaySamples + reverbBuffer.count) % reverbBuffer.count
            let tap = reverbBuffer[readIdx]
            reverbOut += tap * (1.0 - Float(i) * 0.1)
        }
        
        reverbOut /= Float(delays.count)
        
        // Apply damping
        reverbOut *= (1.0 - reverbDamping * 0.5)
        
        // Write to buffer
        reverbBuffer[reverbIndex] = sample + reverbOut * 0.5
        reverbIndex = (reverbIndex + 1) % reverbBuffer.count
        
        // Mix
        return sample * (1.0 - reverbMix) + reverbOut * reverbMix
    }
    
    private var ampState: [Float] = [0, 0, 0, 0]
    
    private func applyAmp(_ sample: Float) -> Float {
        var output = sample
        
        // Preamp gain
        let gain = pow(10.0, ampGainDb / 20.0)
        output *= gain
        
        // Drive (soft clipping)
        let drive = ampDrive * 5.0 + 1.0
        output = tanh(output * drive)
        
        // Simple tone stack (bass, mid, treble filters)
        // Bass boost
        let bassCoeff = pow(10.0, ampBassDb / 20.0)
        ampState[0] = ampState[0] * 0.99 + output * 0.01
        output = output + ampState[0] * (bassCoeff - 1.0)
        
        // Mid
        let midCoeff = pow(10.0, ampMidDb / 20.0)
        let midSample = output - ampState[0] - ampState[2]
        output = output + midSample * (midCoeff - 1.0) * 0.5
        
        // Treble
        let trebleCoeff = pow(10.0, ampTrebleDb / 20.0)
        ampState[2] = ampState[2] * 0.9 + (output - ampState[2]) * 0.1
        output = output + ampState[2] * (trebleCoeff - 1.0) * 0.3
        
        // Presence
        let presenceCoeff = pow(10.0, ampPresenceDb / 20.0)
        output = output * presenceCoeff
        
        // Master volume
        let master = pow(10.0, ampMasterDb / 20.0)
        output *= master
        
        return output
    }
    
    private var noiseGateEnvelope: Float = 0.0
    private var noiseGateHoldCounter: Int = 0
    
    private func applyNoiseGate(_ sample: Float) -> Float {
        let threshold = pow(10.0, noiseGateThreshold / 20.0)
        let attackCoeff = exp(-1.0 / (noiseGateAttack * 48.0))
        let releaseCoeff = exp(-1.0 / (noiseGateRelease * 48.0))
        let holdSamples = Int(noiseGateHold * 48.0)
        let minGain = pow(10.0, noiseGateRange / 20.0)
        
        let inputLevel = abs(sample)
        
        // Envelope follower
        if inputLevel > noiseGateEnvelope {
            noiseGateEnvelope = attackCoeff * noiseGateEnvelope + (1.0 - attackCoeff) * inputLevel
        } else {
            noiseGateEnvelope = releaseCoeff * noiseGateEnvelope + (1.0 - releaseCoeff) * inputLevel
        }
        
        // Gate logic
        var targetGain: Float = 1.0
        if noiseGateEnvelope > threshold {
            noiseGateHoldCounter = holdSamples
        } else if noiseGateHoldCounter > 0 {
            noiseGateHoldCounter -= 1
        } else {
            targetGain = minGain
        }
        
        return sample * targetGain
    }
    
    private var compressorEnvelope: Float = 0.0
    
    private func applyCompressor(_ sample: Float) -> Float {
        let threshold = compressorThreshold
        let inputDb = 20.0 * log10(abs(sample) + 0.00001)
        
        // Simple envelope
        compressorEnvelope = compressorEnvelope * 0.99 + abs(sample) * 0.01
        
        // Calculate gain reduction
        var gainReduction: Float = 0.0
        if compressorEnvelope > pow(10.0, threshold / 20.0) {
            let overDb = 20.0 * log10(compressorEnvelope) - threshold
            gainReduction = overDb * (1.0 - 1.0 / compressorRatio)
        }
        
        gainReduction = -gainReduction
        self.gainReduction = gainReduction
        
        let makeupGain = pow(10.0, compressorMakeup / 20.0)
        let reductionGain = pow(10.0, -gainReduction / 20.0)
        
        return sample * reductionGain * makeupGain
    }
    
    private var eqState: [[Float]] = [[0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0],
                                       [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0],
                                       [0, 0, 0, 0], [0, 0, 0, 0]]
    
    private func applyEQ(_ sample: Float) -> Float {
        var output = sample
        let freqs: [Float] = [31, 63, 125, 250, 500, 1000, 2000, 4000, 8000, 16000]
        let sampleRate: Float = 48000.0
        
        for i in 0..<10 {
            let gain = eqBands[i]
            if abs(gain) < 0.1 { continue }
            
            // Simple peaking filter approximation
            let omega = 2.0 * Float.pi * freqs[i] / sampleRate
            let alpha = sin(omega) / (2.0 * 1.414)
            let A = pow(10.0, gain / 40.0)
            
            let b0 = 1.0 + alpha * A
            let b1 = -2.0 * cos(omega)
            let b2 = 1.0 - alpha * A
            let a0 = 1.0 + alpha / A
            let a1 = -2.0 * cos(omega)
            let a2 = 1.0 - alpha / A
            
            let x = output
            let y = (b0/a0) * x + (b1/a0) * eqState[i][0] + (b2/a0) * eqState[i][1]
                     - (a1/a0) * eqState[i][2] - (a2/a0) * eqState[i][3]
            
            eqState[i][1] = eqState[i][0]
            eqState[i][0] = x
            eqState[i][3] = eqState[i][2]
            eqState[i][2] = y
            
            output = y
        }
        
        return output
    }
    
    private var lfoPhase: Float = 0.0
    
    private func applyModulation(_ sample: Float) -> Float {
        let sampleRate: Float = 48000.0
        lfoPhase += modulationRate / sampleRate
        if lfoPhase >= 1.0 { lfoPhase -= 1.0 }
        
        let lfo = sin(2.0 * Float.pi * lfoPhase)
        var output = sample
        
        switch modulationType {
        case 0: // Chorus
            // Simple chorus using short delay modulation
            let delaySamples = 0.015 * sampleRate + lfo * 0.005 * sampleRate * modulationDepth
            output = sample * (1.0 - modulationMix) + sample * modulationMix * (1.0 + lfo * modulationDepth * 0.1)
            
        case 1: // Flanger
            let flangeDelay = 0.002 * sampleRate + lfo * 0.002 * sampleRate * modulationDepth
            output = sample + sample * modulationMix * lfo * modulationDepth
            
        case 2: // Phaser
            // All-pass approximation
            let phaseShift = lfo * modulationDepth
            output = sample * cos(phaseShift) - sample * sin(phaseShift) * modulationMix
            
        case 3: // Tremolo
            let amplitude = 1.0 - modulationDepth * (1.0 - (lfo + 1.0) * 0.5)
            output = sample * amplitude
            
        case 4: // Vibrato
            let pitchShift = lfo * modulationDepth * 0.02
            output = sample * (1.0 + pitchShift)
            
        default:
            output = sample
        }
        
        return output
    }
    
    private var wahLfoPhase: Float = 0.0
    private var wahEnvelope: Float = 0.0
    
    private func applyWah(_ sample: Float) -> Float {
        let sampleRate: Float = 48000.0
        var position = wahPosition
        
        if wahMode == 1 { // Auto
            wahLfoPhase += 2.0 / sampleRate
            if wahLfoPhase >= 1.0 { wahLfoPhase -= 1.0 }
            let lfo = sin(2.0 * Float.pi * wahLfoPhase)
            position = 0.5 + (lfo * 0.5) * 0.5
        } else if wahMode == 2 { // Touch
            let inputLevel = abs(sample)
            wahEnvelope = wahEnvelope * 0.99 + inputLevel * 0.01
            position = min(1.0, wahEnvelope * 10.0 * 0.5)
        }
        
        // Bandpass filter frequency
        let minFreq: Float = 400.0
        let maxFreq: Float = 2500.0
        let centerFreq = minFreq + (maxFreq - minFreq) * position
        
        // Simple resonant filter approximation
        let omega = 2.0 * Float.pi * centerFreq / sampleRate
        let q = wahQ
        let resonance = q / (1.0 + q)
        
        let bandpass = sample * sin(omega) * resonance
        
        return sample * (1.0 - 0.5) + bandpass * 0.5
    }
    
    // MARK: - New Effects (Cabinet, Acoustic Sim, Harmonizer)
    
    private var cabinetBuffer: [Float] = Array(repeating: 0, count: 4096)
    private var cabinetWritePos: Int = 0
    
    private func applyCabinet(_ sample: Float) -> Float {
        // Simplified cabinet simulation using short convolution
        let sampleRate: Float = 48000.0
        let irLength = 2400  // 50ms at 48kHz
        
        // Store in circular buffer
        cabinetBuffer[cabinetWritePos] = sample
        
        // Generate simple IR based on cabinet type
        var output: Float = 0.0
        for i in 0..<min(irLength, cabinetBuffer.count) {
            let readPos = (cabinetWritePos - i + cabinetBuffer.count) % cabinetBuffer.count
            let t = Float(i) / sampleRate
            
            // IR envelope
            var irSample = exp(-t * 40.0)
            
            // Modify based on cabinet type
            switch cabinetType {
            case 0: // Marshall - aggressive mids
                irSample *= (1.0 + 0.3 * sin(t * 400.0))
            case 1: // Fender - bright, scooped
                irSample *= (t < 0.002) ? 1.3 : 1.0
            case 2: // Vox - chimey
                irSample *= (1.0 + 0.2 * sin(t * 2500.0))
            case 3: // Mesa - thick
                irSample *= 1.2
            case 4: // Orange - heavy
                irSample *= 1.3
            case 5: // Twin - clean
                irSample *= (1.0 + 0.2 * (t < 0.003 ? 1.0 : 0.0))
            case 6: // Greenback - warm
                irSample *= (1.0 - 0.1 * sin(t * 200.0))
            case 7: // Vintage 30 - balanced
                irSample *= (1.0 + 0.15 * sin(t * 4000.0))
            case 8: // Acoustic Sim
                irSample *= exp(-t * 60.0)
                irSample *= (1.0 - 0.3 * sin(t * 1000.0))
            default:
                break
            }
            
            output += cabinetBuffer[readPos] * irSample
        }
        
        cabinetWritePos = (cabinetWritePos + 1) % cabinetBuffer.count
        
        // Normalize and mix
        output /= Float(irLength) * 0.1
        return sample * (1.0 - cabinetMix) + output * cabinetMix
    }
    
    private var acousticBodyState: [Float] = [0, 0, 0, 0]
    
    private func applyAcousticSim(_ sample: Float) -> Float {
        var output = sample
        
        // Body resonance simulation (comb filters)
        let bodyDelays: [Float] = [2.0, 3.0, 5.0, 7.0]  // ms
        let bodyGains: [Float] = [0.2, 0.15, 0.1, 0.05]
        
        for i in 0..<4 {
            let delaySamples = Int(bodyDelays[i] * 48.0 * (1.0 + acousticBodySize))
            let delayIdx = (cabinetWritePos - delaySamples + cabinetBuffer.count) % cabinetBuffer.count
            let delayed = cabinetBuffer[delayIdx]
            
            // Simple lowpass
            acousticBodyState[i] = acousticBodyState[i] * 0.9 + delayed * 0.1
            output += acousticBodyState[i] * bodyGains[i] * acousticAmount
        }
        
        // Brightness enhancement
        let bright = output - acousticBodyState[0]
        output = output + bright * acousticBrightness * 0.3
        
        return sample * (1.0 - acousticAmount) + output * acousticAmount
    }
    
    private var harmonizerBuffer: [Float] = Array(repeating: 0, count: 8192)
    private var harmonizerWritePos: Int = 0
    private var harmonizerReadPos: Float = 0.0
    
    private func applyHarmonizer(_ sample: Float) -> Float {
        // Store in buffer
        harmonizerBuffer[harmonizerWritePos] = sample
        harmonizerWritePos = (harmonizerWritePos + 1) % harmonizerBuffer.count
        
        // Calculate pitch ratio based on mode
        let pitchRatio: Float
        switch harmonizerMode {
        case 0: // Octave down
            pitchRatio = 0.5
        case 1: // Octave up
            pitchRatio = 2.0
        case 2: // Perfect fifth
            pitchRatio = 1.5
        case 3: // Major third
            pitchRatio = 1.26
        default:
            pitchRatio = 0.5
        }
        
        // Read at different rate
        let readIdx = Int(harmonizerReadPos) % harmonizerBuffer.count
        let nextIdx = (readIdx + 1) % harmonizerBuffer.count
        let frac = harmonizerReadPos - floor(harmonizerReadPos)
        
        let shifted = harmonizerBuffer[readIdx] * (1.0 - frac) + harmonizerBuffer[nextIdx] * frac
        
        // Update read position
        harmonizerReadPos += pitchRatio
        if harmonizerReadPos >= Float(harmonizerBuffer.count) {
            harmonizerReadPos -= Float(harmonizerBuffer.count)
        }
        
        // Mix dry/wet
        return sample * (1.0 - harmonizerMix) + shifted * harmonizerMix
    }
    
    private func updateMeters(buffer: AVAudioPCMBuffer) {
        guard let channelData = buffer.floatChannelData?[0] else { return }
        let frameCount = Int(buffer.frameLength)
        
        var peak: Float = 0.0
        for i in 0..<frameCount {
            let absVal = abs(channelData[i])
            if absVal > peak { peak = absVal }
        }
        
        inputLevel = inputLevel * 0.9 + peak * 0.1
        outputLevel = outputLevel * 0.9 + peak * 0.1
    }
    
    // MARK: - Control Methods
    
    func start() throws {
        guard let audioEngine = audioEngine else { return }
        
        try audioEngine.start()
        isRunning = true
    }
    
    func stop() {
        audioEngine?.stop()
        isRunning = false
    }
    
    func savePreset(name: String) -> Bool {
        let preset = Preset(
            name: name,
            inputGainDb: inputGainDb,
            outputGainDb: outputGainDb,
            ampEnabled: ampEnabled,
            effectsEnabled: effectsEnabled,
            distortionEnabled: distortionEnabled,
            distortionType: distortionType,
            distortionDrive: distortionDrive,
            distortionTone: distortionTone,
            distortionLevel: distortionLevel,
            delayEnabled: delayEnabled,
            reverbEnabled: reverbEnabled,
            delayFirst: delayFirst,
            delayTimeMs: delayTimeMs,
            delayFeedback: delayFeedback,
            delayMix: delayMix,
            reverbRoomSize: reverbRoomSize,
            reverbDamping: reverbDamping,
            reverbMix: reverbMix,
            ampGainDb: ampGainDb,
            ampDrive: ampDrive,
            ampBassDb: ampBassDb,
            ampMidDb: ampMidDb,
            ampTrebleDb: ampTrebleDb,
            ampPresenceDb: ampPresenceDb,
            ampMasterDb: ampMasterDb,
            noiseGateEnabled: noiseGateEnabled,
            noiseGateThreshold: noiseGateThreshold,
            noiseGateAttack: noiseGateAttack,
            noiseGateHold: noiseGateHold,
            noiseGateRelease: noiseGateRelease,
            noiseGateRange: noiseGateRange,
            compressorEnabled: compressorEnabled,
            compressorThreshold: compressorThreshold,
            compressorRatio: compressorRatio,
            compressorAttack: compressorAttack,
            compressorRelease: compressorRelease,
            compressorMakeup: compressorMakeup,
            eqEnabled: eqEnabled,
            eqBands: eqBands,
            modulationEnabled: modulationEnabled,
            modulationType: modulationType,
            modulationRate: modulationRate,
            modulationDepth: modulationDepth,
            modulationMix: modulationMix,
            wahEnabled: wahEnabled,
            wahPosition: wahPosition,
            wahMode: wahMode,
            wahQ: wahQ
        )
        
        return PresetManager.shared.savePreset(preset)
    }
    
    func loadPreset(name: String) -> Bool {
        guard let preset = PresetManager.shared.loadPreset(name: name) else {
            return false
        }
        
        inputGainDb = preset.inputGainDb
        outputGainDb = preset.outputGainDb
        ampEnabled = preset.ampEnabled
        effectsEnabled = preset.effectsEnabled
        distortionEnabled = preset.distortionEnabled
        distortionType = preset.distortionType
        distortionDrive = preset.distortionDrive
        distortionTone = preset.distortionTone
        distortionLevel = preset.distortionLevel
        delayEnabled = preset.delayEnabled
        reverbEnabled = preset.reverbEnabled
        delayFirst = preset.delayFirst
        delayTimeMs = preset.delayTimeMs
        delayFeedback = preset.delayFeedback
        delayMix = preset.delayMix
        reverbRoomSize = preset.reverbRoomSize
        reverbDamping = preset.reverbDamping
        reverbMix = preset.reverbMix
        ampGainDb = preset.ampGainDb
        ampDrive = preset.ampDrive
        ampBassDb = preset.ampBassDb
        ampMidDb = preset.ampMidDb
        ampTrebleDb = preset.ampTrebleDb
        ampPresenceDb = preset.ampPresenceDb
        ampMasterDb = preset.ampMasterDb
        noiseGateEnabled = preset.noiseGateEnabled
        noiseGateThreshold = preset.noiseGateThreshold
        noiseGateAttack = preset.noiseGateAttack
        noiseGateHold = preset.noiseGateHold
        noiseGateRelease = preset.noiseGateRelease
        noiseGateRange = preset.noiseGateRange
        compressorEnabled = preset.compressorEnabled
        compressorThreshold = preset.compressorThreshold
        compressorRatio = preset.compressorRatio
        compressorAttack = preset.compressorAttack
        compressorRelease = preset.compressorRelease
        compressorMakeup = preset.compressorMakeup
        eqEnabled = preset.eqEnabled
        eqBands = preset.eqBands
        modulationEnabled = preset.modulationEnabled
        modulationType = preset.modulationType
        modulationRate = preset.modulationRate
        modulationDepth = preset.modulationDepth
        modulationMix = preset.modulationMix
        wahEnabled = preset.wahEnabled
        wahPosition = preset.wahPosition
        wahMode = preset.wahMode
        wahQ = preset.wahQ
        
        return true
    }
}

// MARK: - Preset Model

struct Preset: Codable {
    let name: String
    let inputGainDb: Float
    let outputGainDb: Float
    let ampEnabled: Bool
    let effectsEnabled: Bool
    let distortionEnabled: Bool
    let distortionType: Int
    let distortionDrive: Float
    let distortionTone: Float
    let distortionLevel: Float
    let delayEnabled: Bool
    let reverbEnabled: Bool
    let delayFirst: Bool
    let delayTimeMs: Float
    let delayFeedback: Float
    let delayMix: Float
    let reverbRoomSize: Float
    let reverbDamping: Float
    let reverbMix: Float
    let ampGainDb: Float
    let ampDrive: Float
    let ampBassDb: Float
    let ampMidDb: Float
    let ampTrebleDb: Float
    let ampPresenceDb: Float
    let ampMasterDb: Float
    let noiseGateEnabled: Bool
    let noiseGateThreshold: Float
    let noiseGateAttack: Float
    let noiseGateHold: Float
    let noiseGateRelease: Float
    let noiseGateRange: Float
    let compressorEnabled: Bool
    let compressorThreshold: Float
    let compressorRatio: Float
    let compressorAttack: Float
    let compressorRelease: Float
    let compressorMakeup: Float
    let eqEnabled: Bool
    let eqBands: [Float]
    let modulationEnabled: Bool
    let modulationType: Int
    let modulationRate: Float
    let modulationDepth: Float
    let modulationMix: Float
    let wahEnabled: Bool
    let wahPosition: Float
    let wahMode: Int
    let wahQ: Float
    
    static func defaultPreset() -> Preset {
        Preset(
            name: "Default",
            inputGainDb: 0, outputGainDb: 0,
            ampEnabled: true, effectsEnabled: true,
            distortionEnabled: false, distortionType: 0,
            distortionDrive: 0.5, distortionTone: 0.5, distortionLevel: 0.7,
            delayEnabled: true, reverbEnabled: true, delayFirst: true,
            delayTimeMs: 350, delayFeedback: 0.35, delayMix: 0.25,
            reverbRoomSize: 0.5, reverbDamping: 0.3, reverbMix: 0.25,
            ampGainDb: 0, ampDrive: 0.5, ampBassDb: 0, ampMidDb: 0,
            ampTrebleDb: 0, ampPresenceDb: 0, ampMasterDb: 0,
            noiseGateEnabled: false, noiseGateThreshold: -40,
            noiseGateAttack: 1, noiseGateHold: 50, noiseGateRelease: 100,
            noiseGateRange: -40,
            compressorEnabled: false, compressorThreshold: -20,
            compressorRatio: 4, compressorAttack: 10, compressorRelease: 100,
            compressorMakeup: 0,
            eqEnabled: false, eqBands: [0,0,0,0,0,0,0,0,0,0],
            modulationEnabled: false, modulationType: 0,
            modulationRate: 1.5, modulationDepth: 0.5, modulationMix: 0.5,
            wahEnabled: false, wahPosition: 0.5, wahMode: 0, wahQ: 5.0
        )
    }
}

// MARK: - Preset Manager

class PresetManager {
    static let shared = PresetManager()
    
    private let defaults = UserDefaults.standard
    private let presetsKey = "savedPresets"
    
    func savePreset(_ preset: Preset) -> Bool {
        var presets = loadAllPresets()
        presets.removeAll { $0.name == preset.name }
        presets.append(preset)
        
        guard let data = try? JSONEncoder().encode(presets) else {
            return false
        }
        
        defaults.set(data, forKey: presetsKey)
        return true
    }
    
    func loadPreset(name: String) -> Preset? {
        let presets = loadAllPresets()
        return presets.first { $0.name == name }
    }
    
    func loadAllPresets() -> [Preset] {
        guard let data = defaults.data(forKey: presetsKey),
              let presets = try? JSONDecoder().decode([Preset].self, from: data) else {
            return []
        }
        return presets
    }
    
    func deletePreset(name: String) {
        var presets = loadAllPresets()
        presets.removeAll { $0.name == name }
        
        if let data = try? JSONEncoder().encode(presets) {
            defaults.set(data, forKey: presetsKey)
        }
    }
}
