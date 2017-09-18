/*
 * Protocolo.cpp
 *
 *  Created on: 01/04/2013
 *      Author: luciano.silva
 */

#include "Protocolo.h"
#include <freertos.kernel/FreeRTOSKernel/FreeRTOSKernel.h>
#include <escort.util/CRC16CCITT.h>
#include <escort.util/Math.h>
#include <escort.util/String.h>
#include <trf.application/Version.h>
#include <trf.application/trfProduct/trfProduct.h>

using trf::service::net::ConnectionClient;
using trf::service::net::ConnectionListener;
using escort::util::Math;
using escort::util::String;
using trf::application::IComunicacao;
using escort::util::CRC16CCITT;
using escort::service::FatFs;
using escort::service::Relogio;

#define APENAS_NA_RAIZ false
#define NA_RAIZ_E_TEMP true

#define DELAY_TO_RESET 100

namespace trf {
namespace pontos12 {
namespace application {

Protocolo::Protocolo(
		IComunicacao* comunicacao,
		Plataforma* plataforma,
		Fachada* fachada) :
			ProtocoloNandFFS(plataforma->sistemaArquivo),
			comunicacao(comunicacao),
			plataforma(plataforma),
			fachada(fachada)
{}

void Protocolo_task(void* arg)
{
	((Protocolo*)arg)->task();
}

void Protocolo::init()
{
	xTaskCreate(
			(pdTASK_CODE)Protocolo_task,
			(const signed char*)"protocol",
			424,
			(void*)this,
			(freertos_kernel_FreeRTOSKernel_maxPriorities - 3),
			0);
}

void Protocolo::reset(trf::application::IComunicacao* interface)
{
	U8 answer = SUCESSO_COMM;

	//Envia resposta
	interface->send(&answer, 1);

	//Finaliza a comunica��o
	interface->stop();

	//Aguarda comando ser enviado antes de resetar
	vTaskDelay(DELAY_TO_RESET);
	//Reseta o sistema
	this->plataforma->resetSystem();
}

void Protocolo::rdproduto(trf::application::IComunicacao* interface)
{
	CRC16CCITT crc16;
	crc16.reset();
	char* answer = this->fachada->getVersaoFirmware()->produto;
	U8 lenght = String::getLength(answer);

	//Envia quantidade de bytes
	interface->send((U8 *)&lenght, sizeof(lenght));
	crc16.update((U8 *)&lenght, sizeof(lenght));

	//Envia Id do produto
	interface->send((U8*)answer, lenght);
	crc16.update((U8*)answer, lenght);

	//Envia o CRC
	U16 crcEnviado = crc16.getValue();
	interface->send((U8*)&crcEnviado, sizeof(crcEnviado));
}

void Protocolo::rdversao(trf::application::IComunicacao* interface)
{
	trf::application::Version* version = this->fachada->getVersaoFirmware();
	//Envia vers�o do firmware
	interface->send((U8 *)&version->familia, 3);
}

void Protocolo::rdCRCFirmware(trf::application::IComunicacao* interface)
{
	U16 crcFW = this->fachada->getCRCFirmware();
	//Envia o CRC do firmware
	interface->send((U8 *)&crcFW, 2);
}

void Protocolo::rdnumserie(trf::application::IComunicacao* interface)
{
	CRC16CCITT crc16;
	crc16.reset();
	//Obt�m o n�mero de s�rie
	trf::application::trfProduct::SerialNumber answer = this->fachada->getSerialNumber();
	//Envia n�mero de s�rie
	interface->send((U8*)&answer, sizeof(answer));
	crc16.update((U8*)&answer, sizeof(answer));
	//Envia CRC
	U16 crcCalculado = crc16.getValue();
	interface->send((U8*)&crcCalculado, sizeof(crcCalculado));
}

void Protocolo::wrnumserie(trf::application::IComunicacao* interface)
{
	CRC16CCITT crc16;
	crc16.reset();
	U8 answer = SUCESSO_COMM;
	trf::application::trfProduct::SerialNumber numSerie;
	//L� n�mero de s�rie
	if(interface->receive((U8*)&numSerie, sizeof(numSerie)) < sizeof(numSerie)) {
		//Envia resposta negativa
		answer = TIMEOUT_COMM;
		interface->send((U8 *)&answer, sizeof(answer));
		return;
	}
	//Calcula o CRC
	crc16.update((U8*)&numSerie, sizeof(numSerie));
	//L� o CRC
	U16 crcRecebido;
	if(interface->receive((U8*)&crcRecebido, sizeof(crcRecebido)) < sizeof(crcRecebido)) {
		//Envia resposta negativa
		answer = TIMEOUT_COMM;
		interface->send((U8 *)&answer, sizeof(answer));
		return;
	}
	//Verifica o CRC
	if(crcRecebido != crc16.getValue()){
		//Envia resposta negativa
		answer = ERRO_CRC_COMM;
		interface->send((U8 *)&answer, sizeof(answer));
		return;
	}
	//Atualiza n�mero de s�rie
	this->fachada->setSerialNumber(&numSerie);
	//Envia resposta positiva
	interface->send((U8 *)&answer, sizeof(answer));
}

void Protocolo::rdstatus(trf::application::IComunicacao* interface)
{
	//Obtem o status
	U32 answer = (U32) this->fachada->getStatusFuncionamento();
	//Calcula o CRC
	CRC16CCITT crc16;
	crc16.reset();
	crc16.update((U8*)&answer, sizeof(answer));
	U16 crcEnviado = crc16.getValue();
	//Envia status do painel
	interface->send((U8*)&answer, sizeof(answer));
	//Envia CRC
	interface->send((U8*)&crcEnviado, sizeof(crcEnviado));
}

void Protocolo::factory(trf::application::IComunicacao* interface)
{
	U8 answer = Protocolotrf::SUCESSO_COMM;

	//Reseta as configura��es de f�brica
	if(this->fachada->restaurarConfigFabrica() == false) {
		//Envia resposta negativa
		answer = Protocolotrf::FALHA_NA_FORMATACAO_COMM;
		interface->send(&answer, 1);
		return;
	}

	//Envia resposta positiva
	interface->send((U8 *)&answer, sizeof(answer));
}

void Protocolo::rddatahora(trf::application::IComunicacao* interface)
{
	CRC16CCITT crc16;
	crc16.reset();
	//L� a data/hora atual
	Relogio::DataHora dataHora = this->plataforma->relogio->getDataHora();
	//Envia data e hora
	interface->send((U8*)&dataHora, sizeof(dataHora));
	crc16.update((U8*)&dataHora, sizeof(dataHora));
	//Envia CRC
	U16 crcCalculado = crc16.getValue();
	interface->send((U8*)&crcCalculado, sizeof(crcCalculado));
}

void Protocolo::wrdatahora(trf::application::IComunicacao* interface)
{
	CRC16CCITT crc16;
	crc16.reset();
	U8 answer = SUCESSO_COMM;
	Relogio::DataHora dataHora;
	//L� data e hora
	if(interface->receive((U8*)&dataHora, sizeof(dataHora)) < sizeof(dataHora)) {
		//Envia resposta negativa
		answer = TIMEOUT_COMM;
		interface->send((U8 *)&answer, sizeof(answer));
		return;
	}
	//Calcula o CRC
	crc16.update((U8*)&dataHora, sizeof(dataHora));
	//L� o CRC
	U16 crcRecebido;
	if(interface->receive((U8*)&crcRecebido, sizeof(crcRecebido)) < sizeof(crcRecebido)) {
		//Envia resposta negativa
		answer = TIMEOUT_COMM;
		interface->send((U8 *)&answer, sizeof(answer));
		return;
	}
	//Verifica o CRC
	if(crcRecebido != crc16.getValue()){
		//Envia resposta negativa
		answer = ERRO_CRC_COMM;
		interface->send((U8 *)&answer, sizeof(answer));
		return;
	}
	//Atualiza data e hora
	this->plataforma->relogio->setDataHora(dataHora);
	//Envia resposta positiva
	interface->send((U8 *)&answer, sizeof(answer));
}

void Protocolo::fsformat(trf::application::IComunicacao* client)
{
	U8 answer = SUCESSO_COMM;

	//Formata o sistema de arquivo
	if(this->fachada->format() != Fachada::SUCESSO) {
		//Envia resposta negativa
		answer = Protocolotrf::FALHA_NA_FORMATACAO_COMM;
		client->send(&answer, 1);
		return;
	}
	//Envia resposta positiva
	client->send((U8 *)&answer, sizeof(answer));
}

void Protocolo::wrchunk(trf::application::IComunicacao* client)
{
	this->fachada->processConfigurandoIncrement();
	ProtocoloNandFFS::wrchunk(client);
}

void Protocolo::rde2promdump(trf::application::IComunicacao* interface)
{
	CRC16CCITT crc16;
	U8 answer = Protocolotrf::SUCESSO_COMM;
	// Pega a qtd de bytes a serem enviadas
	U32 qntBytes = this->plataforma->e2prom->getTotalSize();
	if(qntBytes > sizeof(this->buffer))
	{
		//Envia resposta negativa
		answer = Protocolotrf::PARAMETRO_INVALIDO_COMM;
		interface->send((U8 *)&answer, sizeof(answer));
		return;
	}
	//Reseta o CRC
	crc16.reset();

	//L� o conte�do desejado na mem�ria

	if(this->plataforma->e2prom->read(this->buffer,0,qntBytes) == qntBytes){
		// Envia resposta positiva
		answer = Protocolotrf::SUCESSO_COMM;
		interface->send((U8 *)&answer, sizeof(answer));
	}
	else{
		//Envia resposta negativa
		answer = Protocolotrf::ERRO_INESPERADO_COMM;
		interface->send((U8 *)&answer, sizeof(answer));
		return;
	}
	// Envia a qtd de bytes a serem lidos
	interface->send((U8*)&qntBytes,sizeof(qntBytes));
	//Envia os bytes do conte�do
	interface->send(this->buffer,qntBytes);

	// Calcula o CRC
	crc16.reset();
	crc16.update((U8*)&qntBytes,sizeof(qntBytes));
	crc16.update(this->buffer,qntBytes);

	//Envia o CRC
	U16 crcEnviado = crc16.getValue();
	interface->send((U8 *)&crcEnviado, sizeof(crcEnviado));
}

void Protocolo::rdroteiro(trf::application::IComunicacao* interface)
{
	CRC16CCITT crc16;
	crc16.reset();
	U8 answer[20];
	U8 answerLenght;
	U32 indiceRoteiro = this->fachada->getRoteiroSelecionado();
	U8 modo;
	//L� modo de leitura do roteiro
	if(interface->receive(&modo, sizeof(modo)) != sizeof(modo)) {
		//Envia resposta negativa
		answer[0] = answer[1] = 0xFF;
		answerLenght = 2;
		interface->send((U8 *)&answer, answerLenght);
		return;
	}
	if(modo == 0x00) {
		*((U16*)answer) = this->fachada->getIdRoteiro(indiceRoteiro);
		answerLenght = 2;
	}
	else if(modo == 0x01) {
		*((U32*)answer) = indiceRoteiro;
		answerLenght = 4;
	}
	else if(modo == 0x02) {
		this->fachada->getLabelNumeroRoteiro(indiceRoteiro, (char*)answer);
		answerLenght = 20;
	}
	else {
		//Envia resposta negativa
		answer[0] = answer[1] = 0xFF;
		interface->send((U8 *)&answer, 2);
		return;
	}
	//Envia roteiro selecionado
	interface->send((U8 *)&answer, answerLenght);
	crc16.update((U8 *)&answer, answerLenght);
	//Envia CRC
	U16 crcCalculado = crc16.getValue();
	interface->send((U8*)&crcCalculado, sizeof(crcCalculado));
}

void Protocolo::wrroteiro(trf::application::IComunicacao* interface)
{
	CRC16CCITT crc16;
	crc16.reset();
	U8 roteiro[20];
	U8 modo;
	U8 answer = SUCESSO_COMM;
	Fachada::Resultado resultado = Fachada::FALHA_OPERACAO;

	if(this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE)){
		//Envia resposta negativa
		answer = CONFIG_INCONSISTENTE_COMM;
		interface->send((U8 *)&answer, sizeof(answer));
		return;
	}

