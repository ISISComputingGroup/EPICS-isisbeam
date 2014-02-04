#include <ctype.h>
#include <math.h>
#include "1st_nd_post.h"

static char xml_buffer[10240];
static char xml_buffer2[10240];
static char mcr_message[4096];
static char r55_message[4096];

static char* as_iso(time_t t)
{
	char* time_buffer = (char*)malloc(64 * sizeof(char));
	struct tm* pstm = localtime(&t);
	strftime(time_buffer, 64, "%Y-%m-%dT%H:%M:%S", pstm);
	return time_buffer;
}

#ifdef __VMS
static const char* mcr_message_file = "scratch$disk:[mcr]newrecord.tmp";
static const char* r55_message_file = "scratch$disk:[mcr]r55message.tmp";
static const char* uamph_file = "scratch$disk:[mcr]relay_server.tmp";
#else
static const char* mcr_message_file = "/tmp/newrecord.tmp";
static const char* r55_message_file = "/tmp/r55message.tmp";
static const char* uamph_file = "/tmp/relay_server.tmp";
#endif /* __VMS */

static const char* ts2_shutter_mode(int stat)
{
    switch( (stat >> 8) & 0xff )
    {
                case 0:
                    return "DEACT";; // De-Activated
                    break;
                case 1:
                    return "MANUAL"; // Manual (Control Room HMI)
                    break;
                case 2:
                    return "REM-MANUAL"; // Remote Manual (used for shutter scanning)
                    break;
                case 3:
                    return "STC"; // Shield Top Control (used for maintenance)
                    break;
                case 4:
                    return "OPENING-BLR"; // Opening (beam line request)
                    break;
                case 5:
                    return "CLOSING-BLR"; // Closing (beam line request)
                    break;
                case 6:
                    return "POS-CORR"; // Position Correction
                    break;
                case 7:
                    return "OPENING-CR"; // Opening (control Room HMI request)
                    break;
                case 8:
                    return "CLOSING-CR"; // Closing (control Room HMI request)
                    break;
                case 9:
                    return "EMG-CLOSE"; // Emergency Close (control Room HMI request)
                    break;
                case 98:
                    return "BLC"; // Beam Line Control (no auto correction)
                    break;
                case 99:
                    return "BLC"; // Beam Line Control (with auto correction)
                    break;
                default:
                    return "INVALID";
                    break;

    }	    
    return "INVALID"; /*NOTREACHED*/
}

static const char* ts2_shutter_status(int stat)
{
    switch( stat & 0xff )
    {
    	case 0:
    	    /* return "DEACTIVE"; */
    	    return "DEACT";
    	    break;
    	case 1:
    	    return "OPEN";
    	    break;
    	case 2:
    	    return "CLOSED";
    	    break;
    	case 3:
    	    return "MOVING";
    	    break;
    	case 4:
    	    return "FAULT";
    	    break;
    	default:
    	    return "INVALID";
    	    break;
    }	    
    return "INVALID"; /*NOTREACHED*/
}

static const char* ts2_vat_status(int stat)
{
    switch(stat)
    {
    	case 0:
    	    /* return "DEACTIVE"; */
    	    return "DEACT";
    	    break;
    	case 1:
    	    return "OPEN";
    	    break;
    	case 2:
    	    return "MOVING";
    	    break;
    	case 3:
    	    return "CLOSED";
    	    break;
    	case 4:
    	    return "FAULT";
    	    break;
    	default:
    	    return "INVALID";
    	    break;
    }	    
    return "INVALID"; /*NOTREACHED*/
}

static const char* ts1_shutter_status(int stat, int beamline)
{
    switch( stat & (1 << (beamline - 1)) )
    {
    	case 0:
    	    return "CLOSED";
    	    break;
    	default: 
    	    return "OPEN";
    	    break;
    }	    
    return "INVALID"; /*NOTREACHED*/
}

#if 0
        "<?xml-stylesheet type=\"text/xsl\" href=\"status.xsl\" ?>\n"
	"<ISISBEAM version=\"1.0\"\n"
	"  xmlns=\"isisbeam/1.0\"\n"
	"  xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">\n"
