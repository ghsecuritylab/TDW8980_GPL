/*
 *	Ioctl handler
 *	Linux ethernet bridge
 *
 *	Authors:
 *	Lennert Buytenhek		<buytenh@gnu.org>
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 */

#include <linux/capability.h>
#include <linux/kernel.h>
#include <linux/if_bridge.h>
#include <linux/netdevice.h>
#include <linux/times.h>
#include <net/net_namespace.h>
#include <asm/uaccess.h>
#include "br_private.h"
#ifdef CONFIG_TP_IGMP_SNOOPING
#include "br_igmp.h"
#endif

#ifdef CONFIG_TP_IGMP_SNOOPING


void addr_debug(unsigned char *dest)
{
#define NUM2PRINT 50
	char buf[NUM2PRINT * 3 + 1];	/* 3 chars per byte */
	int i = 0;
	for (i = 0; i < 6 && i < NUM2PRINT; i++) {
		sprintf(buf + i * 3, "%2.2x ", 0xff & dest[i]);
	}
	printk("%s ", buf);
}

#if 0 //not used
static void ip_addr_debug(struct in_addr *addr)
{
        char buf[4];
	int i = 0;
	union ip_array src;

        src.ip_addr = addr->s_addr;
	printk("%3d.%3d.%3d.%3d", 
                      src.ip_ar[0], src.ip_ar[1], src.ip_ar[2], src.ip_ar[3]);
}
#endif

static void dolist(struct net_bridge *br)
{
	struct net_bridge_mc_fdb_entry *dst;
	if (!br)
	    return;
	printk("bridge	device	group		   reporter          mode  source timeout\n");
	list_for_each_entry_rcu(dst, &br->mc_list, list) {
	    printk("%s %6s    ", br->dev->name, dst->dst->dev->name);
	    addr_debug((unsigned char *) &dst->addr);
	    //printk("  ");
	    addr_debug((unsigned char *) &dst->host);
            //ip_addr_debug(&dst->src_entry.src);
	    printk(" %2s   %04x   %d\n", 
                        (dst->src_entry.filt_mode == MCAST_EXCLUDE) ? 
                                                            "EX" : "IN",
                                    dst->src_entry.src.s_addr, 
                                 (int) (dst->tstamp - jiffies)/HZ);
	}
}

static int br_set_port_snooping(struct net_bridge_port *p,  void __user * userbuf)
{
    unsigned char tmp[26];
    struct in_addr src;

    if (copy_from_user(tmp, userbuf, sizeof(tmp)))
		return -EFAULT;
    
    src.s_addr = *(unsigned int *)(tmp+13);
    br_igmp_mc_fdb_add(p->br, p, tmp, tmp+6, tmp[12], &src);
    return 0;
}

