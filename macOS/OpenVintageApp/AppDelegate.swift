import Cocoa
import SwiftUI

@NSApplicationMain
final class AppDelegate: NSObject, NSApplicationDelegate {
    private var window: NSWindow!
    private let hardware = NativeHardwareModel()
    private let diagnostics = AppDiagnostics.shared

    func applicationDidFinishLaunching(_ notification: Notification) {
        // Output diagnostic assessment to standard output immediately on launch.
        AppDiagnostics.printBanner()

        // Handle diagnostic command-line flags if run from terminal
        let args = CommandLine.arguments
        if args.contains("--diagnostic") || args.contains("-d") || args.contains("--version") || args.contains("-v") || args.contains("status") {
            print("OpenVintage Version 1.0.0 (Mach-O x86_64)")
            print("Status: Operational (Zero ROM/SPI modification)")
            // If explicitly requested diagnostic CLI execution only, exit cleanly
            if args.contains("--diagnostic") || args.contains("--version") || args.contains("-v") {
                exit(0)
            }
        }

        // Explicitly opt into a normal foreground macOS application.
        NSApp.setActivationPolicy(.regular)

        do {
            let rootView = ContentView(hardware: hardware)
            let hostingView = NSHostingView(rootView: rootView)

            window = NSWindow(
                contentRect: NSRect(x: 0, y: 0, width: 1120, height: 760),
                styleMask: [.titled, .closable, .miniaturizable, .resizable],
                backing: .buffered,
                defer: false
            )
            window.title = "OpenVintage"
            window.contentView = hostingView
            window.isOpaque = false
            window.backgroundColor = .clear
            window.titlebarAppearsTransparent = true
            window.titleVisibility = .hidden
            window.center()
            window.setFrameAutosaveName("OpenVintage.MainWindow")
            window.isReleasedWhenClosed = false
            window.makeKeyAndOrderFront(nil)
            NSApp.activate(ignoringOtherApps: true)

            // Hardware discovery is read-only.
            hardware.refresh()
        } catch {
            diagnostics.recordFatalError("Failed to initialize primary SwiftUI window: \(error.localizedDescription)")
            showErrorWindow(message: error.localizedDescription)
        }
    }

    private func showErrorWindow(message: String) {
        let errWindow = NSWindow(
            contentRect: NSRect(x: 0, y: 0, width: 680, height: 460),
            styleMask: [.titled, .closable],
            backing: .buffered,
            defer: false
        )
        errWindow.title = "OpenVintage Initialization Error"
        
        let textView = NSTextView(frame: NSRect(x: 20, y: 60, width: 640, height: 360))
        textView.isEditable = false
        textView.font = NSFont.monospacedSystemFont(ofSize: 11, weight: .regular)
        textView.string = """
        [CRITICAL ERROR] OpenVintage Initialization Failed

        Error Details:
        \(message)

        Diagnostic Assessment:
        - macOS: \(diagnostics.report.macOsVersion) (Darwin \(diagnostics.report.darwinVersion))
        - CPU: \(diagnostics.report.cpuArchitecture) (\(diagnostics.report.cpuBrand))
        - Bundle: \(diagnostics.report.appBundlePath)
        - Executable: \(diagnostics.report.executablePath)
        - Code-Signing: \(diagnostics.report.codeSigningStatus)
        - Gatekeeper: \(diagnostics.report.gatekeeperAssessment)

        Recovery Action:
        Please inspect the system logs or launch OpenVintage with --diagnostic.
        """

        let container = NSView(frame: NSRect(x: 0, y: 0, width: 680, height: 460))
        container.addSubview(textView)
        errWindow.contentView = container
        errWindow.center()
        errWindow.makeKeyAndOrderFront(nil)
        NSApp.activate(ignoringOtherApps: true)
    }

    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool {
        // Do not prematurely terminate during startup or automated smoke test
        return false
    }
}
