# Ambilight-Clone
Final project for SETR Course

## Setup
### Python
LED controller library: `sudo pip install rpi_ws281x`
POSIX IPC library for named semaphores: `sudo pip install posix_ipc`

screenreader.c:
Make sure $DISPLAY is set to `:0` (when running project through ssh, run `export DISPLAY=:0` before other programs)

## Run 
`./screenreader | sudo python3 leds.py 2>&1`