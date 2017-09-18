/*
 * TelaConfiguracoes.h
 *
 *  Created on: 30/05/2012
 *      Author: Gustavo
 */

#ifndef TELACONFIGURACOES_H_
#define TELACONFIGURACOES_H_

#include "InputOutput.h"
#include <freertos.kernel/FreeRTOSKernel/FreeRTOSKernel.h>
#include "../Fachada.h"
#include "Tela.h"

namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

class TelaConfiguracoes : public Tela {
private:
	typedef enum {
		VERSAO,
		QNT_ROTEIROS,
		QNT_MENSAGENS,
		QNT_PAINEIS,
		NUMERO_SERIE,
		PAINEL
	} Info;

public:
	TelaConfiguracoes(Fachada* fachada, InputOutput* inputOutput);
private:
	virtual void paint();
	virtual void keyEvent(Teclado::Tecla tecla);
	virtual void start();
	virtual void finish();

private:
	Fachada* fachada;
	Info indice;
	char temp[16];
	U8 painelSelecionado;
};

}
}
}
}

#endif /* TELACONFIGURACOES_H_ */
