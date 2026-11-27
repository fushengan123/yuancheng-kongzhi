import SwiftUI

// feature/parity: minimal rule editor. Uses the same profile model as the
// shared key mapping engine; no branding or product copy is included.
struct RemoteKeyMapEditor: View {
    @EnvironmentObject var store: RemoteKeyMapStore
    @State private var draft: KeyMapProfile
    @State private var message = ""

    init(profile: KeyMapProfile? = nil) {
        _draft = State(initialValue: profile ?? KeyMapProfile(name: "", rules: []))
    }

    private func uintBinding(_ value: Binding<UInt32>) -> Binding<String> {
        Binding<String>(
            get: { String(value.wrappedValue) },
            set: { value.wrappedValue = UInt32($0) ?? 0 }
        )
    }

    private func intBinding(_ value: Binding<Int>) -> Binding<String> {
        Binding<String>(
            get: { String(value.wrappedValue) },
            set: { value.wrappedValue = Int($0) ?? 0 }
        )
    }

    private func doubleBinding(_ value: Binding<Double>) -> Binding<String> {
        Binding<String>(
            get: { String(value.wrappedValue) },
            set: { value.wrappedValue = Double($0) ?? 0 }
        )
    }

    var body: some View {
        Form {
            Section(header: Text("配置")) {
                TextField("名称", text: $draft.name)
                Picker("载入配置", selection: $draft.name) {
                    ForEach(store.profiles) { profile in
                        Text(profile.name).tag(profile.name)
                    }
                }
                .onChange(of: draft.name) { newValue in
                    if let found = store.profiles.first(where: { $0.name == newValue }) {
                        draft = found
                    }
                }
            }

            Section(header: Text("规则")) {
                ForEach($draft.rules) { $rule in
                    VStack(alignment: .leading, spacing: 6) {
                        Toggle("启用", isOn: $rule.enabled)
                        Picker("输入类型", selection: $rule.sourceType) {
                            ForEach(KeyMapRule.sourceTypes, id: \.self) { Text($0).tag($0) }
                        }
                        HStack {
                            Text("编号")
                            TextField("0", text: uintBinding($rule.sourceCode))
                                .keyboardType(.numberPad)
                        }
                        if rule.sourceType == "gamepad_axis" {
                            HStack {
                                Text("阈值")
                                TextField("0", text: intBinding($rule.threshold))
                                    .keyboardType(.numbersAndPunctuation)
                            }
                        }
                        Picker("动作类型", selection: $rule.actionType) {
                            ForEach(KeyMapRule.actionTypes, id: \.self) { Text($0).tag($0) }
                        }
                        if rule.actionType == "key" || rule.actionType == "key_chord" || rule.actionType == "macro" {
                            HStack {
                                Text("键值")
                                TextField("87,17", text: Binding(
                                    get: { rule.keys.map(String.init).joined(separator: ",") },
                                    set: { text in
                                        rule.keys = text.split(separator: ",").compactMap { UInt32($0.trimmingCharacters(in: .whitespaces)) }
                                    }
                                ))
                            }
                        }
                        if rule.actionType == "mouse_click" {
                            HStack {
                                Text("按键")
                                TextField("0", text: intBinding($rule.mouseButton))
                                    .keyboardType(.numberPad)
                            }
                        }
                        if rule.actionType == "mouse_move" || rule.actionType == "wheel" {
                            HStack {
                                Text("X")
                                TextField("0", text: doubleBinding($rule.dx))
                                Text("Y")
                                TextField("0", text: doubleBinding($rule.dy))
                            }
                            .keyboardType(.numbersAndPunctuation)
                        }
                    }
                    .padding(.vertical, 4)
                }
                Button("添加规则") {
                    draft.rules.append(KeyMapRule())
                }
            }

            Section {
                Button("保存配置") {
                    guard !draft.name.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty,
                          !draft.rules.isEmpty,
                          draft.rules.allSatisfy({ $0.valid }) else {
                        message = "请填写名称并确认规则有效"
                        return
                    }
                    store.save(draft)
                    message = "已保存"
                }
                if !message.isEmpty { Text(message).foregroundColor(.secondary) }
            }
        }
        .navigationTitle("按键映射")
    }
}
