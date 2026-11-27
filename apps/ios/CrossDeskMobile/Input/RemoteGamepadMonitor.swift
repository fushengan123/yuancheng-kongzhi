import Combine
import Foundation
import GameController

// feature/parity: captures physical game controllers through GameController and
// reports a normalized snapshot. Button indices follow the shared key map
// convention: A=0, B=1, X=2, Y=3, LB=4, RB=5, LT=6, RT=7, Menu=8, Options=9,
// L3=10, R3=11, DUp=12, DDown=13, DLeft=14, DRight=15. Axes: LX=0, LY=1,
// RX=2, RY=3, LT=4, RT=5 in the -32768..32767 range.
final class RemoteGamepadMonitor: ObservableObject {
    var onState: ((Set<UInt32>, [UInt32: Int]) -> Void)?
    private var observers: [NSObjectProtocol] = []
    private var previousButtons: Set<UInt32> = []

    func start() {
        let center = NotificationCenter.default
        observers.append(center.addObserver(forName: .GCControllerDidConnect,
                                           object: nil, queue: .main) { [weak self] note in
            guard let controller = note.object as? GCController else { return }
            self?.attach(controller)
        })
        observers.append(center.addObserver(forName: .GCControllerDidDisconnect,
                                           object: nil, queue: .main) { [weak self] _ in
            self?.publish()
        })
        for controller in GCController.controllers() {
            attach(controller)
        }
        GCController.startWirelessControllerDiscovery {}
    }

    func stop() {
        for observer in observers {
            NotificationCenter.default.removeObserver(observer)
        }
        observers.removeAll()
        GCController.stopWirelessControllerDiscovery()
        for controller in GCController.controllers() {
            controller.extendedGamepad?.valueChangedHandler = nil
        }
        previousButtons = []
    }

    private func attach(_ controller: GCController) {
        guard let pad = controller.extendedGamepad else { return }
        pad.valueChangedHandler = { [weak self] _, _ in
            self?.publish()
        }
        publish()
    }

    private func publish() {
        var buttons = Set<UInt32>()
        var axes: [UInt32: Int] = [0: 0, 1: 0, 2: 0, 3: 0, 4: 0, 5: 0]
        guard let pad = GCController.controllers()
            .compactMap({ $0.extendedGamepad })
            .first else {
            if !previousButtons.isEmpty {
                previousButtons = []
                onState?([], axes)
            }
            return
        }
        func button(_ pressed: Bool, _ index: UInt32) {
            if pressed { buttons.insert(index) }
        }
        button(pad.buttonA.isPressed, 0)
        button(pad.buttonB.isPressed, 1)
        button(pad.buttonX.isPressed, 2)
        button(pad.buttonY.isPressed, 3)
        button(pad.leftShoulder.isPressed, 4)
        button(pad.rightShoulder.isPressed, 5)
        button(pad.leftTrigger.isPressed, 6)
        button(pad.rightTrigger.isPressed, 7)
        button(pad.buttonMenu.isPressed, 8)
        button(pad.buttonOptions?.isPressed ?? false, 9)
        button(pad.leftThumbstickButton?.isPressed ?? false, 10)
        button(pad.rightThumbstickButton?.isPressed ?? false, 11)
        button(pad.dpad.up.isPressed, 12)
        button(pad.dpad.down.isPressed, 13)
        button(pad.dpad.left.isPressed, 14)
        button(pad.dpad.right.isPressed, 15)

        axes[0] = Int(pad.leftThumbstick.xAxis.value * 32767)
        axes[1] = Int(pad.leftThumbstick.yAxis.value * 32767)
        axes[2] = Int(pad.rightThumbstick.xAxis.value * 32767)
        axes[3] = Int(pad.rightThumbstick.yAxis.value * 32767)
        axes[4] = Int(pad.leftTrigger.value * 32767)
        axes[5] = Int(pad.rightTrigger.value * 32767)

        previousButtons = buttons
        onState?(buttons, axes)
    }
}
