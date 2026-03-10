/* ── renderer.js – Jungle Protection System ── */
; (function () {
    'use strict'

    // ── State ──────────────────────────────────────────────────────────────────
    const MAX_RSSI_POINTS = 60
    const rssiData = { labels: [], values: [] }
    const logRows = []   // { time, event, rssi }
    let alertCount = 0
    let alarmPlaying = false

    // ── DOM refs ───────────────────────────────────────────────────────────────
    const $ = id => document.getElementById(id)
    const portSelect = $('portSelect')
    const connectBtn = $('connectBtn')
    const disconnectBtn = $('disconnectBtn')
    const serialStatus = $('serialStatus')
    const alertBanner = $('alertBanner')
    const alertIcon = $('alertIcon')
    const alertTitle = $('alertTitle')
    const alertMeta = $('alertMeta')
    const alertTime = $('alertTime')
    const statusDot = $('statusDot')
    const statusLabel = $('statusLabel')
    const lastHeartbeat = $('lastHeartbeat')
    const currentRssi = $('currentRssi')
    const alertCountEl = $('alertCount')
    const rssiLarge = $('rssiLarge')
    const logBody = $('logBody')
    const saveLogsBtn = $('saveLogsBtn')

    // ── Chart.js RSSI Chart ────────────────────────────────────────────────────
    const chart = new Chart($('rssiChart'), {
        type: 'line',
        data: {
            labels: rssiData.labels,
            datasets: [{
                label: 'RSSI (dBm)',
                data: rssiData.values,
                borderColor: '#0d9488',
                backgroundColor: 'rgba(13,148,136,0.12)',
                borderWidth: 2,
                pointRadius: 0,
                tension: 0.4,
                fill: true
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            animation: { duration: 300 },
            plugins: {
                legend: { display: false }, tooltip: {
                    backgroundColor: '#0f2313',
                    titleColor: '#6ee7b7',
                    bodyColor: '#d1fae5',
                    callbacks: { label: ctx => `${ctx.parsed.y} dBm` }
                }
            },
            scales: {
                x: {
                    ticks: { color: '#374d3c', maxTicksLimit: 8, font: { family: 'JetBrains Mono', size: 10 } },
                    grid: { color: '#1a3d1f' }
                },
                y: {
                    min: -110, max: -40,
                    ticks: {
                        color: '#374d3c', font: { family: 'JetBrains Mono', size: 10 },
                        callback: v => `${v} dBm`
                    },
                    grid: { color: '#1a3d1f' }
                }
            }
        }
    })

    function pushRssi(rssi, timeLabel) {
        if (rssi == null) return
        rssiData.labels.push(timeLabel)
        rssiData.values.push(rssi)
        if (rssiData.labels.length > MAX_RSSI_POINTS) {
            rssiData.labels.shift(); rssiData.values.shift()
        }
        chart.update('none')
    }

    // ── Alarm Sound (Web Audio API) ────────────────────────────────────────────
    const AudioCtx = window.AudioContext || window.webkitAudioContext
    let audioCtx = null

    function playAlarm() {
        if (alarmPlaying) return
        alarmPlaying = true
        if (!audioCtx) audioCtx = new AudioCtx()

        let t = audioCtx.currentTime
        const beepCount = 3
        for (let i = 0; i < beepCount; i++) {
            const osc = audioCtx.createOscillator()
            const gain = audioCtx.createGain()
            osc.connect(gain); gain.connect(audioCtx.destination)
            osc.type = 'square'
            osc.frequency.value = 880
            gain.gain.setValueAtTime(0.25, t + i * 0.4)
            gain.gain.exponentialRampToValueAtTime(0.001, t + i * 0.4 + 0.3)
            osc.start(t + i * 0.4)
            osc.stop(t + i * 0.4 + 0.31)
        }
        setTimeout(() => { alarmPlaying = false }, beepCount * 400 + 200)
    }

    // ── Alert Panel ────────────────────────────────────────────────────────────
    function setAlertDanger(threat, rssi, timestamp) {
        alertBanner.className = 'alert-banner danger'
        alertIcon.textContent = '🚨'
        alertTitle.textContent = `${threat.toUpperCase()} DETECTED!`
        alertMeta.textContent = `RSSI: ${rssi != null ? rssi + ' dBm' : '-'}  |  Immediate action required`
        alertTime.textContent = formatTime(timestamp)
        playAlarm()
        // Auto-reset to safe after 15 s with no new alert
        clearTimeout(setAlertDanger._timer)
        setAlertDanger._timer = setTimeout(setAlertSafe, 15000)
    }

    function setAlertSafe() {
        alertBanner.className = 'alert-banner safe'
        alertIcon.textContent = '🛡️'
        alertTitle.textContent = 'NO THREAT DETECTED'
        alertMeta.textContent = 'Monitoring active - all clear'
        alertTime.textContent = '-'
    }

    // ── Node Status ────────────────────────────────────────────────────────────
    function setNodeOnline(rssi, timestamp) {
        statusDot.className = 'status-dot online'
        statusLabel.className = 'status-label online'
        statusLabel.textContent = 'ONLINE'
        lastHeartbeat.textContent = formatTime(timestamp)
        if (rssi != null) {
            currentRssi.textContent = `${rssi} dBm`
            rssiLarge.textContent = `${rssi} dBm`
            rssiLarge.style.color = rssiColor(rssi)
            currentRssi.style.color = rssiColor(rssi)
        }
    }

    function setNodeOffline() {
        statusDot.className = 'status-dot offline'
        statusLabel.className = 'status-label offline'
        statusLabel.textContent = 'OFFLINE'
    }

    function rssiColor(v) {
        if (v >= -70) return '#22c55e'
        if (v >= -85) return '#f59e0b'
        return '#ef4444'
    }

    // ── Log Table ──────────────────────────────────────────────────────────────
    function appendLog(event, rssi, timestamp) {
        const time = formatTime(timestamp)
        logRows.unshift({ time, event, rssi: rssi != null ? `${rssi} dBm` : '-' })

        // Remove placeholder row
        const emptyRow = logBody.querySelector('.log-empty-row')
        if (emptyRow) emptyRow.remove()

        const tr = document.createElement('tr')
        const isAlert = event.includes('ALERT') || event.includes('CHAINSAW') || event.includes('DETECTED')
        tr.className = isAlert ? 'row-alert' : 'row-heartbeat'
        tr.innerHTML = `
      <td class="log-time">${time}</td>
      <td class="log-event">${event}</td>
      <td class="log-rssi">${rssi != null ? rssi + ' dBm' : '-'}</td>
    `
        logBody.prepend(tr)

        // Trim to 200 rows
        while (logBody.children.length > 200) logBody.removeChild(logBody.lastChild)
    }

    // ── IPC Event Handlers ─────────────────────────────────────────────────────
    window.jps.onData(packet => {
        const ts = packet.timestamp || new Date().toISOString()
        const timeLabel = ts.slice(11, 19)  // HH:MM:SS

        if (packet.type === 'ALERT') {
            alertCount++
            alertCountEl.textContent = alertCount
            setAlertDanger(packet.threat || 'UNKNOWN', packet.rssi, ts)
            appendLog(`🚨 ALERT – ${packet.threat}`, packet.rssi, ts)
            pushRssi(packet.rssi, timeLabel)
        } else if (packet.type === 'HEARTBEAT') {
            appendLog('💚 HEARTBEAT', packet.rssi, ts)
            pushRssi(packet.rssi, timeLabel)
        }
    })

    window.jps.onStatus(({ status, rssi }) => {
        if (status === 'online') {
            setNodeOnline(rssi, new Date().toISOString())
        } else {
            setNodeOffline()
        }
    })

    window.jps.onSerialStatus(({ connected, port, error }) => {
        if (connected) {
            serialStatus.textContent = `● Connected: ${port}`
            serialStatus.className = 'serial-status serial-connected'
            connectBtn.disabled = true
            disconnectBtn.disabled = false
            portSelect.disabled = true
        } else {
            serialStatus.textContent = error ? `● Error: ${error}` : '● Disconnected'
            serialStatus.className = 'serial-status serial-disconnected'
            connectBtn.disabled = false
            disconnectBtn.disabled = true
            portSelect.disabled = false
            setNodeOffline()
        }
    })

    // ── COM Port Connect/Disconnect ────────────────────────────────────────────
    connectBtn.addEventListener('click', async () => {
        const selectedPort = portSelect.value
        if (!selectedPort) return alert('Please select a COM port first.')
        await window.jps.connectPort(selectedPort)
    })

    disconnectBtn.addEventListener('click', async () => {
        await window.jps.disconnect()
    })

    // ── Save Logs ──────────────────────────────────────────────────────────────
    saveLogsBtn.addEventListener('click', async () => {
        if (logRows.length === 0) return alert('No events to save yet.')
        const result = await window.jps.saveLogs(logRows)
        if (result.ok) {
            alert(`✅ Log saved to:\n${result.path}`)
        } else {
            alert(`❌ Failed to save: ${result.error}`)
        }
    })

    // ── Populate port list on load ─────────────────────────────────────────────
    async function loadPorts() {
        const ports = await window.jps.listPorts()
        portSelect.innerHTML = '<option value="">Select COM Port…</option>'
        ports.forEach(p => {
            const opt = document.createElement('option')
            opt.value = p; opt.textContent = p
            portSelect.appendChild(opt)
        })
        // Pre-select COM3 if available
        if (ports.includes('COM3')) portSelect.value = 'COM3'

        const mockOpt = document.createElement('option')
        mockOpt.value = 'MOCK_PORT'; mockOpt.textContent = 'MOCK_PORT (Dummy Data)'
        portSelect.appendChild(mockOpt)
    }

    loadPorts()

    // ── Helpers ────────────────────────────────────────────────────────────────
    function formatTime(iso) {
        try {
            return new Date(iso).toLocaleTimeString('en-GB', {
                hour: '2-digit', minute: '2-digit', second: '2-digit'
            })
        } catch { return '-' }
    }

})()
