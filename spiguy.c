/* SPI Piper */

#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>

#include <linux/ioctl.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>

#define NAME "spiguy"
#define VERSION "0.1"

static char *progname;
static char *input = NULL;
static char *output = NULL;
static unsigned int bufsize = 0;

struct option_values {
    char *device;
    int speed;
    int mode;
    unsigned int maxsize;
};

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

static void slurp_stdin(char *buffer, uint32_t *size)
{
    int byte = getc(stdin);
    unsigned int index = 0;

    while((byte != EOF) && (index < bufsize)) {
        buffer[index] = (char)(byte);
        byte = getc(stdin);
        index++;
    }

    if(ferror(stdin)) {
        fprintf(stderr, "%s: Error reading from stdin.\n", progname);
        quit(EXIT_FAILURE);
    }

    *size = index;
}

static void dump_stdout(char *buffer, uint32_t size)
{
    for(unsigned int x = 0; x < size; x++) {
        putc(buffer[x], stdout);
    }
}

static void usage(char *name)
{
    fprintf(stderr, "Usage: %s DEVICE [options]\n\n", name);

    fprintf(stderr, "Collects all data from stdin and transmits it over the ");
    fprintf(stderr, "MOSI line of a\ntarget spidev device. Data is ");
    fprintf(stderr, "transferred in a single SPI transaction.\nAll data ");
    fprintf(stderr, "received over MISO is printed to stdout.\n");
    fprintf(stderr, "\nPositional arguments:\n");
    fprintf(stderr, "  DEVICE               Target spidev device.\n");
    fprintf(stderr, "\nOptional arguments:\n");
    fprintf(stderr, "  -s, --speed=<speed>  Maximum SPI clock rate (in Hz).\n");
    fprintf(stderr, "  -m, --mode=<int>     SPI transfer mode.\n");
    fprintf(stderr, "  -g, --maxsize=<int>  Maximum transfer size in bytes ");
    fprintf(stderr, "(default: 8192)\n");
    fprintf(stderr, "  -h, --help           Display this screen.\n");
    fprintf(stderr, "  -v, --version        Display the version number.\n");
}

static struct option_values parse_args(int argc, char *argv[])
{
    int opt;
    int long_index = 0;
    struct option_values retval = {
        .device = NULL, .speed = -1,
        .mode = -1, .maxsize = 8192
    };

    struct option options[] = {
        {"speed",   required_argument, NULL, 's'},
        {"mode",    required_argument, NULL, 'm'},
        {"maxsize", required_argument, NULL, 'g'},
        {"help",    no_argument, NULL, 'h'},
        {"version", no_argument, NULL, 'v'},
        {0, 0, 0, 0}
    };

    while((opt = getopt_long(argc, argv, "s:m:g:hv", options,
                             &long_index)) >= 0) {
        switch(opt) {
        case 'h':
            usage(progname);
            quit(EXIT_SUCCESS);
            break;

        case 'v':
            fprintf(stderr, "%s - %s\n", NAME, VERSION);
            quit(EXIT_SUCCESS);
            break;

        case 's':
            if(!sscanf(optarg, "%d", &retval.speed)) {
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
            if(!sscanf(optarg, "%d", &retval.mode)) {
                fprintf(stderr, "%s: Invalid SPI transfer mode [%s].\n",
                        progname, optarg);
                quit(EXIT_FAILURE);
            }

            if((retval.mode < 0) || (retval.mode > 3)) {
                fprintf(stderr, "%s: Mode is out of bounds\n", progname);
                quit(EXIT_FAILURE);
            }
            break;

        case 'g':
            if(!sscanf(optarg, "%u", &retval.maxsize)) {
                fprintf(stderr, "%s: Invalid transfer size [%s]\n", progname,
                        optarg);
                quit(EXIT_FAILURE);
            }
            break;

        default:
            fprintf(stderr, "%s: Invalid option -%c. See -h/--help.\n",
                    progname, opt);
            quit(EXIT_FAILURE);
        }
    }

    if((argc - optind) != 1) {
        usage(progname);
        quit(EXIT_FAILURE);
    }

    retval.device = argv[optind];

    if(access(retval.device, F_OK | R_OK | W_OK) != 0) {
        fprintf(stderr, "%s: Invalid SPI device [%s]\n", progname,
                retval.device);
        quit(EXIT_FAILURE);
    }

    return retval;
}

int run_spi(char *in, char *out, uint32_t len, struct option_values *args)
{
    int fd = open(args->device, O_RDWR);
    int retval;
    struct spi_ioc_transfer transfer = {
        .tx_buf = (uintptr_t) in,
        .rx_buf = (uintptr_t) out,
        .len = len,
        .speed_hz = args->speed,
        .delay_usecs = 0,
        .bits_per_word = 8,
        .cs_change = true
    };

    if(fd < 0) {
        perror(args->device);
        quit(EXIT_FAILURE);
    }

    if(args->speed != -1) {
        if(ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &args->speed) != 0) {
            fprintf(stderr, "Failed to set speed to %d\n", args->speed);
            perror("SPI_IOC_WR_MAX_SPEED_HZ");
            quit(EXIT_FAILURE);
        }
    }

    if(args->mode != -1) {
        if(ioctl(fd, SPI_IOC_WR_MODE, &args->mode) != 0) {
            fprintf(stderr, "Failed to set speed to %d\n", args->mode);
            perror("SPI_IOC_WR_MODE");
            quit(EXIT_FAILURE);
        }

    }

    retval = ioctl(fd, SPI_IOC_MESSAGE(1), &transfer);

    if(retval == EXIT_SUCCESS) {
        perror("SPI_IOC_MESSAGE(1)");
    } 

    return retval;
}

int main(int argc, char *argv[])
{
    struct option_values args;
    uint32_t transfer_size;
    int retval;

    progname = basename(argv[0]);
    args = parse_args(argc, argv);

    fprintf(stderr, "Speed:  %d\n", args.speed);
    fprintf(stderr, "Mode:   %d\n", args.mode);
    fprintf(stderr, "Device: %s\n", args.device);
    fprintf(stderr, "Size:   %u\n", args.maxsize);

    bufsize = args.maxsize;
    input = malloc(bufsize);
    output = malloc(bufsize);

    if(input == NULL) {
        fprintf(stderr, "%s: Couldn't allocte input buffer.\n", progname);
        quit(EXIT_FAILURE);
    }

    if(output == NULL) {
        fprintf(stderr, "%s: Couldn't allocte output buffer.\n", progname);
        quit(EXIT_FAILURE);
    }

    slurp_stdin(input, &transfer_size);

    retval = run_spi(input, output, transfer_size, &args);

    if(retval == EXIT_SUCCESS) {
        dump_stdout(output, transfer_size);
    } else {
        fprintf(stderr, "%s: Couldn't complete SPI successfully.\n", progname);
    }

    quit(retval);
}

