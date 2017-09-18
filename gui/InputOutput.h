/*
 * InputOutput.h
 *
 *  Created on: 30/05/2012
 *      Author: Gustavo
 */

#ifndef INPUTOUTPUT_H_
#define INPUTOUTPUT_H_

#include <escort.driver/IKeypad/IKeypad.h>
#include <escort.driver/ILCDDisplay/ILCDDisplay.h>
#include "Teclado.h"
#include "Display.h"
#include <escort.service/USBHost/USBHost.h>

namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

class InputOutput {
public:
	InputOutput(
		escort::driver::IKeypad* teclado,
		escort::driver::ILCDDisplay* display,
		escort::service::USBHost* usbHost) :
			teclado(teclado),
			display(display),
			usbHost(usbHost)
	{}

public:
	Teclado teclado;
	Display display;
	escort::service::USBHost* usbHost;
};

}
}
}
}

#endif /* INPUTOUTPUT_H_ */
