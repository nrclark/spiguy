/* SPI Piper */

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

#include "spiguy_lib.h"

static bool gpio_exists = false;

static const char gpio_base[] = "/sys/class/gpio/gpio";
static const char exporter[] = "/sys/class/gpio/export";
static const char unexporter[] = "/sys/class/gpio/unexport";

static const char direction_append[] = "/direction";
static const char value_append[] = "/value";
static const char actlow_append[] = "/active_low";

static char gpio_num[9] = {0};
static char gpio[sizeof(gpio_base) + sizeof(gpio_num)] = {0};

static char dir_file[sizeof(gpio) + sizeof(direction_append) + 1] = {0};
static char value_file[sizeof(gpio) + sizeof(value_append) + 1] = {0};
static char actlow_file[sizeof(gpio) + sizeof(actlow_append) + 1] = {0};

static char gpio_direction[4] = {0};
static char gpio_value[4] = {0};
static char gpio_actlow[4] = {0};
static bool gpio_valid = false;

static char msg[128] = {0};
static int spiguy_error = 0;

size_t spiguy_file_read(const char *name, char *dest, size_t maxcount)
{
    FILE *fp = stdin;
    size_t count;

    if(spiguy_error) {
        return 0;
    }

    if(name != NULL) {
        fp = fopen(name, "rb");
    }

    if(fp == NULL) {
        snprintf(msg, sizeof(msg) - 1, "Can't open file [%s]", name);
        perror(msg);
        spiguy_error = 1;
        return 0;
    }

    errno = 0;
    count = fread(dest, (size_t) 1, maxcount, fp);

    if(errno != 0) {
        if(name != NULL) {
            snprintf(msg, sizeof(msg) - 1, "Can't read from file [%s]", name);
            fclose(fp);
        } else {
            snprintf(msg, sizeof(msg) - 1, "Can't read from stdin");
        }

        perror(msg);
        spiguy_error = 1;
        count = 0;
    }

    if(fp != stdin) {
        if(fclose(fp) == EOF) {
            snprintf(msg, sizeof(msg) - 1, "Can't close file [%s]", name);
            perror(msg);
            spiguy_error = 1;
            return 0;
        }
    }

    return count;
}

int spiguy_file_write(const char *name, const char *source, size_t count)
{
    FILE *fp = stdout;
    size_t written;
    int retval = 0;

    if(spiguy_error) {
        return spiguy_error;
    }

    if(name != NULL) {
        fp = fopen(name, "wb");
    }

    if(fp == NULL) {
        snprintf(msg, sizeof(msg) - 1, "Can't open file [%s]", name);
        perror(msg);
        spiguy_error = 1;
        return errno;
    }

    written = fwrite(source, (size_t) 1, count, fp);

    if(written != count) {
        snprintf(msg, sizeof(msg) - 1, "Error writing to file [%s]", name);
        perror(msg);
        spiguy_error = 1;
        retval = errno;
    }

    if(fp != stdout) {
        if(fclose(fp) == EOF) {
            snprintf(msg, sizeof(msg) - 1, "Can't close file [%s]", name);
            perror(msg);
            spiguy_error = 1;
            return errno;
        }
    }

    return retval;
}

