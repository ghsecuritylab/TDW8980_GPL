/*******************************************************************************
**
** FILE NAME    : ifx_ppa_api_misc.c
** PROJECT      : PPA
** MODULES      : PPA API (Routing/Bridging Acceleration APIs)
**
** DATE         : 3 NOV 2008
** AUTHOR       : Xu Liang
** DESCRIPTION  : PPA Protocol Stack Hook API Miscellaneous Functions
** COPYRIGHT    :              Copyright (c) 2009
**                          Lantiq Deutschland GmbH
**                   Am Campeon 3; 85579 Neubiberg, Germany
**
**   For licensing information, see the file 'LICENSE' in the root folder of
**   this software module.
**
** HISTORY
** $Date        $Author         $Comment
** 03 NOV 2008  Xu Liang        Initiate Version
*******************************************************************************/



/*
 * ####################################
 *              Head File
 * ####################################
 */

/*
 *  Common Head File
 */
#include <linux/autoconf.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/slab.h>
#if defined(CONFIG_IFX_PPA_API_PROC)
#include <linux/proc_fs.h>
#endif
#include <linux/netdevice.h>
#include <linux/in.h>
#include <net/sock.h>
#include <net/ip_vs.h>
#include <asm/time.h>

/*
 *  PPA Specific Head File
 */
#include <net/ifx_ppa_api.h>
#include <net/ifx_ppa_ppe_hal.h>
#include "ifx_ppa_api_misc.h"
#include "ifx_ppa_api_session.h"
#include "ifx_ppa_api_netif.h"
#if defined(CONFIG_IFX_PPA_API_PROC)
#include "ifx_ppa_api_proc.h"
#endif
#include "ifx_ppa_api_tools.h"
#ifdef CONFIG_IFX_PPA_MFE
#include "ifx_ppa_api_mfe.h"
#endif
#ifdef CONFIG_IFX_PPA_QOS
#include "ifx_ppa_api_qos.h"
#endif
#include "ifx_ppa_api_mib.h"
#include "ifx_ppe_drv_wrapper.h"

/*
 * ####################################
 *              Definition
 * ####################################
 */

/*
 *  device constant
 */
#define PPA_CHR_DEV_MAJOR                       181
#define PPA_DEV_NAME                            "ifx_ppa"



/*
 * ####################################
 *              Data Type
 * ####################################
 */



/*
 * ####################################
 *             Declaration
 * ####################################
 */

static int ppa_dev_open(struct inode *, struct file *);
static int ppa_dev_release(struct inode *, struct file *);
static int ppa_dev_ioctl(struct inode *, struct file *, unsigned int, unsigned long);

static int32_t ppa_get_routing_session_count(uint16_t, uint32_t);
static int32_t ppa_get_mc_group_count(uint32_t);
static int32_t ppa_get_all_vlan_filter_count(uint32_t);
static int32_t ppa_get_br_count(uint32_t);

#if defined(CONFIG_IFX_PPA_API_PROC)
static int print_fw_ver(char *, int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int);
static int print_ppa_subsystem_ver(char *, int, char *, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int);
static int print_driver_ver(char *, int, char *, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int);

static INLINE void proc_file_create(void);
static INLINE void proc_file_delete(void);
static int proc_read_ver(char *, char **, off_t, int, int *, void *);
#endif

#if defined(CONFIG_IFX_PPA_API_DIRECTPATH)
  int32_t ppa_api_directpath_init(void);
  void ppa_api_directpath_exit(void);
#endif

int32_t ppa_ioctl_bridge_enable(unsigned int cmd, unsigned long arg, PPA_CMD_DATA * cmd_info);
int32_t ppa_ioctl_get_bridge_enable_status(unsigned int cmd, unsigned long arg, PPA_CMD_DATA * cmd_info);
int32_t ppa_ioctl_get_hook_list(unsigned int cmd, unsigned long arg, PPA_CMD_DATA * cmd_info);
int32_t ppa_ioctl_set_hook(unsigned int cmd, unsigned long arg, PPA_CMD_DATA * cmd_info);
int32_t ppa_ioctl_get_max_entry(unsigned int cmd, unsigned long arg, PPA_CMD_DATA *cmd_info);
int32_t ppa_ioctl_get_portid(unsigned int cmd, unsigned long arg, PPA_CMD_DATA *cmd_info);
int32_t ppa_ioctl_del_session(unsigned int cmd, unsigned long arg, PPA_CMD_DATA *cmd_info);
int32_t ppa_ioctl_add_session(unsigned int cmd, unsigned long arg, PPA_CMD_DATA *cmd_info);
int32_t ppa_ioctl_modify_session(unsigned int cmd, unsigned long arg, PPA_CMD_DATA *cmd_info);
int32_t ppa_ioctl_set_session_timer(unsigned int cmd, unsigned long arg, PPA_CMD_DATA *cmd_info);
int32_t ppa_ioctl_get_session_timer(unsigned int cmd, unsigned long arg, PPA_CMD_DATA *cmd_info);


/*
 * ####################################
 *           Global Variable
 * ####################################
 */

#if defined(CONFIG_IFX_PPA_API_PROC)
static int g_ppa_proc_dir_flag = 0;
static int g_ppa_api_proc_dir_flag = 0;
static struct proc_dir_entry *g_ppa_proc_dir = NULL;
static struct proc_dir_entry *g_ppa_api_proc_dir = NULL;
#endif 


static struct file_operations g_ppa_dev_ops = {
    owner:      THIS_MODULE,
    ioctl:      ppa_dev_ioctl,
    open:       ppa_dev_open,
    release:    ppa_dev_release
};

static char ZeroMAC[]={0,0,0,0,0,0};


/*
 * ####################################
 *           Extern Variable
 * ####################################
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)
    extern struct proc_dir_entry proc_root;
#endif



/*
 * ####################################
 *            Local Function
 * ####################################
 */

/*
 *  file operation functions
 */

static int ppa_dev_open(struct inode *inode, struct file *file)
{
    return 0;
}

static int ppa_dev_release(struct inode *inode, struct file *file)
{
    return 0;
}

