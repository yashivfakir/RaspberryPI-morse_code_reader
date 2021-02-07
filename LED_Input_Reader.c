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
#define LED_PIN_1 5 // (RED) This is the GPIO 26  connected to the 1st LED
#define LED_PIN_2 6  // (BLUE) This is the GPIO 6  connected to the 2nd LED
#define LED_ON 26
#define SPI_PIN 0 // This refers to GPIO 8 (SPI0 CE0) on the Pi 
#define ADC_CHANNEL 100 // This refers to the channel on the ADC chip being 100 - 107 (pin 0 -7)

// previous_buttonInterrupt_time 
unsigned long previous_buttonInterrupt_time = 0; 
// When 'Program_Mode' is 1, the program is reading data and when 0 the program is terminated
// And when it is 2, the program is on standby
static volatile int Program_Mode = 2; 
static volatile int Voltage_List_COUNT = 1; 
static volatile int Message_List_COUNT = 1; 
pthread_t Message_Begin;

const char symbol[37] = {' ','A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z','0','1','2','3','4','5','6','7','8','9'};
// Note the 'symbol' array and the 'morseCode' array positions correspond with each other
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



void pushVoltage(Voltage_List * head) {
    // Adds a element to the end of the Voltage linked list
    int newData = analogRead(ADC_CHANNEL);
    Voltage_List * current = head;
    while (current->next != NULL) {
        current = current->next;
    }
    printf("Pushing Voltage Value: %d\n",newData);
    printf("Count: %d\n",Voltage_List_COUNT);

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

     Head_Voltage->data=0;
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
    Message_List_COUNT -= 1;

    return popped_Symbol;
}
// _________________________________________________
//  LED Input Function
// _________________________________________________

void *LED_Input(void *vargp){
    // This function generates a LED Morse Code Message to display 'TEST'
    int small_WAIT = 500; // This is the short waiting period 
    int large_WAIT = 1000; // This is the large_WAIT waiting period 
    
    digitalWrite(LED_PIN_1,LOW); // RED -- DOT
    digitalWrite(LED_PIN_1,LOW); // BLUE -- DASH

    delay(large_WAIT);  
    digitalWrite(LED_PIN_1,HIGH); // BLUE -- DASH -- ON     //    T
    delay(large_WAIT);                                   
    digitalWrite(LED_PIN_1,LOW); // BLUE -- DASH --OFF  
    delay(large_WAIT);                                           //  large_WAIT SPACE
//-------------------------------------------------------------------------
    digitalWrite(LED_PIN_1,HIGH); // RED -- DOT -- ON       //    E
    delay(small_WAIT);
    digitalWrite(LED_PIN_1,LOW); // RED -- DOT -- OFF
    delay(large_WAIT);                                           //  large_WAIT SPACE
//-------------------------------------------------------------------------
    digitalWrite(LED_PIN_1,HIGH); // RED -- DOT -- ON       //    S
    delay(small_WAIT);
    digitalWrite(LED_PIN_1,LOW); // RED -- DOT -- OFF
    delay(small_WAIT);                                           //  small_WAIT SPACE
    digitalWrite(LED_PIN_1,HIGH); // RED -- DOT -- ON       //    
    delay(small_WAIT);
    digitalWrite(LED_PIN_1,LOW); // RED -- DOT -- OFF
    delay(small_WAIT);                                           //  small_WAIT SPACE
    digitalWrite(LED_PIN_1,HIGH); // RED -- DOT -- ON       //    
    delay(small_WAIT);
    digitalWrite(LED_PIN_1,LOW); // RED -- DOT -- OFF
    delay(large_WAIT);                                           //  large_WAIT SPACE
//-------------------------------------------------------------------------
    digitalWrite(LED_PIN_1,HIGH); // BLUE -- DASH -- ON     //    T
    delay(large_WAIT);                                   
    digitalWrite(LED_PIN_1,LOW); // BLUE -- DASH --OFF  
    delay(large_WAIT);                                           //  large_WAIT SPACE


    digitalWrite(LED_PIN_1,LOW);
    digitalWrite(LED_PIN_1,LOW);
    return NULL;                                        
}

