/*
 * TelaConfiguracoes.cpp
 *
 *  Created on: 30/05/2012
 *      Author: Gustavo
 */

#include "TelaConfiguracoes.h"
#include <escort.util/Converter.h>
#include <escort.util/String.h>
#include "Resources.h"

using escort::util::Converter;
using escort::util::String;


#define TEMPO_PISCAGEM_CURSOR \
	500


namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

TelaConfiguracoes::TelaConfiguracoes(Fachada* fachada, InputOutput* inputOutput) :
		Tela(inputOutput),
		fachada(fachada)
{}

void TelaConfiguracoes::paint()
{
	this->inputOutput->display.setLinhaColuna(0,0);

	switch (this->indice) {
		case VERSAO:
			{
				//Obtem a vers�o do firmware
				trf::application::Version* versao = this->fachada->getVersaoFirmware();

				//Monta a string referente � vers�o
				U8 length = 0;
				Converter::itoa(versao->familia, this->temp + length);
				length = String::getLength(this->temp);
				String::strcat(this->temp, ".");
				length++;
				Converter::itoa(versao->major, this->temp + length);
				length = String::getLength(this->temp);
				String::strcat(this->temp, ".");
				length++;
				Converter::itoa(versao->minor, this->temp + length);
				length = String::getLength(this->temp);

				//Imprime a vers�o do firmware
				this->inputOutput->display.print(Resources::getTexto(Resources::VERSAO));
				this->inputOutput->display.println();
				this->inputOutput->display.print(this->temp, length);
				for(U8 i = 0; i < this->inputOutput->display.getWidth() - length - 4; ++i){
					this->inputOutput->display.print(" ");
				}
				//Imprime o CRC do firmware
				U16 crcFW = this->fachada->getCRCFirmware();
				Converter::itoa(crcFW, this->temp, 0, 4, Converter::BASE_16);
				this->inputOutput->display.print(this->temp, 4);
			}
			break;
		case QNT_ROTEIROS:
			{
				//Obtem a quantidade de roteiros
				U32 qntRoteiros = this->fachada->getQntRoteiros();
				//Imprime a quantidade de roteiros
				this->inputOutput->display.print(Resources::getTexto(Resources::ROTEIROS));
				this->inputOutput->display.println();
				if(this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE))
				{
					this->inputOutput->display.print("N/A             ");
				}
				else{
					this->inputOutput->display.print(qntRoteiros, 5);
					this->inputOutput->display.print("           ");
				}
			}
			break;
		case QNT_MENSAGENS:
			{
				//Obtem a quantidade de mensagens
				U32 qntMensagens = this->fachada->getQntMensagens();
				//Imprime a quantidade de mensagens
				this->inputOutput->display.print(Resources::getTexto(Resources::MENSAGENS));
				this->inputOutput->display.println();
				if(this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE))
				{
					this->inputOutput->display.print("N/A             ");
				}
				else{
					this->inputOutput->display.print(qntMensagens, 5);
					this->inputOutput->display.print("           ");
				}
			}
			break;
		case QNT_PAINEIS:
			{
				//Obtem a quantidade de paineis
				U32 qntPaineis = this->fachada->getQntPaineis();
				//Imprime a quantidade de paineis
				this->inputOutput->display.print(Resources::getTexto(Resources::PAINEIS));
				this->inputOutput->display.println();
				if(this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE))
				{
					this->inputOutput->display.print("N/A             ");
				}
				else{
					this->inputOutput->display.print(qntPaineis, 2);
					this->inputOutput->display.print("              ");
				}
			}
			break;
		case NUMERO_SERIE:
			{
				trf::application::trfProduct::SerialNumber serialNumber = this->fachada->getSerialNumber();
				//Imprime o m�mero de s�rie
				this->inputOutput->display.print(Resources::getTexto(Resources::NUMERO_SERIE));
				this->inputOutput->display.println();
				this->inputOutput->display.print(serialNumber.manufacturingYear(), 2);
				this->inputOutput->display.print(serialNumber.manufacturingMonth(), 1, escort::util::Converter::BASE_16);
				this->inputOutput->display.print((serialNumber.productFamily() >> 4) & 0xF, 1);
					this->inputOutput->display.print((serialNumber.productFamily() >> 0) & 0xF, 1);
				this->inputOutput->display.print((serialNumber.productType() >> 8) & 0xF, 1);
					this->inputOutput->display.print((serialNumber.productType() >> 4) & 0xF, 1);
					this->inputOutput->display.print((serialNumber.productType() >> 0) & 0xF, 1);
				this->inputOutput->display.print((serialNumber.serialIndex() >> 16) & 0xF, 1);
					this->inputOutput->display.print((serialNumber.serialIndex() >> 12) & 0xF, 1);
					this->inputOutput->display.print((serialNumber.serialIndex() >> 8) & 0xF, 1);
					this->inputOutput->display.print((serialNumber.serialIndex() >> 4) & 0xF, 1);
					this->inputOutput->display.print((serialNumber.serialIndex() >> 0) & 0xF, 1);
				this->inputOutput->display.print("   ");
			}
			break;
		case PAINEL:
			{
				portTickType idleTime = xTaskGetTickCount() % 6000;

				//Imprime as informa��es do painel
				this->inputOutput->display.print(Resources::getTexto(Resources::PAINEL));
				this->inputOutput->display.setLinhaColuna(14, 0);
				this->inputOutput->display.print(this->painelSelecionado + 1, 2);
				this->inputOutput->display.println();
				//Verifica se houve algum erro de funcionamento (inicializa��o)
				if(this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE))
				{
					this->inputOutput->display.print("N/A             ");
				}
				else
				{
					U8 length = 0;
					if(idleTime < 3000)
					{
						this->inputOutput->display.print(this->fachada->getAlturaPainel(this->painelSelecionado));
						length += Converter::getNumberOfDigits(this->fachada->getAlturaPainel(this->painelSelecionado), Converter::BASE_10);
						this->inputOutput->display.print("x");
						length++;
						this->inputOutput->display.print(this->fachada->getLarguraPainel(this->painelSelecionado));
						length += Converter::getNumberOfDigits(this->fachada->getLarguraPainel(this->painelSelecionado), Converter::BASE_10);
						for(U8 i = 0; i < this->inputOutput->display.getWidth() - length; ++i){
							this->inputOutput->display.print(" ");
						}
					}
					else
					{
						//L� as informa��es de vers�o do Painel
						trf::application::Version version;
						if(this->fachada->readVersaoPainel(this->painelSelecionado, (U8*)&version.familia)){
							this->inputOutput->display.print("v");
							length++;
							this->inputOutput->display.print((U32)version.familia);
							length += Converter::getNumberOfDigits(version.familia, Converter::BASE_10);
							this->inputOutput->display.print(".");
							length++;
							this->inputOutput->display.print((U32)version.major);
							length += Converter::getNumberOfDigits(version.major, Converter::BASE_10);
							this->inputOutput->display.print(".");
							length++;
							this->inputOutput->display.print((U32)version.minor);
							length += Converter::getNumberOfDigits(version.minor, Converter::BASE_10);
						}
						for(U8 i = 0; i < this->inputOutput->display.getWidth() - length - 4; ++i){
							this->inputOutput->display.print(" ");
						}
						U16 crcFW = 0;
						this->fachada->readCRCFirmwarePainel(this->painelSelecionado, &crcFW);
						Converter::itoa(crcFW, this->temp, 0, 4, Converter::BASE_16);
						this->inputOutput->display.print(this->temp, 4);
					}
				}
			}
			break;
		default:
			this->indice = VERSAO;
			break;
	}
}

