/*
 * Protocolo.h
 *
 *  Created on: 01/04/2013
 *      Author: luciano.silva
 */

#ifndef PROTOCOLO_H_
#define PROTOCOLO_H_

#include <trf.service.net/Network/Network.h>
#include <trf.service.net\IdentificationProtocol\IdentificationProtocol.h>
#include <trf.application/Protocolotrf.h>
#include <trf.application/ProtocoloNandFFS.h>
#include "Plataforma.h"
#include "Fachada.h"
#include "trf.application/IComunicacao/IComunicacao.h"

namespace trf {
namespace pontos12 {
namespace application {

class Protocolo : public trf::application::Protocolotrf, public trf::application::ProtocoloNandFFS {
public:
	Protocolo(
			trf::application::IComunicacao* comunicacao,
			Plataforma* plataforma,
			Fachada* fachada);
	void init();
	void task();

private:
	// Comando gerais trf
	virtual void reset(trf::application::IComunicacao* interface);
	virtual void rdproduto(trf::application::IComunicacao* interface);
	virtual void rdversao(trf::application::IComunicacao* interface);
	virtual void rdCRCFirmware(trf::application::IComunicacao* interface);
	virtual void rdnumserie(trf::application::IComunicacao* interface);
	virtual void wrnumserie(trf::application::IComunicacao* interface);
	virtual void rdstatus(trf::application::IComunicacao* interface);
	virtual void factory(trf::application::IComunicacao* interface);
	// Comando NANDFFS
	virtual void fsformat(trf::application::IComunicacao* interface);
	virtual void wrchunk(trf::application::IComunicacao* interface);
	// Comandos gerais pontos-X2
	void rddatahora(trf::application::IComunicacao* interface);
	void wrdatahora(trf::application::IComunicacao* interface);
	void rde2promdump(trf::application::IComunicacao* interface);
	// Comandos do Controlador
	void rdroteiro(trf::application::IComunicacao* interface);
	void wrroteiro(trf::application::IComunicacao* interface);
	void wrtarifa(trf::application::IComunicacao* interface);
	void rdmensagem(trf::application::IComunicacao* interface);
	void wrmensagem(trf::application::IComunicacao* interface);
	void rdmsg2(trf::application::IComunicacao* interface);
	void wrmsg2(trf::application::IComunicacao* interface);
	void rdpainel(trf::application::IComunicacao* interface);
	void wrpainel(trf::application::IComunicacao* interface);
	void rdsentido(trf::application::IComunicacao* interface);
	void wrsentido(trf::application::IComunicacao* interface);
	void rdqntpaineis(trf::application::IComunicacao* interface);
	void rdqntrots(trf::application::IComunicacao* interface);
	void rdqntmsgs(trf::application::IComunicacao* interface);
	void rddimensoes(trf::application::IComunicacao* interface);
	void rdhorasaida(trf::application::IComunicacao* interface);
	void wrhorasaida(trf::application::IComunicacao* interface);
	void erasepw(trf::application::IComunicacao* interface);
	void initconfig(trf::application::IComunicacao* interface);
	void apagartrava(trf::application::IComunicacao* interface);
	void apagarpaineis(trf::application::IComunicacao* interface);
	// Comandos do Protocolo LD 11
	void receberInformacaoCatraca(trf::application::IComunicacao* interface);
	void rdroteiro11(trf::application::IComunicacao* interface);

private:
	trf::application::IComunicacao* comunicacao;
	Plataforma* plataforma;
	Fachada* fachada;
	U8 buffer[32];
};

}
}
}

#endif /* PROTOCOLO_H_ */
