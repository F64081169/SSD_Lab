#include <unistd.h>

#include <scsi/scsi_ioctl.h>


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scsiexe.h"
#define LBA_SIZE 512

/***************************************************************************
 * name: init_io_hdr
 * parameter:
 * function: initialize the sg_io_hdr struct fields with the most common
 * 			 value
 * **************************************************************************/
struct  sg_io_hdr * init_io_hdr() {

	struct sg_io_hdr * p_scsi_hdr = (struct sg_io_hdr *)malloc(sizeof(struct sg_io_hdr));
	memset(p_scsi_hdr, 0, sizeof(struct sg_io_hdr));
	if (p_scsi_hdr) {
		p_scsi_hdr->interface_id = 'S'; /* this is the only choice we have! */
		p_scsi_hdr->flags = SG_FLAG_LUN_INHIBIT; /* this would put the LUN to 2nd byte of cdb*/
	}

	return p_scsi_hdr;
}

void destroy_io_hdr(struct sg_io_hdr * p_hdr) {
	if (p_hdr) {
		free(p_hdr);
	}
}

void set_xfer_data(struct sg_io_hdr * p_hdr, void * data, unsigned int length) {
	if (p_hdr) {
		p_hdr->dxferp = data;
		p_hdr->dxfer_len = length;
	}
}

void set_sense_data(struct sg_io_hdr * p_hdr, unsigned char * data,
		unsigned int length) {
	if (p_hdr) {
		p_hdr->sbp = data;
		p_hdr->mx_sb_len = length;
	}
}

/***************************************************************************
 * name: execute_Inquiry : Read
 * parameter:
 * 		fd:			file descripter
 * 		page_code:	cdb page code
 * 		evpd:			cdb evpd
 * 		p_hdr:		poiter to sg_io_hdr struct
 * function: make Inquiry cdb and execute it.
 * **************************************************************************/
int execute_Read(int fd, int page_code, int evpd, struct sg_io_hdr * p_hdr, int lba,int s_cnt) {
	unsigned char cdb[6];

	/* set the cdb format */
	
	cdb[0] = 0x08; 
	cdb[1] = 0;//evpd & 1;
	cdb[2] = lba & 0xff;//page_code & 0xff;
	cdb[3] = 0;
	cdb[4] = s_cnt & 0xff;
	cdb[5] = 0; 
	
//Copy Scsi Buffer to SPT Buf
	
	unsigned char buf[LBA_SIZE * s_cnt];
	p_hdr->dxfer_direction = SG_DXFER_FROM_DEV;
	p_hdr->cmdp = cdb;
	p_hdr->cmd_len = 6;
	p_hdr->dxfer_len = (LBA_SIZE * s_cnt);
	p_hdr->interface_id = 'S';
	p_hdr->dxferp = buf;

	int ret = ioctl(fd, SG_IO, p_hdr);
	if (ret<0) {
		printf("Sending SCSI Command failed.\n");
		close(fd);
		exit(1);
	}
	unsigned char * buffer = p_hdr->dxferp;
	
	
	for (int i=0; i<LBA_SIZE*s_cnt; i++) {
		printf("%hx ",buffer[i]);
	}
	putchar('\n');
	
	return p_hdr->status;
}

int execute_Write(int fd, int page_code, int evpd, struct sg_io_hdr * p_hdr,int lba,int s_cnt,long data) {
	unsigned char cdb[6];

	/* set the cdb format */
	
	cdb[0] = 0x0A; 
	cdb[1] = 0;//evpd & 1;
	cdb[2] = lba & 0xff;
	cdb[3] = 0;
	cdb[4] = s_cnt & 0xff;
	cdb[5] = 0; 
	
//Copy Scsi Buffer to SPT Buf
	
	unsigned char buf[LBA_SIZE * s_cnt];
	for(int i = 0;i<LBA_SIZE * s_cnt;i++){
		buf[i] = data & 0xff;
	}

	
	p_hdr->dxfer_direction = SG_DXFER_TO_DEV;
	p_hdr->cmdp = cdb;
	p_hdr->cmd_len = 6;
	p_hdr->dxfer_len = (LBA_SIZE * s_cnt);
	p_hdr->interface_id = 'S';
	p_hdr->dxferp = buf;

	int ret = ioctl(fd, SG_IO, p_hdr);
	if (ret<0) {
		printf("Sending SCSI Command failed.\n");
		close(fd);
		exit(1);
	}
	unsigned char * buffer = p_hdr->dxferp;
	
	for (int i=0; i<LBA_SIZE*s_cnt; i++) {
		printf("%hx ",buffer[i]);
	}
	putchar('\n');

	return p_hdr->status;
}
