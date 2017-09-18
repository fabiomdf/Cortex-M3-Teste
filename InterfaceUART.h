/****************************************************************************
* Component: trf.pontos12.service.InterfaceUART
****************************************************************************/


#ifndef trf_pontos12_SERVICE_INTERFACEUART_
#define trf_pontos12_SERVICE_INTERFACEUART_

//************************************************
// Includes
//************************************************
#include <escort.driver\IUARTDriver\IUARTDriver.h>
#include "trf.application/IComunicacao/IComunicacao.h"

//************************************************
// Namespace
//************************************************
namespace trf {
namespace pontos12 {
namespace application {

//************************************************
// Class
//************************************************
class InterfaceUART : public trf::application::IComunicacao{
public:
	InterfaceUART(escort::driver::IUARTDriver* driver_uart);
	virtual void init();
	virtual void start();
	virtual void waitConnection();
	virtual bool isConnected();
	virtual U32 send(U8* dataBytes, U32 nBytes);
	virtual U32 receive(U8* dataBytyes, U32 nBytes);
	virtual U32 bytesToRead();
	virtual void stop();

public:
	escort::driver::IUARTDriver* driver_uart;
};

}
}
}
#endif // trf_pontos12_SERVICE_INTERFACEUART_