#endif

static const char* xml_format = 
	"<?xml version=\"1.0\"?>\n" 
	"<ISISBEAM>"
	"<BEAMS>%.1f</BEAMS>"
	"<BEAME1>%.1f</BEAME1>"
	"<BEAMT>%.1f</BEAMT>"
	"<BEAMT2>%.1f</BEAMT2>"
	"<INJE>%.0f</INJE>"
	"<ACCE>%.0f</ACCE>"
	"<EXTE>%.0f</EXTE>"
	"<REPR>%.1f</REPR>"
	"<REPR2>%.1f</REPR2>"
	"<MODE>%d</MODE>"
	"<GMS1>%d</GMS1>"
	"<GMS2>%d</GMS2>"
	"<TS1ON>%s</TS1ON>"
	"<TS1OFF>%s</TS1OFF>"
	"<TS2ON>%s</TS2ON>"
	"<TS2OFF>%s</TS2OFF>"
	"<TIME>%u</TIME>"
	"<SHUTN>%d</SHUTN>"
	"<SHUTS>%d</SHUTS>"
	"<TS1SHUTTERS>"
	"<SANDALS>%s</SANDALS>" /* N1 */
	"<PRISMA>%s</PRISMA>" /* N2 */
	"<ALF>%s</ALF>" /* N2 */
	"<ROTAX>%s</ROTAX>" /* N2 */
	"<SURF>%s</SURF>" /* N3 */
	"<CRISP>%s</CRISP>" /* N4 */
	"<LOQ>%s</LOQ>" /* N5 */
	"<IRIS>%s</IRIS>" /* N6 */
	"<OSIRIS>%s</OSIRIS>" /* N6 */
	"<POLARIS>%s</POLARIS>" /* N7 */
	"<TOSCA>%s</TOSCA>" /* N8 */
	"<INES>%s</INES>" /* N8 */
	"<HET>%s</HET>" /* N9 */
	"<MAPS>%s</MAPS>" /* S1 */
	"<EVS>%s</EVS>" /* S2 */
	"<SXD>%s</SXD>" /* S3 */
	"<MERLIN>%s</MERLIN>" /* S4 */
	/* "<S5>%s</S5>" /* S5 */
	"<MARI>%s</MARI>" /* S6 */
	"<GEM>%s</GEM>" /* S7 */
	"<HRPD>%s</HRPD>" /* S8 */
	"<ENGINX>%s</ENGINX>" /* S8 */
	"<PEARL>%s</PEARL>" /* S9 */
	"</TS1SHUTTERS>"
	"<MTEMP>%.1f</MTEMP>"
        "<HTEMP>%.1f</HTEMP>"
	"<MUONKICKER>%d</MUONKICKER>"
	"<TS1_TOTAL>%.1f</TS1_TOTAL>"
        "<TS1_TOTAL_YEST>%.1f</TS1_TOTAL_YEST>"
	"<TS2_TOTAL>%.1f</TS2_TOTAL>"
        "<TS2_TOTAL_YEST>%.1f</TS2_TOTAL_YEST>"
        "<SHUTE>%s</SHUTE>"
        "<SHUTW>%s</SHUTW>"
	"<TS2SHUTTERS>"
	/* "<E1>%s</E1>" */
	"<SANS2D>%s</SANS2D>" /* E2 */
	"<POLREF>%s</POLREF>" /* E3 */
	"<INTER>%s</INTER>" /* E4 */
	"<OFFSPEC>%s</OFFSPEC>" /* E5 */
	/* "<E6>%s</E6>" */
	/* "<E7>%s</E7>" */
	"<WISH>%s</WISH>" /* E8 */
	/* "<E9>%s</E9>" */
	/* "<W1>%s</W1>" */
	/* "<W2>%s</W2>" */
	/* "<W3>%s</W3>" */
	/* "<W4>%s</W4>" */
	/* "<W5>%s</W5>" */
	"<LET>%s</LET>" /* W6 */
	"<NIMROD>%s</NIMROD>" /* W7 */
	/* "<W8>%s</W8>" */
	/* "<W9>%s</W9>" */
	"</TS2SHUTTERS>"
	"<T2MTEMP1>%.1f</T2MTEMP1>" /* decoupled methane, TE842 */
	"<T2MTEMP2>%.1f</T2MTEMP2>" /* coupled methane, TE852 */ 
	"<T2HTEMP1>%.1f</T2HTEMP1>" /* hydrogen TT706 */
	"<TIMEF>%s</TIMEF>"
