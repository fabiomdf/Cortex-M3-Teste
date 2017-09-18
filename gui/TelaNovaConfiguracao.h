/*
 * TelaNovaConfiguracao.h
 *
 *  Created on: 30/05/2012
 *      Author: Gustavo
 */

#ifndef TELANOVACONFIGURACAO_H_
#define TELANOVACONFIGURACAO_H_

#include "../Fachada.h"
#include "Tela.h"

namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

class TelaNovaConfiguracao : public Tela {
public:
	TelaNovaConfiguracao(Fachada* fachada, InputOutput* inputOutput);
private:
	virtual void paint();
	virtual void keyEvent(Teclado::Tecla tecla);
	virtual void usbEvent(escort::service::USBHost::StatusUSB statusUSB);
	virtual void start();
	virtual void finish();

private:
	Fachada* fachada;
	escort::service::FatFs::Directory ldx2Folder;
	U32 arquivoSelecionado;
	char pathArquivoSelecionado[20];
};

}
}
}
}

#endif /* TELANOVACONFIGURACAO_H_ */
