/*
 * Config.cpp
 *
 *  Created on: 21/05/2012
 *      Author: Gustavo
 */

#include "ConfigCfg.h"
#include <escort.util/CRC16CCITT.h>


using escort::service::NandFFS;

namespace trf {
namespace proximaparada {
namespace application {

Config::Config(NandFFS* sistemaArquivo) :
		NandFFS::File(sistemaArquivo)
{}

U8 Config::readQntRotas()
{
	const FormatoCfg* addr = 0;
	U8 qntRotas;
	this->read((U32)&addr->qntRotas,sizeof(qntRotas), &qntRotas);
	return qntRotas;
}

void Config::readRotaSelecionada(char* numRotaSelec)
{
	const FormatoCfg* addr = 0;
	this->read((U32)&addr->numeroRotaSelecionada, sizeof(addr->numeroRotaSelecionada), (U8*)numRotaSelec);
}

U8 Config::readRolagemDataHora()
{
	const FormatoCfg* addr = 0;
	U8 rolagem;
	this->read((U32)&addr->opcoesDataHora.rolagem, sizeof(addr->opcoesDataHora.rolagem), (U8*)&rolagem);
	return rolagem;
}

U8 Config::readAlinhamentoDataHora()
{
	const FormatoCfg* addr = 0;
	U8 alinhamento;
	this->read((U32)&addr->opcoesDataHora.alinhamento, sizeof(addr->opcoesDataHora.alinhamento), (U8*)&alinhamento);
	return alinhamento;
}

U32 Config::readTempoRolagemDataHora()
{
	const FormatoCfg* addr = 0;
	U32 tempoRolagem;
	this->read((U32)&addr->opcoesDataHora.tempoRolagem, sizeof(addr->opcoesDataHora.tempoRolagem), (U8*)&tempoRolagem);
	return tempoRolagem;
}

U32 Config::readTempoApresentacaoDataHora()
{
	const FormatoCfg* addr = 0;
	U32 tempoApresentacao;
	this->read((U32)&addr->opcoesDataHora.tempoApresentacao, sizeof(addr->opcoesDataHora.tempoApresentacao), (U8*)&tempoApresentacao);
	return tempoApresentacao;
}

bool Config::readModoApresentacaoHabilitado()
{
	const FormatoCfg* addr = 0;
	bool habilitado;
	this->read((U32)&addr->modoApresentacaoHabilitado, sizeof(addr->modoApresentacaoHabilitado), (U8*)&habilitado);
	return habilitado;
}

bool Config::readLogHabilitado()
{
	const FormatoCfg* addr = 0;
	bool habilitado;
	this->read((U32)&addr->logHabilitado, sizeof(addr->logHabilitado), (U8*)&habilitado);
	return habilitado;
}

S8 Config::readFusoHorario()
{
	const FormatoCfg* addr = 0;
	S8 fusoHorario;
	this->read((U32)&addr->fusoHorario, sizeof(addr->fusoHorario), (U8*)&fusoHorario);
	return fusoHorario;
}

U8 Config::readSentidoRota()
{
	const FormatoCfg* addr = 0;
	U8 sentido;
	this->read((U32)&addr->sentidoRota, sizeof(addr->sentidoRota), (U8*)&sentido);
	return sentido;
}

}
}
}
