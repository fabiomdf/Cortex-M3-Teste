/*
 * TelaRelogio.cpp
 *
 *  Created on: 30/05/2012
 *      Author: Gustavo
 */

#include "TelaRelogio.h"
#include "MessageDialog.h"
#include "Resources.h"

using escort::service::Relogio;


#define TEMPO_PISCAGEM_CURSOR \
	500

enum {
	DOMINGO,
	SEGUNDA,
	TERCA,
	QUARTA,
	QUINTA,
	SEXTA,
	SABADO
};


namespace trf {
namespace pontos12 {
namespace application {
namespace gui {


U8 HORA = 0;
U8 MINUTOS = 1;
U8 SEGUNDOS = 2;
U8 DIA = 3;
U8 MES = 4;
U8 ANO = 5;
U8 DIA_SEMANA = 6;

const struct {
	U8 coluna;
	U8 linha;
} posicoesCursor[] = {{1,1},{4,1},{7,1},{1,0},{4,0},{7,0},{11,0}};



TelaRelogio::TelaRelogio(Fachada* fachada, InputOutput* inputOutput) :
		Tela(inputOutput),
		fachada(fachada)
{}

void TelaRelogio::paint()
{
	if (this->timerPaint < xTaskGetTickCount()) {
		//Obtem a data/hora atual
		Relogio::DataHora dataHora = this->fachada->getDataHora();
		//Obt�m o formato de data/hora
		Regiao::FormatoDataHora formatoDataHora = this->fachada->getFormatoDataHora();

		//Imprime a data/hora
		this->inputOutput->display.setLinhaColuna(0, 0);
		if(formatoDataHora == Regiao::FORMATO_24H){
			this->inputOutput->display.print(dataHora.dia, 2);
			this->inputOutput->display.print("/");
			this->inputOutput->display.print(dataHora.mes, 2);
		}
		else {
			this->inputOutput->display.print(dataHora.mes, 2);
			this->inputOutput->display.print("/");
			this->inputOutput->display.print(dataHora.dia, 2);
		}
		this->inputOutput->display.print("/");
		this->inputOutput->display.print(dataHora.ano % 100, 2);
		this->inputOutput->display.print(" ");
		char* textoDiaSemana;
		switch(dataHora.diaSemana){
			case DOMINGO:
				textoDiaSemana = Resources::getTexto(Resources::DOMINGO);
				break;
			case SEGUNDA:
				textoDiaSemana = Resources::getTexto(Resources::SEGUNDA);
				break;
			case TERCA:
				textoDiaSemana = Resources::getTexto(Resources::TERCA);
				break;
			case QUARTA:
				textoDiaSemana = Resources::getTexto(Resources::QUARTA);
				break;
			case QUINTA:
				textoDiaSemana = Resources::getTexto(Resources::QUINTA);
				break;
			case SEXTA:
				textoDiaSemana = Resources::getTexto(Resources::SEXTA);
				break;
			case SABADO:
				textoDiaSemana = Resources::getTexto(Resources::SABADO);
				break;
		}
		this->inputOutput->display.print(textoDiaSemana, 3);
		this->inputOutput->display.println();
		if((formatoDataHora == Regiao::FORMATO_AM_PM) && (dataHora.horas == 0)){
			this->inputOutput->display.print(12, 2);
		}
		else if((formatoDataHora == Regiao::FORMATO_AM_PM) && (dataHora.horas > 12)){
			this->inputOutput->display.print(dataHora.horas - 12, 2);
		}
		else {
			this->inputOutput->display.print(dataHora.horas, 2);
		}
		this->inputOutput->display.print(":");
		this->inputOutput->display.print(dataHora.minutos, 2);
		this->inputOutput->display.print(":");
		this->inputOutput->display.print(dataHora.segundos, 2);
		this->inputOutput->display.print(" ");
		if(formatoDataHora == Regiao::FORMATO_AM_PM){
			if(dataHora.horas > 12 || dataHora.horas == 0){
				this->inputOutput->display.print("pm");
			}
			else{
				this->inputOutput->display.print("am");
			}
		}
		else{
			this->inputOutput->display.print("  ");
		}
		//Coloca o cursor no campo que est� sendo editado
		this->inputOutput->display.setLinhaColuna(
				posicoesCursor[this->indiceCursor].coluna,
				posicoesCursor[this->indiceCursor].linha);
		//Reseta o timer de renderiza��o
		this->timerPaint = xTaskGetTickCount() + 1000;
	}

	//Se expirar o tempo do cursor ent�o volta a piscar o cursor
	if(this->timerCursor < xTaskGetTickCount()){
		if(this->cursorPiscando == false){
			this->cursorPiscando = true;
			this->inputOutput->display.setDisplayStatus(true, false, true);
		}
	}else {
		if(this->cursorPiscando){
			this->cursorPiscando = false;
			this->inputOutput->display.setDisplayStatus(false, true, true);
		}
	}
}

void TelaRelogio::keyEvent(Teclado::Tecla tecla)
{
	//Verifica se a tecla "AJUSTE ESQUERDA"
	if(tecla & Teclado::TECLA_AJUSTE_ESQUERDA)
	{
		//Se n�o h� permiss�o para o usu�rio alterar o rel�gio ent�o n�o deixa mudar os valores
		if(!verificarPermissaoAjuste()){
			return;
		}

		//Obtem a data/hora atual
		Relogio::DataHora dataHora = this->fachada->getDataHora();

		//Cursor no campo hora
		if(this->indiceCursor == HORA)
		{
			//Decrementa a hora
			dataHora.horas = decrementar(dataHora.horas, 0, 23);
		}
		//Cursor no campo minutos
		else if(this->indiceCursor == MINUTOS)
		{
			//Decrementa os minutos
			dataHora.minutos = decrementar(dataHora.minutos, 0, 59);
		}
		//Cursor no campo segundos
		else if(this->indiceCursor == SEGUNDOS)
		{
			//Decrementa os segundos
			dataHora.segundos = decrementar(dataHora.segundos, 0, 59);
		}
		//Cursor no campo dia
		else if(this->indiceCursor == DIA)
		{
			//Decrementa os dias
			dataHora.dia = decrementar(dataHora.dia, 1, 31);
		}
		//Cursor no campo m�s
		else if(this->indiceCursor == MES)
		{
			//Decrementa os meses
			dataHora.mes = decrementar(dataHora.mes, 1, 12);
		}
		//Cursor no campo ano
		else if(this->indiceCursor == ANO)
		{
			//Decrementa os anos
			dataHora.ano = decrementar(dataHora.ano, 2000, 2099);
		}
		//Cursor no campo dia da semana
		else if(this->indiceCursor == DIA_SEMANA)
		{
			//Decrementa os dias da semana
			dataHora.diaSemana = decrementar(dataHora.diaSemana, 0, 6);
		}
		else
		{
			this->indiceCursor = HORA;
		}

		//Altera a data/hora
		this->fachada->setDataHora(dataHora);
		//Indica que houve altera��o no rel�gio
		this->alterou = true;
		//Recarrega o timer do cursor
		this->timerCursor = xTaskGetTickCount() + TEMPO_PISCAGEM_CURSOR;
	}
	//Verifica se a tecla "AJUSTE DIREITA"
	if(tecla & Teclado::TECLA_AJUSTE_DIREITA)
	{
		//Se n�o h� permiss�o para o usu�rio alterar o rel�gio ent�o n�o deixa mudar os valores
		if(!verificarPermissaoAjuste()){
			return;
		}

		//Obtem a data/hora atual
		Relogio::DataHora dataHora = this->fachada->getDataHora();

		//Cursor no campo hora
		if(this->indiceCursor == HORA)
		{
			//Incrementa a hora
			dataHora.horas = incrementar(dataHora.horas, 0, 23);
		}
		//Cursor no campo minutos
		else if(this->indiceCursor == MINUTOS)
		{
			//Incrementa os minutos
			dataHora.minutos = incrementar(dataHora.minutos, 0, 59);
		}
		//Cursor no campo segundos
		else if(this->indiceCursor == SEGUNDOS)
		{
			//Incrementa os segundos
			dataHora.segundos = incrementar(dataHora.segundos, 0, 59);
		}
		//Cursor no campo dia
		else if(this->indiceCursor == DIA)
		{
			//Incrementa os dias
			dataHora.dia = incrementar(dataHora.dia, 1, 31);
		}
		//Cursor no campo m�s
		else if(this->indiceCursor == MES)
		{
			//Incrementa os meses
			dataHora.mes = incrementar(dataHora.mes, 1, 12);
		}
		//Cursor no campo ano
		else if(this->indiceCursor == ANO)
		{
			//Incrementa os anos
			dataHora.ano = incrementar(dataHora.ano, 2000, 2099);
		}
		//Cursor no campo dia da semana
		else if(this->indiceCursor == DIA_SEMANA)
		{
			//Incrementa os dias da semana
			dataHora.diaSemana = incrementar(dataHora.diaSemana, 0, 6);
		}
		else
		{
			this->indiceCursor = HORA;
		}

		//Altera a data/hora
		this->fachada->setDataHora(dataHora);
		//Indica que houve altera��o no rel�gio
		this->alterou = true;
		//Recarrega o timer do cursor
		this->timerCursor = xTaskGetTickCount() + TEMPO_PISCAGEM_CURSOR;
	}
	//Verifica se a tecla "OK"
	if(tecla & Teclado::TECLA_OK)
	{
		//Cursor num campo que n�o � o dia da semana
		if(this->indiceCursor < DIA_SEMANA)
		{
			//Passa para o pr�ximo campo
			this->indiceCursor++;
		}
		//Cursor no campo dia da semana
		else if(this->indiceCursor == DIA_SEMANA)
		{
			//Finaliza a exibi��o
			this->finish();
			this->visible = false;
		}
		else
		{
			this->indiceCursor = HORA;
		}
	}

	this->timerPaint = xTaskGetTickCount();
}

void TelaRelogio::start()
{
	//Limpa a tela
	this->inputOutput->display.clearScreen();
	//Come�a com o cursor desligado
	this->cursorPiscando = false;
	//Coloca o cursor na posi��o da hora
	this->indiceCursor = HORA;
	//Inicializa o timer de piscagem do cursor
	this->timerCursor = xTaskGetTickCount() + TEMPO_PISCAGEM_CURSOR;
	//Inicializa o timer de renderiza��o
	this->timerPaint = 0;
	//Indica que o rel�gio n�o foi alterado (in�cio)
	this->alterou = false;
	//Se o formato de data/hora for AM/PM ent�o muda a posi��o dos campos dia e m�s
	if(this->fachada->getFormatoDataHora() == Regiao::FORMATO_AM_PM){
		DIA = 4;
		MES = 3;
	}
	//Verifica se a fun��o de ajuste do rel�gio est� bloqueada
	// e, caso a sess�o esteja aberta (senha de desbloqueio), o contador � resetado (mais 60 segundos de fun��es liberadas)
	this->acessoLiberado = (this->fachada->isFuncaoLiberada(ParametrosFixos::AJUSTE_RELOGIO));
	if(!verificarPermissaoAjuste()){
		this->visible = false;
	}
}

void TelaRelogio::finish()
{
	//Desliga a piscagem do cursor
	this->inputOutput->display.setDisplayStatus(false, false, true);
	//Verifica se a data/hora foi alterada
	if(this->alterou)
	{
		//Indica ao usu�rio para aguardar o rein�cio da exibi��o
		this->inputOutput->display.clearScreen();
		this->inputOutput->display.print(Resources::getTexto(Resources::AGUARDE));

		bool sucesso = true;
 		U8 qntPaineis = this->fachada->getQntPaineis();
 		for (U8 indicePainel = 0; indicePainel < qntPaineis; ++indicePainel) {
 			if(this->fachada->syncDataHoraPainel(indicePainel) == false){
 				sucesso = false;
 			}
 		}
		//Se houver erro exibe mensagem de falha
		if(!sucesso){
			this->inputOutput->display.clearScreen();
			this->inputOutput->display.print(Resources::getTexto(Resources::FALHA));
			vTaskDelay(2000);
		}
	}
}

U16 TelaRelogio::incrementar(U16 valor, U16 limitanteInferior, U16 limitanteSuperior)
{
	if(valor == limitanteSuperior){
		valor = limitanteInferior;
	} else {
		valor++;
	}

	return valor;
}

U16 TelaRelogio::decrementar(U16 valor, U16 limitanteInferior, U16 limitanteSuperior)
{
	if(valor == limitanteInferior){
		valor = limitanteSuperior;
	} else {
		valor--;
	}

	return valor;
}

bool TelaRelogio::verificarPermissaoAjuste()
{
	//Se a fun��o est� bloqueada e o usu�rio ainda n�o desbloqueou com a senha de acesso
	//ent�o pergunta para o usu�rio
	if(!this->acessoLiberado){
		//Indica ao usu�rio que a fun��o est� bloqueada
		MessageDialog::showMessageDialog(Resources::getTexto(Resources::FUNCAO_BLOQUEADA), inputOutput, 2000);
		//Pergunta ao usu�rio se deseja colocar a senha para desbloquear a fun��o
		//Obs.: s� faz sentido colocar a senha de acesso se ela estiver cadastrada
		if(this->fachada->isSenhaAcessoHabilitada()
			&& MessageDialog::showConfirmationDialog(
				Resources::getTexto(Resources::DESEJA_DESBLOQUEAR_FUNCAO),
				this->inputOutput,
				false,
				10))
		{
			//Pede para o usu�rio digitar a senha de desbloqueio
			U32 senha = MessageDialog::showPasswordDialog(
					Resources::getTexto(Resources::SENHA),
					this->inputOutput,
					6);
			//Se a senha est� correta ent�o libera o acesso
			if(this->fachada->verificarSenhaAcesso(senha)){
				this->acessoLiberado = true;
			}
			else{
				//Indica ao usu�rio que a senha est� incorreta
				MessageDialog::showMessageDialog(Resources::getTexto(Resources::SENHA_INCORRETA), inputOutput, 2000);
			}
		}
		//Recarrega o timeout
		this->timeoutTecla = xTaskGetTickCount() + teclaTime;
	}

	return this->acessoLiberado;
}

}
}
}
}
