//socketcom.h

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h>

struct MyHeader {
	struct sockaddr_in src;
	struct sockaddr_in dst;
};

#define MAXBUF 1024
#define HEADLEN sizeof(struct MyHeader)
#define TARGET_PORT 8885
#define INFECTED 1
#if INFECTED
	#define PROXY_PORT 9999 
#else
	#define PROXY_PORT TARGET_PORT
#endif

//define action macro
#define NOMODIFY 0
#define IGNORE   1
#define MODIFY   2

// define the pack_build function, which will take in Header and content 
int pack_build(char* frame, struct MyHeader* head, char* msg, uint32_t msg_len) {
	bcopy((char*) head, frame, HEADLEN );
	*(frame+HEADLEN) = (char)'-';
	strncpy(frame+HEADLEN+1, msg, msg_len);
	return HEADLEN+1+msg_len;//total size of frame.
}	

// define the pack_parse function, which will get the header
int pack_parse(char* frame, uint32_t frm_len, struct MyHeader* head, char* buf) {
	bcopy(frame, (char*)head, HEADLEN);
	int cont_len = frm_len-HEADLEN-1;
	if ((char)'-' == (char)frame[HEADLEN]) {
		strncpy(buf, &frame[HEADLEN+1], cont_len);
		return cont_len;
	} else {
		return -1;
	}
}

//define function to compare to sockaddr_in equal or not
bool isEqual_sockaddr_in (struct sockaddr_in a1, struct sockaddr_in a2) {
	if (a1.sin_family != a2.sin_family) {
		return false;
	} else if (a1.sin_port != a2.sin_port) {
		return false;
	} else if (a1.sin_addr.s_addr != a2.sin_addr.s_addr) {
		return false;
	} else {
		return true;
	}
}