static int ppa_dev_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
    int res = 0;
    PPA_CMD_DATA *cmd_info = (PPA_CMD_DATA *)ppa_malloc(sizeof(PPA_CMD_DATA));

    if ( cmd_info == NULL )
        return -EFAULT;

    if ( ppa_ioc_type(cmd) != PPA_IOC_MAGIC )
    {
        printk("ppa_ioc_type(%08X - %d) != PPA_IOC_MAGIC(%d)\n", cmd, _IOC_TYPE(cmd), PPA_IOC_MAGIC);
        goto EXIT_EIO;
    }
    else if( ppa_ioc_nr(cmd) >= PPA_IOC_MAXNR )
    {
        printk("Current cmd is %02x wrong, it should less than %02x\n", _IOC_NR(cmd), PPA_IOC_MAXNR );
        goto EXIT_EIO;
    }

    if ( ((ppa_ioc_dir(cmd) & ppa_ioc_read()) && !access_ok(VERIFY_WRITE, arg, ppa_ioc_size(cmd)))
        || ((ppa_ioc_dir(cmd) & ppa_ioc_write()) && !access_ok(VERIFY_READ, arg, ppa_ioc_size(cmd))) )
    {
        printk("access check: (%08X && %d) || (%08X && %d)\n", (ppa_ioc_dir(cmd) & ppa_ioc_read()), (int)!ppa_ioc_access_ok(ppa_ioc_verify_write(), arg, ppa_ioc_size(cmd)),
                                                               (ppa_ioc_dir(cmd) & ppa_ioc_write()), (int)!ppa_ioc_access_ok(ppa_ioc_verify_read(), arg, ppa_ioc_size(cmd)));
        goto EXIT_EFAULT;
    }

    switch ( cmd )
    {
    case PPA_CMD_INIT:
        {
            PPA_INIT_INFO info;
            PPA_IFINFO ifinfo[ 2 * PPA_MAX_IFS_NUM ];
            int i;

            if ( ppa_copy_from_user(&cmd_info->init_info, (void *)arg, sizeof(cmd_info->init_info)) != 0 )
                goto EXIT_EFAULT;

            ppa_memset(&info, 0, sizeof(info));
            ppa_memset(ifinfo, 0, sizeof(ifinfo));

            info.lan_rx_checks          = cmd_info->init_info.lan_rx_checks;
            info.wan_rx_checks          = cmd_info->init_info.wan_rx_checks;
            info.num_lanifs             = cmd_info->init_info.num_lanifs;
            info.p_lanifs               = ifinfo;
            info.num_wanifs             = cmd_info->init_info.num_wanifs;
            info.p_wanifs               = ifinfo + NUM_ENTITY(cmd_info->init_info.p_lanifs);
            info.max_lan_source_entries = cmd_info->init_info.max_lan_source_entries;
            info.max_wan_source_entries = cmd_info->init_info.max_wan_source_entries;
            info.max_mc_entries         = cmd_info->init_info.max_mc_entries;
            info.max_bridging_entries   = cmd_info->init_info.max_bridging_entries;
            info.add_requires_min_hits  = cmd_info->init_info.add_requires_min_hits;

            for ( i = 0; i < info.num_lanifs; i++ )
            {
                info.p_lanifs[i].ifname   = cmd_info->init_info.p_lanifs[i].ifname;
                info.p_lanifs[i].if_flags = cmd_info->init_info.p_lanifs[i].if_flags;
            }

            for ( i = 0; i < info.num_wanifs; i++ )
            {
                info.p_wanifs[i].ifname   = cmd_info->init_info.p_wanifs[i].ifname;
                info.p_wanifs[i].if_flags = cmd_info->init_info.p_wanifs[i].if_flags;
            }

            if ( ifx_ppa_init(&info, cmd_info->init_info.flags) != IFX_SUCCESS )
            {
                dbg("ifx_ppa_init fail");
                goto EXIT_EIO;
            }
            else
                goto EXIT_ZERO;
        }

    case PPA_CMD_EXIT:
        {
            ifx_ppa_exit();
            goto EXIT_ZERO;
        }

    case PPA_CMD_ENABLE:
        {
            if ( ppa_copy_from_user(&cmd_info->ena_info, (void *)arg, sizeof(cmd_info->ena_info)) != 0 )
                goto EXIT_EFAULT;

            if ( ifx_ppa_enable(cmd_info->ena_info.lan_rx_ppa_enable, cmd_info->ena_info.wan_rx_ppa_enable, cmd_info->ena_info.flags) != IFX_SUCCESS )
                goto EXIT_EIO;
            else
                goto EXIT_ZERO;
        }

    case PPA_CMD_GET_STATUS:
        {
            cmd_info->ena_info.lan_rx_ppa_enable = 0;
            cmd_info->ena_info.wan_rx_ppa_enable = 0;
                
            if ( ppa_copy_from_user(&cmd_info->ena_info, (void *)arg, sizeof(cmd_info->ena_info)) != 0 )
                goto EXIT_EFAULT;

            if ( ifx_ppa_get_status(&cmd_info->ena_info.lan_rx_ppa_enable, &cmd_info->ena_info.wan_rx_ppa_enable, cmd_info->ena_info.flags) != IFX_SUCCESS )
                  cmd_info->ena_info.flags = 0;
            else
                 cmd_info->ena_info.flags = 1;
            if ( ppa_copy_to_user((void *)arg, &cmd_info->ena_info, sizeof(cmd_info->ena_info)) != 0 )
                goto EXIT_EFAULT;
            else
                goto EXIT_ZERO;
        }

    case PPA_CMD_MODIFY_MC_ENTRY:
        {
            if ( ppa_copy_from_user(&cmd_info->mc_entry, (void *)arg, sizeof(cmd_info->mc_entry)) != 0 )
                goto EXIT_EFAULT;

            if ( ifx_ppa_mc_entry_modify(cmd_info->mc_entry.mcast_addr, NULL, &cmd_info->mc_entry.mc_extra, cmd_info->mc_entry.flags) != IFX_SUCCESS )
                goto EXIT_EIO;
            else
                goto EXIT_ZERO;
        }

    case PPA_CMD_GET_MC_ENTRY:
        {
            if ( ppa_copy_from_user(&cmd_info->mc_entry, (void *)arg, sizeof(cmd_info->mc_entry)) != 0 )
                goto EXIT_EFAULT;

            ppa_memset(&cmd_info->mc_entry.mc_extra, 0, sizeof(cmd_info->mc_entry.mc_extra));

            if ( ifx_ppa_mc_entry_get(cmd_info->mc_entry.mcast_addr, &cmd_info->mc_entry.mc_extra, -1) != IFX_SUCCESS ) //-1: to get all flag related parameters
                goto EXIT_EIO;
            else if ( ppa_copy_to_user((void *)arg, &cmd_info->mc_entry, sizeof(cmd_info->mc_entry)) != 0 )
                goto EXIT_EFAULT;
            else
                goto EXIT_ZERO;
        }

    case PPA_CMD_ADD_MAC_ENTRY:
        {
            PPA_NETIF *netif;

            if ( ppa_copy_from_user(&cmd_info->mac_entry, (void *)arg, sizeof(cmd_info->mac_entry)) != 0 )
                goto EXIT_EFAULT;

            netif = ppa_get_netif(cmd_info->mac_entry.ifname);
            if ( !netif )
                goto EXIT_EIO;

            if ( ifx_ppa_bridge_entry_add(cmd_info->mac_entry.mac_addr, netif, cmd_info->mac_entry.flags) != IFX_PPA_SESSION_ADDED )
                goto EXIT_EIO;
            else
                goto EXIT_ZERO;
        }

    case PPA_CMD_DEL_MAC_ENTRY:
        {
            if ( ppa_copy_from_user(&cmd_info->mac_entry, (void *)arg, sizeof(cmd_info->mac_entry)) != 0 )
                goto EXIT_EFAULT;

            if ( ifx_ppa_bridge_entry_delete(cmd_info->mac_entry.mac_addr, cmd_info->mac_entry.flags) != IFX_SUCCESS )
                goto EXIT_EIO;
            else
                goto EXIT_ZERO;
        }

    case PPA_CMD_SET_VLAN_IF_CFG:
        {
            PPA_NETIF *netif;

            if ( ppa_copy_from_user(&cmd_info->br_vlan, (void *)arg, sizeof(cmd_info->br_vlan)) != 0 )
                goto EXIT_EFAULT;

            netif = ppa_get_netif(cmd_info->br_vlan.if_name);
            if ( !netif )
                goto EXIT_EIO;

            if ( ifx_ppa_set_bridge_if_vlan_config(netif, &cmd_info->br_vlan.vlan_tag_ctrl, &cmd_info->br_vlan.vlan_cfg, cmd_info->br_vlan.flags) != IFX_SUCCESS )
                goto EXIT_EIO;
            else
                goto EXIT_ZERO;
        }

    case PPA_CMD_GET_VLAN_IF_CFG:
        {
            PPA_NETIF *netif;

            if ( ppa_copy_from_user(&cmd_info->br_vlan, (void *)arg, sizeof(cmd_info->br_vlan)) != 0 )
                goto EXIT_EFAULT;

            netif = ppa_get_netif(cmd_info->br_vlan.if_name);
            if ( !netif )
                goto EXIT_EIO;

            ppa_memset(&cmd_info->br_vlan.vlan_tag_ctrl, 0, sizeof(cmd_info->br_vlan.vlan_tag_ctrl));
            ppa_memset(&cmd_info->br_vlan.vlan_cfg, 0, sizeof(cmd_info->br_vlan.vlan_cfg));

            if ( ifx_ppa_get_bridge_if_vlan_config(netif, &cmd_info->br_vlan.vlan_tag_ctrl, &cmd_info->br_vlan.vlan_cfg, cmd_info->br_vlan.flags) != IFX_SUCCESS )
                goto EXIT_EIO;
            else if ( ppa_copy_to_user((void *)arg, &cmd_info->br_vlan, sizeof(cmd_info->br_vlan)) != 0 )
               goto EXIT_EFAULT;
            else
                goto EXIT_ZERO;
        }

    case PPA_CMD_ADD_VLAN_FILTER_CFG:
        {
            PPA_VLAN_MATCH_FIELD field;
            PPA_VLAN_INFO info;
            PPA_IFINFO ifinfo[PPA_MAX_IFS_NUM];
            struct netif_info *netifinfo;
            int i;

            if ( ppa_copy_from_user(&cmd_info->vlan_filter, (void *)arg, sizeof(cmd_info->vlan_filter)) != 0 )
                goto EXIT_EFAULT;

            field.match_flags = cmd_info->vlan_filter.vlan_filter_cfg.match_field.match_flags;
            switch ( field.match_flags )
            {
            case PPA_F_VLAN_FILTER_IFNAME:      field.match_field.ifname = cmd_info->vlan_filter.vlan_filter_cfg.match_field.match_field.ifname; break;
            case PPA_F_VLAN_FILTER_IP_SRC:      field.match_field.ip_src = cmd_info->vlan_filter.vlan_filter_cfg.match_field.match_field.ip_src; break;
            case PPA_F_VLAN_FILTER_ETH_PROTO:   field.match_field.eth_protocol = cmd_info->vlan_filter.vlan_filter_cfg.match_field.match_field.eth_protocol; break;
            case PPA_F_VLAN_FILTER_VLAN_TAG:    field.match_field.ingress_vlan_tag = cmd_info->vlan_filter.vlan_filter_cfg.match_field.match_field.ingress_vlan_tag; break;
            default: goto EXIT_EINVAL;
            }

            info.vlan_vci = cmd_info->vlan_filter.vlan_filter_cfg.vlan_info.vlan_vci;
            info.out_vlan_id = cmd_info->vlan_filter.vlan_filter_cfg.vlan_info.out_vlan_id;
            info.qid = cmd_info->vlan_filter.vlan_filter_cfg.vlan_info.qid;
            info.inner_vlan_tag_ctrl = cmd_info->vlan_filter.vlan_filter_cfg.vlan_info.inner_vlan_tag_ctrl;
            info.out_vlan_tag_ctrl = cmd_info->vlan_filter.vlan_filter_cfg.vlan_info.out_vlan_tag_ctrl;
            info.num_ifs = cmd_info->vlan_filter.vlan_filter_cfg.vlan_info.num_ifs;
            info.vlan_if_membership = ifinfo;

            for ( i = 0; i < info.num_ifs; i++ )
            {
                if ( ppa_netif_lookup(cmd_info->vlan_filter.vlan_filter_cfg.vlan_info.vlan_if_membership[i].ifname, &netifinfo) != IFX_SUCCESS )
                {
                    dbg("%s is not a valid interfce", cmd_info->vlan_filter.vlan_filter_cfg.vlan_info.vlan_if_membership[i].ifname);
                    goto EXIT_EIO;
                }
                if( info.qid == PPA_INVALID_QID )
                {
                    info.qid = netifinfo->dslwan_qid;
                }
                ppa_netif_put(netifinfo);

                ifinfo[i].ifname = cmd_info->vlan_filter.vlan_filter_cfg.vlan_info.vlan_if_membership[i].ifname;
                ifinfo[i].if_flags = cmd_info->vlan_filter.vlan_filter_cfg.vlan_info.vlan_if_membership[i].if_flags;

            }

            if ( ifx_ppa_vlan_filter_add(&field, &info, cmd_info->vlan_filter.flags) != IFX_SUCCESS )
            {
                dbg("ifx_ppa_vlan_filter_add fail");
                goto EXIT_EIO;
            }
            else
                goto EXIT_ZERO;
        }

    case PPA_CMD_DEL_VLAN_FILTER_CFG:
        {
            PPA_VLAN_MATCH_FIELD field;

            if ( ppa_copy_from_user(&cmd_info->vlan_filter, (void *)arg, sizeof(cmd_info->vlan_filter)) != 0 )
                goto EXIT_EFAULT;

            field.match_flags = cmd_info->vlan_filter.vlan_filter_cfg.match_field.match_flags;
            switch ( field.match_flags )
            {
            case PPA_F_VLAN_FILTER_IFNAME:      field.match_field.ifname = cmd_info->vlan_filter.vlan_filter_cfg.match_field.match_field.ifname; break;
            case PPA_F_VLAN_FILTER_IP_SRC:      field.match_field.ip_src = cmd_info->vlan_filter.vlan_filter_cfg.match_field.match_field.ip_src; break;
            case PPA_F_VLAN_FILTER_ETH_PROTO:   field.match_field.eth_protocol = cmd_info->vlan_filter.vlan_filter_cfg.match_field.match_field.eth_protocol; break;
            case PPA_F_VLAN_FILTER_VLAN_TAG:    field.match_field.ingress_vlan_tag = cmd_info->vlan_filter.vlan_filter_cfg.match_field.match_field.ingress_vlan_tag; break;
            default: goto EXIT_EINVAL;
            }

            if ( ifx_ppa_vlan_filter_del(&field, NULL, cmd_info->vlan_filter.flags) != IFX_SUCCESS )
                goto EXIT_EIO;
            else
                goto EXIT_ZERO;
        }

    case PPA_CMD_GET_ALL_VLAN_FILTER_CFG:
        {
            uint32_t if_num, i, j, k, count;
            uint32_t vlan_filter_type[]={PPA_F_VLAN_FILTER_IFNAME, PPA_F_VLAN_FILTER_IP_SRC, PPA_F_VLAN_FILTER_ETH_PROTO, PPA_F_VLAN_FILTER_VLAN_TAG};
            PPA_IFNAME *PhysicalPortName[PPA_MAX_IFS_NUM]={"eth0", "wan","CPU0", "EXT0", "EXT1", "EXT2", "EXT3", "EXT4"};
            PPE_BRDG_VLAN_FILTER_MAP filter_map={0};
                
            ppa_memset( &cmd_info->all_vlan_filter, 0, sizeof(cmd_info->all_vlan_filter) );
             if ( ppa_copy_from_user(&cmd_info->all_vlan_filter.count_info, (void *)arg, sizeof(cmd_info->all_vlan_filter.count_info)) != 0 )
                goto EXIT_EFAULT;

            if( cmd_info->all_vlan_filter.count_info.count == 0 )
                goto EXIT_ZERO;

            count = 0;
            for(i=0; i<sizeof(vlan_filter_type)/sizeof(vlan_filter_type[0]); i++ )
            {
                PPE_VFILTER_COUNT_CFG vfilter_count={0};

                vfilter_count.vfitler_type = vlan_filter_type[i];
                ifx_ppa_drv_get_max_vfilter_entries( &vfilter_count, 0);

                for(j=0; j<vfilter_count.num; j++ )
                {                     
                    ppa_memset( &filter_map, 0, sizeof(filter_map));
                    ppa_memset( cmd_info->all_vlan_filter.filters, 0, sizeof(cmd_info->all_vlan_filter.filters) );

                     filter_map.ig_criteria_type = vlan_filter_type[i];
                     filter_map.entry = j;
                     
                     if( ifx_ppa_drv_get_vlan_map(&filter_map, 0 ) == 1 )
                    {                        
                        if( ifx_ppa_drv_get_outer_vlan_entry(&filter_map.out_vlan_info, 0) == IFX_SUCCESS )
                        {
                            cmd_info->all_vlan_filter.filters[0].vlan_filter_cfg.vlan_info.out_vlan_id = filter_map.out_vlan_info.vlan_id;
                        }

                        cmd_info->all_vlan_filter.filters[0].vlan_filter_cfg.match_field.match_flags = vlan_filter_type[i];
                        if( vlan_filter_type[i] == PPA_F_VLAN_FILTER_IFNAME )
                        {
                            if( filter_map.ig_criteria >= PPA_MAX_IFS_NUM )
                            {
                                printk("Why ig_criteria (%d) big than %d for PPA_CMD_GET_ALL_VLAN_FILTER_CFG\n", filter_map.ig_criteria, PPA_MAX_IFS_NUM );
                                continue;
                            }
                            if( filter_map.ig_criteria < PPA_MAX_IFS_NUM )
                                ppa_strncpy( cmd_info->all_vlan_filter.filters[0].vlan_filter_cfg.match_field.match_field.ifname, 
                                                        PhysicalPortName[ filter_map.ig_criteria], 
                                                        sizeof(cmd_info->all_vlan_filter.filters[0].vlan_filter_cfg.match_field.match_field.ifname) );
                            else
                                filter_map.ig_criteria = 0;
                        }
                        else if( vlan_filter_type[i] == PPA_F_VLAN_FILTER_IP_SRC )
                            cmd_info->all_vlan_filter.filters[0].vlan_filter_cfg.match_field.match_field.ip_src = filter_map.ig_criteria;
                        else if( vlan_filter_type[i] == PPA_F_VLAN_FILTER_ETH_PROTO)
                            cmd_info->all_vlan_filter.filters[0].vlan_filter_cfg.match_field.match_field.eth_protocol = filter_map.ig_criteria;
                        else if( vlan_filter_type[i] == PPA_F_VLAN_FILTER_VLAN_TAG)
                            cmd_info->all_vlan_filter.filters[0].vlan_filter_cfg.match_field.match_field.ingress_vlan_tag = filter_map.ig_criteria;
                        else
                        {
                            printk("unknown type: %x\n", vlan_filter_type[i]  );
                            break;
                        }

                         //Since PPA dont' save vlan filter's original network interface name, so here we have to use faked one
                        if_num = 0;
                        for(k=0; k<PPA_MAX_IFS_NUM; k++ )
                        {
                            if( filter_map.vlan_port_map & (1 << k) )
                            {
                                ppa_strncpy( cmd_info->all_vlan_filter.filters[0].vlan_filter_cfg.vlan_info.vlan_if_membership[if_num].ifname, PhysicalPortName[k], sizeof(cmd_info->all_vlan_filter.filters[0].vlan_filter_cfg.vlan_info.vlan_if_membership[if_num].ifname) );
                                if_num ++;
                            }
                        }

                        cmd_info->all_vlan_filter.filters[0].vlan_filter_cfg.vlan_info.vlan_vci = filter_map.new_vci;

                        cmd_info->all_vlan_filter.filters[0].vlan_filter_cfg.vlan_info.out_vlan_tag_ctrl = filter_map.in_out_etag_ctrl;
                        cmd_info->all_vlan_filter.filters[0].vlan_filter_cfg.vlan_info.inner_vlan_tag_ctrl = filter_map.in_out_etag_ctrl;
                        cmd_info->all_vlan_filter.filters[0].vlan_filter_cfg.vlan_info.qid = filter_map.dest_qos;
                        cmd_info->all_vlan_filter.filters[0].vlan_filter_cfg.vlan_info.num_ifs = if_num;

                        if ( ppa_copy_to_user(  (void *)(arg + (void *)cmd_info->all_vlan_filter.filters - (void *)&cmd_info->all_vlan_filter.count_info + count * sizeof(cmd_info->all_vlan_filter.filters[0])) , cmd_info->all_vlan_filter.filters,  sizeof(cmd_info->all_vlan_filter.filters[0])) != 0 )
                            goto EXIT_EFAULT;

                        count ++;

                        if( count == cmd_info->all_vlan_filter.count_info.count  )
                            break;
                    }
                }
            }

            cmd_info->all_vlan_filter.count_info.count = count;  //update the real session number to user space

            if ( ppa_copy_to_user(  (void *)arg, &cmd_info->all_vlan_filter.count_info, sizeof(cmd_info->all_vlan_filter.count_info)) != 0 )
                 goto EXIT_EFAULT;

            goto EXIT_ZERO;
        }

    case PPA_CMD_DEL_ALL_VLAN_FILTER_CFG:
        {
            ifx_ppa_vlan_filter_del_all(0);
            goto EXIT_ZERO;
        }

    case PPA_CMD_SET_IF_MAC:
        {
            if ( ppa_copy_from_user(&cmd_info->if_mac, (void *)arg, sizeof(cmd_info->if_mac)) != 0 )
                goto EXIT_EFAULT;

            if ( ifx_ppa_set_if_mac_address(cmd_info->if_mac.ifname, cmd_info->if_mac.mac, cmd_info->if_mac.flags) != IFX_SUCCESS )
                goto EXIT_EIO;
            else
                goto EXIT_ZERO;
        }

    case PPA_CMD_GET_IF_MAC:
        {
            if ( ppa_copy_from_user(&cmd_info->if_mac, (void *)arg, sizeof(cmd_info->if_mac)) != 0 )
                goto EXIT_EFAULT;

            ppa_memset(cmd_info->if_mac.mac, 0, sizeof(cmd_info->if_mac.mac));

            if ( ifx_ppa_get_if_mac_address(cmd_info->if_mac.ifname, cmd_info->if_mac.mac, cmd_info->if_mac.flags) != IFX_SUCCESS )
                goto EXIT_EIO;
            else if ( ppa_copy_to_user((void *)arg, &cmd_info->if_mac, sizeof(cmd_info->if_mac)) != 0 )
                goto EXIT_EFAULT;
            else
                goto EXIT_ZERO;
        }

    case PPA_CMD_ADD_LAN_IF:
    case PPA_CMD_ADD_WAN_IF:
        {
            PPA_IFINFO ifinfo = {0};

            if ( ppa_copy_from_user(&cmd_info->if_info, (void *)arg, sizeof(cmd_info->if_info)) != 0 )
                goto EXIT_EFAULT;

            ifinfo.ifname = cmd_info->if_info.ifname;
            if ( cmd == PPA_CMD_ADD_LAN_IF )
                ifinfo.if_flags = PPA_F_LAN_IF;
            if ( ifx_ppa_add_if(&ifinfo, 0) != IFX_SUCCESS )
                goto EXIT_EIO;
            else
                goto EXIT_ZERO;
        }

    case PPA_CMD_DEL_LAN_IF:
    case PPA_CMD_DEL_WAN_IF:
        {
            PPA_IFINFO ifinfo = {0};

            if ( ppa_copy_from_user(&cmd_info->if_info, (void *)arg, sizeof(cmd_info->if_info)) != 0 )
                goto EXIT_EFAULT;

            ifinfo.ifname = cmd_info->if_info.ifname;
            if ( cmd == PPA_CMD_DEL_LAN_IF )
                ifinfo.if_flags = PPA_F_LAN_IF;
            if ( ifx_ppa_del_if(&ifinfo, 0) != IFX_SUCCESS )
                goto EXIT_EIO;
            else
                goto EXIT_ZERO;
        }

    case PPA_CMD_GET_LAN_IF:
    case PPA_CMD_GET_WAN_IF:
        {
            struct netif_info *ifinfo;
            uint32_t pos = 0;
            int i = 0;
            uint32_t flag_mask = ( cmd == PPA_CMD_GET_LAN_IF)  ? NETIF_LAN_IF : NETIF_WAN_IF;
            uint32_t if_flags = (cmd == PPA_CMD_GET_LAN_IF) ? PPA_F_LAN_IF : 0;

            ppa_memset(&cmd_info->all_if_info, 0, sizeof(cmd_info->all_if_info));

            if ( ppa_netif_start_iteration(&pos, &ifinfo) == IFX_SUCCESS )
            {
                do
                {
                    if ( (ifinfo->flags & flag_mask) )
                    {
                        strcpy(cmd_info->all_if_info.ifinfo[i].ifname, ifinfo->name);
                        cmd_info->all_if_info.ifinfo[i].if_flags = if_flags;

                        if ( ++i >= NUM_ENTITY(cmd_info->all_if_info.ifinfo) )
                            break;
                    }
                } while ( ppa_netif_iterate_next(&pos, &ifinfo) == IFX_SUCCESS );
            }

            ppa_netif_stop_iteration();

            cmd_info->all_if_info.num_ifinfos = i;

            if ( ppa_copy_to_user((void *)arg, &cmd_info->all_if_info, sizeof(cmd_info->all_if_info)) != 0 )
                goto EXIT_EFAULT;
            else
                goto EXIT_ZERO;
        }

    case PPA_CMD_ADD_MC:
        {
            PPA_MC_GROUP mc_group;
            int32_t res;
            int i, idx = 0;

            ppa_memset( &mc_group, 0, sizeof(mc_group));
            if ( ppa_copy_from_user(&cmd_info->mc_add_info, (void *)arg, sizeof(cmd_info->mc_add_info)) != 0 )
                goto EXIT_EFAULT;

            if ( cmd_info->mc_add_info.num_ifs > PPA_MAX_MC_IFS_NUM )
            {
                /*  if num_ifs is zero, it means to remove the multicast session from PPE HW    */
                printk("Error cmd_info->mc_add_info.num_ifs=%d is over max limit %d", cmd_info->mc_add_info.num_ifs, PPA_MAX_MC_IFS_NUM);
                goto EXIT_EIO;
            }

            for ( i = 0, idx = 0; i < cmd_info->mc_add_info.num_ifs; i++ )
            {
                mc_group.array_mem_ifs[i].ifname = cmd_info->mc_add_info.lan_ifname[i];  //  downstream interfce: 1

                mc_group.if_mask |= 1 << idx;
                idx++;
            }
            mc_group.num_ifs = idx; //  downstream interfce number

            if ( ppa_strlen(cmd_info->mc_add_info.src_ifname) )                              //  upstream interfce: 2
            {
                mc_group.src_ifname = cmd_info->mc_add_info.src_ifname;
            }

            ppa_memcpy( &mc_group.ip_mc_group, &cmd_info->mc_add_info.mc.mcast_addr, sizeof(mc_group.ip_mc_group) ) ;            //  multicast address: 3
            ppa_memcpy( &mc_group.source_ip, &cmd_info->mc_add_info.mc.source_ip, sizeof(mc_group.source_ip));
            mc_group.SSM_flag = cmd_info->mc_add_info.mc.SSM_flag ;
            mc_group.bridging_flag = cmd_info->mc_add_info.bridging_flag;                //  multicast mode: 4
            if( ppa_memcmp( cmd_info->mc_add_info.mac, ZeroMAC, PPA_ETH_ALEN ) != 0 )
                ppa_memcpy(mc_group.mac, cmd_info->mc_add_info.mac, PPA_ETH_ALEN);               //  multicast new mac address: 5. Note, it is not used at present. only for future

            if ( cmd_info->mc_add_info.mc.flags & PPA_F_SESSION_VLAN )                                  //  flag for modifying inner vlan
            {
                mc_group.vlan_insert        = cmd_info->mc_add_info.mc.mc_extra.vlan_insert;
                mc_group.vlan_remove        = cmd_info->mc_add_info.mc.mc_extra.vlan_remove;
                mc_group.vlan_prio          = cmd_info->mc_add_info.mc.mc_extra.vlan_prio;
                mc_group.vlan_cfi           = cmd_info->mc_add_info.mc.mc_extra.vlan_cfi;
                mc_group.vlan_id            = cmd_info->mc_add_info.mc.mc_extra.vlan_id;
            }

            if ( cmd_info->mc_add_info.mc.flags & PPA_F_SESSION_OUT_VLAN )                                  //  flag for modifying outer vlan
            {
                mc_group.out_vlan_insert    = cmd_info->mc_add_info.mc.mc_extra.out_vlan_insert;
                mc_group.out_vlan_remove    = cmd_info->mc_add_info.mc.mc_extra.out_vlan_remove;
                mc_group.out_vlan_tag       = cmd_info->mc_add_info.mc.mc_extra.out_vlan_tag;
            }

            if ( cmd_info->mc_add_info.mc.flags & PPA_F_SESSION_NEW_DSCP )                                  //  flag for modifying dscp
            {
                mc_group.new_dscp_en        = cmd_info->mc_add_info.new_dscp_en;
                mc_group.new_dscp           = cmd_info->mc_add_info.mc.mc_extra.new_dscp;
            }

            mc_group.dslwan_qid_remark = cmd_info->mc_add_info.mc.mc_extra.dslwan_qid_remark;
            mc_group.dslwan_qid = cmd_info->mc_add_info.mc.mc_extra.dslwan_qid;

            res = ifx_ppa_mc_group_update( &mc_group, cmd_info->mc_add_info.mc.flags);

            if ( res != IFX_SUCCESS )
            {
                dbg("ifx_ppa_mc_group_update fail");
                goto EXIT_EIO;
            }
            else
                goto EXIT_ZERO;
        }

    case PPA_CMD_GET_COUNT_LAN_SESSION:
    case PPA_CMD_GET_COUNT_WAN_SESSION:
    case PPA_CMD_GET_COUNT_MC_GROUP:
    case PPA_CMD_GET_COUNT_VLAN_FILTER:
    case PPA_CMD_GET_COUNT_MAC:
    case PPA_CMD_GET_COUNT_WAN_MII0_VLAN_RANGE:
    case PPA_CMD_GET_HOOK_COUNT:    
        {
            if ( ppa_copy_from_user(&cmd_info->count_info, (void *)arg, sizeof(cmd_info->count_info)) != 0 )
                goto EXIT_EFAULT;

            if( cmd == PPA_CMD_GET_COUNT_LAN_SESSION )
                cmd_info->count_info.count = ppa_get_routing_session_count( 1,  cmd_info->count_info.flag) ;
            else if( cmd == PPA_CMD_GET_COUNT_WAN_SESSION )
                cmd_info->count_info.count = ppa_get_routing_session_count(0,  cmd_info->count_info.flag ) ;
            else if( cmd == PPA_CMD_GET_COUNT_MC_GROUP )
                cmd_info->count_info.count = ppa_get_mc_group_count(  cmd_info->count_info.flag ) ;
            else if( cmd == PPA_CMD_GET_COUNT_VLAN_FILTER)
                cmd_info->count_info.count = ppa_get_all_vlan_filter_count(  cmd_info->count_info.flag ) ;
            else if( cmd == PPA_CMD_GET_COUNT_MAC)
                cmd_info->count_info.count = ppa_get_br_count( cmd_info->count_info.flag ) ;
            else if( cmd == PPA_CMD_GET_COUNT_WAN_MII0_VLAN_RANGE )
                   cmd_info->count_info.count = 1;
            else if( cmd == PPA_CMD_GET_HOOK_COUNT )
                    cmd_info->count_info.count = get_ppa_hook_list_count() ;
            if ( ppa_copy_to_user((void *)arg, &cmd_info->count_info, sizeof(cmd_info->count_info)) != 0 )
            {
                dbg("ppa_copy_to_user fail");
                goto EXIT_EFAULT;
            }
            goto EXIT_ZERO;
        }
    case PPA_CMD_GET_MC_GROUPS:
        {
            struct mc_group_list_item *pp_item;
            uint32_t pos = 0;
            int32_t count=0, i, index;

            if ( ppa_copy_from_user(&cmd_info->mc_groups.count_info, (void *)arg, sizeof(cmd_info->mc_groups.count_info)) != 0 )
                goto EXIT_EFAULT;

            if( cmd_info->mc_groups.count_info.count == 0 )
                goto EXIT_ZERO;

            if ( ppa_mc_group_start_iteration(&pos, &pp_item) == IFX_SUCCESS )
            {
                count = 0;
                do
                {   /*copy session information one by one to user space */
                    if ( 1 /*pp_item->flags & SESSION_ADDED_IN_HW */ )
                    {
                        ppa_memset(&cmd_info->mc_groups.mc_group_list, 0, sizeof(cmd_info->mc_groups.mc_group_list) );

                        ppa_memcpy( &cmd_info->mc_groups.mc_group_list[0].mc.mcast_addr, &pp_item->ip_mc_group, sizeof(cmd_info->mc_groups.mc_group_list[0].mc.mcast_addr));
                        ppa_memcpy( &cmd_info->mc_groups.mc_group_list[0].mc.source_ip, &pp_item->source_ip, sizeof(cmd_info->mc_groups.mc_group_list[0].mc.source_ip));
                        cmd_info->mc_groups.mc_group_list[0].mc.SSM_flag = pp_item->SSM_flag;
                        cmd_info->mc_groups.mc_group_list[0].num_ifs = pp_item->num_ifs;
                    

                        for(i=0; i<pp_item->num_ifs; i++ )
                        {
                            if( pp_item->netif[i] )
                                ppa_strncpy( cmd_info->mc_groups.mc_group_list[0].lan_ifname[i], (void *)ppa_get_netif_name(pp_item->netif[i] ), sizeof(cmd_info->mc_groups.mc_group_list[0].lan_ifname[i]) -1);
                        }

                        if( pp_item->src_netif )
                        {
                            ppa_strncpy( cmd_info->mc_groups.mc_group_list[0].src_ifname, (void *)ppa_get_netif_name(pp_item->src_netif), sizeof(cmd_info->mc_groups.mc_group_list[0].src_ifname -1) );
                        }

                        cmd_info->mc_groups.mc_group_list[0].mc.mc_extra.new_dscp = pp_item->new_dscp;

                        cmd_info->mc_groups.mc_group_list[0].mc.mc_extra.vlan_id = pp_item->new_vci &0xFFF;
                        cmd_info->mc_groups.mc_group_list[0].mc.mc_extra.vlan_prio = ( pp_item->new_vci >>13 )& 0x7;
                        cmd_info->mc_groups.mc_group_list[0].mc.mc_extra.vlan_cfi =( pp_item->new_vci >>12 )& 0x1;

                        cmd_info->mc_groups.mc_group_list[0].mc.mc_extra.out_vlan_tag = pp_item->out_vlan_tag;

                        cmd_info->mc_groups.mc_group_list[0].mc.mc_extra.dslwan_qid= pp_item->dslwan_qid;

                        cmd_info->mc_groups.mc_group_list[0].mc.flags= pp_item->flags;
                        cmd_info->mc_groups.mc_group_list[0].bridging_flag= pp_item->bridging_flag;

                        index = (void *)cmd_info->mc_groups.mc_group_list - (void *)&cmd_info->mc_groups.count_info + count * sizeof(cmd_info->mc_groups.mc_group_list[0]);

                        if ( ppa_copy_to_user(  (void *)(arg + index) , cmd_info->mc_groups.mc_group_list,  sizeof(cmd_info->mc_groups.mc_group_list[0])) != 0 )
                        {
                            ppa_mc_group_stop_iteration();
                            goto EXIT_EFAULT;
                        }

                        count ++;
                    }
                } while ( (ppa_mc_group_iterate_next(&pos, &pp_item) == IFX_SUCCESS ) && (count < cmd_info->mc_groups.count_info.count) );
            }

            cmd_info->mc_groups.count_info.count = count;  //update the real session number to user space
            ppa_mc_group_stop_iteration();

            if ( ppa_copy_to_user(  (void *)arg, &cmd_info->mc_groups.count_info, sizeof(cmd_info->mc_groups.count_info)) != 0 )
                goto EXIT_EFAULT;

            goto EXIT_ZERO;
        }

    case PPA_CMD_GET_LAN_SESSIONS:
    case PPA_CMD_GET_WAN_SESSIONS:
        {
            uint32_t   session_flag;
            struct session_list_item *pp_item;
            struct session_list_item *p_item_one_index;
            uint32_t pos = 0;
            int32_t count=0;
            uint32_t i, tmp_num;

            if ( ppa_copy_from_user(&cmd_info->session_info.count_info, (void *)arg, sizeof(cmd_info->session_info.count_info)) != 0 )
                goto EXIT_EFAULT;

            if( cmd == PPA_CMD_GET_LAN_SESSIONS )
                session_flag = SESSION_LAN_ENTRY;
            else if( cmd == PPA_CMD_GET_WAN_SESSIONS )
                session_flag = SESSION_WAN_ENTRY;
            else
                goto EXIT_EIO;            
#if 0
            if ( ppa_session_start_iteration(&pos, &pp_item) == IFX_SUCCESS && cmd_info->session_info.count_info.count )
            {
                do
                {   /*copy session information one by one to user space */
                    if ( pp_item->flags & session_flag )
                    {
                        if( cmd_info->session_info.count_info.flag == 0 ||/* get all LAN or WAN sessions */
                            ( (cmd_info->session_info.count_info.flag == SESSION_ADDED_IN_HW) && (pp_item->flags & SESSION_ADDED_IN_HW) )  || /*get all accelerated sessions only */
                            ( (cmd_info->session_info.count_info.flag == SESSION_NON_ACCE_MASK) && !(pp_item->flags & SESSION_ADDED_IN_HW ) ) ) /*get all non-acelerated sessions*/
                        {
                            cmd_info->session_info.session_list[0].ip_proto = pp_item->ip_proto;
                            cmd_info->session_info.session_list[0].ip_tos = pp_item->ip_tos;
                            memcpy( &cmd_info->session_info.session_list[0].src_ip, &pp_item->src_ip, sizeof(cmd_info->session_info.session_list[0].src_ip) );
                            cmd_info->session_info.session_list[0].src_port = pp_item->src_port;
                            memcpy( &cmd_info->session_info.session_list[0].dst_ip, &pp_item->dst_ip, sizeof(cmd_info->session_info.session_list[0].dst_ip) );
                            cmd_info->session_info.session_list[0].dst_port = pp_item->dst_port;
                            memcpy( &cmd_info->session_info.session_list[0].nat_ip, &pp_item->nat_ip, sizeof(cmd_info->session_info.session_list[0].nat_ip) );;
                            cmd_info->session_info.session_list[0].nat_port= pp_item->nat_port;
                            cmd_info->session_info.session_list[0].new_dscp= pp_item->new_dscp;
                            cmd_info->session_info.session_list[0].new_vci = pp_item->new_vci;
                            cmd_info->session_info.session_list[0].dslwan_qid = pp_item->dslwan_qid;
                            cmd_info->session_info.session_list[0].dest_ifid = pp_item->dest_ifid;
                            cmd_info->session_info.session_list[0].flags= pp_item->flags;
                            cmd_info->session_info.session_list[0].mips_bytes = pp_item->mips_bytes;
                            cmd_info->session_info.session_list[0].hw_bytes = pp_item->acc_bytes;
                            cmd_info->session_info.session_list[0].session = (uint32_t ) pp_item->session;

                            ppa_strncpy( (void *)cmd_info->session_info.session_list[0].rx_if_name, (void *)ppa_get_netif_name(pp_item->rx_if), sizeof(cmd_info->session_info.session_list[0].rx_if_name) );
                            ppa_strncpy( (void *)cmd_info->session_info.session_list[0].tx_if_name, (void *)ppa_get_netif_name(pp_item->tx_if), sizeof(cmd_info->session_info.session_list[0].tx_if_name) );

                            if ( ppa_copy_to_user( ( (void *)arg) + ( (void *)cmd_info->session_info.session_list  - (void *)&cmd_info->session_info.count_info) + count * sizeof(cmd_info->session_info.session_list[0]) , cmd_info->session_info.session_list, sizeof(cmd_info->session_info.session_list[0])) != 0 )
                            {
                                ppa_session_stop_iteration();
                                goto EXIT_EFAULT;
                            }
                            count ++;

                            if( count == cmd_info->session_info.count_info.count ) break;  //buffer is full. We should stop now
                        }
                    }
                } while ( ppa_session_iterate_next(&pos, &pp_item) == IFX_SUCCESS  );
            }
            ppa_session_stop_iteration();
#else          
           do
           {
                if( ppa_session_get_items_via_index(pos, &p_item_one_index, &tmp_num) == IFX_FAILURE )
                    break;
                pos ++;

                if( !p_item_one_index ) 
                    break;
                
                for(i=0; i<tmp_num; i++)
                {
                    pp_item = &p_item_one_index[i];
                    
                    if ( pp_item->flags & session_flag )
                    {
                        if( cmd_info->session_info.count_info.flag == 0 ||/* get all LAN or WAN sessions */
                            ( (cmd_info->session_info.count_info.flag == SESSION_ADDED_IN_HW) && (pp_item->flags & SESSION_ADDED_IN_HW) )  || /*get all accelerated sessions only */
                            ( (cmd_info->session_info.count_info.flag == SESSION_NON_ACCE_MASK) && !(pp_item->flags & SESSION_ADDED_IN_HW ) ) ) /*get all non-acelerated sessions*/
                        {
                            cmd_info->session_info.session_list[0].ip_proto = pp_item->ip_proto;
                            cmd_info->session_info.session_list[0].ip_tos = pp_item->ip_tos;
                            memcpy( &cmd_info->session_info.session_list[0].src_ip, &pp_item->src_ip, sizeof(cmd_info->session_info.session_list[0].src_ip) );
                            cmd_info->session_info.session_list[0].src_port = pp_item->src_port;
                            memcpy( &cmd_info->session_info.session_list[0].dst_ip, &pp_item->dst_ip, sizeof(cmd_info->session_info.session_list[0].dst_ip) );
                            cmd_info->session_info.session_list[0].dst_port = pp_item->dst_port;
                            memcpy( &cmd_info->session_info.session_list[0].nat_ip, &pp_item->nat_ip, sizeof(cmd_info->session_info.session_list[0].nat_ip) );;
                            cmd_info->session_info.session_list[0].nat_port= pp_item->nat_port;
                            cmd_info->session_info.session_list[0].new_dscp= pp_item->new_dscp;
                            cmd_info->session_info.session_list[0].new_vci = pp_item->new_vci;
                            cmd_info->session_info.session_list[0].dslwan_qid = pp_item->dslwan_qid;
                            cmd_info->session_info.session_list[0].dest_ifid = pp_item->dest_ifid;
                            cmd_info->session_info.session_list[0].flags= pp_item->flags;
                            cmd_info->session_info.session_list[0].mips_bytes = pp_item->mips_bytes;
                            cmd_info->session_info.session_list[0].hw_bytes = pp_item->acc_bytes;
                            cmd_info->session_info.session_list[0].session = (uint32_t ) pp_item->session;

                            ppa_strncpy( (void *)cmd_info->session_info.session_list[0].rx_if_name, (void *)ppa_get_netif_name(pp_item->rx_if), sizeof(cmd_info->session_info.session_list[0].rx_if_name) );
                            ppa_strncpy( (void *)cmd_info->session_info.session_list[0].tx_if_name, (void *)ppa_get_netif_name(pp_item->tx_if), sizeof(cmd_info->session_info.session_list[0].tx_if_name) );

                            if ( ppa_copy_to_user( ( (void *)arg) + ( (void *)cmd_info->session_info.session_list  - (void *)&cmd_info->session_info.count_info) + count * sizeof(cmd_info->session_info.session_list[0]) , cmd_info->session_info.session_list, sizeof(cmd_info->session_info.session_list[0])) != 0 )
                            {                                
                                goto EXIT_EFAULT;
                            }
                            count ++;

                            if( count == cmd_info->session_info.count_info.count ) 
                                break;  //buffer is full. We should stop now
                        }
                    }              
                }

                ppa_free(p_item_one_index);

           } while(count < cmd_info->session_info.count_info.count );
#endif

            cmd_info->session_info.count_info.count = count;  //update the real session number to user space
            if ( ppa_copy_to_user(  (void *)arg, &cmd_info->session_info.count_info, sizeof(cmd_info->session_info.count_info)) != 0 )
            {
                 goto EXIT_EFAULT;
            }

            goto EXIT_ZERO;
        }
    case PPA_CMD_GET_ALL_MAC:
        {
            struct bridging_session_list_item *pp_item;
            int32_t count = 0;
            uint32_t pos = 0;

            if ( ppa_copy_from_user(&cmd_info->all_br_info.count_info, (void *)arg, sizeof(cmd_info->all_br_info.count_info)) != 0 )
                goto EXIT_EFAULT;


            if ( ppa_bridging_session_start_iteration(&pos, &pp_item) == IFX_SUCCESS && cmd_info->all_br_info.count_info.count )
            {
                do
                {
                    ppa_memcpy( cmd_info->all_br_info.session_list[0].mac_addr, pp_item->mac, PPA_ETH_ALEN);
                    ppa_strncpy( cmd_info->all_br_info.session_list[0].ifname, ppa_get_netif_name(pp_item->netif), sizeof(cmd_info->all_br_info.session_list[0].ifname) );
                    cmd_info->all_br_info.session_list[0].flags = pp_item->flags;

                    if ( ppa_copy_to_user( ( (void *)arg) + ( (void *)cmd_info->all_br_info.session_list  - (void *)&cmd_info->all_br_info.count_info) + count * sizeof(cmd_info->all_br_info.session_list[0]) , cmd_info->all_br_info.session_list, sizeof(cmd_info->all_br_info.session_list[0])) != 0 )
                    {
                        ppa_bridging_session_stop_iteration();
                        goto EXIT_EFAULT;
                    }
                    count ++;

                    if( count == cmd_info->all_br_info.count_info.count ) break;  //buffer is full. We should stop now
                } while ( ppa_bridging_session_iterate_next(&pos, &pp_item) == IFX_SUCCESS );
            }

            ppa_bridging_session_stop_iteration();
            cmd_info->all_br_info.count_info.count = count;  //update the real session number to user space
            if ( ppa_copy_to_user(  (void *)arg, &cmd_info->all_br_info.count_info, sizeof(cmd_info->all_br_info.count_info)) != 0 )
            {
                goto EXIT_EFAULT;
            }

            goto EXIT_ZERO;
        }
    case PPA_CMD_GET_VERSION:
        {
            ppa_memset(&cmd_info->ver, 0, sizeof(cmd_info->ver) );
            ifx_ppa_get_api_id(&cmd_info->ver.ppa_api_ver.family, &cmd_info->ver.ppa_api_ver.type, &cmd_info->ver.ppa_api_ver.itf, &cmd_info->ver.ppa_api_ver.mode, &cmd_info->ver.ppa_api_ver.major, &cmd_info->ver.ppa_api_ver.mid, &cmd_info->ver.ppa_api_ver.minor);
            ppa_get_stack_al_id(&cmd_info->ver.ppa_stack_al_ver.family, &cmd_info->ver.ppa_stack_al_ver.type, &cmd_info->ver.ppa_stack_al_ver.itf, &cmd_info->ver.ppa_stack_al_ver.mode, &cmd_info->ver.ppa_stack_al_ver.major, &cmd_info->ver.ppa_stack_al_ver.mid, &cmd_info->ver.ppa_stack_al_ver.minor);
            ifx_ppa_drv_get_ppe_hal_id(&cmd_info->ver.ppe_hal_ver, 0);
            cmd_info->ver.ppe_fw_ver[0].index = 0;
            cmd_info->ver.ppe_fw_ver[1].index = 1;
            ifx_ppa_drv_get_firmware_id(&cmd_info->ver.ppe_fw_ver[0], 0);
            ifx_ppa_drv_get_firmware_id(&cmd_info->ver.ppe_fw_ver[1], 0);
            ifx_ppa_subsystem_id(&cmd_info->ver.ppa_subsys_ver.family, &cmd_info->ver.ppa_subsys_ver.type, &cmd_info->ver.ppa_subsys_ver.itf, &cmd_info->ver.ppa_subsys_ver.mode, &cmd_info->ver.ppa_subsys_ver.major, &cmd_info->ver.ppa_subsys_ver.mid, &cmd_info->ver.ppa_subsys_ver.minor);
             if( ifx_ppa_drv_hal_generic_hook ) 
            {
                ifx_ppa_drv_hal_generic_hook(PPA_GENERIC_WAN_INFO,(void *)&cmd_info->ver.ppa_wan_info, 0 ); 
                ifx_ppa_drv_hal_generic_hook(PPA_GENERIC_HAL_GET_FEATURE_LIST,(void *)&cmd_info->ver.ppe_fw_feature, 0 );
            }
             else
                dbg("Why ifx_ppa_drv_hal_generic_hook is NULL\n");

#ifdef CONFIG_IFX_PPA_IPv6_ENABLE
            cmd_info->ver.ppa_feature.ipv6_en = 1;
#endif
#ifdef CONFIG_IFX_PPA_QOS
            cmd_info->ver.ppa_feature.qos_en = 1;
#endif
    
            if ( ppa_copy_to_user(  (void *)arg, &cmd_info->ver, sizeof(cmd_info->ver)) != 0 )
                goto EXIT_EFAULT;
            goto EXIT_ZERO;
        }

    case PPA_CMD_WAN_MII0_VLAN_RANGE_ADD:
        {
            if ( ppa_copy_from_user(&cmd_info->wan_vlanid_range, (void *)arg, sizeof(cmd_info->wan_vlanid_range)) != 0 )
                goto EXIT_EFAULT;

            ifx_ppa_hook_wan_mii0_vlan_range_add(&cmd_info->wan_vlanid_range, 0);

            goto EXIT_ZERO;
        }
    case PPA_CMD_WAN_MII0_VLAN_RANGE_GET:
        {
            PPE_WAN_VID_RANGE vlanid;
            if ( ppa_copy_from_user(&cmd_info->all_wan_vlanid_range_info.count_info, (void *)arg, sizeof(cmd_info->all_wan_vlanid_range_info.count_info)) != 0 )
                goto EXIT_EFAULT;

            if( cmd_info->all_wan_vlanid_range_info.count_info.count == 0 )
                goto EXIT_ZERO;

            if( cmd_info->all_wan_vlanid_range_info.count_info.count > 1 )
                cmd_info->all_wan_vlanid_range_info.count_info.count = 1; //at present only support one wan vlan range

            ifx_ppa_drv_get_mixed_wan_vlan_id(&vlanid, 0);
            cmd_info->all_wan_vlanid_range_info.ranges[0].start_vlan_range = vlanid.vid& 0xFFF;
            cmd_info->all_wan_vlanid_range_info.ranges[0].end_vlan_range = ( vlanid.vid>> 16 ) & 0xFFF;
            if ( ppa_copy_to_user(  (void *)arg, &cmd_info->all_wan_vlanid_range_info, sizeof(cmd_info->all_wan_vlanid_range_info)) != 0 )
                goto EXIT_EFAULT;

            goto EXIT_ZERO;
        }
    case PPA_CMD_GET_SIZE:
        {
            ppa_memset(&cmd_info->size_info, 0, sizeof(cmd_info->size_info) );
            cmd_info->size_info.rout_session_size = sizeof(struct session_list_item);
            cmd_info->size_info.mc_session_size= sizeof(struct mc_group_list_item);
            cmd_info->size_info.br_session_size= sizeof(struct bridging_session_list_item);
            cmd_info->size_info.netif_size= sizeof(struct netif_info);
           
            if ( ppa_copy_to_user(  (void *)arg, &cmd_info->size_info, sizeof(cmd_info->size_info)) != 0 )
                goto EXIT_EFAULT;
            goto EXIT_ZERO;
        }
    case PPA_CMD_BRIDGE_ENABLE:
    {
        res = ppa_ioctl_bridge_enable(cmd, arg, cmd_info );
        break;
    }
    case PPA_CMD_GET_BRIDGE_STATUS:
    {
        res = ppa_ioctl_get_bridge_enable_status(cmd, arg, cmd_info );
        break;
    }
    
#ifdef CONFIG_IFX_PPA_QOS
    case PPA_CMD_GET_QOS_STATUS:
    {
        res = ppa_ioctl_get_qos_status(cmd, arg, cmd_info );
        break;
    }
    case PPA_CMD_GET_QOS_QUEUE_MAX_NUM:
    {
        res = ppa_ioctl_get_qos_qnum(cmd, arg, cmd_info );
        break;
    }
    case PPA_CMD_GET_QOS_MIB:
    {
        res = ppa_ioctl_get_qos_mib(cmd, arg, cmd_info );
        break;
    }
#ifdef CONFIG_IFX_PPA_QOS_WFQ
    case PPA_CMD_SET_CTRL_QOS_WFQ:
    {
        res = ppa_ioctl_set_ctrl_qos_wfq(cmd, arg, cmd_info );
        break;
    }
    case PPA_CMD_GET_CTRL_QOS_WFQ:
    {
        res = ppa_ioctl_get_ctrl_qos_wfq(cmd, arg, cmd_info );
        break;
    }     
    case PPA_CMD_SET_QOS_WFQ:        
    {
        res = ppa_ioctl_set_qos_wfq(cmd, arg, cmd_info );
        break;
    }        
    case PPA_CMD_RESET_QOS_WFQ:
    {
        res = ppa_ioctl_reset_qos_wfq(cmd, arg, cmd_info );
        break;
    }        
    case PPA_CMD_GET_QOS_WFQ:
    {
        res = ppa_ioctl_get_qos_wfq(cmd, arg, cmd_info );
        break;
    }
#endif //end of CONFIG_IFX_PPA_QOS_WFQ  
#ifdef CONFIG_IFX_PPA_QOS_RATE_SHAPING
    case PPA_CMD_SET_CTRL_QOS_RATE:
    {
        res = ppa_ioctl_set_ctrl_qos_rate(cmd, arg, cmd_info );
        break;
    }
    case PPA_CMD_GET_CTRL_QOS_RATE:
    {
        res = ppa_ioctl_get_ctrl_qos_rate(cmd, arg, cmd_info );
        break;
    }     
    case PPA_CMD_SET_QOS_RATE:        
    {
        res = ppa_ioctl_set_qos_rate(cmd, arg, cmd_info );
        break;
    }        
    case PPA_CMD_RESET_QOS_RATE:
    {
        res = ppa_ioctl_reset_qos_rate(cmd, arg, cmd_info );
        break;
    }        
    case PPA_CMD_GET_QOS_RATE:
    {
        res = ppa_ioctl_get_qos_rate(cmd, arg, cmd_info );
        break;
    }
#endif //end of CONFIG_IFX_PPA_QOS_RATE_SHAPING  

#endif //end of CONFIG_IFX_PPA_QOS
    case  PPA_CMD_GET_HOOK_LIST:
    {
        res = ppa_ioctl_get_hook_list(cmd, arg, cmd_info );
        break;
    }
    case  PPA_CMD_SET_HOOK:
    {
        res = ppa_ioctl_set_hook(cmd, arg, cmd_info );
        break;
    }
    case  PPA_CMD_READ_MEM:
    {
        res = ppa_ioctl_read_mem(cmd, arg, cmd_info );
        break;
    }
    case  PPA_CMD_SET_MEM:
    {
        res = ppa_ioctl_set_mem(cmd, arg, cmd_info );
        break;
    }    
#ifdef CONFIG_IFX_PPA_MFE        
    case  PPA_CMD_ENABLE_MULTIFIELD:
    {
        res = ppa_ioctl_enable_multifield(cmd, arg, cmd_info );
        break;
    }
    case  PPA_CMD_GET_MULTIFIELD_STATUS:
    {
        res = ppa_ioctl_get_multifield_status(cmd, arg, cmd_info );
        break;
    }
    case  PPA_CMD_GET_MULTIFIELD_ENTRY_MAX:
    {
        res = ppa_ioctl_get_multifield_max_entry(cmd, arg, cmd_info );
        break;
    }
    case  PPA_CMD_ADD_MULTIFIELD:
    {
        res = ppa_ioctl_add_multifield_flow(cmd, arg, cmd_info );
        break;
    }
    case  PPA_CMD_GET_MULTIFIELD:
    {
        res = ppa_ioctl_get_multifield_flow(cmd, arg, cmd_info );
        break;
    }
    case  PPA_CMD_DEL_MULTIFIELD:
    {
        res = ppa_ioctl_del_multifield_flow(cmd, arg, cmd_info );
        break;
    } 
    case  PPA_CMD_DEL_MULTIFIELD_VIA_INDEX:
    {
        res = ppa_ioctl_del_multifield_flow_via_index(cmd, arg, cmd_info );
        break;
    } 

#endif //CONFIG_IFX_PPA_MFE

    case  PPA_CMD_GET_MAX_ENTRY:
    {
        res = ppa_ioctl_get_max_entry(cmd, arg, cmd_info );
        break;
    }  

    case PPA_CMD_GET_PORTID:
    {
        res = ppa_ioctl_get_portid(cmd, arg, cmd_info );
        break;
    }    
    case PPA_CMD_GET_DSL_MIB:
    {
        res = ppa_ioctl_get_dsl_mib(cmd, arg, cmd_info );
        break;        
    }
    case PPA_CMD_CLEAR_DSL_MIB:
    {
        res = ppa_ioctl_clear_dsl_mib(cmd, arg, cmd_info );
        break;        
    }  
    case PPA_CMD_DEL_SESSION:
    {
        res = ppa_ioctl_del_session(cmd, arg, cmd_info );
        break;        
    }
    case PPA_CMD_ADD_SESSION:
    {
        res = ppa_ioctl_add_session(cmd, arg, cmd_info );
        break;        
    }
    case PPA_CMD_MODIFY_SESSION:
    {
        res = ppa_ioctl_modify_session(cmd, arg, cmd_info );
        break;        
    }
    case PPA_CMD_SET_SESSION_TIMER:
    {
        res = ppa_ioctl_set_session_timer(cmd, arg, cmd_info );
        break;        
    }
    case PPA_CMD_GET_SESSION_TIMER:
    {
        res = ppa_ioctl_get_session_timer(cmd, arg, cmd_info );
        break;        
    }    
    case PPA_CMD_GET_PORT_MIB:
    {
        res = ppa_ioctl_get_ports_mib(cmd, arg, cmd_info );
        break;        
    }
    case PPA_CMD_CLEAR_PORT_MIB:
    {
        if ( ppa_copy_from_user(&cmd_info->port_mib_info, (void *)arg, sizeof(cmd_info->port_mib_info)) != 0 )
                goto EXIT_EFAULT;
        res = ppa_ioctl_clear_ports_mib(cmd, arg, cmd_info );
        break;        
    }  
    case PPA_CMD_SET_HAL_DBG_FLAG:
    {
        if ( ppa_copy_from_user(&cmd_info->genernal_enable_info, (void *)arg, sizeof(cmd_info->genernal_enable_info)) != 0 )
                goto EXIT_EFAULT;
        if( ifx_ppa_drv_hal_generic_hook ) 
        {
            ifx_ppa_drv_hal_generic_hook(PPA_GENERIC_HAL_SET_DEBUG,(void *)&cmd_info->genernal_enable_info.enable, 0 ); 
        }
         else
            dbg("Why ifx_ppa_drv_hal_generic_hook is NULL\n");
         break;
    } 
    
    default:
        printk("wrong cmd:0x%0x(its nr value=0x%0x)\n", cmd, ppa_ioc_nr(cmd) );
        goto EXIT_ENOIOCTLCMD;
    }
    if ( res != IFX_SUCCESS )
    {
        goto EXIT_EIO;
    }
    else
        goto EXIT_ZERO;