	//L� modo de escrita do roteiro
	if(interface->receive(&modo, sizeof(modo)) != sizeof(modo)) {
		//Envia resposta negativa
		answer = TIMEOUT_COMM;
		interface->send((U8 *)&answer, sizeof(answer));
		return;
	}
	//Atualiza o CRC
	crc16.update(&modo, sizeof(modo));
	//L� o par�metro de acordo com o modo
	if(modo == 0x00) {
		//L� o id do roteiro
		if(interface->receive(roteiro, 2) != 2){
			//Envia resposta negativa
			answer = TIMEOUT_COMM;
			interface->send((U8 *)&answer, sizeof(answer));
			return;
		}
		//Atualiza o CRC
		crc16.update(roteiro, 2);
	}
	else if(modo == 0x01) {
		//L� o id do roteiro
		if(interface->receive(roteiro, 4) != 4){
			//Envia resposta negativa
			answer = TIMEOUT_COMM;
			interface->send((U8 *)&answer, sizeof(answer));
			return;
		}
		//Atualiza o CRC
		crc16.update(roteiro, 4);
	}
	else if(modo == 0x02) {
		//L� o id do roteiro
		if(interface->receive(roteiro, 20) != 20){
			//Envia resposta negativa
			answer = TIMEOUT_COMM;
			interface->send((U8 *)&answer, sizeof(answer));
			return;
		}
		//Atualiza o CRC
		crc16.update(roteiro, 20);
	}
	else {
		//Envia resposta negativa
		answer = PARAMETRO_INVALIDO_COMM;
		interface->send((U8 *)&answer, sizeof(answer));
		return;
	}
	//L� o CRC
	U16 crcRecebido;
	if(interface->receive((U8*)&crcRecebido, sizeof(crcRecebido)) < sizeof(crcRecebido)) {
		//Envia resposta negativa
		answer = TIMEOUT_COMM;
		interface->send((U8 *)&answer, sizeof(answer));
		return;
	}
	//Verifica o CRC
	if(crcRecebido != crc16.getValue()){
		//Envia resposta negativa
		answer = ERRO_CRC_COMM;
		interface->send((U8 *)&answer, sizeof(answer));
		return;
	}

