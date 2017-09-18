/*
 * TelaOpcoes.cpp
 *
 *  Created on: 30/05/2012
 *      Author: Gustavo
 */

#include "TelaOpcoes.h"
#include "Resources.h"

#define PRIMEIRA_OPCAO \
	HORA_SAIDA
#define ULTIMA_OPCAO \
	OPCOES_AVANCADAS


namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

TelaOpcoes::TelaOpcoes(Fachada* fachada, InputOutput* inputOutput) :
		Tela(inputOutput),
		telaHoraSaida(fachada, inputOutput),
		telaRelogio(fachada, inputOutput),
		telaBrilho(fachada, inputOutput),
		telaSelecaoMotorista(fachada, inputOutput),
		telaConfiguracoes(fachada, inputOutput),
		telaOpcoesAvancadas(fachada, inputOutput)
{}

void TelaOpcoes::paint()
{
	//Coloca o cursor no come�o do display
	this->inputOutput->display.setLinhaColuna(0,0);

	//Imprime no display a op��o selecionada
	switch (this->opcaoSelecionada) {
		case HORA_SAIDA:
			this->inputOutput->display.print(Resources::getTexto(Resources::AJUSTAR_PARTIDA));
			break;
		case RELOGIO:
			this->inputOutput->display.print(Resources::getTexto(Resources::AJUSTAR_RELOGIO));
			break;
		case BRILHO_PAINEL:
			this->inputOutput->display.print(Resources::getTexto(Resources::AJUSTAR_BRILHO));
			break;
		case SELECAO_MOTORISTA:
			this->inputOutput->display.print(Resources::getTexto(Resources::SELECIONAR_MOTORISTA));
			break;
		case CONFIGURACOES:
			this->inputOutput->display.print(Resources::getTexto(Resources::CONFIGURACOES));
			break;
		case OPCOES_AVANCADAS:
			this->inputOutput->display.print(Resources::getTexto(Resources::OPCOES_AVANCADAS));
			break;
	}
}

void TelaOpcoes::keyEvent(Teclado::Tecla tecla)
{
	//Verifica se a tecla "AJUSTE ESQUERDA"
	if(tecla & Teclado::TECLA_AJUSTE_ESQUERDA)
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
	//Verifica se a tecla "AJUSTE DIREITA"
	if(tecla & Teclado::TECLA_AJUSTE_DIREITA)
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
			case HORA_SAIDA:
				this->telaHoraSaida.show();
				break;
			case RELOGIO:
				this->telaRelogio.show();
				break;
			case BRILHO_PAINEL:
				this->telaBrilho.show();
				break;
			case SELECAO_MOTORISTA:
				this->telaSelecaoMotorista.show();
				break;
			case CONFIGURACOES:
				this->telaConfiguracoes.show();
				break;
			case OPCOES_AVANCADAS:
				this->telaOpcoesAvancadas.show();
				break;
		}

		//Termina a exibi��o da tela de op��es
		this->finish();
		this->visible = false;
	}
}

void TelaOpcoes::start()
{
	//Limpa a tela
	this->inputOutput->display.clearScreen();

	//Seleciona a primeira op��o
	this->opcaoSelecionada = PRIMEIRA_OPCAO;
}

void TelaOpcoes::finish()
{
}

}
}
}
}