EXIT_EIO:
    res = -EIO;
    goto EXIT_LAST;
EXIT_EFAULT:
    res = -EFAULT;
    goto EXIT_LAST;
EXIT_ZERO:
    res = 0;
    goto EXIT_LAST;
EXIT_EINVAL:
    res = -EINVAL;
    goto EXIT_LAST;
EXIT_ENOIOCTLCMD:
    res = ENOIOCTLCMD;
    goto EXIT_LAST;
EXIT_LAST:
    if( cmd_info )
        ppa_free(cmd_info);
    return res;
}

static int32_t ppa_get_routing_session_count(uint16_t bf_lan, uint32_t count_flag)
{
    struct session_list_item *pp_item;
    uint32_t pos = 0;
    int32_t count = 0;
    uint32_t session_flag;

    if ( bf_lan )
    {
        session_flag = SESSION_LAN_ENTRY;
    }
    else
    {
        session_flag = SESSION_WAN_ENTRY;
    } 

    if ( ppa_session_start_iteration(&pos, &pp_item) != IFX_SUCCESS )
    {
        ppa_session_stop_iteration();
        return count;
    }

    do
    {
        if ( pp_item->flags & session_flag )
        {
            if( count_flag == 0 ||/* get all LAN or WAN sessions */
                ( (count_flag == SESSION_ADDED_IN_HW) && (pp_item->flags & SESSION_ADDED_IN_HW) )  || /*get all accelerated sessions only */
                ( (count_flag == SESSION_NON_ACCE_MASK) && !(pp_item->flags & SESSION_ADDED_IN_HW ) ) )
            {
                count++;
                dbg("ppa_get_routing_session_count=%d", count);
            }
        }
    } while ( ppa_session_iterate_next(&pos, &pp_item) == IFX_SUCCESS );

    ppa_session_stop_iteration();

    dbg("ppa_get_routing_session_count=%d with count_flag=%x", count, count_flag);

    return count;
}

