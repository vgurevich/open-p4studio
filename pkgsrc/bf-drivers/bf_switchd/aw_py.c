/* clang-format off */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include <inttypes.h>  //strlen
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>  //inet_addr

#include <lld/lld_reg_if.h>


// Trace buffer max length is (stop-start * 4) + 1
// max of "start - stop" is 56
#define MAXLINE 256
#define AW_GUI_UDP_PORT 1234

// log decoded summary of command/response
int evb_dbg = 0;

// log hex dump of command/response
int evb_dbg_verbose = 0;

// indicates waiting for read data from EVB (for decoder)
bool read_open = false;

int aw_decode_cmd_rsp(char *buffer, int len, bool cmd);
void aw_decode_new_cmd(char *buffer, int len);

int evb_client_sock;
struct sockaddr_in evb_client;

/*********************************************************************
* This stores the macro that will be targeted by an received cmd.
*********************************************************************/
uint32_t tgt_dev_port = 0;
uint32_t tgt_ln = 0;

uint32_t new_rsp_len = 0;
uint8_t new_rsp[12] = {0};

/*********************************************************************
* aw_py_server
*
* This code acts as a proxy for commands between the AW GUI and the
* AW EVB. This allows our code to snoop the commands and responses
* as well as to drive the AW EVB ourselves, independent of the GUI.
*
* Here we create the UDP sockets that AW defined as the interface
* between the GUI and EVB. One socket connects this code to the GUI
* on one sideA second, proxy, socket connects this code to the EVB
* on the other side.
*
* GUI <------> socket_desc - client_sock <--------> EVB
*
*
*********************************************************************/
int aw_py_server(int listen_port, bool local_only) {
  int socket_desc, client_sock;
  struct sockaddr_in server, client;
  int so_reuseaddr = 1;
  char buffer[MAXLINE];
  char response[MAXLINE];

  // Create socket to GUI
  socket_desc = socket(AF_INET, SOCK_DGRAM, 0);
  if (socket_desc < 0) {
    printf("ERROR: aw_python_intf could not create socket");
    return -1;
  }

  // Create socket to EVB
  evb_client_sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (evb_client_sock < 0) {
    printf("ERROR: aw_python_intf could not create EVB socket");
    return -1;
  }
  
  // preset sockaddr_in structs to 0
  memset(&server, 0, sizeof(server));
  memset(&client, 0, sizeof(client));
  memset(&evb_client, 0, sizeof(evb_client));

  printf("aw_python_intf: recvfrom socket created\n");

  // init the GUI sockaddr_in structure
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = local_only ? htonl(INADDR_LOOPBACK) : htonl(INADDR_ANY);
  server.sin_port = htons(listen_port);

  setsockopt(socket_desc,
             SOL_SOCKET,
             SO_REUSEADDR,
             &so_reuseaddr,
             sizeof so_reuseaddr);

  // init the EVB sockaddr_in structure
  evb_client.sin_family = AF_INET;
  evb_client.sin_addr.s_addr = local_only ? htonl(INADDR_LOOPBACK) : htonl(INADDR_ANY);
  evb_client.sin_port = htons(listen_port);

  // store the EVB static IP address in sockaddr_in structure
  inet_pton(AF_INET, "10.232.10.43", &(evb_client.sin_addr));

  setsockopt(socket_desc,
             SOL_SOCKET,
             SO_REUSEADDR,
             &so_reuseaddr,
             sizeof so_reuseaddr);

  // Bind
  if (bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0) {
    perror("bind failed. Error");
    close(socket_desc);
    return -1;
  }

  printf("aw_python_intf: waiting on GUI commands...\n");

  do {
    unsigned int len, evb_len;
    int n, evb_n = 0;
  
    len = sizeof(client);  //len is value/result
    n = recvfrom(socket_desc, (char *)buffer, MAXLINE, 
                MSG_WAITALL, ( struct sockaddr *) &client,
                &len);
    if (evb_dbg_verbose) {
      printf("recvd %d bytes from gui\n", n);
      for (int j = 0; j < n; j++) {
        if ((j % 16) == 0) {
          printf("\n");
        }
        printf(" %02x", buffer[j] & 0xff);
      }
      printf("\n");
    }
    // siphon off new-format (16ln/4ln) commands which are distinguished
    // by their use of lower case letters
    if ((buffer[1] == 'r') || (buffer[1] == 'w') ||(buffer[1] == 's')) {
      aw_decode_new_cmd(buffer, len);
      if (new_rsp_len > 0) {
        // send response (from EVB) to GUI
        sendto(socket_desc, (const char *)new_rsp, new_rsp_len,
            MSG_CONFIRM, (const struct sockaddr *) &client,
                len);
        if (evb_dbg_verbose) {
          printf("Sent %d bytes to gui\n", evb_n);
        }
        continue;
      }
    }

    if (evb_dbg) {
      aw_decode_cmd_rsp(buffer, n, true);
    }

    /*****************************************************************
    # Uses a UDP socket connection to connect to
    # the MSS_ETH_IF platform on the FPGA
    #
    # Command Format:
    #    The command interface parses command frames in bytes
    #    The frame format is:
    #  !CCxxxxxxxx
    #    !: Start of Frame (1byte)
    #    C: Command   (2 bytes)
    #    x: Arguments (0-8 Bytes)
    #
    #    Example
    #     1)the Reset Apb Command (no arguments) is:
    #       !RA
    #     2)the Write Csr Command (8 arguments) is:
    #       !RCaaaadddd (a=addr bytes, d=data bytes lsb...msb)
    #
    #  When executing a command queue, each command must be padded
    #  to 11bytes to force the parser on the write boundary
    #  becuase EndOfFrame does not exist
    *****************************************************************/

    // send (proxy) command to EVB
    sendto(evb_client_sock, (const char *)buffer, n,
        MSG_CONFIRM, (const struct sockaddr *) &evb_client,
            len);
    if (evb_dbg_verbose) {
      printf("Sent %d bytes to evb\n", n);
    }

    // get response from EVB
    evb_len = sizeof(evb_client);  //len is value/result
    evb_n = recvfrom(evb_client_sock, (char *)response, MAXLINE, 
                MSG_WAITALL, ( struct sockaddr *) &evb_client,
                &evb_len);

    if (evb_dbg_verbose) {
      printf("recvd %d bytes from evb\n", evb_n);
      for (int j = 0; j < evb_n; j++) {
        if ((j % 16) == 0) {
          printf("\n");
        }
        printf(" %02x", response[j] & 0xff);
      }
      printf("\n");
    }
    if (evb_dbg) {
      aw_decode_cmd_rsp(response, evb_n, false);
    }

    // send response (from EVB) to GUI
    sendto(socket_desc, (const char *)response, evb_n,
        MSG_CONFIRM, (const struct sockaddr *) &client,
            len);
    if (evb_dbg_verbose) {
      printf("Sent %d bytes to gui\n", evb_n);
    }
  } while (true);

  close(client_sock);
  return 0; // cant get here
}

