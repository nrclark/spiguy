/* SPI Piper */

#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/stat.h>
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

static char progname[64];
static char device[64];

static char *input = NULL;
static char *output = NULL;

static bool unexport = true;
static char gpio_num[9];
static char gpio_direction[8] = {0};
static char gpio_value[4] = {0};
static char gpio_active_low[4] = {0};

static unsigned int bufsize = 0;

static char gpio_dir[32] = {0};
static const char gpio_export[] = "/sys/class/gpio/export";

static char direction_name[42] = {0};
static char value_name[42] = {0};
static char active_low_name[42] = {0};

struct option_values {
    char *device;
    int speed;
    int mode;
    unsigned int maxsize;
    char *gpio;
    int polarity;
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
    fprintf(stderr, "is active-high.\n                        (default 1).\n");
    fprintf(stderr, "  -h, --help            Display this screen.\n");
    fprintf(stderr, "  -v, --version         Display the version number.\n");
}

static struct option_values parse_args(int argc, char *argv[])
{
    int number;
    int opt;
    int long_index = 0;

    struct option_values retval = {
        .device = NULL,
        .speed = -1,
        .mode = -1,
        .maxsize = 8192,
        .gpio = NULL,
        .polarity = -1
    };

    struct option options[] = {
        {"speed",    required_argument, NULL, 's'},
        {"mode",     required_argument, NULL, 'm'},
        {"maxsize",  required_argument, NULL, 'k'},
        {"gpio",     required_argument, NULL, 'g'},
        {"polarity", required_argument, NULL, 'p'},
        {"help",    no_argument, NULL, 'h'},
        {"version", no_argument, NULL, 'v'},
        {0, 0, 0, 0}
    };

    opt = getopt_long(argc, argv, "s:m:k:g:p:hv", options, &long_index);

    while(opt >= 0) {
        switch(opt) {
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
                fprintf(stderr, "%s: Mode is out of bounds. Valid", progname);
                fprintf(stderr, " modes are [0-3].\n");
                quit(EXIT_FAILURE);
            }
            break;

        case 'k':
            if(!sscanf(optarg, "%u", &retval.maxsize)) {
                fprintf(stderr, "%s: Invalid transfer size [%s]\n", progname,
                        optarg);
                quit(EXIT_FAILURE);
            }
            break;

        case 'g':
            if(!sscanf(optarg, "%d", &number)) {
                fprintf(stderr, "%s: Invalid GPIO number [%s]\n", progname,
                        optarg);
                quit(EXIT_FAILURE);
            }
            strncpy(gpio_num, optarg, 7);
            retval.gpio = gpio_num;
            break;

        case 'p':
            if(!sscanf(optarg, "%d", &retval.polarity)) {
                fprintf(stderr, "%s: Invalid GPIO polarity [%s]\n", progname,
                        optarg);
                quit(EXIT_FAILURE);
            }
            break;

        default:
            fprintf(stderr, "%s: Invalid option -%c. See -h/--help.\n",
                    progname, opt);
            quit(EXIT_FAILURE);
        }
        opt = getopt_long(argc, argv, "s:m:k:g:p:hv", options, &long_index);
    }

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


    strncpy(device, argv[optind], 63);
    retval.device = device;

    if(access(retval.device, F_OK | R_OK | W_OK) != 0) {
        fprintf(stderr, "%s: Invalid SPI device [%s]\n", progname,
                retval.device);
        quit(EXIT_FAILURE);
    }

    if((retval.polarity != -1) && (retval.gpio == NULL)) {
        fprintf(stderr, "%s: GPIO polarity setting is invalid ", progname);
        fprintf(stderr, "without -g/--gpio flag.\n");
        quit(EXIT_FAILURE);
    }

    if(retval.polarity != 0) {
        retval.polarity = 1;
    }

    return retval;
}

void read_file(const char *name, char *dest, unsigned int maxcount)
{
    FILE *fp = fopen(name, "rb");

    if(fp == NULL) {
        perror(name);
        quit(EXIT_FAILURE);
    }

    if(fgets(dest, maxcount, fp) == NULL) {
        perror(dest);
        fclose(fp);
        quit(EXIT_FAILURE);
    }

    fclose(fp);
}

