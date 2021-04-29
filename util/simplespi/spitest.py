#!/usr/bin/env python3
# Copyright lowRISC contributors.
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0
r"""Simple Tool for FPGA SPI experiments
"""

import argparse
import logging as log
import os
import subprocess
import sys
import time

import pkg_resources  # part of setuptools
from pyftdi.spi import SpiController


def show_and_exit(clitool, packages):
    util_path = os.path.dirname(os.path.realpath(clitool))
    os.chdir(util_path)
    ver = subprocess.run(
        ["git", "describe", "--always", "--dirty", "--broken"],
        stdout=subprocess.PIPE).stdout.strip().decode('ascii')
    if (ver == ''):
        ver = 'not found (not in Git repository?)'
    sys.stderr.write(clitool + " Git version " + ver + '\n')
    for p in packages:
        sys.stderr.write(p + ' ' + pkg_resources.require(p)[0].version + '\n')
    exit(0)


USAGE = """
    spitest [options] text [text ...]
"""

def int_to_bytes(x: int) -> bytes:
    return x.to_bytes((x.bit_length() + 7) // 8, 'little')

def main():
    done_stdin = False
    parser = argparse.ArgumentParser(
        prog="spitest",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        usage=USAGE,
        description=__doc__)
    parser.add_argument(
        '--version', action='store_true', help='Show version and exit')
    parser.add_argument(
        '-v',
        '--verbose',
        action='store_true',
        help='Verbose output during processing')
    parser.add_argument(
        '-f',
        '--flippy',
        action='store_true',
        help='Flip the SPI/JTAG control GPIO 10 times and exit')
    parser.add_argument(
        '-l',
        '--length',
        type=int,
        action='store',
        help='Construct and send a message of specified length')
    parser.add_argument(
        '-j',
        '--jtag',
        action='store_true',
        help='Set SPI/JTAG control to JTAG and exit')
    parser.add_argument(
        'message',
        nargs='*',
        metavar='input',
        default='1234',
        help='message to send in 4 byte chunks')
    parser.add_argument(
        '-i',
        '--input',
        nargs=1,
        metavar='/path/file.txt',
        action='store',
        help='Specify input with a file')
    parser.add_argument(
        '-r',
        '--receive',
        nargs=2,
        metavar='1024 32',
        action='store',
        help='Specify bytes to receive and bytes to trim')
    args = parser.parse_args()

    if args.version:
        show_and_exit(__file__, ["pyftdi"])

    if (args.verbose):
        log.basicConfig(format="%(levelname)s: %(message)s", level=log.DEBUG)
    else:
        log.basicConfig(format="%(levelname)s: %(message)s")

    # Instanciate a SPI controller
    spi = SpiController(cs_count=1)

    # interfaces start from 1 here, so this is Channel A (called 0 in jtag)
    spi.configure('ftdi://ftdi:2232h/1')

    # Get a port to a SPI device w/ /CS on A*BUS3 and SPI mode 0 @ 1MHz
    device = spi.get_port(cs=0, freq=1E6, mode=0)

    # Get GPIO port to manage extra pins
    # BUS4 = JTAG TRST_N, BUS5 = JTAG SRST_N, BUS6 = JTAG_SPIN
    # Note: something makes FTDI default to BUS6 low, selected that for SPI
    # otherwise SRST being default low holds the chip in reset
    # pyftdi Set Direction also forces the output to zero
    # so initially make SRST an input w/pullup in FPGA in case SPI/JTAG was
    # initially JTAG
    gpio = spi.get_gpio()
    gpio.set_direction(0x40, 0x40)
    time.sleep(1)
    gpio.set_direction(0x70, 0x70)

    if args.jtag:
        gpio.write(0x70)
        return

    gpio.write(0x30)

    if args.flippy:
        for i in range(10):
            print("Select SPI")
            gpio.write(0x30)
            time.sleep(2)
            print("Select JTAG")
            gpio.write(0x70)
            time.sleep(2)
        return
    
    print("Select SPI")
    gpio.write(0x30)
    if args.input:
        print("Opening "+args.input[0])
        with open(args.input[0], 'r') as fp:
            '''
            The code below sends data in `args.input[0]` file to SPI

            Every SPI exchange (simultaneous read and write) is 8 bytes
            Each patch is defined to be 128*8=1024 bytes
            We assume each line of fp is 8 bytes

            For every patch, except the first one, we will check the sanity 
            of the previous patch by comparing the echo returned from SPI

            The echo is computed by the first 8 bytes of every patch
            xor with (^) with 0x10101010

            The echo is not guaranteed to be read from SPI 
            when we exchange the first 8 bytes of a patch
            We will compare the echo during every exchange

            If every exchange mismatches the computed echo,
            then the last patch is not sent to SPI correctly
            Try increasing the wait time after every exchange
            '''
            cnt = 0
            echo_match_fifo =[]
            echo_checked = False
            for s in fp:

                s=s.strip('\n')
                write_buf = bytes.fromhex(s)
                #print("s", s)
                #print("write_buf", write_buf)
                read_buf = device.exchange(write_buf, duplex=True)
                
                if ((not echo_checked) and int(cnt/128)>0):
                    print(cnt, "Got " + str(read_buf), len(read_buf), read_buf.hex(), read_buf[:4].hex())
                    #this echo match is from the last patch
                    if (echo_match_fifo[0] == read_buf[:4]):
                        echo_checked = True
                        echo_match_fifo.pop(0)
                        
                    elif (cnt%128==127):
                        print("ERROR: echo mismatch!")
                        return
                    else:
                        #need to wait long enough so that 
                        #the time to send the next whole patch is greater than
                        #the time for SPI slave to write the previous patch to DDR
                        time.sleep(0.1) 

                if (cnt%128==127): echo_checked = False

                if (cnt%128==0):
                    #this calculates the echo_match to compare to the received echo during the next patch
                    #therefore, note this print will be 1 patch ahead of the "Got ..." prints

                    #echo_match_swp will need to flipped endianness
                    echo_match_swp = bytes(a ^ b for (a, b) in zip(write_buf[:8], bytes.fromhex("01010101")))
                    echo_match = bytes([c for t in zip(echo_match_swp[3::4], echo_match_swp[2::4], echo_match_swp[1::4], echo_match_swp[::4]) for c in t])
                    print("Patch", int(cnt/128), "s[:8]", s[:8], "echo_match", echo_match.hex())
                    echo_match_fifo.append(echo_match)
                    
                cnt+=1

        return

    if args.receive:
        print(int(args.receive[0])+int(args.receive[1]), "bytes will be written to receive.txt")
        with open("receive.txt", 'w') as fp:
            '''
            The code below reads data from SPI and writes to receive.txt
            It trims "extra data" from receive.txt and writes to receive_trim.txt
            For details of "extra data", see comments below

            Every SPI exchange (simultaneous read and write) is 8 bytes
            but we will actually send empty data (b"") to SPI
            Each patch is defined to be 128*8=1024 bytes
            We assume write lines of 8 bytes to receive.txt

            The read data is not guaranteed to be correct
            because the device might not be ready to send data
            Therefore we need to wait long enough after every exchange
            See comments of time.sleep(*) below

            We cannot automate this because only host can initiate an exchange
            There is no ready signal coming from device to let the host know
            '''
            bytes_left = int(args.receive[0])+int(args.receive[1])
            while (bytes_left > 0):
                bytes_read = min(bytes_left, 1024)
                print("read", bytes_read, "bytes")
                for i in range(int(bytes_read/8)):
                    #read 8 bytes
                    read_buf = device.exchange(b"", 8, duplex=True)

                    #flip endianness
                    read_buf_swp = b""
                    for s in read_buf:
                        read_buf_swp = bytes([s]) + read_buf_swp #reverse every byte
                    
                    fp.write(read_buf_swp.hex()+"\n")

                bytes_left -= bytes_read

                #needs to wait long enough so that the device (Opentitan) 
                #has prepared the data to send
                #need to wait >1 when we print out many lines in UART
                #i.e. "diff weight_512.txt receive_trim.txt" to check
                #i.e. print out ddr data in UART and manually check the receive.txt
                time.sleep(0.1) 
        
        print(args.receive[0], "bytes will be written to receive.txt")
        with open("receive.txt", 'r') as reader, open("receive_trim.txt", 'w') as writer:
            '''
            Need to trim the received data
            The hardware writes all the ddr_miso data into dmem
            Ibex then reads all data from dmem and sends to SPI

            All dmem data include: (in this order)
            1. ddr write ack data 
                they are response signals when we send ddr_mosi requests
                hardware writes them to dmem by default
            2. defined data that are previosuly stored in ddr
            3. random data not initialized in ddr

            Therefore, we might want to trim ddr write ack data (1)
            and extra data (3)
            '''

            #assuming we write to ddr in patches of 1024 bytes
            #then we have <trim_head> lines of "0000000000000001"
            trim_head = int(int(args.receive[0])/1024)

            #each line has 8 bytes
            trim_tail = int(int(args.receive[0])/8 + trim_head)

            raw = reader.readlines()
            writer.writelines(raw[trim_head:trim_tail-1])
            #avoid writing newline at the last line
            writer.writelines(raw[trim_tail-1].rstrip())

        return

    print("Select SPI")
    gpio.write(0x30)
    # Synchronous exchange with the remote SPI device
    if args.length:
        s = ''
        for i in range(args.length):
            s += hex(i & 15)[-1]
    else:
        s = ''
        for m in args.message:
            s += m + ' '
            s = s[:-1]  # remove extra space put on end
        # pad to ensure multiple of 4 bytes
        filled = len(s) % 4
        if filled:
            s += '....' [filled:]

    while len(s):
        write_buf = bytes(s[:4], encoding='utf8')
        read_buf = device.exchange(write_buf, duplex=True)
        print("Got " + str(read_buf))
        s = s[4:]


if __name__ == '__main__':
    main()
