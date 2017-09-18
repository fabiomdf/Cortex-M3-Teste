/*
 * TelaSelecaoRoteiro.h
 *
 *  Created on: 30/05/2012
 *      Author: Gustavo
 */

#ifndef TELASELECAOROTEIRO_H_
#define TELASELECAOROTEIRO_H_

#include "InputOutput.h"
#include <freertos.kernel/FreeRTOSKernel/FreeRTOSKernel.h>
#include "../Fachada.h"
#include "Tela.h"

namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

class TelaSelecaoRoteiro : public Tela {
public:
	TelaSelecaoRoteiro(
			Fachada* fachada,
			InputOutput* inputOutput);
private:
	virtual void paint();
	virtual void keyEvent(Teclado::Tecla tecla);
	virtual void start();
	virtual void finish();

private:
	Fachada* fachada;
	U32 indiceRoteiro;
	U16 qntRoteiros;
	char buffer[64];
	bool modificado;
};

}
}
}
}

#endif /* TELASELECAOROTEIRO_H_ */
