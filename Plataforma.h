/*
 * Plataforma.h
 *
 *  Created on: 08/05/2012
 *      Author: Gustavo
 */

#ifndef PLATAFORMApontos12_H_
#define PLATAFORMApontos12_H_

#include <escort.service/NandFFS/NandFFS.h>
#include <escort.service/Relogio/Relogio.h>
#include <escort.hal\ICPU\ICPU.h>
#include "escort.service/USBHost/USBHost.h"
#include <escort.driver\IDriverFlash\IDriverFlash.h>
#include <escort.driver\IRS485Driver\IRS485Driver.h>
#include <escort.driver\IUARTDriver\IUARTDriver.h>

namespace trf {
namespace pontos12 {
namespace application {

class Plataforma {

public:
	Plataforma(
			escort::hal::ICPU* cpu,
			escort::driver::IDriverFlash* e2prom,
			escort::driver::IUARTDriver* uart,
			escort::service::NandFFS* sistemaArquivo,
			escort::service::USBHost* usbHost,
			escort::service::Relogio* relogio,
			escort::driver::IRS485Driver* rs485):
		cpu(cpu),
		e2prom(e2prom),
		uart(uart),
		sistemaArquivo(sistemaArquivo),
		usbHost(usbHost),
		relogio(relogio),
		rs485(rs485)
	{}

	void resetSystem(){
		this->cpu->reset();
	}

public:
	escort::hal::ICPU* cpu;
	escort::driver::IDriverFlash* e2prom;
	escort::driver::IUARTDriver* uart;
	escort::service::NandFFS* sistemaArquivo;
	escort::service::USBHost* usbHost;
	escort::service::Relogio* relogio;
	escort::driver::IRS485Driver* rs485;
};

}
}
}

#endif /* PLATAFORMApontos12_H_ */
