#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <getopt.h>
#include <stdlib.h>

#include "led.h"

int g_verbose = 0;


int open_port(char *port_name)
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

int findone(int fd, int target, long max_usec)
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


void writebuf(int fd, unsigned char *out, int size)
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
            return;
        }

        out += rc;
        size -= rc;
    }
}

void build_bulb(unsigned char *out, unsigned char string, unsigned char addr, 
            unsigned char bright, unsigned char r, unsigned char g, unsigned char b, int more_bulbs)
{
    *out++ = (b & 0x0F) | (string << 4);
    *out++ = (g << 4) | (r & 0xF);
    *out++ = (more_bulbs ? 0x80 : 0x00) | (addr & 0x3F);
    *out++ = bright;
}

void flush_buffer(int fd)
{
    int rc;
    char buf[32000];
    memset(buf, 0, sizeof(buf));
    rc = read(fd, buf, sizeof(buf));
    if (rc > 0)
        write(STDOUT_FILENO, buf, rc);
}


int getok(int fd, int max_tries, long max_delay)
{
    unsigned char aok[4] = {0x80,'O','K','\n'};
    int i;

    for (i = 0; i < max_tries; i++)
    {
        writebuf(fd, aok, 4);
        if (findone(fd, 'O', max_delay) &&
            findone(fd, 'K', max_delay) &&
            findone(fd, '\n', max_delay))
            return 0;
        flush_buffer(fd);
    }

    return -1;

}

void perform_cmd(int fd, int cmd)
{
    unsigned char out[4];
    memset(out, 0, sizeof(out));
    out[0] = cmd;
    writebuf(fd, out, sizeof(out));

    if (getok(fd, 5, 1000 * 1000) != 0)
        fprintf(stderr, "Error getting okay during cmd\n");
}

void init(int fd)
{
    int string;
    int addr;
    unsigned char out[4];

    if (g_verbose)
        printf("Initializing strings..");

    for (addr = 0; addr < ADDR_COUNT; addr++)
    {
        for (string = 0; string < STRING_COUNT; string++)
        {
            build_bulb(out, string, addr, 0, 0, 0, 0, string == (STRING_COUNT - 1) ? 0 : 1);
            writebuf(fd, out, 4);
        }
        if (getok(fd, 1, 100 * 1000) != 0)
            fprintf(stderr, "Error getting okay during init\n");
    }

    if (g_verbose)
        printf("done.\n");

}

void perform_custom(int fd, char *tag)
{
    int string;
    int addr;
    int red;
    int i;
    unsigned char out[4];

    if (strcmp(tag, "hammer") == 0)
    {
        build_bulb(out, 0, 0, 0xcc, 0, 0, 13, 0);
        for (i = 0; i < 6000; i++)
        {
            writebuf(fd, out, 4);
            flush_buffer(fd);
        }
    }

    if (strcmp(tag, "one") == 0)
    {
        build_bulb(out, 0, 0, 0xcc, 15, 0, 0, 0);
        writebuf(fd, out, 4);
        for (red = 0; red < 500; red++)
        {
            flush_buffer(fd);
            usleep(1000);
        }
    }

    if (strcmp(tag, "red") == 0)
    {
        while (1)
        {
            for (red = 0; red <= 15; red++)
            {
                for (addr = 0; addr <= 35; addr++)
                {
                    for (string = 0; string <= 5; string++)
                    {
                        build_bulb(out, string, addr, 0xcc, red, 0, 0, string == 5 ? 0 : 1);
                        writebuf(fd, out, 4);
                    }
                }
            }
        }
    }
}

void usage(char *argv0)
{
    printf("%s: [--init] [--verbose] [--delay=usecs] [--custom=tag] [--cmd=n]\n", argv0);
    printf("%*.*s", strlen(argv0), strlen(argv0), " ");
    printf( "     [--string=n[-m]] [--addr=n[-m]] [--bright=n[-m]] [--red=n[-m]] [--green=n[-m]] [--blue=n[-m]]\n");
    printf("Drive a string of GE lights connected to an Arduino.\n");
}

void parse_range(char *r, int *begin, int *end)
{
    *begin = *end = atoi(r);
    while (*r && *r != '-')
        r++;
    if (*r)
        *end = atoi(r + 1);
}


