/*
 * Teclado.h
 *
 *  Created on: 29/05/2012
 *      Author: Gustavo
 */

#ifndef TECLADO_H_
#define TECLADO_H_

#include <freertos.kernel/FreeRTOSKernel/FreeRTOSKernel.h>
#include <escort.driver/IKeypad/IKeypad.h>

namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

class Teclado {
public:
	typedef enum {
		TECLA_NENHUMA = 			0,
		TECLA_OK = 					(1 << 0),
		TECLA_ROTEIRO_DIREITA = 	(1 << 2),
		TECLA_ROTEIRO_ESQUERDA = 	(1 << 4),
		TECLA_ALTERNA = 			(1 << 5),
		TECLA_MENSAGEM_ESQUERDA = 	(1 << 6),
		TECLA_AJUSTE_DIREITA = 		(1 << 7),
		TECLA_MENSAGEM_DIREITA = 	(1 << 8),
		TECLA_SELECIONA_PAINEL = 	(1 << 9),
		TECLA_IDA_VOLTA = 			(1 << 13),
		TECLA_AJUSTE_ESQUERDA = 	(1 << 15)
	} Tecla;

public:
	Teclado(escort::driver::IKeypad* keypad);
	void init();
	Tecla getEventoTeclado();
	void clearEventosTeclado();
	bool hasEventoTeclado();
	void task();
private:
	Tecla getTeclasPressionadas();

private:
	escort::driver::IKeypad* keypad;
	xQueueHandle buffer;
	portTickType timerTeclado;
};

}
}
}
}

#endif /* TECLADO_H_ */
