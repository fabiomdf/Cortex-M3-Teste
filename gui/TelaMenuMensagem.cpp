/*
 * TelaMenuMensagem.cpp
 *
 *  Created on: 30/05/2012
 *      Author: Gustavo
 */

#include "TelaMenuMensagem.h"
#include "Resources.h"

#define PRIMEIRA_OPCAO \
	MENSAGEM_PRINCIPAL
#define ULTIMA_OPCAO \
	MENSAGEM_SECUNDARIA


namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

TelaMenuMensagem::TelaMenuMensagem(Fachada* fachada, InputOutput* inputOutput) :
		Tela(inputOutput),
		fachada(fachada),
		telaSelecaoMensagem(fachada, inputOutput),
		telaSelecaoMensagem2(fachada, inputOutput)
{}

void TelaMenuMensagem::show()
{
	//Se a mensagem secund�ria est� habilitada para esse painel ent�o exibe
	//o menu para o usu�rio escolher entre "Mensagem principal" e "Mensagem secund�ria"
	//qual �ndice selecionar.
	//Caso contr�rio exibe direto a tela de sele��o da mensagem principal.
	if(this->fachada->isMensagemSecundariaHabilitada(this->fachada->getPainelSelecionado())){
		Tela::show();
	}
	else{
		this->telaSelecaoMensagem.show();
	}
}

void TelaMenuMensagem::paint()
{
	//Coloca o cursor no come�o do display
	this->inputOutput->display.setLinhaColuna(0,0);

	//Imprime no display a op��o selecionada
	switch (this->opcaoSelecionada) {
		case MENSAGEM_PRINCIPAL:
			this->inputOutput->display.print(Resources::getTexto(Resources::MENSAGEM_PRINCIPAL));
			break;
		case MENSAGEM_SECUNDARIA:
			this->inputOutput->display.print(Resources::getTexto(Resources::MENSAGEM_SECUNDARIA));
			break;
	}
}

void TelaMenuMensagem::keyEvent(Teclado::Tecla tecla)
{
	//Verifica se a tecla "MENSAGEM ESQUERDA"
	if(tecla & Teclado::TECLA_MENSAGEM_ESQUERDA)
	{
		//Se o �ndice est� indicando a primeira op��o ent�o vai para a �ltima
		if(this->opcaoSelecionada == PRIMEIRA_OPCAO)
		{
			this->opcaoSelecionada = ULTIMA_OPCAO;
		}
		//Caso contr�rio decrementa o �ndice da op��o selecionado
		else
		{
			this->opcaoSelecionada = (Opcao)((U8)this->opcaoSelecionada - 1);
		}
	}
	//Verifica se a tecla "MENSAGEM DIREITA"
	if(tecla & Teclado::TECLA_MENSAGEM_DIREITA)
	{
		//Se o �ndice est� indicando a �ltima op��o ent�o vai para a primeira
		if(this->opcaoSelecionada == ULTIMA_OPCAO)
		{
			this->opcaoSelecionada = PRIMEIRA_OPCAO;
		}
		//Caso contr�rio decrementa o �ndice do roteiro selecionado
		else
		{
			this->opcaoSelecionada = (Opcao)((U8)this->opcaoSelecionada + 1);
		}
	}
	//Verifica se a tecla "OK"
	if(tecla & Teclado::TECLA_OK)
	{
		//Exibe a tela da op��o selecionada
		switch (this->opcaoSelecionada) {
			case MENSAGEM_PRINCIPAL:
				this->telaSelecaoMensagem.show();
				break;
			case MENSAGEM_SECUNDARIA:
				this->telaSelecaoMensagem2.show();
				break;
		}

		//Termina a exibi��o da tela de op��es
		this->finish();
		this->visible = false;
	}
}

void TelaMenuMensagem::start()
{
	//Limpa a tela
	this->inputOutput->display.clearScreen();

	//Seleciona a primeira op��o
	this->opcaoSelecionada = PRIMEIRA_OPCAO;
}

void TelaMenuMensagem::finish()
{
}

}
}
}
}
