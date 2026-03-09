import { SerialPort } from 'serialport'
import { ReadlineParser } from '@serialport/parser-readline'
import { updateNode, addAlert } from './nodeRegistry.js'

let port = null
let parser = null
let mainWindow = null

export function openPort(comPort, win) {
    mainWindow = win

    if (port && port.isOpen) port.close()

    port = new SerialPort({ path: comPort, baudRate: 115200, autoOpen: false })
    parser = port.pipe(new ReadlineParser({ delimiter: '\n' }))

    port.open((err) => {
        if (err) {
            console.error('[Serial] Failed to open port:', err.message)
            mainWindow?.webContents.send('serial-status', { open: false, error: err.message, port: comPort })
            return
        }
        console.log(`[Serial] Opened ${comPort} at 115200 baud`)
        mainWindow?.webContents.send('serial-status', { open: true, port: comPort })
    })

    parser.on('data', (line) => {
        line = line.trim()
        if (!line) return
        try {
            handlePacket(JSON.parse(line))
        } catch {
            console.warn('[Serial] Non-JSON line:', line)
        }
    })

    port.on('error', (err) => {
        console.error('[Serial] Error:', err.message)
        mainWindow?.webContents.send('serial-status', { open: false, error: err.message, port: comPort })
    })

    port.on('close', () => {
        mainWindow?.webContents.send('serial-status', { open: false, port: comPort })
    })
}

function handlePacket(packet) {
    if (!packet.nodeId) return
    const snapshot = updateNode(packet)
    mainWindow?.webContents.send('registry-update', snapshot)

    if (packet.type === 'alert') {
        const alert = addAlert(packet)
        mainWindow?.webContents.send('new-alert', alert)
    }
}

export function closePort() {
    if (port && port.isOpen) port.close()
}
