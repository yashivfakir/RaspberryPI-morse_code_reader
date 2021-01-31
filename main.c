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
#include <stdlib.h>


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
static volatile int Voltage_List_COUNT = 1; 
static volatile int length_DOT = 0; 

const char symbol[37] = {' ','A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z','0','1','2','3','4','5','6','7','8','9'};
const char *morseCode[37] = {"0000000","01","1000","1010","100","0","0010","110","0000","00","0111","101","0100","11","10","111","0110","1101","010","000","1","001","0001","011","1001","1011","1100","01111","00111","00011","00001","00000","10000","11000","11100","11110","11111"};
// _________________________________________________
//  Linked List Definitions
// _________________________________________________

typedef struct Voltage_List_Type {
    int data;
    struct Voltage_List_Type * next;
} Voltage_List;

typedef struct Message_List_Type {
    char symbol;
    struct Message_List_Type * next;
} Message_List;

Message_List * Head_Message = NULL;
Voltage_List * Head_Voltage = NULL;

/*
void print_list(Voltage_List * head) {
    Voltage_List * current = head;

    while (current != NULL) {
        printf("%d\n", current->data);
        current = current->next;
    }
}*/

void pushVoltage(Voltage_List * head, int newData) {
    // Adds a element to the end of the Voltage linked list
    Voltage_List * current = head;
    while (current->next != NULL) {
        current = current->next;
    }

    Voltage_List_COUNT += 1; // Updates the global voltage linked list counter
    current->next = (Voltage_List *) malloc(sizeof(Voltage_List));
    current->next->data = newData;
    current->next->next = NULL;
}

void pushMessage(Message_List * head, char newSymbol) {
    // Adds a element to the end of the Final Message linked list
    Message_List * current = head;
    while (current->next != NULL) {
        current = current->next;
    }

    current->next = (Message_List *) malloc(sizeof(Message_List));
    current->next->symbol = newSymbol;
    current->next->next = NULL;
}

void linkedList_Init(){
    // Create the first element of the both the voltage and final message linked list
    Head_Voltage= (Voltage_List *) malloc(sizeof(Voltage_List));
    Head_Message= (Message_List *) malloc(sizeof(Message_List));
     if (Head_Message == NULL || Head_Voltage == NULL) {
          printf("Linked list failed to initiate");
     }

     Head_Voltage->data= 0;
     Head_Voltage->next = NULL;
     Head_Message->symbol= ' ';
     Head_Message->next = NULL;
}

int popVoltage(Voltage_List ** head) {
    int popped_Voltage = -1;
    Voltage_List * next_Voltage_Value = NULL;

    if (*head == NULL) {
        return -1;
    }

    next_Voltage_Value = (*head)->next;
    popped_Voltage = (*head)->data;
    free(*head);
    *head = next_Voltage_Value;

    return popped_Voltage;
}

int popMessage(Message_List ** head) {
    int popped_Symbol = -1;
    Message_List * next_Message_Symbol = NULL;

    if (*head == NULL) {
        return -1;
    }

    next_Message_Symbol = (*head)->next;
    popped_Symbol = (*head)->symbol;
    free(*head);
    *head = next_Message_Symbol;

    return popped_Symbol;
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
    

     }

int Average(Voltage_List * head,  int *highest, int limit){
    /* This function is used to determine the average and spread
    of the voltage values used in the DashDot() function
    */
    Voltage_List * current = head;

    int total = 0;
    int average = 0;
    int num_Element = 0;
    *highest = current->data;

    while ((current->next != NULL) || (num_Element < limit)) {
        
        if ((current->data) > *highest){
            *highest= current->data;
        }
        
        total += current->data;
        num_Element += 1;
        current = current->next;
    }
    // Have to run the next 2 lines again to include the final value
    total += current->data;
    num_Element += 1;
    // Then calculate average to define difference between high and low value data points
    average = total/num_Element;

    return average;
}


int DashDot(Voltage_List * head){
    Voltage_List * current = head;
    int highest;

    int current_Count = 0; // this is the count of current consecutive black values detected
    int lowest_Count = 0; // this is the current estimate of length of a dash
    int limit = 500;
    int num_Element = 0;

    int average = Average(Head_Voltage,&highest,limit);
    
    while ((highest - average) < 10){
        /* if this statement is true the message being measured is
        either only white space or only black spaces */
        limit = 100 + limit;
        average = Average(Head_Voltage,&highest,limit);
        // Thus will keep going until black and white parts detected
    }

    while (current->next != NULL || (num_Element < limit)) {
        if (current->data >= average){
            // Counting the black parts detected
            current_Count += 1;
            current = current->next;
            num_Element += 1;
      
        } else {
            // when 'else' is invoked the next white part is detected
            
            if (lowest_Count == 0) {
                // first 'if' is to set the lowest value length of a dash in the beginning
                lowest_Count = current_Count;
            } else if (current_Count < lowest_Count) {
                // The 'else if' is to reset when a smaller dash length is found
                lowest_Count = current_Count;
            }
            current_Count = 0;
            current = current->next;
            num_Element += 1;
        }
    }
    return lowest_Count;

}

// _________________________________________________
//   Main Functions
// _________________________________________________

int main(){
     printf("MORSE CODE DECIPHER\n");
     printf("Please press the button to begin\n");
     linkedList_Init();  // Create the first element of the linked list
     
     wiringPiSetupGpio();    // This sets the pin numbering system to the BCM pin number system
     
     // BUTTON PIN SETUP 
     pinMode(BUTTON_PIN, INPUT);        // Sets the pin to recieve an input
     pullUpDnControl(BUTTON_PIN, PUD_UP);      // Enables the pull down resistor on the button 

     signal(SIGINT, Termination_Handler); // This catches the termination ctrl-c in terminal
     enableADC();
     

     while(Program_Mode){
          wiringPiISR(BUTTON_PIN, INT_EDGE_BOTH, &buttonInterrupt);  // Sets the button listener to call the interupt method when pressed
         
          delay(100);
          
          pushVoltage(Head_Voltage,analogRead(ADC_CHANNEL));
          //printf("%d\n", analogRead(ADC_CHANNEL));
     
     }

return 0;
}