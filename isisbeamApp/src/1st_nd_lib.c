#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#ifdef _WIN32
#	include <winsock2.h>
#	include <ws2tcpip.h>
#   	define INTERNET_SOCKET PF_INET
#else
#	include <unistd.h>
#	include <sys/socket.h>
#	include <netdb.h>
#	include <netinet/in.h>
#	include <arpa/inet.h>
#	include <pwd.h>
#   define SOCKET int
#   define INVALID_SOCKET -1
#   define INTERNET_SOCKET AF_INET
#   define closesocket close
#endif
#include <string.h>
#ifdef _WIN32
#include <time.h>
#else
#include <sys/time.h>
#endif /* _WIN32 */

#ifdef x__VMS
struct ip_mreq {
        struct in_addr  imr_multiaddr;  /* IP multicast address of group */
        struct in_addr  imr_interface;  /* local IP address of interface */
};
#endif /* __VMS */

#include "1st_nd_post.h"

SOCKET open_connection(const char* servers[], short port)
{
    int i, n, nfds, replen, max_replen, connected = 0;
    int setkeepalive = 1;
    struct timeval timeout = { 0, 0 };
    SOCKET sd = INVALID_SOCKET;
    struct hostent *hostp;
    struct sockaddr_in address;
    int enable = 1, disable = 0;
    for(i=0; !connected && (servers[i] != NULL) && (strlen(servers[i]) > 0) ; i++)
    {
	printf("open_connection: trying to establish connection to \"%s\"\n", servers[i]);
        if ((sd = socket(INTERNET_SOCKET, SOCK_STREAM, 0)) == INVALID_SOCKET)
        {
	    perror("open_connection: socket");
	    return INVALID_SOCKET;
        }
        if ( setsockopt(sd, SOL_SOCKET, SO_KEEPALIVE, &setkeepalive, sizeof(setkeepalive)) == -1 )
        {
	    perror("setsockopt: keepalive");
	    closesocket(sd);
            return INVALID_SOCKET;
        }
        timeout.tv_sec = 3;
        if ( setsockopt(sd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) == -1 )
        {
	    perror("setsockopt: sndtimeo");
	    closesocket(sd);
            return INVALID_SOCKET;
        }
        timeout.tv_sec = 10;
        if ( setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1 )
        {
	    perror("setsockopt: rcvtimeo");
	    closesocket(sd);
            return INVALID_SOCKET;
        }
//        if (ioctl(sd, FIONBIO, &enable) == -1)
//        {
//	    perror("ioctl: FIONBIO");
//	    closesocket(sd);
//            return INVALID_SOCKET;
//        }
        memset(&address, 0, sizeof(struct sockaddr_in));
        address.sin_family = AF_INET;
        if ((hostp = gethostbyname(servers[i])) == NULL)
        {
	    printf("open_connection: gethostbyname failed for \"%s\"\n", servers[i]);
	    closesocket(sd);
	    sd = INVALID_SOCKET;
	    continue;
        }
        memcpy(&(address.sin_addr.s_addr), hostp->h_addr, hostp->h_length);
        address.sin_port = htons(port);
        if (connect(sd, (struct sockaddr*)&address, sizeof(address)) == -1) 
        {
	    perror("open_connection: connect");
	    closesocket(sd);
	    sd = INVALID_SOCKET;
        }
	else
	{
#if 0
            if (ioctl(sd, FIONBIO, &disable) == -1)
            {
	        perror("ioctl: FIONBIO");
	        closesocket(sd);
                sd = INVALID_SOCKET;
            }
            else
            {
	        connected = 1;
	    }
#else
	    connected = 1;
#endif
	}
    }
    if (connected)
    {
	return sd;
    }
    else
    {
	if (sd != INVALID_SOCKET)
	{
	    closesocket(sd);
	}
	return INVALID_SOCKET;
    }
}

int initialise_data_packet(data_header* dh, short major, short minor)
{
        dh->major = major;
        dh->minor = minor;
	dh->len = 0;
	return 0;
}

