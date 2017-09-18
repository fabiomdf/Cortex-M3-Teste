/*
 * MessageDialog.cpp
 *
 *  Created on: 11/06/2012
 *      Author: Gustavo
 */

#include "MessageDialog.h"
#include <freertos.kernel/FreeRTOSKernel/FreeRTOSKernel.h>
#include "Resources.h"
#include <escort.util/String.h>
#include <escort.util/Math.h>
#include <escort.util/Converter.h>

using escort::util::String;
using escort::util::Math;
using escort::util::Converter;


#define CURSOR_BLINKING_TIME	500


namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

void MessageDialog::showMessageDialog(char* message, InputOutput* inputOutput, U32 delay)
{
	bool blink, cursor, display;

	//Salva o status atual do display
	inputOutput->display.getDisplayStatus(&blink, &cursor, &display);
	//Altera o status para desligar a piscagem do cursor
	inputOutput->display.setDisplayStatus(false, false, true);
	//Escreve a mensagem
	inputOutput->display.clearScreen();
	inputOutput->display.print(message);
	//Aguarda o tempo de espera
	vTaskDelay(delay);
	//Restaura o status anterior do display
	inputOutput->display.setDisplayStatus(blink, cursor, display);
	//Evita que a tela anterior leia eventos errados do teclado
	inputOutput->teclado.clearEventosTeclado();
}

void MessageDialog::showMessageDialog(Fachada::Resultado erro, InputOutput* inputOutput, U32 delay)
{
	switch (erro) {
		case Fachada::SUCESSO:
			showMessageDialog(Resources::getTexto(Resources::SUCESSO), inputOutput, delay);
			break;
		case Fachada::FUNCAO_BLOQUEADA:
			showMessageDialog(Resources::getTexto(Resources::FUNCAO_BLOQUEADA), inputOutput, delay);
			break;
		case Fachada::SENHA_INCORRETA:
			showMessageDialog(Resources::getTexto(Resources::SENHA_INCORRETA), inputOutput, delay);
			break;
		case Fachada::SENHA_ANTI_FURTO_INCORRETA:
			showMessageDialog(Resources::getTexto(Resources::SENHA_ANTI_FURTO_INCORRETA), inputOutput, delay);
			break;
		case Fachada::ERRO_COMUNICACAO:
			showMessageDialog(Resources::getTexto(Resources::FALHA_COMUNICACAO), inputOutput, delay);
			break;
		case Fachada::NOVA_CONFIG_INVALIDA:
			showMessageDialog(Resources::getTexto(Resources::NOVA_CONFIG_INVALIDA), inputOutput, delay);
			break;
		case Fachada::EMPARELHAMENTO_INCOMPATIVEL:
			showMessageDialog(Resources::getTexto(Resources::EMPARELHAMENTO_INCOMPATIVEL), inputOutput, delay);
			break;
		case Fachada::PAINEIS_TRAVADOS:
			showMessageDialog(Resources::getTexto(Resources::PAINEIS_TRAVADOS), inputOutput, delay);
			break;
		default:
			showMessageDialog(Resources::getTexto(Resources::FALHA), inputOutput, delay);
			break;
	}
}

