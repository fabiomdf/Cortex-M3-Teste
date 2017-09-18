/*
 * Config.h
 *
 *  Created on: 21/05/2012
 *      Author: Gustavo
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include <escort.service/NandFFS/NandFFS.h>

namespace trf {
namespace proximaparada {
namespace application {

class Config : public escort::service::NandFFS::File {
private:

	typedef struct {
		U8 rolagem;
		U8 alinhamento;
		U8 reservado[2];
		U32 tempoRolagem;
		U32 tempoApresentacao;
	} OpcoesExibicao;

	typedef struct {
		U16 qntRotas;
		char numeroRotaSelecionada[10];
		U8 modoApresentacaoHabilitado;
		U8 logHabilitado;
		S8 fusoHorario;
		U8 sentidoRota;
		OpcoesExibicao opcoesDataHora;
	} FormatoCfg;

public:
	Config(escort::service::NandFFS* sistemaArquivo);
	U8 readQntRotas();
	void readRotaSelecionada(char* numRotaSelec);
	U8 readRolagemDataHora();
	U8 readAlinhamentoDataHora();
	U32 readTempoRolagemDataHora();
	U32 readTempoApresentacaoDataHora();
	bool readModoApresentacaoHabilitado();
	bool readLogHabilitado();
	S8 readFusoHorario();
	U8 readSentidoRota();
};

}
}
}

#endif /* CONFIG_H_ */
