
/*
 * Nirav.
 * The actual br2684ctl application is divided into 2 application : br2684ctld, br2684ctl
 * IFX_BR2684DEAMON : This code belongs to the daemon. When the SIGHUP is received, it
 * reads the command from BR2684_CMDFILE and creates the socket interface and keeps the socket open
 * or closes the socket for already opened interface. Thus, for all the interface in the system 
 * only one br2684ctld needs to be run. While previously, as many br2684ctl as many no. of nas interface 
 * were required to be run. Thus, this approach saves RAM.
 * IFX_BR2684CLIENT : This code belongs to the client. This is just a dummy client, which
 * write the command passed to it in the BR2684_CMDFILE and exits.
 * */

//605161:fchang 2006/5/16 Include the same header files to be consistent

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>
#include <syslog.h>
#include <atm.h>
#ifdef CONFIG_FEATURE_IFX_VLAN
#include <linux/types.h>
#endif
//605161:fchang.removed #include <linux/atmdev.h>
//605161:fchang.removed #include <linux/atmbr2684.h>

#include "linux/atmdev.h" //605161:fchang.added
#include "linux/atmbr2684.h" //605161:fchang.added


//#include "ifx_common.h"
#if defined(IFX_BR2684DEAMON) || defined(IFX_BR2684CLIENT)
#include <fcntl.h>
#include <signal.h>
#endif

#define TP_BR2684CTL_MODIFY

#define BR2684_FILE_PREFIX  "nas"

/* Written by Marcell GAL <cell@sch.bme.hu> to make use of the */
/* ioctls defined in the br2684... kernel patch */
/* Compile with cc -o br2684ctl br2684ctl.c -latm */

/*
   Modified feb 2001 by Stephen Aaskov (saa@lasat.com)
   - Added daemonization code
   - Added syslog

TODO: Delete interfaces after exit?
*/


#define LOG_NAME        "RFC1483/2684 bridge"
#define LOG_OPTION      LOG_PERROR
#define LOG_FACILITY    LOG_LOCAL0

#define IFX_MAX_PID_FILENAME	100
#define IFX_PID_FILE_PATH		"/var/run"
static char g_ifx_pidfile_prefix[IFX_MAX_PID_FILENAME];
extern int ifx_get_process_pid(char *file_prefix);
extern int ifx_rm_pid_file(char *file_prefix);
extern int ifx_create_pid_file(char *file_prefix);
extern void ifx_rm_pid_file_atexit(void);

#if defined(IFX_BR2684DEAMON) || defined(IFX_BR2684CLIENT)
#define DEAMON_PIDFILE_NAME	"br2684ctld"
#define BR2684CTL_DEAMON	"/usr/sbin/br2684ctld"
#define BR2684_CMDFILE	"/tmp/br2684cmd"
#endif /* IFX_BR2684DEAMON || IFX_BR2684CLIENT */

#ifdef IFX_BR2684CLIENT
/*Modify by Henry , Add a flag to change bridge PDU / routed PDU.*/
int main (int argc, char **argv)
{
	char strcmd[256];
	int cmd_fd = -1;
	int deamon_pid = -1;
	int i;
	memset(strcmd,0x00,sizeof(strcmd));
	for(i = 1; i < argc; i++) {
		strcat(strcmd,argv[i]);
		strcat(strcmd," ");
	}
	strcmd[strlen(strcmd) - 1] = '\0';
	cmd_fd = open(BR2684_CMDFILE,O_WRONLY | O_CREAT);
	if(cmd_fd < 0) {
		perror("br2684ctl ");
		syslog(LOG_ERR,"Could not open cmd file %s in write mode!!!",BR2684_CMDFILE);
		return -1;
	}
	//printf("-------->IFX_BR2684CLIENT: open %s and write: %s\n", BR2684_CMDFILE, strcmd);
	write(cmd_fd,strcmd,strlen(strcmd));
	close(cmd_fd);
	
	deamon_pid = ifx_get_process_pid(DEAMON_PIDFILE_NAME);
	if(deamon_pid < 1) {
		/* deamon is not running */
		system(BR2684CTL_DEAMON);
		sleep(2);
		deamon_pid = ifx_get_process_pid(DEAMON_PIDFILE_NAME);
	}

	if(deamon_pid < 1) {
		syslog(LOG_ERR,"Could not get pid of %s!!!",BR2684CTL_DEAMON);
	} else {
		syslog(LOG_INFO,"br2684_ctl : Sending SIGHUP to %d",deamon_pid);
		//printf("-------->CLIENT -> DEAMON SIGHUP\n");
		kill(deamon_pid,SIGHUP);
	}

	return 0;
}


