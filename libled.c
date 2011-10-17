#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>
#include <stdlib.h>
#include <libconfig.h>


typedef struct
{
    int string;
    int addr;
} bulb_map_t;

typedef struct
{
    int fd;
    const char *portname;
    unsigned long writes;
    int confirm_every;
    config_t config;
    long height;
    long width;
    bulb_map_t *bulb_map;
} serial_t;

#define LED_HANDLE_T serial_t *

#include "led.h"

static int open_port(const char *port_name)
{
    int fd;
    struct termios options;

    fd = open(port_name, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1)
    {
        char errbuf[256];
        sprintf(errbuf, "open port (%s): ", port_name);
        perror(errbuf);
        return -1;
    }

    fcntl(fd, F_SETFL, FNDELAY);


    tcgetattr(fd, &options);
    options.c_iflag = options.c_oflag = options.c_lflag = 0;

    cfsetispeed(&options, B115200);
    cfsetospeed(&options, B115200);
    options.c_cflag |= (CLOCAL | CREAD);

    /*     No parity (8N1) */
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;

    tcsetattr(fd, TCSANOW, &options);

    return (fd);
}

static void flush_buffer(int fd)
{
    int rc;
    char buf[2048];
    memset(buf, 0, sizeof(buf));
    rc = read(fd, buf, sizeof(buf));
    if (rc > 0)
        write(STDOUT_FILENO, buf, rc);
}


static int findone(int fd, int target, long max_usec)
{
    fd_set readfds;
    struct timeval tv;
    int rc;
    unsigned char c;

    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);

    tv.tv_sec = 0;
    tv.tv_usec = max_usec;

    while (1)
    {
        select(fd + 1, &readfds, NULL, NULL, &tv);

        rc = read(fd, &c, sizeof(c));
        if (rc != 1)
            break;

        if (c == target)
            return 1;
        else
            write(STDOUT_FILENO, &c, 1);
    }

    return -1;
}

static int raw_writebuf(int fd, unsigned char *out, int size)
{
    int rc;

    while (size > 0)
    {
        rc = write(fd, out, size);
        if (rc < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                continue;

            perror("writebuf: ");
            return -1;
        }

        out += rc;
        size -= rc;
    }

    return 0;
}

static int getok(int fd, int max_tries, long max_delay)
{
    unsigned char aok[4] = {COMMAND_ACK,'O','K','\n'};
    int i;

    for (i = 0; i < max_tries; i++)
    {
        if (raw_writebuf(fd, aok, 4) != 0)
            return -1;
        if (findone(fd, 'O', max_delay) &&
            findone(fd, 'K', max_delay) &&
            findone(fd, '\n', max_delay))
            return 0;
        flush_buffer(fd);
    }

    return -1;

}


static void writebuf(serial_t *ser, unsigned char *out, int size)
{
    if (raw_writebuf(ser->fd, out, size) == 0)
    {
        ser->writes++;
        if (ser->confirm_every > 0)
            if (ser->writes % ser->confirm_every == 0)
                if (getok(ser->fd, 1, 1000 * 1000) < 0)
                    fprintf(stderr, "Error:  did not get periodic ack\n");
    }
}

void build_bulb(unsigned char *out, unsigned char string, unsigned char addr,
            unsigned char bright, unsigned char r, unsigned char g, unsigned char b, int more_bulbs)
{
    BULB_FLAG_ADDRESS(out) = addr;
    if (more_bulbs)
        BULB_FLAG_ADDRESS(out) |= BULB_FLAG_COMBINE;
    BULB_BLUE_STRING(out) = (b << 4) | string;
    BULB_GREEN_RED(out) = (g << 4) | (r & 0xF);
    BULB_BRIGHT(out) = bright;
}

