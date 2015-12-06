#!/usr/bin/env python
# Copyright (C) 2015  Ronald Sutherland

# RPUadpt^2 has an FTDI USB chip on it 
# FTDI_!CTS and FTDI_!DSR can be set by the ATtiny1634
# pyserial is getting an overhaul for ver 3.x these are from ver 2.x
# ser.setRTS(True)    ser.getCTS()
# ser.setDTR(True)    ser.getDSR()

# note pyserial silently ignores DSR change on Windows 10-1511

import serial  # pip install pyserial
import _thread

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
while not quit_signal:
    bytesToRead = ser.inWaiting()
    if bytesToRead:
        data_raw = ser.read(bytesToRead)
        print("cts:" + str(ser.getCTS()) + " dsr:" + str(ser.getDSR()) + " data:" +str(data_raw))
    else:
        print("cts:" + str(ser.getCTS()) + " dsr:" + str(ser.getDSR()))