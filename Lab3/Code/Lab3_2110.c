//  Lab3 Code for LM3S2110
//  Richard Szeto & Sean Ho

#include "inc/lm3s8962.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "drivers/rit128x96x4.h"
#include "driverlib/systick.h"
#include "driverlib/uart.h"

#define CODE_SIZE 16			// size of sequence from controller

volatile int waitTime;			// used to determine IR delay
volatile int waitTime2;			// used to determine delay between button presses
volatile int morseCounter;	// used to time morse code
volatile int screenX = 12;		// OLED display x coordinate
volatile int screenY = 60;		// OLED display y coordinate
int flag = 0;				// flag for button press delay
int code[CODE_SIZE];			// array to hold sequence from controller
int code_offset = 0;			// placeholder for position in sequence array
char string[CODE_SIZE];			// array used to convert int to ascii
char display[50];			// OLED display buffer
char display2[50];
char dLocx=0;				// OLED display buffer offset
char buffer[100];

//prototypes
char decode (char* input);		// interpret sequence data
void decodeLetter (char input);		// output corresponding character to OLED display
int getDigit (void);			// return binary number corresponding to pulse delay
void morseLetter (char a); 		//convert letter to morse

//create a space in between dots and dashes
void space ()
{

        morseCounter = 0;
        GPIOPinWrite(GPIO_PORTF_BASE,GPIO_PIN_2,0x00);
        while(morseCounter <10000)
        {
	}		
				
}

//status light dash
void dash ()
{

        morseCounter=0;
        GPIOPinWrite(GPIO_PORTF_BASE,GPIO_PIN_2,0x04);
        while(morseCounter < 15000)
        {
        }
				 
        space();
}

//status light dot
void dot ()
{
	
	
        morseCounter=0;
        GPIOPinWrite(GPIO_PORTF_BASE,GPIO_PIN_2,0x04);
        while(morseCounter<5000)
        {
        }
                     
        space();
}



int myStrCmp(char string2[], char given[])      // compare last 8 bits of IR pulse sequence
{
        int i;
        for(i=0; (i < 8) && (string2[i] == given[i]); i++);
        
        if(i && (i == 8) && (given[i]=='\0'))
                        return 1;
        else 
                        return 0;
}

//converts sting to display
void morseDisplay (char *string, int length)
{
        int i;
        for (i=0; i<length; i++)
        {

                morseLetter(string[i]);
        }
        space();
        space();
        
}

//increments waitTime and waitTime2 every period
void SysTickHandler (){
    waitTime += 1;
		waitTime2 += 1;
		morseCounter +=1;
}

 


//create buffer array to be sent through Xbee
void createBuffer ()
{
	unsigned char sum=0, size;
	int i;
	size = dLocx;
	
	buffer[0]=0x7e;//delimiter
	buffer[1]=0x00;//length msb
	buffer[2]=size + 5;//length lsb
	buffer[3]=0x01;//API identifier
	buffer[4]=0x00;//frame id
	buffer[5]=0x00;//desintation address
	buffer[6]=0x00;//desintation address
	buffer[7]=0x01;//options
	for(i=0; i<size; i++)
		buffer[i+8]=display[i];
	

	
	
	
	for(i=3; i<8+size; i++)
	{
		sum += buffer[i];
	}
	buffer[size+8]= 0xff - sum;
	
}
int checkProtocol(){			// skip repeating pulse sequences
    
	while(GPIOPinRead(GPIO_PORTB_BASE,GPIO_PIN_1))
	{
	}//90
	if(waitTime <80)
		return 0;
	waitTime=0;
	while(!GPIOPinRead(GPIO_PORTB_BASE,GPIO_PIN_1))
	{
	}
	waitTime=0;
	while(GPIOPinRead(GPIO_PORTB_BASE,GPIO_PIN_1))
	{       
	}//24
	if(waitTime <20)
		return 0;
	else
		return 1;
        
}
void getData()					// collect binary numbers
{
	int i, k;
	k =getDigit();
	code_offset=0;
	for(i=0; i<16;i++)
	{
		if(i>=8)
			string[code_offset++]=k + '0';
		
		k=getDigit();
	}
}

