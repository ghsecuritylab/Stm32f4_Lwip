/*
 * module.c
 *
 *  Created on: Aug 1, 2019
 *      Author: Heeze
 */

#ifndef MODULE_C_
#define MODULE_C_

#include "module.h"

/*-----------------------------------------------------------------------------------------------------------------*/
volatile char cIn;
osMessageQueueId_t rxQueue;
osMutexId_t moduleQueueMutex;
/*-----------------------------------------------------------------------------------------------------------------*/

extern UART_HandleTypeDef huart2;

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	osMessageQueuePut(rxQueue, (uint8_t*)&cIn, 0, 0);
	ITM_SendChar(cIn);
}

/*****************************************************************************************************************/
osMessageQueueId_t	moduleQueueInitialize(void)
{
  	rxQueue = osMessageQueueNew (256, 1, NULL);
	moduleQueueMutex = osMutexNew(NULL);
  	return rxQueue;
}
/*****************************************************************************************************************/

/*****************************************************************************************************************/

/*****************************************************************************************************************/
static int transmitATCmd(char* resp)
{
	int cmdLen  = strlen(resp);
    return HAL_UART_Transmit_DMA(&huart2, (uint8_t*)resp, cmdLen);
}

/*A line from the Queue storing the data received asynchronously*/
static int readline(char* buf, int timeout)
{
	int idx = 0;
	uint8_t data = 0;
	while(buf[idx] != '\n')
	{
/*
		if(osMutexAcquire (moduleQueueMutex, portMAX_DELAY ) == osOK)
		{
*/
			if(osMessageQueueGet(rxQueue, &data, 0, timeout) == osOK)
			{
				buf[idx++] = data;
			}
			else
			{
				break;
			}
/*
			osMutexRelease(moduleQueueMutex);
		}
*/
	}
	return idx;
}

static int moduleCheckResponse(char* resp)
{
	char lineBuffer[64] = {0};
	while(1)
	{
		if(readline(lineBuffer, 300) > 2)
		{
			if(strstr(lineBuffer, resp) == NULL)
			{
				return -1;
			}
			else
				break;
		}
		else if(osMessageQueueGetCount(rxQueue) == 0)
		{
			return -1;
		}
		else
		{
			return -1;
		}
	}
	return 0;
}

/*Reset*/
void moduleHwResetAndPowerUp(void)
{
	  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
	  osDelay(1000);
	  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
	  osDelay(1000);
	  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_SET);
	  osDelay(1000);
	  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_RESET);
	  /* Waits for Modem complete its booting procedure */
	  osDelay(5000);
	  HAL_UART_Receive_DMA(&huart2, (uint8_t*)&cIn, sizeof(cIn));
}

/*Executes an AT command and checks the response*/
int executeATCmd(char* cmd, char* resp, int timeout)
{
	int ret = 0;
	if(transmitATCmd(cmd) == HAL_OK)
	{
		ret = (moduleCheckResponse(resp) == 0) ? 0: -1;
	}
	return ret;
}

int moduleCheckGsmNetwork(void)
{
	for(int regTries = 0; regTries<10; regTries++)
	{
		if(executeATCmd("AT+CREG?\r\n", "+CREG: 0,1\r\n", 300) == 0)
		{
			return 0;
		}
		osDelay(3000);
	}
	return -1;
}

int moduleCheckGprsNetwork(void)
{
	for(int regTries = 0; regTries<10; regTries++)
	{
		if(executeATCmd("AT+CGREG?\r\n", "+CGREG: 0,1\r\n", 300) == 0)
		{
			return 0;
		}
		osDelay(3000);
	}
	return -1;
}

int moduleSetApn(char* apn)
{
	char cmdBuffer[64];
	snprintf(cmdBuffer, sizeof(cmdBuffer), "AT+CGDCONT=1,\"IP\",\"%s\"\r\n",apn);
	if(executeATCmd(cmdBuffer, "OK\r\n", 300) == 0)
	{
		return 0;
	}
	return -1;
}

int modulePPPDial(void)
{
 return (executeATCmd("ATD*99#\r\n", "CONNECT\r\n", 300) == 0) ? 0 : -1;
}

int moduleInit(void)
{
	uint32_t connect_count = 0;
	moduleHwResetAndPowerUp();
MODULE_CONNECT_RETRY:
	if(moduleCheckGsmNetwork() != 0)
	{
		MODULE_DEBUG("Unable to Register SIM");
	}
	moduleCheckGprsNetwork();
	if(moduleSetApn("www") != 0)
	{
		MODULE_DEBUG("Unable to set APN");
	}
	if(modulePPPDial() != 0)
	{
		connect_count++;
		if(connect_count < 5)
			goto MODULE_CONNECT_RETRY;
		else
			MODULE_DEBUG("*************************PPP DIAL FAILED*********************************");
	}
	return 0;
}
#endif /* MODULE_C_ */
