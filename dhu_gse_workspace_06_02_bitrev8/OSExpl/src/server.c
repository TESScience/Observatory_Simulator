/******************************************************************************
*
* Copyright (C) 2009 - 2014 Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* XILINX CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/


/*******************************************************************************
	N.B. udp_send(), udp_sendto() mung the p->payload
	and p->len members possibly by adding IP headers or
	maybe lwIP artefacts to the pbuf.  The workaround is:
	pbuf_header( p, -42 ); after using those functions to
	restore the pbuf to its former state.  However, see
	savannah.nongnu.org/bugs/?func=detailitem&item_id=34475
	"raw_sendto() doesn't rewind headers in pbuf on return" "I
	think the behaviour is correct. An application is not allowed
	to reuse a pbuf after it has been passed to a send function
	(unless that function returned an error)."
******************************************************************************/

#include <stdio.h>
#include <string.h>

#include "lwip/err.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#ifdef __arm__
#include "xil_printf.h"
#endif


#define	SERVER_CMDS_PORT	5555
#define	SERVER_DATA_PORT	6666
#define	CLIENT_DATA_PORT	7777
#define	RESP_BYTES		64
#define	DATA_BYTES		128

char cmdPkt[128];
int  cmdLength;		// -1 or 0 is nothing there....

int		have_client;
uint32_t	counter;

struct udp_pcb *last_cmd_pcb_ptr;
struct pbuf *last_PB_ptr;
struct ip_addr *last_client_addr_ptr;
u16_t last_client_src_port;
struct udp_pcb *data_pcb_ptr;

int
transfer_data() {

/*	NOP function required to make lwip network stack work	*/

	return ERR_OK;
}



void print_app_header()
{
	xil_printf("\n\r\n\r-----lwIP UDP echo server ------\n\r");
	xil_printf("Receives command UDP packets at port %d\n\r", SERVER_CMDS_PORT );
	xil_printf("Sends    data    UDP packets to port %d\n\r", CLIENT_DATA_PORT  );
}



err_t
print_pbuf_info( struct pbuf *PB_ptr ) {

	/*  pbuf member info  */
	xil_printf("pbuf->len     = %d\n\r", PB_ptr->len );
	xil_printf("pbuf->tot_len = %d\n\r", PB_ptr->tot_len );
	xil_printf("pbuf->next    = %d\n\r", ( NULL != PB_ptr->next ) );

    return ERR_OK;
}

#if 0
err_t
print_payload( struct pbuf *PB_ptr,  uint16_t len ) {
	uint8_t	*payload_ptr = (uint8_t *)PB_ptr->payload;


	xil_printf( "pbuf->payload = " );
	while ( len ) {
		xil_printf( "%c", *payload_ptr );
		++payload_ptr;
		--len;
	}

	xil_printf( "\r" );	/*  cmds end with \n, but xil doesn't \r  */
	return ERR_OK ;
}



/*
    like print_payload but uses an lwip function to get the payload
    data and prints it as hex
*/



err_t
hexprint_payload( struct pbuf *pbuf_ptr,  uint16_t len ) {


	int	offset;
	for ( offset = 0; offset < pbuf_ptr->len; ++offset )
		xil_printf( "%d:%d\r\n", offset,
				pbuf_get_at( pbuf_ptr, offset ) );
	return ERR_OK;
}
#endif

err_t
send_response( struct udp_pcb *cmd_pcb_ptr, const void *cmd_resp, uint16_t resp_bytes )
{
	struct pbuf	*resp_PB_ptr;
	err_t		result;


	/*	get a packet buffer					*/
	resp_PB_ptr = pbuf_alloc( PBUF_TRANSPORT, resp_bytes, PBUF_POOL );
	if ( NULL == resp_PB_ptr ) {
		xil_printf( "send_response: pbuf_alloc failed @ %d\r\n",
				counter );
		return ERR_BUF;
	}

	/*  copy "response" data to the packet buffer's payload	*/
	result = pbuf_take (resp_PB_ptr, cmd_resp, resp_bytes );
	if ( ERR_MEM == result ) {
		xil_printf( "send_response: pbuf too small @ %d\r\n",
				counter );
		return result;
	}

	/*	send the "response" packet to the client's command port	*/
	result = udp_send( cmd_pcb_ptr, resp_PB_ptr );
	if ( ERR_OK != result ) {
		xil_printf( "send_response: udp_send failed @ %d\r\n",
				counter );
		return result;
	}

	/*	done with the packet buffer				*/
	pbuf_free( resp_PB_ptr );

	return ERR_OK;
}

