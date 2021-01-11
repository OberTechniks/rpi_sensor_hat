

//****************************************************************************************
// Includes
//****************************************************************************************
#include "fram.h"

//****************************************************************************************
// Module constants that can be accessed by users of fram.h
//****************************************************************************************
// TODO: the data structure will change
const uint16_t MAX_NUM_CONFIG = 100;
const uint16_t MAX_NUM_LOGS = 0x3F9B; 			// decimal 16,283

//****************************************************************************************
// Module constants not accessed by users of fram.h
//****************************************************************************************
const uint8_t SIZE_WRITE = 4;					// 4 bytes for a data write: opcode, hi byte, lo byte, data byte
const uint8_t SIZE_WRITE_LOG = 5;				// 5 bytes for a log write: opcode, hi address byte, lo address byte, hi data byte, lo data byte (auto-increment mode)
const uint8_t SIZE_READ = 3;					// 3 bytes for a data read: opcode, hi byte, lo byte
const uint8_t SIZE_READ_LOG = 2;				// 2 bytes for a log read buffer: hi data byte, lo data byte
const uint8_t SIZE_OPCODE = 0x01;				// fram opcodes are one byte
const uint8_t SIZE_ADDR = 0x01;					// size of address lower or upper byte
const uint8_t SIZE_DATA_BYTE = 0x01;			// size of a data byte
const uint16_t SIZE_DEVICE_ID = 0x09;			// fram Device ID is 9 bytes
const uint32_t TIMEOUT = 0x2710;				// SPI timeout = 10,000 milliseconds
const uint16_t MAX_ADDRESS = 0x7FFF;			// FRAM's maximumum data address (2 bytes)
const uint16_t MIN_ADDRESS = 0x0000;			// FRAM's minimum data address (2 bytes)
const uint16_t CONFIG_START_ADDRESS = 0x00;		// Config memory space starting address = decimal 0
const uint16_t CONFIG_END_ADDRESS = 0x0063;		// Config memory space ending address = decimal 99
const uint16_t RESERVED_START_ADDRESS = 0x0064;	// Reserved memory space starting address = decimal 100
const uint16_t RESERVED_END_ADDRESS = 0x00C7;	// Reserved memory space ending address = decimal 199
const uint16_t LOG_START_ADDRESS = 0x00C8;		// Log memory space starting address = decimal 200
const uint16_t LOG_END_ADDRESS = 0x7FFE;		// Log memory space ending address = decimal 32,766 (one short of 32,767 because we need 2 bytes per log entry)
const uint16_t NUM_LOGS_ADDRESS = 0x0064;		// Location of numLogs hi byte, followed by numLogs lo byte
const uint16_t CURR_ADDR_ADDRESS = 0x0066;		// Location of logCurrAddr hi byte, followed by logCurrAddr lo byte
const uint8_t FRAM_DEFAULT_DATA = 0x00;			// Factory default data (value of every FRAM byte after framFactoryReset() is performed)
const uint16_t FRAM_DEFAULT_LOGS = 0x0000;		// Factory default number of logs (zero)
const uint8_t WREN = 0x06;						// Set write enable latch opcode
const uint8_t WRDI = 0x04;						// Reset write enable latch opcode
const uint8_t RDSR = 0x05;						// Read status register opcode
const uint8_t WRSR = 0x01;						// Write status register opcode
const uint8_t READ = 0x03; 						// Read memory data opcode
const uint8_t FSTRD = 0x0B;						// Fast read memory data opcode
const uint8_t WRITE = 0x02;						// Write memory data opcode
const uint8_t SLEEP = 0xB9;						// Enter sleep mode opcode
const uint8_t RDID = 0x9F;						// Read device ID opcode
const uint8_t THREAD_MASK = 0x3F;				// Mask for reading the 3 thread bits in the log bytes (binary = 0b00111111), use to mask off the upper 2 bits
const uint8_t LEVEL_MASK = 0x07;				// Mask for reading the 3 log level bits in the log bytes (binary = 0b00000111), use to mask off the upper 5 bits


//****************************************************************************************
// Module private helper function prototypes
//****************************************************************************************
framReturnStatus_t checkSpiStatus(HAL_StatusTypeDef spi_status);
void assertCS();
void deassertCS();
framReturnStatus_t updateNumLogs(uint16_t data);
framReturnStatus_t updateLogCurrAddr(uint16_t data);
framReturnStatus_t getNumLogs(uint16_t * dataBuffer);
framReturnStatus_t getLogCurrAddr(uint16_t * dataBuffer);
framReturnStatus_t verifyData(uint8_t data, uint16_t address);
framReturnStatus_t resetWriteEnable();
framReturnStatus_t setWriteEnable();