static int32_t ppa_get_mc_group_count(uint32_t count_flag)
{
    struct mc_group_list_item *pp_item;
    uint32_t pos = 0;
    int32_t count = 0;

    if ( ppa_mc_group_start_iteration(&pos, &pp_item) != IFX_SUCCESS )
    {
        ppa_mc_group_stop_iteration();
        return count;
    }

    do
    {
        if ( 1 )    //(pp_item->flags & session_flag) && (pp_item->flags & SESSION_ADDED_IN_HW) )
        {
            count++;
        }
    } while ( ppa_mc_group_iterate_next(&pos, &pp_item) == IFX_SUCCESS );

    ppa_mc_group_stop_iteration();

    return count;
}

static int32_t ppa_get_all_vlan_filter_count(uint32_t count_flag)
{
    uint32_t  i, j;
    int32_t count = 0;
    uint32_t vlan_filter_type[] = {PPA_F_VLAN_FILTER_IFNAME, PPA_F_VLAN_FILTER_IP_SRC, PPA_F_VLAN_FILTER_ETH_PROTO, PPA_F_VLAN_FILTER_VLAN_TAG};
    PPE_VFILTER_COUNT_CFG vfilter_count={0};
    PPE_BRDG_VLAN_FILTER_MAP vlan_map={0};

    count = 0;    
    for ( i = 0; i < NUM_ENTITY(vlan_filter_type); i++ )
    {
        vfilter_count.vfitler_type = vlan_filter_type[i];
        ifx_ppa_drv_get_max_vfilter_entries(&vfilter_count, 0);

        for ( j = 0; j < vfilter_count.num; j++ )
        {
            vlan_map.ig_criteria_type = vlan_filter_type[i];
            vlan_map.entry = j;
        
            if ( ifx_ppa_drv_get_vlan_map( &vlan_map , 0)  == IFX_SUCCESS )
            {
                count++;
            }
        }
    }

    return  count;
}

