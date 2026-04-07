/*
 *  platC64net.c
 *  RetroMate
 *
 *  By S. Wessels and O. Schmidt, 2025.
 *  This is free and unencumbered software released into the public domain.
 *
 */

#include <c64.h>
#include <cbm.h>
#include <string.h>
#include <stdlib.h>

#include "../global.h"

#include "platC64.h"

#define LFN 2     // The logical file number to use for I/O
#define DEV 16    // The network device #
#define SAN 2     // The secondary address (SA) to use on DEV.
#define CMD 15    // The secondary address for the Command channel

char url[64];
unsigned char rxbuf[1518]; // ip_65 eth_buffer.s .res 1518 &  drivers/ethernetcombo.s drivers/ethernet.s stax #1518
uint8_t res;
uint16_t bytes_waiting;
//uint8_t conn_status;
//uint8_t err;
//uint8_t bytes_read;
uint8_t settrans_cmd[] = "settrans,2,4";
uint8_t nw_status_cmd[] = "statusb,2";
extern void ihsetup();
bool trip = false;
bool connected = false;
uint16_t network_bw;
uint8_t network_conn;
uint8_t network_error;
uint8_t tick = 0;

/*-----------------------------------------------------------------------*/
char* cbm_strerror( uint8_t e ) {
     switch(e) // from cbm.h 
     { 
          case 0:
              return "OK";
          case 1:
              return "Too many files";
          case 2:
              return "File open";
          case 3:
              return "File not open";
          case 4:
              return "File not found";
          case 5:
              return "Device not present";
          case 6:
              return "Not input-file";
          case 7:
              return "Not output-file";
          case 8:
              return "Missing file-name";
          case 9:
              return "Illegal device-number";
          case 10:
              return "STOP-key pushed";
          case 11:
              return "General I/O-error";
          default:
              return "Unknown";
     }
}

/*-----------------------------------------------------------------------*/
uint8_t get_network_status(uint16_t *bw, uint8_t *c, uint8_t *err) {
    uint8_t status[4];
    if (cbm_write(CMD, (char*)nw_status_cmd, strlen( nw_status_cmd )) != strlen(nw_status_cmd))  {
        return 1;
    }

    // now the command channel was updated with the correct data channel, let's do a read on the command channel and set the values from it
    if (cbm_read(CMD, status, 4) != 4) {
        return 1;
    }

    if (bw) {
        *bw = (uint16_t)(status[1] << 8) | status[0]; // Combine the first two bytes for bw
    }
    if (c) {
        *c = status[2]; // The third byte is directly assigned to c
    }
    if (err) {
        *err = status[3]; // The fourth byte is directly assigned to err
    }

    return 0;
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
    ihsetup();
    res =  cbm_open( CMD,DEV,CMD, ""); // return 0 if openning was successful
    if( res ) {
        log_add_line(&global.view.terminal, "Initializing Network", -1);
        plat_draw_log(&global.view.terminal, 0, 0, false);
        app_error(true, cbm_strerror(res));
    }
    cbm_write(CMD,settrans_cmd,sizeof(settrans_cmd));
}

/*-----------------------------------------------------------------------*/
void plat_net_connect(const char *server_name, int server_port) {

    strcpy( url, "telnet://");
    strcat( url, server_name );
    strcat( url, ":" );
    itoa( server_port,  url+10 + strlen(server_name), 10 );

    log_add_line(&global.view.terminal, "Connecting to server", -1);
    plat_draw_log(&global.view.terminal, 0, 0, false);


    res = cbm_open( LFN, DEV, SAN, url);
    if ( res ) {
        app_error(false, cbm_strerror(res));
    } else {
        connected = true;
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
    int16_t retval;
    uint8_t *bufptr;
    tick++;
    if( tick < 255 ) {
        return 0;
    } 
    tick = 0;

    if( trip ) {
        retval = cbm_read( LFN, rxbuf, sizeof( rxbuf ) );
        // ( see cbm.h : read up to size of buffer. Returns 
        // number of read bytes.  0 means no-bytesleft/end-of-file; -1 means error.
        if( retval < 0 ) {
            // bad read, check the network status in this case.
            get_network_status( &network_bw, &network_conn, & network_error );
            connected = network_conn;
        } else {
            if( retval > 0 ) fics_tcp_recv( rxbuf, retval );
            return 0;
        }
        
    }
    return !connected; // Flip logic. Original RM returned 1 for error condition. 
  


}

/*-----------------------------------------------------------------------*/
void plat_net_send(const char *text) {
    log_add_line(&global.view.terminal, text, -1);
    //tcp_send((unsigned char *)c64.send_buffer, plat_net_make_ascii(text));
    cbm_write( LFN, (unsigned char *)c64.send_buffer, plat_net_make_ascii( text ) );
}

/*-----------------------------------------------------------------------*/
void plat_net_shutdown() {
    plat_net_disconnect();
}

