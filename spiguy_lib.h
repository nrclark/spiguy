#ifndef _SPIGUY_LIB_H_
#define _SPIGUY_LIB_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef struct options_t {
    int8_t mode;
    int8_t polarity;
    int16_t gpio;
    int32_t speed;
    char *dev;
    size_t maxsize;
    bool keep_export;
} options_t;

int spiguy_run_transfer(char *in, char *out, size_t len, options_t *options);

/*----------------------------------------------------------------------------*/

void spiguy_gpio_prepare(options_t *options);
void spiguy_gpio_release(void);

int spiguy_set_spi_speed(int fd, uint32_t speed);
int spiguy_set_spi_mode(int fd, uint8_t mode);

size_t spiguy_file_read(const char *name, char *dest, size_t maxcount);
int spiguy_file_write(const char *name, const char *source, size_t count);

#endif
