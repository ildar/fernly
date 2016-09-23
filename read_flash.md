

#1. Download

Get the flashrom source from github. 

	git clone https://github.com/mandl/flashrom.git


#2. Build 

Build the flashrom tool from source with fernly support

	make WARNERROR=no


#3. Load Rephone

Upload the mt2502a.bin into the device.

	./firmwareUploader.py --firmPath ./bin/mt2502a.bin  --nobat --native

#3. Run

	./flashrom --programmer fernvale_spi:dev=/dev/ttyUSB0 --read flash.dat


Reading 16MB of flash to take up to 10 minutes !!!



#4. Sample

	flashrom v0.9.9-unknown on Linux 3.16.0-55-generic (x86_64)
	flashrom is free software, get the source code at https://flashrom.org

	Calibrating delay loop... OK.
	Found Macronix flash chip "MX25U12835F" (16384 kB, SPI) on fernvale_spi.
	===
	This flash part has status UNTESTED for operations: PROBE READ ERASE WRITE
	The test status of this chip may have been updated in the latest development
	version of flashrom. If you are running the latest development version,
	please email a report to flashrom@flashrom.org if any of the above operations
	work correctly for you with this flash chip. Please include the flashrom log
	file for all operations you tested (see the man page for details), and mention
	which mainboard or programmer you tested in the subject line.
	Thanks for your help!
	Reading flash... done.


