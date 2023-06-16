#!/usr/bin/env python2

from KermitLib.Kermit import Kermit
import argparse
from argparse import RawTextHelpFormatter
import sys, os
from collections import OrderedDict
import re


DEFAULT_FLASH_TIMEOUT = 120
MCU_ENV_SIZE = 16 * 1024
# DEFAULT_BAUDRATE = 921600
SUPPORT_BAUD_LIST = (
    [115200, 921600, 1843200, 2400000, 2800000, 3000000]
    if sys.platform == "win32"
    else [115200, 921600]
)
DEFAULT_BAUDRATE = SUPPORT_BAUD_LIST[-1]
DEFAULT_FLOWCONTROL = "RTS_CTS"

optParser = argparse.ArgumentParser(
    description="This is a tool to burn mcu IMG to Altair 125X device",
    formatter_class=RawTextHelpFormatter,
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
    help="Specify target device baudrate.\n"
    + "Supported: %s.\n" % (", ".join(str(x) for x in SUPPORT_BAUD_LIST))
    + "Default is %d." % (DEFAULT_BAUDRATE),
)

optionalOpt.add_argument(
    "-F",
    action="store",
    dest="flowcontrol",
    default=DEFAULT_FLOWCONTROL,
    type=str,
    help="Specify flow control setting.\n"
    + "Supported: RTS_CTS, none\n"
    + "Default is %s." % (DEFAULT_FLOWCONTROL),
)

optionalOpt.add_argument(
    "-R", "--reset", action="store_true", help="Reset device after flash done\n"
)

optionalOpt.add_argument(
    "-E", "--erase_env", action="store_true", help="Erase env area of MCU partition.\n"
)
# optParser.add_argument('-u', action='append_const', dest='flashList',
#        const='uboot',
#        help='Add different values to list')

# ------------------------Required Arguments-----------------------------
requiredOpt = optParser.add_argument_group("required arguments")

requiredOpt.add_argument(
    "-L",
    action="store",
    dest="targetLine",
    required=True,
    help="Specify target device port",
)

requiredOpt.add_argument("IMG", action="store", help="Specify mcu image path")


def genFlashProtectKsc(on):
    if not on:
        return [
            "lineout protect off mcu",
            "input 3 \\fpattern(UnProtect Flash)",
            'if fail exit 1 "Failed to execute protect off mcu"',
        ]
    else:
        return [
            "lineout protect on mcu",
            "input 3 \\fpattern(Protect Flash)",
            'if fail exit 1 "Failed to execute protect on mcu"',
        ]


def genBurnPartKsc(partName, img, timeout=DEFAULT_FLASH_TIMEOUT):
    return [
        "lineout loadb %s" % (partName),
        "input 3 \\fpattern(## Ready for binary (kermit) download*)",
        'if fail exit 1 "Failed waiting for U-Boot load prompt!"',
        "send %s" % (img),
        "input %d \\fpattern(## Start Addr*#)" % (timeout),
        'if fail exit 1 "Failed to get U-Boot console after image upload!"',
    ]


def genSetSkipEraseEndEnv(skip):
    if skip:
        return ["lineout setenv skip_erase_end y", "input 3 \\fpattern(#)"]
    else:
        return ["lineout setenv skip_erase_end n", "input 3 \\fpattern(#)"]


def genBurnMcuKsc(img, eraseEnv=False, origSkipEraseEnd=False):
    kscLines = []
    setSkipEraseEnd = not eraseEnv

    if setSkipEraseEnd != origSkipEraseEnd:
        kscLines += genSetSkipEraseEndEnv(setSkipEraseEnd)

    kscLines += genFlashProtectKsc(False)
    kscLines += genBurnPartKsc("mcu", img)
    kscLines += genFlashProtectKsc(True)

    if setSkipEraseEnd != origSkipEraseEnd:
        kscLines += genSetSkipEraseEndEnv(origSkipEraseEnd)

    kscLines.append("exit 0")
    return kscLines


