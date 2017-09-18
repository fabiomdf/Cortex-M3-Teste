/*
 * TelaAlternarCom.cpp
 *
 *  Created on: 04/06/2012
 *      Author: arthur.padilha
 */

#include "TelaAlternarCom.h"
#include "MessageDialog.h"
#include "Resources.h"
#include "GUI.h"

namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

TelaAlternarCom::TelaAlternarCom(Fachada* fachada, InputOutput* inputOutput) :
		Tela(inputOutput),
		fachada(fachada)
{}

void TelaAlternarCom::paint()
{
	if(this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE))
	{
		this->inputOutput->display.setLinhaColuna(0, 0);
		this->inputOutput->display.print("N/A             ");
	}
	else{
		this->inputOutput->display.setLinhaColuna(0,0);
		this->inputOutput->display.print(this->nomeAlternancia, 32);
	}
}

void TelaAlternarCom::keyEvent(Teclado::Tecla tecla)
{
	if(this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE))
	{
		return;
	}

	if(tecla & Teclado::TECLA_ALTERNA)
	{
		this->modificado = true;
		this->alternanciaSelecionada++;
		if(this->alternanciaSelecionada >= this->qntAlternancias){
			this->alternanciaSelecionada = 0;
		}
		this->fachada->getNomeAlternancia(this->alternanciaSelecionada, this->nomeAlternancia);
	}
	if(tecla & Teclado::TECLA_OK)
	{
		this->finish();
		this->visible = false;
	}
}

void TelaAlternarCom::start()
{
	//Limpa a tela
	this->inputOutput->display.clearScreen();

	if((this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE)) == 0){
		this->qntAlternancias = this->fachada->getQntAlternancias();
		this->alternanciaSelecionada = this->fachada->getAlternancia();
		this->fachada->getNomeAlternancia(this->alternanciaSelecionada, this->nomeAlternancia);
		this->modificado = false;
	}
}

void TelaAlternarCom::finish()
{
	if((this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE)) == 0){
		U8 alternanciaAntiga = this->fachada->getAlternancia();
		//Verifica se o valor foi modificado
		if(modificado)
		{
			Fachada::Resultado resultado = Fachada::SUCESSO;
			//Verifica se a altern�ncia selecionada � diferente da atual
			if(this->alternanciaSelecionada != alternanciaAntiga){
				//Indica ao usu�rio para aguardar o rein�cio da exibi��o
				this->inputOutput->display.clearScreen();
				this->inputOutput->display.print(Resources::getTexto(Resources::AGUARDE));
				//Alterar a sele��o de exibi��o
				resultado = this->fachada->setAlternancia(this->alternanciaSelecionada);
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
							resultado = this->fachada->setAlternancia(this->alternanciaSelecionada, true);
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
