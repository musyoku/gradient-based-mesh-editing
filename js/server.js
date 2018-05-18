const express = require("express")
const dev = process.env.NODE_ENV !== "production"
const next = require("next")
const app = next({ dev })
const handle = app.getRequestHandler()
const port_http = 8080
const port_websocket = 8081
const enums = require("./enums")

const WebSocketServer = require("ws").Server
const wss = new WebSocketServer({ "port": port_websocket })
wss.on("connection", (ws) => {
    ws.on("message", (message) => {
        console.log("received: %s", message)
    })
})

const broadcast = (data) => {
    const buffer = Buffer.concat(data)
    wss.clients.forEach(client => {
        client.send(buffer)     // バイナリデータをそのまま送る
    })
}

const pack = (req, res, event_code, done) => {
    // websocketで送られてくるデータの種類を表す整数値を先頭に入れる
    const data = [Buffer.from([event_code])]
    // 逐次的にデータの読み出しが行われる
    req.on("data", (chunk) => {
        data.push(chunk)
    })
    // 全データを読み込んだら呼ばれる
    req.on("end", () => {
        done(data)
    })
    res.send({
        "success": true
    })
}

app.prepare().then(() => {
    const server = express()
    server.post("/init_object", (req, res) => {
        pack(req, res, enums.event.init_object, broadcast)
    })
    server.post("/update_object", (req, res) => {
        pack(req, res, enums.event.update_object, broadcast)
    })
    server.post("/update_top_right_image", (req, res) => {
        pack(req, res, enums.event.update_top_right_image, broadcast)
    })
    server.post("/update_bottom_right_image", (req, res) => {
        pack(req, res, enums.event.update_bottom_right_image, broadcast)
    })
    server.post("/update_top_left_image", (req, res) => {
        pack(req, res, enums.event.update_top_left_image, broadcast)
    })
    server.post("/update_bottom_left_image", (req, res) => {
        pack(req, res, enums.event.update_bottom_left_image, broadcast)
    })
    server.post("/init_image_area", (req, res) => {
        pack(req, res, enums.event.init_image_area, broadcast)
    })
    server.get("/", (req, res) => {
        return app.render(req, res, "/", {})
    })
    server.get("*", (req, res) => {
        return handle(req, res)
    })
    server.listen(port_http, (error) => {
        if (error) {
            throw error
        }
        console.log(`http://localhost:${port_http}`)
    })
})