int send_packet(SOCKET sd, const char* data, int n)
{
    struct timeval delay = { 5, 0};
    int numfds = FD_SETSIZE;
    int stat;
    fd_set writefds;
    if (sd == INVALID_SOCKET)
    {
	return 1;
    }
    FD_ZERO(&writefds);
    FD_SET(sd, &writefds);
    if (select(numfds, NULL, &writefds, NULL, &delay) <= 0)
    {
	printf("send_packet: select timed out after %ld seconds\n", delay.tv_sec);
        return 1;
    }
    stat = send(sd, data, n, 0);
    if (stat != n)
    {
	perror("send");
	return 1;
    }
    else
    {
	return 0;
    }
}

int send_data(SOCKET sd, data_packet_1_0* dp)
{
    struct timeval delay = { 5, 0};
    int numfds = FD_SETSIZE;
    int stat, n;
    char* data;
    fd_set writefds;
    FD_ZERO(&writefds);
    FD_SET(sd, &writefds);
    if (select(numfds, NULL, &writefds, NULL, &delay) <= 0)
    {
	printf("send_data: select timed out after %ld seconds\n", delay.tv_sec);
        return 1;
    }
    n = dp->header.len + sizeof(dp->header) + sizeof(dp->check);
    data = (char*)malloc(n);
    memcpy(data, &(dp->header), sizeof(dp->header));
    memcpy(data+sizeof(dp->header), &(dp->data), dp->header.len);
    memcpy(data+sizeof(dp->header)+dp->header.len, &(dp->check), sizeof(dp->check));
    stat = send(sd, data, n, 0);
    free(data);
    if (stat != n)
    {
	return 1;
    }
    else
    {
	return 0;
    }
}

/* wait until have n bytes of data */
int receive_data_pattern(SOCKET sd, void* data, int maxdata, const char* pattern)
{
    int stat, nread = 0, numfds, sock_size;
    struct sockaddr_in sockin;
    fd_set readfds;
    struct timeval delay = { 180, 0};
    numfds = FD_SETSIZE;
    while(1)
    {
        FD_ZERO(&readfds);
        FD_SET(sd, &readfds);
        if (select(numfds, &readfds, NULL, NULL, &delay) <= 0)
        {
	    printf("receive_data_pattern: select timed out after %ld seconds\n", delay.tv_sec);
            return 1;
        }
        stat = recv(sd, (char*)data + nread, maxdata - nread, 0);
        if (stat <= 0)
        {
	    perror("recv");
	    return 1;
        }
	nread += stat; 
	if (nread == maxdata)
	{
	    printf("too much data\n");
	    return 1;
	}
	((char*)data)[nread] = '\0';
	if (strstr((char*)data, pattern) != NULL)
	{
	    return 0;
	}
    }
}


/* wait until have n bytes of data */
int receive_data_size(SOCKET sd, void* data, int n)
{
    int stat, nread = 0, numfds, sock_size;
    struct sockaddr_in sockin;
    fd_set readfds;
    struct timeval delay = { 30, 0};
    numfds = FD_SETSIZE;
    while(1)
    {
        FD_ZERO(&readfds);
        FD_SET(sd, &readfds);
        if (select(numfds, &readfds, NULL, NULL, &delay) <= 0)
        {
	    printf("receive_data_size: select timed out after %ld seconds\n", delay.tv_sec);
            return 1;
        }
        stat = recv(sd, (char*)data + nread, n - nread, 0);
#if 0
    chsr * address = inet_ntoa(sockin.sin_addr);
    unsigned long add = sockin.sin_addr.s_addr; /* address in network order */
    host = gethostbyaddr((char*)&add, 4, AF_INET);
    print host->h_name
#endif
        if (stat <= 0)
        {
	    return 1;
        }
	nread += stat; 
	if (nread == n)
	{
	    return 0;
	}
    }
}

