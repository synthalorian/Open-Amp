package com.openamp.ui

import android.content.Context
import android.graphics.*
import android.util.AttributeSet
import android.view.MotionEvent
import android.view.View
import kotlin.math.atan2
import kotlin.math.cos
import kotlin.math.sin

class NeonKnobView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0
) : View(context, attrs, defStyleAttr) {

    private var value = 0.5f // 0.0 to 1.0
    private var label = "KNOB"
    
    private val paint = Paint(Paint.ANTI_ALIAS_FLAG)
    private val glowPaint = Paint(Paint.ANTI_ALIAS_FLAG)
    
    private var primaryColor = Color.parseColor("#FF2A6D") // Neon Pink
    private var secondaryColor = Color.parseColor("#05D9E8") // Neon Cyan
    private var bgColor = Color.parseColor("#17132A")
    
    var onValueChanged: ((Float) -> Unit)? = null

    init {
        // In a real app we'd load colors from attrs
        glowPaint.maskFilter = BlurMaskFilter(15f, BlurMaskFilter.Blur.NORMAL)
        setLayerType(LAYER_TYPE_SOFTWARE, null) // Required for BlurMaskFilter
    }

    fun setValue(v: Float) {
        value = v.coerceIn(0f, 1f)
        invalidate()
    }

    fun getValue() = value

    fun setLabel(l: String) {
        label = l
        invalidate()
    }

    override fun onDraw(canvas: Canvas) {
        val size = minOf(width, height).toFloat()
        val centerX = width / 2f
        val centerY = height / 2f
        val radius = size * 0.35f
        
        // Draw background circle (outer glow/ring)
        paint.color = Color.BLACK
        canvas.drawCircle(centerX, centerY, radius + 5f, paint)
        
        // Draw active arc (glow)
        glowPaint.color = primaryColor
        glowPaint.strokeWidth = 10f
        glowPaint.style = Paint.Style.STROKE
        val arcRect = RectF(centerX - radius, centerY - radius, centerX + radius, centerY + radius)
        canvas.drawArc(arcRect, 135f, value * 270f, false, glowPaint)
        
        // Draw active arc (sharp)
        paint.color = primaryColor
        paint.strokeWidth = 4f
        paint.style = Paint.Style.STROKE
        canvas.drawArc(arcRect, 135f, value * 270f, false, paint)

        // Draw the knob body
        paint.style = Paint.Style.FILL
        paint.color = bgColor
        canvas.drawCircle(centerX, centerY, radius * 0.85f, paint)
        
        // Draw pointer
        val angle = 135f + (value * 270f)
        val rad = Math.toRadians(angle.toDouble())
        val pointerX = centerX + (radius * 0.6f * cos(rad)).toFloat()
        val pointerY = centerY + (radius * 0.6f * sin(rad)).toFloat()
        
        paint.color = secondaryColor
        canvas.drawCircle(pointerX, pointerY, radius * 0.1f, paint)
        
        // Draw Label
        paint.color = Color.WHITE
        paint.textSize = size * 0.12f
        paint.textAlign = Paint.Align.CENTER
        canvas.drawText(label, centerX, height - 10f, paint)
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        when (event.action) {
            MotionEvent.ACTION_DOWN, MotionEvent.ACTION_MOVE -> {
                val dx = event.x - (width / 2f)
                val dy = event.y - (height / 2f)
                var angle = Math.toDegrees(atan2(dy.toDouble(), dx.toDouble())).toFloat()
                
                // Adjust angle to match 135-405 range
                angle += 90f
                if (angle < 0) angle += 360f
                
                // Map angle to 0.0-1.0
                // Our range is 135 to 405 (which is 45)
                // Let's simplify: 0% is at 135, 100% is at 45 (via 360)
                // Shift so 135 is 0
                var normalizedAngle = angle - 45f
                if (normalizedAngle < 0) normalizedAngle += 360f
                
                // Now 0 is at 135, 270 is at 45
                // We want 0 to 270
                val newValue = (normalizedAngle / 270f).coerceIn(0f, 1f)
                if (newValue != value) {
                    value = newValue
                    onValueChanged?.invoke(value)
                    invalidate()
                }
                return true
            }
        }
        return super.onTouchEvent(event)
    }
}
