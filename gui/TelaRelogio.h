/*
 * TelaRelogio.h
 *
 *  Created on: 30/05/2012
 *      Author: Gustavo
 */

#ifndef TELARELOGIO_H_
#define TELARELOGIO_H_

#include "InputOutput.h"
#include <freertos.kernel/FreeRTOSKernel/FreeRTOSKernel.h>
#include "../Fachada.h"
#include "Tela.h"

namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

class TelaRelogio : public Tela {
public:
	TelaRelogio(Fachada* fachada, InputOutput* inputOutput);
private:
	virtual void paint();
	virtual void keyEvent(Teclado::Tecla tecla);
	virtual void start();
	virtual void finish();
	U16 incrementar(U16 valor, U16 limitanteInferior, U16 limitanteSuperior);
	U16 decrementar(U16 valor, U16 limitanteInferior, U16 limitanteSuperior);
	bool verificarPermissaoAjuste();

private:
	Fachada* fachada;
	U8 indiceCursor;
	bool cursorPiscando;
	portTickType timerCursor;
	portTickType timerPaint;
	bool alterou;
	bool acessoLiberado;
};

}
}
}
}

#endif /* TELARELOGIO_H_ */
