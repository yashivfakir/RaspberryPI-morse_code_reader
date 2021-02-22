// *****************************************************
// Title: RASPBERRY PI - MORSE CODE READER
// Author: Yashiv Fakir
// Date: 23/01/2021
// *****************************************************

// _________________________________________________
//  Libraries
// _________________________________________________

#include <stdio.h>
#include <wiringPi.h>    // Used to interface with the raspberry Pi and C code
#include <wiringPiSPI.h> // Links ADC to Pi using SPI
#include <mcp3004.h>     // Interface with the ADC
#include <signal.h>      // To catch the ctrl-c signal
#include <pthread.h>     // Enable threads
#include <stdlib.h>
#include <string.h>      



// _________________________________________________
//  Constants and Global Variables
// _________________________________________________

#define BUTTON_PIN 16  // This is GPIO 16 connected to the button
#define LED_PIN 26 // This is the GPIO 26  connected to the LED
#define SPI_PIN 0 // This refers to GPIO 8 (SPI0 CE0) on the Pi 
#define ADC_CHANNEL 101 // This refers to the channel on the ADC chip being 100 - 106 (pin 0 -7)

unsigned long previous_buttonInterrupt_time = 0;  // previous_buttonInterrupt_time 


static volatile int Program_Mode = 3; 
/*  The Reader has 3 possible operational modes:
        Mode 0 - When '0' the program is to terminate
        Mode 1 - When '1' the program is reading data
        Mode 2 - When '2' the program is on standby
    The varibale is set to 3 only in the beginning
*/

// _________________________________________________
//  Function Status Variables
// _________________________________________________
int Middle_Function_STATUS = 0;  // Is set to '1' when the function is completed
int Dash_Dot_Space_Function_STATUS = 0; // Is set to '1' when the function is completed
int Conversion_Function_STATUS = 0;  // Is set to '2' when the function is completed, '1' when running
int Output_Function_STATUS = 0; // Is set to '1' when the function is completed

// _________________________________________________
//  Memory Variables
// _________________________________________________
int input_CYCLES = 0; // This is the number of times that the Voltage Array has been filled and begun again from the beginning
int analysed_CYCLES = 0; // This is the number of times the data in the Voltage Array has begun from the beginning again

#define array_LENGTH 200 // This is the number of elements in the Voltage Array
int array_Append_COUNT = 0; // This is the counter to count the appended voltages to the Voltage Array
int array_Analyse_COUNT = 0; // This is the counter to analyse the voltages in the Voltage Array
int Final_Message_COUNT = 0; // This is the counter to reference the alphanumeric symbols in the Final_Message Array

int Voltage_Values[2*array_LENGTH]; // This Array stores the measured voltage values
char Final_Message[4*array_LENGTH]; // This Array stores the converted alphanumeric symbols


// _________________________________________________
//  Calibrating Constants
// _________________________________________________
// The five variables below are used to analyse the voltage signal as defined by the respective names
// set during analysing the calibrating pattern
int BLACK_WHITE_Differentiator;
int Initial_Dot_LENGTH = 0;
int Initial_Dash_LENGTH = 0;
int Initial_SmallSpace_LENGTH = 0;
int Initial_BigSpace_LENGTH = 0;



// _________________________________________________
//  Thread Definitions
// _________________________________________________
pthread_t Voltage_Record_THREAD;  // Thread to fill array with voltage values
pthread_t Voltage_Middle_THREAD;    // Thread to determine the difference between BLACK and WHITE
pthread_t Voltage_Dash_Dot_THREAD; // Thread to analyse the Calibrating pattern
pthread_t Voltage_Conversion_THREAD;  // Thread to convert voltages to alphanumeric symbols
pthread_t Output_Message_THREAD;  // Thread to display the converted message

// MORSE CODE PATTERNS and ALPHANUMERIC ALPHABET

const char symbol[37] = {' ','A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z','0','1','2','3','4','5','6','7','8','9'};
// Note the 'symbol' array and the 'morseCode' array positions correspond with each other

