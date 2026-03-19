package com.openamp

import android.Manifest
import android.app.AlertDialog
import android.content.pm.PackageManager
import android.media.AudioDeviceInfo
import android.media.AudioManager
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.util.Log
import android.widget.Button
import android.widget.EditText
import android.widget.ImageButton
import android.widget.TextView
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.result.contract.ActivityResultContracts
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import com.openamp.ui.NeonKnobView
import com.openamp.ui.NeonMeterView
import com.openamp.ui.NeonPresetDisplay
import com.openamp.ui.NeonTunerDisplay
import com.openamp.ui.NeonLiveButton
import com.openamp.midi.MidiController
import java.io.File
import java.io.FileOutputStream
import java.io.InputStream
import kotlin.math.max

class MainActivity : ComponentActivity() {
    companion object {
        private const val TAG = "Open Amp"
        private const val PREFS_NAME = "openamp_prefs"
        private const val KEY_PRESETS_COPIED = "presets_copied"
    }
    private lateinit var audioEngine: AudioEngine
    private var running = false

    private lateinit var btnLiveA: NeonLiveButton
    private lateinit var btnLiveB: NeonLiveButton
    private lateinit var btnLiveC: NeonLiveButton

    // New UI components
    private lateinit var neonPresetDisplay: NeonPresetDisplay
    private lateinit var neonTunerDisplay: NeonTunerDisplay
    private lateinit var inputLevelMeter: NeonMeterView
    private lateinit var outputLevelMeter: NeonMeterView
    private lateinit var midiController: MidiController

    private var inputGainDb = 18
    private var outputGainDb = 6
    private var delayTimeMs = 350
    private var delayFeedback = 0.35f
    private var delayMix = 0.25f
    private var reverbRoom = 0.5f
    private var reverbDamp = 0.3f
    private var reverbMix = 0.25f
    private var ampGainDb = 0
    private var ampDrive = 0.5f
    private var ampBassDb = 0
    private var ampMidDb = 0
    private var ampTrebleDb = 0
    private var ampPresenceDb = 0
    private var ampMasterDb = 0
    private var lastDebugText = ""

    private var lastPresetName = "Init"
    private val recordPermissionRequest = 1001

    private val meterHandler = Handler(Looper.getMainLooper())
    private var metersRunning = false
    private var pendingAction: (() -> Unit)? = null
    private lateinit var debugStatusText: TextView
    private lateinit var startButton: Button
    private lateinit var btnInputDevice: Button
    private lateinit var btnOutputDevice: Button
    private lateinit var btnInputMode: Button

