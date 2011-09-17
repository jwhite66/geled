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

int main(int argc, char *argv[])
{
    int fd;
    char buf[32000];
    unsigned char fourgreen[4];
    unsigned char aok[4] = {0x80,'O','K','\n'};
    int rc;
    int i;
    fd = open_port("/dev/ttyUSB0");

    write(fd, aok, 4);

    fourgreen[0] = 0 << 4;
    fourgreen[1] = 0;
    fourgreen[2] = 4;
    fourgreen[3] = 0xCC;

    for (i = 0; i < 36; i++)
    {
        fourgreen[0] = 0 << 4;
        fourgreen[1] = 0;
        fourgreen[2] = 0x80 & i;
        fourgreen[3] = 0xCC;
        write(fd, fourgreen, 4);
        fourgreen[0] = 1 << 4;
        fourgreen[2] = i;
        write(fd, fourgreen, 4);
        usleep(100 * 1000);
    }

    for (i = 0; i < 36; i++)
    {
        fourgreen[0] = 0 << 4;
        fourgreen[2] = 0x80 & i;
        fourgreen[1] = 15 << 4;
        write(fd, fourgreen, 4);
        fourgreen[0] = 1 << 4;
        fourgreen[2] = i;
        write(fd, fourgreen, 4);
        usleep(100 * 1000);
    }

    /*------------------------------------------------------------------------
    **  Wait for a while to gather up any output, and then get + display it.
    **----------------------------------------------------------------------*/
    usleep(500 * 1000);

    rc = read(fd, buf, sizeof(buf));
    if (rc > 0)
        write(STDOUT_FILENO, buf, rc);
    return 0;
}
