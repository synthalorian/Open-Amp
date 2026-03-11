package com.openamp.midi

import android.content.Context
import android.media.midi.*
import android.os.Handler
import android.os.Looper
import android.util.Log
import java.io.Closeable

class MidiController(private val context: Context) : Closeable {
    
    companion object {
        private const val TAG = "OpenAmp-MIDI"
        const val CC_BANK_SELECT_MSB = 0
        const val CC_MODULATION_WHEEL = 1
        const val CC_VOLUME = 7
        const val CC_EXPRESSION = 11
        const val CC_SUSTAIN = 64
        const val CC_GENERAL_1 = 80
        const val CC_GENERAL_2 = 81
        const val CC_GENERAL_3 = 82
        const val CC_GENERAL_4 = 83
    }
    
    private var midiManager: MidiManager? = null
    private var midiDevice: MidiDevice? = null
    private var inputPort: MidiInputPort? = null
    private var outputPort: MidiOutputPort? = null
    private var midiReceiver: MidiReceiver? = null
    
    private val handler = Handler(Looper.getMainLooper())
    
    // MIDI learn mode
    var isLearning = false
    private var learningCallback: ((Int, Int) -> Unit)? = null // (channel, cc) -> callback
    
    // MIDI mappings: "channel_cc" -> parameter name
    private val ccMappings = mutableMapOf<String, String>()
    
    // Parameter change callbacks
    private val parameterCallbacks = mutableMapOf<String, (Float) -> Unit>()
    
    // MIDI device change callback
    var onDeviceConnected: ((String) -> Unit)? = null
    var onDeviceDisconnected: (() -> Unit)? = null
    
    interface MidiParameterListener {
        fun onParameterChange(parameterName: String, value: Float)
    }
    
    private var listener: MidiParameterListener? = null
    
    fun setListener(l: MidiParameterListener) {
        listener = l
    }
    
    fun initialize(): Boolean {
        midiManager = context.getSystemService(Context.MIDI_SERVICE) as? MidiManager
        if (midiManager == null) {
            Log.w(TAG, "MIDI not available on this device")
            return false
        }
        
        // Register for MIDI device notifications
        midiManager?.registerDeviceCallback(object : MidiManager.DeviceCallback() {
            override fun onDeviceAdded(device: MidiDeviceInfo) {
                Log.i(TAG, "MIDI device added: ${device.properties.getString(MidiDeviceInfo.PROPERTY_NAME)}")
                // Auto-connect if no device connected yet
                if (midiDevice == null) {
                    connectToDevice(device)
                }
            }
            
            override fun onDeviceRemoved(device: MidiDeviceInfo) {
                Log.i(TAG, "MIDI device removed")
                if (midiDevice?.info?.id == device.id) {
                    disconnect()
                    onDeviceDisconnected?.invoke()
                }
            }
        }, handler)
        
        // Check for already connected devices
        val devices = midiManager?.devices ?: return false
        if (devices.isNotEmpty()) {
            connectToDevice(devices[0])
            return true
        }
        
        return true
    }
    
    private fun connectToDevice(deviceInfo: MidiDeviceInfo) {
        midiManager?.openDevice(deviceInfo, { device ->
            if (device == null) {
                Log.e(TAG, "Failed to open MIDI device")
                return@openDevice
            }
            
            midiDevice = device
            
            // Find input port (for receiving MIDI data)
            val portInfos = deviceInfo.ports
            var inputPortIndex = -1
            var outputPortIndex = -1
            
            for (info in portInfos) {
                if (info.type == MidiDeviceInfo.PortInfo.TYPE_OUTPUT) {
                    outputPortIndex = info.portNumber
                } else if (info.type == MidiDeviceInfo.PortInfo.TYPE_INPUT) {
                    inputPortIndex = info.portNumber
                }
            }
            
            if (outputPortIndex >= 0) {
                outputPort = device.openOutputPort(outputPortIndex)
                outputPort?.connect(object : MidiReceiver() {
                    override fun onSend(msg: ByteArray?, offset: Int, count: Int, timestamp: Long) {
                        if (msg != null) {
                            processMidiMessage(msg, offset, count)
                        }
                    }
                })
                
                val deviceName = deviceInfo.properties.getString(MidiDeviceInfo.PROPERTY_NAME) ?: "Unknown"
                Log.i(TAG, "Connected to MIDI device: $deviceName")
                onDeviceConnected?.invoke(deviceName)
            }
        }, handler)
    }
    
    private fun processMidiMessage(msg: ByteArray, offset: Int, count: Int) {
        if (count < 3) return
        
        val status = msg[offset].toInt() and 0xFF
        val channel = status and 0x0F
        val command = status and 0xF0
        
        when (command) {
            0xB0 -> { // Control Change
                val ccNumber = msg[offset + 1].toInt() and 0xFF
                val ccValue = msg[offset + 2].toInt() and 0xFF
                
                handleControlChange(channel, ccNumber, ccValue)
            }
            0xC0 -> { // Program Change
                val program = msg[offset + 1].toInt() and 0xFF
                handleProgramChange(channel, program)
            }
        }
    }
    