static int br_clear_port_snooping(struct net_bridge_port *p,  void __user * userbuf)
{
    unsigned char tmp[26];
    unsigned char all[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    struct in_addr src;

    if (copy_from_user(tmp, userbuf, sizeof(tmp)))
		return -EFAULT;

    p->br->igmp_proxy = 1;
    if (!memcmp(tmp+6, all, 6)) {
	br_igmp_mc_fdb_remove_grp(p->br, p, tmp);
    }
    else {
	src.s_addr = *(unsigned int *)(tmp+13);
	br_igmp_mc_fdb_remove(p->br, p, tmp, tmp+6, tmp[12], &src);
    }
    return 1;
}
#endif

/* called with RTNL */
static int get_bridge_ifindices(struct net *net, int *indices, int num)
{
	struct net_device *dev;
	int i = 0;

	for_each_netdev(net, dev) {
		if (i >= num)
			break;
		if (dev->priv_flags & IFF_EBRIDGE)
			indices[i++] = dev->ifindex;
	}

	return i;
}

/* called with RTNL */
static void get_port_ifindices(struct net_bridge *br, int *ifindices, int num)
{
	struct net_bridge_port *p;

	list_for_each_entry(p, &br->port_list, list) {
		if (p->port_no < num)
			ifindices[p->port_no] = p->dev->ifindex;
	}
}

/*
 * Format up to a page worth of forwarding table entries
 * userbuf -- where to copy result
 * maxnum  -- maximum number of entries desired
 *            (limited to a page for sanity)
 * offset  -- number of records to skip
 */
static int get_fdb_entries(struct net_bridge *br, void __user *userbuf,
			   unsigned long maxnum, unsigned long offset)
{
	int num;
	void *buf;
	size_t size;

	/* Clamp size to PAGE_SIZE, test maxnum to avoid overflow */
	if (maxnum > PAGE_SIZE/sizeof(struct __fdb_entry))
		maxnum = PAGE_SIZE/sizeof(struct __fdb_entry);

	size = maxnum * sizeof(struct __fdb_entry);

	buf = kmalloc(size, GFP_USER);
	if (!buf)
		return -ENOMEM;

	num = br_fdb_fillbuf(br, buf, maxnum, offset);
	if (num > 0) {
		if (copy_to_user(userbuf, buf, num*sizeof(struct __fdb_entry)))
			num = -EFAULT;
	}
	kfree(buf);

	return num;
}

static int add_del_if(struct net_bridge *br, int ifindex, int isadd)
{
	struct net_device *dev;
	int ret;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	dev = dev_get_by_index(dev_net(br->dev), ifindex);
	if (dev == NULL)
		return -EINVAL;

	if (isadd)
		ret = br_add_if(br, dev);
	else
		ret = br_del_if(br, dev);

	dev_put(dev);
	return ret;
}

#ifdef CONFIG_IFX_IGMP_SNOOPING

/* Set router port ioctl request */
static int mcast_snoop_set_router_port(struct net_bridge *br, struct ifreq *rq)
{
	struct router_port rp;
	struct net_bridge_port *port;
	struct net_device *dev;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	if (copy_from_user((void *)&rp, rq->ifr_data, sizeof(struct router_port)))
		return -EFAULT;

	dev = dev_get_by_index(dev_net(br->dev), rp.if_index);
	if (dev == NULL)
		return -EINVAL;

	port = dev->br_port;
   if (port == NULL) {
		dev_put(dev);
		return -ENODEV;
	}

	if (rp.type == IPV4) {
		port->igmp_router_port = 1;
		mod_timer(&port->igmp_router_timer, jiffies + rp.expires * HZ);
	} else if (rp.type == IPV6) {
		port->mld_router_port = 1;
		mod_timer(&port->mld_router_timer, jiffies + rp.expires * HZ);
	} else {
		dev_put(dev);
		return -EINVAL;
	}

	dev_put(dev);
	return 0;
}

/* Add / delete multicast entry */
static int add_del_mg_entry(struct net_bridge *br, struct ifreq *rq, int isadd)
{
    struct net_device *dev;
    struct net_bridge_port *port;
    struct br_grp_rec recbuf, *rec;
    char *buf;
    int nsrc = 0;
    int ret;
    int ifindex;

    if (!capable(CAP_NET_ADMIN))
        return -EPERM;

    /* Step 1. get src count from record */
    if (copy_from_user((void *)&recbuf, rq->ifr_data, sizeof(struct br_grp_rec)))
        return -EFAULT;

    ifindex = recbuf.if_index;
    nsrc = recbuf.nsrc;

    /* allocate buffer large enough to accommodate the src list also */
    buf = kmalloc(sizeof(struct br_grp_rec) + nsrc * sizeof(ipaddr_t), GFP_KERNEL);
    if(buf == NULL)
        return -ENOMEM;

    /* Step 2. now copy the entire record */
    if (copy_from_user(buf, rq->ifr_data, sizeof(struct br_grp_rec) + nsrc * sizeof(ipaddr_t)))
        return -EFAULT;

    rec = (struct br_grp_rec *)buf;

    dev = dev_get_by_index(dev_net(br->dev), ifindex);
    if (dev == NULL)
        return -EINVAL;

    port = dev->br_port;
    if (port == NULL) {
		dev_put(dev);
		kfree(buf);
		return -ENODEV;
    }

    if(isadd)
        ret = br_mg_add_entry(port, &rec->gaddr, rec->filter_mode, rec->compat, rec->nsrc, rec->slist);
    else
        ret = br_mg_del_record(port, &rec->gaddr);

    dev_put(dev);
    kfree(buf);
    return ret;
}
#endif /* CONFIG_IFX_IGMP_SNOOPING */


/*
 * Legacy ioctl's through SIOCDEVPRIVATE
 * This interface is deprecated because it was too difficult to
 * to do the translation for 32/64bit ioctl compatability.
 */
static int old_dev_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
	struct net_bridge *br = netdev_priv(dev);
	unsigned long args[4];

	if (copy_from_user(args, rq->ifr_data, sizeof(args)))
		return -EFAULT;

	switch (args[0]) {
	case BRCTL_ADD_IF:
	case BRCTL_DEL_IF:
		return add_del_if(br, args[1], args[0] == BRCTL_ADD_IF);

	case BRCTL_GET_BRIDGE_INFO:
	{
		struct __bridge_info b;

		memset(&b, 0, sizeof(struct __bridge_info));
		rcu_read_lock();
		memcpy(&b.designated_root, &br->designated_root, 8);
		memcpy(&b.bridge_id, &br->bridge_id, 8);
		b.root_path_cost = br->root_path_cost;
		b.max_age = jiffies_to_clock_t(br->max_age);
		b.hello_time = jiffies_to_clock_t(br->hello_time);
		b.forward_delay = br->forward_delay;
		b.bridge_max_age = br->bridge_max_age;
		b.bridge_hello_time = br->bridge_hello_time;
		b.bridge_forward_delay = jiffies_to_clock_t(br->bridge_forward_delay);
		b.topology_change = br->topology_change;
		b.topology_change_detected = br->topology_change_detected;
		b.root_port = br->root_port;

		b.stp_enabled = (br->stp_enabled != BR_NO_STP);
		b.ageing_time = jiffies_to_clock_t(br->ageing_time);
		b.hello_timer_value = br_timer_value(&br->hello_timer);
		b.tcn_timer_value = br_timer_value(&br->tcn_timer);
		b.topology_change_timer_value = br_timer_value(&br->topology_change_timer);
		b.gc_timer_value = br_timer_value(&br->gc_timer);
		rcu_read_unlock();

		if (copy_to_user((void __user *)args[1], &b, sizeof(b)))
			return -EFAULT;

		return 0;
	}

	case BRCTL_GET_PORT_LIST:
	{
		int num, *indices;

		num = args[2];
		if (num < 0)
			return -EINVAL;
		if (num == 0)
			num = 256;
		if (num > BR_MAX_PORTS)
			num = BR_MAX_PORTS;

		indices = kcalloc(num, sizeof(int), GFP_KERNEL);
		if (indices == NULL)
			return -ENOMEM;

		get_port_ifindices(br, indices, num);
		if (copy_to_user((void __user *)args[1], indices, num*sizeof(int)))
			num =  -EFAULT;
		kfree(indices);
		return num;
	}

	case BRCTL_SET_BRIDGE_FORWARD_DELAY:
		if (!capable(CAP_NET_ADMIN))
			return -EPERM;

		spin_lock_bh(&br->lock);
		br->bridge_forward_delay = clock_t_to_jiffies(args[1]);
		if (br_is_root_bridge(br))
			br->forward_delay = br->bridge_forward_delay;
		spin_unlock_bh(&br->lock);
		return 0;

	case BRCTL_SET_BRIDGE_HELLO_TIME:
	{
		unsigned long t = clock_t_to_jiffies(args[1]);
		if (!capable(CAP_NET_ADMIN))
			return -EPERM;

		if (t < HZ)
			return -EINVAL;

		spin_lock_bh(&br->lock);
		br->bridge_hello_time = t;
		if (br_is_root_bridge(br))
			br->hello_time = br->bridge_hello_time;
		spin_unlock_bh(&br->lock);
		return 0;
	}

	case BRCTL_SET_BRIDGE_MAX_AGE:
		if (!capable(CAP_NET_ADMIN))
			return -EPERM;

		spin_lock_bh(&br->lock);
		br->bridge_max_age = clock_t_to_jiffies(args[1]);
		if (br_is_root_bridge(br))
			br->max_age = br->bridge_max_age;
		spin_unlock_bh(&br->lock);
		return 0;

	case BRCTL_SET_AGEING_TIME:
		if (!capable(CAP_NET_ADMIN))
			return -EPERM;

		br->ageing_time = clock_t_to_jiffies(args[1]);
		return 0;

	case BRCTL_GET_PORT_INFO:
	{
		struct __port_info p;
		struct net_bridge_port *pt;

		rcu_read_lock();
		if ((pt = br_get_port(br, args[2])) == NULL) {
			rcu_read_unlock();
			return -EINVAL;
		}

		memset(&p, 0, sizeof(struct __port_info));
		memcpy(&p.designated_root, &pt->designated_root, 8);
		memcpy(&p.designated_bridge, &pt->designated_bridge, 8);
		p.port_id = pt->port_id;
		p.designated_port = pt->designated_port;
		p.path_cost = pt->path_cost;
		p.designated_cost = pt->designated_cost;
		p.state = pt->state;
		p.top_change_ack = pt->topology_change_ack;
		p.config_pending = pt->config_pending;
		p.message_age_timer_value = br_timer_value(&pt->message_age_timer);
		p.forward_delay_timer_value = br_timer_value(&pt->forward_delay_timer);
		p.hold_timer_value = br_timer_value(&pt->hold_timer);

		rcu_read_unlock();

		if (copy_to_user((void __user *)args[1], &p, sizeof(p)))
			return -EFAULT;

		return 0;
	}

	case BRCTL_SET_BRIDGE_STP_STATE:
		if (!capable(CAP_NET_ADMIN))
			return -EPERM;

		br_stp_set_enabled(br, args[1]);
		return 0;

	case BRCTL_SET_BRIDGE_PRIORITY:
		if (!capable(CAP_NET_ADMIN))
			return -EPERM;

		spin_lock_bh(&br->lock);
		br_stp_set_bridge_priority(br, args[1]);
		spin_unlock_bh(&br->lock);
		return 0;

	case BRCTL_SET_PORT_PRIORITY:
	{
		struct net_bridge_port *p;
		int ret = 0;

		if (!capable(CAP_NET_ADMIN))
			return -EPERM;

		if (args[2] >= (1<<(16-BR_PORT_BITS)))
			return -ERANGE;

		spin_lock_bh(&br->lock);
		if ((p = br_get_port(br, args[1])) == NULL)
			ret = -EINVAL;
		else
			br_stp_set_port_priority(p, args[2]);
		spin_unlock_bh(&br->lock);
		return ret;
	}
#ifdef CONFIG_TP_IGMP_SNOOPING
	case BRCTL_ENABLE_SNOOPING:
	{
		char buf[IFNAMSIZ];
		struct net_device *dev;
		struct net_bridge *br;
		if (!capable(CAP_NET_ADMIN))
			{
				printk(KERN_ERR "CAP_NET_ADMIN ERROR!\n");
				return -EPERM;
			}
	

		if (copy_from_user(buf, (void __user *)args[1], IFNAMSIZ))
			{
				printk(KERN_ERR "copy_from_user ERROR!\n");
				return -EFAULT;
			}
		buf[IFNAMSIZ - 1 ] = 0;
	
		dev = __dev_get_by_name(dev_net(dev),buf);
		if(dev == NULL)
			{
				printk(KERN_ERR "__dev_get_by_name error ERROR!\n");
				return -ENXIO; /*Could not find device*/
			}
		br = netdev_priv(dev);
		br->igmp_snooping = args[2];
	

		return 0;
	}
	
	case BRCTL_SET_PORT_SNOOPING:
	{
		struct net_bridge_port *p;
		int ret = 0;

		if (!capable(CAP_NET_ADMIN))
			return -EPERM;

		if ((p = br_get_port(br, args[1])) == NULL)
			ret = -EINVAL;
		else
			br_set_port_snooping(p, (void __user *) args[2]);
		return ret;
	}

	case BRCTL_CLEAR_PORT_SNOOPING:
	{
		struct net_bridge_port *p;
		int ret = 0;

		if (!capable(CAP_NET_ADMIN))
			return -EPERM;
		
		rcu_read_lock();
		p = br_get_port(br, args[1]);
		rcu_read_unlock();

		if(p == NULL)
			ret = -EINVAL;
		else
			br_clear_port_snooping(p, (void __user *) args[2]);

		return ret;
	}
#endif

	case BRCTL_SET_PATH_COST:
	{
		struct net_bridge_port *p;
		int ret = 0;

		if (!capable(CAP_NET_ADMIN))
			return -EPERM;

		if ((p = br_get_port(br, args[1])) == NULL)
			ret = -EINVAL;
		else
			br_stp_set_path_cost(p, args[2]);

		return ret;
	}

	case BRCTL_GET_FDB_ENTRIES:
		return get_fdb_entries(br, (void __user *)args[1],
				       args[2], args[3]);
	}

	return -EOPNOTSUPP;
}

