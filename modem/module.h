/*
 * mdoule.h
 *
 *  Created on: Aug 1, 2019
 *      Author: Heeze
 */

#ifndef MODULE_H_
#define MODULE_H_

#include "main.h"
#include "cmsis_os.h"
#include "string.h"
#include "stdio.h"

#define MODULE_DEBUG(x)    	\
	{						\
	 printMsg(x, strlen(x)); 			\
	 printMsg("\n", 1);		\
	}
/*
	printf(__VA_ARGS__); \
	printf("\n"); \
	}
*/
/*Executes an AT command and checks the response*/
void moduleHwResetAndPowerUp(void);
osMessageQueueId_t	moduleQueueInitialize(void);
int executeATCmd(char* cmd, char* resp, int timeout);
int moduleInit(void);

#endif /* MODULE_H_ */