// The '.' symbol is the terminating symbol for the conversion algorithm
const char morseCode[37][8]= {{'0','0','0','0','0','0','0','.'} /* */,
                                {'0','1','.'}                   /*A*/,
                                {'1','0','0','0','.'}           /*B*/,
                                {'1','0','1','0','.'}           /*C*/,
                                {'1','0','0','.'}               /*D*/,
                                {'0','.'}                       /*E*/,
                                {'0','0','1','0','.'}           /*F*/,
                                {'1','1','0','.'}               /*G*/,
                                {'0','0','0','0','.'}           /*H*/,
                                {'0','0','.'}                   /*I*/,
                                {'0','1','1','1','.'}           /*J*/,
                                {'1','0','1','.'}               /*K*/,
                                {'0','1','0','0','.'}           /*L*/,
                                {'1','1','.'}                   /*M*/,
                                {'1','0','.'}                   /*N*/,
                                {'1','1','1','.'}               /*O*/,
                                {'0','1','1','0','.'}           /*P*/,
                                {'1','1','0','1','.'}           /*Q*/,
                                {'0','1','0','.'}               /*R*/,
                                {'0','0','0','.'}               /*S*/,
                                {'1','.'}                       /*T*/,
                                {'0','0','1','.'}               /*U*/,
                                {'0','0','0','1','.'}           /*V*/,
                                {'0','1','1','.'}               /*w*/,
                                {'1','0','0','1','.'}           /*X*/,
                                {'1','0','1','1','.'}           /*Y*/,
                                {'1','1','0','0','.'}           /*Z*/,
                                {'1','1','1','1','1','.'}       /*0*/,
                                {'0','1','1','1','1','.'}       /*1*/,
                                {'0','0','1','1','1','.'}       /*2*/,
                                {'0','0','0','1','1','.'}       /*3*/,
                                {'0','0','0','0','1','.'}       /*4*/,
                                {'0','0','0','0','0','.'}       /*5*/,
                                {'1','0','0','0','0','.'}       /*6*/,
                                {'1','1','0','0','0','.'}       /*7*/,
                                {'1','1','1','0','0','.'}       /*8*/,
                                {'1','1','1','1','0','.'}       /*9*/};

// _________________________________________________
//  Voltage-Array Interface Definitions
// _________________________________________________

void *fill_Array(){
    // This function appends the measured voltage value to the voltage array
    
   
        
        int currentVoltage_Value = analogRead(ADC_CHANNEL);
        printf("Measured Voltage: %d\n",currentVoltage_Value);
        if (array_Append_COUNT < (3*array_LENGTH)){
            // While array is not yet full
            Voltage_Values[array_Append_COUNT] = currentVoltage_Value;
            array_Append_COUNT += 1;
        } else {
            // When array is fulled up cycle back
            array_Append_COUNT = 0;
            Voltage_Values[array_Append_COUNT] = currentVoltage_Value;
            input_CYCLES += 1;
            array_Append_COUNT += 1;
        }
    
    pthread_exit(NULL); 
}

int analyse_Array(){
    // This function pops the measured voltage values to be analysed.

    int popped_VOLTAGE;
    if (array_Analyse_COUNT < (3*array_LENGTH)){
        // While array is not yet full
        popped_VOLTAGE = Voltage_Values[array_Analyse_COUNT];
        array_Analyse_COUNT += 1;
    } else {
        // When array is fulled up cycle back
        array_Analyse_COUNT = 0;
        analysed_CYCLES += 1;
        popped_VOLTAGE = Voltage_Values[array_Analyse_COUNT];
        array_Analyse_COUNT += 1;
        
    }
    return popped_VOLTAGE;
}


// _________________________________________________
//  Supporting Functions
// _________________________________________________
void buttonInterrupt(){
    // This function handles the actions when the button is pressed
     unsigned long buttonInterrupt_time = millis();
     // Debounce condition to prevent double presses
     if (buttonInterrupt_time - previous_buttonInterrupt_time > 900) {

        if (Program_Mode == 2  || Program_Mode == 3) {

            // The next line sets and illuminate the LED to aid the LDR     
            digitalWrite(LED_PIN,HIGH); 
            
            // When pressed initially, sets the program to read mode
            Program_Mode = 1; 
            
        } else{

            // When pressed again, sets the program to standby mode
            Program_Mode = 2;
            //pthread_join(Voltage_Record_THREAD,NULL);

            Voltage_Values[array_Append_COUNT] = 0; // This is the termination symbol to signify the ending of the voltage input
            

            // Deluminates the LED that aided the LDR
            digitalWrite(LED_PIN,LOW);
            
        }
      }
    // Resets the time that the button was pressed to current time
    previous_buttonInterrupt_time = buttonInterrupt_time;
}


