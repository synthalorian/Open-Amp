package com.openamp.ui

import android.content.Context
import android.graphics.*
import android.util.AttributeSet
import android.view.View
import android.widget.TextView

class NeonPresetDisplay @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0
) : View(context, attrs, defStyleAttr) {

    private var presetName = "Default"
    private var isModified = false
    
    private val paint = Paint(Paint.ANTI_ALIAS_FLAG)
    private val glowPaint = Paint(Paint.ANTI_ALIAS_FLAG)
    
    private val colorPrimary = Color.parseColor("#FF2A6D") // Neon Pink
    private val colorSecondary = Color.parseColor("#05D9E8") // Neon Cyan
    private val colorBg = Color.parseColor("#0B0A16")

    init {
        glowPaint.maskFilter = BlurMaskFilter(10f, BlurMaskFilter.Blur.NORMAL)
        setLayerType(LAYER_TYPE_SOFTWARE, null)
    }

    fun setPreset(name: String, modified: Boolean) {
        presetName = name
        isModified = modified
        invalidate()
    }

    override fun onDraw(canvas: Canvas) {
        val w = width.toFloat()
        val h = height.toFloat()
        val centerX = w / 2f
        val centerY = h / 2f

        // Draw background card (neon style)
        paint.color = colorBg
        paint.style = Paint.Style.FILL
        val cardRect = RectF(10f, 10f, w - 10f, h - 10f)
        canvas.drawRoundRect(cardRect, 15f, 15f, paint)
        
        // Draw frame glow
        paint.color = colorPrimary
        paint.style = Paint.Style.STROKE
        paint.strokeWidth = 2f
        canvas.drawRoundRect(cardRect, 15f, 15f, paint)

        // Draw preset name (center)
        paint.color = colorSecondary
        paint.textSize = h * 0.4f
        paint.textAlign = Paint.Align.CENTER
        paint.typeface = Typeface.MONOSPACE
        val displayText = if (isModified) "$presetName *" else presetName
        canvas.drawText(displayText, centerX, centerY + (paint.textSize / 3f), paint)
        
        // Preset name glow
        glowPaint.color = colorSecondary
        glowPaint.alpha = 100
        canvas.drawText(displayText, centerX, centerY + (paint.textSize / 3f), glowPaint)

        // Draw small label at top left
        paint.color = colorPrimary
        paint.textSize = h * 0.15f
        paint.textAlign = Paint.Align.LEFT
        canvas.drawText("PRESET", 25f, 35f, paint)
    }
}