//****************************************************************************************
// FRAM type structure definition
//****************************************************************************************
struct FRAM_t
{
	SPI_HandleTypeDef	spiHandle;		// The STM32 HAL SPI handle
	uint16_t	NumLogs;				// The number of log entries in FRAM
	uint16_t	LogCurrAddr;			// The address of the next log to be written (the most previously written log will be in the slot preceding logCurrAddr)
} FRAM;


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
framReturnStatus_t framInit(SPI_HandleTypeDef *spi_handle)
{
	// Function variables
	framReturnStatus_t status = FRAM_GENERAL_FAIL;
	uint16_t NumLogs = 0x0000;
	uint16_t LogCurrAddr = 0x0000;


	// Ensure that the FRAM's /HOLD and /WP signals are deasserted (HIGH)
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, GPIO_PIN_SET);						// /HOLD signal
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_SET);						// /WP signal


	// Initialize the HAL SPI handle
	status = FRAM_FAILED_TO_INIT;
	if(spi_handle == NULL)
	{
		return status;	// invalid spi handle
	}
	else
	{
		status = FRAM_SUCCESS;
		FRAM.spiHandle = *spi_handle;
	}


	// Get NumLogs, check for validity, update if needed
	status = getNumLogs(&NumLogs);
	if(status != FRAM_SUCCESS)													// There is a problem, abort
	{
		return status;
	}
	if(NumLogs > MAX_NUM_LOGS)													// Invalid, need to initialize
	{
		status = updateNumLogs(FRAM_DEFAULT_LOGS);
		NumLogs = FRAM_DEFAULT_LOGS;
		if(status != FRAM_SUCCESS)												// There is a problem, abort
		{
			return status;
		}
	}


	// Get LogCurrAddr, check the index for validity, update if needed
	status = getLogCurrAddr(&LogCurrAddr);
	if(status != FRAM_SUCCESS)													// There is a problem, abort
	{
		return status;
	}
	if((LogCurrAddr < LOG_START_ADDRESS) || (LogCurrAddr > LOG_END_ADDRESS))	// Invalid, need to initialize
	{
		status = updateLogCurrAddr(LOG_START_ADDRESS);
		LogCurrAddr = LOG_START_ADDRESS;
		if(status != FRAM_SUCCESS)												// There is a problem, abort
		{
			return status;
		}
	}


	// Everything is good, so finally, store the index in the struct's data member (MCU RAM)
	FRAM.NumLogs = NumLogs;
	FRAM.LogCurrAddr = LogCurrAddr;


	// We're done
	return status;
}


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
// framReturtStatus_t status;
// status = framGetDeviceID(buffer);
//****************************************************************************************
framReturnStatus_t framGetDeviceID(uint8_t * buffer)
{
	// Function scope variables
	framReturnStatus_t status = FRAM_GENERAL_FAIL;
	HAL_StatusTypeDef spi_status = HAL_ERROR;
	uint8_t opcode = 0x0;


	// Check for proper initialization
	if(FRAM.spiHandle.Instance == 0x0)		// SPI handle is not initialized yet
	{
		status = FRAM_UNINITIALIZED;
		return status;
	}


	// Try to send the opcode and read device ID, update status
	opcode = RDID;																			// Set opcode to RDID (read device ID)
	assertCS();
	spi_status = HAL_SPI_Transmit(&FRAM.spiHandle, &opcode, SIZE_OPCODE, TIMEOUT);		    // Attempt SPI transaction
	status = checkSpiStatus(spi_status);													// Check the HAL SPI status and update
	if(status != FRAM_SUCCESS)																// There is a problem, clean up and abort
	{
		deassertCS();
		return status;
	}
	spi_status = HAL_SPI_Receive(&FRAM.spiHandle, buffer, SIZE_DEVICE_ID, TIMEOUT);			// The opcode was success, so try to receive 9 bytes of data
	status = checkSpiStatus(spi_status);													// Check the HAL SPI status and update
	if(status != FRAM_SUCCESS)																// There is a problem, clean up and abort
	{
		deassertCS();
		return status;
	}


	// SPI transaction was successful, so clean up and return
	deassertCS();
	return status;
}


