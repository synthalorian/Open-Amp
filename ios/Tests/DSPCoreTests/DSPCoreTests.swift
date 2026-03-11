import XCTest
@testable import DSPCore

final class DSPCoreTests: XCTestCase {

    var engine: DSPEngineRef?

    override func setUp() {
        super.setUp()
        engine = dsp_engine_create(48000.0, 256)
    }

    override func tearDown() {
        if let engine = engine {
            dsp_engine_destroy(engine)
        }
        engine = nil
        super.tearDown()
    }

    // MARK: - Engine Lifecycle

    func testEngineCreation() {
        XCTAssertNotNil(engine, "Engine should be created successfully")
    }

    func testEngineCreationWithDifferentSampleRates() {
        let rates: [Double] = [44100.0, 48000.0, 96000.0]
        for rate in rates {
            let testEngine = dsp_engine_create(rate, 256)
            XCTAssertNotNil(testEngine, "Engine should be created at \(rate)Hz")
            dsp_engine_destroy(testEngine)
        }
    }

    func testEngineCreationWithDifferentBlockSizes() {
        let sizes: [UInt32] = [128, 256, 512, 1024, 2048]
        for size in sizes {
            let testEngine = dsp_engine_create(48000.0, size)
            XCTAssertNotNil(testEngine, "Engine should be created with block size \(size)")
            dsp_engine_destroy(testEngine)
        }
    }

    // MARK: - Processing

    func testBasicProcessing() {
        guard let engine = engine else {
            XCTFail("Engine not created")
            return
        }

        let numFrames: UInt32 = 256
        var input = [Float](repeating: 0.5, count: Int(numFrames))
        var output = [Float](repeating: 0.0, count: Int(numFrames))

        dsp_engine_process(engine, &input, &output, numFrames)

        // Output should be different from input (processed)
        let allSame = input.elementsEqual(output)
        XCTAssertFalse(allSame, "Output should be processed and different from input")
    }

    func testSilenceProcessing() {
        guard let engine = engine else {
            XCTFail("Engine not created")
            return
        }

        let numFrames: UInt32 = 256
        var input = [Float](repeating: 0.0, count: Int(numFrames))
        var output = [Float](repeating: 0.0, count: Int(numFrames))

        dsp_engine_process(engine, &input, &output, numFrames)

        // Silence in should produce near-silence out
        let maxOutput = output.map { abs($0) }.max() ?? 0.0
        XCTAssertLessThan(maxOutput, 0.001, "Silence input should produce near-silence output")
    }

    func testProcessingDoesNotCrash() {
        guard let engine = engine else {
            XCTFail("Engine not created")
            return
        }

        // Process multiple times
        for _ in 0..<100 {
            let numFrames: UInt32 = 256
            var input = [Float](repeating: 0.3, count: Int(numFrames))
            var output = [Float](repeating: 0.0, count: Int(numFrames))
            dsp_engine_process(engine, &input, &output, numFrames)
        }
    }

    // MARK: - Amp Controls

    func testSetGain() {
        guard let engine = engine else {
            XCTFail("Engine not created")
            return
        }

        dsp_engine_set_gain(engine, 6.0)
        dsp_engine_set_gain(engine, -6.0)
        dsp_engine_set_gain(engine, 0.0)
        // No crash = pass
    }

    func testSetDrive() {
        guard let engine = engine else {
            XCTFail("Engine not created")
            return
        }

        dsp_engine_set_drive(engine, 0.0)
        dsp_engine_set_drive(engine, 0.5)
        dsp_engine_set_drive(engine, 1.0)
    }

    func testSetEQ() {
        guard let engine = engine else {
            XCTFail("Engine not created")
            return
        }

        dsp_engine_set_bass(engine, -10.0)
        dsp_engine_set_mid(engine, 5.0)
        dsp_engine_set_treble(engine, 10.0)
        dsp_engine_set_presence(engine, 3.0)
    }

    func testSetMaster() {
        guard let engine = engine else {
            XCTFail("Engine not created")
            return
        }

        dsp_engine_set_master(engine, 0.0)
        dsp_engine_set_master(engine, -10.0)
        dsp_engine_set_master(engine, 6.0)
    }

    // MARK: - Effect Toggles

    func testEffectToggles() {
        guard let engine = engine else {
            XCTFail("Engine not created")
            return
        }

        dsp_engine_set_amp_enabled(engine, 0)
        dsp_engine_set_amp_enabled(engine, 1)

        dsp_engine_set_distortion_enabled(engine, 1)
        dsp_engine_set_distortion_enabled(engine, 0)

        dsp_engine_set_delay_enabled(engine, 1)
        dsp_engine_set_delay_enabled(engine, 0)

        dsp_engine_set_reverb_enabled(engine, 1)
        dsp_engine_set_reverb_enabled(engine, 0)
    }

