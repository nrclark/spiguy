#ifndef _SPIGUY_STRINGS_H_
#define _SPIGUY_STRINGS_H_

static const char canonical_name[] = "spiguy";

static const char version[] = "1.1";

static const char usage_prefix[] =
    "%s: A SPI command utility.\n"
    "Usage: %s DEVICE [options]\n";

static const char usage_string[] =
    "\nCollects all data from stdin and transmits it over the MOSI line of a\n"
    "target spidev device. Data is transferred in a single SPI transaction.\n"
    "All data received over MISO is printed to stdout.\n"
    "\n"
    "Positional arguments:\n"
    "  DEVICE (required)     Target spidev device.\n"
    "\n"
    "Optional arguments:\n"
    "  -s, --speed=<speed>   Maximum SPI clock rate (in Hz).\n"
    "  -m, --mode=<0,1,2,3>  SPI transfer mode.\n"
    "  -k, --maxsize=<int>   Maximum transfer size in bytes (default: 8192)\n"
    "\n"
    "  -g, --gpio=<number>   Provide chip-select on a GPIO pin in addition to\n"
    "                        any controller output.\n"
    "\n"
    "  -p, --polarity=<0,1>  GPIO chip-select polarity. '1' is active-high.\n"
    "                        (default 0).\n"
    "\n"
    "  -u, --unexport        Unexport/restore GPIO after transaction is over.\n"
    "\n"
    "  -i, --input=<d,x>     Assume input is a space-separated list of bytes\n"
    "                        in decimal ('d') or hexadecimal ('x') format.\n"
    "\n"
    "  -o, --output=<d,x>    Assume output is a space-separated list of bytes\n"
    "                        in decimal ('d') or hexadecimal ('x') format.\n"
    "\n"
    "  -v, --version         Display the version number.\n"
    "  -h, --help            Display this screen.\n\n";

#endif
