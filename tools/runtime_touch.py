"""Preserve pinned XPT2046 sampling; isolate classic-CYD touch from SD HSPI."""
PREAMBLE = r"""
// CYD touch: GPIO25 clock, GPIO32 output, GPIO39 input. Mode 0, MSB first.
// All calls remain in the main processing path; IRQ only sets library wake state.
static void dnsp_touch_begin() {
    digitalWrite(25, LOW); pinMode(25, OUTPUT);
    digitalWrite(32, LOW); pinMode(32, OUTPUT); pinMode(39, INPUT);
}
static uint8_t dnsp_touch_byte(uint8_t value) {
    uint8_t received=0;
    for(unsigned bit=0;bit<8;++bit) {
        digitalWrite(32, (value & 0x80u) ? HIGH : LOW); value <<= 1;
        delayMicroseconds(1); digitalWrite(25, HIGH); delayMicroseconds(1);
        received=uint8_t((received<<1) | (digitalRead(39) ? 1u : 0u));
        digitalWrite(25, LOW);
    }
    return received;
}
static uint16_t dnsp_touch_word(uint16_t value) {
    uint16_t high=dnsp_touch_byte(uint8_t(value>>8));
    return uint16_t((high<<8) | dnsp_touch_byte(uint8_t(value)));
}
"""
def patch(source):
    if source.count('_pspi->begin();')!=1 or source.count('_pspi->beginTransaction(SPI_SETTING);')!=1 or source.count('_pspi->endTransaction();')!=1:
        raise RuntimeError('Pinned XPT2046 software-SPI adapter requires review')
    source=source.replace('_pspi->begin();','dnsp_touch_begin();')
    source=source.replace('_pspi->beginTransaction(SPI_SETTING);','// Software SPI: no hardware controller transaction.')
    source=source.replace('_pspi->endTransaction();','// Software SPI transaction ended by CS high.')
    source=source.replace('_pspi->transfer16(', 'dnsp_touch_word(').replace('_pspi->transfer(', 'dnsp_touch_byte(')
    anchor='static XPT2046_Touchscreen'
    if source.count(anchor)!=1:raise RuntimeError('Pinned touch preamble anchor changed')
    return source.replace(anchor,PREAMBLE+'\n'+anchor,1)
