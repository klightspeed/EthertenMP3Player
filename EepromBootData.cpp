#include <Arduino.h>
#include <EepromBootData.h>
#include <avr/eeprom.h>


EepromBootData::EepromBootData() {
    uint8_t *data = reinterpret_cast<uint8_t *>(this);
    for (int i = 0; i < sizeof(struct EepromBootData); i++) {
	data[i] = eeprom_read_byte((uint8_t *)(0x380 + i));
    }
}

void EepromBootData::update() {
    uint8_t *data = reinterpret_cast<uint8_t *>(this);
    for (int i = 0; i < sizeof(struct EepromBootData); i++) {
	if (eeprom_read_byte((uint8_t *)(0x380 + i)) != data[i]) {
            eeprom_write_byte((uint8_t *)(0x380 + i), data[i]);
	}
    }
}

