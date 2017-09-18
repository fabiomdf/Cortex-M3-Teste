/*
 * GUI.cpp
 *
 *  Created on: 30/05/2012
 *      Author: Gustavo
 */

#include "GUI.h"
#include <config.h>
#include <freertos.kernel/FreeRTOSKernel/FreeRTOSKernel.h>
#include <escort.util/Math.h>
#include <escort.util/String.h>
#include <escort.util/Converter.h>
#include <escort.service/Relogio/Relogio.h>
#include <escort.service/USBHost/USBHost.h>
#include "Resources.h"
#include "MessageDialog.h"

using escort::util::Math;
using escort::util::String;
using escort::util::Converter;
using escort::service::Relogio;
using escort::service::USBHost;

#define NOME_PRODUTO "pontos"

namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

bool GUI::enableProgressBar;
U8 GUI::progressBarValue;

void GUI_task(void* arg)
{
	((GUI*)arg)->task();
}

void GUI_progressBarTask(void* arg)
{
	((GUI*) arg)->progressBarTask();
}

void GUI::setEnableProgressBar(bool enable)
{
	GUI::enableProgressBar = enable;
	vTaskDelay(50);
}

void GUI::resetProgressBar()
{
	GUI::progressBarValue = 0;
}

void GUI::incrementProgressBar()
{
	GUI::progressBarValue++;
	if(GUI::progressBarValue >= 16){
		GUI::progressBarValue = 0;
	}
}

GUI::GUI(
	escort::service::NandFFS* sistemaArquivo,
	Fachada* fachada,
	escort::driver::IKeypad* teclado,
	escort::driver::ILCDDisplay* display,
	escort::service::USBHost* usbHost,
	escort::hal::IGPIOPin* sinalEmergencia) :
		inputOutput(teclado, display, usbHost),
		sinalEmergencia(sinalEmergencia),
		telaRoteiros(fachada, &inputOutput),
		telaMensagens(fachada, &inputOutput),
		telaPaineis(fachada, &inputOutput),
		telaSentido(fachada, &inputOutput),
		telaAlternarCom(fachada, &inputOutput),
		telaOpcoes(fachada, &inputOutput),
		telaNovaConfiguracao(fachada, &inputOutput),
		fachada(fachada),
		sistemaArquivo(sistemaArquivo)
{
}

void GUI::init()
{
	//Inicializa o sinal de emerg�ncia
	this->sinalEmergencia->setDirection(this->sinalEmergencia->INPUT);
	//Inicializa o teclado
	this->inputOutput.teclado.init();
	//Inicializa o display
	this->inputOutput.display.init();

	//Exibe a vers�o do produto (splash)
	this->inputOutput.display.open();
	this->printpontosVersion();
	this->inputOutput.display.close();

	//FIXME - deixar tempo configuravel
	//Seta tempo que o USB ficar� sem alimenta��o ap�s tentativa falha de detectar usb drive
	// 1 minuto
	this->inputOutput.usbHost->setPowerOffTime(1 * 60000);
	//Cria o sem�foro
	vSemaphoreCreateBinary(this->semaforo);
	xSemaphoreTake(this->semaforo, 0);

	//Inicia tarefa
	xTaskCreate(
			(pdTASK_CODE)GUI_task,
			(const signed char*)"gui",
			1024,
			(void*)this,
			trf_pontos12_application_Controlador_GUItaskPriority,
			0);
	xTaskCreate(
			(pdTASK_CODE)GUI_progressBarTask,
			(const signed char*)"progbar",
			128,
			(void*)this,
			trf_pontos12_application_Controlador_GUItaskPriority,
			0);
}

void GUI::start()
{
	xSemaphoreGive(this->semaforo);
}

void GUI::waitStart()
{
	xSemaphoreTake(this->semaforo, portMAX_DELAY);
}

