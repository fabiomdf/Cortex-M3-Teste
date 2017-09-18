/*
 * TelaModoTeste.cpp
 *
 *  Created on: 28/08/2015
 *      Author: Gustavo
 */

#include "TelaModoTeste.h"
#include "MessageDialog.h"
#include <escort.util/String.h>
#include <escort.util/Math.h>
#include "Resources.h"
#include "GUI.h"

using escort::util::String;
using escort::util::Math;


namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

enum {
	AUTOMATICO,
	LINHAS,
	COLUNAS,
	APAGADO,
	ACESO,
	TEXTO,
	DESLIGA_TESTE
};

TelaModoTeste::TelaModoTeste(
	Fachada* fachada,
	InputOutput* inputOutput) :
		Tela(inputOutput),
		fachada(fachada)
{}

void TelaModoTeste::paint()
{
	if(this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE))
	{
		this->inputOutput->display.setLinhaColuna(0, 0);
		this->inputOutput->display.print("N/A             ");
	}
	else{
		//Coloca o cursor no come�o do display
		this->inputOutput->display.setLinhaColuna(0,1);

		//Imprime no display o item selecionado
		switch (this->indiceMenu) {
			case AUTOMATICO:
				this->inputOutput->display.print(Resources::getTexto(Resources::TESTE_AUTOMATICO));
				break;
			case LINHAS:
				this->inputOutput->display.print(Resources::getTexto(Resources::LINHAS));
				break;
			case COLUNAS:
				this->inputOutput->display.print(Resources::getTexto(Resources::COLUNAS));
				break;
			case APAGADO:
				this->inputOutput->display.print(Resources::getTexto(Resources::APAGADO));
				break;
			case ACESO:
				this->inputOutput->display.print(Resources::getTexto(Resources::ACESO));
				break;
			case TEXTO:
				this->inputOutput->display.print(Resources::getTexto(Resources::TEXTO));
				break;
			case DESLIGA_TESTE:
				this->inputOutput->display.print(Resources::getTexto(Resources::DESLIGAR_TESTE));
				break;
		}
	}
}

void TelaModoTeste::keyEvent(Teclado::Tecla tecla)
{
	if(this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE))
	{
		return;
	}
	//Verifica se a tecla "AJUSTE ESQUERDA"
	if(tecla & Teclado::TECLA_AJUSTE_ESQUERDA)
	{
		//Se o �ndice est� indicando o primeiro roteiro  ent�o vai para o �ltimo
		if(this->indiceMenu == 0)
		{
			this->indiceMenu = DESLIGA_TESTE;
		}
		//Caso contr�rio decrementa o �ndice do roteiro selecionado
		else
		{
			this->indiceMenu--;
		}
		//Altera o teste
		if(this->indiceMenu != DESLIGA_TESTE){
			this->setTeste(this->indiceMenu);
		}
	}
	//Verifica se a tecla "AJUSTE DIREITA"
	if(tecla & Teclado::TECLA_AJUSTE_DIREITA)
	{
		//Se o �ndice est� indicando o �ltimo roteiro  ent�o vai para o primeiro
		if(this->indiceMenu == DESLIGA_TESTE)
		{
			this->indiceMenu = 0;
		}
		//Caso contr�rio decrementa o �ndice do roteiro selecionado
		else
		{
			this->indiceMenu++;
		}
		//Altera o teste
		if(this->indiceMenu != DESLIGA_TESTE){
			this->setTeste(this->indiceMenu);
		}
	}
	//Verifica se a tecla "OK"
	if(tecla & Teclado::TECLA_OK)
	{
		if(this->indiceMenu == DESLIGA_TESTE){
			this->fachada->setModoTeste(false, 0, true);
			this->visible = false;
		}
	}
}

void TelaModoTeste::handleKeys()
{
	//Verifica se existe um evento do teclado para ser tratado
	Teclado::Tecla tecla = inputOutput->teclado.getEventoTeclado();
	if(tecla)
	{
		//Trata o evento do teclado
		this->keyEvent(tecla);
	}
}

void TelaModoTeste::start()
{
	//Limpa a tela
	this->inputOutput->display.clearScreen();
	if((this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE)) == 0){
		//Imprime as informa��es fixas da tela
		this->inputOutput->display.print(Resources::getTexto(Resources::MODO_TESTE));

		this->indiceMenu = 0;
		setTeste(indiceMenu);
	}
}

void TelaModoTeste::finish()
{
}

void TelaModoTeste::setTeste(U8 tipo)
{
	Fachada::Resultado resultado = Fachada::SUCESSO;
	//Altera o modo de teste
	resultado = this->fachada->setModoTeste(true, tipo);
	//Verifica se esta opera��o est� bloqueada
	if(resultado == Fachada::FUNCAO_BLOQUEADA){
		//Indica ao usu�rio que a fun��o est� bloqueada
		MessageDialog::showMessageDialog(Resources::getTexto(Resources::FUNCAO_BLOQUEADA), inputOutput, 2000);
		//Pergunta ao usu�rio se deseja colocar a senha para desbloquear a fun��o
		//Obs.: s� faz sentido colocar a senha de acesso se ela estiver cadastrada
		if(this->fachada->isSenhaAcessoHabilitada()
			&& MessageDialog::showConfirmationDialog(
				Resources::getTexto(Resources::DESEJA_DESBLOQUEAR_FUNCAO),
				this->inputOutput,
				false,
				10))
		{
			//Pede para o usu�rio digitar a senha de desbloqueio
			U32 senha = MessageDialog::showPasswordDialog(
					Resources::getTexto(Resources::SENHA),
					this->inputOutput,
					6);
			//Se a senha est� correta ent�o realiza a opera��o for�ando o desbloqueio
			if(this->fachada->verificarSenhaAcesso(senha)){
				resultado = this->fachada->setModoTeste(true, tipo, true);
			}
			else{
				resultado = Fachada::SENHA_INCORRETA;
			}
		}
		else{
			this->visible = false;
			return;
		}
	}
	//Verifica o resultado da opera��o
	if(resultado != Fachada::SUCESSO){
		MessageDialog::showMessageDialog(resultado, inputOutput, 2000);
		this->visible = false;
	}
}

}
}
}
}