	//Altera o roteiro de acordo com o modo
	if(modo == 0x00) {
		resultado = this->fachada->setRoteiroSelecionado(*((U16*)roteiro));
	}
	else if(modo == 0x01) {
		resultado = this->fachada->setRoteiroSelecionado(*((U32*)roteiro));
	}
	else if(modo == 0x02) {
		resultado = this->fachada->setRoteiroSelecionado((char*)roteiro);
	}
	//Verifica o resultado da opera��o
	if(resultado == Fachada::FUNCAO_BLOQUEADA) {
		//Envia resposta negativa
		answer = FUNCAO_BLOQUEADA_COMM;
	}
	else if(resultado != Fachada::SUCESSO) {
		//Envia resposta negativa
		answer = FALHA_ALTERACAO_PARAMETRO_VARIAVEL_COMM;
	}
	//Envia resposta
	interface->send((U8 *)&answer, sizeof(answer));
}

void Protocolo::wrtarifa(trf::application::IComunicacao* interface)
{
	CRC16CCITT crc16;
	crc16.reset();
	U32 tarifaRecebida;
	U8 answer = SUCESSO_COMM;
	Fachada::Resultado resultado = Fachada::FALHA_OPERACAO;

	if(this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE)){
		//Envia resposta negativa
		answer = CONFIG_INCONSISTENTE_COMM;
		interface->send((U8 *)&answer, sizeof(answer));
		return;
	}

