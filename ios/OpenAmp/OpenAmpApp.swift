import SwiftUI

@main
struct OpenAmpApp: App {
    @StateObject private var audioEngine = AudioEngine()
    
    var body: some Scene {
        WindowGroup {
            MainView()
                .environmentObject(audioEngine)
        }
    }
}