// determine binary number from pulse delay
int getDigit ()					
{
	waitTime=0;
	while(!GPIOPinRead(GPIO_PORTB_BASE,GPIO_PIN_1))
	{
	}
	if(waitTime >6)
		return 2;
	waitTime=0;
	while(GPIOPinRead(GPIO_PORTB_BASE,GPIO_PIN_1))
	{       
	}
	if(waitTime>14)
		return 1;
	else
		return 0;
    
}

//sends data through UART and then through XBee
void
UARTSend(const unsigned char *pucBuffer, unsigned long ulCount)
{
    //
    // Loop while there are more characters to send.
    //
		int i;
		int size = pucBuffer[2] - 5;
		char stringArray[50];
		int errorFlag = 0;
	
		
		
		for(i = 8; i < size + 8; i++)
		{
			stringArray[i - 8] = pucBuffer[i];
			if(pucBuffer[i] == 'p')
				errorFlag = 1;
		}
		
	
		
    while(ulCount--)
    {
				
        //
        // Write the next character to the UART.
        //
       UARTCharPut(UART0_BASE, *pucBuffer++);
    }
		if(errorFlag == 0)
			morseDisplay(stringArray,size);
}

// Interrupt handler called upon IR receive
void PortBIntHandler (void)	
{
	
	unsigned long ulStatus;
	// Reset delay counter
	waitTime=0;		
	
	// skip repeating IR pulse sequences
	if(!checkProtocol())							
		return;
		
	// 2 second delay between button presses	
	if(waitTime2 < 20000)							
	{
		flag = 1;
	}
	else
	{
		flag = 0;
	}
	
	
	// parse IR pulse sequence data
	getData();
	
	// concatenate corresponding character to display buffer
	decodeLetter(decode(string));

	
	// clear interrupt
	GPIOPinIntClear(GPIO_PORTB_BASE, GPIO_PIN_1);
     ulStatus = UARTIntStatus(UART0_BASE, true);
    UARTIntClear(UART0_BASE, ulStatus);
	
	// reset delay between button presses
	waitTime2=0;
}

//UART interrupt handler
void UARTIntHandler0 ()
{
		unsigned long ulStatus;
		unsigned long c;
		int display_offset = 0;
		int buffer_offset = 0;
		int size;

    //
    // Get the interrrupt status.
    //
    ulStatus = UARTIntStatus(UART0_BASE, true);

    //
    // Clear the asserted interrupts.
    //
    UARTIntClear(UART0_BASE, ulStatus);

    //
    // Loop while there are characters in the receive FIFO.
    //
    while(UARTCharsAvail(UART0_BASE))
    {
				c = UARTCharGet(UART0_BASE);
			
				//
        // Read the next character from the UART and write it back to the UART.
        //
				if(buffer_offset == 2)
				{
					size = c - 5;
				}
				
				if(buffer_offset >= 8)
				{
						display2[display_offset++] = (char)c;
				}
				buffer_offset++;
				
    }
		display2[size] = '\0';
		
		morseDisplay(display2,size);
		
		GPIOPinIntClear(GPIO_PORTB_BASE, GPIO_PIN_1);
}