bool MessageDialog::showConfirmationDialog(char* message, InputOutput* inputOutput, bool defaultValue, U8 timeoutSeconds)
{
	bool choice = defaultValue;
	bool blink, cursor, display;
	char choiceStr[16] = "\0";
	bool cursorPiscando = false;
	portTickType timerCursor = 0, timerPaint = 0;

	String::trim(message);
	U8 messageLength = String::getLength(message);

	//Salva o status atual do display
	inputOutput->display.getDisplayStatus(&blink, &cursor, &display);
	//Altera o status para ligar a piscagem do cursor
	inputOutput->display.setDisplayStatus(true, false, true);
	//Otem a contagem de tempo atual para marcar o in�cio
	portTickType startTime = xTaskGetTickCount();
	
	U8 blankPos = 16;

	//Escreve a mensagem
	inputOutput->display.clearScreen();
	inputOutput->display.print(message);
	//Evita que os eventos anteriores do teclado interfiram neste diolog
	inputOutput->teclado.clearEventosTeclado();

	while(true){
		//Se existe timeout e o tempo limite estourou ent�o para
		if(timeoutSeconds && (xTaskGetTickCount() > (startTime + ((U32)timeoutSeconds*1000)))){
			break;
		}
		if (timerPaint < xTaskGetTickCount()) {
			//Obtem a string referente � escolha atualmente selecionada
			String::strcpy(choiceStr, Resources::getTexto(Resources::SIM_NAO) + (choice ? 0 : 8), ' ');
			//Calcula o tamanho da string
			int choiceStrLength = String::indexOf(choiceStr, " ");
			if(choiceStrLength < 0 || choiceStrLength >= 8){
				choiceStrLength = Math::min(8, String::getLength(choiceStr));
			}
			choiceStr[choiceStrLength] = '\0';
			//Calcula a posi��o da coluna onde a string da escolha ser� impressa
			int column = 16 - choiceStrLength;
			//Se existe timeout ent�o imprime a quantidade de segundos restantes
			if(timeoutSeconds){
				//Calcula a quantidade de segundos restantes ao timeout
				U8 secondsRemaining = timeoutSeconds - ((xTaskGetTickCount() - startTime)/1000);
				//Atualiza o c�lculo da posi��o da coluna
				column -= (2 + Converter::getNumberOfDigits(secondsRemaining, Converter::BASE_10));
				//Indica quantos segundos falta para a tela sair
				String::strcat(choiceStr, "(");
				Converter::itoa(secondsRemaining, choiceStr + choiceStrLength + 1);
				String::strcat(choiceStr, ")");
				choiceStrLength = String::getLength(choiceStr);
			}
			//Atualiza a posi��o onde deve limpar a tela
			if(blankPos > (16 - choiceStrLength)){
				blankPos = 16 - choiceStrLength;
			}

			//Limpa a regi�o do display
			inputOutput->display.setLinhaColuna(blankPos, 1);
			for (int i = blankPos; i < 16; ++i) {
				inputOutput->display.print(' ');
			}
			//Coloca o cursor na posi��o correta
			inputOutput->display.setLinhaColuna(column, 1);
			//Imprime a escolha atual
			inputOutput->display.print(choiceStr);
			//Volta o cursor na posi��o do in�cio
			inputOutput->display.setLinhaColuna(column, 1);
			//Reseta o timer de renderiza��o
			timerPaint = xTaskGetTickCount() + 1000;
		}
		//Se expirar o tempo do cursor ent�o volta a piscar o cursor
		if(timerCursor < xTaskGetTickCount()){
			if(cursorPiscando == false){
				cursorPiscando = true;
				inputOutput->display.setDisplayStatus(true, false, true);
			}
		}else {
			if(cursorPiscando){
				cursorPiscando = false;
				inputOutput->display.setDisplayStatus(false, true, true);
			}
		}
		//Verifica se existe um evento do teclado para ser tratado
		Teclado::Tecla tecla = inputOutput->teclado.getEventoTeclado();
		if(tecla)
		{
			//Verifica se a tecla "AJUSTE ESQUERDA" ou "AJUSTE DIREITA" foi pressionada
			if(tecla & (Teclado::TECLA_AJUSTE_ESQUERDA | Teclado::TECLA_AJUSTE_DIREITA))
			{
				//Muda a escolha
				choice = !choice;
				//Recarrega o timer do cursor
				timerCursor = xTaskGetTickCount() + CURSOR_BLINKING_TIME;
			}
			//Verifica se a tecla "OK" foi pressionada
			if(tecla & Teclado::TECLA_OK)
			{
				break;
			}
			//Se o usu�rio interagir ent�o o timeout � automaticamente desativado
			timeoutSeconds = 0;
			//For�a refresh do display
			timerPaint = xTaskGetTickCount();
		}

		vTaskDelay(10);
	}

	//Restaura o status anterior do display
	inputOutput->display.setDisplayStatus(blink, cursor, display);
	//Evita que a tela anterior leia eventos errados do teclado
	inputOutput->teclado.clearEventosTeclado();

	return choice;
}

U32 MessageDialog::showPasswordDialog(char* message, InputOutput* inputOutput, U8 nDigits)
{
	return showPasswordDialog(message, inputOutput, nDigits, Converter::BASE_10);
}

