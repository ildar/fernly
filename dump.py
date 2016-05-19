#!/usr/bin/env python
#
# Copyright (C) 2016  Stefan Mandl
#
# A dump Tool for Rephone
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful, but WITHOUT 
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc., 51 Franklin
# Street, Fifth Floor, Boston, MA 02110-1301 USA.

# uncomment #define AUTOMATED in main.mt2502.c
# and rebuild 

from time import sleep
import argparse
import os
import serial
import struct
import sys
import time
import traceback

class DumpTool(object):
   
    
    def __init__(self):
        
        print 'Dump the mt2502'
  
        
    # open the comport
    def open(self, port):
        
        print 'Try to open port %s. Press ctrl+c for break' % port
        while 1:
            try:
                self.ser = serial.Serial(port, 115200, timeout=5)
                self.ser.flushInput()
                self.ser.flushOutput()
                print 'Connect to Port %s' % port
                break
            except:
                time.sleep(0.1)
                continue
 
    
    def close(self):
        self.ser.close()
        
  
    def Sync(self):
        
        while 1:
            val = self.ser.read(1)
            if val == 'k':
                break
            self.ser.write('0')
    
    # dump with 32 bit access
    def DumpMem(self, filename, address, count):
      
        
        # Read.  Format: r[otf]aaaaaaaa
        # otf -> read One, Two, or Four bytes
        # a.. -> address to read
       
        f = file(filename, 'wb')
        while count != 0: 
            
            self.ser.write('rf')
            h = "{:x}".format(address) 
            self.ser.write(h)
            val = self.ser.read(8)
            value = int(val, 16)
            f.write(struct.pack('<L', value))
            print '0x%x' % value
            address = address + 4
            count = count - 1
            val = self.ser.read(1)
            if val != 'k':
                raise Exception('Lost sync')
            
            
        f.close()
    
    # dump with 8 bit access
    def DumpMem8(self, filename, address, count):
          
        # Read.  Format: r[otf]aaaaaaaa
        # otf -> read One, Two, or Four bytes
        # a.. -> address to read
        
        f = file(filename, 'wb')
        while count != 0: 
           
            self.ser.write('ro')
            h = "{:08x}".format(address) 
            self.ser.write(h)
            val = self.ser.read(2)
            value = int(val, 16)
            f.write(struct.pack('<B', value))
            print '0x%x' % value
            address = address + 1
            count = count - 1
            val = self.ser.read(1)
            if val != 'k':
                raise Exception('Lost sync')
    
        f.close()
    
    def flushCom(self):
        self.ser.flushInput()
        self.ser.flushOutput()
    

def main():


    parser = argparse.ArgumentParser(description='Dumper for Rephone', prog='dump')
    parser.add_argument('--port', '-p', help='Serial port device', default='/dev/ttyUSB0')
    
    args = parser.parse_args()
            
    h = DumpTool()
    
    h.open(args.port)
    
    h.Sync()    
    
    # dump boot rom
    h.DumpMem('boot_rom.bin', 0xfff00000, 0x4000)  # 0x878F
    
    # dump sf flash
    h.DumpMem8('firm.bin', 0x00000000, 0x800000)
    
    h.close()
   
if __name__ == '__main__':
    try:
        main()
       
    except Exception, err:
        sys.stderr.write('ERROR: %s\n' % str(err))
        traceback.print_exc()
