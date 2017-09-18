/****************************************************************************
* Component: trf.pontos12.application.Controlador
****************************************************************************/
#include "config.h"

#ifdef __trf_pontos12_application_Controlador__
//************************************************
// Includes
//************************************************
#include "Controlador.h"
#include <freertos.kernel/FreeRTOSKernel/FreeRTOSKernel.h>
#include <trf.service.net/Network/Network.h>
#include <trf.service.net\IdentificationProtocol\IdentificationProtocol.h>
#include <escort.util/String.h>
#include "trf.pontos12.application/Controlador/gui/Resources.h"
#include <escort.driver\IRS485Driver\IRS485Driver.h>
#include <trf.pontos12.application/Controlador/gui/InputOutput.h>

using trf::service::net::ConnectionClient;
using escort::hal::ICPU;
using escort::driver::IKeypad;
using escort::driver::ILCDDisplay;
using escort::service::Relogio;
using trf::application::IComunicacao;
using escort::service::NandFFS;
using escort::service::USBHost;
using escort::util::String;
using escort::hal::IGPIOPin;
using escort::driver::IDriverFlash;
using escort::driver::IRS485Driver;
using trf::service::net::IdentificationProtocol;
using trf::service::net::Network;
using escort::driver::IUARTDriver;
using trf::pontos12::application::gui::GUI;


extern trf::application::Version trf_pontos12_application_Controlador_versao;

//************************************************
// Functions
//************************************************

//TODO: colocar no LOG toda opera��o com teclados


