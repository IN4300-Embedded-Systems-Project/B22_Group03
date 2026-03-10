import React, { createContext, useContext, useState, useEffect, useCallback } from 'react'

const LoraContext = createContext(null)

export function LoraProvider({ children }) {
    const [nodes, setNodes] = useState([])        // Array of NodeEntry
    const [alerts, setAlerts] = useState([])      // Alert history (newest first)
    const [activeAlert, setActiveAlert] = useState(null)  // Latest unread alert
    const [serialStatus, setSerialStatus] = useState({ open: false, port: '-' })

    // Bootstrap: load persisted data on mount
    useEffect(() => {
        window.lora.getRegistry().then(setNodes)
        window.lora.getAlerts().then(setAlerts)

        // Live pushes from main process
        window.lora.onRegistryUpdate((snapshot) => setNodes(snapshot))

        window.lora.onAlert((alert) => {
            setAlerts((prev) => [alert, ...prev].slice(0, 500))
            setActiveAlert(alert)
        })

        window.lora.onSerialStatus((status) => setSerialStatus(status))

        return () => {
            window.lora.removeAllListeners('registry-update')
            window.lora.removeAllListeners('new-alert')
            window.lora.removeAllListeners('serial-status')
        }
    }, [])

    const dismissAlert = useCallback(() => setActiveAlert(null), [])

    const setPort = useCallback(async (port) => {
        return window.lora.setPort(port)
    }, [])

    return (
        <LoraContext.Provider value={{ nodes, alerts, activeAlert, dismissAlert, serialStatus, setPort }}>
            {children}
        </LoraContext.Provider>
    )
}

export function useLora() {
    return useContext(LoraContext)
}
