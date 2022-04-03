#include "raida_server.h"
#include "udp_socket.h"
char execpath[256];
struct server_config server_config_obj;
unsigned char shards_config[SHARDS_MAX];
struct dns_config dns_config_obj[RAIDA_SERVER_MAX];
struct coin_id coin_id_obj[255];
struct coin_config *coin_config_obj;
unsigned int coin_id_cnt;
uint8_t encrypt_key[ENCRYPTION_CONFIG_BYTES];
struct my_id_coins *my_id_coins_obj;

//----------------------------------------------------------
//Welcome message
//----------------------------------------------------------
void welcomeMsg() {
  printf("\nWelcome to PLAYCOIN\n");
}
//----------------------------------------------------------
//Loads raida no from raida_no.txt
//----------------------------------------------------------
int load_raida_no(){
  FILE *fp_inp=NULL;
  int size=0,ch;
  unsigned char buff[24];
  char path[256];
  strcpy(path,execpath);
  strcat(path,"/Data/raida_no.txt");
  if ((fp_inp = fopen(path, "r")) == NULL) {
    printf("raida_no.txt Cannot be opened , exiting \n");
    return 1;
  }
  while( ( ch = fgetc(fp_inp) ) != EOF ){
    size++;
  }
  fclose(fp_inp);
  fp_inp = fopen(path, "r");
  if(fread(buff, 1, size, fp_inp)<size){
    printf("Configuration parameters missing in raida_no.txt \n");
    return 1;
  }
  if(size == 2){
    server_config_obj.raida_id = (buff[0]-48)*10;
    server_config_obj.raida_id+= (buff[1]-48);
  }else{
    server_config_obj.raida_id=buff[0]-48;
  }
  printf("Raida Id  :-%d \n", server_config_obj.raida_id);
  fclose(fp_inp);
  return 0;
}
//----------------------------------------------------------
//Loads server configuation from server.bin
//----------------------------------------------------------
int load_server_config() {
  FILE *fp_inp = NULL;
  int cnt=0;
  unsigned char buff[SERVER_CONFIG_BYTES];
  char path[256];
  strcpy(path,execpath);
  strcat(path,"/Data/server.bin");
  if ((fp_inp = fopen(path, "rb")) == NULL) {
    printf("server.bin Cannot be opened , exiting \n");
    return 1;
  }
  if(fread(buff, 1, SERVER_CONFIG_BYTES, fp_inp)<SERVER_CONFIG_BYTES){
    printf("Configuration parameters missing in server.bin \n");
    return 1;
  }
  server_config_obj.raida_id=buff[0];
  server_config_obj.port_number = buff[2];
  server_config_obj.port_number|= (((uint16_t)buff[1])<<8);
  server_config_obj.security_check_frequency = buff[3] * SECURITY_CHECK_MUL_FACTOR;
  server_config_obj.backup_frequency = buff[4] * BACKUP_FREQ_MUL_FACTOR;
  server_config_obj.refresh_dns = buff[5] * REFRESH_DNS_MUL_FACTOR;
  server_config_obj.show_regs_coins_max = buff[7];
  server_config_obj.show_regs_coins_max |= (((uint16_t)buff[6])<<8);
  server_config_obj.show_denom_coins_max = buff[9];
  server_config_obj.show_denom_coins_max |= (((uint16_t)buff[8])<<8);
  server_config_obj.show_change_coins_max = buff[11];
  server_config_obj.show_change_coins_max |= (((uint16_t)buff[10])<<8);
  server_config_obj.bytes_per_frame = buff[13];
  server_config_obj.bytes_per_frame |= (((uint16_t)buff[12])<<8);
  server_config_obj.del_ticket_time_intervel = (uint16_t)buff[14];
  server_config_obj.del_free_id_time_intervel = buff[15];
  server_config_obj.del_encryp2_time_intervel = buff[16];
  server_config_obj.deno_type = buff[17];
  server_config_obj.my_id_coins_cnt= buff[19];
  server_config_obj.my_id_coins_cnt |= (((uint16_t)buff[18])<<8);
  printf("------------------------------\n");
  printf("Server Configuration Details..\n");
  printf("------------------------------\n");
  printf("Raida_Id  :-%d \n", server_config_obj.raida_id);
  printf("Port Number :- %d \n", server_config_obj.port_number);
  printf("Security check frequency :-%d\n", server_config_obj.security_check_frequency);
  printf("Backup frequency :- %d \n", server_config_obj.backup_frequency);
  printf("Refresh DNS :-%d \n", server_config_obj.refresh_dns);
  printf("Show register coins max :- %d \n", server_config_obj.show_regs_coins_max);
  printf("Show denom coins max :- %d \n", server_config_obj.show_denom_coins_max);
  printf("Show change coins max :- %d \n", server_config_obj.show_change_coins_max);
  printf("Bytes per UDP Request body :- %d \n",server_config_obj.bytes_per_frame);
  printf("Delete Ticket time intervel in secs :- %d \n",server_config_obj.del_ticket_time_intervel);
  printf("Free ID time intervel in days :- %d \n",server_config_obj.del_free_id_time_intervel);
  printf("Encryption 2 time intervel in secs :- %d \n",server_config_obj.del_encryp2_time_intervel);
  printf("Denomination type :- %d \n",server_config_obj.deno_type);
  printf("My Id coins count :- %d \n", server_config_obj.my_id_coins_cnt);
  fclose(fp_inp);
  return 0;
}
//----------------------------------------------------------
//Loads coins configuation from coins_config.bin
//---------------------------------------------------------
int load_coin_config(){
  FILE *fp_inp = NULL;
  unsigned int cnt=0,size=0,i=0,index=0,coin_id_max=0;
  unsigned char buff[SERVER_CONFIG_BYTES];
  char path[256];
  strcpy(path,execpath);
  strcat(path,"/Data/coin_config.bin");
  if ((fp_inp = fopen(path, "rb")) == NULL) {
    printf("coin_config.bin Cannot be opened , exiting \n");
    return 1;
  }
  fseek(fp_inp, 0L, SEEK_END);
  size = ftell(fp_inp);
  fseek(fp_inp, 0L, SEEK_SET);
  if(fread(buff, 1, size, fp_inp)<size){
    printf("Configuration parameters missing in coin_config.bin \n");
    return 1;
  }

  coin_config_obj = (struct coin_config *) malloc(sizeof(struct coin_config)*(size/COIN_CONFIG_BYTES));
  coin_id_cnt = size/COIN_CONFIG_BYTES;
  if (coin_id_cnt != COIN_TABLES_CNT){
    printf("Configuration parameters missing in coin_config.bin \n");
    return 1;
  }
  index=0;
  printf("------------------------------\n");
  printf("Coin's Configuration Details..\n");
  printf("------------------------------\n");
  printf("Coin id count -: %d\n", coin_id_cnt);
  for(i=0;i<(size/COIN_CONFIG_BYTES);i++){
    if(coin_config_obj[i].coin_id>coin_id_max){
      coin_id_max = coin_config_obj[i].coin_id;
    }
    coin_config_obj[i].coin_id = buff[index+0];
    coin_config_obj[i].page_size = buff[index+2];
    coin_config_obj[i].page_size |= (((uint16_t)buff[index+1])<<8);
    coin_config_obj[i].no_of_pages = buff[index+4];
    coin_config_obj[i].no_of_pages |= (((uint16_t)buff[index+3])<<8);
    printf("Coin Id  -: %d ", coin_config_obj[i].coin_id);
    printf("\t AN's per page  -: %d", coin_config_obj[i].page_size);
    printf("\t No of Pages    -: %d\n", coin_config_obj[i].no_of_pages);
    index+=COIN_CONFIG_BYTES;
  }
  //coin_id_obj = (struct coin_id *) malloc(sizeof(struct coin_id)*(coin_id_max+1));
  fclose(fp_inp);
  return 0;
}

