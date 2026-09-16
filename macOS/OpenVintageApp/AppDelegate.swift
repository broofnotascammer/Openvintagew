import Cocoa
import SwiftUI

@NSApplicationMain
final class AppDelegate: NSObject, NSApplicationDelegate {
    private var window: NSWindow!
    private let hardware = NativeHardwareModel()

    func applicationDidFinishLaunching(_ notification: Notification) {
        // Explicitly opt into a normal foreground macOS application. This
        // avoids relying on implicit activation behaviour on older macOS.
        NSApp.setActivationPolicy(.regular)

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

        // Hardware discovery is read-only. Run it after the window is live so
        // a slow/odd IOKit registry cannot prevent the application from opening.
        hardware.refresh()
    }

    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool {
        return true
    }
}
