# RaspberryPI-morse_code_reader

## Overview

This project is meant to interpret a Morse Code message in two ways.


      -- The first way is a paper morse code message.
      -- The second way is the an LED morse code message. 
   
   
For the first method, the sequence should be drawn in Black ink on a White paper. For the second method, the sequence is displayed on an LED light. For a detailed description on how the inputs should be drawn refer to MC_Project_Final_Report.pdf. Both input methods are measured using an LDR (Light Dependent Resistor) sensor that is then analysed and converted to an alphanumeric pattern. The reader utilizes a raspberry Pi and coded in C.

## Repository Breakdown 

The repository consists of several files that are explained below. If one is only looking to focus on using the code refer to MC_Project_Final_Report.pdf and use the components and circuit diagram shown in the  MC_Project_Hardware_Specifications.pdf. 

  For a written overview of the project and the scope refer to:
  
     -- MC_Project_Overview.pdf
   
  For the planning part of the project, the project is separated into to parts being:
  
    -- MC_Project_Hardware_Specifications.pdf
    -- MC_Project_Software_Specifications.pdf
    -- Pseudo Algorithm.txt (Note the final algorithm differs from the this inital version)
  
  For a written overview of the final code and the project as a whole refer to:
    
    -- MC_Project_Final_Report.pdf 
  
  Finally the code for the two versions are:
  
    -- LED_Input_Reader.c
    -- Paper_Input_Reader.c
    
    (Note both files need to be compiled)
        Use the following code to compile the code on a linux machine:
    $ gcc Paper_Input_Reader.c -lwiringPi -lpthread
        or 
    $ gcc LED_Input_Reader.c -lwiringPi -lpthread
    
        depending on the input method to be used.

## Usage Instructions

A summary of how to use the reader is meant to be used is explained below however for a full description refer to the MC Project Final Report.pdf

Once the code has been compiled and th circuit is built, execute the program and then press the puch button to begin reading. Measure the LED input (ensure the distance between the LED and LDR is approximately 1cm) or run the paper input under the LDR at a constant rate (ensure the distance between the LED and LDR is approximately 1cm). Once the message is measured completely press the button again to convert and display the converted message.