//****************************************************************************************
// Function Name: framFactoryReset
// Description: Use this to "wipe" the fram configuration and log data
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
framReturnStatus_t framFactoryReset()
{
	// Function variables
	framReturnStatus_t status = FRAM_GENERAL_FAIL;
	HAL_StatusTypeDef spi_status = HAL_ERROR;
	uint8_t opcode = 0x0;
	uint8_t writeData = FRAM_DEFAULT_DATA;
	uint8_t addrHiByte = 0x00;
	uint8_t addrLoByte = 0x00;
	uint8_t transmitBuffer[SIZE_WRITE];


	// Check for proper initialization, update status
	if(FRAM.spiHandle.Instance == 0x0)		// SPI handle is not initialized yet
	{
		status = FRAM_UNINITIALIZED;
		return status;
	}


	// Cycle through entire FRAM data space, re-initializing all data bytes, check and update status each time
	for(uint16_t i = MIN_ADDRESS; i <= MAX_ADDRESS; i++)
	{
		// Set the FRAM's write enable bit
		status = setWriteEnable();
		if(status != FRAM_SUCCESS)					// There is a problem, abort
		{
			return status;
		}


		// The WREN opcode was successful, so try to send the address high byte, address low byte, and data byte
		addrLoByte = i;																			// Update the address low byte
		addrHiByte = (i >> 8);																	// Update the address high byte
		opcode = WRITE;																			// Set the opcode to WRITE
		transmitBuffer[0] = opcode;																// Update the transmit buffer
		transmitBuffer[1] = addrHiByte;
		transmitBuffer[2] = addrLoByte;
		transmitBuffer[3] = writeData;
		assertCS();																				// Set the CS low
		spi_status = HAL_SPI_Transmit(&FRAM.spiHandle, transmitBuffer, SIZE_WRITE, TIMEOUT);	// Send the opcode
		status = checkSpiStatus(spi_status);													// Check the HAL SPI status and update
		if(status != FRAM_SUCCESS)																// There is a problem, clean up and abort
		{
			deassertCS();
			return status;
		}
		deassertCS();																			// Everything went ok, so set CS high to end transaction
	}


	// We've finished writing, so send the WRDI (reset write enable latch) opcode to reset the WEL bit
	status = resetWriteEnable();
	if(status != FRAM_SUCCESS)																		// There is a problem, abort
	{
		return status;
	}


	// Now we need to re-initialize LogCurrAddr (NumLogs will be zero, so don't need to re-initialize NumLogs)
	status = updateLogCurrAddr(LOG_START_ADDRESS);
	if(status != FRAM_SUCCESS)												// There is a problem, abort
	{
		return status;
	}


	// Lastly, update NumLogs and LogCurrAddr in the MCU's RAM
	FRAM.NumLogs = FRAM_DEFAULT_LOGS;
	FRAM.LogCurrAddr = LOG_START_ADDRESS;


	// We are finished, return status
	return status;
}


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
framReturnStatus_t framWriteConfig(framConfigData_t config, uint8_t data)
{
	// Function variables
	framReturnStatus_t status = FRAM_GENERAL_FAIL;
	HAL_StatusTypeDef spi_status = HAL_ERROR;
	uint8_t opcode = 0x0;
	uint8_t addrHiByte = 0x00;
	uint8_t addrLoByte = 0x00;
	uint16_t temp = 0x00;
	uint8_t writeData = data;
	uint8_t transmitBuffer[SIZE_WRITE];


	// Check for proper initialization, update status
	if(FRAM.spiHandle.Instance == 0x0)														// SPI handle is not initialized yet, abort
	{
		status = FRAM_UNINITIALIZED;
		return status;
	}


	// Check arguments
	if(config >= CONFIG_MAX)																// Config argument is out of bounds, abort
	{
		status = FRAM_INVALID_ARGUMENT;
		return status;
	}


	// Set the FRAM's write enable bit
	status = setWriteEnable();
	if(status != FRAM_SUCCESS)					// There is a problem, abort
	{
		return status;
	}																		// Set CS high to end transaction


	// The WREN opcode was successful, so try to send the WRITE opcode, address high byte, address low byte, and data byte
	temp = (uint16_t)config;																// Convert the enum into memory address bytes
	addrLoByte = temp;
	addrHiByte = temp >> 8;
	opcode = WRITE;																			// Set the opcode to WRITE (write data)
	transmitBuffer[0] = opcode;																// Update the transmit buffer
	transmitBuffer[1] = addrHiByte;
	transmitBuffer[2] = addrLoByte;
	transmitBuffer[3] = writeData;
	assertCS();																				// Set the CS low
	spi_status = HAL_SPI_Transmit(&FRAM.spiHandle, transmitBuffer, SIZE_WRITE, TIMEOUT);	// Send the opcode
	status = checkSpiStatus(spi_status);													// Check the HAL SPI status and update
	if(status != FRAM_SUCCESS)																// There is a problem, clean up and abort
	{
		deassertCS();
		return status;
	}
	deassertCS();																			// Everything went ok, so set CS high to end transaction


	// We've finished writing, so send the WRDI (reset write enable latch) opcode to reset the WEL bit
	status = resetWriteEnable();
	if(status != FRAM_SUCCESS)																		// There is a problem, abort
	{
		return status;
	}


	// Need to read and verify the data
	status = verifyData(data, (uint16_t)config);
	if(status != FRAM_SUCCESS)																// There is a problem, abort
	{
		return status;
	}


	// We are done
	return status;
}


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
framReturnStatus_t framGetSpecificConfig(framConfigData_t config, uint8_t * buffer)
{
	// Function variables
	framReturnStatus_t status = FRAM_GENERAL_FAIL;
	HAL_StatusTypeDef spi_status = HAL_ERROR;
	uint8_t opcode = 0x0;
	uint8_t addrHiByte = CONFIG_START_ADDRESS;
	uint8_t addrLoByte = CONFIG_START_ADDRESS;
	uint8_t readData = 0x00;
	uint8_t transmitBuffer[SIZE_READ];


	// Check for proper initialization, update status
	if(FRAM.spiHandle.Instance == 0x0)														// SPI handle is not initialized yet, abort
	{
		status = FRAM_UNINITIALIZED;
		return status;
	}


	// Check arguments
	if(config >= CONFIG_MAX)																// Config argument is out of bounds, abort
	{
		status = FRAM_INVALID_ARGUMENT;
		return status;
	}


	// Attempt to read the data from memory
	opcode = READ;																			// Set opcode to READ (read memory)
	addrLoByte = (uint8_t)config;															// Update SPI address
	addrHiByte = ((uint8_t)config>>8);														// Update SPI address
	transmitBuffer[0] = opcode;																// Update the transmit buffer
	transmitBuffer[1] = addrHiByte;
	transmitBuffer[2] = addrLoByte;
	assertCS();																				// Set CS low to begin transaction
	spi_status = HAL_SPI_Transmit(&FRAM.spiHandle, transmitBuffer, SIZE_READ, TIMEOUT);		// Attempt to send the opcode
	status = checkSpiStatus(spi_status);													// Check the HAL SPI status and update
	if(status != FRAM_SUCCESS)																// There is a problem, clean up and abort
	{
		deassertCS();
		return status;
	}
	spi_status = HAL_SPI_Receive(&FRAM.spiHandle, &readData, SIZE_DATA_BYTE, TIMEOUT);		// Receive in one byte of data
	status = checkSpiStatus(spi_status);													// Check the HAL SPI status and update
	if(status != FRAM_SUCCESS)																// There is a problem, clean up and abort
	{
		deassertCS();
		return status;
	}
	deassertCS();																			// Transaction is over, so set CS high


	// Update the data byte and return status
	*buffer = readData;
	return status;
}


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
framReturnStatus_t framGetAllConfigs(uint8_t * buffer)
{
	// Function variables
	framReturnStatus_t status = FRAM_GENERAL_FAIL;
	HAL_StatusTypeDef spi_status = HAL_ERROR;
	uint8_t opcode = 0x0;
	uint8_t addrHiByte = 0x00;
	uint8_t addrLoByte = 0x00;
	uint8_t transmitBuffer[SIZE_READ];


	// Check for proper initialization, update status
	if(FRAM.spiHandle.Instance == 0x0)														// SPI handle is not initialized yet, abort
	{
		status = FRAM_UNINITIALIZED;
		return status;
	}


	// Attempt to read the data from memory
	opcode = READ;																			// Set opcode to READ (read memory)
	transmitBuffer[0] = opcode;																// Update transmit buffer
	transmitBuffer[1] = addrHiByte;
	transmitBuffer[2] = addrLoByte;
	assertCS();																				// Set CS low to begin transaction
	spi_status = HAL_SPI_Transmit(&FRAM.spiHandle, transmitBuffer, SIZE_READ, TIMEOUT);		// Attempt to send the opcode
	status = checkSpiStatus(spi_status);													// Check the HAL SPI status and update
	if(status != FRAM_SUCCESS)																// There is a problem, clean up and abort
	{
		deassertCS();
		return status;
	}
	spi_status = HAL_SPI_Receive(&FRAM.spiHandle, buffer, MAX_NUM_CONFIG, TIMEOUT);			// Receive in all the config bytes (FRAM auto-increments internal address)
	status = checkSpiStatus(spi_status);													// Check the HAL SPI status and update
	if(status != FRAM_SUCCESS)																// There is a problem, clean up and abort
	{
		deassertCS();
		return status;
	}
	deassertCS();																			// Transaction is over, so set CS high


	// We've read out the entire config data space, so return status
	return status;
}


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
// Per the FM25V02A's data sheet, a rising CS terminates a write command
//
//
// Example Usage:
// framLogLevel_t level = LOG_WARN;
// framLogThread_t thread = THREAD2;
// framLogCode_t code = CODE2;
// framReturnStatus_t status;
// status = framGetDeviceID(buffer);
//
// Bit definitions of a log
// 3 bits = log
// 3 bits = thread
// 10 bits = code
// MSB[code=10bits][thread=3bits][log=3bits]LSB
// Hi Byte  Lo Byte
// 00000000 00000000
// |||||||| ||||||||_ log bit 1 (lsb)
// |||||||| |||||||__ log bit 2
// |||||||| ||||||___ log bit 3 (msb)
// |||||||| |||||____ thread bit 1 (lsb)
// |||||||| ||||_____ thread bit 2
// |||||||| |||______ thread bit 3 (msb)
// |||||||| ||_______ code bit 1 (lsb)
// |||||||| |________ code bit 2
// ||||||||__________ code bit 3
// |||||||___________ code bit 4
// ||||||____________ code bit 5
// |||||_____________ code bit 6
// ||||______________ code bit 7
// |||_______________ code bit 8
// ||________________ code bit 9
// |_________________ code bit 10 (msb)
//****************************************************************************************
framReturnStatus_t framWriteLog(framLogLevel_t level, framLogThread_t thread, framLogCode_t code)
{
	// Function variables
	framReturnStatus_t status = FRAM_GENERAL_FAIL;
	HAL_StatusTypeDef spi_status = HAL_ERROR;
	uint8_t opcode = 0x0;
	uint8_t SpiAddrLoByte = 0x0;
	uint8_t SpiAddrHiByte = 0x0;
	uint8_t writeDataLoByte = 0x0;
	uint8_t writeDataHiByte = 0x0;
	uint8_t transmitBuffer[SIZE_WRITE_LOG];


	// Check for proper initialization, update status
	if(FRAM.spiHandle.Instance == 0x0)														// SPI handle is not initialized yet, abort
	{
		status = FRAM_UNINITIALIZED;
		return status;
	}


	// Check arguments
	if((level >= LOG_MAX) || (thread >= THREAD_MAX) || (code >= CODE_MAX))					// Argument is out of bounds, abort
	{
		status = FRAM_INVALID_ARGUMENT;
		return status;
	}


	// Encode the log information into 2 bytes to be written to FRAM
	writeDataLoByte = (uint8_t)level;													// Fills in the 3 level bits into the lo byte
	writeDataLoByte |= ((uint8_t)thread<<3);											// Fills in the 3 thread bits into the lo byte
	writeDataLoByte |= ((uint16_t)code<<6);												// Fills in the lower 2 code bits for the lo byte, lo byte is now encoded
	writeDataHiByte = ((uint16_t)code>>2);												// Fills in the upper 8 code bits for the hi byte, hi byte is now encoded


	// Set the FRAM's write enable bit
	status = setWriteEnable();
	if(status != FRAM_SUCCESS)															// There is a problem, abort
	{
		return status;
	}


	// The WREN opcode was successful, so attempt to write the log data to FRAM
	opcode = WRITE;																			// Set opcode to WRITE
	SpiAddrLoByte = FRAM.LogCurrAddr;														// Update address bytes
	SpiAddrHiByte = (FRAM.LogCurrAddr>>8);
	transmitBuffer[0] = opcode;																// Update the SPI transmit buffer
	transmitBuffer[1] = SpiAddrHiByte;
	transmitBuffer[2] = SpiAddrLoByte;
	transmitBuffer[3] = writeDataHiByte;
	transmitBuffer[4] = writeDataLoByte;
	assertCS();																				// Set the CS low
	spi_status = HAL_SPI_Transmit(&FRAM.spiHandle, transmitBuffer, SIZE_WRITE_LOG, TIMEOUT);// Send the opcode
	status = checkSpiStatus(spi_status);													// Check the HAL SPI status and update
	if(status != FRAM_SUCCESS)																// There is a problem, clean up and abort
	{
		deassertCS();
		return status;
	}
	deassertCS();																			// Everything went ok, so set CS high to end transaction


	// We've finished writing, so send the WRDI (reset write enable latch) opcode to reset the WEL bit
	status = resetWriteEnable();
	if(status != FRAM_SUCCESS)																		// There is a problem, abort
	{
		return status;
	}


	// Verify the data by reading it back and comparing both bytes of the log entry
	status = verifyData(writeDataHiByte, FRAM.LogCurrAddr);									// Verify the log data hi byte
	if(status != FRAM_SUCCESS)																// There is a problem, abort
	{
		return status;
	}
	status = verifyData(writeDataLoByte, (FRAM.LogCurrAddr + 1));							// Verify the log data lo byte
	if(status != FRAM_SUCCESS)																// There is a problem, abort
	{
		return status;
	}


	// Update numLogs and logCurrAddr in FRAM (if reset happens here, no problem, we'll just lose the current log)
	if(FRAM.NumLogs == MAX_NUM_LOGS)														// We've reached the log limit and will start writing over the oldest data
	{
		status = updateNumLogs(MAX_NUM_LOGS);
	}
	else																					// Increase NumLogs by 1
	{
		status = updateNumLogs(FRAM.NumLogs + 1);
	}
	if(status != FRAM_SUCCESS)																// There is a problem, abort
	{
		return status;
	}
	if(FRAM.LogCurrAddr == LOG_END_ADDRESS)													// We've reached the end of the log data space and need to roll over
	{
		status = updateLogCurrAddr(LOG_START_ADDRESS);
	}
	else
	{
		status = updateLogCurrAddr(FRAM.LogCurrAddr + 2);									// Increase LogCurrAddr by 2 (each log takes 2 bytes)
	}
	if(status != FRAM_SUCCESS)																// There is a problem, abort
	{
		return status;
	}


	// We're done, return status
	return status;
}


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
// Bit definitions of a log
// 3 bits = log
// 3 bits = thread
// 10 bits = code
// MSB[code=10bits][thread=3bits][log=3bits]LSB
// Hi Byte  Lo Byte
// 00000000 00000000
// |||||||| ||||||||_ log bit 1 (lsb)
// |||||||| |||||||__ log bit 2
// |||||||| ||||||___ log bit 3 (msb)
// |||||||| |||||____ thread bit 1 (lsb)
// |||||||| ||||_____ thread bit 2
// |||||||| |||______ thread bit 3 (msb)
// |||||||| ||_______ code bit 1 (lsb)
// |||||||| |________ code bit 2
// ||||||||__________ code bit 3
// |||||||___________ code bit 4
// ||||||____________ code bit 5
// |||||_____________ code bit 6
// ||||______________ code bit 7
// |||_______________ code bit 8
// ||________________ code bit 9
// |_________________ code bit 10 (msb)
//****************************************************************************************
framReturnStatus_t framGetLastLog(framLogLevel_t * level, framLogThread_t * thread, framLogCode_t * code)
{
	// Function variables
	framReturnStatus_t status = FRAM_GENERAL_FAIL;
	HAL_StatusTypeDef spi_status = HAL_ERROR;
	uint8_t opcode = 0x0;
	uint8_t SpiAddrLoByte = 0x0;
	uint8_t SpiAddrHiByte = 0x0;
	uint8_t readDataBuffer[SIZE_READ_LOG];
	uint8_t transmitBuffer[SIZE_READ];
	uint8_t levelTemp = 0x0;
	uint8_t threadTemp = 0x0;
	uint16_t codeTemp = 0x0;


	// Check for proper initialization, update status
	if(FRAM.spiHandle.Instance == 0x0)														// SPI handle is not initialized yet, abort
	{
		status = FRAM_UNINITIALIZED;
		return status;
	}


	// Check for zero logs
	if(FRAM.NumLogs == 0)																	// The log data space is empty
	{
		status = FRAM_ZERO_LOGS;
		return status;
	}


	// Update variables with valid data
	if(FRAM.LogCurrAddr == LOG_START_ADDRESS)												// We're at the beginning of the log space, need to read the log in the last index
	{
		SpiAddrLoByte = LOG_END_ADDRESS;
		SpiAddrHiByte = (LOG_END_ADDRESS>>8);
	}
	else
	{
		SpiAddrLoByte = (FRAM.LogCurrAddr - 2);												// Get the address of the most recent log index (each log uses 2 bytes)
		SpiAddrHiByte = ((FRAM.LogCurrAddr-2)>>8);
	}
	opcode = READ;																			// Update opcode to FRAM read
	transmitBuffer[0] = opcode;																// Update the SPI transmit buffer
	transmitBuffer[1] = SpiAddrHiByte;
	transmitBuffer[2] = SpiAddrLoByte;


	// Attempt to read the data from memory
	assertCS();																				// Set CS low to begin transaction
	spi_status = HAL_SPI_Transmit(&FRAM.spiHandle, transmitBuffer, SIZE_READ, TIMEOUT);		// Attempt to send the opcode
	status = checkSpiStatus(spi_status);													// Check the HAL SPI status and update
	if(status != FRAM_SUCCESS)																// There is a problem, clean up and abort
	{
		deassertCS();
		return status;
	}
	spi_status = HAL_SPI_Receive(&FRAM.spiHandle, readDataBuffer, SIZE_READ_LOG, TIMEOUT);	// Receive in 2 bytes of data
	status = checkSpiStatus(spi_status);													// Check the HAL SPI status and update
	if(status != FRAM_SUCCESS)																// There is a problem, clean up and abort
	{
		deassertCS();
		return status;
	}
	deassertCS();																			// Transaction is over, so set CS high


	// Decode the log data and return
	// MSB[code=10bits][thread=3bits][log=3bits]LSB
	// Hi Byte  Lo Byte
	// 00000000 00000000
	codeTemp = ((uint16_t)readDataBuffer[0]<<2);											// Fills in the upper 8 code bits from the hi byte
	codeTemp |= (readDataBuffer[1]>>6);														// Fills in the lower 2 code bits from the lo byte
	threadTemp = ((readDataBuffer[1] & THREAD_MASK)>>3);									// Fills in the 3 thread bits from the lo byte
	levelTemp = (readDataBuffer[1] & LEVEL_MASK);											// Fills in the 3 log level bits from the lo byte
	*level = levelTemp;
	*thread = threadTemp;
	*code = codeTemp;
	return status;
}