void write_file(const char *name, const char *source, unsigned int maxcount)
{
    int length = strnlen(source, maxcount);
    FILE *fp = fopen(name, "wb");

    if(fp == NULL) {
        perror(name);
        quit(EXIT_FAILURE);
    }

    fwrite(source, 1, length, fp);
    fclose(fp);
}

void prepare_gpio(char *gpio_num, int polarity)
{
    struct stat folder_stat;
    bool gpio_exists = false;

    snprintf(gpio_dir, 31, "/sys/class/gpio/gpio%s", gpio_num);

    if(access(gpio_export, F_OK | R_OK | W_OK) != 0) {
        fprintf(stderr, "%s: Can't access GPIO sysfs.\n", progname);
        quit(EXIT_FAILURE);
    }

    strncpy(direction_name, gpio_dir, 41);
    strncpy(value_name, gpio_dir, 41);
    strncpy(active_low_name, gpio_dir, 41);

    strcat(direction_name, "/direction");
    strcat(value_name, "/value");
    strcat(active_low_name, "/active_low");

    if((stat(gpio_dir, &folder_stat) == 0) && S_ISDIR(folder_stat.st_mode)) {
        gpio_exists = true;
    }

    if(gpio_exists == false) {
        unexport = true;
        write_file(gpio_export, gpio_num, 8);
    } else {
        unexport = false;
        read_file(direction_name, gpio_direction, 4);
        read_file(value_name, gpio_value, 4);
        read_file(active_low_name, gpio_active_low, 4);
    }

    if(polarity == 1) {
        write_file(active_low_name, "0", 2);
    } else {
        write_file(active_low_name, "1", 2);
    }

    write_file(direction_name, "low", 4);
    write_file(value_name, "0", 2);
}

void release_gpio(char *gpio_num)
{
    write_file(direction_name, "in", 3);
    if(unexport) {
        write_file(direction_name, gpio_num, 8);
    }
}

int run_spi(char *in, char *out, uint32_t len, struct option_values *args)
{
    int fd = open(args->device, O_RDWR);
    int retval;

    struct spi_ioc_transfer transfer = {
        .tx_buf = (uintptr_t) in,
        .rx_buf = (uintptr_t) out,
        .len = len,
        .delay_usecs = 0,
        .bits_per_word = 8,
        .cs_change = 1,
        .tx_nbits = 8,
        .rx_nbits = 8,
        .pad = 0
    };

    if(fd < 0) {
        perror(args->device);
        quit(EXIT_FAILURE);
    }

    if(args->speed != -1) {
        if(ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &args->speed) < 0) {
            fprintf(stderr, "Failed to set speed to %d\n", args->speed);
            perror("SPI_IOC_WR_MAX_SPEED_HZ");
            quit(EXIT_FAILURE);
        }
    }

    if(args->mode != -1) {
        if(ioctl(fd, SPI_IOC_WR_MODE, &args->mode) < 0) {
            fprintf(stderr, "Failed to set speed to %d\n", args->mode);
            perror("SPI_IOC_WR_MODE");
            quit(EXIT_FAILURE);
        }
    }

    if(args->gpio != NULL) {
        prepare_gpio(args->gpio, args->polarity);
        write_file(direction_name, "1", 2);
    }

    retval = ioctl(fd, SPI_IOC_MESSAGE(1), &transfer);

    if(args->gpio != NULL) {
        write_file(direction_name, "0", 2);
        release_gpio(args->gpio);
    }

    if(retval < 0) {
        perror("SPI_IOC_MESSAGE(1)");
    } else {
        retval = 0;
    }

    close(fd);
    return retval;
}

int main(int argc, char *argv[])
{
    struct option_values args;
    uint32_t transfer_size;
    int retval;

    strncpy(progname, basename(argv[0]), 63);
    args = parse_args(argc, argv);

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

