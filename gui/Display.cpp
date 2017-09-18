/*
 * Display.cpp
 *
 *  Created on: 31/03/2011
 *      Author: bruno.silva
 */

#include <config.h>
#include "Display.h"
#include <escort.util/Math.h>

using namespace escort::util;


namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

Display::Display(escort::driver::ILCDDisplay* LCD) :
		LCD(LCD)
{}

void Display::open()
{
	//Abre o driver do LCD
	this->LCD->open();
}

void Display::close()
{
	//Fecha o driver do LCD
	this->LCD->close();
}

void Display::init()
{
	//Cria o sem�foro
	vSemaphoreCreateBinary(this->semaphore);

	this->numeroColunas = this->LCD->getWidth();
	this->numeroLinhas = this->LCD->getHeight();

	this->clearScreen();
	this->open();
	this->print(trf_pontos12_application_Controlador_displayStartupMessage);
	this->close();
}

void Display::clearScreen()
{
	this->LCD->clear();
	this->linhaCursor = 0;
	this->colunaCursor = 0;
}

U8 Display::getWidth()
{
	return this->LCD->getWidth();
}

U8 Display::getHeight()
{
	return this->LCD->getHeight();
}

void Display::setLinhaColuna(U8 coluna, U8 linha)
{
	this->LCD->setCursorPosition(coluna, linha);
	this->colunaCursor = coluna;
	this->linhaCursor = linha;
}

void Display::getDisplayStatus(bool* blink, bool* cursor, bool* display)
{
	this->LCD->getDisplayStatus(blink, cursor, display);
}

void Display::setDisplayStatus(bool blink, bool cursor, bool display)
{
	this->LCD->setDisplayStatus(blink, cursor, display);
}

void Display::turnOffDisplay()
{
	this->LCD->turnOff();
}

U8 Display::getLinhaCursor() const
{
	return this->linhaCursor;
}

U8 Display::getColunaCursor() const
{
	return this->colunaCursor;
}

void Display::print(char character)
{
	this->print(&character, 1);
}

void Display::print(char* string)
{
    while(*string)
    {
    	this->print(string++, 1);
    }
}

void Display::print(char* string, U8 nBytes)
{
	while(nBytes > 0)
	{
		//Calcula o espa�o livre na linha
		U8 posicoesLivresColuna = numeroColunas - colunaCursor;

		//Calcula a quantidade de caracteres a serem escritos na linha
		U8 quantidadeBytesLinha = Math::min(posicoesLivresColuna, nBytes);

		//Escreve os caracteres no Display
		this->LCD->write((U8*)string, quantidadeBytesLinha);
		string += quantidadeBytesLinha;

		//Atualiza a posi��o do cursor
		this->colunaCursor += quantidadeBytesLinha;
		if(this->colunaCursor == this->numeroColunas)
		{
			this->linhaCursor = (this->linhaCursor + 1) % this->numeroLinhas;
			this->colunaCursor = 0;

			this->setLinhaColuna(this->colunaCursor, this->linhaCursor);
		}

		//Atualiza o numero de caracteres a serem escritos
		nBytes -= quantidadeBytesLinha;
	}

}

void Display::print(char* string, U8 nBytes, U8 coluna, U8 linha)
{
	this->setLinhaColuna(coluna, linha);
	this->print(string, nBytes);
}

void Display::print(U32 numero)
{
	this->print(numero, Converter::BASE_10);
}

void Display::print(U32 numero, Converter::NumericalBase radix)
{
	Converter::itoa(numero, buffer, radix);
	print(buffer);
}

void Display::print(U32 numero, U8 nCaracteres)
{
	this->print(numero, nCaracteres, Converter::BASE_10);
}

void Display::print(U32 numero, U8 nCaracteres, Converter::NumericalBase radix)
{
	Converter::itoa(numero, buffer, nCaracteres, radix);
	print(buffer, nCaracteres);
}

void Display::print(U32 numero, U8 coluna, U8 linha, U8 nCaracteres)
{
	this->print(numero, coluna, linha, nCaracteres, Converter::BASE_10);
}

void Display::print(U32 numero, U8 coluna, U8 linha, U8 nCaracteres, Converter::NumericalBase radix)
{
	this->setLinhaColuna(coluna, linha);
	this->print(numero, nCaracteres, radix);
}

void Display::println()
{
	U8 linha = Math::min(this->linhaCursor + 1, this->getHeight() - 1);
	this->setLinhaColuna(0, linha);
}

}
}
}
}
