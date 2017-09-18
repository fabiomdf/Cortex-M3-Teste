/*
 * ProtocoloCl.h
 *
 *  Created on: 22/05/2017
 *      Author: rennason.silva
 */

#include <Compiler.h>
#include <freertos.kernel/FreeRTOSKernel/FreeRTOSKernel.h>
#include <freertos.kernel.port.cortexm3/RealviewPort/portmacro.h>
#include "Plataforma.h"

namespace trf {
namespace pontos12 {
namespace application {
class Plataforma;
} /* namespace application */
} /* namespace pontos12 */
} /* namespace trf */



#ifndef PROTOCOLOCOL_H_
#define PROTOCOLOCOL_H_

namespace trf {
namespace pontos12 {
namespace application {

class ProtocoloCl {
public:
	typedef struct { //      Data | Descripci�n    									| Tama�o (bytes)
		U8 beginMark; //     0xFF | Beginning mark 									| 1
		U8 address; //      	  | Addres         									| 1
		U8 destChMark; //    0xF5 | Destination change mark 						| 1
		U8 numDest[2]; //         | Numero de destino  (servicio sentido) (0-999)   | 2
		U8 extraChMark; //   0xFA | Extra change mark 								| 1
		U8 extraNumber[2]; //     | Extra number (0-999) 							| 2
		U8 checkSum; //           | Checksum       									| 1 or 2
		U8 endMark; //       0xFF | End mark       									| 1
	} PacketRS485Cl;

	typedef struct {
			U16 roteiro;
			bool volta;
		} RoteiroComp;

	ProtocoloCl(Plataforma* plataforma);
	void init();
	//void receiveRoute(char *roteiro);
	void receiveRoute(RoteiroComp *roteiroComp);
	//void sendRoute(char *roteiro);
	void sendRoute(RoteiroComp *roteiroComp);
	U32 getTimerCl();
	void updateTimerCl();

private:
	PacketRS485Cl dado;
	Plataforma* plataforma;
	portTickType timerCl;
	xSemaphoreHandle semaforoCl;//semaforo para o envio via RS485
};

} /* namespace application */
} /* namespace pontos12 */
} /* namespace trf */

#endif /* PROTOCOLOCOL_H_ */
