#!/usr/bin/env python2

import sys
import getopt
import serial
from serial import SerialException
import base64
import binascii
import string
import os
import time
from Tkinter import Tk, Label, Entry, Radiobutton, IntVar
import tkFileDialog

TOOL_NAME = "ALT FW Upgrade (AT) tool"
TOOL_VER = "1.0"
MAX_CHUNK_SZ = 3000
INTERM_SIZE = 256
# critical 0; error 1; warn 2; info 3; debug 4
LOG_LV = 4
RK_VER_T = 3
USE_REPLUG_DETECT = "n"
DUMMY_INTERM_STR = "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000A66359F1"

BAUDRATES = (
    38400,
    57600,
    115200,
    230400,
    460800,
    500000,
    576000,
    921600,
    1000000,
    1152000,
    1500000,
    2000000,
    2500000,
    3000000,
)

ERROR = b"\r\nERROR\r\n"
OK = b"\r\nOK\r\n"
PROMPT = b"\r\n>"

# fmt: off
crctab = [
    0x00000000,     0x04C11DB7,     0x09823B6E,     0x0D4326D9,     0x130476DC,
    0x17C56B6B,     0x1A864DB2,     0x1E475005,     0x2608EDB8,     0x22C9F00F,
    0x2F8AD6D6,     0x2B4BCB61,     0x350C9B64,     0x31CD86D3,     0x3C8EA00A,
    0x384FBDBD,     0x4C11DB70,     0x48D0C6C7,     0x4593E01E,     0x4152FDA9,
    0x5F15ADAC,     0x5BD4B01B,     0x569796C2,     0x52568B75,     0x6A1936C8,
    0x6ED82B7F,     0x639B0DA6,     0x675A1011,     0x791D4014,     0x7DDC5DA3,
    0x709F7B7A,     0x745E66CD,     0x9823B6E0,     0x9CE2AB57,     0x91A18D8E,
    0x95609039,     0x8B27C03C,     0x8FE6DD8B,     0x82A5FB52,     0x8664E6E5,
    0xBE2B5B58,     0xBAEA46EF,     0xB7A96036,     0xB3687D81,     0xAD2F2D84,
    0xA9EE3033,     0xA4AD16EA,     0xA06C0B5D,     0xD4326D90,     0xD0F37027,
    0xDDB056FE,     0xD9714B49,     0xC7361B4C,     0xC3F706FB,     0xCEB42022,
    0xCA753D95,     0xF23A8028,     0xF6FB9D9F,     0xFBB8BB46,     0xFF79A6F1,
    0xE13EF6F4,     0xE5FFEB43,     0xE8BCCD9A,     0xEC7DD02D,     0x34867077,
    0x30476DC0,     0x3D044B19,     0x39C556AE,     0x278206AB,     0x23431B1C,
    0x2E003DC5,     0x2AC12072,     0x128E9DCF,     0x164F8078,     0x1B0CA6A1,
    0x1FCDBB16,     0x018AEB13,     0x054BF6A4,     0x0808D07D,     0x0CC9CDCA,
    0x7897AB07,     0x7C56B6B0,     0x71159069,     0x75D48DDE,     0x6B93DDDB,
    0x6F52C06C,     0x6211E6B5,     0x66D0FB02,     0x5E9F46BF,     0x5A5E5B08,
    0x571D7DD1,     0x53DC6066,     0x4D9B3063,     0x495A2DD4,     0x44190B0D,
    0x40D816BA,     0xACA5C697,     0xA864DB20,     0xA527FDF9,     0xA1E6E04E,
    0xBFA1B04B,     0xBB60ADFC,     0xB6238B25,     0xB2E29692,     0x8AAD2B2F,
    0x8E6C3698,     0x832F1041,     0x87EE0DF6,     0x99A95DF3,     0x9D684044,
    0x902B669D,     0x94EA7B2A,     0xE0B41DE7,     0xE4750050,     0xE9362689,
    0xEDF73B3E,     0xF3B06B3B,     0xF771768C,     0xFA325055,     0xFEF34DE2,
    0xC6BCF05F,     0xC27DEDE8,     0xCF3ECB31,     0xCBFFD686,     0xD5B88683,
    0xD1799B34,     0xDC3ABDED,     0xD8FBA05A,     0x690CE0EE,     0x6DCDFD59,
    0x608EDB80,     0x644FC637,     0x7A089632,     0x7EC98B85,     0x738AAD5C,
    0x774BB0EB,     0x4F040D56,     0x4BC510E1,     0x46863638,     0x42472B8F,
    0x5C007B8A,     0x58C1663D,     0x558240E4,     0x51435D53,     0x251D3B9E,
    0x21DC2629,     0x2C9F00F0,     0x285E1D47,     0x36194D42,     0x32D850F5,
    0x3F9B762C,     0x3B5A6B9B,     0x0315D626,     0x07D4CB91,     0x0A97ED48,
    0x0E56F0FF,     0x1011A0FA,     0x14D0BD4D,     0x19939B94,     0x1D528623,
    0xF12F560E,     0xF5EE4BB9,     0xF8AD6D60,     0xFC6C70D7,     0xE22B20D2,
    0xE6EA3D65,     0xEBA91BBC,     0xEF68060B,     0xD727BBB6,     0xD3E6A601,
    0xDEA580D8,     0xDA649D6F,     0xC423CD6A,     0xC0E2D0DD,     0xCDA1F604,
    0xC960EBB3,     0xBD3E8D7E,     0xB9FF90C9,     0xB4BCB610,     0xB07DABA7,
    0xAE3AFBA2,     0xAAFBE615,     0xA7B8C0CC,     0xA379DD7B,     0x9B3660C6,
    0x9FF77D71,     0x92B45BA8,     0x9675461F,     0x8832161A,     0x8CF30BAD,
    0x81B02D74,     0x857130C3,     0x5D8A9099,     0x594B8D2E,     0x5408ABF7,
    0x50C9B640,     0x4E8EE645,     0x4A4FFBF2,     0x470CDD2B,     0x43CDC09C,
    0x7B827D21,     0x7F436096,     0x7200464F,     0x76C15BF8,     0x68860BFD,
    0x6C47164A,     0x61043093,     0x65C52D24,     0x119B4BE9,     0x155A565E,
    0x18197087,     0x1CD86D30,     0x029F3D35,     0x065E2082,     0x0B1D065B,
    0x0FDC1BEC,     0x3793A651,     0x3352BBE6,     0x3E119D3F,     0x3AD08088,
    0x2497D08D,     0x2056CD3A,     0x2D15EBE3,     0x29D4F654,     0xC5A92679,
    0xC1683BCE,     0xCC2B1D17,     0xC8EA00A0,     0xD6AD50A5,     0xD26C4D12,
    0xDF2F6BCB,     0xDBEE767C,     0xE3A1CBC1,     0xE760D676,     0xEA23F0AF,
    0xEEE2ED18,     0xF0A5BD1D,     0xF464A0AA,     0xF9278673,     0xFDE69BC4,
    0x89B8FD09,     0x8D79E0BE,     0x803AC667,     0x84FBDBD0,     0x9ABC8BD5,
    0x9E7D9662,     0x933EB0BB,     0x97FFAD0C,     0xAFB010B1,     0xAB710D06,
    0xA6322BDF,     0xA2F33668,     0xBCB4666D,     0xB8757BDA,     0xB5365D03,
    0xB1F740B4,
]
# fmt: on

