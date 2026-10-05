import Foundation

// Machine-local settings, read from `client-macos/.env` — a file that is
// gitignored on purpose. The first value living here is the Windows
// server's LAN address: it's handed out by the router (DHCP) and has
// already changed once, and every change used to mean editing source and
// committing it. A local file also puts the mechanism in place before
// Phase 7 brings values that genuinely must never be committed (TURN
// credentials) — see FOUNDATION.md's "Security posture for connection
// details".
//
// `.env` is just a convention: plain `KEY=value` lines. Nothing in Swift
// or macOS reads it automatically, which is why this small parser exists
// instead of pulling in a dependency for a dozen lines of work.
enum LocalConfig {
    // Same `#filePath` trick Package.swift uses: it expands, at compile
    // time, to this source file's absolute path, so walking up three
    // folders (ShadowGlassClient/ → Sources/ → client-macos/) lands on
    // the package root no matter how the app was launched. The current
    // directory can't be trusted for that — it's `client-macos/` under
    // `swift run`, but something else entirely from Xcode or when the
    // .app bundle is double-clicked. The trade-off: the path is baked
    // into the binary, so it only resolves on the machine that built it.
    // Fine for a client that is always built and run from this checkout.
    static let fileURL = URL(fileURLWithPath: #filePath)
        .deletingLastPathComponent()
        .deletingLastPathComponent()
        .deletingLastPathComponent()
        .appendingPathComponent(".env")

    // Read once, on first use. A missing file isn't an error here — it
    // just yields no values, and the UI explains what to do about it.
    private static let values: [String: String] = {
        guard let contents = try? String(contentsOf: fileURL, encoding: .utf8) else {
            return [:]
        }
        return parse(contents)
    }()

    // `nil` when the file or the key is missing, or the value is empty —
    // callers decide how to surface that instead of this crashing.
    static var windowsHost: String? {
        guard let host = values["WINDOWS_HOST"], !host.isEmpty else { return nil }
        return host
    }

    static func parse(_ contents: String) -> [String: String] {
        var result: [String: String] = [:]
        for rawLine in contents.split(whereSeparator: \.isNewline) {
            let line = rawLine.trimmingCharacters(in: .whitespaces)
            if line.isEmpty || line.hasPrefix("#") { continue }
            // Split on the first `=` only, so a value may contain one.
            guard let separator = line.firstIndex(of: "=") else { continue }
            let key = line[..<separator].trimmingCharacters(in: .whitespaces)
            var value = line[line.index(after: separator)...].trimmingCharacters(in: .whitespaces)
            // Accept WINDOWS_HOST="1.2.3.4" as well as the bare form.
            if value.count >= 2, value.hasPrefix("\""), value.hasSuffix("\"") {
                value = String(value.dropFirst().dropLast())
            }
            if !key.isEmpty { result[key] = value }
        }
        return result
    }
}