//----------------------------------------------------------
//Loads shards configuation from shards.bin
//---------------------------------------------------------
int load_shards_config() {
  FILE *fp_inp = NULL;
  int i = 0;
  unsigned char buff[SHARDS_MAX];
  char path[256];
  strcpy(path,execpath);
  strcat(path,"/Data/shards.bin");
  if ((fp_inp = fopen(path, "rb")) == NULL) {
    printf("shards.bin Cannot be opened , exiting \n");
    return 1;
  }
  if(fread(buff, 1, SHARDS_MAX, fp_inp)<SHARDS_MAX){
    printf("Configuration parameters missing in shards.bin \n");
    return 1;
  }
  printf("------------------------------\n");
  printf("Shards Configuration Details..\n");
  printf("------------------------------\n");
  for (i = 0;i < SHARDS_MAX;i++) {
    shards_config[i] = buff[i];
    if (shards_config[i] == 0xff) {
      printf("Shard no :- %d available\n", i+1);
    }else {
      printf("Shard no :- %d not-available\n", i+1);
    }
  }
  fclose(fp_inp);
  return 0;
}
//----------------------------------------------------------
//Loads dns configuation from dns.bin
//---------------------------------------------------------
int load_dns_config() {
  FILE *fp_inp = NULL;
  int i = 0,j=0,index=0;
  unsigned int dns_port;
  char dns_ip[64],tmp[16];
  unsigned char buff[RAIDA_SERVER_MAX*(DNS_LEN_MAX+DNS_PORT_MAX)];
  union coversion{
    unsigned char data[4];
    uint32_t val32;
  }convObj;
  char path[256];
  strcpy(path,execpath);
  strcat(path,"/Data/dns.bin");
  if ((fp_inp = fopen(path, "rb")) == NULL) {
    printf("dns.bin Cannot be opened , exiting \n");
    return 1;
  }
  if(fread(buff, 1, RAIDA_SERVER_MAX*(DNS_LEN_MAX+DNS_PORT_MAX), fp_inp)<(RAIDA_SERVER_MAX*(DNS_LEN_MAX+DNS_PORT_MAX))){
    printf("Configuration parameters missing in dns.bin \n");
    return 1;
  }
  printf("------------------------------\n");
  printf("DNS Configuration Details..\n");
  printf("------------------------------\n");
  index =0;	
  for (i=0;i<RAIDA_SERVER_MAX;i++) {
    strcpy(dns_ip,"");
    for (j=0; j<DNS_LEN_MAX; j++) {
      sprintf(tmp, "%d", buff[index+j]);
      strcat(dns_ip,tmp);
      if(j<DNS_LEN_MAX-1)
        strcat(dns_ip,"."); 
    }
    index=index+DNS_LEN_MAX;
    convObj.val32 = 0;
    for (j=0; j<DNS_PORT_MAX; j++) {
      convObj.data[DNS_PORT_MAX-1-j] = buff[index+j];	
    }
    index=index+DNS_PORT_MAX;
    dns_port = convObj.val32;
    if(server_config_obj.raida_id!=i){
      //-----init_dns_socket(i,dns_port,dns_ip);
    }
    printf("Raida :- %d \tdns :-%s         \t Port Number :-% d\n",i+1, dns_ip,dns_port);
  }
  fclose(fp_inp);
  return 0;
} 
//----------------------------------------------------------
//Loads encrypt key from encryption_key.bin
//--------------------------------------------------------- 
int load_encrypt_key(){
  FILE *fp_inp = NULL;
  int i = 0;
  unsigned char buff[ENCRYPTION_CONFIG_BYTES];
  char path[256];
  strcpy(path,execpath);
  strcat(path,"/Data/encryption_key.bin");
  printf("------------------------------\n");
  printf("ENCRYPTION CONFIG KEY ..\n");
  printf("------------------------------\n");
  if ((fp_inp = fopen(path, "rb")) == NULL) {
    printf("encryption_key.bin Cannot be opened , exiting \n");
    return 1;
  }
  if(fread(buff, 1, ENCRYPTION_CONFIG_BYTES, fp_inp)<(ENCRYPTION_CONFIG_BYTES)){
    printf("Configuration parameters missing in encryption_key.bin \n");
    return 1;
  }
  memcpy(encrypt_key,buff,ENCRYPTION_CONFIG_BYTES);
  for(i=0;i<ENCRYPTION_CONFIG_BYTES;i++){
    printf("%02x,",encrypt_key[i]);
  }
  printf("\n");
  fclose(fp_inp);
  return 0;
}
//----------------------------------------------------------
//configure Authentication numbers 
//---------------------------------------------------------
int configure_an(unsigned int index, int alloc_only){

  FILE *fp_inp = NULL;
  unsigned int i = 0,j=0,an_cnt,no_of_bytes,size;
  unsigned char buff[AN_BYTES_CNT+MS_BYTES_CNT];
  char str_page_no[16],str_coin_id[16];
  char path[256], mkdir_path[256];
  sprintf((char*)str_coin_id,"%d",coin_config_obj[index].coin_id);
  coin_config_obj[index].pages_changed = (unsigned char *) malloc(sizeof(unsigned char) * coin_config_obj[index].no_of_pages);
  memset(coin_config_obj[index].pages_changed,sizeof(unsigned char) * coin_config_obj[index].no_of_pages,0);
  // We don't need to generate anything 
  if (alloc_only) {
    return 0;
  }
  for(i=0;i<coin_config_obj[index].no_of_pages;i++){
    printf("\n Creating AN's for coin  %d Please wait... \n",index);
    strcpy(path,execpath);
    strcat(path,"/Data/coin_3");
    strcpy(mkdir_path,"mkdir -m 777 >>/dev/null 2>>/dev/null ");
    strcat(mkdir_path,path);
    system(mkdir_path);
    strcat(path,"/ANs");
    strcpy(mkdir_path,"mkdir -m 777 >>/dev/null 2>>/dev/null ");
    strcat(mkdir_path,path);
    system(mkdir_path);
    strcat(path,"/");
    sprintf((char*)str_page_no,"%d",i);		
    strcat(path,str_page_no);
    strcat(path,".bin");
    puts(path);
    if ((fp_inp = fopen(path, "ab+")) == NULL) {
      fp_inp = fopen(path, "wb+");
    }
    fseek(fp_inp, 0L, SEEK_END);
    an_cnt = ftell(fp_inp)/(AN_BYTES_CNT+MFS_BYTES_CNT);
    //printf("AN's = %d",an_cnt);
    //printf("coin = %d",coin_config_obj[index].page_size);
    if(an_cnt<coin_config_obj[index].page_size){
      unsigned char upper=255,lower=1, num[1];
      no_of_bytes=an_cnt*(AN_BYTES_CNT+MFS_BYTES_CNT);
      fseek(fp_inp,no_of_bytes,SEEK_SET);	
      for(j=0;j<(coin_config_obj[index].page_size*(AN_BYTES_CNT+MFS_BYTES_CNT))-no_of_bytes;j++){
        //srand(j);
        num[0] = 0;// (rand() % (upper - lower + 1)) + lower;
        fwrite(num,1,1,fp_inp);
      }
      printf(".");
    }
    fclose(fp_inp);
  }
  return 0;
}
//----------------------------------------------------------
//Loads Authentication numbers from AN.bin
//---------------------------------------------------------
int load_an(unsigned int index,unsigned int coin_id){
  FILE *fp_inp = NULL;
  unsigned int i=0,j=0,k=0,an_index=0, an_cnt;
  unsigned char buff[AN_BYTES_CNT+MS_BYTES_CNT];
  char str_page_no[16],str_coin_id[16],path[256];
  printf("\n------------------------------\n");
  printf("%d AN  Details.. \n", coin_id);
  printf("------------------------------\n");
  an_cnt=coin_config_obj[index].no_of_pages * coin_config_obj[index].page_size;
  coin_id_obj[coin_id].AN_CNT =  an_cnt;
  coin_id_obj[coin_id].AN = (unsigned char **) malloc(an_cnt  * sizeof(unsigned char *));
  coin_id_obj[coin_id].MFS = (unsigned char *) malloc(an_cnt  * MFS_BYTES_CNT);
  coin_id_obj[coin_id].free_id_days = (unsigned char *) malloc(an_cnt  * FREE_ID_BYTES_CNT);
  coin_id_obj[coin_id].TICKETS =(struct master_ticket **) malloc(an_cnt *sizeof(struct master_ticket *));
  sprintf((char*)str_coin_id,"%d",coin_id);
  for(i=0;i<coin_config_obj[index].no_of_pages;i++){
    printf("\n Loading AN's for coin  %d Please wait... \n",coin_id);
    strcpy(path,execpath);
    strcat(path,"/Data/coin_3");
    strcat(path,"/ANs/");
    //strcat(path,str_coin_id);
    //strcat(path,"_");
    sprintf((char*)str_page_no,"%d",i);		
    strcat(path,str_page_no);
    strcat(path,".bin");
    //puts(path);
    if ((fp_inp = fopen(path, "rb")) == NULL) {
      printf("%s",path);
      printf("Cannot be opened , exiting \n");
      return 1;
    }
    for(j=0;j<coin_config_obj[index].page_size;j++){
      fread(buff, AN_BYTES_CNT+MFS_BYTES_CNT, 1, fp_inp) ;
      coin_id_obj[coin_id].AN[an_index]= (unsigned char *) malloc(AN_BYTES_CNT);
      memcpy(coin_id_obj[coin_id].AN[an_index],buff,AN_BYTES_CNT);
      coin_id_obj[coin_id].MFS[an_index]=buff[AN_BYTES_CNT];
      /*printf("AN  :- %d \t [",an_index );
        for(k=0;k<AN_BYTES_CNT;k++){		
        printf("%3d, ", coin_id_obj[coin_id].AN[an_index][k]);
        }
        printf("]");
        printf("\t MFS : -[%d]\n",coin_id_obj[index].MFS[an_index]);
        printf("%d,",an_index);*/
      coin_id_obj[coin_id].TICKETS[an_index]=NULL;
      coin_id_obj[coin_id].free_id_days[an_index] = 0;		
      an_index++;
    }
    printf(".");
    fclose(fp_inp);
  }
  printf("\n %d AN's loaded successfully \n",an_index);
  return 0;
}
//---------------------------------------------------------
// Adds ticket at the serial_no index
//--------------------------------------------------------
void add_ticket(unsigned int coin_id,uint32_t ticket_no,uint32_t sr_no){
  if(coin_id_obj[coin_id].TICKETS[sr_no]==NULL){
    coin_id_obj[coin_id].TICKETS[sr_no] = (struct master_ticket  *) malloc(sizeof(struct master_ticket));
  }
  coin_id_obj[coin_id].TICKETS[sr_no]->ticket_no = ticket_no; 
  coin_id_obj[coin_id].TICKETS[sr_no]->time_stamp = time(NULL);
  coin_id_obj[coin_id].TICKETS[sr_no]->raida_claim =0xffffffff;
  printf("Ticket no %d added successfully at serial no %d with time stamp %d \n",coin_id_obj[coin_id].TICKETS[sr_no]->ticket_no, sr_no, coin_id_obj[coin_id].TICKETS[sr_no]->time_stamp);
}
//----------------------------------------------------------
// Returns time in centi seconds
//----------------------------------------------------------
long get_time_cs()
{
  long            ms,cs; // Milliseconds
  time_t          s;  // Seconds
  struct timespec spec;
  clock_gettime(CLOCK_REALTIME, &spec);
  s  = spec.tv_sec;
  ms = round(spec.tv_nsec / 1.0e3); // Convert nanoseconds to milliseconds
  cs = ms /100;	
  //    printf("Current time: %"PRIdMAX".%03ld seconds since the Epoch\n",(intmax_t)s, ms);
  return ms;	
}
//---------------------------------------------------------
// Get the current directory path starting from home dir
//---------------------------------------------------------
void getexepath()
{
  char buff[256];
  int count = readlink( "/proc/self/exe", buff, 256);
  int i=0,slash_pos;
  while(buff[i]!='\0'){
    if(buff[i]=='/'){
      slash_pos = i;
    }
    i++;
  }	
  strncpy(execpath,buff,slash_pos);
}
//-------------------------------------------------------------
// update pages
//-------------------------------------------------------------
void update_an_pages(unsigned int coin_id){
  FILE *fp_inp = NULL;
  unsigned int i=0,j=0,start_index=0,end_index=0;
  char str_page_no[16],str_coin_id[16],tmp[256];
  char path[256];
  sprintf((char*)str_coin_id,"%d",coin_id);
  for(i=0;i<coin_config_obj[coin_id].no_of_pages;i++){
    if(coin_config_obj[coin_id].pages_changed[i]==1){
      printf("%d page updated",i+1);
      /*strcpy(path,execpath);
        strcat(path,"/Data/coin_");
        strcat(path,str_coin_id);
        strcat(path,"/coin_");
        strcat(path,str_coin_id);
        strcat(path,"_");
        sprintf((char*)str_page_no,"%d",i);		
        strcat(path,str_page_no);
        strcpy(tmp,path);
        strcat(path,"_an.bin");
        strcat(tmp,"_an_tmp.bin");*/

      strcpy(path,execpath);
      strcat(path,"/Data/coin_3");
      strcat(path,"/ANs/");
      sprintf((char*)str_page_no,"%d",i);		
      strcat(path,str_page_no);
      strcat(path,".bin");
      strcpy(tmp,path);
      remove(path);
      fp_inp = fopen(tmp, "wb+");
      start_index = i * coin_config_obj[coin_id].page_size;
      end_index = start_index + coin_config_obj[coin_id].page_size;
      //printf("%d - %d", start_index, end_index);
      fseek(fp_inp,0,SEEK_SET);	
      for(j=start_index;j<end_index;j++){
        fwrite(coin_id_obj[coin_id].AN[j],AN_BYTES_CNT,1,fp_inp);
        fwrite(&coin_id_obj[coin_id].MFS[j],MFS_BYTES_CNT,1,fp_inp);
      }
      coin_config_obj[coin_id].pages_changed[i]=0;
      fclose(fp_inp);
      rename(tmp,path);
    }
  }		
}
//-------------------------------------------------------------
// backup an's
//-------------------------------------------------------------
void* backup_an_thread(void *arg){
  uint32_t time1,time2,i=0;
  time1=time(NULL);
  while(1){
    time2 = time(NULL);
    if((time2 - time1) > server_config_obj.backup_frequency){
      time1 = time2;
      //printf("An Back up time \n");
      for(i=0;i<coin_id_cnt;i++){
        update_an_pages(i);
      }
    }
  }
}
int rename_an_files(unsigned int index,unsigned int coin_id){
  FILE *fp_inp = NULL;
  unsigned int i=0,j=0,k=0,an_index=0, an_cnt;
  char str_page_no[16],str_page_no_tmp[16],str_coin_id[16],path[256],tmp[256];
  printf("\n------------------------------\n");
  printf("%d AN  Details.. \n", coin_id);
  printf("------------------------------\n");
  //	coin_config_obj[index].no_of_pages = 4;
  sprintf((char*)str_coin_id,"%d",coin_id);
  for(i=0;i<coin_config_obj[index].no_of_pages;i++){
    printf("\n Renaming coin  %d Please wait... \n",i);
    strcpy(path,execpath);
    strcat(path,"/Data/coin_3");
    strcat(path,"/ANs/");
    sprintf((char*)str_page_no,"%d",i);		
    strcat(path,str_page_no);
    strcpy(tmp,path);
    strcat(path,".bin");
    strcat(tmp,"_.bin");
    puts(path);
    puts(tmp);
    rename(path,tmp);
    printf(".");
  }
  for(i=1;i<coin_config_obj[index].no_of_pages;i++){
    printf("\n Renaming coin  %d Please wait... \n",i);
    strcpy(path,execpath);
    strcat(path,"/Data/coin_3");
    strcat(path,"/ANs/");
    strcpy(tmp,path);
    sprintf((char*)str_page_no,"%d",i);		
    sprintf((char*)str_page_no_tmp,"%d",i-1);		
    strcat(path,str_page_no);
    strcat(tmp,str_page_no_tmp);
    strcat(path,"_.bin");
    strcat(tmp,".bin");
    rename(path,tmp);
  }
  strcpy(path,execpath);
  strcat(path,"/Data/coin_3");
  strcat(path,"/ANs/");
  strcpy(tmp,path);
  sprintf((char*)str_page_no,"%d",0);		
  sprintf((char*)str_page_no_tmp,"%d",coin_config_obj[index].no_of_pages-1);		
  strcat(path,str_page_no);
  strcat(path,"_.bin");
  strcat(tmp,str_page_no_tmp);
  strcat(tmp,".bin");
  rename(path,tmp);	

  /*strcpy(path,execpath);
    strcat(path,"/Data/coin_");
    strcat(path,str_coin_id);
    strcat(path,"/ANs/");
    strcpy(tmp,path);
    sprintf((char*)str_page_no,"%d",0);		
    sprintf((char*)str_page_no_tmp,"%d",0);		
    strcat(path,str_page_no);
    strcat(path,"_.bin");
    strcat(tmp,".bin");
    rename(tmp,path);	*/


  printf("\n");
  return 0;
}
//----------------------------------------------------------
// main function
//---------------------------------------------------------
int main(int argc, char *argv[]) {
  uint32_t packet_size,i=0;	
  int alloc_only = 1;
  welcomeMsg();
  getexepath();
  if(load_raida_no() || load_server_config()  || load_coin_config() || load_shards_config()  || load_dns_config() || load_encrypt_key()){
    exit(0);
  }

  if (argc > 1) {
    alloc_only = atoi(argv[1]);
    printf("Alloc only %d", alloc_only);
  }

  for(i=0;i<coin_id_cnt;i++){
    if(configure_an(i, alloc_only)){
      exit(0);
    }
  }
  for(i=0;i<coin_id_cnt;i++){
    if(load_an(i,coin_config_obj[i].coin_id)){
      exit(0);
    }
  }
  srand(time(NULL));
  //init_udp_socket();
  pthread_t ptid[4];
  //pthread_create(&ptid[0], NULL, &backup_an_thread, NULL);
  /*
  while(1) {
    listen_request();
  } */
  //rename_an_files(1,1);
  return 0;
}