int
main(void)
{

	
		
	//set clock	
	SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_8MHZ);


   
        

     //PB1
     SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
     GPIOPadConfigSet(GPIO_PORTB_BASE,GPIO_PIN_1,GPIO_STRENGTH_4MA,GPIO_PIN_TYPE_STD);
     GPIODirModeSet(GPIO_PORTB_BASE,GPIO_PIN_1,GPIO_DIR_MODE_IN);
     GPIOPortIntRegister(GPIO_PORTB_BASE, PortBIntHandler);
     GPIOIntTypeSet(GPIO_PORTB_BASE, GPIO_PIN_1, GPIO_BOTH_EDGES);
     GPIOPinIntEnable(GPIO_PORTB_BASE, GPIO_PIN_1);

        
    // Status
                SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    GPIOPadConfigSet(GPIO_PORTF_BASE,GPIO_PIN_2,GPIO_STRENGTH_4MA,GPIO_PIN_TYPE_STD);
        GPIODirModeSet(GPIO_PORTF_BASE,GPIO_PIN_2,GPIO_DIR_MODE_OUT);  

    //UART
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 9600,
    (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                   UART_CONFIG_PAR_NONE));
    IntEnable(INT_UART0);
    UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);
   
    IntPrioritySet(INT_UART0, 0x7F);
                
    IntPrioritySet(INT_GPIOB,0x80);
    IntMasterEnable();
    SysTickIntRegister(SysTickHandler);                  
    SysTickPeriodSet(SysCtlClockGet()/10000);   // 0.1ms
    SysTickIntEnable();
    waitTime = 0;                   // initialize
    waitTime2 = 0;
    SysTickEnable();
    while(1)
    {
 

    }
    
}

// interpret sequence data
char decode (char* input) {							
        
	if (myStrCmp(input,"10000100"))
	{
		return '1';
	}
	else if (myStrCmp(input,"01000100"))
	{
	   return '2';
	}
	else if (myStrCmp(input,"11000100"))
	{
	   return '3';
	}
	else if (myStrCmp(input,"00100100"))
	{
	   return '4';
	}
	else if (myStrCmp(input,"10100100"))
	{
	   return '5';
	}
	else if (myStrCmp(input,"01100100"))
	{
	   return '6';
	}
	else if (myStrCmp(input,"11100100"))
	{
	   return '7';
	}
	else if (myStrCmp(input,"00010100"))
	{
	   return '8';
	}
	else if (myStrCmp(input,"10010100"))
	{
	   return '9';
	}
	else if (myStrCmp(input,"00000100"))
	{
	   return '0';
	}
	else if (myStrCmp(input,"10011000"))
	{
	   return 'U';
	}
	else if (myStrCmp(input,"11111000"))
	{
	   return 'L';
	}
	else if (myStrCmp(input,"01111000"))
	{
	   return 'R';
	}
	else if (myStrCmp(input,"00011000"))
	{
	   return 'D';
	}
	else if (myStrCmp(input,"01100111")) // Delete
	{
	   return 'T';
	}
	else if (myStrCmp(input,"11001001")) // Last
	{			
			return 'S';
	}
	else
	{
		return 'P';
	}
 }
 
