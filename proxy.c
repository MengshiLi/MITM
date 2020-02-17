#include "sockcom.h"

int main(int argc, const char * argv[]){
	//int seed = time(NULL);
	//srand(seed);
	int act_up = 0;
	int act_dw = 0;
	int nitems = 0;

	int sockfd_dw, sockfd_up, newsockfd;
	struct sockaddr_in client_dw;
	socklen_t len_client_dw = sizeof(client_dw);
	struct sockaddr_in proxy;// comm btw client and proxy, proxy as server-end
	
	struct sockaddr_in client_up;
	socklen_t len_client_up = sizeof(client_up);
	struct sockaddr_in target;//comm btw proxy and target, proxy as clien-end
	struct hostent *hp, *gethostbyname();
	
	struct MyHeader sendHead_dw;
	struct MyHeader recvHead_dw;
	struct MyHeader sendHead_up;
	struct MyHeader recvHead_up;

	uint32_t frame_len_dw, buf_len_dw, msg_len_dw, count_dw;
	char frame_dw[MAXBUF];
	char buf_dw[MAXBUF-HEADLEN-1];
	char msg_dw[MAXBUF-HEADLEN-1];
	char *proxyMark = "@@@";
	int mark_len = strlen(proxyMark);
	char *outService = "Server Busy Now, Try Later.";
	int warn_len = strlen(outService);
	int flag_dw_ign = 0;
	
	uint32_t frame_len_up, buf_len_up, msg_len_up, count_up;
	char frame_up[MAXBUF];
	char buf_up[MAXBUF-HEADLEN-1];
	char msg_up[MAXBUF-HEADLEN-1];
	
	//***** socket between client and proxy
	if((sockfd_dw = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		printf("Proxy socket() error for downstream.\n");
		return -1;
	}
	
	proxy.sin_family = AF_INET;
	proxy.sin_addr.s_addr = htonl(INADDR_ANY);
	proxy.sin_port = htons(PROXY_PORT); 
	
	if(bind(sockfd_dw, (struct sockaddr *) &proxy, sizeof(proxy)) < 0){
		printf("Proxy bind() error. \n");
		return -1;
	}
	
	if( listen(sockfd_dw, 5) == 0)
		printf("Proxy Listening at port: %d ... \n",ntohs(proxy.sin_port));
	else{
		printf("Proxy Error Listening...\n");
		return -1;
	}
	
	//***** socket between proxy and target
	if( (sockfd_up = socket(AF_INET, SOCK_STREAM, 0)) < 0 ){
		printf("Proxy socket() error for upstream.\n");
		return -1;
	}

	target.sin_family = AF_INET;
	// get server addr via IP.
	if (inet_pton(AF_INET, argv[1], &target.sin_addr) <= 0) {
		printf("inet_pton error occurred\n");
		return -1;
	}
	target.sin_port = htons(atoi(argv[2]));
	// inet_aton("127.0.0.1", &(target.sin_addr));
	// target.sin_port = htons(TARGET_PORT);
	
	if(connect(sockfd_up, (struct sockaddr*) &target, sizeof(target)) < 0){
		printf("Proxy connect() error to upstream target.\n");
		return -1;
	}
	// get the local address, client won't bind port.	
	if( getsockname(sockfd_up, (struct sockaddr*) &client_up, &len_client_up) == -1) {
	       printf("Proxy getsockname() error.\n");
	       printf("error num:%d\n",errno);
	       return -1;
	}


	for (;;) {
		//blocking, wait until someone request a connect.
		newsockfd = accept(sockfd_dw, (struct sockaddr*) 0, (socklen_t*) 0);

		if(!fork()){//in the child process.
			close(sockfd_dw);// parent is listening, thus closed here.
			
			//first get client socket addr:
			if( getpeername(newsockfd, (struct sockaddr*) &client_dw, &len_client_dw) == -1) {
	       			printf("Proxy getpeername() error.\n");
	       			printf("error num:%d\n",errno);
	       			return -1;
			}
			
			bzero(frame_dw,sizeof(frame_dw));
			//hack logic starts here:
			//1. receive package from client.
			while( (count_dw = recv(newsockfd, frame_dw, sizeof(frame_dw), 0)) >= 0){
				flag_dw_ign = 0;
				bzero(buf_dw, sizeof(buf_dw) );
				buf_len_dw = pack_parse(frame_dw, count_dw, &recvHead_dw, buf_dw);
				bzero(frame_dw,sizeof(frame_dw));
	
				printf("\n======== Next Round Evasdroping ========\n");
				if( !isEqual_sockaddr_in(recvHead_dw.dst, target)) { //wrong redirected package, ignore.
					printf("Not package to target server, quit.\n");
					break;
				}
				printf(">> Evasdroping from [%s:%u]\n", inet_ntoa(recvHead_dw.src.sin_addr), ntohs(recvHead_dw.src.sin_port));
				if (isEqual_sockaddr_in(recvHead_dw.src, client_dw)) { // authentication correct
					printf(">> Authentication Client Address Pass.\n");
				}else {
					printf(">> Authentication Client Address Failure.\n");
					return -1;
				}
				printf(">> Size %u, Content: %s\n", buf_len_dw, buf_dw);
				
			//2. send package to target.
				printf("-------- Damage on Upstream--------\n");
				bzero(msg_up, sizeof(msg_up));
				strncpy(msg_up, buf_dw, buf_len_dw);// need protection here >>>
				msg_len_up = buf_len_dw;
				sendHead_up.src = client_up;
				sendHead_up.dst = target;

				//get action from console input
				act_up = 0;//clear
				nitems = 0;
				printf("Enter action(0-NoAction, 1-Drop, 2-Modify):");
				nitems = scanf("%d", &act_up);
				while( (nitems == EOF) | (nitems == 0)) {
					nitems = scanf("%d", &act_up);
				}

				switch(act_up) {
					case NOMODIFY:
						printf("Action-0: forward as is to target.\n");
						
						bzero(frame_up,sizeof(frame_up));
						frame_len_up = pack_build(frame_up, &sendHead_up, msg_up, msg_len_up);

						if (send(sockfd_up, frame_up, frame_len_up, 0) < 0) {
							printf("Proxy send() upstream error.\n");
							break;
						}
						printf("<< Forward to [%s:%u], ",inet_ntoa(sendHead_up.dst.sin_addr), ntohs(sendHead_up.dst.sin_port) );
						printf("Size %u, Content: %s\n", msg_len_up, msg_up);
						//bzero(frame_up,sizeof(frame_up));
						break;

					case IGNORE:
						flag_dw_ign = 1;// used for state jump.
						printf("Action-1: do NOT forward to target.\n");
						//tell client service unavailable.
						bzero(msg_dw, sizeof(msg_dw));
						strncpy(msg_dw, outService, warn_len);// need protection here >>>
						msg_len_dw = warn_len;
						sendHead_dw.src = recvHead_dw.dst;//faking here, cheat client this is target server
						sendHead_dw.dst = client_dw;
						bzero(frame_dw,sizeof(frame_dw));
						frame_len_dw = pack_build(frame_dw, &sendHead_dw, msg_dw, msg_len_dw);

						if (send(newsockfd, frame_dw, frame_len_dw, 0) < 0) {
							printf("Proxy send() downstream error.\n");
							break;
						}
						printf("<< Ack client [%s:%u], %s\n",inet_ntoa(sendHead_dw.dst.sin_addr), ntohs(sendHead_dw.dst.sin_port),msg_dw);
						//bzero(frame_dw,sizeof(frame_dw));
						break;

					case MODIFY:
						printf("Action-2: modify package(by repeating) before send to target.\n");
						strncpy(&msg_up[buf_len_dw], buf_dw, buf_len_dw);// need protection here >>>
						msg_len_up = 2* buf_len_dw;

						bzero(frame_up,sizeof(frame_up));
						frame_len_up = pack_build(frame_up, &sendHead_up, msg_up, msg_len_up);

						if (send(sockfd_up, frame_up, frame_len_up, 0) < 0) {
							printf("Proxy send() upstream error.\n");
							break;
						}
						printf("<< Forward to [%s:%u], ",inet_ntoa(sendHead_up.dst.sin_addr), ntohs(sendHead_up.dst.sin_port) );
						printf("Size %u, Content: %s\n\n", msg_len_up, msg_up);
						//bzero(frame_up,sizeof(frame_up));
						break;

					default:
						printf("Unknown action: will drop package by default.\n");
						flag_dw_ign = 1;// used for state jump.
						//do nothing
						break;

				}
			//3. receive from target.
				if (flag_dw_ign) continue;//skip communication with server.

				bzero(frame_up,sizeof(frame_up));
				if( (count_up = recv(sockfd_up, frame_up, sizeof(frame_up) , 0)) < 0){
					printf("Proxy recv() from target failed");
					break;
				}
				// now client will parse the package, and has no ware what has happened.
				bzero(buf_up,sizeof(buf_up));
				buf_len_up = pack_parse(frame_up, count_up, &recvHead_up, buf_up);
				printf(">> Reply from [%s:%u]\n", inet_ntoa(recvHead_up.src.sin_addr), ntohs(recvHead_up.src.sin_port));
				if (isEqual_sockaddr_in(recvHead_up.src, target)) { // authentication correct
					printf(">> Authentication Target Server Address Pass.\n");
				}else {
					printf(">> Authentication Target Server Address Failure.\n");
					return -1;
				}
				printf(">> Size: %u, Content: %s\n\n",buf_len_up, buf_up);
				

			//4. send back to client after malicious action.
				printf("-------- Damage on Downstream --------\n");
				bzero(msg_dw, sizeof(msg_dw));
				strncpy(msg_dw, buf_up, buf_len_up);// need protection here >>>
				msg_len_dw = buf_len_up;
				sendHead_dw.src = recvHead_dw.dst;//faking here, cheat client this is target server
				sendHead_dw.dst = client_dw;

				act_dw = 0;
				nitems = 0;
				//get action from console input
				printf("Enter action(0-NoAction, 1-Drop, 2-Modify):");
				nitems = scanf("%d", &act_dw);
				while( (nitems == EOF) | (nitems == 0)) {
					nitems = scanf("%d", &act_dw);
				}

				switch(act_dw) {
					case NOMODIFY:
						printf("Action-0: Forward as is to client.\n");
						bzero(frame_dw,sizeof(frame_dw));
						frame_len_dw = pack_build(frame_dw, &sendHead_dw, msg_dw, msg_len_dw);

						if (send(newsockfd, frame_dw, frame_len_dw, 0) < 0) {
							printf("Proxy send() downstream error.\n");
							break;
						}
						printf("<< Forward to [%s:%u], ",inet_ntoa(sendHead_dw.dst.sin_addr), ntohs(sendHead_dw.dst.sin_port) );
						printf("Size %u, Content: %s\n\n", msg_len_dw, msg_dw);
						//bzero(frame_dw,sizeof(frame_dw));
						break;

					case IGNORE:
						printf("Action-1: do NOT forward to client.\n");
						//tell client service unavailable.
						bzero(msg_dw, sizeof(msg_dw));
						strncpy(msg_dw, outService, warn_len);// need protection here >>>
						msg_len_dw = warn_len;
						bzero(frame_dw,sizeof(frame_dw));
						frame_len_dw = pack_build(frame_dw, &sendHead_dw, msg_dw, msg_len_dw);

						if (send(newsockfd, frame_dw, frame_len_dw, 0) < 0) {
							printf("Proxy send() downstream error.\n");
							break;
						}
						printf("<< Ack client [%s:%u], %s\n",inet_ntoa(sendHead_dw.dst.sin_addr), ntohs(sendHead_dw.dst.sin_port),msg_dw);
						//bzero(frame_dw,sizeof(frame_dw));
						break;

					case MODIFY:
						printf("Action-2: modify package(by changing pilot) before send to client.\n");
						strncpy(msg_dw, proxyMark, mark_len);//replace target pilot 

						bzero(frame_dw,sizeof(frame_dw));
						frame_len_dw = pack_build(frame_dw, &sendHead_dw, msg_dw, msg_len_dw);

						if (send(newsockfd, frame_dw, frame_len_dw, 0) < 0) {
							printf("Proxy send() downstream error.\n");
							break;
						}
						printf("<< Forward to [%s:%u], ",inet_ntoa(sendHead_dw.dst.sin_addr), ntohs(sendHead_dw.dst.sin_port) );
						printf("Size %u, Content: %s\n\n", msg_len_dw, msg_dw);
						//bzero(frame_dw,sizeof(frame_dw));
						break;

					default:
						printf("Action-1: drop package at downstream by default.\n");
						//do nothing
						break;
				}
			}

			if (count_dw <  0) {
				printf("End of Reading Stream Message.\n");
				exit(0);
			}
		}
		wait(NULL);
		printf("Child process closed, will close connection with client.\n");
		close(newsockfd);
	}
	return 0;
}
