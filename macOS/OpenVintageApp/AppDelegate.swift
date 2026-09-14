import Cocoa
import SwiftUI

@NSApplicationMain
final class AppDelegate: NSObject, NSApplicationDelegate {
    private var window: NSWindow!
    private let hardware = NativeHardwareModel()

    func applicationDidFinishLaunching(_ notification: Notification) {
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
        window.center()
        window.setFrameAutosaveName("OpenVintage.MainWindow")
        window.isReleasedWhenClosed = false
        window.makeKeyAndOrderFront(nil)

        hardware.refresh()
    }

    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool {
        return true
    }
}
