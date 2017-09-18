/*
 * TelaRegiao.cpp
 *
 *  Created on: 30/05/2012
 *      Author: Gustavo
 */

#include "TelaRegiao.h"
#include "MessageDialog.h"
#include "Resources.h"
#include "GUI.h"

namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

TelaRegiao::TelaRegiao(Fachada* fachada, InputOutput* inputOutput) :
		Tela(inputOutput),
		fachada(fachada)
{}

void TelaRegiao::paint()
{
	if(this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE))
	{
		this->inputOutput->display.setLinhaColuna(0,0);
		this->inputOutput->display.print("N/A             ");
	}
	else{
		//Coloca o cursor no come�o do display
		this->inputOutput->display.setLinhaColuna(0,0);
		//Imprime no display a regi�o selecionada
		this->inputOutput->display.print(nomeRegiao, 16);
	}
}

void TelaRegiao::keyEvent(Teclado::Tecla tecla)
{
	if(this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE))
	{
		return;
	}

	//Verifica se a tecla "AJUSTE ESQUERDA"
	if(tecla & Teclado::TECLA_AJUSTE_ESQUERDA)
	{
		//Indica modifica��o no valor
		this->modificado = true;
		//Se o �ndice est� indicando a primeira regi�o ent�o vai para a �ltima
		if(this->indiceRegiao == 0)
		{
			this->indiceRegiao = (this->qntRegioes - 1);
		}
		//Caso contr�rio decrementa o �ndice da regi�o selecionada
		else
		{
			this->indiceRegiao--;
		}
		//Obt�m o nome da regi�o selecionada
		this->fachada->getNomeRegiao(this->nomeRegiao, this->indiceRegiao);
	}
	//Verifica se a tecla "AJUSTE DIREITA"
	if(tecla & Teclado::TECLA_AJUSTE_DIREITA)
	{
		//Indica modifica��o no valor
		this->modificado = true;
		//Se o �ndice est� indicando a primeira regi�o ent�o vai para a primeira
		if(this->indiceRegiao == (this->qntRegioes - 1))
		{
			this->indiceRegiao = 0;
		}
		//Caso contr�rio decrementa o �ndice da regi�o selecionada
		else
		{
			this->indiceRegiao++;
		}
		//Obt�m o nome da regi�o selecionada
		this->fachada->getNomeRegiao(this->nomeRegiao, this->indiceRegiao);
	}
	//Verifica se a tecla "OK"
	if(tecla & Teclado::TECLA_OK)
	{
		this->finish();
		this->visible = false;
	}
}

void TelaRegiao::start()
{
	//Limpa a tela
	this->inputOutput->display.clearScreen();

	if((this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE)) == 0)
	{
		this->indiceRegiao = this->fachada->getRegiaoSelecionada();
		this->qntRegioes = this->fachada->getQntRegioes();
		this->fachada->getNomeRegiao(this->nomeRegiao, this->indiceRegiao);
		this->modificado = false;
	}
}

void TelaRegiao::finish()
{
	if((this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE)) == 0)
	{
		U32 regiaoAtual = this->fachada->getRegiaoSelecionada();
		//Verifica se o valor foi modificado
		if(modificado)
		{
			Fachada::Resultado resultado = Fachada::SUCESSO;
			//Verifica se a regi�o selecionada � diferente da atual
			if(this->indiceRegiao != regiaoAtual){
				//Indica ao usu�rio para aguardar o rein�cio da exibi��o
				this->inputOutput->display.clearScreen();
				this->inputOutput->display.print(Resources::getTexto(Resources::AGUARDE));
				//Altera a regi�o selecionada
				resultado = this->fachada->setRegiaoSelecionada(this->indiceRegiao, GUI::incrementProgressBar);
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
							resultado = this->fachada->setRegiaoSelecionada(this->indiceRegiao, true);
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
			else{
				Resources::setIdiomaPath(this->fachada->getIdiomaPath());
			}
		}
	}
}

}
}
}
}
