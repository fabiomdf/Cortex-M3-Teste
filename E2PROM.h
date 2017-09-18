/*
 * E2PROM.h
 *
 *  Created on: 14/01/2015
 *      Author: Gustavo
 */

#ifndef E2PROM_H_
#define E2PROM_H_

#include <escort.driver\IDriverFlash\IDriverFlash.h>
#include <trf.application/trfProduct/trfProduct.h>

namespace trf {
namespace pontos12 {
namespace application {

class E2PROM {
protected:
	typedef struct {
		U8 numeroSerie[8];
		U8 address;
	} FormatoPainelAddr;

	typedef struct {
		U8 numeroSerie[8];
		U8 senhaAntiFurto[32];
		U8 travaPaineis[4];
		U8 qntPaineis;
		FormatoPainelAddr* paineisAddrs;
	} FormatoArea1;

public:
	typedef struct {
		U8 mensagem[4];
		U8 alternancia;
		U8 precisaFormatar;
		U8 mensagemSecundaria[4];
		U8 brilhoMax;
		U8 brilhoMin;
	} FormatoParametrosPainel;

	typedef struct {
		U8 statusConfig;
		U8 horaSaida;
		U8 minutosSaida;
		U8 regiao;
		U8 sentidoIda;
		U8 roteiro[4];
		U8 statusConfigAPP;
		U8 statusConfigAdaptador;
		U8 motorista[2];
		U8 tarifa[4];
	} FormatoParametrosVariaveis;

	typedef struct {
		U8 status;
		U8 horaSaida;
		U8 minutosSaida;
		U8 regiao;
		U8 sentidoIda;
		U8 roteiro[4];
		U8 mensagem[4];
		U8 alternancia;
		U8 mensagemSecundaria[4];
		U8 brilhoMax;
		U8 brilhoMin;
		U8 motorista[2];
		U8 tarifa[4];
	} FormatoParametrosVariaveisPainel;

public:
	E2PROM(escort::driver::IDriverFlash* memory);
	void format();
	void apagarEmparelhamentoInfo();
	U8 readQntPaineis();
	void writeQntPaineis(U8 qnt);
	trf::application::trfProduct::SerialNumber getSerialNumber();
	void setSerialNumber(trf::application::trfProduct::SerialNumber* sNumber);
	bool verificarSenhaAntiFurto(U8* senha);
	bool isSenhaAntiFurtoHabilitada();
	void mudarSenhaAntiFurto(U8* novaSenha);
	void readTravaPaineis(U8* trava);
	void writeTravaPaineis(U8* trava);
	U8 getParametrosVariaveisStatus();
	void setParametrosVariaveisStatus(U8 status);
	U8 readRegiaoSelecionada();
	void writeRegiaoSelecionada(U8 regiao);
	U32 readRoteiroSelecionado();
	void writeRoteiroSelecionado(U32 roteiroSelecionado);
	U32 readTarifa();
	void writeTarifa(U32 tarifa);
	U8 readHoraSaida();
	void writeHoraSaida(U8 hora);
	U8 readMinutosSaida();
	void writeMinutosSaida(U8 minutos);
	U8 readSentidoSelecionado();
	void writeSentidoSelecionado(U8 sentido);
	U8 readPainelNetAddress(U8 painel);
	U64 readPainelNumSerie(U8 painel);
	void writePainelParameters(U8 painel, U8 netAddress, U64 numSerie);
	void writePainelNetAddress(U8 painel, U8 netAddress);
	U8 readAlternanciaSelecionada(U8 painel);
	void writeAlternanciaSelecionada(U8 painel, U8 alternanciaSelecionada);
	U32 readMensagemSelecionada(U8 painel);
	void writeMensagemSelecionada(U8 painel, U32 indiceMensagem);
	U32 readMensagemSecundariaSelecionada(U8 painel);
	void writeMensagemSecundariaSelecionada(U8 painel, U32 indiceMensagem);
	void readParametrosVariaveisPainel(U8 painel, FormatoParametrosVariaveisPainel* params);
	U8 readPrecisaFormatarPainel(U8 painel);
	void writePrecisaFormatarPainel(U8 painel, U8 precisa);
	U8 readStatusConfigAPP();
	void writeStatusConfigAPP(U8 status);
	U8 readStatusConfigAdaptador();
	void writeStatusConfigAdaptador(U8 status);
	U8 readBrilhoMaximo(U8 painel);
	void writeBrilhoMaximo(U8 painel, U8 brilho);
	U8 readBrilhoMinimo(U8 painel);
	void writeBrilhoMinimo(U8 painel, U8 brilho);
	U16 readMotoristaSelecionado();
	void writeMotoristaSelecionado(U16 motoristaSelecionado);

private:
	escort::driver::IDriverFlash* memory;
};

}
}
}

#endif /* E2PROM_H_ */
