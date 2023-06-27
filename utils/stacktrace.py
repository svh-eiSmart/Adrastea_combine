#!/usr/bin/python

'''
Data file format is:
SEARCH_TEXT_BEFORE_DUMP

Some data

REGISTERS_DELIM
Partial registers list as hex

STACK_DUMP_DELIM
Stack raw dump as hex

SEARCH_TEXT_DELIMITER
'''

import binascii
import os
import subprocess
import sys
import argparse
import re
from KermitLib.Kermit import Kermit
import jtagmcu
import pdb

REGISTERS_DELIM = "Registers:"
STACK_DUMP_DELIM = "Stack dump:"
SEARCH_TEXT_BEFORE_DUMP = "####@@@@"
SEARCH_TEXT_DELIMITER = "######@@"
GDB_DELIM_TEXT = "##$$"
NEW_LINE = "\n"
OPENOCD_PORT = 3336
TEMP_GDB_CMD_FILE = "temp_gdb_commands_123789456.txt"
TEMP_GDB_STACK_RAW_BIN = "temp_gdb_stack_raw_bin_123789456.txt"

MIPS_REGS_COUNT = 31  # 31 + pc
REG_HEX_SIZE = 8  # each word is 4 byte, each byte is 2 chars
STACK_POINTER_INDEX = 28  # r29 register

DEFAULT_BAUDRATE = 115200

optParser = argparse.ArgumentParser(
    description="This is a tool to probe and analysis the backtrace information of Altair 125X device. " +
    "Please install \"qemu-system-arm\" before using it."
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
    help="Specify the baudrate of u-boot Console",
)

optionalOpt.add_argument(
    "-L",
    action="store",
    dest="targetLine",
    default="COM9",
    help="Specify the serial port of u-boot Console",
)

optionalOpt.add_argument(
    "-OCD",
    action="store",
    dest="ocdbin",
    default="E:/tools/GNU MCU Eclipse/OpenOCD/0.10.0-13/bin/openocd.exe",
    help="Specify the executable of OpenOCD",
)

optionalOpt.add_argument(
    "-GDB",
    action="store",
    dest="gdbbin",
    default="C:/Program Files (x86)/GNU Tools Arm Embedded/7 2018-q2-update/bin/arm-none-eabi-gdb.exe",
    help="Specify the executable of GDB",
)

optionalOpt.add_argument(
    "-QEMU",
    action="store",
    dest="qemubin",
    default="C:/Program Files/qemu/qemu-system-arm.exe",
    help="Specify the executable of QEMU",
)


optionalOpt.add_argument(
    "-DBGR",
    action="store",
    dest="debugger",
    default="qemu",
    help="Specify the debugging equipement(qemu/jlink/stlink)",
)
# ------------------------Required Arguments-----------------------------
requiredOpt = optParser.add_argument_group("required arguments")
requiredOpt.add_argument(
    "-ELF",
    action="store",
    dest="elffile",
    required=True,
    default="examples/ALT125X/MiniConsole/Eclipse/ALT1250/MiniConsole.elf",
    help="Specify the target ELF file",
)

requiredOpt.add_argument(
    "-CORE",
    action="store",
    dest="corefile",
    required=True,
    default="./coredump.txt",
    help="Specify the target coredump file",
)

# file must be closed after use


def createTempCmdFile(debugger, debuggerbin, elfFile):
    try:
        cmdFile = open(TEMP_GDB_CMD_FILE, 'w+')
        # prepare gdb basic startup commands
        if debugger == "jlink" or debugger == "stlink":
            cmdFile.write("set remotetimeout unlimited\n")
            cmdFile.write("eval \"target remote | '" +
                          debuggerbin.replace("\\", "/") + "' \\\n")
            if debugger == "jlink":
                cmdFile.write(
                    "-f interface/jlink.cfg -c \'transport select jtag\'" + " \\\n")
            else:
                cmdFile.write(
                    "-f interface/stlink.cfg -c \'transport select hla_swd\'" + " \\\n")

            cmdFile.write("-f ./utils/ALT125x.cfg" + " \\\n")
            cmdFile.write("-c 'gdb_port pipe; init; reset init'\"\n")
        else:
            cmdFile.write("eval \"target remote | '" +
                          debuggerbin.replace("\\", "/") + "' \\\n")
            cmdFile.write("-machine mps2-an511 -cpu cortex-m3" + " \\\n")
            cmdFile.write("-kernel " + elfFile.replace("\\",
                                                       "/") + " -S -gdb stdio\"\n")
            cmdFile.write("load\n")

        return cmdFile
    except Exception as e:
        print "Unable to create temp commands file for gdb"
        print e
        exit(1)