//****************************************************************************************
// Function Name: framGetSpecificLog
// Description: Use this to read a specific log entry
// Input(s):
// - uint16_t logNum (the "number" of the log entry to be read, must valid number from 1 to NumLogs, 1 being the oldest log, NumLogs being the most recent log)
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
framReturnStatus_t framGetSpecificLog(uint16_t logNum, framLogLevel_t * level, framLogThread_t * thread, framLogCode_t * code)
{
	// Function variables
	framReturnStatus_t status = FRAM_GENERAL_FAIL;
	HAL_StatusTypeDef spi_status = HAL_ERROR;
	uint8_t opcode = 0x0;
	uint16_t index = 0x0;
	uint16_t counter = 0x0;
	uint8_t SpiAddrLoByte = 0x0;
	uint8_t SpiAddrHiByte = 0x0;
	uint8_t readDataBuffer[SIZE_READ_LOG];
	uint8_t transmitBuffer[SIZE_READ];
	uint8_t levelTemp = 0x0;
	uint8_t threadTemp = 0x0;
	uint16_t codeTemp = 0x0;

	// Check for proper initialization, update status
	if(FRAM.spiHandle.Instance == 0x0)														// SPI handle is not initialized yet, abort
	{
		status = FRAM_UNINITIALIZED;
		return status;
	}


	// Check for zero logs
	if(FRAM.NumLogs == 0)																	// The log data space is empty
	{
		status = FRAM_ZERO_LOGS;
		return status;
	}


	// Check parameters
	if((logNum < 1) || (logNum > FRAM.NumLogs))
	{
		status = FRAM_INVALID_ARGUMENT;
		return status;
	}


	// Find the desired log index (this function is currently O(n)...could optimize it to be O(1) using a better algorithm... TO DO later)
	counter = FRAM.NumLogs;																	// Initialize counter to current number of logs
	index = FRAM.LogCurrAddr - 2;															// Initialize index to the most recent log index
	while(counter != logNum)
	{
		if(index == LOG_START_ADDRESS)														// We reached the beginning of the log space, need to roll to end of log space
		{
			index = LOG_END_ADDRESS;
		}
		index = index - 2;																	// Go back 2 addresses (each log entry uses 2 bytes)
		counter = counter - 1;																// Decrease counter by 1
	}																						// When while loop is passed, we have found our desired index


	// Index is now the desired FRAM address
	// Update variables with valid data
	SpiAddrLoByte = index;
	SpiAddrHiByte = (index>>8);
	opcode = READ;																			// Update opcode to FRAM read
	transmitBuffer[0] = opcode;																// Update the SPI transmit buffer
	transmitBuffer[1] = SpiAddrHiByte;
	transmitBuffer[2] = SpiAddrLoByte;


	// Attempt to read the data from memory
	assertCS();																				// Set CS low to begin transaction
	spi_status = HAL_SPI_Transmit(&FRAM.spiHandle, transmitBuffer, SIZE_READ, TIMEOUT);		// Attempt to send the opcode
	status = checkSpiStatus(spi_status);													// Check the HAL SPI status and update
	if(status != FRAM_SUCCESS)																// There is a problem, clean up and abort
	{
		deassertCS();
		return status;
	}
	spi_status = HAL_SPI_Receive(&FRAM.spiHandle, readDataBuffer, SIZE_READ_LOG, TIMEOUT);	// Receive in 2 bytes of data
	status = checkSpiStatus(spi_status);													// Check the HAL SPI status and update
	if(status != FRAM_SUCCESS)																// There is a problem, clean up and abort
	{
		deassertCS();
		return status;
	}
	deassertCS();																			// Transaction is over, so set CS high


	// Decode the log data and return
	// MSB[code=10bits][thread=3bits][log=3bits]LSB
	// Hi Byte  Lo Byte
	// 00000000 00000000
	codeTemp = ((uint16_t)readDataBuffer[0]<<2);											// Fills in the upper 8 code bits from the hi byte
	codeTemp |= (readDataBuffer[1]>>6);														// Fills in the lower 2 code bits from the lo byte
	threadTemp = ((readDataBuffer[1] & THREAD_MASK)>>3);									// Fills in the 3 thread bits from the lo byte
	levelTemp = (readDataBuffer[1] & LEVEL_MASK);											// Fills in the 3 log level bits from the lo byte
	*level = levelTemp;
	*thread = threadTemp;
	*code = codeTemp;


	return status;
}


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
uint16_t framGetNumLogs()
{
	// Check for proper initialization, update status
	if(FRAM.spiHandle.Instance == 0x0)														// SPI handle is not initialized yet, abort
	{
		return 0xFFFF;
	}

	return FRAM.NumLogs;
}


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
framReturnStatus_t framGetLogCurrAddr(uint16_t * address)
{
	// Check for proper initialization, update status
	if(FRAM.spiHandle.Instance == 0x0)														// SPI handle is not initialized yet, abort
	{
		*address = 0x0000;
		return FRAM_UNINITIALIZED;
	}


	// Return the data
	*address = FRAM.LogCurrAddr;
	return FRAM_SUCCESS;
}


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
framReturnStatus_t framGetAllLogsFromEnd(framLogLevel_t *levelBuffer, framLogThread_t * threadBuffer, framLogCode_t * codeBuffer)
{
	// Function variables
	framReturnStatus_t status = FRAM_GENERAL_FAIL;
	framLogLevel_t tempLevel;
	framLogThread_t tempThread;
	framLogCode_t tempCode;


	// Check for proper initialization, update status
	if(FRAM.spiHandle.Instance == 0x0)														// SPI handle is not initialized yet, abort
	{
		status = FRAM_UNINITIALIZED;
		return status;
	}


	// Get all the log entries from most recent to least recent and put them in the buffer
	uint16_t j = 0;
	for(uint16_t i = FRAM.NumLogs; i > 0; i--)
	{
		status = framGetSpecificLog(i, &tempLevel, &tempThread, &tempCode);
		if(status != FRAM_SUCCESS)													// There is a problem, abort
		{
			return status;
		}
		levelBuffer[j] = tempLevel;
		threadBuffer[j] = tempThread;
		codeBuffer[j] = tempCode;
		j++;
	}


	// We've retrieved all the logs in the data space
	return status;
}


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
//****************************************************************************************
framReturnStatus_t framGetAllLogsFromBegin(framLogLevel_t *levelBuffer, framLogThread_t * threadBuffer, framLogCode_t * codeBuffer)
{
	// Function variables
	framReturnStatus_t status = FRAM_GENERAL_FAIL;
	framLogLevel_t tempLevel;
	framLogThread_t tempThread;
	framLogCode_t tempCode;


	// Check for proper initialization, update status
	if(FRAM.spiHandle.Instance == 0x0)														// SPI handle is not initialized yet, abort
	{
		status = FRAM_UNINITIALIZED;
		return status;
	}


	// Get all the log entries from least recent to most recent and put them in the buffer
	for(uint16_t i = 1; i <= FRAM.NumLogs; i++)
	{
		status = framGetSpecificLog(i, &tempLevel, &tempThread, &tempCode);
		if(status != FRAM_SUCCESS)													// There is a problem, abort
		{
			return status;
		}
		levelBuffer[i-1] = tempLevel;
		threadBuffer[i-1] = tempThread;
		codeBuffer[i-1] = tempCode;
	}


	// We've retrieved all the logs in the data space
	return status;
}