static int32_t ppa_get_br_count (uint32_t count_flag)
{
    struct bridging_session_list_item *pp_item;
    uint32_t pos = 0;
    int32_t count = 0;

    if ( ppa_bridging_session_start_iteration(&pos, &pp_item) != IFX_SUCCESS )
    {
        ppa_bridging_session_stop_iteration();
        return count;
    }

    do
    {
        if ( 1 )
        {
            count++;
        }
    } while ( ppa_bridging_session_iterate_next(&pos, &pp_item) == IFX_SUCCESS );

    ppa_bridging_session_stop_iteration();

    return count;
}

#if defined(CONFIG_IFX_PPA_API_PROC)
static int print_fw_ver(char *buf, int buf_len, unsigned int family, unsigned int type, unsigned int itf, unsigned int mode, unsigned int major, unsigned int minor)
{
    static char * fw_ver_family_str[] = {
        "reserved - 0",
        "Danube",
        "Twinpass",
        "Amazon-SE",
        "reserved - 4",
        "AR9",
        "GR9",
        "VR9"
    };
    static char * fw_ver_type_str[] = {
        "reserved - 0",
        "Standard",
        "Acceleration",
        "VDSL2 Bonding"
    };
    static char * fw_ver_interface_str[] = {
        "MII0",
        "MII0 + MII1",
        "MII0 + ATM",
        "MII0 + PTM",
        "MII0/1 + ATM",
        "MII0/1 + PTM"
    };
    static char * fw_ver_mode_str[] = {
        "reserved - 0",
        "reserved - 1",
        "reserved - 2",
        "Routing",
        "reserved - 4",
        "Bridging",
        "Bridging + IPv4 Routing",
        "Bridging + IPv4/6 Routing"
    };

    int len = 0;

    len += ppa_snprintf(buf + len, buf_len - len, "PPE firmware info:\n");
    len += ppa_snprintf(buf + len, buf_len - len, "  Version ID: %d.%d.%d.%d.%d.%d\n", family, type, itf, mode, major, minor);
    if ( family > NUM_ENTITY(fw_ver_family_str) -1 )
        len += ppa_snprintf(buf + len, buf_len - len, "  Family    : reserved - %d\n", family);
    else
        len += ppa_snprintf(buf + len, buf_len - len, "  Family    : %s\n", fw_ver_family_str[family]);
    if ( type > NUM_ENTITY(fw_ver_type_str) -1 )
        len += ppa_snprintf(buf + len, buf_len - len, "  FW Type   : reserved - %d\n", type);
    else
        len += ppa_snprintf(buf + len, buf_len - len, "  FW Type   : %s\n", fw_ver_type_str[type]);
    if ( itf > NUM_ENTITY(fw_ver_interface_str) -1 )
        len += ppa_snprintf(buf + len, buf_len - len, "  Interface : reserved - %d\n", itf);
    else
        len += ppa_snprintf(buf + len, buf_len - len, "  Interface : %s\n", fw_ver_interface_str[itf]);
    if ( mode > NUM_ENTITY(fw_ver_mode_str) -1 )
        len += ppa_snprintf(buf + len, buf_len - len, "  Mode      : reserved - %d\n", mode);
    else
        len += ppa_snprintf(buf + len, buf_len - len, "  Mode      : %s\n", fw_ver_mode_str[mode]);
    len += ppa_snprintf(buf + len, buf_len - len, "  Release   : %d.%d\n", major, minor);

    return len;
}

