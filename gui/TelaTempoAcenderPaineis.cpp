/*
 * TelaTempoAcenderPaineis.cpp
 *
 *  Created on: 12/03/2015
 *      Author: luciano.silva
 */

#include "TelaTempoAcenderPaineis.h"
#include "Resources.h"

namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

TelaTempoAcenderPaineis::TelaTempoAcenderPaineis(Fachada* fachada, InputOutput* inputOutput) :
		Tela(inputOutput),
		fachada(fachada)
{}

void TelaTempoAcenderPaineis::setIndiceAtual(U8 indiceAtual)
{
	this->indiceAtual = indiceAtual;
}

void TelaTempoAcenderPaineis::paint()
{
	if(this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE))
	{
		this->inputOutput->display.setLinhaColuna(0, 0);
		this->inputOutput->display.print("N/A             ");
	}
	else{
		//Coloca o cursor no come�o do display
		this->inputOutput->display.setLinhaColuna(0,0);

		//Imprime no display a op��o selecionada
		this->inputOutput->display.print(Resources::getTexto(Resources::TEMPO_ACESO));

		this->inputOutput->display.setLinhaColuna(0, 1);
		if(this->tempo == 0)
		{
			this->inputOutput->display.print(Resources::getTexto(Resources::INDEFINIDO));
		}
		else
		{
			this->inputOutput->display.print("            ");
			this->inputOutput->display.print(this->tempo, 3);
			this->inputOutput->display.print('s');
		}
	}
}

void TelaTempoAcenderPaineis::keyEvent(Teclado::Tecla tecla)
{
	if(this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE))
	{
		return;
	}
	//Verifica se a tecla "AJUSTE ESQUERDA"
	if(tecla & Teclado::TECLA_AJUSTE_ESQUERDA)
	{
		//Decrementa o �ndice (respeitando as condi��es determinadas)
		if(this->tempo == 0){
			this->tempo = 999;
		}
		else {
			this->tempo = (this->tempo - 1);
		}

	}
	//Verifica se a tecla "AJUSTE DIREITA"
	if(tecla & Teclado::TECLA_AJUSTE_DIREITA)
	{
		if(this->tempo == 999){
			this->tempo = 0;
		}
		else {
			this->tempo = this->tempo + 1;
		}

	}
	//Verifica se a tecla "OK"
	if(tecla & Teclado::TECLA_OK)
	{
		bool result = true;

		if(this->indiceAtual == TODOS_PAINEIS)
		{
			// Acende todos os paineis
			for (U8 painel = 0; painel < this->fachada->getQntPaineis(); ++painel) {
				result &= this->fachada->acenderPainel(painel, this->tempo);
			}
		}
		else
		{
			// Acende o painel selecionado
			result &= this->fachada->acenderPainel(this->indiceAtual - (U8)PAINEL_X, this->tempo);
		}

		if(result == true){
			this->inputOutput->display.clearScreen();
			this->inputOutput->display.print(Resources::getTexto(Resources::SUCESSO));
		}
		else{
			this->inputOutput->display.clearScreen();
			this->inputOutput->display.print(Resources::getTexto(Resources::FALHA));
		}

		vTaskDelay(2000);
		this->visible = false;
	}
}

void TelaTempoAcenderPaineis::start()
{
	this->tempo = 5;
}

void TelaTempoAcenderPaineis::finish()
{
}

}
}
}
}
