

#ifndef INC_FRAM_H_
#define INC_FRAM_H_

// TODO: Differences to investigate
// "don't care" bits of status register and WPEN register are different
// address range is obviously new, 17 bit addresses vs 15 bit addresses
// need to consider adding sleep mode
// Device ID is slightly different


//****************************************************************************************
// Includes
//****************************************************************************************
#include "main.h"


//****************************************************************************************
// fram constants that can be used by users of fram.h
//****************************************************************************************
// TODO: the FRAM content structure will change significantly
extern const uint16_t MAX_NUM_CONFIG;		// defined in .c file, = 100
extern const uint16_t MAX_NUM_LOGS;			// defined in .c file, = 1000


//****************************************************************************************
// Type defining the configuration data space (100 bytes are allocated for this)
// Enumeration must not exceed 100 entries
//****************************************************************************************
typedef enum
{
	CONFIG_PN_SCALE,		// 0d, 0x00
	CONFIG_PN_CONTROL,		// 1d, 0x01
	CONFIG_NW_DEADBAND,		// 2d, 0x02
	CONFIG_PROPBAND,		// 3d, 0x03
	LAST_MOV_TYPE,			// 4d, 0x04
	CONFIG5,				// 5d, 0x05
	CONFIG6,				// 6d, 0x06
	CONFIG7,				// 7d, 0x07
	CONFIG8,				// 8d, 0x08
	CONFIG9, 				// 9d, 0x09
	CONFIG10,				// 10d, 0x0A
							// Insert additional config enums as needed above CONFIG_MAX, do not exceed CONFIG_MAX


	CONFIG_MAX = 100		// 100d, 0x64
} framConfigData_t;


//****************************************************************************************
// Type defining the log type (error, warning, info, etc),
// 3 bits allocated, must remain less than LOG_MAX
//****************************************************************************************
typedef enum
{
	LOG_FATAL,			// 0d, 0x00
	LOG_ERROR,			// 1d, 0x01
	LOG_WARNING,		// 2d, 0x02
	LOG_INFORMATION,	// 3d, 0x03
	LOG_DEBUGING,		// 4d, 0x04
	LOG_SPARE1,			// 5d, 0x05
	LOG_SPARE2, 		// 6d, 0x06
	NOTUSED_FRAM_LEVEL,		// 7d, 0x07
	LOG_MAX = 8			// 8d, 0x08
} framLogLevel_t;


//****************************************************************************************
// Type defining the source thread that the log came from
// 3 bits allocated, must remain less than THREAD_MAX
//****************************************************************************************
typedef enum
{
	InitALL,				// 0d, 0x00
	MAIN_TH,				// 1d, 0x01
	Cyclic_th,				// 2d, 0x02
	Modbus_TH,				// 3d, 0x03
	UART_IRQHandler,		// 4d, 0x04 // USER_UART_IRQHandler()
	CyclicalProcessing,		// 5d, 0x05 // APPL_CyclicalProcessing
	General_NoTh1,			// 6d, 0x06
	General_NoTh2,			// 7d, 0x07
	THREAD_MAX = 8			// 8d, 0x08
} framLogThread_t;


// ***************************************************************************************
// Type defining the log codes
// 10 bits allocated, must remain less than CODE_MAX
//****************************************************************************************
typedef enum
{
	NONEFRAM, //Nothing should log this value. Used to init buffers
	MainStart,			// 0d, 0x00, main thread started
	Semaphore_Error,	// 1d, 0x01
	MODBUSMSGSMALL,		// 2d, 0x02, the modbus message rsp was truncated
	MODBUSMSGSZero,		// 3d, 0x03, the modbus message rsp was zero
	MODBUS_SEND_MSG_FORMAT,	// 4d, 0x04, the message that we created to send was too small or formateed incorrectly
	TickRollOver,				// 5d, 0x05
	ApplyConfigAndReset,// 6d, 0x06, the user applied changed configs and reset the unit
	APPL_SHUTDOWNLG,	// 7d, 0x07
	APPL_ResetLG,	//
	APPL_DRRESETLG,	//
	APPL_HALTLG,	//



						// Insert additional config enums as needed above CODE_MAX, must remain less than CODE_MAX


	CODE_MAX = 1023
} framLogCode_t;


