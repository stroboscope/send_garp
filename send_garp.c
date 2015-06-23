/*
 *  send gratuitous arp on subscribed netdev events
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/stat.h>
#include <linux/inetdevice.h>
#include <linux/netdevice.h>
#include <net/netevent.h>
#include <net/arp.h>
#include <linux/delay.h>

MODULE_VERSION ("0.0.3");
MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("Sergey Viuchny <sergey@de-bs.ru>") ;
MODULE_DESCRIPTION ("Send gratuitous arp on netdev state changes");

static short int debug = 0;
/* send from every avaliable interface */
static short int send_all = 0;
/* wait for device intialization  */
static int garp_delay = 100;

module_param(debug, short, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(debug, " [1] - log net device state changes");
module_param(garp_delay, int, S_IRUSR | S_IWUSR | S_IRGRP);
MODULE_PARM_DESC(garp_delay, " in ms, default [100]");
module_param(send_all, short, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(send_all, 
"\n[1] - send garps from all avaliable interfaces\n\
[0] - send garp only from notificating interface (default)");

static int netdev_callback(struct notifier_block *self,
	unsigned long event, void *ctx);

static struct notifier_block nb_dev = { 
	.notifier_call = netdev_callback
}; 

/* function taken from net/ipv4/devinet.c */
static void inetdev_send_gratuitous_arp(struct net_device *dev,
	struct in_device *in_dev)
{
	struct in_ifaddr *ifa;
	
	for (ifa = in_dev->ifa_list; ifa;
	     ifa = ifa->ifa_next) {
		arp_send(ARPOP_REQUEST, ETH_P_ARP,
			ifa->ifa_local, dev,
			ifa->ifa_local, NULL,
			dev->dev_addr, NULL);
	}
}

static int netdev_callback(struct notifier_block *self,
	unsigned long event, void *ctx)
{
	struct net_device *dev;
	struct net_device *dev_n = netdev_notifier_info_to_dev(ctx);
        struct in_device *in_dev;
	char *lo_if = "lo";

	switch (event)
	{
	case NETDEV_UP:
	case NETDEV_CHANGE:
	case NETDEV_CHANGEADDR:
	case NETDEV_NOTIFY_PEERS:
		in_dev = __in_dev_get_rtnl(dev_n);

		if (!in_dev) {
			printk(KERN_INFO "not in dev, skip\n");
			break;
		}

		if (dev_n->operstate == IF_OPER_UP) {
			msleep( garp_delay );

			if (debug == 1)
				printk(KERN_INFO "send gratuitous arp from [%s]\n",
				 dev_n->name);

			inetdev_send_gratuitous_arp(dev_n, in_dev);
		}

		if (send_all == 1)
		{
			if ( strcmp(dev_n->name, lo_if) == 0){
				if (debug == 1)
					printk(KERN_INFO "notification from lo, skip\n");
				break;
			}
	
			dev = first_net_device(&init_net);
			read_lock(&dev_base_lock);
	
			while (dev) {
				if (debug == 1)
					printk(KERN_INFO 
					"found [%s], notification from [%s]\n",
					dev->name, dev_n->name);
	
				if (dev->operstate == IF_OPER_UP) {
					if (debug == 1)
						printk(KERN_INFO 
					 	"[%s] type [%d] state [%d] is up\n",
					 	dev->name, dev->type, dev->operstate);
				} else {
					if (debug == 1)
						printk(KERN_INFO 
					 	"[%s] type [%d] state [%d] unknown oper state\n",
						 dev->name, dev->type, dev->operstate);
				}
	
				if (strcmp(dev->name, lo_if) == 0){
					if (debug == 1)
						printk(KERN_INFO "skip lo\n");

					dev = next_net_device(dev);
					continue;
				}
	
	        		in_dev = __in_dev_get_rtnl(dev);
				if (!in_dev) {
					if (debug == 1)
						printk(KERN_INFO "not in dev\n");

					dev = next_net_device(dev);
					continue;
				}
	
				if (dev->operstate == IF_OPER_UP) {
					inetdev_send_gratuitous_arp(dev, in_dev);
				}
	
				dev = next_net_device(dev);
			}
			read_unlock(&dev_base_lock);
		}
		break;
	}

	if (debug == 1)
	{
		switch (event)
		{
		case NETDEV_UP:
			printk(KERN_INFO "got NETDEV_UP\n");
			break;
		case NETDEV_DOWN:
			printk(KERN_INFO "got NETDEV_DOWN\n");
			break;
		case NETDEV_REBOOT:
			printk(KERN_INFO "got NETDEV_REBOOT\n");
			break;
		case NETDEV_CHANGE:
			printk(KERN_INFO "got NETDEV_CHANGE\n");
			break;
		case NETDEV_REGISTER:
			printk(KERN_INFO "got NETDEV_REGISTER\n");
			break;
		case NETDEV_UNREGISTER:
			printk(KERN_INFO "got NETDEV_UNREGISTER\n");
			break;
		case NETDEV_CHANGEMTU:
			printk(KERN_INFO "got NETDEV_CHANGEMTU\n");
			break;
		case NETDEV_CHANGEADDR:
			printk(KERN_INFO "got NETDEV_CHANGEADDR\n");
			break;
		case NETDEV_GOING_DOWN:
			printk(KERN_INFO "got NETDEV_GOING_DOWN\n");
			break;
		case NETDEV_CHANGENAME:
			printk(KERN_INFO "got NETDEV_CHANGENAME\n");
			break;
		case NETDEV_FEAT_CHANGE:
			printk(KERN_INFO "got NETDEV_FEAT_CHANGE\n");
			break;
		case NETDEV_BONDING_FAILOVER:
			printk(KERN_INFO "got NETDEV_BONDING_FAILOVER\n");
			break;
		case NETDEV_PRE_UP:
			printk(KERN_INFO "got NETDEV_PRE_UP\n");
			break;
		case NETDEV_PRE_TYPE_CHANGE:
			printk(KERN_INFO "got NETDEV_PRE_TYPE_CHANGE\n");
			break;
		case NETDEV_POST_TYPE_CHANGE:
			printk(KERN_INFO "got NETDEV_POST_TYPE_CHANGE\n");
			break;
		case NETDEV_POST_INIT:
			printk(KERN_INFO "got NETDEV_POST_INIT\n");
			break;
		case NETDEV_UNREGISTER_FINAL:
			printk(KERN_INFO "got NETDEV_UNREGISTER_FINAL\n");
			break;
		case NETDEV_RELEASE:
			printk(KERN_INFO "got NETDEV_RELEASE\n");
			break;
		case NETDEV_NOTIFY_PEERS:
			printk(KERN_INFO "got NETDEV_NOTIFY_PEERS\n");
			break;
		case NETDEV_JOIN:
			printk(KERN_INFO "got NETDEV_JOIN\n");
			break;
		#ifdef NETDEV_PRECHANGEMTU
		case NETDEV_CHANGEUPPER:
			printk(KERN_INFO "got NETDEV_CHANGEUPPER\n");
			break;
		case NETDEV_RESEND_IGMP:
			printk(KERN_INFO "got NETDEV_RESEND_IGMP\n");
			break;
		case NETDEV_PRECHANGEMTU:
			printk(KERN_INFO "got NETDEV_PRECHANGEMTU\n");
			break;
		#endif
		#ifdef NETDEV_CHANGEINFODATA
		case NETDEV_CHANGEINFODATA:
			printk(KERN_INFO "got NETDEV_CHANGEINFODATA\n");
			break;
		#endif
		default:
			printk(KERN_INFO "got unknown NETDEV event\n");
		}
	}

	return 0;

}

static int __init send_garp_init(void)
{
	if (debug == 1)
		printk(KERN_INFO "send_garp module loaded\n");

	register_netdevice_notifier(&nb_dev);

	return 0;
}

static void __exit send_garp_exit(void)
{
	unregister_netdevice_notifier(&nb_dev);

	if (debug == 1)
		printk(KERN_INFO "send_garp module unloaded\n");

}

module_init(send_garp_init);
module_exit(send_garp_exit);

