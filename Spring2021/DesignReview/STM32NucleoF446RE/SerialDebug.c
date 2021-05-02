#include "SerialDebug.h"

UART_HandleTypeDef huart1;

void DebugBegin()
{
	char buffer[50] = "Beginning Program!\r\n";
	DebugLog(buffer);
	return;
}


void DebugLog(char * message)
{

	HAL_UART_Transmit(&huart1, message, strlen((unsigned char *) message), HAL_MAX_DELAY);
	return;

}
