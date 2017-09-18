/*
 * TelaApagarArquivos.cpp
 *
 *  Created on: 07/11/2014
 *      Author: luis.felipe
 */

#include "TelaApagarArquivos.h"
#include "Resources.h"
#include <escort.util/Converter.h>
#include "MessageDialog.h"

namespace trf {
namespace pontos12 {
namespace application {
namespace gui {


TelaApagarArquivos::TelaApagarArquivos(Fachada* fachada, InputOutput* inputOutput) :
		Tela(inputOutput),
		fachada(fachada)
{}



void TelaApagarArquivos::paint()
{
	//Coloca o cursor no come�o do display
	this->inputOutput->display.setLinhaColuna(0,0);

	//Imprime no display a op��o selecionada
	switch (this->indiceAtual) {
		case CONTROLADOR:
			this->inputOutput->display.print(Resources::getTexto(Resources::CONTROLADOR));
			break;
		case TODOS_DISPOSITIVOS:
			this->inputOutput->display.print(Resources::getTexto(Resources::TODOS_DISPOSITIVOS));
			break;
		case APP:
			this->inputOutput->display.print(Resources::getTexto(Resources::APP));
			break;
		case TODOS_PAINEIS:
			this->inputOutput->display.print(Resources::getTexto(Resources::TODOS_PAINEIS));
			break;
		case PAINEL_X:
		default:

			if(this->indiceAtual <= this->indiceMaximo)
			{
				//Obs.: Texto "painel" deve sempre deixar espa�o para a numera��o na parte final dos 16 caracteres
				//Imprime as informa��es do painel
				this->inputOutput->display.print(Resources::getTexto(Resources::PAINEL));
				this->inputOutput->display.setLinhaColuna(14, 0);
				this->inputOutput->display.print(this->indiceAtual - (U8)TODOS_PAINEIS, 2);
			}
			else
			{
				this->indiceAtual = (U8) CONTROLADOR;
			}
			break;
	}

}

void TelaApagarArquivos::keyEvent(Teclado::Tecla tecla)
{
	//Verifica se a tecla "AJUSTE ESQUERDA"
	if(tecla & Teclado::TECLA_AJUSTE_ESQUERDA)
	{
		//Decrementa o �ndice (respeitando as condi��es determinadas)
		if(this->indiceAtual == CONTROLADOR){
			this->indiceAtual = this->indiceMaximo;
		}
		//Nem sempre haver� APP na configura��o
		else if((this->indiceAtual == TODOS_PAINEIS) && this->fachada->isAPPHabilitado()){
			this->indiceAtual = APP;
		}
		else {
			this->indiceAtual = (this->indiceAtual - 1);
		}
	}
	//Verifica se a tecla "AJUSTE DIREITA"
	if(tecla & Teclado::TECLA_AJUSTE_DIREITA)
	{
		if(this->indiceAtual == this->indiceMaximo){
			this->indiceAtual = (U8) CONTROLADOR;
		}
		//Nem sempre haver� APP na configura��o
		else if((this->indiceAtual == TODOS_DISPOSITIVOS) && this->fachada->isAPPHabilitado()){
			this->indiceAtual = APP;
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

		//Confirma se o usu�rio deseja prossegir
		if(MessageDialog::showConfirmationDialog(Resources::getTexto(Resources::DESEJA_FORMATAR), this->inputOutput, false, 10) == false){
			this->finish();
			this->visible = false;
			return;
		}
		//Indica ao usu�rio para aguardar o fim da opera��o
		this->inputOutput->display.clearScreen();
		this->inputOutput->display.print(Resources::getTexto(Resources::AGUARDE));
		//Executa a opera��o
		bool result = true;
		switch(this->indiceAtual)
		{
		case CONTROLADOR:
			// Apaga os arquivos presentes no controlador
			result = this->fachada->apagarArquivos() == Fachada::SUCESSO;
			break;
		case TODOS_DISPOSITIVOS:
			// Apaga os arquivos de todos os paineis e...
			for (int i = 0; i < this->fachada->getQntPaineis(); ++i) {
				result &= this->fachada->apagarArquivosPainel(i, true) == Fachada::SUCESSO;
			}
			// Apaga os arquivos presentes no APP (se estiver habilitado)
			if(result && this->fachada->isAPPHabilitado()){
				result &= this->fachada->apagarArquivosAPP(true) == Fachada::SUCESSO;
			}
			// Apaga os arquivos presentes no controlador
			if(result){
				result &= this->fachada->apagarArquivos() == Fachada::SUCESSO;
			}
			break;
		case APP:
			// Apaga os arquivos presentes no APP
			result = this->fachada->apagarArquivosAPP(true) == Fachada::SUCESSO;
			break;
		case TODOS_PAINEIS:
			// Apaga os arquivos de todos os paineis e...
			for (int i = 0; i < this->fachada->getQntPaineis(); ++i) {
				result &= this->fachada->apagarArquivosPainel(i, true) == Fachada::SUCESSO;
			}
			break;
		case PAINEL_X:
		default:
			// Apaga os arquivos do painel 'X'
			result &= this->fachada->apagarArquivosPainel(this->indiceAtual - (U8)PAINEL_X, true) == Fachada::SUCESSO;
			break;
		}

		//Exibe o resultado da opera��o
		if(result == true){
			MessageDialog::showMessageDialog(Fachada::SUCESSO, this->inputOutput, 3000);
		}
		else{
			MessageDialog::showMessageDialog(Fachada::FALHA_OPERACAO, this->inputOutput, 3000);
		}

		if(this->indiceAtual == CONTROLADOR || this->indiceAtual == TODOS_DISPOSITIVOS){
			//Reseta o sistema
			this->inputOutput->display.clearScreen();
			this->inputOutput->display.print(Resources::getTexto(Resources::RESETANDO));
			vTaskDelay(1000);
			//Desliga o LCD
			this->inputOutput->display.turnOffDisplay();
			vTaskDelay(50);
			//Reseta o sistema
			this->fachada->resetSystem();
		}
	}
}

void TelaApagarArquivos::start()
{
	//Se foi detetado algum problema com o sistema de arquivos ou
	//se os arquivos de configura��o estiverem inconsistentes ent�o s� permite apagar o Controlador
	if(this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE)){
		this->indiceMaximo = CONTROLADOR;
	}
	else{
		this->indiceMaximo = (U8)TODOS_PAINEIS + this->fachada->getQntPaineis();
	}
	this->indiceAtual = CONTROLADOR;
}

void TelaApagarArquivos::finish()
{
}

bool TelaApagarArquivos::verificarPermissao()
{
	//Se a configura��o est� inconsistente ent�o permite acesso incondicional � fun��o de coleta de dump
	if(this->fachada->getStatusFuncionamento() & (Fachada::ERRO_CONFIGURACAO_INCONSISTENTE | Fachada::ERRO_SISTEMA_ARQUIVO)){
		return true;
	}

	//Se a fun��o est� bloqueada e o usu�rio ainda n�o desbloqueou com a senha de acesso
	//ent�o pergunta para o usu�rio
	// e, caso a sess�o esteja aberta (senha de desbloqueio), o contador � resetado (mais 60 segundos de fun��es liberadas)
	bool acessoLiberado = (this->fachada->isFuncaoLiberada(ParametrosFixos::APAGAR_ARQUIVOS));
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
