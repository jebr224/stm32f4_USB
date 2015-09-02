#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#include "stm32f4xx_conf.h"
#include "stm32f4xx.h"
#include "main.h"

#include "usbd_cdc_core.h"
#include "usbd_usr.h"
#include "usbd_desc.h"
#include "usbd_cdc_vcp.h"


#define CR 13 //carriage return

#define SOFTWARE_VERSION "0.0.1"
#define HARDWARE_VERSION "0.0.1"

// Private variables
volatile uint32_t time_var1, time_var2;
__ALIGN_BEGIN USB_OTG_CORE_HANDLE  USB_OTG_dev __ALIGN_END;

// Private function prototypes
void Delay(volatile uint32_t nCount);
void init();
int readString(char * myBuff);
void configCan1(int scaler);
void configCan2(void);



/*
table-1 (SCALE calculated for different CAN bus speeds)

(tqSpeed) = (bussSpeed) * (TQ)
scale = (clock) / (tqSpeed) 

+-----------+-----------+----+----------+-------+
| bitMB/sec | clock MHz | TQ | tqSpeed  | Scale |
+-----------+-----------+----+----------+-------+
|        10 |        42 | 14 |     0.14 |   300 |
|        20 |        42 | 14 |     0.28 |   150 |
|        50 |        42 | 14 |     0.7  |    60 |
|       100 |        42 | 14 |     1.4  |    30 |
|       125 |        42 | 14 |     1.75 |    24 |
|       250 |        42 | 14 |     3.5  |    12 |
|       500 |        42 | 14 |     7    |     6 |
|       750 |        42 | 14 |     10.5 |     4 | //Danger 750 is not 800
|      1000 |        42 | 14 |     14   |     3 |
+-----------+-----------+----+----------+-------+
*/

int main(void) {
	char usbStr [100] ;
	init();

	/*
	 * Disable STDOUT buffering. Otherwise nothing will be printed
	 * before a newline character or when the buffer is flushed.
	 */
	setbuf(stdout, NULL);
		
	while(1){
		//calculation_test();
		readString(usbStr);
		printf("top \n");

		switch(usbStr[0]){
			case 'V':{
			    printf( HARDWARE_VERSION );
				printf("\n");
				break;
			}
	
			case 'v':{
			    printf( SOFTWARE_VERSION );
				printf("\n");
				break;
			}
			case 't':{
				
				//char stdID[3];
				uint8_t TransmitMailbox = 0;
				CanTxMsg TxMessage;
				
			//	TxMessage.StdId = 
			//not done yet!	
				break;
			}
		
			case 'S': {
				printf("Setting Speed\n");
				switch (usbStr[1]) {
					// See table 1 for details.
					case '0': configCan1(300); break; //10 k bits/s
					case '1': configCan1(150); break; //20 k bits/s
					case '2': configCan1(60); break;  //50 k bits/s
					case '3': configCan1(30); break;  //100 k bits/s
					case '4': configCan1(24); break;  //125 k bits/s
					case '5': configCan1(12); break;  //250 k bits/s
					case '6': configCan1(6); break;   //500 k bits/s
					case '7': configCan1(4); break;   //750 k bits/s
					case '8': configCan1(3);  break;  //1 m bits/s
				}
				break;
			}
	
	
		//	printf("/r today /r");
			uint8_t TransmitMailbox = 0;
			CanTxMsg TxMessage;

			//configCan1();
			
			TxMessage.StdId = 0x123;
			TxMessage.ExtId = 0x00;
			TxMessage.RTR = CAN_RTR_DATA;
			TxMessage.IDE = CAN_ID_STD;
			TxMessage.DLC = 4;
	 
			TxMessage.Data[0] = 0x02;
			TxMessage.Data[1] = 0x11;
			TxMessage.Data[2] = 0x11;
			TxMessage.Data[3] = 0x11;
					
			  //send CAN1
			TransmitMailbox = CAN_Transmit(CAN1, &TxMessage);
			int i = 0;
			while((CAN_TransmitStatus(CAN1, TransmitMailbox) != CANTXOK) && (i != 0xFFFFFF)) // Wait on Transmit
			{
				i++;
			}
		
		}
	}
	
	//printf(usbStr);
	/*
	for(;;) {
		GPIO_SetBits(GPIOD, GPIO_Pin_12);
		Delay(500);
		GPIO_ResetBits(GPIOD, GPIO_Pin_12);
		Delay(500);
	}
*/
	return 0;
}