UNSIGNED = lambda n: n & 0xFFFFFFFF


def DPRINT(severity, type, msg):
    global LOG_LV
    if severity <= LOG_LV:
        if severity == 0:
            print ("\n!!!FErr:" + msg)
        elif severity == 1:
            print ("\n !! Err:" + msg)
        elif severity == 2:
            print ("  !Warn:" + msg)
        elif severity == 3:
            print ("   Inf>> " + msg)
        else:
            print ("    Dbg> " + msg)


def memcrc(b):
    n = len(b)
    i = c = s = 0
    for ch in b:
        c = ord(ch)
        tabidx = (s >> 24) ^ c
        s = UNSIGNED((s << 8)) ^ crctab[tabidx]

    while n:
        c = n & 0o0377
        n = n >> 8
        s = UNSIGNED(s << 8) ^ crctab[(s >> 24) ^ c]
    return UNSIGNED(~s)


def send_to_serial_cli(tty, send_buf, comp):
    global OK
    global ERROR

    # send
    DPRINT(4, 0, send_buf)
    tx = bytearray(send_buf.encode("ascii"))
    tx.append(0x0D)
    tty.write(tx)

    rx = bytearray()

    if comp != "0":
        while True:
            bytes = tty.read()
            if bytes:
                for b in bytes:
                    rx.append(b)
                if rx.endswith(PROMPT):
                    return rx
            else:
                break

    time.sleep(1)
    return 0


