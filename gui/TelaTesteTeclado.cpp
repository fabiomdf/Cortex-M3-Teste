/*
 * TelaTesteTeclado.cpp
 *
 *  Created on: 30/05/2012
 *      Author: Gustavo
 */

#include "TelaTesteTeclado.h"
#include "MessageDialog.h"
#include "Resources.h"
#include "GUI.h"



namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

TelaTesteTeclado::TelaTesteTeclado(Fachada* fachada, InputOutput* inputOutput) :
		Tela(inputOutput)
{}

void TelaTesteTeclado::paint()
{
}

void TelaTesteTeclado::keyEvent(Teclado::Tecla tecla)
{
	if(tecla & Teclado::TECLA_ROTEIRO_ESQUERDA){
		this->inputOutput->display.setLinhaColuna(0,1);
		this->inputOutput->display.print(Resources::getTexto(Resources::TECLA_ROT_ESQ));
		estado = ESPERANDO_TECLA_ROT_DIR;
	}
	else if(tecla & Teclado::TECLA_ROTEIRO_DIREITA){
		this->inputOutput->display.setLinhaColuna(0,1);
		this->inputOutput->display.print(Resources::getTexto(Resources::TECLA_ROT_DIR));
		estado = ESPERANDO_TECLA_IDA_VOLTA;
	}
	else if(tecla & Teclado::TECLA_IDA_VOLTA){
		this->inputOutput->display.setLinhaColuna(0,1);
		this->inputOutput->display.print(Resources::getTexto(Resources::TECLA_IDA_VOLTA));
		estado = ESPERANDO_TECLA_ALTERNA;
	}
	else if(tecla & Teclado::TECLA_ALTERNA){
		this->inputOutput->display.setLinhaColuna(0,1);
		this->inputOutput->display.print(Resources::getTexto(Resources::TECLA_ALTERNA));
		estado = ESPERANDO_TECLA_MSG_ESQ;
	}
	else if(tecla & Teclado::TECLA_MENSAGEM_ESQUERDA){
		this->inputOutput->display.setLinhaColuna(0,1);
		this->inputOutput->display.print(Resources::getTexto(Resources::TECLA_MSG_ESQ));
		estado = ESPERANDO_TECLA_MSG_DIR;
	}
	else if(tecla & Teclado::TECLA_MENSAGEM_DIREITA){
		this->inputOutput->display.setLinhaColuna(0,1);
		this->inputOutput->display.print(Resources::getTexto(Resources::TECLA_MSG_DIR));
		estado = ESPERANDO_TECLA_SELEC_PAINEL;
	}
	else if(tecla & Teclado::TECLA_SELECIONA_PAINEL){
		this->inputOutput->display.setLinhaColuna(0,1);
		this->inputOutput->display.print(Resources::getTexto(Resources::TECLA_SELEC_PAINEL));
		estado = ESPERANDO_TECLA_AJUSTES_ESQ;
	}
	if(tecla & Teclado::TECLA_AJUSTE_ESQUERDA){
		this->inputOutput->display.setLinhaColuna(0,1);
		this->inputOutput->display.print(Resources::getTexto(Resources::TECLA_AJUSTES_ESQ));
		estado = ESPERANDO_TECLA_AJUSTES_DIR;
	}
	if(tecla & Teclado::TECLA_AJUSTE_DIREITA){
		this->inputOutput->display.setLinhaColuna(0,1);
		this->inputOutput->display.print(Resources::getTexto(Resources::TECLA_AJUSTES_DIR));
		estado = ESPERANDO_TECLA_OK;
	}
	if(tecla & Teclado::TECLA_OK){
		this->inputOutput->display.setLinhaColuna(0,1);
		MessageDialog::showMessageDialog(Resources::getTexto(Resources::SUCESSO), this->inputOutput, 2000);
		this->visible = false;
	}
}

void TelaTesteTeclado::start()
{
	//Limpa a tela
	this->inputOutput->display.clearScreen();
	//Imprime as informa��es fixas da tela
	this->inputOutput->display.print(Resources::getTexto(Resources::TESTE_TECLADO));
	this->inputOutput->display.println();
	this->inputOutput->display.print(Resources::getTexto(Resources::COMECE_TESTE));

	this->estado = ESPERANDO_TECLA_ROT_ESQ;
}

void TelaTesteTeclado::finish()
{
}

}
}
}
}