static int print_ppa_subsystem_ver(char *buf, int buf_len, char *title, unsigned int family, unsigned int type, unsigned int itf, unsigned int mode, unsigned int major, unsigned int mid, unsigned int minor)
{
    int len = 0;
    len += ppa_snprintf(buf + len, buf_len - len, "%s:\n", title);
    len += ppa_snprintf(buf + len, buf_len - len, "  Version ID: %d.%d.%d\n", major, mid, minor);
    return len;
}

static int print_driver_ver(char *buf, int buf_len, char *title, unsigned int family, unsigned int type, unsigned int itf, unsigned int mode, unsigned int major, unsigned int mid, unsigned int minor)
{
    static char * dr_ver_family_str[] = {
        NULL,
        "Danube",
        "Twinpass",
        "Amazon-SE",
        NULL,
        "AR9",
        "GR9",
        "VR9"
    };
    static char * dr_ver_type_str[] = {
        "Normal Data Path",
        "Indirect-Fast Path",
        "HAL",
        "Hook",
        "OS Adatpion Layer",
        "PPA API"
    };
    static char * dr_ver_interface_str[] = {
        "MII0",
        "MII1",
        "ATM",
        "PTM"
    };
    static char * dr_ver_accmode_str[] = {
        "Routing",
        "Bridging",
    };

    int len = 0;
    unsigned int bit;
    int i, j;

    len += ppa_snprintf(buf + len, buf_len - len, "%s:\n", title);
    len += ppa_snprintf(buf + len, buf_len - len, "  Version ID: %d.%d.%d.%d.%d.%d.%d\n", family, type, itf, mode, major, mid, minor);
    len += ppa_snprintf(buf + len, buf_len - len, "  Family    : ");
    for ( bit = family, i = j = 0; bit != 0 && i < NUM_ENTITY(dr_ver_family_str); bit >>= 1, i++ )
        if ( (bit & 0x01) && dr_ver_family_str[i] != NULL )
        {
            if ( j )
                len += ppa_snprintf(buf + len, buf_len - len, " | %s", dr_ver_family_str[i]);
            else
                len += ppa_snprintf(buf + len, buf_len - len, dr_ver_family_str[i]);
            j++;
        }
    if ( j )
        len += ppa_snprintf(buf + len, buf_len - len, "\n");
    else
        len += ppa_snprintf(buf + len, buf_len - len, "N/A\n");
    len += ppa_snprintf(buf + len, buf_len - len, "  DR Type   : ");
    for ( bit = type, i = j = 0; bit != 0 && i < NUM_ENTITY(dr_ver_type_str); bit >>= 1, i++ )
        if ( (bit & 0x01) && dr_ver_type_str[i] != NULL )
        {
            if ( j )
                len += ppa_snprintf(buf + len, buf_len - len, " | %s", dr_ver_type_str[i]);
            else
                len += ppa_snprintf(buf + len, buf_len - len, dr_ver_type_str[i]);
            j++;
        }
    if ( j )
        len += ppa_snprintf(buf + len, buf_len - len, "\n");
    else
        len += ppa_snprintf(buf + len, buf_len - len, "N/A\n");
    len += ppa_snprintf(buf + len, buf_len - len, "  Interface : ");
    for ( bit = itf, i = j = 0; bit != 0 && i < NUM_ENTITY(dr_ver_interface_str); bit >>= 1, i++ )
        if ( (bit & 0x01) && dr_ver_interface_str[i] != NULL )
        {
            if ( j )
                len += ppa_snprintf(buf + len, buf_len - len, " | %s", dr_ver_interface_str[i]);
            else
                len += ppa_snprintf(buf + len, buf_len - len, dr_ver_interface_str[i]);
            j++;
        }
    if ( j )
        len += ppa_snprintf(buf + len, buf_len - len, "\n");
    else
        len += ppa_snprintf(buf + len, buf_len - len, "N/A\n");
    len += ppa_snprintf(buf + len, buf_len - len, "  Mode      : ");
    for ( bit = mode, i = j = 0; bit != 0 && i < NUM_ENTITY(dr_ver_accmode_str); bit >>= 1, i++ )
        if ( (bit & 0x01) && dr_ver_accmode_str[i] != NULL )
        {
            if ( j )
                len += ppa_snprintf(buf + len, buf_len - len, " | %s", dr_ver_accmode_str[i]);
            else
                len += ppa_snprintf(buf + len, buf_len - len, dr_ver_accmode_str[i]);
            j++;
        }
    if ( j )
        len += ppa_snprintf(buf + len, buf_len - len, "\n");
    else
        len += ppa_snprintf(buf + len, buf_len - len, "N/A\n");
    len += ppa_snprintf(buf + len, buf_len - len, "  Release   : %d.%d.%d\n", major, mid, minor);

    return len;
}