#endif

#ifdef IFX_BR2684DEAMON
#define exit(x)		return(-(x))
#ifdef CONFIG_FEATURE_IFX_VLAN /*For DAEMON only */
struct vlan_config_cmd {
	int vlan_cmd;
	int vlan_if_num;
	char vlan_if_name[16];
	int vlan_if_untag;
	int vlan_if_tag;
	int vlan_if_vlanid;
	int vlan_if_8021p;
}g_vlan_cfg;
int g_vlan_enable=0;
#endif
int lastsock, lastitf;
int unit_id;


int fatal(const char *str, int i)
{
	syslog (LOG_ERR,"Fatal: %s",str);
	exit(2);
};


void exitFunc(void)
{
	syslog (LOG_PID,"Daemon terminated\n");
}

#ifdef TP_BR2684CTL_MODIFY	/* wzy: modify at 25Apr12 */
void list_vcc(void)
{
	int err;

	if(lastsock<0) {
		lastsock = socket(PF_ATMPVC, SOCK_DGRAM, ATM_AAL5);
	}
	if (lastsock<0) {
		syslog(LOG_ERR, "socket creation failed: %s",strerror(errno));
	} else {
		err = ioctl (lastsock, ATM_SHOWBACKENDIF, NULL/* Not use!! */);
	}
}
#endif /* #if 0 */


#ifdef TP_BR2684CTL_MODIFY	/* wzy: modify at 25Apr12 */
int create_br(char *nstr, int payload, int ext, int del)
#else
int create_br(char *nstr, int payload)
#endif /* #if 0 */
{
	int num, err;

	if(lastsock<0) {
		lastsock = socket(PF_ATMPVC, SOCK_DGRAM, ATM_AAL5);
	}
	if (lastsock<0) {
		syslog(LOG_ERR, "socket creation failed: %s",strerror(errno));
	} else {
		/* create the device with ioctl: */
		num=atoi(nstr);
		
		if( num>=0 && num<1234567890)
		{
			struct atm_newif_br2684 ni;
			ni.backend_num = ATM_BACKEND_BR2684;
			ni.media = BR2684_MEDIA_ETHERNET;
#ifdef BR2684_FLAG_ROUTED
			if (payload == 0)
				ni.media |= BR2684_FLAG_ROUTED; //Point-Point LINK Type
#endif
			ni.mtu = 1500;
			ni.payload = payload;	/* bridged or routed */

			sprintf(ni.ifname, "%s%d", BR2684_FILE_PREFIX,  num);

#ifdef TP_BR2684CTL_MODIFY	/* wzy: modify at 25Apr12 */
			if (ext > 0)
			{
				ni.media |= (ext << 24);
				
				err = ioctl (lastsock, ATM_ADDEXTBACKENDIF, &ni);
				
				if (err == 0)
				{
					printf("Interface \"%s_%d\" created sucessfully\n",ni.ifname, ext);
					syslog(LOG_INFO, "Interface \"%s_%d\" created sucessfully\n",ni.ifname, ext);
				}
				else
				{
					printf("Interface \"%s_%d\" is not be created", ni.ifname, ext);
					syslog(LOG_INFO, "Interface \"%s_%d\" is not be created", ni.ifname, ext);
				}
			}
			else if (del > 0)
			{
				ni.media |= (del << 24);
				err = ioctl (lastsock, ATM_DELEXTBACKENDIF, &ni);
				if (err == 0)
				{
					printf("Interface \"%s_%d\" deleted sucessfully\n",ni.ifname, del);
					syslog(LOG_INFO, "Interface \"%s_%d\" deleted sucessfully\n",ni.ifname, del);
				}
				else
				{
					printf("Interface \"%s_%d\" is not be deleted", ni.ifname, del);
					syslog(LOG_INFO, "Interface \"%s_%d\" is not be deleted", ni.ifname, del);
				}
			}
			else
			{
				err = ioctl (lastsock, ATM_NEWBACKENDIF, &ni);
				if (err == 0)
				{
					printf("Interface \"%s\" created sucessfully\n",ni.ifname);
					syslog(LOG_INFO, "Interface \"%s\" created sucessfully\n",ni.ifname);
				}
				else
				{
					printf("Interface \"%s\" is not be created", ni.ifname);
					syslog(LOG_INFO, "Interface \"%s\" is not be created", ni.ifname);
				}
			}
#else
			err=ioctl (lastsock, ATM_NEWBACKENDIF, &ni);

			if (err == 0)
			{
				syslog(LOG_INFO, "Interface \"%s\" created sucessfully\n",ni.ifname);
				unit_id = num;
			}
			else
				syslog(LOG_INFO, "Interface \"%s\" could not be created, reason: %s\n",
						ni.ifname, strerror(errno));
#endif

			lastitf=num;    /* even if we didn't create, because existed, assign_vcc wil want to know it! */
		} else {
			syslog(LOG_ERR,"err: strange interface number %d", num );
		}
	}
	return 0;
}

