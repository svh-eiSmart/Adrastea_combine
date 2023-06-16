#!/usr/bin/env python

from __future__ import print_function
import sys
import os
import tempfile
from zlib import crc32
import getopt
import struct
import shutil

usage = """
usage: {}, [options] binfile [outfile]

optional arguments:
-h, --help      show this help message and exit
-c, --crc32     put crc32 checksum in flash payload header
-v, --verbose
"""


def to_word(integer):
    # python3: integer.to_bytes(4, byteorder='big')
    # https://stackoverflow.com/a/24003070/436813
    s = "%%0%dx" % (4 << 1) % integer
    return (s).decode("hex")[-4:]


def from_word(data):
    return struct.unpack("<I", data)[0]


def die(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)
    sys.exit(1)


def genfw(binpath):
    with open(binpath, "rb") as infile:
        infile.seek(0, 2)  # seek to the end
        filelen = infile.tell()

        partx_base = 0x08000000
        flash_base = 0xB0000000

        infile.seek(0)  # seek to the start

        _ = infile.read(4)  # stack pointer
        word = infile.read(4)  # reset handler
        addr = from_word(word)
        if partx_base == (addr & 0xFFF0000):
            partx_base = 0
        else:
            partx_base |= addr & 0xFFF0000
            partx_base += flash_base

        infile.seek(0)  # seek to the start

        # Test file size
        if filelen & 0x3 != 0:  # should be a multiple of word size
            print("file length not word-size align")

        outfile, filename = tempfile.mkstemp()

        # 4-word flash payload header

        hdrlen = 0x200

        # firmware entry point
        ep = partx_base + hdrlen
        os.write(outfile, to_word(ep))

        # firmware size
        os.write(outfile, to_word(hdrlen + filelen))

        # crc32
        os.write(outfile, bytearray.fromhex("00000000"))

        # signature
        os.write(outfile, bytearray.fromhex("f1e0f1e0"))

        # resource allocation table
        os.write(outfile, bytearray("\x00" * (0x200 - 16)))

        # firmware offset must be on a 128-word boundary

        # reverse byte order of each word

        word = infile.read(4)
        while word:
            if len(word) != 4:
                print("zero-padding for word-size align")
                word += "\x00" * (-len(word) % 4)
            word = word[::-1]  # reverse
            os.write(outfile, word)
            word = infile.read(4)

        os.close(outfile)

        return filename


def print_usage():
    print(usage.format(sys.argv[0]))
    sys.exit(2)


def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hcv", ["help", "cksum", "verbose"])
    except getopt.GetoptError as err:
        print_usage()

    cksum = False
    verbose = False

    for opt, arg in opts:
        if opt in ("-v", "--verbose"):
            verbose = True
        elif opt in ("-h", "--help"):
            print_usage()
        elif opt in ("-c", "--cksum"):
            cksum = True

    if not args:
        print_usage()

    # generate firmware
    fwpath = genfw(args[0])

    # calculate crc32 checksum
    if cksum:
        cksum = crc32("")
        with open(fwpath, "r") as f:
            while True:
                data = f.read()
                if not data:
                    break
                cksum = crc32(data, cksum)
        if cksum < 0:  # https://stackoverflow.com/a/30092291/436813
            cksum = cksum % (1 << 32)

        # fill in crc32 checksum
        with open(fwpath, "rb+") as binfile:
            binfile.seek(8)  # seek to the third word
            for b in to_word(cksum):
                binfile.write(b)

    newpath = args[0]
    if len(args) > 1:
        newpath = args[1]

    try:
        shutil.move(fwpath, newpath)
    except EnvironmentError as e:
        die(e)


if __name__ == "__main__":
    # execute only if run as a script
    main()