uint32_t aw_decode_addr_16to32b(uint32_t b32) {
  if ((b32 & 0xC000) == 0xC000) { //sram
    return( (1<<31) | ((b32 & 0x7FFF) << 1) );
  } else {
    uint32_t ofs = (b32 & 0x7FF) << 1;
    uint32_t blk = (((b32 & 0x7800) >> 11) - 1) & 0x3;
    uint32_t ln  = (((b32 & 0x7800) >> 11) - 1) >> 2;
    //if ((((b32 & 0x7800) >> 11) >> 2) == 0) {
    if ((((b32 & 0x7800) >> 11)) == 0) {
      return ofs; //cmn
    } else {
      return(((ln + 1) << 25) | (blk << 12) | ofs);
    }
  }
}

extern
bf_status_t bf_tof3_serdes_top_init(bf_dev_id_t dev_id, uint32_t run_post);
extern
bf_status_t bf_tof3_serdes_csr_rd_specific_side(bf_dev_id_t dev_id,
                                  bf_dev_port_t dev_port,
                                  uint32_t ln,
                                  uint32_t section,
                                  uint32_t csr,
                                  uint32_t *val);
extern
bf_status_t bf_tof3_serdes_csr_wr_specific_side(bf_dev_id_t dev_id,
                                  bf_dev_port_t dev_port,
                                  uint32_t ln,
                                  uint32_t section,
                                  uint32_t csr,
                                  uint32_t val);