void enableADC(){
     // SPI and ADC setup
     wiringPiSPISetup(SPI_PIN,3000000);  // Enables the SPI functionality on the Pi
     mcp3004Setup(ADC_CHANNEL,SPI_PIN); // Defines the channels that the ADC chip is using

}


void Termination_Handler() {
     // This is the ctrl-c/ctrl-z interrupt handler that sets all pins low again 
     // upon termination

    Program_Mode = 0; // Sets the program to termination mode

    digitalWrite(LED_PIN,LOW); // Sets the LED pin low
    printf("Morse Code Decipher TERMINATED\n");
    exit(0);
}

void *Middle_Voltage(){
    /* This function is used to determine the middle voltage and defines the
    difference between a BLACK and WHITE part or between a lit LED or non-lit
    LED

    The function only analyses the inital voltage values
    */
    int highest = 0;
    int lowest = 1000;
    int count = 0; // temp array count variable
    
    if (array_Append_COUNT >= array_LENGTH){
        // Analyse the  1/3 inital points in array if message is greater than 1/3 the length of the array
        while (count < array_LENGTH) {
            if (Voltage_Values[count] > highest){
                highest = Voltage_Values[count] ;
            } else if (Voltage_Values[count] < lowest){
                lowest = Voltage_Values[count];
            }
            count += 1;
        }
  

    } else {
        // Analyse the message if the message is less than the 1/3 length of array 
        // Thus if only the calibrating pattern is provided
        while (count <= array_Append_COUNT) {
            if (Voltage_Values[count] > highest){
                highest = Voltage_Values[count] ;
            } else if (Voltage_Values[count] < lowest && Voltage_Values[count] != 0){
                lowest = Voltage_Values[count];
            }
            count += 1;
        }
    }
    
    
    // Then calculate Median to define difference between BLACK and WHITE data points
    BLACK_WHITE_Differentiator = (highest + lowest) / 2;
    Middle_Function_STATUS = 1; // Set function status to completed
    pthread_exit(NULL); 
}



void *DashDot_AND_Space_Length(){
    // This function determines the length of a dot and the length of a space between dots and dashes
    // using the middle_Voltage() function to differentiate between BLACK and WHITE

    // This function only analyses the calibrating pattern at the beginning of the message

    int current_Space_Count = 0;
    int current_Voltage_Count = 0;

    int previous = 0;
    int count = 0;
    int while_CONDITION = 0;
    int Initial_Space_Ignore = 0;
     
        while (while_CONDITION != 4) { 
            int temp_Voltage = Voltage_Values[count];

            if (temp_Voltage > BLACK_WHITE_Differentiator) { // FOR SOME REASON: BLACK = LOWER VALUES, WHITE = HIGHER VALUES
                // Found WHITE/SPACE

                if (previous <= BLACK_WHITE_Differentiator && current_Voltage_Count != 0 && previous != 0){
                // Moved from BLACK to WHITE/SPACE 

                    if (Initial_Dot_LENGTH == 0 && Initial_Dash_LENGTH == 0) {
                        Initial_Dash_LENGTH = current_Voltage_Count;
                        while_CONDITION += 1;
                    } else {
                        Initial_Dot_LENGTH = current_Voltage_Count;
                        while_CONDITION += 1;
                    }

                    current_Space_Count = 1; // include the current WHITE node
                    current_Voltage_Count = 0; // reset BLACK part counter

                } else {
                    // Still counting WHITE pattern
                    current_Space_Count += 1; 
                }
                previous = temp_Voltage;
                count += 1;
        
      
            } else if (temp_Voltage <= BLACK_WHITE_Differentiator){
            // Found BLACK


                if (previous > BLACK_WHITE_Differentiator && current_Space_Count != 0 && previous !=0){
                    // Moved from WHITE/SPACE to BLACK
                   
                    if (Initial_SmallSpace_LENGTH == 0 && Initial_BigSpace_LENGTH == 0 && Initial_Space_Ignore == 1) {
                        Initial_SmallSpace_LENGTH = current_Space_Count;
                        while_CONDITION += 1;
                    } else if (Initial_Space_Ignore == 1) {
                        Initial_BigSpace_LENGTH = current_Space_Count;
                        while_CONDITION += 1;
                    }

                    Initial_Space_Ignore = 1;
                    current_Voltage_Count = 1; // include the current BLACK node
                    current_Space_Count = 0;

                } else {
                    // Still counting BLACK pattern
                    current_Voltage_Count += 1;
                }

                previous = temp_Voltage;
                count += 1;
            }
        }
 
   
    Dash_Dot_Space_Function_STATUS = 1; // Set function status to completed
    
    pthread_exit(NULL); 
}