#ifdef CONFIG_FEATURE_IFX_VLAN
int wan_vlan_config(struct vlan_config_cmd *to_send, int enable, int flag)
{
	int err=0;
	struct sockaddr_atmpvc addr;
	int bufsize=8192;

	if(lastsock<0) {
//		syslog(LOG_INFO, "WAN_VLAN::socket not already present !\n");
		lastsock = socket(PF_ATMPVC, SOCK_DGRAM, ATM_AAL5);
	}
	if (lastsock<0) {
		syslog(LOG_ERR, "WAN_VLAN::socket creation failed: %s",strerror(errno));
	} else {
	//	syslog(LOG_INFO, "WAN_VLAN::socket now already present flag=[%d]!\n", flag);

		if (flag==0) {//config toggle
	//		syslog(LOG_INFO, "WAN_VLAN::Sending over [%x] Cmd [%d] val=[%d] \n", lastsock, ATM_BR2684_VLAN_CONFIG, enable);
			err = ioctl(lastsock, ATM_BR2684_VLAN_CONFIG, &enable);
			if (err <0)
				syslog(LOG_ERR, "WAN_VLAN::ioctl enable/disable error: %s",strerror(errno));
		}
		else { //config wan-vlan structure
			struct kernel_copy {
				int vlan_if_num;
				char vlan_if_name[16];
				int8_t tag_vlan_enable;
				int8_t untag_vlan;
				uint16_t vlan_vci;
			}kernel_vlan;
	//		syslog(LOG_INFO, "WAN_VLAN::(2) Sending over [%x] Cmd [%d] if[%d] tag=[%d] untag[%d] vid[%d]\n", 
//					lastsock, ATM_BR2684_VLAN_VCC_CONFIG, to_send->vlan_if_num,
//					to_send->vlan_if_tag, to_send->vlan_if_untag, to_send->vlan_if_vlanid);
			kernel_vlan.vlan_if_num = (int8_t)to_send->vlan_if_num;
			sprintf(kernel_vlan.vlan_if_name, "%s", to_send->vlan_if_name);
			kernel_vlan.tag_vlan_enable = (int8_t)to_send->vlan_if_tag;
			kernel_vlan.untag_vlan = (int8_t)to_send->vlan_if_untag;
			kernel_vlan.vlan_vci = 0;
			kernel_vlan.vlan_vci = htons((uint16_t)( (to_send->vlan_if_vlanid & 0xFFF) | 
					(0<<12) | 
					((to_send->vlan_if_8021p &0x7)<<13)
					));

			err = ioctl(lastsock, ATM_BR2684_VLAN_VCC_CONFIG, &kernel_vlan);
			if (err <0)
				syslog(LOG_ERR, "WAN_VLAN::ioctl error: %s",strerror(errno));
		}
	}
	return 0;
}
#endif