void GUI::printpontosVersion()
{
	U8 length;
	trf::application::Version* versao = this->fachada->getVersaoFirmware();
	this->inputOutput.display.clearScreen();

	//Imprime o nome do produto centralizado
	length = String::getLength(NOME_PRODUTO);
	for (U8 i = 0; i < (this->inputOutput.display.getWidth() - length)/2; ++i) {
		this->inputOutput.display.print(" ");
	}
	this->inputOutput.display.print(NOME_PRODUTO);
	for (U8 i = 0; i < (this->inputOutput.display.getWidth() - length) - ((this->inputOutput.display.getWidth() - length)/2); ++i) {
		this->inputOutput.display.print(" ");
	}

	this->inputOutput.display.println();

	//Imprime a vers�o do produto centralizado
	length = Converter::getNumberOfDigits(versao->familia, Converter::BASE_10)
			+ 1
			+ Converter::getNumberOfDigits(versao->major, Converter::BASE_10)
			+ 1
			+ Converter::getNumberOfDigits(versao->minor, Converter::BASE_10);
	for (U8 i = 0; i < (this->inputOutput.display.getWidth() - length)/2; ++i) {
		this->inputOutput.display.print(" ");
	}
	this->inputOutput.display.print((U32)versao->familia);
	this->inputOutput.display.print(".");
	this->inputOutput.display.print((U32)versao->major);
	this->inputOutput.display.print(".");
	this->inputOutput.display.print((U32)versao->minor);
	for (U8 i = 0; i < (this->inputOutput.display.getWidth() - length) - ((this->inputOutput.display.getWidth() - length)/2); ++i) {
		this->inputOutput.display.print(" ");
	}
}

void GUI::printIdleScreen1()
{
	U8 length;
	this->inputOutput.display.setLinhaColuna(0,0);

#ifdef TURBUS
	//Imprime o nome do produto centralizado
	length = String::getLength("TURBUS");
	for (U8 i = 0; i < (this->inputOutput.display.getWidth() - length)/2; ++i) {
		this->inputOutput.display.print(" ");
	}
	this->inputOutput.display.print("TURBUS");
	for (U8 i = 0; i < (this->inputOutput.display.getWidth() - length) - ((this->inputOutput.display.getWidth() - length)/2); ++i) {
		this->inputOutput.display.print(" ");
	}
#else
	//Imprime o nome do produto centralizado
	length = String::getLength(NOME_PRODUTO);
	for (U8 i = 0; i < (this->inputOutput.display.getWidth() - length)/2; ++i) {
		this->inputOutput.display.print(" ");
	}
	this->inputOutput.display.print(NOME_PRODUTO);
	for (U8 i = 0; i < (this->inputOutput.display.getWidth() - length) - ((this->inputOutput.display.getWidth() - length)/2); ++i) {
		this->inputOutput.display.print(" ");
	}
#endif

	this->inputOutput.display.println();

	//Imprime data e hora
	Relogio::DataHora dataHora = this->fachada->getDataHora();
	Regiao::FormatoDataHora formatoDataHora = this->fachada->getFormatoDataHora();
	char buffer[16];

	//Inicia a montagem
	U32 indice = 0;
	if(formatoDataHora == Regiao::FORMATO_AM_PM)
	{
		//"mm"
		Converter::itoa(dataHora.mes, buffer + indice, 2, Converter::BASE_10);
		indice += String::getLength(buffer + indice);
		//"mm/"
		buffer[indice] = '/';
		indice++;
		//"mm/dd"
		Converter::itoa(dataHora.dia, buffer + indice, 2, Converter::BASE_10);
		indice += String::getLength(buffer + indice);
	}
	else
	{
		//"dd"
		Converter::itoa(dataHora.dia, buffer + indice, 2, Converter::BASE_10);
		indice += String::getLength(buffer + indice);
		//"dd/"
		buffer[indice] = '/';
		indice++;
		//"dd/mm"
		Converter::itoa(dataHora.mes, buffer + indice, 2, Converter::BASE_10);
		indice += String::getLength(buffer + indice);
	}
	//"dd/mm "
	buffer[indice] = ' ';
	indice++;
	//"dd/mm hh"
	U8 horas;
	if((formatoDataHora == Regiao::FORMATO_AM_PM) && (dataHora.horas == 0)){
		horas = 12;
	} else if((formatoDataHora == Regiao::FORMATO_AM_PM) && (dataHora.horas > 12)) {
		horas = dataHora.horas - 12;
	} else {
		horas = dataHora.horas;
	}
	Converter::itoa(horas, buffer + indice, 2, Converter::BASE_10);
	indice += String::getLength(buffer + indice);
	//"dd/mm hh:"
	buffer[indice] = ':';
	indice++;
	//"dd/mm hh:mm"
	Converter::itoa(dataHora.minutos, buffer + indice, 2, Converter::BASE_10);
	indice += String::getLength(buffer + indice);
	//"dd/mm hh:mm[.]"
	if((formatoDataHora == Regiao::FORMATO_AM_PM) && ((dataHora.horas > 12) || (dataHora.horas == 0)))
	{
		buffer[indice] = '.';
		indice++;
	}

	//Imprime data e hora centralizados no display
	for (U8 i = 0; i < (this->inputOutput.display.getWidth() - indice)/2; ++i) {
		this->inputOutput.display.print(" ");
	}
	this->inputOutput.display.print(buffer, indice);
	for (U8 i = 0; i < (this->inputOutput.display.getWidth() - indice) - ((this->inputOutput.display.getWidth() - indice)/2); ++i) {
		this->inputOutput.display.print(" ");
	}
}

