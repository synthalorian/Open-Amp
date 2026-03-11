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

    private lateinit var ampSlotA: Button
    private lateinit var ampSlotB: Button
    private lateinit var ampSlotC: Button

    // New UI components
    private lateinit var neonPresetDisplay: NeonPresetDisplay
    private lateinit var neonTunerDisplay: NeonTunerDisplay
    private lateinit var inputLevelMeter: NeonMeterView
    private lateinit var outputLevelMeter: NeonMeterView

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
            audioEngine = AudioEngine()
        } catch (e: Exception) {
            Log.e(TAG, "Engine init failed", e)
            return
        }

        copyFactoryPresetsFromAssets()
        setContentView(R.layout.activity_main)

        // Find views
        val startButton = findViewById<Button>(R.id.startButton)
        val coffeeButton = findViewById<Button>(R.id.coffeeButton)
        val browsePresetsButton = findViewById<ImageButton>(R.id.browsePresetsButton)
        val cabIrPath = findViewById<EditText>(R.id.cabIrPath)
        val loadCabIrButton = findViewById<Button>(R.id.loadCabIrButton)
        val debugStatusText = findViewById<TextView>(R.id.debugStatusText)

        ampSlotA = findViewById(R.id.ampSlotA)
        ampSlotB = findViewById(R.id.ampSlotB)
        ampSlotC = findViewById(R.id.ampSlotC)

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

        val knobInputGain = findViewById<NeonKnobView>(R.id.knobInputGain)
        val knobOutputGain = findViewById<NeonKnobView>(R.id.knobOutputGain)

        // Configure Knobs
        knobInputGain.setLabel("IN")
        knobOutputGain.setLabel("OUT")
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
            if (running) audioEngine.nativeSetInputGain(inputGainDb.toFloat())
        }
        knobOutputGain.onValueChanged = { v ->
            outputGainDb = (v * 48 - 24).toInt()
            if (running) audioEngine.nativeSetOutputGain(outputGainDb.toFloat())
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

        neonPresetDisplay.setPreset("Init", false)
        updateAmpSlotHighlight(null)

        coffeeButton.setOnClickListener {
            val intent = android.content.Intent(android.content.Intent.ACTION_VIEW)
            intent.data = android.net.Uri.parse("https://buymeacoffee.com/synthalorian")
            startActivity(intent)
        }

        startButton.setOnClickListener {
            if (!hasRecordPermission()) {
                requestRecordPermission()
                return@setOnClickListener
            }
            if (!running) {
                try {
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
                } catch (e: Exception) {
                    Log.e(TAG, "Start error", e)
                }
            } else {
                audioEngine.nativeStop()
                audioEngine.nativeRelease()
                running = false
                stopMeters()
                startButton.text = "START"
            }
        }

        browsePresetsButton.setOnClickListener {
            val presets = listPresets()
            if (presets.isEmpty()) {
                Toast.makeText(this, "No presets found", Toast.LENGTH_SHORT).show()
            } else {
                AlertDialog.Builder(this)
                    .setTitle("Load Preset")
                    .setItems(presets.toTypedArray()) { _, which ->
                        val selected = presets[which]
                        val path = presetPath(selected)
                        if (audioEngine.nativeLoadPreset(path)) {
                            lastPresetName = selected
                            syncFromEngine(
                                cabIrPath,
                                knobInputGain,
                                knobOutputGain,
                                knobAmpGain,
                                knobAmpDrive,
                                knobAmpBass,
                                knobAmpMid,
                                knobAmpTreble,
                                knobAmpPresence,
                                knobAmpMaster,
                                knobDelayTime,
                                knobDelayFeedback,
                                knobDelayMix,
                                knobReverbRoom,
                                knobReverbDamp,
                                knobReverbMix
                            )
                            neonPresetDisplay.setPreset(selected, false)
                        }
                    }
                    .show()
            }
        }

        loadCabIrButton.setOnClickListener {
            irPicker.launch(arrayOf("*/*"))
        }

        ampSlotA.setOnClickListener { loadAmpSlot("A", knobInputGain, knobOutputGain, knobAmpGain, knobAmpDrive, knobAmpBass, knobAmpMid, knobAmpTreble, knobAmpPresence, knobAmpMaster) }
        ampSlotB.setOnClickListener { loadAmpSlot("B", knobInputGain, knobOutputGain, knobAmpGain, knobAmpDrive, knobAmpBass, knobAmpMid, knobAmpTreble, knobAmpPresence, knobAmpMaster) }
        ampSlotC.setOnClickListener { loadAmpSlot("C", knobInputGain, knobOutputGain, knobAmpGain, knobAmpDrive, knobAmpBass, knobAmpMid, knobAmpTreble, knobAmpPresence, knobAmpMaster) }
        
        ampSlotA.setOnLongClickListener { saveAmpSlot("A"); true }
        ampSlotB.setOnLongClickListener { saveAmpSlot("B"); true }
        ampSlotC.setOnLongClickListener { saveAmpSlot("C"); true }
    }

    private fun syncFromEngine(
        cabIrPath: EditText,
        knobIn: NeonKnobView,
        knobOut: NeonKnobView,
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
        
        val cabPath = audioEngine.nativeGetCabIrPath()
        if (cabPath.isNotEmpty()) cabIrPath.setText(cabPath)

        knobIn.setValue((inputGainDb + 24) / 48f)
        knobOut.setValue((outputGainDb + 24) / 48f)
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

        // Sync knobs
        knobs[0].setValue((inputGainDb + 24) / 48f)
        knobs[1].setValue((outputGainDb + 24) / 48f)
        knobs[2].setValue((ampGainDb + 20) / 40f)
        knobs[3].setValue(ampDrive)
        knobs[4].setValue((ampBassDb + 12) / 24f)
        knobs[5].setValue((ampMidDb + 12) / 24f)
        knobs[6].setValue((ampTrebleDb + 12) / 24f)
        knobs[7].setValue((ampPresenceDb + 12) / 24f)
        knobs[8].setValue((ampMasterDb + 10) / 20f)

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
            .apply()
        updateAmpSlotHighlight(slot)
        Toast.makeText(this, "Saved $slot", Toast.LENGTH_SHORT).show()
    }

    private fun updateAmpSlotHighlight(slot: String?) {
        val activeAlpha = 1.0f
        val inactiveAlpha = 0.4f
        ampSlotA.alpha = if (slot == "A") activeAlpha else inactiveAlpha
        ampSlotB.alpha = if (slot == "B") activeAlpha else inactiveAlpha
        ampSlotC.alpha = if (slot == "C") activeAlpha else inactiveAlpha
    }

    private fun listPresets(): List<String> {
        val files = filesDir.listFiles() ?: return emptyList()
        return files.filter { it.name.endsWith(".preset") }
            .map { it.name.removeSuffix(".preset") }
            .sorted()
    }

    private fun presetPath(name: String): String {
        val safeName = name.replace(Regex("[^A-Za-z0-9_-]"), "_")
        return File(filesDir, "$safeName.preset").absolutePath
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
