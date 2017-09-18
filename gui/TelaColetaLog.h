/*
 * TelaColetaLog.h
 *
 *  Created on: 30/05/2012
 *      Author: Gustavo
 */

#ifndef TELACOLETALOG_H_
#define TELACOLETALOG_H_

#include "InputOutput.h"
#include <freertos.kernel/FreeRTOSKernel/FreeRTOSKernel.h>
#include "../Fachada.h"
#include "Tela.h"

namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

class TelaColetaLog : public Tela {
public:
	TelaColetaLog(Fachada* fachada, InputOutput* inputOutput);
	virtual void show();
private:
	virtual void paint();
	virtual void keyEvent(Teclado::Tecla tecla);
	virtual void start();
	virtual void finish();
	bool verificarPermissao();

private:
	Fachada* fachada;
};

}
}
}
}

#endif /* TELACOLETADUMP_H_ */