	//L� o valor da tarifa
	if(interface->receive((U8*)&tarifaRecebida, sizeof(tarifaRecebida)) != sizeof(tarifaRecebida)){
		//Envia resposta negativa
		answer = TIMEOUT_COMM;
		interface->send((U8 *)&answer, sizeof(answer));
		return;
	}
	//Atualiza o CRC
	crc16.update((U8*)&tarifaRecebida, sizeof(tarifaRecebida));
	//L� o CRC
	U16 crcRecebido;
	if(interface->receive((U8*)&crcRecebido, sizeof(crcRecebido)) < sizeof(crcRecebido)) {
		//Envia resposta negativa
		answer = TIMEOUT_COMM;
		interface->send((U8 *)&answer, sizeof(answer));
		return;
	}
	//Verifica o CRC
	if(crcRecebido != crc16.getValue()){
		//Envia resposta negativa
		answer = ERRO_CRC_COMM;
		interface->send((U8 *)&answer, sizeof(answer));
		return;
	}

	//Altera a tarifa
	resultado = this->fachada->setTarifaMemoria(tarifaRecebida);
	//Verifica o resultado da opera��o
	if(resultado != Fachada::SUCESSO) {
		//Envia resposta negativa
		answer = FALHA_ALTERACAO_PARAMETRO_VARIAVEL_COMM;
	}
	//Envia resposta
	interface->send((U8 *)&answer, sizeof(answer));
}

void Protocolo::rdmensagem(trf::application::IComunicacao* interface)
{
	CRC16CCITT crc16;
	crc16.reset();
	U32 answer;
	U8 painel;
	//L� o painel
	if(interface->receive(&painel, sizeof(painel)) != sizeof(painel)) {
		//Envia resposta negativa
		answer = TIMEOUT_COMM;
		interface->send((U8 *)&answer, sizeof(answer));
		return;
	}
	//L� �ndice da mensagem selecionada
	answer = this->fachada->getMensagemSelecionada(painel);
	//Envia roteiro selecionado
	interface->send((U8 *)&answer, sizeof(answer));
	crc16.update((U8 *)&answer, sizeof(answer));
	//Envia CRC
	U16 crcCalculado = crc16.getValue();
	interface->send((U8*)&crcCalculado, sizeof(crcCalculado));
}

void Protocolo::wrmensagem(trf::application::IComunicacao* interface)
{
	CRC16CCITT crc16;
	crc16.reset();
	U8 answer = SUCESSO_COMM;
	U8 painel;
	U32 indice;
	Fachada::Resultado resultado;

	if(this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE)){
		//Envia resposta negativa
		answer = CONFIG_INCONSISTENTE_COMM;
		interface->send((U8 *)&answer, sizeof(answer));
		return;
	}

	//L� o painel
	if(interface->receive(&painel, sizeof(painel)) != sizeof(painel)) {
		//Envia resposta negativa
		answer = TIMEOUT_COMM;
		interface->send((U8 *)&answer, sizeof(answer));
		return;
	}
	//Atualiza o CRC
	crc16.update(&painel, sizeof(painel));

	//L� �ndice da mensagem a ser selecionada
	if(interface->receive((U8*)&indice, sizeof(indice)) != sizeof(indice)) {
		//Envia resposta negativa
		answer = TIMEOUT_COMM;
		interface->send((U8 *)&answer, sizeof(answer));
		return;
	}
	//Atualiza o CRC
	crc16.update((U8*)&indice, sizeof(indice));
	//L� o CRC
	U16 crcRecebido;
	if(interface->receive((U8*)&crcRecebido, sizeof(crcRecebido)) < sizeof(crcRecebido)) {
		//Envia resposta negativa
		answer = TIMEOUT_COMM;
		interface->send((U8 *)&answer, sizeof(answer));
		return;
	}
	//Verifica o CRC
	if(crcRecebido != crc16.getValue()){
		//Envia resposta negativa
		answer = ERRO_CRC_COMM;
		interface->send((U8 *)&answer, sizeof(answer));
		return;
	}

	//Seta mensagem selecionada
	//TODO: a vari�vel 'painel' est� sendo lida mas n�o est� sendo usada
	resultado = this->fachada->setMensagemSelecionada(indice);

