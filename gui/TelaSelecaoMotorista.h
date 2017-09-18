/*
 * TelaSelecaoMotorista.h
 *
 *  Created on: 30/05/2012
 *      Author: Gustavo
 */

#ifndef TELASELECAOMOTORISTA_H_
#define TELASELECAOMOTORISTA_H_

#include "InputOutput.h"
#include <freertos.kernel/FreeRTOSKernel/FreeRTOSKernel.h>
#include "../Fachada.h"
#include "Tela.h"

namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

class TelaSelecaoMotorista : public Tela {
public:
	TelaSelecaoMotorista(
			Fachada* fachada,
			InputOutput* inputOutput);
private:
	virtual void paint();
	virtual void keyEvent(Teclado::Tecla tecla);
	virtual void start();
	virtual void finish();

private:
	Fachada* fachada;
	U16 indiceMotorista;
	U16 qntMotorista;
	char buffer[64];
	bool modificado;
};

}
}
}
}

#endif /* TELASELECAOMOTORISTA_H_ */