def send_to_serial(tty, send_buf, retry):
    global OK
    global ERROR
    run_num = 0

    while run_num < retry + 1:
        # send
        DPRINT(4, 0, send_buf)
        tx = bytearray(send_buf.encode("ascii"))
        tx.append(0x0D)
        tty.write(tx)

        rx = bytearray()

        while True:
            bytes = tty.read()
            if bytes:
                for b in bytes:
                    rx.append(b)
                if rx.endswith(OK):
                    break
            else:
                break

        if rx.endswith(OK):
            return rx

        run_num = run_num + 1
        if run_num < retry + 1 and send_buf != "AT":
            DPRINT(2, 0, "retried: " + str(run_num))
    return 0


def re_gen_tmpfs(tty, interim_map_str):
    send_buf = 'AT%UPGCMD="CFGPART","' + interim_map_str + '"'

    result = send_to_serial(tty, send_buf, 1)
    if result == 0:
        DPRINT(4, 0, "Failed to send command " + send_buf)
        DPRINT(1, 0, "wrong delta image!")
    return result


def sendfw(tty, chunksize, lfile):
    try:
        statinfo = os.stat(lfile)
        filesize = statinfo.st_size
    except IOError as e:
        print ("!!!Error - I/O error({0}): {1}".format(e.errno, e.strerror))
        return 0
    except ValueError:
        print ("!!!Error - Could not convert data to an integer.")
        return 0
    except:
        print ("!!!Error - Unexpected error:", sys.exc_info()[0])
        print ("such as local file " + lfile + " may not exist!")
        return 0

    if filesize <= INTERM_SIZE:
        DPRINT(1, 0, "File size" + filesize + "is too small!")

    # handle interim map
    with open(lfile, "rb") as sent_file:
        sent_file.seek(0, 0)
        sent_buf = sent_file.read(INTERM_SIZE)

    out_hexstr = binascii.hexlify(sent_buf).decode("utf-8")
    # print out_hexstr
    res = re_gen_tmpfs(tty, out_hexstr)
    if res == 0:
        return -1

    result = 1
    offset = INTERM_SIZE
    nextpos = offset
    send_size = int(chunksize / 2)
    total_size = filesize - INTERM_SIZE

    if total_size < 2:
        DPRINT(1, 0, "wrong file size!")
        return 0

    while offset < filesize:
        # adjust offset
        nextpos += send_size
        if nextpos > filesize:
            send_size = filesize - offset

        # prepare send buffer
        with open(lfile, "rb") as sent_file:
            sent_file.seek(offset, 0)
            sent_buf = sent_file.read(send_size)

        # encode
        out_hexstr = binascii.hexlify(sent_buf).decode("utf-8")
        display_sz = send_size * 2

        # prepare AT command
        if nextpos >= filesize:  # no more
            send_buf = (
                'AT%FILEDATA="WRITE",0'
                + ","
                + str(display_sz)
                + ',"'
                + (out_hexstr)
                + '"'
            )
        else:
            send_buf = (
                'AT%FILEDATA="WRITE",1'
                + ","
                + str(display_sz)
                + ',"'
                + (out_hexstr)
                + '"'
            )
        offset += send_size

        sent_size = offset - INTERM_SIZE
        percent = sent_size * 100 / total_size
        DPRINT(3, 0, str(sent_size) + " bytes are sent. (" + str(percent) + "%)")

        result = send_to_serial(tty, send_buf, 3)

        if result == 0:
            DPRINT(1, 0, "failed to WRITE data!")
            break

    sent_file.close()
    return result


def disable_sleep(tty):
    print ("----->[ Disable PM sleep ]")
    send_buf = "pm-sleep disable"
    result = send_to_serial_cli(tty, send_buf, "PM Sleep")
    if result.find("PM Sleep") != (-1):
        return 1
    else:
        return 0


def sw_cli_to_at_channel(tty):
    print ("----->[ Switch CLI to AT port channel ]")
    send_buf = "serialSwitch"
    send_to_serial_cli(tty, send_buf, "0")


def check_upgrade_result(tty):
    print ("----->[ Checking Upgrade result ]")
    send_buf = "printenv upgrade"
    result = send_to_serial_cli(tty, send_buf, "upgrade=")
    if result.find("upgrade=ok") != (-1):
        print "Upgrade successfully"
    else:
        DPRINT(1, 0, "failed to upgrade. -" + result)


