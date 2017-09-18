/*
 * MessageDialog.h
 *
 *  Created on: 11/06/2012
 *      Author: Gustavo
 */

#ifndef MESSAGEDIALOG_H_
#define MESSAGEDIALOG_H_

#include "InputOutput.h"
#include "../Fachada.h"
#include <escort.util/Converter.h>

namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

class MessageDialog {
public:
	static void showMessageDialog(char* message, InputOutput* inputOutput, U32 delay);
	static void showMessageDialog(Fachada::Resultado erro, InputOutput* inputOutput, U32 delay);
	static bool showConfirmationDialog(char* message, InputOutput* inputOutput, bool defaultValue, U8 timeout);
	static U32 showPasswordDialog(char* message, InputOutput* inputOutput, U8 nDigits);
	static U32 showPasswordDialog(char* message, InputOutput* inputOutput, U8 nDigits, escort::util::Converter::NumericalBase base);
	static void showInputStringDialog(char* displayMessage, char* inputString, InputOutput* inputOutput, U8 timeout);
};

}
}
}
}

#endif /* MESSAGEDIALOG_H_ */
