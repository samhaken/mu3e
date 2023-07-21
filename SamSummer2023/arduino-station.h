// author: A. Loreti phys2114.ox.ac.uk ok20686@bristol.ac.uk
// date: May-June 2021

// list of references
// - https://www.cmrr.umn.edu/~strupp/serial.html#3_1
// - https://github.com/todbot/arduino-serial
// -
// https://blog.mbedded.ninja/programming/operating-systems/linux/linux-serial-ports-using-c-cpp/#basic-setup-in-c

#ifndef ARDUINOSTATION_H
#define ARDUINOSTATION_H

#include <errno.h>  // Error handling
#include <fcntl.h>  // File controls  e.g., O_RDWR
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>  // POSIX terminal control definitions
#include <unistd.h>   // write(), read(), close()

#include <fstream>
#include <iostream>

#include "vector"
#endif

#define BYTES 1

//-- GENERAL METHODS FOR SERIAL PORT -------------------------------//

// - Compares entries of buf array with a const char, if different, stores them
// into a tmp buffer.
// - parameters:
//     - buf: array of mixed alphanumerical chars transmitted by arduino.
//     - current index position in the buf array.
//     - input const char M.
inline double get_value1(char* buf, int buf_length, int& i, char M) {
    char buf_tmp[9] = "";
    int L = sizeof(buf_tmp) / sizeof(buf_tmp[0]);
    int j = 0;
    // do we need 43-44? it's repeated inside the while loop
    i++;
    if ((buf[i] == 'H') || (buf[i] == 'T')) i++;
    while (buf[i] != M) {
        if ((buf[i] == 'H') || (buf[i] == 'T')) i++;
        buf_tmp[j] = buf[i];
        j++;
        i++;
        // this loop was breaking - for some reason buffer size was being read
        // as 8. Resolved: replaced size of buffer with length of buffer
        if ((i > buf_length) || (j > L)) break;
        if ((buf[i] == 'K') || (buf[i] == 'N')) break;
    }
    double buffer_t = atof(buf_tmp);
    return buffer_t;
}

// Stores numerical values read from a buffer string inside a vector v.
// - parameters
//     - read_buf: array of mixed alphanumerical char transmitted by arduino.
//     - v: vector that stores numerical values found in buf.
// - error code: 0 successfull, 1 error. TODO:???
inline void strip_stringAd(char* read_buf, int buf_length, std::vector<double>& v) {
    int i = 0;
    if (read_buf[0] == 'T') {
        double v_0 = get_value1(read_buf, buf_length, i, 'F'),
               v_1 = get_value1(read_buf, buf_length, i, 'P'),
               v_2 = get_value1(read_buf, buf_length, i, 'A'),
               v_3 = get_value1(read_buf, buf_length, i, 'S'),
               v_4 = get_value1(read_buf, buf_length, i, 'R'),
               v_5 = get_value1(read_buf, buf_length, i, 'A'),
               v_6 = get_value1(read_buf, buf_length, i, 'V'),
               v_7 = get_value1(read_buf, buf_length, i, 'C'),
               v_8 = get_value1(read_buf, buf_length, i, 'N');

        v.push_back(v_0);
        v.push_back(v_1);
        v.push_back(v_2);
        v.push_back(v_3);
        v.push_back(v_4);
        v.push_back(v_5);
        v.push_back(v_6);
        v.push_back(v_7);
        v.push_back(v_8);

        // doesn't make sense to create the vector just to destroy it lol
        // double v0 = v[0], v1 = v[1], v2 = v[2], v3 = v[3], v4 = v[4], v5 =
        // v[5], v6 = v[6];
        // doesn't let you cout  vector elements, apparently
        // can use cout<< v.at(i)

        // cout << v[0] << '\t' << v[1] << '\t' << v[2] << '\t' <<  v[3] << '\t'
        // << v[4] << '\t' << v[5] << '\t' << v[6] <<  endl;
    }
    return;
}

