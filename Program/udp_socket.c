#include "udp_socket.h"
#include "library.h"

#ifndef VERSION
#error "VERSION must be defined as build time variable"
#endif

int sockfd;
fd_set select_fds,select_dns_fds[RAIDA_SERVER_MAX],select_legacy_fds[RAIDA_SERVER_MAX];                
struct timeval timeout,dns_timeout[RAIDA_SERVER_MAX],legacy_timeout[RAIDA_SERVER_MAX];
struct dns_config dns_config_obj[RAIDA_SERVER_MAX];     
union coversion snObj;
struct coin_id coin_id_obj[255];
unsigned char response_flg;
struct sockaddr_in servaddr, cliaddr;
long time_stamp_before,time_stamp_after;
struct key_table key_table_obj[ENCRY2_KEYS_MAX]={0};
unsigned char udp_buffer[UDP_BUFF_SIZE],response[RESPONSE_HEADER_MAX],coin_table_id[5],EN_CODES[EN_CODES_MAX]={0};
unsigned char free_thread_running_flg;
pthread_t free_id_ptid;
struct fix_validate {
	uint32_t ticketNo;
	uint32_t srNo[COINS_MAX];
	uint32_t srNoCnt;
}fix_validate_obj[RAIDA_SERVER_MAX];

struct thread_args {
    unsigned char dest_raida;
    uint32_t ticket_no;
    unsigned int coin_id;
}thread_args_obj[RAIDA_SERVER_MAX];
 uint8_t nounce[NOUNCE_BYTES_CNT];
uint8_t encrypt_key[AN_BYTES_CNT];
unsigned char encrytion_type;