int get_one_setting(config_setting_t *setting, const char *name, long *out)
{
    if (config_setting_lookup_int(setting, name, out) != CONFIG_TRUE)
    {
        fprintf(stderr, "Error:  no setting '%s' on string %s in config file.\n", name, config_setting_name(setting));
        return 1;
    }
    return 0;
}



void compute_x_y(int addr, long start, long end, long fromx, long fromy, long tox, long toy, int *x, int *y)
{
    int total = end - start + 1;
    int width = tox - fromx + 1;
    int height = total / width;

    int col = addr / height;
    int rowup;
    if (col % 2 == 0)
        rowup = addr % height;
    else
        rowup = height - (addr % height) - 1;

    *x = fromx + col;
    *y = fromy - rowup;
printf("Computed %dx%d for [addr %d|start %ld|end %ld|fromx %ld|fromy %ld|tox %ld|toy %ld]\n",
  *x, *y, addr, start, end, fromx, fromy, tox, toy);
}



serial_t *led_init(void)
{
    serial_t *ser = malloc(sizeof(*ser));
    memset(ser, 0, sizeof(*ser));

    config_init(&ser->config);
    config_read_file(&ser->config, LED_CFG_FILE);

    config_lookup_int(&ser->config, "display/height", &ser->height);
    config_lookup_int(&ser->config, "display/width", &ser->width);

    if (ser->height == 0 || ser->width == 0)
    {
        fprintf(stderr, "Error:  width and height not given in cfg file.\n");
        return NULL;
    }

    ser->bulb_map = malloc(ser->height * ser->width * sizeof(*ser->bulb_map));
    if (! ser->bulb_map)
    {
        fprintf(stderr, "Error:  Cannot allocat %ldx%ld map.\n", ser->width, ser->height);
        return NULL;
    }
    memset(ser->bulb_map, 0, ser->height * ser->width * sizeof(*ser->bulb_map));

    config_setting_t *strings = config_setting_get_member(config_root_setting(&ser->config), "strings");
    if (strings)
    {
        int i;
        for (i = 0; i < STRING_COUNT; i++)
        {
            long string, start, end, fromx, fromy, tox, toy;
            int x, y, addr;
            config_setting_t *str = config_setting_get_elem(strings, i);
            if (! str)
                break;

            if (get_one_setting(str, "string", &string)) continue;
            if (get_one_setting(str, "start", &start)) continue;
            if (get_one_setting(str, "end", &end)) continue;
            if (get_one_setting(str, "fromx", &fromx)) continue;
            if (get_one_setting(str, "fromy", &fromy)) continue;
            if (get_one_setting(str, "tox", &tox)) continue;
            if (get_one_setting(str, "toy", &toy)) continue;

            for (addr = start; addr <= end; addr++)
            {
                compute_x_y(addr, start, end, fromx, fromy, tox, toy, &x, &y);
                (ser->bulb_map + ((y * ser->width) + x))->string = string;
                (ser->bulb_map + ((y * ser->width) + x))->addr = addr;
            }
        }
    }

    ser->portname = "/dev/ttyUSB0";
    config_lookup_string(&ser->config, "ttyname", &ser->portname);

    ser->fd = open_port(ser->portname);
    if (ser->fd == -1)
        return NULL;

    return ser;
}


void led_get_size(serial_t *ser, int *wide, int *high)
{
    *wide = ser->width;
    *high = ser->height;
}

void led_set_pixel(serial_t *ser, int x, int y, int bright, int r, int g, int b)
{
    unsigned char buf[4];
    bulb_map_t *bulb = ser->bulb_map + (y * ser->width) + x;
    build_bulb(buf, bulb->string, bulb->addr, bright, r, g, b, 0);
    writebuf(ser, buf, sizeof(buf));
    printf("x %d, y %x - [String %d|Addr %d|bright 0x%x|r 0x%x|g 0x%x|b 0x%x]\n",
        x, y, bulb->string, bulb->addr, bright, r, g, b);
}

void led_term(serial_t *ser)
{
    free(ser);
}
