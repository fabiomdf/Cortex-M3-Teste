/*
 * TelaSelecaoRoteiro.cpp
 *
 *  Created on: 30/05/2012
 *      Author: Gustavo
 */

#include "TelaSelecaoRoteiro.h"
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

TelaSelecaoRoteiro::TelaSelecaoRoteiro(
	Fachada* fachada,
	InputOutput* inputOutput) :
		Tela(inputOutput),
		fachada(fachada)
{}

void TelaSelecaoRoteiro::paint()
{
	if(this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE))
	{
		this->inputOutput->display.setLinhaColuna(0, 0);
		this->inputOutput->display.print("N/A             ");
	}
	else{
		this->inputOutput->display.setLinhaColuna(11,0);
		this->inputOutput->display.print(this->indiceRoteiro, 5);
		this->inputOutput->display.println();

		////Monta o r�tulo do n�mero do roteiro com o texto do roteiro
		this->fachada->getLabelNumeroComRoteiro(this->indiceRoteiro, this->buffer);
		U32 length = Math::min(this->inputOutput->display.getWidth(), String::getLength(this->buffer));

		this->inputOutput->display.print(this->buffer, length);
		for(U32 i = length; i < this->inputOutput->display.getWidth(); i++){
			this->inputOutput->display.print(" ");
		}
	}
}

void TelaSelecaoRoteiro::keyEvent(Teclado::Tecla tecla)
{
	if(this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE))
	{
		return;
	}
	//Verifica se a tecla "ROTEIRO ESQUERDA"
	if(tecla & Teclado::TECLA_ROTEIRO_ESQUERDA)
	{
		//Indica modifica��o no valor
		this->modificado = true;
		//Se o �ndice est� indicando o primeiro roteiro  ent�o vai para o �ltimo
		if(this->indiceRoteiro == 0)
		{
			this->indiceRoteiro = (this->qntRoteiros - 1);
		}
		//Caso contr�rio decrementa o �ndice do roteiro selecionado
		else
		{
			this->indiceRoteiro--;
		}
	}
	//Verifica se a tecla "ROTEIRO DIREITA"
	if(tecla & Teclado::TECLA_ROTEIRO_DIREITA)
	{
		//Indica modifica��o no valor
		this->modificado = true;
		//Se o �ndice est� indicando o �ltimo roteiro  ent�o vai para o primeiro
		if(this->indiceRoteiro == (this->qntRoteiros - 1))
		{
			this->indiceRoteiro = 0;
		}
		//Caso contr�rio decrementa o �ndice do roteiro selecionado
		else
		{
			this->indiceRoteiro++;
		}
	}
	//Verifica se a tecla "OK"
	if(tecla & Teclado::TECLA_OK)
	{
		this->finish();
		this->visible = false;
	}
}

void TelaSelecaoRoteiro::start()
{
	//Limpa a tela
	this->inputOutput->display.clearScreen();
	if((this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE)) == 0){
		//Imprime as informa��es fixas da tela
		this->inputOutput->display.print(Resources::getTexto(Resources::ROTEIRO));

		this->indiceRoteiro = this->fachada->getRoteiroSelecionado();
		this->qntRoteiros = this->fachada->getQntRoteiros();
		this->modificado = false;
	}
}

void TelaSelecaoRoteiro::finish()
{
	if((this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE)) == 0){
		U32 roteiroAtual = this->fachada->getRoteiroSelecionado();
		//Verifica se houve modifica��o no valor
		if(this->modificado)
		{
			Fachada::Resultado resultado = Fachada::SUCESSO;
			//Verifica se o roteiro selecionado � diferente do atual
			if(this->indiceRoteiro != roteiroAtual){
				//Indica ao usu�rio para aguardar o rein�cio da exibi��o
				this->inputOutput->display.clearScreen();
				this->inputOutput->display.print(Resources::getTexto(Resources::AGUARDE));
				//Alterar a sele��o de exibi��o
				resultado = this->fachada->setRoteiroSelecionado(this->indiceRoteiro);

				//ProtocoloCl Chile
				this->fachada->sendRS485();

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
							resultado = this->fachada->setRoteiroSelecionado(this->indiceRoteiro, true);
						}
						else{
							resultado = Fachada::SENHA_INCORRETA;
						}
					}
					else{
						return;
					}
				}
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