static INLINE void proc_file_create(void)
{
    struct proc_dir_entry *res;

    for ( res = proc_root.subdir; res; res = res->next )
        if ( res->namelen == 3
            && res->name[0] == 'p'
            && res->name[1] == 'p'
            && res->name[2] == 'a' ) //  "ppa"
        {
            g_ppa_proc_dir = res;
            break;
        }
    if ( !res )
    {
        g_ppa_proc_dir = proc_mkdir("ppa", NULL);
        g_ppa_proc_dir_flag = 1;
    }

    for ( res = g_ppa_proc_dir->subdir; res; res = res->next )
        if ( res->namelen == 3
            && res->name[0] == 'a'
            && res->name[1] == 'p'
            && res->name[2] == 'i' )    //  "api"
        {
            g_ppa_api_proc_dir = res;
            break;
        }
    if ( !res )
    {
        g_ppa_api_proc_dir = proc_mkdir("api", g_ppa_proc_dir);
        g_ppa_api_proc_dir_flag = 1;
    }

    res = create_proc_read_entry("ver",
                                  0,
                                  g_ppa_api_proc_dir,
                                  proc_read_ver,
                                  NULL);
}

static INLINE void proc_file_delete(void)
{
    remove_proc_entry("ver",
                      g_ppa_api_proc_dir);

    if ( g_ppa_api_proc_dir_flag )
    {
        remove_proc_entry("api",
                          g_ppa_proc_dir);
        g_ppa_api_proc_dir = NULL;
        g_ppa_api_proc_dir_flag = 0;
    }

    if ( g_ppa_proc_dir_flag )
    {
        remove_proc_entry("ppa", NULL);
        g_ppa_proc_dir = NULL;
        g_ppa_proc_dir_flag = 0;
    }
}

static int proc_read_ver(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    PPA_VERSION ver={0};
    int len = 0;

    ifx_ppa_subsystem_id(&ver.family, &ver.type, &ver.itf, &ver.mode, &ver.major, &ver.mid, &ver.minor);
    len += print_ppa_subsystem_ver(page + off + len, count - len, "PPA Sub System info", ver.family, ver.type, ver.itf, ver.mode, ver.major, ver.mid, ver.minor);
    ifx_ppa_get_api_id(&ver.family, &ver.type, &ver.itf, &ver.mode, &ver.major, &ver.mid, &ver.minor);
    len += print_driver_ver(page + off + len, count - len, "PPA API driver info", ver.family, ver.type, ver.itf, ver.mode, ver.major, ver.mid, ver.minor);
    ppa_get_stack_al_id(&ver.family, &ver.type, &ver.itf, &ver.mode, &ver.major, &ver.mid, &ver.minor);
    len += print_driver_ver(page + off + len, count - len, "PPA Stack Adaption Layer driver info", ver.family, ver.type, ver.itf, ver.mode, ver.major, ver.mid, ver.minor);
    ifx_ppa_drv_get_ppe_hal_id(&ver, 0);
    len += print_driver_ver(page + off + len, count - len, "PPE HAL driver info", ver.family, ver.type, ver.itf, ver.mode, ver.major, ver.mid, ver.minor);
    ver.index = 0;
    if( ifx_ppa_drv_get_firmware_id(&ver, 0) == IFX_SUCCESS )
        len += print_fw_ver(page + off + len, count - len, ver.family, ver.type, ver.itf, ver.mode, ver.major, ver.minor);
    ver.index =1;
    if( ifx_ppa_drv_get_firmware_id(&ver, 0) == IFX_SUCCESS )
        len += print_fw_ver(page + off + len, count - len, ver.family, ver.type, ver.itf, ver.mode, ver.major, ver.minor);

    *eof = 1;

    return len;
}

#endif

int32_t ppa_ioctl_bridge_enable(unsigned int cmd, unsigned long arg, PPA_CMD_DATA *cmd_info)
{
    int res=IFX_FAILURE;
    
    if ( copy_from_user( &cmd_info->br_enable_info, (void *)arg, sizeof(cmd_info->br_enable_info)) != 0 )
        return IFX_FAILURE;

    res = ifx_ppa_hook_bridge_enable( cmd_info->br_enable_info.bridge_enable, cmd_info->br_enable_info.flags);

    if ( res != IFX_SUCCESS )
    {
        dbg("ppa_ioctl_bridge_enable fail");
        res = IFX_FAILURE;
    }
 
   return res;
}

int32_t ppa_ioctl_get_bridge_enable_status(unsigned int cmd, unsigned long arg, PPA_CMD_DATA *cmd_info)
{
    int res=IFX_FAILURE;
    
    ppa_memset(&cmd_info->br_enable_info, 0, sizeof(cmd_info->br_enable_info) );

    if ( copy_from_user( &cmd_info->br_enable_info, (void *)arg, sizeof(cmd_info->br_enable_info)) != 0 )
        return IFX_FAILURE;

    res = ifx_ppa_hook_get_bridge_status( &cmd_info->br_enable_info.bridge_enable, cmd_info->br_enable_info.flags);

    if ( res != IFX_SUCCESS )
    {
        dbg("ppa_ioctl_get_bridge_enable_status fail");
        res = IFX_FAILURE;
    }
    if ( ppa_copy_to_user( (void *)arg, &cmd_info->br_enable_info, sizeof(cmd_info->br_enable_info)) != 0 )
        return IFX_FAILURE;

   return res;
}


int32_t ppa_ioctl_get_max_entry(unsigned int cmd, unsigned long arg, PPA_CMD_DATA *cmd_info)
{
    int res=IFX_FAILURE;
    
    ppa_memset(&cmd_info->acc_entry_info, 0, sizeof(cmd_info->acc_entry_info) );

    if ( copy_from_user( &cmd_info->acc_entry_info, (void *)arg, sizeof(cmd_info->acc_entry_info)) != 0 )
        return IFX_FAILURE;

    res = ifx_ppa_get_max_entries( &cmd_info->acc_entry_info.entries, cmd_info->acc_entry_info.flags);

    if ( res != IFX_SUCCESS )
    {
        dbg("ppa_ioctl_get_max_entry fail");
        res = IFX_FAILURE;
    }
    if ( ppa_copy_to_user( (void *)arg, &cmd_info->acc_entry_info, sizeof(cmd_info->acc_entry_info)) != 0 )
        return IFX_FAILURE;

   return res;
}

int32_t ppa_ioctl_get_portid(unsigned int cmd, unsigned long arg, PPA_CMD_DATA *cmd_info)
{
    int res=IFX_FAILURE;
    struct netif_info *rx_ifinfo=NULL;
    
    ppa_memset(&cmd_info->portid_info, 0, sizeof(cmd_info->portid_info) );

    if ( copy_from_user( &cmd_info->portid_info, (void *)arg, sizeof(cmd_info->portid_info)) != 0 )
        return IFX_FAILURE;

    if ( ppa_netif_lookup(cmd_info->portid_info.ifname, &rx_ifinfo) != IFX_SUCCESS )
    {
        dbg("failed in getting info structure of ifname (%s)", cmd_info->portid_info.ifname );
        
        return IFX_FAILURE;
    }
    cmd_info->portid_info.portid = rx_ifinfo->phys_port;

    ppa_netif_put(rx_ifinfo);

    res = IFX_SUCCESS;
    
    if ( ppa_copy_to_user( (void *)arg, &cmd_info->portid_info, sizeof(cmd_info->portid_info)) != 0 )
        return IFX_FAILURE;

   return res;
}

int32_t ppa_ioctl_del_session(unsigned int cmd, unsigned long arg, PPA_CMD_DATA *cmd_info)
{
    struct session_list_item *pp_item=NULL, *del_pp_item=NULL;
    uint32_t pos = 0;
    PPA_IPADDR zero_ip={0};
    uint8_t buf[100];
    int del_times = 0;
    
    if ( ppa_copy_from_user(&cmd_info->session_info, (void *)arg, sizeof(cmd_info->session_info)) != 0 )
        return IFX_FAILURE;

    if( cmd_info->session_info.session_list[0].session != 0 )   {
            ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"Try to delete: 0x%08x \n", cmd_info->session_info.session_list[0].session);
            if ( ppa_session_start_iteration(&pos, &pp_item) == IFX_SUCCESS )  {   
                do  {   /*copy session information one by one to user space */
                    if( (uint32_t )pp_item->session == cmd_info->session_info.session_list[0].session ||
                        cmd_info->session_info.session_list[0].session == (uint32_t)-1 )  {
                        ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"  Found and  delete: 0x%08x---\n\n", pp_item->session);
                        ppa_hw_del_session(pp_item);
                        if( cmd_info->session_info.session_list[0].session != (uint32_t)-1 )
                        {                       
                            del_times ++;
                            if( del_times == 2 ) //at most there are two PPA session with one same netfilter sessions
                            {
                                ppa_session_stop_iteration();
                                return IFX_SUCCESS;
                            }
                        }
                    }
                } while ( ppa_session_iterate_next(&pos, &pp_item) == IFX_SUCCESS  );
        }
        ppa_session_stop_iteration();
    }
    else {
        if ( ppa_session_start_iteration(&pos, &pp_item) == IFX_SUCCESS )
        {
            ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"Try to delete via five tuple...\n");
            do {   /*note, if the value is 0, it means match all related entries */
                del_pp_item= NULL;

                if( cmd_info->session_info.session_list[0].ip_proto != 0 && 
                    cmd_info->session_info.session_list[0].ip_proto != pp_item->ip_proto)  {
                    ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT, "  Delete Proto NOT match: %d_%d\n", cmd_info->session_info.session_list[0].ip_proto, pp_item->ip_proto );
                    goto END_MATCH;
                }
                ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"  Delete Proto match:%d\n", pp_item->ip_proto);
                
                if( ppa_ip_comare( cmd_info->session_info.session_list[0].src_ip, zero_ip, cmd_info->session_info.session_list[0].flags) != 0 && 
                    ppa_ip_comare( cmd_info->session_info.session_list[0].src_ip, pp_item->src_ip, cmd_info->session_info.session_list[0].flags ) != 0 ) {
                    ppa_ip_sprintf(buf, cmd_info->session_info.session_list[0].src_ip, cmd_info->session_info.session_list[0].flags  );
                    ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT, "  Delete src_ip NOT match: %s_", buf); 
                    ppa_ip_sprintf(buf, pp_item->src_ip, cmd_info->session_info.session_list[0].flags  );
                    ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT, "%s\n", buf); 
                    goto END_MATCH;
                }
                ppa_ip_sprintf(buf, cmd_info->session_info.session_list[0].src_ip, cmd_info->session_info.session_list[0].flags  );                       
                ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT, "  Delete src_ip match: %s\n", buf );

                if( ppa_ip_comare(cmd_info->session_info.session_list[0].dst_ip, zero_ip, cmd_info->session_info.session_list[0].flags ) != 0 &&
                    ppa_ip_comare( cmd_info->session_info.session_list[0].dst_ip, pp_item->dst_ip, cmd_info->session_info.session_list[0].flags )  != 0) {
                    ppa_ip_sprintf(buf, cmd_info->session_info.session_list[0].dst_ip, cmd_info->session_info.session_list[0].flags  );
                    ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT, "  Delete dst_ip NOT match: %s_", buf); 
                    ppa_ip_sprintf(buf, pp_item->dst_ip, cmd_info->session_info.session_list[0].flags  );
                    ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT, "%s\n", buf);
                    goto END_MATCH;
                }
                ppa_ip_sprintf(buf, cmd_info->session_info.session_list[0].dst_ip, cmd_info->session_info.session_list[0].flags  );                       
                ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT, "  Delete dst_ip match: %s\n", buf );
     

                if( cmd_info->session_info.session_list[0].src_port!= 0 && 
                    cmd_info->session_info.session_list[0].src_port != pp_item->src_port) {
                    ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT, "  src_port NOT match: %d_%d\n", cmd_info->session_info.session_list[0].src_port, pp_item->src_port);
                    goto END_MATCH;
                }
                ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"  Delete src_port match:%d\n", cmd_info->session_info.session_list[0].src_port);

                if( cmd_info->session_info.session_list[0].dst_port!= 0 && 
                    cmd_info->session_info.session_list[0].dst_port != pp_item->dst_port) {
                    ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT, "  dst_port NOT match: %d_%d\n", cmd_info->session_info.session_list[0].dst_port, pp_item->dst_port);
                    goto END_MATCH;
                }
                ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"  Delete dst_port match:%d\n", cmd_info->session_info.session_list[0].dst_port);
                
                del_pp_item = pp_item;
                ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"  Found and try to delete the session---\n\n");
                
    END_MATCH:
                if( del_pp_item ) {
                    ppa_hw_del_session(del_pp_item);
                }           
            }while ( ppa_session_iterate_next(&pos, &pp_item) == IFX_SUCCESS  );
        }
        ppa_session_stop_iteration();
    }
    
    return IFX_SUCCESS;
}