void aw_decode_new_cmd(char *buffer, int len) {
  uint32_t u32_adr, u32_dta, dev_port, ln, side, section, subdev_id;
  (void)u32_adr;
  (void)len;

  new_rsp_len = 0; // default to no rsp
 
  if ((buffer[1] == 'r') && (buffer[2] == 'm')) { // re-init serdes
    bf_tof3_serdes_top_init(0, 0);
    // fill out response
    new_rsp_len = 1;
    new_rsp[0] = 0x31; //success
    return;
  } else if ((buffer[1] == 's') && (buffer[2] == 'm')) { // set macro
    dev_port = (uint32_t)(((buffer[3] & 0xff) << 0) |
                     ((buffer[4] & 0xff) << 8) |
                     ((buffer[5] & 0xff) << 16) |
                     ((buffer[6] & 0xff) << 24));
    ln = (uint32_t)(((buffer[7] & 0xff) << 0) |
                     ((buffer[8] & 0xff) << 8) |
                     ((buffer[9] & 0xff) << 16) |
                     ((buffer[10] & 0xff) << 24));
    if ((dev_port < 1004) && (ln < 8)) {
      tgt_dev_port = dev_port;
      tgt_ln = ln;
      // fill out response
      new_rsp_len = 1;
      new_rsp[0] = 0x31; //success
    } else {
      printf("Invalid dev_port or ln: d_p=%d : ln=%d\n", dev_port, ln);
      // fill out response
      new_rsp_len = 1;
      new_rsp[0] = 0x30; //error
    }
  } else if ((buffer[1] == 'r') && (buffer[2] == 'c')) { // read csr
    u32_adr = (uint32_t)(((buffer[3] & 0xff) << 0) |
                         ((buffer[4] & 0xff) << 8) |
                         ((buffer[5] & 0xff) << 16) |
                         ((buffer[6] & 0xff) << 24));
    side = (uint32_t)(buffer[7]);
    /*
      MSS_SECTION_CMN = 0,
      MSS_SECTION_RX,
      MSS_SECTION_TX,
    */
    if (side == 0) {
      section = 0; // CMN
    } else if (side == 1) {
      section = 2; // TX
    } else if (side == 2) {
      section = 1; // RX
    } else {
      section = 1;
    }
    // The python driver was supposed to pass only "lane0" offsets.
    // However, they seem to be including the lane offset as well. This
    // causes an overflow into the upper bits when physical lane 7
    // is referenced. Mask off any lane offset here and make it look
    // like a lane0 csr offset, but be careful not to mess up the CMN
    // accesses, which start from "0".
    u32_adr = (u32_adr & 0x01ffffff);
    if (section != 0) {
      u32_adr = (u32_adr | 0x2000000);
    }
    // perform read
    bf_tof3_serdes_csr_rd_specific_side(0, tgt_dev_port, tgt_ln, section, u32_adr, &u32_dta);

    // fill out response
    new_rsp[0] = 0x42;
    new_rsp[1] = (u32_dta >> 0) & 0xff;
    new_rsp[2] = (u32_dta >> 8) & 0xff;
    new_rsp[3] = (u32_dta >> 16) & 0xff;
    new_rsp[4] = (u32_dta >> 24) & 0xff;
    new_rsp[5] = 0x0;
    new_rsp_len = 6;
  } else if ((buffer[1] == 'w') && (buffer[2] == 'c')) { // write csr
    u32_adr = (uint32_t)(((buffer[3] & 0xff) << 0) |
                     ((buffer[4] & 0xff) << 8) |
                     ((buffer[5] & 0xff) << 16) |
                     ((buffer[6] & 0xff) << 24));
    u32_dta = (uint32_t)(((buffer[7] & 0xff) << 0) |
                     ((buffer[8] & 0xff) << 8) |
                     ((buffer[9] & 0xff) << 16) |
                     ((buffer[10] & 0xff) << 24));
    side = (uint32_t)(buffer[11]);
    /*
      MSS_SECTION_CMN = 0,
      MSS_SECTION_RX,
      MSS_SECTION_TX,
    */
    if (side == 0) {
      section = 0; // CMN
    } else if (side == 1) {
      section = 2; // TX
    } else if (side == 2) {
      section = 1; // RX
    } else {
      section = 1;
    }
    // The python driver was supposed to pass only "lane0" offsets.
    // However, they seem to be including the lane offset as well. This
    // causes an overflow into the upper bits when physical lane 7
    // is referenced. Mask off any lane offset here and make it look
    // like a lane0 csr offset, but be careful not to mess up the CMN
    // accesses, which start from "0".
    u32_adr = (u32_adr & 0x01ffffff);
    if (section != 0) {
      u32_adr = (u32_adr | 0x2000000);
    }
    // perform write
    bf_tof3_serdes_csr_wr_specific_side(0, tgt_dev_port, tgt_ln, section, u32_adr, u32_dta);

    // fill out response
    new_rsp_len = 1;
    new_rsp[0] = 0x31;
  } else if ((buffer[1] == 'r') && (buffer[2] == 'r')) { // read any register
    u32_adr = (uint32_t)(((buffer[3] & 0xff) << 0) |
                         ((buffer[4] & 0xff) << 8) |
                         ((buffer[5] & 0xff) << 16) |
                         ((buffer[6] & 0xff) << 24));
    subdev_id = (uint32_t)(buffer[7]);
    // perform read
    lld_subdev_read_register(0, subdev_id, u32_adr, &u32_dta);

    // fill out response
    new_rsp[0] = 0x42;
    new_rsp[1] = (u32_dta >> 0) & 0xff;
    new_rsp[2] = (u32_dta >> 8) & 0xff;
    new_rsp[3] = (u32_dta >> 16) & 0xff;
    new_rsp[4] = (u32_dta >> 24) & 0xff;
    new_rsp[5] = 0x0;
    new_rsp_len = 6;
  } else if ((buffer[1] == 'w') && (buffer[2] == 'r')) { // write csr
    u32_adr = (uint32_t)(((buffer[3] & 0xff) << 0) |
                     ((buffer[4] & 0xff) << 8) |
                     ((buffer[5] & 0xff) << 16) |
                     ((buffer[6] & 0xff) << 24));
    u32_dta = (uint32_t)(((buffer[7] & 0xff) << 0) |
                     ((buffer[8] & 0xff) << 8) |
                     ((buffer[9] & 0xff) << 16) |
                     ((buffer[10] & 0xff) << 24));
    subdev_id = (uint32_t)(buffer[11]);
    // perform write
    lld_subdev_write_register(0, subdev_id, u32_adr, u32_dta);

    // fill out response
    new_rsp_len = 1;
    new_rsp[0] = 0x31;
  } else {
    printf("Improperly formatted cmd(3)\n");
  }
}

