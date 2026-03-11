import SwiftUI

struct MainView: View {
    @EnvironmentObject var audioEngine: AudioEngine
    @State private var selectedTab = 0
    @State private var showingPresetSheet = false
    @State private var showingTuner = false
    
    var body: some View {
        TabView(selection: $selectedTab) {
            AmpView()
                .tabItem {
                    Label("Amp", systemImage: "speaker.wave.3")
                }
                .tag(0)
            
            EffectsView()
                .tabItem {
                    Label("Effects", systemImage: "slider.horizontal.3")
                }
                .tag(1)
            
            PresetsView()
                .tabItem {
                    Label("Presets", systemImage: "music.note.list")
                }
                .tag(2)
            
            SettingsView()
                .tabItem {
                    Label("Settings", systemImage: "gear")
                }
                .tag(3)
        }
        .accentColor(.orange)
        .toolbar {
            ToolbarItem(placement: .navigationBarLeading) {
                Button {
                    showingTuner = true
                } label: {
                    Image(systemName: "tuningfork")
                }
            }
            
            ToolbarItem(placement: .navigationBarTrailing) {
                HStack(spacing: 8) {
                    // Level meters
                    LevelMeter(level: audioEngine.inputLevel, color: .green)
                        .frame(width: 8, height: 24)
                    
                    LevelMeter(level: audioEngine.outputLevel, color: .blue)
                        .frame(width: 8, height: 24)
                    
                    // Power button
                    Button {
                        toggleEngine()
                    } label: {
                        Image(systemName: audioEngine.isRunning ? "power" : "poweroff")
                            .foregroundColor(audioEngine.isRunning ? .green : .red)
                    }
                }
            }
        }
        .sheet(isPresented: $showingTuner) {
            TunerView()
        }
    }
    
    private func toggleEngine() {
        if audioEngine.isRunning {
            audioEngine.stop()
        } else {
            do {
                try audioEngine.start()
            } catch {
                print("Failed to start audio engine: \(error)")
            }
        }
    }
}

struct LevelMeter: View {
    let level: Float
    let color: Color
    
    var body: some View {
        GeometryReader { geometry in
            ZStack(alignment: .bottom) {
                Rectangle()
                    .fill(Color.gray.opacity(0.3))
                
                Rectangle()
                    .fill(color)
                    .frame(height: geometry.size.height * CGFloat(min(level, 1.0)))
            }
        }
        .cornerRadius(2)
    }
}

#Preview {
    MainView()
        .environmentObject(AudioEngine())
}
