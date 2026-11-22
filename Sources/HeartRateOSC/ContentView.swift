// SPDX-License-Identifier: MIT
import SwiftUI

struct ContentView: View {
    @StateObject private var monitor = Monitor()
    @AppStorage("host") private var host = ""
    @AppStorage("controllerPort") private var controllerPort = ""
    @State private var running = false

    var body: some View {
        Form {
            Section("PC") {
                TextField("Address of the PC", text: $host)
                    .textInputAutocapitalization(.never)
                    .autocorrectionDisabled()
                TextField("Controller port (optional)", text: $controllerPort)
                    .keyboardType(.numberPad)
            }
            Section("Heart rate") {
                Text(monitor.bpm.map { "\($0) bpm" } ?? "No sample yet")
                    .font(.largeTitle)
                Text(monitor.status)
                    .foregroundStyle(.secondary)
            }
            Button(running ? "Stop" : "Start") {
                if running {
                    monitor.stop()
                    running = false
                    UIApplication.shared.isIdleTimerDisabled = false
                } else {
                    running = true
                    UIApplication.shared.isIdleTimerDisabled = true
                    Task { await monitor.start(host: host, controllerPort: UInt16(controllerPort)) }
                }
            }
            .disabled(host.isEmpty)
        }
    }
}
