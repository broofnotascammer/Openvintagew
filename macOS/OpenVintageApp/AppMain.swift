import SwiftUI

struct ContentView: View {
    @ObservedObject var hardware: NativeHardwareModel
    @State private var selection = "Overview"

    private let sections = ["Overview", "Hardware", "Simulator", "Compatibility", "Performance", "Boot", "Integrations", "Diagnostics", "Settings"]

    var body: some View {
        ZStack {
            VisualEffectView(material: .underWindowBackground, blendingMode: .behindWindow)

            HStack(spacing: 0) {
                sidebar
                Divider()
                detail
            }
        }
        .onAppear { hardware.refresh() }
    }

    private var sidebar: some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack(spacing: 10) {
                Text("OV")
                    .font(.system(size: 15, weight: .bold))
                    .frame(width: 34, height: 34)
                    .background(Color.accentColor.opacity(0.14))
                    .clipShape(RoundedRectangle(cornerRadius: 9))
                VStack(alignment: .leading, spacing: 1) {
                    Text("OpenVintage")
                        .font(.headline)
                    Text("Native macOS")
                        .font(.caption)
                        .foregroundColor(.secondary)
                }
            }
            .padding(.horizontal, 14)
            .padding(.top, 18)
            .padding(.bottom, 12)

            ForEach(sections, id: \.self) { item in
                Button(action: { selection = item }) {
                    HStack(spacing: 9) {
                        Text(icon(for: item))
                            .frame(width: 18)
                        Text(item)
                        Spacer()
                    }
                    .frame(maxWidth: .infinity, alignment: .leading)
                }
                .buttonStyle(SidebarButtonStyle(selected: selection == item))
            }

            Spacer()

            HStack(spacing: 7) {
                Circle()
                    .fill(Color.green)
                    .frame(width: 7, height: 7)
                Text("Darwin hardware bridge ready")
                    .font(.caption)
                    .foregroundColor(.secondary)
            }
            .padding(14)
        }
        .frame(width: 225)
        .background(Color.primary.opacity(0.035))
    }

    private var detail: some View {
        VStack(alignment: .leading, spacing: 0) {
            HStack {
                VStack(alignment: .leading, spacing: 3) {
                    Text(selection)
                        .font(.system(size: 26, weight: .semibold))
                    Text("Hardware-aware boot management and configuration")
                        .font(.subheadline)
                        .foregroundColor(.secondary)
                }
                Spacer()
                Button(action: hardware.refresh) {
                    Text("↻  Refresh")
                        .font(.system(size: 12, weight: .medium))
                }
            }
            .padding(.horizontal, 28)
            .padding(.top, 25)
            .padding(.bottom, 20)

            ScrollView {
                if selection == "Overview" || selection == "Hardware" {
                    overview
                } else {
                    placeholder
                }
            }
        }
    }

    private var overview: some View {
        VStack(alignment: .leading, spacing: 16) {
            GlassCard {
                HStack(spacing: 18) {
                    Text("⌘")
                        .font(.system(size: 30, weight: .light))
                        .frame(width: 58, height: 58)
                        .background(Color.primary.opacity(0.07))
                        .clipShape(RoundedRectangle(cornerRadius: 15))
                    VStack(alignment: .leading, spacing: 5) {
                        Text(hardware.model)
                            .font(.headline)
                        Text("Source: NATIVE • Darwin")
                            .font(.system(size: 11, design: .monospaced))
                            .foregroundColor(.secondary)
                        Text("Read-only hardware discovery. Refresh does not change boot configuration.")
                            .font(.caption)
                            .foregroundColor(.secondary)
                    }
                    Spacer()
                    StatusPill(text: "NATIVE")
                }
            }

            HStack(spacing: 16) {
                InfoCard(title: "CPU", value: hardware.cpu, icon: "CPU")
                InfoCard(title: "Memory", value: hardware.memory, icon: "RAM")
            }
            HStack(spacing: 16) {
                InfoCard(title: "Darwin", value: hardware.darwin, icon: "OS")
                InfoCard(title: "Kernel", value: hardware.kernel, icon: "K")
            }

            GlassCard {
                VStack(alignment: .leading, spacing: 12) {
                    HStack {
                        Text("Detected graphics")
                            .font(.headline)
                        Spacer()
                        Text("Physical inventory")
                            .font(.caption)
                            .foregroundColor(.secondary)
                    }
                    ForEach(hardware.gpus, id: \.self) { gpu in
                        HStack {
                            Text("GPU")
                                .font(.system(size: 10, weight: .bold, design: .monospaced))
                                .foregroundColor(.secondary)
                            Text(gpu)
                                .font(.body)
                            Spacer()
                        }
                        .padding(.vertical, 3)
                    }
                }
            }

            GlassCard {
                VStack(alignment: .leading, spacing: 10) {
                    Text("⌘  Architecture")
                        .font(.headline)
                    Text("The native macOS application owns the Darwin-facing layer. The browser UI remains a preview/development surface; privileged boot and hardware operations belong behind the native Core/HAL boundary.")
                        .font(.subheadline)
                        .foregroundColor(.secondary)
                }
            }
        }
        .padding(.horizontal, 28)
        .padding(.bottom, 28)
    }

    private var placeholder: some View {
        VStack(alignment: .leading, spacing: 14) {
            GlassCard {
                Text("\(selection) is part of the native application shell.")
                    .font(.headline)
                Text("This is the native SwiftUI/AppKit application, not the Vite website. Core features are connected through the application boundary so the UI never performs privileged boot operations directly.")
                    .font(.subheadline)
                    .foregroundColor(.secondary)
            }
        }
        .padding(28)
    }

    private func icon(for item: String) -> String {
        switch item {
        case "Overview": return "⌂"
        case "Hardware": return "CPU"
        case "Simulator": return "SIM"
        case "Compatibility": return "✓"
        case "Performance": return "↗"
        case "Boot": return "↑"
        case "Integrations": return "+"
        case "Diagnostics": return "!"
        default: return "⚙"
        }
    }
}