static int old_deviceless(struct net *net, void __user *uarg)
{
	unsigned long args[3];

	if (copy_from_user(args, uarg, sizeof(args)))
		return -EFAULT;

	switch (args[0]) {
	case BRCTL_GET_VERSION:
		return BRCTL_VERSION;

	case BRCTL_GET_BRIDGES:
	{
		int *indices;
		int ret = 0;

		if (args[2] >= 2048)
			return -ENOMEM;
		indices = kcalloc(args[2], sizeof(int), GFP_KERNEL);
		if (indices == NULL)
			return -ENOMEM;

		args[2] = get_bridge_ifindices(net, indices, args[2]);

		ret = copy_to_user((void __user *)args[1], indices, args[2]*sizeof(int))
			? -EFAULT : args[2];

		kfree(indices);
		return ret;
	}

	case BRCTL_ADD_BRIDGE:
	case BRCTL_DEL_BRIDGE:
	{
		char buf[IFNAMSIZ];

		if (!capable(CAP_NET_ADMIN))
			return -EPERM;

		if (copy_from_user(buf, (void __user *)args[1], IFNAMSIZ))
			return -EFAULT;

		buf[IFNAMSIZ-1] = 0;

		if (args[0] == BRCTL_ADD_BRIDGE)
			return br_add_bridge(net, buf);

		return br_del_bridge(net, buf);
	}
#ifdef CONFIG_TP_IGMP_SNOOPING
	case BRCTL_SHOW_SNOOPING:
	{
		char buf[IFNAMSIZ];
		struct net_device *dev;

		if (!capable(CAP_NET_ADMIN))
			return -EPERM;

		if (copy_from_user(buf, (void __user *)args[1], IFNAMSIZ))
			return -EFAULT;

		buf[IFNAMSIZ-1] = 0;

		dev = __dev_get_by_name(net, buf);
		if (dev == NULL) 
		    return  -ENXIO; 	/* Could not find device */
		dolist(netdev_priv(dev));

		return 0;
	}

	case BRCTL_ENABLE_SNOOPING:
	{
		char buf[IFNAMSIZ];
		struct net_device *dev;
		struct net_bridge *br;

		if (!capable(CAP_NET_ADMIN))
			{
				return -EPERM;
			}
	
		
		if (copy_from_user(buf, (void __user *)args[1], IFNAMSIZ))
			{
				return -EFAULT;
			}
		buf[IFNAMSIZ - 1 ] = 0;
		dev = __dev_get_by_name(net, buf);
		if(dev == NULL)
			{
				return -ENXIO; /*Could not find device*/
			}
		br = netdev_priv(dev);
		br->igmp_snooping = args[2];

		return 0;
	}

	case BRCTL_ENABLE_PROXY_MODE:
	{
		char buf[IFNAMSIZ];
		struct net_device *dev;
		struct net_bridge *br;

		if (!capable(CAP_NET_ADMIN))
			return -EPERM;

		if (copy_from_user(buf, (void __user *)args[1], IFNAMSIZ))
			return -EFAULT;

		buf[IFNAMSIZ-1] = 0;

		dev = __dev_get_by_name(net, buf);
		if (dev == NULL) 
		    return  -ENXIO; 	/* Could not find device */
		
		br = netdev_priv(dev);
		br->igmp_proxy = args[2];

		return 0;
	}
	
#endif

	}

	return -EOPNOTSUPP;
}