void GUI::printIdleScreen2()
{
	U8 length;
	char buffer[64];
	this->inputOutput.display.setLinhaColuna(0,0);
	U32 roteiroSelecionado = this->fachada->getRoteiroSelecionado();

	//Monta o r�tulo do n�mero do roteiro
	this->fachada->getLabelNumeroRoteiro(roteiroSelecionado, buffer);
	//Deixa 2 characters no final para informar Ida/Volta
	length = Math::min(this->inputOutput.display.getWidth() - 2, String::getLength(buffer));
	this->inputOutput.display.print(buffer, length);

	//Imprime espa�os em braco
	for(U32 i = length; i < this->inputOutput.display.getWidth() - 1; i++){
		this->inputOutput.display.print(" ");
	}

	//Imprime indica��o de Ida ou Volta
	char* sentido = (this->fachada->getSentidoRoteiro() == Fachada::IDA) ? Resources::getTexto(Resources::IDA) : Resources::getTexto(Resources::VOLTA);
	this->inputOutput.display.print(sentido[0]);

	this->inputOutput.display.println();

	//Monta o texto do roteiro
	if(this->fachada->getSentidoRoteiro() == Fachada::IDA)
	{
		this->fachada->getLabelRoteiroIda(roteiroSelecionado, buffer);
	}
	else
	{
		this->fachada->getLabelRoteiroVolta(roteiroSelecionado, buffer);
		//Se o label de Volta estiver vazio no arquivo, utiliza o de Ida
		if(buffer[0] == '\0')
		{
			this->fachada->getLabelRoteiroIda(roteiroSelecionado, buffer);
		}
	}
	length = Math::min(this->inputOutput.display.getWidth(), String::getLength(buffer));

	this->inputOutput.display.print(buffer, length);
	for(U32 i = length; i < this->inputOutput.display.getWidth(); i++){
		this->inputOutput.display.print(" ");
	}
}

void GUI::loadNewConfig()
{
	this->inputOutput.display.setLinhaColuna(0,0);
	this->inputOutput.display.print(Resources::getTexto(Resources::CARREGUE_CONFIG));
}

