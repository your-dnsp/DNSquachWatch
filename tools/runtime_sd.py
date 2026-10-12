"""Reviewed Arduino 2.0.14 SD compatibility adapter, generated build copy only."""
import hashlib
EXPECTED='aaaade8f55c0c33ab702d08eb1cc8a9bdbaab000eda95f2cfaef041f082c465b'
def patch(source):
    if hashlib.sha256(source.encode()).hexdigest()!=EXPECTED:
        raise RuntimeError('SD driver changed; compatibility adapter requires review')
    # SD now owns HSPI; software touch leaves it untouched. beginTransaction
    # restores clock/mode/order but not TFT_eSPI's USER transfer flags.
    # Restore full-duplex byte transfers under the bus lock on every operation.
    source='#include "soc/spi_reg.h"\n#include "soc/soc.h"\n'+source
    for settings in ('card->frequency', 'frequency'):
        original=f'card->spi->beginTransaction(SPISettings({settings}, MSBFIRST, SPI_MODE0));'
        assert source.count(original)==1
        source=source.replace(original, original+'\n        WRITE_PERI_REG(SPI_USER_REG(2), SPI_USR_MOSI | SPI_USR_MISO | SPI_DOUTDIN);',1)
    source=source.replace('''    digitalWrite(card->ssPin, HIGH);
}

bool sdSelectCard''','''    digitalWrite(card->ssPin, HIGH);
    card->spi->transfer(0xFF); // Clock the card's DO release with CS high.
}

bool sdSelectCard''',1)
    old='''    AcquireSPI card_locked(card, 400000);

    digitalWrite(card->ssPin, HIGH);
    for (uint8_t i = 0; i < 20; i++) {
        card->spi->transfer(0XFF);
    }

    // Fix mount issue - sdWait fail ignored before command GO_IDLE_STATE
    digitalWrite(card->ssPin, LOW);
    if(!sdWait(pdrv, 500)){
        log_w("sdWait fail ignored, card initialize continues");
    }
    if (sdCommand(pdrv, GO_IDLE_STATE, 0, NULL) != 1){
        sdDeselectCard(pdrv);
        log_w("GO_IDLE_STATE failed");
        goto unknown_card;
    }
    sdDeselectCard(pdrv);'''
    new='''    AcquireSPI card_locked(card, 100000);
    // A bounded reset exchange at the low end of the specified startup clock.
    // Never accept 0x00 as an idle response or continue to filesystem access.
    bool idle = false;
    for (unsigned attempt=0; attempt<5; ++attempt) {
        digitalWrite(card->ssPin, HIGH);
        delay(20);
        uint8_t released=0xff;
        for (unsigned i=0;i<32;++i) released=card->spi->transfer(0xff);
        digitalWrite(card->ssPin, LOW);
        // Wait for a selected card to release DO before sending a command.
        // CMD0 may still reset a card that never releases it, so the wait is
        // bounded and a timeout does not substitute for a valid idle response.
        uint8_t ready=0;
        uint32_t waitStart=millis();
        unsigned clocks=0;
        do {
            ready=card->spi->transfer(0xff);
            if(ready==0xff) break;
            // Continue clocking a powered card's pending data, yielding in
            // batches rather than limiting it to one byte per millisecond.
            if((++clocks & 63u)==0) delay(1);
        } while(uint32_t(millis()-waitStart)<500);
        uint8_t response=uint8_t(sdCommand(pdrv, GO_IDLE_STATE, 0, NULL));
        Serial.printf("[sd-compat] CMD0 attempt=%u idle=%02x selected=%02x response=%02x clock=100000\\n",attempt+1,released,ready,response);
        if(response!=1 && attempt==0) {
            // ESP32 RST leaves SD powered. An interrupted read can reject
            // CMD0 until CMD12 stops it. Do not send any write/data token.
            uint8_t stopped=uint8_t(sdCommand(pdrv, STOP_TRANSMISSION, 0, NULL));
            bool recovered=sdWait(pdrv,500);
            Serial.printf("[sd-compat] read-stop response=%02x ready=%u\\n",stopped,recovered?1u:0u);
        }
        sdDeselectCard(pdrv);
        if(response==1) { idle=true; break; }
    }
    if (!idle) goto unknown_card;'''
    assert source.count(old)==1
    source=source.replace(old,new,1)
    # CMD12 has an R1b response: consume its busy phase before reporting
    # a multi-block read complete or returning the shared bus to the display.
    old_stop="""            sdDeselectCard(pdrv);
            if (count == 0) {
                return true;
            }"""
    new_stop="""            bool stoppedReady = sdWait(pdrv, 500);
            sdDeselectCard(pdrv);
            if (count == 0 && stoppedReady) {
                return true;
            }
            if (!stoppedReady) return false;"""
    assert source.count(old_stop)==1
    source=source.replace(old_stop,new_stop,1)
    source=source.replace('0x40100000, NULL','0x40000000, NULL').replace('APP_OP_COND, 0x100000, NULL','APP_OP_COND, 0, NULL')
    source=source.replace('static ardu_sdcard_t* s_cards[FF_VOLUMES] = { NULL };','''static ardu_sdcard_t* s_cards[FF_VOLUMES] = { NULL };
static FATFS* dnsp_volumes[FF_VOLUMES] = { NULL };
extern "C" FATFS* dnsp_sd_volume(uint8_t drive) { return drive<FF_VOLUMES ? dnsp_volumes[drive] : nullptr; }
extern "C" bool dnsp_sd_cached_space(uint64_t* total, uint64_t* used) {
    for(unsigned i=0;i<FF_VOLUMES;++i) {
        FATFS* fs=dnsp_volumes[i];
        if(fs && fs->n_fatent>=2 && fs->free_clst<=fs->n_fatent-2) {
            *total=uint64_t(fs->csize)*(fs->n_fatent-2)*512;
            *used=uint64_t(fs->csize)*(fs->n_fatent-2-fs->free_clst)*512;
            return true;
        }
    }
    return false; // Unknown free count: never force a full FAT scan on the UI task.
}''',1)
    source=source.replace('uint8_t sdcard_unmount(uint8_t pdrv)\n{','uint8_t sdcard_unmount(uint8_t pdrv)\n{\n    if(pdrv<FF_VOLUMES) dnsp_volumes[pdrv]=nullptr;',1)
    source=source.replace('    card->sectors = sdGetSectorsCount(pdrv);\n    return true;', '    card->sectors = sdGetSectorsCount(pdrv);\n    dnsp_volumes[pdrv]=fs;\n    return true;',1)
    return source
