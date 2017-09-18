/*
 * TelaSelecaoMensagem.h
 *
 *  Created on: 30/05/2012
 *      Author: Gustavo
 */

#ifndef TELASELECAOMENSAGEM_H_
#define TELASELECAOMENSAGEM_H_

#include "InputOutput.h"
#include <freertos.kernel/FreeRTOSKernel/FreeRTOSKernel.h>
#include "../Fachada.h"
#include "Tela.h"

namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

class TelaSelecaoMensagem : public Tela {
public:
	TelaSelecaoMensagem(
			Fachada* fachada,
			InputOutput* inputOutput);

private:
	virtual void paint();
	virtual void keyEvent(Teclado::Tecla tecla);
	virtual void start();
	virtual void finish();

protected:
	Fachada* fachada;
	U32 indiceMensagem;
	U16 qntMensagens;
	char buffer[64];
	bool modificado;
};

}
}
}
}

#endif /* TELASELECAOMENSAGEM_H_ */
