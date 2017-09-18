/*
 * TelaIdentificacao.cpp
 *
 *  Created on: 30/05/2012
 *      Author: Gustavo
 */

#include "TelaIdentificacao.h"
#include <freertos.kernel/FreeRTOSKernel/FreeRTOSKernel.h>
#include "Resources.h"
#include "trf.pontos12.application/Controlador/files/ParametrosFixos.h"
#include "MessageDialog.h"
#include <escort.util/String.h>

using escort::util::String;

#define PRIMEIRA_OPCAO \
	HORA_SAIDA
#define ULTIMA_OPCAO \
	FORMATAR_PENDRIVE


namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

TelaIdentificacao::TelaIdentificacao(Fachada* fachada, InputOutput* inputOutput) :
		Tela(inputOutput),
		fachada(fachada)
{
	this->countAltern = 0;
}

void TelaIdentificacao::paint()
{
	//Coloca o cursor no come�o do display
	this->inputOutput->display.setLinhaColuna(0,0);
	//Obs.: Texto "painel" deve sempre deixar espa�o para a numera��o na parte final dos 16 caracteres
	//Imprime as informa��es do painel
	this->inputOutput->display.print(Resources::getTexto(Resources::PAINEL));
	this->inputOutput->display.setLinhaColuna(14,0);
	this->inputOutput->display.print(this->indicePainel + 1, 2);
	//Linha de baixo
	this->inputOutput->display.setLinhaColuna(0,1);
	//Exibe dimens�es e endere�o na rede
	if(countAltern < 30)
	{
		U32 altura = this->fachada->getAlturaPainel(this->indicePainel);
		U32 largura = this->fachada->getLarguraPainel(this->indicePainel);
		this->inputOutput->display.print("            ");
		this->inputOutput->display.setLinhaColuna(0,1);
		//Imprime as dimens�es do painel
		this->inputOutput->display.print(altura);
		this->inputOutput->display.print("x");
		this->inputOutput->display.print(largura);
		//Imprime o endere�o do painel selecionado no final da linha
		this->inputOutput->display.setLinhaColuna(12,1);
		this->inputOutput->display.print("0x");
		this->inputOutput->display.print(this->paineisListados[this->painelListadoSelecionado].netAddr, 2, escort::util::Converter::BASE_16);
	}
	//Exibe "Pressione OK"
	else
	{
		this->inputOutput->display.print("                ");
		this->inputOutput->display.setLinhaColuna(0,1);
		this->inputOutput->display.print(Resources::getTexto(Resources::PRESSIONE_OK));
	}
	//Atualiza contador
	this->countAltern++;
	this->countAltern %= 50;
}

