/*
 * TelaIdaVolta.cpp
 *
 *  Created on: 30/05/2012
 *      Author: Gustavo
 */

#include "TelaIdaVolta.h"
#include "MessageDialog.h"
#include "Resources.h"
#include <escort.util/String.h>
#include "GUI.h"

using escort::util::String;


namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

TelaIdaVolta::TelaIdaVolta(Fachada* fachada, InputOutput* inputOutput) :
		Tela(inputOutput),
		fachada(fachada)
{}

void TelaIdaVolta::paint()
{
	if(this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE)){
		this->inputOutput->display.setLinhaColuna(0, 0);
		this->inputOutput->display.print("N/A             ");
	}
	else{
		char* texto = (this->sentido == Fachada::IDA) ? Resources::getTexto(Resources::IDA) : Resources::getTexto(Resources::VOLTA);
		U8 length = String::getLength(texto);
		this->inputOutput->display.setLinhaColuna(16 - length, 0);
		this->inputOutput->display.print(texto);
	}
}

void TelaIdaVolta::keyEvent(Teclado::Tecla tecla)
{
	if(this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE)){
		return;
	}

	//Verifica se a tecla "IDA/VOLTA"
	if(tecla & Teclado::TECLA_IDA_VOLTA)
	{
		this->modificacao = true;
		this->sentido = (this->sentido == Fachada::IDA) ? Fachada::VOLTA : Fachada::IDA;
	}
	//Verifica se a tecla "OK"
	if(tecla & Teclado::TECLA_OK)
	{
		this->finish();
		this->visible = false;
	}
}

void TelaIdaVolta::start()
{
	//Limpa a tela
	this->inputOutput->display.clearScreen();
	if((this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE)) == 0){
		//Imprime as informa��es fixas da tela
		this->inputOutput->display.print(Resources::getTexto(Resources::SENTIDO));
		//Obtem o sentido atual do roteiro
		this->sentido = this->fachada->getSentidoRoteiro();
		//Inicia indicando que n�o houve altera��o dos valores
		this->modificacao = false;
	}
}

void TelaIdaVolta::finish()
{
	if((this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE)) == 0){
		//Obtem o sentido atual do roteiro
		Fachada::SentidoRoteiro sentidoAtual = this->fachada->getSentidoRoteiro();
		//Verifica se houve modifica��o no valor
		if(this->modificacao)
		{
			Fachada::Resultado resultado = Fachada::SUCESSO;
			//Verifica se o sentido selecionado � diferente do atual
			if(this->sentido != sentidoAtual){
				//Indica ao usu�rio para aguardar o rein�cio da exibi��o
				this->inputOutput->display.clearScreen();
				this->inputOutput->display.print(Resources::getTexto(Resources::AGUARDE));
				//Altera o sentido do roteiro
				resultado = this->fachada->setSentidoRoteiro(this->sentido);

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
							resultado = this->fachada->setSentidoRoteiro(this->sentido, true);
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
