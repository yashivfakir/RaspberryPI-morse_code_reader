// *****************************************************
// Title: RASPBERRY PI - MORSE CODE READER
// Author: Yashiv Fakir
// Date: 23/01/2021
// *****************************************************

// _________________________________________________
//  Libraries
// _________________________________________________

#include <stdio.h>
#include <wiringPi.h>
#include <wiringPiSPI.h> // Links ADC to Pi using SPI
#include <mcp3004.h> // Interface with the ADC
#include <signal.h> // To catch the ctrl-c signal
#include <pthread.h> // Enable threads
#include <string.h>


// _________________________________________________
//  Constants and Global Variables
// _________________________________________________

#define BUTTON_PIN 16  // This is GPIO 16 connected to the button
#define LED_PIN 26 // This is the GPIO 26  connected to the LED
#define SPI_PIN 0 // This refers to GPIO 8 (SPI0 CE0) on the Pi 
#define ADC_CHANNEL 100 // This refers to the channel on the ADC chip being 100 - 107 (pin 0 -7)


// previous_buttonInterrupt_time 
unsigned long previous_buttonInterrupt_time = 0; 
// When 'Program_Mode' is 1, the program is running and when 0 the program is terminated
static volatile int Program_Mode = 1; 

char symbol[] = {' ','A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z','0','1','2','3','4','5','6','7','8','9'};
char * morseCode[] = {'0000000','01','1000','1010','100','0','0010','110','0000','00','0111','101','0100','11','10','111','0110','1101','010','000','1','001','0001','011','1001','1011','1100','01111','00111','00011','00001','00000','10000','11000','11100','11110','11111'};
// _________________________________________________
//  Linked List Definitions
// _________________________________________________

struct Voltage_Node{
     int data;
     struct Voltage_Node * next;
} Voltage_List;

struct Message_Node{
     char symbol;
     struct Message_Node * next;
} Message_List;

Voltage_List * head_Voltage = NULL;
Message_List * head_Message = NULL;


void linkedListInit() {
     head_Voltage = (Voltage_List *) malloc(sizeof(Voltage_List));
     head_Message = (Message_List *) malloc(sizeof(Message_List));

     head_Voltage->data = 0;
     head_Voltage->next = NULL;

     head_Message->symbol = ' ';
     head_Message->next = NULL;
}

void pushVoltage(Voltage_List * head_Voltage, int newData) {
    Voltage_List * current = head_Voltage;
    while (current->next != NULL) {
        current = current->next;
    }
    current->next = (Voltage_List *) malloc(sizeof(Voltage_List));
    current->next->data = newData;
    current->next->next = NULL;
}

void pushMessage(Message_List * head_Message, symbol newChar) {
    Message_List * current = head_Message;
    while (current->next != NULL) {
        current = current->next;
    }
    current->next = (Message_List *) malloc(sizeof(Message_List));
    current->next->data = newChar;
    current->next->next = NULL;
}




// _________________________________________________
//  Supporting Functions
// _________________________________________________
void buttonInterrupt(){
     unsigned long buttonInterrupt_time = millis();
     // Debounce condition to prevent 
     if (buttonInterrupt_time - previous_buttonInterrupt_time > 700) {
          printf("Now run your encoded message under the LDR sensor\n");
          // The next two lines set and illuminate the LED         
          pinMode(LED_PIN,OUTPUT);
          digitalWrite(LED_PIN,HIGH);

          enableADC();
      }
     previous_buttonInterrupt_time = buttonInterrupt_time;
}

void enableADC(){
     // SPI and ADC setup
     wiringPiSPISetup(SPI_PIN,100000);  // Enables the SPI functionality on the Pi
     mcp3004Setup(ADC_CHANNEL,SPI_PIN);

}

void Termination_Handler() {
     // This is the ctrl-c interrupt handler that sets all pins low again 
     // upon termination
     Program_Mode = 0;
     digitalWrite(LED_PIN,LOW); // Sets the LED pin low
     printf("Morse Code Decipher TERMINATED\n");
}

void Conversion(){
     // The symbols and morse code array have a fixed length of 37
     for (int i=0; i<37; i++) {

     }
}

// _________________________________________________
//   Main Functions
// _________________________________________________

int main(){
     printf("MORSE CODE DECIPHER\n");
     printf("Please press the button to begin\n");
     
     wiringPiSetupGpio();    // This sets the pin numbering system to the BCM pin number system
     
     // BUTTON PIN SETUP 
     pinMode(BUTTON_PIN, INPUT);        // Sets the pin to recieve an input
     pullUpDnControl(BUTTON_PIN, PUD_UP);      // Enables the pull down resistor on the button 


     signal(SIGINT, Termination_Handler); // This catches the termination ctrl-c in terminal

     while(Program_Mode){
          wiringPiISR(BUTTON_PIN, INT_EDGE_BOTH, &buttonInterrupt);  // Sets the button listener to call the interupt method when pressed
         
          delay(100);
          printf("%d\n", analogRead(ADC_CHANNEL));
     }

return 0 ;
}