	if(resultado == Fachada::FUNCAO_BLOQUEADA) {
		//Envia resposta negativa
		answer = FUNCAO_BLOQUEADA_COMM;
	}
	else if(resultado != Fachada::SUCESSO) {
		//Envia resposta negativa
		answer = FALHA_ALTERACAO_PARAMETRO_VARIAVEL_COMM;
	}

	//Envia roteiro selecionado
	interface->send((U8 *)&answer, sizeof(answer));
}

void Protocolo::rdmsg2(trf::application::IComunicacao* interface)
{
	CRC16CCITT crc16;
	crc16.reset();
	U32 answer;
	U8 painel;
	//L� o painel
	if(interface->receive(&painel, sizeof(painel)) != sizeof(painel)) {
		//Envia resposta negativa
		answer = TIMEOUT_COMM;
		interface->send((U8 *)&answer, sizeof(answer));
		return;
	}
	//L� �ndice da mensagem selecionada
	answer = this->fachada->getMensagemSecundariaSelecionada(painel);
	//Envia roteiro selecionado
	interface->send((U8 *)&answer, sizeof(answer));
	crc16.update((U8 *)&answer, sizeof(answer));
	//Envia CRC
	U16 crcCalculado = crc16.getValue();
	interface->send((U8*)&crcCalculado, sizeof(crcCalculado));
}

void Protocolo::wrmsg2(trf::application::IComunicacao* interface)
{
	CRC16CCITT crc16;
	crc16.reset();
	U8 answer = SUCESSO_COMM;
	U8 painel;
	U32 indice;
	Fachada::Resultado resultado;

	if(this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE)){
		//Envia resposta negativa
		answer = CONFIG_INCONSISTENTE_COMM;
		interface->send((U8 *)&answer, sizeof(answer));
		return;
	}

	//L� o painel
	if(interface->receive(&painel, sizeof(painel)) != sizeof(painel)) {
		//Envia resposta negativa
		answer = TIMEOUT_COMM;
		interface->send((U8 *)&answer, sizeof(answer));
		return;
	}
	//Atualiza o CRC
	crc16.update(&painel, sizeof(painel));

	//L� �ndice da mensagem a ser selecionada
	if(interface->receive((U8*)&indice, sizeof(indice)) != sizeof(indice)) {
		//Envia resposta negativa
		answer = TIMEOUT_COMM;
		interface->send((U8 *)&answer, sizeof(answer));
		return;
	}
	//Atualiza o CRC
	crc16.update((U8*)&indice, sizeof(indice));
	//L� o CRC
	U16 crcRecebido;
	if(interface->receive((U8*)&crcRecebido, sizeof(crcRecebido)) < sizeof(crcRecebido)) {
		//Envia resposta negativa
		answer = TIMEOUT_COMM;
		interface->send((U8 *)&answer, sizeof(answer));
		return;
	}
	//Verifica o CRC
	if(crcRecebido != crc16.getValue()){
		//Envia resposta negativa
		answer = ERRO_CRC_COMM;
		interface->send((U8 *)&answer, sizeof(answer));
		return;
	}

	//Seta mensagem selecionada
	//TODO: a vari�vel 'painel' est� sendo lida mas n�o est� sendo usada
	resultado = this->fachada->setMensagemSecundariaSelecionada(indice);

	if(resultado == Fachada::FUNCAO_BLOQUEADA) {
		//Envia resposta negativa
		answer = FUNCAO_BLOQUEADA_COMM;
	}
	else if(resultado != Fachada::SUCESSO) {
		//Envia resposta negativa
		answer = FALHA_ALTERACAO_PARAMETRO_VARIAVEL_COMM;
	}

	//Envia roteiro selecionado
	interface->send((U8 *)&answer, sizeof(answer));
}

void Protocolo::rdpainel(trf::application::IComunicacao* interface)
{
	U8 painel = this->fachada->getPainelSelecionado();

	//Envia o painel selecionado
	interface->send((U8 *)&painel, sizeof(painel));
}

void Protocolo::wrpainel(trf::application::IComunicacao* interface)
{
	U8 answer = SUCESSO_COMM;
	U8 painel;
	U8 resultado;

	if(this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE)){
		//Envia resposta negativa
		answer = CONFIG_INCONSISTENTE_COMM;
		interface->send((U8 *)&answer, sizeof(answer));
		return;
	}

	//L� o painel
	if(interface->receive(&painel, sizeof(painel)) != sizeof(painel)) {
		//Envia resposta negativa
		answer = TIMEOUT_COMM;
		interface->send((U8 *)&answer, sizeof(answer));
		return;
	}

	//Verifica se o painel existe
	if(painel >= this->fachada->getQntPaineis()){
		//Envia resposta negativa
		answer = PARAMETRO_INVALIDO_COMM;
		interface->send((U8 *)&answer, sizeof(answer));
		return;
	}

	//Seta painel selecionado
	resultado = this->fachada->setPainelSelecionado(painel);

	if(resultado == Fachada::PARAMETRO_INVALIDO){
		//Envia resposta negativa
		answer = PARAMETRO_INVALIDO_COMM;
		interface->send((U8 *)&answer, sizeof(answer));
		return;
	}

	//Envia resposta positiva
	interface->send((U8 *)&answer, sizeof(answer));
}