//****************************************************************************************
// Implementation of module private helper functions
//****************************************************************************************
framReturnStatus_t checkSpiStatus(HAL_StatusTypeDef spi_status)
{
	// Function variables
	framReturnStatus_t status = FRAM_GENERAL_FAIL;


	// Check the spi status and update and return the framReturnStatus_t appropriately
	if(spi_status == HAL_ERROR)																// The HAL SPI API returned an error status
	{
		status = FRAM_SPI_ERROR;
		return status;
	}
	else if(spi_status == HAL_BUSY)															// The HAL SPI API returned a busy status
	{
		status = FRAM_SPI_BUSY;
		return status;
	}
	else if(spi_status == HAL_TIMEOUT)														// The HAL SPI API returned a timeout status
	{
		status = FRAM_SPI_TIMEOUT;
		return status;
	}
	else if (spi_status == HAL_OK)															// The HAL SPI API returned OK
	{
		status = FRAM_SUCCESS;
	}

	return status;																			// Should never reach here
}


void assertCS()
{
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);									// Set the chip select LOW
	return;
}


void deassertCS()
{
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);								// Set the chip select HIGH
	return;
}


framReturnStatus_t updateNumLogs(uint16_t data)
{
	// Function variables
	framReturnStatus_t status = FRAM_GENERAL_FAIL;
	HAL_StatusTypeDef spi_status = HAL_ERROR;
	uint8_t opcode = 0x0;
	uint8_t SpiAddrHiByte = 0x00;
	uint8_t SpiAddrLoByte = 0x00;
	uint8_t WriteDataLoByte = data;
	uint8_t WriteDataHiByte = (data>>8);
	uint8_t TransmitBuffer2[SIZE_WRITE_LOG];


	// Check for proper initialization, update status
	if(FRAM.spiHandle.Instance == 0x0)														// SPI handle is not initialized yet, abort
	{
		status = FRAM_UNINITIALIZED;
		return status;
	}


	// Check parameters
	if(data > MAX_NUM_LOGS)
	{
		status = FRAM_INVALID_ARGUMENT;
		return status;
	}


	// Update the SPI transmit buffer data before writing
	opcode = WRITE;
	SpiAddrHiByte = (NUM_LOGS_ADDRESS>>8);
	SpiAddrLoByte = NUM_LOGS_ADDRESS;
	TransmitBuffer2[0] = opcode;				// FRAM op code
	TransmitBuffer2[1] = SpiAddrHiByte;			// FRAM starting address hi byte
	TransmitBuffer2[2] = SpiAddrLoByte;			// FRAM starting address lo byte
	TransmitBuffer2[3] = WriteDataHiByte;		// NumLogs hi byte
	TransmitBuffer2[4] = WriteDataLoByte;		// NumLogs lo byte


	// Set the FRAM's write enable bit
	status = setWriteEnable();
	if(status != FRAM_SUCCESS)					// There is a problem, abort
	{
		return status;
	}


	// Attempt SPI transmission, update status
	assertCS();
	spi_status = HAL_SPI_Transmit(&FRAM.spiHandle, TransmitBuffer2, SIZE_WRITE_LOG, TIMEOUT);		// Attempt SPI transmit
	status = checkSpiStatus(spi_status);															// Check the HAL SPI status and update
	if(status != FRAM_SUCCESS)																		// There is a problem, clean up and abort
	{
		deassertCS();
		return status;
	}
	deassertCS();


	// We've finished writing, so send the WRDI (reset write enable latch) opcode to reset the WEL bit
	status = resetWriteEnable();
	if(status != FRAM_SUCCESS)																		// There is a problem, abort
	{
		return status;
	}


	// Now verify the data
	status = verifyData(WriteDataHiByte, NUM_LOGS_ADDRESS);
	if(status != FRAM_SUCCESS)																		// There is a problem, abort
	{
		return status;
	}
	status = verifyData(WriteDataLoByte, (NUM_LOGS_ADDRESS+1));
	if(status != FRAM_SUCCESS)																		// There is a problem, abort
	{
		return status;
	}


	// Everything is good. Update the FRAM.NumLogs data member and return status
	FRAM.NumLogs = data;
	return status;
}


