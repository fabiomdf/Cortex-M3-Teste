/*
 * TelaSelecaoMotorista.cpp
 *
 *  Created on: 30/05/2012
 *      Author: Gustavo
 */

#include "TelaSelecaoMotorista.h"
#include "MessageDialog.h"
#include <escort.util/String.h>
#include <escort.util/Math.h>
#include "Resources.h"
#include "GUI.h"

using escort::util::String;
using escort::util::Math;


namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

TelaSelecaoMotorista::TelaSelecaoMotorista(
	Fachada* fachada,
	InputOutput* inputOutput) :
		Tela(inputOutput),
		fachada(fachada)
{}

void TelaSelecaoMotorista::paint()
{
	if((this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE))
		|| this->fachada->getQntMotoristas() == 0)
	{
		this->inputOutput->display.setLinhaColuna(0, 0);
		this->inputOutput->display.print("N/A             ");
	}
	else{
		this->inputOutput->display.setLinhaColuna(11,0);
		this->inputOutput->display.print(this->indiceMotorista, 5);
		this->inputOutput->display.println();

		////Monta o r�tulo da identifica��o do motorista com o nome
		this->fachada->getLabelIdComNomeMotorista(this->indiceMotorista, this->buffer);
		U32 length = Math::min(this->inputOutput->display.getWidth(), String::getLength(this->buffer));

		this->inputOutput->display.print(this->buffer, length);
		for(U32 i = length; i < this->inputOutput->display.getWidth(); i++){
			this->inputOutput->display.print(" ");
		}
	}
}

void TelaSelecaoMotorista::keyEvent(Teclado::Tecla tecla)
{
	if((this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE))
		|| this->fachada->getQntMotoristas() == 0)
	{
		return;
	}
	//Verifica se a tecla "AJUSTE ESQUERDA"
	if(tecla & Teclado::TECLA_AJUSTE_ESQUERDA)
	{
		//Indica modifica��o no valor
		this->modificado = true;
		//Se o �ndice est� indicando o primeiro motorista ent�o vai para o �ltimo
		if(this->indiceMotorista == 0)
		{
			this->indiceMotorista = (this->qntMotorista - 1);
		}
		//Caso contr�rio decrementa o �ndice do motorista selecionado
		else
		{
			this->indiceMotorista--;
		}
	}
	//Verifica se a tecla "AJUSTE DIREITA"
	if(tecla & Teclado::TECLA_AJUSTE_DIREITA)
	{
		//Indica modifica��o no valor
		this->modificado = true;
		//Se o �ndice est� indicando o �ltimo motorista ent�o vai para o primeiro
		if(this->indiceMotorista == (this->qntMotorista - 1))
		{
			this->indiceMotorista = 0;
		}
		//Caso contr�rio decrementa o �ndice do motorista selecionado
		else
		{
			this->indiceMotorista++;
		}
	}
	//Verifica se a tecla "OK"
	if(tecla & Teclado::TECLA_OK)
	{
		this->finish();
		this->visible = false;
	}
}

void TelaSelecaoMotorista::start()
{
	//Limpa a tela
	this->inputOutput->display.clearScreen();
	if(((this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE)) == 0)
			&& this->fachada->getQntMotoristas() > 0){
		//Imprime as informa��es fixas da tela
		this->inputOutput->display.print(Resources::getTexto(Resources::ROTEIRO));

		this->indiceMotorista = this->fachada->getMotoristaSelecionado();
		this->qntMotorista = this->fachada->getQntMotoristas();
		this->modificado = false;
	}
}

void TelaSelecaoMotorista::finish()
{
	if((this->fachada->getStatusFuncionamento() & (Fachada::ERRO_SISTEMA_ARQUIVO | Fachada::ERRO_CONFIGURACAO_INCONSISTENTE)) == 0){
		U16 motoristaAtual = this->fachada->getMotoristaSelecionado();
		//Verifica se houve modifica��o no valor
		if(modificado)
		{
			Fachada::Resultado resultado = Fachada::SUCESSO;
			//Verifica se o motorista selecionado � diferente do atual
			if(this->indiceMotorista != motoristaAtual){
				//Indica ao usu�rio para aguardar o rein�cio da exibi��o
				this->inputOutput->display.clearScreen();
				this->inputOutput->display.print(Resources::getTexto(Resources::AGUARDE));
				//Alterar a sele��o de exibi��o
				resultado = this->fachada->setMotoristaSelecionado(this->indiceMotorista, false);
				//Verifica se esta opera��o est� bloqueada
				if(resultado == Fachada::FUNCAO_BLOQUEADA){
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
						//Se a senha est� correta ent�o realiza a opera��o for�ando o desbloqueio
						if(this->fachada->verificarSenhaAcesso(senha)){
							resultado = this->fachada->setMotoristaSelecionado(this->indiceMotorista, true);
						}
						else{
							resultado = Fachada::SENHA_INCORRETA;
						}
					}
					else{
						return;
					}
				}
			}
			//Exibe o erro
			if(resultado != Fachada::SUCESSO){
				MessageDialog::showMessageDialog(resultado, this->inputOutput, 2000);
			}
		}
	}
}

}
}
}
}
