# xcorners

A small utility to draw rounded screen corners on X11 window managers.

## Usage

```
$ xcorners --help
Usage: ./xcorners [OPTIONS]

Options:
  -W <width>    Set the horizontal space between the corners. [3000]
  -H <height>   Set the vertical space between the corners. [1920]
  -x <offset>   Set the horizontal offset. [0]
  -y <offset>   Set the vertical offset. [0]
  -r <radius>   Set the corner radius. [12]
  -t, -T        Enable or disable top corners. [enabled]
  -b, -B        Enable or disable bottom corners. [disabled]
  -c <color>    Set the corner color. [000000ff]
  -1            Only allow one instance.
  -h, --help    Print this help text and exit.
```

## Install

**Dependencies:**

- `gcc`
- `make`
- `libX11`, `libXfixes`, `libCairo` (install `-dev` or `-devel` packages)

**Building:**

After you cloned the repository, use `make` to build `xcorners`:

```sh
$ make
```

If successful, this creates the executable `./xcorners`.

**Installation:**

```
# make install
```

This installs `xcorners` to `/usr/bin`, if you want another prefix, set it using `PREFIX=""` or `EXEC_PREFIX=""`.

## License

`xcorners` is licensed under the MIT license. See [./LICENSE](./LICENSE) for more information.