#if 0
err_t
process_cmd_packet( struct udp_pcb *cmd_pcb_ptr, struct pbuf *p )
{
	static int	packet_nr = 0;
	int		i, new_packet = 1;
	uint8_t	cmd_resp[ RESP_BYTES ];
	struct pbuf	*current_pbuf = p;

	while ( new_packet ) {
		xil_printf("packet nr. %6d\n\r", ++packet_nr );
		print_pbuf_info( current_pbuf );
		print_payload( current_pbuf, current_pbuf->len ) ;
		while ( current_pbuf->len != current_pbuf->tot_len ) {
			if ( NULL == current_pbuf->next ) {
				xil_printf("process_cmd_packet:next=NULL\n\r");
				return ERR_VAL;
			}
			current_pbuf = current_pbuf->next;
			print_pbuf_info( current_pbuf );
			print_payload( current_pbuf, current_pbuf->len ) ;
		}
		new_packet = ( NULL != current_pbuf->next );
	}

	/*
	   simulate sending a command response to a client
	*/
	for (i =0; i < RESP_BYTES; ++i ){
		cmd_resp[ i ] = 'r';
	}
	cmd_resp[ RESP_BYTES - 1 ] = '\n';

	send_response( cmd_pcb_ptr, cmd_resp, RESP_BYTES );

	return ERR_OK;
}
#endif

err_t
send_data( const void *data, uint16_t data_bytes )
{
	struct pbuf	*data_PB_ptr;
	err_t		result;


	data_PB_ptr = pbuf_alloc( PBUF_TRANSPORT, data_bytes, PBUF_POOL );
	if ( NULL == data_PB_ptr ) {
		xil_printf( "send_data: pbuf_alloc failed @ %d\r\n",
				counter );
		return ERR_BUF;
	}

	result = pbuf_take( data_PB_ptr, data, data_bytes  );
	if ( ERR_MEM == result ) {
		xil_printf( "send_data: pbuf too small @ %d\r\n",
				counter );
		return result;
	}

#if 0
	xil_printf( "Sending following fake data to client:\n\r" );
	print_pbuf_info( data_PB_ptr );
	print_payload( data_PB_ptr, data_PB_ptr->len  );
	xil_printf( "\n\r" );
#endif

	result = udp_send( data_pcb_ptr, data_PB_ptr );

	if ( ERR_OK != result ) {
		xil_printf( "send_data: udp_send failed @ %d\r\n",
				counter );
		return result;
	}

	pbuf_free( data_PB_ptr );

	return ERR_OK;
}

err_t
udp_cmd_callback(void *arg,          struct udp_pcb *cmd_pcb_ptr,
		struct pbuf *PB_ptr, struct ip_addr *client_addr_ptr,
		u16_t client_src_port )
{
		uint8_t			cam_data[ DATA_BYTES ];
		err_t			result;
		u16_t			i;
		char	*msg = "udp_cmd_callback: %s udp_connect failed %d\r\n";


	if ( ! have_client ) {
		/*
		   set client addr and ports as UDP packet destinations:
		    client source port for command responses
		    fixed port number for camera data
		*/
		result = udp_connect( cmd_pcb_ptr, client_addr_ptr,
					client_src_port );
		if ( ERR_OK != result ) {
			xil_printf( msg, "command", result );
			 return result;
		}

		result = udp_connect( data_pcb_ptr, client_addr_ptr,
					CLIENT_DATA_PORT );
		if ( ERR_OK != result ) {
			xil_printf( msg, "data", result );
			return result;
		}
		last_cmd_pcb_ptr = cmd_pcb_ptr;
		last_PB_ptr = PB_ptr;
		have_client = 1;
	}

	++counter;
	//
	// Just copy out to buffer for someone else to look at...
	//
	cmdLength = sizeof(cmdPkt);
	if (PB_ptr->len < cmdLength) cmdLength = PB_ptr->len;
	memcpy(cmdPkt,PB_ptr->payload, cmdLength);
	pbuf_free( PB_ptr );
	return ERR_OK;
}

int
start_application()
{
	struct udp_pcb *cmd_pcb_ptr;
	err_t err;


	/* create and bind to this server's command port */
	cmd_pcb_ptr = udp_new();
	if ( !cmd_pcb_ptr ) {
		xil_printf("Error creating command PCB. Out of Memory\n\r");
		return ERR_MEM;
	}

	err = udp_bind(cmd_pcb_ptr, IP_ADDR_ANY, SERVER_CMDS_PORT );
	if ( ERR_OK != err ) {
		xil_printf("Can't bind to server command port %d: err = %d\n\r",
				SERVER_CMDS_PORT, err);
		return err;
	}

	/* create and bind to this server's data port */
	data_pcb_ptr = udp_new();
	if (!data_pcb_ptr) {
		xil_printf("Error creating data PCB. Out of Memory\n\r");
		return ERR_MEM;
	}

	err = udp_bind(data_pcb_ptr, IP_ADDR_ANY, SERVER_DATA_PORT);
	if ( ERR_OK !=  err ) {
		xil_printf("Can't bind to server data port %d: err = %d\n\r",
				SERVER_DATA_PORT, err);
		return err;
	}

	have_client = 0;
	counter = 0;

	udp_recv(cmd_pcb_ptr, udp_cmd_callback, NULL);

	xil_printf("UDP TESS server started @ port %d\n\r", SERVER_CMDS_PORT );

	return ERR_OK;
}
