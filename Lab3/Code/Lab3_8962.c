//	Lab3 Code for LM3S8962
//	Richard Szeto & Sean Ho

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "drivers/rit128x96x4.h"

#include "inc/lm3s8962.h"
#include "driverlib/systick.h"

#define CODE_SIZE 16			// size of sequence from controller

volatile int waitTime;			// used to determine IR delay
volatile int waitTime2;			// used to determine delay between button presses
volatile int screenX = 0;		// OLED display x coordinate
volatile int screenY = 0;		// OLED display y coordinate
int flag = 0;				// flag for button press delay
int code[CODE_SIZE];			// array to hold sequence from controller
int code_offset = 0;			// placeholder for position in sequence array
char string[CODE_SIZE];			// array used to convert int to ascii
char display[50];			// OLED display buffer
int dLocx=0;				// OLED display buffer offset
char buffer[100];
char display2[50];

//prototypes
char decode (char* input);		// interpret sequence data
void decodeLetter (char input);		// output corresponding character to OLED display
int getDigit (void);			// return binary number corresponding to pulse delay


//UART1 interrupt handler
void
UARTIntHandler1 () {
		unsigned long ulStatus;
		unsigned long c;
		int display_offset = 0;
		int i;
		int errorFlag = 0;

    // Get the interrrupt status.
    ulStatus = UARTIntStatus(UART1_BASE, true);


    // Clear the asserted interrupts.
    UARTIntClear(UART1_BASE, ulStatus);

    // Loop while there are characters in the receive FIFO.
    while(UARTCharsAvail(UART1_BASE))
    {
				c = UARTCharGet(UART1_BASE);
			
			
				display2[display_offset++] = (char)c;
				
    }
		
		display2[display_offset-1] = '\0';
		
		for(i = 0; i < (display_offset - 1); i++)
		{
			if(display2[i] == 'p')
			{
					errorFlag = 1;
			}
		}
		
		if(errorFlag == 1)
		{
			RIT128x96x4Clear();
			RIT128x96x4StringDraw("----------------------", 0, 50, 15);
			RIT128x96x4StringDraw("Error", 50, 75, 15);
			RIT128x96x4StringDraw(display, 0, 0, 15);
		}
		else
		{
			RIT128x96x4Clear();
			RIT128x96x4StringDraw("----------------------", 0, 50, 15);
			RIT128x96x4StringDraw(display, 0, 0, 15);
			RIT128x96x4StringDraw(display2, 0, 60, 15);
		}

}
	
//UART0 interrupt handler
void
UARTIntHandler0(void)
{
    unsigned long ulStatus;

    // Get the interrrupt status.
    ulStatus = UARTIntStatus(UART0_BASE, true);

    // Clear the asserted interrupts.
    UARTIntClear(UART0_BASE, ulStatus);


}


//sends data through UART and then XBee
void
UARTSend(const unsigned char *pucBuffer, unsigned long ulCount)
{
  
	unsigned char p;
    	while(ulCount--)
    	{
		p = *pucBuffer;
		*pucBuffer++;

				
	UARTCharPut(UART1_BASE, p);
    	}
}

//create buffer array to send through XBee
void createBuffer ()
{
        unsigned char sum=0, size;
        int i;
        size = dLocx;
        
        buffer[0]=0x7e;//delimiter
        buffer[1]=0x00;//length
        buffer[2]=size + 5;
        buffer[3]=0x01;//API identifier
        buffer[4]=0x00;//frame id
        buffer[5]=0x00;//desintation address
        buffer[6]=0x01;//desintation address
        buffer[7]=0x01;//options
        for(i=0; i<size; i++)
                buffer[i+8]=display[i];
        
        
        
        
        for(i=3; i<8+size; i++)
        {
                sum += buffer[i];
        }
        buffer[size+8]= 0xff - sum;
        
}

//displays Error
void showError (void) {			// OLED display

	RIT128x96x4StringDraw("Error", 50, 25, 15);

}

