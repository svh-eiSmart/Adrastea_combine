import sys, os
import re
import time
import struct
from ctypes import *


class MessageStruct(Structure):
    ARG1_LEN = 255
    ARG2_LEN = 1024
    _fields_ = [("filePath", c_char * ARG1_LEN), ("deviceName", c_char * ARG2_LEN)]


class MessageStruct_Uart(Structure):
    ARG2_LEN = 255
    ARG3_LEN = 1024
    ARG4_LEN = 1024
    _fields_ = [
        ("buadRate", c_int),
        ("filePath", c_char * ARG2_LEN),
        ("deviceName", c_char * ARG3_LEN),
        ("flowControl", c_char * ARG4_LEN),
    ]


class KermitModule:
    def __init__(self):
        curDir = os.path.dirname(__file__)
        dllName = (
            "KermitModule64.dll"
            if (struct.calcsize("P") * 8) == 64
            else "KermitModule.dll"
        )
        kermitDll = os.path.join(curDir, dllName)
        # print 'using KermitDLL %s'%(kermitDll)
        self.winDLL = CDLL(kermitDll)

    def SendFileToDev(self, strArg1, strArg2, progressCB, cancelCB):
        if (
            len(strArg1) > MessageStruct.ARG1_LEN
            or len(strArg2) > MessageStruct.ARG2_LEN
        ):
            raise Exception
        message = MessageStruct(strArg1, strArg2)
        return self.winDLL.SendFileToDev(byref(message), progressCB, cancelCB)

    def SendFileToDev_Uart(
        self, intArg1, strArg2, strArg3, strArg4, progressCB, cancelCB
    ):
        if (
            len(strArg2) > MessageStruct_Uart.ARG2_LEN
            or len(strArg2) > MessageStruct_Uart.ARG3_LEN
            or len(strArg4) > MessageStruct_Uart.ARG4_LEN
        ):
            raise Exception
        message = MessageStruct_Uart(c_int(intArg1), strArg2, strArg3, strArg4)
        return self.winDLL.SendFileToDev_Uart(byref(message), progressCB, cancelCB)

    def OpenDev(self, strArg1, strArg2):
        if (
            len(strArg1) > MessageStruct.ARG1_LEN
            or len(strArg2) > MessageStruct.ARG2_LEN
        ):
            raise Exception
        message = MessageStruct(strArg1, strArg2)
        return self.winDLL.OpenDev(byref(message))

    def OpenDev_Uart(self, intArg1, strArg2, strArg3, strArg4):
        if (
            len(strArg2) > MessageStruct_Uart.ARG2_LEN
            or len(strArg2) > MessageStruct_Uart.ARG3_LEN
            or len(strArg4) > MessageStruct_Uart.ARG4_LEN
        ):
            raise Exception
        message = MessageStruct_Uart(c_int(intArg1), strArg2, strArg3, strArg4)
        return self.winDLL.OpenDev_Uart(byref(message))

    def CloseDev(self):
        return self.winDLL.CloseDev()

    def IsComDevOpen(self):
        return self.winDLL.IsComDevOpen()

    def WriteDev(self, strArg1, strArg2):
        if (
            len(strArg1) > MessageStruct.ARG1_LEN
            or len(strArg2) > MessageStruct.ARG2_LEN
        ):
            raise Exception
        message = MessageStruct(strArg1, strArg2)
        return self.winDLL.WriteDev(byref(message))

    def WriteDev_Uart(self, intArg1, strArg2, strArg3, strArg4):
        if (
            len(strArg2) > MessageStruct_Uart.ARG2_LEN
            or len(strArg2) > MessageStruct_Uart.ARG3_LEN
            or len(strArg4) > MessageStruct_Uart.ARG4_LEN
        ):
            raise Exception
        message = MessageStruct_Uart(c_int(intArg1), strArg2, strArg3, strArg4)
        return self.winDLL.WriteDev_Uart(byref(message))

    def ReadLineDev(self, strArg1, intArg2, intArg3):
        return self.winDLL.ReadLineDev(strArg1, c_int(intArg2), c_int(intArg3))

    def ReadDev(self, strArg1, intArg2, intArg3, strArg4):
        return self.winDLL.ReadDev(
            strArg1, c_int(intArg2), c_int(intArg3), c_char_p(strArg4)
        )


