import os
import sys

DEFAULT_CMD_TIMEOUT = 3


class Kermit:
    def __init__(self, line, baud, flowControl="RTS_CTS"):
        self.baudList = [115200]
        self.initBaud = 115200
        self.initFlowControl = "none"
        self.newBaud = 115200
        self.currentBaud = 115200
        self.line = line
        self.baud = baud
        self.flowControl = flowControl

    def connect(self, retry=1):
        if sys.platform == "linux" or sys.platform == "linux2":
            from LinuxKermit import LinuxKermit

            flowControl = self.flowControl.replace("_", "/")
            self.kmWorker = LinuxKermit(self.line)
        elif sys.platform == "win32":
            from WinKermit import WinKermit

            flowControl = self.flowControl.replace("/", "_")
            self.kmWorker = WinKermit(self.line)
        else:
            raise Exception("We do not support %s" % (sys.platform))

        for b in self.baudList:
            failMsg = self.kmWorker.tryConnect(b, self.initFlowControl)
            if failMsg:
                continue
            # print failMsg
            if self.baud != b and not self.kmWorker.setBaudrate(self.baud):
                failMsg = "can not set baudrate to %d on %s" % (self.baud, self.line)
                break
            self.initBaud = b
            self.newBaud = self.currentBaud = self.baud
            if flowControl != "none" and not self.kmWorker.setFlowControl(flowControl):
                failMsg = "Cannot configure device flow contorl to %s" % (flowControl)
            break
        if failMsg:
            if retry >= 1:
                self.connect(retry - 1)
            else:
                raise Exception(failMsg)

    def runScript(self, kscLines, silence=False):
        return self.kmWorker.runScript(kscLines, silence)

    def close(self, resetUart=True):
        return self.kmWorker.close(self.initBaud, self.initFlowControl, resetUart)

    def runSingleCmd(self, cmd, timeout=DEFAULT_CMD_TIMEOUT, resStr="*#"):
        return self.kmWorker.runSingleCmd(cmd, timeout, resStr)

    def toggleBaudrate(self):
        print "init %d, new %d, current %d" % (
            self.initBaud,
            self.newBaud,
            self.currentBaud,
        )
        if self.initBaud == self.newBaud:
            return 0
        self.currentBaud = (
            self.initBaud if self.initBaud != self.currentBaud else self.newBaud
        )
        if not self.kmWorker.setBaudrate(self.currentBaud):
            print "can not set baudrate to %d on %s" % (baud, self.line)
            return -1
        return 0
