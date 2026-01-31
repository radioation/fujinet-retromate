/*
 *  platC64net.c
 *  RetroMate
 *
 *  By S. Wessels and O. Schmidt, 2025.
 *  This is free and unencumbered software released into the public domain.
 *
 */

#include <c64.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "../global.h"

#include "platC64.h"

#include "fujinet-network.h"

char devicespec[64];
char rxbuf[1518]; // ip_65 eth_buffer.s .res 1518 &  drivers/ethernetcombo.s drivers/ethernet.s stax #1518
uint16_t bytes_waiting;
uint8_t conn_status;
uint8_t err;
uint8_t tick;
uint8_t res;
int16_t bytes_read;

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
        char c = *text;
        if(c == 0x0d) { // \r to \n
            c64.send_buffer[i++] = 0x0a;
        } else if (c < 219 && c >= 32) {    // ignore too small and big
            if(c >= 193) { // Petscii 'A-Z' to Ascii 'A-Z'
               c64.send_buffer[i++] = c & ~128;
            } else if(c < 123) { // Ignore 123 - 192
                if(c >= 91 || c < 65) {  // pass all (that remains), but 'a-z', as-is
                    c64.send_buffer[i++] = c;
                } else {    // Petscii 'a-z' to Ascii 'a-z'
                    c64.send_buffer[i++] = c | 32;
                }
            }
        }
        text++;
    }
    c64.send_buffer[i++] = '\x0a';
    return i;
}


/*-----------------------------------------------------------------------*/
void plat_net_init() {
    res = network_init();

    if ( res ) { // returns status/error FN_ERR_* values  FN_ERR_OK ==0x00, so non zero is error
        log_add_line(&global.view.terminal, "Init Network", -1);
        plat_draw_log(&global.view.terminal, 0, 0, false);
        app_error(true, fn_strerror(res));
    }
}


/*-----------------------------------------------------------------------*/
void plat_net_connect(const char *server_name, int server_port) {
    strcpy( devicespec, "N:TELNET://");
    strcat( devicespec, server_name );
    strcat( devicespec, ":" );
    itoa( server_port,  devicespec + strlen(devicespec), 10 );

    log_add_line(&global.view.terminal, "Connect to server", -1);
    log_add_line(&global.view.terminal, devicespec, -1);
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
    if( network_status( devicespec, &bytes_waiting, &conn_status, &err ) == FN_ERR_OK ) {
        if( conn_status && bytes_waiting ) {
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
    // Got an error if we're here. network_status returns either FN_ERR_OK or FN_ERR_IO_ERROR.
    return 1;
}

/*-----------------------------------------------------------------------*/
void plat_net_send(const char *text) {
    log_add_line(&global.view.terminal, text, -1);
    //tcp_send((unsigned char *)c64.send_buffer, plat_net_make_ascii(text));
    network_write( devicespec, (unsigned char *)c64.send_buffer, plat_net_make_ascii(text));
}

/*-----------------------------------------------------------------------*/
void plat_net_shutdown() {
    plat_net_disconnect();
}

