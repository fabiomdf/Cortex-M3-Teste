/*
 * TelaNovaConfiguracao.cpp
 *
 *  Created on: 30/05/2012
 *      Author: Gustavo
 */

#include "TelaNovaConfiguracao.h"
#include "MessageDialog.h"
#include <freertos.kernel/FreeRTOSKernel/FreeRTOSKernel.h>
#include "Resources.h"
#include "GUI.h"
#include <escort.util/String.h>

using escort::service::FatFs;
using escort::util::String;


namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

TelaNovaConfiguracao::TelaNovaConfiguracao(Fachada* fachada, InputOutput* inputOutput) :
		Tela(inputOutput),
		fachada(fachada),
		ldx2Folder(&inputOutput->usbHost->fileSystem)
{}

void TelaNovaConfiguracao::paint()
{
	this->timeoutTecla = xTaskGetTickCount() + teclaTime;
	this->ldx2Folder.open("ldx2");
	FatFs::FileList* listaArquivos = this->ldx2Folder.getFileList("b12|nfx");

	// Obtem o nome do arquivo selecionado
	char* nomeArquivo = this->pathArquivoSelecionado + 5;
	for (U32 i = 0; i <= this->arquivoSelecionado; ++i)
	{
		U8 attr;
		listaArquivos->getNext(nomeArquivo, &attr);
	}
	//Exibe na tela
	this->inputOutput->display.setLinhaColuna(0,1);
	this->inputOutput->display.print(nomeArquivo);
	for (int i = 0; i < 16 - String::getLength(nomeArquivo); ++i) {
		this->inputOutput->display.print(' ');
	}
}

void TelaNovaConfiguracao::keyEvent(Teclado::Tecla tecla)
{
	FatFs::FileList* listaArquivos = this->ldx2Folder.getFileList("b12|nfx");

	//Verifica se a tecla "AJUSTE ESQUERDA"
	if(tecla & Teclado::TECLA_AJUSTE_ESQUERDA)
	{
		if(this->arquivoSelecionado == 0)
		{
			this->arquivoSelecionado = listaArquivos->getLength() - 1;
		}
		else
		{
			this->arquivoSelecionado--;
		}
	}
	//Verifica se a tecla "AJUSTE DIREITA"
	if(tecla & Teclado::TECLA_AJUSTE_DIREITA)
	{
		this->arquivoSelecionado = (this->arquivoSelecionado + 1) % listaArquivos->getLength();
	}
	//Verifica se a tecla "OK"
	if(tecla & Teclado::TECLA_OK)
	{
		//Indica no display que est� atualizando a configura��o
		this->inputOutput->display.clearScreen();
		this->inputOutput->display.print(Resources::getTexto(Resources::ATUALIZANDO));
		//Liga a progress bar
		gui::GUI::setEnableProgressBar(true);
		gui::GUI::resetProgressBar();
		//Executa a sincroniza��o com o pendrive
		Fachada::Resultado resultado = this->fachada->penDriveSync(this->pathArquivoSelecionado, GUI::incrementProgressBar);
		//Desliga a progress bar
		gui::GUI::setEnableProgressBar(false);

		//Verifica se esta opera��o est� bloqueada...
		if(resultado == Fachada::FUNCAO_BLOQUEADA){
			//Verifica se h� algum erro de funcionamento que s� � corrigido carregando uma nova configura��o...
			if((this->fachada->getStatusFuncionamento() & Fachada::ERRO_CONFIGURACAO_INCONSISTENTE)
				//... ou se n�o tem nenhuma senha de acesso cadastrada
				//(se a fun��o de nova configura��o estiver bloqueada e n�o tiver senha cadastrada
				//ent�o nunca mais seria poss�vel mudar a configura��o)
				|| !this->fachada->isSenhaAcessoHabilitada())
			{
				//Indica no display que est� atualizando a configura��o
				this->inputOutput->display.clearScreen();
				this->inputOutput->display.print(Resources::getTexto(Resources::ATUALIZANDO));
				//Liga a progress bar
				gui::GUI::setEnableProgressBar(true);
				gui::GUI::resetProgressBar();
				resultado = this->fachada->penDriveSync(this->pathArquivoSelecionado, GUI::incrementProgressBar, true);
				//Desliga a progress bar
				gui::GUI::setEnableProgressBar(false);
			}
			else{
				//Indica ao usu�rio que a fun��o est� bloqueada
				MessageDialog::showMessageDialog(Resources::getTexto(Resources::FUNCAO_BLOQUEADA), inputOutput, 2000);
				//Pergunta ao usu�rio se deseja colocar a senha para desbloquear a fun��o
				//Obs.: s� faz sentido colocar a senha de acesso se ela estiver cadastrada
				if(MessageDialog::showConfirmationDialog(
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
						//Indica no display que est� atualizando a configura��o
						this->inputOutput->display.clearScreen();
						this->inputOutput->display.print(Resources::getTexto(Resources::ATUALIZANDO));
						//Liga a progress bar
						gui::GUI::setEnableProgressBar(true);
						gui::GUI::resetProgressBar();
						resultado = this->fachada->penDriveSync(this->pathArquivoSelecionado, GUI::incrementProgressBar, true);
						//Desliga a progress bar
						gui::GUI::setEnableProgressBar(false);
					}
					else{
						resultado = Fachada::SENHA_INCORRETA;
					}
				}
				else{
					//Finaliza
					this->finish();
					this->visible = false;
					return;
				}
			}
		}

		//Indica ao usu�rio o resultado
		MessageDialog::showMessageDialog(resultado, inputOutput, 2000);
		//Finaliza
		this->finish();
		this->visible = false;
	}
}

