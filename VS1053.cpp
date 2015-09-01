#include "config.h"
#include <util/delay.h>
#include "spi.h"
#include "util.h"
#include "VS1053.h"

/*
 * Based on SFEMP3Shield.cpp from
 * https://github.com/madsci1016/Sparkfun-MP3-Player-Shield-Arduino-Library
 */

#define spi_setup() spi_spi0_setup_param(spi_param)
#define set_divisor(div) do { spi_param = spi_spi0_calcparam(div); } while (0)
#define spi_transfer(val) spi_spi0_transfer(val)
#define cs_assert() PORT_CLR(VS1053_XCS)
#define cs_deassert() PORT_SET(VS1053_XCS)
#define dcs_assert() PORT_CLR(VS1053_XDCS)
#define dcs_deassert() PORT_SET(VS1053_XDCS)
#define reset_assert() PORT_CLR(VS1053_RESET)
#define reset_deassert() PORT_SET(VS1053_RESET)
#define dreq() PIN_VAL(VS1053_DREQ)

uint8_t VS1053Base::begin() {
    DDR_IN(VS1053_DREQ);
    DDR_OUT(VS1053_XCS);
    DDR_OUT(VS1053_XDCS);
    DDR_OUT(VS1053_RESET);

    cs_deassert();
    dcs_deassert();
    reset_assert();

    state = initialized;

    uint8_t result = vs_init();

    if (result) {
        cs_deassert();
        dcs_deassert();
        reset_assert();
        state = deactivated;

        return result;
    }

    return 0;
}

void VS1053Base::end() {
    stoptrack();
    cs_deassert();
    dcs_deassert();
    reset_assert();
    state = deactivated;
}

uint8_t VS1053Base::vs_init() {
    _delay_ms(100);
    reset_assert();
    _delay_ms(100);
    reset_deassert();

    set_divisor(32);
    _delay_ms(10);

    uint16_t mp3mode = register_read(VS1053_SCI_MODE);

    if(mp3mode != (VS1053_SM_LINE1 | VS1053_SM_SDINEW)){
        return 4;
    }

    register_write(VS1053_SCI_CLOCKF, 0x6000);

    set_divisor(4);

    _delay_ms(10);

    uint16_t mp3clock = register_read(VS1053_SCI_CLOCKF);

    if (mp3clock != 0x6000) {
        return 5;
    }

    return 0;
}

uint8_t VS1053Base::apply_patch(VS1053ReaderBase &reader) {
    union twobyte val;
    union twobyte addr;
    union twobyte n;

    if (!PIN_VAL(VS1053_RESET)) {
        return 3;
    }

    if (isplaying()) {
        return 1;
    }

    if (!PIN_VAL(VS1053_RESET)) {
        return 3;
    }

    while(1) {
        //addr = Plugin[i++];
        if(!reader.read(addr.byte, 2)) break;
        //n = Plugin[i++];
        if(!reader.read(n.byte, 2)) break;
        if(n.word & 0x8000U) { /* RLE run, replicate n samples */
            n.word &= 0x7FFF;
            //val = Plugin[i++];
            if(!reader.read(val.byte, 2)) break;
            while(n.word--) {
                register_write(addr.word, val.word);
            }
        } else { /* Copy run, copy n samples */
            while(n.word--) {
                //val = Plugin[i++];
                if(!reader.read(val.byte, 2))   break;
                register_write(addr.word, val.word);
            }
        }
    }

    return 0;
}

uint8_t VS1053Base::begin_play_track(VS1053ReaderBase &reader) {
    if (!PIN_VAL(VS1053_RESET)) {
        return 3;
    }

    if (isplaying()) {
        return 1;
    }

    if (!PIN_VAL(VS1053_RESET)) {
        return 3;
    }

    if (state != playback) {
        state = playback;
        register_write(VS1053_SCI_DECODE_TIME, 0);
        _delay_ms(100);
    }

    return 0;
}

uint8_t VS1053Base::continue_play_track(VS1053ReaderBase &reader) {
    if (!isplaying()) {
        return 1;
    }

    if (!PIN_VAL(VS1053_RESET)) {
        return 3;
    }

    while (dreq()) {
        uint8_t buf[32];
        int len = reader.read(buf, 32);

        if (len == 0) {
            flush_cancel(post);
            return 1;
        }

        dcs_assert();

        spi_setup();

        for (uint8_t i = 0; i < len; i++) {
            spi_transfer(buf[i]);
        }

        dcs_deassert();
    }

    return 0;
}

void VS1053Base::fillend(uint8_t fillbyte, int len) {
    dcs_assert();

    for (int i = 0; i < len; i++) {
        while (!dreq());
        spi_transfer(fillbyte);
    }

    dcs_deassert();
}

void VS1053Base::flush_cancel(VS1053_Flush flushtype) {
    uint8_t fillbyte = (uint8_t)(wram_read(VS1053_para_endFillByte) & 0xFF);

    spi_setup();

    if (flushtype == post || flushtype == both) {
        fillend(fillbyte, 2052);
    }

    for (int j = 0; j < 64; j++) {
        register_write(VS1053_SCI_MODE, register_read(VS1053_SCI_MODE) | VS1053_SM_CANCEL);

        fillend(fillbyte, 32);

        uint16_t cancel = register_read(VS1053_SCI_MODE) & VS1053_SM_CANCEL;

        if (cancel == 0) {
            if (flushtype == pre || flushtype == both) {
                fillend(fillbyte, 2052);
                return;
            }
        }

        register_write(VS1053_SCI_MODE, register_read(VS1053_SCI_MODE) | VS1053_SM_RESET);
    }

    state = ready;
}

void VS1053Base::register_write(uint8_t reg, uint8_t hi, uint8_t lo) {
    if (PIN_VAL(VS1053_RESET)) {
        return;
    }

    while (!dreq());

    cs_assert();

    spi_setup();
    spi_transfer(0x02);
    spi_transfer(reg);
    spi_transfer(hi);
    spi_transfer(lo);

    while (!dreq());

    cs_deassert();
}

uint16_t VS1053Base::register_read(uint8_t reg) {
    union twobyte v;

    if (PIN_VAL(VS1053_RESET)) {
        return 0;
    }

    while (!dreq());

    cs_assert();

    spi_setup();
    spi_transfer(0x03);
    spi_transfer(reg);

    v.byte[1] = spi_transfer(0xFF);
    while (!dreq());
    v.byte[0] = spi_transfer(0xFF);
    while (!dreq());

    cs_deassert();

    return v.word;
}

void VS1053Base::wram_write(uint16_t addr, uint16_t data) {
    register_write(VS1053_SCI_WRAMADDR, addr);
    register_write(VS1053_SCI_WRAM, data);
}

uint16_t VS1053Base::wram_read(uint16_t addr) {
    uint16_t v1, v2;

    register_write(VS1053_SCI_WRAMADDR, addr);
    v1 = register_read(VS1053_SCI_WRAM);

    for (uint8_t i = 0; i < 4; i++) {
        register_write(VS1053_SCI_WRAMADDR, addr);
        v2 = register_read(VS1053_SCI_WRAM);

        if (v1 == v2) {
            return v1;
        }

        v1 = v2;
    }

    return v1;
}

