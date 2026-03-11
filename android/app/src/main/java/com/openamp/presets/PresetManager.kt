package com.openamp.presets

import android.content.Context
import android.util.Log
import java.io.File
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors

class PresetManager(private val context: Context) {
    private val TAG = "OpenAmp-Presets"
    private val presetDir = File(context.filesDir, "presets")
    private val executor: ExecutorService = Executors.newSingleThreadExecutor()

    init {
        if (!presetDir.exists()) {
            presetDir.mkdirs()
        }
    }

    fun getPresetList(): List<String> {
        return presetDir.listFiles()?.filter { it.name.endsWith(".preset") }
            ?.map { it.name.removeSuffix(".preset") }
            ?.sorted() ?: emptyList()
    }

    fun getPresetCategories(): Map<String, List<String>> {
        val categories = mutableMapOf<String, MutableList<String>>()
        val files = presetDir.listFiles()?.filter { it.name.endsWith(".preset") } ?: return emptyMap()

        for (file in files) {
            val name = file.name.removeSuffix(".preset")
            // Basic categorization based on naming convention or internal tag
            val category = when {
                name.contains("Clean", ignoreCase = true) -> "Clean"
                name.contains("Crunch", ignoreCase = true) || name.contains("Drive", ignoreCase = true) -> "Crunch"
                name.contains("Lead", ignoreCase = true) || name.contains("Metal", ignoreCase = true) -> "High Gain"
                name.contains("Ambient", ignoreCase = true) || name.contains("Space", ignoreCase = true) -> "Ambient"
                else -> "Other"
            }
            categories.getOrPut(category) { mutableListOf() }.add(name)
        }
        return categories
    }

    fun deletePreset(name: String): Boolean {
        val file = File(presetDir, "$name.preset")
        return if (file.exists()) file.delete() else false
    }

    fun renamePreset(oldName: String, newName: String): Boolean {
        val oldFile = File(presetDir, "$oldName.preset")
        val newFile = File(presetDir, "$newName.preset")
        return if (oldFile.exists()) oldFile.renameTo(newFile) else false
    }

    // Mock for Cloud Sync - placeholder for future Firebase/Supabase integration
    fun syncToCloud(onComplete: (Boolean) -> Unit) {
        executor.execute {
            Log.i(TAG, "Syncing presets to cloud...")
            Thread.sleep(2000) // Simulate network
            onComplete(true)
        }
    }
}
