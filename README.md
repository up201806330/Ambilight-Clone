# Ambilight-Clone

[![build](https://github.com/up201806330/Ambilight-Clone/actions/workflows/build.yml/badge.svg)](https://github.com/up201806330/Ambilight-Clone/actions/workflows/build.yml)

Final project for SETR Course

## Setup
### Python
- LED controller library: `sudo pip install rpi_ws281x`
- POSIX IPC library for named semaphores: `sudo pip install posix_ipc`

Note for screenreader.c:
Make sure $DISPLAY is set to `:0` (when running project through ssh, run `export DISPLAY=:0` before other programs)

## Run 
Build project

`make`

Run each process in a separate terminal

`./screenreader.app`

`./intensity.app`

`sudo python3 leds.py`
