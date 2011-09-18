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

int readone(int fd, long max_usec)
{
    fd_set readfds;
    struct timeval tv;
    int rc;
    unsigned char c;

    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);

    tv.tv_sec = 0;
    tv.tv_usec = max_usec;

    select(fd + 1, &readfds, NULL, NULL, &tv);

    rc = read(fd, &c, sizeof(c));
    if (rc == 1)
        return c;

    return -1;
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
        write(fd, aok, 4);
        if (readone(fd, max_delay) == 'O' &&
            readone(fd, max_delay) == 'K' &&
            readone(fd, max_delay) == '\n')
            return 0;
        flush_buffer(fd);
    }

    return -1;

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
            write(fd, out, 4);
        }
        if (getok(fd, 1, 100 * 1000) != 0)
            fprintf(stderr, "Error getting okay during init\n");
    }

    if (g_verbose)
        printf("done.\n");

}

void usage(char *argv0)
{
    printf("%s: [--init] [--verbose]\n", argv0);
    printf("Drive a string of GE lights connected to an Arduino.\n");
}


int main(int argc, char *argv[])
{
    int fd;
    char *sp;
    int c;
    int perform_init = 0;
    long writes = 0;
    int addr;
    int string;
    int bright;
    int red;
    int green;
    int blue;
    unsigned char out[4];

    static struct option long_options[] =
    {
        {"init",    no_argument,        NULL, 'i'},
        {"verbose", no_argument,        NULL, 'v'},
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
        c = getopt_long(argc, argv, "iva:s:b:r:g:l:", long_options, NULL);
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
                addr = atoi(optarg);
                break;
            case 's':
                string = atoi(optarg);
                break;
            case 'b':
                bright = atoi(optarg);
                break;
            case 'r':
                red = atoi(optarg);
                break;
            case 'g':
                green = atoi(optarg);
                break;
            case 'l':
                blue = atoi(optarg);
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
    else
    {
        build_bulb(out, string, addr, bright, red, green, blue, 0);
        write(fd, out, 4);
        writes++;
    }


#if defined(HACK_OLD_STUFF_OUT_FOR_NOW)
r = 1;
for (bright = 0; bright <= 0xcc; bright++)
for (addr = 0x19; addr <= 0x19; addr++)
{
for (string = 0; string < 1; string++)
{
build_bulb(out, string, addr, bright, 15, 0, 0, string == 0 ? 0 : 1);
write(fd, out, 4);
writes++;
r++;

printf("0x%x: 0x%x\n", addr, r);
//usleep(10 * 1000);
}
if (r > 15)
    r = 1;
if (getok(fd, 1, 100 * 1000) != 0)
    fprintf(stderr, "Error getting okay\n");

//usleep(10 * 1000);

for (string = 0; string < 1; string++)
{
build_bulb(out, string, addr, 0, 0, 0, 0, string == 0 ? 0 : 1);
write(fd, out, 4);
writes++;
}
}
flush_buffer(fd);
#if 0
    for (bright = 0xcc; bright <= 0xcc; bright += 0x40)
        for (r = 5; r < 16; r += 5)
            for (g = 5; g < 16; g += 5)
                for (b = 5; b < 16; b += 5)
                    for (addr = 0; addr < 36; addr++)
                        for (string = 0; string < 6; string++)
                        {
                            build_bulb(out, string, addr, bright, r, g, b, string == 5 ? 0 : 1);
                            write(fd, out, 4);
                            writes++;
                            if (getok(fd, 1, 100 * 1000) != 0)
                                fprintf(stderr, "Error getting okay\n");
                        }
#endif

#endif

    if (getok(fd, 1, 1000 * 1000) < 0)
        fprintf(stderr, "Error:  did not get closing ack\n");

    close(fd);

    if (g_verbose)
        printf("Wrote %ld bulbs\n", writes);

    return 0;
}