//	"<MCRMESSAGE time=\"%lu\">%s</MCRMESSAGE>"
//	"<R55MESSAGE time=\"%lu\">%s</R55MESSAGE>"
	"</ISISBEAM>"; // this pattern is used as a trailer marker

static const char* xml_format2 = 
	"<?xml version=\"1.0\"?>\n" 
	"<ISISBEAM2>\n"
	"<BEAMI>%.1f</BEAMI>"
	"<BEAMR>%.1f</BEAMR>"
	"<BEAML>%.1f</BEAML>"
        "<VATE>%s</VATE>"
        "<VATW>%s</VATW>"
	"<TS2VAT>\n"
	/* "<E1>%s</E1>\n" */
	"<SANS2D>%s</SANS2D>" /* E2 */
	"<POLREF>%s</POLREF>" /* E3 */
	"<INTER>%s</INTER>" /* E4 */
	"<OFFSPEC>%s</OFFSPEC>" /* E5 */
	/* "<E6>%s</E6>" */
	/* "<E7>%s</E7>" */
	"<WISH>%s</WISH>" /* E8 */
	/* "<E9>%s</E9>" */
	/* "<W1>%s</W1>" */
	/* "<W2>%s</W2>" */
	/* "<W3>%s</W3>" */
	/* "<W4>%s</W4>" */
	/* "<W5>%s</W5>" */
	"<LET>%s</LET>" /* W6 */
	"<NIMROD>%s</NIMROD>" /* W7 */
	/* "<W8>%s</W8>" */
	/* "<W9>%s</W9>" */
	"</TS2VAT>\n"
	"<TS2SHUTTERMODES>"
	/* "<E1>%s</E1>" */
	"<SANS2D>%s</SANS2D>" /* E2 */
	"<POLREF>%s</POLREF>" /* E3 */
	"<INTER>%s</INTER>" /* E4 */
	"<OFFSPEC>%s</OFFSPEC>" /* E5 */
	/* "<E6>%s</E6>" */
	/* "<E7>%s</E7>" */
	"<WISH>%s</WISH>" /* E8 */
	/* "<E9>%s</E9>" */
	/* "<W1>%s</W1>" */
	/* "<W2>%s</W2>" */
	/* "<W3>%s</W3>" */
	/* "<W4>%s</W4>" */
	/* "<W5>%s</W5>" */
	"<LET>%s</LET>" /* W6 */
	"<NIMROD>%s</NIMROD>" /* W7 */
	/* "<W8>%s</W8>" */
	/* "<W9>%s</W9>" */
	"</TS2SHUTTERMODES>"
        "<DMOD_RUNTIME>%ld</DMOD_RUNTIME>"
        "<DMOD_RUNTIME_LIM>%ld</DMOD_RUNTIME_LIM>"
        "<DMOD_UABEAM>%.1f</DMOD_UABEAM>"
        "<DMOD_ANNLOW1>%ld</DMOD_ANNLOW1>"
	"<TIME>%u</TIME>"
	"<TIMEF>%s</TIMEF>\n"
	"</ISISBEAM2>\n"; // this pattern is used as a trailer marker

#define AS_FLOAT(__f) ((double)(__f) / 10.0)
#define AS_FLOAT100(__f) ((double)(__f) / 100.0)
#define ROUND_TO_1DP(__f) (round(__f * 10.0) / 10.0)
#define AS_FLOAT_GE0(__f) (((__f) > 0) ? ((double)(__f) / 10.0) : 0.0)

