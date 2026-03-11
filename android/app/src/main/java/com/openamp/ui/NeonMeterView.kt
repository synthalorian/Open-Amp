package com.openamp.ui

import android.content.Context
import android.graphics.*
import android.util.AttributeSet
import android.view.View
import kotlin.math.max

class NeonMeterView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0
) : View(context, attrs, defStyleAttr) {

    private var level = 0f // 0 to 1
    private var peak = 0f
    private var isStereo = false
    
    private val paint = Paint(Paint.ANTI_ALIAS_FLAG)
    private val glowPaint = Paint(Paint.ANTI_ALIAS_FLAG)
    
    private val colorNormal = Color.parseColor("#00FF66") // Neon Green
    private val colorWarn = Color.parseColor("#FFCC00") // Yellow
    private val colorClip = Color.parseColor("#FF2A6D") // Neon Pink
    private val bgColor = Color.parseColor("#0B0A16")

    init {
        glowPaint.maskFilter = BlurMaskFilter(8f, BlurMaskFilter.Blur.NORMAL)
        setLayerType(LAYER_TYPE_SOFTWARE, null)
    }

    fun setLevel(l: Float) {
        level = l.coerceIn(0f, 1f)
        if (level > peak) peak = level
        invalidate()
    }

    fun setPeak(p: Float) {
        peak = p.coerceIn(0f, 1f)
        invalidate()
    }

    fun resetPeak() {
        peak = 0f
        invalidate()
    }

    override fun onDraw(canvas: Canvas) {
        val w = width.toFloat()
        val h = height.toFloat()
        
        // Draw background
        paint.color = bgColor
        canvas.drawRect(0f, 0f, w, h, paint)
        
        // Draw grid lines
        paint.color = Color.parseColor("#222233")
        paint.strokeWidth = 1f
        for (i in 1 until 10) {
            val y = h * (i / 10f)
            canvas.drawLine(0f, y, w, y, paint)
        }
        
        // Calculate color based on level
        val currentMainColor = when {
            level > 0.95f -> colorClip
            level > 0.8f -> colorWarn
            else -> colorNormal
        }
        
        // Draw level bar
        val barHeight = h * level
        val top = h - barHeight
        
        // Bar Glow
        glowPaint.color = currentMainColor
        glowPaint.alpha = 100
        canvas.drawRect(4f, top, w - 4f, h, glowPaint)
        
        // Bar solid
        paint.color = currentMainColor
        paint.style = Paint.Style.FILL
        canvas.drawRect(8f, top, w - 8f, h, paint)
        
        // Peak indicator line
        if (peak > 0) {
            val peakY = h - (h * peak)
            paint.color = if (peak > 0.95f) colorClip else Color.WHITE
            paint.strokeWidth = 4f
            canvas.drawLine(4f, peakY, w - 4f, peakY, paint)
        }
    }
}
