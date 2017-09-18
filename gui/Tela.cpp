/*
 * Tela.cpp
 *
 *  Created on: 30/05/2012
 *      Author: Gustavo
 */

#include "Tela.h"


namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

Tela::Tela(InputOutput* inputOutput) :
		inputOutput(inputOutput),
		teclaTime(5000)
{}

void Tela::show()
{
	//Torna a tela vis�vel
	this->visible = true;
	//Carrega o status atual da USB
	this->oldStatusUSB = this->inputOutput->usbHost->getStatusUSB();
	//Inicia a exibi��o da tela
	this->start();
	//Carrega o timeout do teclado
	this->timeoutTecla = xTaskGetTickCount() + teclaTime;

	while(this->visible)
	{
		//Pinta a tela
		this->paint();
		//Trata os eventos do teclado
		this->handleKeys();
		//Trata os eventos de USB host
		this->handleUSB();

		vTaskDelay(40);
	}
}

void Tela::handleKeys()
{
	//Verifica se existe um evento do teclado para ser tratado
	Teclado::Tecla tecla = inputOutput->teclado.getEventoTeclado();
	if(tecla)
	{
		//Recarrega o timeout
		this->timeoutTecla = xTaskGetTickCount() + teclaTime;
		//Trata o evento do teclado
		this->keyEvent(tecla);
	}

	//Verifica se expirou o timeout
	if(this->visible && this->timeoutTecla < xTaskGetTickCount())
	{
		//Finaliza a exibi��o da tela
		this->finish();
		this->visible = false;
	}
}

void Tela::handleUSB()
{
	escort::service::USBHost::StatusUSB newStatus = this->inputOutput->usbHost->getStatusUSB();

	//Se o novo status do USB host � diferente do status anterior ent�o gera um evento
	if(newStatus != oldStatusUSB)
	{
		oldStatusUSB = newStatus;
		this->usbEvent(newStatus);
	}
}

void Tela::keyEvent(Teclado::Tecla tecla)
{}

void Tela::usbEvent(escort::service::USBHost::StatusUSB statusUSB)
{}

}
}
}
}
