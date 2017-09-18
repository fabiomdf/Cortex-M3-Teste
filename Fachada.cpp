/*
 * Fachada.cpp
 *
 *  Created on: 23/05/2012
 *      Author: arthur.padilha
 */

#include "Fachada.h"
#include "files/Roteiro.h"
#include "files/Mensagem.h"
#include "files/PainelConfig.h"
#include <trf.pontos12.service/Renderizacao/Renderizacao.h>
#include "trf.pontos12.service/Renderizacao/Video.h"
#include "trf.pontos12.service/Renderizacao/Video01.h"
#include "trf.pontos12.service/Renderizacao/Video02.h"
#include "trf.pontos12.service/Renderizacao/Video03.h"
#include "trf.pontos12.service/Renderizacao/Playlist.h"
#include "trf.pontos12.application/Controlador/files/RoteiroPaths.h"
#include "trf.pontos12.application/Controlador/files/MensagemPaths.h"
#include <escort.util/CRC16CCITT.h>
#include <escort.util/Log.h>
#include <escort.util/String.h>
#include <escort.util/Converter.h>
#include "trf.pontos12.service/Renderizacao/Video04.h"
#include "trf.pontos12.application/Controlador/files/Alternancias.h"
#include "trf.pontos12.service/Renderizacao/Fonte.h"
#include "trf.pontos12.application/Controlador/files/Idioma.h"
#include "escort.util/SHA256.h"
#include <trf.service.net\IdentificationProtocol\IdentificationProtocol.h>
#include <trf.service.net/Network/Network.h>
#include <escort.util/Math.h>
#include <trf.pontos12.application/Controlador/files/Agenda.h>
#include <trf.pontos12.application/Controlador/files/Motorista.h>
#include <trf.pontos12.application/Controlador/files/MotoristaPaths.h>
#include "app_files/PainelCfg.h"
#include "app_files/ConfigCfg.h"

using escort::service::Relogio;
using escort::service::FatFs;
using escort::service::NandFFS;
using escort::util::String;
using trf::pontos12::service::Video;
using trf::pontos12::service::Video01;
using trf::pontos12::service::Video02;
using trf::pontos12::service::Video03;
using trf::pontos12::service::Video04;
using trf::pontos12::service::Playlist;
using trf::pontos12::service::Renderizacao;
using escort::util::CRC16CCITT;
using escort::util::Converter;
using trf::pontos12::service::Video04;
using trf::application::trfProduct;
using trf::pontos12::service::Fonte;
using escort::util::SHA256;
using trf::service::net::ConnectionClient;
using trf::service::net::IdentificationProtocol;
using trf::application::Protocolotrf;
using trf::service::net::Network;
using escort::util::Math;
using trf::service::net::IAsyncHandler;

extern trf::application::Version trf_pontos12_application_Controlador_versao;

#define MAXIMA_PROFUNDIDADE_ESTRUTURA_ARQUIVOS	\
	4
#define MAXIMO_TAMANHO_ARQUIVO	\
	(10*1000*1000)

#define APPLICATION_ADDRESS    			0x08008000
#define APPLICATION_VERSION_OFFSET		1024
#define APPLICATION_CRC_OFFSET			1047


