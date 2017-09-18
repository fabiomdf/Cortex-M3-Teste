/*
 * Fachada.h
 *
 *  Created on: 23/05/2012
 *      Author: arthur.padilha
 */

#ifndef FACHADA_H_
#define FACHADA_H_

#include <trf.application/Version.h>
#include "files/ParametrosVariaveis.h"
#include "files/ParametrosFixos.h"
#include "files/ListaArquivos.h"
#include "files/Mensagem.h"
#include <escort.service/Relogio/Relogio.h>
#include <escort.service/FatFs/FatFs.h>
#include <escort.service/NandFFS/NandFFS.h>
#include "Plataforma.h"
#include <trf.application/trfProduct/trfProduct.h>
#include "trf.pontos12.application/Controlador/files/Regiao.h"
#include "escort.util/SHA256.h"
#include <trf.application/Protocolotrf.h>
#include <trf.pontos12.application/Controlador/files/Agenda.h>
#include <trf.pontos12.application/Controlador/ProtocoloCl.h>
#include <trf.pontos12.service/Renderizacao/Renderizacao.h>
#include "freertos.kernel.port.cortexm3/RealviewPort/portmacro.h"
#include "E2PROM.h"
#include <trf.service.net\IdentificationProtocol\IdentificationProtocol.h>

using trf::pontos12::service::Renderizacao;

#define TAMANHO_PAGINA_FLASH 2048
#define TAMANHO_SPARE_FLASH 64

namespace trf {
namespace pontos12 {
namespace application {

class Fachada {
public:
	typedef enum{
		IDA,
		VOLTA
	}SentidoRoteiro;

	typedef enum {
		SUCESSO,
		FUNCAO_BLOQUEADA,
		FALHA_OPERACAO,
		PARAMETRO_INVALIDO,
		NOVA_CONFIG_INVALIDA,
		SENHA_INCORRETA,
		SENHA_ANTI_FURTO_INCORRETA,
		EMPARELHAMENTO_INCOMPATIVEL,
		ERRO_COMUNICACAO,
		OPERACAO_CANCELADA,
		NAO_EMPARELHADO,
		PAINEIS_TRAVADOS,
		PAINEIS_NAO_TRAVADOS
	} Resultado;

	typedef enum {
		FUNCIONANDO_OK = 0,
		ERRO_SISTEMA_ARQUIVO = (1 << 0),
		ERRO_CONFIGURACAO_INCONSISTENTE = (1 << 1),
		CONFIGURANDO_REMOTAMENTE = (1 << 2),
		ERRO_SENHA_ANTI_FURTO = (1 << 3)
	} StatusFuncionamento;

	typedef struct {
		U8 numeroSerie[8];
		U16 altura;
		U16 largura;
		struct {
			U8 ambiguo;
		}flags;
		U8 netAddr;
		U8 indice;
	} InfoPainelListado;

	typedef enum {
		STATUS_CONFIG_INVALIDA = 0xFF,
		STATUS_CONFIG_VALIDA = 0xAA,
		STATUS_CONFIG_NAO_VERIFICADA = 0x33,
		STATUS_CONFIG_E2PROM_DESATUALIZADA = 0x77
	} StatusConfig;

	typedef enum {
		STATUS_CONFIG_APP_SINCRONIZADO,
		STATUS_CONFIG_APP_PRECISANDO_SINCRONIZAR_PARAMETROS
	} StatusConfigAPP;

	typedef enum {
		STATUS_CONFIG_ADAPTADOR_SINCRONIZADO,
		STATUS_CONFIG_ADAPTADOR_PRECISANDO_SINCRONIZAR_PARAMETROS
	} StatusConfigAdaptador;

	typedef struct {
		char* string;
		U32 delayAnimacao;
		U32 delayApresentacao;
		Renderizacao::Animacao animacao;
		Renderizacao::Alinhamento alinhamento;
		U8 repeticoes;
		bool otimizacao;
	} ParametrosShowText;

public:
	Fachada(
			trf::service::net::Network* network,
			trf::service::net::IdentificationProtocol* idProtocol,
			Plataforma* plataforma);