// convert button to character output
 void decodeLetter (char input) {
	 int i;
	if (input == '1')
	{
		display[dLocx++] = '1';
	}
	else if (input == 'U')
	{
		screenY--;
	}
	else if (input == 'D')
	{
		screenY++;
	}
	else if (input == 'L')
	{
		screenX--;
	}
	else if (input == 'R')
	{
		screenX++;
	}
	else if (input == 'T')
	{
		if(dLocx != 0)
		{
			dLocx--;
			display[dLocx] = '\0';
		}
	}
	else if (input == 'S')
	{
		createBuffer();
		UARTSend (buffer, dLocx+9);
		for(i=0; i<dLocx + 1; i++)
			display[i]='\0';
		dLocx=0;
		return;
	}
	else if(input == 'P')
	{
				// Error
		display[dLocx++] = 'p';
	}
	
	if (dLocx != 0)
	{
		if (input == '2')
		{
			if (flag == 1)
			{
				if (display[dLocx - 1] == 'A')
					display[dLocx - 1] = 'B';
				else if (display[dLocx - 1] == 'B')
					display[dLocx - 1] = 'C';
				else if (display[dLocx - 1] == 'C')
					display[dLocx - 1] = '2';
				else if (display[dLocx - 1] == '2')
					display[dLocx - 1] = 'A';
				else
					display[dLocx++] = 'A';
				
			}
			else
			{
				display[dLocx++] = 'A';
			}
		}
		else if (input == '3')
		{
			if (flag == 1)
			{
				if (display[dLocx - 1] == 'D')
					display[dLocx - 1] = 'E';
				else if (display[dLocx - 1] == 'E')
					display[dLocx - 1] = 'F';
				else if (display[dLocx - 1] == 'F')
					display[dLocx - 1] = '3';
				else if (display[dLocx - 1] == '3')
					display[dLocx - 1] = 'D';
				else
					display[dLocx++] = 'D';
			}
			else
	efficency		{
				display[dLocx++] = 'D';
			}
		}
		else if (input == '4')
		{
			if (flag == 1)
			{   
				if (display[dLocx - 1] == 'G')
					display[dLocx - 1] = 'H';
				else if (display[dLocx - 1] == 'H')
					display[dLocx - 1] = 'I';
				else if (display[dLocx - 1] == 'I')
					display[dLocx - 1] = '4';
				else if (display[dLocx - 1] == '4')
					display[dLocx - 1] = 'G';
				else
					display[dLocx++] = 'G';
			}
			else
			{
				display[dLocx++] = 'G';
			}
		}
		else if (input == '5')
		{       
			if (flag == 1)
			{
				if (display[dLocx - 1] == 'J')
					display[dLocx - 1] = 'K';
				else if (display[dLocx - 1] == 'K')
					display[dLocx - 1] = 'L';
				else if (display[dLocx - 1] == 'L')
					display[dLocx - 1] = '5';
				else if (display[dLocx - 1] == '5')
					display[dLocx - 1] = 'J';
				else
					display[dLocx++] = 'J';
			}
			else
			{
				display[dLocx++] = 'J';
			}
		}
		else if (input == '6')
		{
			if (flag == 1)  
			{   
				if (display[dLocx - 1] == 'M')
					display[dLocx - 1] = 'N';
				else if (display[dLocx - 1] == 'N')
					display[dLocx - 1] = 'O';
				else if (display[dLocx - 1] == 'O')
					display[dLocx - 1] = '6';
				else if (display[dLocx - 1] == '6')
					display[dLocx - 1] = 'M';
				else
					display[dLocx++] = 'M';
			}
			else
			{
				display[dLocx++] = 'M';
			}
		}
		else if (input == '7')
		{
			if (flag == 1)
			{
				if (display[dLocx - 1] == 'P')
					display[dLocx - 1] = 'Q';
				else if (display[dLocx - 1] == 'Q')
					display[dLocx - 1] = 'R';
				else if (display[dLocx - 1] == 'R')
					display[dLocx - 1] = 'S';
				else if (display[dLocx - 1] == 'S')
					display[dLocx - 1] = '7';
				else if (display[dLocx - 1] == '7')
					display[dLocx - 1] = 'P';
				else
					display[dLocx++] = 'P';
			}
			else
			{
				display[dLocx++] = 'P';
			}
		}
		else if (input == '8')
		{
			if (flag == 1)
			{
				if (display[dLocx - 1] == 'T')
					display[dLocx - 1] = 'U';
				else if (display[dLocx - 1] == 'U')
					display[dLocx - 1] = 'V';
				else if (display[dLocx - 1] == 'V')
					display[dLocx - 1] = '8';
				else if (display[dLocx - 1] == '8')
					display[dLocx - 1] = 'T';
				else
					display[dLocx++] = 'T';
			}
			else
			{
				display[dLocx++] = 'T';
			}
		}
		else if (input == '9')
		{
			if (flag == 1)
			{
				if (display[dLocx - 1] == 'W')
					display[dLocx - 1] = 'X';
				else if (display[dLocx - 1] == 'X')
					display[dLocx - 1] = 'Y';
				else if (display[dLocx - 1] == 'Y')
					display[dLocx - 1] = 'Z';
				else if (display[dLocx - 1] == 'Z')
					display[dLocx - 1] = '9';
				else if (display[dLocx - 1] == '9')
					display[dLocx - 1] = 'W';
				else
					display[dLocx++] = 'W';
			}
			else
			{
				display[dLocx++] = 'W';
			}
		}
		else if (input == '0')
		{
			if (flag == 1)
			{   
				if (display[dLocx - 1] == ' ')
					display[dLocx - 1] = '0';
				else if (display[dLocx - 1] == '0')
					display[dLocx - 1] = ' ';
				else
					display[dLocx++] = ' ';
			}
			else
			{
				display[dLocx++] = ' ';
			}
		}
	}
	else
	{
		if (input == '2')
		{
			display[dLocx++] = 'A';
		}
		else if (input == '3')
		{
			display[dLocx++] = 'D';
		}
		else if (input == '4')
		{
			display[dLocx++] = 'G';
		}
		else if (input == '5')
		{
			display[dLocx++] = 'J';
		}
		else if (input == '6')
		{
			display[dLocx++] = 'M';
		}
		else if (input == '7')
		{
			display[dLocx++] = 'P';
		}
		else if (input == '8')
		{
			display[dLocx++] = 'T';
		}
		else if (input == '9')
		{
			display[dLocx++] = 'W';
		}
		else if (input == '0')
		{
			display[dLocx++] = ' ';
		}
	}
	display[dLocx] = '\0';
 }       