//****************************************************************************************
// Type defining the return status of fram functions
//****************************************************************************************
typedef enum
{
	FRAM_GENERAL_FAIL,			// 0d, 0x00
    FRAM_SUCCESS,				// 1d, 0x01
	FRAM_UNINITIALIZED,			// 2d, 0x02
	FRAM_FAILED_TO_INIT,		// 3d, 0x03
    FRAM_SPI_ERROR,				// 4d, 0x04
	FRAM_SPI_BUSY,				// 5d, 0x05
	FRAM_SPI_TIMEOUT,			// 6d, 0x06
	FRAM_WRITE_NOT_ENABLED,		// 7d, 0x07
	FRAM_WRITE_PROTECTED,		// 8d, 0x08
	FRAM_INVALID_ARGUMENT,		// 9d, 0x09
	FRAM_WRITE_FAILED,			// 10d, 0x0A
	FRAM_ZERO_LOGS,				// 11d, 0x0B
	FRAM_RETURN_STATUS_MAX		// 12d, 0x0C
} framReturnStatus_t;


//****************************************************************************************
// Function Name: framInit
// Description:  This function is used to initialize the fram interface
// Input(s):
// - spi_handle (a pointer to the HAL SPI handle)
// Return: framReturnStatus_t
//
// Notes:
// This function must be called before any other fram functions can be used
// !!! You must wait at least 250 microseconds after power up before accessing the FRAM !!!
// Operate the HAL SPI peripheral in SPI Mode 0 (CPOL = LOW/0, CPHA = 1 Edge/0)
// Motorola frame format
// data size = 8 bits, MSB first
// NSSP Mode = disabled, NSS signal type = software
//
// Example Usage:
// SPI_HandleTypeDef hspi2;
// framReturnStatus_t status;
// status = framInit(&hspi2);
//****************************************************************************************
framReturnStatus_t framInit(SPI_HandleTypeDef *spi_handle);


//****************************************************************************************
// Function Name: framGetDeviceID
// Description: Use this to read the FRAM's Device ID, 9 bytes
// Input(s)
// - uint8_t * buffer (pointer to array of uint8_t element size, must be at least 9 bytes large)
// Return: framReturnStatus_t
//
// Notes:
// framInit function must have been called prior to using this function
// The Device ID is 9 bytes = bits 71 (MSB) through 0 (LSB), expect 0x7F7F7F7F7F7FC22208 in buffer[0] MSB through buffer[8] LSB respectively
// bits 71-16 (56 bits) = Manufacturer ID, expect 0x7F7F7F7F7F7FC2
// bits 15-13 (3 bits) = Family, expect 0b001
// bits 12-8 (5 bits) = Density, expect 0b00010
// bits 7-6 (2 bits) = Sub, expect 0b00
// bits 5-3 (3 bits) = Rev, expect 0b001
// bits 2-0 (3 bits) = Reserved, expect 0b000
//
// Example Usage:
// uint8_t buffer[9];
// framReturnStatus_t status;
// status = framGetDeviceID(buffer);
//****************************************************************************************
// TODO: update the fram Device ID info
framReturnStatus_t framGetDeviceID(uint8_t * buffer);

//****************************************************************************************
// Function Name: framFactoryReset
// Description: Use this to "wipe" the fram configuration and log data (will fill in all bytes with 0xFF)
// Input: none
// Return: framReturnStatus_t
//
// Notes:
// framInit function must have been called prior to using this function
// Not recommended for use in normal operation, could be used as part of the production test procedure at PCBA manufacturer
//
// Example Usage:
// framReturnStatus_t status;
// status = framFactoryReset();
//****************************************************************************************
framReturnStatus_t framFactoryReset();


//****************************************************************************************
// Function Name: framWriteConfig
// Description: Use this to write one byte of config data
// Input(s):
// - enum config (informs fram driver which config entry to write)
// - uint8_t data (actual config data byte)
// Return: framReturnStatus_t
//
// Notes:
// framInit function must have been called prior to using this function
//
// Example Usage:
// uint8_t data = 0xAA;
// framReturnStatus_t status;
// framConfigData_t config = CONFIG3;
// status = framWriteConfig(config, data);
//****************************************************************************************
// TODO: this needs updated
framReturnStatus_t framWriteConfig(framConfigData_t config, uint8_t data);


