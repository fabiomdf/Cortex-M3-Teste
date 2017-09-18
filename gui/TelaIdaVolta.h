/*
 * TelaIdaVolta.h
 *
 *  Created on: 30/05/2012
 *      Author: Gustavo
 */

#ifndef TELAIDAVOLTA_H_
#define TELAIDAVOLTA_H_

#include "InputOutput.h"
#include <freertos.kernel/FreeRTOSKernel/FreeRTOSKernel.h>
#include "../Fachada.h"
#include "Tela.h"

namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

class TelaIdaVolta : public Tela {
public:
	TelaIdaVolta(Fachada* fachada, InputOutput* inputOutput);
private:
	virtual void paint();
	virtual void keyEvent(Teclado::Tecla tecla);
	virtual void start();
	virtual void finish();

private:
	Fachada* fachada;
	Fachada::SentidoRoteiro sentido;
	bool modificacao;
};

}
}
}
}

#endif /* TELAIDAVOLTA_H_ */
