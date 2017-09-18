/****************************************************************************
* Component: trf.pontos12.service.InterfaceConexao
****************************************************************************/
#include "config.h"

//************************************************
// Includes
//************************************************
#include "InterfaceConexao.h"
#include <freertos.kernel/FreeRTOSKernel/FreeRTOS.h>
#include <freertos.kernel/FreeRTOSKernel/task.h>

using trf::service::net::Network;
using trf::service::net::IdentificationProtocol;
using trf::service::net::ConnectionClient;
using trf::service::net::ConnectionListener;

//************************************************
// Functions
//************************************************

namespace trf {
namespace pontos12 {
namespace application {

InterfaceConexao::InterfaceConexao(
	Network* network,
	IdentificationProtocol* idProtocol) :
		network(network),
		idProtocol(idProtocol)
{}

void InterfaceConexao::init()
{}

void InterfaceConexao::start()
{
	//Aguarda o dispositivo atribuir um endere�o na rede
	while(!this->idProtocol->isAddressAssigned()){
		vTaskDelay(1);
	}
	//Cria um listener
	this->listener = this->network->listen(0);
}

void InterfaceConexao::waitConnection()
{
	do {
		this->client = this->listener->accept();
	}while(this->client == 0);
	this->client->setReceiveTimeout(2000);
}

bool InterfaceConexao::isConnected()
{
	if(this->client) return this->client->isConnected();
	else return false;
}

U32 InterfaceConexao::send(U8* dataBytes, U32 nBytes)
{
	if(this->client) return this->client->send(dataBytes, nBytes);
	else return 0;
}

U32 InterfaceConexao::receive(U8* dataBytyes, U32 nBytes)
{
	if(this->client) return this->client->receive(dataBytyes, nBytes);
	else return 0;
}

U32 InterfaceConexao::bytesToRead()
{
	if(this->client && this->client->isConnected()) return this->client->getInputStream()->available();
	else return 0;
}

void InterfaceConexao::stop()
{
	if(this->client) this->client->close();
}

}
}
}