	void init();
	void carregarConfiguracao(void (*incrementProgress)(void));
	trf::application::Version* getVersaoFirmware();
	U16 getCRCFirmware();
	bool readVersaoPainel(U8 painel, U8* versao);
	bool readCRCFirmwarePainel(U8 painel, U16* crcFW);
	trf::application::trfProduct::SerialNumber getSerialNumber();
	void setSerialNumber(trf::application::trfProduct::SerialNumber* sNumber);
	StatusFuncionamento getStatusFuncionamento();
	void setStatusFuncionamento(StatusFuncionamento status);
	void ativarModoEmergencia(bool ativa);
	Resultado coletarDump(void (*incrementProgress)(void), char* destFolder);
	Resultado coletarLog(void (*incrementProgress)(void));
	Resultado coletarDumpPainel(U8 painel, void (*incrementProgress)(void), char* destFolder);
	bool formatarPendrive(escort::service::FatFs* sistemaArquivo);
	Resultado apagarArquivos();
	Resultado format();
	void resetSystem();
	bool verificarSenhaAcesso(U32 senha);
	bool isSenhaAcessoHabilitada();
	bool isSenhaAntiFurtoHabilitada();
	void mudarSenhaAntiFurto(U8* novaSenha);
	void zerarSenhaAntiFurto();
	Resultado emparelharPaineis(InfoPainelListado** lista, bool automatico);
	U8 getQntPaineisRede();
	Resultado restaurarConfigFabrica();
	U32 getQntRoteiros();
	U16 getQntMotoristas();
	U32 getQntMensagens();
	void responderRS485();
	void sendRS485();
	void tratarAgendamentos();
	bool isConfigurandoTimedout();
	void processConfigurandoIncrement();
	void iniciarConfiguracao();
	bool isPaineisTravados();
	bool deveTravarPaineis();
	U32 getIdDestravaPaineis(InfoPainelListado* paineis, U8 qntPaineis);
	void apagarTrava();
	U8 getQntPaineisEmparelhados();

private:
	void carregarArquivos();
	bool carregarParametrosVariaveis();
	bool verificarArquivosConfig(void (*incrementProgress)(void));
	trf::application::Version* getFileFirmwareVersion(escort::service::NandFFS::File* file);
	U8 getModoAtualizacao(escort::service::NandFFS::File* optionsFile);
	bool verificarIntegridadeArquivoFirmware(escort::service::NandFFS::File* firmwareFile);
	bool verificarIntegridadeArquivoOpcoes(escort::service::NandFFS::File* optionsFile);
	bool verificarAtualizacaoFirmwarePainel(trf::application::Version* currentPainelVersion);
	bool verificarSenhaAntiFurto();

// PAINEL //
public:
	U32 getAlturaPainel(U8 painel);
	U32 getLarguraPainel(U8 painel);
	U8 getPainelSelecionado();
	U64 getPainelNumserie(U8 painel);
	U8 getPainelNetAddress(U8 painel);
	Resultado setPainelNetAddress(U8 painel, U8 netAddress);
	void writePainelParameters(U8 painel, U8 netAddress, U64 numSerie);
	Resultado travarPaineis();
	Resultado destravarPaineis(InfoPainelListado* paineis, U8 qntPaineis, U8* senhaDestrava);
	Resultado setPainelSelecionado(U8 painel);
	Resultado resetConfigFabricaPainel(U8 painel);
	Resultado getPainelStatusFuncionamento(U8 painel, U32* status, bool (*isCanceled)(void));
	bool isPainelNaRede(U8 painel);
	Resultado apagarArquivosPainel(U8 numeroPainel, bool reset);
	bool acenderPainel(U8 numeroPainel, U16 tempo);
	bool acenderPainelByAddr(U8 netAddr, U16 tempo);
	bool apagarPainel(U8 numeroPainel, U16 tempo);
	bool apagarPainelByAddr(U8 netAddr, U16 tempo);
	bool isPainelSincronizado(U8 painel);
	void setPainelSincronizado(U8 painel, bool status);
	bool isPainelPrecisandoFormatar(U8 painel);
	void setPainelPrecisandoFormatar(U8 painel, bool precisando);
	bool syncArquivosPainel(U8 painel, void (*incrementProgress)(void), bool (*isCanceled)(void));
	bool syncDataHoraPainel(U8 painel);
	Resultado coletarDumpPainel(U8 painel, void (*incrementProgress)(void));
	Resultado setModoTeste(bool ativa, U8 tipo);
	Resultado setModoTeste(bool ativa, U8 tipo, bool force);
	bool painelWrTexto(U8 painel, char* texto);

// APP //
public:
	void setAPPStatusConfig(StatusConfigAPP status);
	Resultado apagarArquivosAPP(bool reset);
	Resultado resetConfigFabricaAPP();
	StatusConfigAPP getAPPStatusConfig();
	bool isAPPHabilitado();
	void setAPPHabilitado(bool habilitaAPP);
	bool isAPPDetectado();
	Resultado getAPPStatusFuncionamento(U32* status, bool (*isCanceled)(void));
	bool isAPPNaRede();
	Resultado detectarAPP(bool (*isCanceled)(void));
	bool sincronizarAPP(void (*incrementProgress)(void), bool (*isCanceled)(void));
	bool setAPPModoDemonstracao(bool habilitaDemoAPP);
private:
	bool syncParametrosAPP(trf::service::net::ConnectionClient* client, void (*incrementProgress)(void));

// Adaptador Catraca/Sensores //
public:
	void setAdaptadorCatracaStatusConfig(StatusConfigAdaptador status);
	void setAdaptadorSensoresStatusConfig(StatusConfigAdaptador status);
	StatusConfigAdaptador getAdaptadorCatracaStatusConfig();
	StatusConfigAdaptador getAdaptadorSensoresStatusConfig();
	bool isAdaptadorCatracaHabilitado();
	bool isAdaptadorSensoresHabilitado();
	void setAdaptadorCatracaHabilitado(bool habilitaAdaptadorCatraca);
	void setAdaptadorSensoresHabilitado(bool habilitaAdaptadorSensores);
	bool isAdaptadorCatracaDetectado();
	bool isAdaptadorSensoresDetectado();
	bool isAdaptadorCatracaNaRede();
	bool isAdaptadorSensoresNaRede();
	bool isCatracaSincronizada();
	void setCatracaSincronizada(bool status);
	bool isSensoresSincronizados();
	void setSensoresSincronizados(bool status);
	Resultado detectarAdaptadorCatraca(bool (*isCanceled)(void));
	Resultado detectarAdaptadorSensores(bool (*isCanceled)(void));
	bool sincronizarAdaptadorCatraca(void (*incrementProgress)(void), bool (*isCanceled)(void));
	bool sincronizarAdaptadorSensores(void (*incrementProgress)(void), bool (*isCanceled)(void));
	bool addItemListaPaineisSensores(U8 addrPainel);
	void removeItemListaPaineisSensores(U8 addrPainel);
private:
	bool syncParametrosAdaptadorCatraca(trf::service::net::ConnectionClient* client, void (*incrementProgress)(void));
	bool syncParametrosAdaptadorSensores(trf::service::net::ConnectionClient* client, void (*incrementProgress)(void));

// PARAMETROS VARIAVEIS //
public:
	U8 getAlternancia(U8 indicePainel);
	U8 getAlternancia();
	Resultado setAlternancia(U8 indiceAlternancia);
	Resultado setAlternancia(U8 indiceAlternancia, bool force);
	U8 getQntAlternancias();
	U8 getQntExibicoesAlternancia(U8 indiceAlternancia, U8 indicePainel);
	U8 getExibicao(U8 indiceAlternancia, U8 indicePainel, U8 indiceExibicao);
	void getNomeAlternancia(U8 indiceAlternancia, char* nome);
	U32 getRoteiroSelecionado();
	void getPathRoteiro(U32 indiceRoteiro, char* path);
	void getPathMotorista(U32 indiceMotorista, char* path);
	U16 getIdRoteiro(U32 indiceRoteiro);
	void getLabelNumeroRoteiro(U32 indiceRoteiro, char* labelNumRoteiroSelecionado);
	void getLabelRoteiroIda(U32 indiceRoteiro, char* labelRoteiroSelecionado);
	void getLabelRoteiroVolta(U32 indiceRoteiro, char* labelRoteiroSelecionado);
	void getLabelNumeroComRoteiro(U32 indiceRoteiro, char* label);
	void getLabelIdComNomeMotorista(U32 indiceMotorista, char* label);
	Resultado setRoteiroSelecionado(U16 idRoteiro);
	Resultado setRoteiroSelecionado(U32 indiceRoteiro);
	Resultado setRoteiroSelecionado(U32 indiceRoteiro, bool force);
	Resultado setRoteiroSelecionado(char* numRoteiro);
	Resultado setRoteiroSelecionado(char* numRoteiro, bool force);
	Resultado setTarifaMemoria(U32 tarifa);
	U32 getTarifaMemoria();
	U8 getRegiaoSelecionada();
	Resultado setRegiaoSelecionada(U8 indiceRegiao);
	Resultado setRegiaoSelecionada(U8 indiceRegiao, bool force);
	U32 getMensagemSelecionada(U8 painel);
	U32 getMensagemSelecionada();
	U32 getMensagemSecundariaSelecionada(U8 painel);
	U32 getMensagemSecundariaSelecionada();
	void getPathMensagem(U32 indiceMensagem, char* path);
	void getLabelMessagem(U32 indiceMensagem, char* labelMensagemSelecionada);
	Resultado setMensagemSelecionada(U32 indiceMensagem);
	Resultado setMensagemSelecionada(U32 indiceMensagem, bool force);
	Resultado setMensagemSelecionada(U16 idMensagem);
	Resultado setMensagemSelecionada(U16 idMensagem, bool force);
	Resultado setMensagemSecundariaSelecionada(U32 indiceMensagem);
	Resultado setMensagemSecundariaSelecionada(U32 indiceMensagem, bool force);
	SentidoRoteiro getSentidoRoteiro();
	Resultado setSentidoRoteiro(SentidoRoteiro sentido);
	Resultado setSentidoRoteiro(SentidoRoteiro sentido, bool force);
	escort::service::Relogio::DataHora getDataHora();
	Resultado setDataHora(escort::service::Relogio::DataHora dataHora);
	void getHoraSaida(U8 *horaSaida, U8 *minutosSaida);
	Resultado setHoraSaida(U8 horaSaida, U8 minutosSaida);
	Resultado setHoraSaida(U8 horaSaida, U8 minutosSaida, bool force);
	void getVariacaoBrilho(U8 painel, U8* brilhoMax, U8* brilhoMin);
	Resultado setVariacaoBrilho(U8 painel, U8 brilhoMax, U8 brilhoMin, bool force);
	U16 getMotoristaSelecionado();
	Resultado setMotoristaSelecionado(U16 indiceMotorista, bool force);

// REGI�O //
public:
	U8 getQntRegioes();
	Regiao::FormatoDataHora getFormatoDataHora();
	char* getIdiomaPath();
	void getNomeRegiao(char* nome, U8 indiceRegiao);

// PARAMETROS FIXOS //
public:
	U8 getVersaoParametrosFixos();
	U8 getHoraBomDia();
	U8 getHoraBoaTarde();
	U8 getHoraBoaNoite();
	ParametrosFixos::FuncoesBloqueadas getFuncoesBloqueadas();
	bool isFuncaoLiberada(ParametrosFixos::FuncoesBloqueadas funcao);
	U32 getQntPaineis();
	bool isMensagemSecundariaHabilitada(U8 painel);
	U8 getBaudrateSerial();

// IDENTIFICA��O
public:
	bool isAddressAssigned();
	trf::service::net::IdentificationProtocol::WhoIsReply* whoIs(trf::application::trfProduct::SerialNumber numSerie, U32 timeout);

private:
	void getPathPainelConfig(char* path, U8 indicePainel);
	void getPathAlternancias(char* path, U8 indicePainel);

// REGISTRO DE ERROS //
private:
	bool verificarConsistenciaFonte(char* pathFonte);
	void registrarErroTamanhoArquivo(char* nomeArquivo);
	void registrarErroArquivoInexistente(char* nomeArquivo);
	void registrarErroCRCArquivo(char* nomeArquivo);
	void registrarErroArquivoDadosInconsistentes(char* nomeArquivo, char* descricao);

// COMANDOS DE COMUNICA��O NANDFFS //
private:
	bool remoteSyncArquivo(trf::service::net::ConnectionClient* client, char* path, bool (*isCanceled)(void));
	bool remoteSyncArquivo(trf::service::net::ConnectionClient* client, char* srcPath, char* destPath, bool (*isCanceled)(void));
	bool remoteSyncDir(trf::service::net::ConnectionClient* client, char* srcPath, char* destPath, bool (*isCanceled)(void));
	trf::application::Protocolotrf::ResultadoComunicacao remoteFopen(trf::service::net::ConnectionClient* client, char* path, U8 mode);
	trf::application::Protocolotrf::ResultadoComunicacao remoteFclose(trf::service::net::ConnectionClient* client);
	U32 remoteFread(trf::service::net::ConnectionClient* client, U32 nBytes, U8* buffer);
	U32 remoteFwrite(trf::service::net::ConnectionClient* client, U32 nBytes, U8* buffer);
	trf::application::Protocolotrf::ResultadoComunicacao remoteFseek(trf::service::net::ConnectionClient* client, U32 posicao);
	trf::application::Protocolotrf::ResultadoComunicacao remoteFformat(trf::service::net::ConnectionClient* client);
	trf::application::Protocolotrf::ResultadoComunicacao remoteFactory(trf::service::net::ConnectionClient* client);
	trf::application::Protocolotrf::ResultadoComunicacao remoteWrchunk(trf::service::net::ConnectionClient* client, U8* chunk);
	trf::application::Protocolotrf::ResultadoComunicacao remoteRdchunk(trf::service::net::ConnectionClient* client, U8* chunk, U32 pageNumber);
	trf::application::Protocolotrf::ResultadoComunicacao remoteRdnumchunks(trf::service::net::ConnectionClient* client, U32* numChunks);
	trf::application::Protocolotrf::ResultadoComunicacao remoteRdDumpE2prom(trf::service::net::ConnectionClient* client, U8* buffer, U32* bytesRead);

// COMANDOS GERAIS DE COMUNICA��O //
private:
	trf::application::Protocolotrf::ResultadoComunicacao remoteReset(trf::service::net::ConnectionClient* client);
	trf::application::Protocolotrf::ResultadoComunicacao remoteRdProduto(trf::service::net::ConnectionClient* client, U8* buff);
	trf::application::Protocolotrf::ResultadoComunicacao remoteRdVersao(trf::service::net::ConnectionClient* client, U8* versao);
	trf::application::Protocolotrf::ResultadoComunicacao remoteRdCRCFirmware(trf::service::net::ConnectionClient* client, U16* crcFW);

// COMANDOS DE COMUNICA��O COM PAINEIS //
private:
	trf::application::Protocolotrf::ResultadoComunicacao painelAcender(trf::service::net::ConnectionClient* client, U16 tempo);
	trf::application::Protocolotrf::ResultadoComunicacao painelApagar(trf::service::net::ConnectionClient* client, U16 tempo);
	trf::application::Protocolotrf::ResultadoComunicacao painelWrParams(trf::service::net::ConnectionClient* client, E2PROM::FormatoParametrosVariaveisPainel* params);
	bool painelSyncParamVar(trf::service::net::ConnectionClient* client, U8 painel);
	trf::application::Protocolotrf::ResultadoComunicacao painelRdDimensoes(trf::service::net::ConnectionClient* client, U16* altura, U16* largura);
	trf::application::Protocolotrf::ResultadoComunicacao painelWrDataHora(trf::service::net::ConnectionClient* client, escort::service::Relogio::DataHora dataHora);
	trf::application::Protocolotrf::ResultadoComunicacao painelWrTarifa(trf::service::net::ConnectionClient* client, U32 tarifa);
	trf::application::Protocolotrf::ResultadoComunicacao painelReadStatus(trf::service::net::ConnectionClient* client, U32* status);
	trf::application::Protocolotrf::ResultadoComunicacao painelWrEmergencia(trf::service::net::ConnectionClient* client, bool ativa);
	trf::application::Protocolotrf::ResultadoComunicacao painelWrTexto(trf::service::net::ConnectionClient* client, ParametrosShowText* parametrosShowText);
	trf::application::Protocolotrf::ResultadoComunicacao painelWrModoTeste(trf::service::net::ConnectionClient* client, bool ativa, U8 tipo);
	trf::application::Protocolotrf::ResultadoComunicacao painelWrTrava(trf::service::net::ConnectionClient* client, U8* trava);
	trf::application::Protocolotrf::ResultadoComunicacao painelLiberarTrava(trf::service::net::ConnectionClient* client, U8* chave);
	trf::application::Protocolotrf::ResultadoComunicacao painelApagarTrava(trf::service::net::ConnectionClient* client);

// COMANDOS DE COMUNICA��O COM APP //
private:
	trf::application::Protocolotrf::ResultadoComunicacao appWrRota(trf::service::net::ConnectionClient* client, char* rota);
	trf::application::Protocolotrf::ResultadoComunicacao appWrSentido(trf::service::net::ConnectionClient* client, U8 sentido);
	trf::application::Protocolotrf::ResultadoComunicacao appRdModoDemo(trf::service::net::ConnectionClient* client, U8* modo);
	trf::application::Protocolotrf::ResultadoComunicacao appWrModoDemo(trf::service::net::ConnectionClient* client, U8 modo);
	trf::application::Protocolotrf::ResultadoComunicacao appWrPainelNS(trf::service::net::ConnectionClient* client, U64 nSerie);
	trf::application::Protocolotrf::ResultadoComunicacao appWrPainelNS(trf::service::net::ConnectionClient* client, U64 nSerie, U8 indicePainelParaOAPP);

// PENDRIVE
public:
	Resultado penDriveSync(char* configFolderPath, void (*incrementProgress)(void));
	Resultado penDriveSync(char* configFolderPath, void (*incrementProgress)(void), bool force);
	Resultado penDriveUpdateFW(char* configFolderPath, void (*incrementProgress)(void));
private:
	Resultado copyFile(escort::service::NandFFS* srcFS, char* srcPath, escort::service::FatFs* destFAT, char* destPath, void (*incrementProgress)(void), bool syncMode);
	Resultado copyFolder(escort::service::NandFFS* srcFS, char* srcPath, escort::service::FatFs* destFAT, char* destPath, void (*incrementProgress)(void), bool syncMode);
	void closeAllFiles();
	Resultado copyB12File(escort::service::FatFs::File* imagem, void (*incrementProgress)(void));
	Resultado copyNFXFile(escort::service::FatFs::File* imagem, void (*incrementProgress)(void));

public:
	void initInfoPaineisListados(InfoPainelListado* infos);
	U8 listaPaineisRede(InfoPainelListado* infos);


private:
	ParametrosFixos parametrosFixos;
	ListaArquivos listaRoteiros;
	ListaArquivos listaMotoristas;
	ListaArquivos listaMensagens;
	Regiao regiao;
	E2PROM e2prom;
	trf::service::net::Network* network;
	trf::service::net::IdentificationProtocol* idProtocol;
	Plataforma* plataforma;
	ProtocoloCl protocoloCl;
	escort::service::FatFs::File dumpFirUpdate;
	escort::service::FatFs::Directory firmwareFolder;
	StatusFuncionamento statusFuncionamento;
	U8 buffer[2048];
	char pathBuffer[64];
	char pathBuffer2[64];
	U8 chunk[TAMANHO_PAGINA_FLASH];
	U8 tags[TAMANHO_SPARE_FLASH];
	escort::service::NandFFS::File file;
	U8 painelSelecionado;
	U8 statusSincronizacaoPaineis[12];
	bool appHabilitado;
	bool adaptadorCatracaHabilitado;
	bool statusSincronizacaoCatraca;
	bool adaptadorSensoresHabilitado;
	bool statusSincronizacaoSensores;
	escort::util::SHA256::sha256_context sha256Strct;
	portTickType tempoParaBloquear;
	bool emergenciaAtiva;
	U8 appNetAddress;
	U8 adaptadorCatracaNetAddress;
	U8 adaptadorSensoresNetAddress;
	U8 ocorrenciasAgendamentos[32];
	portTickType configurandoTimeout;
	U8 listaPaineisSensores[8];
	//char bufferRS485[16];
	ProtocoloCl::RoteiroComp roteiroComp;
	ProtocoloCl::RoteiroComp roteiroCompEnvio;
	xSemaphoreHandle semaforo;
};

}
}
}

#endif /* FACHADA_H_ */