int aw_decode_cmd_rsp(char *buffer, int len, bool cmd) {
  if (cmd) {
    if (len < 3) {
      printf("Improperly formatted cmd(1)\n");
    } else if (buffer[0] != '!') {
      printf("Improperly formatted cmd(2)\n");
    } else {
      uint32_t a16, a32;

      a16 = (uint32_t)(((buffer[3] & 0xff) << 0) |
               ((buffer[4] & 0xff) << 8) |
               ((buffer[5] & 0xff) << 16) |
               ((buffer[6] & 0xff) << 24));
      a32 = aw_decode_addr_16to32b( a16 );

      if ((buffer[1] == 'R') && (buffer[2] == 'C') &&(len == 7)) {
        // !RC, read csr cmd
        read_open = true;
        printf("Read : %08x : %08x\n",
              (((buffer[3] & 0xff) << 0) |
               ((buffer[4] & 0xff) << 8) |
               ((buffer[5] & 0xff) << 16) |
               ((buffer[6] & 0xff) << 24)),
              a32);
      } else if ((buffer[1] == 'R') && (buffer[2] == 'T') &&(len == 11)) {
        printf("Read Trace buffer:\n");
        printf("  ctl adr=%08x, ctl data=%08x, start=%d, stop=%d\n",
              aw_decode_addr_16to32b(((buffer[3] & 0xff) << 0) | ((buffer[4] & 0xff) << 8)),
              aw_decode_addr_16to32b(((buffer[5] & 0xff) << 0) | ((buffer[6] & 0xff) << 8)),
              (((buffer[7] & 0xff) << 0) | ((buffer[8] & 0xff) << 8)),
              (((buffer[9] & 0xff) << 0) | ((buffer[10] & 0xff) << 8)));
      } else if ((buffer[1] == 'W') && (buffer[2] == 'C') && (len == 11)) {
        // !WC, write csr cmd
        printf("Write: %08x : %08x : %08x\n",
              (((buffer[3] & 0xff) << 0) |
               ((buffer[4] & 0xff) << 8) |
               ((buffer[5] & 0xff) << 16) |
               ((buffer[6] & 0xff) << 24)),
              a32,
              (((buffer[7] & 0xff) << 0) |
               ((buffer[8] & 0xff) << 8) |
               ((buffer[9] & 0xff) << 16) |
               ((buffer[10] & 0xff) << 24)));
      } else { // something we dont know aboout?
        printf("****************************** Unk  : ");
        for (int j = 0; j < len; j++) {
          printf(" %02x", buffer[j]);
        }
        printf("\n");
      }
    }
  } else {
    if ((buffer[0] == 'B') && (len == 6) && read_open) {
      printf("Data :          :          : %08x\n",
              (((buffer[1] & 0xff) << 0) |
               ((buffer[2] & 0xff) << 8) |
               ((buffer[3] & 0xff) << 16) |
               ((buffer[4] & 0xff) << 24)));
    } else if (buffer[0] == 'T') {
      printf("*** EVB cmd timeout ***\n");
    }
  }
  return 0;
}