//-----------------------------------------------------------
//Set time out for UDP frames
//-----------------------------------------------------------
void set_time_out(unsigned char secs){     
	FD_ZERO(&select_fds);            
	FD_SET(sockfd, &select_fds);           	                                  
	timeout.tv_sec = secs; 
	timeout.tv_usec = 0;
}
//-----------------------------------------------------------
//Set time out for UDP frames
//-----------------------------------------------------------
void set_dns_time_out(int index,int secs){     
	FD_ZERO(&select_dns_fds[index]);            
	FD_SET(dns_config_obj[index].sockfd, &select_dns_fds[index]);           	                                  
	dns_timeout[index].tv_sec = secs; 
	dns_timeout[index].tv_usec = 0;
}
//-----------------------------------------------------------
//Initialize UDP Socket and bind to the port
//-----------------------------------------------------------
int init_udp_socket() {

  setvbuf(stdout, NULL, _IONBF, 0);

	// Creating socket file descriptor
	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}
	memset(&servaddr, 0, sizeof(servaddr));
	memset(&cliaddr, 0, sizeof(cliaddr));
	// Filling server information
	servaddr.sin_family = AF_INET; // IPv4
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = htons(server_config_obj.port_number);
	// Bind the socket with the server address
	if ( bind(sockfd, (const struct sockaddr *)&servaddr,sizeof(servaddr)) < 0 ){
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
}
//-----------------------------------------------------------
//Initialize UDP socket for routing
//-----------------------------------------------------------
int init_dns_socket(unsigned int index,unsigned int port, char ip_addr[64]) {
	if ((dns_config_obj[index].sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}	
	memset(&dns_config_obj[index].servaddr, 0, sizeof(dns_config_obj[index].servaddr));
	dns_config_obj[index].servaddr.sin_family = AF_INET;
	dns_config_obj[index].servaddr.sin_port = htons(port);
	dns_config_obj[index].servaddr.sin_addr.s_addr = inet_addr(ip_addr);
}	
//-----------------------------------------------------------
// listen to dns socket
//-----------------------------------------------------------
int listen_dns_socket(unsigned int index){
	int i=0,n=0;
	socklen_t len=sizeof(struct sockaddr_in);
	set_dns_time_out(index,2);
	if (select(32, &select_dns_fds[index], NULL, NULL, &dns_timeout[index]) == 0 ){
		printf("\nTime out error \n");
		return 0;
	}else{
		n = recvfrom(dns_config_obj[index].sockfd,(unsigned char *)dns_config_obj[index].buffer,
					1024,MSG_WAITALL, ( struct sockaddr *) &dns_config_obj[index].cliaddr,&len);
		printf("\n DNS RECVD %d\n",n);
		for(i=0;i<n;i++){
			printf("%d,",dns_config_obj[index].buffer[i]);
		}
	}
	return n;
}
//-----------------------------------------------------------
//Send to dns socket
//-----------------------------------------------------------
void send_to_dns(unsigned int index,unsigned int len){
 	sendto(dns_config_obj[index].sockfd, (unsigned char *)dns_config_obj[index].buffer, len,
	        MSG_CONFIRM, (struct sockaddr *)&dns_config_obj[index].servaddr, 
	            sizeof(dns_config_obj[index].servaddr));
}

//-----------------------------------------------------------
// receives the UDP packet from the client
//-----------------------------------------------------------
int listen_request(){
	unsigned char *buffer,state=STATE_WAIT_START,status_code,buffer_tmp[1024],err_flg=0;
	uint16_t frames_expected=0,curr_frame_no=0,n=0,i=0,index=0,j=0,coin_id;
	uint32_t client_s_addr=0;
  uint16_t csum = 0;
  unsigned int crc;
  uint8_t c0, c1;
		 	
	socklen_t len=sizeof(struct sockaddr_in);
	buffer = (unsigned char *) malloc(server_config_obj.bytes_per_frame);
	response_flg = UDP_RESPONSE;
	while(1){
		switch(state){
			case STATE_WAIT_START:
				printf("---------------------WAITING FOR REQ HEADER ----------------------\n");
				index=0;
				curr_frame_no=0;
				client_s_addr = 0;	
				memset(buffer,0,server_config_obj.bytes_per_frame);
				n = recvfrom(sockfd, (unsigned char *)buffer, server_config_obj.bytes_per_frame,MSG_WAITALL, ( struct sockaddr *) &cliaddr,&len);
				curr_frame_no=1;
				encrytion_type =0;
				index=0;
				err_flg=0;
				printf("--------RECVD  FRAME NO ------ %d\n", curr_frame_no);
				state = STATE_START_RECVD;
	
			break;		
			case STATE_START_RECVD:
				printf("---------------------REQ HEADER RECEIVED ----------------------------\n");
				 status_code=validate_request_header(buffer,n);
				if(status_code!=NO_ERR_CODE){
					send_err_resp_header(status_code);			
					state = STATE_WAIT_START;
					err_flg = 1;
				}else{
					frames_expected = buffer[REQ_FC+1];
					frames_expected|=(((uint16_t)buffer[REQ_FC])<<8);
					memset(nounce,0,NOUNCE_BYTES_CNT);
					//We take nouce 5 bytes
					for(i=0;i<5;i++){
						nounce[i] = buffer[REQ_NO_1+i];
					}
					j=0;
					//We take nouce 3 bytes 
					for(i=5;i<8;i++,j++){
						nounce[i] = buffer[REQ_NO_6+j];

					}
					//check the type of encryption 
					if(buffer[REQ_EN] == ENCRYP_128_AES_CTR_SN){
						encrytion_type = ENCRYP_128_AES_CTR_SN;
						printf("\n ENCRYPTION TYPE := %d \n", encrytion_type);
						memcpy(buffer_tmp,&buffer[REQ_HEAD_MIN_LEN],n-REQ_HEAD_MIN_LEN);
						memset(snObj.data,0,4);
						for(j=0;j<SN_BYTES_CNT;j++){
							snObj.data[j]=buffer[REQ_SR_N0+(SN_BYTES_CNT-1-j)];
						}
						coin_id = buffer[REQ_COIN_ID+1];
						coin_id |= (((uint16_t)buffer[REQ_COIN_ID])<<8);
						printf("%d\n",coin_id);
						printf("%d\n",snObj.val32);
						if(snObj.val32 >=coin_id_obj[coin_id].AN_CNT){
							send_err_resp_header(COIN_NO_NOT_FOUND);
							state = STATE_WAIT_START;
							err_flg = 1;
						} else{
							memcpy(encrypt_key,coin_id_obj[coin_id].AN[snObj.val32],AN_BYTES_CNT);
							crypt_ctr(encrypt_key,buffer_tmp,n-REQ_HEAD_MIN_LEN,nounce);
							memcpy(&buffer[REQ_HEAD_MIN_LEN],buffer_tmp,n-REQ_HEAD_MIN_LEN);
							printf(" encry key\n");
							for(i=0;i<16;i++){
								printf("%d,",encrypt_key[i]);
							}
							printf("\n nounce \n");
							for(i=0;i<16;i++){
								printf("%d,",nounce[i]);
							}
	
							printf("\n After decry");
							for(i=0;i<n;i++){
								printf("%d,",buffer[i]);
							}
							printf("\n");
						}
					}else if (buffer[REQ_EN] == ENCRYP_128_AES_CTR_KEY_TABLE){
						int key_index;
						memset(snObj.data,0,4);
						for(j=0;j<SN_BYTES_CNT;j++){
							snObj.data[j]=buffer[REQ_SR_N0+(SN_BYTES_CNT-1-j)];
						}
						key_index = find_encry2_key(snObj.val32);
						printf("%d",	snObj.val32);
						if(key_index == -1){
							send_err_resp_header(KEY_NOT_CONFIG_ENCRY_2);
							state = STATE_WAIT_START;
							err_flg = 1;
						} else {
							memcpy(buffer_tmp,&buffer[REQ_HEAD_MIN_LEN],n-REQ_HEAD_MIN_LEN);
							memcpy(encrypt_key,key_table_obj[key_index].key,KEY_BYTES_CNT);
							crypt_ctr(encrypt_key,buffer_tmp,n-REQ_HEAD_MIN_LEN,nounce);
							memcpy(&buffer[REQ_HEAD_MIN_LEN],buffer_tmp,n-REQ_HEAD_MIN_LEN);
							for(i=0;i<16;i++){
								printf("%d,",encrypt_key[i]);
							}
							printf("\n nounce \n");
							for(i=0;i<16;i++){
								printf("%d,",nounce[i]);
							}
	
							printf("\n After decry");
							for(i=0;i<n;i++){
								printf("%d,",buffer[i]);
							}
							printf("\n");
						}
					}
					if(err_flg == 0){
						memcpy(udp_buffer,buffer,n);
						index = n;
						client_s_addr = cliaddr.sin_addr.s_addr;
						if(frames_expected == 1){
							state = STATE_END_RECVD;
						}else{
							state = STATE_WAIT_END;
						}
					}
				}
			break;
			case STATE_WAIT_END:
				set_time_out(FRAME_TIME_OUT_SECS);
				if (select(32, &select_fds, NULL, NULL, &timeout) == 0 ){
					//send_err_resp_header(FRAME_TIME_OUT);
					send_err_resp_header(PACKET_ORDER_LOSS);
					state = STATE_WAIT_START;
					printf("Time out error \n");
				}else{
					n = recvfrom(sockfd, (unsigned char *)buffer, server_config_obj.bytes_per_frame,MSG_WAITALL, ( struct sockaddr *) &cliaddr,&len);
					if(client_s_addr==cliaddr.sin_addr.s_addr){
						if(encrytion_type == ENCRYP_128_AES_CTR_SN){
							crypt_ctr(encrypt_key,buffer,n,nounce);
						}else if (buffer[REQ_EN] == ENCRYP_128_AES_CTR_KEY_TABLE){
							crypt_ctr(encrypt_key,buffer,n,nounce);
						}
						memcpy(&udp_buffer[index],buffer,n);
						index+=n;
						curr_frame_no++;
						printf("--------RECVD  FRAME NO ------ %d \n", curr_frame_no);
						if(curr_frame_no==frames_expected){
							state = STATE_END_RECVD;
						}
					}						
				}	
			break;			
			case STATE_END_RECVD:

					if(udp_buffer[index-1]!=REQ_END|| udp_buffer[index-2]!=REQ_END){
						send_err_resp_header(INVALID_END_OF_REQ);
						printf("Invalid end of packet  \n");
					}else{
            csum = (udp_buffer[6] << 8) | udp_buffer[14];
            if (frames_expected > 1) {
              crc = crc32b(&udp_buffer[REQ_HEAD_MIN_LEN], index - REQ_HEAD_MIN_LEN);
              //c0 = (crc >> 8) & 0xff;
              c1 = crc & 0xff;
              printf("csum %x %x, crc=%x (%x %x)\n",udp_buffer[6], udp_buffer[14],  crc, c0, c1);


              //if (c0 != udp_buffer[6] && c1 != udp_buffer[14]) {
              if (c1 != udp_buffer[6]) {
      					send_err_resp_header(PACKET_ORDER_LOSS);
			      		state = STATE_WAIT_START;
      					printf("Invalid checksum \n");
                break;
              }

            }

						printf("---------------------END RECVD----------------------------------------------fr=%d b6=%x b14=%x csum=%x\n", frames_expected, udp_buffer[6], udp_buffer[14], csum);
						printf("---------------------PROCESSING REQUEST-----------------------------\n");
						process_request(index);
					}
					state = STATE_WAIT_START;
			break;
		}
	}
}
//-----------------------------------------------------------
// Processes the UDP packet 
//-----------------------------------------------------------
void process_request(unsigned int packet_len){
	uint16_t cmd_no=0, coin_id,i=0,j=0;
	time_stamp_before = get_time_cs();
	memset(response,0,RESPONSE_HEADER_MAX-1);
	cmd_no = udp_buffer[REQ_CM+1];
	cmd_no |= (((uint16_t)udp_buffer[REQ_CM])<<8);
	coin_id = udp_buffer[REQ_CI+1];
	coin_id |= (((uint16_t)udp_buffer[REQ_CI])<<8);
	
	switch(cmd_no){
		case CMD_POWN : 					execute_pown(packet_len,coin_id);break;
		case CMD_DETECT : 					execute_detect(packet_len,coin_id);break;
		case CMD_FIND : 					execute_find(packet_len,coin_id);break;
		
		//case CMD_FIX :						execute_fix(packet_len,coin_id,FIX_VERSION_1);break;
		
		case CMD_FIX_V2 :					execute_fix(packet_len,coin_id,FIX_VERSION_2);break;
		case CMD_ECHO:						execute_echo(packet_len);break;
		case CMD_VALIDATE : 					execute_validate(packet_len,coin_id);break;
		
		//case CMD_PUT_KEY:					execute_put_key(packet_len);break;
		//case CMD_GET_KEY:					execute_get_key(packet_len);break;
		//case CMD_IDENTIFY : 					execute_identify(packet_len,coin_id);break;
		
		case CMD_GET_TICKET :				execute_get_ticket(packet_len,coin_id);break;
		
		//case CMD_VERSION:					execute_version();break;	
		case CMD_NEWS:						execute_news();break;	
		case CMD_LOGS:						execute_logs(packet_len);break;
		case CMD_PANG : 					execute_pang(packet_len,coin_id);break;
		case CMD_BECOME_PRIMARY:				execute_become_primary(packet_len);break;
		case CMD_BECOME_MIRROR:				execute_become_mirror(packet_len);break;
		case CMD_CHECK_UPDATES:				execute_check_updates(packet_len);break;
		case CMD_FREE_ID_VER_0:				execute_free_id(packet_len,FREE_ID_VER_0);break;
		case CMD_FREE_ID_VER_1:				execute_free_id(packet_len,FREE_ID_VER_1);break;
		case CMD_DEPOSITE:					execute_deposite(packet_len);break;
		case CMD_DEPOSITE_PANG:				execute_deposite_pang(packet_len);break;
		case CMD_WITHDRAW:					execute_withdraw(packet_len);break;
		case CMD_TRANSFER:					execute_transfer(packet_len);break;
		case CMD_SHOW_BALANCE:				execute_show_balance(packet_len);break;
		case CMD_SHOW_REGISTRY: 				execute_show_registry(packet_len);break;
		case CMD_SHOW_COINS_DENOM:			execute_show_by_denom(packet_len);break;
		case CMD_SHOW_COINS_BY_TYPE:			execute_show_by_coin_type(packet_len);break;
		case CMD_SHOW_CHANGE:				execute_show_change(packet_len);break;
		case CMD_JOIN	:					execute_join(packet_len);break;
		case CMD_JOIN_IN_BANK:				execute_join_in_bank(packet_len);break;
		case CMD_BREAK:					execute_break(packet_len);break;
		case CMD_BREAK_IN_BANK:				execute_break_in_bank(packet_len);break;
		case CMD_SHOW_STATEMENT:			execute_show_statement(packet_len);break;
		case CMD_DEL_ALL_STATEMENT:			execute_del_statements(packet_len);break;
		case CMD_SHOW_PAYMENT:				execute_show_payment(packet_len);break;
		case CMD_CHANGE_COIN_TYPE:			execute_change_coin_type(packet_len);break;
		case CMD_SYNC_TRANS_ADD :   			execute_sync_trans_add(packet_len);break;
		case CMD_SYNC_TRANS_DEL :   			execute_sync_trans_del(packet_len);break;
		case CMD_SYNC_TRANS_RESP :   			execute_sync_trans_resp(packet_len);break;
		case CMD_UPGRADE_COIN	:			execute_upgrade_coin(packet_len);break;	
		case CMD_COIN_CLAIM 	:			execute_coin_claim(packet_len);break;	
		default:							send_err_resp_header(INVALID_CMD);	
	}
	
}
//-----------------------------------------------------------
// Prepare error response and send it.
//-----------------------------------------------------------
void send_err_resp_header(int status_code){
	int len,size=12;
	unsigned char ex_time;
	time_stamp_after = get_time_cs();
	if((time_stamp_after-time_stamp_before) > 255){
		ex_time = 255;
	}else{
		ex_time= time_stamp_after-time_stamp_before;
	}
	printf("Error Status code %d  \n",status_code);
	response[RES_RI] = server_config_obj.raida_id;
	response[RES_SH] = 0;
	response[RES_SS] = status_code;
	response[RES_EX] = 0;
	response[RES_RE] = 0;
	response[RES_RE+1] = 0;
	response[RES_EC] = udp_buffer[REQ_EC];
	response[RES_EC+1] = udp_buffer[REQ_EC+1];
	response[RES_HS] = 0;
	response[RES_HS+1] = 0;
	response[RES_HS+2] = 0;
	response[RES_HS+3] = 0;
	len=sizeof(cliaddr);
	sendto(sockfd, (const char *)response, size,
		MSG_CONFIRM, (const struct sockaddr *) &cliaddr,
		len);
}
//-----------------------------------------------------------
// Prepare response and send it.
//-----------------------------------------------------------
void prepare_resp_header(unsigned char status_code){
	unsigned char ex_time;
	time_stamp_after = get_time_cs();
	if((time_stamp_after-time_stamp_before) > 255){
		ex_time = 255;
	}else{
		ex_time= time_stamp_after-time_stamp_before;
	}
	response[RES_RI] = server_config_obj.raida_id;
	response[RES_SH] = 0;
	response[RES_SS] = status_code;
	response[RES_EX] = ex_time;
	response[RES_RE] = 0;
	response[RES_RE+1] = 0;
	response[RES_EC] = udp_buffer[REQ_EC];
	response[RES_EC+1] = udp_buffer[REQ_EC+1];
	response[RES_HS] = 0;
	response[RES_HS+1] = 0;
	response[RES_HS+2] = 0;
	response[RES_HS+3] = 0;
}
//-----------------------------------------------------------
//  Validate request header
//-----------------------------------------------------------
unsigned char validate_request_header(unsigned char * buff,int packet_size){
	uint16_t frames_expected,i=0,request_header_exp_len= REQ_HEAD_MIN_LEN, coin_id=0;
	/*if(buff[REQ_EN]!=0){
		for(i=1;i<EN_CODES_MAX+1;i++){
			if(EN_CODES[buff[REQ_EN]]>0)
				break;
		}
	}*/
	if(buff[REQ_EN] ==0 || buff[REQ_EN] ==1 || buff[REQ_EN] ==2){
		
	}else{
		return INVALID_EN_CODE;
	}
	request_header_exp_len = REQ_HEAD_MIN_LEN;
	if(packet_size< request_header_exp_len){
		printf("Invalid request header  \n");
		return INVALID_PACKET_LEN;
	}
	frames_expected = buff[REQ_FC+1];
	frames_expected|=(((uint16_t)buff[REQ_FC])<<8);
	printf("No of frames expected :- %d\n", frames_expected);
	if(frames_expected <=0  || frames_expected > FRAMES_MAX){
		printf("Invalid frame count  \n");
		return INVALID_FRAME_CNT;
	}	
	if(buff[REQ_CL]!=0){
		printf("Invalid cloud id \n");
		return INVALID_CLOUD_ID;
	}
	if(buff[REQ_SP]!=0){
		printf("Invalid split id \n");
		return INVALID_SPLIT_ID;
	}
	if(buff[REQ_RI]!=server_config_obj.raida_id){
		printf("Invalid Raida id \n");
		return WRONG_RAIDA;
	}
	if(shards_config[buff[REQ_SH]] ==SHARD_NOT_AVAILABLE_VALUE){
		printf("Shard %d not available \n", buff[REQ_SH] );
		return SHARD_NOT_AVAILABLE;
	}
	coin_id = buff[REQ_CI+1];
	coin_id |= (((uint16_t)buff[REQ_CI])<<8);
	for(i=0;i<coin_id_cnt;i++){
		if(coin_config_obj[i].coin_id==coin_id){
			break;
		}
	}
	if(i>=coin_id_cnt){
		return COIN_ID_NOT_FOUND; 
	}
	return NO_ERR_CODE;
}
//------------------------------------------------------------------------------------------
//  Validate coins and request body and return number of coins 
//-----------------------------------------------------------------------------------------
unsigned char validate_request_body(unsigned int packet_len,unsigned char bytes_per_coin,unsigned int req_body_without_coins,int *req_header_min){
	unsigned int no_of_coins=0;
	*req_header_min = REQ_HEAD_MIN_LEN;// + EN_CODES[udp_buffer[REQ_EN]];	
	no_of_coins = (packet_len-(*req_header_min+req_body_without_coins))/bytes_per_coin;
	if((packet_len-(*req_header_min+req_body_without_coins))%bytes_per_coin!=0){
		send_err_resp_header(LEN_OF_BODY_CANT_DIV_IN_COINS);
		return 0;
	}
	if(no_of_coins==0){
		send_err_resp_header(LEN_OF_BODY_CANT_DIV_IN_COINS);
		return 0;
	}
	if(no_of_coins>COINS_MAX){
		send_err_resp_header(COIN_LIMIT_EXCEED);
		return 0;
	}
	printf("Number of coins = :  %d \n", no_of_coins);	
	return no_of_coins;
}
//------------------------------------------------------------------------------------------
//  Validate coins and request body and return number of coins 
//-----------------------------------------------------------------------------------------
unsigned char validate_request_body_general(unsigned int packet_len,unsigned int req_body,int *req_header_min){
	*req_header_min = REQ_HEAD_MIN_LEN;// + EN_CODES[udp_buffer[REQ_EN]];
	if(packet_len != (*req_header_min) + req_body){
		send_err_resp_header(INVALID_PACKET_LEN);
		return 0;
	}
	return 1;
}
//---------------------------------------------------------------
//	SEND RESPONSE
//---------------------------------------------------------------
void send_response(unsigned char status_code,unsigned int size){
	int len=sizeof(cliaddr);
	unsigned char buffer_tmp[1024];
	prepare_resp_header(status_code);
	if(encrytion_type == ENCRYP_128_AES_CTR_SN){
  printf("\n1111enc stuff\n");
  for (int i = 0; i < AN_BYTES_CNT; i++) {
    printf("%d ", encrypt_key[i]);
  }
  printf("\n1111nnnonc\n");
  for (int i = 0; i < 16; i++) {
    printf("%d ", nounce[i]);
  }
  printf("\n");
  printf("1111 sending this before enc\n");
  for (int i = 0; i < size; i++) {
    printf("%d ", response[i]);
  }
  printf("\n");


		memcpy(buffer_tmp,&response[RES_HEAD_MIN_LEN],size-RES_HEAD_MIN_LEN);
		crypt_ctr(encrypt_key,buffer_tmp,size-RES_HEAD_MIN_LEN,nounce);
		memcpy(&response[RES_HEAD_MIN_LEN],buffer_tmp,size-RES_HEAD_MIN_LEN);
  printf("\n1111 sending this after enc\n");
  for (int i = 0; i < size; i++) {
    printf("%d ", response[i]);
  }
  printf("\n");
	}else if (encrytion_type == ENCRYP_128_AES_CTR_KEY_TABLE){
		memcpy(buffer_tmp,&response[RES_HEAD_MIN_LEN],size-RES_HEAD_MIN_LEN);
		crypt_ctr(encrypt_key,buffer_tmp,size-RES_HEAD_MIN_LEN,nounce);
		memcpy(&response[RES_HEAD_MIN_LEN],buffer_tmp,size-RES_HEAD_MIN_LEN);
	}
	sendto(sockfd, (const char *)response, size,
		MSG_CONFIRM, (const struct sockaddr *) &cliaddr,
		len);
}
//---------------------------------------------------------------
// POWN COMMAND 0
//---------------------------------------------------------------
void execute_pown(unsigned int packet_len,unsigned int coin_id){
	int req_body_without_coins = CH_BYTES_CNT + CMD_END_BYTES_CNT,bytes_per_coin = SN_BYTES_CNT+AN_BYTES_CNT+PAN_BYTES_CNT;
	int req_header_min, no_of_coins,ticket_no=0;
	unsigned int i=0,index=0,j=0,pass_cnt=0,fail_cnt=0,size=0;
	unsigned char status_code,pass_fail[COINS_MAX]={0};
	time_t now = time(0);
	struct tm *t = gmtime(&now);
	printf("POWN Command \n");
	no_of_coins = validate_request_body(packet_len,bytes_per_coin,req_body_without_coins,&req_header_min);
	if(no_of_coins <=0){
		return;
	}
	ticket_no=rand();
	for(i=0;i<no_of_coins;i++) {
		index = (i * bytes_per_coin) + req_header_min+CH_BYTES_CNT;
		memset(snObj.data,0,4);
		for(j=0;j<SN_BYTES_CNT;j++)
			snObj.data[j]=udp_buffer[index+(SN_BYTES_CNT-1-j)];

		if(snObj.val32 >=coin_id_obj[coin_id].AN_CNT){
			send_err_resp_header(COIN_NO_NOT_FOUND);
			return;
		}
		index +=SN_BYTES_CNT;
		pass_fail[i]=1;
		if(memcmp(&udp_buffer[index],&udp_buffer[index+PAN_BYTES_CNT],PAN_BYTES_CNT)==0){
			pass_fail[i]=0;
			fail_cnt++;
			//send_err_resp_header(POWN_AN_PAN_SAME);
			//return;
		}
		if(memcmp(coin_id_obj[coin_id].AN[snObj.val32],&udp_buffer[index],AN_BYTES_CNT)!=0){
			pass_fail[i]=0;
			fail_cnt++;
		}		
		if(pass_fail[i]==1){
			add_ticket(coin_id,ticket_no,snObj.val32);	
			index +=AN_BYTES_CNT;
			memcpy(coin_id_obj[coin_id].AN[snObj.val32],&udp_buffer[index],PAN_BYTES_CNT);
			coin_id_obj[coin_id].MFS[snObj.val32]=t->tm_mon+1;
			coin_config_obj[coin_id].pages_changed[((snObj.val32)/coin_config_obj[coin_id].page_size)]=1;
			pass_cnt++;	
		}			
	}
	status_code = MIX;
	if (fail_cnt == no_of_coins){
		status_code = ALL_FAIL;
	}else if(pass_cnt == no_of_coins){
		status_code = ALL_PASS;
	}
	index = RES_HS+HS_BYTES_CNT;
	size    =  RES_HS+HS_BYTES_CNT;
	if(status_code == ALL_PASS || status_code == MIX){
		snObj.val32 = ticket_no;
		for(j=0;j<MS_BYTES_CNT;j++)
			response[index+j]=snObj.data[MS_BYTES_CNT-1-j];
		index+=MS_BYTES_CNT;
		size+=MS_BYTES_CNT;
	}
	if(status_code == MIX){
		for(i=0;i<no_of_coins;i++){
			if( pass_fail[i]==1)
				response[index + (i/8)] |= 1<<(i%8);
		}
		size +=no_of_coins/8;
		if((no_of_coins % 8)!=0)
			size ++;		
	}
	send_response(status_code,size);
}

// PANG
void execute_pang(unsigned int packet_len,unsigned int coin_id){
	int req_body_without_coins = CH_BYTES_CNT + CMD_END_BYTES_CNT + PG_BYTES_CNT,bytes_per_coin = SN_BYTES_CNT+AN_BYTES_CNT;
	int req_header_min, no_of_coins,ticket_no=0;
	unsigned int i=0,index=0,j=0,pass_cnt=0,fail_cnt=0,size=0;
	unsigned char status_code,pass_fail[COINS_MAX]={0};
  unsigned char pg[PG_BYTES_CNT], md_output[64], tmp[64], md_input[64];
  int offset;
	time_t now = time(0);
	struct tm *t = gmtime(&now);
	printf("PANG Command \n");
	no_of_coins = validate_request_body(packet_len,bytes_per_coin,req_body_without_coins,&req_header_min);
	if(no_of_coins <=0){
    printf("INVALID NUMBDER OF COINS\n");
		return;
	}

  offset = req_header_min + CH_BYTES_CNT;
  memcpy(pg, &udp_buffer[offset], PG_BYTES_CNT);
	offset += PG_BYTES_CNT;

  printf("pg \n");
  for (i = 0; i < PG_BYTES_CNT; i++) {
    printf("%d ", pg[i]);
  }
  printf("\n");

	ticket_no=rand();
	for(i=0;i<no_of_coins;i++) {
		index = (i * bytes_per_coin) + offset;
		memset(snObj.data,0,4);
		for(j=0;j<SN_BYTES_CNT;j++)
			snObj.data[j]=udp_buffer[index+(SN_BYTES_CNT-1-j)];

    printf("gotsn %d\n", snObj.val32);
		if(snObj.val32 >=coin_id_obj[coin_id].AN_CNT){
      printf("gotsn %d not found\n", snObj.val32);
			send_err_resp_header(COIN_NO_NOT_FOUND);
			return;
		}
		index +=SN_BYTES_CNT;
		pass_fail[i]=1;

    printf("sn %d an\n", snObj.val32);
  for (j = 0; j < 16; j++) {
    printf("%d ", udp_buffer[index + i]);
  }
  printf("\n");


		if(memcmp(coin_id_obj[coin_id].AN[snObj.val32],&udp_buffer[index],AN_BYTES_CNT)!=0){
      printf("failed\n");

			pass_fail[i]=0;
			fail_cnt++;
		}		
      printf("continue\n");


		if(pass_fail[i]==1){
			add_ticket(coin_id,ticket_no,snObj.val32);	
			index +=AN_BYTES_CNT;

				sprintf(md_input, "%d", udp_buffer[REQ_RI]);
				sprintf(tmp, "%d", snObj.val32);
				strcat(md_input,tmp);
				printf("mdinput now=%s\n",md_input);
				for(j=0;j<PG_BYTES_CNT;j++){		
          printf("j=%d v=%02x %d\n", j, pg[j], pg[j]);
					sprintf(tmp, "%02x", pg[j]);
  				strcat(md_input,tmp);
				}
				printf("o=%s\n",md_input);
				md5(md_input,md_output);
				printf("r=%s\n",md_output);


			memcpy(coin_id_obj[coin_id].AN[snObj.val32],md_output,AN_BYTES_CNT);

			coin_id_obj[coin_id].MFS[snObj.val32]=t->tm_mon+1;
			coin_config_obj[coin_id].pages_changed[((snObj.val32)/coin_config_obj[coin_id].page_size)]=1;
			pass_cnt++;	
		}			
	}
	status_code = MIX;
	if (fail_cnt == no_of_coins){
		status_code = ALL_FAIL;
	}else if(pass_cnt == no_of_coins){
		status_code = ALL_PASS;
	}
	index = RES_HS+HS_BYTES_CNT;
	size    =  RES_HS+HS_BYTES_CNT;
	if(status_code == ALL_PASS || status_code == MIX){
		snObj.val32 = ticket_no;
		for(j=0;j<MS_BYTES_CNT;j++)
			response[index+j]=snObj.data[MS_BYTES_CNT-1-j];
		index+=MS_BYTES_CNT;
		size+=MS_BYTES_CNT;
	}
	if(status_code == MIX){
		for(i=0;i<no_of_coins;i++){
			if( pass_fail[i]==1)
				response[index + (i/8)] |= 1<<(i%8);
		}
		size +=no_of_coins/8;
		if((no_of_coins % 8)!=0)
			size ++;		
	}
	send_response(status_code,size);
}


//---------------------------------------------------------------
// GET_TICKET COMMAND  11
//---------------------------------------------------------------
void execute_get_ticket(unsigned int packet_len, unsigned int coin_id){
	int req_body_without_coins = CH_BYTES_CNT + CMD_END_BYTES_CNT,bytes_per_coin = SN_BYTES_CNT+AN_BYTES_CNT;
	int req_header_min, no_of_coins,size,ticket_no=0;
	unsigned int i=0,index=0,j=0,pass_cnt=0,fail_cnt=0;
	unsigned char status_code,pass_fail[COINS_MAX]={0};
	printf("GET TICKET Command \n");
	no_of_coins = validate_request_body(packet_len,bytes_per_coin,req_body_without_coins,&req_header_min);
	if(no_of_coins <=0){
		return;
	}
	ticket_no=rand();
	for(i=0;i<no_of_coins;i++) {
		index = (i * bytes_per_coin) + req_header_min+CH_BYTES_CNT;
		memset(snObj.data,0,4);
		for(j=0;j<SN_BYTES_CNT;j++)
			snObj.data[j]=udp_buffer[index+(SN_BYTES_CNT-1-j)];

		index +=SN_BYTES_CNT;
		printf("Serial number %d \n", snObj.val32);
		if(snObj.val32 >=coin_id_obj[coin_id].AN_CNT){
			send_err_resp_header(COIN_NO_NOT_FOUND);
			return;
		}
		pass_fail[i]=1;
		if(memcmp(coin_id_obj[coin_id].AN[snObj.val32],&udp_buffer[index],AN_BYTES_CNT)!=0){
			pass_fail[i]=0;
			fail_cnt++;
		}
		if(pass_fail[i]==1){
			add_ticket(coin_id,ticket_no,snObj.val32);
			pass_cnt++;	
		}			
	}
	status_code = MIX;
	if (fail_cnt == no_of_coins){
		status_code = ALL_FAIL;
	}else if(pass_cnt == no_of_coins){
		status_code = ALL_PASS;
	}

	index = RES_HS+HS_BYTES_CNT;
	size    =  RES_HS+HS_BYTES_CNT;
	if(status_code == ALL_PASS || status_code == MIX){
		snObj.val32 = ticket_no;
		for(j=0;j<MS_BYTES_CNT;j++)
			response[index+j]=snObj.data[MS_BYTES_CNT-1-j];
		index+=MS_BYTES_CNT;
		size+=MS_BYTES_CNT;
	}
	if(status_code == MIX){
		for(i=0;i<no_of_coins;i++){
			if( pass_fail[i]==1)
				response[index + (i/8)] |= 1<<(i%8);
		}
		size +=no_of_coins/8;
		if((no_of_coins % 8)!=0)
			size ++;		
	}
	send_response(status_code,size);
}
//---------------------------------------------------------------
// FIND COMMAND  2
//---------------------------------------------------------------
void execute_find(unsigned int packet_len,unsigned int coin_id){
	int req_body_without_coins = CH_BYTES_CNT + CMD_END_BYTES_CNT,bytes_per_coin = SN_BYTES_CNT+AN_BYTES_CNT+PAN_BYTES_CNT;
	int req_header_min, no_of_coins,size;
	unsigned int i=0,index=0,j=0,pass_an_cnt=0,pass_pa_cnt=0,fail_cnt=0;
	unsigned char status_code,pass_fail[COINS_MAX]={0};
	printf("FIND Command \n");
	no_of_coins = validate_request_body(packet_len,bytes_per_coin,req_body_without_coins,&req_header_min);
	if(no_of_coins <=0){
		return;
	}
	for(i=0;i<no_of_coins;i++) {
		index = (i * bytes_per_coin) + req_header_min+CH_BYTES_CNT;
		memset(snObj.data,0,4);
		for(j=0;j<SN_BYTES_CNT;j++)
			snObj.data[j]=udp_buffer[index+(SN_BYTES_CNT-1-j)];

		index +=SN_BYTES_CNT;
		printf("Serial number %d \n", snObj.val32);
		if(snObj.val32 >=coin_id_obj[coin_id].AN_CNT){
			send_err_resp_header(COIN_NO_NOT_FOUND);
			return;
		}
		pass_fail[i]=FIND_COIN_AN_PASSED;
		if(memcmp(coin_id_obj[coin_id].AN[snObj.val32],&udp_buffer[index],AN_BYTES_CNT)!=0){
			pass_fail[i]=FIND_COIN_FAILED;
			fail_cnt++;
		}
		if(pass_fail[i]==FIND_COIN_FAILED){
			pass_fail[i]=FIND_COIN_PA_PASSED;
			index +=AN_BYTES_CNT;
			if(memcmp(coin_id_obj[coin_id].AN[snObj.val32],&udp_buffer[index],AN_BYTES_CNT)!=0){
				pass_fail[i]=FIND_COIN_FAILED;
				fail_cnt++;
			}
		}
		if(pass_fail[i]==FIND_COIN_AN_PASSED){
			pass_an_cnt++;
		}
		if(pass_fail[i]==FIND_COIN_PA_PASSED){
			pass_pa_cnt++;
		}
	}	
	
	if(pass_an_cnt == no_of_coins){
		status_code = FIND_ALL_AN;
	}else if(pass_pa_cnt == no_of_coins){
		status_code = FIND_ALL_PA;
	}else if (fail_cnt == no_of_coins*2){
		status_code = FIND_ALL_NONE;
	}else{
		status_code = FIND_MIXED;
	}

	index = RES_HS+HS_BYTES_CNT;
	size    =  RES_HS+HS_BYTES_CNT;

	for(i=0;i<no_of_coins;i++){
		response[index] = pass_fail[i];
		index++;
	}
	size +=no_of_coins;
	send_response(status_code,size);
}
//-------------------------------------------------------------------------------------------------
// Fix Validate result gathering
//-------------------------------------------------------------------------------------------------
void* fix_validate_result(void *args){
  union coversion snObj;
	struct thread_args *thread_args_obj =  (struct thread_args *)args;
	unsigned int i =0,len,index,no_of_srNos,j=0,k=0,n=0,rand_no;
	unsigned char buffer[64]={0,0,0,0,0,5,0,0,0,0,0,0,22,22,0,1,0,0,0,0,0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,1,102,212,43,55,0x3E,0x3E};
	unsigned char recv_buffer[COINS_MAX],buffer_tmp[1024],encry[AN_BYTES_CNT]={0},nounce[NOUNCE_BYTES_CNT]={0};
	buffer[38]=server_config_obj.raida_id;
	buffer[REQ_RI]=thread_args_obj->dest_raida;
	memset(snObj.data,0,4);
	snObj.val32= thread_args_obj->ticket_no;


  buffer[8] = thread_args_obj->coin_id;

	index=39;
	for(k=0;k<TK_BYTES_CNT;k++)
		buffer[index+k]=snObj.data[(TK_BYTES_CNT-1-k)];
	len=0;
	for(i=0;i<64;i++){
		len++;
		if(buffer[i]==0x3E && buffer[i-1]==0x3E){
			break;
		}
	}
	srand (time(NULL));	
	rand_no=rand() % (server_config_obj.my_id_coins_cnt-1); 
	//printf("The random number is %d",rand_no);
	buffer[REQ_EN] = ENCRYP_128_AES_CTR_SN;
	snObj.val32 = my_id_coins_obj[rand_no].sr_no;
	for(j=0;j<SN_BYTES_CNT;j++){
		buffer[REQ_SR_N0+j]=snObj.data[SN_BYTES_CNT-1-j];
	}
	//We take nouce 5 bytes
	for(i=0;i<5;i++){
		nounce[i] = buffer[REQ_NO_1+i];
	}
	j=0;
	//We take nouce 3 bytes 
	for(i=5;i<8;i++,j++){
		nounce[i] = buffer[REQ_NO_6+j];
	}
		
	memcpy(buffer_tmp,&buffer[REQ_HEAD_MIN_LEN],len-REQ_HEAD_MIN_LEN);
	memcpy(encry,my_id_coins_obj[rand_no].AN[thread_args_obj->dest_raida],AN_BYTES_CNT);//


	crypt_ctr(encry,buffer_tmp,len-REQ_HEAD_MIN_LEN,nounce);
	memcpy(&buffer[REQ_HEAD_MIN_LEN],buffer_tmp,len-REQ_HEAD_MIN_LEN);
	memcpy(dns_config_obj[thread_args_obj->dest_raida].buffer,buffer,len);
	printf("\n");
	for(i=0;i<len;i++){
		printf("%d,",dns_config_obj[thread_args_obj->dest_raida].buffer[i]);
	}
  printf("\nsent\n");

	send_to_dns(thread_args_obj->dest_raida,len);
	n=listen_dns_socket(thread_args_obj->dest_raida);
	if(n>0){
		no_of_srNos = (n-RESP_BUFF_MIN_CNT)/SN_BYTES_CNT;

    printf("\nRaida %d received before enc\n", thread_args_obj->dest_raida);
	for(i=0;i<len;i++){
		printf("%d,",dns_config_obj[thread_args_obj->dest_raida].buffer[i]);
	}
  printf("\nreceived enc stuff\n");
  for (int i = 0; i < AN_BYTES_CNT; i++) {
    printf("%d ", encry[i]);
  }
  printf("\nreceived nnnonc\n");
  for (int i = 0; i < 16; i++) {
    printf("%d ", nounce[i]);
  }
/*
		memcpy(buffer_tmp,&dns_config_obj[thread_args_obj->dest_raida].buffer[REQ_HEAD_MIN_LEN],len-REQ_HEAD_MIN_LEN);
		memcpy(encry,my_id_coins_obj[rand_no].AN[thread_args_obj->dest_raida],AN_BYTES_CNT);//
		crypt_ctr(encry,buffer_tmp,len-REQ_HEAD_MIN_LEN,nounce);
		memcpy(&dns_config_obj[thread_args_obj->dest_raida].buffer[REQ_HEAD_MIN_LEN],buffer_tmp,len-REQ_HEAD_MIN_LEN);		
    */
		memcpy(buffer_tmp,&dns_config_obj[thread_args_obj->dest_raida].buffer[RES_HEAD_MIN_LEN],n-RES_HEAD_MIN_LEN);
//		memcpy(encry,my_id_coins_obj[rand_no].AN[thread_args_obj->dest_raida],AN_BYTES_CNT);//
		crypt_ctr(encry,buffer_tmp,n-RES_HEAD_MIN_LEN,nounce);
		memcpy(&dns_config_obj[thread_args_obj->dest_raida].buffer[RES_HEAD_MIN_LEN],buffer_tmp,n-RES_HEAD_MIN_LEN);		

    printf("\nRaida %d received after enc\n", thread_args_obj->dest_raida);
	for(i=0;i<len;i++){
		printf("%d,",dns_config_obj[thread_args_obj->dest_raida].buffer[i]);
	}

		printf("\nSerial Numbers cnt : %d",no_of_srNos);
		index=RESP_BUFF_MIN_CNT;
		fix_validate_obj[thread_args_obj->dest_raida].srNoCnt=no_of_srNos;
		for(j=0;j<no_of_srNos;j++){
			memset(snObj.data,0,4);
			for(k=0;k<SN_BYTES_CNT;k++)
				snObj.data[k]=dns_config_obj[thread_args_obj->dest_raida].buffer[index+(SN_BYTES_CNT-1-k)];
			index+=SN_BYTES_CNT;
			fix_validate_obj[thread_args_obj->dest_raida].srNo[j]=snObj.val32;	
			printf("\n Sr no : %d",snObj.val32);	
		}
		
	}

}
//---------------------------------------------------------------
// FIX COMMAND  50
//---------------------------------------------------------------
void execute_fix(unsigned int packet_len, unsigned int coin_id,unsigned char version){
	int req_body_without_coins = CH_BYTES_CNT + (TK_BYTES_CNT*RAIDA_SERVER_MAX)+ PG_BYTES_CNT+CMD_END_BYTES_CNT,bytes_per_coin = SN_BYTES_CNT;
	int req_header_min, no_of_coins,size,srNoMatchedCnt=0;
	unsigned int i=0,index=0,j=0,m=0,k=0,pass_cnt=0,fail_cnt=0;
	unsigned char status_code,pass_fail[COINS_MAX]={0},current_raida_flg=0,pg[PG_BYTES_CNT],pan[PAN_BYTES_CNT],md_input[64],md_output[64],tmp[64];
	uint32_t input_sr_nos[COINS_MAX],ticket_no;
	pthread_t ptid[RAIDA_SERVER_MAX];
	if(version == FIX_VERSION_1){
		printf("FIX Command Ver-1\n");	
	}else if(version == FIX_VERSION_2){
		printf("FIX Command Ver-2\n");	
		bytes_per_coin += PAN_BYTES_CNT;
    req_body_without_coins -= PG_BYTES_CNT;
	}
	no_of_coins = validate_request_body(packet_len,bytes_per_coin,req_body_without_coins,&req_header_min);
	if(no_of_coins <=0){
		return;
	}
	//Check for all tickets as 0 except the current raida
	index = (no_of_coins * bytes_per_coin) + req_header_min+CH_BYTES_CNT;
	if(version == FIX_VERSION_1){
		memcpy(pg,&udp_buffer[index],PG_BYTES_CNT);
		index+=PG_BYTES_CNT;
	}
	for(i=0;i<RAIDA_SERVER_MAX;i++){
		memset(snObj.data,0,4);
		for(k=0;k<TK_BYTES_CNT;k++)
			snObj.data[k]=udp_buffer[index+(TK_BYTES_CNT-1-k)];
		if(server_config_obj.raida_id !=i){
			if(snObj.val32!=0){
				break;
			}
		}else{
			if(snObj.val32==0){
				current_raida_flg= 1;
				ticket_no = snObj.val32;	
			}
		}
		index+=TK_BYTES_CNT;	
	}
	//if raida's have 0 in ticket field
	if(i>=RAIDA_SERVER_MAX){
		//if this raida also have 0 in ticket field
		if(current_raida_flg==1){
			send_response(FAIL,RES_HS+HS_BYTES_CNT);
			return;
		}
		//if this raida alone has ticket in tiecket field
		//check if the ticket is available
		for(i=0;i<coin_id_obj[coin_id].AN_CNT;i++){
			if(coin_id_obj[coin_id].TICKETS[i]==NULL)continue;
			if(ticket_no == coin_id_obj[coin_id].TICKETS[i]->ticket_no){
				break;
			}
		}
		if(i>=coin_id_obj[coin_id].AN_CNT){
			send_response(FAIL,RES_HS+HS_BYTES_CNT);
			return;
		}	
		for(i=0;i<no_of_coins;i++){
			index = (i * bytes_per_coin) + req_header_min+CH_BYTES_CNT;
			memset(snObj.data,0,4);
			for(k=0;k<SN_BYTES_CNT;k++)
				snObj.data[k]=udp_buffer[index+(SN_BYTES_CNT-1-k)];
			if(version == FIX_VERSION_1){
				sprintf(md_input, "%d", udp_buffer[REQ_RI]);
				sprintf(tmp, "%d", snObj.val32);
				strcat(md_input,tmp);
				for(j=0;j<PG_BYTES_CNT;j++){		
					sprintf(tmp, "%02x", pg[j]);
  				strcat(md_input,tmp);
				}
				printf("%s\n",md_input);
				md5(md_input,md_output);
				memcpy(coin_id_obj[coin_id].AN[snObj.val32],md_output,AN_BYTES_CNT);
			}else if(version == FIX_VERSION_2){
				index+=PAN_BYTES_CNT;
				memcpy(coin_id_obj[coin_id].AN[snObj.val32],&udp_buffer[index],PAN_BYTES_CNT);	
			}
			coin_config_obj[coin_id].pages_changed[((snObj.val32)/coin_config_obj[coin_id].page_size)]=1;
			pass_cnt++;
			pass_fail[i]=1;
			
		}
	}else{

		index = (no_of_coins * bytes_per_coin) + req_header_min+CH_BYTES_CNT;
  	if (version == FIX_VERSION_1){
      index += PG_BYTES_CNT;
    }
		//Send all tickets to respective raidas and get the serial number's
		for(i=0;i<RAIDA_SERVER_MAX;i++){
			memset(snObj.data,0,4);
			for(k=0;k<TK_BYTES_CNT;k++)
				snObj.data[k]=udp_buffer[index+(TK_BYTES_CNT-1-k)];
			if(server_config_obj.raida_id !=i){
				thread_args_obj[i].dest_raida = i;
				thread_args_obj[i].ticket_no = snObj.val32;
				thread_args_obj[i].coin_id = coin_id;

        printf("sending ticket to raida %d (%x, %d)\n", i, snObj.val32, snObj.val32);
				pthread_create(&ptid[i], NULL, &fix_validate_result, &thread_args_obj[i]);
			}
			index+=TK_BYTES_CNT;	
		}
		for(i=0;i<RAIDA_SERVER_MAX;i++){
			if(server_config_obj.raida_id !=i){
				pthread_join(ptid[i], NULL);
			}
		}
		//Search all SrNo's 
		fail_cnt =0;
		pass_cnt =0;
		for(i=0;i<no_of_coins;i++){
			srNoMatchedCnt=0;
			index = (i * bytes_per_coin) + req_header_min+CH_BYTES_CNT;
			memset(snObj.data,0,4);
			for(k=0;k<SN_BYTES_CNT;k++)
				snObj.data[k]=udp_buffer[index+(SN_BYTES_CNT-1-k)];
			if(version == FIX_VERSION_2){
				index+=SN_BYTES_CNT;
				memcpy(pan,&udp_buffer[index],PAN_BYTES_CNT);
			}
			fail_cnt++;
			pass_fail[i]=0;

      printf("done sending\n");
			for(m=0;m<RAIDA_SERVER_MAX;m++){
        printf("resp r%d\n", m);
				for(j=0;j<fix_validate_obj[m].srNoCnt;j++){
          printf(" resp r%d j=%d v=%d rv=%d\n", m, j, snObj.val32, fix_validate_obj[m].srNo[j]);
					if(fix_validate_obj[m].srNo[j]== snObj.val32){
						srNoMatchedCnt++;
						if(srNoMatchedCnt==FIX_SRNO_MATCH_CNT) {
							if(version == FIX_VERSION_1){
								sprintf(md_input, "%d", udp_buffer[REQ_RI]);
								sprintf(tmp, "%d", snObj.val32);
								strcat(md_input,tmp);
								for(k=0;k<PG_BYTES_CNT;k++){		
									sprintf(tmp, "%02x", pg[k]);
									strcat(md_input,tmp);
								}
								printf("%s\n",md_input);
								md5(md_input,md_output);
								memcpy(coin_id_obj[coin_id].AN[snObj.val32],md_output,PG_BYTES_CNT);
							}else if(version == FIX_VERSION_2){
								memcpy(coin_id_obj[coin_id].AN[snObj.val32],pan,PG_BYTES_CNT);
							}
							coin_config_obj[coin_id].pages_changed[((snObj.val32)/coin_config_obj[coin_id].page_size)]=1;
							pass_cnt++;
							pass_fail[i]=1;
							fail_cnt--;
							break;
			
						}
					}
				}
			}
			
		}
	}
	status_code = MIX;
	if (fail_cnt >= no_of_coins){
		status_code = ALL_FAIL;
	}else if(pass_cnt >= no_of_coins){
		status_code = ALL_PASS;
	}

	index = RES_HS+HS_BYTES_CNT;
	size    =  RES_HS+HS_BYTES_CNT;
	if(status_code == MIX){
		for(i=0;i<no_of_coins;i++){
			if( pass_fail[i]==1)
				response[index + (i/8)] |= 1<<(i%8);
		}
		size +=no_of_coins/8;
		if((no_of_coins % 8)!=0)
			size ++;		
	}
	send_response(status_code,size);	
}
//---------------------------------------------------------------
// ECHO COMMAND  4
//---------------------------------------------------------------
void execute_echo(unsigned int packet_len){
	int req_body = 0,req_header_min=0,size;
	printf("ECHO Command \n");
	size    =  RES_HS+HS_BYTES_CNT;
	send_response(SUCCESS,size);
}
//-----------------------------------------------------------------
//VALIDATE COMMAND 5
//-----------------------------------------------------------------
void execute_validate(unsigned int packet_len,unsigned int coin_id){
	int req_body_with_ticket = CH_BYTES_CNT + RAIDA_BYTE_CNT+ TK_BYTES_CNT +  CMD_END_BYTES_CNT;
	unsigned int index=0,i=0,j=0,k=0,size,ticket_no=0;
	unsigned char status_code,raida_id;
	uint32_t tmp,time_val;
	int req_header_min;
	printf("VALIDATE Command coind_id=%u\n", coin_id);
	if (validate_request_body_general(packet_len,req_body_with_ticket,&req_header_min) ==0){
		return;
	}
	index = req_header_min+CH_BYTES_CNT;
	raida_id = udp_buffer[index];
	if(raida_id>=RAIDA_SERVER_MAX) {
		send_err_resp_header(VALIDATE_TICKET_INVALID_RAIDA);
		return;
	}
	index = req_header_min+CH_BYTES_CNT + RAIDA_BYTE_CNT;
	for(j=0;j<TK_BYTES_CNT;j++)
		snObj.data[j]=udp_buffer[index+(TK_BYTES_CNT-1-j)];
	printf("Ticket number %d \n", snObj.val32);
	ticket_no= snObj.val32;
	index = RES_HS+TK_BYTES_CNT;
	size    =  RES_HS+TK_BYTES_CNT;
	status_code=VALIDATE_TICKET_NOT_FOUND;

	time_val=time(NULL);
	for(i=0;i<coin_id_obj[coin_id].AN_CNT;i++){
		if(coin_id_obj[coin_id].TICKETS[i]==NULL)continue;
		if((time_val  - coin_id_obj[coin_id].TICKETS[i]->time_stamp) > server_config_obj.del_ticket_time_intervel){
			free(coin_id_obj[coin_id].TICKETS[i]);
			coin_id_obj[coin_id].TICKETS[i]=NULL;
			continue;
		}
		if(ticket_no == coin_id_obj[coin_id].TICKETS[i]->ticket_no){
			//printf("The Ticket no %d found at serial no %d with time stamp %d\n", master_ticket_obj[i].ticket_no,snObj.val32,master_ticket_obj[i].time_stamp);
			tmp=0;
			tmp = (uint32_t)(1 << raida_id);
      printf("found ticket %d (%x) %x rid=%d sn=%d", ticket_no, coin_id_obj[coin_id].TICKETS[i]->raida_claim, tmp, raida_id, i);
			if(coin_id_obj[coin_id].TICKETS[i]->raida_claim & tmp){
				snObj.val32 = i;
				for(k=0;k<SN_BYTES_CNT;k++)
					response[index+k]=snObj.data[SN_BYTES_CNT-1-k];
				coin_id_obj[coin_id].TICKETS[i]->raida_claim &=  (~tmp);
				index +=SN_BYTES_CNT;
				size+=SN_BYTES_CNT;
				printf("claim %x\n",coin_id_obj[coin_id].TICKETS[i]->raida_claim);
				status_code=VALIDATE_TICKET_CLAIMED;
			}else{
				status_code=VALIDATE_TICKET_CLAIMED_EARLIER;
			}
		}
	}
  printf("rrr before\n");
  for(k=0;k<RES_HEAD_MIN_LEN + size;k++) {
    printf("%d ", response[k]);
  }
	send_response(status_code,size);
  printf("\nrrr after\n");
  for(k=0;k<RES_HEAD_MIN_LEN + size;k++) {
    printf("%d ", response[k]);
  }
}

//---------------------------------------------------------------
// DETECT COMMAND  5
//---------------------------------------------------------------
void execute_detect(unsigned int packet_len, unsigned int coin_id){
	int req_body_without_coins = CH_BYTES_CNT + CMD_END_BYTES_CNT,bytes_per_coin = SN_BYTES_CNT+AN_BYTES_CNT;
	int req_header_min, no_of_coins,size=0;
	unsigned int i=0,index=0,j=0,pass_cnt=0,fail_cnt=0;
	unsigned char status_code,pass_fail[COINS_MAX]={0};
	printf("DETECT Command \n");
	no_of_coins = validate_request_body(packet_len,bytes_per_coin,req_body_without_coins,&req_header_min);
	if(no_of_coins <=0){
		return;
	}
	for(i=0;i<no_of_coins;i++) {
		index = (i * bytes_per_coin) + req_header_min+CH_BYTES_CNT;
		memset(snObj.data,0,4);
		for(j=0;j<SN_BYTES_CNT;j++)
			snObj.data[j]=udp_buffer[index+(SN_BYTES_CNT-1-j)];

		index +=SN_BYTES_CNT;
		printf("Serial number %d \n", snObj.val32);
		if(snObj.val32 >=coin_id_obj[coin_id].AN_CNT){
			send_err_resp_header(COIN_NO_NOT_FOUND);
			return;
		}
		pass_fail[i]=1;
		if(memcmp(coin_id_obj[coin_id].AN[snObj.val32],&udp_buffer[index],AN_BYTES_CNT)!=0){
			pass_fail[i]=0;
			fail_cnt++;
		}
		if(pass_fail[i]==1){
			pass_cnt++;	
		}			
	}
	status_code = MIX;
	if (fail_cnt == no_of_coins){
		status_code = ALL_FAIL;
	}else if(pass_cnt == no_of_coins){
		status_code = ALL_PASS;
	}
	index = RES_HS+HS_BYTES_CNT;
	size    =  RES_HS+HS_BYTES_CNT;
	if(status_code == MIX){
		for(i=0;i<no_of_coins;i++){
			if( pass_fail[i]==1)
				response[index + (i/8)] |= 1<<(i%8);
		}
		size +=no_of_coins/8;
		if((no_of_coins % 8)!=0)
			size ++;		
	}
	send_response(status_code,size);
}