void Protocolo::rdsentido(trf::application::IComunicacao* interface)
{
	U8 sentido = (U8)this->fachada->getSentidoRoteiro();

	//Envia o sentido selecionado
	interface->send((U8 *)&sentido, sizeof(sentido));
}

void Protocolo::wrsentido(trf::application::IComunicacao* interface)
{
	U8 answer = SUCESSO_COMM;
	U8 sentido;
	Fachada::Resultado resultado;

	if(this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE)){
		//Envia resposta negativa
		answer = CONFIG_INCONSISTENTE_COMM;
		interface->send((U8 *)&answer, sizeof(answer));
		return;
	}

	//L� o sentido
	if(interface->receive(&sentido, sizeof(sentido)) != sizeof(sentido)) {
		//Envia resposta negativa
		answer = TIMEOUT_COMM;
		interface->send((U8 *)&answer, sizeof(answer));
		return;
	}

	//Seta sentido selecionado
	resultado = this->fachada->setSentidoRoteiro((Fachada::SentidoRoteiro)sentido);

	if(resultado == Fachada::FUNCAO_BLOQUEADA) {
		//Envia resposta negativa
		answer = FUNCAO_BLOQUEADA_COMM;
	}
	else if(resultado == Fachada::FALHA_OPERACAO) {
		//Envia resposta negativa
		answer = FALHA_ALTERACAO_PARAMETRO_VARIAVEL_COMM;
	}

	//Envia resposta
	interface->send((U8 *)&answer, sizeof(answer));
}

void Protocolo::rdqntpaineis(trf::application::IComunicacao* interface)
{
	U32 qntPaineis = this->fachada->getQntPaineis();

	//Envia quantidade de paineis
	interface->send((U8 *)&qntPaineis, sizeof(qntPaineis));
}

void Protocolo::rdqntrots(trf::application::IComunicacao* interface)
{
	U32 qntRoteiros = this->fachada->getQntRoteiros();

	//Envia quantidade de roteiros
	interface->send((U8 *)&qntRoteiros, sizeof(qntRoteiros));
}

void Protocolo::rdqntmsgs(trf::application::IComunicacao* interface)
{
	U32 qntMensagens = this->fachada->getQntMensagens();

	//Envia quantidade de mensagens
	interface->send((U8 *)&qntMensagens, sizeof(qntMensagens));
}

void Protocolo::rddimensoes(trf::application::IComunicacao* interface)
{
	U32 altura, largura;
	U8 painel;

	//L� o painel
	if(interface->receive(&painel, sizeof(painel)) != sizeof(painel)) {
		//Envia resposta negativa
		altura = largura = 0xFFFFFFFF;
		interface->send((U8 *)&altura, sizeof(altura));
		interface->send((U8 *)&largura, sizeof(largura));
		return;
	}

	//L� dimens�es do painel
	altura = this->fachada->getAlturaPainel(painel);
	largura = this->fachada->getLarguraPainel(painel);

	//Envia dimens�es do painel
	interface->send((U8 *)&altura, sizeof(altura));
	interface->send((U8 *)&largura, sizeof(largura));
}

void Protocolo::rdhorasaida(trf::application::IComunicacao* interface)
{
	U8 horaSaida, minutosSaida;
	this->fachada->getHoraSaida(&horaSaida, &minutosSaida);

	//Envia hor�rio de sa�da
	interface->send((U8 *)&horaSaida, sizeof(horaSaida));
	interface->send((U8 *)&minutosSaida, sizeof(minutosSaida));
}

void Protocolo::wrhorasaida(trf::application::IComunicacao* interface)
{
	U8 answer = SUCESSO_COMM;
	U8 horaSaida, minutosSaida;
	Fachada::Resultado resultado;

	if(this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE)){
		//Envia resposta negativa
		answer = CONFIG_INCONSISTENTE_COMM;
		interface->send((U8 *)&answer, sizeof(answer));
		return;
	}

	//L� a hora de sa�da
	if(interface->receive(&horaSaida, sizeof(horaSaida)) != sizeof(horaSaida)) {
		//Envia resposta negativa
		answer = TIMEOUT_COMM;
		interface->send((U8 *)&answer, sizeof(answer));
		return;
	}
	//L� os minutos de sa�da
	if(interface->receive(&minutosSaida, sizeof(minutosSaida)) != sizeof(minutosSaida)) {
		//Envia resposta negativa
		answer = TIMEOUT_COMM;
		interface->send((U8 *)&answer, sizeof(answer));
		return;
	}

	//Seta hor�rio de sa�da
	resultado = this->fachada->setHoraSaida(horaSaida, minutosSaida);

	if(resultado == Fachada::FUNCAO_BLOQUEADA) {
		//Envia resposta negativa
		answer = FUNCAO_BLOQUEADA_COMM;
	}
	else if(resultado == Fachada::FALHA_OPERACAO) {
		//Envia resposta negativa
		answer = FALHA_ALTERACAO_PARAMETRO_VARIAVEL_COMM;
	}

	//Envia resposta
	interface->send((U8 *)&answer, sizeof(answer));
}