//****************************************************************************************
// Function Name: framGetSpecificConfig
// Description: Use this to read a specific configuration data entry in FRAM
// Input(s):
// - enum config (the configuration entry you want to read)
// - uint8_t * buffer (the pass-by-reference byte that will be written to with the data)
// Return: framReturnStatus_t
//
// Notes:
// framInit function must have been called prior to using this function
//
// Example Usage:
// uint8_t * buffer;
// framConfigData_t config = CONFIG3;
// framReturnStatus_t status;
// status = framGetSpecificConfig(config, buffer);
//****************************************************************************************
// TODO: this needs updated
framReturnStatus_t framGetSpecificConfig(framConfigData_t config, uint8_t * buffer);


//****************************************************************************************
// Function Name: framGetAllConfigs
// Description: Use this to read the entire configuration data space from FRAM
// Input(s):
// - uint8_t * buffer (the pass-by-reference byte that will be written to with the data, must be large enough to store all data bytes)
// Return: framReturnStatus_t
//
// Notes:
// framInit function must have been called prior to using this function
//
// Example Usage:
// uint8_t buffer[MAX_NUM_CONFIG];
// framReturnStatus_t status;
// status = framGetAllConfigs(buffer);
//****************************************************************************************
// TODO: this needs updated or deleted
framReturnStatus_t framGetAllConfigs(uint8_t * buffer);


//****************************************************************************************
// Function Name: framWriteLog
// Description: Use this to write a log entry to the FRAM
// Input(s):
// - framLogLevel_t level (the log level enumeration of the entry)
// - framLogThread_t thread (the thread enumeration of the entry i.e. which thread did the log come from)
// - framLogCode_t code (the log/error code enumeration of the entry, the user should define the code definitions)
// Return: framReturnStatus_t
//
// Notes:
// framInit function must have been called prior to using this function
//
// Example Usage:
// framLogLevel_t level = LOG_WARN;
// framLogThread_t thread = THREAD2;
// framLogCode_t code = CODE2;
// framReturnStatus_t status;
// status = framGetDeviceID(buffer);
//****************************************************************************************
// TODO: this needs updated or deleted
framReturnStatus_t framWriteLog(framLogLevel_t level, framLogThread_t thread, framLogCode_t code);


//****************************************************************************************
// Function Name: framGetLastLog
// Description: Use this to read the latest written log entry
// Input(s):
// - framLogLevel_t * level (pointer to a log level enumeration, which the function will fill in with data from FRAM)
// - framLogThread_t * thread (pointer to a thread enumeration, which the function will fill in with data from FRAM)
// - framLogCode_t * code (pointer to a log code enumeration, which the function will fill in with data from FRAM)
// Return: framReturnStatus_t
//
// Notes:
// framInit function must have been called prior to using this function
// "last" means most recent log entry
//
// Example Usage:
// framLogLevel_t level;
// framLogThread_t thread;
// framLogCode_t code;
// framReturnStatus_t status;
// status = framGetLastLog(&level, &thread, &code);
//
//framLogLevel_t level1 = NOTUSED_FRAM_LEVEL;
//framLogThread_t thread1 = THREAD_MAX;
//framLogCode_t code1 = NONEFRAM;
//framLogLevel_t * plevel1 = &level1;
//framLogThread_t * pthread1 = &thread1;
//framLogCode_t * pcode1 = &code1;
//framGetLastLog(plevel1, pthread1, pcode1);
//****************************************************************************************
// TODO: this needs updated or deleted
framReturnStatus_t framGetLastLog(framLogLevel_t * level, framLogThread_t * thread, framLogCode_t * code);


//****************************************************************************************
// Function Name: framGetSpecificLog
// Description: Use this to read a specific log entry
// Input(s):
// - uint16_t logNum (the "number" of the log entry to be read, must valid number from 1 to NumLogs)
// - framLogLevel_t * level (pointer to a log level enumeration, which the function will fill in with data from FRAM)
// - framLogThread_t * thread (pointer to a thread enumeration, which the function will fill in with data from FRAM)
// - framLogCode_t * code (pointer to a log code enumeration, which the function will fill in with data from FRAM)
// Return: framReturnStatus_t
//
// Notes:
// framInit function must have been called prior to using this function
//
// Example Usage:
// uint16_t logNum = 3;
// framLogLevel_t level;
// framLogThread_t thread;
// framLogCode_t code;
// framReturnStatus_t status;
// status = framGetSpecificLog(logNum, &level, &thread, &code);
//****************************************************************************************
// TODO: this needs updated or deleted
framReturnStatus_t framGetSpecificLog(uint16_t logNum, framLogLevel_t * level, framLogThread_t * thread, framLogCode_t * code);


