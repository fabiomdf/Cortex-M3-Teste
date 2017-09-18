/*
 * TelaColetaDump.h
 *
 *  Created on: 30/05/2012
 *      Author: Gustavo
 */

#ifndef TELACOLETADUMP_H_
#define TELACOLETADUMP_H_

#include "InputOutput.h"
#include <freertos.kernel/FreeRTOSKernel/FreeRTOSKernel.h>
#include "../Fachada.h"
#include "Tela.h"

namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

class TelaColetaDump : public Tela {
private:
	typedef enum {
		CONTROLADOR,
		TODOS_DISPOSITIVOS,
		TODOS_PAINEIS,
		PAINEL_X
	} Opcoes;

public:
	TelaColetaDump(Fachada* fachada, InputOutput* inputOutput);
	virtual void show();
	void setOpcao(U8 opcao);
	void setDestFolder(char* destFolder);

private:
	virtual void paint();
	virtual void keyEvent(Teclado::Tecla tecla);
	virtual void start();
	virtual void finish();
	bool verificarPermissao();
	Fachada::Resultado coletarDump(void (*incrementProgress)(void));

private:
	Fachada* fachada;
	U8 opcao;
	char* destFolder;
};

}
}
}
}

#endif /* TELACOLETADUMP_H_ */
