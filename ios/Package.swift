// swift-tools-version: 5.9
import PackageDescription

let package = Package(
    name: "Open Amp",
    platforms: [
        .iOS(.v16),
        .macOS(.v13)
    ],
    products: [
        .library(
            name: "DSPCore",
            targets: ["DSPCore"]),
    ],
    targets: [
        .target(
            name: "DSPCore",
            dependencies: [],
            path: "Sources/DSPCore",
            publicHeadersPath: "include",
            cxxSettings: [
                .headerSearchPath("."),
                .headerSearchPath("../../dsp-core/include"),
                .define("OPENAMP_BUILDING", to: "1")
            ]),
        .testTarget(
            name: "DSPCoreTests",
            dependencies: ["DSPCore"],
            path: "Tests/DSPCoreTests"),
    ],
    cxxLanguageStandard: .cxx17
)