int32_t ppa_ioctl_add_session(unsigned int cmd, unsigned long arg, PPA_CMD_DATA *cmd_info)
{
    #define TEST_INTERFACE_NAME "test"
    struct session_list_item *p_item;
    uint32_t pos = 0;

    if ( ppa_copy_from_user(&cmd_info->detail_session_info, (void *)arg, sizeof(cmd_info->detail_session_info)) != 0 ) {
        return IFX_FAILURE;
    }

    if ( ppa_session_start_iteration(&pos, &p_item) == IFX_SUCCESS )  {
        do {
            if( ppa_ip_comare( cmd_info->detail_session_info.src_ip, p_item->src_ip, cmd_info->detail_session_info.flags) == 0 && 
                 ppa_ip_comare( cmd_info->detail_session_info.dst_ip, p_item->dst_ip, cmd_info->detail_session_info.flags) == 0 && 
                 cmd_info->detail_session_info.src_port == p_item->src_port &&
                 cmd_info->detail_session_info.dst_port == p_item->dst_port &&
                 cmd_info->detail_session_info.dest_ifid == p_item->dest_ifid )  {
                ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"Already exist\n");  
                ppa_session_stop_iteration();
                return IFX_SUCCESS;         
            }            
        } while ( ppa_session_iterate_next(&pos, &p_item) == IFX_SUCCESS  );
    }
    else { 
        ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"No session found. So can be added\n");
    }
    ppa_session_stop_iteration();
   
#ifdef CONFIG_IFX_PPA_IPv6_ENABLE
    if( cmd_info->detail_session_info.flags & SESSION_IS_IPV6 ) {
        ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT, "add session:%s from %04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x/%5d to %04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x/%5d\n", 
            (cmd_info->detail_session_info.ip_proto == IFX_IPPROTO_TCP)?"tcp":"udp",
            NIP6(cmd_info->detail_session_info.src_ip.ip6), cmd_info->detail_session_info.src_port,
            NIP6(cmd_info->detail_session_info.dst_ip.ip6), cmd_info->detail_session_info.dst_port);

    }
    else 
#endif            
    {
        ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT, "add session:%s from %u.%u.%u.%u/%5d to %u.%u.%u.%u/%5d\n", 
            (cmd_info->detail_session_info.ip_proto == IFX_IPPROTO_TCP)?"tcp":"udp",
            NIPQUAD(cmd_info->detail_session_info.src_ip), cmd_info->detail_session_info.src_port,
            NIPQUAD(cmd_info->detail_session_info.dst_ip), cmd_info->detail_session_info.dst_port);
    }
    

    
    //Try to add
    if( (p_item = ppa_alloc_session_list_item() ) == NULL  ) {
        ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"ppa_malloc fail for session_list_item\n");
        return IFX_FAILURE;
    }
    
    p_item->session = (PPA_SESSION *)ppa_malloc( sizeof(PPA_SESSION) );
    if( !p_item->session ) {
        ppa_debug(DBG_ENABLE_MASK_DEBUG_PRINT,"ppa_malloc fail for ppa_session\n");
        ppa_free_session_list_item(p_item);
        return IFX_FAILURE;
    }

    p_item->dest_ifid = (uint16_t)cmd_info->detail_session_info.dest_ifid;
    p_item->flags = cmd_info->detail_session_info.flags;
    p_item->ip_proto = cmd_info->detail_session_info.ip_proto;
    ppa_memcpy( &p_item->dst_ip, &cmd_info->detail_session_info.dst_ip, sizeof(p_item->dst_ip) );
    p_item->dst_port = cmd_info->detail_session_info.dst_port;
    ppa_memcpy( &p_item->src_ip, &cmd_info->detail_session_info.src_ip, sizeof(p_item->src_ip) );
    p_item->src_port = cmd_info->detail_session_info.src_port;
    ppa_memcpy(  p_item->src_mac, &cmd_info->detail_session_info.src_mac, sizeof(p_item->src_mac) );
    ppa_memcpy(  p_item->dst_mac, &cmd_info->detail_session_info.dst_mac, sizeof(p_item->dst_mac) );
    p_item->nat_ip = cmd_info->detail_session_info.nat_ip;
    p_item->nat_port = cmd_info->detail_session_info.nat_port;
    p_item->rx_if=(PPA_NETIF *)TEST_INTERFACE_NAME;
    p_item->tx_if=(PPA_NETIF *)TEST_INTERFACE_NAME;
    p_item->new_dscp = cmd_info->detail_session_info.new_dscp;
    p_item->pppoe_session_id = cmd_info->detail_session_info.pppoe_session_id;
    p_item->new_vci = cmd_info->detail_session_info.in_vci_vlanid;
    p_item->out_vlan_tag = cmd_info->detail_session_info.out_vlan_tag;
    p_item->mtu = DEFAULT_MTU;
    p_item->dslwan_qid = cmd_info->detail_session_info.qid;
    p_item->num_adds = 1;

    ppa_insert_session_item( p_item ); //don't 
    
    ppa_hw_add_session( p_item, 1);
    
    return IFX_SUCCESS;
}

int32_t ppa_ioctl_modify_session(unsigned int cmd, unsigned long arg, PPA_CMD_DATA *cmd_info)
{
    int32_t res1, res2;
    if ( ppa_copy_from_user(&cmd_info->session_extra_info, (void *)arg, sizeof(cmd_info->session_extra_info)) != 0 ) {
        return IFX_FAILURE;
    }

    res1 = ifx_ppa_session_modify( (PPA_SESSION *)cmd_info->session_extra_info.session, &cmd_info->session_extra_info.session_extra, cmd_info->session_extra_info.flags );
    res2 = ifx_ppa_session_modify( (PPA_SESSION *)cmd_info->session_extra_info.session, &cmd_info->session_extra_info.session_extra, cmd_info->session_extra_info.flags | PPA_F_SESSION_REPLY_DIR );

    if( res1 != IFX_SUCCESS && res2 != IFX_SUCCESS ) return IFX_FAILURE;
    return IFX_SUCCESS;
}

int32_t ppa_ioctl_set_session_timer(unsigned int cmd, unsigned long arg, PPA_CMD_DATA *cmd_info)
{
    if ( ppa_copy_from_user(&cmd_info->session_timer_info, (void *)arg, sizeof(cmd_info->session_timer_info)) != 0 ) {
        return IFX_FAILURE;
    }

    if( ppa_hook_set_inactivity_fn ) 
        return ppa_hook_set_inactivity_fn( (PPA_U_SESSION *)cmd_info->session_timer_info.session, cmd_info->session_timer_info.timer_in_sec);
    else return IFX_FAILURE;
}

int32_t ppa_ioctl_get_session_timer(unsigned int cmd, unsigned long arg, PPA_CMD_DATA *cmd_info)
{
    ppa_memset(&cmd_info->session_timer_info, 0, sizeof(cmd_info->session_timer_info) );
    cmd_info->session_timer_info.timer_in_sec = ppa_api_get_session_poll_timer();

    if ( ppa_copy_to_user( (void *)arg, &cmd_info->session_timer_info, sizeof(cmd_info->session_timer_info)) != 0 )
        return IFX_FAILURE;
    else 
        return IFX_SUCCESS;
}

/*
 * ####################################
 *           Global Function
 * ####################################
 */



/*
 * ####################################
 *           Init/Cleanup API
 * ####################################
 */

int __init ifx_ppa_driver_init(void)
{
    int ret;

    if ( ifx_ppa_drv_hal_init(0) != IFX_SUCCESS )
        return -1;

    if ( ppa_api_session_manager_create() != IFX_SUCCESS )
        goto PPA_API_SESSION_MANAGER_CREATE_FAIL;

    if ( ppa_api_netif_manager_create() != IFX_SUCCESS )
        goto PPA_API_NETIF_MANAGER_CREATE_FAIL;

#if defined(CONFIG_IFX_PPA_API_DIRECTPATH)
    if ( ppa_api_directpath_init() != IFX_SUCCESS )
        goto PPA_API_DIRECTPATH_INIT_FAIL;
#endif

    ppa_api_netif_manager_init();

#if defined(CONFIG_IFX_PPA_API_PROC)
    ppa_api_proc_file_create();
#endif

    ppa_hook_init_fn = ifx_ppa_init;
    ppa_hook_exit_fn = ifx_ppa_exit;

    ret = ppa_register_chrdev(PPA_CHR_DEV_MAJOR, PPA_DEV_NAME, &g_ppa_dev_ops);
    if ( ret != 0 )
        printk("PPA API --- failed in register_chrdev(%d, %s, g_ppa_dev_ops), errno = %d\n", PPA_CHR_DEV_MAJOR, PPA_DEV_NAME, ret);

#if defined(CONFIG_IFX_PPA_API_PROC)
    proc_file_create();
#endif

    printk("PPA API --- init successfully\n");
    return 0;

#if defined(CONFIG_IFX_PPA_API_DIRECTPATH)
PPA_API_DIRECTPATH_INIT_FAIL:
    ppa_api_directpath_exit();
#endif
PPA_API_NETIF_MANAGER_CREATE_FAIL:
    ppa_api_session_manager_destroy();
PPA_API_SESSION_MANAGER_CREATE_FAIL:
    ifx_ppa_drv_hal_exit(0);
    return -1;
}

void __exit ifx_ppa_driver_exit(void)
{
#if defined(CONFIG_IFX_PPA_API_PROC)
    proc_file_delete();
#endif

    ppa_unregister_chrdev(PPA_CHR_DEV_MAJOR, PPA_DEV_NAME);

    ppa_hook_init_fn = NULL;
    ppa_hook_exit_fn = NULL;

#if defined(CONFIG_IFX_PPA_API_PROC)
    ppa_api_proc_file_destroy();
#endif

    ifx_ppa_exit();

#if defined(CONFIG_IFX_PPA_API_DIRECTPATH)
    ppa_api_directpath_exit();
#endif

    ppa_api_netif_manager_destroy();

    ppa_api_session_manager_destroy();

    ifx_ppa_drv_hal_exit(0);
}

module_init(ifx_ppa_driver_init);
module_exit(ifx_ppa_driver_exit);


