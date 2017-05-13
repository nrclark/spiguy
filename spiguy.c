/* SPI Piper */

#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/ioctl.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>

#define NAME "spiguy"
#define VERSION "0.1"

struct option_values {
    char *device;
    int speed;
    int mode;
};

static void usage(char *name)
{
    fprintf(stderr, "usage: %s options...\n", name);
    fprintf(stderr, "  options:\n");
    fprintf(stderr, "    -d --device=<dev>   Target spidev device.\n");
    fprintf(stderr, "    -s --speed=<speed>  Maximum SPI clock rate (in Hz).\n");
    fprintf(stderr, "    -m --mode=<int>     SPI transfer mode.\n");
    fprintf(stderr, "    -h --help           Display this screen.\n");
    fprintf(stderr, "    -v --version        Display the version number.\n");
}

struct option_values parse_args(int argc, char *argv[])
{

    struct option_values retval = {.device = NULL, .speed = -1, .mode = -1};
    int opt;
    int long_index = 0;

    struct option options[] = {
        {"device",  required_argument, NULL, 'd'},
        {"speed",   required_argument, NULL, 's'},
        {"mode",    required_argument, NULL, 'm'},
        {"help",    no_argument, NULL, 'h'},
        {"version", no_argument, NULL, 'v'},
        {0, 0, 0, 0}
    };

    while((opt = getopt_long(argc, argv, "d:s:b:n:rhv", options,
                             &long_index)) >= 0) {
        switch(opt) {
        case 'h':
            usage(argv[0]);
            exit(EXIT_SUCCESS);
            break;

        case 'v':
            fprintf(stderr, "%s - %s\n", NAME, VERSION);
            exit(EXIT_SUCCESS);
            break;

        case 'd':
            retval.device = optarg;
            if(access(retval.device, F_OK | R_OK | W_OK) != 0) {
                fprintf(stderr, "%s: Invalid SPI device [%s]\n", argv[0],
                        retval.device);
                exit(EXIT_FAILURE);
            }
            break;

        case 's':
            if(!sscanf(optarg, "%d", &retval.speed)) {
                fprintf(stderr, "%s: Invalid speed [%d]\n", argv[0],
                        retval.speed);
                exit(EXIT_FAILURE);
            }

            if((retval.speed < 100) || (retval.speed > 100000000)) {
                fprintf(stderr, "%s: Speed is out of bounds\n", argv[0]);
                exit(EXIT_FAILURE);
            }
            break;

        case 'm':
            if(!sscanf(optarg, "%d", &retval.mode)) {
                fprintf(stderr, "%s: Invalid mode [%d]\n", argv[0],
                        retval.mode);
                exit(EXIT_FAILURE);
            }

            if((retval.mode < 0) || (retval.mode > 3)) {
                fprintf(stderr, "%s: Mode is out of bounds\n", argv[0]);
                exit(EXIT_FAILURE);
            }
            break;

        default:
            fprintf(stderr, "%s: Invalid option -%c. See -h/--help.\n",
                    argv[0], opt);
            exit(EXIT_FAILURE);
        }
    }

    if(retval.device == NULL) {
        fprintf(stderr, "%s: -d/--device argument is mandatory.\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    return retval;
}

int main(int argc, char *argv[])
{
    struct option_values args = parse_args(argc, argv);

    fprintf(stdout, "Speed: %d\n", args.speed);
    fprintf(stdout, "Mode: %d\n", args.mode);
    fprintf(stdout, "Device: %s\n", args.device);

    if(args.speed != -1) {
        if(ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &args.speed) != 0) {
            fprintf(stderr, "Failed to set speed to %d\n", args.speed);
            perror("SPI_IOC_WR_MAX_SPEED_HZ");
            exit(EXIT_FAILURE);
        }
    }

    if(args.mode != -1) {
        if(ioctl(fd, SPI_IOC_WR_MODE, &args.mode) != 0) {
            fprintf(stderr, "Failed to set speed to %d\n", args.mode);
            perror("SPI_IOC_WR_MODE");
            exit(EXIT_FAILURE);
        }
    }

    return EXIT_SUCCESS;
}

