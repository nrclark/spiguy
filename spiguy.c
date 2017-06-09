/* SPI Piper */

#include <fcntl.h>
#include <getopt.h>
#include <inttypes.h>
#include <libgen.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "spiguy_lib.h"
#include "usage_strings.h"

static char progname[64] = {0};
static char device[64] = {0};

static char *input = NULL;
static char *output = NULL;

static void quit(int exit_code)
{
    if((output != NULL) && (output != input)) {
        free(output);
    }

    if(input != NULL) {
        free(input);
    }

    exit(exit_code);
}

static void usage(char *name)
{
    fprintf(stderr, usage_prefix, canonical_name, name);
    fprintf(stderr, usage_string);
}

static options_t parse_args(int argc, char *argv[])
{
    size_t string_length;
    int opt;
    int long_index = 0;

    options_t retval = {.dev = NULL, .speed = -1, .mode = -1, .maxsize = 8192,
                        .gpio = -1, .polarity = -1, .unexport = false
                       };

    struct option cmd_opts[] = {
        {"speed",    required_argument, NULL, 's'},
        {"mode",     required_argument, NULL, 'm'},
        {"maxsize",  required_argument, NULL, 'k'},
        {"gpio",     required_argument, NULL, 'g'},
        {"polarity", required_argument, NULL, 'p'},
        {"unexport", required_argument, NULL, 'e'},
        {"input",    required_argument, NULL, 'i'},
        {"output",   required_argument, NULL, 'o'},
        {"help",    no_argument, NULL, 'h'},
        {"version", no_argument, NULL, 'v'},
        {0, 0, 0, 0}
    };

    do {
        opt = getopt_long(argc, argv, "s:m:k:g:p:i:o:uhv", cmd_opts,
                          &long_index);

        switch(opt) {
            case -1:
                break;

            case 'h':
                usage(progname);
                quit(EXIT_SUCCESS);
                break;

            case 'v':
                fprintf(stderr, "%s: version %s.\n", canonical_name, version);
                fprintf(stderr, "Compiled on %s at %s.\n", __DATE__, __TIME__);
                quit(EXIT_SUCCESS);
                break;

            case 's':
                if(!sscanf(optarg, "%" SCNd32, &retval.speed)) {
                    fprintf(stderr, "%s: Invalid SPI clock speed [%s].\n",
                            progname, optarg);
                    quit(EXIT_FAILURE);
                }

                if((retval.speed < 100) || (retval.speed > 100000000)) {
                    fprintf(stderr, "%s: Speed is out of bounds\n", progname);
                    quit(EXIT_FAILURE);
                }
                break;

            case 'm':
                if(!sscanf(optarg, "%" SCNd8, &retval.mode)) {
                    fprintf(stderr, "%s: Invalid SPI transfer mode [%s].\n",
                            progname, optarg);
                    quit(EXIT_FAILURE);
                }

                if((retval.mode < 0) || (retval.mode > 3)) {
                    fprintf(stderr, "%s: Mode is out of bounds. Valid",
                            progname);
                    fprintf(stderr, " modes are [0-3].\n");
                    quit(EXIT_FAILURE);
                }
                break;

            case 'k':
                if(!sscanf(optarg, "%zu", &retval.maxsize)) {
                    fprintf(stderr, "%s: Invalid transfer size [%s]\n",
                            progname, optarg);
                    quit(EXIT_FAILURE);
                }
                break;

            case 'g':
                if(!sscanf(optarg, "%" SCNd16, &retval.gpio)) {
                    fprintf(stderr, "%s: Invalid GPIO number [%s]\n", progname,
                            optarg);
                    quit(EXIT_FAILURE);
                }
                break;

            case 'p':
                if(!sscanf(optarg, "%" SCNd8, &retval.polarity)) {
                    fprintf(stderr, "%s: Invalid GPIO polarity [%s]\n",
                            progname, optarg);
                    quit(EXIT_FAILURE);
                }

                if((retval.polarity < 0) || (retval.polarity > 1)) {
                    fprintf(stderr, "%s: Polarity is out of bounds. ",
                            progname);
                    fprintf(stderr, "Valid polarities are [0, 1].\n");
                    quit(EXIT_FAILURE);
                }
                break;

            case 'u':
                retval.unexport = true;
                break;

            case 'i':
                if((*optarg == 'd') || (*optarg == 'x')) {
                    retval.input_mode = *optarg;
                } else {
                    fprintf(stderr, "%s: Input type '%c' is unknown.\n",
                            progname, *optarg);
                    fprintf(stderr, "Valid input types are [x, d].\n");
                    quit(EXIT_FAILURE);
                }
                break;

            case 'o':
                if((*optarg == 'd') || (*optarg == 'x')) {
                    retval.output_mode = *optarg;
                } else {
                    fprintf(stderr, "%s: Output type '%c' is unknown.\n",
                            progname, *optarg);
                    fprintf(stderr, "Valid output types are [x, d].\n");
                    quit(EXIT_FAILURE);
                }
                break;

            case '?':
                quit(EXIT_FAILURE);
                break;

        }
    } while(opt != -1);

    if((argc - optind) == 0) {
        fprintf(stderr, "%s: error: No SPI device specified. ", progname);
        fprintf(stderr, "See -h/--help for usage details.\n");
        quit(EXIT_FAILURE);
    }

    else if((argc - optind) != 1) {
        fprintf(stderr, "%s: error: multiple SPI devices ", progname);
        fprintf(stderr, "specified. See -h/--help for usage details.\n");
        quit(EXIT_FAILURE);
    }

    string_length = strnlen(argv[optind], sizeof(device) - 1);
    strncpy(device, argv[optind], string_length);
    retval.dev = device;

    if(access(retval.dev, F_OK | R_OK | W_OK) != 0) {
        fprintf(stderr, "%s: Invalid SPI device [%s]\n", progname,
                retval.dev);
        quit(EXIT_FAILURE);
    }

    if((retval.polarity != -1) && (retval.gpio == -1)) {
        fprintf(stderr, "%s: GPIO polarity setting is invalid ", progname);
        fprintf(stderr, "without -g/--gpio flag.\n");
        quit(EXIT_FAILURE);
    }

    if(retval.polarity == -1) {
        retval.polarity = 0;
    }

    if((retval.unexport) && (retval.gpio == -1)) {
        fprintf(stderr, "%s: GPIO unexport setting is invalid ", progname);
        fprintf(stderr, "without -g/--gpio flag.\n");
        quit(EXIT_FAILURE);
    }

    return retval;
}

