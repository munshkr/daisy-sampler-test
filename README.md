# sampler-test

## Description

You will need to have:
* MicroSD card with [these samples](https://drive.google.com/drive/folders/1RSWYkXHd0QHwRWoU4sSeadVigvZOFy5y?usp=sharing)
  stored in the root
* PHONE output connected to speaker

On power up, the programm will open and play all 16 samples in the SD card at
the same time.  Button 1 will restart playback of all samples, while button 2
will re-open all sample files (close+open).

## Development

### First time

If you are on Mac or Linux, run `./prepare.sh` to install everything.

We are using the **custom bootloader**, so we need to flash it first:

Plug Daisy Seed and activate DFU mode: press and hold Boot, then press and
hold Reset button, and finally release both buttons.  Then run:

```sh
make program-boot
```

### Compile and flash program

Run `make -j` to compile:

```sh
make -j
```

Then enter DFU mode from the bootloader (press Reset once, then press Boot)
Finally run:

```sh
make program-dfu
```
