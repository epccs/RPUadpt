# Remote

This bus manager firmware is for an RPUadpt board, it will watch for a byte on DTR to reset the local MCU board (e.g. an RPUadpt on Irrigate7) which places it in bootloader mode.

## Overview

RS485 pairs have RX driver disabled and receiver enabled, TX driver enabled and receiver disabled.

Basicly the RX pair receiver provides output for the local MCU RX input. While the TX pair driver takes input from the loclal MCU TX output. 

A byte on the DTR pair will reset the local MCU. DTR_DE is held high to enable the DTR pair driver. DTR_nRE is held low to enable the DTR pair receiver. 

The byte on DTR is sent from Host2Remote firmware on an RPUftdi board. The two programs are an early proof of concept and need a lot more work.