void TelaNovaConfiguracao::usbEvent(escort::service::USBHost::StatusUSB statusUSB)
{
	if(statusUSB == escort::service::USBHost::USB_DISCONNECTED)
	{
		this->visible = false;
	}
}

void TelaNovaConfiguracao::start()
{
	//Tenta abrir pasta de atualiza��o de Firmware
	if(this->ldx2Folder.open("ldx2/firmware") == true && this->ldx2Folder.getFileList("fir|opt")->getLength() != 0)
	{
		//Limpa a tela
		this->inputOutput->display.clearScreen();
		if(MessageDialog::showConfirmationDialog(Resources::getTexto(Resources::DESEJA_ATUALIZAR_FW), this->inputOutput, false, 10))
		{
			//Limpa a tela
			this->inputOutput->display.clearScreen();
			//Imprime as informa��es fixas da tela
			this->inputOutput->display.print(Resources::getTexto(Resources::ATUALIZANDO));
			//Liga a progress bar
			gui::GUI::setEnableProgressBar(true);
			gui::GUI::resetProgressBar();
			//Copia arquivos de firmware para a mem�ria flash
			Fachada::Resultado resultado = this->fachada->penDriveUpdateFW("firmware", GUI::incrementProgressBar);
			//Desliga a progress bar
			gui::GUI::setEnableProgressBar(false);
			//Indica ao usu�rio o resultado
			MessageDialog::showMessageDialog(resultado, inputOutput, 2000);
			//Finaliza
			this->finish();
			this->visible = false;

			return;
		}
	}
	//Limpa a tela
	this->inputOutput->display.clearScreen();
	//Imprime as informa��es fixas da tela
	this->inputOutput->display.print(Resources::getTexto(Resources::NOVA_CONFIG));
	//Abre o diret�rio
	this->arquivoSelecionado = 0;
	String::strcpy(this->pathArquivoSelecionado, "ldx2/");
	if(this->ldx2Folder.open("ldx2") == false || this->ldx2Folder.getFileList("b12|nfx")->getLength() == 0){
		//Indica que n�o tem nenhum arquivo
		this->inputOutput->display.setLinhaColuna(0,1);
		this->inputOutput->display.print(Resources::getTexto(Resources::NENHUM_ARQUIVO));
		//Espera o pendrive ser removido
		while(this->inputOutput->usbHost->getStatusUSB() != escort::service::USBHost::USB_DISCONNECTED){
			vTaskDelay(50);
		}
		//Fecha a tela
		this->visible = false;
	}
}

void TelaNovaConfiguracao::finish()
{
	//Indica ao usu�rio para remover o pendrive
	this->inputOutput->display.clearScreen();
	this->inputOutput->display.print(Resources::getTexto(Resources::REMOVA_PENDRIVE));
	//Espera o pendrive ser removido
	while(this->inputOutput->usbHost->getStatusUSB() != escort::service::USBHost::USB_DISCONNECTED){
		vTaskDelay(50);
	}
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
}
}
