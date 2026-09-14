import Foundation
import IOKit
import IOKit.graphics
import Darwin

final class NativeHardwareModel: ObservableObject {
    @Published var model = "Detecting Mac…"
    @Published var cpu = "Detecting CPU…"
    @Published var memory = "Detecting memory…"
    @Published var darwin = "Detecting Darwin…"
    @Published var kernel = "Detecting kernel…"
    @Published var gpus: [String] = []

    func refresh() {
        model = sysctlString("hw.model") ?? "Unknown Mac"
        cpu = sysctlString("machdep.cpu.brand_string") ?? "Apple / Unknown CPU"
        darwin = sysctlString("kern.osproductversion") ?? sysctlString("kern.osrelease") ?? "Unknown"
        kernel = sysctlString("kern.osrelease") ?? "Unknown"

        if let bytes = sysctlUInt64("hw.memsize") {
            memory = ByteCountFormatter.string(fromByteCount: Int64(bytes), countStyle: .binary)
        } else {
            memory = "Unknown"
        }

        gpus = detectGPUs()
        if gpus.isEmpty {
            gpus = ["No PCI graphics devices reported by IOKit"]
        }
    }

    private func sysctlString(_ name: String) -> String? {
        var size = 0
        guard sysctlbyname(name, nil, &size, nil, 0) == 0, size > 0 else { return nil }
        var value = [CChar](repeating: 0, count: size)
        guard sysctlbyname(name, &value, &size, nil, 0) == 0 else { return nil }
        return String(cString: value)
    }

    private func sysctlUInt64(_ name: String) -> UInt64? {
        var value: UInt64 = 0
        var size = MemoryLayout<UInt64>.size
        guard sysctlbyname(name, &value, &size, nil, 0) == 0 else { return nil }
        return value
    }

    private func detectGPUs() -> [String] {
        guard let matching = IOServiceMatching("IOPCIDevice") else { return [] }
        var iterator: io_iterator_t = 0
        guard IOServiceGetMatchingServices(kIOMainPortDefault, matching, &iterator) == KERN_SUCCESS else { return [] }
        defer { IOObjectRelease(iterator) }

        var result: [String] = []
        while true {
            let service = IOIteratorNext(iterator)
            if service == 0 { break }
            defer { IOObjectRelease(service) }

            guard let className = IORegistryEntryCreateCFProperty(service, "class-code" as CFString, kCFAllocatorDefault, 0)?.takeRetainedValue() else {
                continue
            }

            var isDisplay = false
            if let number = className as? NSNumber {
                let classCode = number.uint32Value >> 8
                isDisplay = (classCode == 0x03 || classCode == 0x00)
            } else if let data = className as? Data, data.count >= 3 {
                isDisplay = data[data.startIndex.advanced(by: 2)] == 0x03
            }
            if !isDisplay { continue }

            let vendor = registryNumber(service, key: "vendor-id")
            let device = registryNumber(service, key: "device-id")
            let name = (IORegistryEntryCreateCFProperty(service, "model" as CFString, kCFAllocatorDefault, 0)?.takeRetainedValue() as? String)
                ?? (IORegistryEntryCreateCFProperty(service, "compatible" as CFString, kCFAllocatorDefault, 0)?.takeRetainedValue() as? [String])?.first
                ?? "PCI graphics device"

            if let vendor, let device {
                result.append("\(name)  [PCI \(String(format: "%04X", vendor)):\(String(format: "%04X", device))]")
            } else {
                result.append(name)
            }
        }

        return Array(NSOrderedSet(array: result)) as? [String] ?? result
    }

    private func registryNumber(_ service: io_service_t, key: String) -> UInt32? {
        guard let value = IORegistryEntryCreateCFProperty(service, key as CFString, kCFAllocatorDefault, 0)?.takeRetainedValue() else { return nil }
        if let number = value as? NSNumber { return number.uint32Value }
        if let data = value as? Data, data.count >= 4 {
            return data.withUnsafeBytes { raw in
                raw.load(as: UInt32.self).bigEndian
            }
        }
        return nil
    }
}
