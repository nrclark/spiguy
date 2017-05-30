/* SPI Piper */

#include <fcntl.h>
#include <getopt.h>
#include <inttypes.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "spiguy_lib.h"

#define NAME "spiguy"
#define VERSION "0.1"

static char progname[64] = {0};
static char device[64] = {0};

static char *input = NULL;
static char *output = NULL;

static void quit(int exit_code)
{
    if(input != NULL) {
        free(input);
    }

    if(output != NULL) {
        free(output);
    }
    exit(exit_code);
}

static void usage(char *name)
{
    fprintf(stderr, "%s: A SPI command utility.\n", NAME);
    fprintf(stderr, "Usage: %s DEVICE [options]\n\n", name);

    fprintf(stderr, "Collects all data from stdin and transmits it over the ");
    fprintf(stderr, "MOSI line of a\ntarget spidev device. Data is ");
    fprintf(stderr, "transferred in a single SPI transaction.\nAll data ");
    fprintf(stderr, "received over MISO is printed to stdout.\n");
    fprintf(stderr, "\nPositional arguments:\n");
    fprintf(stderr, "  DEVICE (required)     Target spidev device.");
    fprintf(stderr, "\n\nOptional arguments:\n");
    fprintf(stderr, "  -s, --speed=<speed>   Maximum SPI clock rate (in Hz).");
    fprintf(stderr, "\n  -m, --mode=<0,1,2,3>  SPI transfer mode.\n");
    fprintf(stderr, "  -k, --maxsize=<int>   Maximum transfer size in bytes ");
    fprintf(stderr, "(default: 8192)\n  ");
    fprintf(stderr, "-g, --gpio=<number>   Provide chip-select on a GPIO pin");
    fprintf(stderr, " in addition to \n                        any controller");
    fprintf(stderr, " output.\n");
    fprintf(stderr, "  -p, --polarity=<0,1>  GPIO chip-select polarity. '1' ");
    fprintf(stderr, "is active-high.\n                        (default 0).\n");
    fprintf(stderr, "  -h, --help            Display this screen.\n");
    fprintf(stderr, "  -v, --version         Display the version number.\n");
}

