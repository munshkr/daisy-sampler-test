# sampler-test

This is a WavPlayer test rom for Daisy Pod, to debug some audio issues we are
having while streaming multiple samples from an SD card.

## Development

### First time

*Note*: If you are on Mac or Linux, run `./prepare.sh` to install everything.
Otherwise, keep reading for the manual instructions.

After cloning the repository, compile libDaisy and DaisySP with:

```sh
make -j -C libs/libDaisy
```

### Compile and flash firmware

Run `make -j` to compile the firmware:

```sh
make -j
```

Plug your Daisy Seed and activate DFU mode: press and hold Boot, then press and
hold Reset button, and finally release both buttons in your Daisy.  Now run:

```sh
make program-dfu
```