U32 MessageDialog::showPasswordDialog(char* message, InputOutput* inputOutput, U8 nDigits, Converter::NumericalBase base)
{
	U32 password = 0;
	bool blink, cursor, display;
	bool cursorPiscando = false;
	U8 pos = 0;
	U32 increment = 1;
	U8 digitUnity = 10;
	portTickType timerCursor = 0, timerPaint = 0;

	String::trim(message);
	U8 messageLength = String::getLength(message);

	if(base == Converter::BASE_16){
		digitUnity = 16;
	}
	else{
		digitUnity = 10;
	}

	//Calcula o incremento inicial
	for (int i = 1; i < nDigits; ++i) {
		increment *= digitUnity;
	}

	//Salva o status atual do display
	inputOutput->display.getDisplayStatus(&blink, &cursor, &display);
	//Altera o status para ligar a piscagem do cursor
	inputOutput->display.setDisplayStatus(true, false, true);
	//Escreve a mensagem
	inputOutput->display.clearScreen();
	inputOutput->display.print(message);
	//Evita que os eventos anteriores do teclado interfiram neste diolog
	inputOutput->teclado.clearEventosTeclado();

	while(true){
		//Se expirar o tempo do cursor ent�o volta a piscar o cursor
		if(timerCursor < xTaskGetTickCount()){
			if(cursorPiscando == false){
				cursorPiscando = true;
				inputOutput->display.setDisplayStatus(true, false, true);
				//For�a refresh do display
				timerPaint = xTaskGetTickCount();
			}
		}else {
			if(cursorPiscando){
				cursorPiscando = false;
				inputOutput->display.setDisplayStatus(false, true, true);
			}
		}
		//Atualiza a tela de 1 em 1 segundo
		if (timerPaint < xTaskGetTickCount()) {
			//Imprime o campo da senha
			U32 aux = password;
			for (int i = 16; i > (16 - nDigits); i--) {
				//Coloca o cursor na posi��o correta
				inputOutput->display.setLinhaColuna(i - 1, 1);
				//Exibe o algarismo que est� sendo alterado no momento
				if(!cursorPiscando && ((16 - nDigits + pos) == (i - 1))){
					inputOutput->display.print(aux % digitUnity, base);
				}
				else{
					inputOutput->display.print("*");
				}
				aux = aux / digitUnity;
			}
			//Volta o cursor na posi��o do in�cio
			inputOutput->display.setLinhaColuna(16 - nDigits + pos, 1);
			//Reseta o timer de renderiza��o
			timerPaint = xTaskGetTickCount() + 1000;
		}
		//Verifica se existe um evento do teclado para ser tratado
		Teclado::Tecla tecla = inputOutput->teclado.getEventoTeclado();
		if(tecla)
		{
			//Verifica se a tecla "AJUSTE ESQUERDA" foi pressionada
			if(tecla & Teclado::TECLA_AJUSTE_ESQUERDA)
			{
				//Decrementa o algarismo
				if(((password / increment) % digitUnity) == 0){
					password += ((digitUnity - 1)*increment);
				}
				else{
					password -= increment;
				}
				//Recarrega o timer do cursor
				timerCursor = xTaskGetTickCount() + CURSOR_BLINKING_TIME;
			}
			//Verifica se a tecla "AJUSTE DIREITA" foi pressionada
			if(tecla & Teclado::TECLA_AJUSTE_DIREITA)
			{
				//Incrementa o algarismo
				if(((password / increment) % digitUnity) == (digitUnity - 1)){
					password -= ((digitUnity - 1)*increment);
				}
				else{
					password += increment;
				}
				//Recarrega o timer do cursor
				timerCursor = xTaskGetTickCount() + CURSOR_BLINKING_TIME;
			}
			//Verifica se a tecla "OK" foi pressionada
			if(tecla & Teclado::TECLA_OK)
			{
				//Passa para o pr�ximo d�gito
				pos++;
				increment /= digitUnity;
				//Verifica se terminou
				if(pos == nDigits){
					break;
				}
			}
			//For�a refresh do display
			timerPaint = xTaskGetTickCount();
		}

		vTaskDelay(10);
	}

	//Restaura o status anterior do display
	inputOutput->display.setDisplayStatus(blink, cursor, display);
	//Evita que a tela anterior leia eventos errados do teclado
	inputOutput->teclado.clearEventosTeclado();

	return password;
}

