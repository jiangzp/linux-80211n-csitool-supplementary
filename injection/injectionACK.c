/* 
 * File:   main.c
 * Author: jiangzp
 *
 * Created on December 9, 2013, 5:05 PM
 */

#include <linux/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <linux/socket.h>
#include <linux/netlink.h>
#include <linux/connector.h>
#include "bfee.h"

#include <tx80211.h>
#include <tx80211_packet.h>


#include "util.h"

struct lorcon_packet
{
	__le16	fc;
	__le16	dur;
	u_char	addr1[6];
	u_char	addr2[6];
	u_char	addr3[6];
	__le16	seq;
	u_char	payload[0];
} __attribute__ ((packed));

struct tx80211	tx;
struct tx80211_packet	tx_packet;
uint8_t *payload_buffer;
#define PAYLOAD_SIZE	2000000


FILE* open_file(char* filename, char* spec);
void caught_signal(int sig);
void exit_program(int code);
void exit_program_err(int code, char* func);
FILE* open_file(char* filename, char* spec);
static void init_lorcon();

int delay_us;
int sock_fd = -1;
FILE* out = NULL;
int display_interval = 100;

static void payload_memcpy(uint8_t *dest, uint32_t length,
        uint32_t offset) {
    uint32_t i;
    for (i = 0; i < length; ++i) {
        dest[i] = payload_buffer[(offset + i) % PAYLOAD_SIZE];
    }
}

/*
 * 
 */
int main(int argc, char** argv) {


    struct sockaddr_nl proc_addr, kern_addr; // addrs for recv, send, bind
    struct cn_msg *cmsg;
    char buf[99999];
    int ret, packet_size = 6;
    unsigned short l;
    unsigned short l2;
    uint32_t count = 0;
    int code;
    int display_interval;
    struct lorcon_packet *packet;
    struct iwl_bfee_notif *bfee;
    if (argc != 4) {
        printf("Usage: injectionACK <delay in us> <output file> <display interval>\n");
        return (EXIT_SUCCESS);
    } else {
        sscanf(argv[1], "%d", &delay_us);
        out = open_file(argv[2], "w");
        sscanf(argv[3], "%d", &display_interval);
        printf("delay_us : %d, file name %s, interval : %d \n", delay_us, argv[2], display_interval);
    }

    printf("Generating packet payloads \n");
    payload_buffer = malloc(PAYLOAD_SIZE);
    if (payload_buffer == NULL) {
        perror("malloc payload buffer");
        exit(1);
    }
    generate_payloads(payload_buffer, PAYLOAD_SIZE);

    /* Setup the interface for lorcon */
    printf("Initializing LORCON\n");
    init_lorcon();

    /* Allocate packet */
    packet = malloc(sizeof (*packet) + packet_size);
    if (!packet) {
        perror("malloc packet");
        exit(1);
    }
    packet->fc = (0x08 /* Data frame */
            | (0x0 << 8) /* Not To-DS */);
    packet->dur = 0xffff;

    memcpy(packet->addr1, "\x00\x16\xea\x12\x34\x56", 6);
    memcpy(packet->addr2, "\x00\x16\xea\x12\x34\x56", 6);
    memcpy(packet->addr3, "\xff\xff\xff\xff\xff\xff", 6);


    tx_packet.packet = (uint8_t *) packet;
    tx_packet.plen = sizeof (*packet) + packet_size;


    /* Open and check log file */
    out = open_file(argv[1], "w");

    /* Setup the socket */
    sock_fd = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_CONNECTOR);
    if (sock_fd == -1)
        exit_program_err(-1, "socket");

    /* Initialize the address structs */
    memset(&proc_addr, 0, sizeof (struct sockaddr_nl));
    proc_addr.nl_family = AF_NETLINK;
    proc_addr.nl_pid = getpid(); // this process' PID
    proc_addr.nl_groups = CN_IDX_IWLAGN;
    memset(&kern_addr, 0, sizeof (struct sockaddr_nl));
    kern_addr.nl_family = AF_NETLINK;
    kern_addr.nl_pid = 0; // kernel
    kern_addr.nl_groups = CN_IDX_IWLAGN;

    /* Now bind the socket */
    if (bind(sock_fd, (struct sockaddr *) &proc_addr, sizeof (struct sockaddr_nl)) == -1)
        exit_program_err(-1, "bind");

    /* And subscribe to netlink group */

    int on = proc_addr.nl_groups;
    ret = setsockopt(sock_fd, 270, NETLINK_ADD_MEMBERSHIP, &on, sizeof (on));
    if (ret)
        exit_program_err(-1, "setsockopt");


    /* Set up the "caught_signal" function as this program's sig handler */
    signal(SIGINT, caught_signal);

    while (1) {
        /* Receive from socket with infinite timeout */
        ret = recv(sock_fd, buf, sizeof (buf), 0);
        if (ret == -1)
            exit_program_err(-1, "recvs");
        /* Pull out the message portion and print some stats */
        cmsg = NLMSG_DATA(buf);

        code = cmsg->data[0];
	/* Beamforming packet */
	if (code == 0xBB) {
		bfee = (void *)&cmsg->data[1];
		//printf("rate=0x%x\n", bfee->client_sequence);
                count = bfee->client_sequence;
	}

        /*++count; */

        payload_memcpy(packet->payload, packet_size,
                (count * packet_size) % PAYLOAD_SIZE);
        packet->seq = (__le16) (count & 0x0000ffff);
        packet->dur = (__le16) (count >> 16);
        packet->addr3[0] = packet->dur >> 8;
        packet->addr3[1] = packet->dur & 0x00ff;

        usleep(delay_us);

        ret = tx80211_txpacket(&tx, &tx_packet);
        if (ret < 0) {
            fprintf(stderr, "Unable to transmit packet: %s\n",
                    tx.errstr);
            exit(1);
        }


        l = (unsigned short) cmsg->len;
        l2 = htons(l);
        fwrite(&l2, 1, sizeof (unsigned short), out);
        ret = fwrite(cmsg->data, 1, l, out);
        if (count % display_interval == 0) {
            printf("\b");
            printf("\r");
            printf("%db, cnt=%d", ret, count);
            fflush(stdout);
        }

        if (ret != l)
            exit_program_err(1, "fwrite");
    }


    return (EXIT_SUCCESS);
}

FILE* open_file(char* filename, char* spec) {
    FILE* fp = fopen(filename, spec);
    if (!fp) {
        perror("fopen");
        exit_program(1);
    }
    return fp;
}

void caught_signal(int sig) {
    fprintf(stderr, "Caught signal %d\n", sig);
    exit_program(0);
}

void exit_program(int code) {
    if (out) {
        fclose(out);
        out = NULL;
    }
    if (sock_fd != -1) {
        close(sock_fd);
        sock_fd = -1;
    }
    exit(code);
}

void exit_program_err(int code, char* func) {
    perror(func);
    exit_program(code);
}

static void init_lorcon() {
    /* Parameters for LORCON */
    int drivertype = tx80211_resolvecard("iwlwifi");

    /* Initialize LORCON tx struct */
    if (tx80211_init(&tx, "mon0", drivertype) < 0) {
        fprintf(stderr, "Error initializing LORCON: %s\n",
                tx80211_geterrstr(&tx));
        exit(1);
    }
    if (tx80211_open(&tx) < 0) {
        fprintf(stderr, "Error opening LORCON interface\n");
        exit(1);
    }

    /* Set up rate selection packet */
    tx80211_initpacket(&tx_packet);
}