void TelaIdentificacao::keyEvent(Teclado::Tecla tecla)
{
	//Verifica se a tecla "AJUSTE ESQUERDA"
	if(tecla & Teclado::TECLA_AJUSTE_ESQUERDA)
	{
		this->countAltern = 0;
		//Apaga o painel selecionado (pelo endere�o de rede)
		this->fachada->apagarPainelByAddr(this->paineisListados[this->painelListadoSelecionado].netAddr, 0);
		//Decrementa o �ndice (respeitando as condi��es determinadas)
		do{
			if(this->painelListadoSelecionado == 0){
				this->painelListadoSelecionado = this->qntPaineis - 1;
			}
			else {
				this->painelListadoSelecionado--;
			}
		}while(this->paineisListados[this->painelListadoSelecionado].indice != (U8)-1
				&& !this->paineisListados[this->painelListadoSelecionado].flags.ambiguo);
		//Acende o painel selecionado (pelo endere�o de rede)
		this->fachada->acenderPainelByAddr(this->paineisListados[this->painelListadoSelecionado].netAddr, 0xFFFF);
	}
	//Verifica se a tecla "AJUSTE DIREITA"
	if(tecla & Teclado::TECLA_AJUSTE_DIREITA)
	{
		this->countAltern = 0;
		//Apaga o painel selecionado (pelo endere�o de rede)
		this->fachada->apagarPainelByAddr(this->paineisListados[this->painelListadoSelecionado].netAddr, 0);
		//Incrementa o �ndice (respeitando as condi��es determinadas)
		do{
			if(this->painelListadoSelecionado == this->qntPaineis - 1){
				this->painelListadoSelecionado = 0;
			}
			else {
				this->painelListadoSelecionado++;
			}
		}while(this->paineisListados[this->painelListadoSelecionado].indice != (U8)-1
				&& !this->paineisListados[this->painelListadoSelecionado].flags.ambiguo);
		//Acende o painel selecionado (pelo endere�o de rede)
		this->fachada->acenderPainelByAddr(this->paineisListados[this->painelListadoSelecionado].netAddr, 0xFFFF);
	}
	//Verifica se a tecla "OK"
	if(tecla & Teclado::TECLA_OK)
	{
		this->countAltern = 0;
		//Apaga o painel selecionado (pelo endere�o de rede)
		this->fachada->apagarPainelByAddr(this->paineisListados[this->painelListadoSelecionado].netAddr, 0);
		//Salva a informa��o do �ndice para o painel selecionado
		this->paineisListados[this->painelListadoSelecionado].indice = this->indicePainel;
		this->paineisListados[this->painelListadoSelecionado].flags.ambiguo = false;
		//Procura o pr�ximo �ndice que ainda n�o foi atribu�do a nenhum painel
		this->indicePainel++;
		for(; this->indicePainel < this->qntPaineis; this->indicePainel++){
			bool atribuido = false;
			for (U8 i = 0; i < this->qntPaineis; ++i) {
				if(this->paineisListados[i].indice == this->indicePainel
					&& !this->paineisListados[i].flags.ambiguo)
				{
					atribuido = true;
					break;
				}
			}
			if(!atribuido){
				break;
			}
		}
		//Verifica se todos os paineis j� est�o emparelhados
		if(this->indicePainel >= this->qntPaineis){
			//Salva os endere�os e n�meros de s�rie de cada painel
			this->finalizarProcedimento();
			//Finaliza a tela
			this->visible = false;
			return;
		}
		//Decrementa o �ndice (respeitando as condi��es determinadas)
		do{
			if(this->painelListadoSelecionado == 0){
				this->painelListadoSelecionado = this->qntPaineis - 1;
			}
			else {
				this->painelListadoSelecionado--;
			}
		}while(this->paineisListados[this->painelListadoSelecionado].indice != (U8)-1
				&& !this->paineisListados[this->painelListadoSelecionado].flags.ambiguo);
		//Acende o painel selecionado (pelo endere�o de rede)
		this->fachada->acenderPainelByAddr(this->paineisListados[this->painelListadoSelecionado].netAddr, 0xFFFF);
	}
}