    private val irPicker = registerForActivityResult(ActivityResultContracts.OpenDocument()) { uri ->
        if (uri == null) return@registerForActivityResult
        val cachedPath = cacheIrFromUri(uri)
        if (cachedPath != null) {
            findViewById<EditText>(R.id.cabIrPath).setText(cachedPath)
            val ok = audioEngine.nativeSetCabIRFromFile(cachedPath)
            Toast.makeText(this, if (ok) "IR loaded" else "IR load failed", Toast.LENGTH_SHORT).show()
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        try {
            try {
                setContentView(R.layout.activity_main)
            } catch (e: Throwable) {
                Log.e(TAG, "Layout inflate failed", e)
                val cause = e.cause
                val msg = cause?.javaClass?.simpleName ?: e.javaClass.simpleName
                Toast.makeText(this, "App failed to start: $msg ${e.message}", Toast.LENGTH_LONG).show()
                return
            }

            try {
                audioEngine = AudioEngine()
            } catch (e: Exception) {
                Log.e(TAG, "Engine init failed", e)
                Toast.makeText(this, "Audio engine init failed: ${e.message}", Toast.LENGTH_LONG).show()
                return
            } catch (e: UnsatisfiedLinkError) {
                Log.e(TAG, "Native library load failed", e)
                Toast.makeText(this, "Native library load failed: ${e.message}", Toast.LENGTH_LONG).show()
                return
            }

            copyFactoryPresetsFromAssets()

            // Find views
        startButton = findViewById(R.id.startButton)
        val coffeeButton = findViewById<Button>(R.id.coffeeButton)
        val browsePresetsButton = findViewById<ImageButton>(R.id.browsePresetsButton)
        val loadCabIrButton = findViewById<Button>(R.id.loadCabIrButton)
        debugStatusText = findViewById(R.id.debugStatusText)
        val etMetronomeBpm = findViewById<EditText>(R.id.etMetronomeBpm)
        btnInputDevice = findViewById(R.id.btnInputDevice)
        btnOutputDevice = findViewById(R.id.btnOutputDevice)
        btnInputMode = findViewById(R.id.btnInputMode)

        btnLiveA = findViewById(R.id.btnLiveA)
        btnLiveB = findViewById(R.id.btnLiveB)
        btnLiveC = findViewById(R.id.btnLiveC)
        
        btnLiveA.setLabel("A")
        btnLiveB.setLabel("B")
        btnLiveC.setLabel("C")

        // Looper & Metronome views
        val btnLooperRec = findViewById<Button>(R.id.btnLooperRec)
        val btnLooperPlay = findViewById<Button>(R.id.btnLooperPlay)
        val btnLooperStop = findViewById<Button>(R.id.btnLooperStop)
        val btnLooperClear = findViewById<Button>(R.id.btnLooperClear)
        val btnMetronomeToggle = findViewById<Button>(R.id.btnMetronomeToggle)
        val knobMetronomeVol = findViewById<NeonKnobView>(R.id.knobMetronomeVol)

        neonPresetDisplay = findViewById(R.id.neonPresetDisplay)
        neonTunerDisplay = findViewById(R.id.neonTunerDisplay)
        inputLevelMeter = findViewById(R.id.inputLevelMeter)
        outputLevelMeter = findViewById(R.id.outputLevelMeter)

        // Find Knobs
        val knobAmpGain = findViewById<NeonKnobView>(R.id.knobAmpGain)
        val knobAmpDrive = findViewById<NeonKnobView>(R.id.knobAmpDrive)
        val knobAmpBass = findViewById<NeonKnobView>(R.id.knobAmpBass)
        val knobAmpMid = findViewById<NeonKnobView>(R.id.knobAmpMid)
        val knobAmpTreble = findViewById<NeonKnobView>(R.id.knobAmpTreble)
        val knobAmpPresence = findViewById<NeonKnobView>(R.id.knobAmpPresence)
        val knobAmpMaster = findViewById<NeonKnobView>(R.id.knobAmpMaster)

        val knobDelayTime = findViewById<NeonKnobView>(R.id.knobDelayTime)
        val knobDelayFeedback = findViewById<NeonKnobView>(R.id.knobDelayFeedback)
        val knobDelayMix = findViewById<NeonKnobView>(R.id.knobDelayMix)
        val knobReverbRoom = findViewById<NeonKnobView>(R.id.knobReverbRoom)
        val knobReverbDamp = findViewById<NeonKnobView>(R.id.knobReverbDamp)
        val knobReverbMix = findViewById<NeonKnobView>(R.id.knobReverbMix)
        val knobNoiseGate = findViewById<NeonKnobView>(R.id.knobNoiseGate)
        val knobInputGain = findViewById<NeonKnobView>(R.id.knobInputGain)
        val knobOutputGain = findViewById<NeonKnobView>(R.id.knobOutputGain)

        // Configure Knobs
        knobInputGain.setLabel("IN")
        knobOutputGain.setLabel("OUT")
        knobNoiseGate.setLabel("GATE")
        knobAmpGain.setLabel("GAIN")
        knobAmpDrive.setLabel("DRIVE")
        knobAmpBass.setLabel("BASS")
        knobAmpMid.setLabel("MID")
        knobAmpTreble.setLabel("TREBLE")
        knobAmpPresence.setLabel("PRESENCE")
        knobAmpMaster.setLabel("MASTER")

        knobDelayTime.setLabel("TIME")
        knobDelayFeedback.setLabel("FBACK")
        knobDelayMix.setLabel("DLY MIX")
        knobReverbRoom.setLabel("ROOM")
        knobReverbDamp.setLabel("DAMP")
        knobReverbMix.setLabel("RVB MIX")
        knobMetronomeVol.setLabel("METRO")

        // Knob Change Listeners
        knobAmpGain.onValueChanged = { v ->
            ampGainDb = (v * 40 - 20).toInt()
            if (running) audioEngine.nativeSetAmpGainDb(ampGainDb.toFloat())
            neonPresetDisplay.setPreset(lastPresetName, true)
        }
        knobAmpDrive.onValueChanged = { v ->
            ampDrive = v
            if (running) audioEngine.nativeSetAmpDrive(ampDrive)
            neonPresetDisplay.setPreset(lastPresetName, true)
        }
        knobAmpBass.onValueChanged = { v ->
            ampBassDb = (v * 24 - 12).toInt()
            if (running) audioEngine.nativeSetAmpBassDb(ampBassDb.toFloat())
            neonPresetDisplay.setPreset(lastPresetName, true)
        }
        knobAmpMid.onValueChanged = { v ->
            ampMidDb = (v * 24 - 12).toInt()
            if (running) audioEngine.nativeSetAmpMidDb(ampMidDb.toFloat())
            neonPresetDisplay.setPreset(lastPresetName, true)
        }
        knobAmpTreble.onValueChanged = { v ->
            ampTrebleDb = (v * 24 - 12).toInt()
            if (running) audioEngine.nativeSetAmpTrebleDb(ampTrebleDb.toFloat())
            neonPresetDisplay.setPreset(lastPresetName, true)
        }
        knobAmpPresence.onValueChanged = { v ->
            ampPresenceDb = (v * 24 - 12).toInt()
            if (running) audioEngine.nativeSetAmpPresenceDb(ampPresenceDb.toFloat())
            neonPresetDisplay.setPreset(lastPresetName, true)
        }
        knobAmpMaster.onValueChanged = { v ->
            ampMasterDb = (v * 20 - 10).toInt()
            if (running) audioEngine.nativeSetAmpMasterDb(ampMasterDb.toFloat())
            neonPresetDisplay.setPreset(lastPresetName, true)
        }

        knobInputGain.onValueChanged = { v ->
            inputGainDb = (v * 48 - 24).toInt()
            runWhenEngineRunning("Input gain") {
                audioEngine.nativeSetInputGain(inputGainDb.toFloat())
            }
        }
        knobOutputGain.onValueChanged = { v ->
            outputGainDb = (v * 48 - 24).toInt()
            runWhenEngineRunning("Output gain") {
                audioEngine.nativeSetOutputGain(outputGainDb.toFloat())
            }
        }

        knobDelayTime.onValueChanged = { v ->
            delayTimeMs = (v * 2000).toInt().coerceAtLeast(1)
            if (running) audioEngine.nativeSetDelayTimeMs(delayTimeMs.toFloat())
            neonPresetDisplay.setPreset(lastPresetName, true)
        }
        knobDelayFeedback.onValueChanged = { v ->
            delayFeedback = v.coerceAtMost(0.95f)
            if (running) audioEngine.nativeSetDelayFeedback(delayFeedback)
            neonPresetDisplay.setPreset(lastPresetName, true)
        }
        knobDelayMix.onValueChanged = { v ->
            delayMix = v
            if (running) audioEngine.nativeSetDelayMix(delayMix)
            neonPresetDisplay.setPreset(lastPresetName, true)
        }
        knobReverbRoom.onValueChanged = { v ->
            reverbRoom = v
            if (running) audioEngine.nativeSetReverbRoomSize(reverbRoom)
            neonPresetDisplay.setPreset(lastPresetName, true)
        }
        knobReverbDamp.onValueChanged = { v ->
            reverbDamp = v
            if (running) audioEngine.nativeSetReverbDamping(reverbDamp)
            neonPresetDisplay.setPreset(lastPresetName, true)
        }
        knobReverbMix.onValueChanged = { v ->
            reverbMix = v
            if (running) audioEngine.nativeSetReverbMix(reverbMix)
            neonPresetDisplay.setPreset(lastPresetName, true)
        }

        knobMetronomeVol.onValueChanged = { v ->
            if (running) audioEngine.nativeMetronomeSetVolume(v)
        }

        neonPresetDisplay.setPreset("Init", false)
        updateAmpSlotHighlight(null)
        
        // Initialize noise gate
        audioEngine.nativeSetNoiseGateEnabled(true)
        audioEngine.nativeSetNoiseGateThreshold(-45.0f) // Default threshold
        audioEngine.nativeSetNoiseGateAttack(1.0f)
        audioEngine.nativeSetNoiseGateRelease(100.0f)

        // Initialize MIDI Controller
        midiController = MidiController(this)
        if (midiController.initialize()) {
            midiController.onDeviceConnected = { deviceName ->
                Toast.makeText(this, "MIDI: $deviceName", Toast.LENGTH_SHORT).show()
            }
            midiController.onDeviceDisconnected = {
                Toast.makeText(this, "MIDI disconnected", Toast.LENGTH_SHORT).show()
            }
            
            // Set up default parameter callbacks
            midiController.registerParameterCallback("preset_a") { loadAmpSlot("A", knobInputGain, knobOutputGain, knobNoiseGate, knobAmpGain, knobAmpDrive, knobAmpBass, knobAmpMid, knobAmpTreble, knobAmpPresence, knobAmpMaster) }
            midiController.registerParameterCallback("preset_b") { loadAmpSlot("B", knobInputGain, knobOutputGain, knobNoiseGate, knobAmpGain, knobAmpDrive, knobAmpBass, knobAmpMid, knobAmpTreble, knobAmpPresence, knobAmpMaster) }
            midiController.registerParameterCallback("preset_c") { loadAmpSlot("C", knobInputGain, knobOutputGain, knobNoiseGate, knobAmpGain, knobAmpDrive, knobAmpBass, knobAmpMid, knobAmpTreble, knobAmpPresence, knobAmpMaster) }
            midiController.registerParameterCallback("output_gain") { v ->
                outputGainDb = (v * 48 - 24).toInt()
                knobOutputGain.setValue(v)
                if (running) audioEngine.nativeSetOutputGain(outputGainDb.toFloat())
            }
            midiController.registerParameterCallback("looper_toggle") { _ ->
                if (running) audioEngine.nativeLooperRecord()
            }
        }

        coffeeButton.setOnClickListener {
            val intent = android.content.Intent(android.content.Intent.ACTION_VIEW)
            intent.data = android.net.Uri.parse("https://buymeacoffee.com/synthalorian")
            startActivity(intent)
        }

        val openSourceButton = findViewById<Button>(R.id.openSourceButton)
        openSourceButton.setOnClickListener {
            val intent = android.content.Intent(android.content.Intent.ACTION_VIEW)
            intent.data = android.net.Uri.parse("https://github.com/synthalorian/Open-Amp")
            startActivity(intent)
        }

        startButton.setOnClickListener {
            if (!running) {
                runWhenEngineRunning("Start") {
                    // no-op after startup
                }
            } else {
                stopEngine(startButton)
            }
        }

        browsePresetsButton.setOnClickListener {
            runWhenEngineRunning("Preset browser") {
                showPresetCategories()
            }
        }

        loadCabIrButton.setOnClickListener {
            irPicker.launch(arrayOf("*/*"))
        }

        btnInputDevice.setOnClickListener {
            runWhenEngineRunning("Input source") {
                showInputDeviceSelector()
            }
        }

        btnOutputDevice.setOnClickListener {
            runWhenEngineRunning("Output source") {
                showOutputDeviceSelector()
            }
        }

        btnInputMode.setOnClickListener {
            runWhenEngineRunning("Input mode") {
                showInputModeSelector()
            }
        }

        btnLiveA.onClick = {
            runWhenEngineRunning("Load slot A") { loadAmpSlot("A", knobInputGain, knobOutputGain, knobNoiseGate, knobAmpGain, knobAmpDrive, knobAmpBass, knobAmpMid, knobAmpTreble, knobAmpPresence, knobAmpMaster) }
        }
        btnLiveB.onClick = {
            runWhenEngineRunning("Load slot B") { loadAmpSlot("B", knobInputGain, knobOutputGain, knobNoiseGate, knobAmpGain, knobAmpDrive, knobAmpBass, knobAmpMid, knobAmpTreble, knobAmpPresence, knobAmpMaster) }
        }
        btnLiveC.onClick = {
            runWhenEngineRunning("Load slot C") { loadAmpSlot("C", knobInputGain, knobOutputGain, knobNoiseGate, knobAmpGain, knobAmpDrive, knobAmpBass, knobAmpMid, knobAmpTreble, knobAmpPresence, knobAmpMaster) }
        }
        
        btnLiveA.onLongClick = { saveAmpSlot("A") }
        btnLiveB.onLongClick = { saveAmpSlot("B") }
        btnLiveC.onLongClick = { saveAmpSlot("C") }
        
        // Looper Listeners
        btnLooperRec.setOnClickListener {
            runWhenEngineRunning("Looper REC") {
                audioEngine.nativeLooperRecord()
            }
        }
        btnLooperPlay.setOnClickListener {
            runWhenEngineRunning("Looper PLAY") {
                audioEngine.nativeLooperPlay()
            }
        }
        btnLooperStop.setOnClickListener {
            runWhenEngineRunning("Looper STOP") {
                audioEngine.nativeLooperStop()
            }
        }
        btnLooperClear.setOnClickListener {
            runWhenEngineRunning("Looper CLEAR") {
                audioEngine.nativeLooperClear()
            }
        }

        // Metronome Listeners
        btnMetronomeToggle.setOnClickListener {
            runWhenEngineRunning("Metronome") {
                if (audioEngine.nativeMetronomeIsPlaying()) {
                    audioEngine.nativeMetronomeStop()
                    btnMetronomeToggle.text = "METRO: OFF"
                } else {
                    val bpm = etMetronomeBpm.text.toString().toFloatOrNull() ?: 120f
                    audioEngine.nativeMetronomeSetTempo(bpm)
                    audioEngine.nativeMetronomeStart()
                    btnMetronomeToggle.text = "METRO: ON"
                }
            }
        }
        } catch (e: Throwable) {
            Log.e(TAG, "App startup failed", e)
            Toast.makeText(this, "App failed to start: ${e.javaClass.simpleName} ${e.message}", Toast.LENGTH_LONG).show()
        }
    }

    private fun syncFromEngine(
        cabIrPath: EditText,
        knobIn: NeonKnobView,
        knobOut: NeonKnobView,
        knobGate: NeonKnobView,
        knobGain: NeonKnobView,
        knobDrive: NeonKnobView,
        knobBass: NeonKnobView,
        knobMid: NeonKnobView,
        knobTreble: NeonKnobView,
        knobPresence: NeonKnobView,
        knobMaster: NeonKnobView,
        knobDlyTime: NeonKnobView,
        knobDlyFeedback: NeonKnobView,
        knobDlyMix: NeonKnobView,
        knobRvbRoom: NeonKnobView,
        knobRvbDamp: NeonKnobView,
        knobRvbMix: NeonKnobView
    ) {
        inputGainDb = audioEngine.nativeGetInputGainDb().toInt()
        outputGainDb = audioEngine.nativeGetOutputGainDb().toInt()
        ampGainDb = audioEngine.nativeGetAmpGainDb().toInt()
        ampDrive = audioEngine.nativeGetAmpDrive()
        ampBassDb = audioEngine.nativeGetAmpBassDb().toInt()
        ampMidDb = audioEngine.nativeGetAmpMidDb().toInt()
        ampTrebleDb = audioEngine.nativeGetAmpTrebleDb().toInt()
        ampPresenceDb = audioEngine.nativeGetAmpPresenceDb().toInt()
        ampMasterDb = audioEngine.nativeGetAmpMasterDb().toInt()
        
        delayTimeMs = audioEngine.nativeGetDelayTimeMs().toInt()
        delayFeedback = audioEngine.nativeGetDelayFeedback()
        delayMix = audioEngine.nativeGetDelayMix()
        reverbRoom = audioEngine.nativeGetReverbRoom()
        reverbDamp = audioEngine.nativeGetReverbDamp()
        reverbMix = audioEngine.nativeGetReverbMix()
        
        val noiseGateThreshold = audioEngine.nativeGetNoiseGateThreshold()
        
        val cabPath = audioEngine.nativeGetCabIrPath()
        if (cabPath.isNotEmpty()) cabIrPath.setText(cabPath)

        knobIn.setValue((inputGainDb + 24) / 48f)
        knobOut.setValue((outputGainDb + 24) / 48f)
        knobGate.setValue((noiseGateThreshold + 35) / 60f) // -35 to -95 dB
        knobGain.setValue((ampGainDb + 20) / 40f)
        knobDrive.setValue(ampDrive)
        knobBass.setValue((ampBassDb + 12) / 24f)
        knobMid.setValue((ampMidDb + 12) / 24f)
        knobTreble.setValue((ampTrebleDb + 12) / 24f)
        knobPresence.setValue((ampPresenceDb + 12) / 24f)
        knobMaster.setValue((ampMasterDb + 10) / 20f)

        knobDlyTime.setValue(delayTimeMs / 2000f)
        knobDlyFeedback.setValue(delayFeedback)
        knobDlyMix.setValue(delayMix)
        knobRvbRoom.setValue(reverbRoom)
        knobRvbDamp.setValue(reverbDamp)
        knobRvbMix.setValue(reverbMix)

        if (running) applyCurrentSettings()
    }

    private fun applyCurrentSettings() {
        audioEngine.nativeSetInputGain(inputGainDb.toFloat())
        audioEngine.nativeSetOutputGain(outputGainDb.toFloat())
        audioEngine.nativeSetAmpGainDb(ampGainDb.toFloat())
        audioEngine.nativeSetAmpDrive(ampDrive)
        audioEngine.nativeSetAmpBassDb(ampBassDb.toFloat())
        audioEngine.nativeSetAmpMidDb(ampMidDb.toFloat())
        audioEngine.nativeSetAmpTrebleDb(ampTrebleDb.toFloat())
        audioEngine.nativeSetAmpPresenceDb(ampPresenceDb.toFloat())
        audioEngine.nativeSetAmpMasterDb(ampMasterDb.toFloat())
        audioEngine.nativeSetDelayTimeMs(delayTimeMs.toFloat())
        audioEngine.nativeSetDelayFeedback(delayFeedback)
        audioEngine.nativeSetDelayMix(delayMix)
        audioEngine.nativeSetReverbRoomSize(reverbRoom)
        audioEngine.nativeSetReverbDamping(reverbDamp)
        audioEngine.nativeSetReverbMix(reverbMix)
    }

    private fun runWhenEngineRunning(label: String, action: () -> Unit) {
        if (running) {
            action()
            return
        }

        pendingAction = action
        if (hasRecordPermission()) {
            if (startEngine(startButton, debugStatusText)) {
                val queued = pendingAction
                pendingAction = null
                queued?.invoke()
            } else {
                pendingAction = null
                Toast.makeText(this, "$label blocked: unable to start engine", Toast.LENGTH_SHORT).show()
            }
            return
        }

        Toast.makeText(this, "$label: requesting mic permission", Toast.LENGTH_SHORT).show()
        requestRecordPermission()
    }

    private fun startEngine(startButton: Button, debugStatusText: TextView): Boolean {
        return try {
            audioEngine.nativeCreate()
            applyCurrentSettings()
            running = audioEngine.nativeStart()
            if (running) {
                startButton.text = "STOP"
                startMeters(debugStatusText)
                Toast.makeText(this, "Engine Active", Toast.LENGTH_SHORT).show()
            } else {
                Toast.makeText(this, "Start Failed", Toast.LENGTH_LONG).show()
            }
            running
        } catch (e: Exception) {
            Log.e(TAG, "Start error", e)
            Toast.makeText(this, "Start failed: ${e.message}", Toast.LENGTH_LONG).show()
            false
        }
    }

    private fun stopEngine(startButton: Button) {
        try {
            audioEngine.nativeStop()
            audioEngine.nativeRelease()
            running = false
            stopMeters()
            startButton.text = "START"
        } catch (e: Exception) {
            Log.e(TAG, "Stop error", e)
        }
    }

    private fun startMeters(debugStatusText: TextView) {
        if (metersRunning) return
        metersRunning = true
        audioEngine.nativeResetClipIndicator()
        meterHandler.post(object : Runnable {
            override fun run() {
                if (!metersRunning) return
                val input = audioEngine.nativeGetInputLevel().coerceIn(0.0f, 1.0f)
                val output = audioEngine.nativeGetOutputLevel().coerceIn(0.0f, 1.0f)

                inputLevelMeter.setLevel(input)
                outputLevelMeter.setLevel(output)

                val debug = audioEngine.nativeGetDebugStatus()
                if (debug != lastDebugText) {
                    debugStatusText.text = debug
                    lastDebugText = debug
                }
                
                neonTunerDisplay.setNote("E", 0f, true)
                
                // Sync Metronome Button State
                if (running) {
                    val isMetroOn = audioEngine.nativeMetronomeIsPlaying()
                    val toggleBtn = findViewById<Button>(R.id.btnMetronomeToggle)
                    toggleBtn.text = if (isMetroOn) "METRO: ON" else "METRO: OFF"
                }

                meterHandler.postDelayed(this, 100)
            }
        })
    }

    private fun stopMeters() {
        metersRunning = false
        meterHandler.removeCallbacksAndMessages(null)
    }

    private fun loadAmpSlot(slot: String, vararg knobs: NeonKnobView) {
        val prefs = getSharedPreferences("amp_slots", MODE_PRIVATE)
        if (!prefs.contains("${slot}_gain")) {
            Toast.makeText(this, "Empty slot $slot", Toast.LENGTH_SHORT).show()
            return
        }
        ampGainDb = prefs.getInt("${slot}_gain", 0)
        ampDrive = prefs.getFloat("${slot}_drive", 0.5f)
        ampBassDb = prefs.getInt("${slot}_bass", 0)
        ampMidDb = prefs.getInt("${slot}_mid", 0)
        ampTrebleDb = prefs.getInt("${slot}_treble", 0)
        ampPresenceDb = prefs.getInt("${slot}_presence", 0)
        ampMasterDb = prefs.getInt("${slot}_master", 0)
        inputGainDb = prefs.getInt("${slot}_input_gain", 0)
        outputGainDb = prefs.getInt("${slot}_output_gain", 0)

        // Sync knobs
        if (knobs.size >= 9) {
            knobs[0].setValue((inputGainDb + 24) / 48f)
            knobs[1].setValue((outputGainDb + 24) / 48f)
            // Use current noise gate threshold since it's not in amp slots yet
            val gateThreshold = if (running) audioEngine.nativeGetNoiseGateThreshold() else -45.0f
            knobs[2].setValue((gateThreshold + 35) / 60f)
            knobs[3].setValue((ampGainDb + 20) / 40f)
            knobs[4].setValue(ampDrive)
            knobs[5].setValue((ampBassDb + 12) / 24f)
            knobs[6].setValue((ampMidDb + 12) / 24f)
            knobs[7].setValue((ampTrebleDb + 12) / 24f)
            knobs[8].setValue((ampPresenceDb + 12) / 24f)
            // Note: master knob usually exists but let's check size
            if (knobs.size >= 10) knobs[9].setValue((ampMasterDb + 10) / 20f)
        }

        applyCurrentSettings()
        updateAmpSlotHighlight(slot)
        neonPresetDisplay.setPreset("Slot $slot", false)
    }

    private fun saveAmpSlot(slot: String) {
        val prefs = getSharedPreferences("amp_slots", MODE_PRIVATE)
        prefs.edit()
            .putInt("${slot}_gain", ampGainDb)
            .putFloat("${slot}_drive", ampDrive)
            .putInt("${slot}_bass", ampBassDb)
            .putInt("${slot}_mid", ampMidDb)
            .putInt("${slot}_treble", ampTrebleDb)
            .putInt("${slot}_presence", ampPresenceDb)
            .putInt("${slot}_master", ampMasterDb)
            .putInt("${slot}_input_gain", inputGainDb)
            .putInt("${slot}_output_gain", outputGainDb)
            .apply()
        updateAmpSlotHighlight(slot)
        Toast.makeText(this, "Saved $slot", Toast.LENGTH_SHORT).show()
    }

    private fun updateAmpSlotHighlight(slot: String?) {
        btnLiveA.setActive(slot == "A")
        btnLiveB.setActive(slot == "B")
        btnLiveC.setActive(slot == "C")
    }

    private fun showSavePresetDialog() {
        val input = EditText(this)
        input.hint = "Preset name"
        input.setText(lastPresetName)
        
        AlertDialog.Builder(this)
            .setTitle("Save Preset")
            .setView(input)
            .setPositiveButton("Save") { _, _ ->
                val name = input.text.toString().ifBlank { "Untitled" }
                saveCurrentPreset(name)
            }
            .setNegativeButton("Cancel", null)
            .show()
    }

    private fun saveCurrentPreset(name: String) {
        val path = presetPath(name, filesDir)
        if (audioEngine.nativeSavePreset(path, name)) {
            lastPresetName = name
            neonPresetDisplay.setPreset(name, false)
            Toast.makeText(this, "Saved: $name", Toast.LENGTH_SHORT).show()
        } else {
            Toast.makeText(this, "Save failed", Toast.LENGTH_SHORT).show()
        }
    }

    private fun showPresetCategories() {
        val factoryDir = File(filesDir, "presets")
        val categories = mutableMapOf<String, Pair<List<String>, File>>()
        categories["Saved"] = Pair(listPresets(filesDir).sorted(), filesDir)
        categories["Factory"] = Pair(listPresetNames(factoryDir).sorted(), factoryDir)

        val categoryNames = categories.keys.toList()
        AlertDialog.Builder(this)
            .setTitle("Preset Categories")
            .setItems(categoryNames.toTypedArray()) { _, which ->
                val category = categoryNames[which]
                val entry = categories[category]
                val presets = entry?.first ?: emptyList()
                val sourceDir = entry?.second ?: filesDir
                if (presets.isEmpty()) {
                    Toast.makeText(this, "No presets in $category", Toast.LENGTH_SHORT).show()
                } else {
                    showPresetList(presets, sourceDir)
                }
            }
            .setNeutralButton("Save Current") { _, _ ->
                showSavePresetDialog()
            }
            .show()
    }

    private fun showPresetList(presets: List<String>, sourceDir: File) {
        AlertDialog.Builder(this)
            .setTitle("Select Preset")
            .setItems(presets.toTypedArray()) { _, which ->
                val selected = presets[which]
                val path = presetPath(selected, sourceDir, sanitize = false)
                val apply = {
                    if (audioEngine.nativeLoadPreset(path)) {
                        lastPresetName = selected
                        syncFromEngine(
                            findViewById(R.id.cabIrPath),
                            findViewById(R.id.knobInputGain),
                            findViewById(R.id.knobOutputGain),
                            findViewById(R.id.knobNoiseGate),
                            findViewById(R.id.knobAmpGain),
                            findViewById(R.id.knobAmpDrive),
                            findViewById(R.id.knobAmpBass),
                            findViewById(R.id.knobAmpMid),
                            findViewById(R.id.knobAmpTreble),
                            findViewById(R.id.knobAmpPresence),
                            findViewById(R.id.knobAmpMaster),
                            findViewById(R.id.knobDelayTime),
                            findViewById(R.id.knobDelayFeedback),
                            findViewById(R.id.knobDelayMix),
                            findViewById(R.id.knobReverbRoom),
                            findViewById(R.id.knobReverbDamp),
                            findViewById(R.id.knobReverbMix)
                        )
                        neonPresetDisplay.setPreset(selected, false)
                        val state = "Loaded: $selected | IN=${inputGainDb}dB OUT=${outputGainDb}dB Delay=${delayTimeMs}ms Reverb=${reverbMix}" 
                        Toast.makeText(this, state, Toast.LENGTH_LONG).show()
                        lastDebugText = state
                        debugStatusText.text = state
                    } else {
                        Toast.makeText(this, "Load failed: $selected", Toast.LENGTH_SHORT).show()
                    }
                }
                runWhenEngineRunning("Preset: $selected") {
                    apply()
                }
            }
            .show()
    }

    private fun listPresetNames(dir: File): List<String> {
        val files = dir.listFiles() ?: return emptyList()
        return files.filter { it.name.endsWith(".preset") }
            .map { it.name.removeSuffix(".preset") }
            .sorted()
    }

    private fun listPresets(dir: File): List<String> = listPresetNames(dir)

    private fun presetPath(name: String, dir: File, sanitize: Boolean = true): String {
        val safeName = if (sanitize) {
            name.replace(Regex("[^A-Za-z0-9_-]"), "_")
        } else {
            name
        }
        return File(dir, "$safeName.preset").absolutePath
    }

    private fun showInputDeviceSelector() {
        val manager = getSystemService(AUDIO_SERVICE) as AudioManager
        val devices = manager.getDevices(AudioManager.GET_DEVICES_INPUTS)
            .filter { it.type != AudioDeviceInfo.TYPE_UNKNOWN }

        if (devices.isEmpty()) {
            Toast.makeText(this, "No input devices available", Toast.LENGTH_SHORT).show()
            return
        }

        val names = devices.map { device ->
            val typeName = device.productName?.toString() ?: "Unknown"
            "${device.type}: $typeName (${device.id})"
        }
        val ids = devices.map { it.id }

        AlertDialog.Builder(this)
            .setTitle("Select Input Device")
            .setItems(names.toTypedArray()) { _, which ->
                val id = ids[which]
                audioEngine.nativeSetInputDeviceId(id)
                btnInputDevice.text = names[which].substringBefore(" (")
                Toast.makeText(this, "Input device set", Toast.LENGTH_SHORT).show()
            }
            .setNegativeButton("Cancel", null)
            .show()
    }

    private fun showOutputDeviceSelector() {
        val manager = getSystemService(AUDIO_SERVICE) as AudioManager
        val devices = manager.getDevices(AudioManager.GET_DEVICES_OUTPUTS)
            .filter { it.type != AudioDeviceInfo.TYPE_UNKNOWN }

        if (devices.isEmpty()) {
            Toast.makeText(this, "No output devices available", Toast.LENGTH_SHORT).show()
            return
        }

        val names = devices.map { device ->
            val typeName = device.productName?.toString() ?: "Unknown"
            "${device.type}: $typeName (${device.id})"
        }
        val ids = devices.map { it.id }

        AlertDialog.Builder(this)
            .setTitle("Select Output Device")
            .setItems(names.toTypedArray()) { _, which ->
                val id = ids[which]
                audioEngine.nativeSetOutputDeviceId(id)
                btnOutputDevice.text = names[which].substringBefore(" (")
                Toast.makeText(this, "Output device set", Toast.LENGTH_SHORT).show()
            }
            .setNegativeButton("Cancel", null)
            .show()
    }

    private fun showInputModeSelector() {
        val modeLabels = arrayOf("L/Mono", "R", "Sum", "Auto")
        AlertDialog.Builder(this)
            .setTitle("Input Channel Mode")
            .setItems(modeLabels) { _, which ->
                audioEngine.nativeSetInputChannelMode(which)
                btnInputMode.text = modeLabels[which]
            }
            .setNegativeButton("Cancel", null)
            .show()
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)

        if (requestCode == recordPermissionRequest) {
            if (grantResults.isNotEmpty() && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                Toast.makeText(this, "Microphone permission granted", Toast.LENGTH_SHORT).show()
                if (startEngine(startButton, debugStatusText)) {
                    val action = pendingAction
                    pendingAction = null
                    action?.invoke()
                }
            } else {
                pendingAction = null
                Toast.makeText(this, "Record permission denied. Controls blocked until granted.", Toast.LENGTH_LONG).show()
            }
        }
    }

