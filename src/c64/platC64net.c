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

#include "../global.h"

#include "platC64.h"

#define LFN 2     // The logical file number to use for I/O
#define DEV 16    // The network device #
#define SAN 2     // The secondary address (SA) to use on DEV.
#define CMD 15    // The secondary address for the Command channel

char url[64];
unsigned char rxbuf[1518]; // ip_65 eth_buffer.s .res 1518 &  drivers/ethernetcombo.s drivers/ethernet.s stax #1518


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
    if( cbm_open( CMD,DEV,CMD, "") ) {
        log_add_line(&global.view.terminal, "Initializing Network", -1);
        plat_draw_log(&global.view.terminal, 0, 0, false);
        app_error(true, ip65_strerror(ip65_error));
    }
}

/*-----------------------------------------------------------------------*/
void plat_net_connect(const char *server_name, int server_port) {

    strcpy( url, "TELNET://");
    strcat( url, server_name );
    strcat( url, ":" );
    itoa( server_port,  url+10 + strlen(server_name), 10 );

    log_add_line(&global.view.terminal, "Connecting to server", -1);
    plat_draw_log(&global.view.terminal, 0, 0, false);


    res = cbm_open( LFN, DEV, SAN, device);
    if ( res ) {
        app_error(false, fn_strerror(res));
    }

    log_add_line(&global.view.terminal, "Logging in, please be patient", -1);
    plat_draw_log(&global.view.terminal, 0, 0, false);
}

/*-----------------------------------------------------------------------*/
void plat_net_disconnect() {
    cbm_close(LFN);
    cbm_close(CMD);
}

/*-----------------------------------------------------------------------*/
bool plat_net_update() {

    if( network_status( devicespec, &bytes_waiting, &conn_status, &err ) == FN_ERR_OK ) {
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
    int len = strlen(text);
    log_add_line(&global.view.terminal, text, -1);
    //tcp_send((unsigned char *)c64.send_buffer, plat_net_make_ascii(text));
    cbm_write( LFN, (unsigned char *)text, len );
}

/*-----------------------------------------------------------------------*/
void plat_net_shutdown() {
    plat_net_disconnect();
}

