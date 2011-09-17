#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>

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

void build_bulb(unsigned char *out, unsigned char string, unsigned char addr, 
            unsigned char bright, unsigned char r, unsigned char g, unsigned char b, int more_bulbs)
{
    *out++ = (b & 0x0F) | (string << 4);
    *out++ = (g << 4) | (r & 0xF);
    *out++ = (more_bulbs ? 0x80 : 0x00) | (addr & 0x3F);
    *out++ = bright;
}

int getok(int fd, long max_delay)
{
    unsigned char aok[4] = {0x80,'O','K','\n'};
    int rc;
    char buf[4096];
    long delays = 0;

    while (delays < max_delay)
    {
        write(fd, aok, 4);
        usleep(1000);
        rc = read(fd, buf, sizeof(buf));
        if (rc >= 2 && buf[0] == aok[1] && buf[1] == aok[2])
            return 0;

        delays += 100 * 1000;
    }

    return -1;

}

void flush_buffer(int fd)
{
    int rc;
    char buf[32000];
    rc = read(fd, buf, sizeof(buf));
    if (rc > 0)
        write(STDOUT_FILENO, buf, rc);
}

int main(int argc, char *argv[])
{
    int fd;
    unsigned char out[4];
    int string;
    int addr;
    int bright;
    int r;
    int g;
    int b;
    long writes = 0;

    fd = open_port("/dev/ttyUSB0");

    if (getok(fd, 1000 * 1000) < 0)
    {
        fprintf(stderr, "Error:  could not get an ok back\n");
        return -1;
    }

    for (bright = 0x40; bright <= 0xcc; bright += 0x40)
        for (r = 5; r < 16; r += 5)
            for (g = 5; g < 16; g += 5)
                for (b = 5; b < 16; b += 5)
                    for (addr = 0; addr < 2; addr++)
                        for (string = 0; string < 6; string++)
                        {
                            build_bulb(out, string, addr, bright, r, g, b, string == 5 ? 0 : 1);
                            write(fd, out, 4);
                            writes++;
                            getok(fd, 100 * 1000);
                            //usleep(1000);
                        }

    flush_buffer(fd);
    if (getok(fd, 1000 * 1000) < 0)
        fprintf(stderr, "Error:  did not get closing ack\n");

    flush_buffer(fd);

    close(fd);

    printf("Wrote %ld bulbs\n", writes);

    return 0;
}
