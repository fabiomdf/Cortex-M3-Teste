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

TelaSelecaoMensagem2::TelaSelecaoMensagem2(
	Fachada* fachada,
	InputOutput* inputOutput) :
		TelaSelecaoMensagem(fachada, inputOutput)
{}

void TelaSelecaoMensagem2::start()
{
	//Limpa a tela
	this->inputOutput->display.clearScreen();
	if((this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE)) == 0){
		//Imprime as informa��es fixas da tela
		this->inputOutput->display.print(Resources::getTexto(Resources::MENSAGEM));

		this->indiceMensagem = this->fachada->getMensagemSecundariaSelecionada();
		this->qntMensagens = this->fachada->getQntMensagens();
		this->modificado = false;
	}
}

void TelaSelecaoMensagem2::finish()
{
	if((this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE)) == 0){
		U32 mensagemAtual = this->fachada->getMensagemSecundariaSelecionada();
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
				resultado = this->fachada->setMensagemSecundariaSelecionada(this->indiceMensagem);
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
							resultado = this->fachada->setMensagemSecundariaSelecionada(this->indiceMensagem, true);
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