def parseMTDInfo(infoStr):
    mtdDict = OrderedDict()
    for mtd in infoStr.split(","):
        name = mtd.split("(")[1].strip(")")
        size = mtd.split("(")[0]
        if size != "-":
            mtdDict[name] = (
                int(size[:-1]) * 1024 if size.endswith("m") else int(size[:-1]) * 1024
            )
        else:
            mtdDict[name] = -1
    return mtdDict


def genStartAddrFormSizeInfo(sizeDict, boardInfo):
    currentStart = int(boardInfo["flashstart"], 0)
    addrDict = OrderedDict()
    for part, size in sizeDict.items():
        addrDict[part] = currentStart
        if size > 0:
            currentStart += size
    return addrDict


if __name__ == "__main__":
    opts = optParser.parse_args()
    imgPath = os.path.abspath(opts.IMG)
    if not os.path.isfile(imgPath):
        raise Exception("MCU image file not found: %s" % (imgPath))

    if not opts.baudrate in SUPPORT_BAUD_LIST:
        raise Exception("Sorry, we do not support baudrate %d" % (opts.baudrate))

    if not opts.flowcontrol in ["RTS_CTS", "none"]:
        raise Exception(
            "Sorry, we do not support flow control setting %s" % (opts.flowcontrol)
        )

    print "Trying to connect to device..."
    kermit = Kermit(opts.targetLine, opts.baudrate, opts.flowcontrol)
    kermit.connect()
    print "Connect to device sucessfully"

    retCode, retString = kermit.runSingleCmd("bdinfo", 1, "#")
    if retCode:
        raise Exception("Cannot get bdinfo from device")
    bdinfo = retString.split("\r\n")[1:-1]
    boardInfo = {}
    for i in bdinfo:
        boardInfo[i.split("=")[0].strip()] = i.split("=")[1].strip()

    retCode, retString = kermit.runSingleCmd("printenv mtdparts", 3, "#")
    if retCode:
        raise Exception("Cannot get mtdparts from device")
    mtdParts = retString.split("\n")[1]
    mtdParts = re.sub(r"mtdparts=", "", mtdParts)
    mtdParts = re.sub(r".*:", "", mtdParts).strip()

    mtdInfoDict = parseMTDInfo(mtdParts)
    mtdStartDict = genStartAddrFormSizeInfo(mtdInfoDict, boardInfo)
    # count the size of last partition if '-' in mtdinfo
    if mtdInfoDict.values()[-1] == -1:
        mtdInfoDict[mtdInfoDict.keys()[-1]] = (
            int(boardInfo["flashstart"], 0) + int(boardInfo["flashsize"], 0)
        ) - mtdStartDict.values()[-1]

    # for part, size in mtdInfoDict.items():
    #    print "%s %x"%(part, size)

    # for part, start in mtdStartDict.items():
    #    print "%s %x"%(part, start)

    if not mtdInfoDict.has_key("mcu") or not mtdStartDict.has_key("mcu"):
        raise Exception("Cannot find mcu partition in mtdparts")

    mcuPartStart = mtdStartDict["mcu"]
    mcuPartSize = mtdInfoDict["mcu"]
    if not opts.erase_env:
        mcuPartSize -= MCU_ENV_SIZE

    if os.path.getsize(imgPath) > mcuPartSize:
        raise Exception(
            "MCU image size %d is larger than MCU partiton size %d"
            % (os.path.getsize(imgPath), mcuPartSize)
        )

    retCode, retString = kermit.runSingleCmd("printenv skip_erase_end", 1, "#")
    if not "Error" in retString and "=" in retString:
        origSkipEraseEnd = retString.split("=")[1][0] == "y"
    else:
        origSkipEraseEnd = False

    retCode, retString = kermit.runScript(
        genBurnMcuKsc(imgPath, opts.erase_env, origSkipEraseEnd)
    )
    if retCode:
        print "Failed", retString
        kermit.close()
        sys.exit(1)
    else:
        print "Done"
        if opts.reset:
            print "Reset device..."
            kermit.runSingleCmd("reset", timeout=1)
            kermit.close(resetUart=False)
        else:
            kermit.close()
        sys.exit(0)
