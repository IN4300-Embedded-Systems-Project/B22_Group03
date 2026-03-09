import { SerialPort } from 'serialport'
import { openPort, closePort } from './serialManager.js'
import { getRegistrySnapshot, getAlertHistory, updateNode, addAlert } from './nodeRegistry.js'

const MOCK_NODES = [
    { nodeId: 'N1', lat: 6.927, lng: 79.861 },
    { nodeId: 'N2', lat: 6.932, lng: 79.855 },
    { nodeId: 'N3', lat: 6.921, lng: 79.870 }
]

let mockInterval = null

function startMockMode(win) {
    console.log('[Mock] Starting mock data injection')
    mockInterval = setInterval(() => {
        const node = MOCK_NODES[Math.floor(Math.random() * MOCK_NODES.length)]
        const rssi = -60 - Math.floor(Math.random() * 40)
        const isAlert = Math.random() < 0.15

        const packet = {
            nodeId: node.nodeId,
            type: isAlert ? 'alert' : 'data',
            rssi,
            lat: node.lat + (Math.random() - 0.5) * 0.001,
            lng: node.lng + (Math.random() - 0.5) * 0.001,
            severity: ['info', 'warn', 'critical'][Math.floor(Math.random() * 3)],
            message: isAlert ? `Motion detected at sector ${Math.floor(Math.random() * 9) + 1}` : ''
        }

        const snapshot = updateNode(packet)
        win.webContents.send('registry-update', snapshot)

        if (isAlert) {
            const alert = addAlert(packet)
            win.webContents.send('new-alert', alert)
        }
    }, 2000)
}

function stopMockMode() {
    if (mockInterval) clearInterval(mockInterval)
}

export function registerIpcHandlers(ipcMain, win) {
    // Default: start in mock mode
    startMockMode(win)
    win.webContents.send('serial-status', { open: true, port: 'MOCK', mock: true })

    ipcMain.handle('get-registry', () => getRegistrySnapshot())
    ipcMain.handle('get-alerts', () => getAlertHistory())

    ipcMain.handle('set-port', async (_e, comPort) => {
        if (comPort === 'MOCK') {
            closePort()
            stopMockMode()
            startMockMode(win)
            win.webContents.send('serial-status', { open: true, port: 'MOCK', mock: true })
            return { ok: true }
        }
        stopMockMode()
        try {
            openPort(comPort, win)
            return { ok: true }
        } catch (e) {
            return { ok: false, error: e.message }
        }
    })

    ipcMain.handle('list-ports', async () => {
        try {
            const ports = await SerialPort.list()
            return ports.map(p => p.path)
        } catch {
            return []
        }
    })
}