def runGDB(gdbbin, elfFile, noprint=False):
    commandToRun = [gdbbin, "-x", TEMP_GDB_CMD_FILE, "--batch", elfFile]
    if not noprint:
        print commandToRun
    process = subprocess.Popen(commandToRun, shell=False, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE)
    process.wait()
    # get output from the process
    (stdoutdata, stderrdata) = process.communicate()
    return (stdoutdata, stderrdata)


def delFile(filePath):
    if os.path.isfile(filePath):
        os.remove(filePath)


def prepareScript(coreFile, elfFile, debugger, debuggerbin, noprint=False):
    with open(coreFile) as f:
        data = f.read()

    dataSplitted = data.strip().split(SEARCH_TEXT_BEFORE_DUMP)
    for dataDump in dataSplitted:
        # skip if doesn't include registers
        if not REGISTERS_DELIM in dataDump:
            continue

        # create temp commands file for gdb
        cmdFile = createTempCmdFile(debugger, debuggerbin, elfFile)
        if not noprint:
            print cmdFile

        # Prepare register and stack section
        regsAndStack = dataDump.split(REGISTERS_DELIM)[1].split(
            SEARCH_TEXT_DELIMITER)[0].strip()
        (regs, stack) = regsAndStack.split(STACK_DUMP_DELIM)
        # prepare stack to memory command
        stack = "".join(stack.split())  # clean all white-spaces in string
        with open(TEMP_GDB_STACK_RAW_BIN, 'wb+') as tempStackBin:
            tempStackBin.write(binascii.unhexlify(stack))

        # prepare set registers commands
        for regName in ["SP", "R0", "R1", "R2", "R3", "R12", "LR", "PC"]:
            regValue = re.search(
                regName + r'\s+(\b0x[0-9A-Fa-f]+\b)', regs).group(1)
            # Restore SP raw data
            if regName == "SP":
                cmd = ("restore " + TEMP_GDB_STACK_RAW_BIN +
                       " binary %s 0 %d") % (regValue, len(stack) / 2)
                cmdFile.write(cmd + NEW_LINE)

            cmd = "set $%s = %s" % (regName.lower(), regValue)
            cmdFile.write(cmd + NEW_LINE)

        cmdFile.write("flushreg" + NEW_LINE)
        cmdFile.write("echo " + GDB_DELIM_TEXT + NEW_LINE)
        cmdFile.write("bt" + NEW_LINE)
        cmdFile.close()


if __name__ == "__main__":
    opts = optParser.parse_args()
    if not os.path.isfile(opts.elffile):
        print "Can't find ELF at: " + opts.elffile
        exit(1)

    if not os.path.isfile(opts.corefile):
        print "Can't find core file at: " + opts.corefile
        exit(1)

    if not os.path.isfile(opts.gdbbin):
        print "Can't find 'gdb' program at: " + opts.gdbbin
        exit(1)

    if opts.debugger not in ["jlink", "stlink", "qemu"]:
        print "Invalid debugger: " + opts.debugger
        exit(1)

    if opts.debugger != "qemu":
        if not os.path.isfile(opts.ocdbin):
            print "Can't find 'openocd' program at: " + opts.ocdbin
            exit(1)

        print "Trying to connect to device..."
        kermit = Kermit(opts.targetLine, opts.baudrate, "none")
        kermit.connect()
        print "Connect to device sucessfully"
        kscLines = jtagmcu.genJtagKsc(False)
        retCode, retString = kermit.runScript(kscLines, silence=True)
        kermit.close()
        if retCode:
            print "Failed", retString
            exit(1)
        else:
            print "Done"

        prepareScript(opts.corefile, opts.elffile,
                      opts.debugger, opts.ocdbin, noprint=False)
    else:
        if not os.path.isfile(opts.qemubin):
            print "Can't find 'qemu' program at: " + opts.qemubin
            exit(1)
        prepareScript(opts.corefile, opts.elffile,
                      opts.debugger, opts.qemubin, noprint=False)

    print "Please wait while getting stack trace"
    (gdbPrints, _) = runGDB(opts.gdbbin, opts.elffile, noprint=False)
    if GDB_DELIM_TEXT in gdbPrints:
        print gdbPrints.split(GDB_DELIM_TEXT)[1].strip()
    else:
        print "Error running gdb"
        exit(1)