framReturnStatus_t updateLogCurrAddr(uint16_t data)
{
	// Function variables
	framReturnStatus_t status = FRAM_GENERAL_FAIL;
	HAL_StatusTypeDef spi_status = HAL_ERROR;
	uint8_t opcode = 0x0;
	uint8_t SpiAddrHiByte = 0x00;
	uint8_t SpiAddrLoByte = 0x00;
	uint8_t WriteDataLoByte = data;
	uint8_t WriteDataHiByte = (data>>8);
	uint8_t TransmitBuffer2[SIZE_WRITE_LOG];


	// Update the SPI transmit buffer data before writing
	opcode = WRITE;
	SpiAddrHiByte = (CURR_ADDR_ADDRESS>>8);
	SpiAddrLoByte = CURR_ADDR_ADDRESS;
	TransmitBuffer2[0] = opcode;				// FRAM op code
	TransmitBuffer2[1] = SpiAddrHiByte;			// FRAM starting address hi byte
	TransmitBuffer2[2] = SpiAddrLoByte;			// FRAM starting address lo byte
	TransmitBuffer2[3] = WriteDataHiByte;		// NumLogs hi byte
	TransmitBuffer2[4] = WriteDataLoByte;		// NumLogs lo byte


	// Set the FRAM's write enable bit
	status = setWriteEnable();
	if(status != FRAM_SUCCESS)					// There is a problem, abort
	{
		return status;
	}

	// Attempt SPI transmission, update status
	assertCS();
	spi_status = HAL_SPI_Transmit(&FRAM.spiHandle, TransmitBuffer2, SIZE_WRITE_LOG, TIMEOUT);		// Attempt SPI transmit
	status = checkSpiStatus(spi_status);															// Check the HAL SPI status and update
	if(status != FRAM_SUCCESS)																		// There is a problem, clean up and abort
	{
		deassertCS();
		return status;
	}
	deassertCS();


	// We've finished writing, so send the WRDI (reset write enable latch) opcode to reset the WEL bit
	status = resetWriteEnable();
	if(status != FRAM_SUCCESS)																		// There is a problem, abort
	{
		return status;
	}


	// Now verify the data
	status = verifyData(WriteDataHiByte, CURR_ADDR_ADDRESS);
	if(status != FRAM_SUCCESS)																		// There is a problem, abort
	{
		return status;
	}
	status = verifyData(WriteDataLoByte, (CURR_ADDR_ADDRESS+1));
	if(status != FRAM_SUCCESS)																		// There is a problem, abort
	{
		return status;
	}


	// Everything is good. Update the FRAM.NumLogs data member and return status
	FRAM.LogCurrAddr = data;
	return status;
}


