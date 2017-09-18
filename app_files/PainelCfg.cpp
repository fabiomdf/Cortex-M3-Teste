/*
 * PainelConfig.cpp
 *
 *  Created on: 21/05/2012
 *      Author: Gustavo
 */

#include "PainelCfg.h"
#include <escort.util/CRC16CCITT.h>


using escort::service::NandFFS;

namespace trf {
namespace proximaparada {
namespace application {

//Formato do arquivo painel.cfg/////////////////////////////////////////////////
//
//<uint8 nome="versao"/>
//<uint8[] nome="reservado1" length="3"/>
//<uint32 nome="altura"/>
//<uint32 nome="largura"/>
//<string nome="fontePath" length="64">
//<uint8[] nome="reservado2" length="2"/>
//<uint16 nome="crc"/>
//////////////////////////////////////////////////////////////////////////

PainelConfig::PainelConfig(NandFFS* sistemaArquivo) :
		NandFFS::File(sistemaArquivo)
{}

U8 PainelConfig::readVersao()
{
	const FormatoPainelCfg* addr = 0;
	U8 versao;
	this->read((U32)&addr->versao,sizeof(versao), &versao);
	return versao;
}

U32 PainelConfig::readAltura()
{
	const FormatoPainelCfg* addr = 0;
	U32 altura;
	this->read((U32)&addr->altura, sizeof(altura), (U8*)&altura);
	return altura;
}

U32 PainelConfig::readLargura()
{
	const FormatoPainelCfg* addr = 0;
	U32 largura;
	this->read((U32)&addr->largura, sizeof(largura), (U8*)&largura);
	return largura;
}

void PainelConfig::readFontePath(char* path)
{
	const FormatoPainelCfg* addr = 0;
	for (U8 i = 0; i < sizeof(addr->fontePath); ++i) {
		this->read(((U32)&addr->fontePath) + i, 1, (U8*)&path[i]);
		if(path[i] == '\0'){
			break;
		}
	}
}

U8 PainelConfig::verificarConsistencia()
{
	escort::util::CRC16CCITT crc16;
	U32 tamanhoArquivo = this->getSize();
	const FormatoPainelCfg* addr = 0;

	//Verifica se o tamanho � incompat�vel com tipo do arquivo
	if(tamanhoArquivo != sizeof(FormatoPainelCfg))
	{
		return 1;
	}

	//Obtem o valor de CRC que est� no arquivo
	U16 crcArquivo;
	this->read((U32)&addr->crc, sizeof(crcArquivo), (U8*)&crcArquivo);
	//Calcula o CRC do arquivo
	crc16.reset();
	for(U32 i = 0; i < tamanhoArquivo; ++i)
	{
		if(i < (U32)&addr->crc || i >= (U32)&addr->crc + sizeof(addr->crc))
		{
			U8 dataByte;
			this->read(i, sizeof(dataByte), &dataByte);
			crc16.update(dataByte);
		}
	}
	//Verifica se o CRC est� batendo
	if(crc16.getValue() != crcArquivo)
	{
		return 2;
	}

	return 0;
}

}
}
}