void Protocolo::erasepw(trf::application::IComunicacao* interface)
{
	U8 answer = SUCESSO_COMM;
	if(this->fachada->isSenhaAntiFurtoHabilitada())
	{
		this->fachada->zerarSenhaAntiFurto();
	}
	//Envia resposta
	interface->send(&answer,sizeof(answer));
}

void Protocolo::initconfig(trf::application::IComunicacao* interface)
{
	U8 answer = SUCESSO_COMM;
	this->fachada->iniciarConfiguracao();
	//Envia resposta
	interface->send(&answer,sizeof(answer));
}

void Protocolo::apagartrava(trf::application::IComunicacao* interface)
{
	U8 answer = SUCESSO_COMM;
	this->fachada->apagarTrava();
	//Envia resposta
	interface->send(&answer,sizeof(answer));
}

void Protocolo::apagarpaineis(trf::application::IComunicacao* interface)
{
	U8 answer = SUCESSO_COMM;
	//L� a quantidade de pain�is na rede
	U32 qntPaineis = this->fachada->getQntPaineis();
	//Apaga todos os pain�is da rede
	for (U32 i = 0; i < qntPaineis; ++i)
	{
		this->fachada->apagarPainel(i, 0xFFFF);
	}
	//Envia resposta
	interface->send(&answer,sizeof(answer));
}

void Protocolo::receberInformacaoCatraca(trf::application::IComunicacao* interface)
{
	U8 tamanhoNumero = 0;
	U8 tamanhoTarifa = 0;
	U8 sentidoRota = 0;
	U32 valorTarifa = 0;

	//array � preenchido com valor padrao
	for (U8 i = 0; i < 16; i++) {
		this->buffer[i] = '\0';
	}

	////////////////////////////////////////////////////////
	// RECEBE O NUMERO DO ROTEIRO
	////////////////////////////////////////////////////////
	//L� a quantidade de bytes do numero
	if(!interface->receive(&tamanhoNumero, sizeof(tamanhoNumero)) || (tamanhoNumero > 16))
	{
		return;
	}

	//L� o numero do painel em forma de string
	if(interface->receive(this->buffer, tamanhoNumero) != tamanhoNumero)
	{
		return;
	}

	//Procura o indice do roteiro
	this->fachada->setRoteiroSelecionado((char*)this->buffer, true);

	////////////////////////////////////////////////////////
	// RECEBE O SENTIDO DO ROTEIRO
	////////////////////////////////////////////////////////

	//L� o numero do painel em forma de string
	if(!interface->receive(&sentidoRota, sizeof(sentidoRota)))
	{
		return;
	}

	Fachada::SentidoRoteiro sentido = (sentidoRota == 'V') ? Fachada::VOLTA : Fachada::IDA;
	this->fachada->setSentidoRoteiro(sentido, true);

	////////////////////////////////////////////////////////
	// RECEBE O VALOR DA TARIFA
	////////////////////////////////////////////////////////

	//L� a quantidade de bytes da tarifa
	if(!interface->receive(&tamanhoTarifa, sizeof(tamanhoTarifa)))
	{
		return;
	}

	//L� o valor da tarifa
	if(interface->receive(this->buffer, tamanhoTarifa) != tamanhoTarifa)
	{
		return;
	}

	//Checa se todos os caracteres s�o n�meros
	for (U8 i = 0; i < tamanhoTarifa; ++i) {
		if(this->buffer[i] < '0' || this->buffer[i] > '9')
		{
			return;
		}
	}

	//Passa o valor da tarifa de ASCII para num�rico
	for(U8 i = 0; i < tamanhoTarifa; i++){
		valorTarifa = valorTarifa * 10;
		valorTarifa = valorTarifa + (this->buffer[i] - '0');
	}

	//Altera a tarifa
	this->fachada->setTarifaMemoria(valorTarifa);
}

void Protocolo::rdroteiro11(trf::application::IComunicacao* interface)
{
	CRC16CCITT crc16;
	crc16.reset();
	U8 answer[20];
	U8 answerLenght;
	U32 indiceRoteiro = this->fachada->getRoteiroSelecionado();
	this->fachada->getLabelNumeroRoteiro(indiceRoteiro, (char*)answer);
	answerLenght = 16;
	//Envia roteiro selecionado
	interface->send((U8 *)&answer, answerLenght);
	crc16.update((U8 *)&answer, answerLenght);
	//Envia CRC
	U16 crcCalculado = crc16.getValue();
	interface->send((U8*)&crcCalculado, sizeof(crcCalculado));
}

