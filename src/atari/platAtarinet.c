/*
 *  platAtarinet.c
 *  RetroMate
 *
 *  By S. Wessels and O. Schmidt, 2025.
 *  This is free and unencumbered software released into the public domain.
 *
 */

#include <atari.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "../global.h"

#include "platAtari.h"

#include "fujinet-network.h"

#pragma bss-name( push, "FUJI_BSS")
char devicespec[64];
char rxbuf[1518]; // ip_65 eth_buffer.s .res 1518 &  drivers/ethernetcombo.s drivers/ethernet.s stax #1518
uint16_t bytes_waiting;
uint8_t conn_status;
uint8_t err;
uint8_t tick;
uint8_t res;
int16_t bytes_read;
#pragma bss-name( pop )



char* fn_strerror( uint8_t e ) {
    switch (e)
    {
        case FN_ERR_OK:
            return "OK";
        case FN_ERR_IO_ERROR:
            return "IO Error";
        case FN_ERR_BAD_CMD:
            return "bad cmd";
        case FN_ERR_OFFLINE:
           return "offline";
        case FN_ERR_WARNING:
           return "Warning issued";
        case FN_ERR_NO_DEVICE:
           return "no device";
        default:
            return "Unknown";
    }
}

/*-----------------------------------------------------------------------*/
static int plat_net_make_ascii(const char *text) {
    char i = 0;
    while (*text) {
        char c = *text++;
        if (c == 0x9b) {
            atari.send_buffer[i++] = 0x0a;
        } else {
            atari.send_buffer[i++] = c;
        }
    }
    // This seems like a good idea but it locks the Atari up.
    // atari.send_buffer[i++] = '\x0a';
    return i;
}

/*-----------------------------------------------------------------------*/
void plat_net_init() {
    res = network_init();

    (*(uint8_t*)0x41) = 0; // Quiet you.
     
    if ( res ) { // returns status/error FN_ERR_* values  FN_ERR_OK ==0x00, so non zero is error
        log_add_line(&global.view.terminal, "Init Network", -1);
        plat_draw_log(&global.view.terminal, 0, 0, false);
        app_error(true, fn_strerror(res));
    }
}


/*-----------------------------------------------------------------------*/
void plat_net_connect(const char *server_name, int server_port) {
    
    //sprintf( devicespec, "N:TELNET://%s:%d/", server_name, server_port ); sprintf() adds ~1.5K to size.
    strcpy( devicespec, "N:TELNET://");
    strcpy( devicespec+11, server_name );
    strcpy( devicespec+11 + strlen(server_name), ":" );
    itoa( server_port,  devicespec+12 + strlen(server_name), 10 );

    log_add_line(&global.view.terminal, "Connect to server", -1);
    plat_draw_log(&global.view.terminal, 0, 0, false);

    res = network_open( devicespec, OPEN_MODE_RW, OPEN_TRANS_NONE );
    if( res ) {
        app_error(true, fn_strerror(res));
    }
    log_add_line(&global.view.terminal, "Logging in", -1);
    plat_draw_log(&global.view.terminal, 0, 0, false);
}

/*-----------------------------------------------------------------------*/
void plat_net_disconnect() {
    network_close(devicespec);
}

/*-----------------------------------------------------------------------*/
bool plat_net_update() {
    // throttle SIO reads (interferes with keyboard)
    tick++;
    if( tick % 120 ) return 0;

    // network status gets bytes_waiting from DVSTAT, conn_status from DVSTAT+2
    // and actual error code from DVSTAT+3 ( atari/src/fn_network/network_status.s)
    if( network_status( devicespec, &bytes_waiting, &conn_status, &err ) == FN_ERR_OK ) {
        // https://github.com/FujiNetWIFI/fujinet-firmware/wiki/N%3A-SIO-Command-%27S%27---Status
        // 0	LO Byte of # of bytes waiting
        // 1	HI Byte of # of bytes waiting
        // 2	0=Disconnected, 1=Connected
        // 3	Extended Error code
        if( conn_status  ){
            if(  bytes_waiting ) {
                bytes_read = network_read( devicespec, rxbuf, bytes_waiting < sizeof( rxbuf ) ? bytes_waiting : sizeof( rxbuf ) );
                if( bytes_read < 0 ) {
                    return 1;
                }
                if( bytes_read > 0 ) {
                  fics_tcp_recv( rxbuf, bytes_read ); 
                }
                return 0;
            }
        }
    }
    // Got an error if we're here. network_status returns either FN_ERR_OK or FN_ERR_IO_ERROR.
    return 1;
}

/*-----------------------------------------------------------------------*/
void plat_net_send(const char *text) {
  log_add_line(&global.view.terminal, text, -1);
  /*
   *
   * Keeping this block to document 
   *
     tcp_send((unsigned char *)atari.send_buffer, plat_net_make_ascii(text));
    // Don't send \x0a in he buffer, send seprately.  I don't know why it
    // improves stability, but it absolutely does
    tcp_send((unsigned char *)"\x0a", 1);
  */

  network_write( devicespec, (unsigned char *)atari.send_buffer, plat_net_make_ascii(text));
  network_write( devicespec, (unsigned char *)"\x0a", 1 );
}

/*-----------------------------------------------------------------------*/
void plat_net_shutdown() {
  plat_net_disconnect();
}

