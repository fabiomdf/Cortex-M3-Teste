/*
 * TelaOpcoesAvancadas.cpp
 *
 *  Created on: 30/05/2012
 *      Author: Gustavo
 */

#include "TelaOpcoesAvancadas.h"
#include "Resources.h"
#include "MessageDialog.h"

#define PRIMEIRA_OPCAO \
	COLETAR_DUMP
#define ULTIMA_OPCAO \
	MODO_TESTE


namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

TelaOpcoesAvancadas::TelaOpcoesAvancadas(Fachada* fachada, InputOutput* inputOutput) :
		Tela(inputOutput),
		fachada(fachada),
		telaSelecionaDump(fachada, inputOutput),
//		telaColetaLog(fachada, inputOutput),
		telaFormatarPendrive(fachada, inputOutput),
		telaApagarArquivos(fachada, inputOutput),
		telaAcenderPaineis(fachada, inputOutput),
		telaIdentificacao(fachada, inputOutput),
		telaDestravaPaineis(fachada, inputOutput),
		telaConfigFabrica(fachada, inputOutput),
		telaTesteTeclado(fachada, inputOutput),
		telaModoTeste(fachada, inputOutput)
{}

void TelaOpcoesAvancadas::paint()
{
	//Coloca o cursor no come�o do display
	this->inputOutput->display.setLinhaColuna(0,0);

	//Imprime no display a op��o selecionada
	switch (this->opcaoSelecionada) {
		case COLETAR_DUMP:
			this->inputOutput->display.print(Resources::getTexto(Resources::COLETAR_DUMP));
			break;
//		case COLETAR_LOG:
//			this->inputOutput->display.print(Resources::getTexto(Resources::COLETAR_LOG));
//			break;
		case FORMATAR_PENDRIVE:
			this->inputOutput->display.print(Resources::getTexto(Resources::FORMAT_PENDRIVE));
			break;
		case APAGAR_CONFIG:
			this->inputOutput->display.print(Resources::getTexto(Resources::APAGAR_ARQUIVOS));
			break;
		case ACENDER_PAINEIS:
			this->inputOutput->display.print(Resources::getTexto(Resources::ACENDER_PAINEIS));
			break;
		case EMPARELHAR_PAINEIS:
			this->inputOutput->display.print(Resources::getTexto(Resources::IDENTIF_PAINEIS));
			break;
		case DESTRAVAR_PAINEIS:
			this->inputOutput->display.print(Resources::getTexto(Resources::DESTRAVAR_PAINEIS));
			break;
		case CONFIG_FABRICA:
			this->inputOutput->display.print(Resources::getTexto(Resources::CONFIG_FABRICA));
			break;
		case TESTE_TECLADO:
			this->inputOutput->display.print(Resources::getTexto(Resources::TESTE_TECLADO));
			break;
		case APP_MODO_DEMONSTRACAO:
			this->inputOutput->display.print(Resources::getTexto(Resources::APP_MODO_DEMONSTRACAO));
			break;
		case MODO_TESTE:
			this->inputOutput->display.print(Resources::getTexto(Resources::MODO_TESTE));
			break;
	}
}

void TelaOpcoesAvancadas::keyEvent(Teclado::Tecla tecla)
{
	//Verifica se a tecla "AJUSTE ESQUERDA"
	if(tecla & Teclado::TECLA_AJUSTE_ESQUERDA)
	{
		//Se o �ndice est� indicando a primeira op��o ent�o vai para a �ltima
		if(this->opcaoSelecionada == PRIMEIRA_OPCAO)
		{
			this->opcaoSelecionada = ULTIMA_OPCAO;
		}
		//Nem sempre haver� APP na configura��o
		else if((this->opcaoSelecionada == MODO_TESTE) && this->fachada->isAPPHabilitado())
		{
			this->opcaoSelecionada = APP_MODO_DEMONSTRACAO;
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
		//Nem sempre haver� APP na configura��o
		else if((this->opcaoSelecionada == TESTE_TECLADO) && this->fachada->isAPPHabilitado())
		{
			this->opcaoSelecionada = APP_MODO_DEMONSTRACAO;
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
			case COLETAR_DUMP:
				this->telaSelecionaDump.show();
				break;
//			case COLETAR_LOG:
//				this->telaColetaLog.show();
//				break;
			case FORMATAR_PENDRIVE:
				this->telaFormatarPendrive.show();
				break;
			case APAGAR_CONFIG:
				this->telaApagarArquivos.show();
				break;
			case ACENDER_PAINEIS:
				this->telaAcenderPaineis.show();
				break;
			case EMPARELHAR_PAINEIS:
				this->telaIdentificacao.show();
				break;
			case DESTRAVAR_PAINEIS:
				this->telaDestravaPaineis.show();
				break;
			case CONFIG_FABRICA:
				this->telaConfigFabrica.show();
				break;
			case TESTE_TECLADO:
				this->telaTesteTeclado.show();
				break;
			case APP_MODO_DEMONSTRACAO:
				if(MessageDialog::showConfirmationDialog(Resources::getTexto(Resources::APP_MODO_DEMONSTRACAO), this->inputOutput, false, 10) == true){
					this->fachada->setAPPModoDemonstracao(true);
				}
				else
				{
					this->fachada->setAPPModoDemonstracao(false);
				}
				break;
			case MODO_TESTE:
				this->telaModoTeste.show();
				break;
		}

		//Termina a exibi��o da tela de op��es
		this->finish();
		this->visible = false;
	}
}

void TelaOpcoesAvancadas::start()
{
	//Limpa a tela
	this->inputOutput->display.clearScreen();

	//Seleciona a primeira op��o
	this->opcaoSelecionada = PRIMEIRA_OPCAO;
}

void TelaOpcoesAvancadas::finish()
{
}

}
}
}
}
