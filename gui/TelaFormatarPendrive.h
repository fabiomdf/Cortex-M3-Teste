/*
 * TelaFormatarPendrive.h
 *
 *  Created on: 30/05/2012
 *      Author: Gustavo
 */

#ifndef TELAFORMATARPENDRIVE_H_
#define TELAFORMATARPENDRIVE_H_

#include "InputOutput.h"
#include <freertos.kernel/FreeRTOSKernel/FreeRTOSKernel.h>
#include "../Fachada.h"
#include "Tela.h"

namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

class TelaFormatarPendrive : public Tela {
public:
	TelaFormatarPendrive(Fachada* fachada, InputOutput* inputOutput);
	virtual void show();
private:
	virtual void paint();
	virtual void keyEvent(Teclado::Tecla tecla);
	virtual void start();
	virtual void finish();

private:
	Fachada* fachada;
};

}
}
}
}

#endif /* TELAFORMATARPENDRIVE_H_ */
