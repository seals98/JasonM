#!/usr/bin/env python3
"""Wrap a raw application .bin with the bootloader image header.

Usage: mkimage.py app.bin app_signed.bin [--version N] [--header-size 0x400]
The app must be linked at APP_SLOT_ADDR + header size (vector table first).
"""
import argparse, struct, zlib

MAGIC, HDR_VER = 0x50414D4A, 1

p = argparse.ArgumentParser()
p.add_argument("input"); p.add_argument("output")
p.add_argument("--version", type=lambda x: int(x, 0), default=0)
p.add_argument("--header-size", type=lambda x: int(x, 0), default=0x400)
a = p.parse_args()

body = open(a.input, "rb").read()
body += b"\xff" * (-len(body) % 4)
hdr = struct.pack("<8I", MAGIC, HDR_VER, len(body), zlib.crc32(body), a.version, 0, 0, 0)
hdr += b"\xff" * (a.header_size - len(hdr))
open(a.output, "wb").write(hdr + body)
print(f"{a.output}: {len(body)} byte app, crc32=0x{zlib.crc32(body):08x}")
