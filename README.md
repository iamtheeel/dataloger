# dataloger
A Dataloger using the STM NUCLEO-L476RG, a MicroSD Card, and the FatFS library for SFSU ENGR 498 

<img src="images/IMG_7176.jpg" width="400" >

#### Table of Contents
- [Background](#background)
- [Indent](#indent)
- [Center](#center)
- [Using the datalogger](#use)

### Background {#background}
In 2008 Tesla was just shipping the first of the Roadsters. Unfortunately the chassis they had received from the UK had some issues with "Squeak"
My company was brought in to attempt to diagnose where the "Squeak" was coming from (It turned out to be many places!).
In order to do this our first task was to instrument the car and capture the "Squeak" in order to analyze it's charictor.
To do this, I setup a TEAC Digital Tape Data-logger. Well, it was not comfortable. See below.
It was also awkward to use. You had to record the data, then lug the entire instrument back to the lab and play the data back to your analysis equipment. Oh, I just checked and you can currently pick one of these up on eBay well used for $700.

![Instrumenting the Tesla Roadster!](/images/tesla.png "Datalogger In Tesla")

When thinking about what we could do for our project, this experience jumped to my mind.
I wanted something small, lightweight, low power, and it had to write to a file.
We definitely did not want to truck with a filesystem, so we used the <a href="http://elm-chan.org/fsw/ff/00index_e.html">FatFS</a> library by ChaN to take care of the files themselves. We just had to write an SD Card driver... Oh, if I knew then what I knew now.
Oh, the FatFS web page usualy returns a 404. Please see the <a href="https://github.com/abbrev/fatfs"> Copy put on Git</a> by Christopher Williams.

Building: 
The system is built with the Keil compiler. Some members used the Arm Keil Studio Pack under VS Code, and others used the Keil uVision.
We did not use any of the ST HAL library functions, everything was done with Arm CMSIS

Each of these bits was hand-twiddled by our team of trained code monkeys.

<img src="images/sdRead.jpg">

In writing the software it was important to make sure that everything was interrupt driven. The only thing in the main loop is setup, and a command to go to sleep when not needed.

![The Datalogger Software Flow Chart](/images/flowChart.jpg "Flow Chart")


The circuitry is fairly simple. It turns out that the SD Card contains the SPI circutry!
All we needed was a few pull-up resisters (Ok, we used a <a href="https://www.adafruit.com/product/4682">$3.50 Micro SD Breakout</a> from our friends at Adafruit that had the pull-ups built in.
We also added a button for start stop, and an LED to indicate when a file is being written.
For shutdown and a heartbeat/status we used the button and LED that are built into the NUCLIO development board.

![The Datalogger Circuit Diagram](/images/circuitDiagram.jpg "Circuit")


<a href="#use">Heading IDs</a>
### Using the datalogger {#use}
LED Status



