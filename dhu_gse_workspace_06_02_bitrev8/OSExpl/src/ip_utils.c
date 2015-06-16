/*
 * ip_utils.c
 *
 *  Created on: Apr 8, 2015
 *      Author: jal
 */

#include "platform.h"
#include "xparameters.h"
#include <xbram.h>
#include <xbram_hw.h>
#include <xil_io.h>
#include <xil_printf.h>
#include <xil_types.h>

#include "lwip/ip_addr.h"
#include "lwip/opt.h"
#include "lwip/def.h"

void
print_ip(char *msg, struct ip_addr *ip)
{
	xil_printf("%s ", msg);
	xil_printf("%d.%d.%d.%d\n\r", ip4_addr1(ip), ip4_addr2(ip),
			ip4_addr3(ip), ip4_addr4(ip));
}




void
print_ip_settings(struct ip_addr *ip, struct ip_addr *mask, struct ip_addr *gw)
{

	print_ip("Board IP: ", ip);
	print_ip("Netmask : ", mask);
	print_ip("Gateway : ", gw);
}


