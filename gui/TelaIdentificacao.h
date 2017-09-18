/*
 * TelaIdentificacao.h
 *
 *  Created on: 30/05/2012
 *      Author: Gustavo
 */

#ifndef TELAIDENTIFICACAO_H_
#define TELAIDENTIFICACAO_H_

#include "InputOutput.h"
#include "../Fachada.h"
#include "Tela.h"

namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

class TelaIdentificacao : public Tela {
public:
	TelaIdentificacao(Fachada* fachada, InputOutput* inputOutput);

protected:
	virtual void paint();
	virtual void keyEvent(Teclado::Tecla tecla);
	virtual void start();
	virtual void finish();
	void finalizarProcedimento();
	bool verificarPermissao();

private:
	Fachada* fachada;
	Fachada::InfoPainelListado* paineisListados;
	U8 painelListadoSelecionado;
	U8 indicePainel;
	bool automatico;
	U8 qntPaineis;
	U16 countAltern;
};

}
}
}
}

#endif /* TELAIDENTIFICACAO_H_ */