    private fun cacheIrFromUri(uri: android.net.Uri): String? {
        return try {
            contentResolver.openInputStream(uri)?.use { input ->
                val irDir = File(filesDir, "ir_cache")
                if (!irDir.exists()) irDir.mkdirs()
                val target = File(irDir, "cab_ir_${System.currentTimeMillis()}.txt")
                FileOutputStream(target).use { output ->
                    val buffer = ByteArray(8 * 1024)
                    while (true) {
                        val read = input.read(buffer)
                        if (read <= 0) break
                        output.write(buffer, 0, read)
                    }
                }
                target.absolutePath
            }
        } catch (e: Exception) { null }
    }

    private fun hasRecordPermission(): Boolean {
        return ContextCompat.checkSelfPermission(this, Manifest.permission.RECORD_AUDIO) == PackageManager.PERMISSION_GRANTED
    }

    private fun requestRecordPermission() {
        ActivityCompat.requestPermissions(this, arrayOf(Manifest.permission.RECORD_AUDIO), recordPermissionRequest)
    }

    private fun copyFactoryPresetsFromAssets() {
        val prefs = getSharedPreferences(PREFS_NAME, MODE_PRIVATE)
        if (prefs.getBoolean(KEY_PRESETS_COPIED, false)) return
        try {
            val presetDir = File(filesDir, "presets")
            if (!presetDir.exists()) presetDir.mkdirs()
            assets.list("presets")?.forEach { filename ->
                assets.open("presets/$filename").use { input ->
                    File(presetDir, filename).outputStream().use { output ->
                        input.copyTo(output)
                    }
                }
            }
            prefs.edit().putBoolean(KEY_PRESETS_COPIED, true).apply()
        } catch (e: Exception) { Log.e(TAG, "Asset copy failed", e) }
    }
}
