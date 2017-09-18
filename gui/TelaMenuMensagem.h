/*
 * TelaMenuMensagem.h
 *
 *  Created on: 30/05/2012
 *      Author: Gustavo
 */

#ifndef TELAMENUMENSAGEM_H_
#define TELAMENUMENSAGEM_H_

#include "InputOutput.h"
#include <freertos.kernel/FreeRTOSKernel/FreeRTOSKernel.h>
#include "../Fachada.h"
#include "Tela.h"

#include "TelaSelecaoMensagem.h"
#include "TelaSelecaoMensagem2.h"

namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

class TelaMenuMensagem : public Tela {
private:
	typedef enum {
		MENSAGEM_PRINCIPAL,
		MENSAGEM_SECUNDARIA
	} Opcao;

public:
	TelaMenuMensagem(Fachada* fachada, InputOutput* inputOutput);
	virtual void show();
protected:
	virtual void paint();
	virtual void keyEvent(Teclado::Tecla tecla);
	virtual void start();
	virtual void finish();

	bool isMensagemSecundariaHabilitada();

private:
	Fachada* fachada;
	Opcao opcaoSelecionada;
	TelaSelecaoMensagem telaSelecaoMensagem;
	TelaSelecaoMensagem2 telaSelecaoMensagem2;
};

}
}
}
}

#endif /* TELAMENUMENSAGEM_H_ */