//****************************************************************************************
// Function Name: framGetNumLogs
// Description: Use to read the current number of logs
// Input(s): none
// Return: returns a uint16_t value equal to current number of log entries in FRAM, will be between 0 and and MAX_NUM_LOGS
//
// Notes:
// framInit function must have been called prior to using this function
//
// Example Usage:
// uint16_t numLogs;
// numLogs = framGetNumLogs();
//****************************************************************************************
// TODO: this needs updated or deleted
uint16_t framGetNumLogs();


//****************************************************************************************
// Function Name: framGetLogCurrAddr
// Description: Use to read the FRAM's current log address
// Input(s): pointer to uint16_t data
// Return: framReturnStatus_t
//
// Notes:
// framInit function must have been called prior to using this function
// Intended only to be used in FRAM testing, not intended for use in normal operation
//
// Example Usage:
// uint16_t address;
// framReturnStatus_t status;
// status = framGetLogCurrAddr(&address);
//****************************************************************************************
// TODO: this needs updated or deleted
framReturnStatus_t framGetLogCurrAddr(uint16_t * address);


//****************************************************************************************
// Function Name: framGetAllLogsFromEnd
// Description: Use this to read all logs from latest to earliest
// Input(s):
// - framLogLevel_t * levelBuffer (pointer to an array of level enumerations, which the function will fill in with data from FRAM, array must be large enough to hold max possible logs)
// - framLogLevel_t * threadBuffer (pointer to an array of thread enumerations, which the function will fill in with data from FRAM, array must be large enough to hold max possible logs)
// - framLogLevel_t * codeBuffer (pointer to an array of level enumerations, which the function will fill in with data from FRAM, array must be large enough to hold max possible logs)
// Return: framReturnStatus_t
//
// Notes:
// framInit function must have been called prior to using this function
// !!! Inefficient, so not advisable for use in normal operation !!!
// This function will put the latest log in the 0th element of each array and fill in chronologically working back in time
//
// Example Usage:
// framLogLevel_t levelBuffer [MAX_NUM_LOGS];
// framLogThread_t thread [MAX_NUM_LOGS];
// framLogCode_t code [MAX_NUM_LOGS];
// framReturnStatus_t status;
// status = framGetAllLogsFromEnd(&levelBuffer, &threadBuffer, &codeBuffer);
//****************************************************************************************
// TODO: this needs updated or deleted
framReturnStatus_t framGetAllLogsFromEnd(framLogLevel_t *levelBuffer, framLogThread_t * threadBuffer, framLogCode_t * codeBuffer);


//****************************************************************************************
// Function Name: framGetAllLogsFromBegin
// Description: Use this to read all logs from earliest to latest
// Input(s):
// - framLogLevel_t * levelBuffer (pointer to an array of level enumerations, which the function will fill in with data from FRAM, array must be large enough to hold max possible logs)
// - framLogLevel_t * threadBuffer (pointer to an array of thread enumerations, which the function will fill in with data from FRAM, array must be large enough to hold max possible logs)
// - framLogLevel_t * codeBuffer (pointer to an array of level enumerations, which the function will fill in with data from FRAM, array must be large enough to hold max possible logs)
// Return: framReturnStatus_t
//
// Notes:
// framInit function must have been called prior to using this function
// !!! Inefficient, so not advisable for use in normal operation !!!
// This function will put the earliest log in the 0th element of each array and fill in chronologically working forward in time
//
// Example Usage:
// framLogLevel_t levelBuffer [MAX_NUM_LOGS];
// framLogThread_t thread [MAX_NUM_LOGS];
// framLogCode_t code [MAX_NUM_LOGS];
// framReturnStatus_t status;
// status = framGetAllLogsFromBegin(&levelBuffer, &threadBuffer, &codeBuffer);
//Exmaple 2
//framLogLevel_t levelBuffer [MAX_NUM_LOGS];
//framLogThread_t threadBuffer [MAX_NUM_LOGS];
//framLogCode_t codeBuffer [MAX_NUM_LOGS];
//framReturnStatus_t status;
//status = framGetAllLogsFromBegin(levelBuffer, threadBuffer, codeBuffer);
//****************************************************************************************
// TODO: this needs updated or deleted
framReturnStatus_t framGetAllLogsFromBegin(framLogLevel_t * levelBuffer, framLogThread_t * threadBuffer, framLogCode_t * codeBuffer);

#endif /* INC_FRAM_H_ */

