#!/usr/bin/env python

# usage: bin2h.py <out> <in> <name>

import sys
with open(sys.argv[1], 'w') as fout:
  with open(sys.argv[2], 'rb') as fin:
    arr = ','.join([('0x%02x' % ord(b)) for b in fin.read()])
    fout.write('static const PROGMEM uint8_t {0}[{1}] = {{ {2} }};\n'.format(
      sys.argv[3],
      fin.tell(),
      arr
    ))