int readString(char * myBuff){
	int len = 0;
	do
	{
		myBuff[len] = getchar();
		len++;
	}while( myBuff[len] != CR );
	myBuff[len+1] = 0;
	len++;
	return len;
}

void clearBuff(char * myBuff){
	int i = 0;
	while(myBuff[i] != 0){
		myBuff[i] = 0;
		i++;
	}
}



/**
 * Parse hex value of given string
 *
 * @param line Input string
 * @param len Count of characters to interpret
 * @param value Pointer to variable for the resulting decoded value
 * @return 0 on error, 1 on success
 *
 * great artist steal
 */
unsigned char parseHex(char * line, unsigned char len, unsigned long * value) {
    *value = 0;
    while (len--) {
        if (*line == 0) return 0;
        *value <<= 4;
        if ((*line >= '0') && (*line <= '9')) {
           *value += *line - '0';
        } else if ((*line >= 'A') && (*line <= 'F')) {
           *value += *line - 'A' + 10;
        } else if ((*line >= 'a') && (*line <= 'f')) {
           *value += *line - 'a' + 10;
        } else {
			printf("Invalid attempt to convert char to hex \n");
			return 0;
		}
        line++;
    }
    return 1;
}




 /**@brief set up can1 
   *@param speed
   *@retval None
   */
void configCan1(int scaler){

  GPIO_InitTypeDef      GPIO_InitStructure;
  CAN_InitTypeDef       CAN_InitStructure;
  CAN_FilterInitTypeDef CAN_FilterInitStructure;
	
	
  /* CAN GPIOs configuration **************************************************/
  /* Enable GPIO clock */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
 
  /* Connect CAN pins */
  GPIO_PinAFConfig(GPIOD, GPIO_PinSource0, GPIO_AF_CAN1);
  GPIO_PinAFConfig(GPIOD, GPIO_PinSource1, GPIO_AF_CAN1);
 
  /* Configure CAN RX and TX pins */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(GPIOD, &GPIO_InitStructure);
 
  /* CAN configuration ********************************************************/
  /* Enable CAN clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);
 
  /* CAN register init */
  CAN_DeInit(CAN1);
  CAN_StructInit(&CAN_InitStructure);
 
  /* CAN cell init */
  CAN_InitStructure.CAN_TTCM = DISABLE;
  CAN_InitStructure.CAN_ABOM = DISABLE;
  CAN_InitStructure.CAN_AWUM = DISABLE;
  CAN_InitStructure.CAN_NART = DISABLE;
  CAN_InitStructure.CAN_RFLM = DISABLE;
  CAN_InitStructure.CAN_TXFP = DISABLE;
  CAN_InitStructure.CAN_Mode = CAN_Mode_Normal;
 
 
  /* quanta 1+6+7 = 14, 14 * 3 = 42, 42000000 / 42 = 1000000 */
  /* CAN Baudrate = 1Mbps (CAN clocked at 42 MHz) Prescale = 3 */
 
  /* Requires a clock with integer division into APB clock */
 
  CAN_InitStructure.CAN_SJW = CAN_SJW_1tq; // ? posible combos, I bet? // 1+6+7 = 14, 1+14+6 = 21, 1+15+5 = 21
  CAN_InitStructure.CAN_BS1 = CAN_BS1_6tq;  //nvm //4 + 3+ 1 = 8
  CAN_InitStructure.CAN_BS2 = CAN_BS2_7tq;
  //below is cleaver.  8MHz/14M = 0.571428571 = 0
  //CAN_InitStructure.CAN_Prescaler = 5;//?;1;//RCC_Clocks.PCLK1_Frequency / (14 * 1000000); // quanta by baudrate
  CAN_InitStructure.CAN_Prescaler = scaler; //3;//RCC_Clocks.PCLK1_Frequency / (14 * 1000000);
  CAN_Init(CAN1, &CAN_InitStructure);
 
  /* CAN filter init */
  CAN_FilterInitStructure.CAN_FilterNumber = 0; // CAN1 [ 0..13]
 
  CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask; // IdMask or IdList
  CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit; // 16 or 32
 
  CAN_FilterInitStructure.CAN_FilterIdHigh      = 0x0000; // Everything, otherwise 11-bit in top bits
  CAN_FilterInitStructure.CAN_FilterIdLow       = 0x0000;
  CAN_FilterInitStructure.CAN_FilterMaskIdHigh  = 0x0000;
  CAN_FilterInitStructure.CAN_FilterMaskIdLow   = 0x0000;
 
  CAN_FilterInitStructure.CAN_FilterFIFOAssignment = CAN_FIFO0; // Rx
  CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;
 
  CAN_FilterInit(&CAN_FilterInitStructure);
 
  return;
}
   
 /**@brief set up can1 
   *@param speed
   *@retval None
   */
