/*
 * TelaDestravaPaineis.cpp
 *
 *  Created on: 28/01/2016
 *      Author: Gustavo
 */

#include "TelaDestravaPaineis.h"
#include "Resources.h"
#include "MessageDialog.h"
#include <escort.util/Converter.h>

using escort::util::Converter;

#define PRIMEIRA_OPCAO \
	COLETAR_DUMP
#define ULTIMA_OPCAO \
	MODO_TESTE


namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

TelaDestravaPaineis::TelaDestravaPaineis(Fachada* fachada, InputOutput* inputOutput) :
		Tela(inputOutput),
		fachada(fachada)
{}

void TelaDestravaPaineis::paint()
{
}

void TelaDestravaPaineis::keyEvent(Teclado::Tecla tecla)
{
	//Verifica se a tecla "OK"
	if(tecla & Teclado::TECLA_OK)
	{
		//Solicita a senha ao usu�rio
		this->senha = MessageDialog::showPasswordDialog(
				Resources::getTexto(Resources::SENHA),
				this->inputOutput,
				8,
				Converter::BASE_16);
		//Avisa o usu�rio para aguardar
		MessageDialog::showMessageDialog(
				Resources::getTexto(Resources::AGUARDE),
				this->inputOutput,
				0);
		//Destrava os paineis com a senha obtida
		Fachada::Resultado res = this->fachada->destravarPaineis(this->paineisInfo, this->qntPaineisRede, (U8*)&this->senha);
		//Exibe o resultado da opera��o
		MessageDialog::showMessageDialog(res, this->inputOutput, 2000);
		//Termina a exibi��o da tela de destrava
		this->visible = false;
	}
}

void TelaDestravaPaineis::start()
{
	//Limpa a tela
	this->inputOutput->display.clearScreen();
	//Avisa o usu�rio para aguardar
	MessageDialog::showMessageDialog(
			Resources::getTexto(Resources::AGUARDE),
			this->inputOutput,
			0);
	//Lista os pain�is presentes na rede
	this->qntPaineisRede = this->fachada->listaPaineisRede(this->paineisInfo);
	//Exibe o ID de desbloqueio
	this->inputOutput->display.clearScreen();
	this->inputOutput->display.print(Resources::getTexto(Resources::ID_DESTRAVAMENTO));
	this->inputOutput->display.setLinhaColuna(0,1);
	this->inputOutput->display.print(this->fachada->getIdDestravaPaineis(this->paineisInfo, this->qntPaineisRede), Converter::BASE_16);
	//Aumenta o tempo de timeout da tela
	this->teclaTime = 30000;
}

void TelaDestravaPaineis::finish()
{
}

}
}
}
}
