#!/usr/bin/env python2

from KermitLib.Kermit import Kermit
import argparse
from argparse import RawTextHelpFormatter
import sys, os
from flashmcu import genFlashProtectKsc

DEFAULT_BAUDRATE = 115200
optParser = argparse.ArgumentParser(
    description="This is a tool to erase flash partition of Altair 125X MCU",
    formatter_class=RawTextHelpFormatter,
)
optParser._action_groups.pop()

# ------------------------Required Arguments-----------------------------
requiredOpt = optParser.add_argument_group("required arguments")

requiredOpt.add_argument(
    "-L",
    action="store",
    dest="targetLine",
    required=True,
    help="Specify target device port",
)


def genEraseMcuKsc():
    kscLines = genFlashProtectKsc(False)
    kscLines += [
        "lineout erase mcu",
        "input 15 \\fpattern(Erased*sectors)",
        'if fail exit 1 "Failed to execute erase mcu"',
    ]
    kscLines += genFlashProtectKsc(True)
    kscLines.append("exit 0")
    return kscLines


if __name__ == "__main__":
    opts = optParser.parse_args()
    print "Trying to connect to device..."
    kermit = Kermit(opts.targetLine, DEFAULT_BAUDRATE, "none")
    kermit.connect()
    print "Connect to device successfully"
    retCode, retString = kermit.runScript(genEraseMcuKsc())
    kermit.close()
    if retCode:
        print "Failed", retString
        sys.exit(1)
    else:
        print "Done"
        sys.exit(0)
