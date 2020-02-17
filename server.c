#include "sockcom.h"

int main(int argc, const char * argv[]) {
	int sockfd, newsockfd;
	struct sockaddr_in server;
	struct sockaddr_in src;
	socklen_t len_src = sizeof(src);
	struct MyHeader sendHead;
	struct MyHeader recvHead;

	uint32_t frame_len, buf_len, msg_len, count;
	char frame[MAXBUF];
	char buf[MAXBUF-HEADLEN-1];
	char msg[MAXBUF-HEADLEN-1];
	char *servMark = "$$$";
	int mark_len = strlen(servMark);

	if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ){
		printf("Server socket() error.\n");
		return -1;
	}
	
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	// server.sin_port = htons(TARGET_PORT); 
	server.sin_port = htons(atoi(argv[1]));
	
	if(bind(sockfd, (struct sockaddr *) &server, sizeof(server)) < 0){
		printf("Server bind() error. \n");
		return -1;
	}
	
	if( listen(sockfd, 5) == 0)
		printf("Server Listening at port: %d ... \n",ntohs(server.sin_port));
	else{
		printf("Server Error listening...\n");
		return -1;
	}
	
	for (;;) {
		newsockfd = accept(sockfd, (struct sockaddr*) 0, (socklen_t*) 0);
		
		if(!fork()){//in the child process, server do nothing but echo what it received.
			
			//first get client socket addr:
			if( getpeername(newsockfd, (struct sockaddr*) &src, &len_src) == -1) {
	       			printf("Server getpeername() error.\n");
	       			printf("error num:%d\n",errno);
	       			return -1;
			}
			
			close(sockfd);// parent is listening, thus closed here.
			bzero(frame,sizeof(frame));
			while( (count = recv(newsockfd, frame, sizeof(frame), 0)) >= 0){
				bzero(buf, sizeof(buf) );
				buf_len = pack_parse(frame, count, &recvHead, buf);
				bzero(frame,sizeof(frame));
				printf(">> Request from [%s:%u]\n", inet_ntoa(recvHead.src.sin_addr), ntohs(recvHead.src.sin_port));
				
				if (isEqual_sockaddr_in(recvHead.src, src)) { // authentication correct
					printf(">> Authentication Client Address Pass.\n");
				}else {
					printf(">> Authentication Client Address Failure.\n");
					return -1;
				}
				
				printf(">> Size %u, Content: %s\n", buf_len, buf);
				
				// add your server function below===>
				bzero(msg, sizeof(msg));
				strncpy(msg, servMark, mark_len);// add mark to demo its server echo.
				strncpy(msg+mark_len, buf, buf_len);// need protection here >>>
				msg_len = mark_len + buf_len;

				/******Construct Package to Respond to Client*****/
				sendHead.src = recvHead.dst;
				sendHead.dst = recvHead.src;
				frame_len = pack_build(frame, &sendHead, msg, msg_len);

				if (send(newsockfd, frame, frame_len, 0) < 0) {
					printf("Server send() error.\n");
					break;
				}
				printf("<< Reply to [%s:%u], ",inet_ntoa(sendHead.dst.sin_addr), ntohs(sendHead.dst.sin_port) );
				printf("Size %u, Content: %s\n\n", msg_len, msg);
				bzero(frame,sizeof(frame));
							
			}
			if (count <  0) {
				printf("Client Stops Input.\n");
				exit(0);
			}
		}
		wait(NULL);
		printf("Child process closed, will close connection with client.\n");
		close(newsockfd);
	}
	return 0;
}
