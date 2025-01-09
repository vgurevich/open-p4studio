/* clang-format off */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>

#include <inttypes.h>  //strlen
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>  //inet_addr

#include <lld/lld_reg_if.h>
#include <lld/lld_err.h>
#include <lld/lld_sku.h>

int dbg_print = 0;
int py_reg_chnl = 0;
int proc_server(void);
uint32_t actv_tile = 0;
uint32_t actv_grp = 0;

/*********************************************************************
* credo_py_server
*********************************************************************/
int credo_py_server(int listen_port, bool local_only) {
  int socket_desc, client_sock, c;
  struct sockaddr_in server, client;
  int so_reuseaddr = 1;

  // Create socket
  socket_desc = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_desc == -1) {
    printf("ERROR: credo_python_intf could not create socket");
    return -1;
  }
  printf("credo_python_intf: listen socket created\n");

  // Prepare the sockaddr_in structure
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = local_only ? htonl(INADDR_LOOPBACK) : htonl(INADDR_ANY);
  server.sin_port = htons(listen_port);

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
  printf("credo_python_intf: bind done on port %d, listening...\n", listen_port);
  do {
    // Listen
    puts("credo_python_intf: listening for incoming connections...");
    listen(socket_desc, 1);


    // accept connection from an incoming client
    c = sizeof(struct sockaddr_in);
    client_sock =
        accept(socket_desc, (struct sockaddr *)&client, (socklen_t *)&c);
    if (client_sock < 0) {
      printf("accept failed");
      //close(socket_desc);
      continue;
    }
    py_reg_chnl = client_sock;

    printf("credo_python_intf: <CONNECT> connection accepted on client sock: %d\n", client_sock);
    proc_server(); // start processing
    printf("credo_python_intf: <DISCONNECT> connection terminated on client sock: %d\n", client_sock);

    close(client_sock);
  } while (true);

  return client_sock;
}

/*********************************************************************
* write_to_socket
*********************************************************************/
static int write_to_socket(int sock, uint8_t *buf, int len) {
  if (send(sock, buf, len, 0) < 0) {
    printf("write_to_socket failed");
    return -3;
  }
  return 0;
}

/*********************************************************************
* read_from_socket
*********************************************************************/
static int read_from_socket(int sock, uint8_t *buf, int len) {
  int n_read = 0, n_read_this_time = 0, i = 1;

  // Receive a message from client
  do {
    n_read_this_time =
        recv(sock, (buf + n_read), (len - n_read), MSG_WAITALL);
        //recv(sock, (buf + n_read), (len - n_read), 0 /*MSG_WAITALL*/);
    if (n_read_this_time > 0) {
      n_read += n_read_this_time;
      if (n_read < len) {
        printf("Partial recv: %d of %d so far..\n", n_read, len);
      }
    } else {
      return n_read;
    }
    setsockopt(sock, IPPROTO_TCP, TCP_QUICKACK, (void *)&i, sizeof(i));

  } while (n_read < len);
  return n_read;
}

uint32_t proc_so_read(uint32_t offset) {
  uint32_t data = 0;
  uint32_t mac_stn_id;
  lld_sku_map_tile_and_group_to_mac_stn_id(0,
                                           actv_tile,
                                           actv_grp,
                                           &mac_stn_id);
  uint32_t bfn_addr = (3 << 24) | (mac_stn_id << 18) | (offset << 2);

  lld_read_register(0, bfn_addr, &data);

  return data;
}

void proc_so_write(uint32_t offset, uint32_t data) {
  uint32_t mac_stn_id;
  lld_sku_map_tile_and_group_to_mac_stn_id(0,
                                           actv_tile,
                                           actv_grp,
                                           &mac_stn_id);
  uint32_t bfn_addr = (3 << 24) | (mac_stn_id << 18) | (offset << 2);

  lld_write_register(0, bfn_addr, data);
  return;
}

uint32_t proc_glb_read(uint32_t offset) {
  uint32_t data = 0;

  lld_read_register(0, offset, &data);

  return data;
}

void proc_glb_write(uint32_t offset, uint32_t data) {

  lld_write_register(0, offset, data);
  return;
}

