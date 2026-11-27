import Foundation
import SwiftUI

// feature/parity: iOS-side key mapping storage. The serialized form is the
// same line format consumed by libs/keymap on desktop and Android native.
struct KeyMapRule: Codable, Identifiable, Equatable {
    var id: String = UUID().uuidString
    var enabled: Bool = true
    var sourceType: String = "gamepad_button"
    var sourceCode: UInt32 = 0
    var threshold: Int = 0
    var rect: [Double] = [0, 0, 0, 0]
    var actionType: String = "key"
    var keys: [UInt32] = []
    var mouseButton: Int = 0
    var dx: Double = 0
    var dy: Double = 0
    var delayMs: UInt32 = 0

    static let sourceTypes = ["key", "gamepad_button", "gamepad_axis", "touch_zone", "mouse_button"]
    static let actionTypes = ["key", "key_chord", "mouse_click", "mouse_move", "wheel", "macro"]

    var valid: Bool {
        Self.sourceTypes.contains(sourceType) &&
        Self.actionTypes.contains(actionType) &&
        (actionType == "mouse_click" || actionType == "mouse_move" || actionType == "wheel" || !keys.isEmpty) &&
        (actionType != "key_chord" || keys.count >= 2) &&
        (sourceType != "touch_zone" || rect.count == 4)
    }

    var sourceLine: String {
        switch sourceType {
        case "gamepad_axis":
            return "\(sourceType):\(sourceCode):\(threshold)"
        case "touch_zone":
            let r = rect.count == 4 ? rect : [0, 0, 0, 0]
            return "\(sourceType):\(sourceCode):\(r[0]),\(r[1]),\(r[2]),\(r[3])"
        default:
            return "\(sourceType):\(sourceCode)"
        }
    }

    var targetLine: String {
        switch actionType {
        case "mouse_click":
            return "\(actionType):\(mouseButton)"
        case "mouse_move", "wheel":
            return "\(actionType):\(dx):\(dy)"
        case "macro":
            return "\(actionType):\(keys.map(String.init).joined(separator: ",")):\(delayMs)"
        default:
            return "\(actionType):\(keys.map(String.init).joined(separator: ","))"
        }
    }
}

struct KeyMapProfile: Codable, Identifiable, Equatable {
    var id: String { name }
    var name: String
    var rules: [KeyMapRule]
}

final class RemoteKeyMapStore: ObservableObject {
    @Published private(set) var revision = 0
    private let defaults: UserDefaults
    private let prefix = "remote_keymap_v1."

    init(defaults: UserDefaults = .standard) {
        self.defaults = defaults
        if profiles.isEmpty {
            let fps = KeyMapProfile(name: "FPS", rules: [
                KeyMapRule(id: "look", sourceType: "touch_zone", sourceCode: 0,
                           rect: [0.5, 0, 0.5, 0.75], actionType: "mouse_move", dx: 1, dy: 1),
                KeyMapRule(id: "fire", sourceType: "gamepad_button", sourceCode: 0,
                           actionType: "mouse_click", mouseButton: 0),
                KeyMapRule(id: "reload", sourceType: "gamepad_button", sourceCode: 2,
                           actionType: "key", keys: [0x52]),
            ])
            let moba = KeyMapProfile(name: "MOBA", rules: [
                KeyMapRule(id: "attack", sourceType: "gamepad_button", sourceCode: 0,
                           actionType: "mouse_click", mouseButton: 0),
                KeyMapRule(id: "spell", sourceType: "gamepad_button", sourceCode: 1,
                           actionType: "key", keys: [0x51]),
            ])
            write([fps, moba])
        }
    }

    var profiles: [KeyMapProfile] {
        guard let data = defaults.data(forKey: prefix + "profiles"),
              let decoded = try? JSONDecoder().decode([KeyMapProfile].self, from: data) else {
            return []
        }
        return decoded.filter { !$0.name.isEmpty }
    }

    var activeName: String {
        get { defaults.string(forKey: prefix + "active") ?? "FPS" }
        set { defaults.set(newValue, forKey: prefix + "active"); revision &+= 1 }
    }

    var activeProfile: KeyMapProfile? {
        profiles.first { $0.name == activeName } ?? profiles.first
    }

    func serialized(profile: KeyMapProfile) -> String {
        var lines = ["profile \(profile.name)"]
        for rule in profile.rules where rule.enabled {
            lines.append("rule \(rule.id) 1 \(rule.sourceLine) \(rule.targetLine)")
        }
        return lines.joined(separator: "\n") + "\n"
    }

    func activeText(device: String) -> String {
        guard let profile = activeProfile else { return "" }
        return serialized(profile: profile)
    }

    func save(_ profile: KeyMapProfile) {
        var values = profiles
        if let index = values.firstIndex(where: { $0.name == profile.name }) {
            values[index] = profile
        } else {
            values.append(profile)
        }
        write(values)
        activeName = profile.name
    }

    func remove(name: String) {
        var values = profiles
        values.removeAll { $0.name == name }
        write(values)
        if activeName == name { activeName = values.first?.name ?? "" }
    }

    private func write(_ values: [KeyMapProfile]) {
        guard let data = try? JSONEncoder().encode(values) else { return }
        defaults.set(data, forKey: prefix + "profiles")
        revision &+= 1
    }
}
