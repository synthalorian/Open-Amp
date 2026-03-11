import SwiftUI

struct TunerView: View {
    @Environment(\.dismiss) var dismiss
    @State private var frequency: Float = 0.0
    @State private var noteName: String = "-"
    @State private var cents: Float = 0.0
    @State private var confidence: Float = 0.0
    @State private var isMuted: Bool = true
    
    // Animation
    @State private var needleAngle: Double = 0
    @State private var isAnimating = false
    
    let timer = Timer.publish(every: 0.1, on: .main, in: .common).autoconnect()
    
    var body: some View {
        NavigationView {
            ZStack {
                // Background
                Color.black.ignoresSafeArea()
                
                VStack(spacing: 30) {
                    // Note Display
                    Text(noteName)
                        .font(.system(size: 72, weight: .bold, design: .rounded))
                        .foregroundColor(noteColor)
                        .shadow(color: noteColor.opacity(0.5), radius: 20)
                    
                    // Frequency
                    Text(frequency > 0 ? "\(frequency, specifier: "%.1f") Hz" : "-- Hz")
                        .font(.title2)
                        .foregroundColor(.secondary)
                    
                    // Tuning Meter
                    TuningMeterView(cents: cents)
                        .frame(height: 120)
                        .padding(.horizontal, 40)
                    
                    // Cents Display
                    HStack(spacing: 40) {
                        VStack {
                            Text("Flat")
                                .font(.caption)
                                .foregroundColor(cents < -5 ? .blue : .secondary)
                            Text("♭")
                                .font(.title)
                        }
                        
                        VStack {
                            Text("In Tune")
                                .font(.caption)
                                .foregroundColor(abs(cents) < 5 ? .green : .secondary)
                            Text("\(cents, specifier: "%+.0f")")
                                .font(.system(size: 36, weight: .bold, design: .monospaced))
                                .foregroundColor(abs(cents) < 5 ? .green : .orange)
                            Text("cents")
                                .font(.caption)
                                .foregroundColor(.secondary)
                        }
                        
                        VStack {
                            Text("Sharp")
                                .font(.caption)
                                .foregroundColor(cents > 5 ? .red : .secondary)
                            Text("♯")
                                .font(.title)
                        }
                    }
                    
                    // Reference Pitch
                    HStack {
                        Text("Reference Pitch")
                            .foregroundColor(.secondary)
                        
                        Text("A4 = 440 Hz")
                            .foregroundColor(.white)
                    }
                    .padding(.top, 20)
                    
                    // Mute Toggle
                    Toggle("Mute Output", isOn: $isMuted)
                        .foregroundColor(.secondary)
                        .padding(.horizontal, 60)
                    
                    // Target Notes
                    VStack(spacing: 8) {
                        Text("Standard Tuning")
                            .font(.caption)
                            .foregroundColor(.secondary)
                        
                        HStack(spacing: 16) {
                            ForEach(["E2", "A2", "D3", "G3", "B3", "E4"], id: \.self) { note in
                                Text(note)
                                    .font(.caption)
                                    .foregroundColor(noteName == note ? .orange : .secondary)
                                    .frame(width: 36, height: 36)
                                    .background(noteName == note ? Color.orange.opacity(0.2) : Color.clear)
                                    .cornerRadius(8)
                            }
                        }
                    }
                    .padding(.top, 20)
                    
                    Spacer()
                }
                .padding(.top, 40)
            }
            .navigationTitle("Tuner")
            .toolbar {
                ToolbarItem(placement: .confirmationAction) {
                    Button("Done") {
                        dismiss()
                    }
                }
            }
        }
        .onReceive(timer) { _ in
            updateTuner()
        }
    }
    
    private var noteColor: Color {
        if abs(cents) < 5 {
            return .green
        } else if cents < -10 || cents > 10 {
            return .red
        } else {
            return .orange
        }
    }
    
    private func updateTuner() {
        // Simulate tuner values (in real app, these come from AudioEngine)
        // This is just for UI demonstration
        
        // In production, you'd get these from the audio engine's pitch detection
        // For now, just animate the UI
    }
}

struct TuningMeterView: View {
    let cents: Float
    
    var body: some View {
        GeometryReader { geometry in
            ZStack {
                // Background arc
                ArcView(startAngle: -45, endAngle: 45, color: Color.gray.opacity(0.3))
                
                // Center marker
                Rectangle()
                    .fill(Color.green)
                    .frame(width: 2, height: 20)
                    .offset(y: -geometry.size.height / 2 + 30)
                
                // Tick marks
                ForEach([-50, -25, 0, 25, 50], id: \.self) { tick in
                    Rectangle()
                        .fill(Color.secondary)
                        .frame(width: tick == 0 ? 2 : 1, height: tick == 0 ? 15 : 10)
                        .rotationEffect(.degrees(Double(tick) * 0.9))
                        .offset(y: -geometry.size.height / 2 + 25)
                }
                
                // Needle
                NeedleView(cents: cents)
                    .frame(width: geometry.size.width, height: geometry.size.height)
            }
        }
    }
}

struct ArcView: View {
    let startAngle: Double
    let endAngle: Double
    let color: Color
    
    var body: some View {
        GeometryReader { geometry in
            Path { path in
                let radius = min(geometry.size.width, geometry.size.height) / 2 - 10
                path.addArc(
                    center: CGPoint(x: geometry.size.width / 2, y: geometry.size.height),
                    radius: radius,
                    startAngle: Angle(degrees: startAngle),
                    endAngle: Angle(degrees: endAngle),
                    clockwise: false
                )
            }
            .stroke(color, lineWidth: 8)
        }
    }
}

struct NeedleView: View {
    let cents: Float
    
    var body: some View {
        GeometryReader { geometry in
            let clampedCents = max(-50, min(50, cents))
            let angle = Double(clampedCents) * 0.9  // Scale to arc
            
            Rectangle()
                .fill(LinearGradient(
                    colors: [.red, .orange, .green, .orange, .red],
                    startPoint: .leading,
                    endPoint: .trailing
                ))
                .frame(width: 4, height: geometry.size.height - 20)
                .cornerRadius(2)
                .rotationEffect(.degrees(angle), anchor: .bottom)
                .position(x: geometry.size.width / 2, y: geometry.size.height - 10)
        }
    }
}

#Preview {
    TunerView()
}
