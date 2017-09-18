/*
 * TelaHoraSaida.cpp
 *
 *  Created on: 30/05/2012
 *      Author: Gustavo
 */

#include "TelaHoraSaida.h"
#include "MessageDialog.h"
#include "Resources.h"
#include "GUI.h"


#define TEMPO_PISCAGEM_CURSOR \
	500


namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

TelaHoraSaida::TelaHoraSaida(Fachada* fachada, InputOutput* inputOutput) :
		Tela(inputOutput),
		fachada(fachada)
{}

void TelaHoraSaida::paint()
{
	if(this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE))
	{
		this->inputOutput->display.setLinhaColuna(0, 0);
		this->inputOutput->display.print("N/A             ");
	}
	else{
		//Imprime a hora de sa�da
		this->inputOutput->display.setLinhaColuna(0, 1);
		this->inputOutput->display.print(this->horas, 2);
		this->inputOutput->display.print(":");
		this->inputOutput->display.print(this->minutos, 2);
		//Coloca o cursor no campo que est� sendo editado
		this->inputOutput->display.setLinhaColuna(this->cursor, 1);
		//Se expirar o tempo do cursor ent�o volta a piscar o cursor
		if(this->timerCursor != 0)
		{
			if(this->timerCursor < xTaskGetTickCount()){
				this->timerCursor = 0;
				this->inputOutput->display.setDisplayStatus(true, false, true);
			}else {
				this->inputOutput->display.setDisplayStatus(false, true, true);
			}
		}
	}
}

void TelaHoraSaida::keyEvent(Teclado::Tecla tecla)
{
	if(this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE))
	{
		return;
	}

	//Verifica se a tecla "AJUSTE ESQUERDA"
	if(tecla & Teclado::TECLA_AJUSTE_ESQUERDA)
	{
		//Se n�o h� permiss�o para o usu�rio alterar a hora de sa�da ent�o n�o deixa mudar os valores
		if(!verificarPermissaoAjuste()){
			return;
		}

		//Indica que houve modifica��o nos valores
		this->modificado = true;
		//Cursor no campo hora
		if(this->cursor == 1)
		{
			//Decrementa a hora (se a hora j� estiver com valor zero faz um wrap)
			if(this->horas == 0){
				//Wrap
				this->horas = 23;
			}else {
				this->horas--;
			}
		}
		//Cursor no campo minutos
		else if(this->cursor == 4)
		{
			//Decrementa os minutos (se os minutos j� estiverem com valor zero faz um wrap)
			if(this->minutos == 0){
				//Wrap
				this->minutos = 59;
			}else {
				this->minutos--;
			}
		}
		else
		{
			this->cursor = 1;
		}

		//Recarrega o timer do cursor
		this->timerCursor = xTaskGetTickCount() + TEMPO_PISCAGEM_CURSOR;
	}
	//Verifica se a tecla "AJUSTE DIREITA"
	if(tecla & Teclado::TECLA_AJUSTE_DIREITA)
	{
		//Se n�o h� permiss�o para o usu�rio alterar a hora de sa�da ent�o n�o deixa mudar os valores
		if(!verificarPermissaoAjuste()){
			return;
		}

		//Indica que houve modifica��o nos valores
		this->modificado = true;
		//Cursor no campo hora
		if(this->cursor == 1)
		{
			//Incrementa a hora (se a hora j� estiver com valor m�ximo faz um wrap)
			if(this->horas == 23){
				//Wrap
				this->horas = 0;
			}else {
				this->horas++;
			}
		}
		//Cursor no campo minutos
		else if(this->cursor == 4)
		{
			//Incrementa os minutos (se os minutos j� estiverem com valor m�ximo faz um wrap)
			if(this->minutos == 59){
				//Wrap
				this->minutos = 0;
			}else {
				this->minutos++;
			}
		}
		else
		{
			this->cursor = 1;
		}

		//Recarrega o timer do cursor
		this->timerCursor = xTaskGetTickCount() + TEMPO_PISCAGEM_CURSOR;
	}
	//Verifica se a tecla "OK"
	if(tecla & Teclado::TECLA_OK)
	{
		//Cursor no campo hora
		if(this->cursor == 1)
		{
			//Passa para o campo de minutos
			this->cursor = 4;
		}
		//Cursor no campo minutos
		else if(this->cursor == 4)
		{
			//Finaliza a exibi��o
			this->finish();
			this->visible = false;
		}
		else
		{
			this->cursor = 1;
		}
	}
}

void TelaHoraSaida::start()
{
	//Limpa a tela
	this->inputOutput->display.clearScreen();

	if((this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE)) == 0){
		//Imprime as informa��es fixas da tela
		this->inputOutput->display.print(Resources::getTexto(Resources::AJUSTAR_PARTIDA));
		//Obtem a hora de sa�da atual
		this->fachada->getHoraSaida(&this->horas, &this->minutos);
		//Coloca o cursor na posi��o da hora
		this->cursor = 1;
		//Inicializa o timer de piscagem do cursor
		this->timerCursor = xTaskGetTickCount() + TEMPO_PISCAGEM_CURSOR;
		//Inicia indicando que os valores n�o foram modificados
		this->modificado = false;
		//Verifica se a fun��o de ajuste de hora de sa�da est� bloqueada
		// e, caso a sess�o esteja aberta (senha de desbloqueio), o contador � resetado (mais 60 segundos de fun��es liberadas)
		this->acessoLiberado = (this->fachada->isFuncaoLiberada(ParametrosFixos::AJUSTE_HORA_SAIDA));
		if(!verificarPermissaoAjuste()){
			this->visible = false;
		}
	}
}

void TelaHoraSaida::finish()
{
	if((this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE)) == 0){
		U8 horas, minutos;

		//Desliga a piscagem do cursor
		this->inputOutput->display.setDisplayStatus(false, false, true);
		//Obtem a hora de sa�da atual
		this->fachada->getHoraSaida(&horas, &minutos);
		//Verifica se a hora de sa�da ajustada � diferente da atual
		if(modificado)
		{
			Fachada::Resultado resultado = Fachada::SUCESSO;

			if(horas != this->horas || minutos != this->minutos){
				//Indica ao usu�rio para aguardar o rein�cio da exibi��o
				this->inputOutput->display.clearScreen();
				this->inputOutput->display.print(Resources::getTexto(Resources::AGUARDE));
				//Altera a hora de sa�da
				resultado = this->fachada->setHoraSaida(this->horas, this->minutos, GUI::incrementProgressBar);
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
							resultado = this->fachada->setHoraSaida(this->horas, this->minutos, true);
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

bool TelaHoraSaida::verificarPermissaoAjuste()
{
	//Se a fun��o est� bloqueada e o usu�rio ainda n�o desbloqueou com a senha de acesso
	//ent�o pergunta para o usu�rio
	if(!this->acessoLiberado){
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
				this->acessoLiberado = true;
			}
			else{
				//Indica ao usu�rio que a senha est� incorreta
				MessageDialog::showMessageDialog(Resources::getTexto(Resources::SENHA_INCORRETA), inputOutput, 2000);
			}
		}
		//Recarrega o timeout
		this->timeoutTecla = xTaskGetTickCount() + teclaTime;
	}

	return this->acessoLiberado;
}

}
}
}
}
