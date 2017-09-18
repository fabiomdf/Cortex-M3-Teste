/*
 * TelaTesteTeclado.h
 *
 *  Created on: 30/05/2012
 *      Author: Gustavo
 */

#ifndef TELATESTETECLADO_H_
#define TELATESTETECLADO_H_

#include "InputOutput.h"
#include <freertos.kernel/FreeRTOSKernel/FreeRTOSKernel.h>
#include "../Fachada.h"
#include "Tela.h"

namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

class TelaTesteTeclado : public Tela {
private:
	typedef enum{
		ESPERANDO_TECLA_ROT_ESQ,
		ESPERANDO_TECLA_ROT_DIR,
		ESPERANDO_TECLA_IDA_VOLTA,
		ESPERANDO_TECLA_ALTERNA,
		ESPERANDO_TECLA_MSG_ESQ,
		ESPERANDO_TECLA_MSG_DIR,
		ESPERANDO_TECLA_SELEC_PAINEL,
		ESPERANDO_TECLA_AJUSTES_ESQ,
		ESPERANDO_TECLA_AJUSTES_DIR,
		ESPERANDO_TECLA_OK
	} Estado;

public:
	TelaTesteTeclado(Fachada* fachada, InputOutput* inputOutput);
private:
	virtual void paint();
	virtual void keyEvent(Teclado::Tecla tecla);
	virtual void start();
	virtual void finish();

private:
	Estado estado;
};

}
}
}
}

#endif /* TELATESTETECLADO_H_ */
