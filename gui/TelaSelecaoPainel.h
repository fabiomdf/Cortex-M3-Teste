/*
 * TelaSelecaoPainel.h
 *
 *  Created on: 17/04/2013
 *      Author: luciano.silva
 */

#ifndef TELASELECAOPAINEL_H_
#define TELASELECAOPAINEL_H_

#include "InputOutput.h"
#include <freertos.kernel/FreeRTOSKernel/FreeRTOSKernel.h>
#include "../Fachada.h"
#include "Tela.h"

namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

class TelaSelecaoPainel : public Tela {
public:
	TelaSelecaoPainel(
			Fachada* fachada,
			InputOutput* inputOutput);

private:
	virtual void paint();
	virtual void keyEvent(Teclado::Tecla tecla);
	virtual void start();
	virtual void finish();

private:
	Fachada* fachada;
	U8 indicePainel;
	U8 qntPaineis;
	char buffer[64];
	bool modificado;
};

}
}
}
}

#endif /* TELASELECAOPAINEL_H_ */