void GUI::qntdPaineisRede()
{
	U8 length;
	this->inputOutput.display.setLinhaColuna(0,0);

	char buffer[16];
	//Adiciona a quantidade de pain�is detectados
	U8 qntPaineisRede = this->fachada->getQntPaineisRede();
	Converter::itoa(qntPaineisRede, buffer);
	length = String::getLength(buffer);
	if(qntPaineisRede > 1)
	{
		String::strcpy(buffer + length, " pain�is");
	}
	else
	{
		String::strcpy(buffer + length, " painel");
	}
	length = Math::min(this->inputOutput.display.getWidth(), String::getLength(buffer));

	this->inputOutput.display.print(buffer, length);
	for(U32 i = length; i < this->inputOutput.display.getWidth(); i++){
		this->inputOutput.display.print(" ");
	}

	this->inputOutput.display.println();

	if(qntPaineisRede > 1)
	{
		this->inputOutput.display.print("detectados");
		length = String::getLength("detectados");
	}
	else
	{
		this->inputOutput.display.print("detectado");
		length = String::getLength("detectado");
	}
	for(U32 i = length; i < this->inputOutput.display.getWidth(); i++){
		this->inputOutput.display.print(" ");
	}
}

void GUI::printTelaInicial()
{
	//Verifica se h� erro no sistema de arquivo
	if(this->fachada->getStatusFuncionamento() & Fachada::ERRO_SISTEMA_ARQUIVO)
	{
		this->inputOutput.display.setLinhaColuna(0,0);
		this->inputOutput.display.print(Resources::getTexto(Resources::SISTEMA_ARQUIVO_DEFEITUOSO));
	}
	//Verifica se h� erro na configura��o atual
	else if(this->fachada->getStatusFuncionamento() & Fachada::ERRO_CONFIGURACAO_INCONSISTENTE)
	{
		portTickType idleTime = xTaskGetTickCount() % 6000;
		idleTime < 2000 ? this->loadNewConfig() : this->qntdPaineisRede();
	}
	//Verifica se h� erro de senha anti-furto
	else if(this->fachada->getStatusFuncionamento() & Fachada::ERRO_SENHA_ANTI_FURTO)
	{
		this->inputOutput.display.setLinhaColuna(0,0);
		this->inputOutput.display.print(Resources::getTexto(Resources::SENHA_ANTI_FURTO_INCORRETA));
	}
	else
	{
		portTickType idleTime = xTaskGetTickCount() % 10000;
		idleTime < 1000 ? this->printIdleScreen1() : this->printIdleScreen2();
	}
}

void GUI::tratarTeclas()
{
	//Trata eventos do teclado
	Teclado::Tecla tecla = inputOutput.teclado.getEventoTeclado();
	if(tecla)
	{
		//Verifica se a tecla "ROTEIRO ESQUERDA/DIREITA" foi pressionada
		if((tecla & Teclado::TECLA_ROTEIRO_ESQUERDA)
			|| (tecla & Teclado::TECLA_ROTEIRO_DIREITA))
		{
			//Exibe tela de sele��o de roteiro
			this->telaRoteiros.show();
		}
		//Verifica se a tecla "IDA/VOLTA" foi pressionada
		if(tecla & Teclado::TECLA_IDA_VOLTA)
		{
			//Exibe a tela de sele��o IDA/VOLTA
			this->telaSentido.show();
		}
		//Verifica se a tecla "ALTERNA COM" foi pressionada
		if(tecla & Teclado::TECLA_ALTERNA)
		{
			this->telaAlternarCom.show();
		}
		//Verifica se a tecla "MENSAGEM ESQUERDA/DIREITA" foi pressionada
		if((tecla & Teclado::TECLA_MENSAGEM_ESQUERDA)
			|| (tecla & Teclado::TECLA_MENSAGEM_DIREITA))
		{
			//Exibe tela de sele��o de mensagens
			this->telaMensagens.show();
		}
		//Verifica se a tecla "SELECIONA PAINEL" foi pressionada
		if(tecla & Teclado::TECLA_SELECIONA_PAINEL)
		{
			//Exibe tela de sele��o de paineis
			this->telaPaineis.show();
		}
		//Verifica se a tecla "AJUSTE ESQUERDA/DIREITA"
		if((tecla & Teclado::TECLA_AJUSTE_ESQUERDA)
			|| (tecla & Teclado::TECLA_AJUSTE_DIREITA))
		{
			//Exibe a tela de op��es
			this->telaOpcoes.show();
		}
	}
}