static options_t parse_args(int argc, char *argv[])
{
    size_t string_length;
    int opt;
    int long_index = 0;

    options_t retval = {.dev = NULL, .speed = -1, .mode = -1, .maxsize = 8192,
                        .gpio = -1, .polarity = -1
                       };

    struct option cmd_options[] = {
        {"speed",    required_argument, NULL, 's'},
        {"mode",     required_argument, NULL, 'm'},
        {"maxsize",  required_argument, NULL, 'k'},
        {"gpio",     required_argument, NULL, 'g'},
        {"polarity", required_argument, NULL, 'p'},
        {"help",    no_argument, NULL, 'h'},
        {"version", no_argument, NULL, 'v'},
        {0, 0, 0, 0}
    };

    do {
        opt = getopt_long(argc, argv, "s:m:k:g:p:hv", cmd_options, &long_index);

        switch(opt) {
        case -1:
            break;

        case 'h':
            usage(progname);
            quit(EXIT_SUCCESS);
            break;

        case 'v':
            fprintf(stderr, "%s: version %s.\n", NAME, VERSION);
            fprintf(stderr, "Compiled on %s at %s.\n", __DATE__, __TIME__);
            quit(EXIT_SUCCESS);
            break;

        case 's':
            if(!sscanf(optarg, SCNd32, &retval.speed)) {
                fprintf(stderr, "%s: Invalid SPI clock speed [%s].\n", progname,
                        optarg);
                quit(EXIT_FAILURE);
            }

            if((retval.speed < 100) || (retval.speed > 100000000)) {
                fprintf(stderr, "%s: Speed is out of bounds\n", progname);
                quit(EXIT_FAILURE);
            }
            break;

        case 'm':
            if(!sscanf(optarg, SCNd8, &retval.mode)) {
                fprintf(stderr, "%s: Invalid SPI transfer mode [%s].\n",
                        progname, optarg);
                quit(EXIT_FAILURE);
            }

            if((retval.mode < 0) || (retval.mode > 3)) {
                fprintf(stderr, "%s: Mode is out of bounds. Valid", progname);
                fprintf(stderr, " modes are [0-3].\n");
                quit(EXIT_FAILURE);
            }
            break;

        case 'k':
            if(!sscanf(optarg, SCNu16, &retval.maxsize)) {
                fprintf(stderr, "%s: Invalid transfer size [%s]\n", progname,
                        optarg);
                quit(EXIT_FAILURE);
            }
            break;

        case 'g':
            if(!sscanf(optarg, SCNd16, &retval.gpio)) {
                fprintf(stderr, "%s: Invalid GPIO number [%s]\n", progname,
                        optarg);
                quit(EXIT_FAILURE);
            }
            break;

        case 'p':
            if(!sscanf(optarg, SCNd8, &retval.polarity)) {
                fprintf(stderr, "%s: Invalid GPIO polarity [%s]\n", progname,
                        optarg);
                quit(EXIT_FAILURE);
            }

            if((retval.polarity < 0) || (retval.polarity > 1)) {
                fprintf(stderr, "%s: Polarity is out of bounds. ", progname);
                fprintf(stderr, "Valid polarities are [0, 1].\n");
                quit(EXIT_FAILURE);
            }
            break;

        default:
            fprintf(stderr, "%s: Invalid option -%c. See -h/--help.\n",
                    progname, opt);
            quit(EXIT_FAILURE);
        }
    } while(opt != -1);

    if((argc - optind) == 0) {
        fprintf(stderr, "%s: error: No SPI device specified. See ", progname);
        fprintf(stderr, "-h/--help for usage details.\n");
        quit(EXIT_FAILURE);
    }

    else if((argc - optind) != 1) {
        fprintf(stderr, "%s: error: multiple SPI devices specified.", progname);
        fprintf(stderr, " See -h/--help for usage details.\n");
        quit(EXIT_FAILURE);
    }

    string_length = strnlen(argv[optind], sizeof(device) - 1);
    strncpy(device, argv[optind], string_length);
    retval.dev = device;

    if(access(retval.dev, F_OK | R_OK | W_OK) != 0) {
        fprintf(stderr, "%s: Invalid SPI device [%s]\n", progname, retval.dev);
        quit(EXIT_FAILURE);
    }

    if((retval.polarity != -1) && (retval.gpio == -1)) {
        fprintf(stderr, "%s: GPIO polarity setting is invalid ", progname);
        fprintf(stderr, "without -g/--gpio flag.\n");
        quit(EXIT_FAILURE);
    }

    if(retval.polarity != 0) {
        retval.polarity = 1;
    }

    return retval;
}

int main(int argc, char *argv[])
{
    size_t string_length;
    options_t options;
    size_t transfer_size;
    int retval;

    string_length = strnlen(basename(argv[0]), sizeof(device) - 1);
    strncpy(progname, basename(argv[0]), string_length);

    options = parse_args(argc, argv);

    input = (char *) malloc((size_t) options.maxsize);
    output = (char *) malloc((size_t) options.maxsize);

    if(input == NULL) {
        fprintf(stderr, "%s: Couldn't allocte input buffer.\n", progname);
        quit(EXIT_FAILURE);
    }

    if(output == NULL) {
        fprintf(stderr, "%s: Couldn't allocte output buffer.\n", progname);
        quit(EXIT_FAILURE);
    }

    transfer_size = (uint32_t) spiguy_file_read(NULL, input, options.maxsize);
    retval = spiguy_run_transfer(input, output, transfer_size, &options);

    if(retval == EXIT_SUCCESS) {
        spiguy_file_write(NULL, output, transfer_size);
    } else {
        fprintf(stderr, "%s: Couldn't complete SPI successfully.\n", progname);
    }

    quit(retval);
}