void configCan2(){

  GPIO_InitTypeDef      GPIO_InitStructure;
  CAN_InitTypeDef       CAN_InitStructure;
  CAN_FilterInitTypeDef CAN_FilterInitStructure;
	
  /* CAN GPIOs configuration **************************************************/
  /* Enable GPIO clock */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
 
  /* Connect CAN pins */
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource5, GPIO_AF_CAN2);
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_CAN2);
 
  /* Configure CAN RX and TX pins */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
 
  /* CAN configuration ********************************************************/
  /* Enable CAN clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN2, ENABLE);
  /* CAN register init */
  CAN_DeInit(CAN2);
  CAN_StructInit(&CAN_InitStructure);
 
  /* CAN cell init */
  CAN_InitStructure.CAN_TTCM = DISABLE;
  CAN_InitStructure.CAN_ABOM = DISABLE;
  CAN_InitStructure.CAN_AWUM = DISABLE;
  CAN_InitStructure.CAN_NART = DISABLE;
  CAN_InitStructure.CAN_RFLM = DISABLE;
  CAN_InitStructure.CAN_TXFP = DISABLE;
  CAN_InitStructure.CAN_Mode = CAN_Mode_Normal;
 
 
 
  /* Requires a clock with integer division into APB clock */
 
  CAN_InitStructure.CAN_SJW = CAN_SJW_1tq; // ? posible combos, I bet? // 1+6+7 = 14, 1+14+6 = 21, 1+15+5 = 21
  CAN_InitStructure.CAN_BS1 = CAN_BS1_6tq;  //nvm //4 + 3+ 1 = 8
  CAN_InitStructure.CAN_BS2 = CAN_BS2_7tq;
  CAN_InitStructure.CAN_Prescaler = 3;
  CAN_Init(CAN2, &CAN_InitStructure);
 
  /* CAN filter init */
  CAN_FilterInitStructure.CAN_FilterNumber = 0; // CAN1 [ 0..13]
 
  CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask; // IdMask or IdList
  CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit; // 16 or 32
 
  CAN_FilterInitStructure.CAN_FilterIdHigh      = 0x0000; // Everything, otherwise 11-bit in top bits
  CAN_FilterInitStructure.CAN_FilterIdLow       = 0x0000;
  CAN_FilterInitStructure.CAN_FilterMaskIdHigh  = 0x0000;
  CAN_FilterInitStructure.CAN_FilterMaskIdLow   = 0x0000;
 
  CAN_FilterInitStructure.CAN_FilterFIFOAssignment = CAN_FIFO0; // Rx
  CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;
 
  CAN_FilterInit(&CAN_FilterInitStructure);
 
  return;
}


void init() {
	GPIO_InitTypeDef  GPIO_InitStructure;

	// ---------- SysTick timer -------- //
	if (SysTick_Config(SystemCoreClock / 1000)) {
		// Capture error
		while (1){};
	}

	// ---------- GPIO -------- //
	// GPIOD Periph clock enable
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

	// Configure PD12, PD13, PD14 and PD15 in output pushpull mode
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13| GPIO_Pin_14| GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	// ------------- USB -------------- //
	USBD_Init(&USB_OTG_dev,
	            USB_OTG_FS_CORE_ID,
	            &USR_desc,
	            &USBD_CDC_cb,
	            &USR_cb);
}

/*
 * Called from systick handler
 */
void timing_handler() {
	if (time_var1) {
		time_var1--;
	}

	time_var2++;
}

/*
 * Delay a number of systick cycles (1ms)
 */
void Delay(volatile uint32_t nCount) {
	time_var1 = nCount;
	while(time_var1){};
}

/*
 * Dummy function to avoid compiler error
 */
void _init() {

}