    private fun handleControlChange(channel: Int, ccNumber: Int, value: Int) {
        Log.d(TAG, "CC: ch=$channel cc=$ccNumber val=$value")
        
        // If in learn mode, report the CC
        if (isLearning) {
            learningCallback?.invoke(channel, ccNumber)
            return
        }
        
        // Check for mapped parameter
        val mappingKey = "${channel}_$ccNumber"
        val globalMappingKey = "*_$ccNumber" // Global mapping (any channel)
        
        val parameterName = ccMappings[mappingKey] ?: ccMappings[globalMappingKey]
        if (parameterName != null) {
            val normalizedValue = value / 127.0f
            parameterCallbacks[parameterName]?.invoke(normalizedValue)
            listener?.onParameterChange(parameterName, normalizedValue)
        }
        
        // Default mappings for common CCs
        if (parameterName == null) {
            handleDefaultCC(ccNumber, value)
        }
    }
    
    private fun handleDefaultCC(ccNumber: Int, value: Int) {
        val normalizedValue = value / 127.0f
        
        when (ccNumber) {
            CC_EXPRESSION -> {
                // Expression pedal -> output volume
                parameterCallbacks["output_gain"]?.invoke(normalizedValue)
                listener?.onParameterChange("output_gain", normalizedValue)
            }
            CC_MODULATION_WHEEL -> {
                // Mod wheel -> modulation depth
                parameterCallbacks["modulation_depth"]?.invoke(normalizedValue)
                listener?.onParameterChange("modulation_depth", normalizedValue)
            }
            CC_SUSTAIN -> {
                // Sustain pedal -> looper record/play toggle
                if (value >= 64) {
                    parameterCallbacks["looper_toggle"]?.invoke(1.0f)
                    listener?.onParameterChange("looper_toggle", 1.0f)
                }
            }
            CC_GENERAL_1 -> {
                // General purpose -> preset slot A
                if (value >= 64) {
                    parameterCallbacks["preset_a"]?.invoke(1.0f)
                    listener?.onParameterChange("preset_a", 1.0f)
                }
            }
            CC_GENERAL_2 -> {
                // General purpose -> preset slot B
                if (value >= 64) {
                    parameterCallbacks["preset_b"]?.invoke(1.0f)
                    listener?.onParameterChange("preset_b", 1.0f)
                }
            }
            CC_GENERAL_3 -> {
                // General purpose -> preset slot C
                if (value >= 64) {
                    parameterCallbacks["preset_c"]?.invoke(1.0f)
                    listener?.onParameterChange("preset_c", 1.0f)
                }
            }
        }
    }
    
    private fun handleProgramChange(channel: Int, program: Int) {
        Log.d(TAG, "PC: ch=$channel program=$program")
        
        // Program change 0-2 -> preset slots A-C
        when (program) {
            0 -> {
                parameterCallbacks["preset_a"]?.invoke(1.0f)
                listener?.onParameterChange("preset_a", 1.0f)
            }
            1 -> {
                parameterCallbacks["preset_b"]?.invoke(1.0f)
                listener?.onParameterChange("preset_b", 1.0f)
            }
            2 -> {
                parameterCallbacks["preset_c"]?.invoke(1.0f)
                listener?.onParameterChange("preset_c", 1.0f)
            }
        }
    }
    
    // MIDI learn functionality
    fun startLearning(callback: (Int, Int) -> Unit) {
        isLearning = true
        learningCallback = callback
    }
    
    fun stopLearning() {
        isLearning = false
        learningCallback = null
    }
    
    // Map a MIDI CC to a parameter
    fun mapCC(channel: Int, ccNumber: Int, parameterName: String) {
        val key = "${channel}_$ccNumber"
        ccMappings[key] = parameterName
        Log.i(TAG, "Mapped CC $ccNumber (ch $channel) to $parameterName")
    }
    
    // Map a CC globally (any channel)
    fun mapCCGlobal(ccNumber: Int, parameterName: String) {
        val key = "*_$ccNumber"
        ccMappings[key] = parameterName
        Log.i(TAG, "Mapped CC $ccNumber (global) to $parameterName")
    }
    
    // Remove a mapping
    fun unmapCC(channel: Int, ccNumber: Int) {
        val key = "${channel}_$ccNumber"
        ccMappings.remove(key)
    }
    
    // Register a parameter callback
    fun registerParameterCallback(parameterName: String, callback: (Float) -> Unit) {
        parameterCallbacks[parameterName] = callback
    }
    
    // Unregister a parameter callback
    fun unregisterParameterCallback(parameterName: String) {
        parameterCallbacks.remove(parameterName)
    }
    
    // Clear all mappings
    fun clearMappings() {
        ccMappings.clear()
    }
    
    // Get all mappings
    fun getMappings(): Map<String, String> = ccMappings.toMap()
    
    // Send MIDI CC (for feedback to controllers)
    fun sendCC(channel: Int, ccNumber: Int, value: Int) {
        inputPort?.let { port ->
            val msg = byteArrayOf(
                (0xB0 or channel).toByte(),
                ccNumber.toByte(),
                value.coerceIn(0, 127).toByte()
            )
            port.send(msg, 0, msg.size)
        }
    }
    
    // Disconnect from MIDI device
    fun disconnect() {
        outputPort?.close()
        outputPort = null
        inputPort?.close()
        inputPort = null
        midiDevice?.close()
        midiDevice = null
    }
    
    override fun close() {
        disconnect()
    }
    
    // Check if MIDI is available
    fun isAvailable(): Boolean = midiManager != null
    
    // Check if connected to a device
    fun isConnected(): Boolean = midiDevice != null
    
    // Get connected device name
    fun getDeviceName(): String? {
        return midiDevice?.info?.properties?.getString(MidiDeviceInfo.PROPERTY_NAME)
    }
}
