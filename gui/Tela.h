/*
 * Tela.h
 *
 *  Created on: 30/05/2012
 *      Author: Gustavo
 */

#ifndef TELA_H_
#define TELA_H_

#include "InputOutput.h"
#include <freertos.kernel/FreeRTOSKernel/FreeRTOSKernel.h>

namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

class Tela {
public:
	Tela(InputOutput* inputOutput);
	virtual void show();
protected:
	virtual void paint() = 0;
	virtual void keyEvent(Teclado::Tecla tecla);
	virtual void usbEvent(escort::service::USBHost::StatusUSB statusUSB);
	virtual void start() = 0;
	virtual void finish() = 0;
	virtual void handleKeys();
	virtual void handleUSB();

protected:
	InputOutput* inputOutput;
	bool visible;
	portTickType timeoutTecla;
	portTickType teclaTime;
	escort::service::USBHost::StatusUSB oldStatusUSB;
};

}
}
}
}

#endif /* TELA_H_ */
