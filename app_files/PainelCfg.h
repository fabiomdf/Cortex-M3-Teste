/*
 * PainelConfig.h
 *
 *  Created on: 21/05/2012
 *      Author: Gustavo
 */

#ifndef APP_PAINELCONFIG_H_
#define APP_PAINELCONFIG_H_

#include <escort.service/NandFFS/NandFFS.h>

namespace trf {
namespace proximaparada {
namespace application {

class PainelConfig : public escort::service::NandFFS::File {
private:
	typedef struct {
		U8 versao;
		U8 reservado1[3];
		U32 altura;
		U32 largura;
		char fontePath[64];
		U8 reservado2[2];
		U16 crc;
	} FormatoPainelCfg;

public:
	PainelConfig(escort::service::NandFFS* sistemaArquivo);
	U8 readVersao();
	U32 readAltura();
	U32 readLargura();
	void readFontePath(char* path);
	U8 verificarConsistencia();
};

}
}
}

#endif /* APP_PAINELCONFIG_H_ */
