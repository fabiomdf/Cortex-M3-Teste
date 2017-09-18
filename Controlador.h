/****************************************************************************
* Component: trf.pontos12.application.Controlador
****************************************************************************/


#ifndef trf_pontos12_APPLICATION_CONTROLADOR_
#define trf_pontos12_APPLICATION_CONTROLADOR_

//************************************************
// Includes
//************************************************
#include <trf.application/trfProduct/trfProduct.h>
#include <escort.hal\IGPIOPin\IGPIOPin.h>
#include <escort.driver\IKeypad\IKeypad.h>
#include <escort.driver\ILCDDisplay\ILCDDisplay.h>
#include <escort.service/Relogio/Relogio.h>
#include <escort.service/FatFs/FatFs.h>
#include "Fachada.h"
#include "gui/GUI.h"
#include "Plataforma.h"
#include <escort.hal\ICPU\ICPU.h>
#include "Protocolo.h"
#include "escort.service/USBHost/USBHost.h"
#include "InterfaceConexao.h"
#include "InterfaceUART.h"
#include <trf.service.net/Network/Network.h>
#include <trf.service.net\IdentificationProtocol\IdentificationProtocol.h>
#include <escort.driver\IUARTDriver\IUARTDriver.h>
#include "InterfaceConexao.h"


//************************************************
// Namespace
//************************************************
namespace trf {
namespace pontos12 {
namespace application {

//************************************************
// Class
//************************************************
class Controlador : public trf::application::trfProduct {
public:
	Controlador(
			escort::hal::ICPU* cpu,
			escort::hal::IGPIOPin* sinalEmergencia,
			escort::driver::IKeypad* teclado,
			escort::driver::ILCDDisplay* display,
			escort::driver::IDriverFlash* e2prom,
			escort::driver::IUARTDriver* uart,
			escort::service::Relogio* relogio,
			trf::service::net::Network* network,
			trf::service::net::IdentificationProtocol* idProtocol,
			escort::service::NandFFS* fileSystem,
			escort::service::USBHost* usbHost,
			escort::driver::IRS485Driver* rs485);

	virtual void init();
	virtual SerialNumber getSerialNumber();
	virtual bool provideService(char* serviceName);
	bool cancelaOperacaoNormal();
	void monitoramentoTask();
	void rs485Task();
	void agendaTask();

private:
	void sincronizarPaineis();
	void checarStatusPaineis();
	void enviarTurbusString();
	void sincronizarAPP();
	void checarStatusAPP();
	void sincronizarAdaptadorCatraca();
	void sincronizarAdaptadorSensores();
	void checarStatusAdaptadorCatraca();

private:
	Plataforma plataforma;
	Fachada fachada;
	gui::GUI gui;
	InterfaceConexao interfaceCAN;
	Protocolo protocoloCAN;
	InterfaceUART interfaceUART;
	Protocolo protocoloUART;
};

}
}
}
#endif // trf_pontos12_APPLICATION_CONTROLADOR_