void aw_py_start_server(bool local_only) {
  int rc = aw_py_server(AW_GUI_UDP_PORT, local_only);
  if (-1 == rc) {
    /* Indicates bind/accept error in socket creation/initializatio */
    printf("Socket already in use. Exiting the application\n");
    exit(1);
  }
}

void evb_mss_reset(uint32_t macro_base_addr) {
  unsigned int len, evb_len;
  int n = 3, evb_n;
  char buffer[MAXLINE];
  char response[MAXLINE];
  (void)macro_base_addr;

  snprintf(buffer, 4, "!RM");
  len = sizeof(evb_client);  //len is value/result

  // send (proxy) command to EVB
  sendto(evb_client_sock, (const char *)buffer, n,
      MSG_CONFIRM, (const struct sockaddr *) &evb_client,
          len);
  if (evb_dbg_verbose) {
    printf("Sent %d bytes to evb\n", n);
    for (int j = 0; j < n; j++) {
        if ((j % 16) == 0) {
          printf("\n");
        }
      printf(" %02x", buffer[j]);
    }
    printf("\n");
  }

  // get response from EVB
  evb_len = sizeof(evb_client);  //len is value/result
  evb_n = recvfrom(evb_client_sock, (char *)response, MAXLINE,
              MSG_WAITALL, ( struct sockaddr *) &evb_client,
              &evb_len);

  if (evb_dbg_verbose) {
    printf("recvd %d bytes from evb\n", evb_n);
    for (int j = 0; j < evb_n; j++) {
      printf(" %02x", response[j]);
    }
    printf("\n");
  }
  if (evb_dbg) {
    aw_decode_cmd_rsp(response, evb_n, false);
  }

  snprintf(buffer, 4, "!RA");
  len = sizeof(evb_client);  //len is value/result

  if (evb_dbg_verbose) {
    printf("Sent %d bytes to evb\n", n);
    for (int j = 0; j < n; j++) {
        if ((j % 16) == 0) {
          printf("\n");
        }
      printf(" %02x", buffer[j]);
    }
    printf("\n");
  }

  // send (proxy) command to EVB
  sendto(evb_client_sock, (const char *)buffer, n,
      MSG_CONFIRM, (const struct sockaddr *) &evb_client,
          len);
  if (evb_dbg_verbose) {
    printf("Sent %d bytes to evb\n", n);
  }

  // get response from EVB
  evb_len = sizeof(evb_client);  //len is value/result
  evb_n = recvfrom(evb_client_sock, (char *)response, MAXLINE,
              MSG_WAITALL, ( struct sockaddr *) &evb_client,
              &evb_len);

  if (evb_dbg_verbose) {
    printf("recvd %d bytes from evb\n", evb_n);
    for (int j = 0; j < evb_n; j++) {
        if ((j % 16) == 0) {
          printf("\n");
        }
      printf(" %02x", response[j]);
    }
    printf("\n");
  }
  if (evb_dbg) {
    aw_decode_cmd_rsp(response, evb_n, false);
  }
}

