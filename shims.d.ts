// Automatisch nutzbar aus main.ts – nicht direkt als Blöcke sichtbar
declare namespace funkaudio {
    //% shim=funkaudio::startRecording
    function startRecording(): void;
    //% shim=funkaudio::stopRecording
    function stopRecording(): void;
    //% shim=funkaudio::recordedLength
    function recordedLength(): number;
    //% shim=funkaudio::maxLength
    function maxLength(): number;
    //% shim=funkaudio::setGain
    function _setGain(g: number): void;
    //% shim=funkaudio::getChunk
    function getChunk(offset: number, len: number): Buffer;
    //% shim=funkaudio::setChunk
    function setChunk(offset: number, buf: Buffer): void;
    //% shim=funkaudio::prepareReceive
    function prepareReceive(len: number): void;
    //% shim=funkaudio::playRecording
    function _play(): void;
    //% shim=funkaudio::isPlaying
    function _isPlaying(): boolean;
}