def check_at_channel(tty, retries):
    print ("----->[ Waiting for AT port ready ]")
    send_buf = "AT"
    result = send_to_serial(tty, send_buf, retries)
    if result == 0:
        DPRINT(
            1, 0, "failed to connect to AT port -" + send_buf + ",retries :" + retries
        )
        sys.exit()


def get_file_mode(tty):
    send_buf = "AT%GETACFG=admin.services.file_cmd_full_access"
    result = send_to_serial(tty, send_buf, 3)
    if result == 0:
        DPRINT(4, 0, "Your device SW version is older.")
        return 2
    elif result.find("enable") != (-1):
        DPRINT(3, 0, "debug mode")
        return 1
    else:
        DPRINT(3, 0, "commercial mode")
        return 0


def chk_dev_cfg(tty):
    send_buf = "AT%GETACFG=filemgr.file.put_fcksum"
    result = send_to_serial(tty, send_buf, 3)
    if result == 0:
        DPRINT(2, 0, "CFG/failed to get CRC value")
        return 0
    elif result.find("-1") != (-1):
        DPRINT(2, 0, "CFG/not do CRC value")
        return 0
    else:
        return 1


def get_file_chk_ability(tty):
    send_buf = "AT%GETACFG=filemgr.file.put_fcksum"
    result = send_to_serial(tty, send_buf, 3)
    if result == 0:
        DPRINT(4, 0, "CFG/failed to get CRC value")
        return 0
    else:
        return 1


def config_file_param_v1(tty, rfile):
    send_buf = 'AT%FILECMD="PUT","' + str(rfile) + '",1'
    result = send_to_serial(tty, send_buf, 3)
    if result == 0:
        DPRINT(1, 0, "failed to configure file -" + rfile)
    return result


def config_file_param(tty, rfile, lfile):
    try:
        statinfo = os.stat(lfile)
        filesize = statinfo.st_size
        img_sz = filesize - INTERM_SIZE
        with open(lfile, "rb") as sent_file:
            sent_file.seek(INTERM_SIZE, 0)
            csbuffer = sent_file.read(img_sz)
    except:
        DPRINT(1, 0, "No such file or directory : " + lfile)
        return 0

    cksum = memcrc(csbuffer)
    DPRINT(4, 0, "cksum: %d, len: %d, file: %s" % (cksum, len(csbuffer), lfile))
    send_buf = (
        'AT%FILECMD="PUT","'
        + str(rfile)
        + '",1,'
        + str(len(csbuffer))
        + ',"'
        + str(cksum)
        + '"'
    )

    result = send_to_serial(tty, send_buf, 3)
    if result == 0:
        DPRINT(1, 0, "failed to configure file -" + rfile)

    return result


def configure_at_echo(tty, set_val):
    send_buf = "ATE" + set_val
    result = send_to_serial(tty, send_buf, 3)
    if result == 0:
        DPRINT(2, 0, "failed to " + send_buf)


def send_atz(tty):
    send_buf = "ATZ"
    result = send_to_serial(tty, send_buf, 3)
    if result == 0:
        DPRINT(1, 0, "failed to send ATZ (reset device)")
        sys.exit()


def config_upg_param(tty, lfile):
    send_buf = 'AT%UPGCMD="UPGVRM","' + lfile + '"'
    result = send_to_serial(tty, send_buf, 3)
    if result == 0:
        DPRINT(1, 0, "failed to " + send_buf)
        return 0
    return 1


def print_sw_ver(tty):
    send_buf = "at AT AT%VER"
    result = send_to_serial(tty, send_buf, 5)

    if str(result).find("NP Package:") != (-1):
        print ("\n\n===================== Finish =====================")
        print ("----------------- Device version -----------------")
        print result
        print ("\n==================================================")
        return 1
    else:
        DPRINT(2, 0, "failed to get device version!")
        return 0


# Function: fsend_resp_chk
# return: allow_to_upgrade 0:no; 1:yes
def fsend_resp_chk(retv, rsp):
    allow_to_upgrade = 0

    if rsp != 0:
        if str(rsp).find("%FILEDATA:") != (-1):
            if str(rsp).find(",") != (-1):
                ret = str(rsp).split(",")[1][0]
                if retv == 1 and ret == "0":
                    DPRINT(3, 0, "Success to CRC checking!")
                    allow_to_upgrade = 1
                elif retv == 1 and ret == "1":
                    DPRINT(1, 0, "Failed to CRC checking!")
                else:
                    DPRINT(2, 0, "Unknown failed to CRC checking!")
            else:
                DPRINT(2, 0, "Not do CRC checking!")
                allow_to_upgrade = 1
        else:
            DPRINT(1, 0, "Wrong response format!")
    else:
        DPRINT(1, 0, "Response is NULL!")

    return allow_to_upgrade


