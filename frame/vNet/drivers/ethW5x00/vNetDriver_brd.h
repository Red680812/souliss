/**************************************************************************
	Souliss - vNet Virtualized Network
    Copyright (C) 2014  Veseo

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
	
	Originally developed by Dario Di Maio
	
***************************************************************************/
/*!
    \file 
    \ingroup

*/
/**************************************************************************/
#ifndef VNET_ETHBRD_H
#define VNET_ETHBRD_H

#include "Arduino.h"
#include "GetConfig.h"						// need : ethUsrCfg.h

#include "vNetDriver_eth.h"

#define	vNet_Init_M3()						vNet_Init_M1()
#define	vNet_DataAvailable_M3()				vNet_DataAvailable_M1()
#define	vNet_RetrieveData_M3(data)			vNet_RetrieveData_M1(data)

#define	VNET_M3_HEADER		1
#define	VNET_M3_APPEND		2

uint16_t vNetM3_address=0;							// Node address for the media
uint16_t vNetM3_srcaddr=0;							// Node address from incoming frame
oFrame vNetM3_oFrame;								// Data structure for output frame

extern oFrame vNetM1_oFrame;	
extern uint8_t vNetM1_header;
extern TCPIP stack;

#if (VNET_DEBUG)
	#define VNET_LOG Serial.print
#endif
/**************************************************************************/
/*!
	Set the vNet address and all the network parameters
*/
/**************************************************************************/
#if(!VNET_MEDIA1_ENABLE)
	void vNet_SetAddress_M3(uint16_t addr)
	{
		uint8_t ip_addr[4], mac_addr[6];
		
		// Locally store the address
		vNetM3_address=addr;
		oFrame_Define(&vNetM3_oFrame);
		oFrame_Set((uint8_t*)(&vNetM3_address), 0, VNET_M3_APPEND, 0, 0);
		
		// Translate and set the address
		eth_vNettoIP(0x00FF, &ip_addr[0]);
		eth_SetIPAddress(&ip_addr[0]);
		
		// Set the MAC Address	
		#if(AUTO_MAC)
			eth_vNettoMAC(addr, mac_addr);
			W5x00.setMACAddress(mac_addr);
		#else
			W5x00.setMACAddress((uint8_t*)MAC_ADDRESS);
		#endif
		
		// Set the IP
		W5x00.setIPAddress(stack.ip);
		W5x00.setGatewayIp(stack.gateway);
		W5x00.setSubnetMask(stack.subnetmask);
		
		vNet_Begin_M1(UDP_SOCK);								// Start listen on socket

		// Include debug functionalities, if required
		#if(VNET_DEBUG)
		uint8_t addrval[6];
		
		// Print MAC address 
		W5x00.getMACAddress(addrval);
		VNET_LOG("(MAC)<");
		for(U8 i=0; i<6; i++)
		{
			VNET_LOG(addrval[i],HEX);
			VNET_LOG(",");
		}
		VNET_LOG(">\r\n");

		// Print IP address 
		W5x00.getIPAddress(addrval);
		VNET_LOG("(IP)<");
		for(U8 i=0; i<4; i++)
		{
			VNET_LOG(addrval[i],HEX);
			VNET_LOG(",");
		}
		
		VNET_LOG(">\r\n");
		#endif	
		
	}
#else
	void vNet_SetAddress_M3(uint16_t addr)
	{
		// Locally store the address
		vNetM3_address=addr;	
		oFrame_Define(&vNetM3_oFrame);
		oFrame_Set((uint8_t*)(&vNetM3_address), 0, 1, 0, 0);
	}
#endif

uint16_t vNet_GetSourceAddress_M3(){return vNetM3_srcaddr;}

/**************************************************************************/
/*!
	Send a message via UDP/IP
*/
/**************************************************************************/
uint8_t vNet_Send_M3(uint16_t addr, oFrame *frame, uint8_t len)
{
	uint8_t ip_addr[4];
	uint16_t vNet_port;
	
	// Define the standard vNet port
	vNet_port = ETH_PORT;
	
	// Set the IP broadcast address
	for(U8 i=0;i<4;i++)
		ip_addr[i]=0xFF;
		
	/***
		Add the whole length as first byte and the node address
		at the end of the frame
	***/

	// Add the node address
	oFrame_Define(&vNetM3_oFrame);
	oFrame_Set((uint8_t*)(&vNetM3_address), 0, VNET_M3_APPEND, 0, 0);
	
	// Add the length as first byte
	vNetM1_header = len+VNET_M3_HEADER+VNET_M3_APPEND;
	oFrame_Define(&vNetM1_oFrame);
	oFrame_Set(&vNetM1_header, 0, 1, 0, frame);

	// Append the address as last, this is contained into a dedicated oFrame
	oFrame_AppendLast(&vNetM3_oFrame);
	
	// Send data	
	if(!sendto(UDP_SOCK, (uint8_t*)&vNetM1_oFrame, 0, &ip_addr[0], vNet_port))
	{
		oFrame_Reset();		// Free the frame
		
		// Restart the socket
		vNet_Stop_M1(UDP_SOCK);
		vNet_Begin_M1(UDP_SOCK);
		
		return ETH_FAIL;	// If data sent fail, return
	}
	
	// At this stage data are processed or socket is failed, so we can
	// securely reset the oFrame
	oFrame_Reset();		
		
	return ETH_SUCCESS;
}

#endif
