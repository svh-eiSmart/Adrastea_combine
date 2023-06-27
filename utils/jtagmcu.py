#!/usr/bin/env python2

from KermitLib.Kermit import Kermit
import argparse
import sys, os

DEFAULT_BAUDRATE = 115200

optParser = argparse.ArgumentParser(
    description="This is a tool to enable JTAG on mcu core of Altair 125X device"
)
optParser._action_groups.pop()

# ------------------------Optional Arguments-----------------------------
optionalOpt = optParser.add_argument_group("optional agruments")

optionalOpt.add_argument(
    "-B",
    action="store",
    dest="baudrate",
    default=DEFAULT_BAUDRATE,
    type=int,
    help="Specify target device baudrate",
)

optionalOpt.add_argument(
    "-r",
    action="store_true",
    dest="retention",
    default=False,
    help="Enable loading image to retention memory via J-TAG",
)

# ------------------------Required Arguments-----------------------------
requiredOpt = optParser.add_argument_group("required arguments")

requiredOpt.add_argument(
    "-L",
    action="store",
    dest="targetLine",
    required=True,
    help="Specify target device port",
)


def genJtagKsc(retention=False):
    kscLines = []
    bootCmds = ["mcu_jtag", "boot"]
    for c in bootCmds:
        kscLines.append("lineout %s" % c)

    kscLines.append("input 1 \\fpattern(Version)")
    kscLines.append('if fail exit 1 "Failed to execute boot command!"')
    kscLines.append("exit 0")
    # print '\n'.join(kscLines)
    return kscLines


if __name__ == "__main__":
    opts = optParser.parse_args()
    print "Trying to connect to device..."
    kermit = Kermit(opts.targetLine, opts.baudrate, "none")
    kermit.connect()
    print "Connect to device sucessfully"
    # print opts.retention
    kscLines = genJtagKsc(opts.retention)
    retCode, retString = kermit.runScript(kscLines, silence=True)
    kermit.close()
    if retCode:
        print "Failed", retString
        sys.exit(1)
    else:
        print "Done"
        sys.exit(0)
