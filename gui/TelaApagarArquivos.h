/*
 * TelaApagarArquivos.h
 *
 *  Created on: 07/11/2014
 *      Author: luis.felipe
 */

#ifndef TELAAPAGARARQUIVOS_H_
#define TELAAPAGARARQUIVOS_H_

#include "InputOutput.h"
#include <freertos.kernel/FreeRTOSKernel/FreeRTOSKernel.h>
#include "../Fachada.h"
#include "Tela.h"

namespace trf {
namespace pontos12 {
namespace application {
namespace gui {


class TelaApagarArquivos : public Tela {
private:
	typedef enum {
		CONTROLADOR,
		TODOS_DISPOSITIVOS,
		APP,
		TODOS_PAINEIS,
		PAINEL_X
	} Opcoes;

public:
	TelaApagarArquivos(Fachada* fachada, InputOutput* inputOutput);
private:
	virtual void paint();
	virtual void keyEvent(Teclado::Tecla tecla);
	virtual void start();
	virtual void finish();
	bool verificarPermissao();

private:
	Fachada* fachada;
	Opcoes opcoes;
	U8 indiceMaximo;
	U8 indiceAtual;
};

}
}
}
}
#endif /* TELAAPAGARARQUIVOS_H_ */
