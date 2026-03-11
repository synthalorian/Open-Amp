import SwiftUI

struct KnobView: View {
    let label: String
    @Binding var value: Float
    let range: ClosedRange<Float>
    let unit: String
    var percentage: Bool = false
    
    @State private var isDragging = false
    @GestureState private var dragOffset: CGFloat = 0
    
    private let knobSize: CGFloat = 70
    private let sensitivity: CGFloat = 200
    
    var body: some View {
        VStack(spacing: 6) {
            // Label
            Text(label)
                .font(.caption)
                .foregroundColor(.secondary)
            
            // Knob
            ZStack {
                // Outer ring
                Circle()
                    .stroke(Color.gray.opacity(0.3), lineWidth: 4)
                    .frame(width: knobSize, height: knobSize)
                
                // Value arc
                Circle()
                    .trim(from: 0, to: CGFloat((value - range.lowerBound) / (range.upperBound - range.lowerBound)))
                    .stroke(
                        LinearGradient(
                            colors: [.orange, .red],
                            startPoint: .topLeading,
                            endPoint: .bottomTrailing
                        ),
                        style: StrokeStyle(lineWidth: 4, lineCap: .round)
                    )
                    .frame(width: knobSize, height: knobSize)
                    .rotationEffect(.degrees(-90))
                
                // Knob face
                Circle()
                    .fill(
                        RadialGradient(
                            colors: [Color(white: 0.3), Color(white: 0.15)],
                            center: .center,
                            startRadius: 0,
                            endRadius: knobSize / 2
                        )
                    )
                    .frame(width: knobSize - 10, height: knobSize - 10)
                    .shadow(color: .black.opacity(0.5), radius: 4, x: 2, y: 2)
                
                // Indicator line
                Rectangle()
                    .fill(Color.orange)
                    .frame(width: 2, height: knobSize / 3 - 5)
                    .offset(y: -knobSize / 6 + 2)
                    .rotationEffect(.degrees(knobRotation))
                
                // Inner circle
                Circle()
                    .fill(Color.black.opacity(0.3))
                    .frame(width: 8, height: 8)
            }
            .gesture(
                DragGesture()
                    .updating($dragOffset) { value, state, _ in
                        state = value.translation.height
                    }
                    .onChanged { gesture in
                        isDragging = true
                        let delta = -gesture.translation.height / sensitivity
                        let rangeWidth = range.upperBound - range.lowerBound
                        let newValue = value + Float(delta) * rangeWidth
                        value = min(range.upperBound, max(range.lowerBound, newValue))
                    }
                    .onEnded { _ in
                        isDragging = false
                    }
            )
            .scaleEffect(isDragging ? 1.1 : 1.0)
            .animation(.easeInOut(duration: 0.1), value: isDragging)
            
            // Value display
            Text(displayValue)
                .font(.caption2)
                .foregroundColor(isDragging ? .orange : .secondary)
                .frame(width: 60)
        }
    }
    
    private var knobRotation: Double {
        let normalized = (value - range.lowerBound) / (range.upperBound - range.lowerBound)
        return Double(normalized) * 270 - 135
    }
    
    private var displayValue: String {
        if percentage {
            let percent = Int((value - range.lowerBound) / (range.upperBound - range.lowerBound) * 100)
            return "\(percent)%"
        }
        return "\(value, specifier: "%.1f")\(unit)"
    }
}

// Alternative vertical slider style knob
struct VerticalSliderKnob: View {
    let label: String
    @Binding var value: Float
    let range: ClosedRange<Float>
    
    var body: some View {
        VStack(spacing: 4) {
            Text(label)
                .font(.caption2)
                .foregroundColor(.secondary)
            
            RoundedRectangle(cornerRadius: 8)
                .fill(Color.gray.opacity(0.2))
                .frame(width: 40, height: 120)
                .overlay(
                    RoundedRectangle(cornerRadius: 8)
                        .fill(Color.orange)
                        .frame(width: 36, height: knobHeight)
                        .offset(y: knobOffset),
                    alignment: .bottom
                )
                .gesture(
                    DragGesture()
                        .onChanged { gesture in
                            let normalized = 1 - (gesture.location.y / 120)
                            value = Float(normalized) * (range.upperBound - range.lowerBound) + range.lowerBound
                            value = min(range.upperBound, max(range.lowerBound, value))
                        }
                )
            
            Text("\(value, specifier: "%.1f")")
                .font(.caption2)
                .foregroundColor(.secondary)
        }
    }
    
    private var knobHeight: CGFloat {
        let normalized = (value - range.lowerBound) / (range.upperBound - range.lowerBound)
        return CGFloat(normalized) * 110
    }
    
    private var knobOffset: CGFloat {
        return -(120 - knobHeight) / 2 + 5
    }
}

// Preview
#Preview {
    VStack(spacing: 40) {
        HStack(spacing: 30) {
            KnobView(label: "Gain", value: .constant(0.5), range: 0...1, unit: "", percentage: true)
            KnobView(label: "Tone", value: .constant(0.7), range: -12...12, unit: "dB")
            KnobView(label: "Mix", value: .constant(0.3), range: 0...1, unit: "", percentage: true)
        }
        
        HStack(spacing: 30) {
            VerticalSliderKnob(label: "Vol", value: .constant(0.6), range: 0...1)
            VerticalSliderKnob(label: "Drive", value: .constant(0.8), range: 0...1)
        }
    }
    .padding()
    .background(Color.black)
}
