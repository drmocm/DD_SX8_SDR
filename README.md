# DVB-SCAN
An example programs in C to show IQ data with libdddvb and 
DigitalDevices MAX SX8

**For the C example**

You have to build dvb_iq with

`make` 

You need to install the libdddvb library 

https://github.com/DigitalDevices/dddvb

and GTK 3.0 libraries as well as dvben50221 and libfftw3.


On an Ubuntu system this would look like this:

`sudo apt-get install build-essential libgtk-3-dev libglib2.0-dev pkg-config dvb-apps libfftw3-dev`


`git clone https://github.com/DigitalDevices/internal_dddvb.git` 

`cd internal_dddvb/lib/; make` 

`sudo make install` 

`cd`

`git clone https://github.com/drmocm/DVB-IQ.git` 

`cd DVB-SCAN` 

`make` 


(Tell me if I forgot any)

A typical example call would be:

`./iq-display -p v -f 10847000  -f 23000000 -c ~/ddzapconf/ -l 64 -t 0` 

this visualizes the output you would get from: 

`ddzap -d S2 -p v -f 10847000  -s 23000000  -c ~/ddzapconf/ -l64 -i 0x10000000 -o` 

Additionally you can use iq-display to show the spectrum of the IQ data, either
around a given frequency

`./iq-display -p v -f 10847000  -c ~/ddzapconf/ -l 64  -t 1` 

with the ability to change it interactively. Or for the entire spectrum:

`./iq-display -p v -c ~/ddzapconf/ -l 64  -t 2` 

Here you have to use the "scan" button to start the scan.

Try 

`./iq-display  -d S2 -p v -f 12522000  -s 22500000  -c ~/ddzapconf/ -l64  -i 0x10000000 -q6` 

on Astra 28°E for a nice 16PSK example.

There is one more cli option for chosing the color scheme:

`-q X`

selects scheme number X (1=red, 2=green, 3=blue, 4= multi color)


