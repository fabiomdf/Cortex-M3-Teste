/*
 * Display.h
 *
 *  Created on: 31/03/2011
 *      Author: bruno.silva
 */

#ifndef GUI_DISPLAY_H_
#define GUI_DISPLAY_H_

/*
 * author bruno.silva
 *
 * To change this generated comment edit the template variable "comment":
 * Window > Preferences > C/C++ > Editor > Templates.
 */

//************************************************
// Includes
//************************************************
#include <Compiler.h>
#include <freertos.kernel/FreeRTOSKernel/FreeRTOSKernel.h>
#include <escort.util/Converter.h>
#include <escort.driver/ILCDDisplay/ILCDDisplay.h>

//************************************************
// Namespace
//************************************************
namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

//************************************************
// Class
//************************************************
class Display {
public:
	Display(escort::driver::ILCDDisplay* LCD);
	void init();
	void setLinhaColuna(U8 coluna, U8 linha);
	void getDisplayStatus(bool* blink, bool* cursor, bool* display);
	void setDisplayStatus(bool blink, bool cursor, bool display);
	void turnOffDisplay();
	U8 getLinhaCursor() const;
	U8 getColunaCursor() const;

	void open();
	void close();

	U8 getWidth();
	U8 getHeight();
	void clearScreen();
	void print(char character);
	void print(char* string);
	void print(char* string, U8 nBytes);
	void print(char* string, U8 nBytes, U8 coluna, U8 linha);
	void print(U32 numero);
	void print(U32 numero, escort::util::Converter::NumericalBase radix);
	void print(U32 numero, U8 nCaracteres);
	void print(U32 numero, U8 nCaracteres, escort::util::Converter::NumericalBase radix);
	void print(U32 numero, U8 coluna, U8 linha, U8 nCaracteres);
	void print(U32 numero, U8 coluna, U8 linha, U8 nCaracteres, escort::util::Converter::NumericalBase radix);
	void println();

protected:
	escort::driver::ILCDDisplay* LCD;

private:
	U8 numeroLinhas;
	U8 numeroColunas;

	U8 linhaCursor;
	U8 colunaCursor;
	char buffer[8];
	xSemaphoreHandle semaphore;
};

}
}
}
}

#endif /* GUI_DISPLAY_H_ */
