const { contextBridge, ipcRenderer } = require('electron')

contextBridge.exposeInMainWorld('jps', {
    // Serial port control
    listPorts: () => ipcRenderer.invoke('list-ports'),
    connectPort: (port) => ipcRenderer.invoke('connect-port', port),
    disconnect: () => ipcRenderer.invoke('disconnect'),

    // Log saving
    saveLogs: (data) => ipcRenderer.invoke('save-logs', data),

    // Push events from main → renderer
    onData: (cb) => ipcRenderer.on('serial-data', (_e, d) => cb(d)),
    onStatus: (cb) => ipcRenderer.on('node-status', (_e, s) => cb(s)),
    onSerialStatus: (cb) => ipcRenderer.on('serial-status', (_e, s) => cb(s)),

    removeAll: (ch) => ipcRenderer.removeAllListeners(ch)
})
