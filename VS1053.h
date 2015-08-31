#ifndef VS1053_H_
#define VS1053_H_

/*
 * Based on SFEMP3Shield.cpp from
 * https://github.com/madsci1016/Sparkfun-MP3-Player-Shield-Arduino-Library
 */

#include "ports.h"

enum VS1053_State {
    uninitialized,
    initialized,
    deactivated,
    loading,
    ready,
    playback,
    paused_playback,
};

enum VS1053_Flush {
    post,
    pre,
    both,
    none,
};

// SCI Registers
#define VS1053_SCI_MODE              0x00
#define VS1053_SCI_STATUS            0x01
#define VS1053_SCI_BASS              0x02
#define VS1053_SCI_CLOCKF            0x03
#define VS1053_SCI_DECODE_TIME       0x04
#define VS1053_SCI_AUDATA            0x05
#define VS1053_SCI_WRAM              0x06
#define VS1053_SCI_WRAMADDR          0x07
#define VS1053_SCI_HDAT0             0x08
#define VS1053_SCI_HDAT1             0x09
#define VS1053_SCI_AIADDR            0x0A
#define VS1053_SCI_VOL               0x0B
#define VS1053_SCI_AICTRL0           0x0C
#define VS1053_SCI_AICTRL1           0x0D
#define VS1053_SCI_AICTRL2           0x0E
#define VS1053_SCI_AICTRL3           0x0F

// SCI_MODE bits
#define VS1053_SM_DIFF             0x0001
#define VS1053_SM_LAYER12          0x0002
#define VS1053_SM_RESET            0x0004
#define VS1053_SM_CANCEL           0x0008
#define VS1053_SM_EARSPEAKER_LO    0x0010
#define VS1053_SM_TESTS            0x0020
#define VS1053_SM_STREAM           0x0040
#define VS1053_SM_EARSPEAKER_HI    0x0080
#define VS1053_SM_DACT             0x0100
#define VS1053_SM_SDIORD           0x0200
#define VS1053_SM_SDISHARE         0x0400
#define VS1053_SM_SDINEW           0x0800
#define VS1053_SM_ADPCM            0x1000
#define VS1053_SM_PAUSE            0x2000
#define VS1053_SM_LINE1            0x4000
#define VS1053_SM_CLK_RANGE        0x8000

// SCI_STATUS bits
#define VS1053_SS_VU_ENABLE        0x0200

// Register Addresses
#define VS1053_para_chipID_0       0x1E00
#define VS1053_para_chipID_1       0x1E01
#define VS1053_para_version        0x1E02
#define VS1053_para_config1        0x1E03
#define VS1053_para_playSpeed      0x1E04
#define VS1053_para_byteRate       0x1E05
#define VS1053_para_endFillByte    0x1E06
#define VS1053_para_MonoOutput     0x1E09
#define VS1053_para_positionMsec_0 0x1E27
#define VS1053_para_positionMsec_1 0x1E28
#define VS1053_para_resync         0x1E29

#define VS1053_XCS                 6
#define VS1053_XDCS                7
#define VS1053_DREQ                2
#define VS1053_RESET               8

union VS1053_BassTreble {
    uint16_t word;

    struct {
        uint8_t Bass_Freqlimit   : 4;
        uint8_t Bass_Amplitude   : 4;
        uint8_t Treble_Freqlimit : 4;
         int8_t Treble_Amplitude : 4;
    };
};

#define SET_BASS_TREBLE(prop,val,min,max) \
    do { \
        int16_t __val = (val); \
        if (__val < min) __val = min; \
        if (__val > max) __val = max; \
        union VS1053_BassTreble __basstreb; \
        __basstreb.word = register_read(VS1053_SCI_BASS); \
        __basstreb.prop = val; \
        register_write(VS1053_SCI_BASS, __basstreb.word); \
    } while (0)

class VS1053ReaderBase {
    public:
        virtual int read(uint8_t buf[], int len) { return 0; }
    protected:
        void *reader;
};

template <typename T, int F (uint8_t[], int, T&)>
class VS1053ReaderFunctor : VS1053ReaderBase {
    public:
        VS1053ReaderFunctor(T &instance) {
            reader = reinterpret_cast<void *>(&instance);
        }

        virtual int read(uint8_t buf[], int len) {
            return F(buf, len, getinstance());
        }

    private:
        T &getinstance() {
            return *reinterpret_cast<T *>(reader);
        }
};

// TReader should be a functor with the following prototype:
//
// class TReader {
//     public:
//         int operator()(uint8_t buf[], int len);
// }
template <typename TReader>
class VS1053Reader : public VS1053ReaderBase {
    public:
        VS1053Reader(TReader &reader) {
            reader = reinterpret_cast<void *>(&reader);
        }

        virtual int read(uint8_t buf[], int len) {
            return getreader()(buf, len);
        }

    private:
        TReader &getreader() {
            return *reinterpret_cast<TReader *>(reader);
        }
};

class VS1053Base {
    public:
        uint8_t begin();
        void end();
        uint8_t vs_init();

        uint8_t begin_play_track(VS1053ReaderBase &reader);
        uint8_t continue_play_track(VS1053ReaderBase &reader);
        void stoptrack();
        uint8_t apply_patch(VS1053ReaderBase &reader);

