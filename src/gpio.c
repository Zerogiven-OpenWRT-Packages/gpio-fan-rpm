// gpio.c
// GPIO setup, export, and base detection

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include "gpio-fan-rpm.h"

#define SYSFS_GPIO_DIR "/sys/class/gpio"
#define MAX_BUF 128

// Read GPIO base from a given chip name
int read_gpio_base(const char *chip)
{
    if (!chip || chip[0] == 0)
        return -1;

    char path[MAX_BUF];
    snprintf(path, sizeof(path), SYSFS_GPIO_DIR "/%s/base", chip);
    FILE *f = fopen(path, "r");

    if (!f) {
        perror("fopen (read_gpio_base)");
        return -1;
    }

    int base = -1;
    fscanf(f, "%d", &base);
    fclose(f);

    return base;
}

// Try to find the first gpiochip and return its base and name
int default_gpio_base(char *chip_buf, size_t len)
{
    DIR *dir = opendir(SYSFS_GPIO_DIR);
    if (!dir) {
        perror("opendir (default_gpio_base)");
        return -1;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (strncmp(entry->d_name, "gpiochip", 8) == 0)
        {
            strncpy(chip_buf, entry->d_name, len);
            chip_buf[len - 1] = '\0';
            closedir(dir);
            return read_gpio_base(entry->d_name);
        }
    }

    closedir(dir);
    fprintf(stderr, "[ERROR] No gpiochip found in %s\n", SYSFS_GPIO_DIR);
    return -1;
}

// Export a GPIO (if not already exported)
int export_gpio(int gpio_abs)
{
    char path[MAX_BUF];
    snprintf(path, sizeof(path), SYSFS_GPIO_DIR "/gpio%d", gpio_abs);
    if (access(path, F_OK) == 0)
        return 0;

    int fd = open(SYSFS_GPIO_DIR "/export", O_WRONLY);
    if (fd < 0) {
        perror("open (export)");
        return -1;
    }

    char buf[16];
    snprintf(buf, sizeof(buf), "%d", gpio_abs);
    if (write(fd, buf, strlen(buf)) < 0) {
        perror("write (export_gpio)");
        close(fd);
        return -1;
    }

    close(fd);
    usleep(100000);
    return 0;
}

// Set GPIO direction and edge detection
void setup_gpio(int gpio_abs)
{
    char path[MAX_BUF];
    int fd;

    snprintf(path, sizeof(path), SYSFS_GPIO_DIR "/gpio%d/direction", gpio_abs);
    fd = open(path, O_WRONLY);
    if (fd >= 0)
    {
        if (write(fd, "in", 2) < 0)
            perror("write (direction)");
        close(fd);
    }
    else
    {
        perror("open (direction)");
    }

    snprintf(path, sizeof(path), SYSFS_GPIO_DIR "/gpio%d/edge", gpio_abs);
    fd = open(path, O_WRONLY);
    if (fd >= 0)
    {
        if (write(fd, "falling", 7) < 0)
            perror("write (edge)");
        close(fd);
    }
    else
    {
        perror("open (edge)");
    }
}

int read_gpio(int gpio_abs) {
    // Simulate GPIO read (replace with actual GPIO read logic)
    // Return 1 for high signal, 0 for low signal
    return gpio_get_value(gpio_abs); // Example function call
}
