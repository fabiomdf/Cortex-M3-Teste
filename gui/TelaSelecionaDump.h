/*
 * TelaSelecionaDump.h
 *
 *  Created on: 10/12/2014
 *      Author: luciano.silva
 */

#ifndef TELASELECIONADUMP_H_
#define TELASELECIONADUMP_H_

#include "InputOutput.h"
#include <freertos.kernel/FreeRTOSKernel/FreeRTOSKernel.h>
#include "../Fachada.h"
#include "Tela.h"
#include "TelaColetaDump.h"

namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

class TelaSelecionaDump : public Tela {
private:
	typedef enum {
		CONTROLADOR,
		TODOS_DISPOSITIVOS,
		TODOS_PAINEIS,
		PAINEL_X
	} Opcoes;

public:
	TelaSelecionaDump(Fachada* fachada, InputOutput* inputOutput);
private:
	virtual void paint();
	virtual void keyEvent(Teclado::Tecla tecla);
	virtual void start();
	virtual void finish();

private:
	Fachada* fachada;
	TelaColetaDump telaColetaDump;
	U8 indiceMaximo;
	U8 indiceAtual;
};

}
}
}
}

#endif /* TELASELECIONADUMP_H_ */
