// Lab 4 LM3S2110 
// Sean Ho & Richard Szeto
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/pwm.h"
#include "driverlib/sysctl.h"
#include "drivers/rit128x96x4.h"


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

#define CODE_SIZE 16                    // size of sequence from controller

volatile int waitTime;                  // used to determine IR delay
volatile int waitTime2;                 // used to determine delay between button presses
volatile int pwmOffset1=0;              // tilt pwm offset
volatile int pwmOffset2=0;              // pan pwm offset
int flag = 0;                           // flag for button press delay
int code_offset = 0;                    // placeholder for position in sequence array
char string[CODE_SIZE];                 // array used to convert int to ascii
char display2[CODE_SIZE];               // used to store the payload from the Uart

//prototypes
char decode (char* input);              // interpret sequence data
void checkBounds ();                    // check the bounds of the servo
void move();                            // change pulse width to control servo

int myStrCmp(char string2[], char given[])      // compare last 8 bits of IR pulse sequence
{
        int i;
        for(i=0; (i < 8) && (string2[i] == given[i]); i++);
        
        if(i && (i == 8) && (given[i]=='\0'))
                        return 1;
        else 
                        return 0;
}


void SysTickHandler (){
    waitTime += 1;
    waitTime2 += 1;
	
            
}

// adjust the angle of the servo
void adjustAngle (char *str)
{
	
	float percent, percent2;
	int i, num, angle1=0, angle2=0;
	for(i=0;str[i]!=' ';i++)
	{
		num=angle1*10 + (str[i] - '0');
		angle1=num;
	}
	i++;		
	for(i;str[i]!='\0';i++)
	{
		num=angle2*10 + (str[i] - '0');
		angle2=num;
	}
	
	percent = angle1;
	percent = percent/360 * 100 * 98/100;
	percent -= 49;
	pwmOffset2 = percent;
	
	percent = angle2;
	percent2 = percent/120 * 100;
	percent = percent2;
	percent -= 50;
	pwmOffset1 = percent;
	if(angle2==121)
		GPIOPinWrite(GPIO_PORTF_BASE,GPIO_PIN_2,0x04);
	move ();
}
//UART interrupt handler
void UARTIntHandler0 ()
{
		unsigned long ulStatus;
		unsigned long c;
	
		int display_offset = 0;
		int size, i;
    ulStatus = UARTIntStatus(UART0_BASE, true);
    UARTIntClear(UART0_BASE, ulStatus);

    if(UARTCharsAvail(UART0_BASE))
    {
				UARTCharGet(UART0_BASE);//0
				UARTCharGet(UART0_BASE);//1
				size = UARTCharGet(UART0_BASE) - 5;//2
				UARTCharGet(UART0_BASE);//3
				UARTCharGet(UART0_BASE);//4
				UARTCharGet(UART0_BASE);//5
				UARTCharGet(UART0_BASE);//6
				UARTCharGet(UART0_BASE);//7
			i=0;
				for(display_offset=0; display_offset <size; display_offset++)
				{
					i++;
						c = UARTCharGet(UART0_BASE);
						display2[display_offset] = c;
				}
				UARTCharGet(UART0_BASE);
				
    }
		display2[size] = '\0';
		
		adjustAngle(display2);
		

		while(UARTCharsAvail(UART0_BASE))
				UARTCharGet(UART0_BASE);
		
		
		GPIOPinIntClear(GPIO_PORTB_BASE, GPIO_PIN_1);
}

int checkProtocol(){                    // skip repeating pulse sequences
    
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

void getData()                     // collect binary numbers
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

// checks bounds of the servo
void checkBounds ()
{
				if(pwmOffset2>49)
					pwmOffset2 = 49;
				else if(pwmOffset2<-49)
					pwmOffset2 = -49;
				
				if(pwmOffset1>50)
					pwmOffset1 = 50;
				else if(pwmOffset1<-50)
					pwmOffset1 = -50;
}

// change pulse width to move serve
void move ()
{
				checkBounds();
				PWMPulseWidthSet(PWM_BASE, PWM_OUT_0, SysCtlClockGet() * (0.0015 + 0.00001*pwmOffset1)/8);
				PWMPulseWidthSet(PWM_BASE, PWM_OUT_1, SysCtlClockGet() * (0.0015 + 0.00001*pwmOffset2)/8);
}

// Interrupt handler called upon IR receive
void PortBIntHandler (void)     
{
        
        unsigned long ulStatus;
				int i;
        // Reset delay counter
        waitTime=0;             
        // skip repeating IR pulse sequences
        if(!checkProtocol())                                                    
                return;
        
        // parse IR pulse sequence data
        getData();
        

				if(decode(string)=='L')
					pwmOffset2 -= 1;
				else if(decode(string)=='R')
					pwmOffset2 += 1;
				else if (decode(string)=='U')
					pwmOffset1 += 10;
				else if (decode(string)=='D')
					pwmOffset1 -= 10;
				
				checkBounds ();
				move ();
        
        // clear interrupt
      GPIOPinIntClear(GPIO_PORTB_BASE, GPIO_PIN_1);
			ulStatus = UARTIntStatus(UART0_BASE, true);
			UARTIntClear(UART0_BASE, ulStatus);
        // reset delay between button presses
        waitTime2=0;
}

int
main(void)
{
    unsigned long ulPeriod;

	  //
    // Set the clocking to run directly from the crystal.
    //
    SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_8MHZ);
    SysCtlPWMClockSet(SYSCTL_PWMDIV_8);
	
     //PB1
     SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
     GPIOPadConfigSet(GPIO_PORTB_BASE,GPIO_PIN_1,GPIO_STRENGTH_4MA,GPIO_PIN_TYPE_STD);
     GPIODirModeSet(GPIO_PORTB_BASE,GPIO_PIN_1,GPIO_DIR_MODE_IN);
     GPIOPortIntRegister(GPIO_PORTB_BASE, PortBIntHandler);
     GPIOIntTypeSet(GPIO_PORTB_BASE, GPIO_PIN_1, GPIO_BOTH_EDGES);
     GPIOPinIntEnable(GPIO_PORTB_BASE, GPIO_PIN_1);
	
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
		SysTickIntRegister(SysTickHandler);                  
    SysTickPeriodSet(SysCtlClockGet()/10000);   // 0.1ms
    SysTickIntEnable();
    waitTime = 0;                   // initialize
    waitTime2 = 0;
    SysTickEnable();

	
			// Status
		SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
		GPIOPadConfigSet(GPIO_PORTF_BASE,GPIO_PIN_2,GPIO_STRENGTH_4MA,GPIO_PIN_TYPE_STD);
    GPIODirModeSet(GPIO_PORTF_BASE,GPIO_PIN_2,GPIO_DIR_MODE_OUT); 



		//pwm
    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_0);
    GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_1);
		PWMGenConfigure(PWM_BASE, PWM_GEN_0,
		PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);
		ulPeriod = SysCtlClockGet()/50/8;
		PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, ulPeriod);
		PWMPulseWidthSet(PWM_BASE, PWM_OUT_0, SysCtlClockGet() * 0.0015/8);
		PWMPulseWidthSet(PWM_BASE, PWM_OUT_1, SysCtlClockGet() * 0.0015/8);
		PWMGenEnable(PWM_BASE, PWM_GEN_0);
		PWMOutputState(PWM_BASE, (PWM_OUT_0_BIT | PWM_OUT_1_BIT), true);



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

