/* Portable Ping, pinging made easy
 *
 * PPing is a C library allowing to make ping requests. It aims at being
 * and avoid requiring non-necessary privileges. As such, you can use the same
 * simple interface on Linux, Windows and OSX without requiring root or
 * administrator permissions.
 *
 * It also comes without dependencies outside of standard system libraries,
 * no include path, nor library to install.
 *
 * If you want a header-only version, just include the .c
 *
 * Author: Sylvain Gadrat
 * License: WTFPL v2 (except copy-pasted parts from random locations, they are the property of their authors)
 */

/* How does it work?
 *
 * This lib knows three ways to emit ping requests: raw socket, icmp socket and IcmpSendEcho.
 *
 *  - raw socket: universally supported, but requires root/administrator privileges.
 *  - icmp socket: supported by Linux and some other Unix, does not require root privileges.
 *  - IcmpSendEcho: supported by Windows (since Windows 2000 and Windows XP), does not require administrator privileges.
 *
 * The lib selects its implementation based on whatever works.
 */

#ifdef PPING_DEBUG
#	define PPING_DBG(...) fprintf(stderr, __VA_ARGS__)
#else
#	define PPING_DBG(...)
#endif

#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
	// include network API
#	include <winsock2.h> // Most network things
#	include <ws2tcpip.h> // getnameinfo and socket_len_t
#	pragma comment(lib, "Ws2_32.lib") // Something related to linking

	// IcmpSendEcho
#	include <iphlpapi.h>
#	include <icmpapi.h>
#	pragma comment(lib, "iphlpapi.lib") //FIXME better link dynimically with this one: the lib name is not the same on all windows

	// If the OS doesn't declare it, do it ourself (copy-pasted from GNU C Library, license: LGPL)
#	include <stdint.h>
	struct icmphdr
	{
		uint8_t type;		/* message type */
		uint8_t code;		/* type sub-code */
		uint16_t checksum;
		union
		{
			struct
			{
				uint16_t	id;
				uint16_t	sequence;
			} echo;			/* echo datagram */
			uint32_t	gateway;	/* gateway address */
			struct
			{
				uint16_t	__unused;
				uint16_t	mtu;
			} frag;			/* path mtu discovery */
			/*uint8_t reserved[4];*/
		} un;
	};

	// If the OS doesn't declare it, do it ourself
#	define ICMP_ECHO 8

	// Fix slightly changed names
#	define SOL_IP IPPROTO_IP

	// Beloved bzero
#	define bzero(b,len) (memset((b), '\0', (len)), (void) 0)

	// Init networking
	static int net_init(void) {
		WSADATA wsa_data;
		if (WSAStartup(MAKEWORD(2, 2), &wsa_data)) {
			PPING_DBG("WSA init failed\n");
			return 1;
		}
		return 0;
	}

	// Same API but not the same type for byte pointers
	static char* net_payload(void* p) {
		return (char*)p;
	}

	// errno underused on Windows
	static void merror(char const* m) {
		//TODO get a description of the error, in the meantime go check it here https://docs.microsoft.com/fr-fr/windows/win32/winsock/windows-sockets-error-codes-2
		PPING_DBG("%s: error %d\n", m, WSAGetLastError());
	}

#else
#	include <sys/socket.h>
#	include <netinet/in.h>
#	include <arpa/inet.h>
#	include <netdb.h>
#	include <unistd.h>
#	include <netinet/ip_icmp.h>
#	include <errno.h>

	static  int net_init(void) { return 0; }
	static  void* net_payload(void* p) { return p; }
	static  void merror(char const* m) {
#		ifdef PPING_DEBUG
			perror(m);
#		endif
	}
#endif

// Define the Packet Constants ping packet size
#define PING_PKT_S 64

// Automatic port number
#define PORT_NO 0

// Gives the timeout delay for receiving packets in seconds
#define RECV_TIMEOUT 1

// Length to reserve for IP header when manipulating RAW sockets
// Note: That's the minimal and typical length, actual length is stored in byte 0 if the IP header.
//       Not sure if the case of a larger header can happen.
#define IP_HEADER_LEN 20

