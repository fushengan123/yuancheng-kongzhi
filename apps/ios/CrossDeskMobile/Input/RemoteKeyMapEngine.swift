import Foundation

// feature/parity: resolves the active key map profile into concrete remote
// actions. The rule model is shared with RemoteKeyMapStore so the editor and
// the input path stay in sync without a second on-disk format.
enum RemoteKeyMapAction {
    case key(UInt32)
    case chord([UInt32])
    case mouseClick(Int)
    case mouseMove(Double, Double)
    case wheel(Double, Double)
    case macro([UInt32], UInt32)

    static func from(rule: KeyMapRule) -> RemoteKeyMapAction? {
        guard rule.enabled else { return nil }
        switch rule.actionType {
        case "key":
            guard let key = rule.keys.first else { return nil }
            return .key(key)
        case "key_chord":
            guard rule.keys.count >= 2 else { return nil }
            return .chord(rule.keys)
        case "mouse_click":
            return .mouseClick(rule.mouseButton)
        case "mouse_move":
            return .mouseMove(rule.dx, rule.dy)
        case "wheel":
            return .wheel(rule.dx, rule.dy)
        case "macro":
            guard !rule.keys.isEmpty else { return nil }
            return .macro(rule.keys, rule.delayMs)
        default:
            return nil
        }
    }
}

final class RemoteKeyMapEngine {
    private var keyRules: [UInt32: KeyMapRule] = [:]
    private var buttonRules: [UInt32: KeyMapRule] = [:]
    private var axisRules: [UInt32: KeyMapRule] = [:]

    init(profile: KeyMapProfile?) {
        guard let profile else { return }
        for rule in profile.rules where rule.enabled && rule.valid {
            switch rule.sourceType {
            case "key":
                keyRules[rule.sourceCode] = rule
            case "gamepad_button":
                buttonRules[rule.sourceCode] = rule
            case "gamepad_axis":
                axisRules[rule.sourceCode] = rule
            default:
                continue
            }
        }
    }

    var isEmpty: Bool {
        keyRules.isEmpty && buttonRules.isEmpty && axisRules.isEmpty
    }

    func resolveKey(_ code: UInt32) -> RemoteKeyMapAction? {
        guard let rule = keyRules[code] else { return nil }
        return RemoteKeyMapAction.from(rule: rule)
    }

    func resolveButton(_ code: UInt32) -> RemoteKeyMapAction? {
        guard let rule = buttonRules[code] else { return nil }
        return RemoteKeyMapAction.from(rule: rule)
    }

    func resolveAxis(_ code: UInt32, value: Int) -> RemoteKeyMapAction? {
        guard let rule = axisRules[code] else { return nil }
        let threshold = max(1, rule.threshold)
        guard abs(value) >= threshold else { return nil }
        return RemoteKeyMapAction.from(rule: rule)
    }
}
