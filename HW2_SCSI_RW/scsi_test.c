#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scsiexe.h"

unsigned char sense_buffer[SENSE_LEN];
unsigned char data_buffer[BLOCK_LEN*256];

void show_hdr_outputs(struct sg_io_hdr * hdr) {
	printf("status:%d\n", hdr->status);
	printf("masked_status:%d\n", hdr->masked_status);
	printf("msg_status:%d\n", hdr->msg_status);
	printf("sb_len_wr:%d\n", hdr->sb_len_wr);
	printf("host_status:%d\n", hdr->host_status);
	printf("driver_status:%d\n", hdr->driver_status);
	printf("resid:%d\n", hdr->resid);
	printf("duration:%d\n", hdr->duration);
	printf("info:%d\n", hdr->info);
}

void show_sense_buffer(struct sg_io_hdr * hdr) {
	unsigned char * buffer = hdr->sbp;
	int i;
	for (i=0; i<hdr->mx_sb_len; ++i) {
		putchar(buffer[i]);
	}
}

void show_vendor(struct sg_io_hdr * hdr) {
	unsigned char * buffer = hdr->dxferp;
	int i;
	printf("vendor id:");
	for (i=8; i<16; ++i) {
		putchar(buffer[i]);
	}
	putchar('\n');
}

void show_product(struct sg_io_hdr * hdr) {
	unsigned char * buffer = hdr->dxferp;
	int i;
	printf("product id:");
	for (i=16; i<32; ++i) {
		putchar(buffer[i]);
	}
	putchar('\n');
}

void show_product_rev(struct sg_io_hdr * hdr) {
	unsigned char * buffer = hdr->dxferp;
	int i;
	printf("product ver:");
	for (i=32; i<36; ++i) {
		putchar(buffer[i]);
	}
	putchar('\n');
}

void test_execute_Inquiry(char * path, int evpd, int page_code,int is_read,int lba,int s_cnt,long data) {

	struct sg_io_hdr * p_hdr = init_io_hdr();

	set_xfer_data(p_hdr, data_buffer, BLOCK_LEN*256);
	set_sense_data(p_hdr, sense_buffer, SENSE_LEN);

	int status = 0;

	int fd = open(path, O_RDWR);
	if (fd>0) {
		if(!is_read){
			status = execute_Read(fd, page_code, evpd, p_hdr,lba,s_cnt);
			printf("the return status is %d\n", status);
		}
		else{
			status = execute_Write(fd, page_code, evpd, p_hdr,lba,s_cnt,data);
			printf("the return status is %d\n", status);
		}
		if (status!=0) {
			show_sense_buffer(p_hdr);
			
		} else{
			//show_vendor(p_hdr);
			//show_product(p_hdr);
			//show_product_rev(p_hdr);
		}
	} else {
		printf("failed to open sg file %s\n", path);
	}
	close(fd);
	destroy_io_hdr(p_hdr);
}

int main(int argc, char * argv[]) {
	int is_read = 0;
	
	if(argv[2][0]=='r') is_read = 0;
	else is_read = 1;
	int lba = 0;
	int s_cnt = 0;
	for(int i = 3;argv[3][i]!='\0';i++){
		lba+=argv[3][i]-'0';
		lba*=10;
	}
	for(int j = 5;argv[4][j]!='\0';j++){
		s_cnt+=argv[4][j]-'0';
		s_cnt*=10;
	}
	lba/=10;
	s_cnt/=10;
	long data = 0;
	char* ptr;
	if(argc==6){
		data = strtol(argv[5],&ptr,16);
		test_execute_Inquiry(argv[1], 0, 0,is_read,lba,s_cnt,data);
		printf("%s mode,DATA=%lx, LBA=%d, Sector_count=%d\n",argv[2],data,lba,s_cnt);
	}else{	
		test_execute_Inquiry(argv[1], 0, 0,is_read,lba,s_cnt,data);
		printf("%s mode, LBA=%d, Sector_count=%d\n",argv[2],lba,s_cnt);
	}
	
	return EXIT_SUCCESS;
}