void TelaIdentificacao::start()
{
	//Verifica se o Controlador est� funcionando corretamente
	if(this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE))
	{
		this->inputOutput->display.clearScreen();
		this->inputOutput->display.print("N/A             ");
		vTaskDelay(2000);
		this->visible = false;
		return;
	}
	//S� realiza o processo de identifica��o dos paineis se houver permiss�o
	if(verificarPermissao() == false){
		this->visible = false;
		return;
	}
	//Pergunta ao usu�rio se deseja um emparelhamento autom�tico ou manual
	this->automatico = MessageDialog::showConfirmationDialog(
					Resources::getTexto(Resources::AUTOMATICO),
					this->inputOutput,
					true,
					10);
	//Inicia o emparelhamento
	this->inputOutput->display.clearScreen();
	this->inputOutput->display.print(Resources::getTexto(Resources::IDENTIFICANDO));
	Fachada::Resultado res = this->fachada->emparelharPaineis(&this->paineisListados, automatico);
	if(res != Fachada::SUCESSO){
		MessageDialog::showMessageDialog(res, inputOutput, 2000);
		this->visible = false;
		return;
	}
	//Procura o �ndice que ainda n�o foi atribu�do a nenhum painel
	this->qntPaineis = this->fachada->getQntPaineis();
	for(this->indicePainel = 0; this->indicePainel < this->qntPaineis; this->indicePainel++){
		bool atribuido = false;
		for (U8 i = 0; i < this->qntPaineis; ++i) {
			if(this->paineisListados[i].indice == this->indicePainel
				&& !this->paineisListados[i].flags.ambiguo)
			{
				atribuido = true;
				break;
			}
		}
		if(!atribuido){
			break;
		}
	}
	//Verifica se todos os paineis j� est�o emparelhados
	if(this->indicePainel >= this->qntPaineis){
		//Salva os endere�os e n�meros de s�rie de cada painel
		this->finalizarProcedimento();
		//Finaliza a tela
		this->visible = false;
		return;
	}
	//Procura o primeiro painel listado que ainda n�o teve um �ndice atribu�do
	this->painelListadoSelecionado = 0;
	while(this->paineisListados[this->painelListadoSelecionado].indice != (U8)-1
			&& !this->paineisListados[this->painelListadoSelecionado].flags.ambiguo)
	{
		this->painelListadoSelecionado++;
	}
	//Acende o painel selecionado (pelo endere�o de rede)
	this->fachada->acenderPainelByAddr(this->paineisListados[this->painelListadoSelecionado].netAddr, 0xFFFF);
	//Altera o timeout da tecla para 60 segundos
	this->teclaTime = 60000;
	//Limpa a tela
	this->inputOutput->display.clearScreen();
}

void TelaIdentificacao::finish()
{
	//Apaga o painel selecionado (pelo endere�o de rede)
	this->fachada->apagarPainelByAddr(this->paineisListados[this->painelListadoSelecionado].netAddr, 0);
}

void TelaIdentificacao::finalizarProcedimento()
{
	//Indica para o usu�rio aguardar
	this->inputOutput->display.clearScreen();
	this->inputOutput->display.print(Resources::getTexto(Resources::AGUARDE));
	//Salva os endere�os
	for (U8 i = 0; i < this->qntPaineis; ++i) {
		U8 indice = this->paineisListados[i].indice;
		U8 netAddr = this->paineisListados[i].netAddr;
		U64 numSerie;
		String::memcpy((U8*)&numSerie, this->paineisListados[i].numeroSerie, sizeof(numSerie));
		this->fachada->writePainelParameters(
				indice,
				netAddr,
				numSerie);
	}
	for (U8 p = 0; p < this->qntPaineis; ++p) {
		this->fachada->setPainelPrecisandoFormatar(p, true);
		this->fachada->setPainelSincronizado(p, false);
	}
	this->fachada->setAPPStatusConfig(Fachada::STATUS_CONFIG_APP_PRECISANDO_SINCRONIZAR_PARAMETROS);
	//Indica ao usu�rio o sucesso da opera��o
	MessageDialog::showMessageDialog(Resources::getTexto(Resources::SUCESSO), inputOutput, 2000);
}

bool TelaIdentificacao::verificarPermissao()
{
	//Se a fun��o est� bloqueada e o usu�rio ainda n�o desbloqueou com a senha de acesso
	//ent�o pergunta para o usu�rio
	// e, caso a sess�o esteja aberta (senha de desbloqueio), o contador � resetado (mais 60 segundos de fun��es liberadas)
	bool acessoLiberado = (this->fachada->isFuncaoLiberada(ParametrosFixos::IDENTIFICAR_PAINEIS));
	if(!acessoLiberado){
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
				acessoLiberado = true;
			}
			else{
				//Indica ao usu�rio que a senha est� incorreta
				MessageDialog::showMessageDialog(Resources::getTexto(Resources::SENHA_INCORRETA), inputOutput, 2000);
			}
		}
		//Recarrega o timeout
		this->timeoutTecla = xTaskGetTickCount() + teclaTime;
	}

	return acessoLiberado;
}

}
}
}
}
