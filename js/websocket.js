export default class WebSocketClient {
    constructor(domain, port) {
        this.listeners = []
        this.ws = null
        this.initial_reconnect_interval = 1000
        this.max_reconnect_interval = 30000
        this.reconnect_decay = 1.5
        this.reconnect_interval = this.initial_reconnect_interval
        this.timer_id = 0
        this.domain = domain
        this.port = port
        this.initWebSocket()
    }
    initWebSocket() {
        if (this.ws) {
            for (const listener of this.listeners) {
                this.ws.removeEventListener(listener.name, listener.callback)
            }
        }
        const url = `ws://${this.domain}:${this.port}`
        console.log(`connecting ${url}`)
        this.ws = new WebSocket(url)
        this.ws.onerror = (e) => {
            console.log("onerror", e)
        }
        this.ws.onclose = (e) => {
            console.log("onclose", e)
            clearTimeout(this.timer_id)
            this.timer_id = setTimeout(() => this.initWebSocket(), this.reconnect_interval);
            this.reconnect_interval = Math.min(this.max_reconnect_interval, this.reconnect_interval * this.reconnect_decay)
        }
        this.ws.onopen = (e) => {
            console.log("onopen", e)
            this.reconnect_interval = this.initial_reconnect_interval
        }
        this.ws.onerror = (e) => {
            console.log("onerror", e)
        }
        for (const listener of this.listeners) {
            this.ws.addEventListener(listener.name, listener.callback)
        }
    }
    addEventListener(name, callback) {
        this.listeners.push({ name, callback })
        this.ws.addEventListener(name, callback)
    }
}