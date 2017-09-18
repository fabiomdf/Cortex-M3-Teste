/*
 * GUI.h
 *
 *  Created on: 30/05/2012
 *      Author: Gustavo
 */

#ifndef GUI_H_
#define GUI_H_

#include "../Fachada.h"
#include "InputOutput.h"
#include "TelaSelecaoRoteiro.h"
#include "TelaMenuMensagem.h"
#include "TelaIdaVolta.h"
#include "TelaOpcoes.h"
#include "TelaAlternarCom.h"
#include "TelaNovaConfiguracao.h"
#include "TelaSelecaoPainel.h"
#include <escort.hal\IGPIOPin\IGPIOPin.h>

namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

class GUI  {
public:
	GUI(
			escort::service::NandFFS* sistemaArquivo,
			Fachada* fachada,
			escort::driver::IKeypad* teclado,
			escort::driver::ILCDDisplay* display,
			escort::service::USBHost* usbHost,
			escort::hal::IGPIOPin* sinalEmergencia);

	virtual void init();
	void task();
	void progressBarTask();
	void start();
	static void setEnableProgressBar(bool enable);
	static void incrementProgressBar();
	static void resetProgressBar();

private:
	void tratarTeclas();
	void printpontosVersion();
	void printIdleScreen1();
	void printIdleScreen2();
	void loadNewConfig();
	void qntdPaineisRede();
	void printTelaInicial();
	void waitStart();

public:
	InputOutput inputOutput;
private:
	escort::hal::IGPIOPin* sinalEmergencia;
	TelaSelecaoRoteiro telaRoteiros;
	TelaMenuMensagem telaMensagens;
	TelaSelecaoPainel telaPaineis;
	TelaIdaVolta telaSentido;
	TelaAlternarCom telaAlternarCom;
	TelaOpcoes telaOpcoes;
	TelaNovaConfiguracao telaNovaConfiguracao;
	Fachada* fachada;
	escort::service::NandFFS* sistemaArquivo;
	xSemaphoreHandle semaforo;
	static bool enableProgressBar;
	static U8 progressBarValue;
};

}
}
}
}

#endif /* GUI_H_ */