void GUI::progressBarTask()
{
	while(true){
		if(GUI::enableProgressBar){
			U8 value = GUI::progressBarValue;
			this->inputOutput.display.setLinhaColuna(0,1);
			for (int i = 0; i < 16; ++i) {
				if(i <= value){
					this->inputOutput.display.print(".");
				}
				else{
					this->inputOutput.display.print(" ");
				}
			}
		}
		vTaskDelay(50);
	}
}

void GUI::task()
{
	static enum {
		RECONHECENDO_USB,
		VERIFICANDO_USB,
		EXIBINDO_MENU_ARQUIVOS,
		ESPERANDO_TECLA
	}estado = ESPERANDO_TECLA;

	//Aguarda o boot do sistema
	this->waitStart();

	//Inicializa a classe Resources
	Resources::init(this->sistemaArquivo, this->fachada->getIdiomaPath());

	while(true)
	{
		//N�o executa o loop da gui se estiver em modo de configura��o remota
		if(this->fachada->getStatusFuncionamento() & Fachada::CONFIGURANDO_REMOTAMENTE){
			vTaskDelay(40);
			continue;
		}

		this->inputOutput.display.open();

		//Trata o sinal de emerg�ncia
		this->fachada->ativarModoEmergencia(this->sinalEmergencia->getStatus());

		do {
			switch (estado) {
				case RECONHECENDO_USB:
					// Indica ao usu�rio que est� reconhecendo dispositivo
					inputOutput.display.print(Resources::getTexto(Resources::RECONHECENDO_USB));
					if(this->inputOutput.usbHost->getStatusUSB() == USBHost::USB_CONNECTED && this->inputOutput.usbHost->isReady())
					{
						estado = VERIFICANDO_USB;
					}
					else if(this->inputOutput.usbHost->getStatusUSB() == USBHost::USB_DISCONNECTED)
					{
						estado = ESPERANDO_TECLA;
					}
					break;
				case VERIFICANDO_USB:
					// Aguarda um delay por seguran�a
					vTaskDelay(100);
					// Verifica se foi poss�vel inicializar o sistema de arquivos do pendrive com sucesso
					if(this->inputOutput.usbHost->fileSystem.wasInitiated())
					{
						estado = EXIBINDO_MENU_ARQUIVOS;
					}
					else
					{
						//Indica ao usuario que o dispositivo � icompat�vel
						inputOutput.display.clearScreen();
						inputOutput.display.print(Resources::getTexto(Resources::DISPOSITIVO_INCOMPATIVEL));
						vTaskDelay(2000);

						estado = ESPERANDO_TECLA;
					}
					break;
				case EXIBINDO_MENU_ARQUIVOS:
					// Exibe tela de sele��o de nova configura��o
					this->telaNovaConfiguracao.show();

					//Aguarda at� o pendrive ser retirado
					while(this->inputOutput.usbHost->getStatusUSB() == USBHost::USB_CONNECTED);

					estado = ESPERANDO_TECLA;
					break;
				case ESPERANDO_TECLA:
					//Imprime tela inicial
					this->printTelaInicial();
					//Trata as teclas
					this->tratarTeclas();

					//Se um dispositivo USB for plugado ent�o tenta reconhecer o dispositivo
					if(this->inputOutput.usbHost->getStatusUSB() == USBHost::USB_PLUGGED)
					{
						inputOutput.display.clearScreen();
						estado = RECONHECENDO_USB;
					}
					break;
				default:
					estado = ESPERANDO_TECLA;
					break;
			}
		}while(estado != ESPERANDO_TECLA);
		this->inputOutput.display.close();

		vTaskDelay(40);
	}
}

}
}
}
}
