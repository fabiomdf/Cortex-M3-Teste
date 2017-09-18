/*
 * TelaTempoAcenderPaineis.h
 *
 *  Created on: 12/03/2015
 *      Author: luciano.silva
 */

#ifndef TELATEMPOACENDERPAINEIS_H_
#define TELATEMPOACENDERPAINEIS_H_

#include "InputOutput.h"
#include <freertos.kernel/FreeRTOSKernel/FreeRTOSKernel.h>
#include "../Fachada.h"
#include "Tela.h"

namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

class TelaTempoAcenderPaineis : public Tela {
private:
	typedef enum {
		TODOS_PAINEIS,
		PAINEL_X
	} Opcoes;

public:
	TelaTempoAcenderPaineis(Fachada* fachada, InputOutput* inputOutput);
	void setIndiceAtual(U8 indiceAtual);

private:
	virtual void paint();
	virtual void keyEvent(Teclado::Tecla tecla);
	virtual void start();
	virtual void finish();

private:
	Fachada* fachada;
	U8 indiceAtual;
	U16 tempo;
};

}
}
}
}

#endif /* TELATEMPOACENDERPAINEIS_H_ */