void spiguy_gpio_prepare(options_t *options)
{
    struct stat folder_stat;

    snprintf(gpio_num, sizeof(gpio_num) - 1, "%u", (uint16_t)options->gpio);
    snprintf(gpio, sizeof(gpio) - 1, "/sys/class/gpio/gpio%s", gpio_num);

    if(access(exporter, F_OK | R_OK | W_OK) != 0) {
        perror("Can't access GPIO sysfs");
        return;
    }

    snprintf(dir_file, sizeof(dir_file) - 1, "%s%s", gpio, direction_append);
    snprintf(value_file, sizeof(value_file) - 1, "%s%s", gpio, value_append);
    snprintf(actlow_file, sizeof(actlow_file) - 1, "%s%s", gpio, actlow_append);

    if((stat(gpio, &folder_stat) == 0) && S_ISDIR(folder_stat.st_mode)) {
        gpio_exists = true;
    } else {
        spiguy_file_write(exporter, gpio_num, strlen(gpio_num));
    }

    if(options->unexport && gpio_exists) {
        spiguy_file_read(dir_file, gpio_direction, sizeof(gpio_direction) - 1);
        spiguy_file_read(value_file, gpio_value, sizeof(gpio_value) - 1);
        spiguy_file_read(actlow_file, gpio_actlow, sizeof(gpio_actlow) - 1);
    }

    if(options->polarity == 1) {
        spiguy_file_write(actlow_file, "0", (size_t) 1);
        spiguy_file_write(dir_file, "low", (size_t) 3);
    } else {
        spiguy_file_write(actlow_file, "1", (size_t) 1);
        spiguy_file_write(dir_file, "high", (size_t) 4);
    }

    spiguy_file_write(value_file, "0", (size_t) 1);
    gpio_valid = true;
}

void spiguy_gpio_release(void)
{
    if(gpio_valid == false) {
        return;
    }

    if(gpio_exists) {
        spiguy_file_write(dir_file, gpio_direction, strlen(gpio_direction));
        spiguy_file_write(value_file, gpio_value, strlen(gpio_value));
        spiguy_file_write(actlow_file, gpio_actlow, strlen(gpio_actlow));
    } else {
        spiguy_file_write(value_file, "0", (size_t) 1);
        spiguy_file_write(dir_file, "in", (size_t) 2);
        spiguy_file_write(unexporter, gpio_num, strlen(gpio_num));
    }
}

int spiguy_set_spi_speed(int fd, uint32_t speed)
{
    if(ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
        snprintf(msg, sizeof(msg) - 1, "Failed to set speed to %u", speed);
        perror(msg);
        spiguy_error = 1;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int spiguy_set_spi_mode(int fd, uint8_t mode)
{
    unsigned int temp = mode;
    if(ioctl(fd, SPI_IOC_WR_MODE, &temp) < 0) {
        snprintf(msg, sizeof(msg) - 1, "Failed to set mode to %u", mode);
        perror(msg);
        spiguy_error = 1;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int spiguy_run_transfer(char *in, char *out, size_t len, options_t *options)
{
    int fd;
    int retval;

    struct spi_ioc_transfer transfer = {
        .tx_buf = (uintptr_t) in,
        .rx_buf = (uintptr_t) out,
        .len = (unsigned int) len,
        .delay_usecs = 0,
        .bits_per_word = 8,
        .cs_change = 1,
        .tx_nbits = 8,
        .rx_nbits = 8,
        .pad = 0
    };

    fd = open(options->dev, O_RDWR);

    if(fd < 0) {
        snprintf(msg, sizeof(msg) - 1, "Error opening [%s]", options->dev);
        perror(msg);
        spiguy_error = 1;
        return EXIT_FAILURE;
    }

    if(options->speed != -1) {
        if(spiguy_set_spi_speed(fd, (uint32_t) options->speed) != 0) {
            return EXIT_FAILURE;
        }
    }

    if(options->mode != -1) {
        if(spiguy_set_spi_mode(fd, (uint8_t) options->mode) != 0) {
            return EXIT_FAILURE;
        }
    }

    if(options->gpio != -1) {
        spiguy_gpio_prepare(options);
        spiguy_file_write(value_file, "1", (size_t) 1);
    }

    if(spiguy_error == 0) {
        retval = ioctl(fd, SPI_IOC_MESSAGE(1), &transfer);
    }

    if(options->gpio != -1) {
        spiguy_file_write(value_file, "0", (size_t) 1);
        if(options->unexport) {
            spiguy_gpio_release();
        }
    }

    if(retval < 0) {
        snprintf(msg, sizeof(msg) - 1, "Failed to complete transfer ioctl "
                 "[code %d]", retval);
        perror(msg);
        spiguy_error = 1;
        return EXIT_FAILURE;
    } else {
        retval = 0;
    }

    close(fd);
    return retval;
}
