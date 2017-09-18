/*
 * TelaAcenderPaineis.cpp
 *
 *  Created on: 20/11/2014
 *      Author: luciano.silva
 */

#include "TelaAcenderPaineis.h"
#include "Resources.h"
#include <escort.util/Converter.h>
#include "MessageDialog.h"

namespace trf {
namespace pontos12 {
namespace application {
namespace gui {


TelaAcenderPaineis::TelaAcenderPaineis(Fachada* fachada, InputOutput* inputOutput) :
		Tela(inputOutput),
		fachada(fachada),
		telaTempoAcenderPaineis(fachada, inputOutput)
{}



void TelaAcenderPaineis::paint()
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
		if (this->indiceAtual ==  TODOS_PAINEIS)
		{
			this->inputOutput->display.print(Resources::getTexto(Resources::TODOS_PAINEIS)); //
		}
		else
		{
			if(this->indiceAtual <= ((U8)TODOS_PAINEIS + this->qntPaineis))
			{
				//Obs.: Texto "painel" deve sempre deixar espa�o para a numera��o na parte final dos 16 caracteres
				//Imprime as informa��es do painel
				this->inputOutput->display.print(Resources::getTexto(Resources::PAINEL));
				this->inputOutput->display.setLinhaColuna(14, 0);
				this->inputOutput->display.print(this->indiceAtual - (U8)TODOS_PAINEIS, 2);
			}
			else
			{
				this->indiceAtual = (U8) TODOS_PAINEIS;
			}
		}
	}
}

void TelaAcenderPaineis::keyEvent(Teclado::Tecla tecla)
{
	if(this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE))
	{
		return;
	}
	//Verifica se a tecla "AJUSTE ESQUERDA"
	if(tecla & Teclado::TECLA_AJUSTE_ESQUERDA)
	{
		//Decrementa o �ndice (respeitando as condi��es determinadas)
		if(this->indiceAtual == TODOS_PAINEIS){
			this->indiceAtual = (U8)TODOS_PAINEIS + this->qntPaineis;
		}

		else {
			this->indiceAtual = (this->indiceAtual - 1);
		}

	}
	//Verifica se a tecla "AJUSTE DIREITA"
	if(tecla & Teclado::TECLA_AJUSTE_DIREITA)
	{
		if(this->indiceAtual == ((U8)TODOS_PAINEIS + this->qntPaineis)){
			this->indiceAtual = (U8) TODOS_PAINEIS;
		}
		else {
			this->indiceAtual = this->indiceAtual + 1;
		}

	}
	//Verifica se a tecla "OK"
	if(tecla & Teclado::TECLA_OK)
	{
		//Verifica se a fun��o est� bloqueada
		if(!verificarPermissao()){
			this->visible = false;
			return;
		}

		this->telaTempoAcenderPaineis.setIndiceAtual(this->indiceAtual);
		this->telaTempoAcenderPaineis.show();

		//Termina a exibi��o da tela
		this->finish();
		this->visible = false;
	}
}

void TelaAcenderPaineis::start()
{
	this->indiceAtual = TODOS_PAINEIS;
	this->qntPaineis = this->fachada->getQntPaineis();
}

void TelaAcenderPaineis::finish()
{
}

bool TelaAcenderPaineis::verificarPermissao()
{
	//Se a configura��o est� inconsistente ent�o permite acesso incondicional � fun��o de coleta de dump
	if(this->fachada->getStatusFuncionamento() & Fachada::ERRO_CONFIGURACAO_INCONSISTENTE){
		return true;
	}

	//Se a fun��o est� bloqueada e o usu�rio ainda n�o desbloqueou com a senha de acesso
	//ent�o pergunta para o usu�rio
	bool acessoLiberado = (this->fachada->isFuncaoLiberada(ParametrosFixos::ACENDER_PAINEIS));
	if(!acessoLiberado){
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
			//Se a senha est� correta ent�o libera o acesso
			if(this->fachada->verificarSenhaAcesso(senha)){
				acessoLiberado = true;
			}
			else{
				//Indica ao usu�rio que a senha est� incorreta
				MessageDialog::showMessageDialog(Resources::getTexto(Resources::SENHA_INCORRETA), inputOutput, 2000);
			}
		}
		//Recarrega o timeout
		this->timeoutTecla = xTaskGetTickCount() + teclaTime;
	}

	return acessoLiberado;
}

}
}
}
}
