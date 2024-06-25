# nopia-sampler-test

This is an example with our current implementation of the SampleReader (based on
WavPlayer) for Daisy Pod, to debug the audio issues we are having while
streaming multiple samples from an SD card.

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