int assign_vcc(char *astr, int encap, int payload, int bufsize, struct atm_qos qos)
{
	int err=0;
	struct sockaddr_atmpvc addr;
	int fd;
	struct atm_backend_br2684 be;

	memset(&addr, 0, sizeof(addr));
	err=text2atm(astr,(struct sockaddr *)(&addr), sizeof(addr), T2A_PVC);
	if (err!=0)
		syslog(LOG_ERR,"Could not parse ATM parameters (error=%d)\n",err);

	syslog(LOG_INFO,"Communicating over ATM %d.%d.%d, encapsulation: %s\n", addr.sap_addr.itf,
			addr.sap_addr.vpi,
			addr.sap_addr.vci,
			encap?"VC mux":"LLC");

	if ((fd = socket(PF_ATMPVC, SOCK_DGRAM, ATM_AAL5)) < 0)
		syslog(LOG_ERR,"failed to create socket %d, reason: %s", errno,strerror(errno));

	if (qos.aal == 0) {
		qos.aal                     = ATM_AAL5;
		qos.txtp.traffic_class      = ATM_UBR;
		qos.txtp.max_sdu            = 1524;
		qos.txtp.pcr                = ATM_MAX_PCR;
		qos.rxtp = qos.txtp;
	}

	if ( (err=setsockopt(fd,SOL_SOCKET,SO_SNDBUF, &bufsize ,sizeof(bufsize))) )
		syslog(LOG_ERR,"setsockopt SO_SNDBUF: (%d) %s\n",err, strerror(err));

	if (setsockopt(fd, SOL_ATM, SO_ATMQOS, &qos, sizeof(qos)) < 0)
		syslog(LOG_ERR,"setsockopt SO_ATMQOS %d", errno);

	err = connect(fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_atmpvc));

	if (err < 0)
		fatal("failed to connect on socket", err);

	if(err < 0) {
		syslog(LOG_ERR,"br2684ctld : Connect Error!!!");
		//return err;
	}


	/* attach the vcc to device: */

	be.backend_num = ATM_BACKEND_BR2684;
	be.ifspec.method = BR2684_FIND_BYIFNAME;
	sprintf(be.ifspec.spec.ifname, "%s%d", BR2684_FILE_PREFIX, lastitf);
	be.fcs_in = BR2684_FCSIN_NO;
	be.fcs_out = BR2684_FCSOUT_NO;
	be.fcs_auto = 0;
	be.encaps = encap ? BR2684_ENCAPS_VC : BR2684_ENCAPS_LLC;
	be.payload = payload;
	be.has_vpiid = 0;
	be.send_padding = 0;
	be.min_size = 0;
    if(err < 0) {
    	/* This ioctl deletes/removes the net_device from kernel */ 
     	ioctl(fd, ATM_DELBACKENDIF, &be);
		return err;
    } else {	 
    	err=ioctl(fd, ATM_SETBACKEND, &be);
	}

	
	if (err == 0)
		syslog (LOG_INFO,"Interface configured");
	else {
		syslog (LOG_ERR,"Could not configure interface:%s",strerror(errno));
		exit(2);
	}
	return fd ;
}


int usage(char *s)
{
	//printf("usage: %s [-b] [-p 0|1] [-s sndbuf] [-q qos] [-c number] [-e 0|1] [-a [itf.]vpi.vci]\n", s);
	printf("Usage:\n");
	printf("\tCreate: %s [-b] [-p 0|1] [-s sndbuf] [-q qos] [-c number] [-e 0|1] [-a [itf.]vpi.vci]\n", s);
	printf("\tClose: %s -k <nas_index>\n", s);
#ifdef CONFIG_FEATURE_IFX_VLAN
	printf("\tGlobal VLAN Tag Flag: %s -v [-g 0|1] \n", s);
	printf("\tSet VLAN Tag: %s -v -i <nas_index> [-t 0|1] [-u] <vlan_id> <802.1p> \n", s);
#endif
#ifdef TP_BR2684CTL_MODIFY
	printf("\tadd/delete ext dev: %s -x/d number ... \n", s); 
	printf("\tlist atm interfaces: %s -l \n", s); 
#endif

	exit(1);
}


#if defined(IFX_BR2684DEAMON) || defined(IFX_BR2684CLIENT)
#define DEAMON_PIDFILE_NAME	"br2684ctld"
#define BR2684CTL_DEAMON	"/usr/sbin/br2684ctld"
#define BR2684_CMDFILE	"/tmp/br2684cmd"
#endif /* IFX_BR2684DEAMON || IFX_BR2684CLIENT */


#define PROGRAM_NAME	"br2684ctl"
#define CLOSEIF_OPTION	"-k"

