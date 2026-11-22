// SPDX-License-Identifier: MIT
// The thin HealthKit shim: it reads heart-rate samples and hands them to the C encoder in OSC.
import Foundation
import HealthKit
import OSC

@MainActor
final class Monitor: ObservableObject {
    @Published var bpm: Int?
    @Published var status = "Stopped"

    private let store = HKHealthStore()
    private var query: HKAnchoredObjectQuery?
    private var timer: Timer?
    private var lastBpm = 0.0
    private var lastSample: Date?
    private var host = ""
    private var controllerPort: UInt16?
    private let vrchatPort: UInt16 = 9000
    // No new sample for this long counts as disconnected. A watch writes samples only now and then.
    private let staleSeconds = 120.0

    func start(host: String, controllerPort: UInt16?) async {
        guard HKHealthStore.isHealthDataAvailable() else {
            status = "Health data is not available on this device"
            return
        }
        self.host = host
        self.controllerPort = controllerPort
        let type = HKQuantityType(.heartRate)
        do {
            try await store.requestAuthorization(toShare: [], read: [type])
        } catch {
            status = "Health access failed: \(error.localizedDescription)"
            return
        }
        let handler: @Sendable (HKAnchoredObjectQuery, [HKSample]?, [HKDeletedObject]?, HKQueryAnchor?, Error?) -> Void = { [weak self] _, samples, _, _, _ in
            let unit = HKUnit.count().unitDivided(by: .minute())
            let points = (samples as? [HKQuantitySample])?.map { ($0.quantity.doubleValue(for: unit), $0.endDate) } ?? []
            guard let latest = points.max(by: { $0.1 < $1.1 }) else { return }
            Task { @MainActor in self?.take(latest.0, latest.1) }
        }
        let predicate = HKQuery.predicateForSamples(withStart: Date().addingTimeInterval(-300), end: nil)
        let q = HKAnchoredObjectQuery(type: type, predicate: predicate, anchor: nil, limit: HKObjectQueryNoLimit, resultsHandler: handler)
        q.updateHandler = handler
        store.execute(q)
        query = q
        timer = Timer.scheduledTimer(withTimeInterval: 1, repeats: true) { [weak self] _ in
            Task { @MainActor in self?.tick() }
        }
        status = "Waiting for a heart-rate sample"
    }

    func stop() {
        if let query { store.stop(query) }
        query = nil
        timer?.invalidate()
        timer = nil
        send(vrchat: osc_heart_rate_connected, flag: 0)
        status = "Stopped"
    }

    private func take(_ value: Double, _ date: Date) {
        guard lastSample == nil || date >= lastSample! else { return }
        lastBpm = value
        lastSample = date
        bpm = Int(value.rounded())
    }

    private func tick() {
        guard let sample = lastSample else { return }
        let age = Date().timeIntervalSince(sample)
        if age > staleSeconds {
            send(vrchat: osc_heart_rate_connected, flag: 0)
            status = "No sample for \(Int(age)) s"
            return
        }
        let rounded = Int32(min(255, max(0, lastBpm.rounded())))
        let ageMs = Int32(min(Double(Int32.max), age * 1000))
        var buf = [UInt8](repeating: 0, count: 128)
        send(vrchat: osc_heart_rate_connected, flag: 1)
        var n = osc_heart_rate(&buf, buf.count, rounded)
        emit(buf, n, port: vrchatPort)
        n = osc_heart_rate_percent(&buf, buf.count, Float(rounded) / 255)
        emit(buf, n, port: vrchatPort)
        if let controllerPort {
            n = osc_presence(&buf, buf.count, UInt64(sample.timeIntervalSince1970 * 1000), Float(lastBpm), osc_confidence(ageMs), ageMs)
            emit(buf, n, port: controllerPort)
        }
        status = "Sending, sample \(Int(age)) s old"
    }

    private func send(vrchat encode: (UnsafeMutablePointer<UInt8>?, Int, Int32) -> Int, flag: Int32) {
        var buf = [UInt8](repeating: 0, count: 128)
        let n = encode(&buf, buf.count, flag)
        emit(buf, n, port: vrchatPort)
    }

    private func emit(_ buf: [UInt8], _ n: Int, port: UInt16) {
        guard n > 0, !host.isEmpty else { return }
        _ = osc_udp_send(host, port, buf, n)
    }
}
