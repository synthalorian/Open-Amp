package com.openamp.ui

import android.content.Context
import android.graphics.*
import android.util.AttributeSet
import android.view.View
import kotlin.math.cos
import kotlin.math.sin

class NeonTunerDisplay @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0
) : View(context, attrs, defStyleAttr) {

    private var noteName = "--"
    private var centOffset = 0f // -50 to 50
    private var isActive = false
    
    private val paint = Paint(Paint.ANTI_ALIAS_FLAG)
    private val glowPaint = Paint(Paint.ANTI_ALIAS_FLAG)
    
    private val colorInTune = Color.parseColor("#00FF66") // Neon Green
    private val colorOutTune = Color.parseColor("#05D9E8") // Neon Cyan
    private val colorBg = Color.parseColor("#17132A")

    init {
        glowPaint.maskFilter = BlurMaskFilter(15f, BlurMaskFilter.Blur.NORMAL)
        setLayerType(LAYER_TYPE_SOFTWARE, null)
    }

    fun setNote(note: String, offset: Float, active: Boolean) {
        noteName = note
        centOffset = offset.coerceIn(-50f, 50f)
        isActive = active
        invalidate()
    }

    override fun onDraw(canvas: Canvas) {
        val w = width.toFloat()
        val h = height.toFloat()
        val centerX = w / 2f
        val centerY = h / 2f
        val radius = minOf(w, h) * 0.45f

        // Draw background card (neon style)
        paint.color = colorBg
        paint.style = Paint.Style.FILL
        val cardRect = RectF(10f, 10f, w - 10f, h - 10f)
        canvas.drawRoundRect(cardRect, 20f, 20f, paint)
        
        // Draw frame glow
        paint.color = colorOutTune
        paint.style = Paint.Style.STROKE
        paint.strokeWidth = 2f
        canvas.drawRoundRect(cardRect, 20f, 20f, paint)

        if (!isActive) {
            paint.color = Color.GRAY
            paint.textSize = h * 0.3f
            paint.textAlign = Paint.Align.CENTER
            canvas.drawText("--", centerX, centerY + (paint.textSize / 3f), paint)
            return
        }

        // Determine if in tune (threshold +/- 2 cents)
        val inTune = Math.abs(centOffset) < 2f
        val mainColor = if (inTune) colorInTune else colorOutTune

        // Draw note name (large, center)
        paint.color = mainColor
        paint.textSize = h * 0.5f
        paint.textAlign = Paint.Align.CENTER
        paint.typeface = Typeface.DEFAULT_BOLD
        canvas.drawText(noteName, centerX, centerY + (paint.textSize / 3f), paint)
        
        // Note name glow
        glowPaint.color = mainColor
        glowPaint.alpha = 120
        canvas.drawText(noteName, centerX, centerY + (paint.textSize / 3f), glowPaint)

        // Draw cent scale (arc at top)
        val scaleRect = RectF(centerX - radius, 20f, centerX + radius, h * 0.4f)
        paint.color = Color.parseColor("#333344")
        paint.strokeWidth = 4f
        paint.style = Paint.Style.STROKE
        canvas.drawArc(scaleRect, 210f, 120f, false, paint)

        // Draw indicator needle
        // 0 cents is at 270 degrees
        // -50 cents is at 210 degrees
        // +50 cents is at 330 degrees
        val needleAngle = 270f + (centOffset * 1.2f)
        val needleRad = Math.toRadians(needleAngle.toDouble())
        val startR = radius * 0.7f
        val endR = radius * 0.9f
        
        val startX = centerX + (startR * cos(needleRad)).toFloat()
        val startY = centerY + (startR * sin(needleRad)).toFloat()
        val endX = centerX + (endR * cos(needleRad)).toFloat()
        val endY = centerY + (endR * sin(needleRad)).toFloat()

        paint.color = mainColor
        paint.strokeWidth = 6f
        canvas.drawLine(startX, startY, endX, endY, paint)
        
        // Needle glow
        glowPaint.color = mainColor
        glowPaint.strokeWidth = 10f
        canvas.drawLine(startX, startY, endX, endY, glowPaint)

        // Draw cent value below note
        paint.color = Color.WHITE
        paint.textSize = h * 0.12f
        val centText = if (centOffset >= 0) "+${centOffset.toInt()} cents" else "${centOffset.toInt()} cents"
        canvas.drawText(centText, centerX, h - 30f, paint)
    }
}