int main(int argc, char *argv[])
{
    int fd;
    char *sp;
    int c;
    int perform_init = 0;
    long writes = 0;
    int addr, addr_start = 0, addr_end = 0;
    int string, string_start = 0, string_end = 0;
    int bright, bright_start = MAX_BRIGHT, bright_end = MAX_BRIGHT;
    int red, red_start = 0, red_end = 0;
    int green, green_start = 0, green_end = 0;
    int blue, blue_start = 0, blue_end = 0;
    int cmd = 0;
    unsigned long delay = 100;
    char custom[256] = {0};
    unsigned char out[4];

    static struct option long_options[] =
    {
        {"init",    no_argument,        NULL, 'i'},
        {"verbose", no_argument,        NULL, 'v'},
        {"delay",   required_argument,  NULL, 'd'},
        {"custom",  required_argument,  NULL, 'c'},
        {"cmd",     required_argument,  NULL, 'm'},
        {"addr",    required_argument,  NULL, 'a'},
        {"string",  required_argument,  NULL, 's'},
        {"bright",  required_argument,  NULL, 'b'},
        {"red",     required_argument,  NULL, 'r'},
        {"green",   required_argument,  NULL, 'g'},
        {"blue",    required_argument,  NULL, 'l'},
        {0, 0, 0, 0}
    };


    if (argc <= 1)
    {
        usage(argv[0]);
        return(-1);
    }

    while (1)
    {
        c = getopt_long(argc, argv, "iva:s:b:r:g:l:d:c:", long_options, NULL);
        if (c == -1)
            break;

        switch(c)
        {
            case 'i':
                perform_init = 1;
                break;
            case 'v':
                g_verbose = 1;
                break;
            case 'a':
                parse_range(optarg, &addr_start, &addr_end);
                break;
            case 's':
                parse_range(optarg, &string_start, &string_end);
                break;
            case 'b':
                parse_range(optarg, &bright_start, &bright_end);
                break;
            case 'r':
                parse_range(optarg, &red_start, &red_end);
                break;
            case 'g':
                parse_range(optarg, &green_start, &green_end);
                break;
            case 'l':
                parse_range(optarg, &blue_start, &blue_end);
                break;
            case 'd':
                delay = atoi(optarg);
                break;
            case 'm':
                cmd = atoi(optarg);
                break;
            case 'c':
                strcpy(custom, optarg);
                break;
            default:
                usage(argv[0]);
                return(-1);
        }
    }

    /*------------------------------------------------------------------------
    **  Connect to the arduino
    **----------------------------------------------------------------------*/
    sp = getenv("ARDUINO_SERIAL_PORT");
    if (!sp)
        sp = DEFAULT_ARDUINO_SERIAL_PORT;

    fd = open_port(sp);
    if (fd <= 0)
    {
        char buf[256];
        sprintf(buf, "Error opening %s:", sp);
        perror(buf);
    }

    if (getok(fd, 5, 1000 * 1000) < 0)
    {
        fprintf(stderr, "Error:  could not get an ok back\n");
        return -1;
    }

    flush_buffer(fd);

    if (perform_init)
        init(fd);
    else if (cmd > 0)
    {
        perform_cmd(fd, cmd);
        if (g_verbose)
            printf("Action done\n");
    }
    else if (strlen(custom) > 0)
        perform_custom(fd, custom);
    else
    {
        for (addr = addr_start; addr <= addr_end; addr++)
            for (bright = bright_start; bright <= bright_end; bright++)
                for (red = red_start; red <= red_end; red++)
                    for (green = green_start; green <= green_end; green++)
                        for (blue = blue_start; blue <= blue_end; blue++)
                        {
                            for (string = string_start; string <= string_end; string++)
                            {
                                build_bulb(out, string, addr, bright, red, green, blue, string == string_end ? 0 : 1);
                                writebuf(fd, out, 4);
                                writes++;
                            }

                            if (delay > 0)
                                usleep(delay);
                        }
    }

    flush_buffer(fd);

    if (getok(fd, 1, 1000 * 1000) < 0)
        fprintf(stderr, "Error:  did not get closing ack\n");

    close(fd);

    if (g_verbose)
        printf("Wrote %ld bulbs\n", writes);

    return 0;
}
