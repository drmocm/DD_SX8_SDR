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
>    ./ddsx8-spec [-f frequency] [-s rate] [-a adapter] [-i input] [-k] [-l alpha] [-b] [-c] [-n nfft] [-x] [-h] 
>
>      -f frequency: center frequency of the spectrum in kHz
>
>      -s rate     : the signal rate used for the FFT in Hz
>
>      -p pol      : polarisation 0=vertical 1=horizontal
>
>      -a adapter  : the number n of the DVB adapter, i.e. 
>                    /dev/dvb/adapter[n] (default=0)
>
>      -i input    : the physical input of the SX8 (default=0)
>
>      -k          : use Kaiser window before FFT
>
>      -b          : turn on agc
>
>	   -n          : number of FFTs for averaging (default 1000)
>
>      -c          : continuous output
>
>	   -x f1 f2    : full spectrum scan from f1 to f2\n\n"
>	                 (default -x 0 : 950000 to 2150000 kHz) \n\n"
>
>      -d          :  use 1s delay to wait for LNB power up
>
>      -l alpha    : parameter of the Kaiser window

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


If you use the -t option and write the resulting comma separated list
into the file test.csv like this: 

`/ddsx8-spec -f 1210000  -b -t -n 1000 -i 1 > test.csv` 

you can use the gnuplot program to display the data with the added 
gnuplot file test.gnuplot like this:

`gnuplot test.gnuplot` 

The -x option can be used to get a full spectrum scan with CSV output
like this ( it will take a few seconds ):

`/ddsx8-spec -f 1210000  -x 0 -b -t -n 1000 -i 1 > test.csv` 

or for a specific range, e.g. 1000000 kHz to 1200000 kHz

`/ddsx8-spec -f 1210000  -x  1000000 1200000 -b -t -n 1000 -i 1 > test.csv` 