framReturnStatus_t getNumLogs(uint16_t * dataBuffer)
{
	// Function variables
	framReturnStatus_t status = FRAM_GENERAL_FAIL;
	HAL_StatusTypeDef spi_status = HAL_ERROR;
	uint8_t opcode = READ;																	// Set opcode to READ (read memory)
	uint8_t numLogsHiByte = 0x00;
	uint8_t numLogsLoByte = 0x00;
	uint8_t SpiAddrLoByte = NUM_LOGS_ADDRESS;
	uint8_t SpiAddrHiByte = (NUM_LOGS_ADDRESS>>8);
	uint8_t transmitBuffer[SIZE_READ];														// Create the transmit buffer
	uint8_t receiveBuffer[SIZE_READ_LOG];													// Create the receive buffer
	transmitBuffer[0] = opcode;
	transmitBuffer[1] = SpiAddrHiByte;
	transmitBuffer[2] = SpiAddrLoByte;


	// Attempt to read the data
	assertCS();																				// Set CS low to begin transaction
	spi_status = HAL_SPI_Transmit(&FRAM.spiHandle, transmitBuffer, SIZE_READ, TIMEOUT);		// Attempt to send the buffer
	status = checkSpiStatus(spi_status);													// Check the HAL SPI status and update
	if(status != FRAM_SUCCESS)																// There is a problem, clean up and abort
	{
		deassertCS();
		return status;
	}
	spi_status = HAL_SPI_Receive(&FRAM.spiHandle, receiveBuffer, SIZE_READ_LOG, TIMEOUT);	// Receive in two bytes of data (FRAM auto-increments internal address)
	status = checkSpiStatus(spi_status);													// Check the HAL SPI status and update
	if(status != FRAM_SUCCESS)																// There is a problem, clean up and abort
	{
		deassertCS();
		return status;
	}
	deassertCS();																			// Transaction is over, so set CS high
	numLogsHiByte = receiveBuffer[0];														// Store the data hi byte
	numLogsLoByte = receiveBuffer[1];														// Store the data lo byte


	// Store the two bytes into the return buffer
	*dataBuffer = (numLogsHiByte << 8) | numLogsLoByte;
	return status;
}