// _________________________________________________
//  Supporting Functions
// _________________________________________________
void buttonInterrupt(){
     unsigned long buttonInterrupt_time = millis();
     // Debounce condition to prevent 
     if (buttonInterrupt_time - previous_buttonInterrupt_time > 1000) {

        if (Program_Mode == 2) {
            Program_Mode = 1;
            pthread_create(&Message_Begin, NULL, LED_Input, NULL);
        } else{
            Program_Mode = 2;
            pthread_join(Message_Begin, NULL); 
            digitalWrite(LED_PIN_1,LOW);
            digitalWrite(LED_PIN_2,LOW);
            digitalWrite(LED_ON,HIGH);
        }
        
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
    digitalWrite(LED_PIN_1,LOW); // Sets the LED pin low
    digitalWrite(LED_PIN_2,LOW); // Sets the LED pin low
     digitalWrite(LED_ON,LOW);
    printf("Morse Code Decipher TERMINATED\n");

}


int Average(Voltage_List * head,  int *highest, int * lowest, int limit){
    /* This function is used to determine the average and spread
    of the voltage values used in the DashDot() function
    */
    Voltage_List * current = head;


    int average = 0;
    int num_Element = 0;
    *highest = 0;
    *lowest = 1000;
    
    while (/*(num_Element < limit) || */ current->next != NULL ) {
        
        if ((current->data) > *highest){
            *highest = current->data;
        }else if ((current->data) < *lowest && current->data != 0){
            *lowest = current->data;
        }
      
        
        //total += current->data;
        num_Element += 1;
        current = current->next;

    }
     printf("Highest: %d\n",*highest);
    printf("Lowest: %d\n",*lowest);
    // Have to run the next 2 lines again to include the final value
   // total += current->data;
    num_Element += 1;
    // Then calculate average to define difference between high and low value data points
   // average = total/num_Element;
    average = (*highest + *lowest) / 2;
 
    return average;
}


int DashDot(Voltage_List * head, int * avg, int * lowest_space){
    
    Voltage_List * current = head;
    int highest;
    int lowest;

    int current_Voltage_Count = 0; // this is the count of current consecutive black values detected
    int lowest_Voltage_Count = 0; // this is the current estimate of length of a dash
    
    int num_Element = 0;

    int current_Space_Count = 0;
    int lowest_Space_Count = 0;

    int factor = 0.3;
    int limit = (factor * Voltage_List_COUNT);

    int average = Average(Head_Voltage,&highest,&lowest,limit); 

    if (highest - lowest < 5){
        // This 'IF - STATEMENT' ensures that the inital data isn't just either just BLACK or just WHITE
        if (factor<=1){
            factor = factor + 0.1;
        }
        limit = (int)(factor * Voltage_List_COUNT);
        average = Average(Head_Voltage,&highest,&lowest,limit);
    }

    int previous = 0;

     
    while (/* (num_Element < limit) ||*/ current->next != NULL ) {    
    
        if (current->data > average) { // FOR SOME REASON: BLACK = LOWER VALUES, WHITE = HIGHER VALUES
            // Found WHITE/SPACE

            if (previous <= average && current_Voltage_Count != 0 && previous != 0){
                // Moved from BLACK to WHITE/SPACE 

                 if (lowest_Voltage_Count == 0){
                    // Sets the initial lowest VOLTAGE count at the beginning
                    lowest_Voltage_Count = current_Voltage_Count; 
                } else if (current_Voltage_Count < lowest_Voltage_Count){
                    // Found a small_WAITer length BLACK pattern
                    lowest_Voltage_Count = current_Voltage_Count; 
                }
                printf("BLACK: %d\n",current_Voltage_Count);
                current_Space_Count = 1; // include the current WHITE node
                current_Voltage_Count = 0;

            } else {
                current_Space_Count += 1; // Increase the space count
            }
            previous = current->data;  // Save previous voltage value
            current = current->next;  // Move to next voltage node
            num_Element += 1; // Increment the nodes assessed by one
      
        } else if (current->data <= average){
            // Found BLACK


            if (previous > average && current_Space_Count != 0 && previous !=0){
                // Moved from WHITE/SPACE to BLACK
                if (lowest_Space_Count == 0){
                    // Sets the initial lowest SPACE count at the beginning
                    lowest_Space_Count = current_Space_Count; 
                } else if (current_Space_Count < lowest_Space_Count){
                    // Found a small_WAITer in length WHITE/SPACE pattern
                    lowest_Space_Count = current_Space_Count; 
                }
                printf("WHITE: %d\n",current_Space_Count);
                current_Voltage_Count = 1; // include the current BLACK node
                current_Space_Count = 0;

            } else {
                // Still counting BLACK pattern
                current_Voltage_Count += 1;
            }

        previous = current->data;  // Save previous voltage value
        current = current->next;  // Move to next voltage node
        num_Element += 1; // Increment the nodes assessed by one
        }
    }
    
    *avg = average; // Averge/Middle value of the voltages
    // Adds a 20% lead to the threshold values to allow for inconsistancies of the input message 
    *lowest_space = 1.2 * lowest_Space_Count;   // Length of space between a dot and dash with in a Morse Code pattern (Anything greater is a end of a pattern)
    return 1.2 * lowest_Voltage_Count; // Length of a dot (Anything greater is a dash)

}


void Conversion(Voltage_List ** head){
    printf("................................................\n");
    printf("Converting Morse Code Message now\n");
    

    int DotDash_Count = 0; // used to count the length of a dash or dot
    int Space_Count = 0;

    int Space_Length; // Length of space between a dot and dash with in a Morse Code pattern (Anything greater is a end of a pattern)
    int Average; // Averge/Middle value of the voltages
    
    int Dot_Length = DashDot(Head_Voltage,&Average,&Space_Length); // Length of a dot (Anything greater is a dash)
    
    char morseCode_Current[8]; // this is used to store the current 0's and 1's to compare later
    int morseCode_Current_CHECK = 0; // This is used to see if the temp array has any values or is just NULL
                                     // 1 if has any data and 0 if NULL
    int morseCode_Current_COUNT = 0; // counter for the array

    int previous_Voltage_Value = 0;
    printf("\n");
    printf("Dot Length: %d\n",Dot_Length);
    printf("Space Length: %d\n",Space_Length);
    printf("Average: %d\n",Average);
    printf("\n");

    while ((*head)->next != NULL) {
        if ((*head)->data > Average){
            // Found WHITE 
            if (previous_Voltage_Value <= Average && previous_Voltage_Value != 0){
                // Moved from BLACK to WHITE

                // Analyse if the BLACK part is a dash or dot
                if (DotDash_Count > Dot_Length){
                    // Found a DASH
                    morseCode_Current[morseCode_Current_COUNT] = '1'; // for a dash
                    morseCode_Current_COUNT += 1;
                    morseCode_Current_CHECK = 1;
                } else {
                    // Found a DOT
                    morseCode_Current[morseCode_Current_COUNT] = '0'; // for a dot
                    morseCode_Current_COUNT += 1;
                    morseCode_Current_CHECK = 1;
                }
                DotDash_Count = 0;  // Reset BLACK part counter
                Space_Count = 1; // Reset WHITE space count including current WHITE part
            }
            else{
                // Just counting WHITE
                Space_Count += 1;
            }
        } else if ((*head)->data <= Average) { 
            // Found BLACK

            if (previous_Voltage_Value > Average && previous_Voltage_Value != 0){
                // Moved from WHITE to BLACK

                // Analyse if the WHITE part is a short or long space
                if (Space_Count > Space_Length && morseCode_Current_CHECK != 0){
                    // Found a long space meaning end of a alphanumeric symbol
                    morseCode_Current[morseCode_Current_COUNT] = '.';
                    int stop = 0;
                    for (int i = 0; i<37 && stop == 0; i++){
                        for (int j = 0; j<7; j++){
                            
                            if (morseCode_Current[j] == morseCode[i][j]){
                                if (j == 6){
                                    // Found a ' ' or space between words in a message
                                    pushMessage(Head_Message, symbol[i]);
                                    stop = 1;
                                    break;
                                } else if (morseCode_Current[j] == '.' && morseCode[i][j] == '.') {
                                    // Found a matching pattern thats less than 7 units long
                                    pushMessage(Head_Message, symbol[i]);
                                    stop = 1;
                                    break;
                                } else{
                                    // keep checking to see if remainder matches
                                    continue;
                                }
                            } else if (morseCode_Current[j] != morseCode[i][j]) {
                                
                                break; // used to break the inner for loop and continue to the outer loop
                            }
                        
                        }
                    }
                    memset(morseCode_Current, 0, 8);
                 
                    
                    morseCode_Current_COUNT = 0; // reset temp array counter
                }
                Space_Count = 0;  // Reset WHITE part counter
                DotDash_Count = 1; // Reset BLACK space count including current BLACK part

            }
            else{
                // Just counting BLACK
                DotDash_Count += 1;
            }
            
        }


        previous_Voltage_Value = (*head)->data;  // Save the current value for use later

        // Frees the data from the linked list
        Voltage_List * next_Voltage_Value = NULL;
        next_Voltage_Value = (*head)->next;
        free(*head);
        *head = next_Voltage_Value; 
        Voltage_List_COUNT -= 1;
    }



    // Run the code below again to convert the end BLACK part
    int stop = 0;
    morseCode_Current[morseCode_Current_COUNT] = '.';
    for (int i = 0; i<37 && stop == 0; i++){
                
        for (int j = 0; j<7; j++){
               
            if (morseCode_Current[j] == morseCode[i][j]){
                if (j == 6){
                    // Found a ' ' or space between words in a message
                    pushMessage(Head_Message, symbol[i]);
                    stop = 1;
                    break;
                } else if (morseCode_Current[j] == '.' && morseCode[i][j] == '.') {
                    // Found a matching pattern thats less than 7 units long
                    pushMessage(Head_Message, symbol[i]);
                    stop = 1;
                    break;
                } else{
                    // keep checking to see if remainder matches
                    continue;
                }
            } else if (morseCode_Current[j] != morseCode[i][j]) {
              
                break; // used to break the inner for loop and continue to the outer loop
            }
                       
        }
    }
    memset(morseCode_Current, 0, 8);
    
}

void Output(){
    
    printf("\nThe converted Morse Code Message is shown below: \n");
    printf("________________________________________________\n");
    while(Message_List_COUNT !=0){

        char temp = popMessage(&Head_Message);
        printf("%c",temp);
        

    }
    printf("\n");
    printf("________________________________________________\n");
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
    linkedList_Init();  // Create the first element of the linked list
     
     wiringPiSetupGpio();    // This sets the pin numbering system to the BCM pin number system
     
     // BUTTON PIN SETUP 
     pinMode(BUTTON_PIN, INPUT);        // Sets the pin to recieve an input
     pullUpDnControl(BUTTON_PIN, PUD_UP);      // Enables the pull down resistor on the button 

     signal(SIGINT, Termination_Handler); // This catches the termination ctrl-c in terminal
     enableADC(); // Sets up the ADC and ONLY transfers the data not saves as of yet
     
     pinMode(LED_PIN_1,OUTPUT);
     pinMode(LED_PIN_2,OUTPUT);
     pinMode(LED_ON,OUTPUT);
     digitalWrite(LED_PIN_2,LOW);
     digitalWrite(LED_PIN_1,LOW);
     digitalWrite(LED_ON,LOW);

     while(Program_Mode){
        wiringPiISR(BUTTON_PIN, INT_EDGE_BOTH, &buttonInterrupt);  // Sets the button listener to call the interupt method when pressed
         
         
        if (Program_Mode == 1){
            pushVoltage(Head_Voltage);
          // printf("%d\n", analogRead(ADC_CHANNEL));
        }


      /*  if (Voltage_List_COUNT != 1 && Program_Mode == 2){
                int average;
                int space;
                int dot = DashDot(Head_Voltage,&average,&space);
                printf("Avg = %d, space = %d, dot = %d \n",average,space,dot);
                return 0;
        } */
        if (Voltage_List_COUNT != 1 && Program_Mode == 2){
                Conversion(&Head_Voltage);
                Output();
                Termination_Handler();
                
        }
     
     }

return 0;
}