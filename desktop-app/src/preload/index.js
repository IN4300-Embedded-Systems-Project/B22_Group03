const { contextBridge, ipcRenderer } = require('electron')

contextBridge.exposeInMainWorld('lora', {
    // One-time data fetch
    getRegistry: () => ipcRenderer.invoke('get-registry'),
    getAlerts: () => ipcRenderer.invoke('get-alerts'),
    setPort: (port) => ipcRenderer.invoke('set-port', port),

    // Live push events from main process
    onRegistryUpdate: (cb) => {
        ipcRenderer.on('registry-update', (_e, data) => cb(data))
    },
    onAlert: (cb) => {
        ipcRenderer.on('new-alert', (_e, alert) => cb(alert))
    },
    onSerialStatus: (cb) => {
        ipcRenderer.on('serial-status', (_e, status) => cb(status))
    },

    // List available serial ports
    listPorts: () => ipcRenderer.invoke('list-ports'),

    // Cleanup
    removeAllListeners: (channel) => ipcRenderer.removeAllListeners(channel)
})