int Input_Speed_Adjuster(int Current_Length, int Message_Type){
    // Message_Type = 1: means that the length is BLACK
    // Message_Type = 0: means that the length is WHITE
    int New_length;
    if (Message_Type == 0){
        // Adjusting a BLACK part
        int dot_Difference = abs(Initial_Dot_LENGTH - Current_Length);
        int dash_Difference = abs(Initial_Dash_LENGTH - Current_Length);
        
        if (dot_Difference < dash_Difference){
            New_length = Initial_Dot_LENGTH;
        } else{
            New_length = Initial_Dash_LENGTH;
        }

    } else {
        // Adjusting a WHITE part
        int small_Difference = abs(Initial_SmallSpace_LENGTH - Current_Length);
        int Big_Difference = abs(Initial_BigSpace_LENGTH - Current_Length);

        if (small_Difference < Big_Difference){
            New_length = Initial_SmallSpace_LENGTH;
        } else{
            New_length = Initial_BigSpace_LENGTH;
        }

    }
    return New_length;
}

void *Conversion(){
    printf("................................................\n");
    printf("Currently Converting:\n");
    Conversion_Function_STATUS = 1;
    

    printf("\n");
    printf("Dot Length: %d\n",Initial_Dot_LENGTH);
    printf("Dash Length: %d\n",Initial_Dash_LENGTH);
    printf("Small Space Length: %d\n",Initial_SmallSpace_LENGTH);
    printf("Large Space Length: %d\n",Initial_BigSpace_LENGTH);
    printf("BLK/WHT Mid-Value: %d\n",BLACK_WHITE_Differentiator);
    printf("\n");

        int Conversion_Function_DashDot_Count = 0; // used to count the length of a dash or dot
        int Conversion_Function_Space_Count = 0;
        char Conversion_Function_MorseCode_Current[8]; // this is used to store the current 0's and 1's to compare later
       
        int Conversion_Function_MorseCode_Current_CHECK = 0; // This is used to ignore the first white space of the input message
        int Conversion_Function_MorseCode_Current_COUNT = 0; // counter for the array
        
        int Conversion_Function_Previous_Voltage = 0;

    
        // This means that the message has endend
        // Not possible to exceed the length of array
        int voltage_Value = analyse_Array();
        while (voltage_Value!=0) {
        
        
        
        if ( voltage_Value > BLACK_WHITE_Differentiator){
            // Found WHITE 
            if (Conversion_Function_Previous_Voltage <= BLACK_WHITE_Differentiator && Conversion_Function_Previous_Voltage != 0){
                // Moved from BLACK to WHITE
                printf("BLACK: %d\n",Conversion_Function_DashDot_Count);

                Conversion_Function_DashDot_Count = Input_Speed_Adjuster(Conversion_Function_DashDot_Count,0); // Invoke for BLACK


                // Analyse if the BLACK part is a dash or dot
                if (Conversion_Function_DashDot_Count == Initial_Dash_LENGTH){
                    
                    // Found a DASH
                    Conversion_Function_MorseCode_Current[Conversion_Function_MorseCode_Current_COUNT] = '1'; // for a dash
                    Conversion_Function_MorseCode_Current_COUNT += 1;
                    Conversion_Function_MorseCode_Current_CHECK = 1;  // Ensures that the first white space is ignored
                } else {
                    // Found a DOT
                    Conversion_Function_MorseCode_Current[Conversion_Function_MorseCode_Current_COUNT] = '0'; // for a dot
                    Conversion_Function_MorseCode_Current_COUNT += 1;
                    Conversion_Function_MorseCode_Current_CHECK = 1;  // Ensures that the first white space is ignored
                }
                Conversion_Function_DashDot_Count = 0;  // Reset BLACK part counter
                Conversion_Function_Space_Count = 1; // Reset WHITE space count including current WHITE part
            }
            else{
                // Just counting WHITE
                Conversion_Function_Space_Count += 1;
            }

        } else if ( voltage_Value <= BLACK_WHITE_Differentiator) { 
            // Found BLACK
            if (Conversion_Function_Previous_Voltage > BLACK_WHITE_Differentiator && Conversion_Function_Previous_Voltage != 0){
                // Moved from WHITE to BLACK
                printf("White: %d\n", Conversion_Function_Space_Count);



                Conversion_Function_Space_Count= Input_Speed_Adjuster(Conversion_Function_Space_Count,1); // Invoke for WHITE

                // Analyse if the WHITE part is a short or long space
                if (Conversion_Function_Space_Count == Initial_BigSpace_LENGTH && Conversion_Function_MorseCode_Current_CHECK != 0){
                    // Found a long space meaning end of a alphanumeric symbol
                    Conversion_Function_MorseCode_Current[Conversion_Function_MorseCode_Current_COUNT] = '.';
                    int stop = 0;
                    for (int i = 0; i<37 && stop == 0; i++){
                        for (int j = 0; j<7; j++){
                            if (Conversion_Function_MorseCode_Current[j] == morseCode[i][j]){
                                if (j == 6){
                                    // Found a ' ' or space between words in a message
                                    Final_Message[Final_Message_COUNT] = symbol[i];
                                    Final_Message_COUNT += 1;
                                    stop = 1;
                                    break;
                                } else if (Conversion_Function_MorseCode_Current[j] == '.' && morseCode[i][j] == '.') {
                                    // Found a matching pattern thats less than 7 units long
                                    Final_Message[Final_Message_COUNT] = symbol[i];
                                    Final_Message_COUNT += 1;
                                    stop = 1;
                                    break;
                                } else{
                                    // keep checking to see if remainder matches
                                    continue;
                                }
                            } else if (Conversion_Function_MorseCode_Current[j] != morseCode[i][j]) {
                                break; // used to break the inner for loop and continue to the outer loop
                            }
                        }
                    }
                    
                    memset(Conversion_Function_MorseCode_Current, 0, 8); // Empties Array for the next BLACK pattern                    
                    Conversion_Function_MorseCode_Current_COUNT = 0; // reset temp array counter
                }
                Conversion_Function_Space_Count = 0;  // Reset WHITE part counter
                Conversion_Function_DashDot_Count = 1; // Reset BLACK space count including current BLACK part

            }
            else{
                // Just counting BLACK
                Conversion_Function_DashDot_Count += 1;
            }
        }


        Conversion_Function_Previous_Voltage = voltage_Value;  // Save the current value for use later
        voltage_Value = analyse_Array();
    }



    // Run the code below again to convert the last BLACK pattern
    int stop = 0;
    Conversion_Function_MorseCode_Current[Conversion_Function_MorseCode_Current_COUNT] = '.';
    for (int i = 0; i<37 && stop == 0; i++){
        for (int j = 0; j<7; j++){
            if (Conversion_Function_MorseCode_Current[j] == morseCode[i][j]){
                if (j == 6){
                    // Found a ' ' or space between words in a message
                    Final_Message[Final_Message_COUNT] = symbol[i];
                    Final_Message_COUNT += 1;
                    stop = 1;
                    break;
                } else if (Conversion_Function_MorseCode_Current[j] == '.' && morseCode[i][j] == '.') {
                    // Found a matching pattern thats less than 7 units long
                    Final_Message[Final_Message_COUNT] = symbol[i];
                    Final_Message_COUNT += 1;
                    stop = 1;
                    break;
                } else{
                    // keep checking to see if remainder matches
                    continue;
                }
            } else if (Conversion_Function_MorseCode_Current[j] != morseCode[i][j]) {
                break; // used to break the inner for loop and continue to the outer loop
            }
                       
        }
    }
    memset(Conversion_Function_MorseCode_Current, 0, 8); // Empties the array 
    Conversion_Function_STATUS = 2;
    pthread_exit(NULL); 
}




