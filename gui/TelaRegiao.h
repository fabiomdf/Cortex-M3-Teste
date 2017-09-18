/*
 * TelaRegiao.h
 *
 *  Created on: 30/05/2012
 *      Author: Gustavo
 */

#ifndef TELAREGIAO_H_
#define TELAREGIAO_H_

#include "InputOutput.h"
#include "../Fachada.h"
#include "Tela.h"

namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

class TelaRegiao : public Tela {
public:
	TelaRegiao(Fachada* fachada, InputOutput* inputOutput);
protected:
	virtual void paint();
	virtual void keyEvent(Teclado::Tecla tecla);
	virtual void start();
	virtual void finish();

private:
	Fachada* fachada;
	U8 qntRegioes;
	U8 indiceRegiao;
	char nomeRegiao[16];
	bool modificado;
};

}
}
}
}

#endif /* TELAREGIAO_H_ */