framReturnStatus_t getLogCurrAddr(uint16_t * dataBuffer)
{
	// Function variables
	framReturnStatus_t status = FRAM_GENERAL_FAIL;
	HAL_StatusTypeDef spi_status = HAL_ERROR;
	uint8_t opcode = READ;																	// Set opcode to READ (read memory)
	uint8_t numLogsHiByte = 0x00;
	uint8_t numLogsLoByte = 0x00;
	uint8_t SpiAddrLoByte = CURR_ADDR_ADDRESS;
	uint8_t SpiAddrHiByte = (CURR_ADDR_ADDRESS>>8);
	uint8_t transmitBuffer[SIZE_READ];														// Create the transmit buffer
	uint8_t receiveBuffer[SIZE_READ_LOG];													// Create the receive buffer
	transmitBuffer[0] = opcode;
	transmitBuffer[1] = SpiAddrHiByte;
	transmitBuffer[2] = SpiAddrLoByte;


	// Attempt to read the data
	assertCS();																				// Set CS low to begin transaction
	spi_status = HAL_SPI_Transmit(&FRAM.spiHandle, transmitBuffer, SIZE_READ, TIMEOUT);		// Attempt to send the buffer
	status = checkSpiStatus(spi_status);													// Check the HAL SPI status and update
	if(status != FRAM_SUCCESS)																// There is a problem, clean up and abort
	{
		deassertCS();
		return status;
	}
	spi_status = HAL_SPI_Receive(&FRAM.spiHandle, receiveBuffer, SIZE_READ_LOG, TIMEOUT);	// Receive in two bytes of data (FRAM auto-increments internal address)
	status = checkSpiStatus(spi_status);													// Check the HAL SPI status and update
	if(status != FRAM_SUCCESS)																// There is a problem, clean up and abort
	{
		deassertCS();
		return status;
	}
	deassertCS();																			// Transaction is over, so set CS high
	numLogsHiByte = receiveBuffer[0];														// Store the data hi byte
	numLogsLoByte = receiveBuffer[1];														// Store the data lo byte


	// Store the two bytes into the return buffer
	*dataBuffer = (numLogsHiByte << 8) | numLogsLoByte;
	return status;
}


framReturnStatus_t verifyData(uint8_t data, uint16_t address)
{
	// Function variables
	framReturnStatus_t status = FRAM_GENERAL_FAIL;
	HAL_StatusTypeDef spi_status = HAL_ERROR;
	uint8_t opcode = 0x00;
	uint8_t transmitBuffer[SIZE_READ];
	uint8_t recieveBuffer = 0x00;
	uint8_t addrLoByte = address;
	uint8_t addrHiByte = (address>>8);


	// Verify the data by reading it back and comparing
	opcode = READ;																			// Set opcode to READ (read memory)
	transmitBuffer[0] = opcode;																// Update the transmit buffer
	transmitBuffer[1] = addrHiByte;
	transmitBuffer[2] = addrLoByte;
	assertCS();																				// Set CS low to begin transaction
	spi_status = HAL_SPI_Transmit(&FRAM.spiHandle, transmitBuffer, SIZE_READ, TIMEOUT);		// Attempt to send the opcode
	status = checkSpiStatus(spi_status);													// Check the HAL SPI status and update
	if(status != FRAM_SUCCESS)																// There is a problem, clean up and abort
	{
		deassertCS();
		return status;
	}
	spi_status = HAL_SPI_Receive(&FRAM.spiHandle, &recieveBuffer, SIZE_DATA_BYTE, TIMEOUT);	// Receive in one byte of data
	status = checkSpiStatus(spi_status);													// Check the HAL SPI status and update
	if(status != FRAM_SUCCESS)																// There is a problem, clean up and abort
	{
		deassertCS();
		return status;
	}
	deassertCS();																			// Transaction is over, so set CS high
	if(recieveBuffer == data)																// The data matches, success
	{
		status = FRAM_SUCCESS;
	}
	else																					// Data mismatch, write failed
	{
		status = FRAM_WRITE_FAILED;
	}
	return status;																			// We're done, return the status
}


framReturnStatus_t resetWriteEnable()
{
	// Function variables
	framReturnStatus_t status = FRAM_GENERAL_FAIL;
	HAL_StatusTypeDef spi_status = HAL_ERROR;
	uint8_t opcode = 0x00;


	// We've finished writing, so send the WRDI (reset write enable latch) opcode to reset the WEL bit
	assertCS();																				// Set the CS low
	opcode = WRDI;																			// Set opcode to WRDI (reset write enable latch)
	spi_status = HAL_SPI_Transmit(&FRAM.spiHandle, &opcode, SIZE_OPCODE, TIMEOUT);		    // Attempt SPI transaction
	status = checkSpiStatus(spi_status);													// Check the HAL SPI status and update
	if(status != FRAM_SUCCESS)																// There is a problem, clean up and abort
	{
		deassertCS();
		return status;
	}
	deassertCS();																			// Set CS high to end transaction
	return status;
}


framReturnStatus_t setWriteEnable()
{
	// Function variables
	framReturnStatus_t status = FRAM_GENERAL_FAIL;
	HAL_StatusTypeDef spi_status = HAL_ERROR;
	uint8_t opcode = 0x00;


	// Send the WREN opcode to enable writing
	opcode = WREN;																			// Set opcode to WREN (write enable latch)
	assertCS();																				// Set the CS low
	spi_status = HAL_SPI_Transmit(&FRAM.spiHandle, &opcode, SIZE_OPCODE, TIMEOUT);		    // Attempt SPI transaction
	status = checkSpiStatus(spi_status);													// Check the HAL SPI status and update
	if(status != FRAM_SUCCESS)																// There is a problem, clean up and abort
	{
		deassertCS();
		return status;
	}
	deassertCS();
	return status;
}