void *Output(){
    // This function prints the final message and symbols in the Message linked list
    printf("\nThe converted Morse Code Message is shown below: \n");
    printf("________________________________________________\n");
    int count = 1; // Set to 1 to avoid the inital calibration pattern
    while(count != Final_Message_COUNT){

        printf("%c",Final_Message[count]);
        count += 1;
        
    }
    printf("\n");
    printf("________________________________________________\n");
    Output_Function_STATUS = 1;
    pthread_exit(NULL);
}



     

// _________________________________________________
//   Main Functions
// _________________________________________________

int main(){
    printf("________________________________________________\n");
    printf("            MORSE CODE DECIPHER\n");
    printf("________________________________________________\n");
    printf("|To begin press the button until the LED is on.|\n");
    printf("|Then run the encoded Morse Code Message under |\n");
    printf("|        the LDR sensor at an EVEN RATE.       |\n");
    printf("| Then press the button again when completed.  |\n");
    printf("________________________________________________\n");
   
     
    wiringPiSetupGpio();    // This sets the pin numbering system to the BCM pin number system
     
    // BUTTON PIN SETUP 
    pinMode(BUTTON_PIN, INPUT);             // Sets the pin to recieve an input
    pullUpDnControl(BUTTON_PIN, PUD_UP);    // Enables the pull down resistor on the button 
    signal(SIGTSTP, Termination_Handler);   // This catches the termination ctrl-z in terminal
    //signal(SIGINT, Termination_Handler);    // This catches the termination ctrl-c in terminal
    enableADC();                            // Sets up the ADC 
    pinMode(LED_PIN,OUTPUT);                // Sets the LED pin on the Pi as a output pin


    while(Program_Mode){ // While not in termination mode

        /*
            The function STATUS variables are used here to ensure that the functions 
            only executed once and executed in the order that they are called
        */

        // Sets the button listener to call the interupt method when pressed
        wiringPiISR(BUTTON_PIN, INT_EDGE_BOTH, &buttonInterrupt);  
         
        
        if (Program_Mode == 1){  // This is Read-Mode

            pthread_create(&Voltage_Record_THREAD, NULL, fill_Array, NULL);  // Records Voltage value
            
            if (array_Append_COUNT >= array_LENGTH && Middle_Function_STATUS == 0){
                pthread_create(&Voltage_Middle_THREAD, NULL, Middle_Voltage, NULL);
               // Calls the Middle Function thread
            }

            if (array_Append_COUNT >= array_LENGTH && Middle_Function_STATUS == 1 && Dash_Dot_Space_Function_STATUS == 0){
                pthread_create(&Voltage_Dash_Dot_THREAD, NULL, DashDot_AND_Space_Length, NULL);
               // Calls the Dash-Dot-Space Function thread
            }
            
            
            if (Conversion_Function_STATUS == 0 && Dash_Dot_Space_Function_STATUS == 1 ){
                pthread_create(&Voltage_Conversion_THREAD, NULL, Conversion, NULL);
               // Calls the Conversion Function thread
            }
            
      

        } else if (Program_Mode == 2){ // This is Stand-By mode

            // Have to call the first 3 threads again incase the message is smaller then half the size of the array
            
            
            if (Middle_Function_STATUS == 0){ 
                pthread_create(&Voltage_Middle_THREAD, NULL, Middle_Voltage, NULL);
                // Calls the Middle Function thread
            }

            if (Dash_Dot_Space_Function_STATUS == 0 && Middle_Function_STATUS == 1){
                pthread_create(&Voltage_Dash_Dot_THREAD, NULL, DashDot_AND_Space_Length, NULL);
                // Calls the Dash-Dot-Space Function thread
            }
            if (Conversion_Function_STATUS == 0 && Dash_Dot_Space_Function_STATUS == 1 ){
                pthread_create(&Voltage_Conversion_THREAD, NULL, Conversion, NULL);
                // Calls the Conversion Function thread
            }
             if (Output_Function_STATUS == 0 && Conversion_Function_STATUS == 2 ){
                
                pthread_create(&Output_Message_THREAD, NULL, Output, NULL);
                // Calls the Output Function thread
            }
            
        }
     
     }
pthread_exit(NULL); // Terminates if any threads still open before exiting
return 0;
}