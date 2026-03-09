const { app, BrowserWindow, ipcMain, dialog } = require('electron')
const path = require('path')
const fs = require('fs')
const { SerialPort } = require('serialport')
const { ReadlineParser } = require('@serialport/parser-readline')

const HEARTBEAT_TIMEOUT = 30_000  // ms before node goes offline
const LOGS_DIR = path.join(app.getPath('userData'), 'logs')

let mainWindow = null
let port = null
let parser = null
let heartbeatTimer = null

// ── Window ─────────────────────────────────────────────────────────────────

function createWindow() {
    mainWindow = new BrowserWindow({
        width: 1100,
        height: 700,
        minWidth: 800,
        minHeight: 500,
        backgroundColor: '#0a1a0f',
        webPreferences: {
            preload: path.join(__dirname, 'preload.js'),
            contextIsolation: true,
            nodeIntegration: false
        },
        title: 'Jungle Protection System – Node J01',
        icon: path.join(__dirname, '../assets/icon.png')
    })

    mainWindow.loadFile(path.join(__dirname, 'renderer/index.html'))
    mainWindow.on('closed', () => { mainWindow = null })
}

// ── Serial Port ─────────────────────────────────────────────────────────────

function scheduleOffline() {
    clearTimeout(heartbeatTimer)
    heartbeatTimer = setTimeout(() => {
        mainWindow?.webContents.send('node-status', { status: 'offline' })
    }, HEARTBEAT_TIMEOUT)
}

function parseSerialLine(raw) {
    const line = raw.trim()
    if (!line) return

    // Format: ALERT|CHAINSAW|RSSI:-78  or  HEARTBEAT|RSSI:-65
    const parts = line.split('|')
    const eventType = parts[0]?.toUpperCase()

    let rssi = null
    const rssiPart = parts.find(p => p.toUpperCase().startsWith('RSSI:'))
    if (rssiPart) rssi = parseInt(rssiPart.split(':')[1], 10)

    if (eventType === 'ALERT') {
        const threat = parts[1] || 'UNKNOWN'
        const payload = { type: 'ALERT', threat, rssi, timestamp: new Date().toISOString() }
        mainWindow?.webContents.send('serial-data', payload)
        mainWindow?.webContents.send('node-status', { status: 'online', rssi })
        scheduleOffline()
    } else if (eventType === 'HEARTBEAT') {
        const payload = { type: 'HEARTBEAT', rssi, timestamp: new Date().toISOString() }
        mainWindow?.webContents.send('serial-data', payload)
        mainWindow?.webContents.send('node-status', { status: 'online', rssi })
        scheduleOffline()
    } else {
        // Unknown line — still reset heartbeat if anything arrives
        scheduleOffline()
    }
}

function connectPort(comPort) {
    if (port && port.isOpen) {
        port.close()
        port = null
    }

    port = new SerialPort({ path: comPort, baudRate: 115200, autoOpen: false })
    parser = port.pipe(new ReadlineParser({ delimiter: '\n' }))

    port.open(err => {
        if (err) {
            mainWindow?.webContents.send('serial-status', { connected: false, port: comPort, error: err.message })
            return
        }
        mainWindow?.webContents.send('serial-status', { connected: true, port: comPort })
        scheduleOffline()  // start offline timer immediately
    })

    parser.on('data', parseSerialLine)
    port.on('error', err => mainWindow?.webContents.send('serial-status', { connected: false, error: err.message }))
    port.on('close', () => {
        clearTimeout(heartbeatTimer)
        mainWindow?.webContents.send('serial-status', { connected: false, port: comPort })
        mainWindow?.webContents.send('node-status', { status: 'offline' })
    })
}

// ── Log Saving ──────────────────────────────────────────────────────────────

async function saveLogs(data) {
    try {
        if (!fs.existsSync(LOGS_DIR)) fs.mkdirSync(LOGS_DIR, { recursive: true })
        const date = new Date().toISOString().slice(0, 10)
        const filePath = path.join(LOGS_DIR, `jungle-log-${date}.csv`)
        const header = 'Time,Event,RSSI\n'
        const rows = data.map(r => `"${r.time}","${r.event}","${r.rssi}"`).join('\n')
        const content = header + rows + '\n'
        fs.writeFileSync(filePath, content, 'utf8')
        return { ok: true, path: filePath }
    } catch (e) {
        return { ok: false, error: e.message }
    }
}

// ── IPC ─────────────────────────────────────────────────────────────────────

function registerIPC() {
    ipcMain.handle('list-ports', async () => {
        try {
            const ports = await SerialPort.list()
            return ports.map(p => p.path)
        } catch { return [] }
    })

    ipcMain.handle('connect-port', (_e, comPort) => {
        try {
            if (comPort === 'MOCK_PORT') {
                startMockData();
                return { ok: true }
            }
            connectPort(comPort);
            return { ok: true }
        }
        catch (e) { return { ok: false, error: e.message } }
    })

    ipcMain.handle('disconnect', () => {
        if (port && port.isOpen) port.close()
        clearTimeout(heartbeatTimer)
        clearInterval(mockInterval)
        return { ok: true }
    })

    ipcMain.handle('save-logs', (_e, data) => saveLogs(data))
}

let mockInterval = null;
function startMockData() {
    if (mockInterval) clearInterval(mockInterval);
    mainWindow?.webContents.send('serial-status', { connected: true, port: 'MOCK_PORT' });
    scheduleOffline();
    mockInterval = setInterval(() => {
        const isAlert = Math.random() < 0.1;
        const rssi = -90 + Math.floor(Math.random() * 30);
        if (isAlert) {
            const threats = ['CHAINSAW', 'GUNSHOT', 'HUMAN_VOICE'];
            const randomThreat = threats[Math.floor(Math.random() * threats.length)];
            const line = `ALERT|${randomThreat}|RSSI:${rssi}`;
            parseSerialLine(line);
        } else {
            const line = `HEARTBEAT|RSSI:${rssi}`;
            parseSerialLine(line);
        }
    }, 2000);
}

// ── App lifecycle ────────────────────────────────────────────────────────────

app.whenReady().then(() => {
    createWindow()
    registerIPC()
    app.on('activate', () => { if (!mainWindow) createWindow() })
})

app.on('window-all-closed', () => {
    if (port && port.isOpen) port.close()
    if (process.platform !== 'darwin') app.quit()
})
