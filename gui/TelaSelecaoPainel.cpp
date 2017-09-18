/*
 * TelaSelecaoPainel.cpp
 *
 *  Created on: 17/04/2013
 *      Author: luciano.silva
 */

#include "TelaSelecaoPainel.h"
#include "MessageDialog.h"
#include <escort.util/String.h>
#include <escort.util/Math.h>
#include "Resources.h"

using escort::util::String;
using escort::util::Math;

namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

TelaSelecaoPainel::TelaSelecaoPainel(
	Fachada* fachada,
	InputOutput* inputOutput) :
		Tela(inputOutput),
		fachada(fachada)
{}

void TelaSelecaoPainel::paint()
{
	if(this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE))
	{
		this->inputOutput->display.setLinhaColuna(0, 0);
		this->inputOutput->display.print("N/A             ");
	}
	else{
		this->inputOutput->display.setLinhaColuna(14,1);
		this->inputOutput->display.print(this->indicePainel + 1, 2);
	}
}

void TelaSelecaoPainel::keyEvent(Teclado::Tecla tecla)
{
	if(this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE))
	{
		return;
	}

	//Verifica se a tecla "SELECIONA_PAINEL" foi pressionada
	if(tecla & Teclado::TECLA_SELECIONA_PAINEL)
	{
		this->modificado = true;
		//Se o �ndice est� indicando o �ltimo painel ent�o vai para o primeiro
		if(this->indicePainel == (this->qntPaineis - 1))
		{
			this->indicePainel = 0;
		}
		//Caso contr�rio decrementa o �ndice do painel selecionado
		else
		{
			this->indicePainel++;
		}
	}
	//Verifica se a tecla "OK"
	if(tecla & Teclado::TECLA_OK)
	{
		this->finish();
		this->visible = false;
	}
}

void TelaSelecaoPainel::start()
{
	//Limpa a tela
	this->inputOutput->display.clearScreen();
	if((this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE)) == 0){
		//Imprime as informa��es fixas da tela
		this->inputOutput->display.print(Resources::getTexto(Resources::SELECIONA_PAINEL));
		this->inputOutput->display.println();
		this->inputOutput->display.print(Resources::getTexto(Resources::PAINEL));

		this->indicePainel = this->fachada->getPainelSelecionado();
		this->qntPaineis = this->fachada->getQntPaineis();
		this->modificado = false;
	}
}

void TelaSelecaoPainel::finish()
{
	if((this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE)) == 0){
		U8 painelAtual = this->fachada->getPainelSelecionado();
		//Verifica se os valores foram modificados
		if(modificado)
		{
			Fachada::Resultado resultado = Fachada::SUCESSO;
			//Verifica se o painel selecionado � diferente do atual
			if(this->indicePainel != painelAtual){
				//Indica ao usu�rio para aguardar o rein�cio da exibi��o
				this->inputOutput->display.clearScreen();
				this->inputOutput->display.print(Resources::getTexto(Resources::AGUARDE));
				//Altera o painel selecionado
				resultado = this->fachada->setPainelSelecionado(this->indicePainel);
			}
			//Exibe o erro
			if(resultado != Fachada::SUCESSO){
				MessageDialog::showMessageDialog(resultado, this->inputOutput, 2000);
			}
		}
	}
}

}
}
}
}
