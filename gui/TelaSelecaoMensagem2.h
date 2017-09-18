/*
 * TelaSelecaoMensagem.h
 *
 *  Created on: 30/05/2012
 *      Author: Gustavo
 */

#ifndef TELASELECAOMENSAGEM2_H_
#define TELASELECAOMENSAGEM2_H_

#include "TelaSelecaoMensagem.h"

namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

class TelaSelecaoMensagem2 : public TelaSelecaoMensagem {
public:
	TelaSelecaoMensagem2(
			Fachada* fachada,
			InputOutput* inputOutput);

private:
	virtual void start();
	virtual void finish();
};

}
}
}
}

#endif /* TELASELECAOMENSAGEM2_H_ */