static ssize_t read_formatted_input(char *storage, char format)
{
    const char hex_formatter[] = "%02X";
    const char dec_formatter[] = "%u";
    const char *formatter = NULL;
    ssize_t transfer_size = 0;
    int result;

    if(format == 'd') {
        formatter = dec_formatter;
    }

    if(format == 'x') {
        formatter = hex_formatter;
    }

    if(formatter != NULL) {
        transfer_size = -1;

        do {
            result = scanf(formatter, (unsigned char *)storage++);
            transfer_size++;
        } while(result > 0);

        if(result == 0) {
            fprintf(stderr, "%s: Error: couldn't parse input data.\n",
                    progname);
            quit(EXIT_FAILURE);
        }
    }
    return transfer_size;
}

static void write_formatted_output(const char *data, char format, uint32_t len)
{
    const char hex_formatter[] = "%02X";
    const char dec_formatter[] = "%u";
    const char *formatter = NULL;

    if(len-- == 0) {
        return;
    }

    if(format == 'd') {
        formatter = dec_formatter;
    }

    if(format == 'x') {
        formatter = hex_formatter;
    }

    printf(formatter, *(data++));

    while((len--) != 0) {
        putchar(' ');
        printf(formatter, *(data++));
    }
    putchar('\n');
}

int main(int argc, char *argv[])
{
    size_t string_length;
    options_t opts;
    size_t transfer_size;
    int retval;

    string_length = strnlen(basename(argv[0]), sizeof(device) - 1);
    strncpy(progname, basename(argv[0]), string_length);

    opts = parse_args(argc, argv);

    input = (char *) malloc((size_t) opts.maxsize);
    output = (char *) malloc((size_t) opts.maxsize);

    if(input == NULL) {
        fprintf(stderr, "%s: Couldn't allocte input buffer.\n", progname);
        quit(EXIT_FAILURE);
    }

    if(output == NULL) {
        fprintf(stderr, "%s: Couldn't allocte output buffer.\n", progname);
        quit(EXIT_FAILURE);
    }

    if(opts.input_mode == '\0') {
        transfer_size = (uint32_t) spiguy_file_read(NULL, input, opts.maxsize);
    } else {
        transfer_size = (uint32_t) read_formatted_input(input, opts.input_mode);
    }

    retval = spiguy_run_transfer(input, output, transfer_size, &opts);

    if(retval != EXIT_SUCCESS) {
        fprintf(stderr, "%s: Couldn't complete SPI successfully.\n", progname);
        quit(retval);
    }

    if(opts.output_mode == '\0') {
        spiguy_file_write(NULL, output, transfer_size);
        quit(retval);
    }

    write_formatted_output(output, opts.output_mode, transfer_size);
    quit(retval);
}

