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

#include "../global.h"

#include "platAtari.h"

#include "fujinet-network.h"



char devicespec[64];
uint16_t bytes_waiting;
uint8_t conn_status;
uint8_t err;
char rxbuf[1518]; // eth_buffer.s lenght 1518

char* fn_strerror( uint8_t e ) {
    switch (e)
    {
        case FN_ERR_OK:
            return "No error";
        case FN_ERR_IO_ERROR:
            return "There was IO error/issue with the device";
        case FN_ERR_BAD_CMD:
            return "Function called with bad arguments";
        case FN_ERR_OFFLINE:
           return "The device is offline";
        case FN_ERR_WARNING:
           return "Device specific non-fatal warning issued";
        case FN_ERR_NO_DEVICE:
           return "There is no network device";
        default:
            return "Unkown Error";
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
    //if (ip65_init(ETH_INIT_DEFAULT)) {  // true if error, false otherwise.
    int8_t ret = network_init();
     
    if ( ret ) { // returns status/error FN_ERR_* values  FN_ERR_OK ==0x00, so non zero is error
        log_add_line(&global.view.terminal, "Initializing Network", -1);
        plat_draw_log(&global.view.terminal, 0, 0, false);
        //app_error(true, ip65_strerror(ip65_error));
        app_error(true, fn_strerror(ret));
    }
}


/*-----------------------------------------------------------------------*/
void plat_net_connect(const char *server_name, int server_port) {
    //uint32_t server;
    int8_t ret = 0;

/* internal to fujinet
    log_add_line(&global.view.terminal, "Obtaining IP address", -1);
    plat_draw_log(&global.view.terminal, 0, 0, false);
    if (dhcp_init()) {
        app_error(true, ip65_strerror(ip65_error));
    }

    log_add_line(&global.view.terminal, "Resolving Server", -1);
    plat_draw_log(&global.view.terminal, 0, 0, false);
    server = dns_resolve(server_name);
    if (!server) {
        app_error(false, ip65_strerror(ip65_error));
    }

*/
    //snprintf( devicespec, "N:TELNET://%s:%d/", server_name, server_port );
    strcpy( devicespec, "N:TELNET://FREECHESS.ORG:5000/" );
    log_add_line(&global.view.terminal, "Connecting to server", -1);
    plat_draw_log(&global.view.terminal, 0, 0, false);
    //if (tcp_connect(server, server_port, fics_tcp_recv)) {
    //    app_error(false, ip65_strerror(ip65_error));
    //}
    ret = network_open( devicespec, OPEN_MODE_RW, OPEN_TRANS_NONE );
    if( ret ) {
        app_error(true, fn_strerror(ret));
    }


    log_add_line(&global.view.terminal, "Logging in, please be patient", -1);
    plat_draw_log(&global.view.terminal, 0, 0, false);
}

/*-----------------------------------------------------------------------*/
void plat_net_disconnect() {
    //tcp_close();
    network_close(devicespec);
}

/*-----------------------------------------------------------------------*/
bool plat_net_update() {
/*
    if (ip65_process()) {
        // I am not sure what erors could be returned here
        if (ip65_error >= IP65_ERROR_PORT_IN_USE) {
            return 1;
        }
    }

*/
    network_status( devicespec, &bytes_waiting, &conn_status, &err );
    if( conn_status && bytes_waiting ) {
        if( bytes_waiting > sizeof(rxbuf) ) 
        network_read( devicespec, rxbuf, bytes_waiting );
    } 

    
    return 0;
}

/*-----------------------------------------------------------------------*/
void plat_net_send(const char *text) {
    log_add_line(&global.view.terminal, text, -1);
/*
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

