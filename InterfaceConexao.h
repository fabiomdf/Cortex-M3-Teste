/****************************************************************************
* Component: trf.pontos12.service.InterfaceConexao
****************************************************************************/


#ifndef trf_pontos12_SERVICE_INTERFACECONEXAO_
#define trf_pontos12_SERVICE_INTERFACECONEXAO_

//************************************************
// Includes
//************************************************
#include <trf.service.net/Network/Network.h>
#include <trf.service.net\IdentificationProtocol\IdentificationProtocol.h>
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
class InterfaceConexao : public trf::application::IComunicacao{
public:
	InterfaceConexao(
			trf::service::net::Network* network,
			trf::service::net::IdentificationProtocol* idProtocol);
	virtual void init();
	virtual void start();
	virtual void waitConnection();
	virtual bool isConnected();
	virtual U32 send(U8* dataBytes, U32 nBytes);
	virtual U32 receive(U8* dataBytyes, U32 nBytes);
	virtual U32 bytesToRead();
	virtual void stop();

public:
	trf::service::net::Network* network;
	trf::service::net::IdentificationProtocol* idProtocol;
	trf::service::net::ConnectionListener* listener;
	trf::service::net::ConnectionClient* client;
};

}
}
}
#endif // trf_pontos12_SERVICE_INTERFACECONEXAO_
