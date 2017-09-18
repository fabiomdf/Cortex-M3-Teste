/*
 * TelaModoTeste.h
 *
 *  Created on: 28/08/2015
 *      Author: Gustavo
 */

#ifndef TELAMODOTESTE_H_
#define TELAMODOTESTE_H_

#include "InputOutput.h"
#include <freertos.kernel/FreeRTOSKernel/FreeRTOSKernel.h>
#include "../Fachada.h"
#include "Tela.h"

namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

class TelaModoTeste : public Tela {
public:
	TelaModoTeste(
			Fachada* fachada,
			InputOutput* inputOutput);
protected:
	virtual void handleKeys();
private:
	virtual void paint();
	virtual void keyEvent(Teclado::Tecla tecla);
	virtual void start();
	virtual void finish();
	void setTeste(U8 tipo);

private:
	Fachada* fachada;
	U8 indiceMenu;
};

}
}
}
}

#endif /* TELAMODOTESTE_H_ */
