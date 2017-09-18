/*
 * ProtocoloCl.cpp
 *
 *  Created on: 22/05/2017
 *      Author: rennason.silva
 */

#include <escort.util/Converter.h>
#include <trf.pontos12.application/Controlador/ProtocoloCl.h>

using escort::util::Converter;

namespace trf {
namespace pontos12 {
namespace application {


ProtocoloCl::ProtocoloCl(Plataforma* plataforma) :
		plataforma(plataforma)
{}

void ProtocoloCl::init()
{
	this->semaforoCl = xSemaphoreCreateMutex();
	this->timerCl = xTaskGetTickCount();
}

void ProtocoloCl::receiveRoute(RoteiroComp *roteiroComp)
{
	//roteiro[0] = '\0';

	roteiroComp->roteiro = 0xFFFF;

	if((this->plataforma->rs485->isRxEmpty() == false) &&
			this->plataforma->rs485->read((U8*)&this->dado, sizeof(this->dado)) == sizeof(this->dado))
	{
		if(this->dado.beginMark == 0xFF &&
				this->dado.address == 0x00 &&
				//this->dado.address == 0xFE &&
				this->dado.destChMark == 0xF5 &&
				this->dado.extraChMark == 0xFA)
		{
			U8 checkSum = this->dado.address + this->dado.destChMark +this->dado.numDest[0] + this->dado.numDest[1] + this->dado.extraChMark + this->dado.extraNumber[0] +
					this->dado.extraNumber[1];

			if((checkSum != 0xFF) && (checkSum != this->dado.checkSum))
			{
				//Falha de checksum
				return;
			}
			else if(checkSum == 0xFF)
			{
				//L� mais um byte (End Mark)
				this->plataforma->rs485->read((U8*)&this->dado.beginMark,1);
				//Verifica CRC de 2 bytes
				if(this->dado.checkSum != 0xFE || this->dado.endMark != 0x01)
				{
					//Falha de checksum
					return;
				}
			}

			//U16 rot = (this->dado.numDest[0] << 8) + this->dado.numDest[1];

			roteiroComp->roteiro = ((this->dado.numDest[0] << 8) + this->dado.numDest[1]) & 0x03FF;
			roteiroComp->volta = (this->dado.numDest[0] & 0x04) != 0; //consideramos volta=1 para volta e volta=0 para ida


//			rot += ((this->dado.extraNumber[0] << 8) + this->dado.extraNumber[1])*1000;


			//Converter::itoa(rot, roteiro, 3, Converter::BASE_10);
		}
	}
}

void ProtocoloCl::sendRoute(RoteiroComp *roteiroCompEnvio)
{
	//U32 roteiroInt = Converter::atoi(roteiroComp->roteiro);
	//U16 roteiroInt = roteiroCompEnvio->roteiro;

	this->dado.beginMark = 0xFF;
	this->dado.address = 0xFE;
	//this->dado.address = 0x00;
	this->dado.destChMark = 0xF5;

	/*this->dado.numDest[0] = (U8) (((roteiroInt % 1000) & 0xFF00) >> 8);
	this->dado.numDest[1] = (U8) ((roteiroInt % 1000) & 0x00FF);*/

	this->dado.numDest[0] = (U8) ((roteiroCompEnvio->roteiro & 0x0300) >> 8); // 0x01
	if(roteiroCompEnvio->volta)
	{
		this->dado.numDest[0] += 0x04;
	}
	this->dado.numDest[1] = (U8) ((roteiroCompEnvio->roteiro) & 0x00FF); // 0x0B

	this->dado.extraChMark = 0xFA;
	this->dado.extraNumber[0] = 0x00;//n�o utilizados(padr�o mobitec)
	this->dado.extraNumber[1] = 0x00;//n�o utilizados(padr�o mobitec)
	this->dado.checkSum = this->dado.address + this->dado.destChMark + this->dado.numDest[0] +
			this->dado.numDest[1] + this->dado.extraChMark + this->dado.extraNumber[0] + this->dado.extraNumber[1];
	this->dado.endMark = 0xFF;

	this->plataforma->rs485->setMode(this->plataforma->rs485->WRITE_MODE);
	this->plataforma->rs485->write((U8*)&this->dado,sizeof(this->dado));
	this->plataforma->rs485->setMode(this->plataforma->rs485->READ_MODE);

}

U32 ProtocoloCl::getTimerCl()//getTimerCl retorna o delta entre o tempo atual e o anterior
{
	U32 deltaTime = 0;
	portTickType currentTime = xTaskGetTickCount();

	xSemaphoreTake(this->semaforoCl, portMAX_DELAY);
	if(currentTime > this->timerCl)
	{
		deltaTime = currentTime - this->timerCl;//caso n�o ocorra overflow no currentTime
	}
	else
	{
		deltaTime = portMAX_DELAY - this->timerCl + currentTime; //caso ocorra overflow no currentTime
	}
	xSemaphoreGive(this->semaforoCl);

	return deltaTime;
}

void ProtocoloCl::updateTimerCl()
{
	xSemaphoreTake(this->semaforoCl, portMAX_DELAY);
	this->timerCl = xTaskGetTickCount();
	xSemaphoreGive(this->semaforoCl);
}

} /* namespace application */
} /* namespace pontos12 */
} /* namespace trf */