def main(port, baudrate, rtscts, timeout, chunksize, lfile, dbglvl):
    global LOG_LV
    global RK_VER_T
    start_upgrade = 0
    LOG_LV = dbglvl
    ser_ready = False

    # boot detect - optional
    if USE_REPLUG_DETECT == "y":
        while ser_ready == False:
            ser_ready = True
            try:
                tty = serial.Serial(
                    port=port, baudrate=baudrate, rtscts=rtscts, timeout=timeout
                )

            except SerialException:
                DPRINT(
                    0,
                    0,
                    "can not open port "
                    + str(port)
                    + ".\nPlease make sure the port number or re-plug the cable!!",
                )
                ser_ready = False
                time.sleep(2)

        rx_boot = bytearray()
        while True:
            bytes = tty.read()
            if bytes:
                for b in bytes:
                    rx_boot.append(b)
                if rx_boot.endswith("Init logs:"):
                    # print rx_boot
                    rx_boot[:] = 0
                    break
                if bytes == "\n":
                    # print rx_boot
                    rx_boot[:] = 0
            else:
                DPRINT(0, 0, "Timeout. Please try again!!")
                # print rx_boot
                sys.exit(-1)
    else:
        try:
            tty = serial.Serial(
                port=port, baudrate=baudrate, rtscts=rtscts, timeout=timeout
            )

        except SerialException:
            DPRINT(0, 0, "can not open port " + str(port))
            sys.exit(-1)

    tty.flushInput()
    if disable_sleep(tty) == 0:
        DPRINT(0, 0, "CLI is not response. Please try again!!")
        sys.exit(-1)

    tty.flushInput()
    sw_cli_to_at_channel(tty)

    tty.flushInput()
    check_at_channel(tty, 20)
    DPRINT(4, 0, "AT is connected")

    DPRINT(3, 0, "checking device mode...")
    tty.flushInput()
    debug = get_file_mode(tty)
    if debug == 0:
        rfile = "update.ua"
    elif debug == 1:
        rfile = "b:/update.ua"
    elif debug == 2:  # downgrade handle
        tty.flushInput()
        DPRINT(3, 0, "checking device version...")
        chk_ability = get_file_chk_ability(tty)
        if chk_ability == 0:
            DPRINT(2, 0, "Detected device SW is old. Auto-downgrade to type 1")
            RK_VER_T = 1
        else:
            DPRINT(2, 0, "Detected device SW is old. Auto-downgrade to type 2")
            RK_VER_T = 2

    if RK_VER_T == 1 or RK_VER_T == 2:
        rfile = "update.ua"

    tty.flushInput()
    configure_at_echo(tty, "0")
    DPRINT(4, 0, "diabled AT echo")

    tty.flushInput()
    rsp = 1
    if RK_VER_T == 1:
        ret_v = config_file_param_v1(tty, rfile)
        if ret_v != 0:
            tty.flushInput()
            rsp = sendfw(tty, chunksize, lfile)
            if rsp != 0 and rsp != -1:
                start_upgrade = 1
        else:
            DPRINT(0, 0, "failed to File transmission!")
    else:
        ret_v = config_file_param(tty, rfile, lfile)
        if ret_v != 0:
            tty.flushInput()
            retv = chk_dev_cfg(tty)
            rsp = sendfw(tty, chunksize, lfile)
            if fsend_resp_chk(retv, rsp) == 1:
                start_upgrade = 1
            else:
                DPRINT(0, 0, "failed to File transmission!")
        else:
            DPRINT(0, 0, "failed to File transmission!")

    if start_upgrade == 1:
        print ("----->[ Complete to file download ]")
        if config_upg_param(tty, rfile) == 0:
            DPRINT(1, 0, "Failed to set upgrade parameters")
            start_upgrade = 0
    if rsp == 0:
        re_gen_tmpfs(tty, DUMMY_INTERM_STR)

    configure_at_echo(tty, "1")
    DPRINT(4, 0, "enabled AT echo")

    if start_upgrade == 1:
        send_atz(tty)
        print ("----->[ Upgrading device... please wait ]")

        rx_upg = bytearray()
        while True:
            bytes = tty.read()
            if bytes:
                for b in bytes:
                    rx_upg.append(b)
                if rx_upg.endswith("Init logs:"):
                    break
                if bytes == "\n":
                    print rx_upg[:-1]
                    rx_upg = bytearray()
            else:
                DPRINT(2, 0, "No reponse from device")
                print rx_upg
                break

        time.sleep(2)
        tty.flushInput()
        check_upgrade_result(tty)

        print ("\n =========== MCU upgrade done! =========== ")