void morseLetter (char a)
 {
        if(a=='A')
        {
                dot();
                dash();
        }else if (a=='B')
        {
                dash();
                dot();
                dot();
                dot();
                
        }else if (a=='C')
        {
                dash();
                dot();
                dash();
                dot();
                
        }else if (a=='D')
        {
                dash();
                dot();
                dot();
                
        }else if (a=='E')
        {
                dot();
        }else if (a=='F')
        {
                dot();
                dot();
                dash();
                dot();
        }else if (a=='G')
        {
                dash();
                dash();
                dot();
        }else if (a=='H')
        {
                dot();
                dot();
                dot();
                dot();
        }else if (a=='I')
        {
                dot();
                dot();
        }else if (a=='J')
        {
                dot();
                dash();
                dash();
                dash();
        }else if (a=='K')
        {
                dash();
                dot();
                dash();
        }else if (a=='L')
        {
                dot();
                dash();
                dot();
                dot();
        }else if (a=='M')
        {
                dash();
                dash();
        }else if (a=='N')
        {
                dash();
                dot();
        }else if (a=='O')
        {
                dash();
                dash();
                dash();
        }else if (a=='P')
        {
                dot();
                dash();
                dash();
                dot();
        }else if (a=='Q')
        {
                dash();
                dash();
                dot();
                dash();
        }else if (a=='R')
        {
                dot();
                dash();
                dot();
        }else if (a=='S')
        {
                dot();
                dot();
                dot();
        }else if (a=='T')
        {
                dash();
        }else if (a=='U')
        {
                dot();
                dot();
                dash();
        }else if (a=='V')
        {
                dot();
                dot();
                dot();
                dash();
        }else if (a=='W')
        {
                dot();
                dash();
                dash();
        }else if (a=='X')
        {
                dash();
                dot();
                dot();
                dash();
        }else if (a=='Y')
        {
                dash();
                dot();
                dash();
                dash();
        }else if (a=='Z')
        {
                dash();
                dash();
                dot();
                dot();
        }else if (a=='1')
        {
                dot();
                dash();
                dash();
                dash();
                dash();
        }else if (a=='2')
        {
                dot();
                dot();
                dash();
                dash();
                dash();
        }else if (a=='3')
        {
                dot();
                dot();
                dot();
                dash();
                dash();
        }else if (a=='4')
        {
                dot();
                dot();
                dot();
                dot();
                dash();
        }else if (a=='5')
        {
                dot();
                dot();
                dot();
                dot();
                dot();
        }else if (a=='6')
        {
                dash();
                dot();
                dot();
                dot();
                dot();
        }else if (a=='7')
        {
                dash();
                dash();
                dot();
                dot();
                dot();
        }else if (a=='8')
        {
                dash();
                dash();
                dash();
                dot();
                dot();
        }else if (a=='9')
        {
                dash();
                dash();
                dash();
                dash();
                dot();
        }else if (a=='0')
        {
                dash();
                dash();
                dash();
                dash();
                dash();
        }
	else if (a == ' ')
	{
		space();
		space();
	}
				
 }