// compare last 8 bits of IR pulse sequence
int myStrCmp(char string2[], char given[])	
{
	int i;
	for(i=0; (i < 8) && (string2[i] == given[i]); i++);
	
	if(i && (i == 8) && (given[i]=='\0'))
			return 1;
	else 
			return 0;
}


//increments waitTime and waitTime2 every period
void SysTickHandler (){
    waitTime += 1;
    waitTime2 += 1;
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

//get data from XBee
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

//get Digit from IR
int getDigit ()					// determine binary number from pulse delay
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
void PortBIntHandler (void)	// Interrupt handler called upon IR receive
{
	// Reset delay counter
	waitTime=0;		

	// Turn on Status light upon first IR receive
	GPIOPinWrite(GPIO_PORTF_BASE,GPIO_PIN_0,0x01);	
	
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
	
	// reset delay between button presses
	waitTime2=0;
}
int
main(void)
{
	display[0] = '\0';
	display2[0] = '\0';
    	// Set the clocking to run directly from the crystal.
    	SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_8MHZ);

    	// Initialize the OLED display and write status.
    	RIT128x96x4Init(1000000);
	RIT128x96x4StringDraw("----------------------", 0, 50, 15);

    	// Enable the peripherals used by this example.

    	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1);
    	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);		
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
		
	// Status
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	GPIOPadConfigSet(GPIO_PORTF_BASE,GPIO_PIN_0,GPIO_STRENGTH_4MA,GPIO_PIN_TYPE_STD);
	GPIODirModeSet(GPIO_PORTF_BASE,GPIO_PIN_0,GPIO_DIR_MODE_OUT);
		
	//PB1
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
	GPIOPadConfigSet(GPIO_PORTB_BASE,GPIO_PIN_1,GPIO_STRENGTH_4MA,GPIO_PIN_TYPE_STD);
	GPIODirModeSet(GPIO_PORTB_BASE,GPIO_PIN_1,GPIO_DIR_MODE_IN);
		
	GPIOPortIntRegister(GPIO_PORTB_BASE, PortBIntHandler);
	GPIOIntTypeSet(GPIO_PORTB_BASE, GPIO_PIN_1, GPIO_BOTH_EDGES);
	GPIOPinIntEnable(GPIO_PORTB_BASE, GPIO_PIN_1);
		
	IntPrioritySet(INT_GPIOB,0x80);
	SysTickIntRegister(SysTickHandler);                  
	SysTickPeriodSet(SysCtlClockGet()/10000);	// 0.1ms
	SysTickIntEnable();
	waitTime = 0;					// initialize
	waitTime2 = 0;
	SysTickEnable();


    	// Enable processor interrupts.
    	IntMasterEnable();

    	// Set GPIO A0 and A1 as UART pins.
    	GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
	GPIOPinTypeUART(GPIO_PORTD_BASE, GPIO_PIN_2 | GPIO_PIN_3);

    	// Configure the UART for 115,200, 8-N-1 operation.
    	UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 9600,
                        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                         UART_CONFIG_PAR_NONE));
												 
	UARTConfigSetExpClk(UART1_BASE, SysCtlClockGet(), 9600,
                  	(UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                         UART_CONFIG_PAR_NONE));

   	 // Enable the UART interrupt.
    	IntEnable(INT_UART0);
   	UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);
		IntEnable(INT_UART1);
    	UARTIntEnable(UART1_BASE, UART_INT_RX | UART_INT_RT);



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
			// output display buffer to OLED display
			int i;			
			createBuffer();
      UARTSend (buffer, dLocx+9);
		
			RIT128x96x4Clear();
			RIT128x96x4StringDraw("----------------------", 0, 50, 15);
			RIT128x96x4StringDraw(display, 0, 0, 15);
			RIT128x96x4StringDraw(display2, 0, 60, 15);
		
      for(i=0; i<dLocx + 1; i++)
				display[i]='\0';
      dLocx=0;
      return;
	}
	else if(input == 'P')
	{
		showError();		// Error
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
			{
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
