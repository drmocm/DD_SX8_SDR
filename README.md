![spectrum](screenshot/spectrum.jpg)

# 	DD_SX8_SDR

ddsx8-spec is an **example** program written in C to show how to use 
the SDR mode of the DigitalDevices MAX SX8 to get IQ data. The program
calculates the frequency spectrum of the incoming raw IQ data signal 
around a given center frequency. The bandwidth used is 51.2 MHz.
The spectrun is of course distorted at the edges, due to the finite 
aspect of the FFT. For a full spectrum, one would have to see, how big 
of a window in the center is usable and stitch the spectrum together 
accordingly.
There also seem to be some problems with the SX8 firmware when
trying too do this in a fast manner, since the SDR feature is
still experimental and will probably improve in later firmwares.

I also, don't send any DiSEqC commands for Switches or Unicable LNBs, 
so that at full scan as such is not possible with this program.

The spectrum is written to stdout either in the PAM image format
or as CSV.

**Compilation of the program**

You have to build ddsx8-spec with

`make` 

You need to install the libfftw3 library

On an Ubuntu system this would look like this:

`sudo apt-get install libfftw3-dev`

`cd  DD_SX8_SPECTRUM` 

`make` 


**Usage**

For usage information use the -h option.
>./ddsx8-spec -h
>
>    usage:
>    ./ddsx8-spec [-f frequency] [-a adapter] [-i input] [-k] [-l alpha] [-b] [-c] [-h]
>
>      frequency: center frequency of the spectrum in KHz
>
>      adapter  : the number n of the DVB adapter, i.e. 
>                 /dev/dvb/adapter[n] (default=0)
>
>      input    : the physical input of the SX8 (default=0)
>
>      -k       : use Kaiser window before FFT
>
>      -b       : turn on agc
>
>      -c       : continuous output
>
>      alpha    : parameter of the KAiser window

Typical calls would be:

`./ddsx8-spec -f 1030000  -k -t >test.csv`

to get the spectrum around 1030000 kHz with the (for now) fixed bandwidth of 51200 kHz and write it 
to a CSV file, which could be visualizprogram like gnuplot (see test.gnuplot).
I switch off the AGC, but with -b you can turn it back on (see dvb.h and dvb.c on how to do it).

If you select the PAM format as output you can use ffplay to view the data
as single image:

`/ddsx8-spec -f 1030000  -k  | ffplay -f pam_pipe -`

or continuously:

`./ddsx8-spec -f 1030000  -k -c  | ffplay -f pam_pipe -` 



