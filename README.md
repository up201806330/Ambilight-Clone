# Ambilight-Clone

[![build](https://github.com/up201806330/Ambilight-Clone/actions/workflows/build.yml/badge.svg)](https://github.com/up201806330/Ambilight-Clone/actions/workflows/build.yml)

- **Project name:** Ambilight TV Project with Raspberry Pi
- **Short description:** LED strip installed around a screen that reacts to the colors displayed
- **Environment:** Raspberry Pi OS (Debian-based)
- **Tools:** Raspberry Pi, C++, Python
- **Institution:** [FEUP](https://sigarra.up.pt/feup/en/web_page.Inicial)
- **Course:** [ERTS](https://sigarra.up.pt/feup/en/ucurr_geral.ficha_uc_view?pv_ocorrencia_id=486266) (Embedded and Real Time Systems)
- **Project grade:** 17.2/20
- **Group members:**
    - [Diogo Miguel Ferreira Rodrigues](https://github.com/dmfrodrigues) (<dmfrodrigues2000@gmail.com> / <diogo.rodrigues@fe.up.pt>)
    - Maria Aurora Bota ([up202111378@edu.fe.up.pt](mailto:up202111378@edu.fe.up.pt))
    - [Rafael Soares Ribeiro](https://github.com/up201806330) (<up201806330@fe.up.pt>)
    - [Xavier Ruivo Pisco](https://github.com/Xavier-Pisco) ([up201806134@edu.fe.up.pt](mailto:up201806134@edu.fe.up.pt))

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

`sudo python3 leds.py` (*must be sudo to write to RaspberryPi's GPIO pins*)

## Performance

#### Screenreader
Change `NUM_RUNS` to the number of screenreads you want, compile with make and run `./screenreader`.
If `ledPrint();` is not commented, comment for more accurate results.

#### Leds
Change `NUM_RUNS` to the number of led color change you want to be made, run `./screenreader | sudo python3 leds.py 2>&1`.
If `else: print(colors)` is not commented, comment for more accurate results.