struct br2684_sock {
	int ifnum;
	int sockfd;
	struct br2684_sock *next;
};

struct br2684_sock *brsock = NULL;	/* 每个已经打开的sar channel有一个对应的socket */

int br2684_main(int argc, char **argv);

void terminate(int num) 
{
//	syslog(LOG_INFO,"br2684ctld : Received SIGTERM or no interface configured.");
//	ifx_rm_pid_file(DEAMON_PIDFILE_NAME);
#ifdef exit
#undef exit
//	exit(0);
#endif
}

void config_br2684(int num)
{
	int cmd_fd = -1;
	char strcmd[256];
	char **argv = NULL;
	int argc = 0;
	char *ptemp = NULL;
	int sockfd = -1;
	int i = 0;

//	syslog(LOG_INFO,"br2684ctld HUP received");
	cmd_fd = open(BR2684_CMDFILE,O_RDONLY);
	if(cmd_fd < 0) {
		syslog(LOG_INFO,"br2684ctld : Could not open %s file!!!",BR2684_CMDFILE);	
		return ;
	}
//	syslog(LOG_INFO,"br2684ctld CMD file read!\n");
	memset(strcmd,0x00,sizeof(strcmd));
	if(read(cmd_fd,strcmd,sizeof(strcmd)) <= 0) {
		syslog(LOG_INFO,"br2684ctld : No data read from file %s!!!",BR2684_CMDFILE);
		close(cmd_fd);
		return ;
	}
	close(cmd_fd);

	//060414:leejack	
	//printf("-------->IFX_BR2684DEAMON: open %s and read: %s\n", BR2684_CMDFILE, strcmd);
	unlink(BR2684_CMDFILE);

	if(strstr(strcmd,CLOSEIF_OPTION)) {
		int intfnum = 0;
		struct br2684_sock *ptrcur = NULL;
		struct br2684_sock *ptrpre = NULL;
		ptemp = strstr(strcmd,CLOSEIF_OPTION);
		ptemp += strlen(CLOSEIF_OPTION);
		intfnum = atoi(ptemp);
		ptrcur = brsock;
		ptrpre = brsock;
		while(ptrcur) {
			if(ptrcur->ifnum == intfnum)
				break;
			ptrpre = ptrcur;	
			ptrcur = ptrcur->next;
		}
		
		if(ptrcur) {
//			syslog(LOG_INFO,"br2684ctld : Closing socket for nas%d",ptrcur->ifnum);
			close(ptrcur->sockfd);
			printf("%s(): close(%d): %p\n", __func__, ptrcur->sockfd, close);
			ptrpre->next = ptrcur->next;
			if(ptrcur == brsock)
				brsock = ptrcur->next;
			free(ptrcur);
		} else {
			syslog(LOG_INFO,"br2684ctld : Could not find socket for nas%d",intfnum);
		}
		if(brsock == NULL)
			terminate(SIGTERM);
		return;
	}

	argc++;
	argv = (char **)calloc(1,sizeof(char **));
	argv[argc - 1] = (char *)calloc(1, strlen(PROGRAM_NAME) + 1);
	strncpy(argv[argc - 1],PROGRAM_NAME,strlen(PROGRAM_NAME));

	ptemp = strtok(strcmd," ");
	while(ptemp) {
		argc++;
		argv = (char **)realloc(argv,argc * sizeof(char **));
		argv[argc - 1] = (char *)calloc(1,strlen(ptemp) + 1);
		strncpy(argv[argc - 1],ptemp,strlen(ptemp));
		ptemp = strtok(NULL," ");
	}
	sockfd = br2684_main(argc,argv);
	if(sockfd > 0) {
		struct br2684_sock *ptr = NULL;
		ptr = (struct br2684_sock *)calloc(1,sizeof(*ptr));
		ptr->ifnum = lastitf;
		ptr->sockfd = sockfd;
		ptr->next = brsock;
		brsock = ptr;
//		syslog(LOG_INFO,"br2684ctld : Interface nas%d with sockfd %d",lastitf,sockfd);
	} 

	for(i = 0; i < argc; i++)
		if(argv[i])
			free(argv[i]);
	free(argv);
	argc = 0;
	argv = NULL;
	if(brsock == NULL)
		terminate(SIGTERM);
	return ;
}

