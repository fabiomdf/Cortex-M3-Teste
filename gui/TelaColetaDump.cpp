/*
 * TelaColetaDump.cpp
 *
 *  Created on: 30/05/2012
 *      Author: Gustavo
 */

#include "TelaColetaDump.h"
#include <escort.util/Converter.h>
#include <escort.util/String.h>
#include "Resources.h"
#include "GUI.h"
#include "MessageDialog.h"

using escort::util::Converter;
using escort::util::String;


#define TEMPO_PISCAGEM_CURSOR \
	500


namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

TelaColetaDump::TelaColetaDump(Fachada* fachada, InputOutput* inputOutput) :
		Tela(inputOutput),
		fachada(fachada)
{}

void TelaColetaDump::show()
{
	enum {
		RECONHECENDO_USB,
		COLETANDO_DUMP,
		ESPERANDO_PENDRIVE,
		PENDRIVE_INCOMPATIVEL,
	}estado = ESPERANDO_PENDRIVE;

	//Se foi detetado algum problema com o sistema de arquivos ent�o est� funcionalidade fica indispon�vel
	if(this->fachada->getStatusFuncionamento() & Fachada::ERRO_SISTEMA_ARQUIVO)
	{
		MessageDialog::showMessageDialog("N/A             ", this->inputOutput, 2000);
		return;
	}

	this->visible = true;
	this->start();

	while(this->visible)
	{
		switch (estado) {
			case RECONHECENDO_USB:
				// Indica ao usu�rio que est� reconhecendo disp�sitivo
				this->inputOutput->display.clearScreen();
				this->inputOutput->display.print(Resources::getTexto(Resources::RECONHECENDO_USB));
				//Se o dispositivo foi reconhecido com sucesso come�a a coleta
				if(this->inputOutput->usbHost->getStatusUSB() == escort::service::USBHost::USB_CONNECTED && this->inputOutput->usbHost->isReady()){
					estado = COLETANDO_DUMP;
				}
				//Se o dispositivo for desconectado ent�o sai da tela
				else if(this->inputOutput->usbHost->getStatusUSB() == escort::service::USBHost::USB_DISCONNECTED) {
					this->visible = (false);
				}
				break;
			case COLETANDO_DUMP:
				// Verifica se foi poss�vel inicializar o sistema de arquivos do pendrive com sucesso
				if(this->inputOutput->usbHost->fileSystem.wasInitiated())
				{
					//TODO-	Verificar se a pasta destino selecionada na TelaSelecionaDump j� existe assim que o pendrive for reconhecido
					//		As a��es tomadas a partir daqui considerar�o que a paste destino selecionada n�o existe / est� vazia
					//Indica no display que est� coletando
					this->inputOutput->display.clearScreen();
					this->inputOutput->display.print(Resources::getTexto(Resources::COLETANDO));
					//Liga a progress bar
					gui::GUI::setEnableProgressBar(true);
					gui::GUI::resetProgressBar();
					//Efetua coleta do dump
					Fachada::Resultado resultado = this->coletarDump(GUI::incrementProgressBar);
	 				//Desliga a progress bar
	 				gui::GUI::setEnableProgressBar(false);
	 				//Indica ao usu�rio o resultado
	 				if(resultado == Fachada::SUCESSO){
	 					MessageDialog::showMessageDialog(Resources::getTexto(Resources::SUCESSO), this->inputOutput, 2000);
	 					//Indica ao usu�rio para remover o pendrive
	 					this->inputOutput->display.clearScreen();
	 					this->inputOutput->display.print(Resources::getTexto(Resources::REMOVA_PENDRIVE));
	 				}
	 				else{
	 					this->inputOutput->display.clearScreen();
	 					this->inputOutput->display.print(Resources::getTexto(Resources::FALHA));
	 				}
	 				//Aguarda o pendrive ser removido
	 				while(this->inputOutput->usbHost->getStatusUSB() != escort::service::USBHost::USB_DISCONNECTED){
	 					vTaskDelay(50);
	 				}
	 				//Finaliza a tela
	 				this->visible = (false);
				}
				else
				{
					//Indica ao usuario que o dispositivo � icompat�vel
					MessageDialog::showMessageDialog(Resources::getTexto(Resources::DISPOSITIVO_INCOMPATIVEL), this->inputOutput, 2000);
					//Finaliza a tela
					this->visible = (false);
				}
				break;
			case ESPERANDO_PENDRIVE:
				//Se um dispositivo USB for plugado ent�o tenta reconhecer o dispositivo
				if(this->inputOutput->usbHost->getStatusUSB() == escort::service::USBHost::USB_PLUGGED)
				{
					estado = RECONHECENDO_USB;
				}
				else
				{
					//Se alguma tecla for pressionada ent�o sai da tela
					Teclado::Tecla tecla = this->inputOutput->teclado.getEventoTeclado();
					if(tecla)
					{
						this->visible = (false);
					}
				}

				break;
			default:
				estado = ESPERANDO_PENDRIVE;
				break;
		}

		vTaskDelay(200);
	}

	//Limpa os eventos que tenham sido gerados durante a exibi��o da tela
	this->inputOutput->teclado.clearEventosTeclado();
}

