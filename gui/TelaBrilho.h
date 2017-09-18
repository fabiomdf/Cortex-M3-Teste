/*
 * TelaBrilho.h
 *
 *  Created on: 30/05/2012
 *      Author: Gustavo
 */

#ifndef TELABRILHO_H_
#define TELABRILHO_H_

#include "InputOutput.h"
#include <freertos.kernel/FreeRTOSKernel/FreeRTOSKernel.h>
#include "../Fachada.h"
#include "Tela.h"

namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

class TelaBrilho : public Tela {
public:
	TelaBrilho(Fachada* fachada, InputOutput* inputOutput);
private:
	virtual void paint();
	virtual void keyEvent(Teclado::Tecla tecla);
	virtual void start();
	virtual void finish();

private:
	Fachada* fachada;
	U8 brilhoMax;
	U8 brilhoMin;
	U8 cursor;
	portTickType timerCursor;
};

}
}
}
}

#endif /* TELABRILHO_H_ */