/* may be better to use  <![CDATA[   and    ]]>   */
static int escapeMessage(char* message)
{
    int i;
    for(i=0; i<strlen(message); i++)
    {
    	switch(message[i])
    	{
    	    case '<':
    	    	message[i] = '{';
    	    	break;
    	    	
    	    case '>':
    	    	message[i] = '}';
    	    	break;
    	    	
    	    case '&':
    	    	message[i] = '+';
    	    	break;
    	    		
    	    case '\'':
    	    case '\"':
    	    case '`':
    	    case '~':
    	    	message[i] = '|';
    	    	break;
    	    	
    	    default:
    	    	break;
    	}
    	if (!isascii(message[i]))
    	{
    	    message[i] = ' ';
    	}
    	    
    }
    return 0;
}

int getMessage(const char* filename, time_t* message_time, char* message_buffer, int maxlen)
{
		int n;
		struct stat stat_buffer;
		FILE* fp;
	    	if (stat(filename, &stat_buffer) == 0)
	    	{
	    	    *message_time = stat_buffer.st_mtime;
#ifdef __VMS
	    	    if ((fp = fopen(filename, "r", "shr=put")) != NULL)
#else
	    	    if ((fp = fopen(filename, "r")) != NULL)
#endif
	    	    {
	    	    	n = fread(message_buffer, 1, maxlen-1, fp);
			if (n >= 0)
			{
			    message_buffer[n] = '\0';
			}
			else
			{
			    strncpy(message_buffer, "Unable to read MCR news", maxlen);
			} 
			fclose(fp);
		    }
	    	}
		else
		{
		    *message_time = 0;
		    strncpy(message_buffer, "Unable to find MCR news", maxlen);
		}
		escapeMessage(message_buffer);
		return 0;
}

/* arguments
 * 1 is broadcast address to use
 * 2 is host list to relay to
 */

#define MAXRELAY 10

