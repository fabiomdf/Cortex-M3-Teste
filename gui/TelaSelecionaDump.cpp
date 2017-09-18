/*
 * TelaSelecionaDump.cpp
 *
 *  Created on: 10/12/2014
 *      Author: luciano.silva
 */

#include "TelaSelecionaDump.h"
#include <escort.util/Converter.h>
#include <escort.util/String.h>
#include "Resources.h"
#include "GUI.h"
#include "MessageDialog.h"

using escort::util::Converter;
using escort::util::String;


#define TEMPO_PISCAGEM_CURSOR \
	500

namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

TelaSelecionaDump::TelaSelecionaDump(Fachada* fachada, InputOutput* inputOutput) :
		Tela(inputOutput),
		fachada(fachada),
		telaColetaDump(fachada, inputOutput)
{}

void TelaSelecionaDump::paint()
{
	//Coloca o cursor no come�o do display
	this->inputOutput->display.setLinhaColuna(0,0);

	//Imprime no display a op��o selecionada
	switch (this->indiceAtual) {
		case CONTROLADOR:
			this->inputOutput->display.print(Resources::getTexto(Resources::CONTROLADOR)); //
			break;
		case TODOS_DISPOSITIVOS:
			this->inputOutput->display.print(Resources::getTexto(Resources::TODOS_DISPOSITIVOS)); // )
			break;
		case TODOS_PAINEIS:
			this->inputOutput->display.print(Resources::getTexto(Resources::TODOS_PAINEIS)); //
			break;
		//
		case PAINEL_X:
		default:

			if(this->indiceAtual <= this->indiceMaximo)
			{
				//Obs.: Texto "painel" deve sempre deixar espa�o para a numera��o na parte final dos 16 caracteres
				//Imprime as informa��es do painel
				this->inputOutput->display.print(Resources::getTexto(Resources::PAINEL));
				this->inputOutput->display.setLinhaColuna(14, 0);
				this->inputOutput->display.print(this->indiceAtual - (U8)TODOS_PAINEIS, 2);
			}
			else
			{
				this->indiceAtual = (U8) CONTROLADOR;
			}
			break;
	}
}

void TelaSelecionaDump::keyEvent(Teclado::Tecla tecla)
{
	//Verifica se a tecla "AJUSTE ESQUERDA"
	if(tecla & Teclado::TECLA_AJUSTE_ESQUERDA)
	{
		//Decrementa o �ndice (respeitando as condi��es determinadas)
		if(this->indiceAtual == CONTROLADOR){
			this->indiceAtual = this->indiceMaximo;
		}

		else {
			this->indiceAtual = (this->indiceAtual - 1);
		}

	}
	//Verifica se a tecla "AJUSTE DIREITA"
	if(tecla & Teclado::TECLA_AJUSTE_DIREITA)
	{
		if(this->indiceAtual == this->indiceMaximo){
			this->indiceAtual = (U8) CONTROLADOR;
		}
		else {
			this->indiceAtual = this->indiceAtual + 1;
		}

	}
	//Verifica se a tecla "OK"
	if(tecla & Teclado::TECLA_OK)
	{
		static char inputString[14];
		String::memset((U8*)inputString, 0, 14);
		String::strcpy(inputString, "ldx2/dump");

		MessageDialog::showInputStringDialog(Resources::getTexto(Resources::NOMEIE_PASTA), inputString + 5, this->inputOutput, 5);

		this->telaColetaDump.setOpcao(this->indiceAtual);
		this->telaColetaDump.setDestFolder(inputString);
		this->telaColetaDump.show();

		//Termina a exibi��o da tela de op��es
		this->finish();
		this->visible = false;
	}
}

void TelaSelecionaDump::start()
{
	//Limpa a tela
	this->inputOutput->display.clearScreen();

	//Se os arquivos de configura��o estiverem inconsistentes ent�o s� permite apagar o Controlador
	if(this->fachada->getStatusFuncionamento() & Fachada::ERRO_CONFIGURACAO_INCONSISTENTE){
		this->indiceMaximo = CONTROLADOR;
	}
	else{
		this->indiceMaximo = (U8)TODOS_PAINEIS + this->fachada->getQntPaineis();
	}
	this->indiceAtual = CONTROLADOR;
}

void TelaSelecionaDump::finish()
{}

}
}
}
}
