/*
 * Resources.cpp
 *
 *  Created on: 10/05/2013
 *      Author: gustavo
 */

#include "Resources.h"
#include <escort.util/String.h>
#include "../files/Idioma.h"

using escort::util::String;


namespace trf {
namespace pontos12 {
namespace application {
namespace gui {

escort::service::NandFFS* Resources::fileSystem;
char Resources::idiomaPath[64];
char Resources::buffer[33];
char* Resources_textos[] = {
		"  RECONHECENDO  ",
		"DISPOSITIVO USB ",
		"E001 DISPOSITIVO",
		"IMCOMPAT�VEL    ",
		"E002 FUN��O     ",
		"BLOQUEADA       ",
		"AGUARDE         ",
		"    COLOQUE O   ",
		"    PENDRIVE    ",
		"VERS�O          ",
		"ROTEIROS        ",
		"MENSAGENS       ",
		"NUMERO DE S�RIE ",
		"PAINEL          ",
		"AJUSTAR PARTIDA ",
		"SENTIDO         ",
		"IDA             ",
		"VOLTA           ",
		"EMPARELHANDO    ",
		"E003 FALHA      ",
		"     SUCESSO    ",
		"                ",
		"NOVA CONFIG.    ",
		"AJUSTAR RELOGIO ",
		"SELEC. REGI�O   ",
		"CONFIGURA��ES   ",
		"EMPARELHAR      ",
		"COLETAR DUMP    ",
		"FORMAT. PENDRIVE",
		"DOMSEGTERQUAQUI ",
		"SEXSAB          ",
		"MENSAGEM        ",
		"SELECIONA PAINEL",
		"ROTEIRO         ",
		"Autom�tico?     ",
		"                ",
		"PAINEIS         ",
		"Tem certeza em  ",
		"formatar?       ",
		"ATUALIZANDO     ",
		"REMOVA PENDRIVE ",
		"    RESETANDO   ",
		"E004 Carregue   ",
		"nova config.    ",
		"APAGAR ARQUIVOS ",
		"SINCRONIZANDO   ",
		"E005 Painel xx  ",
		"n�o detectado   ",
		"OP��ES AVAN�ADAS",
		"COLETANDO       ",
		"COLETAR LOG     ",
		"E006 Painel xx  ",
		"est� com defeito",
		"SIM     N�O     ",
		"Desbloquear a   ",
		"fun��o?         ",
		"SENHA           ",
		"E007 SENHA      ",
		"INCORRETA       ",
		"CONTROLADOR     ",
		"TODOS DISP.     ",
		"TODOS PAINEIS   ",		
		"E008 PRECISA    ",
		"FORMATAR MEM�RIA",
		"ACENDER PAINEIS ",
		"E009 SENHA ANTI ",
		"FURTO INCORRETA ",
		"SA�DA           ",
		"DOM             ",
		"SEG             ",
		"TER             ",
		"QUA             ",
		"QUI             ",
		"SEX             ",
		"S�B             ",
		"CONFIG. F�BRICA ",
		"E010 FALHA DE   ",
		"COMUNICA��O     ",
		"E011 NOVA CONFIG",
		"INV�LIDA        ",
		"E012 EMPARELHAM.",
		"INCOMPAT�VEL    ",
		"  TEMPO ACESO   ",
		" TEMPO APAGADO  ",
		"INDEFINIDAMENTE ",
		"E013 NSS n�o foi",
		"detectado       ",
		"nenhum arquivo  ",
		"MSG SECUND�RIA  ",
		"MSG PRINCIPAL   ",
		"Nomeie a pasta: ",
		"Autom�tico      ",
		"Linhas          ",
		"Colunas         ",
		"Apagado         ",
		"Aceso           ",
		"Texto           ",
		"Desligar teste  ",
		"MODO TESTE      ",
		"      APP       ",
		"AJUSTAR BRILHO  ",
		"SELEC. MOTORISTA",
		"PR�XIMA PARADA  ",
		"MENSAGEM NSS    ",		
		"E014 PAINEIS    ",
		"TRAVADOS        ",
		"DESTRAV. PAINEIS",
		"ID DESTRAVAMENTO",
		"TESTE TECLADO   ",
		"Roteiro Esquerda",
		"Roteiro Direita ",
		"Ida / Volta     ",
		"Alterna Roteiro ",
		"Msg esquerda    ",
		"Msg Direita     ",
		"Seleciona Painel",
		"Ajustes Esquerda",
		"Ajustes Direita ",
		"Comece o teste  ",
		"Pressione OK    ",
		"APP DEMONSTRACAO",
		"   ATUALIZAR    ",
		"FIRMWARE?       "
};

void Resources::init(escort::service::NandFFS* fileSystem, char* idiomaPath)
{
	Resources::fileSystem = fileSystem;
	setIdiomaPath(idiomaPath);
}

void Resources::setIdiomaPath(char* idiomaPath)
{
	String::strcpy(Resources::idiomaPath, idiomaPath, 0, sizeof(Resources::idiomaPath));
}

char* Resources::getTexto(Texto texto)
{
	U32 indiceTexto = texto;
	Resources::buffer[0] = 0;

	if(Resources::fileSystem->wasInitiated()){
		Idioma idioma(Resources::fileSystem);
		if(idioma.open(Resources::idiomaPath) == 0){
			//Copia o texto de 16 bytes
			idioma.readTexto(*(Idioma::TextoID*)&indiceTexto, Resources::buffer);
			Resources::buffer[16] = 0;
			//Verifica se o texto � de 32 bytes
			if(		texto == RECONHECENDO_USB
				|| 	texto == DISPOSITIVO_INCOMPATIVEL
				||	texto == FUNCAO_BLOQUEADA
				||	texto == COLOQUE_PENDRIVE
				||	texto == SUCESSO
				||	texto == AUTOMATICO
				|| 	texto == DESEJA_FORMATAR
				|| 	texto == COLOQUE_PENDRIVE
				||	texto == CARREGUE_CONFIG
				||  texto == PAINEL_NAO_DETECTADO
				||  texto == PAINEL_COM_DEFEITO
				||	texto == DESEJA_DESBLOQUEAR_FUNCAO
				||	texto == SENHA_INCORRETA
				||	texto == SISTEMA_ARQUIVO_DEFEITUOSO
				||	texto == SENHA_ANTI_FURTO_INCORRETA
				||  texto == FALHA_COMUNICACAO
				||  texto == NOVA_CONFIG_INVALIDA
				||  texto == EMPARELHAMENTO_INCOMPATIVEL
				||  texto == APP_NAO_DETECTADO
				||  texto == PAINEIS_TRAVADOS
				||	texto == DESEJA_ATUALIZAR_FW)
			{
				indiceTexto++;
				idioma.readTexto(*(Idioma::TextoID*)&indiceTexto, Resources::buffer + 16);
				Resources::buffer[32] = 0;
			}

			idioma.close();
		}
	}

	//Verifica se n�o conseguiu obter o texto
	if(Resources::buffer[0] == 0){
		indiceTexto = texto;
		//Copia o texto de 16 bytes
		String::strcpy(Resources::buffer, Resources_textos[indiceTexto], 0, 16);
		Resources::buffer[16] = 0;
		//Verifica se o texto � de 32 bytes
		if(		texto == RECONHECENDO_USB
			|| 	texto == DISPOSITIVO_INCOMPATIVEL
			||	texto == FUNCAO_BLOQUEADA
			||	texto == AUTOMATICO
			||	texto == COLOQUE_PENDRIVE
			||	texto == SUCESSO
			|| 	texto == DESEJA_FORMATAR
			|| 	texto == COLOQUE_PENDRIVE
			||	texto == CARREGUE_CONFIG
			||  texto == PAINEL_NAO_DETECTADO
			||  texto == PAINEL_COM_DEFEITO
			||	texto == DESEJA_DESBLOQUEAR_FUNCAO
			||	texto == SENHA_INCORRETA
			||	texto == SISTEMA_ARQUIVO_DEFEITUOSO
			||	texto == SENHA_ANTI_FURTO_INCORRETA
			||  texto == FALHA_COMUNICACAO
			||  texto == NOVA_CONFIG_INVALIDA
			||  texto == EMPARELHAMENTO_INCOMPATIVEL
			||  texto == APP_NAO_DETECTADO
			||  texto == PAINEIS_TRAVADOS
			||	texto == DESEJA_ATUALIZAR_FW)
		{
			indiceTexto++;
			String::strcpy(Resources::buffer, Resources_textos[indiceTexto], 16, 16);
			Resources::buffer[32] = 0;
		}
	}

	int length = String::getLength(Resources::buffer);
	if(length < 16){
		for(int i = length; i < 16; i++)Resources::buffer[i] = ' ';
	}
	else if(length > 16 && length < 32){
		for(int i = length; i < 32; i++)Resources::buffer[i] = ' ';
	}

	return Resources::buffer;
}

}
}
}
}