void evb_write_csr(uint32_t addr, uint32_t data) {
  unsigned int len, evb_len;
  int n = 11, evb_n;
  char buffer[MAXLINE];
  char response[MAXLINE];

  //snprintf(buffer, 11, "!WC%08x%08x", addr, data);

  addr &= 0xffff;
  buffer[0] = '!';
  buffer[1] = 'W';
  buffer[2] = 'C';
  buffer[3] = (addr & 0xff); 
  buffer[4] = ((addr >> 8) & 0xff); 
  buffer[5] = 0x00;
  buffer[6] = 0x00;
  buffer[7] = (data & 0xff);
  buffer[8] = ((data >> 8) & 0xff);
  buffer[9] = ((data >> 16) & 0xff);
  buffer[10] = ((data >> 24) & 0xff);

  len = sizeof(evb_client);  //len is value/result

  // send (proxy) command to EVB
  sendto(evb_client_sock, (const char *)buffer, n,
      MSG_CONFIRM, (const struct sockaddr *) &evb_client,
          len);
  if (evb_dbg_verbose) {
    printf("Sent %d bytes to evb\n", n);
  }
  if (evb_dbg) {
    aw_decode_cmd_rsp(buffer, n, true);
  }

  // get response from EVB
  evb_len = sizeof(evb_client);  //len is value/result
  evb_n = recvfrom(evb_client_sock, (char *)response, MAXLINE,
              MSG_WAITALL, ( struct sockaddr *) &evb_client,
              &evb_len);

  if (evb_dbg_verbose) {
    printf("recvd %d bytes from evb\n", evb_n);
    for (int j = 0; j < evb_n; j++) {
        if ((j % 16) == 0) {
          printf("\n");
        }
      printf(" %02x", response[j] & 0xff);
    }
    printf("\n");
  }
  if (evb_dbg) {
    aw_decode_cmd_rsp(response, evb_n, false);
  }
}

void evb_read_csr(uint32_t addr, uint32_t *data) {
  unsigned int len, evb_len;
  int n = 7, evb_n;
  char buffer[MAXLINE];
  char response[MAXLINE];

  //snprintf(buffer, 7, "!RC%04x", addr & 0xffff);

  addr &= 0xffff;
  buffer[0] = '!';
  buffer[1] = 'R';
  buffer[2] = 'C';
  buffer[3] = (addr & 0xff); 
  buffer[4] = ((addr >> 8) & 0xff); 
  buffer[5] = 0x00;
  buffer[6] = 0x00;
  len = sizeof(evb_client);  //len is value/result

  if (evb_dbg_verbose) { 
    printf("sending 7 bytes to evb\n");
    for (int j = 0; j < 7; j++) { 
      printf(" %02x", buffer[j]);
    } 
    printf("\n");
  }
  if (evb_dbg) {
    aw_decode_cmd_rsp(buffer, n, true);
  }

  read_open = true;

  // send (proxy) command to EVB
  sendto(evb_client_sock, (const char *)buffer, n,
      MSG_CONFIRM, (const struct sockaddr *) &evb_client,
          len);
  if (evb_dbg_verbose) { 
    printf("Sent %d bytes to evb\n", n);
  }

  // get response from EVB
  evb_len = sizeof(evb_client);  //len is value/result
  evb_n = recvfrom(evb_client_sock, (char *)response, MAXLINE,
              MSG_WAITALL, ( struct sockaddr *) &evb_client,
              &evb_len);

  if (evb_dbg_verbose) { 
    printf("recvd %d bytes from evb\n", evb_n);
    for (int j = 0; j < evb_n; j++) { 
        if ((j % 16) == 0) {
          printf("\n");
        }
      printf(" %02x", response[j]);
    } 
    printf("\n");
  }
  if (evb_dbg) { 
    aw_decode_cmd_rsp(response, evb_n, false);
  }
  if ((response[0] == 'B') && (evb_n == 6) && read_open) {
    uint32_t data_val;
    data_val = (((response[1] & 0xff) << 0) | 
                ((response[2] & 0xff) << 8) | 
                ((response[3] & 0xff) << 16) | 
                ((response[4] & 0xff) << 24));
    *data = data_val;
    //printf(" Data :          : %08x\n", data_val);
    read_open = false;
  }
}