int main(int argc, char **argv)
{
	struct sigaction sa;

	daemon(0, 1/* 0 */);
	ifx_create_pid_file(DEAMON_PIDFILE_NAME);

	syslog(LOG_INFO,"br2684ctld started");
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = config_br2684;
	sigaction(SIGHUP, &sa, NULL);
	sa.sa_handler = terminate;
	sigaction(SIGTERM, &sa, NULL);

	while (1) sleep(10);    /* to keep the sockets... */
	return 0;
}

/*Modify by Henry , Add a flag to change bridge PDU / routed PDU.*/
int br2684_main(int argc, char **argv)
{
	int c, background=0, encap=0, sndbuf=8192;
	int payload=1;

	struct atm_qos reqqos;
	lastsock=-1;
	lastitf=0;
#ifdef CONFIG_FEATURE_IFX_VLAN
	char itf_name[20];
#endif
	int fd = -1;
	
	/* st qos to 0 */
	memset(&reqqos, 0, sizeof(reqqos));

	//openlog (LOG_NAME,LOG_OPTION,LOG_FACILITY);
	optind = 0;
	if (argc>1) {
	
#ifdef CONFIG_FEATURE_IFX_VLAN
		if (strcmp(argv[1], "-v")==0) {
			syslog(LOG_INFO,"\nEnter VLAN options\n");
			memset(&g_vlan_cfg, 0x00, sizeof(g_vlan_cfg));
			while((c = getopt(argc, argv, "vg:i:ut:I:?h")) !=EOF) {
				switch(c) {
				case 'v':
					g_vlan_cfg.vlan_cmd = 1;
					syslog(LOG_INFO,"VLAN cmd\n");
					break;
				case 'g':
					g_vlan_enable = atoi (optarg);
					syslog(LOG_INFO,"VLAN global %s\n", g_vlan_enable==1?"enable":"disable");
					wan_vlan_config(&g_vlan_cfg/*Not used*/,g_vlan_enable, 0); //0:global enable/disable 
					break;
				case 'i':
					g_vlan_cfg.vlan_if_num = atoi(optarg);
					syslog(LOG_INFO,"VLAN if num %d \n", g_vlan_cfg.vlan_if_num);
					break;
				case 'I':
					sprintf(g_vlan_cfg.vlan_if_name, "%s", optarg);
					syslog(LOG_INFO,"VLAN if name %s \n", g_vlan_cfg.vlan_if_name);
					break;
				case 'u':
					g_vlan_cfg.vlan_if_untag = 1;
					syslog(LOG_INFO,"VLAN UNTAG\n");
					break;
				case 't':
					g_vlan_cfg.vlan_if_tag = atoi(optarg);
					syslog(LOG_INFO, "VLAN TAG %s\n", g_vlan_cfg.vlan_if_tag==1?"enable":"disable");
					if(g_vlan_cfg.vlan_if_tag) {
						g_vlan_cfg.vlan_if_vlanid = atoi(argv[argc-2]);
						syslog(LOG_INFO, "VLAN VID %d \n", g_vlan_cfg.vlan_if_vlanid);
						if (argv[argc-1] != NULL ) {
						g_vlan_cfg.vlan_if_8021p = atoi(argv[argc-1]);
						syslog(LOG_INFO,"VLAN 8021P %d \n", g_vlan_cfg.vlan_if_8021p);
						}
						else {
							g_vlan_cfg.vlan_if_8021p = 0;
							syslog(LOG_INFO,"VLAN 8021P %d \n", g_vlan_cfg.vlan_if_8021p);
						}
						optind+=2;
					}
					wan_vlan_config(&g_vlan_cfg, 0/*Not used*/, 1); 
					break;
					
					case '?':
					case 'h':
						usage(argv[0]);
				}
			}
					
		}
		else 
#endif
#ifdef TP_BR2684CTL_MODIFY	/* wzy: modify at 25Apr12 */
		if (strcmp(argv[1], "-l")==0) {
			list_vcc();
			optind = argc;
		}
		else
#endif
		{
			syslog(LOG_INFO,"-v not found!\n");
#ifndef TP_BR2684CTL_MODIFY	/* wzy: modify at 25Apr12 */
		while ((c = getopt(argc, argv,"a:bc:e:s:p:q:?h")) !=EOF)
#else
		int ext_add_flag = 0, ext_add_infnum = 0;
		int ext_del_flag = 0, ext_del_infnum = 0;
		while ((c = getopt(argc, argv,"a:bc:e:s:p:q:x:d:?h")) !=EOF)
#endif /* #if 0 */
			switch (c) {
				case 'b':
					background=1;
					break;
				case 'p':	/* payload type: routed (0) or bridged (1) */
					payload = atoi(optarg);
					break;
#ifdef TP_BR2684CTL_MODIFY	/* wzy: modify at 25Apr12 */
				case 'x':
					ext_add_flag = 1;
					ext_add_infnum = atoi(optarg);
					break;
				case 'd':
					ext_del_flag = 1;
					ext_del_infnum = atoi(optarg);
					break;
				case 'c':
					create_br(optarg, 
								payload,
								(ext_add_flag==1?ext_add_infnum:0),
								(ext_del_flag==1?ext_del_infnum:0));
					break;
#else
				case 'c':
					create_br(optarg, payload);
					break;
#endif /* #if 0 */
				case 'e':
					encap=(atoi(optarg));
					if(encap<0){
						syslog (LOG_ERR, "invalid encapsulation: %s:\n",optarg);
						encap=0;
					}
					break;
				case 's':
					sndbuf=(atoi(optarg));
					if(sndbuf<0){
						syslog(LOG_ERR, "Invalid sndbuf: %s, using size of 8192 instead\n",optarg);
						sndbuf=8192;
					}
					break;
				case 'q':
					printf ("optarg : %s",optarg);
					if (text2qos(optarg,&reqqos,0)) 
						fprintf(stderr,"QOS parameter invalid\n");
					break;
				case 'a':
#ifdef TP_BR2684CTL_MODIFY	/* wzy: modify at 25Apr12 */
					if (ext_add_flag || ext_del_flag)
					{
						break;	/* 如果是添加或者删除扩展节点, 则不需要重新设置ATM Params */
					}
					else
					{
						fd = assign_vcc(optarg, encap, payload, sndbuf, reqqos);
						break;
					}
#else
					fd = assign_vcc(optarg, encap, payload,sndbuf, reqqos);
					break;
#endif /* #if 0 */
				case '?':
				case 'h':
				default:
					usage(argv[0]);
			}
		}
	}
	else
		usage(argv[0]);

	if (argc != optind) 
		usage(argv[0]);

	if(lastsock>=0) 
		close(lastsock);

	return fd;
}