int receive_data(SOCKET sd, data_packet_1_0* dp)
{
    char* data;
    if (receive_data_size(sd, &(dp->header), sizeof(dp->header)) != 0)
    {
	printf("receive_data: error reading header\n");
	return 1;
    }
    if (dp->header.len != sizeof(dp->data))
    {
	printf("receive_data: packet size mismatch (got %d, expected %d)\n", dp->header.len, sizeof(dp->data));
	printf("receive_data: there is probably a mismatch in the data_packet_1_0 definitions on this and\n");
	printf("receive_data: the ISIS controls sending computer\n");
	return 1;
    }
    data = (char*)malloc(dp->header.len);
    if (receive_data_size(sd, data, dp->header.len) != 0)
    {
	printf("receive_data: error reading data\n");
	free(data);
	return 1;
    }
    if (receive_data_size(sd, &(dp->check), sizeof(dp->check)) != 0)
    {
	printf("receive_data: error reading checksum\n");
	free(data);
	return 1;
    }
    memcpy(&(dp->data), data, sizeof(dp->data));
    free(data);
    return 0;
}

int receive_data_udp(SOCKET sd, char* dp, int n)
{
    struct sockaddr_in sockin;
    int stat;
#ifndef __unix
    unsigned sock_size;
#else
    socklen_t sock_size;
#endif
    sock_size = sizeof(sockin);
    stat = recvfrom(sd, dp, n, 0, (struct sockaddr*)&sockin, &sock_size);
    if (stat <= 0)
    {
	return -1;
    }
    else
    {
	return stat;
    }
}

SOCKET setup_socket(unsigned short port, int num_listen)
{
    SOCKET sd;
    struct sockaddr_in sin;
    if ( (sd = socket(INTERNET_SOCKET, SOCK_STREAM, 0)) == -1)
    {
	perror("socket");
        return INVALID_SOCKET;
    }
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = INTERNET_SOCKET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(port);
    if (bind(sd, (struct sockaddr*)&sin, sizeof(sin)) == -1)
    {
	perror("bind");
        closesocket(sd);
        return INVALID_SOCKET;
    }
    if (listen(sd, num_listen) == -1)
    {
	perror("listen");
        closesocket(sd);
        return INVALID_SOCKET;
    }
    return sd;
}

SOCKET setup_udp_socket(unsigned short port, int multicast)
{
    int setreuse = 1;
    SOCKET sd;
    struct sockaddr_in sin;
    struct ip_mreq mreq;
    if ( (sd = socket(INTERNET_SOCKET, SOCK_DGRAM, 0)) == -1 )
    {
	perror("socket");
        return INVALID_SOCKET;
    }
#ifdef __VMS
    if ( setsockopt(sd, SOL_SOCKET, SO_REUSEPORT, &setreuse, sizeof(setreuse)) == -1 )
    {
	perror("setsockopt: reuseport");
	closesocket(sd);
        return INVALID_SOCKET;
    }
#else
    if ( setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &setreuse, sizeof(setreuse)) == -1 )
    {
	perror("setsockopt: reuseaddr");
	closesocket(sd);
        return INVALID_SOCKET;
    }
#endif
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = INTERNET_SOCKET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(port);
    if (bind(sd, (struct sockaddr*)&sin, sizeof(sin)) == -1)
    {
	perror("bind");
        closesocket(sd);
        return INVALID_SOCKET;
    }
    if (multicast)
    {
	mreq.imr_multiaddr.s_addr = inet_addr(ND_MULTICAST_ADDRESS);
	mreq.imr_interface.s_addr = INADDR_ANY;
	if (setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) == -1)
	{
	    perror("setsockopt: add multicast membership");
	    closesocket(sd);
	    return INVALID_SOCKET;
	}
    }
    return sd;
}