void credo_py_start_server(bool local_only) {
  py_reg_chnl = credo_py_server(9001, local_only);
  if (-1 == py_reg_chnl) {
    /* Indicates bind/accept error in create server */
    printf("Socket already in use. Exiting the application\n");
    exit(1);
  }
}

/*****************************************************************
* proc_server
*
* This option provides a faster way to process I2C requests in
* support of the Credo eval boards.
* Credo supplies python scripts to configure their eval boards.
* We run these scripts on a switch CPU and connect the CP2112
* of that switch to the I2C pins of the eval board (using the
* CPU port QSFP channel).
*
* Because the driver is in python, the protocol is string based,
* using colons as separators.
*
* The command protocol is as follows:
* Serdes tile access:
*   Read: "r:<address>:<unused_data>"
*  Write: "w:<address>:<data>"
* Global Chip access:
*   Read: "R:<address>:<unused_data>"
*  Write: "W:<address>:<data>"
*
* E.g.
*   "r:0x00009816:0x00000000"
*   "w:0x00009818:0x00001234"
*   "R:0x0124000C:0x00000000"
*   "W:0x01240000:0x12345678"
*
*****************************************************************/
#define CMD_LEN 23
int proc_server(void) {
  while (true) {
    char cmd[CMD_LEN+1] = {0};
    uint32_t address, data;
    int rc, n_read, is_tile = false, is_grp = false, is_read = true, is_serdes_access = true;

    memset(cmd, 0, sizeof(cmd));
    n_read = read_from_socket(py_reg_chnl, (uint8_t *)cmd, CMD_LEN);
    if (0 && dbg_print) {
      int i;
      printf("\nsocket (%d bytes) => ", n_read);
      for (i = 0; i < n_read; i++) {
        printf("%02x ", cmd[i]);
      }
      printf("\n");
    }
    if (n_read != CMD_LEN) return -1;

    cmd[12] = 0;
    address = strtoul(&cmd[2], NULL, 16);
    cmd[CMD_LEN] = 0;
    data = strtoul(&cmd[13], NULL, 16);
    if ((cmd[0] == 'w') || (cmd[0] == 'W')) is_read = false;
    else if ((cmd[0] == 'r') || (cmd[0] == 'R')) is_read = true;
    else if ((cmd[0] == 'g') || (cmd[0] == 'G')) {is_read = false; is_grp = true;}
    else if ((cmd[0] == 't') || (cmd[0] == 'T')) {is_read = false; is_tile = true;}
    else return -2;

    // upper-case means global chip access
    if ((cmd[0] == 'R') || (cmd[0] == 'W')) is_serdes_access = false;

    if (is_tile) {
      actv_tile = address;
      //data = 0;
      data = address;
    } else if (is_grp) {
      actv_grp = address;
      //data = 0;
      data = actv_grp;
    } else if (is_read) {
      if (is_serdes_access) {
        data = proc_so_read(address);
      } else {
        data = proc_glb_read(address);
      }
      snprintf(&cmd[13], 11, "0x%08x", data); // return data
    } else {
      uint32_t wr_data = data;
      if (is_serdes_access) {
        proc_so_write(address, data);
      } else {
        proc_glb_write(address, data);
      }
      data = wr_data;
    }
    // put back colon
    cmd[12] = ':';
    cmd[23] = 0;
    if (1 && dbg_print) {
      if (is_serdes_access) {
        printf("Tile: %d : Group: %d : %s : Addr: %04x : Data : %04x\n",
             actv_tile,
             actv_grp,
             is_tile ? " Tile" : 
             is_grp  ? "Group" : 
             is_read ? " Read" : "Write",
             address,
             data);
      } else {
        printf("Global Chip Access   : %s : Addr: %08x : Data : %08x\n",
             is_read ? " Read" : "Write",
             address,
             data);
      }
    }
    fflush(stdout);
    rc = write_to_socket(py_reg_chnl, (uint8_t *)cmd, CMD_LEN);
    if (rc != 0) return -4;
  }
  return py_reg_chnl;
}