void Protocolo::task()
{
	//Inicia a conex�o
	this->comunicacao->start();

	while(true){
		//Aguarda uma conex�o
		this->comunicacao->waitConnection();
		//L� os comandos enquanto o cliente estiver conectado
		while(this->comunicacao->isConnected()){
			char* command = (char *)this->buffer;
			//L� um comando
			this->readCommand(this->comunicacao, command);
			//Decodifica comando
			if(String::equals(command, "RESET")){
				this->reset(this->comunicacao);
			}
			else if(String::equals(command, "RDPRODUTO")){
				this->rdproduto(this->comunicacao);
			}
			else if(String::equals(command, "RDVERSAO")){
				this->rdversao(this->comunicacao);
			}
			else if(String::equals(command, "RDCRCFIRMWAR")){
				this->rdCRCFirmware(this->comunicacao);
			}
			else if(String::equals(command, "RDNUMSERIE")){
				this->rdnumserie(this->comunicacao);
			}
			else if(String::equals(command, "WRNUMSERIE")){
				this->wrnumserie(this->comunicacao);
			}
			else if(String::equals(command, "RDSTATUS")){
				this->rdstatus(this->comunicacao);
			}
			else if(String::equals(command, "FACTORY")){
				this->factory(this->comunicacao);
			}
			else if(String::equals(command, "FOPEN")){
				this->fopen(this->comunicacao);
			}
			else if(String::equals(command, "FCLOSE")){
				this->fclose(this->comunicacao);
			}
			else if(String::equals(command, "FREAD")){
				this->fread(this->comunicacao);
			}
			else if(String::equals(command, "FWRITE")){
				this->fwrite(this->comunicacao);
			}
			else if(String::equals(command, "FSEEK")){
				this->fseek(this->comunicacao);
			}
			else if(String::equals(command, "FSFORMAT")){
				this->fsformat(this->comunicacao);
			}
			else if(String::equals(command, "WRCHUNK")){
				this->wrchunk(this->comunicacao);
			}
			else if(String::equals(command, "RDCHUNK")){
				this->rdchunk(this->comunicacao);
			}
			else if(String::equals(command, "RDNUMCHUNKS")){
				this->rdnumchunks(this->comunicacao);
			}
			else if(String::equals(command, "RDDATAHORA")){
				this->rddatahora(this->comunicacao);
			}
			else if(String::equals(command, "WRDATAHORA")){
				this->wrdatahora(this->comunicacao);
			}
			else if(String::equals(command, "RDE2PROMDMP")){
				this->rde2promdump(this->comunicacao);
			}
			else if(String::equals(command, "RDROTEIRO")){
				this->rdroteiro(this->comunicacao);
			}
			else if(String::equals(command, "WRROTEIRO")){
				this->wrroteiro(this->comunicacao);
			}
			else if(String::equals(command, "WRTARIFA")){
				this->wrtarifa(this->comunicacao);
			}
			else if(String::equals(command, "RDMENSAGEM")){
				this->rdmensagem(this->comunicacao);
			}
			else if(String::equals(command, "WRMENSAGEM")){
				this->wrmensagem(this->comunicacao);
			}
			else if(String::equals(command, "RDMSG2")){
				this->rdmsg2(this->comunicacao);
			}
			else if(String::equals(command, "WRMSG2")){
				this->wrmsg2(this->comunicacao);
			}
			else if(String::equals(command, "RDPAINEL")){
				this->rdpainel(this->comunicacao);
			}
			else if(String::equals(command, "WRPAINEL")){
				this->wrpainel(this->comunicacao);
			}
			else if(String::equals(command, "RDSENTIDO")){
				this->rdsentido(this->comunicacao);
			}
			else if(String::equals(command, "WRSENTIDO")){
				this->wrsentido(this->comunicacao);
			}
			else if(String::equals(command, "RDQNTPAINEIS")){
				this->rdqntpaineis(this->comunicacao);
			}
			else if(String::equals(command, "RDQNTROTS")){
				this->rdqntrots(this->comunicacao);
			}
			else if(String::equals(command, "RDQNTMSGS")){
				this->rdqntmsgs(this->comunicacao);
			}
			else if(String::equals(command, "RDDIMENSOES")){
				this->rddimensoes(this->comunicacao);
			}
			else if(String::equals(command, "RDHORASAIDA")){
				this->rdhorasaida(this->comunicacao);
			}
			else if(String::equals(command, "WRHORASAIDA")){
				this->wrhorasaida(this->comunicacao);
			}
			else if (String::equals(command, "ERASEPW")){
				this->erasepw(this->comunicacao);
			}
			else if (String::equals(command, "INITCONFIG")){
				this->initconfig(this->comunicacao);
			}
			else if (String::equals(command, "APAGARTRAVA")){
				this->apagartrava(this->comunicacao);
			}
			else if (String::equals(command, "STANDBY")){
				this->apagarpaineis(this->comunicacao);
			}
			//Commandos do protocolo antigo
			else if (String::equals(command, "??")){
				this->rdproduto(this->comunicacao);
			}
			else if (String::equals(command, "VV")){
				this->receberInformacaoCatraca(this->comunicacao);
			}
			else if (String::equals(command, "ii")){
				this->rdnumserie(this->comunicacao);
			}
			else if (String::equals(command, "rr")){
				this->rdroteiro11(this->comunicacao);
			}
			//Delay
			vTaskDelay(5);
		}
	}
}

}
}
}