struct SidebarButtonStyle: ButtonStyle {
    let selected: Bool

    func makeBody(configuration: Configuration) -> some View {
        configuration.label
            .font(.system(size: 13, weight: selected ? .semibold : .regular))
            .foregroundColor(selected ? .primary : .secondary)
            .padding(.horizontal, 12)
            .padding(.vertical, 8)
            .background(selected ? Color.primary.opacity(0.09) : Color.clear)
            .clipShape(RoundedRectangle(cornerRadius: 8))
            .opacity(configuration.isPressed ? 0.72 : 1)
    }
}

struct GlassCard<Content: View>: View {
    @ViewBuilder let content: Content

    var body: some View {
        content
            .padding(18)
            .frame(maxWidth: .infinity, alignment: .leading)
            .background(VisualEffectView(material: .contentBackground, blendingMode: .withinWindow))
            .overlay(RoundedRectangle(cornerRadius: 16).stroke(Color.primary.opacity(0.09), lineWidth: 0.7))
            .clipShape(RoundedRectangle(cornerRadius: 16))
    }
}

struct InfoCard: View {
    let title: String
    let value: String
    let icon: String

    var body: some View {
        GlassCard {
            HStack(spacing: 12) {
                Text(icon)
                    .font(.system(size: 11, weight: .bold, design: .monospaced))
                    .foregroundColor(.secondary)
                    .frame(width: 30, height: 30)
                    .background(Color.primary.opacity(0.06))
                    .clipShape(RoundedRectangle(cornerRadius: 8))
                VStack(alignment: .leading, spacing: 3) {
                    Text(title)
                        .font(.caption)
                        .foregroundColor(.secondary)
                    Text(value)
                        .font(.system(size: 13, weight: .medium))
                        .lineLimit(2)
                }
            }
        }
    }
}

struct StatusPill: View {
    let text: String

    var body: some View {
        Text("✓  \(text)")
            .font(.caption.weight(.semibold))
            .padding(.horizontal, 10)
            .padding(.vertical, 6)
            .background(Color.green.opacity(0.11))
            .clipShape(Capsule())
    }
}
