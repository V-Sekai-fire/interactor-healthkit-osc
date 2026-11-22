// swift-tools-version: 6.0

import PackageDescription

let package = Package(
    name: "HeartRateOSC",
    platforms: [
        .iOS(.v17),
        .macOS(.v14),
    ],
    products: [
        // An xtool project holds exactly one library product, the main app.
        .library(
            name: "HeartRateOSC",
            targets: ["HeartRateOSC"]
        ),
    ],
    targets: [
        .target(name: "OSC", path: "Sources/OSC"),
        .target(name: "HeartRateOSC", dependencies: ["OSC"], path: "Sources/HeartRateOSC"),
    ]
)
