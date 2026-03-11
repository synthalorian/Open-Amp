package com.openamp.ui

import android.content.Context
import android.graphics.*
import android.util.AttributeSet
import android.view.MotionEvent
import android.view.View

class NeonLiveButton @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0
) : View(context, attrs, defStyleAttr) {

    private var label = "SLOT"
    private var isActive = false
    private var isPressed = false
    private var pressStartTime = 0L
    
    private val paint = Paint(Paint.ANTI_ALIAS_FLAG)
    private val glowPaint = Paint(Paint.ANTI_ALIAS_FLAG)
    
    private val colorActive = Color.parseColor("#00FF66") // Neon Green
    private val colorInactive = Color.parseColor("#FF2A6D") // Neon Pink
    private val colorBg = Color.parseColor("#17132A")

    var onClick: (() -> Unit)? = null
    var onLongClick: (() -> Unit)? = null

    companion object {
        private const val LONG_PRESS_TIMEOUT = 500L
    }

    init {
        glowPaint.maskFilter = BlurMaskFilter(20f, BlurMaskFilter.Blur.NORMAL)
        setLayerType(LAYER_TYPE_SOFTWARE, null)
        isLongClickable = true
    }

    fun setActive(active: Boolean) {
        isActive = active
        invalidate()
    }

    fun setLabel(text: String) {
        label = text
        invalidate()
    }

    override fun onDraw(canvas: Canvas) {
        val w = width.toFloat()
        val h = height.toFloat()
        val mainColor = if (isActive) colorActive else colorInactive
        val padding = if (isPressed) 15f else 10f

        // Background
        paint.color = colorBg
        paint.style = Paint.Style.FILL
        val rect = RectF(padding, padding, w - padding, h - padding)
        canvas.drawRoundRect(rect, 25f, 25f, paint)
        
        // Frame Glow
        glowPaint.color = mainColor
        glowPaint.alpha = if (isActive) 180 else 80
        canvas.drawRoundRect(rect, 25f, 25f, glowPaint)

        // Frame
        paint.color = mainColor
        paint.style = Paint.Style.STROKE
        paint.strokeWidth = 4f
        canvas.drawRoundRect(rect, 25f, 25f, paint)

        // Text
        paint.color = Color.WHITE
        paint.style = Paint.Style.FILL
        paint.textSize = h * 0.35f
        paint.textAlign = Paint.Align.CENTER
        paint.typeface = Typeface.DEFAULT_BOLD
        canvas.drawText(label, w / 2f, h / 2f + (paint.textSize / 3f), paint)
        
        if (isActive) {
            glowPaint.alpha = 100
            canvas.drawText(label, w / 2f, h / 2f + (paint.textSize / 3f), glowPaint)
        }
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        when (event.action) {
            MotionEvent.ACTION_DOWN -> {
                isPressed = true
                pressStartTime = System.currentTimeMillis()
                invalidate()
                return true
            }
            MotionEvent.ACTION_UP -> {
                isPressed = false
                val pressDuration = System.currentTimeMillis() - pressStartTime
                if (pressDuration >= LONG_PRESS_TIMEOUT) {
                    onLongClick?.invoke()
                } else {
                    onClick?.invoke()
                }
                invalidate()
                return true
            }
            MotionEvent.ACTION_CANCEL -> {
                isPressed = false
                invalidate()
                return true
            }
        }
        return super.onTouchEvent(event)
    }
}
