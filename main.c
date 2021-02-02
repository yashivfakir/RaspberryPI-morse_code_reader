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
static volatile int Message_List_COUNT = 1; 

const char symbol[38] = {' ','A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z','0','1','2','3','4','5','6','7','8','9'};
const char *morseCode[38] = {"0000000","01","1000","1010","100","0","0010","110","0000","00","0111","101","0100","11","10","111","0110","1101","010","000","1","001","0001","011","1001","1011","1100","01111","00111","00011","00001","00000","10000","11000","11100","11110","11111"};
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

/* void print_list(Voltage_List * head) {
    Voltage_List * current = head;

    while (current != NULL) {
        printf("%d\n", current->data);
        current = current->next;
    }
} */

void pushVoltage(Voltage_List * head) {
    // Adds a element to the end of the Voltage linked list
    int newData = analogRead(ADC_CHANNEL);
    Voltage_List * current = head;
    while (current->next != NULL) {
        current = current->next;
    }
    printf("pushing voltage\n");

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
    Message_List_COUNT += 1;
}

void linkedList_Init(){
    // Create the first element of the both the voltage and final message linked list
    Head_Voltage= (Voltage_List *) malloc(sizeof(Voltage_List));
    Head_Message= (Message_List *) malloc(sizeof(Message_List));
     if (Head_Message == NULL || Head_Voltage == NULL) {
          printf("Linked list failed to initiate\n");
     }

     Head_Voltage->data= 0;
     Head_Voltage->next = NULL;
     Head_Message->symbol= ' ';
     Head_Message->next = NULL;
}

char popMessage(Message_List ** head) {
    char popped_Symbol;
    Message_List * next_Message_Symbol = NULL;

    if (*head == NULL) {
        printf("Nothing more to pop\n");
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

        // Only begin the Thread when the button is pressed
       
        
     
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


int DashDot(Voltage_List * head, int * avg, int * space){
    printf("Test");
    Voltage_List * current = head;
    int highest;

    int current_Voltage_Count = 0; // this is the count of current consecutive black values detected
    int lowest_Voltage_Count = 0; // this is the current estimate of length of a dash
    int limit = 10;
    int num_Element = 0;

    int current_Space_Count = 0;
    int lowest_Space_Count = 0;

    int average = Average(Head_Voltage,&highest,limit);
    
    while ((highest - average) < 10){
        /* if this statement is true the message being measured is
        either only white space or only black spaces */
        limit = 10 + limit;
        average = Average(Head_Voltage,&highest,limit);
        // Thus will keep going until black and white parts detected
    }

    while (current->next != NULL || (num_Element < limit)) {
        if (current->data <= average){ // FOR SOME REASON: BLACK = LOWER VALUES, WHITE = HIGHER VALUES
            // Counting the black parts detected
            current_Voltage_Count += 1;
            current = current->next;
            num_Element += 1;
            if (current_Space_Count < lowest_Space_Count && current_Space_Count != 0){
                lowest_Space_Count = current_Space_Count;
            }
      
        } else {
            // when 'else' is invoked the next white part is detected
            

            if (lowest_Voltage_Count == 0) {
                // first 'if' is to set the lowest value length of a dash in the beginning
                lowest_Voltage_Count = current_Voltage_Count;
            } else if (current_Voltage_Count < lowest_Voltage_Count) {
                // The 'else if' is to reset when a smaller dash length is found
                lowest_Voltage_Count = current_Voltage_Count;
            }
            current_Voltage_Count = 0;
            current_Space_Count += 1;
            current = current->next;
            num_Element += 1;
        }
    }
    *avg = average;
    *space = lowest_Space_Count;
    return lowest_Voltage_Count;

}

void Conversion(Voltage_List ** head){
    printf("Converting morse code now\n");
    char * zero = "0";  // For a dot
    char * one = "1";  // For a dash
    

    int DotDash_Count = 0; // used to count the length of a dash or dot
    int Space_Count = 0;

    int Space_Length;
    int Average;
    
    int Dot_Length = DashDot(Head_Voltage,&Average,&Space_Length);
    char morseCode_Current[8]=" "; // this is used to store the current 0's and 1's to compare later

    int previous_Voltage_Value = 0;
    
    while ((*head)->next != NULL) {

        if ((*head)->data > Average){
            // This 'IF': counts white spaces at the beginning or between symbols
            
            Space_Count += 1;
            
            previous_Voltage_Value = (*head)->data;

            // Frees the data from the linked list
            Voltage_List * next_Voltage_Value = NULL;
            next_Voltage_Value = (*head)->next;
            free(*head);
            *head = next_Voltage_Value;

        } else if ((*head)->data <= Average) { 
            // This means that there is a black space

            if (Space_Count <= Space_Length && previous_Voltage_Value > Average) {
                // This signifies the space between dash and dots
                // Now can convert the 0's and 1's to an alphanumeric symbol
                if (DotDash_Count <= Dot_Length){
                    // This is a dot
                    strncat(morseCode_Current,zero,1); // adds a '0' to the current pattern
                } else{
                    // This is a dash
                    strncat(morseCode_Current,one,1); // adds a '1' to the current pattern
                }
                DotDash_Count = 1; // Reset the black space count including current black node


            } else if (Space_Count > Space_Length && previous_Voltage_Value > Average) {
                // This signifies the end of a pattern of dots and dashes
                for (int i = 0; i<38; i++){
                    if ((strncmp(morseCode[i], morseCode_Current, 8)) == 0){
                        pushMessage(Head_Message, symbol[i]);  // adds the converted alphanumeric symbol to the linked list
                    } else {
                        // 'else' : This means that the morse code message doesn't exist
                        printf("This is not a Morse Code Pattern\n");
                    }
                }
                DotDash_Count = 1; // Reset the black space count including current black node
            } else{
                DotDash_Count += 1;
            }
             
            Space_Count = 0; // Reset the white space count

            previous_Voltage_Value = (*head)->data; 

            Voltage_List * next_Voltage_Value = NULL;
            next_Voltage_Value = (*head)->next;
            free(*head);
            *head = next_Voltage_Value; 
            Voltage_List_COUNT -= 1;
        }
    }
}

/*void Output(){
    char Final_Message[Message_List_COUNT+1];

    while(Message_List_COUNT !=0){

        char temp = popMessage(&Head_Message);
        char str[2] = {temp, '\0'};
        strncat(Final_Message,str,1);
        Message_List_COUNT -= 1;

    }
        
    printf("_______________________________________________\n");
    printf("The converted Morse Code Message is: \n");
    printf("%s\n",Final_Message);
    printf("_______________________________________________\n");
}*/

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
     enableADC(); // Sets up the ADC and ONLY transfers the data not saves as of yet
     

     while(Program_Mode){
          wiringPiISR(BUTTON_PIN, INT_EDGE_BOTH, &buttonInterrupt);  // Sets the button listener to call the interupt method when pressed
         
         
          if (Voltage_List_COUNT < 50){
            pushVoltage(Head_Voltage);
          // printf("%d\n", analogRead(ADC_CHANNEL));
          }


           if (Voltage_List_COUNT == 50){
                int Space_Length;
                int Average;
                int Dot_Length = DashDot(Head_Voltage,&Average,&Space_Length);
                printf("%d",Dot_Length);
          }
     
     }

return 0;
}