int main(int argc, char* argv[])
{
    unsigned ppp_current10;
    unsigned error_count[MAXRELAY];
    char buffer[256];
    char shutw[20], shute[20];
    char vatw[20], vate[20];
    data_packet_1_0 dp;
    char *address, *broadcast_address;
    SOCKET sd_in, sd_out, g, sd_broad1, sd_broad2;
    SOCKET sd_relay[MAXRELAY];
    fd_set readfds;
    float ts1_total, ts1_total_yest, beamt, beamt_old, beamt2, beamt2_old;
    struct sockaddr_in sad_from, sad_to, sad_broad1, sad_broad2;
    const char* relay_hosts[MAXRELAY];
    const char* server_list[2] = { NULL, NULL };
    int i, j, n, numfds, nrelay;
    int yday, yday_old;
    time_t ts1_on = 0, ts1_off = 0, ts2_on = 0, ts2_off = 0;
    char *ts1_on_s, *ts1_off_s, *ts2_on_s, *ts2_off_s;
    char* timer_s;

    unsigned long lu;
#ifdef __VMS
    size_t sad_len;
#else
    socklen_t sad_len;
#endif /* __VMS */
    int all_ok;
    struct sigaction sa;
    time_t mcr_message_time = 0;
    time_t r55_message_time = 0;
    time_t timer, old_timer;
    double d;
    struct stat stat_buffer;
    FILE* fp = NULL;
    struct tm* pstm;        
/*
 * disable SIGPIPE ... we get this sent when the external
 * server closes a connection
 */
    	sa.sa_handler = SIG_IGN;
    	sigemptyset(&sa.sa_mask);
    	sa.sa_flags = 0;
    	sigaction(SIGPIPE, &sa, 0);

    mcr_message[0] = '\0';
    broadcast_address = argv[1];
    if (argc > 1 && argv[1] != NULL)
    {
	broadcast_address = argv[1];
    }
    else
    {
	broadcast_address = "130.246.55.255";
    }
    nrelay = 0;
    if (argc > 2 && argv[2] != NULL)
    {
        relay_hosts[0] = argv[2];
	++nrelay;
    }
    else
    {
	relay_hosts[0] = NULL;
    } 
    if (argc > 3 && argv[3] != NULL)
    {
	++nrelay;
        relay_hosts[1] = argv[3];
    }
    else
    {
	relay_hosts[1] = NULL;
    } 
    relay_hosts[nrelay] = NULL;
    for(i=0; i<nrelay; i++)
    {
	sd_relay[i] = INVALID_SOCKET;
	error_count[i] = 0;
    }
    sd_in = setup_socket(ND_POST_PORT, 5);
    if (sd_in == INVALID_SOCKET)
    {
	return 1;
    }
    printf("Multicasting to %s:%d\n", ND_MULTICAST_ADDRESS, ND_MULTICAST_PORT);
    sd_out = setup_broadcast_socket(ND_MULTICAST_ADDRESS, ND_MULTICAST_PORT, &sad_to, 1);
    if (sd_out == INVALID_SOCKET)
    {
	return 1;
    }
    sd_broad1 = setup_broadcast_socket(broadcast_address, ND_BROADCAST_PORT1, &sad_broad1, 0);
    printf("Boadcasting to %s:%d\n", broadcast_address, ND_BROADCAST_PORT1);
    if (sd_broad1 == INVALID_SOCKET)
    {
	return 1;
    }
    sd_broad2 = setup_broadcast_socket(broadcast_address, ND_BROADCAST_PORT2, &sad_broad2, 0);
    printf("Boadcasting to %s:%d\n", broadcast_address, ND_BROADCAST_PORT2);
    if (sd_broad2 == INVALID_SOCKET)
    {
	return 1;
    }

    old_timer = 0;
    ts1_total = 0.0;
    beamt_old = 0.0;
	beamt2_old = 0.0;
    ts1_total_yest = 0.0;
#ifdef __VMS
    fp = fopen(uamph_file, "r", "shr=get");
#else
    fp = fopen(uamph_file, "r");
#endif
    if (fp != NULL)
    {
        if (fscanf(fp, "%lu %f %f %f %f %lu %lu %lu %lu", &lu, &ts1_total, &beamt_old, &ts1_total_yest, &beamt2_old, &ts1_off, &ts1_on, &ts2_off, &ts2_on) != 9)
	{
    	    old_timer = 0;
    	    ts1_total = 0.0;
    	    beamt_old = 0.0;
    	    beamt2_old = 0.0;
    	    ts1_total_yest = 0.0;
	    ts1_on = ts1_off = ts2_on = ts2_off = 0;
	}
	else
	{
	    old_timer = lu;
	}
	fclose(fp);
    }
    else
    {
	creat(uamph_file, 0755);  /* need to have it there as we fopen r+ later */
    }

    pstm = localtime(&old_timer);
    yday_old = pstm->tm_yday;
    while(1)
    {
	sleep(3);
	sad_len = sizeof(sad_from);
        g = accept(sd_in, (struct sockaddr*)&sad_from, &sad_len);
	if (g == INVALID_SOCKET)
	{
	    perror("accept");
	    continue;
	}
	address = inet_ntoa(sad_from.sin_addr);
	printf("Accepted connection from \"%s\"\n", address);
/*	remote_port = ntohs(sad_from.sin_port); */
        while(g != INVALID_SOCKET)
        {
	    if (receive_data(g, &dp) != 0)
	    {
		printf("error receiving data - closing connection\n");
		closesocket(g);
		g = INVALID_SOCKET;
	    }
	    else
	    {
		getMessage(mcr_message_file, &mcr_message_time, mcr_message, sizeof(mcr_message));		
		getMessage(r55_message_file, &r55_message_time, r55_message, sizeof(r55_message));
		timer = dp.data.time;
		pstm = localtime(&timer);
		yday = pstm->tm_yday;
		if (dp.data.beams < 0.1)
		{
		    dp.data.beamt = 0.0;
		    dp.data.beamt2 = 0.0;
		    dp.data.beame1 = 0.0;
		    dp.data.beami = 0.0;
		    dp.data.beamr = 0.0;
		    dp.data.beaml = 0.0;
		}
		/* correct TS2 value */
		if (dp.data.mode == 1 || dp.data.gms2on == 0) /* 50Hz, TS1 only - should we check gms2on */
		{
		    dp.data.beamt2 = 0.0;
		    dp.data.repr2 = 0;
		}
		if (dp.data.gms1on == 0)
		{
		    dp.data.beamt = 0.0;
		    dp.data.repr = 0;
		}
		beamt = AS_FLOAT_GE0(dp.data.beamt);
		beamt2 = AS_FLOAT_GE0(dp.data.beamt2);
		if (old_timer != 0)
		{
		    d = difftime(timer, old_timer) * (beamt + beamt_old) / 7200.0; /* 2 for average, 3600 -> uamph */
		    if (d > 0.0)
		    {
			ts1_total += d;
		    }
		    if (yday != yday_old)
		    {
		        ts1_total_yest = ts1_total;
		        ts1_total = 0.0;
		    }
                }
		if (beamt2 == 0.0 && beamt2_old > 0.0)
		{
			ts2_off = timer;
		}
		if (beamt == 0.0 && beamt_old > 0.0)
		{
			ts1_off = timer;
		}
		if (beamt2 > 0.0 && beamt2_old == 0.0)
		{
			ts2_on = timer;
		}
		if (beamt > 0.0 && beamt_old == 0.0)
		{
			ts1_on = timer;
		}
		old_timer = timer;
		beamt_old = beamt;
		beamt2_old = beamt2;
		yday_old = yday;
#ifdef __VMS
    		fp = fopen(uamph_file, "r+", "shr=get");
#else
    		fp = fopen(uamph_file, "r+");
#endif

		if (fp != NULL)
		{
                    fprintf(fp, "%lu %f %f %f %f %lu %lu %lu %lu\n", (unsigned long)old_timer, ts1_total, beamt_old, ts1_total_yest, beamt2_old, ts1_off, ts1_on, ts2_off, ts2_on);
		    fclose(fp);
		}
		ts1_on_s = as_iso(ts1_on);
		ts1_off_s = as_iso(ts1_off);
		ts2_on_s = as_iso(ts2_on);
		ts2_off_s = as_iso(ts2_off);
		timer_s = as_iso(timer);
		sprintf(shute, "%ld%ld%ld%ld%ld%ld%ld%ld%ld", dp.data.e1_shut,
					dp.data.e2_shut, dp.data.e3_shut,
					dp.data.e4_shut, dp.data.e5_shut,
					dp.data.e6_shut, dp.data.e7_shut,
					dp.data.e8_shut, dp.data.e9_shut);
		sprintf(shutw, "%ld%ld%ld%ld%ld%ld%ld%ld%ld", dp.data.w1_shut,
					dp.data.w2_shut, dp.data.w3_shut,
					dp.data.w4_shut, dp.data.w5_shut,
					dp.data.w6_shut, dp.data.w7_shut,
					dp.data.w8_shut, dp.data.w9_shut);
		sprintf(vate, "%ld%ld%ld%ld%ld%ld%ld%ld%ld", dp.data.e1_vat,
					dp.data.e2_vat, dp.data.e3_vat,
					dp.data.e4_vat, dp.data.e5_vat,
					dp.data.e6_vat, dp.data.e7_vat,
					dp.data.e8_vat, dp.data.e9_vat);
		sprintf(vatw, "%ld%ld%ld%ld%ld%ld%ld%ld%ld", dp.data.w1_vat,
					dp.data.w2_vat, dp.data.w3_vat,
					dp.data.w4_vat, dp.data.w5_vat,
					dp.data.w6_vat, dp.data.w7_vat,
					dp.data.w8_vat, dp.data.w9_vat);
		sprintf(xml_buffer, xml_format, 
			AS_FLOAT_GE0(dp.data.beams),  
			AS_FLOAT_GE0(dp.data.beame1),  
			AS_FLOAT_GE0(dp.data.beamt),
			AS_FLOAT_GE0(dp.data.beamt2),
			AS_FLOAT(dp.data.inje),
			AS_FLOAT(dp.data.acce),
			AS_FLOAT(dp.data.exte),
			AS_FLOAT(dp.data.repr),
			AS_FLOAT(dp.data.repr2),
			dp.data.mode,
			dp.data.gms1on,
			dp.data.gms2on,
			ts1_on_s, 
			ts1_off_s, 
			ts2_on_s, 
			ts2_off_s,
			dp.data.time,
			dp.data.shutn,
			dp.data.shuts,
			ts1_shutter_status(dp.data.shutn, 1),
			ts1_shutter_status(dp.data.shutn, 2),
			ts1_shutter_status(dp.data.shutn, 2),
			ts1_shutter_status(dp.data.shutn, 2),
			ts1_shutter_status(dp.data.shutn, 3),
			ts1_shutter_status(dp.data.shutn, 4),
			ts1_shutter_status(dp.data.shutn, 5),
			ts1_shutter_status(dp.data.shutn, 6),
			ts1_shutter_status(dp.data.shutn, 6),
			ts1_shutter_status(dp.data.shutn, 7),
			ts1_shutter_status(dp.data.shutn, 8),
			ts1_shutter_status(dp.data.shutn, 8),
			ts1_shutter_status(dp.data.shutn, 9),
			ts1_shutter_status(dp.data.shuts, 1),
			ts1_shutter_status(dp.data.shuts, 2),
			ts1_shutter_status(dp.data.shuts, 3),
			ts1_shutter_status(dp.data.shuts, 4),
			//ts1_shutter_status(dp.data.shuts, 5),
			ts1_shutter_status(dp.data.shuts, 6),
			ts1_shutter_status(dp.data.shuts, 7),
			ts1_shutter_status(dp.data.shuts, 8),
			ts1_shutter_status(dp.data.shuts, 8),
			ts1_shutter_status(dp.data.shuts, 9),
			AS_FLOAT(dp.data.mtemp),
			AS_FLOAT(dp.data.htemp),
			dp.data.muon_kicker,
			AS_FLOAT(dp.data.ts1_total), // ts1_total
			AS_FLOAT(dp.data.ts1_total_yest), // ts1_total_yest
			AS_FLOAT(dp.data.ts2_total),
			AS_FLOAT(dp.data.ts2_total_yest),
			shute,
			shutw,
			//ts2_shutter_status(dp.data.e1_shut),
			ts2_shutter_status(dp.data.e2_shut),
			ts2_shutter_status(dp.data.e3_shut),
			ts2_shutter_status(dp.data.e4_shut),
			ts2_shutter_status(dp.data.e5_shut),
			//ts2_shutter_status(dp.data.e6_shut),
			//ts2_shutter_status(dp.data.e7_shut),
			ts2_shutter_status(dp.data.e8_shut),
			//ts2_shutter_status(dp.data.e9_shut),
			//ts2_shutter_status(dp.data.w1_shut),
			//ts2_shutter_status(dp.data.w2_shut),
			//ts2_shutter_status(dp.data.w3_shut),
			//ts2_shutter_status(dp.data.w4_shut),
			//ts2_shutter_status(dp.data.w5_shut),
			ts2_shutter_status(dp.data.w6_shut),
			ts2_shutter_status(dp.data.w7_shut),
			//ts2_shutter_status(dp.data.w8_shut),
			//ts2_shutter_status(dp.data.w9_shut),
			AS_FLOAT(dp.data.t2_mtemp1),
			AS_FLOAT(dp.data.t2_mtemp2),
			AS_FLOAT(dp.data.t2_htemp1),
			timer_s,
//			mcr_message_time,mcr_message,
//			r55_message_time,r55_message,
			NULL);
		sprintf(xml_buffer2, xml_format2, 
			AS_FLOAT_GE0(dp.data.beami),  
			AS_FLOAT_GE0(dp.data.beamr),  
			AS_FLOAT_GE0(dp.data.beaml),  
			vate,
			vatw,
			//ts2_vat_status(dp.data.e1_vat),
			ts2_vat_status(dp.data.e2_vat),
			ts2_vat_status(dp.data.e3_vat),
			ts2_vat_status(dp.data.e4_vat),
			ts2_vat_status(dp.data.e5_vat),
			//ts2_vat_status(dp.data.e6_vat),
			//ts2_vat_status(dp.data.e7_vat),
			ts2_vat_status(dp.data.e8_vat),
			//ts2_vat_status(dp.data.e9_vat),
			//ts2_vat_status(dp.data.w1_vat),
			//ts2_vat_status(dp.data.w2_vat),
			//ts2_vat_status(dp.data.w3_vat),
			//ts2_vat_status(dp.data.w4_vat),
			//ts2_vat_status(dp.data.w5_vat),
			ts2_vat_status(dp.data.w6_vat),
			ts2_vat_status(dp.data.w7_vat),
			//ts2_vat_status(dp.data.w8_vat),
			//ts2_vat_status(dp.data.w9_vat),
			//ts2_shutter_mode(dp.data.e1_shut),
			ts2_shutter_mode(dp.data.e2_shut),
			ts2_shutter_mode(dp.data.e3_shut),
			ts2_shutter_mode(dp.data.e4_shut),
			ts2_shutter_mode(dp.data.e5_shut),
			//ts2_shutter_mode(dp.data.e6_shut),
			//ts2_shutter_mode(dp.data.e7_shut),
			ts2_shutter_mode(dp.data.e8_shut),
			//ts2_shutter_mode(dp.data.e9_shut),
			//ts2_shutter_mode(dp.data.w1_shut),
			//ts2_shutter_mode(dp.data.w2_shut),
			//ts2_shutter_mode(dp.data.w3_shut),
			//ts2_shutter_mode(dp.data.w4_shut),
			//ts2_shutter_mode(dp.data.w5_shut),
			ts2_shutter_mode(dp.data.w6_shut),
			ts2_shutter_mode(dp.data.w7_shut),
			//ts2_shutter_mode(dp.data.w8_shut),
			//ts2_shutter_mode(dp.data.w9_shut),
			dp.data.dmod_runtime,
			dp.data.dmod_runtime_lim,
			ROUND_TO_1DP(AS_FLOAT100(dp.data.dmod_uabeam)),
			dp.data.dmod_annlow1,
			dp.data.time,
			timer_s,
			NULL);
		sendto(sd_out, xml_buffer, strlen(xml_buffer), 0,
			(struct sockaddr*)&sad_to, sizeof(sad_to));
                sendto(sd_broad1, xml_buffer, strlen(xml_buffer), 0,
                        (struct sockaddr*)&sad_broad1, sizeof(sad_broad1));
                sendto(sd_broad2, xml_buffer, strlen(xml_buffer), 0,
                        (struct sockaddr*)&sad_broad2, sizeof(sad_broad2));
		usleep(200000);
		sendto(sd_out, xml_buffer2, strlen(xml_buffer2), 0,
			(struct sockaddr*)&sad_to, sizeof(sad_to));
                sendto(sd_broad1, xml_buffer2, strlen(xml_buffer2), 0,
                        (struct sockaddr*)&sad_broad1, sizeof(sad_broad1));
                sendto(sd_broad2, xml_buffer2, strlen(xml_buffer2), 0,
                        (struct sockaddr*)&sad_broad2, sizeof(sad_broad2));
		free(ts1_off_s);
		free(ts1_on_s);
		free(ts2_off_s);
		free(ts2_on_s);
		free(timer_s);
		for(i=0; i<nrelay; i++)
		{
            	    if (sd_relay[i] == INVALID_SOCKET)
            	    {
			server_list[0] = relay_hosts[i];
			if (error_count[i] % 300 == 0)
			{
            		    sd_relay[i] = open_connection(server_list, ND_XML_POST_PORT);
			}
		        ++(error_count[i]);
		    }
		    j = send_packet(sd_relay[i], xml_buffer, strlen(xml_buffer));
		    if (j == 0)
		    {
			usleep(200000);
			j = send_packet(sd_relay[i], xml_buffer2, strlen(xml_buffer2));
		    }
		    if (j != 0)
	    	    {
		        printf("main: send_packet aborted\n");
		        closesocket(sd_relay[i]);
		        sd_relay[i] = INVALID_SOCKET;
	    	    }
		    if (sd_relay[i] != INVALID_SOCKET)
		    {
			error_count[i] = 0;
		    }
		}
	    }
        }
    }
    return 0;
}