#endif 


int ifx_get_process_pid(char *file_prefix)
{
	int process_id=-1;
	char file_name[IFX_MAX_PID_FILENAME];
	FILE *fp = NULL;

	sprintf(file_name,"%s/%s.pid",IFX_PID_FILE_PATH, file_prefix);	
	fp = fopen(file_name,"r");
	if (fp == NULL) {
		syslog(LOG_INFO, "Error : Couldn't open file [%s] in read mode!",file_name);
		return (process_id);
	} else {
		if (fscanf(fp,"%d",&process_id) < 0) {
			process_id=-1; /* Error */
		}
		fclose(fp);
	}
	return process_id;
}
int ifx_rm_pid_file(char *file_prefix)
{
	char file_name[IFX_MAX_PID_FILENAME];


	sprintf(file_name,"%s/%s.pid",IFX_PID_FILE_PATH, file_prefix);	

	return (unlink(file_name));
}

int ifx_create_pid_file(char *file_prefix)
{
	int process_id = 0;
	FILE *fp;
	char file_name[IFX_MAX_PID_FILENAME];

	process_id = getpid();

	sprintf(file_name,"%s/%s.pid",IFX_PID_FILE_PATH, file_prefix);	
	fp = fopen(file_name,"w");
	if (fp == NULL) {
		syslog(LOG_INFO, "Error : Couldn't open file [%s] in write mode!",file_name);
		return -1;
	} else {
		fprintf(fp,"%d",process_id);
		fclose(fp);
	}
	strcpy(g_ifx_pidfile_prefix, file_prefix);
	atexit(ifx_rm_pid_file_atexit);
	return 0;
}

void ifx_rm_pid_file_atexit(void)
{
	ifx_rm_pid_file(g_ifx_pidfile_prefix);
}

