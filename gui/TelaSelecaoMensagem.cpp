/*
 * TelaSelecaoMensagem.cpp
 *
 *  Created on: 30/05/2012
 *      Author: Gustavo
 */

#include "TelaSelecaoMensagem.h"
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

TelaSelecaoMensagem::TelaSelecaoMensagem(
	Fachada* fachada,
	InputOutput* inputOutput) :
		Tela(inputOutput),
		fachada(fachada)
{}

void TelaSelecaoMensagem::paint()
{
	if(this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE)){
		this->inputOutput->display.setLinhaColuna(0, 0);
		this->inputOutput->display.print("N/A             ");
	}
	else{
		this->inputOutput->display.setLinhaColuna(11,0);
		this->inputOutput->display.print(this->indiceMensagem, 5);
		this->inputOutput->display.println();

		U32 length = 0;

		////Monta o r�tulo da mensagem selecionada
		this->fachada->getLabelMessagem(this->indiceMensagem, this->buffer);
		length += Math::min(this->inputOutput->display.getWidth(), String::getLength(this->buffer));

		this->inputOutput->display.print(this->buffer, length);
		for(U32 i = length; i < this->inputOutput->display.getWidth(); i++){
			this->inputOutput->display.print(" ");
		}
	}
}

void TelaSelecaoMensagem::keyEvent(Teclado::Tecla tecla)
{
	if(this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE)){
		return;
	}
	//Verifica se a tecla "MENSAGEM ESQUERDA"
	if(tecla & Teclado::TECLA_MENSAGEM_ESQUERDA)
	{
		this->modificado = true;
		//Se o �ndice est� indicando o primeiro roteiro  ent�o vai para o �ltimo
		if(this->indiceMensagem == 0)
		{
			this->indiceMensagem = (this->qntMensagens - 1);
		}
		//Caso contr�rio decrementa o �ndice do roteiro selecionado
		else
		{
			this->indiceMensagem--;
		}
	}
	//Verifica se a tecla "MENSAGEM DIREITA"
	if(tecla & Teclado::TECLA_MENSAGEM_DIREITA)
	{
		this->modificado = true;
		//Se o �ndice est� indicando o �ltimo roteiro  ent�o vai para o primeiro
		if(this->indiceMensagem == (this->qntMensagens - 1))
		{
			this->indiceMensagem = 0;
		}
		//Caso contr�rio decrementa o �ndice do roteiro selecionado
		else
		{
			this->indiceMensagem++;
		}
	}
	//Verifica se a tecla "OK"
	if(tecla & Teclado::TECLA_OK)
	{
		this->finish();
		this->visible = false;
	}
}

void TelaSelecaoMensagem::start()
{
	//Limpa a tela
	this->inputOutput->display.clearScreen();
	if((this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE)) == 0){
		//Imprime as informa��es fixas da tela
		this->inputOutput->display.print(Resources::getTexto(Resources::MENSAGEM));

		this->indiceMensagem = this->fachada->getMensagemSelecionada();
		this->qntMensagens = this->fachada->getQntMensagens();
		this->modificado = false;
	}
}

void TelaSelecaoMensagem::finish()
{
	if((this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE)) == 0){
		U32 mensagemAtual = this->fachada->getMensagemSelecionada();
		//Verifica se o valor foi modificado
		if(modificado)
		{
			Fachada::Resultado resultado = Fachada::SUCESSO;
			//Verifica se a mensagem selecionada � diferente da atual
			if(this->indiceMensagem != mensagemAtual){
				//Indica ao usu�rio para aguardar o rein�cio da exibi��o
				this->inputOutput->display.clearScreen();
				this->inputOutput->display.print(Resources::getTexto(Resources::AGUARDE));
				//Altera a mensagem selecionada
				resultado = this->fachada->setMensagemSelecionada(this->indiceMensagem);
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
							resultado = this->fachada->setMensagemSelecionada(this->indiceMensagem, true);
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
