/****************************************************************************
* Component: trf.pontos12.service.InterfaceUART
****************************************************************************/
#include "config.h"

//************************************************
// Includes
//************************************************
#include "InterfaceUART.h"
#include <freertos.kernel/FreeRTOSKernel/FreeRTOSKernel.h>

using escort::driver::IUARTDriver;

//************************************************
// Functions
//************************************************

namespace trf {
namespace pontos12 {
namespace application {

InterfaceUART::InterfaceUART(IUARTDriver* driver_uart) :
			driver_uart(driver_uart)
{}

void InterfaceUART::init()
{
}

void InterfaceUART::start()
{
	this->driver_uart->setRxTimeout(2000);
	this->driver_uart->open();
}

void InterfaceUART::waitConnection()
{
	while(this->driver_uart->isRxEmpty()){
		vTaskDelay(1);
	}
}

bool InterfaceUART::isConnected()
{
	return this->driver_uart->isRxEmpty() == false;
}

U32 InterfaceUART::send(U8* dataBytes, U32 nBytes)
{
	return this->driver_uart->write(dataBytes, nBytes);
}

U32 InterfaceUART::receive(U8* dataBytes, U32 nBytes)
{
	return this->driver_uart->read(dataBytes, nBytes);
}

U32 InterfaceUART::bytesToRead()
{
	return this->driver_uart->getAvailable();
}

void InterfaceUART::stop()
{
	this->driver_uart->close();
}

}
}
}

