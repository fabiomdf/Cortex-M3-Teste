/*
 * TelaOpcoesAvancadas.h
 *
 *  Created on: 30/05/2012
 *      Author: Gustavo
 */

#ifndef TELAOPCOESAVANCADAS_H_
#define TELAOPCOESAVANCADAS_H_

#include "InputOutput.h"
#include <freertos.kernel/FreeRTOSKernel/FreeRTOSKernel.h>
#include "../Fachada.h"
#include "Tela.h"
#include "TelaSelecionaDump.h"
#include "TelaFormatarPendrive.h"
#include "TelaColetaLog.h"
#include "TelaApagarArquivos.h"
#include "TelaAcenderPaineis.h"
#include "TelaIdentificacao.h"
#include "TelaDestravaPaineis.h"
#include "TelaConfigFabrica.h"
#include "TelaModoTeste.h"
#include "TelaTesteTeclado.h"

namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

class TelaOpcoesAvancadas : public Tela {
private:
	typedef enum {
		COLETAR_DUMP,
//		COLETAR_LOG,
		FORMATAR_PENDRIVE,
		APAGAR_CONFIG,
		ACENDER_PAINEIS,
		EMPARELHAR_PAINEIS,
		DESTRAVAR_PAINEIS,
		CONFIG_FABRICA,
		TESTE_TECLADO,
		APP_MODO_DEMONSTRACAO,
		MODO_TESTE
	} Opcao;

public:
	TelaOpcoesAvancadas(Fachada* fachada, InputOutput* inputOutput);
protected:
	virtual void paint();
	virtual void keyEvent(Teclado::Tecla tecla);
	virtual void start();
	virtual void finish();

private:
	Fachada* fachada;
	TelaSelecionaDump telaSelecionaDump;
//	TelaColetaLog telaColetaLog;
	TelaFormatarPendrive telaFormatarPendrive;
	TelaApagarArquivos telaApagarArquivos;
	TelaAcenderPaineis telaAcenderPaineis;
	TelaIdentificacao telaIdentificacao;
	TelaDestravaPaineis telaDestravaPaineis;
	TelaConfigFabrica telaConfigFabrica;
	TelaTesteTeclado telaTesteTeclado;
	TelaModoTeste telaModoTeste;
	Opcao opcaoSelecionada;
};

}
}
}
}

#endif /* TELAOPCOESAVANCADAS_H_ */
