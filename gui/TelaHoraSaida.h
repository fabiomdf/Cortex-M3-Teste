/*
 * TelaHoraSaida.h
 *
 *  Created on: 30/05/2012
 *      Author: Gustavo
 */

#ifndef TELAHORASAIDA_H_
#define TELAHORASAIDA_H_

#include "InputOutput.h"
#include <freertos.kernel/FreeRTOSKernel/FreeRTOSKernel.h>
#include "../Fachada.h"
#include "Tela.h"

namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

class TelaHoraSaida : public Tela {
public:
	TelaHoraSaida(Fachada* fachada, InputOutput* inputOutput);
private:
	virtual void paint();
	virtual void keyEvent(Teclado::Tecla tecla);
	virtual void start();
	virtual void finish();
	bool verificarPermissaoAjuste();

private:
	Fachada* fachada;
	U8 horas;
	U8 minutos;
	U8 cursor;
	portTickType timerCursor;
	bool modificado;
	bool acessoLiberado;
};

}
}
}
}

#endif /* TELAHORASAIDA_H_ */