void MessageDialog::showInputStringDialog(char* displayMessage, char* inputString, InputOutput* inputOutput, U8 timeoutSeconds)
{
	bool blink, cursor, display;
	U8 indiceCursor = 0;
	bool cursorPiscando = false;
	portTickType timerCursor = 0, timerPaint = 0;

	String::trim(displayMessage);
	U8 messageLength = String::getLength(displayMessage);

	//Salva o status atual do display
	inputOutput->display.getDisplayStatus(&blink, &cursor, &display);
	//Altera o status para ligar a piscagem do cursor
	inputOutput->display.setDisplayStatus(true, false, true);
	//Otem a contagem de tempo atual para marcar o in�cio
	portTickType startTime = xTaskGetTickCount();

	//Escreve a mensagem
	inputOutput->display.clearScreen();
	inputOutput->display.print(displayMessage);
	//Evita que os eventos anteriores do teclado interfiram neste diolog
	inputOutput->teclado.clearEventosTeclado();

	//Escreve inputString default no display
	inputOutput->display.setLinhaColuna(0, 1);
	for (U8 i = 0; (i < 8) && (inputString[i] != '\0'); ++i) {
		inputOutput->display.print(inputString[i]);
	}

	while(true){
		//Se existe timeout e o tempo limite estourou ent�o para
		if(timeoutSeconds && (xTaskGetTickCount() > (startTime + ((U32)timeoutSeconds*1000)))){
			break;
		}
		if (timerPaint < xTaskGetTickCount()) {
			//Coloca o cursor na posi��o correta
			inputOutput->display.setLinhaColuna(indiceCursor, 1);
			//Imprime a escolha atual
			inputOutput->display.print(inputString[indiceCursor]);
			//Volta o cursor na posi��o do in�cio
			inputOutput->display.setLinhaColuna(indiceCursor, 1);
			//Reseta o timer de renderiza��o
			timerPaint = xTaskGetTickCount() + 1000;
		}
		//Se expirar o tempo do cursor ent�o volta a piscar o cursor
		if(timerCursor < xTaskGetTickCount()){
			if(cursorPiscando == false){
				cursorPiscando = true;
				inputOutput->display.setDisplayStatus(true, false, true);
			}
		}else {
			if(cursorPiscando){
				cursorPiscando = false;
				inputOutput->display.setDisplayStatus(false, true, true);
			}
		}
		//Verifica se existe um evento do teclado para ser tratado
		Teclado::Tecla tecla = inputOutput->teclado.getEventoTeclado();
		if(tecla)
		{
			//Verifica se a tecla "AJUSTE ESQUERDA" foi pressionada
			if(tecla & Teclado::TECLA_AJUSTE_ESQUERDA)
			{
				//Decrementa o caractere
				if(inputString[indiceCursor] == 'a'){
					inputString[indiceCursor] = '\0';
				}
				else if(inputString[indiceCursor] == '0'){
					inputString[indiceCursor] = 'z';
				}
				else if(inputString[indiceCursor] == '_'){
					inputString[indiceCursor] = '9';
				}
				else if(inputString[indiceCursor] == '\0'){
					inputString[indiceCursor] = '_';
				}
				else{
					inputString[indiceCursor]--;
				}
				//Recarrega o timer do cursor
				timerCursor = xTaskGetTickCount() + CURSOR_BLINKING_TIME;
			}
			//Verifica se a tecla "AJUSTE DIREITA" foi pressionada
			if(tecla & Teclado::TECLA_AJUSTE_DIREITA)
			{
				//Incrementa o caractere
				if(inputString[indiceCursor] == 'z'){
					inputString[indiceCursor] = '0';
				}
				else if(inputString[indiceCursor] == '9'){
					inputString[indiceCursor] = '_';
				}
				else if(inputString[indiceCursor] == '_'){
					inputString[indiceCursor] = '\0';
				}
				else if(inputString[indiceCursor] == '\0'){
					inputString[indiceCursor] = 'a';
				}
				else{
					inputString[indiceCursor]++;
				}
				//Recarrega o timer do cursor
				timerCursor = xTaskGetTickCount() + CURSOR_BLINKING_TIME;
			}
			//Verifica se a tecla "OK" foi pressionada
			if(tecla & Teclado::TECLA_OK)
			{
				if(indiceCursor == 7){
					break;
				}
				else if(inputString[indiceCursor] == '\0'){
					for (U8 i = indiceCursor; i < 8; ++i) {
						inputString[i] = '\0';
					}
					break;
				}
				else{
					indiceCursor++;
				}
			}
			//Se o usu�rio interagir ent�o o timeout � automaticamente desativado
			timeoutSeconds = 0;
			//For�a refresh do display
			timerPaint = xTaskGetTickCount();
		}

		vTaskDelay(10);
	}

	//Restaura o status anterior do display
	inputOutput->display.setDisplayStatus(blink, cursor, display);
	//Evita que a tela anterior leia eventos errados do teclado
	inputOutput->teclado.clearEventosTeclado();
}

}
}
}
}
