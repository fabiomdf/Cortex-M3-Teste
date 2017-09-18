/*
 * TelaAjustes.h
 *
 *  Created on: 30/05/2012
 *      Author: Gustavo
 */

#ifndef TELAAJUSTES_H_
#define TELAAJUSTES_H_

#include "InputOutput.h"
#include <freertos.kernel/FreeRTOSKernel/FreeRTOSKernel.h>
#include "../Fachada.h"
#include "Tela.h"
#include "TelaHoraSaida.h"
#include "TelaRelogio.h"
#include "TelaConfiguracoes.h"
#include "TelaRegiao.h"
#include "TelaOpcoesAvancadas.h"
#include "TelaBrilho.h"
#include "TelaSelecaoMotorista.h"

namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

class TelaOpcoes : public Tela {
private:
	typedef enum {
		HORA_SAIDA,
		RELOGIO,
		BRILHO_PAINEL,
		SELECAO_MOTORISTA,
		CONFIGURACOES,
		OPCOES_AVANCADAS
	} Opcao;

public:
	TelaOpcoes(Fachada* fachada, InputOutput* inputOutput);
protected:
	virtual void paint();
	virtual void keyEvent(Teclado::Tecla tecla);
	virtual void start();
	virtual void finish();

private:
	TelaHoraSaida telaHoraSaida;
	TelaRelogio telaRelogio;
	TelaBrilho telaBrilho;
	TelaSelecaoMotorista telaSelecaoMotorista;
	TelaConfiguracoes telaConfiguracoes;
	TelaOpcoesAvancadas telaOpcoesAvancadas;
	Opcao opcaoSelecionada;
};

}
}
}
}

#endif /* TELAAJUSTES_H_ */
