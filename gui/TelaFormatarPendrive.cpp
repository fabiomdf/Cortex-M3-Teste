/*
 * TelaFormatarPendrive.cpp
 *
 *  Created on: 30/05/2012
 *      Author: Gustavo
 */

#include "TelaFormatarPendrive.h"
#include <escort.util/Converter.h>
#include <escort.util/String.h>
#include "Resources.h"

using escort::util::Converter;
using escort::util::String;


#define TEMPO_PISCAGEM_CURSOR \
	500


namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

TelaFormatarPendrive::TelaFormatarPendrive(Fachada* fachada, InputOutput* inputOutput) :
		Tela(inputOutput),
		fachada(fachada)
{}

void TelaFormatarPendrive::show()
{
	enum {
		RECONHECENDO_USB,
		CONFIRMANDO_FORMATACAO,
		FORMATANDO_PENDRIVE,
		ESPERANDO_PENDRIVE,
		PENDRIVE_INCOMPATIVEL,
	}estado = ESPERANDO_PENDRIVE;

	this->visible = true;
	this->start();

	while(this->visible)
	{
		switch (estado) {
			case RECONHECENDO_USB:
				// Indica ao usu�rio que est� reconhecendo disp�sitivo
				this->inputOutput->display.clearScreen();
				this->inputOutput->display.print(Resources::getTexto(Resources::RECONHECENDO_USB));
				//Se o dispositivo foi reconhecido com sucesso come�a a formata��o
				if(this->inputOutput->usbHost->getStatusUSB() == escort::service::USBHost::USB_CONNECTED){
					//Indica ao usu�rio para inserir um pendrive
					this->inputOutput->display.clearScreen();
					this->inputOutput->display.print(Resources::getTexto(Resources::DESEJA_FORMATAR));
					estado = CONFIRMANDO_FORMATACAO;
				}
				//Se o dispositivo for desconectado ent�o sai da tela
				else if(this->inputOutput->usbHost->getStatusUSB() == escort::service::USBHost::USB_DISCONNECTED) {
					this->visible = (false);
				}
				break;
			case CONFIRMANDO_FORMATACAO:
			{
				//Verifica se alguma tecla for pressionada
				Teclado::Tecla tecla = this->inputOutput->teclado.getEventoTeclado();
				if(tecla)
				{
					//Se a tecla OK foi pressionada ent�o prossegue
					if(tecla & Teclado::TECLA_OK){
						estado = FORMATANDO_PENDRIVE;
					} else {
						this->visible = (false);
					}
				}
				else if(this->inputOutput->usbHost->getStatusUSB() == escort::service::USBHost::USB_DISCONNECTED){
					this->visible = (false);
				}
			}
			break;
			case FORMATANDO_PENDRIVE:
				this->inputOutput->display.clearScreen();
				this->inputOutput->display.print(Resources::getTexto(Resources::AGUARDE));
				//Formata pendrive
				if(this->fachada->formatarPendrive(&this->inputOutput->usbHost->fileSystem) == true){
					this->inputOutput->display.clearScreen();
					this->inputOutput->display.print(Resources::getTexto(Resources::SUCESSO));
				}
				else{
					this->inputOutput->display.clearScreen();
					this->inputOutput->display.print(Resources::getTexto(Resources::FALHA));
				}
				vTaskDelay(2000);
				//Finaliza a tela
				this->visible = (false);
				break;
			case ESPERANDO_PENDRIVE:
				//Indica ao usu�rio para inserir um pendrive
				this->inputOutput->display.clearScreen();
				this->inputOutput->display.print(Resources::getTexto(Resources::COLOQUE_PENDRIVE));
				//Se um dispositivo USB for plugado ent�o tenta reconhecer o dispositivo
				if(this->inputOutput->usbHost->getStatusUSB() == escort::service::USBHost::USB_PLUGGED)
				{
					estado = RECONHECENDO_USB;
				}
				else
				{
					//Se alguma tecla for pressionada ent�o sai da tela
					Teclado::Tecla tecla = this->inputOutput->teclado.getEventoTeclado();
					if(tecla)
					{
						this->visible = (false);
					}
				}
				break;
			default:
				estado = ESPERANDO_PENDRIVE;
				break;
		}

		vTaskDelay(200);
	}

	//Limpa os eventos que tenham sido gerados durante a exibi��o da tela
	this->inputOutput->teclado.clearEventosTeclado();
}

void TelaFormatarPendrive::paint()
{}

void TelaFormatarPendrive::keyEvent(Teclado::Tecla tecla)
{}

void TelaFormatarPendrive::start()
{
	//Indica ao usu�rio para inserir um pendrive
	this->inputOutput->display.clearScreen();
	this->inputOutput->display.print(Resources::getTexto(Resources::COLOQUE_PENDRIVE));
}

void TelaFormatarPendrive::finish()
{}

}
}
}
}
