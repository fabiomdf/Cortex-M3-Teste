/*
 * TelaDestravaPaineis.h
 *
 *  Created on: 28/01/2016
 *      Author: Gustavo
 */

#ifndef TELADESTRAVAPAINEIS_H_
#define TELADESTRAVAPAINEIS_H_

#include "InputOutput.h"
#include "../Fachada.h"
#include "Tela.h"

namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

class TelaDestravaPaineis : public Tela {
public:
	TelaDestravaPaineis(Fachada* fachada, InputOutput* inputOutput);
protected:
	virtual void paint();
	virtual void keyEvent(Teclado::Tecla tecla);
	virtual void start();
	virtual void finish();

private:
	Fachada* fachada;
	U32 senha;
	Fachada::InfoPainelListado paineisInfo[16];
	U8 qntPaineisRede;
};

}
}
}
}

#endif /* TELADESTRAVAPAINEIS_H_ */
