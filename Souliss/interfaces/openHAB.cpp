/**************************************************************************
	Souliss 
    Copyright (C) 2012  Veseo

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
	
	Originally developed by Fulvio Spelta and Dario Di Maio
	
***************************************************************************/
/*!
    \file 
    \ingroup
*/

#include "Typicals.h"
#include "Souliss.h"
#include "frame/MaCaco/MaCaco.h"
#include "frame/vNet/vNet.h"

#include "openHAB.h"

#if(OPENHAB && VNET_MEDIA1_ENABLE && ( ETH_W5100 || ETH_W5200))

#include "ASCIItools.c"

String incomingURL = String(HTTP_REQBYTES);			// The GET request is stored in incomingURL
char buf[HTTP_BUFBYTES];							// Used for temporary operations
uint8_t i, *buff = (uint8_t *)buf;
uint8_t indata=0, bufferlen, data_len;				// End of HTML request

// Send JSON string
const char* xml[] = {"<s", ">", "</s", "<id", "</id"};

/**************************************************************************
/*!
	Init the interface
*/	
/**************************************************************************/
void openHABInit()
{
	// Set an internal subscription in order to collect data from other
	// nodes in the network
	
	MaCaco_InternalSubcription();
}

/**************************************************************************
/*!
	Parse incoming HTTP GET for incoming commands
*/	
/**************************************************************************/
void openHABInterface(U8 *memory_map)
{
	// Set the socket
	srvcln_setsocket(SRV_SOCK1);
	
	// Look for available data on the listening port
	data_len = srvcln_dataavailable(HTTPPORT);
	
	// If there are incoming data on the listened port
	if(data_len) 
	{	
		// If the socket is connected and the data size doesn't exceed the buffer
		if (srvcln_connected(HTTPPORT))													// If data are available
		{
			// Include debug functionalities, if required
			#if(OPENHAB_DEBUG)
				OPENHAB_LOG("(HTTP/XML)Indata <0x");
				OPENHAB_LOG(data_len,HEX);
				OPENHAB_LOG(">\r\n");		
			#endif
			
			// Retrieve data from the Ethernet controller
			if (data_len > HTTP_REQBYTES) 
				data_len = HTTP_REQBYTES;
			
			// Include debug functionalities, if required
			#if(OPENHAB_DEBUG)
				OPENHAB_LOG("(HTTP/XML)Trunked Indata <0x");
				OPENHAB_LOG(data_len,HEX);
				OPENHAB_LOG(">\r\n");		
			#endif
			
			srvcln_retrieve(buff, data_len);	
			
			// Include debug functionalities, if required
			#if(OPENHAB_DEBUG)
				OPENHAB_LOG("(HTTP/XML)Instring<");
				for(i=0;i<data_len;i++)
					OPENHAB_LOG(buf[i]);

				OPENHAB_LOG(">\r\n");					
			#endif
			
			// Move data into a string for parsing
			incomingURL = "";
			for(i=0;i<data_len;i++)						
				incomingURL = incomingURL + buf[i];	
		
			// Flag the incoming data and quit
			indata=1;
		}	
		else
			srvcln_stop();				// Stop the socket, it will be restarted at next iteration
	}	
	
	// Parse the incoming data
	if(indata)
    {				
		// Send HTTP Hello	
		char* str[] = {"HTTP/1.1 200 OK\n\rContent-Type: text/plain\n\r\n\r"};
		srvcln_send((uint8_t*)str[0], strlen(str[0])); 
	
		// Handler1, if /force is requested from client	
		// an example command is "GET /force?id=4&slot=2&val=1 "
		if(incomingURL.startsWith("GET /force") || ((incomingURL.indexOf("GET /force",0) > 0)))
		{			
			// Include debug functionalities, if required
			#if(OPENHAB_DEBUG)
				OPENHAB_LOG("(HTTP/XML)<GET /force");
				OPENHAB_LOG(data_len,HEX);
				OPENHAB_LOG(">\r\n");		
			#endif		
		
			// Find start and end index for callback request
			U8 force  = incomingURL.indexOf("force",0);
			if(incomingURL.indexOf("?id=",force) > 0)
			{						
				U8 id   = incomingURL.indexOf("?id=",force);		
				U8 slot = incomingURL.indexOf("&slot=", id);		
				U8 val_s  = incomingURL.indexOf("&val=", slot);		
				id = incomingURL.substring(id+4, slot).toInt();				// Sum lenght of "?id="
				slot = incomingURL.substring(slot+6, val_s).toInt();		// Sum lenght of "&slot="
								
				// This buffer is used to load values
				U8 vals[MAXVALUES];
				for(i=0;i<MAXVALUES;i++)	vals[i]=0;
				
				// Identify how many values (up to 5) are provided
				if(incomingURL.indexOf(",", val_s))
				{
					// Buffer used to store offset of values
					U8 valf[MAXVALUES];
					for(i=0;i<MAXVALUES;i++)	valf[i]=0;
					
					// Get the offset of all values
					valf[0]  = incomingURL.indexOf(",", val_s);
					for(i=1;i<MAXVALUES;i++)
						valf[i]  = incomingURL.indexOf(",", valf[i-1]+1);
					
					vals[0] = incomingURL.substring(val_s+5, valf[0]).toInt();			// Sum lenght of "&val="						
					for(i=1;i<MAXVALUES;i++)
						vals[i] = incomingURL.substring(valf[i-1]+1, valf[i]).toInt();	// Sun the lenght of ","
				}
				else
				{		
					// Just one values
					U8 val_f  = incomingURL.indexOf(" ", val_s);					// Command should end with a space	
					
					// Convert to number
					id = incomingURL.substring(id+4, slot).toInt();				// Sum lenght of "?id="
					slot = incomingURL.substring(slot+6, val_s).toInt();		// Sum lenght of "&slot="
					vals[0] = incomingURL.substring(val_s+5, val_f).toInt();	// Sum lenght of "&val="						
				}
			
			
			#if(OPENHAB_DEBUG)
				OPENHAB_LOG("(HTTP/XML)<GET /force");
				OPENHAB_LOG(">\r\n");		
				
				OPENHAB_LOG("(HTTP/XML)<id=");
				OPENHAB_LOG(id,DEC);
				OPENHAB_LOG(">\r\n");		
				
				OPENHAB_LOG("(HTTP/XML)<slot=");
				OPENHAB_LOG(slot,DEC);
				OPENHAB_LOG(">\r\n");		
				
				OPENHAB_LOG("(HTTP/XML)<val=");
				OPENHAB_LOG(val_s,DEC);
				OPENHAB_LOG(">\r\n");		
			
				for(i=0;i<MAXVALUES;i++)
				{				
					OPENHAB_LOG(vals[i]);
					OPENHAB_LOG(">\r\n");		
				}
				OPENHAB_LOG("id=");
				OPENHAB_LOG(id);
				OPENHAB_LOG(">\r\n");		
				OPENHAB_LOG("slot=");
				OPENHAB_LOG(slot);
				OPENHAB_LOG(">\r\n");		
			#endif				
			
				// Send a command to the node	
				if((id < MaCaco_NODES) && (id != MaCaco_LOCNODE) && (*(U16 *)(memory_map + MaCaco_ADDRESSES_s + 2*id) != 0x0000))	// If is a remote node, the command act as remote input				
					MaCaco_send(*(U16 *)(memory_map + MaCaco_ADDRESSES_s + 2*id), MaCaco_FORCEREGSTR, 0x00, MaCaco_IN_s + slot, MAXVALUES, vals);		
				else if (id == MaCaco_LOCNODE)								// If is a local node (me), the command is written back
				{
					i = 0;
					while((vals[i] != 0) && (i < MAXVALUES))
					{
						#if(OPENHAB_DEBUG)
							OPENHAB_LOG(slot);
							OPENHAB_LOG(" ");
							OPENHAB_LOG(vals[i]);
							OPENHAB_LOG(">\r\n");
						#endif		
						memory_map[MaCaco_IN_s+slot] = vals[i];
						slot++;
						i++;
					}
				}					
			}
			else if(incomingURL.indexOf("?typ=",force) > 0)
			{
				U8 typ_mask;
				U8 typ   = incomingURL.indexOf("?typ=",force);
				U8 val_s  = incomingURL.indexOf("&val=", typ);		
				U8 val_f  = incomingURL.indexOf(" ", val_s);				// Command should end with a space	
				typ = incomingURL.substring(typ+5, val_s).toInt();			// Sum lenght of "?typ="
				val_s = incomingURL.substring(val_s+5, val_f).toInt();		// Sum lenght of "&val="								

			#if(OPENHAB_DEBUG)
				OPENHAB_LOG("(HTTP/XML)<GET /typ");
				OPENHAB_LOG(">\r\n");		
			
				OPENHAB_LOG("(HTTP/XML)<val=");
				OPENHAB_LOG(val_s,DEC);
				OPENHAB_LOG(">\r\n");		
			#endif		
				
				U8* val_sp = &val_s;
				
				// Send a multicast command to all typicals, first identify if the command is issued
				// for a typical or a typical class
				if((typ & 0x0F) == 0x00)
					typ_mask = 0xF0;	// We look only to the typical class value
				else
					typ_mask = 0xFF;	// We look to whole typical value
				
				// Look for all slot assigned to this typical and put value in
				for(U8 id=0;id<MaCaco_NODES;id++)
				{						
					// Send a command to the node	
					if((id != MaCaco_LOCNODE) && ((*(U16 *)(memory_map + MaCaco_ADDRESSES_s + 2*id)) != 0x0000))	// If is a remote node, the command act as remote input								
						MaCaco_send(*(U16 *)(memory_map + MaCaco_ADDRESSES_s + 2*id), MaCaco_TYP, 0, typ, 1, val_sp);		
					else if (id == MaCaco_LOCNODE)																	// If is a local node (me), the command is written back
					{
						U8 typ_mask;
						
						// Identify if the command is issued for a typical or a typical class
						if((typ & 0x0F) == 0x00)
							typ_mask = 0xF0;	// we look only to the typical class value
						else
							typ_mask = 0xFF;	// we look to whole typical value
					
						for(i=0; i<MaCaco_SLOT; i++)		
							if((*(memory_map + MaCaco_TYP_s + i) & typ_mask) == typ)	// Start offset used as typical logic indication
								*(memory_map+MaCaco_IN_s + i) = val_s;
					}
				}
			}
		}	

		// Handler2, if /statusrequested from client	
		// an example command is "GET /status?id=0 "
		if(incomingURL.startsWith("GET /status") || ((incomingURL.indexOf("GET /status",0) > 0)))
		{
			U8 id_for=0;
			
			// Init the buffer
			bufferlen=0;
						
			// Find start and end index request
			U8 status  = incomingURL.indexOf("status",0);
			if(incomingURL.indexOf("?id=",status) > 0)
			{						
				U8 id   = incomingURL.indexOf("?id=",status);		
				U8 id_f  = incomingURL.indexOf(" ", id);						// Command should end with a space	
				
				// If one ID is requested
				id_for    = incomingURL.substring(id+4, id_f).toInt();			// Sum lenght of "?id="
			}					
	
		#if(!MaCaco_PERSISTANCE)
			// No support for other nodes is active	
			id_for=0;
		#endif
	
		// The request has a wrong id value
		if(!(id_for<MaCaco_NODES))
			return;	
	
		#if(OPENHAB_DEBUG)
			OPENHAB_LOG("(HTTP/XML)<GET /status");
			OPENHAB_LOG(">\r\n");		
								
			OPENHAB_LOG("(HTTP/XML)<id=");
			OPENHAB_LOG(id_for,DEC);
			OPENHAB_LOG(">\r\n");		
		#endif			
	
			// Print "<id"
			memmove(&buf[bufferlen],xml[3],strlen(xml[3]));
			bufferlen += strlen(xml[3]);

			// Print id number
			*(unsigned long*)(buf+bufferlen) = (unsigned long)id_for;
			ASCII_num2str((uint8_t*)(buf+bufferlen), DEC, &bufferlen);						
			
			// Print ">"
			memmove(&buf[bufferlen],xml[1],strlen(xml[1]));
			bufferlen += strlen(xml[1]);
			
			for(U8 slot=0;slot<MaCaco_SLOT;slot++)
			{
				if( memory_map[MaCaco_G_TYP_s+(id_for*MaCaco_TYPLENGHT)+slot] != 0x00 )	
				{
					// Print "<s"
					memmove(&buf[bufferlen],xml[0],strlen(xml[0]));
					bufferlen += strlen(xml[0]);
					
					// Print slot number
					*(unsigned long*)(buf+bufferlen) = (unsigned long)slot;
					ASCII_num2str((uint8_t*)(buf+bufferlen), DEC, &bufferlen);						
					
					// Print ">"
					memmove(&buf[bufferlen],xml[1],strlen(xml[1]));
					bufferlen += strlen(xml[1]);

					// Typicals in group Souliss_T5n need convertion from half-precision float to string
					if((memory_map[MaCaco_G_TYP_s+(id_for*MaCaco_TYPLENGHT)+slot] & 0xF0 ) == Souliss_T5n)	
					{
						float f_val;
						
						float32((U16*)(memory_map+(MaCaco_G_OUT_s+(id_for*MaCaco_OUTLENGHT)+slot)), &f_val);
						ASCII_float2str((float)f_val, 2, &buf[bufferlen], HTTP_BUFBYTES);
						bufferlen = strlen(buf); 
					} 
					else // All others values are stored as unsigned byte
					{
						// Print "val" value
						*(unsigned long*)(buf+bufferlen) = (unsigned long)memory_map[MaCaco_G_OUT_s+(id_for*MaCaco_OUTLENGHT)+slot];
						ASCII_num2str((uint8_t*)(buf+bufferlen), DEC, &bufferlen);						
					}

					// Print "</slot"
					memmove(&buf[bufferlen],xml[2],strlen(xml[2]));
					bufferlen += strlen(xml[2]);
					
					// Print slot number
					*(unsigned long*)(buf+bufferlen) = (unsigned long)slot;
					ASCII_num2str((uint8_t*)(buf+bufferlen), DEC, &bufferlen);						
					
					// Print ">"
					memmove(&buf[bufferlen],xml[1],strlen(xml[1]));
					bufferlen += strlen(xml[1]);
				}
			}

			// Print "</id"
			memmove(&buf[bufferlen],xml[4],strlen(xml[4]));
			bufferlen += strlen(xml[4]);

			// Print id number
			*(unsigned long*)(buf+bufferlen) = (unsigned long)id_for;
			ASCII_num2str((uint8_t*)(buf+bufferlen), DEC, &bufferlen);						
			
			// Print ">"
			memmove(&buf[bufferlen],xml[1],strlen(xml[1]));
			bufferlen += strlen(xml[1]);
									
			// The frame is now assembled, send the data					
			srvcln_send((uint8_t*)buf, bufferlen); 
		}

		// Handler3, if /statusrequested from client	
		// an example command is "GET /typicals?id=0 "
		if(incomingURL.startsWith("GET /typicals") || ((incomingURL.indexOf("GET /typicals",0) > 0)))
		{
			U8 id_for=0;
			
			// Init the buffer
			bufferlen=0;
						
			// Find start and end index request
			U8 status  = incomingURL.indexOf("typicals",0);
			if(incomingURL.indexOf("?id=",status) > 0)
			{						
				U8 id   = incomingURL.indexOf("?id=",status);		
				U8 id_f  = incomingURL.indexOf(" ", id);						// Command should end with a space	
				
				// If one ID is requested
				id_for    = incomingURL.substring(id+4, id_f).toInt();			// Sum lenght of "?id="
			}					
	
		#if(!MaCaco_PERSISTANCE)
			// No support for other nodes is active	
			id_for=0;
		#endif
	
		#if(OPENHAB_DEBUG)
			OPENHAB_LOG("(HTTP/XML)<GET /typicals");
			OPENHAB_LOG(">\r\n");		
								
			OPENHAB_LOG("(HTTP/XML)<id=");
			OPENHAB_LOG(id_for,DEC);
			OPENHAB_LOG(">\r\n");		
		#endif			
	
			// The request has a wrong id value
			if(!(id_for<MaCaco_NODES))
				return;
		
			// Print "<id"
			memmove(&buf[bufferlen],xml[3],strlen(xml[3]));
			bufferlen += strlen(xml[3]);

			// Print id number
			*(unsigned long*)(buf+bufferlen) = (unsigned long)id_for;
			ASCII_num2str((uint8_t*)(buf+bufferlen), DEC, &bufferlen);						
			
			// Print ">"
			memmove(&buf[bufferlen],xml[1],strlen(xml[1]));
			bufferlen += strlen(xml[1]);
			
			for(U8 slot=0;slot<MaCaco_SLOT;slot++)
			{
				if( memory_map[MaCaco_G_TYP_s+(id_for*MaCaco_TYPLENGHT)+slot] != 0x00 )	
				{
					// Print "<s"
					memmove(&buf[bufferlen],xml[0],strlen(xml[0]));
					bufferlen += strlen(xml[0]);
					
					// Print slot number
					*(unsigned long*)(buf+bufferlen) = (unsigned long)slot;
					ASCII_num2str((uint8_t*)(buf+bufferlen), DEC, &bufferlen);						
					
					// Print ">"
					memmove(&buf[bufferlen],xml[1],strlen(xml[1]));
					bufferlen += strlen(xml[1]);

					// Print "val" value
					*(unsigned long*)(buf+bufferlen) = (unsigned long)memory_map[MaCaco_G_TYP_s+(id_for*MaCaco_TYPLENGHT)+slot];
					ASCII_num2str((uint8_t*)(buf+bufferlen), HEX, &bufferlen);						

					// Print "</slot"
					memmove(&buf[bufferlen],xml[2],strlen(xml[2]));
					bufferlen += strlen(xml[2]);
					
					// Print slot number
					*(unsigned long*)(buf+bufferlen) = (unsigned long)slot;
					ASCII_num2str((uint8_t*)(buf+bufferlen), DEC, &bufferlen);						
					
					// Print ">"
					memmove(&buf[bufferlen],xml[1],strlen(xml[1]));
					bufferlen += strlen(xml[1]);
				}
			}

			// Print "</id"
			memmove(&buf[bufferlen],xml[4],strlen(xml[4]));
			bufferlen += strlen(xml[4]);

			// Print id number
			*(unsigned long*)(buf+bufferlen) = (unsigned long)id_for;
			ASCII_num2str((uint8_t*)(buf+bufferlen), DEC, &bufferlen);						
			
			// Print ">"
			memmove(&buf[bufferlen],xml[1],strlen(xml[1]));
			bufferlen += strlen(xml[1]);
									
			// The frame is now assembled, send the data					
			srvcln_send((uint8_t*)buf, bufferlen); 
		}
		
		// Close the connection
		indata=0;
		srvcln_stop();
	}		
}

#endif