class WinKermit:
    PROGRESS_CB = WINFUNCTYPE(None, c_int)
    CANCEL_CB = WINFUNCTYPE(c_bool)
    KERMIT_BUF_SIZE = 1024
    KERMIT_READ_DEF_TIMEOUT = 500

    def __init__(self, line):
        self.km = KermitModule()
        self.line = line
        self.lastCmdRes = 1
        self.baud = 115200
        self.flowControl = "none"

    def tryConnect(self, baudrate, flowControl, retry=1):
        self.baud = baudrate
        self.flowControl = flowControl
        # print "try %d %s"%(self.baud, self.flowControl)
        failMsg = None
        if not self.km.OpenDev_Uart(self.baud, "", self.line, self.flowControl):
            failMsg = "Can not open %s" % (self.line)
        elif not self.verifyInUboot():
            failMsg = "Can not get uboot prompt on %s" % (self.line)
            self.km.CloseDev()

        if retry >= 1 and failMsg:
            return self.tryConnect(baudrate, flowControl, retry - 1)
        else:
            return failMsg

    def setFlowControl(self, flowControl):
        print "set flow control: %s" % (flowControl)
        hwFlow = "on" if flowControl == "RTS_CTS" else "off"
        self.writeDevice("hwflow %s\r" % (hwFlow))
        self.writeDevice("\r")
        self.km.CloseDev()
        return self.tryConnect(self.baud, flowControl) == None

    def setBaudrate(self, baudrate):
        print "set baudrate to %d" % (baudrate)
        kscLines = [
            "lineout setenv baudrate %d" % (baudrate),
            "input 5 \\fpattern(## Switch baudrate to*bps andpress ENTER ...)",
            "if fail exit 1",
            "exit 0",
        ]
        retCode, retString = self.runScript(kscLines, silence=True)
        self.km.CloseDev()
        if retCode == 0:
            retCode = self.tryConnect(baudrate, self.flowControl) == None
        else:
            retCode = False
        return retCode

    def verifyInUboot(self):
        self.writeDevice("test\r")
        self.writeDevice("test\r")
        ret = self.readDevice("test", 200)
        # print ret
        if "#" in ret:
            return True
        else:
            return False

    def updateProgressCallback(self, percentage):
        # sys.stdout.write("\033[F") #back to previous line
        # sys.stdout.write("\033[K") #clear line
        sys.stdout.write("Progress %d%%\r" % (percentage))
        sys.stdout.flush()

    def cancelCallback(self):
        return False

    def writeDevice(self, cmd):
        if not self.km.IsComDevOpen():
            print "%s is not opened" % (self.line)
            return 0
        return self.km.WriteDev_Uart(0, "", cmd, "")

    def sendFile(self, file, prgCB=None, canCB=None):
        printDone = False
        if not self.km.IsComDevOpen():
            print "%s is not opened" % (self.line)
            return 0

        if not os.path.isfile(file):
            print "File not found: %s" % (file)
            return 0

        if not prgCB:
            print "WinKermit starts to send %s" % (file)
            printDone = True
            prgCB = WinKermit.PROGRESS_CB(self.updateProgressCallback)
        if not canCB:
            canCB = WinKermit.CANCEL_CB(self.cancelCallback)

        ret = self.km.SendFileToDev_Uart(0, file, "", "", prgCB, canCB)
        if printDone:
            print ""
            print "Finished" if ret > 0 else "Failed"
        return ret

    def readDevice(self, fPattern="", timeout=KERMIT_READ_DEF_TIMEOUT):
        rc = 0
        if not self.km.IsComDevOpen():
            print "%s is not opened" % (self.line)
            return ""
        line = create_string_buffer(WinKermit.KERMIT_BUF_SIZE)
        rc = self.km.ReadDev(line, WinKermit.KERMIT_BUF_SIZE, timeout, fPattern)
        return line.value if rc > 0 else ""

    def close(self, initBaud, initFlowControl, resetUart=True):
        if resetUart and initFlowControl != self.flowControl:
            self.setFlowControl(initFlowControl)
        if resetUart and initBaud != self.baud:
            self.setBaudrate(initBaud)
        return self.km.CloseDev()

    # TODO: Refine this.
    def simpleKscParser(self, line, silence):
        shouldExit = 0
        exitCode = 0
        exitString = ""
        cmd = line.split()[0]
        msg = line[len(cmd) :].strip()
        # print line.strip()
        if cmd.lower() == "output":
            self.lastCmdRes = self.writeDevice(msg)
        elif cmd.lower() == "lineout":
            self.lastCmdRes = self.writeDevice("%s\r" % (msg))
        elif cmd.lower() == "send":
            self.lastCmdRes = self.sendFile("%s" % (msg))
        elif cmd.lower() == "input":
            tokens = msg.split()
            if len(tokens) and tokens[0].isdigit():
                timeout = int(tokens[0]) * 1000
                fpattern = ""
                if len(tokens) > 1:
                    match = msg[len(tokens[0]) :].strip()
                    fpattern = re.search(r"\\fpattern\(.*\)", match).group(0)
                    if fpattern:
                        fpattern = fpattern.lstrip("\\fpattern(").rstrip(")")
                    else:
                        fpattern = match
                resStr = self.readDevice(fpattern.split("*")[0], timeout)
                if not silence:
                    print resStr
                # print fpattern
                self.lastCmdRes = 1 if fpattern.split("*")[0] in resStr else 0
            else:
                res = 0
        elif cmd.lower() == "if":
            cond = msg.split()[0]
            if cond.lower() == "fail" and self.lastCmdRes == 0:
                act = msg.lstrip(cond).strip()
                shouldExit, exitCode, exitString = self.simpleKscParser(act, silence)
            elif cond.lower == "success" and self.lastCmdRes == 1:
                act = msg.lstrip(cond).strip()
                shouldExit, exitCode, exitString = self.simpleKscParser(act, silence)
                None
        elif cmd.lower() == "exit":
            exitToken = msg.split()
            if len(exitToken) > 0 and exitToken[0].isdigit():
                exitCode = int(exitToken[0])
                if len(exitToken) > 1:
                    exitString = msg[len(exitToken[0]) :].strip()
            shouldExit = 1
        return shouldExit, exitCode, exitString

    def runSingleCmd(self, cmd, timeout, resStr):
        fpattern = resStr.split("*")[0]
        # print fpattern
        self.writeDevice("%s\r" % (cmd))
        resp = self.readDevice(fpattern, timeout * 1000)
        if len(fpattern):
            retCode = 0 if fpattern in resp else 1
        else:
            retCode = 0
        return retCode, resp

    def runScript(self, kscLines, silence=False):
        res = 1
        shouldExit = 0
        exitCode = 0
        for line in kscLines:
            if len(line.strip()):
                shouldExit, exitCode, exitString = self.simpleKscParser(line, silence)
                if shouldExit:
                    break
        return exitCode, exitString