void TelaConfiguracoes::keyEvent(Teclado::Tecla tecla)
{
	//Verifica se a tecla "AJUSTE ESQUERDA"
	if(tecla & Teclado::TECLA_AJUSTE_ESQUERDA)
	{
		//Decrementa o �ndice (respeitando as condi��es determinadas)
		if(this->indice == VERSAO){
			this->indice = PAINEL;
			this->painelSelecionado = this->fachada->getQntPaineis() - 1;
		}
		else if(this->indice == PAINEL){
			if(this->painelSelecionado == 0){
				this->indice = (Info)(this->indice - 1);
			}
			else{
				this->painelSelecionado--;
			}
		}
		else {
			this->indice = (Info)(this->indice - 1);
		}

		//Verifica se a informa��o sobre o �ndice est� dspon�vel
		if(this->indice == PAINEL
			&& (this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE)))
		{
			this->indice = (Info)(this->indice - 1);
		}
	}
	//Verifica se a tecla "AJUSTE DIREITA"
	if(tecla & Teclado::TECLA_AJUSTE_DIREITA)
	{
		if(this->indice == PAINEL){
			if(this->painelSelecionado == (this->fachada->getQntPaineis() - 1)){
				this->indice = VERSAO;
			}
			else{
				this->painelSelecionado++;
			}
		}
		else {
			if(this->indice == NUMERO_SERIE){
				this->painelSelecionado = 0;
			}
			this->indice = (Info)(this->indice + 1);
		}

		//Verifica se a informa��o sobre o �ndice est� dspon�vel
		if(this->indice == PAINEL
			&& (this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE)))
		{
			this->indice = VERSAO;
		}
	}
	//Verifica se a tecla "OK"
	if(tecla & Teclado::TECLA_OK)
	{
		this->finish();
		this->visible = false;
	}
}

void TelaConfiguracoes::start()
{
	//Limpa a tela
	this->inputOutput->display.clearScreen();
	//Altera o timeout da tecla para 15 segundos
	this->teclaTime = 15000;
	//Come�a com a informa��o de vers�o do firmware
	this->indice = VERSAO;
}

void TelaConfiguracoes::finish()
{
}

}
}
}
}
