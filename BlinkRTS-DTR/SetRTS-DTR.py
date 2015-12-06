#!/usr/bin/env python
# Copyright (C) 2015  Ronald Sutherland

# RPUadpt^2 has an FTDI USB chip on it 
# FTDI_!RTS and FTDI_!DTR can be set with Python
# pyserial is getting an overhaul for ver 3.x these are from ver 2.x
# ser.setRTS(True)    ser.getCTS()
# ser.setDTR(True)    ser.getDSR()

# note pyserial silently ignores DSR change on Windows 10-1511

import serial  # pip install pyserial
import _thread, time

# press enter key and it will set the signal to quit
# Any key won't work you have to Enter to exit, a deep thought ;)
# a Python list is by referance
def input_thread(quit_signal):
    input()
    quit_signal.append(None)

quit_signal = []
_thread.start_new_thread(input_thread, (quit_signal,))
ser = serial.Serial("COM4", timeout=None, baudrate=9600, xonxoff=False, rtscts=False, dsrdtr=False)
ser.flushInput()
ser.flushOutput()
blinker = 0     # 0 blinks rts, 1 blinks dtr
set_rts_rate = 0.25
set_dtr_rate = 2
swap_rate = 8
swap_now = time.time() + swap_rate
set_rts_now = time.time() + set_rts_rate
rts = False
ser.setRTS(rts)
set_dtr_now = time.time() + set_dtr_rate
dtr = False
ser.setDTR(dtr)
while not quit_signal:
    bytesToRead = ser.inWaiting()
    if swap_now < time.time():
        rts = False
        ser.setRTS(rts)
        dtr = False
        ser.setDTR(dtr)
        swap_now = time.time() + swap_rate
        if blinker == 0:
            blinker =1
        else:
            blinker = 0
    if blinker:
        if set_dtr_now < time.time():
            ser.setDTR(dtr)
            dtr = not dtr
            set_dtr_now = time.time() + set_dtr_rate
    else:
        if set_rts_now < time.time():
            ser.setRTS(rts)
            rts = not rts
            set_rts_now = time.time() + set_rts_rate
    if bytesToRead:
        data_raw = ser.read(bytesToRead)
        print("rts:" + str(rts) + " cts:" + str(ser.getCTS()) + " dtr:" + str(dtr) + " dsr:" + str(ser.getDSR()) + " data:" +str(data_raw))
    else:
        print("rts:" + str(rts) + " cts:" + str(ser.getCTS()) + " dtr:" + str(dtr) + " dsr:" + str(ser.getDSR()))