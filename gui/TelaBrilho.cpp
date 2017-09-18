/*
 * TelaBrilho.cpp
 *
 *  Created on: 30/05/2012
 *      Author: Gustavo
 */

#include "TelaBrilho.h"
#include "MessageDialog.h"
#include "Resources.h"
#include "GUI.h"


#define TEMPO_PISCAGEM_CURSOR \
	500


namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

TelaBrilho::TelaBrilho(Fachada* fachada, InputOutput* inputOutput) :
		Tela(inputOutput),
		fachada(fachada)
{}

void TelaBrilho::paint()
{
	if(this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE))
	{
		this->inputOutput->display.setLinhaColuna(0, 0);
		this->inputOutput->display.print("N/A             ");
	}
	else{
		//Imprime a varia��o de brilho
		//"max:000 min:000 "
		this->inputOutput->display.setLinhaColuna(0, 1);
		this->inputOutput->display.print("max:");
		this->inputOutput->display.print(this->brilhoMax, 3);
		this->inputOutput->display.print(" min:");
		this->inputOutput->display.print(this->brilhoMin, 3);
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

void TelaBrilho::keyEvent(Teclado::Tecla tecla)
{
	if(this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE))
	{
		return;
	}

	//Verifica se a tecla "AJUSTE ESQUERDA"
	if(tecla & Teclado::TECLA_AJUSTE_ESQUERDA)
	{
		//Cursor no campo hora
		if(this->cursor == 6)
		{
			//Decrementa a hora (se a hora j� estiver com valor zero faz um wrap)
			if(this->brilhoMax == 0){
				//Wrap
				this->brilhoMax = 100;
			}else {
				this->brilhoMax--;
			}
		}
		//Cursor no campo minutos
		else if(this->cursor == 14)
		{
			//Decrementa os minutos (se os minutos j� estiverem com valor zero faz um wrap)
			if(this->brilhoMin == 0){
				//Wrap
				this->brilhoMin = 100;
			}else {
				this->brilhoMin--;
			}
		}
		else
		{
			this->cursor = 6;
		}

		//Recarrega o timer do cursor
		this->timerCursor = xTaskGetTickCount() + TEMPO_PISCAGEM_CURSOR;
	}
	//Verifica se a tecla "AJUSTE DIREITA"
	if(tecla & Teclado::TECLA_AJUSTE_DIREITA)
	{
		//Cursor no campo hora
		if(this->cursor == 6)
		{
			//Incrementa a hora (se a hora j� estiver com valor m�ximo faz um wrap)
			if(this->brilhoMax == 100){
				//Wrap
				this->brilhoMax = 0;
			}else {
				this->brilhoMax++;
			}
		}
		//Cursor no campo minutos
		else if(this->cursor == 14)
		{
			//Incrementa os minutos (se os minutos j� estiverem com valor m�ximo faz um wrap)
			if(this->brilhoMin == 100){
				//Wrap
				this->brilhoMin = 0;
			}else {
				this->brilhoMin++;
			}
		}
		else
		{
			this->cursor = 6;
		}

		//Recarrega o timer do cursor
		this->timerCursor = xTaskGetTickCount() + TEMPO_PISCAGEM_CURSOR;
	}
	//Verifica se a tecla "OK"
	if(tecla & Teclado::TECLA_OK)
	{
		//Cursor no campo hora
		if(this->cursor == 6)
		{
			//Passa para o campo de minutos
			this->cursor = 14;
		}
		//Cursor no campo minutos
		else if(this->cursor == 14)
		{
			//Finaliza a exibi��o
			this->finish();
			this->visible = false;
		}
		else
		{
			this->cursor = 6;
		}
	}
}

void TelaBrilho::start()
{
	//Limpa a tela
	this->inputOutput->display.clearScreen();

	if((this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE)) == 0){
		//Imprime as informa��es fixas da tela
		this->inputOutput->display.print(Resources::getTexto(Resources::AJUSTAR_BRILHO));
		//Obtem a varia��o de brilho atual
		this->fachada->getVariacaoBrilho(this->fachada->getPainelSelecionado(), &this->brilhoMax, &this->brilhoMin);
		//Coloca o cursor na posi��o da hora
		this->cursor = 6;
		//Inicializa o timer de piscagem do cursor
		this->timerCursor = xTaskGetTickCount() + TEMPO_PISCAGEM_CURSOR;
	}
}

void TelaBrilho::finish()
{
	if((this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE)) == 0){
		U8 bMax, bMin;

		//Desliga a piscagem do cursor
		this->inputOutput->display.setDisplayStatus(false, false, true);
		//Obtem os valore atuais de brilho
		this->fachada->getVariacaoBrilho(this->fachada->getPainelSelecionado(), &bMax, &bMin);
		//Verifica se o brilho ajustado � diferente do atual
		Fachada::Resultado resultado = Fachada::SUCESSO;
		if(bMax != this->brilhoMax || bMin != this->brilhoMin){
			//Indica ao usu�rio para aguardar o rein�cio da exibi��o
			this->inputOutput->display.clearScreen();
			this->inputOutput->display.print(Resources::getTexto(Resources::AGUARDE));
			//Altera os ajustes de brilho
			resultado = this->fachada->setVariacaoBrilho(this->fachada->getPainelSelecionado(), this->brilhoMax, this->brilhoMin, false);
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
						resultado = this->fachada->setVariacaoBrilho(this->fachada->getPainelSelecionado(), this->brilhoMax, this->brilhoMin, true);
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