void TelaColetaDump::paint()
{}

void TelaColetaDump::keyEvent(Teclado::Tecla tecla)
{}

void TelaColetaDump::start()
{
	if(verificarPermissao()){
		//Indica ao usu�rio para inserir um pendrive
		this->inputOutput->display.clearScreen();
		this->inputOutput->display.print(Resources::getTexto(Resources::COLOQUE_PENDRIVE));
	}
	else{
		this->visible = false;
	}
}

void TelaColetaDump::finish()
{}

bool TelaColetaDump::verificarPermissao()
{
	//Se a configura��o est� inconsistente ent�o permite acesso incondicional � fun��o de coleta de dump
	if(this->fachada->getStatusFuncionamento() & Fachada::ERRO_CONFIGURACAO_INCONSISTENTE){
		return true;
	}

	//Se a fun��o est� bloqueada e o usu�rio ainda n�o desbloqueou com a senha de acesso
	//ent�o pergunta para o usu�rio
	// e, caso a sess�o esteja aberta (senha de desbloqueio), o contador � resetado (mais 60 segundos de fun��es liberadas)
	bool acessoLiberado = (this->fachada->isFuncaoLiberada(ParametrosFixos::COLETAR_DUMP));
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

void TelaColetaDump::setOpcao(U8 opcao)
{
	this->opcao = opcao;
}

void TelaColetaDump::setDestFolder(char* destFolder)
{
	this->destFolder = destFolder;
}

Fachada::Resultado TelaColetaDump::coletarDump(void (*incrementProgress)(void))
{
	Fachada::Resultado result = Fachada::SUCESSO;
	//FIXME - Atualmente o resultado retornado � apenas da ultima opera��o
	switch(this->opcao)
	{
		case CONTROLADOR:
			//Coleta dump do controlador
			result = this->fachada->coletarDump(GUI::incrementProgressBar, this->destFolder);
			break;
		case TODOS_DISPOSITIVOS:
			//Coleta dump do controlador
			result = this->fachada->coletarDump(GUI::incrementProgressBar, this->destFolder);
			//Coleta dump dos paineis
			for (U8 i = 0; (i < this->fachada->getQntPaineis() && (result == Fachada::SUCESSO)); ++i) {
				result = this->fachada->coletarDumpPainel(i, GUI::incrementProgressBar, this->destFolder);
			}
			break;
		case TODOS_PAINEIS:
			//Coleta dump dos paineis
			for (U8 i = 0; (i < this->fachada->getQntPaineis() && (result == Fachada::SUCESSO)); ++i) {
				result = this->fachada->coletarDumpPainel(i, GUI::incrementProgressBar, this->destFolder);
			}
			break;
		case PAINEL_X:
		default:
			//Coleta dump do painel
			result = this->fachada->coletarDumpPainel(this->opcao - (U8)PAINEL_X, GUI::incrementProgressBar, this->destFolder);
	}

	return result;
}

}
}
}
}
