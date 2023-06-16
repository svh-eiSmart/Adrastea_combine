#!/usr/bin/env python2

from KermitLib.Kermit import Kermit
import sys, os
import flashmcu
import jtagmcu

if __name__ == "__main__":
    opts = flashmcu.optParser.parse_args()
    imgPath = os.path.abspath(opts.IMG)
    if not os.path.isfile(imgPath):
        raise Exception("MCU image file not found: %s" % (imgPath))

    if not opts.baudrate in flashmcu.SUPPORT_BAUD_LIST:
        raise Exception("Sorry, we do not support baudrate %d" % (opts.baudrate))

    print "Trying to connect to device..."
    kermit = Kermit(opts.targetLine, opts.baudrate, opts.flowcontrol)
    kermit.connect()
    print "Connect to device successfully"

    retCode, retString = kermit.runSingleCmd("printenv skip_erase_end", 1, "#")
    if not "Error" in retString and "=" in retString:
        origSkipEraseEnd = retString.split("=")[1][0] == "y"
    else:
        origSkipEraseEnd = False

    cmdList = [
        [
            "Burn image",
            flashmcu.genBurnMcuKsc(imgPath, opts.erase_env, origSkipEraseEnd),
            True,
        ],
        ["Enable JTAG", jtagmcu.genJtagKsc(), False],
    ]
    for cmd in cmdList:
        print cmd[0]
        retCode, retString = kermit.runScript(cmd[1])
        if cmd[2]:
            retCode |= kermit.toggleBaudrate()
        if retCode:
            break

    kermit.close()
    if retCode:
        print "Failed", retString
        sys.exit(1)
    else:
        print "Done"
        sys.exit(0)
