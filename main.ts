/**
 * Sprachnachrichten per Funk (nur micro:bit V2)
 */
//% color="#E63022" icon="\uf130" block="Funk-Audio" weight=90
namespace funkaudio {
    const T_START = 1
    const T_DATA = 2
    const T_END = 3
    const PAYLOAD = 16

    let initialized = false
    let receivedHandler: () => void = null
    let receiving = false
    let expectedLen = 0

    function init() {
        if (initialized) return
        initialized = true
        radio.onReceivedBuffer(onPacket)
    }

    function onPacket(b: Buffer) {
        if (b.length < 3) return
        const type = b[0]
        const num = b.getNumber(NumberFormat.UInt16LE, 1)
        if (type == T_START) {
            if (!receiving) {
                receiving = true
                expectedLen = num
                prepareReceive(num)
            }
        } else if (type == T_DATA && receiving) {
            setChunk(num * PAYLOAD, b.slice(3))
        } else if (type == T_END && receiving) {
            receiving = false
            if (receivedHandler) control.runInParallel(receivedHandler)
        }
    }

    /**
     * Startet den Empfang. Einmal beim Start aufrufen.
     * @param group Funkgruppe (muss bei beiden micro:bits gleich sein)
     */
    //% block="Funk-Audio aktivieren in Gruppe %group"
    //% group.min=0 group.max=255 group.defl=1
    //% weight=100
    export function enable(group: number) {
        radio.setGroup(group)
        radio.setTransmitPower(7)
        init()
    }

    /**
     * Nimmt über das Mikrofon auf (max. 4000 ms).
     */
    //% block="nimm %ms ms auf"
    //% ms.min=100 ms.max=4000 ms.defl=3000
    //% weight=90
    export function record(ms: number) {
        startRecording()
        basic.pause(Math.min(ms, 4000))
        stopRecording()
    }

    /**
     * Schickt die letzte Aufnahme per Funk.
     */
    //% block="sende Aufnahme"
    //% weight=80
    export function send() {
        init()
        const len = recordedLength()
        if (len == 0) return
        const chunks = Math.idiv(len + PAYLOAD - 1, PAYLOAD)

        const head = control.createBuffer(3)
        head[0] = T_START
        head.setNumber(NumberFormat.UInt16LE, 1, len)
        for (let i = 0; i < 3; i++) { radio.sendBuffer(head); basic.pause(20) }

        // zwei Durchgänge für mehr Zuverlässigkeit
        for (let pass = 0; pass < 2; pass++) {
            for (let c = 0; c < chunks; c++) {
                const pkt = control.createBuffer(3)
                pkt[0] = T_DATA
                pkt.setNumber(NumberFormat.UInt16LE, 1, c)
                radio.sendBuffer(pkt.concat(getChunk(c * PAYLOAD, PAYLOAD)))
                basic.pause(4)
            }
        }

        const end = control.createBuffer(3)
        end[0] = T_END
        for (let i = 0; i < 3; i++) { radio.sendBuffer(end); basic.pause(20) }
    }

    /**
     * Spielt die gespeicherte (eigene oder empfangene) Aufnahme ab.
     */
    //% block="spiele Aufnahme ab"
    //% weight=70
    export function play() {
        _play()
        basic.pause(20)
        while (_isPlaying()) basic.pause(20)
    }

    /**
     * Wird ausgeführt, wenn eine Aufnahme komplett empfangen wurde.
     */
    //% block="wenn Aufnahme empfangen"
    //% weight=60
    export function onReceived(handler: () => void) {
        init()
        receivedHandler = handler
    }

    /**
     * Mikrofon-Verstärkung (1 = leise, 64 = sehr laut). Standard: 4
     */
    //% block="setze Mikrofon-Verstärkung auf %g"
    //% g.min=1 g.max=64 g.defl=4
    //% advanced=true
    export function setGain(g: number) {
        _setGain(g)
    }

    /**
     * Länge der gespeicherten Aufnahme in Samples (4000 = 1 Sekunde)
     */
    //% block="Länge der Aufnahme"
    //% advanced=true
    export function length(): number {
        return recordedLength()
    }
}