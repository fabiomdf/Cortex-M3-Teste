/*
 * Teclado.cpp
 *
 *  Created on: 29/05/2012
 *      Author: Gustavo
 */

#include "Teclado.h"
#include <config.h>




#define TEMPO_TECLA_SEGURADA \
	1000
#define TEMPO_REPETICAO_TECLA \
	80



namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

void Teclado_task(void* arg)
{
	((Teclado*) arg)->task();
}

Teclado::Teclado(escort::driver::IKeypad* keypad) :
		keypad(keypad)
{}

void Teclado::init()
{
	//Inicializa o buffer
	this->buffer = xQueueCreate( trf_pontos12_application_Controlador_tecladoBufferLength, sizeof(Tecla));

	//Inicia tarefa
	xTaskCreate(
			(pdTASK_CODE)Teclado_task,
			(const signed char*)"teclado",
			trf_pontos12_application_Controlador_tecladoTaskStackSize,
			(void*)this,
			trf_pontos12_application_Controlador_tecladoTaskPriority,
			0);
}

Teclado::Tecla Teclado::getTeclasPressionadas()
{
	Tecla keyFlags = TECLA_NENHUMA;

	//Escaneia o teclado
	bool* keyMap = this->keypad->scan();

	for (U32 i = 0; i < this->keypad->getKeyMapSize(); ++i) {
		//Se a tecla 'i' estiver pressionada registra em 'keyFlags'
		if(keyMap[i] == true)
		{
			keyFlags = (Tecla)(keyFlags | (1 << i));
		}
	}

	return keyFlags;
}

Teclado::Tecla Teclado::getEventoTeclado()
{
	Tecla tecla = TECLA_NENHUMA;
	xQueueReceive(this->buffer, &tecla, 0);

	return tecla;
}

void Teclado::clearEventosTeclado()
{
	while(uxQueueMessagesWaiting(this->buffer)){
		Tecla tecla;
		xQueueReceive(this->buffer, &tecla, 0);
	}
}

bool Teclado::hasEventoTeclado()
{
	return uxQueueMessagesWaiting(this->buffer) != 0;
}

void Teclado::task()
{
	while(true)
	{
		static enum {
			TECLA_OFF,
			TECLA_ON
		} estado = TECLA_OFF;

		//L� o status atual das teclas
		Tecla tecla = this->getTeclasPressionadas();

		switch(estado)
		{
			case TECLA_OFF:
				//Verifica se alguma tecla foi apertada
				if(tecla != TECLA_NENHUMA)
				{
					//Insere o evento de tecla no buffer
					xQueueSend(this->buffer, &tecla, 0);
					//Carrega o timer com tempo de tecla segurada
					this->timerTeclado = xTaskGetTickCount() + TEMPO_TECLA_SEGURADA;
					estado = TECLA_ON;
				}
				break;

			case TECLA_ON:
				//Verifica se a tecla foi liberada
				if(tecla == TECLA_NENHUMA)
				{
					this->timerTeclado = 0;
					estado = TECLA_OFF;
				}
				else
				{
					//Espera o tempo para gerar o evento
					if(this->timerTeclado < xTaskGetTickCount())
					{
						//Insere o evento de tecla no buffer
						xQueueSend(this->buffer, &tecla, 0);
						//Carrega o timer com tempo de repeti��o de tecla
						this->timerTeclado = xTaskGetTickCount() + TEMPO_REPETICAO_TECLA;
					}
				}
				break;

			default:
				estado = TECLA_OFF;
				break;
		}

		vTaskDelay(5);
	}
}

}
}
}
}
