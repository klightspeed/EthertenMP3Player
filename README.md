Etherten MP3 player
===================

This uses the Etherten / Arduino Ethernet baseboard
with the Sparkfun MP3 shield, with a microsd card
in the MP3 shield's microsd slot.

The bootloader and application restart functions
rely on the AVR TFTP Bootloader at
https://github.com/klightspeed/ArduinoTFTPBootloader

Configuration
-------------

The program uses a configuration file 'config.txt' in
the root of the SD card.

It can contain the following options:

Option  | Meaning
------- | -------
mac     | Ethernet Hardware address
ip      | IP address
gw      | IP default gateway
netmask | IP netmask
mp3file | MP3 File name
volume  | MP3 volume attenuation (0 is loudest, 254 is quietest)
auth    | Base64-encoded authentication (username:password)