def usage(port, baudrate):
    global TOOL_NAME
    global TOOL_VER
    global MAX_CHUNK_SZ

    print (TOOL_NAME + "({:s}) - Rev: ".format(prog) + TOOL_VER)
    print ("\nSynopsis -")
    print (
        "    {:s} -o <local file> [-p <at port>] [-b <baudrate>] [-r] [-t <timeout>] [-l <chunksize>] [-d <level>]".format(
            prog
        )
    )
    print ("\nDefault setting -")
    print ("    port= " + str(port) + " ;baudrate= " + str(baudrate))
    print ("\nNotice: parameter [-p] MUST be specified")
    print ("\n\nParameters -")
    print ("\n-o, --localf")
    print (
        "\n\tSpecify the local file which you wanted to download/upload. Must be specified."
    )
    print ("\n-d, --dbglv")
    print (
        "\n\tSet debug print level. 0: Critical / 1: Error / 2: Warn / 3: Info / 4: Debug . Default value: 2"
    )
    print ("\n-r, --rtscts")
    print ("\n\tEnables rtscts HW flow control")
    print ("\n-t, --timeout")
    print (
        "\n\tTimeout value for opening the windows COM port and response time, minimum is 5, default value: 5 seconds"
    )
    print ("\n-p, --port")
    print (
        "\n\tCOM port used for send CLI/AT command, must be specified correctly to CLI/AT COM port. Default value: COM95"
    )
    print ("\n-b, --baudrate")
    print ("\n\tCOM port baudrate, range: 38400 - 3000000. Default value: 115200")
    print ("\n-l, --chunksize")
    print (
        "\n\tDefine the size for file transmission, useful for controlling data transfer speed. Range: 2 - "
        + str(MAX_CHUNK_SZ)
    )
    print ("\n\tNote that in early device software version, it only support up to 960.")

    sys.exit(-1)


if __name__ == "__main__":
    port = "COM95"
    baudrate = 115200
    rtscts = 0
    timeout = 10
    chunksize = 3000
    lfile = ""
    vport = None
    dbglvl = 2
    force = 0
    prog = sys.argv[0]

    try:
        opts, args = getopt.getopt(
            sys.argv[1:],
            "p:b:rt:l:o:d:s",
            [
                "port=",
                "baudrate=",
                "rtscts",
                "timeout=",
                "chunksize=",
                "localf=",
                "dbglv=",
            ],
        )
    except getopt.GetoptError:
        usage(port, baudrate)

    for opt, arg in opts:
        if opt in ("-p", "--port"):
            port = arg
        elif opt in ("-b", "--baudrate"):
            if int(arg) in BAUDRATES:
                baudrate = int(arg)
            else:
                usage(port, baudrate)
        elif opt in ("-r", "--rtscts"):
            rtscts = True
        elif opt in ("-t", "--timeout"):
            timeout = float(arg)
            if timeout < 5:
                usage(port, baudrate)
        elif opt in ("-l", "--chunksize"):
            chunksize = int(arg)
            if chunksize < 2 or chunksize > MAX_CHUNK_SZ:
                usage(port, baudrate)
        elif opt in ("-o", "--localf"):
            lfile = arg
        elif opt in ("-d", "--dbglv"):
            dbglvl = int(arg)
            if dbglvl < 0 or dbglvl > 4:
                usage(port, baudrate)

    if lfile == "":
        # select local file
        default_dir = "C:\\"
        lfile = tkFileDialog.askopenfilename(
            title="Choose the delta image file",
            initialdir=(os.path.expanduser(default_dir)),
        )
        if not lfile:
            print ("\n\tUser terminated!")
            sys.exit(-1)
    main(port, baudrate, rtscts, timeout, chunksize, lfile, dbglvl)