        VS1053_State get_state() const { return state; }
        bool isplaying() const { return (state == playback || state == paused_playback); }
        void set_volume(uint8_t left, uint8_t right) { register_write(VS1053_SCI_VOL, left, right); }
        void set_volume(uint8_t vol) { set_volume(vol, vol); }
        void set_volume(uint16_t vol) { register_write(VS1053_SCI_VOL, vol); }
        uint16_t get_treble_frequency() { return get_bass_treble().Treble_Freqlimit * 1000; }
        int8_t get_treble_amplitude() { return get_bass_treble().Treble_Amplitude; }
        uint16_t get_bass_frequency() { return get_bass_treble().Bass_Freqlimit * 10; }
        uint8_t get_bass_amplitude() { return get_bass_treble().Bass_Amplitude; }
        void set_treble_frequency(uint16_t freq) { SET_BASS_TREBLE(Treble_Freqlimit, freq / 1000, 1, 15); }
        void set_treble_amplitude(int8_t amp) { SET_BASS_TREBLE(Treble_Amplitude, amp, -8, 7); }
        void set_bass_frequency(uint16_t freq) { SET_BASS_TREBLE(Bass_Freqlimit, freq / 10, 2, 15); }
        void set_bass_amplitude(uint8_t amp) { SET_BASS_TREBLE(Bass_Amplitude, amp, 0, 15); }
        uint16_t get_play_speed() { return wram_read(VS1053_para_playSpeed); }
        void set_play_speed(uint16_t data) { wram_write(VS1053_para_playSpeed, data); }
        void pause_playback() { if (isplaying()) state = paused_playback; }
        void resume_playback() { if (isplaying()) state = playback; }
        uint16_t get_mode() { return register_read(VS1053_SCI_MODE); }
        void set_mode(uint16_t val) { register_write(VS1053_SCI_MODE, val); }
        void set_mode(uint16_t mask, uint8_t val) { set_mode((get_mode() & ~mask) | (val ? mask : 0)); }
        uint16_t get_position() { return register_read(VS1053_SCI_DECODE_TIME); }
        uint8_t get_vumeter() { return !!(register_read(VS1053_SCI_STATUS) & VS1053_SS_VU_ENABLE); }
        void set_vumeter(uint8_t enable) { register_write(VS1053_SCI_STATUS, (register_read(VS1053_SCI_STATUS) & ~VS1053_SS_VU_ENABLE) | (enable ? VS1053_SS_VU_ENABLE : 0)); }
        uint16_t get_vulevel() { return register_read(VS1053_SCI_AICTRL3); }
        uint8_t get_diff_output() { return !!(get_mode() & VS1053_SM_DIFF); }
        void set_diff_output(uint8_t val) { set_mode(VS1053_SM_DIFF, val); }
        uint8_t get_mono_mode() { return wram_read(VS1053_para_MonoOutput) & 1; }
        void set_mono_mode(uint8_t stereo) { wram_write(VS1053_para_MonoOutput, (wram_read(VS1053_para_MonoOutput) & 0xFFFE) | stereo ? 1 : 0); }

        uint8_t get_earspeaker() {
            uint16_t mode = get_mode();
            return ((mode & VS1053_SM_EARSPEAKER_LO) ? 1 : 0) |
                   ((mode & VS1053_SM_EARSPEAKER_HI) ? 2 : 0);
        }

        void set_earspeaker(uint8_t val) {
            uint16_t mode = get_mode() & ~(VS1053_SM_EARSPEAKER_LO | VS1053_SM_EARSPEAKER_HI);
            mode |= ((val & 1) ? VS1053_SM_EARSPEAKER_LO : 0) | ((val & 2) ? VS1053_SM_EARSPEAKER_HI : 0);
            set_mode(mode);
        }

        union VS1053_BassTreble get_bass_treble() {
            union VS1053_BassTreble basstreb;
            basstreb.word = register_read(VS1053_SCI_BASS);
            return basstreb;
        }

        template <typename TReader>
        uint8_t begin_play_track(TReader &reader) {
            VS1053Reader<TReader> vsreader(reader);
            return begin_play_track(vsreader);
        }

        template <typename T, int F (uint8_t[], int, T&)>
        uint8_t begin_play_track(T &instance) {
            VS1053ReaderFunctor<T, F> vsreader(instance);
            return begin_play_track(vsreader);
        }

        template <typename TReader>
        uint8_t continue_play_track(TReader &reader) {
            VS1053Reader<TReader> vsreader(&reader);
            return continue_play_track(vsreader);
        }

        template <typename T, int F (uint8_t[], int, T&)>
        uint8_t continue_play_track(T &instance) {
            VS1053ReaderFunctor<T, F> vsreader(instance);
            return continue_play_track(vsreader);
        }

        template <typename TReader>
        uint8_t apply_patch(TReader &reader) {
            VS1053Reader<TReader> vsreader(&reader);
            return apply_patch(vsreader);
        }

        template <typename T, int F (uint8_t[], int, T&)>
        uint8_t apply_patch(T &instance) {
            VS1053ReaderFunctor<T, F> vsreader(instance);
            return apply_patch(vsreader);
        }

    protected:
        void cs_assert() { PORT_CLR(VS1053_XCS); }
        void cs_deassert() { PORT_SET(VS1053_XCS); }
        void dcs_assert() { PORT_CLR(VS1053_XDCS); }
        void dcs_deassert() { PORT_SET(VS1053_XDCS); }
        void reset_assert() { PORT_CLR(VS1053_RESET); }
        void reset_deassert() { PORT_SET(VS1053_RESET); }
        uint8_t dreq() { return PIN_VAL(VS1053_DREQ); }

        void fillend(uint8_t fillbyte, int len);
        void flush_cancel(VS1053_Flush flushtype);
        void register_write(uint8_t reg, uint8_t hi, uint8_t lo);
        uint16_t register_read(uint8_t reg);
        void wram_write(uint16_t addr, uint16_t data);
        uint16_t wram_read(uint16_t addr);

        void register_write(uint8_t reg, uint16_t val) {
            union twobyte v;
            v.word = val;
            register_write(reg, v.byte[1], v.byte[0]);
        }

        VS1053_State state;
        uint8_t spi_divisor;
};

#endif /* VS1053_H_ */