namespace trf {
namespace pontos12 {
namespace application {

static Controlador* _controlador;

void Controlador_monitoramentoTask(void* arg)
{
	((Controlador*)arg)->monitoramentoTask();
}

void Controlador_rs485Task(void* arg)
{
	((Controlador*)arg)->rs485Task();
}

void Controlador_agendaTask(void* arg)
{
	((Controlador*)arg)->agendaTask();
}

bool Controlador_cancelaOperacaoNornal(){
	return _controlador->cancelaOperacaoNormal();
}

Controlador::Controlador(
	ICPU* cpu,
	IGPIOPin* sinalEmergencia,
	IKeypad* teclado,
	ILCDDisplay* display,
	IDriverFlash* e2prom,
	IUARTDriver* uart,
	Relogio* relogio,
	Network* network,
	IdentificationProtocol* idProtocol,
	NandFFS* fileSystem,
	USBHost* usbHost,
	IRS485Driver* rs485) :
		plataforma(cpu, e2prom, uart, fileSystem, usbHost, relogio, rs485),
		fachada(network, idProtocol, &plataforma),
		gui(fileSystem, &fachada, teclado, display, usbHost, sinalEmergencia),
		interfaceCAN(network, idProtocol),
		protocoloCAN(&interfaceCAN, &plataforma, &fachada),
		interfaceUART(uart),
		protocoloUART(&interfaceUART, &plataforma, &fachada)
{
	_controlador = this;
}

void Controlador::init()
{
	this->fachada.init();
	this->gui.init();
	this->protocoloCAN.init();
	this->protocoloUART.init();

	//Cria tarefa do RS485
	xTaskCreate(
			(pdTASK_CODE)Controlador_rs485Task,
			(const signed char*)"rs485",
			216,
			(void*)this,
			(freertos_kernel_FreeRTOSKernel_maxPriorities - 3),
			0);

	vTaskDelay(1500);

	//Verifica e carrega os arquivos de configura��o
	this->gui.inputOutput.display.open();
	this->gui.inputOutput.display.clearScreen();
	this->gui.inputOutput.display.print("VERIFYING       ");
	gui::GUI::setEnableProgressBar(true);
	gui::GUI::resetProgressBar();
	if(this->fachada.getStatusFuncionamento() != Fachada::ERRO_SISTEMA_ARQUIVO){
		this->fachada.carregarConfiguracao(gui::GUI::incrementProgressBar);
	}
	gui::GUI::setEnableProgressBar(false);
	this->gui.inputOutput.display.clearScreen();
	this->gui.inputOutput.display.close();

	//Configura baudrate da porta serial
	if(this->fachada.getBaudrateSerial() == 0)
	{
		this->plataforma.uart->setBaudrate(115200);
	}
	else
	{
		this->plataforma.uart->setBaudrate(57600);
	}

	//Inicia a opera��o da GUI
	this->gui.start();

	//Cria a tarefa de monitoramento dos paineis
	xTaskHandle taskHandle;
	xTaskCreate(
			(pdTASK_CODE)Controlador_monitoramentoTask,
			(const signed char*)"contrl",
			620,
			(void*)this,
			trf_pontos12_application_Controlador_monitoramentoTaskPriority,
			&taskHandle);

	//Checa se o sistema est� funcionado corretamente
	if(this->fachada.getStatusFuncionamento() == Fachada::FUNCIONANDO_OK)
	{
		//Cria tarefa de gerenciamento dos agendamentos
		xTaskCreate(
				(pdTASK_CODE)Controlador_agendaTask,
				(const signed char*)"agenda",
				128,
				(void*)this,
				(freertos_kernel_FreeRTOSKernel_maxPriorities - 3),
				0);
	}
}

bool Controlador::cancelaOperacaoNormal()
{
	//Se houver alguma intera��o do usu�rio (tecla pressionada ou pendrive inserido)
	//ou se saiu do status de funcionamento OK
	//ent�o a opera��o deve ser cancelada
	return this->gui.inputOutput.teclado.hasEventoTeclado()
			|| this->gui.inputOutput.usbHost->getStatusUSB() != this->gui.inputOutput.usbHost->USB_DISCONNECTED
			|| this->fachada.getStatusFuncionamento() != this->fachada.FUNCIONANDO_OK;
}

void Controlador::monitoramentoTask()
{
	//Aguarda o dispositivo atribuir um endere�o na rede
	while(!this->fachada.isAddressAssigned()){
		vTaskDelay(10);
	}

	//Loop infinito
	while(true){
		static enum {
			INICIO,
			NORMAL,
			CONFIGURANDO,
			ERRO
		} estado = INICIO;

		switch(estado){
		case NORMAL:
			{
				//Verifica se existe algum painel com sincroniza��o pendente
				bool sincronizacaoPendente = false;
				U8 qntPaineis = this->fachada.getQntPaineis();
				for (U8 indicePainel = 0; indicePainel < qntPaineis; ++indicePainel) {
					if(this->fachada.isPainelSincronizado(indicePainel) == false){
						sincronizacaoPendente = true;
						break;
					}
				}

				if(sincronizacaoPendente){
					//Sincroniza os paineis
					this->sincronizarPaineis();
				}
				else if(!this->fachada.isPaineisTravados() && this->fachada.deveTravarPaineis()){
					//Verifica se deve travar os pain�is
					this->fachada.travarPaineis();
				}
				else if(this->fachada.isAPPHabilitado()
						&& this->fachada.getAPPStatusConfig() != Fachada::STATUS_CONFIG_APP_SINCRONIZADO)
				{
					//Sincroniza o APP
					this->sincronizarAPP();
				}
				else if(this->fachada.isAdaptadorCatracaHabilitado()
						&& this->fachada.isCatracaSincronizada() == false)
				{
					//Sincroniza o Adaptador da Catraca
					this->sincronizarAdaptadorCatraca();
				}
				else if(this->fachada.isAdaptadorSensoresHabilitado()
						&& this->fachada.isSensoresSincronizados() == false)
				{
					//Sincroniza o Adaptador da Placa de Sensores
					this->sincronizarAdaptadorSensores();
				}
				else
				{
					//Checa o status dos paineis
					this->checarStatusPaineis();
#ifdef TURBUS
					//Envia String para os pain�is
					this->enviarTurbusString();
#endif
					//Checa o status do APP
					if(this->fachada.isAPPHabilitado()){
						this->checarStatusAPP();
					}
					//Checa o status do Adaptador
					if(this->fachada.isAdaptadorCatracaHabilitado()){
						this->checarStatusAdaptadorCatraca();
					}
				}
			}
			break;
		case CONFIGURANDO:
			{
				//Obtem o controle do display
				this->gui.inputOutput.display.open();
				//Verifica se parou de configurar (tem um timeout)
				if(this->fachada.isConfigurandoTimedout()){
					this->fachada.setStatusFuncionamento(Fachada::ERRO_CONFIGURACAO_INCONSISTENTE);
					//Desliga a progress bar
					gui::GUI::setEnableProgressBar(false);
				}
				else{
					//Incrementa a barra de progresso
					gui::GUI::incrementProgressBar();
				}
				//Libera o controle do display
				this->gui.inputOutput.display.close();
			}
			break;
		}

		//Verifica o status de funcionamento
		if(this->fachada.getStatusFuncionamento() == Fachada::FUNCIONANDO_OK){
			estado = NORMAL;
		}
		else if(this->fachada.getStatusFuncionamento() & Fachada::CONFIGURANDO_REMOTAMENTE){
			if(estado != CONFIGURANDO){
				//Indica ao usu�rio que est� ocorrendo uma atualiza��o
				this->gui.inputOutput.display.clearScreen();
				this->gui.inputOutput.display.print(gui::Resources::getTexto(gui::Resources::ATUALIZANDO));
				gui::GUI::setEnableProgressBar(true);
				gui::GUI::resetProgressBar();
			}
			estado = CONFIGURANDO;
		}
		else {
			estado = ERRO;
		}

		vTaskDelay(250);
	}
}

void Controlador::rs485Task()
{
	this->plataforma.rs485->open();
	this->plataforma.rs485->setMode(this->plataforma.rs485->READ_MODE);

	while(true){
		this->fachada.responderRS485();
		vTaskDelay(50);
	}
}

void Controlador::agendaTask()
{
	portTickType lastTime = xTaskGetTickCount();

	while(true){
		this->fachada.tratarAgendamentos();
		vTaskDelayUntil(&lastTime, 1000);
	}
}

Controlador::SerialNumber Controlador::getSerialNumber()
{
	return this->fachada.getSerialNumber();
}

bool Controlador::provideService(char* serviceName)
{
	return String::equals(serviceName, trf_pontos12_application_Controlador_versao.produto);
}

void Controlador::sincronizarPaineis()
{
	static portTickType lastTime = 0;

	//S� executa a checagem de 5 em 5 segundo
	if((lastTime + 5000) > xTaskGetTickCount()){
		return;
	}
	lastTime = xTaskGetTickCount();

	//Verifica antes se a opera��o foi cancelada
	if(Controlador_cancelaOperacaoNornal()){
		return;
	}

	//Obtem o controle do display
	this->gui.inputOutput.display.open();

	U8 qntPaineis = this->fachada.getQntPaineis();
	U8 qntPaineisEmparelhados = this->fachada.getQntPaineisEmparelhados();
	for (U8 indicePainel = 0; indicePainel < qntPaineis; ++indicePainel) {
		if(this->fachada.isPainelSincronizado(indicePainel) == false){
			bool addrOK = false;
			//Se a quantidade de paineis emparelhados for diferente da quantidade de paineis da configura��o
			// ent�o considera que os paineis precisam ser emparelhados novamente e n�o detecta os paineis
			if(qntPaineis == qntPaineisEmparelhados){
				//Verifica se o painel est� respondendo (atualiza o seu endere�o de rede)
				//Obs.: tenta 3 vezes
				for (U8 retry = 0; retry < 3 && !addrOK; ++retry) {
					//Obt�m o n� de s�rie do painel
					trfProduct::SerialNumber serialNumber;
					*(U64*)&serialNumber = this->fachada.getPainelNumserie(indicePainel);
					if((*(U64*)&serialNumber) != 0 && (*(U64*)&serialNumber) != (U64)-1){
						//Pergunta na rede quem tem este n�mero de s�rie
						IdentificationProtocol::WhoIsReply* reply = this->fachada.whoIs(serialNumber, 200);
						while(reply->getStatus() == IdentificationProtocol::WhoIsReply::WAITING){
							vTaskDelay(1);
						}
						//Se houve resposta ent�o o painel est� presente
						if(reply->getStatus() == IdentificationProtocol::WhoIsReply::SUCCESS){
							if(reply->getAddress() == this->fachada.getPainelNetAddress(indicePainel)){
								addrOK = true;
							}
							else if(this->fachada.setPainelNetAddress(indicePainel, reply->getAddress()) == Fachada::SUCESSO){
								addrOK = true;
							}
						}
					}
				}
			}

			//Se o painel est� presente ent�o sincroniza
			if(addrOK){
				//Indica o in�cio da sincroniza��o
				this->gui.inputOutput.display.clearScreen();
				this->gui.inputOutput.display.print(gui::Resources::getTexto(gui::Resources::SINCRONIZANDO));
				this->gui.inputOutput.display.setLinhaColuna(14, 0);
				this->gui.inputOutput.display.print(indicePainel + 1, 2);
				//Liga a progress bar
				gui::GUI::setEnableProgressBar(true);
				gui::GUI::resetProgressBar();

				//Verifica se o painel precisa formatar antes de sincronizar
				if(this->fachada.isPainelPrecisandoFormatar(indicePainel)){
					if(this->fachada.apagarArquivosPainel(indicePainel, false) == Fachada::SUCESSO)
					{
						this->fachada.setPainelPrecisandoFormatar(indicePainel, false);
					}
					else{
						continue;
					}
				}

				//Sincroniza o painel
				bool sucesso = this->fachada.syncArquivosPainel(indicePainel, gui::GUI::incrementProgressBar, Controlador_cancelaOperacaoNornal);
				//Desliga a progress bar
				gui::GUI::setEnableProgressBar(false);
				//Indica o resultado
				if(!sucesso){
					//Indica que o painel N�O est� sincronizado
					this->fachada.setPainelSincronizado(indicePainel, false);
					if(!Controlador_cancelaOperacaoNornal()){
						//Mostra ao usu�rio que houve falha
						this->gui.inputOutput.display.clearScreen();
						this->gui.inputOutput.display.print(gui::Resources::getTexto(gui::Resources::FALHA));
						this->gui.inputOutput.display.setLinhaColuna(14, 0);
						this->gui.inputOutput.display.print(indicePainel + 1, 2);
						vTaskDelay(2000);
					}
					else{
						this->gui.inputOutput.display.clearScreen();
						this->gui.inputOutput.display.close();
						return;
					}
				}
				else{
					//Indica que o painel est� sincronizado
					this->fachada.setPainelSincronizado(indicePainel, true);
				}
			}
			else{
				//Indica ao usu�rio que n�o foi poss�vel detectar o painel
				this->gui.inputOutput.display.clearScreen();
				this->gui.inputOutput.display.print(gui::Resources::getTexto(gui::Resources::PAINEL_NAO_DETECTADO));
				int index = escort::util::String::indexOf(gui::Resources::getTexto(gui::Resources::PAINEL_NAO_DETECTADO), "xx");
				if(index < 0){
					index = 14;
				}
				this->gui.inputOutput.display.setLinhaColuna(index, 0);
				this->gui.inputOutput.display.print(indicePainel + 1, 2);
				//Aguarda at� 2 segundos se n�o tiver tecla pressionada
				for (int i = 0; i < 2000; ++i) {
					vTaskDelay(1);
					if(Controlador_cancelaOperacaoNornal())
					{
						this->gui.inputOutput.display.close();
						return;
					}
				}
				//Atualiza a vari�vel de tempo para que esse m�todo n�o monopolize o LCD se houver muitos pain�is
				lastTime = xTaskGetTickCount();
			}
		}
	}

	//Verifica se todos os paineis foram sincronizados com sucesso
	bool sincronizacaoSucesso = true;
	for (U8 indicePainel = 0; indicePainel < qntPaineis; ++indicePainel) {
		if(this->fachada.isPainelSincronizado(indicePainel) == false){
			sincronizacaoSucesso = false;
			break;
		}
	}
	if(sincronizacaoSucesso){
		this->gui.inputOutput.display.clearScreen();
		this->gui.inputOutput.display.print(gui::Resources::getTexto(gui::Resources::SUCESSO));
		vTaskDelay(2000);
	}

	//Libera o controle do display
	this->gui.inputOutput.display.close();
}

void Controlador::checarStatusPaineis()
{
	static portTickType lastTime = 0;

	//S� executa a checagem de 5 em 5 segundo
	if((lastTime + 5000) > xTaskGetTickCount()){
		return;
	}
	lastTime = xTaskGetTickCount();

	if(Controlador_cancelaOperacaoNornal()){
		return;
	}

	this->gui.inputOutput.display.open();
	
	U8 qntPaineis = this->fachada.getQntPaineis();
	for (U8 indicePainel = 0; indicePainel < qntPaineis; ++indicePainel) {
		U32 painelStatus = 0;

		//Verifica se o painel est� conectado na rede
		if(this->fachada.isPainelNaRede(indicePainel) == false){
			//Indica ao usu�rio que n�o foi poss�vel detectar o painel
			this->gui.inputOutput.display.clearScreen();
			this->gui.inputOutput.display.print(gui::Resources::getTexto(gui::Resources::PAINEL_NAO_DETECTADO));
			int index = escort::util::String::indexOf(gui::Resources::getTexto(gui::Resources::PAINEL_NAO_DETECTADO), "xx");
			if(index < 0){
				index = 14;
			}
			this->gui.inputOutput.display.setLinhaColuna(index, 0);
			this->gui.inputOutput.display.print(indicePainel + 1, 2);
			//Aguarda at� 2 segundos se n�o tiver tecla pressionada
			for (int i = 0; i < 2000; ++i) {
				vTaskDelay(1);
				if(Controlador_cancelaOperacaoNornal())
				{
					this->gui.inputOutput.display.close();
					return;
				}
			}
			//Atualiza a vari�vel de tempo para que esse m�todo n�o monopolize o LCD se houver muitos pain�is
			lastTime = xTaskGetTickCount();
		}
		//L� o status de funcionamento do painel
		else if(this->fachada.getPainelStatusFuncionamento(indicePainel, &painelStatus, Controlador_cancelaOperacaoNornal) == Fachada::SUCESSO){
			//Verifica se o painel N�O est� funcionando corretamente
			if(painelStatus != 0)
			{
				//A pedido de Raul, se o status de funcionamento do painel indicar
				//configura��o inconsistente ent�o o painel deve ser formatado e resincronizado.
				//Nesse caso n�s levamos em considera��o "configura��o inconsistente" e "funcionando parcialmente"
				//se n�o houver erro de senha anti-furto
				if((painelStatus & ((1 << 1) | (1 << 3)))
						&& (painelStatus & (1 << 2)) == 0)
				{
					this->fachada.setPainelPrecisandoFormatar(indicePainel, true);
					this->fachada.setPainelSincronizado(indicePainel, false);
				}
				//Indica ao usu�rio que o painel n�o est� funcionando corretamente				
				this->gui.inputOutput.display.clearScreen();
				this->gui.inputOutput.display.print(gui::Resources::getTexto(gui::Resources::PAINEL));
				this->gui.inputOutput.display.setLinhaColuna(14, 0);
				this->gui.inputOutput.display.print(indicePainel + 1, 2);
				this->gui.inputOutput.display.setLinhaColuna(0, 1);
				this->gui.inputOutput.display.print("ERR ");
				this->gui.inputOutput.display.print(painelStatus, 2);
				//Aguarda at� 4 segundos se n�o tiver tecla pressionada
				for (int i = 0; i < 4000; ++i) {
					vTaskDelay(1);
					if(Controlador_cancelaOperacaoNornal())
					{
						this->gui.inputOutput.display.close();
						return;
					}
				}
				
				break;
			}
		}
		else if(Controlador_cancelaOperacaoNornal()){
			this->gui.inputOutput.display.close();
			return;
		}
	}
	
	this->gui.inputOutput.display.close();
}

void Controlador::enviarTurbusString()
{
	static portTickType lastTime = 0;

	//S� envia a string de 60 em 60 segundos
	if(((lastTime + 60000) > xTaskGetTickCount()) ||
			//Em caso de overflow
			(xTaskGetTickCount() > (lastTime + 180000))){
		return;
	}
	lastTime = xTaskGetTickCount();

	if(Controlador_cancelaOperacaoNornal()){
		return;
	}

	this->gui.inputOutput.display.open();

	U8 qntPaineis = this->fachada.getQntPaineis();
	for (U8 indicePainel = 0; indicePainel < qntPaineis; ++indicePainel) {

		this->fachada.painelWrTexto(indicePainel, "TURBUS");
		if(Controlador_cancelaOperacaoNornal()){
			this->gui.inputOutput.display.close();
			return;
		}
	}

	this->gui.inputOutput.display.close();
}

void Controlador::sincronizarAPP()
{
	//Verifica antes se a opera��o foi cancelada
	if(Controlador_cancelaOperacaoNornal()){
		return;
	}

	//Obtem o controle do display
	this->gui.inputOutput.display.open();

	//Se o APP est� presente ent�o sincroniza
	if(this->fachada.isAPPHabilitado()){
		bool addrOK = false;
		//Verifica se o APP foi detectado na rede
		if(this->fachada.isAPPDetectado()){
			addrOK = true;
		}
		//Caso contr�rio tenta detectar o APP (atualiza o seu endere�o de rede)
		//Obs.: tenta 3 vezes
		else{
			for (U8 retry = 0; retry < 3; ++retry) {
				Fachada::Resultado res = this->fachada.detectarAPP(Controlador_cancelaOperacaoNornal);
				if(res == Fachada::SUCESSO){
					addrOK = true;
					break;
				}
				else if(res == Fachada::OPERACAO_CANCELADA){
					this->gui.inputOutput.display.clearScreen();
					this->gui.inputOutput.display.close();
					return;
				}
			}
		}

		//Se o APP est� presente ent�o sincroniza
		if(addrOK){
			//Indica o in�cio da sincroniza��o
			this->gui.inputOutput.display.clearScreen();
			this->gui.inputOutput.display.print(gui::Resources::getTexto(gui::Resources::SINCRONIZANDO));
			this->gui.inputOutput.display.setLinhaColuna(13, 0);
			this->gui.inputOutput.display.print("NSS");
			//Liga a progress bar
			gui::GUI::setEnableProgressBar(true);
			gui::GUI::resetProgressBar();
			//Realiza a sincroniza��o
			bool sucesso = this->fachada.sincronizarAPP(gui::GUI::incrementProgressBar, Controlador_cancelaOperacaoNornal);
			//Desliga a progress bar
			gui::GUI::setEnableProgressBar(false);
			//Indica o resultado
			if(!sucesso){
				if(!Controlador_cancelaOperacaoNornal()){
					//Mostra ao usu�rio que houve uma falha
					this->gui.inputOutput.display.clearScreen();
					this->gui.inputOutput.display.print(gui::Resources::getTexto(gui::Resources::FALHA));
					vTaskDelay(2000);
				}
				else{
					this->gui.inputOutput.display.clearScreen();
				}
			}
			else{
				this->gui.inputOutput.display.clearScreen();
				this->gui.inputOutput.display.print(gui::Resources::getTexto(gui::Resources::SUCESSO));
				vTaskDelay(2000);
			}
		}
		else{
			//Indica ao usu�rio que n�o foi poss�vel detectar o APP
			this->gui.inputOutput.display.clearScreen();
			this->gui.inputOutput.display.print(gui::Resources::getTexto(gui::Resources::APP_NAO_DETECTADO));
			//Aguarda at� 4 segundos se n�o tiver tecla pressionada
			for (int i = 0; i < 4000; ++i) {
				vTaskDelay(1);
				if(Controlador_cancelaOperacaoNornal())
				{
					break;
				}
			}
		}
	}

	//Libera o controle do display
	this->gui.inputOutput.display.close();
}

void Controlador::checarStatusAPP()
{
	static portTickType lastTime = 0;

	//S� executa a checagem de 5 em 5 segundos
	if((lastTime + 5000) > xTaskGetTickCount()){
		return;
	}
	lastTime = xTaskGetTickCount();

	if(Controlador_cancelaOperacaoNornal()){
		return;
	}

	this->gui.inputOutput.display.open();

	bool addrOK = false;
	//Verifica se o APP foi detectado na rede
	if(this->fachada.isAPPDetectado()){
		addrOK = true;
	}
	//Caso contr�rio tenta detectar o APP (atualiza o seu endere�o de rede)
	//Obs.: tenta 3 vezes
	else{
		for (U8 retry = 0; retry < 3; ++retry) {
			Fachada::Resultado res = this->fachada.detectarAPP(Controlador_cancelaOperacaoNornal);
			if(res == Fachada::SUCESSO){
				addrOK = true;
				break;
			}
			else if(res == Fachada::OPERACAO_CANCELADA){
				this->gui.inputOutput.display.clearScreen();
				this->gui.inputOutput.display.close();
				return;
			}
		}
	}

	U32 APPStatus = 0;

	//Verifica se o APP est� conectado na rede
	if((addrOK == false) || (this->fachada.isAPPNaRede() == false)){
		//Indica ao usu�rio que n�o foi poss�vel detectar o APP
		this->gui.inputOutput.display.clearScreen();
		this->gui.inputOutput.display.print(gui::Resources::getTexto(gui::Resources::APP_NAO_DETECTADO));
		//Aguarda at� 4 segundos se n�o tiver tecla pressionada
		for (int i = 0; i < 4000; ++i) {
			vTaskDelay(1);
			if(Controlador_cancelaOperacaoNornal())
			{
				this->gui.inputOutput.display.close();
				return;
			}
		}
	}
	//L� o status de funcionamento do APP
	else if(this->fachada.getAPPStatusFuncionamento(&APPStatus, Controlador_cancelaOperacaoNornal) == Fachada::SUCESSO){
		//Verifica se o APP N�O est� funcionando corretamente
		if(APPStatus != 0)
		{
			//Indica ao usu�rio que o APP n�o est� funcionando corretamente
			this->gui.inputOutput.display.clearScreen();
			this->gui.inputOutput.display.print(gui::Resources::getTexto(gui::Resources::APP));
			this->gui.inputOutput.display.setLinhaColuna(0, 1);
			this->gui.inputOutput.display.print("ERR ");
			this->gui.inputOutput.display.print(APPStatus, 2);
			//Aguarda at� 4 segundos se n�o tiver tecla pressionada
			for (int i = 0; i < 4000; ++i) {
				vTaskDelay(1);
				if(Controlador_cancelaOperacaoNornal())
				{
					this->gui.inputOutput.display.close();
					return;
				}
			}
		}
	}
	else if(Controlador_cancelaOperacaoNornal()){
		this->gui.inputOutput.display.close();
		return;
	}

	this->gui.inputOutput.display.close();
}

void Controlador::sincronizarAdaptadorCatraca()
{
	//Verifica antes se a opera��o foi cancelada
	if(Controlador_cancelaOperacaoNornal()){
		return;
	}

	//Obtem o controle do display
	this->gui.inputOutput.display.open();

	//Se o Adaptador est� presente ent�o sincroniza
	if(this->fachada.isAdaptadorCatracaHabilitado() && this->fachada.isCatracaSincronizada() == false){
		bool addrOK = false;
		//Verifica se o Adaptador foi detectada na rede
		if(this->fachada.isAdaptadorCatracaDetectado()){
			addrOK = true;
		}
		//Caso contr�rio tenta detectar o Adaptador (atualiza o seu endere�o de rede)
		//Obs.: tenta 3 vezes
		else{
			for (U8 retry = 0; retry < 3; ++retry) {
				Fachada::Resultado res = this->fachada.detectarAdaptadorCatraca(Controlador_cancelaOperacaoNornal);
				if(res == Fachada::SUCESSO){
					addrOK = true;
					break;
				}
				else if(res == Fachada::OPERACAO_CANCELADA){
					this->gui.inputOutput.display.clearScreen();
					this->gui.inputOutput.display.close();
					return;
				}
			}
		}

		//Se o Adaptador est� presente ent�o sincroniza
		if(addrOK){
			//Indica o in�cio da sincroniza��o
			this->gui.inputOutput.display.clearScreen();
			this->gui.inputOutput.display.print(gui::Resources::getTexto(gui::Resources::SINCRONIZANDO));
			this->gui.inputOutput.display.setLinhaColuna(7, 0);
			this->gui.inputOutput.display.print("Adaptador");
			//Liga a progress bar
			gui::GUI::setEnableProgressBar(true);
			gui::GUI::resetProgressBar();
			//Realiza a sincroniza��o
			bool sucesso = this->fachada.sincronizarAdaptadorCatraca(gui::GUI::incrementProgressBar, Controlador_cancelaOperacaoNornal);
			//Desliga a progress bar
			gui::GUI::setEnableProgressBar(false);
			//Indica o resultado
			if(!sucesso){
				if(!Controlador_cancelaOperacaoNornal()){
					//Mostra ao usu�rio que houve uma falha
					this->gui.inputOutput.display.clearScreen();
					this->gui.inputOutput.display.print(gui::Resources::getTexto(gui::Resources::FALHA));
					vTaskDelay(2000);
				}
				else{
					this->gui.inputOutput.display.clearScreen();
				}
			}
			else{
				this->gui.inputOutput.display.clearScreen();
				this->gui.inputOutput.display.print(gui::Resources::getTexto(gui::Resources::SUCESSO));
				//Indica que o painel est� sincronizado
				this->fachada.setCatracaSincronizada(true);
				vTaskDelay(2000);
			}
		}
		else{
			//Indica ao usu�rio que n�o foi poss�vel detectar o Adaptador
			this->gui.inputOutput.display.clearScreen();
			this->gui.inputOutput.display.print("Adapt nao detect");
			//Aguarda at� 4 segundos se n�o tiver tecla pressionada
			for (int i = 0; i < 4000; ++i) {
				vTaskDelay(1);
				if(Controlador_cancelaOperacaoNornal())
				{
					break;
				}
			}
		}
	}

	//Libera o controle do display
	this->gui.inputOutput.display.close();
}

void Controlador::sincronizarAdaptadorSensores()
{
	//Verifica antes se a opera��o foi cancelada
	if(Controlador_cancelaOperacaoNornal()){
		return;
	}

	//Obtem o controle do display
	this->gui.inputOutput.display.open();

	//Se o Adaptador est� presente ent�o sincroniza
	if(this->fachada.isAdaptadorSensoresHabilitado() && this->fachada.isSensoresSincronizados() == false){
		bool addrOK = false;
		//Tenta detectar o Adaptador da placa de sensores(atualiza o seu endere�o de rede)
		//Obs.: tenta 3 vezes
		for (U8 retry = 0; retry < 3; ++retry) {
			Fachada::Resultado res = this->fachada.detectarAdaptadorSensores(Controlador_cancelaOperacaoNornal);
			if(res == Fachada::SUCESSO){
				addrOK = true;
				break;
			}
			else if(res == Fachada::OPERACAO_CANCELADA){
				this->gui.inputOutput.display.clearScreen();
				this->gui.inputOutput.display.close();
				return;
			}
		}

		//Se o Adaptador est� presente ent�o sincroniza
		if(addrOK){
			//Indica o in�cio da sincroniza��o
			this->gui.inputOutput.display.clearScreen();
			this->gui.inputOutput.display.print(gui::Resources::getTexto(gui::Resources::SINCRONIZANDO));
			this->gui.inputOutput.display.setLinhaColuna(7, 0);
			this->gui.inputOutput.display.print("Adaptador");
			//Liga a progress bar
			gui::GUI::setEnableProgressBar(true);
			gui::GUI::resetProgressBar();
			//Realiza a sincroniza��o
			bool sucesso = this->fachada.sincronizarAdaptadorSensores(gui::GUI::incrementProgressBar, Controlador_cancelaOperacaoNornal);
			//Desliga a progress bar
			gui::GUI::setEnableProgressBar(false);
			//Indica o resultado
			if(!sucesso){
				if(!Controlador_cancelaOperacaoNornal()){
					//Mostra ao usu�rio que houve uma falha
					this->gui.inputOutput.display.clearScreen();
					this->gui.inputOutput.display.print(gui::Resources::getTexto(gui::Resources::FALHA));
					vTaskDelay(2000);
				}
				else{
					this->gui.inputOutput.display.clearScreen();
				}
			}
			else{
				this->gui.inputOutput.display.clearScreen();
				this->gui.inputOutput.display.print(gui::Resources::getTexto(gui::Resources::SUCESSO));
				//Indica que o painel est� sincronizado
				this->fachada.setSensoresSincronizados(true);
				vTaskDelay(2000);
			}
		}
		else{
			//Indica ao usu�rio que n�o foi poss�vel detectar o Adaptador
			this->gui.inputOutput.display.clearScreen();
			this->gui.inputOutput.display.print("Adapt nao detect");
			//Aguarda at� 4 segundos se n�o tiver tecla pressionada
			for (int i = 0; i < 4000; ++i) {
				vTaskDelay(1);
				if(Controlador_cancelaOperacaoNornal())
				{
					break;
				}
			}
		}
	}

	//Libera o controle do display
	this->gui.inputOutput.display.close();
}

void Controlador::checarStatusAdaptadorCatraca()
{
	static portTickType lastTime = 0;

	//S� executa a checagem de 5 em 5 segundos
	if((lastTime + 5000) > xTaskGetTickCount()){
		return;
	}
	lastTime = xTaskGetTickCount();

	if(Controlador_cancelaOperacaoNornal()){
		return;
	}

	this->gui.inputOutput.display.open();

	bool addrOK = false;
	//Verifica se o Adaptador foi detectada na rede
	if(this->fachada.isAdaptadorCatracaDetectado()){
		addrOK = true;
	}
	//Caso contr�rio tenta detectar o Adaptador (atualiza o seu endere�o de rede)
	//Obs.: tenta 3 vezes
	else{
		for (U8 retry = 0; retry < 3; ++retry) {
			Fachada::Resultado res = this->fachada.detectarAdaptadorCatraca(Controlador_cancelaOperacaoNornal);
			if(res == Fachada::SUCESSO){
				addrOK = true;
				break;
			}
			else if(res == Fachada::OPERACAO_CANCELADA){
				this->gui.inputOutput.display.clearScreen();
				this->gui.inputOutput.display.close();
				return;
			}
		}
	}

	//Verifica se o Adaptador est� conectado na rede
	if((addrOK == false) || (this->fachada.isAdaptadorCatracaNaRede() == false)){
		//Indica ao usu�rio que n�o foi poss�vel detectar o Adaptador
		this->gui.inputOutput.display.clearScreen();
		this->gui.inputOutput.display.print("Adapt nao detect");
		//Aguarda at� 4 segundos se n�o tiver tecla pressionada
		for (int i = 0; i < 4000; ++i) {
			vTaskDelay(1);
			if(Controlador_cancelaOperacaoNornal())
			{
				this->gui.inputOutput.display.close();
				return;
			}
		}
	}
	else if(Controlador_cancelaOperacaoNornal()){
		this->gui.inputOutput.display.close();
		return;
	}

	this->gui.inputOutput.display.close();
}

}
}
}

#endif // __trf_pontos12_application_Controlador__