// On success, return socket file descriptor and populate sockaddr_in
// On error, Return -1
SOCKET setup_broadcast_socket(const char* address, short port, struct sockaddr_in* sockin, int multicast)
{
    SOCKET s;
    int one = 1;
    u_char ttl;
    struct hostent* hostp;
    struct in_addr ifaddress;
#ifdef _WIN32
    typedef  unsigned long in_addr_t;
#endif
    in_addr_t add;
    add = inet_addr(address);
    if ( add == (in_addr_t)-1 )
    {
	return -1;
    }
    if (multicast)
    {
        ifaddress.s_addr = add;
    }
    hostp = malloc(sizeof(struct hostent));
    hostp->h_addr_list = malloc(sizeof(char*));
    hostp->h_addr_list[0] = (char*)&add;
    hostp->h_length = sizeof(in_addr_t);
    if ( (s = socket(INTERNET_SOCKET, SOCK_DGRAM, 0)) == -1 )
    {
	return -1;
    }
    memset(sockin, 0, sizeof(struct sockaddr_in));
    sockin->sin_family = AF_INET;
    sockin->sin_port = htons(port);
    memcpy(&(sockin->sin_addr.s_addr), hostp->h_addr_list[0], hostp->h_length);
    if (multicast)
    {
        ttl = 2;
        if (setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) == -1)
        {
	    perror("setsockopt: multicast ttl");
        }
#if 0
        if (setsockopt(s, IPPROTO_IP, IP_MULTICAST_IF, &ifaddress, sizeof(ifaddress)) == -1)
        {
	    perror("setsockopt: multicast if");
        }
#endif
    }
    else
    {
        if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, &one, sizeof(one)) == -1)
        {
	    perror("setsockopt: broadcast");
        }
    }
    return s;
}


/*
 * simple XML parser 
 *                     xml_parse("<beam>10.0</beam>","beam")   will return a malloced "10.0"
 * returns NULL on error
 */
char* xml_parse(const char* input_string, const char* token)
{
    char *start_token, *end_token, *result = NULL;
    char *start_pos, *end_pos;
    int n, len_tok;
    if (input_string == NULL || token == NULL)
    {
	return NULL;
    }
    len_tok = strlen(token);
    start_token = malloc(len_tok+2+1);
    if (start_token == NULL)
    {
	return NULL;
    }
    end_token = malloc(len_tok+3+1);
    if (end_token == NULL)
    {
	return NULL;
    }
    sprintf(start_token, "<%s>", token);
    sprintf(end_token, "</%s>", token);
    start_pos = strstr(input_string, start_token);
    end_pos = strstr(input_string, end_token);
    if (start_pos != NULL && end_pos != NULL)
    {
	n = (end_pos - start_pos) - len_tok - 2;
	result = malloc(n+1);
	if (result != NULL)
	{
	    strncpy(result, start_pos + len_tok + 2, n);
	    result[n] = '\0';
	}
    }
    free(start_token);
    free(end_token);
    return result;
}

static void junk_reply(sd)
{
    static char buffer[1024];
    struct timeval delay = { 2, 0};
    int nread = 1, numfds;
    fd_set readfds;
    while(nread > 0)
    {
        FD_ZERO(&readfds);
        FD_SET(sd, &readfds);
        numfds = FD_SETSIZE;
	nread = 0;
        if (select(numfds, &readfds, NULL, NULL, &delay) > 0)
	{
	    nread = recv(sd, buffer, sizeof(buffer), 0);
	    buffer[nread] = '\0';
/*	    printf("receive: %s\n", buffer); */
	}
    }
}

static void send_mess(SOCKET sd, const char* format, ... )
{
    static char buffer[1024];
    va_list ap;
    va_start(ap, format);
    vsprintf(buffer, format, ap);
    va_end(ap);
/*    printf("send: %s", buffer); */
    send(sd, buffer, strlen(buffer), 0);
    junk_reply(sd);
}

void send_mail(const char* from, const char* to, const char* message)
{
    	SOCKET sd;
	char host_name[256];
	const char* servers[] = { "localhost", "hathor.nd.rl.ac.uk", "thoth.nd.rl.ac.uk", NULL };
	if (gethostname(host_name, sizeof(host_name)) != 0)
	{
	    strcpy(host_name, "localhost");
	}
	sd = open_connection(servers, 25);
	if (sd != INVALID_SOCKET)
	{
	    send_mess(sd, "HELO %s\r\n", host_name);
	    send_mess(sd, "MAIL FROM:<%s>\r\n", from);
	    send_mess(sd, "RCPT TO:<%s>\r\n", to);
	    send_mess(sd, "DATA\r\n");
	    send_mess(sd, "%s\r\n.\r\n", message);
	    send_mess(sd, "QUIT\r\n");
	    closesocket(sd);
	}
	else
	{
	    printf("send_mail: error opening connection\n");
	}
}