#ifdef __cplusplus
extern "C" {
#endif

struct ping_pkt {
	char fill[IP_HEADER_LEN]; // Let at least enough space for an IP header (there may be padding after this field, don't use it to parse IP header)
	struct icmphdr hdr;
	char msg[PING_PKT_S-sizeof(struct icmphdr)];
};

struct pping_s {
	struct sockaddr_in addr;
	int socket;
	int socket_is_raw;

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
	HANDLE icmp_file;
#endif
};

// Shamelessly copy-pasted from https://www.geeksforgeeks.org/ping-in-c/
static unsigned short checksum(void *b, int len) {
	unsigned short *buf = (unsigned short *)b;
	unsigned int sum = 0;
	unsigned short result;

	for (sum = 0; len > 1; len -= 2) {
		sum += *buf++;
	}
	if (len == 1) {
		sum += *(unsigned char*)buf;
	}
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	result = ~sum;
	return result;
}

static int dns_lookup(char const*addr_host, struct sockaddr_in *addr_con) {
	struct hostent *he;

	he = gethostbyname(addr_host);
	if (he == NULL) {
		merror("failed to resolve host");
		return 1;
	}

	addr_con->sin_family = he->h_addrtype;
	addr_con->sin_port = htons(PORT_NO);
	addr_con->sin_addr.s_addr  = *(long*)he->h_addr;
	return 0;
}

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
// Windows specific ping implementation
static int pping_ping_icmp_send_echo(HANDLE icmp_file, struct sockaddr_in *ping_addr) {
	unsigned long ipaddr = ping_addr->sin_addr.s_addr;
	DWORD r = 0;
	char ping_msg[PING_PKT_S];
	LPVOID reply_buffer = NULL;
	DWORD reply_size = 0;
	size_t i;

	for (i = 0; i < sizeof(ping_msg)-1; i++) {
		ping_msg[i] = i+'0';
	}
	ping_msg[i] = 0;

	reply_size = sizeof(ICMP_ECHO_REPLY) + sizeof(ping_msg);
	reply_buffer = (VOID*) malloc(reply_size);
	if (reply_buffer == NULL) {
		PPING_DBG("failed to allocate reply buffer\n");
		return 1;
	}


	r = IcmpSendEcho(
		icmp_file, ipaddr,
		ping_msg, sizeof(ping_msg),
		NULL,
		reply_buffer, reply_size,
		RECV_TIMEOUT * 1000
	);
	if (r == 0) {
		free(reply_buffer);
		if (GetLastError() == WSA_QOS_ADMISSION_FAILURE) {
			return 2;
		}else {
			PPING_DBG("IcmpSendEcho error: %ld\n", GetLastError());
			return 1;
		}
	}

	free(reply_buffer);
	return 0;
}
#endif

static int pping_ping_posix(int ping_sockfd, struct sockaddr_in *ping_addr, int sock_is_raw) {
	int ttl_val=56, i;
	unsigned int addr_len;

	struct ping_pkt pckt;
	struct sockaddr_in r_addr;
	struct timeval tv_out;
	tv_out.tv_sec = RECV_TIMEOUT;
	tv_out.tv_usec = 0;

	// Set socket TTL
	if (setsockopt(ping_sockfd, SOL_IP, IP_TTL, net_payload(&ttl_val), sizeof(ttl_val)) != 0) {
		PPING_DBG("failed to set TTL\n");
		return 1;
	}

	// Craft ICMP echo request
	bzero(&pckt, sizeof(pckt));

	pckt.hdr.type = ICMP_ECHO;
	if (sock_is_raw) {
		pckt.hdr.un.echo.id = getpid();
	}else {
		pckt.hdr.un.echo.id = 0;
	}

	for (i = 0; i < sizeof(pckt.msg)-1; i++) {
		pckt.msg[i] = i+'0';
	}

	pckt.msg[i] = 0;
	pckt.hdr.un.echo.sequence = 0; // TODO may be configurable
	pckt.hdr.checksum = checksum(&pckt.hdr, sizeof(pckt.hdr) + sizeof(pckt.msg));

	// Send request
	if (sendto(ping_sockfd, net_payload(&pckt.hdr), sizeof(pckt.hdr) + sizeof(pckt.msg), 0, (struct sockaddr*) ping_addr, sizeof(*ping_addr)) <= 0) {
		perror("send failed");
		return 1;
	}

	// Wait for packet
	setsockopt(ping_sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv_out, sizeof tv_out);

	char* rcv_buf = (char*)&pckt.hdr;
	size_t rcv_buf_len = sizeof(pckt.hdr) + sizeof(pckt.msg);
	if (sock_is_raw) {
		rcv_buf -= IP_HEADER_LEN;
		rcv_buf_len += IP_HEADER_LEN;
	}

	addr_len = sizeof(r_addr);
	if (recvfrom(ping_sockfd, net_payload(rcv_buf), rcv_buf_len, 0, (struct sockaddr*)&r_addr, &addr_len) <= 0) {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
		int timeout_error = (WSAGetLastError() == WSAETIMEDOUT);
#else
		int timeout_error = (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINPROGRESS);
#endif
		if (timeout_error) {
			return 2;
		}else {
			merror("packet receive failed");
			return 1;
		}
	}
	
	// Check received packet
	//TODO Check sequence number
	if(!(pckt.hdr.type == 0 && pckt.hdr.code == 0)) {
		PPING_DBG("Error..Packet received with ICMP type %d code %d\n", pckt.hdr.type, pckt.hdr.code);
		return 1;
	}

	return 0;
}

struct pping_s* pping_init(char const* addr) {
	struct pping_s* pping;
	int skip_socket_creation;

	// Sanity check
	if (addr == NULL) {
		PPING_DBG("empty address\n");
		return NULL;
	}

	// Winsock initialization
	if (net_init() != 0) {
		return NULL;
	}

	// Allocate pping
	pping = (struct pping_s*)malloc(sizeof(struct pping_s));
	pping->socket = -1;
	pping->socket_is_raw = 0;
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
	pping->icmp_file = INVALID_HANDLE_VALUE;
#endif

	// DNS lookup
	if (dns_lookup(addr, &pping->addr)) {
		merror("DNS lookup failed");
		free(pping);
		return NULL;
	}

	// ICMP file creation
	skip_socket_creation = 0;
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
	pping->icmp_file = IcmpCreateFile();
	if (pping->icmp_file == INVALID_HANDLE_VALUE) {
		PPING_DBG("failed to open icmp file, fallback to posix implementation (you will need administrator privilege): %ld\n", GetLastError());
	}else {
		skip_socket_creation = 1;
	}
#endif

	// Socket creation
	if (!skip_socket_creation) {
		pping->socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
		if (pping->socket < 0) {
			pping->socket_is_raw = 1;
			pping->socket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
			if (pping->socket < 0) {
				merror("failed to create socket");
			}
		}

		if (pping->socket >= 0) {
			PPING_DBG("socket created raw=%d\n", pping->socket_is_raw);
		}
	}

	return pping;
}

int pping_ping(struct pping_s* pping) {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
	// IcmpSendEcho implementation
	if (pping->icmp_file != INVALID_HANDLE_VALUE) {
		return pping_ping_icmp_send_echo(pping->icmp_file, &pping->addr);
	}
#endif

	// Posix socket implementations
	if (pping->socket >= 0) {
		return pping_ping_posix(pping->socket, &pping->addr, pping->socket_is_raw);
	}

	// Out of idea
	PPING_DBG("no usable ping implementation found\n");
	return 1;
}

void pping_free(struct pping_s* pping) {
	free(pping);
}

int pping_easy_ping(char const* addr) {
	struct pping_s* pping;
	int r;

	pping = pping_init(addr);
	if (pping == NULL) {
		return 1;
	}
	r = pping_ping(pping);
	pping_free(pping);
	return r;
}

#ifdef __cplusplus
}
#endif