    // MARK: - Distortion Controls

    func testDistortionControls() {
        guard let engine = engine else {
            XCTFail("Engine not created")
            return
        }

        dsp_engine_set_distortion_type(engine, 0) // Overdrive
        dsp_engine_set_distortion_type(engine, 1) // Fuzz
        dsp_engine_set_distortion_type(engine, 2) // Tube
        dsp_engine_set_distortion_type(engine, 3) // HardClip

        dsp_engine_set_distortion_drive(engine, 0.8)
        dsp_engine_set_distortion_tone(engine, 0.5)
        dsp_engine_set_distortion_level(engine, 0.7)
    }

    // MARK: - Delay Controls

    func testDelayControls() {
        guard let engine = engine else {
            XCTFail("Engine not created")
            return
        }

        dsp_engine_set_delay_time(engine, 100.0)
        dsp_engine_set_delay_time(engine, 500.0)
        dsp_engine_set_delay_time(engine, 1000.0)

        dsp_engine_set_delay_feedback(engine, 0.0)
        dsp_engine_set_delay_feedback(engine, 0.5)
        dsp_engine_set_delay_feedback(engine, 0.9)

        dsp_engine_set_delay_mix(engine, 0.0)
        dsp_engine_set_delay_mix(engine, 0.5)
        dsp_engine_set_delay_mix(engine, 1.0)

        dsp_engine_set_delay_first(engine, 1)
        dsp_engine_set_delay_first(engine, 0)
    }

    // MARK: - Reverb Controls

    func testReverbControls() {
        guard let engine = engine else {
            XCTFail("Engine not created")
            return
        }

        dsp_engine_set_reverb_room(engine, 0.0)
        dsp_engine_set_reverb_room(engine, 0.5)
        dsp_engine_set_reverb_room(engine, 1.0)

        dsp_engine_set_reverb_damp(engine, 0.0)
        dsp_engine_set_reverb_damp(engine, 0.5)
        dsp_engine_set_reverb_damp(engine, 1.0)

        dsp_engine_set_reverb_mix(engine, 0.0)
        dsp_engine_set_reverb_mix(engine, 0.5)
        dsp_engine_set_reverb_mix(engine, 1.0)
    }

    // MARK: - Presets

    func testGetPreset() {
        guard let engine = engine else {
            XCTFail("Engine not created")
            return
        }

        var preset = DSPPreset()
        dsp_engine_get_preset(engine, &preset)

        XCTAssertEqual(String(cString: preset.name), "Default")
    }

    func testApplyPreset() {
        guard let engine = engine else {
            XCTFail("Engine not created")
            return
        }

        var preset = DSPPreset()
        strcpy(&preset.name, "Test Preset")
        preset.ampEnabled = 1
        preset.ampGainDb = 6.0
        preset.ampDrive = 0.7
        preset.delayEnabled = 1
        preset.reverbEnabled = 1

        dsp_engine_apply_preset(engine, &preset)

        // Verify preset was applied
        var retrieved = DSPPreset()
        dsp_engine_get_preset(engine, &retrieved)

        XCTAssertEqual(String(cString: retrieved.name), "Test Preset")
        XCTAssertEqual(retrieved.ampGainDb, 6.0, accuracy: 0.001)
        XCTAssertEqual(retrieved.ampDrive, 0.7, accuracy: 0.001)
    }

    func testReset() {
        guard let engine = engine else {
            XCTFail("Engine not created")
            return
        }

        // Process some audio
        var input = [Float](repeating: 0.5, count: 256)
        var output = [Float](repeating: 0.0, count: 256)
        dsp_engine_process(engine, &input, &output, 256)

        // Reset
        dsp_engine_reset(engine)

        // Process again - should work fine
        dsp_engine_process(engine, &input, &output, 256)
    }

    func testLatency() {
        guard let engine = engine else {
            XCTFail("Engine not created")
            return
        }

        let latency = dsp_engine_get_latency(engine)
        XCTAssertGreaterThan(latency, 0.0, "Latency should be positive")

        // At 48kHz with 256 sample buffer, latency should be ~5.33ms
        let expectedLatency = 256.0 / 48000.0
        XCTAssertEqual(latency, expectedLatency, accuracy: 0.0001)
    }
}
