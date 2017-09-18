/*
 * TelaAcenderPaineis.h
 *
 *  Created on: 20/11/2014
 *      Author: luciano.silva
 */

#ifndef TELAACENDERPAINEIS_H_
#define TELAACENDERPAINEIS_H_

#include "InputOutput.h"
#include <freertos.kernel/FreeRTOSKernel/FreeRTOSKernel.h>
#include "../Fachada.h"
#include "Tela.h"
#include "TelaTempoAcenderPaineis.h"

namespace trf {
namespace pontos12 {
namespace application {
namespace gui {


class TelaAcenderPaineis : public Tela {
private:
	typedef enum {
		TODOS_PAINEIS,
		PAINEL_X
	} Opcoes;

public:
	TelaAcenderPaineis(Fachada* fachada, InputOutput* inputOutput);
private:
	virtual void paint();
	virtual void keyEvent(Teclado::Tecla tecla);
	virtual void start();
	virtual void finish();
	bool verificarPermissao();

private:
	Fachada* fachada;
	TelaTempoAcenderPaineis telaTempoAcenderPaineis;
	Opcoes opcoes;
	U8 indiceAtual;
	U8 qntPaineis;
};

}
}
}
}

#endif /* TELAACENDERPAINEIS_H_ */
