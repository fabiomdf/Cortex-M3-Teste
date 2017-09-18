/*
 * E2PROM.cpp
 *
 *  Created on: 14/01/2015
 *      Author: Gustavo
 */

#include "E2PROM.h"
#include <escort.util/String.h>

using escort::util::String;


#define PARAMETROS_VARIAVEIS_ADDR	256


namespace trf {
namespace pontos12 {
namespace application {

E2PROM::E2PROM(escort::driver::IDriverFlash* memory) :
		memory(memory)
{}

void E2PROM::format()
{
	//Apaga as informa��es de emparelhamento dos paineis
	U8 qntPaineis = this->readQntPaineis();
	if(qntPaineis != 0xFF){
		for (int i = 0; i < qntPaineis; ++i) {
			this->writePainelParameters(i, (U8)-1, (U64)-1);
		}
		this->writeQntPaineis(0);
	}
	//Apaga a regi�o de par�metros vari�veis
	for (int i = 0; i < sizeof(FormatoParametrosVariaveis); ++i) {
		this->memory->write(PARAMETROS_VARIAVEIS_ADDR + i, 0xFF);
	}
}

void E2PROM::apagarEmparelhamentoInfo()
{
	FormatoArea1* addr = 0;

	this->writeQntPaineis(0xFF);
	for (U16 i = (U32)&addr->paineisAddrs; i < PARAMETROS_VARIAVEIS_ADDR; ++i) {
		this->memory->write(i, 0xFF);
	}
}

U8 E2PROM::readQntPaineis()
{
	U8 qnt;
	FormatoArea1* addr = 0;
	this->memory->read((U8*)&qnt, (U32)&addr->qntPaineis, sizeof(addr->qntPaineis));
	return qnt;
}

void E2PROM::writeQntPaineis(U8 qnt)
{
	FormatoArea1* addr = 0;
	this->memory->write((U8*)&qnt, (U32)&addr->qntPaineis, sizeof(addr->qntPaineis));
}

trf::application::trfProduct::SerialNumber E2PROM::getSerialNumber()
{
	FormatoArea1* addr = 0;
	trf::application::trfProduct::SerialNumber sNumber;
	this->memory->read((U8*)&sNumber, (U32)&addr->numeroSerie, sizeof(addr->numeroSerie));
	return sNumber;
}

void E2PROM::setSerialNumber(trf::application::trfProduct::SerialNumber* sNumber)
{
	FormatoArea1* addr = 0;
	this->memory->write((U8*)sNumber, (U32)&addr->numeroSerie, sizeof(addr->numeroSerie));
}

bool E2PROM::verificarSenhaAntiFurto(U8* senha)
{
	FormatoArea1* addr = 0;
	bool result = true;
	for (int i = 0; (i < 32) && (result == true); ++i) {
		U8 tmp = this->memory->read((U32)&addr->senhaAntiFurto + i);
		if(tmp != senha[i]){
			result = false;
		}
	}
	return result;
}

bool E2PROM::isSenhaAntiFurtoHabilitada()
{
	FormatoArea1* addr = 0;
	U8 first = this->memory->read((U32)&addr->senhaAntiFurto[0]);
	if(this->memory->read((U32)&addr->senhaAntiFurto[0]) != 0x00
		&& this->memory->read((U32)&addr->senhaAntiFurto[0]) != 0xFF)
	{
		return true;
	}
	for (int i = 0; i < 32; ++i) {
		if(this->memory->read((U32)&addr->senhaAntiFurto[i]) != first){
			return true;
		}
	}
	return false;
}

void E2PROM::mudarSenhaAntiFurto(U8* novaSenha)
{
	FormatoArea1* addr = 0;
	this->memory->write(novaSenha, (U32)&addr->senhaAntiFurto, sizeof(addr->senhaAntiFurto));
}

void E2PROM::readTravaPaineis(U8* trava)
{
	FormatoArea1* addr = 0;
	this->memory->read((U8*)trava, (U32)&addr->travaPaineis, sizeof(addr->travaPaineis));
}

void E2PROM::writeTravaPaineis(U8* trava)
{
	FormatoArea1* addr = 0;
	this->memory->write((U8*)trava, (U32)&addr->travaPaineis, sizeof(addr->travaPaineis));
}

U8 E2PROM::getParametrosVariaveisStatus()
{
	FormatoParametrosVariaveis* addr = (FormatoParametrosVariaveis*)PARAMETROS_VARIAVEIS_ADDR;
	U8 status;
	this->memory->read((U8*)&status, (U32)&addr->statusConfig, sizeof(addr->statusConfig));
	return status;
}

void E2PROM::setParametrosVariaveisStatus(U8 status)
{
	FormatoParametrosVariaveis* addr = (FormatoParametrosVariaveis*)PARAMETROS_VARIAVEIS_ADDR;
	this->memory->write((U8*)&status, (U32)&addr->statusConfig, sizeof(addr->statusConfig));
}

U8 E2PROM::readRegiaoSelecionada()
{
	FormatoParametrosVariaveis* addr = (FormatoParametrosVariaveis*)PARAMETROS_VARIAVEIS_ADDR;
	U8 regiao;
	this->memory->read((U8*)&regiao, (U32)&addr->regiao, sizeof(addr->regiao));
	return regiao;
}

void E2PROM::writeRegiaoSelecionada(U8 regiao)
{
	FormatoParametrosVariaveis* addr = (FormatoParametrosVariaveis*)PARAMETROS_VARIAVEIS_ADDR;
	this->memory->write((U8*)&regiao, (U32)&addr->regiao, sizeof(addr->regiao));
}

U32 E2PROM::readRoteiroSelecionado()
{
	FormatoParametrosVariaveis* addr = (FormatoParametrosVariaveis*)PARAMETROS_VARIAVEIS_ADDR;
	U32 roteiro;
	this->memory->read((U8*)&roteiro, (U32)&addr->roteiro, sizeof(addr->roteiro));
	return roteiro;
}

void E2PROM::writeRoteiroSelecionado(U32 roteiroSelecionado)
{
	FormatoParametrosVariaveis* addr = (FormatoParametrosVariaveis*)PARAMETROS_VARIAVEIS_ADDR;
	this->memory->write((U8*)&roteiroSelecionado, (U32)&addr->roteiro, sizeof(addr->roteiro));
	//Mudan�a de roteiro apaga a tarifa recebida da catraca
	U32 tarifa = 0xFFFFFFFF;
	this->memory->write((U8*)&tarifa, (U32)&addr->tarifa, sizeof(addr->tarifa));
}

U32 E2PROM::readTarifa()
{
	FormatoParametrosVariaveis* addr = (FormatoParametrosVariaveis*)PARAMETROS_VARIAVEIS_ADDR;
	U32 tarifa;
	this->memory->read((U8*)&tarifa, (U32)&addr->tarifa, sizeof(addr->tarifa));
	return tarifa;
}

void E2PROM::writeTarifa(U32 tarifa)
{
	FormatoParametrosVariaveis* addr = (FormatoParametrosVariaveis*)PARAMETROS_VARIAVEIS_ADDR;
	this->memory->write((U8*)&tarifa, (U32)&addr->tarifa, sizeof(addr->tarifa));
}

U8 E2PROM::readHoraSaida()
{
	FormatoParametrosVariaveis* addr = (FormatoParametrosVariaveis*)PARAMETROS_VARIAVEIS_ADDR;
	U8 hora;
	this->memory->read((U8*)&hora, (U32)&addr->horaSaida, sizeof(addr->horaSaida));
	return hora;
}

void E2PROM::writeHoraSaida(U8 hora)
{
	FormatoParametrosVariaveis* addr = (FormatoParametrosVariaveis*)PARAMETROS_VARIAVEIS_ADDR;
	this->memory->write((U8*)&hora, (U32)&addr->horaSaida, sizeof(addr->horaSaida));
}

U8 E2PROM::readMinutosSaida()
{
	FormatoParametrosVariaveis* addr = (FormatoParametrosVariaveis*)PARAMETROS_VARIAVEIS_ADDR;
	U8 minutos;
	this->memory->read((U8*)&minutos, (U32)&addr->minutosSaida, sizeof(addr->minutosSaida));
	return minutos;
}

void E2PROM::writeMinutosSaida(U8 minutos)
{
	FormatoParametrosVariaveis* addr = (FormatoParametrosVariaveis*)PARAMETROS_VARIAVEIS_ADDR;
	this->memory->write((U8*)&minutos, (U32)&addr->minutosSaida, sizeof(addr->minutosSaida));
}

U8 E2PROM::readSentidoSelecionado()
{
	FormatoParametrosVariaveis* addr = (FormatoParametrosVariaveis*)PARAMETROS_VARIAVEIS_ADDR;
	U8 sentido;
	this->memory->read((U8*)&sentido, (U32)&addr->sentidoIda, sizeof(addr->sentidoIda));
	return sentido;
}

void E2PROM::writeSentidoSelecionado(U8 sentido)
{
	FormatoParametrosVariaveis* addr = (FormatoParametrosVariaveis*)PARAMETROS_VARIAVEIS_ADDR;
	this->memory->write((U8*)&sentido, (U32)&addr->sentidoIda, sizeof(addr->sentidoIda));
}

U8 E2PROM::readPainelNetAddress(U8 painel)
{
	FormatoPainelAddr* addr = (FormatoPainelAddr*)&((FormatoArea1*)0)->paineisAddrs;
	U8 address;
	this->memory->read((U8*)&address, (U32)&addr->address + (painel*sizeof(FormatoPainelAddr)), sizeof(addr->address));
	return address;
}

U64 E2PROM::readPainelNumSerie(U8 painel)
{
	FormatoPainelAddr* addr = (FormatoPainelAddr*)&((FormatoArea1*)0)->paineisAddrs;
	U64 numSerie;
	this->memory->read((U8*)&numSerie, (U32)&addr->numeroSerie + (painel*sizeof(FormatoPainelAddr)), sizeof(addr->numeroSerie));
	return numSerie;
}

void E2PROM::writePainelParameters(U8 painel, U8 netAddress, U64 numSerie)
{
	FormatoPainelAddr* addr = (FormatoPainelAddr*)&((FormatoArea1*)0)->paineisAddrs;
	this->memory->write((U8*)&netAddress, (U32)&addr->address + (painel*sizeof(FormatoPainelAddr)), sizeof(addr->address));
	this->memory->write((U8*)&numSerie, (U32)&addr->numeroSerie + (painel*sizeof(FormatoPainelAddr)), sizeof(addr->numeroSerie));
}

void E2PROM::writePainelNetAddress(U8 painel, U8 netAddress)
{
	FormatoPainelAddr* addr = (FormatoPainelAddr*)&((FormatoArea1*)0)->paineisAddrs;
	this->memory->write((U8*)&netAddress, (U32)&addr->address + (painel*sizeof(FormatoPainelAddr)), sizeof(addr->address));
}

U8 E2PROM::readAlternanciaSelecionada(U8 painel)
{
	FormatoParametrosPainel* addr = (FormatoParametrosPainel*)(PARAMETROS_VARIAVEIS_ADDR + sizeof(FormatoParametrosVariaveis));
	U8 alternancia;
	this->memory->read((U8*)&alternancia, (U32)&addr->alternancia + (painel*sizeof(FormatoParametrosPainel)), sizeof(addr->alternancia));
	return alternancia;
}

void E2PROM::writeAlternanciaSelecionada(U8 painel, U8 alternanciaSelecionada)
{
	FormatoParametrosPainel* addr = (FormatoParametrosPainel*)(PARAMETROS_VARIAVEIS_ADDR + sizeof(FormatoParametrosVariaveis));
	this->memory->write((U8*)&alternanciaSelecionada, (U32)&addr->alternancia + (painel*sizeof(FormatoParametrosPainel)), sizeof(addr->alternancia));
}

U32 E2PROM::readMensagemSelecionada(U8 painel)
{
	FormatoParametrosPainel* addr = (FormatoParametrosPainel*)(PARAMETROS_VARIAVEIS_ADDR + sizeof(FormatoParametrosVariaveis));
	U32 mensagem;
	this->memory->read((U8*)&mensagem, (U32)&addr->mensagem + (painel*sizeof(FormatoParametrosPainel)), sizeof(addr->mensagem));
	return mensagem;
}

void E2PROM::writeMensagemSelecionada(U8 painel, U32 indiceMensagem)
{
	FormatoParametrosPainel* addr = (FormatoParametrosPainel*)(PARAMETROS_VARIAVEIS_ADDR + sizeof(FormatoParametrosVariaveis));
	this->memory->write((U8*)&indiceMensagem, (U32)&addr->mensagem + (painel*sizeof(FormatoParametrosPainel)), sizeof(addr->mensagem));
}

U32 E2PROM::readMensagemSecundariaSelecionada(U8 painel)
{
	FormatoParametrosPainel* addr = (FormatoParametrosPainel*)(PARAMETROS_VARIAVEIS_ADDR + sizeof(FormatoParametrosVariaveis));
	U32 mensagem;
	this->memory->read((U8*)&mensagem, (U32)&addr->mensagemSecundaria + (painel*sizeof(FormatoParametrosPainel)), sizeof(addr->mensagemSecundaria));
	return mensagem;
}

void E2PROM::writeMensagemSecundariaSelecionada(U8 painel, U32 indiceMensagem)
{
	FormatoParametrosPainel* addr = (FormatoParametrosPainel*)(PARAMETROS_VARIAVEIS_ADDR + sizeof(FormatoParametrosVariaveis));
	this->memory->write((U8*)&indiceMensagem, (U32)&addr->mensagemSecundaria + (painel*sizeof(FormatoParametrosPainel)), sizeof(addr->mensagemSecundaria));
}

void E2PROM::readParametrosVariaveisPainel(U8 painel, FormatoParametrosVariaveisPainel* params)
{
	U8 tmp8; 
	U32 tmp32;
	U16 tmp16;

	//Copia o status
	tmp8 = this->getParametrosVariaveisStatus();
	String::memcpy(&params->status, &tmp8, sizeof(tmp8));
	//Copia a hora de sa�da
	tmp8 = this->readHoraSaida();
	String::memcpy(&params->horaSaida, &tmp8, sizeof(tmp8));
	//Copia o minuto de sa�da
	tmp8 = this->readMinutosSaida();
	String::memcpy(&params->minutosSaida, &tmp8, sizeof(tmp8));
	//Copia a regi�o selecionada
	tmp8 = this->readRegiaoSelecionada();
	String::memcpy(&params->regiao, &tmp8, sizeof(tmp8));
	//Copia o sentido selecionado
	tmp8 = this->readSentidoSelecionado();
	String::memcpy(&params->sentidoIda, &tmp8, sizeof(tmp8));
	//Copia o roteiro selecionado
	tmp32 = this->readRoteiroSelecionado();
	String::memcpy(params->roteiro, (U8*)&tmp32, sizeof(tmp32));
	//Copia a mensagem selecionada
	tmp32 = this->readMensagemSelecionada(painel);
	String::memcpy(params->mensagem, (U8*)&tmp32, sizeof(tmp32));
	//Copia a altern�ncia selecionada
	tmp8 = this->readAlternanciaSelecionada(painel);
	String::memcpy(&params->alternancia, &tmp8, sizeof(tmp8));
	//Copia a mensagem secund�ria selecionada
	tmp32 = this->readMensagemSecundariaSelecionada(painel);
	String::memcpy(params->mensagemSecundaria, (U8*)&tmp32, sizeof(tmp32));
	//Copia o brilho m�ximo
	tmp8 = this->readBrilhoMaximo(painel);
	String::memcpy(&params->brilhoMax, &tmp8, sizeof(tmp8));
	//Copia o brilho m�nimo
	tmp8 = this->readBrilhoMinimo(painel);
	String::memcpy(&params->brilhoMin, &tmp8, sizeof(tmp8));
	//Copia o motorista selecionado
	tmp16 = this->readMotoristaSelecionado();
	String::memcpy(params->motorista, (U8*)&tmp16, sizeof(tmp16));
	//Copia a tarifa
	tmp32 = this->readTarifa();
	String::memcpy(params->tarifa, (U8*)&tmp32, sizeof(tmp32));
}

U8 E2PROM::readPrecisaFormatarPainel(U8 painel)
{
	FormatoParametrosPainel* addr = (FormatoParametrosPainel*)(PARAMETROS_VARIAVEIS_ADDR + sizeof(FormatoParametrosVariaveis));
	U8 precisa;
	this->memory->read((U8*)&precisa, (U32)&addr->precisaFormatar + (painel*sizeof(FormatoParametrosPainel)), sizeof(addr->precisaFormatar));
	return precisa;
}

void E2PROM::writePrecisaFormatarPainel(U8 painel, U8 precisa)
{
	FormatoParametrosPainel* addr = (FormatoParametrosPainel*)(PARAMETROS_VARIAVEIS_ADDR + sizeof(FormatoParametrosVariaveis));
	this->memory->write((U8*)&precisa, (U32)&addr->precisaFormatar + (painel*sizeof(FormatoParametrosPainel)), sizeof(addr->precisaFormatar));
}

U8 E2PROM::readStatusConfigAPP()
{
	FormatoParametrosVariaveis* addr = (FormatoParametrosVariaveis*)PARAMETROS_VARIAVEIS_ADDR;
	U8 status;
	this->memory->read((U8*)&status, (U32)&addr->statusConfigAPP, sizeof(addr->statusConfigAPP));
	return status;
}

void E2PROM::writeStatusConfigAPP(U8 status)
{
	FormatoParametrosVariaveis* addr = (FormatoParametrosVariaveis*)PARAMETROS_VARIAVEIS_ADDR;
	this->memory->write((U8*)&status, (U32)&addr->statusConfigAPP, sizeof(addr->statusConfigAPP));
}

U8 E2PROM::readStatusConfigAdaptador()
{
	FormatoParametrosVariaveis* addr = (FormatoParametrosVariaveis*)PARAMETROS_VARIAVEIS_ADDR;
	U8 status;
	this->memory->read((U8*)&status, (U32)&addr->statusConfigAdaptador, sizeof(addr->statusConfigAdaptador));
	return status;
}

void E2PROM::writeStatusConfigAdaptador(U8 status)
{
	FormatoParametrosVariaveis* addr = (FormatoParametrosVariaveis*)PARAMETROS_VARIAVEIS_ADDR;
	this->memory->write((U8*)&status, (U32)&addr->statusConfigAdaptador, sizeof(addr->statusConfigAdaptador));
}

U8 E2PROM::readBrilhoMaximo(U8 painel)
{
	FormatoParametrosPainel* addr = (FormatoParametrosPainel*)(PARAMETROS_VARIAVEIS_ADDR + sizeof(FormatoParametrosVariaveis));
	U8 brilho;
	this->memory->read((U8*)&brilho, (U32)&addr->brilhoMax + (painel*sizeof(FormatoParametrosPainel)), sizeof(addr->brilhoMax));
	return brilho;
}

void E2PROM::writeBrilhoMaximo(U8 painel, U8 brilho)
{
	FormatoParametrosPainel* addr = (FormatoParametrosPainel*)(PARAMETROS_VARIAVEIS_ADDR + sizeof(FormatoParametrosVariaveis));
	this->memory->write((U8*)&brilho, (U32)&addr->brilhoMax + (painel*sizeof(FormatoParametrosPainel)), sizeof(addr->brilhoMax));
}

U8 E2PROM::readBrilhoMinimo(U8 painel)
{
	FormatoParametrosPainel* addr = (FormatoParametrosPainel*)(PARAMETROS_VARIAVEIS_ADDR + sizeof(FormatoParametrosVariaveis));
	U8 brilho;
	this->memory->read((U8*)&brilho, (U32)&addr->brilhoMin + (painel*sizeof(FormatoParametrosPainel)), sizeof(addr->brilhoMin));
	return brilho;
}

void E2PROM::writeBrilhoMinimo(U8 painel, U8 brilho)
{
	FormatoParametrosPainel* addr = (FormatoParametrosPainel*)(PARAMETROS_VARIAVEIS_ADDR + sizeof(FormatoParametrosVariaveis));
	this->memory->write((U8*)&brilho, (U32)&addr->brilhoMin + (painel*sizeof(FormatoParametrosPainel)), sizeof(addr->brilhoMin));
}

U16 E2PROM::readMotoristaSelecionado()
{
	FormatoParametrosVariaveis* addr = (FormatoParametrosVariaveis*)PARAMETROS_VARIAVEIS_ADDR;
	U16 motorista;
	this->memory->read((U8*)&motorista, (U32)&addr->motorista, sizeof(addr->motorista));
	return motorista;
}

void E2PROM::writeMotoristaSelecionado(U16 motoristaSelecionado)
{
	FormatoParametrosVariaveis* addr = (FormatoParametrosVariaveis*)PARAMETROS_VARIAVEIS_ADDR;
	this->memory->write((U8*)&motoristaSelecionado, (U32)&addr->motorista, sizeof(addr->motorista));
}

}
}
}