int br_ioctl_deviceless_stub(struct net *net, unsigned int cmd, void __user *uarg)
{
	switch (cmd) {
	case SIOCGIFBR:
	case SIOCSIFBR:
		return old_deviceless(net, uarg);

	case SIOCBRADDBR:
	case SIOCBRDELBR:
	{
		char buf[IFNAMSIZ];

		if (!capable(CAP_NET_ADMIN))
			return -EPERM;

		if (copy_from_user(buf, uarg, IFNAMSIZ))
			return -EFAULT;

		buf[IFNAMSIZ-1] = 0;
		if (cmd == SIOCBRADDBR)
			return br_add_bridge(net, buf);

		return br_del_bridge(net, buf);
	}
#ifdef CONFIG_TP_IGMP_SNOOPING
	case SIOCBRSHOWSNOOPING:
	{
		char buf[IFNAMSIZ];
		struct net_device *dev;

		if (!capable(CAP_NET_ADMIN))
			return -EPERM;

		if (copy_from_user(buf, uarg, IFNAMSIZ))
			return -EFAULT;

		buf[IFNAMSIZ-1] = 0;
		
		dev = __dev_get_by_name(net,buf);
		if (dev == NULL) 
		    return -ENXIO; 	/* Could not find device */
		dolist(netdev_priv(dev));
		
		return 0;
	}
#endif

	}
	return -EOPNOTSUPP;
}

int br_dev_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
	struct net_bridge *br = netdev_priv(dev);

	switch(cmd) {
	case SIOCDEVPRIVATE:
		return old_dev_ioctl(dev, rq, cmd);

	case SIOCBRADDIF:
	case SIOCBRDELIF:
		return add_del_if(br, rq->ifr_ifindex, cmd == SIOCBRADDIF);
#ifdef CONFIG_IFX_IGMP_SNOOPING
	case SIOCBRADDMGREC:
	case SIOCBRDELMGREC:
		return add_del_mg_entry(br, rq, cmd == SIOCBRADDMGREC);
	case SIOCBRSETROUTERPORT:
		//printk("[%s():%d]\n", __func__, __LINE__);
		return mcast_snoop_set_router_port(br, rq);
#endif /* CONFIG_IFX_IGMP_SNOOPING */

	}

	pr_debug("Bridge does not support ioctl 0x%x\n", cmd);
	return -EOPNOTSUPP;
}
