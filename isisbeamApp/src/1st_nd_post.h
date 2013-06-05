#ifndef CONTROLS_H
#define CONTROLS_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#ifdef __VMS
#include <unixio.h>
#endif
#ifdef _WIN32
#   include <time.h>
#   include <winsock2.h>
#   define INTERNET_SOCKET PF_INET
#else
#include <unistd.h>
#   include <sys/socket.h>
#   include <sys/stat.h>
#   include <signal.h>
#   include <fcntl.h>
#   include <time.h>
#   include <sys/time.h>
#   include <netdb.h>
#   include <netinet/in.h>
#   include <arpa/inet.h>
#   define SOCKET int
#   define INVALID_SOCKET -1
#   define INTERNET_SOCKET AF_INET
#   define closesocket close
#endif


/* port the "isis controls" server posts packets to */
#define ND_POST_PORT 7001
/* port the "isis instrumentation" server broadcasts packets to */
#define ND_MULTICAST_PORT  7002
#define ND_BROADCAST_PORT1 7003
#define ND_BROADCAST_PORT2 7004
#define SHUTTER_SERVER_PORT 7003
#define ND_XML_POST_PORT 7005
/* multicast address */
#define ND_MULTICAST_ADDRESS "224.1.0.2"

/*
 * general data packet definitions
 */

typedef struct
{ 
    short major; 
    short minor; 
    int len;
} data_header;

typedef struct
{ 
    data_header header;
    void* data;
    int check;
} generic_data_packet;

/*
 * data for a version 1.0 packet
 */
typedef struct 
{
    long beams; 
    long beame1; /* epb1 */
    long beamt; 
    long beamt2;
    long inje;
    long acce;
    long exte;
    long repr;
    long repr2;
    long mode;   /* 0 for 40Hz, 1 for 50Hz */
    long gms1on; /* 1 if beam enabled to TS1 */
    long gms2on; /* 1 if beam enabled to TS2 */
    long shutn;
    long shuts;
    long mtemp;
    long htemp; /* hydrogen moderator */
    long muon_kicker;	/* 0 = supply off, 1 = supply on */
    long ts1_total; /* uah today * 10*/
    long ts1_total_yest; /* uah yesterday * 10 */
    long ts2_total; /* uah today * 10*/
    long ts2_total_yest; /* uah yesterday * 10 */
    long e1_shut; /* TS2 shutters: 0:deactivated, 1:open, 2:closed, 3:moving, 4:fault */
    long e2_shut;
    long e3_shut;
    long e4_shut;
    long e5_shut;
    long e6_shut;
    long e7_shut;
    long e8_shut;
    long e9_shut;
    long w1_shut;
    long w2_shut;
    long w3_shut;
    long w4_shut;
    long w5_shut;
    long w6_shut;
    long w7_shut;
    long w8_shut;
    long w9_shut;
    long t2_mtemp1;  /* TS2: TE842 * 10 - decoupled methane */
    long t2_mtemp2;  /* TS2: TE852 * 10 - coupled methane */
    long t2_htemp1;  /* TS2: TT706 * 10 - hydrogen */
    long e1_vat; /* TS2 vacuum VAT status: 0:deactivated, 1:open, 2:closed, 3:moving, 4:fault */
    long e2_vat;
    long e3_vat;
    long e4_vat;
    long e5_vat;
    long e6_vat;
    long e7_vat;
    long e8_vat;
    long e9_vat;
    long w1_vat;
    long w2_vat;
    long w3_vat;
    long w4_vat;
    long w5_vat;
    long w6_vat;
    long w7_vat;
    long w8_vat;
    long w9_vat;
    time_t time;
    long dmod_runtime;   // in minutes
    long dmod_runtime_lim;   // in minutes
    long dmod_uabeam; //    float value, multipled by 100
    long dmod_annlow1;  // Anneal Pressure Flag 0=pressume too low, 1 =OK
} data_1_0;
    
typedef struct
{ 
    data_header header;
    data_1_0 data;
    int check;
} data_packet_1_0;

/* shutters
Where the LSB is bit 1, bit numbers correspond to beam port number N1, N2 etc as follows:


shutn	1	SLS
	2	PRISMA
	3	SURF
	4	CRISP
	5	LOQ
	6	IRIS/OSIRIS
	7	POL
	8	TOSCA
	9	HET

shuts	1	MAPS
	2	eVS
	3	SXD
	4	NO INSTRUMENT
	5	NO INSTRUMENT
	6	MARI
	7	GEM
	8	HRPD
	9	PEARL
*/

typedef enum { SLS = 0, PRISMA, SURF, CRISP, LOQ, IRIS, POLARIS, TOSCA, HET } ShutN;

/*
 * If more than one instrument use a beamline, separate the names with "/";
 * this is the character assumed in shutter_server.c for splitting them up
 */
static const char* ShutN_names[] = {
    "SLS","PRISMA","SURF","CRISP","LOQ","IRIS_OSIRIS","POL","TOSCA","HET", NULL };

typedef enum { MAPS = 0, EVS, SXD, NO_INST1, NO_INST2, MARI, GEM, HRPD, PEARL } ShutS;
static const char* ShutS_names[] = {
    "MAPS","eVS","SXD","NO INSTRUMENT","NO INSTRUMENT","MARI","GEM","HRPD","PEARL", NULL };

#define SHUTTER_OPEN(__inst,__var) \
	((__var & (1 << __inst)) != 0)

#ifdef __cplusplus
extern "C" {
#endif

SOCKET open_connection(const char* servers[], short port);
int send_data(SOCKET sd, data_packet_1_0* dp);
int send_packet(SOCKET sd, const char* data, int n);
int receive_data(SOCKET sd, data_packet_1_0* dp);
int receive_data_pattern(SOCKET sd, void* data, int maxdata, const char* pattern);
SOCKET setup_socket(unsigned short port, int num_listen);
int initialise_data_packet(data_header* dh, short major, short minor);
SOCKET setup_broadcast_socket(const char* address, short port, struct sockaddr_in* sockin, int multicast);
SOCKET setup_udp_socket(unsigned short port, int multicast);
int receive_data_udp(SOCKET sd, char* dp, int n);
char* xml_parse(const char* input_string, const char* token);

#ifdef __cplusplus
}
#endif

#endif /* CONTROLS_H */