//-- SETTING ARDUINO SERIAL PORT COMMUNICATION ------------------------------
// - RDWR: write and read
// - O_NOCTTY: tell Unix not to pass any keyboard stroke to our port
// - Other options: O_NDELAY O_NONBLOCKto set no delay in my port
// - communication and non-block mode
// - tty: termios object for handling ttys
// - tcgetattr(): get current options for the port
// - cfsetispeed()/cfsetospeed(): set baud rate

bool portOpen = false;

int setting_arduino() {
    struct termios tty;
    int serial_port = open("/dev/ttyACM0", O_RDWR | O_NOCTTY);
    if (serial_port < 0) {
        printf("Error %i from open: %s\n", errno, strerror(errno));
        portOpen = false;
    } else {
        portOpen = true;
        std::cout << "Port opened successfully " << std::endl;
    }
    if (tcgetattr(serial_port, &tty) != 0) {
        std::cout << " Error when reading port attributes " << std::endl;
    } else {
        cfsetispeed(&tty, B9600);
        cfsetospeed(&tty, B9600);

        // Enable receiver
        tty.c_cflag |= (CLOCAL | CREAD);

        // Disables bit-parity checks
        tty.c_cflag &= ~PARENB;
        tty.c_cflag &= ~PARENB;
        tty.c_cflag &= ~CSTOPB;
        tty.c_cflag &= ~CSIZE;
        tty.c_cflag |= CS8;  // 8 bits per byte , valid for our ASCII data

        // Disables two stop bits, only one will be used
        tty.c_cflag &= ~CSTOPB;

        // Canonical mode and other echo modes
        // Canonical mode disabled or the VTIME and
        // VMIN will be ignored
        tty.c_lflag &= ~(ECHO | ECHOE | ISIG);
        tty.c_lflag &= ~ICANON;

        // Timeout
        // UNSET F_SETFL allows VTIME and VMIN to be used
        fcntl(serial_port, F_SETFL, 0);

        // Time to wait for every call read in unit of 1/10 sec.
        tty.c_cc[VTIME] = 1;

        // Minimum number of characters to read. If zero, the read
        // call will not wait for any block to be read
        tty.c_cc[VMIN] = BYTES;

        // Upload the new settings in our protocol PORT.
        tcsetattr(serial_port, TCSANOW, &tty);
    }
    return serial_port;
}

// -- I/O ----------------------------------------------------------
// Write bytes into arduino
// - parameters:
//     - serial_port: file id for the current serial port
//     - data: data to write
//     - data_size
void write_data(int serial_port, unsigned char data[], int data_size) {
    if (write(serial_port, data, data_size) != data_size) {
        std::cout << "Error in writing bytes" << std::endl;
    }
    return;
}

// Read bytes until a flag character is met
// - parameters:
//     - fd: serial port file id
//     - buf: buffer array that stores bytes
//     - until: terminating byte
//     - buf_max: max nr of bytes to read
// - timeout(sec): wait for first byte to arrive then leave
// - read expects an array, so we give it a 1-byte array
inline int serialport_read_until(int fd, char* buf, char until, int buf_max) {
    char b[1];
    int i = 0;
    do {
        // read a char at a time
        int n = read(fd, b, BYTES);
        if (n == -1) {
            std::cout << "Couldn't read port" << std::endl;
            return 1;
        }
        if (n == 0) {
            std::cout << "No bytes available at the moment " << std::endl;
            return 1;
        }
        if (n == 1) {
            buf[i] = b[0];
            i++;
        }
    } while (b[0] != until && i < buf_max);
    buf[i] = 0;  // null terminate the string
    return 0;
}

// read_data1()
// - read bytes from serial port
// Scroll through what's already in the buffer up to a new-line character
// char dump[255] = {};
// serialport_read_until(serial_port, dump, '\n', sizeof(dump));
int read_data1(int serial_port, std::vector<double>& v) {
    tcflush(serial_port, TCIOFLUSH);
    char read_buf[51] = {};
    int err1 =
        serialport_read_until(serial_port, read_buf, '\n', sizeof(read_buf));
    int err2 = 0;
    int buf_length = strlen(read_buf);
    strip_stringAd(read_buf, buf_length, v);
    return err1 | err2;
}