namespace trf {
namespace pontos12 {
namespace application {

//FIXME const char errorHeader[] = "trf.pontos12.application.Fachada:";

Fachada::Fachada(
	Network* network,
	IdentificationProtocol* idProtocol,
	Plataforma* plataforma) :
		parametrosFixos(plataforma->sistemaArquivo),
		listaRoteiros(plataforma->sistemaArquivo),
		listaMotoristas(plataforma->sistemaArquivo),
		listaMensagens(plataforma->sistemaArquivo),
		regiao(plataforma->sistemaArquivo),
		e2prom(plataforma->e2prom),
		network(network),
		idProtocol(idProtocol),
		plataforma(plataforma),
		protocoloCl(plataforma),
		dumpFirUpdate(&plataforma->usbHost->fileSystem),
		firmwareFolder(&plataforma->usbHost->fileSystem),
		file(plataforma->sistemaArquivo),
		tempoParaBloquear(0)
{}

void Fachada::init()
{
	this->emergenciaAtiva = false;
	//Seta o painel principal como selecionado
	this->painelSelecionado = 0;
	//Seta o endere�o do APP para 0xFF
	this->appNetAddress = 0xFF;
	//Seta habilita��o do APP para falso
	this->appHabilitado = false;
	//Seta o endere�o dos Adaptadores para 0xFF
	this->adaptadorCatracaNetAddress = 0xFF;
	this->adaptadorSensoresNetAddress = 0xFF;
	//Seta habilita��o dos Adaptadores para falso
	this->adaptadorCatracaHabilitado = false;
	this->adaptadorSensoresHabilitado = false;
	//Inicializa o status de funcionamento
	this->statusFuncionamento = ERRO_CONFIGURACAO_INCONSISTENTE;
	//Inicializa o sem�foro
	this->semaforo = xSemaphoreCreateMutex();
	//inicializa sem�foro do protocoloCl
	this->protocoloCl.init();

	//Verifica se o sistema de arquivo foi inicializado corretamente
	if(!this->plataforma->sistemaArquivo->wasInitiated())
	{
		this->setStatusFuncionamento(Fachada::ERRO_SISTEMA_ARQUIVO);
		return;
	}

	//Inicia lista de endere�os de pain�is que necessitam de informa��o da placa de sensores
	for (U8 i = 0; i < sizeof(this->listaPaineisSensores); ++i) {
		this->listaPaineisSensores[i] = 0;
	}

	//Inicializa o registro de LOG
//	escort_util_Log_init("log", this->plataforma->sistemaArquivo, this->plataforma->relogio);
}

void Fachada::carregarArquivos()
{
	//Abre o arquivo de par�metros fixos
	this->parametrosFixos.open("param.fix");
	//Abre a lista de roteiros
	this->listaRoteiros.open("roteiros/roteiros.lst");
	//Abre a lista de motoristas
	this->listaMotoristas.open("drivers/drivers.lst");
	//Abre a lista de mensagens
	this->listaMensagens.open("msgs/msgs.lst");
	//Abre a lista de regi�es
	ListaArquivos listaRegioes(this->plataforma->sistemaArquivo);
	listaRegioes.open("regioes/regioes.lst");
	String::strcpy(this->pathBuffer, "regioes/");
	listaRegioes.readNameArquivo(this->e2prom.readRegiaoSelecionada(), this->pathBuffer + String::indexOf(this->pathBuffer, "/") + 1);
	String::strcat( this->pathBuffer, ".rgn");
	listaRegioes.close();
	//Abre o arquivo de regi�o
	this->regiao.open(this->pathBuffer);
	//TODO - modificar a verificacao para se adequar ao versao com app com mais de 1 painel
	//Verifica se existe configura��o de APP
	if(this->parametrosFixos.readPaineisNSS()!=0 &&
			(this->parametrosFixos.readPaineisNSS() < (1<<this->parametrosFixos.readQntPaineis())))
	{
		this->setAPPHabilitado(true);
		this->detectarAPP(0);
	}

	//Verifica se existe configura��o de Adaptador de Catraca
	if(this->parametrosFixos.isCatracaNaRede()){
		this->setAdaptadorCatracaHabilitado(true);
		this->detectarAdaptadorCatraca(0);
	}
	//Verifica se existe configura��o de Adaptador de Placa de Sensores
	if(this->parametrosFixos.isSensoresNaRede()){
		this->setAdaptadorSensoresHabilitado(true);
		this->detectarAdaptadorSensores(0);
	}
}

bool Fachada::carregarParametrosVariaveis()
{
	U32 tmp32;
	U16 tmp16;
	U8 tmp8;

	//Abre o arquivo de par�metros vari�veis
	ParametrosVariaveis parametrosVariaveis(this->plataforma->sistemaArquivo);
	if(parametrosVariaveis.open("param.var") != 0){
		return false;
	}
	//Copia os valores do arquivo para a E2PROM
	////hora de sa�da
	tmp8 = parametrosVariaveis.readHoraSaida();
	this->e2prom.writeHoraSaida(tmp8);
	////minutos da sa�da
	tmp8 = parametrosVariaveis.readMinutosSaida();
	this->e2prom.writeMinutosSaida(tmp8);
	////regi�o
	tmp8 = parametrosVariaveis.readRegiaoSelecionada();
	this->e2prom.writeRegiaoSelecionada(tmp8);
	////sentido
	tmp8 = parametrosVariaveis.readSentidoIda();
	this->e2prom.writeSentidoSelecionado(tmp8);
	////roteiro
	tmp32 = parametrosVariaveis.readRoteiroSelecionado();
	this->e2prom.writeRoteiroSelecionado(tmp32);
	////motorista
	tmp16 = parametrosVariaveis.readMotoristaSelecionado();
	this->e2prom.writeMotoristaSelecionado(tmp16);
	//Fecha o arquivo
	parametrosVariaveis.close();

	//Obtem a quantidade de paineis
	U8 qntPaineis;
	ParametrosFixos parametrosFixos(this->plataforma->sistemaArquivo);
	if(parametrosFixos.open("param.fix") != 0){
		return false;
	}
	qntPaineis = parametrosFixos.readQntPaineis();
	parametrosFixos.close();

	//Copia os valores dos par�metros dos paineis
	for (U8 painel = 0; painel < qntPaineis; ++painel) {
		//Abre o arquivo de configura��o do painel
		PainelConfig painelCfg(this->plataforma->sistemaArquivo);
		this->getPathPainelConfig(this->pathBuffer, painel);
		if(painelCfg.open(this->pathBuffer) != 0){
			return false;
		}
		//Copia os valores do arquivo para a E2PROM
		////altern�ncia
		tmp8 = painelCfg.readAlternanciaSelecionada();
		this->e2prom.writeAlternanciaSelecionada(painel, tmp8);
		////mensagem
		tmp32 = painelCfg.readMensagemSelecionada();
		this->e2prom.writeMensagemSelecionada(painel, tmp32);
		////mensagem secund�ria
		tmp32 = painelCfg.readMensagemSecundariaSelecionada();
		this->e2prom.writeMensagemSecundariaSelecionada(painel, tmp32);
		////brilho m�ximo
		tmp8 = painelCfg.readBrilhoMaximo();
		this->e2prom.writeBrilhoMaximo(painel, tmp8);
		////brilho m�nimo
		tmp8 = painelCfg.readBrilhoMinimo();
		this->e2prom.writeBrilhoMinimo(painel, tmp8);
		////precisa formatar os paineis
		tmp8 = true;
		this->e2prom.writePrecisaFormatarPainel(painel, tmp8);
		//Fecha o arquivo
		painelCfg.close();
	}

	return true;
}

bool Fachada::verificarSenhaAntiFurto()
{
	//Obt�m as informa��es de senha anti-furto do arquivo
	bool ativaSenhaAntiFurto = this->parametrosFixos.readAtivaSenhaAtinFurto();
	U8* senhaAntiFurto = this->buffer;
	this->parametrosFixos.readSenhaAntiFurto(senhaAntiFurto);
	// Verfica se h� uma senha anti-furto cadastrada
	if(this->isSenhaAntiFurtoHabilitada()){
		// Verfica se a senha do arquivo est� correta
		if(this->e2prom.verificarSenhaAntiFurto(senhaAntiFurto)){
			//Se a senha est� desativada no arquivo
			if(ativaSenhaAntiFurto == false){
				//Desativa a senha no controlador
				String::memset(senhaAntiFurto, 0, 32);
				this->mudarSenhaAntiFurto(senhaAntiFurto);
			}
		}
		else{
			//Senha anti-furto incorreta
			return false;
		}
	}
	else{
		// Verfica se a senha est� ativada no arquivo
		if(ativaSenhaAntiFurto){
			bool novaSenhaValida = false;
			//Verifica se a nova senha � uma senha v�lida
			for (int i = 0; i < 32; ++i) {
				if((senhaAntiFurto[i] != 0x00 && senhaAntiFurto[i] != 0xFF) || (senhaAntiFurto[i] != senhaAntiFurto[0])){
					novaSenhaValida = true;
					break;
				}
			}
			if(novaSenhaValida){
				// Copia a senha do arquivo para o controlador
				this->mudarSenhaAntiFurto(senhaAntiFurto);
			}
		}
	}

	return true;
}

bool Fachada::verificarConsistenciaFonte(char* pathFonte)
{
	Fonte arquivoFonte(this->plataforma->sistemaArquivo);
	//Verifica se o arquivo de fonte existe
	if(arquivoFonte.open(pathFonte) != 0)
	{
		this->registrarErroArquivoInexistente(pathFonte);
		return false;
	}
	//Verifica a consist�ncia
	U8 erro = arquivoFonte.verificarConsistencia();
	//Fecha o arquivo
	arquivoFonte.close();
	//Verifica se h� inconsist�ncia com o tamanho do arquivo
	if(erro == 1)
	{
		this->registrarErroTamanhoArquivo(pathFonte);
		return false;
	}
	//Verifica se h� inconsist�ncia no CRC do arquivo
	else if(erro == 2)
	{
		this->registrarErroCRCArquivo(pathFonte);
		return false;
	}
	//Verifica se h� dados inconsistentes no arquivo
	else if(erro == 3)
	{
		this->registrarErroArquivoDadosInconsistentes(pathFonte, "est� inconsistente");
		return false;
	}

	return true;
}

bool Fachada::verificarArquivosConfig(void (*incrementProgress)(void))
{
	U8 qntPaineis;
	U8 regiaoSelecionada;
	U32 roteiroSelecionado;
	U16 motoristaSelecionado;
//	U8 painelNSS;

	//Abre o arquivo de par�metros fixos
	ParametrosFixos paramFix(this->plataforma->sistemaArquivo);
	if(paramFix.open("param.fix") != 0){
		this->registrarErroArquivoInexistente("param.fix");
		return false;
	}
	else
	{
		ParametrosFixos::ErroArquivo erro = paramFix.verificarConsistencia();
		if(erro != ParametrosFixos::SUCESSO)
		{
			switch(erro)
			{
			case ParametrosFixos::ERRO_CRC:
				this->registrarErroCRCArquivo("param.fix");
				break;
			case ParametrosFixos::ERRO_TAMANHO_ARQUIVO:
				this->registrarErroTamanhoArquivo("param.fix");
				break;
			case ParametrosFixos::ERRO_HORA_BOMDIA:
				this->registrarErroArquivoDadosInconsistentes("param.fix", "hora de bom dia inv�lida");
				break;
			case ParametrosFixos::ERRO_HORA_BOATARDE:
				this->registrarErroArquivoDadosInconsistentes("param.fix", "hora de boa tarde inv�lida");
				break;
			case ParametrosFixos::ERRO_HORA_BOANOITE:
				this->registrarErroArquivoDadosInconsistentes("param.fix", "hora de boa noite inv�lida");
				break;
			case ParametrosFixos::ERRO_QNT_PAINEIS:
				this->registrarErroArquivoDadosInconsistentes("param.fix", "configura��o com zero paineis");
				break;
			}
			paramFix.close();
			return false;
		}
	}
	//Obtem a quantidade de paineis
	qntPaineis = paramFix.readQntPaineis();
	//Obt�m o painel NSS
	//FIXME - n�o � utilizado.
//	painelNSS = paramFix.readPainelNSS();
	//Fecha o arquivo
	paramFix.close();
	//Indica a progress�o da tarefa
	incrementProgress();
	//Abre o arquivo de agenda
	Agenda agenda(this->plataforma->sistemaArquivo);
	if(agenda.open("agenda.sch") != 0){
		this->registrarErroArquivoInexistente("agenda.sch");
		return false;
	}
	else
	{
		Agenda::ErroArquivo erro = agenda.verificarConsistencia();
		if(erro != Agenda::SUCESSO)
		{
			switch(erro)
			{
			case Agenda::ERRO_CRC:
				this->registrarErroCRCArquivo("agenda.sch");
				break;
			case Agenda::ERRO_TAMANHO_ARQUIVO:
				this->registrarErroTamanhoArquivo("agenda.sch");
				break;
			}
			agenda.close();
			return false;
		}
	}
	//Fecha o arquivo
	agenda.close();
	//Indica a progress�o da tarefa
	incrementProgress();
	//Abre o arquivo de par�metros vari�veis
	ParametrosVariaveis paramVar(this->plataforma->sistemaArquivo);
	if(paramVar.open("param.var") != 0){
		this->registrarErroArquivoInexistente("param.var");
		return false;
	}
	else
	{
		ParametrosVariaveis::Erro erro = paramVar.verificarConsistencia();
		if(erro != ParametrosVariaveis::SUCESSO)
		{
			switch(erro)
			{
			case ParametrosVariaveis::ERRO_CRC:
				this->registrarErroCRCArquivo("param.var");
				break;
			case ParametrosVariaveis::ERRO_TAMANHO_ARQUIVO:
				this->registrarErroTamanhoArquivo("param.var");
				break;
			case ParametrosVariaveis::ERRO_HORA_SAIDA:
				this->registrarErroArquivoDadosInconsistentes("param.var", "hora de sa�da inv�lida");
				break;

			}
			paramVar.close();
			return false;
		}
	}
	//Obt�m a regi�o selecionada
	regiaoSelecionada = paramVar.readRegiaoSelecionada();
	//Obt�m o roteiro selecionado
	roteiroSelecionado = paramVar.readRoteiroSelecionado();
	//Obt�m o motorista selecionado
	motoristaSelecionado = paramVar.readMotoristaSelecionado();
	//Fecha o arquivo de par�metros vari�veis
	paramVar.close();
	//Indica a progress�o da tarefa
	incrementProgress();

	//Abre a lista de roteiros
	ListaArquivos roteirosLst(this->plataforma->sistemaArquivo);
	if(roteirosLst.open("roteiros/roteiros.lst") != 0){
		this->registrarErroArquivoInexistente("roteiros/roteiros.lst");
		return false;
	}
	else
	{
		ListaArquivos::ErroArquivo erro = roteirosLst.verificarConsistencia();
		if(erro != ListaArquivos::SUCESSO)
		{
			switch(erro)
			{
			case ListaArquivos::ERRO_CRC:
				this->registrarErroCRCArquivo("roteiros/roteiros.lst");
				break;
			case ListaArquivos::ERRO_TAMANHO_ARQUIVO:
				this->registrarErroTamanhoArquivo("roteiros/roteiros.lst");
				break;
			}
			roteirosLst.close();
			return false;
		}

		// Verificar arquivos .rot de todos os roteiros
		Roteiro rot(this->plataforma->sistemaArquivo);
		String::strcpy(this->pathBuffer, "roteiros/");
		roteirosLst.readNameArquivo(roteiroSelecionado, this->pathBuffer + 9);
		String::strcat(this->pathBuffer, ".rot");
		if(rot.open(this->pathBuffer) != 0)
		{
			this->registrarErroArquivoInexistente(this->pathBuffer);
			return false;
		}
		else
		{
			Roteiro::ErroArquivo erro = rot.verificarConsistencia();
			if(erro != Roteiro::SUCESSO)
			{
				switch(erro)
				{
				case Roteiro::ERRO_CRC:
					this->registrarErroCRCArquivo(this->pathBuffer);
					break;
				case Roteiro::ERRO_TAMANHO_ARQUIVO:
					this->registrarErroTamanhoArquivo(this->pathBuffer);
					break;
				}
				rot.close();
				return false;
			}
			rot.close();
		}
		//Indica a progress�o da tarefa
		incrementProgress();
		roteirosLst.close();
	}
	//Indica a progress�o da tarefa
	incrementProgress();

	//Abre a lista de motoristas
	ListaArquivos motoristasLst(this->plataforma->sistemaArquivo);
	if(motoristasLst.open("drivers/drivers.lst") != 0){
		this->registrarErroArquivoInexistente("drivers/drivers.lst");
		return false;
	}
	else
	{
		ListaArquivos::ErroArquivo erro = motoristasLst.verificarConsistencia();
		if(erro != ListaArquivos::SUCESSO)
		{
			switch(erro)
			{
			case ListaArquivos::ERRO_CRC:
				this->registrarErroCRCArquivo("drivers/drivers.lst");
				break;
			case ListaArquivos::ERRO_TAMANHO_ARQUIVO:
				this->registrarErroTamanhoArquivo("drivers/drivers.lst");
				break;
			}
			motoristasLst.close();
			return false;
		}

		//Verifica se tem algum motorista cadastrado
		if(motoristasLst.readQntArquivos() > 0){
			// Verificar arquivos .drv de todos os motoristas
			Motorista drv(this->plataforma->sistemaArquivo);
			String::strcpy(this->pathBuffer, "drivers/");
			motoristasLst.readNameArquivo(motoristaSelecionado, this->pathBuffer + 8);
			String::strcat(this->pathBuffer, ".drv");
			if(drv.open(this->pathBuffer) != 0)
			{
				this->registrarErroArquivoInexistente(this->pathBuffer);
				return false;
			}
			else
			{
				Motorista::ErroArquivo erro = drv.verificarConsistencia();
				if(erro != Motorista::SUCESSO)
				{
					switch(erro)
					{
					case Motorista::ERRO_CRC:
						this->registrarErroCRCArquivo(this->pathBuffer);
						break;
					case Motorista::ERRO_TAMANHO_ARQUIVO:
						this->registrarErroTamanhoArquivo(this->pathBuffer);
						break;
					}
					drv.close();
					return false;
				}
				drv.close();
			}
		}
		//Indica a progress�o da tarefa
		incrementProgress();
		motoristasLst.close();
	}
	//Indica a progress�o da tarefa
	incrementProgress();

	//Abre a lista de mensagens
	ListaArquivos msgsLst(this->plataforma->sistemaArquivo);
	if(msgsLst.open("msgs/msgs.lst") != 0){
		this->registrarErroArquivoInexistente("msgs/msgs.lst");
		return false;
	}
	else
	{
		ListaArquivos::ErroArquivo erro = msgsLst.verificarConsistencia();
		if(erro != ListaArquivos::SUCESSO)
		{
			switch(erro)
			{
			case ListaArquivos::ERRO_CRC:
				this->registrarErroCRCArquivo("msgs/msgs.lst");
				break;
			case ListaArquivos::ERRO_TAMANHO_ARQUIVO:
				this->registrarErroTamanhoArquivo("msgs/msgs.lst");
				break;
			}
			msgsLst.close();
			return false;
		}
		msgsLst.close();
	}
	//Indica a progress�o da tarefa
	incrementProgress();
	msgsLst.close();

	// Verifica o arquivo de lista de regi�es
	ListaArquivos listaRegioes(this->plataforma->sistemaArquivo);
	if(listaRegioes.open("regioes/regioes.lst") != 0){
		this->registrarErroArquivoInexistente("regioes/regioes.lst");
		return false;
	}
	else{
		ListaArquivos::ErroArquivo erro = listaRegioes.verificarConsistencia();
		if(erro != ListaArquivos::SUCESSO)
		{
			switch(erro)
			{
			case ListaArquivos::ERRO_CRC:
				this->registrarErroCRCArquivo("regioes/regioes.lst");
				break;
			case ListaArquivos::ERRO_TAMANHO_ARQUIVO:
				this->registrarErroTamanhoArquivo("regioes/regioes.lst");
				break;
			}
			listaRegioes.close();
			return false;
		}

		//Verifica se a regi�o selecionada � uma regi�o v�lida
		if(regiaoSelecionada >= listaRegioes.readQntArquivos())
		{
			this->registrarErroArquivoDadosInconsistentes("param.fix","regiao selecionada nao existe");
		}

		//Verifica todas as regi�es listadas no arquivo
		U32 qntRegioes = listaRegioes.readQntArquivos();
		for (U32 indiceRegiao = 0; indiceRegiao < qntRegioes; indiceRegiao++) {
			Regiao reg(this->plataforma->sistemaArquivo);
			String::strcpy(this->pathBuffer, "regioes/");
			listaRegioes.readNameArquivo(indiceRegiao, this->pathBuffer + 8);
			String::strcat(this->pathBuffer, ".rgn");
			if(reg.open(this->pathBuffer) != 0)
			{
				this->registrarErroArquivoInexistente(this->pathBuffer);
				listaRegioes.close();
				return false;
			}
			else
			{
				Regiao::ErroArquivo erro = reg.verificarConsistencia();
				if(erro != Regiao::SUCESSO)
				{
					switch(erro)
					{
					case Regiao::ERRO_CRC:
						this->registrarErroCRCArquivo(this->pathBuffer);
						break;
					case Regiao::ERRO_TAMANHO_ARQUIVO:
						this->registrarErroTamanhoArquivo(this->pathBuffer);
						break;
					}
					reg.close();
					return false;
				}
				else{
					// Verificar arquivo de idioma
					reg.readIdiomaPath(this->pathBuffer);
					{
						Idioma idioma(this->plataforma->sistemaArquivo);
						if(idioma.open(this->pathBuffer) != 0)
						{
							this->registrarErroArquivoInexistente(this->pathBuffer);
							return false;
						}
						else
						{
							Idioma::ErroArquivo erro = idioma.verificarConsistencia();
							if(erro != Idioma::SUCESSO)
							{
								switch(erro)
								{
								case Idioma::ERRO_CRC:
									this->registrarErroCRCArquivo(this->pathBuffer);
									break;
								case Idioma::ERRO_TAMANHO_ARQUIVO:
									this->registrarErroTamanhoArquivo(this->pathBuffer);
									break;
								}
								idioma.close();
								return false;
							}
							//Fecha o arquivo (abriu apenas para testar exist�ncia)
							idioma.close();
						}
						//Indica a progress�o da tarefa
						incrementProgress();
					}
				}
				reg.close();
			}
			//Indica a progress�o da tarefa
			incrementProgress();
		}
		listaRegioes.close();
	}
	//Indica a progress�o da tarefa
	incrementProgress();

	// Verificar arquivos de todos os paineis
	for (U8 painel = 0; painel < qntPaineis; ++painel) {
		String::strcpy(this->pathBuffer, "paineis/");
		Converter::itoa(painel, this->pathBuffer + 8, 2, Converter::BASE_10);

		//Verifica o arquivo painel.cfg
		String::strcpy(this->pathBuffer + 10, "/painel.cfg");
		PainelConfig arquivoPainel(this->plataforma->sistemaArquivo);
		if (arquivoPainel.open(this->pathBuffer) != 0)
		{
			this->registrarErroArquivoInexistente(this->pathBuffer);
			return false;
		}
		else
		{
			PainelConfig::Erro erro = arquivoPainel.verificarConsistencia();
			if(erro != PainelConfig::SUCESSO)
			{
				switch(erro)
				{
				case PainelConfig::ERRO_CRC:
					this->registrarErroCRCArquivo(this->pathBuffer);
					break;
				case PainelConfig::ERRO_VALORES_INCONSISTENTES:
					this->registrarErroArquivoDadosInconsistentes(this->pathBuffer,"valores incosistentes");
					break;
				case PainelConfig::ERRO_TAMANHO_ARQUIVO:
					this->registrarErroTamanhoArquivo(this->pathBuffer);
					break;

				}
				arquivoPainel.close();
				return false;
			}

			//verificar consistencia da fonte
			arquivoPainel.readFontePath(this->pathBuffer);
			//Fecha o arquivo
			arquivoPainel.close();
			if(this->verificarConsistenciaFonte(this->pathBuffer) == false)
			{
				return false;
			}
		}
		//Obt�m a mensagem selecionada
		U32 mensagemSelecionada = arquivoPainel.readMensagemSelecionada();
		//Indica a progress�o da tarefa
		incrementProgress();
		arquivoPainel.close();

		//Verifica o arquivo altern.alt
		String::strcpy(this->pathBuffer, "paineis/");
		Converter::itoa(painel, this->pathBuffer + 8, 2, Converter::BASE_10);
		String::strcpy(this->pathBuffer + 10, "/altern.alt");
		Alternancias arquivoAlternancias(this->plataforma->sistemaArquivo);
		if(arquivoAlternancias.open(this->pathBuffer) != 0)
		{
			this->registrarErroArquivoInexistente(this->pathBuffer);
		}
		else
		{
			Alternancias::Erro erro = arquivoAlternancias.verificarConsistencia();
			if(erro != Alternancias::SUCESSO)
			{
				switch(erro)
				{
				case Alternancias::ERRO_CRC:
					this->registrarErroCRCArquivo(this->pathBuffer);
					break;
				case Alternancias::ERRO_TAMANHO_ARQUIVO:
					this->registrarErroTamanhoArquivo(this->pathBuffer);
					break;
				case Alternancias::ERRO_NENHUMA_ALTERNANCIA:
					this->registrarErroArquivoDadosInconsistentes(this->pathBuffer,"n�o possui nenhuma alternancia");
					break;
				case Alternancias::ERRO_NENHUMA_EXIBICAO:
					this->registrarErroArquivoDadosInconsistentes(this->pathBuffer,"possui alternancia sem exibi��o");
					break;
				}
				arquivoAlternancias.close();
				return false;
			}
			arquivoAlternancias.close();
		}
		//Indica a progress�o da tarefa
		incrementProgress();

		//Verifica o arquivo emerg.pls
		if(painel == 0){
			String::strcpy(this->pathBuffer, "paineis/");
			Converter::itoa(painel, this->pathBuffer + 8, 2, Converter::BASE_10);
			String::strcpy(this->pathBuffer + 10, "/emerg.pls");
			if(this->plataforma->sistemaArquivo->exists(this->pathBuffer) == false)
			{
				this->registrarErroArquivoInexistente(this->pathBuffer);
			}
			//Indica a progress�o da tarefa
			incrementProgress();
		}

		//Abre a lista de roteiros
		roteirosLst.open("roteiros/roteiros.lst");
		//Verifica arquivos .rpt
		String::strcpy(this->pathBuffer, "paineis/");
		Converter::itoa(painel, this->pathBuffer + 8, 2, Converter::BASE_10);
		String::strcpy(this->pathBuffer + 10, "/roteiros/");
		roteirosLst.readNameArquivo(roteiroSelecionado, this->pathBuffer + 20);
		String::strcat( this->pathBuffer, ".rpt");

		RoteiroPaths roteiroRpt(this->plataforma->sistemaArquivo);
		if(roteiroRpt.open(this->pathBuffer) != 0)
		{
			this->registrarErroArquivoInexistente(this->pathBuffer);
			roteirosLst.close();
			return false;
		}
		else
		{
			RoteiroPaths::ErroArquivo erro = roteiroRpt.verificarConsistencia();
			if(erro != RoteiroPaths::SUCESSO)
			{
				switch(erro)
				{
				case RoteiroPaths::ERRO_CRC:
					this->registrarErroCRCArquivo(this->pathBuffer);
					break;
				case RoteiroPaths::ERRO_TAMANHO_ARQUIVO:
					this->registrarErroTamanhoArquivo(this->pathBuffer);
					break;
				}
				roteiroRpt.close();
				roteirosLst.close();
				return false;
			}

			roteiroRpt.close();
		}
		//Indica a progress�o da tarefa
		incrementProgress();
		roteirosLst.close();

		//Abre a lista de motoristas
		motoristasLst.open("drivers/drivers.lst");
		//Verifica se tem motorista cadastrado
		if(motoristasLst.readQntArquivos() > 0){
			//Verifica arquivos .dpt
			String::strcpy(this->pathBuffer, "paineis/");
			Converter::itoa(painel, this->pathBuffer + 8, 2, Converter::BASE_10);
			String::strcpy(this->pathBuffer + 10, "/drivers/");
			motoristasLst.readNameArquivo(motoristaSelecionado, this->pathBuffer + 19);
			String::strcat( this->pathBuffer, ".dpt");

			MotoristaPaths motoristaDpt(this->plataforma->sistemaArquivo);
			if(motoristaDpt.open(this->pathBuffer) != 0)
			{
				this->registrarErroArquivoInexistente(this->pathBuffer);
				motoristasLst.close();
				return false;
			}
			else
			{
				MotoristaPaths::ErroArquivo erro = motoristaDpt.verificarConsistencia();
				if(erro != MotoristaPaths::SUCESSO)
				{
					switch(erro)
					{
					case MotoristaPaths::ERRO_CRC:
						this->registrarErroCRCArquivo(this->pathBuffer);
						break;
					case MotoristaPaths::ERRO_TAMANHO_ARQUIVO:
						this->registrarErroTamanhoArquivo(this->pathBuffer);
						break;
					}
					motoristaDpt.close();
					motoristasLst.close();
					return false;
				}

				motoristaDpt.close();
			}
		}
		//Indica a progress�o da tarefa
		incrementProgress();
		motoristasLst.close();

		//Abre arquivo de lista de mensagens
		msgsLst.open("msgs/msgs.lst");

		//Verifica arquivo .msg
		String::strcpy(this->pathBuffer, "/msgs/");
		msgsLst.readNameArquivo(mensagemSelecionada, this->pathBuffer + 6);
		String::strcat( this->pathBuffer, ".msg");
		Mensagem mensagemMsg(this->plataforma->sistemaArquivo);
		if(mensagemMsg.open(this->pathBuffer) != 0)
		{
			this->registrarErroArquivoInexistente(this->pathBuffer);
			msgsLst.close();
			return false;
		}
		else
		{
			Mensagem::ErroArquivo erro = mensagemMsg.verificarConsistencia();
			if(erro != Mensagem::SUCESSO)
			{
				switch(erro)
				{
				case Mensagem::ERRO_CRC:
					this->registrarErroCRCArquivo(this->pathBuffer);
					break;
				case Mensagem::ERRO_TAMANHO_ARQUIVO:
					this->registrarErroTamanhoArquivo(this->pathBuffer);
					break;
				}
				mensagemMsg.close();
				msgsLst.close();
				return false;
			}

			mensagemMsg.close();
		}

		//Verifica arquivo .mpt
		String::strcpy(this->pathBuffer, "paineis/");
		Converter::itoa(painel, this->pathBuffer + 8, 2, Converter::BASE_10);
		String::strcpy(this->pathBuffer + 10, "/msgs/");
		msgsLst.readNameArquivo(mensagemSelecionada, this->pathBuffer + 16);
		String::strcat( this->pathBuffer, ".mpt");
		MensagemPaths mensagemRpt(this->plataforma->sistemaArquivo);
		if(mensagemRpt.open(this->pathBuffer) != 0)
		{
			this->registrarErroArquivoInexistente(this->pathBuffer);
			msgsLst.close();
			return false;
		}
		else
		{
			MensagemPaths::ErroArquivo erro = mensagemRpt.verificarConsistencia();
			if(erro != MensagemPaths::SUCESSO)
			{
				switch(erro)
				{
				case MensagemPaths::ERRO_CRC:
					this->registrarErroCRCArquivo(this->pathBuffer);
					break;
				case MensagemPaths::ERRO_TAMANHO_ARQUIVO:
					this->registrarErroTamanhoArquivo(this->pathBuffer);
					break;
				}
				mensagemRpt.close();
				msgsLst.close();
				return false;
			}

			mensagemRpt.close();
		}

		//Indica a progress�o da tarefa
		incrementProgress();
		msgsLst.close();
	}

	return true;
}

void Fachada::carregarConfiguracao(void (*incrementProgress)(void))
{
	if(this->e2prom.getParametrosVariaveisStatus() == STATUS_CONFIG_NAO_VERIFICADA){
		//Verifica a consist�ncia dos arquivos de configura��o
		if(verificarArquivosConfig(incrementProgress) == false){
			//Registra a configura��o como inv�lida
			this->e2prom.setParametrosVariaveisStatus(STATUS_CONFIG_INVALIDA);
			//Assinala erro de funcionamento
			this->setStatusFuncionamento(ERRO_CONFIGURACAO_INCONSISTENTE);
			return;
		}
		//Obtem a quantidade de paineis
		U8 qntPaineis;
		ParametrosFixos parametrosFixos(this->plataforma->sistemaArquivo);
		if(parametrosFixos.open("param.fix") != 0){
			return;
		}
		qntPaineis = parametrosFixos.readQntPaineis();
		parametrosFixos.close();
		//Assinala a necessidade de formatar os paineis
		for (U8 p = 0; p < qntPaineis; ++p) {
			this->setPainelPrecisandoFormatar(p, true);
		}
		//Assinala a necessidade de formatar o APP
		this->setAPPStatusConfig(STATUS_CONFIG_APP_PRECISANDO_SINCRONIZAR_PARAMETROS);
		//Assinala a necessidade de carregar os par�metros vari�veis na E2PROM
		this->e2prom.setParametrosVariaveisStatus(STATUS_CONFIG_E2PROM_DESATUALIZADA);
	}
	if(this->e2prom.getParametrosVariaveisStatus() == STATUS_CONFIG_E2PROM_DESATUALIZADA){
		//Carrega os par�metros vari�veis dos arquivos para a E2PROM
		if(this->carregarParametrosVariaveis() == false){
			//Assinala erro de funcionamento
			this->setStatusFuncionamento(ERRO_CONFIGURACAO_INCONSISTENTE);
			return;
		}
		//Indica a progress�o da tarefa
		incrementProgress();
		//Assinala a necessidade de carregar o par�metros vari�veis na E2PROM
		this->e2prom.setParametrosVariaveisStatus(STATUS_CONFIG_VALIDA);
	}
	if(this->e2prom.getParametrosVariaveisStatus() == STATUS_CONFIG_VALIDA){
		//Abre os arquvos necess�rios para o funcionamento do Controlador
		this->carregarArquivos();
		//Indica a progress�o da tarefa
		incrementProgress();
		//Verifica a senha anti-furto
		if(this->verificarSenhaAntiFurto()){
			//Assinala funcionamento correto
			this->setStatusFuncionamento(FUNCIONANDO_OK);
		}
		else{
			//Assinala senha anti-furto incorreta
			this->setStatusFuncionamento(ERRO_SENHA_ANTI_FURTO);
		}
		//Indica a progress�o da tarefa
		incrementProgress();
	}
}

//Registro do erro E001
void Fachada::registrarErroTamanhoArquivo(char* nomeArquivo)
{
//FIXME	escort_util_Log_append((char*)errorHeader, "E001:Arquivo \"");
//	escort_util_Log_append(nomeArquivo);
//	escort_util_Log_append("\" com tamanho inconsistente\r\n");
}
//Registro do erro E002
void Fachada::registrarErroArquivoInexistente(char* nomeArquivo)
{
//FIXME	escort_util_Log_append((char*)errorHeader, "E002:Arquivo \"");
//	escort_util_Log_append(nomeArquivo);
//	escort_util_Log_append("\" inexistente\r\n");
}
//Registro do erro E003
void Fachada::registrarErroCRCArquivo(char* nomeArquivo)
{
//FIXME	escort_util_Log_append((char*)errorHeader, "E003:Arquivo \"");
//	escort_util_Log_append(nomeArquivo);
//	escort_util_Log_append("\" com CRC errado\r\n");
}
//Registro do erro E004
void Fachada::registrarErroArquivoDadosInconsistentes(char* nomeArquivo, char* descricao)
{
//FIXME	escort_util_Log_append((char*)errorHeader, "E004:Arquivo \"");
//	escort_util_Log_append(nomeArquivo);
//	escort_util_Log_append("\" com dado incoerente (");
//	escort_util_Log_append(descricao);
//	escort_util_Log_append(")\r\n");
}

trf::application::Version* Fachada::getVersaoFirmware()
{	
	return &trf_pontos12_application_Controlador_versao;
}

U16 Fachada::getCRCFirmware()
{
	return *((U16*)(APPLICATION_ADDRESS + APPLICATION_CRC_OFFSET));
}

bool Fachada::readVersaoPainel(U8 painel, U8* versao)
{
	bool result = false;
	//Conecta com o painel
	ConnectionClient* client = this->network->connect(this->getPainelNetAddress(painel), 0);
	if(client){
		//Configura o timeout de recebimento
		client->setReceiveTimeout(2000);

		//Se os paineis est�o travados ent�o envia comando para liberar a trava (autentica��o)
		if(this->isPaineisTravados()){
			U8 chave[4];
			this->e2prom.readTravaPaineis(chave);
			if(this->painelLiberarTrava(client, chave) != Protocolotrf::SUCESSO_COMM)
			{
				client->close();
				return false;
			}
		}

		//L� as informa��es de vers�o do Painel
		Protocolotrf::ResultadoComunicacao res = this->remoteRdVersao(client, versao);
		result = (res == Protocolotrf::SUCESSO_COMM);
		//Fecha a conex�o
		client->close();
	}

	return result;
}

bool Fachada::readCRCFirmwarePainel(U8 painel, U16* crcFW)
{
	bool result = false;
	//Conecta com o painel
	ConnectionClient* client = this->network->connect(this->getPainelNetAddress(painel), 0);
	if(client){
		//Configura o timeout de recebimento
		client->setReceiveTimeout(2000);

		//Se os paineis est�o travados ent�o envia comando para liberar a trava (autentica��o)
		if(this->isPaineisTravados()){
			U8 chave[4];
			this->e2prom.readTravaPaineis(chave);
			if(this->painelLiberarTrava(client, chave) != Protocolotrf::SUCESSO_COMM)
			{
				client->close();
				return false;
			}
		}

		//L� as informa��es de vers�o do Painel
		Protocolotrf::ResultadoComunicacao res = this->remoteRdCRCFirmware(client, crcFW);
		result = (res == Protocolotrf::SUCESSO_COMM);
		//Fecha a conex�o
		client->close();
	}

	return result;
}

trf::application::trfProduct::SerialNumber Fachada::getSerialNumber()
{
	return this->e2prom.getSerialNumber();
}

void Fachada::setSerialNumber(trf::application::trfProduct::SerialNumber* sNumber)
{
	this->e2prom.setSerialNumber(sNumber);
}

Fachada::StatusFuncionamento Fachada::getStatusFuncionamento()
{
	return this->statusFuncionamento;
}

void Fachada::setStatusFuncionamento(Fachada::StatusFuncionamento status)
{
	this->statusFuncionamento = status;
}

void Fachada::getPathPainelConfig(char* path, U8 indicePainel)
{
	//Monta path do arquivo de par�metros vari�veis do painel
	String::strcpy(path, "paineis/");
	Converter::itoa(indicePainel, path + 8, 2, Converter::BASE_10);
	String::strcpy(path + 10, "/painel.cfg");
}

void Fachada::getPathAlternancias(char* path, U8 indicePainel)
{
	//Monta path do arquivo de par�metros vari�veis do painel
	String::strcpy(path, "paineis/");
	Converter::itoa(indicePainel, path + 8, 2, Converter::BASE_10);
	String::strcpy(path + 10, "/altern.alt");
}

U8 Fachada::getAlternancia(U8 indicePainel)
{
	return this->e2prom.readAlternanciaSelecionada(indicePainel);
}

U8 Fachada::getAlternancia()
{
	return this->getAlternancia(this->painelSelecionado);
}

Fachada::Resultado Fachada::setAlternancia(U8 indiceAlternancia)
{
	return setAlternancia(indiceAlternancia, false);
}

Fachada::Resultado Fachada::setAlternancia(U8 indiceAlternancia, bool force)
{
	Fachada::Resultado resultado = SUCESSO;

	//Verifica se a fun��o de mudar altern�ncia est� bloqueada
	// e, caso a sess�o esteja aberta (senha de desbloqueio), o contador � resetado (mais 60 segundos de fun��es liberadas)
	if(!force && (!this->isFuncaoLiberada(ParametrosFixos::SELECAO_ALTERNACIA)))
	{
		return FUNCAO_BLOQUEADA;
	}

	//Muda a altern�ncia
	this->e2prom.writeAlternanciaSelecionada(this->painelSelecionado, indiceAlternancia);
	//Sincroniza os arquivos do painel
	this->setPainelSincronizado(this->painelSelecionado, false);
	//Sincroniza a placa de sensores
	this->setSensoresSincronizados(false);

	return resultado;
}

U8 Fachada::getQntAlternancias()
{
	Alternancias alternancias(this->plataforma->sistemaArquivo);
	U8 qntAlternancias = 0;

	//L� path do arquivo configura��o do painel
	this->getPathAlternancias(this->pathBuffer, this->painelSelecionado);

	//Abre o arquivo e l� as flags de exibi��o
	if(alternancias.open(this->pathBuffer) == 0) {
		qntAlternancias = alternancias.readQntAlternancias();
		alternancias.close();
	}

	return qntAlternancias;
}

U8 Fachada::getQntExibicoesAlternancia(U8 indiceAlternancia, U8 indicePainel)
{
	Alternancias alternancias(this->plataforma->sistemaArquivo);
	U8 qntExibicoes = 0;

	//L� path do arquivo configura��o do painel
	this->getPathAlternancias(this->pathBuffer, indicePainel);

	//Abre o arquivo e l� as flags de exibi��o
	if(alternancias.open(this->pathBuffer) == 0) {
		qntExibicoes = alternancias.readQntExibicoes(indiceAlternancia);
		alternancias.close();
	}

	return qntExibicoes;
}

U8 Fachada::getExibicao(U8 indiceAlternancia, U8 indicePainel, U8 indiceExibicao)
{
	Alternancias alternancias(this->plataforma->sistemaArquivo);
	U8 exibicao = 0;

	//L� path do arquivo configura��o do painel
	this->getPathAlternancias(this->pathBuffer, indicePainel);

	//Abre o arquivo e l� as flags de exibi��o
	if(alternancias.open(this->pathBuffer) == 0) {
		exibicao = alternancias.readExibicao(indiceAlternancia, indiceExibicao);
		alternancias.close();
	}

	return exibicao;
}

void Fachada::getNomeAlternancia(U8 indiceAlternancia, char* nome)
{
	Alternancias alternancias(this->plataforma->sistemaArquivo);

	//L� path do arquivo configura��o do painel
	this->getPathAlternancias(this->pathBuffer, this->painelSelecionado);

	//Abre o arquivo e l� as flags de exibi��o
	if(alternancias.open(this->pathBuffer) == 0) {
		alternancias.readNomeAlternancia(indiceAlternancia, nome);
		alternancias.close();
	}
}

U32 Fachada::getRoteiroSelecionado()
{
	return this->e2prom.readRoteiroSelecionado();
}

void Fachada::getPathRoteiro(U32 indiceRoteiro, char* path)
{
	String::strcpy(path, "roteiros/");
	this->listaRoteiros.readNameArquivo(indiceRoteiro, path + String::indexOf( path, "/") + 1);
	String::strcat( path, ".rot");
}

void Fachada::getPathMotorista(U32 indiceMotorista, char* path)
{
	String::strcpy(path, "drivers/");
	this->listaMotoristas.readNameArquivo(indiceMotorista, path + String::indexOf( path, "/") + 1);
	String::strcat( path, ".drv");
}

U16 Fachada::getIdRoteiro(U32 indiceRoteiro)
{
	U16 idRoteiro = 0xFFFF;
	String::strcpy(pathBuffer, "roteiros/");
	this->listaRoteiros.readNameArquivo(indiceRoteiro, this->pathBuffer + String::indexOf( this->pathBuffer, "/") + 1);
	String::strcat( this->pathBuffer, ".rot");

	Roteiro roteiro(this->plataforma->sistemaArquivo);
	if(roteiro.open(this->pathBuffer) == 0)
	{
		idRoteiro = roteiro.readID();
		roteiro.close();
	}
	return idRoteiro;
}

void Fachada::getLabelNumeroRoteiro(U32 indiceRoteiro, char* labelNumRoteiroSelecionado)
{
	xSemaphoreTake(this->semaforo, portMAX_DELAY);

	String::strcpy(this->pathBuffer, "roteiros/");
	this->listaRoteiros.readNameArquivo(indiceRoteiro, this->pathBuffer + String::indexOf( this->pathBuffer, "/") + 1);
	String::strcat( this->pathBuffer, ".rot");

	Roteiro roteiro(this->plataforma->sistemaArquivo);
	if(roteiro.open(this->pathBuffer) == 0)
	{
		roteiro.readLabelNumero(labelNumRoteiroSelecionado);
		roteiro.close();
	}

	xSemaphoreGive(this->semaforo);
}

void Fachada::getLabelRoteiroIda(U32 indiceRoteiro, char* labelRoteiroSelecionado)
{
	String::strcpy(this->pathBuffer, "roteiros/");
	this->listaRoteiros.readNameArquivo(indiceRoteiro, this->pathBuffer + String::indexOf( this->pathBuffer, "/") + 1);
	String::strcat( this->pathBuffer, ".rot");

	Roteiro roteiro(this->plataforma->sistemaArquivo);
	if(roteiro.open(this->pathBuffer) == 0)
	{
		roteiro.readLabelRoteiro(labelRoteiroSelecionado);
		roteiro.close();
	}
}

void Fachada::getLabelRoteiroVolta(U32 indiceRoteiro, char* labelRoteiroSelecionado)
{
	String::strcpy(this->pathBuffer, "roteiros/");
	this->listaRoteiros.readNameArquivo(indiceRoteiro, this->pathBuffer + String::indexOf( this->pathBuffer, "/") + 1);
	String::strcat( this->pathBuffer, ".rot");

	Roteiro roteiro(this->plataforma->sistemaArquivo);
	if(roteiro.open(this->pathBuffer) == 0)
	{
		roteiro.readLabelRoteiroVolta(labelRoteiroSelecionado);
		roteiro.close();
	}
}

void Fachada::getLabelNumeroComRoteiro(U32 indiceRoteiro, char* label)
{
	String::strcpy(this->pathBuffer, "roteiros/");
	this->listaRoteiros.readNameArquivo(indiceRoteiro, this->pathBuffer + String::indexOf( this->pathBuffer, "/") + 1);
	String::strcat( this->pathBuffer, ".rot");

	Roteiro roteiro(this->plataforma->sistemaArquivo);
	if(roteiro.open(this->pathBuffer) == 0)
	{
		U32 length = 0;

		roteiro.readLabelNumero(label);

		length += Math::min(16, String::getLength(label));
		label[length] = ':';
		length += Math::min(16 - length, 1);

		roteiro.readLabelRoteiro(label + length);

		roteiro.close();
	}
}

void Fachada::getLabelIdComNomeMotorista(U32 indiceMotorista, char* label)
{
	String::strcpy(this->pathBuffer, "drivers/");
	this->listaMotoristas.readNameArquivo(indiceMotorista, this->pathBuffer + String::indexOf( this->pathBuffer, "/") + 1);
	String::strcat( this->pathBuffer, ".drv");

	Motorista motorista(this->plataforma->sistemaArquivo);
	if(motorista.open(this->pathBuffer) == 0)
	{
		U32 length = 0;

		motorista.readLabelIdentificacao(label);

		length += Math::min(16, String::getLength(label));
		label[length] = ':';
		length += Math::min(16 - length, 1);

		motorista.readLabelNome(label + length);

		motorista.close();
	}
}

Fachada::Resultado Fachada::setRoteiroSelecionado(U16 idRoteiro)
{

	//Verifica se a fun��o de mudar roteiro est� bloqueada
	// e, caso a sess�o esteja aberta (senha de desbloqueio), o contador � resetado (mais 60 segundos de fun��es liberadas)
	if(!this->isFuncaoLiberada(ParametrosFixos::SELECAO_ROTEIRO) )
	{
		return FUNCAO_BLOQUEADA;
	}

	//Procura o roteiro com o ID correto
	U32 qntRoteiros = this->listaRoteiros.readQntArquivos();
	for (U32 indice = 0; indice < qntRoteiros; ++indice) {
		//L� o path do roteiro
		String::strcpy(this->pathBuffer, "roteiros/");
		this->listaRoteiros.readNameArquivo(indice, this->pathBuffer + String::indexOf( this->pathBuffer, "/") + 1);
		String::strcat( this->pathBuffer, ".rot");
		//Abre o roteiro
		Roteiro roteiro(this->plataforma->sistemaArquivo);
		if(roteiro.open(this->pathBuffer) != 0){
			return FALHA_OPERACAO;
		}
		//L� o ID
		U16 id = roteiro.readID();
		//Fecha o arquivo de roteiro
		roteiro.close();
		//Compara os IDs
		if(id == idRoteiro){
			//Muda o roteiro selecionado
			this->e2prom.writeRoteiroSelecionado(indice);
			//Indica que os paineis precisam ser sincronizados
			U8 qntPaineis = this->parametrosFixos.readQntPaineis();
			for (U8 p = 0; p < qntPaineis; ++p) {
				this->setPainelSincronizado(p, false);
			}
			//Sincroniza a catraca
			this->setCatracaSincronizada(false);
			//Indica que o APP precisa ser sincronizado
			this->setAPPStatusConfig(STATUS_CONFIG_APP_PRECISANDO_SINCRONIZAR_PARAMETROS);
			return SUCESSO;
		}
	}

	return PARAMETRO_INVALIDO;
}

Fachada::Resultado Fachada::setRoteiroSelecionado(U32 indiceRoteiro)
{
	return setRoteiroSelecionado(indiceRoteiro, false);
}

Fachada::Resultado Fachada::setRoteiroSelecionado(U32 indiceRoteiro, bool force)
{
	Fachada::Resultado resultado = SUCESSO;

	//Verifica se a fun��o de mudar roteiro est� bloqueada
	// e, caso a sess�o esteja aberta (senha de desbloqueio), o contador � resetado (mais 60 segundos de fun��es liberadas)
	if(!force && (!this->isFuncaoLiberada(ParametrosFixos::SELECAO_ROTEIRO)))
	{
		return FUNCAO_BLOQUEADA;
	}
	//Muda o roteiro selecionado
	this->e2prom.writeRoteiroSelecionado(indiceRoteiro);
	//Sincroniza os arquivos dos paineis
	U8 qntPaineis = this->parametrosFixos.readQntPaineis();
	for (U8 p = 0; p < qntPaineis; ++p) {
		this->setPainelSincronizado(p, false);
	}
	//Sincroniza a catraca
	this->setCatracaSincronizada(false);
	//Indica que o APP precisa ser sincronizado
	this->setAPPStatusConfig(STATUS_CONFIG_APP_PRECISANDO_SINCRONIZAR_PARAMETROS);

	//Envia roteiro pela serial apenas se estiver conectado no padr�o antigo
	//LD 11 baudrate 57600
	if((this->getBaudrateSerial() != 0) && (indiceRoteiro <= 0xFFFF)) //Formato antigo tem roteiro U16
	{
		CRC16CCITT crc;
		crc.reset();
		U16 indiceRoteiroLD11 = indiceRoteiro & 0xFFFF;

		this->getLabelNumeroRoteiro(indiceRoteiroLD11, (char*) this->buffer);
		//Limita em 16 caracteres
		this->buffer[17] =  '\0';
		U8 lengthTexto = String::getLength((char*) this->buffer);
		//Trim dos espa�os do final do n�mero do roteiro
		for (; lengthTexto > 0; lengthTexto--) {
			if(this->buffer[lengthTexto - 1] != ' ')
			{break;}
		}

		crc.update((U8*)&lengthTexto, sizeof(lengthTexto));
		crc.update(this->buffer, lengthTexto);
		crc.update((U8*)&indiceRoteiroLD11, sizeof(indiceRoteiroLD11));
		U16 crcCalc = crc.getValue();

		this->plataforma->uart->write((U8*)"//vv", 4);
		this->plataforma->uart->write(&lengthTexto, sizeof(lengthTexto));
		this->plataforma->uart->write(this->buffer, lengthTexto);
		this->plataforma->uart->write((U8*)&indiceRoteiroLD11, sizeof(indiceRoteiroLD11));
		this->plataforma->uart->write((U8*)&crcCalc, sizeof(crcCalc));
	}

	return resultado;
}

Fachada::Resultado Fachada::setRoteiroSelecionado(char* numRoteiro)
{
	return this->setRoteiroSelecionado(numRoteiro, false);
}
Fachada::Resultado Fachada::setRoteiroSelecionado(char* numRoteiro, bool force)
{

	//Verifica se a fun��o de mudar roteiro est� bloqueada
	// e, caso a sess�o esteja aberta (senha de desbloqueio), o contador � resetado (mais 60 segundos de fun��es liberadas)
	if(!force && (!this->isFuncaoLiberada(ParametrosFixos::SELECAO_ROTEIRO)))
	{
		return FUNCAO_BLOQUEADA;
	}

	//Procura o roteiro com o n�mero correto
	U32 qntRoteiros = this->listaRoteiros.readQntArquivos();
	for (U32 indice = 0; indice < qntRoteiros; ++indice) {
		xSemaphoreTake(this->semaforo, portMAX_DELAY);
		//L� o path do roteiro
		String::strcpy(this->pathBuffer, "roteiros/");
		this->listaRoteiros.readNameArquivo(indice, this->pathBuffer + String::indexOf( this->pathBuffer, "/") + 1);
		String::strcat( this->pathBuffer, ".rot");
		//Abre o roteiro
		Roteiro roteiro(this->plataforma->sistemaArquivo);
		if(roteiro.open(this->pathBuffer) != 0){
			xSemaphoreGive(this->semaforo);
			return FALHA_OPERACAO;
		}
		//L� o n�mero do roteiro
		roteiro.readLabelNumero((char*)this->buffer);
		//Fecha o arquivo de roteiro
		roteiro.close();
		xSemaphoreGive(this->semaforo);
		//Compara os n�meros
		if(String::equals((char*)this->buffer, numRoteiro)){
			//Muda o roteiro selecionado
			this->e2prom.writeRoteiroSelecionado(indice);
			//Sincroniza os arquivos dos paineis
			U8 qntPaineis = this->parametrosFixos.readQntPaineis();
			for (U8 p = 0; p < qntPaineis; ++p) {
				this->setPainelSincronizado(p, false);
			}
			//Sincroniza a catraca
			this->setCatracaSincronizada(false);
			//Indica que o APP precisa ser sincronizado
			this->setAPPStatusConfig(STATUS_CONFIG_APP_PRECISANDO_SINCRONIZAR_PARAMETROS);
			return SUCESSO;
		}
	}

	return PARAMETRO_INVALIDO;
}

Fachada::Resultado Fachada::setTarifaMemoria(U32 tarifa)
{
	Fachada::Resultado resultado = SUCESSO;

	//Muda a tarifa
	this->e2prom.writeTarifa(tarifa);
	//Sincroniza os arquivos dos paineis
	U8 qntPaineis = this->parametrosFixos.readQntPaineis();
	for (U8 p = 0; p < qntPaineis; ++p) {
		this->setPainelSincronizado(p, false);
	}
	//Sincroniza a catraca
	this->setCatracaSincronizada(false);

	return resultado;
}

U32 Fachada::getTarifaMemoria()
{
	return this->e2prom.readTarifa();
}

U8 Fachada::getRegiaoSelecionada()
{
	return this->e2prom.readRegiaoSelecionada();
}

Fachada::Resultado Fachada::setRegiaoSelecionada(U8 indiceRegiao)
{
	return setRegiaoSelecionada(indiceRegiao, false);
}

Fachada::Resultado Fachada::setRegiaoSelecionada(U8 indiceRegiao, bool force)
{
	Fachada::Resultado resultado = SUCESSO;

	//Verifica se a fun��o de mudar regi�o est� bloqueada
	// e, caso a sess�o esteja aberta (senha de desbloqueio), o contador � resetado (mais 60 segundos de fun��es liberadas)
	if(!force && (!this->isFuncaoLiberada(ParametrosFixos::SELECAO_REGIAO)))
	{
		return FUNCAO_BLOQUEADA;
	}
	//Muda a regi�o selecionada
	this->e2prom.writeRegiaoSelecionada(indiceRegiao);
	this->regiao.close();
	ListaArquivos listaRegioes(this->plataforma->sistemaArquivo);
	if(listaRegioes.open("regioes/regioes.lst") == 0){
		String::strcpy(this->pathBuffer, "regioes/");
		listaRegioes.readNameArquivo(indiceRegiao, this->pathBuffer + String::indexOf( this->pathBuffer, "/") + 1);
		String::strcat( this->pathBuffer, ".rgn");
		this->regiao.open(this->pathBuffer);
		listaRegioes.close();
	}

	if(resultado == SUCESSO){
		//Sincroniza os arquivos dos paineis
		U8 qntPaineis = this->parametrosFixos.readQntPaineis();
		for (U8 p = 0; p < qntPaineis; ++p) {
			this->setPainelSincronizado(p, false);
		}
	}

	return resultado;
}

U32 Fachada::getMensagemSelecionada(U8 painel)
{
	return this->e2prom.readMensagemSelecionada(painel);
}

U32 Fachada::getMensagemSelecionada()
{
	return this->getMensagemSelecionada(this->painelSelecionado);
}

U32 Fachada::getMensagemSecundariaSelecionada(U8 painel)
{
	return this->e2prom.readMensagemSecundariaSelecionada(painel);
}

U32 Fachada::getMensagemSecundariaSelecionada()
{
	return this->getMensagemSecundariaSelecionada(this->painelSelecionado);
}

void Fachada::getPathMensagem(U32 indiceMensagem, char* path)
{
	String::strcpy(path, "msgs/");
	this->listaMensagens.readNameArquivo(indiceMensagem, path + String::indexOf( path, "/") + 1);
	String::strcat( path, ".msg");
}

void Fachada::getLabelMessagem(U32 indiceMensagem, char* labelMensagemSelecionada)
{
	String::strcpy(this->pathBuffer, "msgs/");
	this->listaMensagens.readNameArquivo(indiceMensagem, this->pathBuffer + String::indexOf( this->pathBuffer, "/") + 1);
	String::strcat( this->pathBuffer, ".msg");

	Mensagem mensagem(this->plataforma->sistemaArquivo);
	if(mensagem.open(this->pathBuffer) == 0)
	{
		mensagem.readLabelMessagem(labelMensagemSelecionada);
		mensagem.close();
	}
}

Fachada::Resultado Fachada::setMensagemSelecionada(U32 indiceMensagem)
{
	return setMensagemSelecionada(indiceMensagem, false);
}

Fachada::Resultado Fachada::setMensagemSelecionada(U32 indiceMensagem, bool force)
{
	Fachada::Resultado resultado = SUCESSO;

	//Verifica se a fun��o de mudar mensagem est� bloqueada
	// e, caso a sess�o esteja aberta (senha de desbloqueio), o contador � resetado (mais 60 segundos de fun��es liberadas)
	if(!force && (!this->isFuncaoLiberada(ParametrosFixos::SELECAO_MENSAGEM)))
	{
		return FUNCAO_BLOQUEADA;
	}

	//Muda a mensagem selecionada
	this->e2prom.writeMensagemSelecionada(this->painelSelecionado, indiceMensagem);
	//Sincroniza os arquivos do painel
	this->setPainelSincronizado(this->painelSelecionado, false);

	return resultado;
}

Fachada::Resultado Fachada::setMensagemSelecionada(U16 idMensagem)
{
	return setMensagemSelecionada(idMensagem, false);
}

Fachada::Resultado Fachada::setMensagemSelecionada(U16 idMensagem, bool force)
{
	//Verifica se a fun��o de mudar mensagem est� bloqueada
	// e, caso a sess�o esteja aberta (senha de desbloqueio), o contador � resetado (mais 60 segundos de fun��es liberadas)
	if(!force && (!this->isFuncaoLiberada(ParametrosFixos::SELECAO_MENSAGEM)))
	{
		return FUNCAO_BLOQUEADA;
	}

	//Procura a mensagem com o ID correto
	U32 qntMensagens = this->listaMensagens.readQntArquivos();
	for (U32 indice = 0; indice < qntMensagens; ++indice) {
		//L� o path da mensagem
		String::strcpy(this->pathBuffer, "msgs/");
		this->listaMensagens.readNameArquivo(indice, this->pathBuffer + String::indexOf( this->pathBuffer, "/") + 1);
		String::strcat( this->pathBuffer, ".msg");
		//Abre a mensagem
		Mensagem mensagem(this->plataforma->sistemaArquivo);
		if(mensagem.open(this->pathBuffer) != 0){
			return FALHA_OPERACAO;
		}
		//L� o ID
		U16 id = mensagem.readID();
		//Fecha o arquivo de mensagem
		mensagem.close();
		//Compara os IDs
		if(id == idMensagem){
			//Muda a mensagem selecionada
			this->e2prom.writeMensagemSelecionada(this->painelSelecionado, indice);
			//Sincroniza os arquivos do painel
			this->setPainelSincronizado(this->painelSelecionado, false);
			return SUCESSO;
		}
	}

	return PARAMETRO_INVALIDO;
}

Fachada::Resultado Fachada::setMensagemSecundariaSelecionada(U32 indiceMensagem)
{
	return setMensagemSecundariaSelecionada(indiceMensagem, false);
}

Fachada::Resultado Fachada::setMensagemSecundariaSelecionada(U32 indiceMensagem, bool force)
{
	Fachada::Resultado resultado = SUCESSO;

	//Verifica se a fun��o de mudar mensagem est� bloqueada
	// e, caso a sess�o esteja aberta (senha de desbloqueio), o contador � resetado (mais 60 segundos de fun��es liberadas)
	if(!force && (!this->isFuncaoLiberada(ParametrosFixos::SELECAO_MENSAGEM2)))
	{
		return FUNCAO_BLOQUEADA;
	}

	//Muda a mensagem selecionada
	this->e2prom.writeMensagemSecundariaSelecionada(this->painelSelecionado, indiceMensagem);
	//Sincroniza os arquivos do painel
	this->setPainelSincronizado(this->painelSelecionado, false);

	return resultado;
}

Fachada::SentidoRoteiro Fachada::getSentidoRoteiro()
{
	Fachada::SentidoRoteiro sentido = IDA;
	//L� sentido do roteiro selecionado
	if(this->e2prom.readSentidoSelecionado())
	{
		sentido = IDA;
	}
	else
	{
		sentido = VOLTA;
	}

	return sentido;
}

Fachada::Resultado Fachada::setSentidoRoteiro(SentidoRoteiro sentido)
{
	return setSentidoRoteiro(sentido, false);
}

Fachada::Resultado Fachada::setSentidoRoteiro(SentidoRoteiro sentido, bool force)
{
	Fachada::Resultado resultado = SUCESSO;

	//Verifica se a fun��o de mudar sentido do roteiro est� bloqueada
	// e, caso a sess�o esteja aberta (senha de desbloqueio), o contador � resetado (mais 60 segundos de fun��es liberadas)
	if(!force && (!this->isFuncaoLiberada(ParametrosFixos::SELECAO_SENTIDO)))
	{
		return FUNCAO_BLOQUEADA;
	}
	//Mudao sentido do roteiro
	this->e2prom.writeSentidoSelecionado(sentido == IDA);
	//Sincroniza os arquivos dos paineis
	U8 qntPaineis = this->parametrosFixos.readQntPaineis();
	for (U8 p = 0; p < qntPaineis; ++p) {
		this->setPainelSincronizado(p, false);
	}
	//Indica que o APP precisa ser sincronizado
	this->setAPPStatusConfig(STATUS_CONFIG_APP_PRECISANDO_SINCRONIZAR_PARAMETROS);

	//Envia roteiro pela serial apenas se estiver conectado no padr�o antigo
	//LD 11 baudrate 57600
	if(this->getBaudrateSerial() != 0)
	{
		U8 sentidoLD11 = (sentido == IDA) ? 'I' : 'V';

		this->plataforma->uart->write((U8*)"//AA", 4);
		this->plataforma->uart->write(&sentidoLD11, sizeof(sentidoLD11));
	}

	return resultado;
}

Relogio::DataHora Fachada::getDataHora()
{
	return this->plataforma->relogio->getDataHora();
}

Fachada::Resultado Fachada::setDataHora(Relogio::DataHora dataHora)
{
	//Altera data/hora
	this->plataforma->relogio->setDataHora(dataHora);
	return SUCESSO;
}

void Fachada::getHoraSaida(U8 *horaSaida, U8 *minutosSaida)
{
	*horaSaida = this->e2prom.readHoraSaida();
	*minutosSaida = this->e2prom.readMinutosSaida();
}

Fachada::Resultado Fachada::setHoraSaida(U8 horaSaida, U8 minutosSaida)
{
	return setHoraSaida(horaSaida, minutosSaida, false);
}

Fachada::Resultado Fachada::setHoraSaida(U8 horaSaida, U8 minutosSaida, bool force)
{
	Fachada::Resultado resultado = SUCESSO;

	//Verifica se a fun��o de mudar hora de sa�da est� bloqueada
	// e, caso a sess�o esteja aberta (senha de desbloqueio), o contador � resetado (mais 60 segundos de fun��es liberadas)
	if(!force && (!this->isFuncaoLiberada(ParametrosFixos::AJUSTE_HORA_SAIDA)))
	{
		return FUNCAO_BLOQUEADA;
	}

	//Muda a hora de sa�da
	this->e2prom.writeHoraSaida(horaSaida);
	this->e2prom.writeMinutosSaida(minutosSaida);
	//Sincroniza os arquivos dos paineis
	U8 qntPaineis = this->parametrosFixos.readQntPaineis();
	for (U8 p = 0; p < qntPaineis; ++p) {
		this->setPainelSincronizado(p, false);
	}

	return resultado;
}

U8 Fachada::getQntRegioes()
{
	U8 qntRegioes = 0;

	ListaArquivos listaRegioes(this->plataforma->sistemaArquivo);
	if(listaRegioes.open("regioes/regioes.lst") == 0){
		qntRegioes = listaRegioes.readQntArquivos();
		listaRegioes.close();
	}

	return qntRegioes;
}

Regiao::FormatoDataHora Fachada::getFormatoDataHora()
{
	Regiao::FormatoDataHora formato;

	//L� formato de data/hora
	if(this->regiao.readFormatoDataHora()){
		formato = Regiao::FORMATO_AM_PM;
	}
	else{
		formato = Regiao::FORMATO_24H;
	}

	return formato;
}

char* Fachada::getIdiomaPath()
{
	this->regiao.readIdiomaPath(this->pathBuffer);
	return this->pathBuffer;
}

void Fachada::getNomeRegiao(char* nome, U8 indiceRegiao)
{
	ListaArquivos listaRegioes(this->plataforma->sistemaArquivo);
	if(listaRegioes.open("regioes/regioes.lst") == 0){
		String::strcpy(this->pathBuffer, "regioes/");
		listaRegioes.readNameArquivo(indiceRegiao, this->pathBuffer + String::indexOf( this->pathBuffer, "/") + 1);
		String::strcat( this->pathBuffer, ".rgn");
		Regiao regiao(this->plataforma->sistemaArquivo);
		if(regiao.open(this->pathBuffer) == 0){
			regiao.readNome(nome);
			regiao.close();
		}
		listaRegioes.close();
	}
}

U8 Fachada::getVersaoParametrosFixos()
{
	return this->parametrosFixos.readVersao();
}

U8 Fachada::getHoraBomDia()
{
	return this->parametrosFixos.readHoraBomDia();
}

U8 Fachada::getHoraBoaTarde()
{
	return this->parametrosFixos.readHoraBoaTarde();
}

U8 Fachada::getHoraBoaNoite()
{
	return this->parametrosFixos.readHoraBoaNoite();
}

ParametrosFixos::FuncoesBloqueadas Fachada::getFuncoesBloqueadas()
{
	return (ParametrosFixos::FuncoesBloqueadas)this->parametrosFixos.readFuncoesBloqueadas();
}

bool Fachada::isFuncaoLiberada(ParametrosFixos::FuncoesBloqueadas funcao)
{
	if(this->tempoParaBloquear < xTaskGetTickCount())
	{
		return (this->getFuncoesBloqueadas() & funcao) == 0;
	}
	// Se o tempo para bloquear for maior que o tempo atual, a sess�o ainda est� ativa
	// (As fun��es devem continuar desbloqueadas)
	else
	{
		// reseta o timeout da sess�o
		this->tempoParaBloquear = xTaskGetTickCount() + 60000;
		return true;
	}
}

U32 Fachada::getQntPaineis()
{
	if(this->getStatusFuncionamento() == FUNCIONANDO_OK){
		return this->parametrosFixos.readQntPaineis();
	}
	else{
		return 0;
	}
}

U32 Fachada::getAlturaPainel(U8 painel)
{
	PainelConfig painelConfig(this->plataforma->sistemaArquivo);
	U32 altura = 0;

	//L� path do arquivo configura��o do painel
	this->getPathPainelConfig(this->pathBuffer, painel);

	//Abre o arquivo e l� a altura do painel
	if(painelConfig.open(this->pathBuffer) == 0) {
		altura = painelConfig.readAltura();
		painelConfig.close();
	}

	return altura;
}

U32 Fachada::getLarguraPainel(U8 painel)
{
	PainelConfig painelConfig(this->plataforma->sistemaArquivo);
	U32 largura = 0;

	//L� path do arquivo configura��o do painel
	this->getPathPainelConfig(this->pathBuffer, painel);

	//Abre o arquivo e l� a largura do painel
	if(painelConfig.open(this->pathBuffer) == 0) {
		largura = painelConfig.readLargura();
		painelConfig.close();
	}

	return largura;
}

U32 Fachada::getQntRoteiros()
{
	return this->listaRoteiros.readQntArquivos();
}

U16 Fachada::getQntMotoristas()
{
	return this->listaMotoristas.readQntArquivos();
}

U32 Fachada::getQntMensagens()
{
	return this->listaMensagens.readQntArquivos();
}

U8 Fachada::getPainelSelecionado()
{
	return this->painelSelecionado;
}

U64 Fachada::getPainelNumserie(U8 painel)
{
	return this->e2prom.readPainelNumSerie(painel);
}

U8 Fachada::getPainelNetAddress(U8 painel)
{
	return this->e2prom.readPainelNetAddress(painel);
}

Fachada::Resultado Fachada::setPainelNetAddress(U8 painel, U8 netAddress)
{
	this->e2prom.writePainelNetAddress(painel, netAddress);
	return SUCESSO;
}

Fachada::StatusConfigAPP Fachada::getAPPStatusConfig()
{
	return (StatusConfigAPP)this->e2prom.readStatusConfigAPP();
}

void Fachada::setAPPStatusConfig(StatusConfigAPP status)
{
	this->e2prom.writeStatusConfigAPP(status);
}

Fachada::StatusConfigAdaptador Fachada::getAdaptadorCatracaStatusConfig()
{
	return (StatusConfigAdaptador)(this->e2prom.readStatusConfigAdaptador() & 0x0F);
}

Fachada::StatusConfigAdaptador Fachada::getAdaptadorSensoresStatusConfig()
{
	return (StatusConfigAdaptador)(this->e2prom.readStatusConfigAdaptador() >> 4);
}

void Fachada::setAdaptadorCatracaStatusConfig(StatusConfigAdaptador status)
{
	//Guarda status da Catraca nos 4 bits menos significativos
	U8 temp = (StatusConfigAdaptador)(this->e2prom.readStatusConfigAdaptador() & 0xF0);
	temp |= (status & 0x0F);
	this->e2prom.writeStatusConfigAdaptador((StatusConfigAdaptador)temp);
}

void Fachada::setAdaptadorSensoresStatusConfig(StatusConfigAdaptador status)
{
	//Guarda status da Catraca nos 4 bits mais significativos
	U8 temp = this->e2prom.readStatusConfigAdaptador() & 0x0F;
	temp |= (status << 4);
	this->e2prom.writeStatusConfigAdaptador((StatusConfigAdaptador)temp);
}

void Fachada::setAPPHabilitado(bool habilitaAPP)
{
	this->appHabilitado = habilitaAPP;
}

bool Fachada::isAPPHabilitado()
{
	return this->appHabilitado;
}

bool Fachada::isAdaptadorCatracaHabilitado()
{
	return this->adaptadorCatracaHabilitado;
}

bool Fachada::isAdaptadorSensoresHabilitado()
{
	return this->adaptadorSensoresHabilitado;
}

void Fachada::setAdaptadorCatracaHabilitado(bool habilitaAdaptadorCatraca)
{
	this->adaptadorCatracaHabilitado = habilitaAdaptadorCatraca;
}

void Fachada::setAdaptadorSensoresHabilitado(bool habilitaAdaptadorSensores)
{
	this->adaptadorSensoresHabilitado = habilitaAdaptadorSensores;
}

bool Fachada::isAPPDetectado()
{
	return this->appNetAddress != 0xFF;
}

bool Fachada::isAdaptadorCatracaDetectado()
{
	return this->adaptadorCatracaNetAddress != 0xFF;
}

bool Fachada::isAdaptadorSensoresDetectado()
{
	return this->adaptadorSensoresNetAddress != 0xFF;
}

bool Fachada::isAdaptadorCatracaNaRede()
{
	IdentificationProtocol::PingReply* reply = this->idProtocol->sendPing(this->adaptadorCatracaNetAddress, 250);
	while(reply->getStatus() == reply->WAITING){
		vTaskDelay(1);
	}
	return reply->getStatus() == reply->SUCCESS;
}

bool Fachada::isAdaptadorSensoresNaRede()
{
	IdentificationProtocol::PingReply* reply = this->idProtocol->sendPing(this->adaptadorSensoresNetAddress, 250);
	while(reply->getStatus() == reply->WAITING){
		vTaskDelay(1);
	}
	return reply->getStatus() == reply->SUCCESS;
}

bool Fachada::isCatracaSincronizada()
{
	return this->statusSincronizacaoCatraca;
}

void Fachada::setCatracaSincronizada(bool status)
{
	this->statusSincronizacaoCatraca = status;
}

bool Fachada::isSensoresSincronizados()
{
	return this->statusSincronizacaoSensores;
}

void Fachada::setSensoresSincronizados(bool status)
{
	this->statusSincronizacaoSensores = status;
}

Fachada::Resultado Fachada::getAPPStatusFuncionamento(U32* status, bool (*isCanceled)(void))
{
	Resultado resultado = ERRO_COMUNICACAO;

	//Inicia a conex�o com o APP
	IAsyncHandler* handler = this->network->beginConnect(this->appNetAddress, 0);
	//Aguarda por 2 segundos a conex�o ser efetuada
	for(int i = 0; i < 2000 && !handler->isCompleted(); i++){
		//Verifica se a opera��o foi cancelada
		if(isCanceled()){
			handler->cancel();
			return OPERACAO_CANCELADA;
		}
		vTaskDelay(1);
	}
	//Se a opera��o n�o foi completada significa que ocorreu timeout
	if(!handler->isCompleted()){
		handler->cancel();
		return ERRO_COMUNICACAO;
	}
	//Conecta com o APP
	ConnectionClient* client = this->network->endConnect();
	if(!client){
		return ERRO_COMUNICACAO;
	}
	client->setReceiveTimeout(200);
	//L� o status de funcionamento do APP
	if(this->painelReadStatus(client, status) == Protocolotrf::SUCESSO_COMM){
		resultado = SUCESSO;
	}
	//Fecha a conex�o
	client->close();

	return resultado;
}

bool Fachada::isAPPNaRede()
{
	IdentificationProtocol::PingReply* reply = this->idProtocol->sendPing(this->appNetAddress, 250);
	while(reply->getStatus() == reply->WAITING){
		vTaskDelay(1);
	}
	return reply->getStatus() == reply->SUCCESS;
}

void Fachada::writePainelParameters(U8 painel, U8 netAddress, U64 numSerie)
{
	this->e2prom.writePainelParameters(painel, netAddress, numSerie);
}

Fachada::Resultado Fachada::travarPaineis()
{
	U8 qntPaineis = this->getQntPaineis();
	U8* trava = this->buffer;

	//Verifica se o Controlador est� corretamente emparelhado (de acordo com a configura��o vigente)
	if(this->e2prom.readQntPaineis() != qntPaineis){
		return NAO_EMPARELHADO;
	}
	//Verifica se os pain�is j� est�o travados
	if(this->isPaineisTravados()){
		return PAINEIS_TRAVADOS;
	}
	//Calcula chave de trava dos pain�is (baseada nos n�meros de s�rie )
	SHA256::starts(&this->sha256Strct);
	for (U8 p = 0; p < qntPaineis; ++p) {
		U64 nSerie = this->getPainelNumserie(p);
		SHA256::update(&this->sha256Strct, (U8*)&nSerie, sizeof(nSerie));
	}
	SHA256::finish(&this->sha256Strct, trava);
	//Envia a trava para os pain�is
	for (U8 p = 0; p < qntPaineis; ++p) {
		//Conecta com o painel
		ConnectionClient* client = this->network->connect(this->getPainelNetAddress(p), 0);
		if(client == 0){
			return ERRO_COMUNICACAO;
		}
		client->setReceiveTimeout(2000);
		//Envia o comando para enviar a trava ao painel
		if(this->painelWrTrava(client, trava) != Protocolotrf::SUCESSO_COMM){
			client->close();
			return ERRO_COMUNICACAO;
		}
		client->close();
	}
	//Grava a trava na E2PROM
	this->e2prom.writeTravaPaineis(trava);

	return SUCESSO;
}

Fachada::Resultado Fachada::destravarPaineis(InfoPainelListado* paineis, U8 qntPaineis, U8* senhaDestrava)
{
	//Calcula qual deve ser a senha de destravar os pain�is
	U8* senhaEsperada = this->buffer;
	*(U32*)senhaEsperada = this->getIdDestravaPaineis(paineis, qntPaineis);
	char* chave = "tu �s o cildo da quic�rcia lira";
	SHA256::starts(&this->sha256Strct);
	SHA256::update(&this->sha256Strct, (U8*)senhaEsperada, 4);
	SHA256::update(&this->sha256Strct, (U8*)chave, String::getLength(chave));
	SHA256::finish(&this->sha256Strct, senhaEsperada);

	//Verifica a senha
	if(*(U32*)senhaDestrava != *(U32*)senhaEsperada){
		return SENHA_INCORRETA;
	}

	//Envia comando de destrava para os pain�is
	for (U8 p = 0; p < qntPaineis; ++p) {
		//Conecta com o painel
		ConnectionClient* client = this->network->connect(paineis[p].netAddr, 0);
		if(client == 0){
			return ERRO_COMUNICACAO;
		}
		client->setReceiveTimeout(2000);
		//Envia o comando para apagar a trava do painel
		if(this->painelApagarTrava(client) != Protocolotrf::SUCESSO_COMM){
			client->close();
			return ERRO_COMUNICACAO;
		}
		client->close();
	}
	//Apaga a trava na E2PROM
	U8* trava = this->buffer;
	trava[0] = trava[1] = trava[2] = trava[3] = 0xFF;
	this->e2prom.writeTravaPaineis(trava);

	return SUCESSO;
}

Fachada::Resultado Fachada::setPainelSelecionado(U8 painel)
{
	//Verifica se o painel existe
	if(painel >= this->parametrosFixos.readQntPaineis()){
		return PARAMETRO_INVALIDO;
	}

	this->painelSelecionado = painel;
	return SUCESSO;
}

Fachada::Resultado Fachada::resetConfigFabricaPainel(U8 painel)
{
	Protocolotrf::ResultadoComunicacao result = Protocolotrf::TIMEOUT_COMM;
	//Conecta com o painel
	ConnectionClient* client = this->network->connect(this->getPainelNetAddress(painel), 0);
	if(client == 0)
	{
		return ERRO_COMUNICACAO;
	}
	//Envia o comando para apagar o painel
	client->setReceiveTimeout(2000);

	//Se os paineis est�o travados ent�o envia comando para liberar a trava (autentica��o)
	if(this->isPaineisTravados()){
		U8 chave[4];
		this->e2prom.readTravaPaineis(chave);
		Protocolotrf::ResultadoComunicacao res = this->painelLiberarTrava(client, chave);
		if(res == Protocolotrf::TIMEOUT_COMM)
		{
			client->close();
			return ERRO_COMUNICACAO;
		}
		else if(res == Protocolotrf::PARAMETRO_INVALIDO_COMM){
			client->close();
			return PAINEIS_TRAVADOS;
		}
	}

	result = this->painelApagar(client, 0);
	//Verifica se conseguiu apagar o painel
	if(result == Protocolotrf::SUCESSO_COMM){
		//Envia o comando para formatar o painel
		client->setReceiveTimeout(5000);
		result = this->remoteFactory(client);
		//Envia o comando de reset
		if(result == Protocolotrf::SUCESSO_COMM){
			this->remoteReset(client);
		}
	}
	//Fecha a conex�o
	client->close();

	if(result == Protocolotrf::SUCESSO_COMM){
		return SUCESSO;
	}
	if(result == Protocolotrf::TIMEOUT_COMM){
		return ERRO_COMUNICACAO;
	}
	else{
		return FALHA_OPERACAO;
	}
}

Fachada::Resultado Fachada::getPainelStatusFuncionamento(U8 painel, U32* status, bool (*isCanceled)(void))
{
	Resultado resultado = ERRO_COMUNICACAO;

	//Inicia a conex�o com o painel
	IAsyncHandler* handler = this->network->beginConnect(this->getPainelNetAddress(painel), 0);
	//Aguarda por 2 segundos a conex�o ser efetuada
	for(int i = 0; i < 2000 && !handler->isCompleted(); i++){
		//Verifica se a opera��o foi cancelada
		if(isCanceled()){
			handler->cancel();
			return OPERACAO_CANCELADA;
		}
		vTaskDelay(1);
	}
	//Se a opera��o n�o foi completada significa que ocorreu timeout
	if(!handler->isCompleted()){
		handler->cancel();
		return ERRO_COMUNICACAO;
	}
	//Conecta com o painel
	ConnectionClient* client = this->network->endConnect();
	if(!client){
		return ERRO_COMUNICACAO;
	}
	client->setReceiveTimeout(200);
	//L� o status de funcionamento do painel
	if(this->painelReadStatus(client, status) == Protocolotrf::SUCESSO_COMM){
		resultado = SUCESSO;
	}
	//Fecha a conex�o
	client->close();

	return resultado;
}

bool Fachada::isPainelNaRede(U8 painel)
{
	IdentificationProtocol::PingReply* reply = this->idProtocol->sendPing(this->getPainelNetAddress(painel), 250);
	while(reply->getStatus() == reply->WAITING){
		vTaskDelay(1);
	}
	return reply->getStatus() == reply->SUCCESS;
}

Fachada::Resultado Fachada::coletarDump(void (*incrementProgress)(void), char* destFolder)
{
	// Forma o nome do arquivo
	String::strcpy(this->pathBuffer,destFolder);
	String::strcat(this->pathBuffer,"/ctrl.nfx");

	// Cria o arquivo
	FatFs::File dump(&this->plataforma->usbHost->fileSystem);
	if(dump.open(this->pathBuffer,'w') != 0)
	{
		return FALHA_OPERACAO;
	}

	//Copia o dump para o arquivo
	for(U32 page = 0; page < this->plataforma->sistemaArquivo->getUsedPages(); page++)
	{
		//L� o chunk e tags
		if(this->plataforma->sistemaArquivo->readChunk(page, this->chunk, this->tags) != 0){
			dump.close();
			return FALHA_OPERACAO;
		}
		//Assinala a progress�o do processo de carregamento de nova configura��o
		incrementProgress();

		//Escreve os tags do chunk
		if(dump.write(16, this->tags) != 16){
			dump.close();
			return FALHA_OPERACAO;
		}
		//Limpa bytes de 17 a 64 de this->tags
		String::memset(&this->tags[16], 0x00, TAMANHO_SPARE_FLASH - 16);
		if(dump.write(TAMANHO_SPARE_FLASH - 16, &this->tags[16]) != TAMANHO_SPARE_FLASH - 16){
			dump.close();
			return FALHA_OPERACAO;
		}
		//Escreve o conte�do do chunk
		if(dump.write(((NandFFS::ChunkTags*)this->tags)->chunkLength, this->chunk) != ((NandFFS::ChunkTags*)this->tags)->chunkLength){
			dump.close();
			return FALHA_OPERACAO;
		}
	}
	//Fecha o arquivo de dump
	dump.close();

	// Forma o nome do arquivo
	String::strcpy(this->pathBuffer,destFolder);
	String::strcat(this->pathBuffer,"/ctrl.e2p");

	// Cria o arquivo
	FatFs::File dumpE2prom(&this->plataforma->usbHost->fileSystem);
	if(dumpE2prom.open(this->pathBuffer,'w')!=0)
	{
		return FALHA_OPERACAO;
	}

	// Ser� utilizado o chunk como buffer
	U32 size = this->plataforma->e2prom->getTotalSize();
	if(size > sizeof(this->chunk))
	{
		dumpE2prom.close();
		return FALHA_OPERACAO;
	}
	//Copia os bytes da e2prom para o buffer(chunk)
	if(this->plataforma->e2prom->read(this->chunk,0,size) != size)
	{
		dumpE2prom.close();
		return FALHA_OPERACAO;
	}
	if(dumpE2prom.write(size, this->chunk) !=  size){
		dumpE2prom.close();
		return FALHA_OPERACAO;
	}

	dumpE2prom.close();
	return SUCESSO;
}

Fachada::Resultado Fachada::coletarLog(void (*incrementProgress)(void))
{
	char logPath[9] = "LOG";

	//Procura um nome de um arquivo inexistente para gravar o LOG
	bool achou = false;
	for (int indice = 0; indice < 100; ++indice) {
		FatFs::File fileLog(&this->plataforma->usbHost->fileSystem);
		Converter::itoa(indice, logPath, 3, 2, Converter::BASE_10);
		if(fileLog.open(logPath, 'r') != 0){
			fileLog.close();
			achou = true;
			break;
		}
	}

	//Se n�o achou um path vago para o dump ent�o apaga a pasta "LOG00"
	if(!achou){
		Converter::itoa(0, logPath, 3, 2, Converter::BASE_10);
	}

	//Copia o arquivo
	return copyFile(
			this->plataforma->sistemaArquivo,
			"LOG",
			&this->plataforma->usbHost->fileSystem,
			logPath,
			incrementProgress,
			false);
}

Fachada::Resultado Fachada::coletarDumpPainel(U8 painel, void (*incrementProgress)(void), char* destFolder)
{
	// Forma o nome do arquivo
	String::strcpy(this->pathBuffer,destFolder);
	String::strcat(this->pathBuffer,"/pn");
	U32 length = String::getLength(this->pathBuffer);
	Converter::itoa(painel, this->pathBuffer + length, 2,Converter::BASE_10);
	String::strcpy(this->pathBuffer + length + 2,".nfx");

	// Cria o arquivo
	FatFs::File dump(&this->plataforma->usbHost->fileSystem);
	if(dump.open(this->pathBuffer,'w')!=0)
	{
		return FALHA_OPERACAO;
	}

	//Conecta com o painel
	ConnectionClient* client = this->network->connect(this->getPainelNetAddress(painel), 0);
	if(!client)
	{
		dump.close();
		return ERRO_COMUNICACAO;
	}
	client->setReceiveTimeout(2000);

	//Se os paineis est�o travados ent�o envia comando para liberar a trava (autentica��o)
	if(this->isPaineisTravados()){
		U8 chave[4];
		this->e2prom.readTravaPaineis(chave);
		Protocolotrf::ResultadoComunicacao res = this->painelLiberarTrava(client, chave);
		if(res == Protocolotrf::TIMEOUT_COMM)
		{
			client->close();
			return ERRO_COMUNICACAO;
		}
		else if(res == Protocolotrf::PARAMETRO_INVALIDO_COMM){
			client->close();
			return PAINEIS_TRAVADOS;
		}
	}

	U32 qntPaginas = 0;
	if(this->remoteRdnumchunks(client,&qntPaginas)!= Protocolotrf::SUCESSO_COMM)
	{
		dump.close();
		client->close();
		return ERRO_COMUNICACAO;
	}
	// Copia o dump para o arquivo
	for (U32 page = 0; page < qntPaginas; page++)
	{
		//L� o chunk e tags
		if(this->remoteRdchunk(client,this->chunk,page) != 0){
			client->close();
			dump.close();
			return FALHA_OPERACAO;
		}
		//Assinala a progress�o do processo de carregamento de nova configura��o
		incrementProgress();

		//Escreve os tags do chunk
		if(dump.write(16, this->tags) != 16){
			client->close();
			dump.close();
			return FALHA_OPERACAO;
		}
		//Limpa bytes de 17 a 64 de this->tags
		String::memset(&this->tags[16], 0x00, TAMANHO_SPARE_FLASH - 16);
		if(dump.write(TAMANHO_SPARE_FLASH - 16, &this->tags[16]) != TAMANHO_SPARE_FLASH - 16){
			dump.close();
			return FALHA_OPERACAO;
		}
		//Escreve o conte�do do chunk
		if(dump.write(((NandFFS::ChunkTags*)this->tags)->chunkLength, this->chunk) != ((NandFFS::ChunkTags*)this->tags)->chunkLength){
			client->close();
			dump.close();
			return FALHA_OPERACAO;
		}
	}

	//Fecha o arquivo de dump
	dump.close();

	//Coleta o dump da e2prom

	// Forma o nome do arquivo
	String::strcpy(this->pathBuffer,destFolder);
	String::strcat(this->pathBuffer,"/pn");
	length = String::getLength(this->pathBuffer);
	Converter::itoa(painel, this->pathBuffer + length, 2,Converter::BASE_10);
	String::strcpy(this->pathBuffer + length + 2,".e2p");

	// Cria o arquivo
	FatFs::File dumpE2prom(&this->plataforma->usbHost->fileSystem);
	if(dumpE2prom.open(this->pathBuffer,'w')!=0)
	{
		client->close();
		return FALHA_OPERACAO;
	}

	U32 bytesLidos = 0;
	// Ser� utilizado o chunk como buffer
	if(this->remoteRdDumpE2prom(client,this->chunk,&bytesLidos)!= Protocolotrf::SUCESSO_COMM)
	{
		dumpE2prom.close();
		client->close();
		return ERRO_COMUNICACAO;
	}
	// Escreve no arquivo o dump da e2prom
	if(dumpE2prom.write(bytesLidos, this->chunk) !=  bytesLidos){
		dumpE2prom.close();
		client->close();
		return FALHA_OPERACAO;
	}

	dumpE2prom.close();
	client->close();
	return SUCESSO;
}

bool Fachada::formatarPendrive(FatFs* sistemaArquivo)
{
	return sistemaArquivo->format();
}

Fachada::Resultado Fachada::apagarArquivos()
{
	this->closeAllFiles();
	this->e2prom.setParametrosVariaveisStatus(STATUS_CONFIG_INVALIDA);
	this->setStatusFuncionamento(ERRO_CONFIGURACAO_INCONSISTENTE);
	return this->plataforma->sistemaArquivo->format() == 0 ? SUCESSO : FALHA_OPERACAO;
}

Fachada::Resultado Fachada::format()
{
	this->closeAllFiles();
	this->e2prom.setParametrosVariaveisStatus(STATUS_CONFIG_NAO_VERIFICADA);
	this->setStatusFuncionamento(CONFIGURANDO_REMOTAMENTE);
	this->configurandoTimeout = xTaskGetTickCount() + 3000;
	return this->plataforma->sistemaArquivo->format() == 0 ? SUCESSO : FALHA_OPERACAO;
}

Fachada::Resultado Fachada::apagarArquivosPainel(U8 numeroPainel, bool reset)
{
	Protocolotrf::ResultadoComunicacao result = Protocolotrf::TIMEOUT_COMM;
	//Conecta com o painel
	ConnectionClient* client = this->network->connect(this->getPainelNetAddress(numeroPainel), 0);
	if(client == 0)
	{
		return ERRO_COMUNICACAO;
	}
	client->setReceiveTimeout(2000);

	//Se os paineis est�o travados ent�o envia comando para liberar a trava (autentica��o)
	if(this->isPaineisTravados()){
		U8 chave[4];
		this->e2prom.readTravaPaineis(chave);
		Protocolotrf::ResultadoComunicacao res = this->painelLiberarTrava(client, chave);
		if(res == Protocolotrf::TIMEOUT_COMM)
		{
			client->close();
			return ERRO_COMUNICACAO;
		}
		else if(res == Protocolotrf::PARAMETRO_INVALIDO_COMM){
			client->close();
			return PAINEIS_TRAVADOS;
		}
	}

	//Envia o comando para formatar o painel
	result = this->painelApagar(client, 0xFFFF);
	if(result == Protocolotrf::SUCESSO_COMM){
		//Envia o comando para formatar o painel
		client->setReceiveTimeout(5000);
		result = this->remoteFformat(client);
		//Se for pra resetar ent�o envia o comando de reset
		if(reset) this->remoteReset(client);
	}
	//Fecha a conex�o
	client->close();

	if(result == Protocolotrf::SUCESSO_COMM){
		return SUCESSO;
	}
	else if(result == Protocolotrf::TIMEOUT_COMM){
		return ERRO_COMUNICACAO;
	}
	//Painel travado por outro Controlador
	else if(result == Protocolotrf::ACESSO_NEGADO_COMM){
		//Seta para n�o formatar
		this->setPainelPrecisandoFormatar(numeroPainel, false);
		return PAINEIS_TRAVADOS;
	}
	else{
		return FALHA_OPERACAO;
	}
}

bool Fachada::acenderPainel(U8 numeroPainel, U16 tempo)
{
	return acenderPainelByAddr(this->getPainelNetAddress(numeroPainel), tempo);
}

bool Fachada::acenderPainelByAddr(U8 netAddr, U16 tempo)
{
	bool result = false;
	//Conecta com o painel
	ConnectionClient* client = this->network->connect(netAddr, 0);
	if(client)
	{
		client->setReceiveTimeout(2000);

		//Se os paineis est�o travados ent�o envia comando para liberar a trava (autentica��o)
		if(this->isPaineisTravados()){
			U8 chave[4];
			this->e2prom.readTravaPaineis(chave);
			Protocolotrf::ResultadoComunicacao res = this->painelLiberarTrava(client, chave);
			if(res != Protocolotrf::SUCESSO_COMM)
			{
				client->close();
				return false;
			}
		}

		//Envia comando de acender painel
		Protocolotrf::ResultadoComunicacao res = this->painelAcender(client, tempo);
		result = (res == Protocolotrf::SUCESSO_COMM);
		//Fecha a conex�o
		client->close();
	}
	return result;
}

bool Fachada::apagarPainel(U8 numeroPainel, U16 tempo)
{
	return apagarPainelByAddr(this->getPainelNetAddress(numeroPainel), tempo);
}

bool Fachada::apagarPainelByAddr(U8 netAddr, U16 tempo)
{
	bool result = false;
	//Conecta com o painel
	ConnectionClient* client = this->network->connect(netAddr, 0);
	if(client)
	{
		client->setReceiveTimeout(2000);

		//Se os paineis est�o travados ent�o envia comando para liberar a trava (autentica��o)
		if(this->isPaineisTravados()){
			U8 chave[4];
			this->e2prom.readTravaPaineis(chave);
			Protocolotrf::ResultadoComunicacao res = this->painelLiberarTrava(client, chave);
			if(res == Protocolotrf::TIMEOUT_COMM)
			{
				client->close();
				return ERRO_COMUNICACAO;
			}
			else if(res == Protocolotrf::PARAMETRO_INVALIDO_COMM){
				client->close();
				return PAINEIS_TRAVADOS;
			}
		}

		//Envia comando para apagar painel
		result = (this->painelApagar(client, tempo) == Protocolotrf::SUCESSO_COMM);
		//Fecha a conex�o
		client->close();
	}
	return result;
}

void Fachada::resetSystem()
{
	this->plataforma->resetSystem();
}

bool Fachada::isPainelSincronizado(U8 painel)
{
	U8 indice = painel / 8;
	U8 offset = painel % 8;
	return ((this->statusSincronizacaoPaineis[indice] >> offset) & 0x01);
}

void Fachada::setPainelSincronizado(U8 painel, bool status)
{
	U8 indice = painel / 8;
	U8 offset = painel % 8;
	if(status){
		this->statusSincronizacaoPaineis[indice] |= (1 << offset);
	}
	else{
		this->statusSincronizacaoPaineis[indice] &= ~(1 << offset);
	}
}

Fachada::Resultado Fachada::apagarArquivosAPP(bool reset)
{
	if(this->isAPPDetectado() == false){
		return ERRO_COMUNICACAO;
	}

	Protocolotrf::ResultadoComunicacao result = Protocolotrf::TIMEOUT_COMM;
	//Conecta com o painel
	ConnectionClient* client = this->network->connect(this->appNetAddress, 0);
	if(client == 0)
	{
		return ERRO_COMUNICACAO;
	}
	//Envia o comando para formatar o painel
	client->setReceiveTimeout(5000);
	result = this->remoteFformat(client);
	client->setReceiveTimeout(2000);
	//Se for pra resetar ent�o envia o comando de reset
	if(reset) this->remoteReset(client);
	//Fecha a conex�o
	client->close();

	if(result == Protocolotrf::SUCESSO_COMM){
		return SUCESSO;
	}
	if(result == Protocolotrf::TIMEOUT_COMM){
		return ERRO_COMUNICACAO;
	}
	else{
		return FALHA_OPERACAO;
	}
}

Fachada::Resultado Fachada::resetConfigFabricaAPP()
{
	if(this->isAPPDetectado() == false){
		return ERRO_COMUNICACAO;
	}

	Protocolotrf::ResultadoComunicacao result = Protocolotrf::TIMEOUT_COMM;
	//Conecta com o painel
	ConnectionClient* client = this->network->connect(this->appNetAddress, 0);
	if(client == 0)
	{
		return ERRO_COMUNICACAO;
	}
	//Envia o comando para formatar o APP
	client->setReceiveTimeout(5000);
	result = this->remoteFactory(client);
	client->setReceiveTimeout(2000);
	if(result == Protocolotrf::SUCESSO_COMM){
		this->remoteReset(client);
	}
	//Fecha a conex�o
	client->close();

	if(result == Protocolotrf::SUCESSO_COMM){
		return SUCESSO;
	}
	if(result == Protocolotrf::TIMEOUT_COMM){
		return ERRO_COMUNICACAO;
	}
	else{
		return FALHA_OPERACAO;
	}
}

bool Fachada::isPainelPrecisandoFormatar(U8 painel)
{
	return this->e2prom.readPrecisaFormatarPainel(painel);
}

void Fachada::setPainelPrecisandoFormatar(U8 painel, bool precisando)
{
	this->e2prom.writePrecisaFormatarPainel(painel, precisando);
}

bool Fachada::remoteSyncArquivo(ConnectionClient* client, char* path, bool (*isCanceled)(void))
{
	return this->remoteSyncArquivo(client, path, path, isCanceled);
}

bool Fachada::remoteSyncArquivo(ConnectionClient* client, char* srcPath, char* destPath, bool (*isCanceled)(void))
{
	//Abre o arquivo local
	if(this->file.open(srcPath, 'r') == 0){
		bool sucesso = false;
		//Verifica se o arquivo remoto existe
		Protocolotrf::ResultadoComunicacao res = this->remoteFopen(client, destPath, 'r');
		if(res == Protocolotrf::SUCESSO_COMM) {
			//Fecha o arquivo
			this->file.close();
			//Arquivo j� sincronizado
			return true;
		}
		//Painel travado por outro Controlador
		else if(res == Protocolotrf::ACESSO_NEGADO_COMM){
			//Fecha o arquivo
			this->file.close();
			//Marca como sincronizado
			return true;
		}
		else if(res != Protocolotrf::ARQUIVO_NAO_ENCONTRADO_COMM){
			//Fecha o arquivo
			this->file.close();
			//Falha de comunica��o
			return false;
		}
		//Envia comando de abrir arquivo remoto no modo escrita
		Protocolotrf::ResultadoComunicacao answer = this->remoteFopen(client, destPath, 'w');
		if(answer == Protocolotrf::SUCESSO_COMM){
			U16 qntBytes = 0;
			this->file.seek(0);
			//Envia o conte�do do arquivo
			while((qntBytes = this->file.read(sizeof(this->buffer), this->buffer)) > 0){
				sucesso = false;
				const U8 MAX_RETRIES = 3;
				for(U8 retry = 0; retry < MAX_RETRIES && !sucesso; retry++){
					//Verifica se a opera��o foi cancelada
					if(isCanceled()){
						break;
					}
					client->getInputStream()->clear();
					//Envia comando de escrita
					if(this->remoteFwrite(client, qntBytes, this->buffer) != qntBytes){
						vTaskDelay(10);
					}
					else{
						//Comando enviado com sucesso.
						sucesso = true;
					}
				}
				//Se deu erro em todas as tentativas ent�o para de enviar o arquivo
				if(!sucesso){
					break;
				}
			}

			//Envia comando	de fechar arquivo remoto
			if(sucesso){
				this->remoteFclose(client);
			}
		}
		//Fecha o arquivo local
		this->file.close();

		return sucesso;
	}

	return false;
}

bool Fachada::remoteSyncDir(trf::service::net::ConnectionClient* client, char* srcPath, char* destPath, bool (*isCanceled)(void))
{
	NandFFS::Folder folder(this->plataforma->sistemaArquivo);

	String::strcpy(this->pathBuffer, srcPath);
	U8 index = String::getLength(this->pathBuffer);
	String::strcpy(this->pathBuffer2, destPath);
	U8 index2 = String::getLength(this->pathBuffer2);

	if(folder.open(this->pathBuffer) == 0){
		U8 childType;
		while(folder.getNext(&this->pathBuffer[index+1], &childType) == 0){
			//Verifica se a opera��o foi cancelada
			if(isCanceled()){
				return false;
			}
			this->pathBuffer[index] = '/';
			String::strcpy(&this->pathBuffer2[index2], &this->pathBuffer[index]);
			//File
			if(childType == NandFFS::FILE_OBJECT){
				if(this->remoteSyncArquivo(client, this->pathBuffer, this->pathBuffer2, isCanceled) == false){
					return false;
				}
			}
			//Folder
			else{
				if(this->remoteSyncDir(client, this->pathBuffer, this->pathBuffer2, isCanceled) == false){
					return false;
				}
			}
		}
	}

	return true;
}

Protocolotrf::ResultadoComunicacao Fachada::remoteFopen(trf::service::net::ConnectionClient* client, char* path, U8 modo)
{
	CRC16CCITT crc16;
	U16 crcEnviado;
	//Quantidade de bytes
	U8 pathLength = String::getLength(path);
	//Calcula o CRC
	crc16.reset();
	crc16.update((U8*)&pathLength, sizeof(pathLength));
	crc16.update((U8*)path, pathLength);
	crc16.update(&modo, sizeof(modo));
	crcEnviado = crc16.getValue();
	//Limpa o buffer de recebimento
	client->getInputStream()->clear();
	//Envia comando de abrir arquivo
	client->send((U8*)"//FOPEN\0\0\0\0\0\0\0", 14);
	//Envia o comprimento do path
	client->send((U8*)&pathLength, sizeof(pathLength));
	//Envia o path do arquivo
	client->send((U8*)path, pathLength);
	//Envia modo de abertura
	client->send(&modo, sizeof(modo));
	//Envia CRC
	client->send((U8*)&crcEnviado, sizeof(crcEnviado));
	//L� resposta
	Protocolotrf::ResultadoComunicacao answer = Protocolotrf::TIMEOUT_COMM;
	client->receive((U8 *)&answer, 1);
	return answer;
}

Protocolotrf::ResultadoComunicacao Fachada::remoteFclose(trf::service::net::ConnectionClient* client)
{
	//Limpa o buffer de recebimento
	client->getInputStream()->clear();
	//Envia comando de abrir arquivo
	client->send((U8*)"//FCLOSE\0\0\0\0\0\0", 14);
	//L� resposta
	Protocolotrf::ResultadoComunicacao answer = Protocolotrf::TIMEOUT_COMM;
	client->receive((U8 *)&answer, 1);
	return answer;
}

U32 Fachada::remoteFread(trf::service::net::ConnectionClient* client, U32 nBytes, U8* buffer)
{
	CRC16CCITT crc16;
	U16 crcEnviado;
	//Calcula o CRC
	crc16.reset();
	crc16.update((U8*)&nBytes, sizeof(nBytes));
	crcEnviado = crc16.getValue();
	//Limpa o buffer de recebimento
	client->getInputStream()->clear();
	//Envia comando de abrir arquivo
	client->send((U8*)"//FREAD\0\0\0\0\0\0\0", 14);
	//Envia a quantidade de bytes a ser lida
	client->send((U8*)&nBytes, sizeof(nBytes));
	//Envia CRC
	client->send((U8*)&crcEnviado, sizeof(crcEnviado));
	//L� quantidade de bytes lidos
	U32 answer = 0;
	client->receive((U8 *)&answer, sizeof(answer));
	//Escreve os bytes recebidos no buffer
	client->receive(buffer, answer);
	//L� o CRC dos dados
	U16 crcRecebido;
	client->receive((U8 *)&crcRecebido, sizeof(crcRecebido));
	//Calcula o CRC
	crc16.reset();
	crc16.update((U8 *)&answer, sizeof(answer));
	crc16.update(buffer, answer);
	//Verifica o CRC
	if(crcRecebido != crc16.getValue()){
		return 0;
	}
	return answer;
}

U32 Fachada::remoteFwrite(trf::service::net::ConnectionClient* client, U32 nBytes, U8* buffer)
{
	CRC16CCITT crc16;
	U16 crcEnviado;
	//Calcula o CRC
	crc16.reset();
	crc16.update((U8*)&nBytes, sizeof(nBytes));
	crc16.update(buffer, nBytes);
	crcEnviado = crc16.getValue();
	//Limpa o buffer de recebimento
	client->getInputStream()->clear();
	//Envia comando de abrir arquivo
	client->send((U8*)"//FWRITE\0\0\0\0\0\0", 14);
	//Envia a quantidade de bytes a ser escrita
	client->send((U8*)&nBytes, sizeof(nBytes));
	//Envia os bytes do buffer
	client->send(buffer, nBytes);
	//Envia CRC
	client->send((U8*)&crcEnviado, sizeof(crcEnviado));
	//L� quantidade de bytes escritos
	U32 answer = 0;
	client->receive((U8 *)&answer, sizeof(answer));
	return answer;
}

Protocolotrf::ResultadoComunicacao Fachada::remoteFseek(trf::service::net::ConnectionClient* client, U32 posicao)
{
	CRC16CCITT crc16;
	U16 crcEnviado;
	//Calcula o CRC
	crc16.reset();
	crc16.update((U8*)&posicao, sizeof(posicao));
	crcEnviado = crc16.getValue();
	//Limpa o buffer de recebimento
	client->getInputStream()->clear();
	//Envia comando de abrir arquivo
	client->send((U8*)"//FSEEK\0\0\0\0\0\0\0", 14);
	//Envia a posi��o
	client->send((U8*)&posicao, sizeof(posicao));
	//Envia CRC
	client->send((U8*)&crcEnviado, sizeof(crcEnviado));
	//L� resposta
	Protocolotrf::ResultadoComunicacao answer = Protocolotrf::TIMEOUT_COMM;
	client->receive((U8 *)&answer, 1);
	return answer;
}

Protocolotrf::ResultadoComunicacao Fachada::remoteFformat(trf::service::net::ConnectionClient* client)
{
	//Limpa o buffer de recebimento
	client->getInputStream()->clear();
	//Envia comando de abrir arquivo
	client->send((U8*)"//FSFORMAT\0\0\0\0", 14);
	//L� resposta
	Protocolotrf::ResultadoComunicacao answer;
	if(client->receive((U8 *)&answer, sizeof(answer)) == 0)
	{
		answer = Protocolotrf::TIMEOUT_COMM;
	}
	return answer;
}

trf::application::Protocolotrf::ResultadoComunicacao Fachada::remoteFactory(trf::service::net::ConnectionClient* client)
{
	//Limpa o buffer de recebimento
	client->getInputStream()->clear();
	//Envia comando de abrir arquivo
	client->send((U8*)"//FACTORY\0\0\0\0\0", 14);
	//L� resposta
	Protocolotrf::ResultadoComunicacao answer = Protocolotrf::TIMEOUT_COMM;
	client->receive((U8 *)&answer, 1);
	return answer;
}

Protocolotrf::ResultadoComunicacao Fachada::remoteWrchunk(trf::service::net::ConnectionClient* client, U8* chunk)
{
	U8* tags = chunk + 2048;
	U32 length = ((NandFFS::ChunkTags*)tags)->chunkLength;
	//Calcula o CRC
	CRC16CCITT crc16;
	crc16.reset();
	crc16.update(tags, 16);
	crc16.update(chunk, length);
	U16 crcEnviado = crc16.getValue();
	//Limpa o buffer de recebimento
	client->getInputStream()->clear();
	//Envia comando de abrir arquivo
	client->send((U8*)"//WRCHUNK\0\0\0\0\0", 14);
	//Envia o chunk tag
	client->send(tags, 16);
	//Envia o chunk
	client->send(chunk, length);
	//Envia CRC
	client->send((U8*)&crcEnviado, sizeof(crcEnviado));
	//L� resposta
	Protocolotrf::ResultadoComunicacao answer;
	if(client->receive((U8 *)&answer, sizeof(answer)) == 0)
	{
		answer = Protocolotrf::TIMEOUT_COMM;
	}
	return answer;
}

Protocolotrf::ResultadoComunicacao Fachada::remoteRdchunk(trf::service::net::ConnectionClient* client, U8* chunk, U32 pageNumber)
{
	Protocolotrf::ResultadoComunicacao answer = Protocolotrf::SUCESSO_COMM;
	U32 qntBytes = 2048 + 64;

	CRC16CCITT crc16;

	//Limpa o buffer de recebimento
	client->getInputStream()->clear();

	//Envia comando de abrir arquivo
	client->send((U8*)"//RDCHUNK\0\0\0\0\0", 14);

	//Envia o numero da pagina desejada
	client->send((U8*)&pageNumber, sizeof(pageNumber));

	// Atualiza o CRC com o valor a ser enviado
	crc16.reset();
	crc16.update((U8*)&pageNumber,sizeof(pageNumber));
	U16 crcEnviado = crc16.getValue();

	//Envia CRC
	client->send((U8*)&crcEnviado, sizeof(crcEnviado));

	//L� resposta
	if(client->receive((U8 *)&answer, sizeof(answer)) == 0)
	{
		return Protocolotrf::TIMEOUT_COMM;
	}

	// Se a resposta for diferente de sucesso, significa que o chunk n�o foi lido com sucesso.
	if(answer != Protocolotrf::SUCESSO_COMM)
	{
		return answer;
	}

	// Recebe os bytes do chunk
	U32 count = 0;
	for (; count < qntBytes; count++) {
		if(client->receive(chunk + count, 1) == 0){
			break;
		}
	}

	// Verifica se conseguiu receber todos os bytes
	if(count < qntBytes){
		return Protocolotrf::TIMEOUT_COMM;
	}

	// L� o CRC dos dados
	U16 crcRecebido;
	client->receive((U8 *)&crcRecebido, sizeof(crcRecebido));

	// Calcula o crc
	crc16.reset();
	crc16.update(chunk,qntBytes);

	//Verifica o CRC
	if(crcRecebido != crc16.getValue()){
		return Protocolotrf::ERRO_CRC_COMM;
	}

	return answer;
}


Protocolotrf::ResultadoComunicacao Fachada::remoteRdnumchunks(trf::service::net::ConnectionClient* client, U32* numChunks)
{
	CRC16CCITT crc16;

	//Limpa o buffer de recebimento
	client->getInputStream()->clear();

	//Envia comando de abrir arquivo
	client->send((U8*)"//RDNUMCHUNKS\0", 14);

	//L� o numero de chunks
	if(client->receive((U8 *)numChunks, sizeof(*numChunks)) != sizeof(*numChunks)){
		return Protocolotrf::TIMEOUT_COMM;
	}

	// L� o CRC dos dados
	U16 crcRecebido;
	if(client->receive((U8 *)&crcRecebido, sizeof(crcRecebido)) != sizeof(crcRecebido))
	{
		return Protocolotrf::TIMEOUT_COMM;
	}

	// Calcula o crc
	crc16.reset();
	crc16.update((U8 *)numChunks,sizeof(*numChunks));

	//Verifica o CRC
	if(crcRecebido != crc16.getValue()){
		return Protocolotrf::ERRO_CRC_COMM;
	}

	return Protocolotrf::SUCESSO_COMM;
}

Protocolotrf::ResultadoComunicacao Fachada::remoteRdDumpE2prom(trf::service::net::ConnectionClient* client, U8* buffer, U32* bytesRead)
{
	Protocolotrf::ResultadoComunicacao answer = Protocolotrf::SUCESSO_COMM;

	CRC16CCITT crc16;
	// inicia como 0 bytes lidos
	*bytesRead = 0;
	//Limpa o buffer de recebimento
	client->getInputStream()->clear();

	//Envia comando de abrir arquivo
	client->send((U8*)"//RDE2PROMDMP\0", 14);

	//L� resposta
	if(client->receive((U8 *)&answer, sizeof(answer)) == 0)
	{
		return Protocolotrf::TIMEOUT_COMM;
	}

	// Se a resposta for diferente de sucesso, significa que o buffer n�o foi lido com sucesso.
	if(answer != Protocolotrf::SUCESSO_COMM)
	{
		return answer;
	}

	U32 qntBytes = 0;
	//L� qtd de bytes a serem lidos
	if(client->receive((U8 *)&qntBytes, sizeof(qntBytes)) == 0)
	{
		return Protocolotrf::TIMEOUT_COMM;
	}

	// Recebe os bytes do buffer
	U32 count = 0;
	for (; count < qntBytes; count++) {
		if(client->receive(buffer + count, 1) == 0){
			break;
		}
	}
	*bytesRead = count;
	// Verifica se conseguiu receber todos os bytes
	if(count < qntBytes){
		return Protocolotrf::TIMEOUT_COMM;
	}

	// L� o CRC dos dados
	U16 crcRecebido;
	client->receive((U8 *)&crcRecebido, sizeof(crcRecebido));

	// Calcula o crc
	crc16.reset();
	crc16.update((U8*)&qntBytes,sizeof(qntBytes));
	crc16.update(buffer,qntBytes);

	//Verifica o CRC
	if(crcRecebido != crc16.getValue()){
		return Protocolotrf::ERRO_CRC_COMM;
	}

	return answer;
}

trf::application::Protocolotrf::ResultadoComunicacao Fachada::painelWrParams(trf::service::net::ConnectionClient* client, E2PROM::FormatoParametrosVariaveisPainel* params)
{
	//Calcula o CRC
	CRC16CCITT crc16;
	crc16.reset();
	crc16.update((U8*)params, sizeof(E2PROM::FormatoParametrosVariaveisPainel));
	U16 crcEnviado = crc16.getValue();
	//Limpa o buffer de recebimento
	client->getInputStream()->clear();
	//Envia comando de alterar par�metros vari�veis
	client->send((U8*)"//WRPARAMS\0\0\0\0", 14);
	//Envia os segundos
	client->send((U8*)params, sizeof(E2PROM::FormatoParametrosVariaveisPainel));
	//Envia o CRC
	client->send((U8*)&crcEnviado, sizeof(crcEnviado));
	//L� resposta
	Protocolotrf::ResultadoComunicacao answer = Protocolotrf::TIMEOUT_COMM;
	client->receive((U8 *)&answer, 1);
	return answer;
}

bool Fachada::painelSyncParamVar(trf::service::net::ConnectionClient* client, U8 painel)
{
	//Obt�m os par�metros vari�veis do painel
	this->e2prom.readParametrosVariaveisPainel(painel, (E2PROM::FormatoParametrosVariaveisPainel*)this->buffer);
	//Tenta 3 vezes para enviar os par�metros
	Protocolotrf::ResultadoComunicacao res = Protocolotrf::TIMEOUT_COMM;
	for (U8 retry = 0; retry < 3; ++retry) {
		res = this->painelWrParams(client, (E2PROM::FormatoParametrosVariaveisPainel*)this->buffer);
		if(res == Protocolotrf::SUCESSO_COMM){
			//Transmitiu com sucesso
			return true;
		}
		else if(res == Protocolotrf::ACESSO_NEGADO_COMM){
			//Painel travado por outro Controlador
			return true;
		}
	}

	return false;
}

bool Fachada::syncArquivosPainel(U8 painel, void (*incrementProgress)(void), bool (*isCanceled)(void))
{
	bool result = false;

	//Conecta com o painel
	ConnectionClient* client = this->network->connect(this->getPainelNetAddress(painel), 0);
	if(client){
		//Configura o timeout de recebimento
		client->setReceiveTimeout(2000);

		//Se os paineis est�o travados ent�o envia comando para liberar a trava (autentica��o)
		if(this->isPaineisTravados()){
			U8 chave[4];
			this->e2prom.readTravaPaineis(chave);
			Protocolotrf::ResultadoComunicacao res = this->painelLiberarTrava(client, chave);
			//Verifica se n�o foi poss�vel liberar trava devido ao fato de o painel n�o estar travado
			if(res == Protocolotrf::OPERACAO_NAO_PERMITIDA_COMM){
				//Limpa a trava local for�ando a refazer a trava com todos os pain�is
				*((U32*)chave) = 0xFFFFFFFF;
				this->e2prom.writeTravaPaineis(chave);
			}
			//Verifica se houve algum outro erro
			else if(res != Protocolotrf::SUCESSO_COMM)
			{
				client->close();
				return false;
			}
		}

		// Sincroniza o arquivo de lista de roteiros
		if(this->remoteSyncArquivo(client, "roteiros/roteiros.lst", isCanceled) == false){
			client->close();
			return false;
		}
		//Incrementa a barra de progresso
		if(incrementProgress)incrementProgress();

		// Sincroniza o arquivo de lista de motoristas
		if(this->remoteSyncArquivo(client, "drivers/drivers.lst", isCanceled) == false){
			client->close();
			return false;
		}
		//Incrementa a barra de progresso
		if(incrementProgress)incrementProgress();

		// Sincroniza o arquivo de lista de mensagens
		if(this->remoteSyncArquivo(client, "msgs/msgs.lst", isCanceled) == false){
			client->close();
			return false;
		}
		//Incrementa a barra de progresso
		if(incrementProgress)incrementProgress();

		// Sincroniza o arquivo de lista de regi�es
		if(this->remoteSyncArquivo(client, "regioes/regioes.lst", isCanceled) == false){
			client->close();
			return false;
		}
		//Incrementa a barra de progresso
		if(incrementProgress)incrementProgress();

		// Sincroniza o arquivo de parametros fixos
		if(this->remoteSyncArquivo(client, "param.fix", isCanceled) == false){
			client->close();
			return false;
		}
		//Incrementa a barra de progresso
		if(incrementProgress)incrementProgress();

		//L� as informa��es de vers�o do Painel
		trf::application::Version version;
		String::memset((U8*)version.produto, '\0', sizeof(version.produto));
		this->remoteRdProduto(client, (U8*)version.produto);
		this->remoteRdVersao(client, (U8*)&version.familia);
		//Verifica se existe os arquivos do firmware do painel
		if(this->verificarAtualizacaoFirmwarePainel(&version))
		{
			// Sincroniza o arquivo de firmware
			if(String::startWith(version.produto, "Painelpontos"))
			{
				if(this->remoteSyncArquivo(client, "firmware/painel.fir", "firmware/firmware.fir", isCanceled) == false){
					client->close();
					return false;
				}
			}
			else if(String::startWith(version.produto, "PainelMultilinhas"))
			{
				if(this->remoteSyncArquivo(client, "firmware/multilin.fir", "firmware/firmware.fir", isCanceled) == false){
					client->close();
					return false;
				}
			}
			else if(String::startWith(version.produto, "PainelMultiplex2x8"))
			{
				if(this->remoteSyncArquivo(client, "firmware/mux_2x8.fir", "firmware/firmware.fir", isCanceled) == false){
					client->close();
					return false;
				}
			}
			else if(String::startWith(version.produto, "PainelMultiplex2x13"))
			{
				if(this->remoteSyncArquivo(client, "firmware/mux_2x13.fir", "firmware/firmware.fir", isCanceled) == false){
					client->close();
					return false;
				}
			}
			else if(String::startWith(version.produto, "PainelMultplex2vias"))
			{
				if(this->remoteSyncArquivo(client, "firmware/mux2vias.fir", "firmware/firmware.fir", isCanceled) == false){
					client->close();
					return false;
				}
			}
			//Incrementa a barra de progresso
			if(incrementProgress)incrementProgress();
			// Sincroniza o arquivo de op��o de atualiza��o de firmware
			if(String::startWith(version.produto, "Painelpontos"))
			{
				if(this->remoteSyncArquivo(client, "firmware/painel.opt", "firmware/firmware.opt", isCanceled) == false){
					client->close();
					return false;
				}
			}
			else if(String::startWith(version.produto, "PainelMultilinhas"))
			{
				if(this->remoteSyncArquivo(client, "firmware/multilin.opt", "firmware/firmware.opt", isCanceled) == false){
					client->close();
					return false;
				}
			}
			else if(String::startWith(version.produto, "PainelMultiplex2x8"))
			{
				if(this->remoteSyncArquivo(client, "firmware/mux_2x8.opt", "firmware/firmware.opt", isCanceled) == false){
					client->close();
					return false;
				}
			}
			else if(String::startWith(version.produto, "PainelMultiplex2x13"))
			{
				if(this->remoteSyncArquivo(client, "firmware/mux_2x13.opt", "firmware/firmware.opt", isCanceled) == false){
					client->close();
					return false;
				}
			}
			else if(String::startWith(version.produto, "PainelMultplex2vias"))
			{
				if(this->remoteSyncArquivo(client, "firmware/mux2vias.opt", "firmware/firmware.opt", isCanceled) == false){
					client->close();
					return false;
				}
			}
			//Incrementa a barra de progresso
			if(incrementProgress)incrementProgress();
		}

		//Sincroniza o arquivo de configura��o do painel
		String::strcpy(this->pathBuffer, "paineis/");
		Converter::itoa(painel, this->pathBuffer + 8, 2, Converter::BASE_10);
		PainelConfig arquivoPainel(this->plataforma->sistemaArquivo);
		String::strcpy(this->pathBuffer + 10, "/painel.cfg");
		if(this->remoteSyncArquivo(client, this->pathBuffer, this->pathBuffer + 11, isCanceled) == false){
			client->close();
			return false;
		}		//Incrementa a barra de progresso
		if(incrementProgress)incrementProgress();

		//Verifica se existe o arquivo de configura��o do painel
		if(arquivoPainel.open(this->pathBuffer) == 0)
		{
			//L� o caminho do arquivo de fonte do painel
			arquivoPainel.readFontePath((char*)this->pathBuffer);
			//Fecha o arquivo de configura��o do painel
			arquivoPainel.close();
			//Sincroniza o arquivo de fonte
			if(this->remoteSyncArquivo(client, (char*)this->pathBuffer, isCanceled) == false){
				client->close();
				return false;
			}
			//Incrementa a barra de progresso
			if(incrementProgress)incrementProgress();
		}

		//Sincroniza o arquivo de video de emerg�ncia (somente para o painel 0)
		if(painel == 0){
			String::strcpy(this->pathBuffer, "paineis/");
			Converter::itoa(painel, this->pathBuffer + 8, 2, Converter::BASE_10);
			String::strcpy(this->pathBuffer + 10, "/emerg.pls");
			if(this->remoteSyncArquivo(client, this->pathBuffer, this->pathBuffer + 11, isCanceled) == false){
				client->close();
				return false;
			}
			//Incrementa a barra de progresso
			if(incrementProgress)incrementProgress();
		}

		//Sincroniza o arquivo de altern�ncias do painel
		String::strcpy(this->pathBuffer, "paineis/");
		Converter::itoa(painel, this->pathBuffer + 8, 2, Converter::BASE_10);
		String::strcpy(this->pathBuffer + 10, "/altern.alt");
		if(this->remoteSyncArquivo(client, this->pathBuffer, this->pathBuffer + 11, isCanceled) == false){
			client->close();
			return false;
		}
		//Incrementa a barra de progresso
		if(incrementProgress)incrementProgress();

		U8 alternanciaSelecionada = this->getAlternancia(painel);
		U8 qntExibicoes = this->getQntExibicoesAlternancia(alternanciaSelecionada, painel);
		for (U8 exb = 0; exb < qntExibicoes; ++exb) {
			U8 exibicao = this->getExibicao(alternanciaSelecionada, painel, exb);
			//Verifica se as exibi��es que utilizam informa��es do roteiro est�o selecionadas
			if(    (exibicao == Alternancias::EXIBICAO_ROTEIRO)
				|| (exibicao == Alternancias::EXIBICAO_NUMERO)
				|| (exibicao == Alternancias::EXIBICAO_TARIFA))
			{
				U32 indiceRoteiro = this->getRoteiroSelecionado();
				this->getPathRoteiro(indiceRoteiro, (char*)this->pathBuffer);
				Roteiro roteiro(this->plataforma->sistemaArquivo);

				//Verifica se o arquivo de roteiro existe
				if(roteiro.open((char*)this->pathBuffer) == 0)
				{
					//Fecha o arquivo aberto
					roteiro.close();
					//Sincroniza o arquivo de roteiro
					if(this->remoteSyncArquivo(client, (char*)this->pathBuffer, isCanceled) == false){
						client->close();
						return false;
					}
					//Incrementa a barra de progresso
					if(incrementProgress)incrementProgress();

					RoteiroPaths roteiroPaths(this->plataforma->sistemaArquivo);

					U8 pathLength = String::getLength((char*)this->pathBuffer);
					String::strcpy((char *)this->pathBuffer + pathLength - 3, "rpt");
					//Copia o path do arquivo no buffer dando um deslocamento para copiar o path da pasta
					for (int i = pathLength; i >= 0; i--) {
						this->pathBuffer[i+11] = this->pathBuffer[i];
					}
					//Adiciona o path da pasta do painel
					String::strcpy((char *)this->pathBuffer , "paineis/");
					Converter::itoa(painel, this->pathBuffer + 8, 2, Converter::BASE_10);
					this->pathBuffer[10] = '/';

					if(roteiroPaths.open((char*)this->pathBuffer) == 0){
						//Fecha o arquivo aberto
						roteiroPaths.close();
						//Sincroniza o arquivo de paths de roteiros
						if(this->remoteSyncArquivo(client, (char*)this->pathBuffer, (char*)this->pathBuffer + 11, isCanceled) == false){
							client->close();
							return false;
						}
						//Incrementa a barra de progresso
						if(incrementProgress)incrementProgress();
					}
				}
			}
			//Verifica se as exibi��es que utilizam informa��es do motorista est�o selecionadas
			else if(    (exibicao == Alternancias::EXIBICAO_ID_MOTORISTA)
				|| (exibicao == Alternancias::EXIBICAO_NOME_MOTORISTA))
			{
				U32 indiceMotorista = this->getMotoristaSelecionado();
				this->getPathMotorista(indiceMotorista, (char*)this->pathBuffer);
				Motorista motorista(this->plataforma->sistemaArquivo);

				//Verifica se o arquivo de motorista existe
				if(motorista.open((char*)this->pathBuffer) == 0)
				{
					//Fecha o arquivo aberto
					motorista.close();
					//Sincroniza o arquivo de motorista
					if(this->remoteSyncArquivo(client, (char*)this->pathBuffer, isCanceled) == false){
						client->close();
						return false;
					}
					//Incrementa a barra de progresso
					if(incrementProgress)incrementProgress();

					MotoristaPaths motoristaPaths(this->plataforma->sistemaArquivo);

					U8 pathLength = String::getLength((char*)this->pathBuffer);
					String::strcpy((char *)this->pathBuffer + pathLength - 3, "dpt");
					//Copia o path do arquivo no buffer dando um deslocamento para copiar o path da pasta
					for (int i = pathLength; i >= 0; i--) {
						this->pathBuffer[i+11] = this->pathBuffer[i];
					}
					//Adiciona o path da pasta do painel
					String::strcpy((char *)this->pathBuffer , "paineis/");
					Converter::itoa(painel, this->pathBuffer + 8, 2, Converter::BASE_10);
					this->pathBuffer[10] = '/';

					if(motoristaPaths.open((char*)this->pathBuffer) == 0){
						//Fecha o arquivo aberto
						motoristaPaths.close();
						//Sincroniza o arquivo de paths de motorista
						if(this->remoteSyncArquivo(client, (char*)this->pathBuffer, (char*)this->pathBuffer + 11, isCanceled) == false){
							client->close();
							return false;
						}
						//Incrementa a barra de progresso
						if(incrementProgress)incrementProgress();
					}
				}
			}
			//Verifica se a exibi��o com mensagem est� selecionada
			else if(exibicao == Alternancias::EXIBICAO_MENSAGEM || exibicao == Alternancias::EXIBICAO_MENSAGEM_SECUNDARIA)
			{
				U32 indiceMensagem =
						exibicao == Alternancias::EXIBICAO_MENSAGEM ?
						this->getMensagemSelecionada(painel) :
						this->getMensagemSecundariaSelecionada(painel);

				this->getPathMensagem(indiceMensagem, (char*)this->pathBuffer);
				Mensagem mensagem(this->plataforma->sistemaArquivo);
				//Verifica se o arquivo de mensagem existe
				if(mensagem.open((char*)this->pathBuffer) == 0){
					//Fecha o arquivo aberto
					mensagem.close();
					//Sincroniza o arquivo de mensagem
					if(this->remoteSyncArquivo(client, (char*)this->pathBuffer, isCanceled) == false){
						client->close();
						return false;
					}
					//Incrementa a barra de progresso
					if(incrementProgress)incrementProgress();

					MensagemPaths mensagemPaths(this->plataforma->sistemaArquivo);

					U8 pathLength = String::getLength((char*)this->pathBuffer);
					String::strcpy((char *)this->pathBuffer + pathLength - 3, "mpt");
					//Copia o path do arquivo no buffer dando um deslocamento para copiar o path da pasta
					for (int i = pathLength; i >= 0; i--) {
						this->pathBuffer[i+11] = this->pathBuffer[i];
					}
					//Adiciona o path da pasta do painel
					String::strcpy((char *)this->pathBuffer , "paineis/");
					Converter::itoa(painel, this->pathBuffer + 8, 2, Converter::BASE_10);
					this->pathBuffer[10] = '/';
					if(mensagemPaths.open((char*)this->pathBuffer) == 0){
						//Fecha o arquivo aberto
						mensagemPaths.close();
						//Sincroniza o arquivo de paths de mensagens
						if(this->remoteSyncArquivo(client, (char*)this->pathBuffer, (char*)this->pathBuffer + 11, isCanceled) == false){
							client->close();
							return false;
						}
						//Incrementa a barra de progresso
						if(incrementProgress)incrementProgress();
					}
				}
			}
			//Verifica se as exibi��es que usam informa��o de regi�o
			if(		exibicao == Alternancias::EXIBICAO_HORA_E_TEMPERATURA
				||  exibicao == Alternancias::EXIBICAO_DATA_HORA
				|| 	exibicao == Alternancias::EXIBICAO_HORA
				|| 	exibicao == Alternancias::EXIBICAO_TARIFA
				|| 	exibicao == Alternancias::EXIBICAO_VELOCIDADE
				|| 	exibicao == Alternancias::EXIBICAO_TEMPERATURA
				|| 	exibicao == Alternancias::EXIBICAO_HORA_SAIDA)
			{
				//Sincroniza o arquivo de regi�o
				U8 regiaoSelecionada = this->getRegiaoSelecionada();
				ListaArquivos listaRegioes(this->plataforma->sistemaArquivo);
				if(listaRegioes.open("regioes/regioes.lst") == 0){
					String::strcpy(this->pathBuffer, "regioes/");
					listaRegioes.readNameArquivo(regiaoSelecionada, this->pathBuffer + String::indexOf( this->pathBuffer, "/") + 1);
					String::strcat( this->pathBuffer, ".rgn");
					listaRegioes.close();
					if(this->remoteSyncArquivo(client, (char*)this->pathBuffer, isCanceled) == false){
						client->close();
						return false;
					}
					//Incrementa a barra de progresso
					if(incrementProgress)incrementProgress();
				}
			}
			//Verifica as exibi��es que usam a placa de sensores
			if(		exibicao == Alternancias::EXIBICAO_VELOCIDADE
				|| 	exibicao == Alternancias::EXIBICAO_TEMPERATURA)
			{
				this->addItemListaPaineisSensores(this->getPainelNetAddress(painel));
			}
			else
			{
				this->removeItemListaPaineisSensores(this->getPainelNetAddress(painel));
			}
			//Verifica se as exibi��es que usam informa��o de data/hora
			if(		exibicao == Alternancias::EXIBICAO_DATA_HORA
					|| 	exibicao == Alternancias::EXIBICAO_SAUDACAO
					|| 	exibicao == Alternancias::EXIBICAO_HORA
					|| 	exibicao == Alternancias::EXIBICAO_HORA_E_TEMPERATURA)
			{
				//Atualiza a data/hora
				if(this->painelWrDataHora(client, this->plataforma->relogio->getDataHora()) != Protocolotrf::SUCESSO_COMM){
					client->close();
					return false;
				}
				//Incrementa a barra de progresso
				if(incrementProgress)incrementProgress();
			}
		}

		// Sincroniza os par�metros vari�veis
		if(this->painelSyncParamVar(client, painel) == false){
			client->close();
			return false;
		}
		//Incrementa a barra de progresso
		if(incrementProgress)incrementProgress();

		//Reseta o painel
		result = this->remoteReset(client) == Protocolotrf::SUCESSO_COMM;
		//Fecha a conex�o
		client->close();
	}

	return result;
}

bool Fachada::syncParametrosAPP(trf::service::net::ConnectionClient* client, void (*incrementProgress)(void))
{
	//Envia a rota selecionada (n�mero da rota)
	this->getLabelNumeroRoteiro(this->getRoteiroSelecionado(), (char*)this->buffer);
	if(this->appWrRota(client, (char*)this->buffer) != Protocolotrf::SUCESSO_COMM){
		return false;
	}
	//Envia o sentido da rota
	U8 sentido = (this->getSentidoRoteiro() == IDA) ? 'I' : 'V';
	if(this->appWrSentido(client, sentido) != Protocolotrf::SUCESSO_COMM){
		return false;
	}
	// TODO - modificar para se adequar a versao com app com varios paineis
	//Envia o n�mero de s�rie do painel NSS
	for (U8 painel = 0, indiceAPP=0; painel < this->getQntPaineis(); ++painel)
	{
		if(this->parametrosFixos.isPainelAPP(painel))
		{
			U64 nSerie = this->getPainelNumserie(painel);

			if(this->appWrPainelNS(client, nSerie,indiceAPP) != Protocolotrf::SUCESSO_COMM){
				return false;
			}
			indiceAPP++;
		}
	}
	//Reseta o APP
	if(this->remoteReset(client) != Protocolotrf::SUCESSO_COMM){
		return false;
	}

	return true;
}

bool Fachada::sincronizarAPP(void (*incrementProgress)(void), bool (*isCanceled)(void))
{
	bool result = false;

	//Verifica se o APP foi detectado na rede
	if(isAPPDetectado() == false){
		return false;
	}

	//Conecta com o painel
	ConnectionClient* client = this->network->connect(this->appNetAddress, 0);
	if(client){
		result = true;
		//Configura o timeout de recebimento
		client->setReceiveTimeout(2000);

		//Verifica se o APP precisa sincronizar os par�metros
		if(this->getAPPStatusConfig() == Fachada::STATUS_CONFIG_APP_PRECISANDO_SINCRONIZAR_PARAMETROS){
			//Sincroniza os par�metros do APP
			if(this->syncParametrosAPP(client, incrementProgress)){
				this->setAPPStatusConfig(Fachada::STATUS_CONFIG_APP_SINCRONIZADO);
			}
			else{
				result = false;
			}
		}
		//Fecha a conex�o
		client->close();
	}

	return result;
}

bool Fachada::setAPPModoDemonstracao(bool habilitaDemoAPP)
{
	bool result = false;

	//Verifica se o APP foi detectado na rede
	if(isAPPDetectado() == false){
		return false;
	}

	//Conecta com o APP
	ConnectionClient* client = this->network->connect(this->appNetAddress, 0);
	if(client){
		//Configura o timeout de recebimento
		client->setReceiveTimeout(2000);
		//Envia comando de modo de demonstra��o para o APP
		result = this->appWrModoDemo(client, (U8) habilitaDemoAPP) == Protocolotrf::SUCESSO_COMM;
		//Fecha a conex�o
		client->close();
	}

	return result;
}

void Fachada::ativarModoEmergencia(bool ativa)
{
	static portTickType lastSending = 0;
	static bool sucessoPainel = true;

	//Garante que mudan�as de estado sejam enviadas imediatamente
	if(this->emergenciaAtiva != ativa){
		sucessoPainel = false;
	}

	//Garante que o comando n�o ser� reenviado antes de 15 segndos
	if(lastSending + 15000 < xTaskGetTickCount()){
		sucessoPainel = false;
	}

	if(!sucessoPainel){
		//Verifica se o sistema est� configurado e funcionando corretamente
		if(this->statusFuncionamento == FUNCIONANDO_OK){
			//Verifica se o painel 0 j� est� sincronizado
			if(this->isPainelSincronizado(0) && !sucessoPainel){
				//Inicia a conex�o com o painel
				IAsyncHandler* handler = this->network->beginConnect(this->getPainelNetAddress(0), 0);
				//Aguarda por 1 segundo a conex�o ser efetuada
				for(int i = 0; i < 100 && !handler->isCompleted(); i++){
					vTaskDelay(1);
				}
				//Se a opera��o n�o foi completada significa que ocorreu timeout
				if(!handler->isCompleted()){
					handler->cancel();
					return;
				}
				//Conecta com o painel 0
				ConnectionClient* client = this->network->endConnect();
				if(client){
					//Configura o timeout de recebimento
					client->setReceiveTimeout(100);
					//Se os paineis est�o travados ent�o envia comando para liberar a trava (autentica��o)
					if(this->isPaineisTravados()){
						U8 chave[4];
						this->e2prom.readTravaPaineis(chave);
						Protocolotrf::ResultadoComunicacao res = this->painelLiberarTrava(client, chave);
						if(res != Protocolotrf::SUCESSO_COMM)
						{
							client->close();
							return;
						}
					}
					//Atualiza o modo de emerg�ncia
					sucessoPainel = this->painelWrEmergencia(client, ativa) == Protocolotrf::SUCESSO_COMM;
					//Atualiza o contador de tempo
					lastSending = xTaskGetTickCount();
					//Fecha a conex�o
					client->close();
				}
			}
			//Verifica se obteve sucesso na comunica��o
			if(sucessoPainel){
				this->emergenciaAtiva = ativa;
			}
		}
	}
}

bool Fachada::syncDataHoraPainel(U8 painel)
{
	bool result = false;

	//Conecta com o painel
	ConnectionClient* client = this->network->connect(this->getPainelNetAddress(painel), 0);
	if(client){
		//Configura o timeout de recebimento
		client->setReceiveTimeout(2000);

		//Se os paineis est�o travados ent�o envia comando para liberar a trava (autentica��o)
		if(this->isPaineisTravados()){
			U8 chave[4];
			this->e2prom.readTravaPaineis(chave);
			Protocolotrf::ResultadoComunicacao res = this->painelLiberarTrava(client, chave);
			if(res != Protocolotrf::SUCESSO_COMM)
			{
				client->close();
				return false;
			}
		}

		//Atualiza a data/hora
		result = this->painelWrDataHora(client, this->plataforma->relogio->getDataHora()) == Protocolotrf::SUCESSO_COMM;

		//Fecha a conex�o
		client->close();
	}

	return result;
}

Protocolotrf::ResultadoComunicacao Fachada::remoteReset(trf::service::net::ConnectionClient* client)
{
	//Limpa o buffer de recebimento
	client->getInputStream()->clear();
	//Envia comando de abrir arquivo
	client->send((U8*)"//RESET\0\0\0\0\0\0\0", 14);
	//L� resposta
	Protocolotrf::ResultadoComunicacao answer = Protocolotrf::TIMEOUT_COMM;
	client->receive((U8 *)&answer, 1);
	return answer;
}

Protocolotrf::ResultadoComunicacao Fachada::painelWrTarifa(trf::service::net::ConnectionClient* client, U32 tarifa)
{
	//Calcula o CRC
	CRC16CCITT crc16;
	crc16.reset();
	crc16.update((U8*)&tarifa, sizeof(tarifa));
	U16 crcEnviado = crc16.getValue();
	//Limpa o buffer de recebimento
	client->getInputStream()->clear();
	//Envia comando de alterar data/hora
	client->send((U8*)"//WRTARIFA\0\0\0\0", 14);
	//Envia a data/hora
	client->send((U8*)&tarifa, sizeof(tarifa));
	//Envia o CRC
	client->send((U8*)&crcEnviado, sizeof(crcEnviado));
	//L� resposta
	Protocolotrf::ResultadoComunicacao answer = Protocolotrf::TIMEOUT_COMM;
	client->receive((U8 *)&answer, 1);
	return answer;
}

Protocolotrf::ResultadoComunicacao Fachada::remoteRdProduto(trf::service::net::ConnectionClient* client, U8* buff)
{
	//Limpa o buffer de recebimento
	client->getInputStream()->clear();
	//Envia comando de abrir arquivo
	client->send((U8*)"//RDPRODUTO\0\0\0", 14);
	//L� resposta
	U8 length;
	if(client->receive((U8 *)&length, sizeof(length)) != sizeof(length)){
		return Protocolotrf::TIMEOUT_COMM;
	}
	if(client->receive(buff, length) != length){
		return Protocolotrf::TIMEOUT_COMM;
	}
	//L� CRC
	U16 crcRecebido;
	if(client->receive((U8 *)&crcRecebido, sizeof(crcRecebido)) != sizeof(crcRecebido)){
		return Protocolotrf::TIMEOUT_COMM;
	}
	//Calcula o CRC
	CRC16CCITT crc16;
	crc16.reset();
	crc16.update((U8 *)&length, sizeof(length));
	crc16.update(buff, length);
	//Verifica o CRC
	if(crcRecebido != crc16.getValue()){
		return Protocolotrf::ERRO_CRC_COMM;
	}
	return Protocolotrf::SUCESSO_COMM;
}

Protocolotrf::ResultadoComunicacao Fachada::remoteRdVersao(trf::service::net::ConnectionClient* client, U8* versao)
{
	//Limpa o buffer de recebimento
	client->getInputStream()->clear();
	//Envia comando de abrir arquivo
	client->send((U8*)"//RDVERSAO\0\0\0\0", 14);
	//L� a vers�o
	if(client->receive(versao, 3) != 3){
		return Protocolotrf::TIMEOUT_COMM;
	}
	return Protocolotrf::SUCESSO_COMM;
}

Protocolotrf::ResultadoComunicacao Fachada::remoteRdCRCFirmware(trf::service::net::ConnectionClient* client, U16* crcFW)
{
	//Limpa o buffer de recebimento
	client->getInputStream()->clear();
	//Envia comando de abrir arquivo
	client->send((U8*)"//RDCRCFIRMWAR", 14);
	//L� a vers�o
	if(client->receive((U8*)crcFW, 2) != 2){
		return Protocolotrf::TIMEOUT_COMM;
	}
	return Protocolotrf::SUCESSO_COMM;
}

Protocolotrf::ResultadoComunicacao Fachada::painelAcender(trf::service::net::ConnectionClient* client, U16 tempo)
{
	//Limpa o buffer de recebimento
	client->getInputStream()->clear();
	//Envia comando de abrir arquivo
	client->send((U8*)"//ACENDER\0\0\0\0\0", 14);
	//Envia o tempo que o painel deve permanecer aceso
	client->send((U8*)&tempo, 2);
	//L� resposta
	Protocolotrf::ResultadoComunicacao answer = Protocolotrf::TIMEOUT_COMM;
	client->receive((U8 *)&answer, 1);
	return answer;
}

Protocolotrf::ResultadoComunicacao Fachada::painelApagar(trf::service::net::ConnectionClient* client, U16 tempo)
{
	//Limpa o buffer de recebimento
	client->getInputStream()->clear();
	//Envia comando de abrir arquivo
	client->send((U8*)"//APAGAR\0\0\0\0\0\0", 14);
	//Envia o tempo que o painel deve permanecer apagado
	client->send((U8*)&tempo, 2);
	//L� resposta
	Protocolotrf::ResultadoComunicacao answer = Protocolotrf::TIMEOUT_COMM;
	client->receive((U8 *)&answer, 1);
	return answer;
}

Protocolotrf::ResultadoComunicacao Fachada::painelWrDataHora(trf::service::net::ConnectionClient* client, Relogio::DataHora dataHora)
{
	//Calcula o CRC
	CRC16CCITT crc16;
	crc16.reset();
	crc16.update((U8*)&dataHora, sizeof(dataHora));
	U16 crcEnviado = crc16.getValue();
	//Limpa o buffer de recebimento
	client->getInputStream()->clear();
	//Envia comando de alterar data/hora
	client->send((U8*)"//WRDATAHORA\0\0", 14);
	//Envia a data/hora
	client->send((U8*)&dataHora, sizeof(dataHora));
	//Envia o CRC
	client->send((U8*)&crcEnviado, sizeof(crcEnviado));
	//L� resposta
	Protocolotrf::ResultadoComunicacao answer = Protocolotrf::TIMEOUT_COMM;
	client->receive((U8 *)&answer, 1);
	return answer;
}

Protocolotrf::ResultadoComunicacao Fachada::painelReadStatus(trf::service::net::ConnectionClient* client, U32* status)
{
	//Limpa o buffer de recebimento
	client->getInputStream()->clear();
	//Envia comando de abrir arquivo
	client->send((U8*)"//RDSTATUS\0\0\0\0", 14);
	//L� o status
	if(client->receive((U8 *)status, sizeof(*status)) != sizeof(*status)){
		return Protocolotrf::TIMEOUT_COMM;
	}
	//L� CRC
	U16 crcRecebido;
	if(client->receive((U8 *)&crcRecebido, sizeof(crcRecebido)) != sizeof(crcRecebido)){
		return Protocolotrf::TIMEOUT_COMM;
	}
	//Calcula o CRC
	CRC16CCITT crc16;
	crc16.reset();
	crc16.update((U8 *)status, sizeof(*status));
	//Verifica o CRC
	if(crcRecebido != crc16.getValue()){
		return Protocolotrf::ERRO_CRC_COMM;
	}
	return Protocolotrf::SUCESSO_COMM;
}

Protocolotrf::ResultadoComunicacao Fachada::painelRdDimensoes(trf::service::net::ConnectionClient* client, U16* altura, U16* largura)
{
	//Limpa o buffer de recebimento
	client->getInputStream()->clear();
	//Envia comando de ler as dimens�es
	client->send((U8*)"//RDDIMENSOES\0", 14);
	//L� a altura
	if(client->receive((U8 *)altura, sizeof(*altura)) != sizeof(*altura)){
		return Protocolotrf::TIMEOUT_COMM;
	}
	//L� a largura
	if(client->receive((U8 *)largura, sizeof(*largura)) != sizeof(*largura)){
		return Protocolotrf::TIMEOUT_COMM;
	}

	return Protocolotrf::SUCESSO_COMM;
}

Protocolotrf::ResultadoComunicacao Fachada::painelWrEmergencia(trf::service::net::ConnectionClient* client, bool ativa)
{
	//Limpa o buffer de recebimento
	client->getInputStream()->clear();
	//Envia comando de alterar modo de emerg�ncia
	client->send((U8*)"//WREMERGENCIA", 14);
	//Envia o par�metro de ativa��o
	client->send((U8*)&ativa, sizeof(ativa));
	//L� resposta
	Protocolotrf::ResultadoComunicacao answer = Protocolotrf::TIMEOUT_COMM;
	client->receive((U8 *)&answer, 1);
	return answer;
}

Protocolotrf::ResultadoComunicacao Fachada::painelWrTexto(trf::service::net::ConnectionClient* client, ParametrosShowText* parametrosShowText)
{
	U8 tempU8;
	U32 tempU32;
	CRC16CCITT crc16;
	crc16.reset();

	//Limpa o buffer de recebimento
	client->getInputStream()->clear();
	//Envia comando de exibir texto
	client->send((U8*)"//SHOWTEXT\0\0\0\0", 14);
	//Envia tamanho do texto
	tempU8 = String::getLength(parametrosShowText->string);
	client->send(&tempU8, sizeof(tempU8));
	crc16.update(&tempU8, sizeof(tempU8));
	//Envia o texto
	client->send((U8*)parametrosShowText->string, tempU8);
	crc16.update((U8*)parametrosShowText->string, tempU8);
	//Envia anima��o
	tempU8 = (U8) parametrosShowText->animacao;
	client->send(&tempU8, sizeof(tempU8));
	crc16.update(&tempU8, sizeof(tempU8));
	//Envia alinhamento
	tempU8 = (U8) parametrosShowText->alinhamento;
	client->send(&tempU8, sizeof(tempU8));
	crc16.update(&tempU8, sizeof(tempU8));
	//Envia delay de anima��o
	tempU32 = parametrosShowText->delayAnimacao;
	client->send((U8*)&tempU32, sizeof(tempU32));
	crc16.update((U8*)&tempU32, sizeof(tempU32));
	//Envia delay de apresenta��o
	tempU32 = parametrosShowText->delayApresentacao;
	client->send((U8*)&tempU32, sizeof(tempU32));
	crc16.update((U8*)&tempU32, sizeof(tempU32));
	//Envia otimiza��o
	tempU8 = (U8) parametrosShowText->otimizacao;
	client->send(&tempU8, sizeof(tempU8));
	crc16.update(&tempU8, sizeof(tempU8));
	//Envia n�mero de repeti��es
	tempU8 = parametrosShowText->repeticoes;
	client->send(&tempU8, sizeof(tempU8));
	crc16.update(&tempU8, sizeof(tempU8));
	//Calcula o CRC
	U16 crcEnviado = crc16.getValue();
	//Envia o CRC
	client->send((U8*)&crcEnviado, sizeof(crcEnviado));
	//L� resposta
	Protocolotrf::ResultadoComunicacao answer = Protocolotrf::TIMEOUT_COMM;
	client->receive((U8 *)&answer, 1);
	return answer;
}

Protocolotrf::ResultadoComunicacao Fachada::painelWrModoTeste(trf::service::net::ConnectionClient* client, bool ativa, U8 tipo)
{
	//Limpa o buffer de recebimento
	client->getInputStream()->clear();
	//Envia comando de alterar modo de teste
	client->send((U8*)"//WRMODOTESTE\0", 14);
	//Envia o par�metro de ativa��o
	client->send((U8*)&ativa, sizeof(ativa));
	//Envia o tipo de teste
	client->send((U8*)&tipo, sizeof(tipo));
	//L� resposta
	Protocolotrf::ResultadoComunicacao answer = Protocolotrf::TIMEOUT_COMM;
	client->receive((U8 *)&answer, 1);
	return answer;
}

Protocolotrf::ResultadoComunicacao Fachada::appWrRota(trf::service::net::ConnectionClient* client, char* rota)
{
	//Calcula o CRC
	CRC16CCITT crc16;
	crc16.reset();
	crc16.update((U8*)rota, 10);
	U16 crcEnviado = crc16.getValue();
	//Limpa o buffer de recebimento
	client->getInputStream()->clear();
	//Envia comando de alterar rota
	client->send((U8*)"//WRROTA\0\0\0\0\0\0", 14);
	//Envia a rota
	client->send((U8*)rota, 10);
	//Envia o CRC
	client->send((U8*)&crcEnviado, sizeof(crcEnviado));
	//L� resposta
	Protocolotrf::ResultadoComunicacao answer = Protocolotrf::TIMEOUT_COMM;
	client->receive((U8 *)&answer, 1);
	return answer;
}

Protocolotrf::ResultadoComunicacao Fachada::appWrSentido(trf::service::net::ConnectionClient* client, U8 sentido)
{
	//Limpa o buffer de recebimento
	client->getInputStream()->clear();
	//Envia comando de alterar o sentido da rota
	client->send((U8*)"//WRSENTIDO\0\0\0", 14);
	//Envia o sentido
	client->send((U8*)&sentido, sizeof(sentido));
	//L� resposta
	Protocolotrf::ResultadoComunicacao answer = Protocolotrf::TIMEOUT_COMM;
	client->receive((U8 *)&answer, 1);
	return answer;
}

Protocolotrf::ResultadoComunicacao Fachada::appRdModoDemo(trf::service::net::ConnectionClient* client, U8* modo)
{
	//Limpa o buffer de recebimento
	client->getInputStream()->clear();
	//Envia comando de ler o modo de demonstra��o
	client->send((U8*)"//RDMODODEMO\0\0", 14);
	//L� o modo
	if(client->receive(modo, sizeof(*modo)) != sizeof(*modo)){
		return Protocolotrf::TIMEOUT_COMM;
	}

	return Protocolotrf::SUCESSO_COMM;
}

Protocolotrf::ResultadoComunicacao Fachada::appWrModoDemo(trf::service::net::ConnectionClient* client, U8 modo)
{
	//Limpa o buffer de recebimento
	client->getInputStream()->clear();
	//Envia comando de habilitar/desabilitar o modo de demonstra��o
	client->send((U8*)"//WRMODODEMO\0\0", 14);
	//Envia o modo
	client->send(&modo, sizeof(modo));
	//L� resposta
	Protocolotrf::ResultadoComunicacao answer = Protocolotrf::TIMEOUT_COMM;
	client->receive((U8 *)&answer, 1);
	return answer;
}

Protocolotrf::ResultadoComunicacao Fachada::appWrPainelNS(trf::service::net::ConnectionClient* client, U64 nSerie)
{
	//Calcula o CRC
	CRC16CCITT crc16;
	crc16.reset();
	crc16.update((U8*)&nSerie, sizeof(nSerie));
	U16 crcEnviado = crc16.getValue();
	//Limpa o buffer de recebimento
	client->getInputStream()->clear();
	//Envia comando de alterar o endere�o do painel NSS
	client->send((U8*)"//WRPAINELNS\0\0", 14);
	//Envia o endere�o
	client->send((U8*)&nSerie, sizeof(nSerie));
	//Envia o CRC
	client->send((U8*)&crcEnviado, sizeof(crcEnviado));
	//L� resposta
	Protocolotrf::ResultadoComunicacao answer = Protocolotrf::TIMEOUT_COMM;
	client->receive((U8 *)&answer, 1);
	return answer;
}

Protocolotrf::ResultadoComunicacao Fachada::appWrPainelNS(trf::service::net::ConnectionClient* client, U64 nSerie, U8 indicePainelParaOAPP)
{
	//Calcula o CRC
	CRC16CCITT crc16;
	crc16.reset();
	crc16.update(indicePainelParaOAPP);
	crc16.update((U8*)&nSerie, sizeof(nSerie));
	U16 crcEnviado = crc16.getValue();
	//Limpa o buffer de recebimento
	client->getInputStream()->clear();
	//Envia comando de alterar o endere�o do painel NSS
	client->send((U8*)"//WRPAINELXNS\0", 14);
	// envia o indice do painel
	client->send(&indicePainelParaOAPP,sizeof(indicePainelParaOAPP));
	//Envia o endere�o
	client->send((U8*)&nSerie, sizeof(nSerie));
	//Envia o CRC
	client->send((U8*)&crcEnviado, sizeof(crcEnviado));
	//L� resposta
	Protocolotrf::ResultadoComunicacao answer = Protocolotrf::TIMEOUT_COMM;
	client->receive((U8 *)&answer, 1);
	return answer;
}

Protocolotrf::ResultadoComunicacao Fachada::painelWrTrava(trf::service::net::ConnectionClient* client, U8* trava)
{
	//Limpa o buffer de recebimento
	client->getInputStream()->clear();
	//Envia comando de alterar modo de emerg�ncia
	client->send((U8*)"//WRTRAVA\0\0\0\0\0", 14);
	//Envia o par�metro de ativa��o
	client->send((U8*)trava, 4);
	//L� resposta
	Protocolotrf::ResultadoComunicacao answer = Protocolotrf::TIMEOUT_COMM;
	client->receive((U8 *)&answer, 1);
	return answer;
}

Protocolotrf::ResultadoComunicacao Fachada::painelLiberarTrava(trf::service::net::ConnectionClient* client, U8* chave)
{
	//Limpa o buffer de recebimento
	client->getInputStream()->clear();
	//Envia comando de alterar modo de emerg�ncia
	client->send((U8*)"//LIBERARTRAVA", 14);
	//Envia o par�metro de ativa��o
	client->send((U8*)chave, 4);
	//L� resposta
	Protocolotrf::ResultadoComunicacao answer = Protocolotrf::TIMEOUT_COMM;
	client->receive((U8 *)&answer, 1);
	return answer;
}

Protocolotrf::ResultadoComunicacao Fachada::painelApagarTrava(trf::service::net::ConnectionClient* client)
{
	//Limpa o buffer de recebimento
	client->getInputStream()->clear();
	//Envia comando de alterar modo de emerg�ncia
	client->send((U8*)"//APAGARTRAVA\0", 14);
	//L� resposta
	Protocolotrf::ResultadoComunicacao answer = Protocolotrf::TIMEOUT_COMM;
	client->receive((U8 *)&answer, 1);
	return answer;
}

Fachada::Resultado Fachada::penDriveSync(char* configFolderPath, void (*incrementProgress)(void))
{
	return penDriveSync(configFolderPath, incrementProgress, false);
}

Fachada::Resultado Fachada::penDriveSync(char* configFilePath, void (*incrementProgress)(void), bool force)
{
	//Verifica se a fun��o de carregar nova configura��o est� bloqueada
	// e, caso a sess�o esteja aberta (senha de desbloqueio), o contador � resetado (mais 60 segundos de fun��es liberadas)
	if(!force && (!this->isFuncaoLiberada(ParametrosFixos::NOVA_CONFIGURACAO)))
	{
		return FUNCAO_BLOQUEADA;
	}

	//Fecha todos os arquivos abertos
	this->closeAllFiles();

	//Abre o arquivo de imagem da NandFlash
	String::strcpy(this->pathBuffer, configFilePath);
	FatFs::File imagem(&this->plataforma->usbHost->fileSystem);
	if(imagem.open(this->pathBuffer, 'r') != 0){
		return NOVA_CONFIG_INVALIDA;
	}

//FIXME	//Obt�m as informa��es de senha anti-furto do arquivo
//	//TODO
//	// Verfica se h� uma senha anti-furto cadastrada
//	if(this->isSenhaAntiFurtoHabilitada()){
//		// Verfica se a senha do arquivo est� correta
//		if(this->e2prom.verificarSenhaAntiFurto(senhaAntiFurto) == false){
//			return SENHA_ANTI_FURTO_INCORRETA;
//		}
//	}

	//Altera a verifica��o da configura��o como pendente
	this->e2prom.setParametrosVariaveisStatus(STATUS_CONFIG_NAO_VERIFICADA);

	//Formatar o Sistema de Arquivos
	if(this->plataforma->sistemaArquivo->format() != 0){
		imagem.close();
		return NOVA_CONFIG_INVALIDA;
	}

	Resultado resultado = SUCESSO;
	//Verifica se � um arquivo .b12
	if(String::endWithIgnoreCase(this->pathBuffer, "b12")){
		resultado = copyB12File(&imagem, incrementProgress);
	}
	//Verifica se � um arquivo .nfx
	else if(String::endWithIgnoreCase(this->pathBuffer, "nfx")){
		resultado = copyNFXFile(&imagem, incrementProgress);
	}
	//Verifica se n�o � um arquivo aceit�vel
	else{
		resultado = NOVA_CONFIG_INVALIDA;
	}

	//Fecha o arquivo de imagem
	imagem.close();

	return resultado;
}

Fachada::Resultado Fachada::penDriveUpdateFW(char* configFolderPath, void (*incrementProgress)(void))
{
	Resultado resultado = SUCESSO;
	//Cria o arquivo
	//Forma o nome do arquivo
	String::strcpy(this->pathBuffer, "temp");
	String::strcat(this->pathBuffer, "/ctrl");
	U16 fileCount = 0;
	U32 objectIdCounter = 0, deltaCounter = 0;
	bool existeArq = true;
	while(existeArq == true)
	{
		Converter::itoa(fileCount, this->pathBuffer, 9, 4, Converter::BASE_16);
		String::strcat(this->pathBuffer, ".nfx");
		fileCount++;
		//Verifica se o arquivo j� existe
		if(this->dumpFirUpdate.open(this->pathBuffer, 'r') == 0)
		{
			this->dumpFirUpdate.close();
		}
		else
		{
			existeArq = false;
		}
	}
	//Abre o arquivo em modo escrita
	if(this->dumpFirUpdate.open(this->pathBuffer,'w') != 0)
	{
		return FALHA_OPERACAO;
	}

	FatFs::File auxFile(&this->plataforma->usbHost->fileSystem);
	//Abre o arquivo em modo escrita
	if(auxFile.open("temp/auxFile.nfx", 'w') != 0)
	{
		this->dumpFirUpdate.close();
		return FALHA_OPERACAO;
	}
	//Copia o dump para o arquivo
	for (U32 page = 0; page < this->plataforma->sistemaArquivo->getUsedPages(); page++)
	{
		//L� o chunk e tags
		if(this->plataforma->sistemaArquivo->readChunk(page, this->chunk, this->tags) != 0){
			this->dumpFirUpdate.close();
			auxFile.close();
			return FALHA_OPERACAO;
		}
		//Assinala a progress�o do processo de carregamento de nova configura��o
		incrementProgress();

		//Copia todos os diret�rios para o dump
		if(((NandFFS::ChunkTags*)this->tags)->objectType == NandFFS::DIR_OBJECT)
		{
			//Atualiza o contador de p�ginas total
			objectIdCounter++;
			//Escreve os tags do chunk
			if(this->dumpFirUpdate.write(16, this->tags) != 16){
				this->dumpFirUpdate.close();
				auxFile.close();
				return FALHA_OPERACAO;
			}
			//Limpa this->tags
			String::memset(&this->tags[16], 0x00, TAMANHO_SPARE_FLASH - 16);
			if(this->dumpFirUpdate.write(TAMANHO_SPARE_FLASH - 16, &this->tags[16]) != TAMANHO_SPARE_FLASH - 16){
				this->dumpFirUpdate.close();
				auxFile.close();
				return FALHA_OPERACAO;
			}
			//Escreve o conte�do do chunk
			if(this->dumpFirUpdate.write(((NandFFS::ChunkTags*)this->tags)->chunkLength, this->chunk) != ((NandFFS::ChunkTags*)this->tags)->chunkLength){
				this->dumpFirUpdate.close();
				auxFile.close();
				return FALHA_OPERACAO;
			}
		}
		//Copia todos os dados de arquivos para o auxFile
		else if(((NandFFS::ChunkTags*)this->tags)->objectType == NandFFS::DATA_OBJECT)
		{
			//Atualiza o objectId nas tags do chunk
			((NandFFS::ChunkTags*)this->tags)->objectId = objectIdCounter + deltaCounter;
			//Atualiza o contador de p�ginas do arquivo atual
			deltaCounter++;
			//Escreve os tags do chunk
			if(auxFile.write(16, this->tags) != 16){
				this->dumpFirUpdate.close();
				auxFile.close();
				return FALHA_OPERACAO;
			}
			//Limpa this->tags
			String::memset(&this->tags[16], 0x00, TAMANHO_SPARE_FLASH - 16);
			if(auxFile.write(TAMANHO_SPARE_FLASH - 16, &this->tags[16]) != TAMANHO_SPARE_FLASH - 16){
				this->dumpFirUpdate.close();
				auxFile.close();
				return FALHA_OPERACAO;
			}
			//Escreve o conte�do do chunk
			if(auxFile.write(((NandFFS::ChunkTags*)this->tags)->chunkLength, this->chunk) != ((NandFFS::ChunkTags*)this->tags)->chunkLength){
				this->dumpFirUpdate.close();
				auxFile.close();
				return FALHA_OPERACAO;
			}
		}
		//Copia todos os cabe�alhos de arquivos para o auxFile
		else if(((NandFFS::ChunkTags*)this->tags)->objectType == NandFFS::FILE_OBJECT)
		{
			//Atualiza o objectId nas tags do chunk
			((NandFFS::ChunkTags*)this->tags)->objectId = objectIdCounter + deltaCounter;
			//Escreve os tags do chunk
			if(auxFile.write(16, this->tags) != 16){
				this->dumpFirUpdate.close();
				auxFile.close();
				return FALHA_OPERACAO;
			}
			//Limpa this->tags
			String::memset(&this->tags[16], 0x00, TAMANHO_SPARE_FLASH - 16);
			if(auxFile.write(TAMANHO_SPARE_FLASH - 16, &this->tags[16]) != TAMANHO_SPARE_FLASH - 16){
				this->dumpFirUpdate.close();
				auxFile.close();
				return FALHA_OPERACAO;
			}
			//Escreve o conte�do do chunk
			if(auxFile.write(((NandFFS::ChunkTags*)this->tags)->chunkLength, this->chunk) != ((NandFFS::ChunkTags*)this->tags)->chunkLength){
				this->dumpFirUpdate.close();
				auxFile.close();
				return FALHA_OPERACAO;
			}
			//Assinala a progress�o do processo de carregamento de nova configura��o
			incrementProgress();
			//Se o arquivo n�o for .fir nem .opt, copia para o dump
			if(String::startWithIgnoreCase((char*)((NandFFS::FileChunk*)this->chunk)->extension, "fir") == false &&
					String::startWithIgnoreCase((char*)((NandFFS::FileChunk*)this->chunk)->extension, "opt") == false)
			{
				//Fecha o arquivo auxiliar
				auxFile.close();
				//Abre o arquivo auxiliar em modo leitura
				if(auxFile.open("temp/auxFile.nfx", 'r') != 0)
				{
					this->dumpFirUpdate.close();
					return FALHA_OPERACAO;
				}
				//Calcula o m�nimo entre o tamanho do arquivo e o tamanho do chunk
				U32 bytesToReadAux = auxFile.getSize();
				U32 sizeAuxRead = (bytesToReadAux > TAMANHO_PAGINA_FLASH) ? TAMANHO_PAGINA_FLASH : bytesToReadAux;
				//L� uma parte do conte�do e atualiza contador de bytes que ainda faltam ser lidos
				bytesToReadAux -= auxFile.read(sizeAuxRead, this->chunk);
				while(sizeAuxRead > 0)
				{
					//Escreve o conte�do
					if(this->dumpFirUpdate.write(sizeAuxRead, this->chunk) != sizeAuxRead){
						this->dumpFirUpdate.close();
						auxFile.close();
						return FALHA_OPERACAO;
					}
					sizeAuxRead = (bytesToReadAux > TAMANHO_PAGINA_FLASH) ? TAMANHO_PAGINA_FLASH : bytesToReadAux;
					//L� uma parte do conte�do e atualiza contador de bytes que ainda faltam ser lidos
					bytesToReadAux -= auxFile.read(sizeAuxRead, this->chunk);
					//Assinala a progress�o do processo de carregamento de nova configura��o
					incrementProgress();
				}
				//Fecha o arquivo auxiliar
				auxFile.close();
				//Abre o arquivo auxiliar em modo escrita
				if(auxFile.open("temp/auxFile.nfx", 'w') != 0)
				{
					this->dumpFirUpdate.close();
					return FALHA_OPERACAO;
				}
				//Atualiza o contador de p�ginas total
				objectIdCounter += (deltaCounter + 1);
			}
			else
			{
				//Fecha o arquivo auxiliar
				auxFile.close();
				//Abre o arquivo auxiliar em modo escrita
				if(auxFile.open("temp/auxFile.nfx", 'w') != 0)
				{
					this->dumpFirUpdate.close();
					return FALHA_OPERACAO;
				}
			}
			//Zera o contador de p�ginas de arquivo atual
			deltaCounter = 0;
		}
		//Assinala a progress�o do processo de carregamento de nova configura��o
		incrementProgress();
	}
	//Fecha o arquivo de dump
	this->dumpFirUpdate.close();
	auxFile.close();

	//Altera a verifica��o da configura��o como pendente
	this->e2prom.setParametrosVariaveisStatus(STATUS_CONFIG_NAO_VERIFICADA);
	//Formatar o Sistema de Arquivos
	if(this->plataforma->sistemaArquivo->format() != 0){
		return FALHA_OPERACAO;
	}
	//Abre o arquivo dump em modo leitura
	if(this->dumpFirUpdate.open(this->pathBuffer, 'r') != 0){
		return FALHA_OPERACAO;
	}
	//Transfere o conte�do do dump
	resultado = this->copyNFXFile(&this->dumpFirUpdate, incrementProgress);
	//Fecha o arquivo de dump
	this->dumpFirUpdate.close();

	//Transfere os novos arquivos .fir e .opt
	if(this->firmwareFolder.open("ldx2/firmware") == true && this->firmwareFolder.getFileList("fir|opt")->getLength() != 0)
	{
		//L� lista de arquivos da pasta "ldx2/firmware"
		FatFs::FileList* listaArquivos = this->firmwareFolder.getFileList("fir|opt");
		//Seta os caminhos dos arquivos
		String::strcpy(this->pathBuffer, "ldx2/firmware/");
		U8 index = String::getLength(this->pathBuffer);
		String::strcpy(this->pathBuffer2, "firmware/");
		U8 index2 = String::getLength(this->pathBuffer2);
		NandFFS::File nandFile(this->plataforma->sistemaArquivo);
		U8 att;
		while(listaArquivos->getNext(&this->pathBuffer2[index2], &att))
		{
			String::strcpy(&this->pathBuffer[index], &this->pathBuffer2[index2]);
			//Abre os arquivos
			auxFile.open(this->pathBuffer, 'r');
			nandFile.open(this->pathBuffer2, 'w');
			//L� arquivo USB
			U16 sizeAuxRead = auxFile.read(TAMANHO_PAGINA_FLASH, this->chunk);
			while(sizeAuxRead > 0)
			{
				//Escreve na NandFlash
				nandFile.write(sizeAuxRead, this->chunk);
				//L� arquivo USB
				sizeAuxRead = auxFile.read(TAMANHO_PAGINA_FLASH, this->chunk);
				//Assinala a progress�o do processo de carregamento de nova configura��o
				incrementProgress();
			}
			//Fecha os arquivos
			auxFile.close();
			nandFile.close();
		}
	}

	return resultado;
}

Fachada::Resultado Fachada::copyB12File(escort::service::FatFs::File* imagem, void (*incrementProgress)(void))
{
	//Escreve conte�do do arquivo na mem�ria
	U32 tamanho = imagem->getSize();
	//Verifica tamanho do arquivo
	if(tamanho % (TAMANHO_PAGINA_FLASH + TAMANHO_SPARE_FLASH)){
		imagem->close();
		return NOVA_CONFIG_INVALIDA;
	}

	//Pula diret�rio raiz
	U32 ptr = TAMANHO_PAGINA_FLASH + TAMANHO_SPARE_FLASH;
	imagem->seek(ptr);

	while(ptr < tamanho)
	{
		//L� o conte�do do chunk
		if(imagem->read(TAMANHO_PAGINA_FLASH, this->chunk) != TAMANHO_PAGINA_FLASH){
			return FALHA_OPERACAO;
		}
		ptr += TAMANHO_PAGINA_FLASH;
		//L� os tags do chunk
		if(imagem->read(TAMANHO_SPARE_FLASH, this->tags) != TAMANHO_SPARE_FLASH){
			return FALHA_OPERACAO;
		}
		ptr += TAMANHO_SPARE_FLASH;
		//Envia o chunk
		if(this->plataforma->sistemaArquivo->writeChunk(chunk, tags) != 0){
			return FALHA_OPERACAO;
		}
		//Assinala a progress�o do processo de carregamento de nova configura��o
		incrementProgress();
	}

	return SUCESSO;
}

Fachada::Resultado Fachada::copyNFXFile(escort::service::FatFs::File* imagem, void (*incrementProgress)(void))
{
	U32 tamanho = imagem->getSize();

	//Escreve conte�do do arquivo na mem�ria
	U32 ptr = 0;
	do
	{
		//L� os tags do chunk
		if(imagem->read(sizeof(this->tags), this->tags) != sizeof(this->tags)){
			imagem->close();
			return NOVA_CONFIG_INVALIDA;
		}
		//Obtem o tamanho do chunk
		U16 chunkLength = ((NandFFS::ChunkTags*)this->tags)->chunkLength;
		//Verifica o tamanho do chunk
		if(chunkLength > TAMANHO_PAGINA_FLASH){
			return NOVA_CONFIG_INVALIDA;
		}
		//L� o conte�do do chunk
		if(imagem->read(chunkLength, this->chunk) != chunkLength){
			return NOVA_CONFIG_INVALIDA;
		}
		//Pula diret�rio raiz
		if(ptr != 0){
			//Envia o chunk
			if(this->plataforma->sistemaArquivo->writeChunk(chunk, tags) != 0){
				return FALHA_OPERACAO;
			}
		}
		//Incrementa o ponteiro
		ptr += sizeof(this->tags) + chunkLength;
		//Assinala a progress�o do processo de carregamento de nova configura��o
		incrementProgress();
	}while(ptr < tamanho);

	return SUCESSO;
}

Fachada::Resultado Fachada::copyFile(NandFFS* srcFS, char* srcFilePath, FatFs* destFAT, char* destFilePath, void (*incrementProgress)(void), bool syncMode)
{
	NandFFS::File srcCopyFile(srcFS);
	FatFs::File destCopyFile(destFAT);

	//Abre o arquivo fonte
	U8 i = 0;
	for (i = 0; i < 5; ++i) {
		if(srcCopyFile.open(srcFilePath, 'r') == 0){
			break;
		}
	}
	if(i >= 5)
	{
		return FALHA_OPERACAO;
	}


	//Verifica se n�o � um arquivo muito grande ou vazio
	if(srcCopyFile.getSize() > MAXIMO_TAMANHO_ARQUIVO || srcCopyFile.getSize() == 0){
		srcCopyFile.close();
		return NOVA_CONFIG_INVALIDA;
	}

	if(syncMode){
		if (destCopyFile.open(destFilePath, 'r') == 0)
		{
			//Verifica se os tamanhos dos arquivos s�o iguais
			if(destCopyFile.getSize() == srcCopyFile.getSize()){
				U16 remoteCRC, localCRC;
				//L� o CRC do arquivo local
				if(srcCopyFile.read(62, sizeof(localCRC), (U8 *)&localCRC) != sizeof(localCRC)){
					return FALHA_OPERACAO;
				}
				//L� o CRC do arquivo remoto
				if(destCopyFile.read(62, sizeof(remoteCRC), (U8 *)&remoteCRC) != sizeof(remoteCRC)){
					return FALHA_OPERACAO;
				}
				//Fecha o arquivo destino aberto como leitura
				destCopyFile.close();
				//Compara os CRCs
				if(localCRC == remoteCRC){
					//Fecha o arquivo fonte
					srcCopyFile.close();
					//Arquivo j� sincronizado
					return SUCESSO;
				}
			}
			else{
				//Caso contr�rio fecha o arquivo aberto como leitura e continua o processo
				destCopyFile.close();
			}
		}
	}

	//Cria o arquivo destino
	if(destCopyFile.open(destFilePath, 'w') != 0){
		srcCopyFile.close();
		return FALHA_OPERACAO;
	}

	//Volta para o in�cio do arquivo
	if(srcCopyFile.seek(0) == false){
		srcCopyFile.close();
		return FALHA_OPERACAO;
	}
	//Copia o conte�do do arquivo
	U16 qntLido = 0, tamanhoDest = 0;
	do {
		qntLido = srcCopyFile.read(sizeof(this->buffer), this->buffer);
		if(destCopyFile.write(qntLido, this->buffer) != qntLido){
			srcCopyFile.close();
			destCopyFile.close();
			return FALHA_OPERACAO;
		}
		tamanhoDest += qntLido;
		incrementProgress();
	} while (qntLido != 0);

	//Verifica se copiou todos os bytes
	if(tamanhoDest != srcCopyFile.getSize()){
		srcCopyFile.close();
		destCopyFile.close();
		return FALHA_OPERACAO;
	}

	//Fecha os arquivos abertos
	srcCopyFile.close();
	destCopyFile.close();

	return SUCESSO;
}

Fachada::Resultado Fachada::copyFolder(NandFFS* srcFS, char* srcFolderPath, FatFs* destFAT, char* destFolderPath, void (*incrementProgress)(void), bool syncMode)
{
	static int profundidade = 0;
	profundidade++;

	if(profundidade > MAXIMA_PROFUNDIDADE_ESTRUTURA_ARQUIVOS){
		profundidade--;
		return NOVA_CONFIG_INVALIDA;
	}

	//TODO
//	NandFFS::Directory srcDir(srcFS);
//	if(srcDir.open(srcFolderPath))
//	{
//		char* srcItemPath = (char*)this->pathBuffer;
//		String::strcpy(srcItemPath, srcFolderPath);
//		U16 srcItemPathLength = String::getLength(srcItemPath);
//		String::strcpy(srcItemPath + srcItemPathLength, "/");
//		srcItemPathLength++;
//
//		char* destItemPath = (char*)this->pathBuffer2;
//		String::strcpy(destItemPath, destFolderPath);
//		U16 destItemPathLength = String::getLength(destItemPath);
//		String::strcpy(destItemPath + destItemPathLength, "/");
//		destItemPathLength++;
//
//		FatFs::FileList* listaArquivos = srcDir.getFileList("*");
//		U8 fileAttrib;
//
//		while(listaArquivos->getNext(srcItemPath + srcItemPathLength, &fileAttrib)){
//			String::strcpy(destItemPath + destItemPathLength, srcItemPath + srcItemPathLength);
//			Resultado resultado = SUCESSO;
//			if(fileAttrib & FatFs::ATTR_DIRECTORY){
//				resultado = this->copyFolder(srcFS, srcItemPath, destFAT, destItemPath, incrementProgress, syncMode);
//			}
//			else if(fileAttrib & FatFs::ATTR_ARCHIVE){
//				resultado = this->copyFile(srcFS, srcItemPath, destFAT, destItemPath, incrementProgress, syncMode);
//			}
//
//			if(resultado != SUCESSO){
//				profundidade--;
//				return resultado;
//			}
//		}
//		incrementProgress();
//	}
//
//	profundidade--;

	return SUCESSO;
}

void Fachada::closeAllFiles()
{
	this->parametrosFixos.close();
	this->listaRoteiros.close();
	this->listaMensagens.close();
	this->regiao.close();
}

bool Fachada::isAddressAssigned()
{
	return this->idProtocol->isAddressAssigned();
}

IdentificationProtocol::WhoIsReply* Fachada::whoIs(trf::application::trfProduct::SerialNumber numSerie, U32 timeout)
{
	return this->idProtocol->whoIs(numSerie, timeout);
}

bool Fachada::verificarSenhaAcesso(U32 senha)
{
	SHA256::starts(&this->sha256Strct);
	SHA256::update(&this->sha256Strct, (U8*)&senha, sizeof(senha));
	SHA256::update(&this->sha256Strct, (U8*)"321894231456156732135765123186423168465105615641", 48);
	SHA256::finish(&this->sha256Strct, (U8*)this->pathBuffer);
	this->parametrosFixos.readSenhaAcesso((U8*)this->pathBuffer2);

	bool result = String::compare(this->pathBuffer, 32, this->pathBuffer2, 32) == 0;

	if(result)
	{
		// Ap�s este tempo (60 segundos) as fun��es ser�o bloqueadas novamente
		this->tempoParaBloquear = xTaskGetTickCount() + 60000;
	}

	return result;
}

bool Fachada::isSenhaAcessoHabilitada()
{
	this->parametrosFixos.readSenhaAcesso((U8*)this->pathBuffer2);
	for (int i = 0; i < 32; ++i) {
		if(this->pathBuffer2[i] != 0){
			return true;
		}
	}
	return false;
}

bool Fachada::isSenhaAntiFurtoHabilitada()
{
	return this->e2prom.isSenhaAntiFurtoHabilitada();
}

void Fachada::mudarSenhaAntiFurto(U8* novaSenha)
{
	this->e2prom.mudarSenhaAntiFurto(novaSenha);
}

void Fachada::zerarSenhaAntiFurto()
{
	for (U8 i = 0; i < 32; ++i) {
		this->buffer[i] = 0xFF;
	}
	this->mudarSenhaAntiFurto(this->buffer);
}

bool Fachada::isPaineisTravados()
{
	U8 trava[4];
	this->e2prom.readTravaPaineis(trava);
	return (*((U32*)trava) != 0xFFFFFFFF && *((U32*)trava) != 0x00000000 && *((U32*)trava) != 0);
}

bool Fachada::deveTravarPaineis()
{
	return this->parametrosFixos.readAtivaTravaPaineis();
}

U32 Fachada::getIdDestravaPaineis(InfoPainelListado* paineis, U8 qntPaineis)
{
	//Obtem a data atual
	Relogio::DataHora data = this->plataforma->relogio->getDataHora();
	//Obtem o n�mero de s�rie
	U64 myNumSerie = this->getSerialNumber().content;
	//Calcula o ID de destrava dos pain�is
	U8* idDestrava = this->buffer;
	SHA256::starts(&this->sha256Strct);
	SHA256::update(&this->sha256Strct, (U8*)&myNumSerie, sizeof(myNumSerie));
	if(qntPaineis > 0){
		U8 count = 0;
		//Calcula o ID na ordem crescente do endere�o de rede dos pain�is
		for (U8 i = 0; i < 0xF && (count < qntPaineis); ++i) {
			for (U8 j = 0; j < 0xF && (count < qntPaineis); ++j) {
				for (U8 p = 0; p < qntPaineis && (count < qntPaineis); ++p) {
					if(((i << 4) | j) == paineis[p].netAddr){
						SHA256::update(&this->sha256Strct, (U8*)paineis[p].numeroSerie, sizeof(paineis[p].numeroSerie));
						count++;
					}
				}
			}
		}
	}
	SHA256::update(&this->sha256Strct, (U8*)&data.ano, sizeof(data.ano));
	SHA256::update(&this->sha256Strct, (U8*)&data.mes, sizeof(data.mes));
	SHA256::update(&this->sha256Strct, (U8*)&data.dia, sizeof(data.dia));
	SHA256::finish(&this->sha256Strct, idDestrava);

	return *(U32*)idDestrava;
}

Fachada::Resultado Fachada::emparelharPaineis(InfoPainelListado** lista, bool automatico)
{
	//Verifica se os pain�is est�o travados
	if(this->isPaineisTravados()){
		return PAINEIS_TRAVADOS;
	}

	U32 ptr = (U32)&this->buffer[0];
	if(ptr % sizeof(U32) != 0){
		ptr += (sizeof(U32) - (ptr % sizeof(U32)));
	}
	InfoPainelListado* infos = (InfoPainelListado*)ptr;
	U8 qntPaineis = this->getQntPaineis();

	//Inicializa as informa��es sobre os paineis
	initInfoPaineisListados(infos);
	//Lista todos os paineis na rede
	if(listaPaineisRede(infos) != qntPaineis){
		return EMPARELHAMENTO_INCOMPATIVEL;
	}
	//Apaga as informa��es de emparelhamento atuais
	this->e2prom.apagarEmparelhamentoInfo();
	//Escreve a quantidade de pain�is na E2PROM
	this->e2prom.writeQntPaineis(qntPaineis);
	//Verifica se deve efetuar atribui��es autom�ticas dos �ndices dos paineis
	//baseado na heur�stica das dimens�es dos paineis listados
	if(automatico){
		//Consulta as dimens�es dos paineis
		for(U8 painelListado = 0; painelListado < qntPaineis; painelListado++){
			//Conecta com o painel listado
			ConnectionClient* client = this->network->connect(infos[painelListado].netAddr, 0);
			if(client == 0){
				return ERRO_COMUNICACAO;
			}
			client->setReceiveTimeout(2000);
			//L� as dimens�es do painel (3 tentativas)
			Protocolotrf::ResultadoComunicacao res;
			for (U8 retry = 0; retry < 3; ++retry) {
				if((res = this->painelRdDimensoes(client, &infos[painelListado].altura, &infos[painelListado].largura)) == Protocolotrf::SUCESSO_COMM){
					break;
				}
			}
			//Verifica se leu as dimens�es com sucesso
			if(res != Protocolotrf::SUCESSO_COMM){
				client->close();
				return ERRO_COMUNICACAO;
			}
			//Fecha a conex�o
			client->close();
		}
		//Verifica se as dimens�es listadas s�o compat�veis com a configura��o do Controlador
		for(U8 painelListado = 0; painelListado < qntPaineis; painelListado++){
			for(U8 p = 0; p < qntPaineis; p++){
				//Checa se as dimens�es do painel listado batem com o painel 'p'
				if(infos[painelListado].altura == this->getAlturaPainel(p)
					&& infos[painelListado].largura == this->getLarguraPainel(p))
				{
					//Atribui o �ndice ao painel cujas dimens�es baterem
					infos[painelListado].indice = p;
					break;
				}
			}
		}
		//Verifica se existe mais de um painel com a mesma dimens�o
		for(U8 painelListado = 0; painelListado < qntPaineis; painelListado++){
			for(U8 pL = 0; pL < qntPaineis; pL++){
				if(infos[painelListado].indice == infos[pL].indice
					&& infos[painelListado].netAddr != infos[pL].netAddr)
				{
					infos[painelListado].flags.ambiguo = true;
					break;
				}
			}
		}
	}
	//Retorna as informa��es dos paineis listados
	*lista = infos;

	return SUCESSO;
}

U8 Fachada::getQntPaineisRede()
{
	U32 ptr = (U32)&this->buffer[0];
	if(ptr % sizeof(U32) != 0){
		ptr += (sizeof(U32) - (ptr % sizeof(U32)));
	}
	InfoPainelListado* infos = (InfoPainelListado*)ptr;

	//Lista todos os paineis na rede
	return listaPaineisRede(infos);
}

Fachada::Resultado Fachada::detectarAPP(bool (*isCanceled)(void))
{
	U32 ptr = (U32)&this->buffer[(sizeof(this->buffer)/2)];
	if(ptr % sizeof(int)){
		ptr += sizeof(int) - (ptr % sizeof(int));
	}
	IdentificationProtocol::FindServiceReply* reply =  this->idProtocol->findService("NSS", (IdentificationProtocol::FindServiceReply::Device*)ptr, 1000, 1);
	while(reply->getStatus() == reply->WAITING){
		if(isCanceled != 0 && isCanceled()){
			return OPERACAO_CANCELADA;
		}
		vTaskDelay(10);
	}
	if(reply->getStatus() == reply->FINISHED && reply->getQntResponses() > 0){
		this->appNetAddress = reply->getAddress(0);
		return SUCESSO;
	}
	else{
		return FALHA_OPERACAO;
	}
}

Fachada::Resultado Fachada::detectarAdaptadorCatraca(bool (*isCanceled)(void))
{
	U32 ptr = (U32)&this->buffer[(sizeof(this->buffer)/2)];
	if(ptr % sizeof(int)){
		ptr += sizeof(int) - (ptr % sizeof(int));
	}
	IdentificationProtocol::FindServiceReply* reply =  this->idProtocol->findService("AdaptadorCANCatraca", (IdentificationProtocol::FindServiceReply::Device*)ptr, 1000, 1);
	while(reply->getStatus() == reply->WAITING){
		if(isCanceled != 0 && isCanceled()){
			return OPERACAO_CANCELADA;
		}
		vTaskDelay(10);
	}
	if(reply->getStatus() == reply->FINISHED && reply->getQntResponses() > 0){
		this->adaptadorCatracaNetAddress = reply->getAddress(0);
		return SUCESSO;
	}
	else{
		return FALHA_OPERACAO;
	}
}

Fachada::Resultado Fachada::detectarAdaptadorSensores(bool (*isCanceled)(void))
{
	U32 ptr = (U32)&this->buffer[(sizeof(this->buffer)/2)];
	if(ptr % sizeof(int)){
		ptr += sizeof(int) - (ptr % sizeof(int));
	}
	IdentificationProtocol::FindServiceReply* reply =  this->idProtocol->findService("SensorVelocidade", (IdentificationProtocol::FindServiceReply::Device*)ptr, 1000, 1);
	while(reply->getStatus() == reply->WAITING){
		if(isCanceled != 0 && isCanceled()){
			return OPERACAO_CANCELADA;
		}
		vTaskDelay(10);
	}
	if(reply->getStatus() == reply->FINISHED && reply->getQntResponses() > 0){
		this->adaptadorSensoresNetAddress = reply->getAddress(0);
		return SUCESSO;
	}
	else{
		return FALHA_OPERACAO;
	}
}

bool Fachada::sincronizarAdaptadorCatraca(void (*incrementProgress)(void), bool (*isCanceled)(void))
{
	bool result = false;

	//Verifica se o Adaptador foi detectado na rede
	if(isAdaptadorCatracaDetectado() == false){
		return false;
	}

	//Conecta com o Adaptador
	ConnectionClient* client = this->network->connect(this->adaptadorCatracaNetAddress, 0);
	if(client){
		//Configura o timeout de recebimento
		client->setReceiveTimeout(2000);
		//Sincroniza os par�metros do Adaptador de Catraca
		result = this->syncParametrosAdaptadorCatraca(client, incrementProgress);
		//Fecha a conex�o
		client->close();
	}

	return result;
}

bool Fachada::sincronizarAdaptadorSensores(void (*incrementProgress)(void), bool (*isCanceled)(void))
{
	bool result = false;

	//Verifica se o Adaptador foi detectado na rede
	if(isAdaptadorSensoresDetectado() == false){
		return false;
	}

	//Conecta com o Adaptador
	ConnectionClient* client = this->network->connect(this->adaptadorSensoresNetAddress, 0);
	if(client){
		//Configura o timeout de recebimento
		client->setReceiveTimeout(2000);
		//Sincroniza os par�metros do Adaptador da Placa de Sensores
		result = this->syncParametrosAdaptadorSensores(client, incrementProgress);
		//Fecha a conex�o
		client->close();
	}

	return result;
}

bool Fachada::addItemListaPaineisSensores(U8 addrPainel)
{
	bool resultado = false;

	for (U8 i = 0; i < sizeof(this->listaPaineisSensores); ++i) {
		if(this->listaPaineisSensores[i] == 0)
		{
			//Adiciona endere�o na lista
			this->listaPaineisSensores[i] = addrPainel;
			//Conseguiu adicionar em um espa�o vazio
			resultado = true;
			break;
		}
	}

	return resultado;
}

void Fachada::removeItemListaPaineisSensores(U8 addrPainel)
{
	for (U8 i = 0; i < sizeof(this->listaPaineisSensores); ++i) {
		//Verifica se h� algum item na lista com esse endere�o
		if(this->listaPaineisSensores[i] == addrPainel)
		{
			this->listaPaineisSensores[i] = 0;
		}
	}
}

bool Fachada::syncParametrosAdaptadorCatraca(trf::service::net::ConnectionClient* client, void (*incrementProgress)(void))
{
	//Envia a rota selecionada (n�mero da rota)
	this->getLabelNumeroRoteiro(this->getRoteiroSelecionado(), (char*)this->buffer);
	//Utiliza o mesmo m�todo usado pelo APP mudando apenas o endere�o do cliente
	if(this->appWrRota(client, (char*)this->buffer) != Protocolotrf::SUCESSO_COMM){
		return false;
	}
	//Envia o sentido da rota
	U8 sentido = (this->getSentidoRoteiro() == IDA) ? 'I' : 'V';
	//Utiliza o mesmo m�todo usado pelo APP mudando apenas o endere�o do cliente
	if(this->appWrSentido(client, sentido) != Protocolotrf::SUCESSO_COMM){
		return false;
	}

	return true;
}

bool Fachada::syncParametrosAdaptadorSensores(trf::service::net::ConnectionClient* client, void (*incrementProgress)(void))
{
	//Calcula o CRC
	CRC16CCITT crc16;
	crc16.reset();
	crc16.update(this->listaPaineisSensores, sizeof(this->listaPaineisSensores));
	U16 crcEnviado = crc16.getValue();
	//Limpa o buffer de recebimento
	client->getInputStream()->clear();
	//Envia comando de escrever lista de pain�is que exibem informa��es dos sensores
	client->send((U8*)"//WRLISTAPAINS", 14);
	//Envia a lista de pain�is
	client->send(this->listaPaineisSensores, sizeof(this->listaPaineisSensores));
	//Envia o CRC
	client->send((U8*)&crcEnviado, sizeof(crcEnviado));
	//L� resposta
	Protocolotrf::ResultadoComunicacao answer = Protocolotrf::TIMEOUT_COMM;
	client->receive((U8 *)&answer, 1);
	//Verifica a resposta
	if(answer != Protocolotrf::SUCESSO_COMM){
		return false;
	}

	return true;
}

void Fachada::initInfoPaineisListados(InfoPainelListado* infos)
{
	U8 qntPaineis = this->getQntPaineis();
	String::memset((U8*)&infos[0], 0, sizeof(infos[0]) * qntPaineis);

	for (U8 p = 0; p < qntPaineis; ++p) {
		infos[p].indice = (U8)-1;
		infos[p].flags.ambiguo = false;
	}
}

U8 Fachada::listaPaineisRede(InfoPainelListado* infos)
{
	U8 qntPaineisListados = 0;
	U32 ptr = (U32)&this->buffer[(sizeof(this->buffer)/2)];
	if(ptr % sizeof(int)){
		ptr += sizeof(int) - (ptr % sizeof(int));
	}
	IdentificationProtocol::FindServiceReply* reply =  this->idProtocol->findService("Painelpontos", (IdentificationProtocol::FindServiceReply::Device*)ptr, 1000);
	while(reply->getStatus() == reply->WAITING){
		vTaskDelay(10);
	}
	if(reply->getStatus() == reply->FINISHED){
		qntPaineisListados = reply->getQntResponses();
		for (U8 i = 0; i < qntPaineisListados; ++i) {
			infos[i].netAddr = reply->getAddress(i);
			trfProduct::SerialNumber serialN = reply->getNumSerie(i);
			String::memcpy(infos[i].numeroSerie, (U8*)&serialN, sizeof(serialN));
		}
	}

	return qntPaineisListados;
}

Fachada::Resultado Fachada::restaurarConfigFabrica()
{
	this->e2prom.format();
	this->setStatusFuncionamento(ERRO_CONFIGURACAO_INCONSISTENTE);
	return this->plataforma->sistemaArquivo->format() == 0 ? SUCESSO : FALHA_OPERACAO;
}

Fachada::Resultado Fachada::setModoTeste(bool ativa, U8 tipo, bool force)
{
	Resultado result = ERRO_COMUNICACAO;

	//Verifica se a fun��o de modo teste est� bloqueada
	// e, caso a sess�o esteja aberta (senha de desbloqueio), o contador � resetado (mais 60 segundos de fun��es liberadas)
	if(!force && (!this->isFuncaoLiberada(ParametrosFixos::MODO_TESTE)))
	{
		return FUNCAO_BLOQUEADA;
	}

	//Conecta com o painel
	ConnectionClient* client = this->network->connect(this->getPainelNetAddress(this->painelSelecionado), 0);
	if(client)
	{
		client->setReceiveTimeout(2000);

		//Se os paineis est�o travados ent�o envia comando para liberar a trava (autentica��o)
		if(this->isPaineisTravados()){
			U8 chave[4];
			this->e2prom.readTravaPaineis(chave);
			Protocolotrf::ResultadoComunicacao res = this->painelLiberarTrava(client, chave);
			if(res == Protocolotrf::TIMEOUT_COMM)
			{
				client->close();
				return ERRO_COMUNICACAO;
			}
			else if(res == Protocolotrf::PARAMETRO_INVALIDO_COMM){
				client->close();
				return PAINEIS_TRAVADOS;
			}
		}

		if(this->painelWrModoTeste(client, ativa, tipo) == Protocolotrf::SUCESSO_COMM){
			result = SUCESSO;
		}

		//Fecha a conex�o
		client->close();
	}
	return result;
}

bool Fachada::painelWrTexto(U8 painel, char* texto)
{
	bool result = false;

	//Conecta com o painel
	ConnectionClient* client = this->network->connect(this->getPainelNetAddress(painel), 0);
	if(client){
		//Configura o timeout de recebimento
		client->setReceiveTimeout(2000);

		//Se os paineis est�o travados ent�o envia comando para liberar a trava (autentica��o)
		if(this->isPaineisTravados()){
			U8 chave[4];
			this->e2prom.readTravaPaineis(chave);
			Protocolotrf::ResultadoComunicacao res = this->painelLiberarTrava(client, chave);
			if(res != Protocolotrf::SUCESSO_COMM)
			{
				client->close();
				return false;
			}
		}

		ParametrosShowText parametros;
		parametros.string = texto;
		parametros.repeticoes = 1;
		parametros.otimizacao = false;
		parametros.alinhamento = Renderizacao::CENTRALIZADO;
		parametros.animacao = Renderizacao::FIXA;
		parametros.delayAnimacao = 0;
		parametros.delayApresentacao = 1000;
		result = this->painelWrTexto(client, &parametros) == Protocolotrf::SUCESSO_COMM;

		//Fecha a conex�o
		client->close();
	}

	return result;
}

Fachada::Resultado Fachada::setModoTeste(bool ativa, U8 tipo)
{
	return setModoTeste(ativa, tipo, false);
}

trf::application::Version* Fachada::getFileFirmwareVersion(escort::service::NandFFS::File* file)
{
	trf::application::Version	*version = (trf::application::Version*)this->buffer;

	// Vai para o in�cio do arquivo
	file->seek(0x400);

	// L� a vers�o do firmware contido no arquivo
	if(file->read(sizeof(trf::application::Version), this->buffer) != sizeof(trf::application::Version))
	{
		return 0;
	}

	return version;
}

U8 Fachada::getModoAtualizacao(escort::service::NandFFS::File* optionsFile)
{
	U8 modo = 0x00;

	// Vai para a posi��o do modo de atualiza��o
	optionsFile->seek(1);

	// L� o modo de atualiza��o do firmware configurado no arquivo de op��es
	optionsFile->read(sizeof(U8), (U8*)&modo);

	return modo;
}

bool Fachada::verificarIntegridadeArquivoFirmware(escort::service::NandFFS::File* firmwareFile)
{
	U16 crcArquivo;
	U16 dummyBytes;
	U32 tamanhoArquivo;
	CRC16CCITT crc16;

	//Formato do arquivo
	// +----------------------+---------+--------------+--------------------+------------------+--------------------+
	// | Startup (1024 bytes) | version | CRC (2bytes) | reservado (2bytes) | tamanho (4bytes) |    Application     |
	// +----------------------+---------+--------------+--------------------+------------------+--------------------+

	// Vai para a posi��o do campo de CRC no arquivo de firmware
	firmwareFile->seek(APPLICATION_VERSION_OFFSET + sizeof(trf::application::Version));

	// L� o CRC do arquivo
	if(firmwareFile->read(sizeof(crcArquivo), (U8*)&crcArquivo) != sizeof(crcArquivo))
	{
		return false;
	}

	// L� bytes dummy do arquivo
	if(firmwareFile->read(sizeof(dummyBytes), (U8*)&dummyBytes) != sizeof(dummyBytes))
	{
		return false;
	}

	// L� o tamnho do conte�do do arquivo
	if(firmwareFile->read(sizeof(tamanhoArquivo), (U8*)&tamanhoArquivo) != sizeof(tamanhoArquivo))
	{
		return false;
	}

	//Verifica se o tamanho do conte�co definido no arquivo corresponde ao tamanho real do conte�do
	if(firmwareFile->getSize() != tamanhoArquivo)
	{
		return false;
	}

	//Reseta o CRC
	crc16.reset();

	U32 addr = 0;
	U32 bytesRead = 0;
	//Vai pra o come�o do arquivo
	firmwareFile->seek(0);

	// Percorre o arquivo inteiro
	while((bytesRead = firmwareFile->read(sizeof(buffer), buffer)) != 0)
	{
		//Calcula o CRC (pulando o campo CRC no arquivo)
		for(U32 i = 0; i < bytesRead; ++i)
		{
			if(addr < (APPLICATION_VERSION_OFFSET + sizeof(trf::application::Version))
				|| addr >= (APPLICATION_VERSION_OFFSET + sizeof(trf::application::Version) + sizeof(U16)))
			{
				crc16.update(buffer[i]);
			}

			addr++;
		}
	}

	//Verifica se o CRC bateu
	if(crcArquivo != crc16.getValue())
	{
		return false;
	}

	return true;
}

bool Fachada::verificarIntegridadeArquivoOpcoes(escort::service::NandFFS::File* optionsFile)
{
	U32 fileSize = optionsFile->getSize();
	CRC16CCITT crc16;

	// Vai para o come�o do arquivo de op��es
	optionsFile->seek(0);

	// L� o modo de atualiza��o do firmware configurado no arquivo de op��es
	if(optionsFile->read(fileSize, buffer) != fileSize)
	{
		return false;
	}

	//Calcula o CRC do conte�do do arquivo
	crc16.reset();
	crc16.update(buffer, fileSize - sizeof(U16));

	//Comapara o CRC calculado com o CRC do arquivo
	U16 crcArquivo = *((U16*)&buffer[fileSize - sizeof(U16)]);
	if(crcArquivo != crc16.getValue())
	{
		return false;
	}

	return true;
}

bool Fachada::verificarAtualizacaoFirmwarePainel(trf::application::Version* currentPainelVersion)
{
	NandFFS::File firmwareFile(this->plataforma->sistemaArquivo);
	NandFFS::File optionsFile(this->plataforma->sistemaArquivo);

	bool firmwareFilesOK = false;

	if(String::startWith(currentPainelVersion->produto, "Painelpontos"))
	{
		if(firmwareFile.open("firmware/painel.fir") == 0 && optionsFile.open("firmware/painel.opt") == 0)
		{
			firmwareFilesOK = true;
		}
	}
	else if(String::startWith(currentPainelVersion->produto, "PainelMultilinhas"))
	{
		if(firmwareFile.open("firmware/multilin.fir") == 0 && optionsFile.open("firmware/multilin.opt") == 0)
		{
			firmwareFilesOK = true;
		}
	}
	else if(String::startWith(currentPainelVersion->produto, "PainelMultiplex2x8"))
	{
		if(firmwareFile.open("firmware/mux_2x8.fir") == 0 && optionsFile.open("firmware/mux_2x8.opt") == 0)
		{
			firmwareFilesOK = true;
		}
	}
	else if(String::startWith(currentPainelVersion->produto, "PainelMultiplex2x13"))
	{
		if(firmwareFile.open("firmware/mux_2x13.fir") == 0 && optionsFile.open("firmware/mux_2x13.opt") == 0)
		{
			firmwareFilesOK = true;
		}
	}
	else if(String::startWith(currentPainelVersion->produto, "PainelMultplex2vias"))
	{
		if(firmwareFile.open("firmware/mux2vias.fir") == 0 && optionsFile.open("firmware/mux2vias.opt") == 0)
		{
			firmwareFilesOK = true;
		}
	}

	if(firmwareFilesOK)
	{
		//Se o arquivo de firmware ou o arquivo de op��es estiver corrompido ent�o n�o atualiza
		if(this->verificarIntegridadeArquivoFirmware(&firmwareFile) == false
			|| this->verificarIntegridadeArquivoOpcoes(&optionsFile) == false)
		{
			return false;
		}

		//Obt�m a vers�o do firmware do arquivo
		trf::application::Version* versaoArquivo = this->getFileFirmwareVersion(&firmwareFile);
		//Obt�m o modo de atualiza��o de firmware definido no arquivo de op��es
		U8 modoAtualizacao = this->getModoAtualizacao(&optionsFile);

		switch (modoAtualizacao) {
			case 0x00:
				//Se a vers�o do arquivo n�o for mais nova que a vers�o da flash ent�o n�o atualiza
				if(versaoArquivo->major < currentPainelVersion->major)
				{
					return false;
				}
				else if(versaoArquivo->major == currentPainelVersion->major)
				{
					if(versaoArquivo->minor <= currentPainelVersion->minor)
					{
						return false;
					}
				}
				break;

			case 0x01:
				//Se o produto e a fam�lia do arquivo for diferente do produto e fam�lia da flash ent�o n�o atualiza
				if(escort::util::String::startWith(currentPainelVersion->produto, versaoArquivo->produto) == false
				  || versaoArquivo->familia != currentPainelVersion->familia)
				{
					return false;
				}
				break;

			case 0x02:
				//Se a vers�o do arquivo n�o for mais nova que a vers�o da flash ent�o n�o atualiza
				if(versaoArquivo->familia < currentPainelVersion->familia)
				{
					return false;
				}
				else if(versaoArquivo->familia == currentPainelVersion->familia)
				{
					if(versaoArquivo->major < currentPainelVersion->major)
					{
						return false;
					}
					else if(versaoArquivo->major == currentPainelVersion->major)
					{
						if(versaoArquivo->minor <= currentPainelVersion->minor)
						{
							return false;
						}
					}
				}
				break;

			case 0x03:
				//Se o firmware do arquivo n�o for do mesmo produto do firmware da flash n�o atualiza
				if(escort::util::String::startWith(currentPainelVersion->produto, versaoArquivo->produto) == false)
				{
					return false;
				}
				break;

			case 0x04:
				break;

			default:
				return false;
		}

		//Precisa atualizar o firmware
		return true;
	}

	return false;
}

bool Fachada::isMensagemSecundariaHabilitada(U8 painel)
{
	Alternancias alternancias(this->plataforma->sistemaArquivo);

	//L� path do arquivo configura��o do painel
	this->getPathAlternancias(this->pathBuffer, painel);

	//Abre o arquivo e l� as flags de exibi��o
	if(alternancias.open(this->pathBuffer) == 0) {
		U8 qntAlternancias = alternancias.readQntAlternancias();
		for(U8 indiceAlternancia = 0; indiceAlternancia < qntAlternancias; indiceAlternancia++){
			U8 qntExibicoes = alternancias.readQntExibicoes(indiceAlternancia);
			for(U8 indiceExibicoes = 0; indiceExibicoes < qntExibicoes; indiceExibicoes++){
				if(alternancias.readExibicao(indiceAlternancia, indiceExibicoes) == Alternancias::EXIBICAO_MENSAGEM_SECUNDARIA){
					alternancias.close();
					return true;
				}
			}
		}
		alternancias.close();
	}

	return false;
}

U8 Fachada::getBaudrateSerial()
{
	return this->parametrosFixos.readBaudrateSerial();
}

void Fachada::responderRS485()
{
//	this->protocoloCl.receiveRoute(this->bufferRS485);
	this->protocoloCl.receiveRoute(&this->roteiroComp);

	//if(String::getLength(this->bufferRS485) > 0)
	if(this->roteiroComp.roteiro != 0xFFFF)
	{
		this->protocoloCl.updateTimerCl();
		this->protocoloCl.sendRoute(&this->roteiroComp);//enviar ack

		char rotString[20];
		this->getLabelNumeroRoteiro(this->getRoteiroSelecionado(), rotString);

		U16 rotAtual;

		//Converter::itoa(this->roteiroComp.roteiro, rotRecebido);
		rotAtual = Converter::atoi(rotString);

		//if(String::equals(rotString, this->roteiroComp.roteiro) == false)
		//if(String::equals(rotString, rotRecebido) == false)
		if(rotAtual != this->roteiroComp.roteiro)
		{
			Converter::itoa(this->roteiroComp.roteiro,rotString);
			this->setRoteiroSelecionado(rotString); //estamos deprezando o retorno desta fun��o pois n�o � necess�rio testar o sucesso desta altera��o
		}
		//Atualiza��o de roteiro sendo feira independente de altera��o de sentido
		if(this->roteiroComp.volta == 1 && (this->getSentidoRoteiro() == IDA))
		{
			this->setSentidoRoteiro(VOLTA);
		}
		else if(this->roteiroComp.volta == 0 && (this->getSentidoRoteiro() == VOLTA))
		{
			this->setSentidoRoteiro(IDA);
		}
	}
}

void Fachada::sendRS485()
{
	char rotString[20];

	U32 deltaTime = (this->protocoloCl.getTimerCl() % 60000);
	while(deltaTime < 1000 || deltaTime > 55000)//margem de tempo para controlar atualiza��es de roteiro simult�neas
	{
		vTaskDelay(100);
		deltaTime = (this->protocoloCl.getTimerCl() % 60000);
	}

	this->getLabelNumeroRoteiro(this->getRoteiroSelecionado(), rotString);

	this->roteiroCompEnvio.roteiro = Converter::atoi(rotString);
	this->roteiroCompEnvio.volta = (this->getSentidoRoteiro() == VOLTA);//consideramos volta=1 para volta e volta=0 para ida

	this->protocoloCl.sendRoute(&roteiroCompEnvio);
}

void Fachada::tratarAgendamentos()
{
	bool finalizado = true;
	Relogio::DataHora dataHoraAtual = this->plataforma->relogio->getDataHora();
	Agenda agenda(this->plataforma->sistemaArquivo);

	do{
		//Abre o arquivo de agenda
		if(agenda.open("agenda.sch") != 0){
			return;
		}
		//L� a quantidade de agendamentos no arquivo
		U16 qntAgendamentos = agenda.readQntAgendamentos();
		if(qntAgendamentos > 32){
			qntAgendamentos = 32;
		}
		//Busca a ocorr�ncia de agendamento mais pr�xima � data/hora atual
		// levando em considera��o a opera��o do agendamento
		U16 agendamentoMaisProximo = 0xFFFF;
		U64 horaMaisProximo = 0;
		U8 operacao = 0xFF;
		U8 painel = 0xFF;
		for(U16 i = 0; i < qntAgendamentos; i++){
			//Verifica se o agendamento j� foi tratado
			if(this->ocorrenciasAgendamentos[i] == dataHoraAtual.dia){
				continue;
			}
			//L� as informa��es do agendamento
			Agenda::Agendamento agendamento;
			agenda.readAgendamento(i, &agendamento);
			//Monta a data/hora do agendamento 'i'
			Relogio::DataHora dataHora;
			String::memcpy((U8*)&dataHora, (U8*)&agendamento, sizeof(dataHora));
			if(agendamento.mascara & (1 << 0)){//dia
				dataHora.dia = dataHoraAtual.dia;
			}
			if(agendamento.mascara & (1 << 1)){//mes
				dataHora.mes = dataHoraAtual.mes;
			}
			if(agendamento.mascara & (1 << 2)){//dia da semana
				dataHora.diaSemana = dataHoraAtual.diaSemana;
			}
			if(agendamento.mascara & (1 << 3)){//ano
				dataHora.ano = dataHoraAtual.ano;
			}
			//Verifica se o agendamento 'i' tem a mesma opera��o que
			//o agendamento mais pr�ximo at� agora identificado
			if((operacao == 0xFF)
				|| (agendamento.operacao == operacao && operacao && operacao != Agenda::SELECAO_ALTERNANCIA && operacao != Agenda::SELECAO_MSG_PRINCIPAL && operacao != Agenda::SELECAO_MSG_SECUNDARIA)
				|| (agendamento.operacao == operacao && painel == agendamento.painel)){
				//Verifica se existe uma ocorr�ncia de agendamento pendente
				if(((U64)dataHora <= (U64)dataHoraAtual)
					&&
					//verificando tamb�m o dia da semana, se for o caso
					((agendamento.mascara & (1 << 2)) != 0 || dataHoraAtual.diaSemana == agendamento.diaSemana))
				{
					//Atualiza a opera��o e painel
					operacao = agendamento.operacao;
					painel = agendamento.painel;
					//Atualiza a ocorr�ncia do dia
					this->ocorrenciasAgendamentos[i] = dataHoraAtual.dia;
					//Indica que pelo menos uma ocorr�ncia existe,
					//ent�o deve executar a verifica��o novamente a fim de encontrar outras
					finalizado = false;
					//Verifica se esta ocorr�ncia � mais pr�xima da data/hora atual
					if((U64)dataHora > horaMaisProximo){
						horaMaisProximo = (U64)dataHora;
						agendamentoMaisProximo = i;
					}
				}
			}
		}
		if(agendamentoMaisProximo != 0xFFFF){
			//L� as informa��es do agendamento
			Agenda::Agendamento agendamento;
			agenda.readAgendamento(agendamentoMaisProximo, &agendamento);
			//Executa opera��o
			switch (agendamento.operacao) {
				case Agenda::SELECAO_ROTEIRO:
					this->setRoteiroSelecionado((U32)agendamento.valorParametro, true);
					break;
				case Agenda::SELECAO_IDA_VOLTA:
					this->setSentidoRoteiro(agendamento.valorParametro == 0 ? IDA : VOLTA, true);
					break;
				case Agenda::SELECAO_ALTERNANCIA:
					{
						U8 painelBkp = this->painelSelecionado;
						this->painelSelecionado = agendamento.painel;
						this->setAlternancia((U8)agendamento.valorParametro, true);
						this->painelSelecionado = painelBkp;
					}
					break;
				case Agenda::SELECAO_MSG_PRINCIPAL:
					{
						U8 painelBkp = this->painelSelecionado;
						this->painelSelecionado = agendamento.painel;
						this->setMensagemSelecionada((U32)agendamento.valorParametro, true);
						this->painelSelecionado = painelBkp;
					}
					break;
				case Agenda::SELECAO_MSG_SECUNDARIA:
					{
						U8 painelBkp = this->painelSelecionado;
						this->painelSelecionado = agendamento.painel;
						this->setMensagemSecundariaSelecionada((U32)agendamento.valorParametro, true);
						this->painelSelecionado = painelBkp;
					}
					break;
				case Agenda::ALTERACAO_HORA_SAIDA:
					this->setHoraSaida(((U8*)&agendamento.valorParametro)[0], ((U8*)&agendamento.valorParametro)[1], true);
					break;
			}
		}
		else{
			finalizado = true;
		}

		agenda.close();
		vTaskDelay(1);
	}while(!finalizado);
}

void Fachada::getVariacaoBrilho(U8 painel, U8* brilhoMax, U8* brilhoMin)
{
	*brilhoMax = this->e2prom.readBrilhoMaximo(painel);
	*brilhoMin = this->e2prom.readBrilhoMinimo(painel);
}

Fachada::Resultado Fachada::setVariacaoBrilho(U8 painel, U8 brilhoMax, U8 brilhoMin, bool force)
{
	//Verifica se a fun��o de mudar hora de sa�da est� bloqueada
	// e, caso a sess�o esteja aberta (senha de desbloqueio), o contador � resetado (mais 60 segundos de fun��es liberadas)
	if(!force && (!this->isFuncaoLiberada(ParametrosFixos::AJUSTE_BRILHO)))
	{
		return FUNCAO_BLOQUEADA;
	}

	this->e2prom.writeBrilhoMaximo(painel, brilhoMax);
	this->e2prom.writeBrilhoMinimo(painel, brilhoMin);
	this->setPainelSincronizado(painel, false);

	return SUCESSO;
}

U16 Fachada::getMotoristaSelecionado()
{
	return this->e2prom.readMotoristaSelecionado();
}

Fachada::Resultado Fachada::setMotoristaSelecionado(U16 indiceMotorista, bool force)
{
	Fachada::Resultado resultado = SUCESSO;

	//Verifica se a fun��o de mudar roteiro est� bloqueada
	// e, caso a sess�o esteja aberta (senha de desbloqueio), o contador � resetado (mais 60 segundos de fun��es liberadas)
	if(!force && (!this->isFuncaoLiberada(ParametrosFixos::SELECAO_MOTORISTA)))
	{
		return FUNCAO_BLOQUEADA;
	}
	//Muda o motorista selecionado
	this->e2prom.writeMotoristaSelecionado(indiceMotorista);
	//Sincroniza os arquivos dos paineis
	U8 qntPaineis = this->parametrosFixos.readQntPaineis();
	for (U8 p = 0; p < qntPaineis; ++p) {
		this->setPainelSincronizado(p, false);
	}

	return resultado;
}

bool Fachada::isConfigurandoTimedout()
{
	return (this->configurandoTimeout < xTaskGetTickCount());
}

void Fachada::processConfigurandoIncrement()
{
	this->setStatusFuncionamento(CONFIGURANDO_REMOTAMENTE);
	this->configurandoTimeout = xTaskGetTickCount() + 3000;
}

void Fachada::iniciarConfiguracao()
{
	this->setStatusFuncionamento(CONFIGURANDO_REMOTAMENTE);
	this->configurandoTimeout = xTaskGetTickCount() + 3000;
}

void Fachada::apagarTrava()
{
	//Apaga a trava na E2PROM
	U32 trava = 0xFFFFFFFF;
	this->e2prom.writeTravaPaineis((U8*)&trava);
}

U8 Fachada::getQntPaineisEmparelhados()
{
	return this->e2prom.readQntPaineis();
}

}
}
}
