import sys, os, time
import subprocess


class LinuxKermit:
    TMP_KSC = ".tmp.ksc"

    def __init__(self, line):
        self.line = line
        self.baud = 115200
        self.flowControl = "none"

        ret, stdout, stderr = self.exeCmd("which kermit")
        if not ret:
            self.kermitCmd = stdout.strip()
        else:
            raise Exception(
                "kermit command not found. Please install c-kermit package."
            )

    def tryConnect(self, baudrate, flowControl):
        self.baud = baudrate
        self.flowControl = flowControl
        # print "try %d %s"%(self.baud, self.flowControl)
        failMsg = None
        try:
            fd = os.open(self.line, os.O_WRONLY)
            os.close(fd)
        except Exception, e:
            failMsg = "Can not open %s: %s" % (self.line, str(e))
        if not failMsg and not self.verifyInUboot():
            failMsg = "Can not get uboot prompt on %s" % (self.line)
            # print failMsg
        return failMsg

    def setFlowControl(self, flowControl):
        print "set flow control: %s" % (flowControl)
        hwFlow = "on" if flowControl == "RTS/CTS" else "off"
        retCode, resp = self.runSingleCmd(
            "hwflow %s" % (hwFlow),
            timeout=1,
            resStr="hardware flow control: %s" % (hwFlow),
        )
        print resp
        if retCode == 0:
            self.flowControl = flowControl
        return retCode == 0

    def setBaudrate(self, baudrate):
        print "current baudrate = %d set baudrate to %d" % (self.baud, baudrate)
        kscLines = [
            "lineout setenv baudrate %d" % (baudrate),
            "input 2 \\fpattern(## Switch baudrate to*bps andpress ENTER ...)",
            "if fail exit 1",
            "exit 0",
        ]
        retCode, retString = self.runScript(kscLines, silence=True)
        if retCode == 0:
            self.baud = baudrate
            # time.sleep(1)
            return self.verifyInUboot()
        else:
            return False

    def verifyInUboot(self):
        kscLines = [
            "lineout",
            "lineout",
            "lineout test",
            "input 1 \\fpattern(#)",
            "if fail exit 1",
            "exit 0",
        ]
        retCode, retString = self.runScript(kscLines, silence=True)
        return retCode == 0

    def getinitCmds(self):
        return [
            "set line %s" % (self.line),
            "set speed %d" % (self.baud),
            "set carrier-watch off",
            "set handshake none",
            "set flow-control %s" % (self.flowControl),
            "set parity none",
            "set stop-bits 1",
            "set modem none",
            "robust",
            "set file type bin",
            "set file name lit",
            "set rec pack 448",
            "set send pack 448",
            "set window 5",
            "set input echo on",
        ]

    def close(self, initBaud, initFlowControl, resetUart=True):
        ret = True
        if resetUart and initBaud != self.baud:
            ret = self.setBaudrate(initBaud)
        if resetUart and initFlowControl != self.flowControl:
            ret = self.setFlowControl(initFlowControl)
        return ret

    def exeCmd(self, cmd, silence=True):
        # print cmd
        process = subprocess.Popen(
            cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT
        )
        # stdout, stderr = process.communicate()
        stdout = ""

        while True:
            out = process.stdout.read(1)
            if out == "" and process.poll() != None:
                break
            if out != "":
                if not silence:
                    sys.stdout.write(out)
                    sys.stdout.flush()
                stdout += out
        if not silence:
            print ""
        return process.returncode, stdout, process.stderr

    def runSingleCmd(self, cmd, timeout, resStr):
        kscLines = self.getinitCmds()
        kscLines.append("lineout %s" % (cmd))
        kscLines.append("input %d \\fpattern(%s)" % (timeout, resStr))
        kscLines.append("if fail exit 1")
        kscLines.append("exit 0")
        with open(LinuxKermit.TMP_KSC, "w") as f:
            f.write("\n".join(kscLines))
            f.write("\n")
        retCode, stdout, stderr = self.exeCmd(
            "%s %s" % (self.kermitCmd, LinuxKermit.TMP_KSC), True
        )
        if os.path.isfile(LinuxKermit.TMP_KSC):
            os.remove(LinuxKermit.TMP_KSC)
        return retCode, stdout

    def runScript(self, kscLines, silence=False):
        kscLines = self.getinitCmds() + kscLines
        with open(LinuxKermit.TMP_KSC, "w") as f:
            f.write("\n".join(kscLines))
            f.write("\n")
        retCode, stdout, stderr = self.exeCmd(
            "%s %s" % (self.kermitCmd, LinuxKermit.TMP_KSC), silence
        )
        retStr = stdout.split("\n")[-1] if retCode and len(stdout) > 0 else ""
        # print stdout.split('\n')[-1]
        if os.path.isfile(LinuxKermit.TMP_KSC):
            os.remove(LinuxKermit.TMP_KSC)
        return retCode, retStr
