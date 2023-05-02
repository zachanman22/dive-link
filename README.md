# dive-link
This is the GITHUB project for the DiveLink System. This system requires both transmitter and receiver boards produced by the Systems Research Lab at Georgia Tech. The system requires two Teensy boards, one for transmission and another for reception. To interface with these boards Arduino 1.8 is needed and the Teensy add-on imported in the program.

For the transmission Teensy, use the Big_Transmit file. In this file, the transmission baud, Transmit ID, and Number of Transmitters must be correct for proper transmission. The wiring for the Transmitter is commented in this file.

For the receiving Teensy, use the ADC_logger code. Only change this file if you need to change the sampling frequency, it is currently set to 400KHz. Refer to the documentation for the SI5351 external timer. This is not recommended.

For the receiving laptop, run the main.py file in the serial_queue folder. Change the COM port manually to whichever port the Teensy is connected and change the baud to the same baud of the transmission teensy. Once running, the success of the real time application is dependant on the amount of processing time dedicated to the program. Closing down other programs and ending unnecessary tasks is recommended.
