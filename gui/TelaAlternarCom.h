/*
 * TelaAlternarCom.h
 *
 *  Created on: 04/06/2012
 *      Author: arthur.padilha
 */

#ifndef TELAALTERNARCOM_H_
#define TELAALTERNARCOM_H_


#include "InputOutput.h"
#include <freertos.kernel/FreeRTOSKernel/FreeRTOSKernel.h>
#include "Tela.h"
#include "../Fachada.h"

namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

class TelaAlternarCom : public Tela {
public:
	TelaAlternarCom(Fachada* fachada, InputOutput* inputOutput);

private:
	virtual void paint();
	virtual void keyEvent(Teclado::Tecla tecla);
	virtual void start();
	virtual void finish();

private:
	Fachada* fachada;
	U8 alternanciaSelecionada;
	U8 qntAlternancias;
	char nomeAlternancia[32];
	bool modificado;
};
}
}
}
}


#endif /* TELAALTERNARCOM_H_ */
