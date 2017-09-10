/*
 * xHCI host controller driver
 *
 * Copyright (C) 2008 Intel Corp.
 *
 * Author: Sarah Sharp
 * Some code borrowed from the Linux EHCI driver.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __LINUX_XHCI_HCD_H
#define __LINUX_XHCI_HCD_H

#include <linux/version.h>
#include <linux/usb.h>
#include <linux/timer.h>
#include <linux/kernel.h>

#include "../core/hcd.h"

#ifdef __XHCI_DESC_SWAP__
	static inline __u16 DescAdj16(__u16 val)
	{
		return (
			    (((val)&0xFF00)>>8 ) |
			    (((val)&0x00FF)<<8 ) );
	}
	static inline __u32 DescAdj32(__u32 val)
	{
		return (
			    (((val)&0xFF000000)>>24) |
			    (((val)&0x00FF0000)>>8 ) |
			    (((val)&0x0000FF00)<<8 ) |
			    (((val)&0x000000FF)<<24 ));
	}
	#define ConstAdj16(val) ((((val)&0xFF00)>>8 ) |(((val)&0x00FF)<<8 ))
	#define ConstAdj32(val) ((((val)&0xFF000000)>>24) |(((val)&0x00FF0000)>>8 ) |(((val)&0x0000FF00)<<8 ) |(((val)&0x000000FF)<<24 ))
#else
	#define DescAdj16(val) (val)
	#define DescAdj32(val) (val)
	#define ConstAdj16(val) (val)
	#define ConstAdj32(val) (val)
#endif

#ifdef __XHCI_SWAP_DATA__
#else
	#define DataAdj32(val) (val)
#endif

#ifdef __IS_AXI__
#define F_REN(FuncName) xhciaxi_##FuncName
#else
#define F_REN(FuncName) xhci_##FuncName
#endif

/* Code sharing between pci-quirks and xhci hcd */
#include	"xhci-ext-caps.h"

/* xHCI PCI Configuration Registers */

#ifndef __IS_AXI__
	#define XHCI_SBRN_OFFSET	(0x60)
#endif

#ifdef __LTQUSB3__
	/* Max number of USB devices for any host controller - limit in section 6.1 */
	#define MAX_HC_SLOTS		256
	/* Section 5.3.3 - MaxPorts */
	#define MAX_HC_PORTS		127
#else
	/* Max number of USB devices for any host controller - limit in section 6.1 */
	#define MAX_HC_SLOTS		256
	/* Section 5.3.3 - MaxPorts */
	#define MAX_HC_PORTS		127
#endif

/*
 * xHCI register interface.
 * This corresponds to the eXtensible Host Controller Interface (xHCI)
 * Revision 0.95 specification
 */

/**
 * struct xhci_cap_regs - xHCI Host Controller Capability Registers.
 * @hc_capbase:		length of the capabilities register and HC version number
 * @hcs_params1:	HCSPARAMS1 - Structural Parameters 1
 * @hcs_params2:	HCSPARAMS2 - Structural Parameters 2
 * @hcs_params3:	HCSPARAMS3 - Structural Parameters 3
 * @hcc_params:		HCCPARAMS - Capability Parameters
 * @db_off:		DBOFF - Doorbell array offset
 * @run_regs_off:	RTSOFF - Runtime register space offset
 */
struct xhci_cap_regs {
	u32	hc_capbase;
	u32	hcs_params1;
	u32	hcs_params2;
	u32	hcs_params3;
	u32	hcc_params;
	u32	db_off;
	u32	run_regs_off;
	/* Reserved up to (CAPLENGTH - 0x1C) */
};

/* hc_capbase bitmasks */
/* bits 7:0 - how long is the Capabilities register */
#define HC_LENGTH(p)		XHCI_HC_LENGTH(p)
/* bits 31:16	*/
#define HC_VERSION(p)		(((p) >> 16) & 0xffff)

/* HCSPARAMS1 - hcs_params1 - bitmasks */
/* bits 0:7, Max Device Slots */
#define HCS_MAX_SLOTS(p)	(((p) >> 0) & 0xff)
#define HCS_SLOTS_MASK		0xff
/* bits 8:18, Max Interrupters */
#define HCS_MAX_INTRS(p)	(((p) >> 8) & 0x7ff)
/* bits 24:31, Max Ports - max value is 0x7F = 127 ports */
#define HCS_MAX_PORTS(p)	(((p) >> 24) & 0x7f)

/* HCSPARAMS2 - hcs_params2 - bitmasks */
/* bits 0:3, frames or uframes that SW needs to queue transactions
 * ahead of the HW to meet periodic deadlines */
#define HCS_IST(p)		(((p) >> 0) & 0xf)
/* bits 4:7, max number of Event Ring segments */
#define HCS_ERST_MAX(p)		(((p) >> 4) & 0xf)
/* bit 26 Scratchpad restore - for save/restore HW state - not used yet */
/* bits 27:31 number of Scratchpad buffers SW must allocate for the HW */
#define HCS_MAX_SCRATCHPAD(p)   (((p) >> 27) & 0x1f)

/* HCSPARAMS3 - hcs_params3 - bitmasks */
/* bits 0:7, Max U1 to U0 latency for the roothub ports */
#define HCS_U1_LATENCY(p)	(((p) >> 0) & 0xff)
/* bits 16:31, Max U2 to U0 latency for the roothub ports */
#define HCS_U2_LATENCY(p)	(((p) >> 16) & 0xffff)

/* HCCPARAMS - hcc_params - bitmasks */
/* true: HC can use 64-bit address pointers */
#define HCC_64BIT_ADDR(p)	((p) & (1 << 0))
/* true: HC can do bandwidth negotiation */
#define HCC_BANDWIDTH_NEG(p)	((p) & (1 << 1))
/* true: HC uses 64-byte Device Context structures
 * FIXME 64-byte context structures aren't supported yet.
 */
#define HCC_64BYTE_CONTEXT(p)	((p) & (1 << 2))
/* true: HC has port power switches */
#define HCC_PPC(p)		((p) & (1 << 3))
/* true: HC has port indicators */
#define HCS_INDICATOR(p)	((p) & (1 << 4))
/* true: HC has Light HC Reset Capability */
#define HCC_LIGHT_RESET(p)	((p) & (1 << 5))
/* true: HC supports latency tolerance messaging */
#define HCC_LTC(p)		((p) & (1 << 6))
/* true: no secondary Stream ID Support */
#define HCC_NSS(p)		((p) & (1 << 7))
/* Max size for Primary Stream Arrays - 2^(n+1), where n is bits 12:15 */
#define HCC_MAX_PSA		(1 << ((((p) >> 12) & 0xf) + 1))
/* Extended Capabilities pointer from PCI base - section 5.3.6 */
#define HCC_EXT_CAPS(p)		XHCI_HCC_EXT_CAPS(p)

/* db_off bitmask - bits 0:1 reserved */
#define	DBOFF_MASK	(~0x3)

/* run_regs_off bitmask - bits 0:4 reserved */
#define	RTSOFF_MASK	(~0x1f)


/* Number of registers per port */
#define	NUM_PORT_REGS	4

/**
 * struct xhci_op_regs - xHCI Host Controller Operational Registers.
 * @command:		USBCMD - xHC command register
 * @status:		USBSTS - xHC status register
 * @page_size:		This indicates the page size that the host controller
 * 			supports.  If bit n is set, the HC supports a page size
 * 			of 2^(n+12), up to a 128MB page size.
 * 			4K is the minimum page size.
 * @cmd_ring:		CRP - 64-bit Command Ring Pointer
 * @dcbaa_ptr:		DCBAAP - 64-bit Device Context Base Address Array Pointer
 * @config_reg:		CONFIG - Configure Register
 * @port_status_base:	PORTSCn - base address for Port Status and Control
 * 			Each port has a Port Status and Control register,
 * 			followed by a Port Power Management Status and Control
 * 			register, a Port Link Info register, and a reserved
 * 			register.
 * @port_power_base:	PORTPMSCn - base address for
 * 			Port Power Management Status and Control
 * @port_link_base:	PORTLIn - base address for Port Link Info (current
 * 			Link PM state and control) for USB 2.1 and USB 3.0
 * 			devices.
 */
struct xhci_op_regs {
	u32	command;
	u32	status;
	u32	page_size;
	u32	reserved1;
	u32	reserved2;
	u32	dev_notification;
	u64	cmd_ring;
	/* rsvd: offset 0x20-2F */
	u32	reserved3[4];
	u64	dcbaa_ptr;
	u32	config_reg;
	/* rsvd: offset 0x3C-3FF */
	u32	reserved4[241];
	/* port 1 registers, which serve as a base address for other ports */
	u32	port_status_base;
	u32	port_power_base;
	u32	port_link_base;
	u32	reserved5;
	/* registers for ports 2-255 */
	u32	reserved6[NUM_PORT_REGS*254];
};

/* USBCMD - USB command - command bitmasks */
/* start/stop HC execution - do not write unless HC is halted*/
#define CMD_RUN		XHCI_CMD_RUN
/* Reset HC - resets internal HC state machine and all registers (except
 * PCI config regs).  HC does NOT drive a USB reset on the downstream ports.
 * The xHCI driver must reinitialize the xHC after setting this bit.
 */
#define CMD_RESET	(1 << 1)
/* Event Interrupt Enable - a '1' allows interrupts from the host controller */
#define CMD_EIE		XHCI_CMD_EIE
/* Host System Error Interrupt Enable - get out-of-band signal for HC errors */
#define CMD_HSEIE	XHCI_CMD_HSEIE
/* bits 4:6 are reserved (and should be preserved on writes). */
/* light reset (port status stays unchanged) - reset completed when this is 0 */
#define CMD_LRESET	(1 << 7)
/* FIXME: ignoring host controller save/restore state for now. */
#define CMD_CSS		(1 << 8)
#define CMD_CRS		(1 << 9)
/* Enable Wrap Event - '1' means xHC generates an event when MFINDEX wraps. */
#define CMD_EWE		XHCI_CMD_EWE
/* MFINDEX power management - '1' means xHC can stop MFINDEX counter if all root
 * hubs are in U3 (selective suspend), disconnect, disabled, or powered-off.
 * '0' means the xHC can power it off if all ports are in the disconnect,
 * disabled, or powered-off state.
 */
#define CMD_PM_INDEX	(1 << 11)
/* bits 12:31 are reserved (and should be preserved on writes). */

/* USBSTS - USB status - status bitmasks */
/* HC not running - set to 1 when run/stop bit is cleared. */
#define STS_HALT	XHCI_STS_HALT
/* serious error, e.g. PCI parity error.  The HC will clear the run/stop bit. */
#define STS_FATAL	(1 << 2)
/* event interrupt - clear this prior to clearing any IP flags in IR set*/
#define STS_EINT	(1 << 3)
/* port change detect */
#define STS_PORT	(1 << 4)
/* bits 5:7 reserved and zeroed */
/* save state status - '1' means xHC is saving state */
#define STS_SAVE	(1 << 8)
/* restore state status - '1' means xHC is restoring state */
#define STS_RESTORE	(1 << 9)
/* true: save or restore error */
#define STS_SRE		(1 << 10)
/* true: Controller Not Ready to accept doorbell or op reg writes after reset */
#define STS_CNR		XHCI_STS_CNR
/* true: internal Host Controller Error - SW needs to reset and reinitialize */
#define STS_HCE		(1 << 12)
/* bits 13:31 reserved and should be preserved */

/*
 * DNCTRL - Device Notification Control Register - dev_notification bitmasks
 * Generate a device notification event when the HC sees a transaction with a
 * notification type that matches a bit set in this bit field.
 */
#define	DEV_NOTE_MASK		(0xffff)
#define ENABLE_DEV_NOTE(x)	(1 << x)
/* Most of the device notification types should only be used for debug.
 * SW does need to pay attention to function wake notifications.
 */
#define	DEV_NOTE_FWAKE		ENABLE_DEV_NOTE(1)

/* CRCR - Command Ring Control Register - cmd_ring bitmasks */
/* bit 0 is the command ring cycle state */
/* stop ring operation after completion of the currently executing command */
#define CMD_RING_PAUSE		(1 << 1)
/* stop ring immediately - abort the currently executing command */
#define CMD_RING_ABORT		(1 << 2)
/* true: command ring is running */
#define CMD_RING_RUNNING	(1 << 3)
/* bits 4:5 reserved and should be preserved */
/* Command Ring pointer - bit mask for the lower 32 bits. */
#define CMD_RING_RSVD_BITS	(0x3f)

/* CONFIG - Configure Register - config_reg bitmasks */
/* bits 0:7 - maximum number of device slots enabled (NumSlotsEn) */
#define MAX_DEVS(p)	((p) & 0xff)
/* bits 8:31 - reserved and should be preserved */

/* PORTSC - Port Status and Control Register - port_status_base bitmasks */
/* true: device connected */
#define PORT_CONNECT	(1 << 0)
/* true: port enabled */
#define PORT_PE		(1 << 1)
/* bit 2 reserved and zeroed */
/* true: port has an over-current condition */
#define PORT_OC		(1 << 3)
/* true: port reset signaling asserted */
#define PORT_RESET	(1 << 4)
/* Port Link State - bits 5:8
 * A read gives the current link PM state of the port,
 * a write with Link State Write Strobe set sets the link state.
 */
/* true: port has power (see HCC_PPC) */
#define PORT_POWER	(1 << 9)
/* bits 10:13 indicate device speed:
 * 0 - undefined speed - port hasn't be initialized by a reset yet
 * 1 - full speed
 * 2 - low speed
 * 3 - high speed
 * 4 - super speed
 * 5-15 reserved
 */
#define DEV_SPEED_MASK		(0xf << 10)
#define	XDEV_FS			(0x1 << 10)
#define	XDEV_LS			(0x2 << 10)
#define	XDEV_HS			(0x3 << 10)
#define	XDEV_SS			(0x4 << 10)
#define DEV_UNDEFSPEED(p)	(((p) & DEV_SPEED_MASK) == (0x0<<10))
#define DEV_FULLSPEED(p)	(((p) & DEV_SPEED_MASK) == XDEV_FS)
#define DEV_LOWSPEED(p)		(((p) & DEV_SPEED_MASK) == XDEV_LS)
#define DEV_HIGHSPEED(p)	(((p) & DEV_SPEED_MASK) == XDEV_HS)
#define DEV_SUPERSPEED(p)	(((p) & DEV_SPEED_MASK) == XDEV_SS)
/* Bits 20:23 in the Slot Context are the speed for the device */
#define	SLOT_SPEED_FS		(XDEV_FS << 10)
#define	SLOT_SPEED_LS		(XDEV_LS << 10)
#define	SLOT_SPEED_HS		(XDEV_HS << 10)
#define	SLOT_SPEED_SS		(XDEV_SS << 10)
/* Port Indicator Control */
#define PORT_LED_OFF	(0 << 14)
#define PORT_LED_AMBER	(1 << 14)
#define PORT_LED_GREEN	(2 << 14)
#define PORT_LED_MASK	(3 << 14)
/* Port Link State Write Strobe - set this when changing link state */
#define PORT_LINK_STROBE	(1 << 16)
/* true: connect status change */
#define PORT_CSC	(1 << 17)
/* true: port enable change */
#define PORT_PEC	(1 << 18)
/* true: warm reset for a USB 3.0 device is done.  A "hot" reset puts the port
 * into an enabled state, and the device into the default state.  A "warm" reset
 * also resets the link, forcing the device through the link training sequence.
 * SW can also look at the Port Reset register to see when warm reset is done.
 */
#define PORT_WRC	(1 << 19)
/* true: over-current change */
#define PORT_OCC	(1 << 20)
/* true: reset change - 1 to 0 transition of PORT_RESET */
#define PORT_RC		(1 << 21)
/* port link status change - set on some port link state transitions:
 *  Transition				Reason
 *  ------------------------------------------------------------------------------
 *  - U3 to Resume			Wakeup signaling from a device
 *  - Resume to Recovery to U0		USB 3.0 device resume
 *  - Resume to U0			USB 2.0 device resume
 *  - U3 to Recovery to U0		Software resume of USB 3.0 device complete
 *  - U3 to U0				Software resume of USB 2.0 device complete
 *  - U2 to U0				L1 resume of USB 2.1 device complete
 *  - U0 to U0 (???)			L1 entry rejection by USB 2.1 device
 *  - U0 to disabled			L1 entry error with USB 2.1 device
 *  - Any state to inactive		Error on USB 3.0 port
 */
#define PORT_PLC	(1 << 22)
/* port configure error change - port failed to configure its link partner */
#define PORT_CEC	(1 << 23)
/* bit 24 reserved */
/* wake on connect (enable) */
#define PORT_WKCONN_E	(1 << 25)
/* wake on disconnect (enable) */
#define PORT_WKDISC_E	(1 << 26)
/* wake on over-current (enable) */
#define PORT_WKOC_E	(1 << 27)
/* bits 28:29 reserved */
/* true: device is removable - for USB 3.0 roothub emulation */
#define PORT_DEV_REMOVE	(1 << 30)
/* Initiate a warm port reset - complete when PORT_WRC is '1' */
#define PORT_WR		(1 << 31)

/* Port Power Management Status and Control - port_power_base bitmasks */
/* Inactivity timer value for transitions into U1, in microseconds.
 * Timeout can be up to 127us.  0xFF means an infinite timeout.
 */
#define PORT_U1_TIMEOUT(p)	((p) & 0xff)
/* Inactivity timer value for transitions into U2 */
#define PORT_U2_TIMEOUT(p)	(((p) & 0xff) << 8)
/* Bits 24:31 for port testing */


/**
 * struct xhci_intr_reg - Interrupt Register Set
 * @irq_pending:	IMAN - Interrupt Management Register.  Used to enable
 *			interrupts and check for pending interrupts.
 * @irq_control:	IMOD - Interrupt Moderation Register.
 * 			Used to throttle interrupts.
 * @erst_size:		Number of segments in the Event Ring Segment Table (ERST).
 * @erst_base:		ERST base address.
 * @erst_dequeue:	Event ring dequeue pointer.
 *
 * Each interrupter (defined by a MSI-X vector) has an event ring and an Event
 * Ring Segment Table (ERST) associated with it.  The event ring is comprised of
 * multiple segments of the same size.  The HC places events on the ring and
 * "updates the Cycle bit in the TRBs to indicate to software the current
 * position of the Enqueue Pointer." The HCD (Linux) processes those events and
 * updates the dequeue pointer.
 */
struct xhci_intr_reg {
	u32	irq_pending;
	u32	irq_control;
	u32	erst_size;
	u32	rsvd;
	u64	erst_base;
	u64	erst_dequeue;
};

/* irq_pending bitmasks */
#define	ER_IRQ_PENDING(p)	((p) & 0x1)
/* bits 2:31 need to be preserved */
/* THIS IS BUGGY - FIXME - IP IS WRITE 1 TO CLEAR */
#define	ER_IRQ_CLEAR(p)		((p) & 0xfffffffe)
#define	ER_IRQ_ENABLE(p)	((ER_IRQ_CLEAR(p)) | 0x2)
#define	ER_IRQ_DISABLE(p)	((ER_IRQ_CLEAR(p)) & ~(0x2))

/* irq_control bitmasks */
/* Minimum interval between interrupts (in 250ns intervals).  The interval
 * between interrupts will be longer if there are no events on the event ring.
 * Default is 4000 (1 ms).
 */
#define ER_IRQ_INTERVAL_MASK	(0xffff)
/* Counter used to count down the time to the next interrupt - HW use only */
#define ER_IRQ_COUNTER_MASK	(0xffff << 16)

/* erst_size bitmasks */
/* Preserve bits 16:31 of erst_size */
#define	ERST_SIZE_MASK		(0xffff << 16)

/* erst_dequeue bitmasks */
/* Dequeue ERST Segment Index (DESI) - Segment number (or alias)
 * where the current dequeue pointer lies.  This is an optional HW hint.
 */
#define ERST_DESI_MASK		(0x7)
/* Event Handler Busy (EHB) - is the event ring scheduled to be serviced by
 * a work queue (or delayed service routine)?
 */
#define ERST_EHB		(1 << 3)
#define ERST_PTR_MASK		(0xf)

/**
 * struct xhci_run_regs
 * @microframe_index:
 * 		MFINDEX - current microframe number
 *
 * Section 5.5 Host Controller Runtime Registers:
 * "Software should read and write these registers using only Dword (32 bit)
 * or larger accesses"
 */
struct xhci_run_regs {
	u32			microframe_index;
	u32			rsvd[7];
	struct xhci_intr_reg	ir_set[128];
};

/**
 * struct doorbell_array
 *
 * Section 5.6
 */
struct xhci_doorbell_array {
	u32	doorbell[256];
};

#define	DB_TARGET_MASK		0xFFFFFF00
#define	DB_STREAM_ID_MASK	0x0000FFFF
#define	DB_TARGET_HOST		0x0
#define	DB_STREAM_ID_HOST	0x0
#define	DB_MASK			(0xff << 8)

/* Endpoint Target - bits 0:7 */
#define EPI_TO_DB(p)		(((p) + 1) & 0xff)






#ifdef __LTQUSB3__
	struct ltqxhci_global_regs {
		u32			gsbuscfg0;       //0xc100
				#define	LTQXHCI_BUS_END_DATA          0x00000800
				#define	LTQXHCI_BUS_BURST_INCR256     0x00000080
				#define	LTQXHCI_BUS_BURST_INCR128     0x00000040
				#define	LTQXHCI_BUS_BURST_INCR64      0x00000020
				#define	LTQXHCI_BUS_BURST_INCR32      0x00000010
				#define	LTQXHCI_BUS_BURST_INCR16      0x00000008
				#define	LTQXHCI_BUS_BURST_INCR8       0x00000004
				#define	LTQXHCI_BUS_BURST_INCR4       0x00000002
				#define	LTQXHCI_BUS_BURST_INCRU       0x00000001
		u32			gsbuscfg1;       //0xc104
				#define	LTQXHCI_1K_BOUNDRY            0x00001000
				#define	LTQXHCI_AXI_BURST_LIMIT_MSK   0x000000F0
				#define	LTQXHCI_AXI_BURST_LIMIT_OFF   4
		u32			gtxthrcfg;       //0xc108
				#define	LTQXHCI_TX_PKT_COUNT_EN       0x20000000
				#define	LTQXHCI_TX_PKT_COUNT_MSK      0x0F000000
				#define	LTQXHCI_TX_PKT_COUNT_OFF      24
				#define	LTQXHCI_TX_PKT_BURST_MAX_MSK  0x00FF0000
				#define	LTQXHCI_TX_PKT_BURST_MAX_OFF  16
				#define	LTQXHCI_TX_PKT_BURST_MAX_MSK  0x00FF0000
				#define	LTQXHCI_TX_PKT_BURST_MAX_OFF  16
		u32			grxthrcfg;       //0xc10c
				#define	LTQXHCI_RX_PKT_COUNT_EN       0x20000000
				#define	LTQXHCI_RX_PKT_COUNT_MSK      0x0F000000
				#define	LTQXHCI_RX_PKT_COUNT_OFF      24
				#define	LTQXHCI_RX_PKT_BURST_MAX_MSK  0x00F80000
				#define	LTQXHCI_RX_PKT_BURST_MAX_OFF  19
				#define	LTQXHCI_RX_PKT_BURST_MAX_MSK  0x00FF0000
				#define	LTQXHCI_RX_PKT_BURST_MAX_OFF  16
		u32			gctl;            //0xc110
				#define	LTQXHCI_PWRDOWNSCALE_MSK      0xFFF80000
				#define	LTQXHCI_PWRDOWNSCALE_OFF      19
				#define	LTQXHCI_U2RSTECN              0x00010000
				#define	LTQXHCI_FRMSCLDWN_MSK         0x0000C000
				#define	LTQXHCI_FRMSCLDWN_OFF         14
				#define	LTQXHCI_PRTCAPDIR_HOST        0x00001000
				#define	LTQXHCI_PRTCAPDIR_DEVICE      0x00002000
				#define	LTQXHCI_CORE_SOFT_RESET       0x00000800
				#define	LTQXHCI_LOCALLPBKEN           0x00000400
				#define	LTQXHCI_LPBKEN                0x00000200
				#define	LTQXHCI_DEBUGATTACH           0x00000100
				#define	LTQXHCI_RAMCLKSEL_MSK         0x000000C0
				#define	LTQXHCI_RAMCLKSEL_OFF         6
				#define	LTQXHCI_SCALEDOWN_DISABLE     0x00000000
				#define	LTQXHCI_SCALEDOWN_01          0x00000010
				#define	LTQXHCI_SCALEDOWN_10          0x00000020
				#define	LTQXHCI_SCALEDOWN_11          0x00000030
				#define	LTQXHCI_DISSCRAMBLE           0x00000008
				#define	LTQXHCI_SSPWRCLMP             0x00000004
				#define	LTQXHCI_HSFSLSPWRCLMP         0x00000002
				#define	LTQXHCI_DSBLCLKGTNG           0x00000001
		u32			gevten;          //0xc114
				#define	LTQXHCI_I2CEVTEN              0x00000002
				#define	LTQXHCI_ULPICKEVTEN           0x00000001
		u32			gsts;            //0xc118
				#define	LTQXHCI_CBELT_MSK             0xFFF00000
				#define	LTQXHCI_CBELT_OFF             20
				#define	LTQXHCI_OTG_IP                0x00000400
				#define	LTQXHCI_BC_IP                 0x00000200
				#define	LTQXHCI_ADP_IP                0x00000100
								#define	LTQXHCI_HOST_IP               0x00000080
				#define	LTQXHCI_DEVICE_IP             0x00000040
				#define	LTQXHCI_CSRTIMEOUT            0x00000020
				#define	LTQXHCI_BUSERRADDRVLD         0x00000010
				#define	LTQXHCI_CURMOD_MSK            0x00000003
				#define	LTQXHCI_CURMOD_OFF            0
				#define	LTQXHCI_CURMOD_DEVICE         0
				#define	LTQXHCI_CURMOD_HOST           1
				#define	LTQXHCI_CURMOD_DRD            2
		u32			resev11c;        //0xc11c
		u32			gsnpsid;         //0xc120
		u32			ggpio;           //0xc124
				#define	LTQXHCI_GPO_MSK               0xFFFF0000
				#define	LTQXHCI_GPO_OFF               16
				#define	LTQXHCI_GPI_MSK               0x0000FFFF
				#define	LTQXHCI_GPI_OFF               0
		u32			guid;            //0xc128
		u32			guctl;           //0xc12c
				#define	LTQXHCI_SPRSCTRLTRANSEN       0x00020000
				#define	LTQXHCI_RESBWHSEPS            0x00010000
				#define	LTQXHCI_CMDEVADDR             0x00008000
				#define	LTQXHCI_USBHSTINAUTORETRYEN   0x00004000
				#define	LTQXHCI_USBHSTINMAXBURST_NO   0x00000000
				#define	LTQXHCI_USBHSTINMAXBURST_01   0x00000800
				#define	LTQXHCI_USBHSTINMAXBURST_04   0x00001000
				#define	LTQXHCI_USBHSTINMAXBURST_08   0x00001800
				#define	LTQXHCI_USBHSTINMAXBURST_16   0x00002000
				#define	LTQXHCI_DTCT_DTFT             0x00000000
				#define	LTQXHCI_DTCT_500U             0x00000200
				#define	LTQXHCI_DTCT_1500U            0x00000400
				#define	LTQXHCI_DTCT_6500U            0x00000600
				#define	LTQXHCI_DTFT_MSK              0x000001FF
				#define	LTQXHCI_DTFT_OFF              0
		u32			gbuserraddrl;    //0xc130
		u32			gbuserraddrh;    //0xc134
		u32			gprtbimapl;      //0xc138
		u32			gprtbimaph;      //0xc13c
		u32			ghwparams[8];    //0xc140
		u32			gdbgfifospace;   //0xc160
		u32			gdbgltssm;       //0xc164
		u32			gdbglnmcc;       //0xc168
		u32			gdbgbmu;         //0xc16c
		u32			gdbglspmux;      //0xc170
		u32			gdbglsp;         //0xc174
		u32			gdbgepinfo0;     //0xc178
		u32			gdbgepinfo1;     //0xc17c
		u32			gprtbimap_hsl;   //0xc180
		u32			gprtbimap_hsh;   //0xc184
		u32			gprtbimap_fsl;   //0xc188
		u32			gprtbimap_fsh;   //0xc18c
		u32			gusb2phycfg[16]; //0xc200
		u32			gusb2i2cctl[16]; //0xc240
		u32			gusb2phyacc[16]; //0xc280
		u32			gusb3pipectl[16];//0xc2c0
		u32			gtxfifosiz[32];  //0xc300
		u32			grxfifosiz[32];  //0xc380
		struct{
			u32			gevntadrl;   //0xcxx0
			u32			gevntadrh;   //0xcxx4
			u32			gevntsiz;    //0xcxx8
			u32			gevntcount;  //0xcxxc
		}event_buf[32];
		u32			ghwparams8;      //0xc600
	};


	struct ltqxhci_device_regs {
		u32			dcfg;            //0xc700
		u32			dctl;            //0xc704
		u32			devten;          //0xc708
		u32			dsts;            //0xc70c
		u32			dgcmdpar;        //0xc710
		u32			dgcmd;           //0xc714
		u32			resev71871c[2];  //0xc718
		u32			dalepena;        //0xc720
		u32			resev7247fc[55]; //0xc718
		struct{
			u32			depcmdpar2;  //0xcxx0
			u32			depcmdpar0;  //0xcxx8
			u32			depcmd;      //0xcxxc
		}endpoint[32];
	};

#endif





/**
 * struct xhci_container_ctx
 * @type: Type of context.  Used to calculated offsets to contained contexts.
 * @size: Size of the context data
 * @bytes: The raw context data given to HW
 * @dma: dma address of the bytes
 *
 * Represents either a Device or Input context.  Holds a pointer to the raw
 * memory used for the context (bytes) and dma address of it (dma).
 */
struct xhci_container_ctx {
	unsigned type;
#define XHCI_CTX_TYPE_DEVICE  0x1
#define XHCI_CTX_TYPE_INPUT   0x2

	int size;

	u8 *bytes;
	dma_addr_t dma;
};

/**
 * struct xhci_slot_ctx
 * @dev_info:	Route string, device speed, hub info, and last valid endpoint
 * @dev_info2:	Max exit latency for device number, root hub port number
 * @tt_info:	tt_info is used to construct split transaction tokens
 * @dev_state:	slot state and device address
 *
 * Slot Context - section 6.2.1.1.  This assumes the HC uses 32-byte context
 * structures.  If the HC uses 64-byte contexts, there is an additional 32 bytes
 * reserved at the end of the slot context for HC internal use.
 */
struct xhci_slot_ctx {
	u32	dev_info;
	u32	dev_info2;
	u32	tt_info;
	u32	dev_state;
	/* offset 0x10 to 0x1f reserved for HC internal use */
	u32	reserved[4];
};

/* dev_info bitmasks */
/* Route String - 0:19 */
#define ROUTE_STRING_MASK	(0xfffff)
/* Device speed - values defined by PORTSC Device Speed field - 20:23 */
#define DEV_SPEED	(0xf << 20)
/* bit 24 reserved */
/* Is this LS/FS device connected through a HS hub? - bit 25 */
#define DEV_MTT		(0x1 << 25)
/* Set if the device is a hub - bit 26 */
#define DEV_HUB		(0x1 << 26)
/* Index of the last valid endpoint context in this device context - 27:31 */
#define LAST_CTX_MASK	(0x1f << 27)
#define LAST_CTX(p)	((p) << 27)
#define LAST_CTX_TO_EP_NUM(p)	(((p) >> 27) - 1)
#define SLOT_FLAG	(1 << 0)
#define EP0_FLAG	(1 << 1)

/* dev_info2 bitmasks */
/* Max Exit Latency (ms) - worst case time to wake up all links in dev path */
#define MAX_EXIT	(0xffff)
/* Root hub port number that is needed to access the USB device */
#define ROOT_HUB_PORT(p)	(((p) & 0xff) << 16)
/* Maximum number of ports under a hub device */
#define XHCI_MAX_PORTS(p)	(((p) & 0xff) << 24)

/* tt_info bitmasks */
/*
 * TT Hub Slot ID - for low or full speed devices attached to a high-speed hub
 * The Slot ID of the hub that isolates the high speed signaling from
 * this low or full-speed device.  '0' if attached to root hub port.
 */
#define TT_SLOT		(0xff)
/*
 * The number of the downstream facing port of the high-speed hub
 * '0' if the device is not low or full speed.
 */
#define TT_PORT		(0xff << 8)
#define TT_THINK_TIME(p)	(((p) & 0x3) << 16)

/* dev_state bitmasks */
/* USB device address - assigned by the HC */
#define DEV_ADDR_MASK	(0xff)
/* bits 8:26 reserved */
/* Slot state */
#define SLOT_STATE	(0x1f << 27)
#define GET_SLOT_STATE(p)	(((p) & (0x1f << 27)) >> 27)


/**
 * struct xhci_ep_ctx
 * @ep_info:	endpoint state, streams, mult, and interval information.
 * @ep_info2:	information on endpoint type, max packet size, max burst size,
 * 		error count, and whether the HC will force an event for all
 * 		transactions.
 * @deq:	64-bit ring dequeue pointer address.  If the endpoint only
 * 		defines one stream, this points to the endpoint transfer ring.
 * 		Otherwise, it points to a stream context array, which has a
 * 		ring pointer for each flow.
 * @tx_info:
 * 		Average TRB lengths for the endpoint ring and
 * 		max payload within an Endpoint Service Interval Time (ESIT).
 *
 * Endpoint Context - section 6.2.1.2.  This assumes the HC uses 32-byte context
 * structures.  If the HC uses 64-byte contexts, there is an additional 32 bytes
 * reserved at the end of the endpoint context for HC internal use.
 */
struct xhci_ep_ctx {
	u32	ep_info;
	u32	ep_info2;
	#if defined(__XHCI_DESC_SWAP__) || defined(__XHCI_DESC_SWAP64__)
		u32	deq;
		u32	deq_reserved;
	#else
		u64	deq;
	#endif
	u32	tx_info;
	/* offset 0x14 - 0x1f reserved for HC internal use */
	u32	reserved[3];
};

/* ep_info bitmasks */
/*
 * Endpoint State - bits 0:2
 * 0 - disabled
 * 1 - running
 * 2 - halted due to halt condition - ok to manipulate endpoint ring
 * 3 - stopped
 * 4 - TRB error
 * 5-7 - reserved
 */
#define EP_STATE_MASK		(0xf)
#define EP_STATE_DISABLED	0
#define EP_STATE_RUNNING	1
#define EP_STATE_HALTED		2
#define EP_STATE_STOPPED	3
#define EP_STATE_ERROR		4
/* Mult - Max number of burtst within an interval, in EP companion desc. */
#define EP_MULT(p)		((p & 0x3) << 8)
/* bits 10:14 are Max Primary Streams */
/* bit 15 is Linear Stream Array */
/* Interval - period between requests to an endpoint - 125u increments. */
#define EP_INTERVAL(p)		((p & 0xff) << 16)
#define EP_INTERVAL_TO_UFRAMES(p)		(1 << (((p) >> 16) & 0xff))

/* ep_info2 bitmasks */
/*
 * Force Event - generate transfer events for all TRBs for this endpoint
 * This will tell the HC to ignore the IOC and ISP flags (for debugging only).
 */
#define	FORCE_EVENT	(0x1)
#define ERROR_COUNT(p)	(((p) & 0x3) << 1)
#define CTX_TO_EP_TYPE(p)	(((p) >> 3) & 0x7)
#define EP_TYPE(p)	((p) << 3)
#define ISOC_OUT_EP	1
#define BULK_OUT_EP	2
#define INT_OUT_EP	3
#define CTRL_EP		4
#define ISOC_IN_EP	5
#define BULK_IN_EP	6
#define INT_IN_EP	7
/* bit 6 reserved */
/* bit 7 is Host Initiate Disable - for disabling stream selection */
#define MAX_BURST(p)	(((p)&0xff) << 8)
#define MAX_PACKET(p)	(((p)&0xffff) << 16)
#define MAX_PACKET_MASK		(0xffff << 16)
#define MAX_PACKET_DECODED(p)	(((p) >> 16) & 0xffff)

/* tx_info bitmasks */
#define AVG_TRB_LENGTH_FOR_EP(p)	((p) & 0xffff)
#define MAX_ESIT_PAYLOAD_FOR_EP(p)	(((p) & 0xffff) << 16)


/**
 * struct xhci_input_control_context
 * Input control context; see section 6.2.5.
 *
 * @drop_context:	set the bit of the endpoint context you want to disable
 * @add_context:	set the bit of the endpoint context you want to enable
 */
struct xhci_input_control_ctx {
	u32	drop_flags;
	u32	add_flags;
	u32	rsvd2[6];
};

/* Represents everything that is needed to issue a command on the command ring.
 * It's useful to pre-allocate these for commands that cannot fail due to
 * out-of-memory errors, like freeing streams.
 */
struct xhci_command {
	/* Input context for changing device state */
	struct xhci_container_ctx	*in_ctx;
	u32				status;
	/* If completion is null, no one is waiting on this command
	 * and the structure can be freed after the command completes.
	 */
	struct completion		*completion;
	union xhci_trb			*command_trb;
	struct list_head		cmd_list;
};

/* drop context bitmasks */
#define	DROP_EP(x)	(0x1 << x)
/* add context bitmasks */
#define	ADD_EP(x)	(0x1 << x)

struct xhci_virt_ep {
	struct xhci_ring		*ring;
	/* Temporary storage in case the configure endpoint command fails and we
	 * have to restore the device state to the previous state
	 */
	struct xhci_ring		*new_ring;
	unsigned int			ep_state;
#define SET_DEQ_PENDING		(1 << 0)
#define EP_HALTED		(1 << 1)
	/* ----  Related to URB cancellation ---- */
	struct list_head	cancelled_td_list;
	unsigned int		cancels_pending;
	/* The TRB that was last reported in a stopped endpoint ring */
	union xhci_trb		*stopped_trb;
	struct xhci_td		*stopped_td;
};

struct xhci_virt_device {
	/*
	 * Commands to the hardware are passed an "input context" that
	 * tells the hardware what to change in its data structures.
	 * The hardware will return changes in an "output context" that
	 * software must allocate for the hardware.  We need to keep
	 * track of input and output contexts separately because
	 * these commands might fail and we don't trust the hardware.
	 */
	struct xhci_container_ctx       *out_ctx;
	/* Used for addressing devices and configuration changes */
	struct xhci_container_ctx       *in_ctx;
	struct xhci_virt_ep		eps[31];
	struct completion		cmd_completion;
	/* Status of the last command issued for this device */
	u32				cmd_status;
	struct list_head		cmd_list;
};


/**
 * struct xhci_device_context_array
 * @dev_context_ptr	array of 64-bit DMA addresses for device contexts
 */
struct xhci_device_context_array {
	/* 64-bit device addresses; we only write 32-bit addresses */
	#if defined(__XHCI_DESC_SWAP__) || defined(__XHCI_DESC_SWAP64__)
		struct
		{
			u32	addr;
			u32	addr_reserved;
		}dev_context_ptrs[MAX_HC_SLOTS];
	#else
		u64			dev_context_ptrs[MAX_HC_SLOTS];
	#endif
	/* private xHCD pointers */
	dma_addr_t	dma;
};
/* TODO: write function to set the 64-bit device DMA address */
/*
 * TODO: change this to be dynamically sized at HC mem init time since the HC
 * might not be able to handle the maximum number of devices possible.
 */


struct xhci_stream_ctx {
	/* 64-bit stream ring address, cycle state, and stream type */
	#if defined(__XHCI_DESC_SWAP__) || defined(__XHCI_DESC_SWAP64__)
		u32	stream_ring;
		u32	stream_ring_reserved;
	#else
		u64	stream_ring;
	#endif
	/* offset 0x14 - 0x1f reserved for HC internal use */
	u32	reserved[2];
};


struct xhci_transfer_event {
	/* 64-bit buffer address, or immediate data */
	#if defined(__XHCI_DESC_SWAP__) || defined(__XHCI_DESC_SWAP64__)
		u32	buffer;
		u32	buffer_reserved;
	#else
		u64	buffer;
	#endif
	u32	transfer_len;
	/* This field is interpreted differently based on the type of TRB */
	u32	flags;
};

/** Transfer Event bit fields **/
#define	TRB_TO_EP_ID(p)	(((p) >> 16) & 0x1f)

/* Completion Code - only applicable for some types of TRBs */
#define	COMP_CODE_MASK		(0xff << 24)
#define GET_COMP_CODE(p)	(((p) & COMP_CODE_MASK) >> 24)
#define COMP_SUCCESS	1
/* Data Buffer Error */
#define COMP_DB_ERR	2
/* Babble Detected Error */
#define COMP_BABBLE	3
/* USB Transaction Error */
#define COMP_TX_ERR	4
/* TRB Error - some TRB field is invalid */
#define COMP_TRB_ERR	5
/* Stall Error - USB device is stalled */
#define COMP_STALL	6
/* Resource Error - HC doesn't have memory for that device configuration */
#define COMP_ENOMEM	7
/* Bandwidth Error - not enough room in schedule for this dev config */
#define COMP_BW_ERR	8
/* No Slots Available Error - HC ran out of device slots */
#define COMP_ENOSLOTS	9
/* Invalid Stream Type Error */
#define COMP_STREAM_ERR	10
/* Slot Not Enabled Error - doorbell rung for disabled device slot */
#define COMP_EBADSLT	11
/* Endpoint Not Enabled Error */
#define COMP_EBADEP	12
/* Short Packet */
#define COMP_SHORT_TX	13
/* Ring Underrun - doorbell rung for an empty isoc OUT ep ring */
#define COMP_UNDERRUN	14
/* Ring Overrun - isoc IN ep ring is empty when ep is scheduled to RX */
#define COMP_OVERRUN	15
/* Virtual Function Event Ring Full Error */
#define COMP_VF_FULL	16
/* Parameter Error - Context parameter is invalid */
#define COMP_EINVAL	17
/* Bandwidth Overrun Error - isoc ep exceeded its allocated bandwidth */
#define COMP_BW_OVER	18
/* Context State Error - illegal context state transition requested */
#define COMP_CTX_STATE	19
/* No Ping Response Error - HC didn't get PING_RESPONSE in time to TX */
#define COMP_PING_ERR	20
/* Event Ring is full */
#define COMP_ER_FULL	21
/* Missed Service Error - HC couldn't service an isoc ep within interval */
#define COMP_MISSED_INT	23
/* Successfully stopped command ring */
#define COMP_CMD_STOP	24
/* Successfully aborted current command and stopped command ring */
#define COMP_CMD_ABORT	25
/* Stopped - transfer was terminated by a stop endpoint command */
#define COMP_STOP	26
/* Same as COMP_EP_STOPPED, but the transfered length in the event is invalid */
#define COMP_STOP_INVAL	27
/* Control Abort Error - Debug Capability - control pipe aborted */
#define COMP_DBG_ABORT	28
/* TRB type 29 and 30 reserved */
/* Isoc Buffer Overrun - an isoc IN ep sent more data than could fit in TD */
#define COMP_BUFF_OVER	31
/* Event Lost Error - xHC has an "internal event overrun condition" */
#define COMP_ISSUES	32
/* Undefined Error - reported when other error codes don't apply */
#define COMP_UNKNOWN	33
/* Invalid Stream ID Error */
#define COMP_STRID_ERR	34
/* Secondary Bandwidth Error - may be returned by a Configure Endpoint cmd */
/* FIXME - check for this */
#define COMP_2ND_BW_ERR	35
/* Split Transaction Error */
#define	COMP_SPLIT_ERR	36

struct xhci_link_trb {
	/* 64-bit segment pointer*/
	#if defined(__XHCI_DESC_SWAP__) || defined(__XHCI_DESC_SWAP64__)
		u32	segment_ptr;
		u32	segment_ptr_reserved;
	#else
		u64	segment_ptr;
	#endif
	u32 intr_target;
	u32 control;
};

/* control bitfields */
#define LINK_TOGGLE	(0x1<<1)

/* Command completion event TRB */
struct xhci_event_cmd {
	/* Pointer to command TRB, or the value passed by the event data trb */
	#if defined(__XHCI_DESC_SWAP__) || defined(__XHCI_DESC_SWAP64__)
		u32	cmd_trb;
		u32	cmd_trb_reserved;
	#else
		u64	cmd_trb;
	#endif
	u32 status;
	u32 flags;
};

/* flags bitmasks */
/* bits 16:23 are the virtual function ID */
/* bits 24:31 are the slot ID */
#define TRB_TO_SLOT_ID(p)	(((p) & (0xff<<24)) >> 24)
#define SLOT_ID_FOR_TRB(p)	(((p) & 0xff) << 24)

/* Stop Endpoint TRB - ep_index to endpoint ID for this TRB */
#define TRB_TO_EP_INDEX(p)		((((p) & (0x1f << 16)) >> 16) - 1)
#define	EP_ID_FOR_TRB(p)		((((p) + 1) & 0x1f) << 16)


/* Port Status Change Event TRB fields */
/* Port ID - bits 31:24 */
#define GET_PORT_ID(p)		(((p) & (0xff << 24)) >> 24)

/* Normal TRB fields */
/* transfer_len bitmasks - bits 0:16 */
#define	TRB_LEN(p)		((p) & 0x1ffff)
/* TD size - number of bytes remaining in the TD (including this TRB):
 * bits 17 - 21.  Shift the number of bytes by 10. */
#define TD_REMAINDER(p)		((((p) >> 10) & 0x1f) << 17)
/* Interrupter Target - which MSI-X vector to target the completion event at */
#define TRB_INTR_TARGET(p)	(((p) & 0x3ff) << 22)
#define GET_INTR_TARGET(p)	(((p) >> 22) & 0x3ff)

/* Cycle bit - indicates TRB ownership by HC or HCD */
#define TRB_CYCLE		(1<<0)
/*
 * Force next event data TRB to be evaluated before task switch.
 * Used to pass OS data back after a TD completes.
 */
#define TRB_ENT			(1<<1)
/* Interrupt on short packet */
#define TRB_ISP			(1<<2)
/* Set PCIe no snoop attribute */
#define TRB_NO_SNOOP		(1<<3)
/* Chain multiple TRBs into a TD */
#define TRB_CHAIN		(1<<4)
/* Interrupt on completion */
#define TRB_IOC			(1<<5)
/* The buffer pointer contains immediate data */
#define TRB_IDT			(1<<6)


/* Control transfer TRB specific fields */
#define TRB_DIR_IN		(1<<16)

struct xhci_generic_trb {
	u32 field[4];
};

union xhci_trb {
	struct xhci_link_trb		link;
	struct xhci_transfer_event	trans_event;
	struct xhci_event_cmd		event_cmd;
	struct xhci_generic_trb		generic;
};

/* TRB bit mask */
#define	TRB_TYPE_BITMASK	(0xfc00)
#define TRB_TYPE(p)		((p) << 10)
/* TRB type IDs */
/* bulk, interrupt, isoc scatter/gather, and control data stage */
#define TRB_NORMAL		1
/* setup stage for control transfers */
#define TRB_SETUP		2
/* data stage for control transfers */
#define TRB_DATA		3
/* status stage for control transfers */
#define TRB_STATUS		4
/* isoc transfers */
#define TRB_ISOC		5
/* TRB for linking ring segments */
#define TRB_LINK		6
#define TRB_EVENT_DATA		7
/* Transfer Ring No-op (not for the command ring) */
#define TRB_TR_NOOP		8
/* Command TRBs */
/* Enable Slot Command */
#define TRB_ENABLE_SLOT		9
/* Disable Slot Command */
#define TRB_DISABLE_SLOT	10
/* Address Device Command */
#define TRB_ADDR_DEV		11
/* Configure Endpoint Command */
#define TRB_CONFIG_EP		12
/* Evaluate Context Command */
#define TRB_EVAL_CONTEXT	13
/* Reset Endpoint Command */
#define TRB_RESET_EP		14
/* Stop Transfer Ring Command */
#define TRB_STOP_RING		15
/* Set Transfer Ring Dequeue Pointer Command */
#define TRB_SET_DEQ		16
/* Reset Device Command */
#define TRB_RESET_DEV		17
/* Force Event Command (opt) */
#define TRB_FORCE_EVENT		18
/* Negotiate Bandwidth Command (opt) */
#define TRB_NEG_BANDWIDTH	19
/* Set Latency Tolerance Value Command (opt) */
#define TRB_SET_LT		20
/* Get port bandwidth Command */
#define TRB_GET_BW		21
/* Force Header Command - generate a transaction or link management packet */
#define TRB_FORCE_HEADER	22
/* No-op Command - not for transfer rings */
#define TRB_CMD_NOOP		23
/* TRB IDs 24-31 reserved */
/* Event TRBS */
/* Transfer Event */
#define TRB_TRANSFER		32
/* Command Completion Event */
#define TRB_COMPLETION		33
/* Port Status Change Event */
#define TRB_PORT_STATUS		34
/* Bandwidth Request Event (opt) */
#define TRB_BANDWIDTH_EVENT	35
/* Doorbell Event (opt) */
#define TRB_DOORBELL		36
/* Host Controller Event */
#define TRB_HC_EVENT		37
/* Device Notification Event - device sent function wake notification */
#define TRB_DEV_NOTE		38
/* MFINDEX Wrap Event - microframe counter wrapped */
#define TRB_MFINDEX_WRAP	39
/* TRB IDs 40-47 reserved, 48-63 is vendor-defined */

/*
 * TRBS_PER_SEGMENT must be a multiple of 4,
 * since the command ring is 64-byte aligned.
 * It must also be greater than 16.
 */
#define TRBS_PER_SEGMENT	64
/* Allow two commands + a link TRB, along with any reserved command TRBs */
#define MAX_RSVD_CMD_TRBS	(TRBS_PER_SEGMENT - 3)
#define SEGMENT_SIZE		(TRBS_PER_SEGMENT*16)
/* TRB buffer pointers can't cross 64KB boundaries */
#define TRB_MAX_BUFF_SHIFT		16
#define TRB_MAX_BUFF_SIZE	(1 << TRB_MAX_BUFF_SHIFT)

struct xhci_segment {
	union xhci_trb		*trbs;
	/* private to HCD */
	struct xhci_segment	*next;
	dma_addr_t		dma;
};

struct xhci_td {
	struct list_head	td_list;
	struct list_head	cancelled_td_list;
	struct urb		*urb;
	struct xhci_segment	*start_seg;
	union xhci_trb		*first_trb;
	union xhci_trb		*last_trb;
};

struct xhci_dequeue_state {
	struct xhci_segment *new_deq_seg;
	union xhci_trb *new_deq_ptr;
	int new_cycle_state;
};

struct xhci_ring {
	struct xhci_segment	*first_seg;
	union  xhci_trb		*enqueue;
	struct xhci_segment	*enq_seg;
	unsigned int		enq_updates;
	union  xhci_trb		*dequeue;
	struct xhci_segment	*deq_seg;
	unsigned int		deq_updates;
	struct list_head	td_list;
	/*
	 * Write the cycle state into the TRB cycle field to give ownership of
	 * the TRB to the host controller (if we are the producer), or to check
	 * if we own the TRB (if we are the consumer).  See section 4.9.1.
	 */
	u32			cycle_state;
};

struct xhci_erst_entry {
	/* 64-bit event ring segment address */
	#if defined(__XHCI_DESC_SWAP__) || defined(__XHCI_DESC_SWAP64__)
		u32	seg_addr;
		u32	seg_addr_reserved;
	#else
		u64	seg_addr;
	#endif
	u32	seg_size;
	/* Set to zero */
	u32	rsvd;
};

struct xhci_erst {
	struct xhci_erst_entry	*entries;
	unsigned int		num_entries;
	/* xhci->event_ring keeps track of segment dma addresses */
	dma_addr_t		erst_dma_addr;
	/* Num entries the ERST can contain */
	unsigned int		erst_size;
};

struct xhci_scratchpad {
	u64 *sp_array;
	dma_addr_t sp_dma;
	void **sp_buffers;
	dma_addr_t *sp_dma_buffers;
};

/*
 * Each segment table entry is 4*32bits long.  1K seems like an ok size:
 * (1K bytes * 8bytes/bit) / (4*32 bits) = 64 segment entries in the table,
 * meaning 64 ring segments.
 * Initial allocated size of the ERST, in number of entries */
#define	ERST_NUM_SEGS	1
/* Initial allocated size of the ERST, in number of entries */
#define	ERST_SIZE	64
/* Initial number of event segment rings allocated */
#define	ERST_ENTRIES	1
/* Poll every 60 seconds */
#define	POLL_TIMEOUT	60
/* XXX: Make these module parameters */


/* There is one ehci_hci structure per controller */
struct xhci_hcd {
	/* glue to PCI and HCD framework */
	struct xhci_cap_regs __iomem *cap_regs;
	struct xhci_op_regs __iomem *op_regs;
	struct xhci_run_regs __iomem *run_regs;
	struct xhci_doorbell_array __iomem *dba;
	/* Our HCD's current interrupter register set */
	struct	xhci_intr_reg __iomem *ir_set;

	#ifdef __LTQUSB3__
		struct ltqxhci_global_regs __iomem *global_regs;
			#define LTQXHCI_GLOBAL_REGS_OFF 0x0000C100
//		struct ltqxhci_device_regs __iomem *device_regs;
//			#define LTQXHCI_DEVICE_REGS_OFF
		__u32 __iomem *debug_regs_0;
			#define LTQXHCI_DEBUG_REGS_0_OFF 0x00040000
		__u32 __iomem *debug_regs_1;
			#define LTQXHCI_DEBUG_REGS_1_OFF 0x00080000
		__u32 __iomem *debug_regs_2;
			#define LTQXHCI_DEBUG_REGS_2_OFF 0x000C0000
	#endif

	/* Cached register copies of read-only HC data */
	__u32		hcs_params1;
	__u32		hcs_params2;
	__u32		hcs_params3;
	__u32		hcc_params;

	spinlock_t	lock;

	#ifdef __IS_AXI__
	struct platform_device *pdev;
	#endif

	/* packed release number */
	u8		sbrn;
	u16		hci_version;
	u8		max_slots;
	u8		max_interrupters;
	u8		max_ports;
	u8		isoc_threshold;
	int		event_ring_max;
	int		addr_64;
	/* 4KB min, 128MB max */
	int		page_size;
	/* Valid values are 12 to 20, inclusive */
	int		page_shift;
	/* only one MSI vector for now, but might need more later */
	int		msix_count;
	struct msix_entry	*msix_entries;
	/* data structures */
	struct xhci_device_context_array *dcbaa;
	struct xhci_ring	*cmd_ring;
	unsigned int		cmd_ring_reserved_trbs;
	struct xhci_ring	*event_ring;
	struct xhci_erst	erst;
	/* Scratchpad */
	struct xhci_scratchpad  *scratchpad;

	/* slot enabling and address device helpers */
	struct completion	addr_dev;
	int slot_id;
	/* Internal mirror of the HW's dcbaa */
	struct xhci_virt_device	*devs[MAX_HC_SLOTS];

	/* DMA pools */
	struct dma_pool	*device_pool;
	struct dma_pool	*segment_pool;

#ifdef CONFIG_USB_XHCI_HCD_DEBUGGING
	/* Poll the rings - for debugging */
	struct timer_list	event_ring_timer;
	int			zombie;
#endif
	/* Statistics */
	int			noops_submitted;
	int			noops_handled;
	int			error_bitmask;
	unsigned int		quirks;
#define	XHCI_LINK_TRB_QUIRK	(1 << 0)
#define XHCI_RESET_EP_QUIRK	(1 << 1)
};

/* For testing purposes */
#define NUM_TEST_NOOPS	0

/* convert between an HCD pointer and the corresponding EHCI_HCD */
static inline struct xhci_hcd *hcd_to_xhci(struct usb_hcd *hcd)
{
	return (struct xhci_hcd *) (hcd->hcd_priv);
}

static inline struct usb_hcd *xhci_to_hcd(struct xhci_hcd *xhci)
{
	return container_of((void *) xhci, struct usb_hcd, hcd_priv);
}

#ifdef CONFIG_USB_XHCI_HCD_DEBUGGING
#define XHCI_DEBUG	1
#else
#define XHCI_DEBUG	0
#endif

#define xhci_dbg(xhci, fmt, args...) \
	do { if (XHCI_DEBUG) dev_dbg(xhci_to_hcd(xhci)->self.controller , fmt , ## args); } while (0)
#define xhci_info(xhci, fmt, args...) \
	do { if (XHCI_DEBUG) dev_info(xhci_to_hcd(xhci)->self.controller , fmt , ## args); } while (0)
#define xhci_err(xhci, fmt, args...) \
	dev_err(xhci_to_hcd(xhci)->self.controller , fmt , ## args)
#define xhci_warn(xhci, fmt, args...) \
	dev_warn(xhci_to_hcd(xhci)->self.controller , fmt , ## args)

/* TODO: copied from ehci.h - can be refactored? */
/* xHCI spec says all registers are little endian */
static inline unsigned int xhci_readl(const struct xhci_hcd *xhci,
		__u32 __iomem *regs)
{
	#ifdef __XHCI_REG_SWAP__
		__u32 ret=readl(regs);
		ret =
		((ret&0xFF000000)>>24) |
		((ret&0x00FF0000)>>8 ) |
		((ret&0x0000FF00)<<8 ) |
		((ret&0x000000FF)<<24) ;
		return ret;
	#else
		return readl(regs);
	#endif
}
static inline void xhci_writel(struct xhci_hcd *xhci,
		const unsigned int val, __u32 __iomem *regs)
{
	#ifdef __XHCI_REG_SWAP__
	__u32 val2;
	#endif
	xhci_dbg(xhci,
			"`MEM_WRITE_DWORD(3'b000, 32'h%p, 32'h%0x, 4'hf);\n",
			regs, val);
	#ifdef __XHCI_REG_SWAP__
		val2 =
			((val&0xFF000000)>>24) |
			((val&0x00FF0000)>>8 ) |
			((val&0x0000FF00)<<8 ) |
			((val&0x000000FF)<<24) ;
		writel(val2, regs);
	#else
		writel(val, regs);
	#endif
}

/*
 * Registers should always be accessed with double word or quad word accesses.
 *
 * Some xHCI implementations may support 64-bit address pointers.  Registers
 * with 64-bit address pointers should be written to with dword accesses by
 * writing the low dword first (ptr[0]), then the high dword (ptr[1]) second.
 * xHCI implementations that do not support 64-bit address pointers will ignore
 * the high dword, and write order is irrelevant.
 */
static inline u64 xhci_read_64(const struct xhci_hcd *xhci,
		__u64 __iomem *regs)
{
	__u32 __iomem *ptr = (__u32 __iomem *) regs;
	u64 val_lo = xhci_readl(xhci,(ptr+0));
	u64 val_hi = xhci_readl(xhci,(ptr+1));
	return val_lo + (val_hi << 32);
}
static inline void xhci_write_64(struct xhci_hcd *xhci,
		const u64 val, __u64 __iomem *regs)
{
	__u32 __iomem *ptr = (__u32 __iomem *) regs;
	u32 val_lo = lower_32_bits(val);
	u32 val_hi = upper_32_bits(val);

	xhci_dbg(xhci,
			"`MEM_WRITE_DWORD(3'b000, 64'h%p, 64'h%0lx, 4'hf);\n",
			regs, (long unsigned int) val);
	xhci_writel(xhci,val_lo, (ptr+0));
	xhci_writel(xhci,val_hi, (ptr+1));
}

static inline int xhci_link_trb_quirk(struct xhci_hcd *xhci)
{
	u32 temp = xhci_readl(xhci, &xhci->cap_regs->hc_capbase);
	return ((HC_VERSION(temp) == 0x95) &&
			(xhci->quirks & XHCI_LINK_TRB_QUIRK));
}

/* xHCI debugging */
void F_REN(print_ir_set)(struct xhci_hcd *xhci, struct xhci_intr_reg *ir_set, int set_num);
void F_REN(print_registers)(struct xhci_hcd *xhci);
void F_REN(dbg_regs)(struct xhci_hcd *xhci);
void F_REN(print_run_regs)(struct xhci_hcd *xhci);
void F_REN(print_trb_offsets)(struct xhci_hcd *xhci, union xhci_trb *trb);
void F_REN(debug_trb)(struct xhci_hcd *xhci, union xhci_trb *trb);
void F_REN(debug_segment)(struct xhci_hcd *xhci, struct xhci_segment *seg);
void F_REN(debug_ring)(struct xhci_hcd *xhci, struct xhci_ring *ring);
void F_REN(dbg_erst)(struct xhci_hcd *xhci, struct xhci_erst *erst);
void F_REN(dbg_cmd_ptrs)(struct xhci_hcd *xhci);
void F_REN(dbg_ring_ptrs)(struct xhci_hcd *xhci, struct xhci_ring *ring);
void F_REN(dbg_ctx)(struct xhci_hcd *xhci, struct xhci_container_ctx *ctx, unsigned int last_ep);

/* xHCI memory management */
void F_REN(mem_cleanup)(struct xhci_hcd *xhci);
int F_REN(mem_init)(struct xhci_hcd *xhci, gfp_t flags);
void F_REN(free_virt_device)(struct xhci_hcd *xhci, int slot_id);
int F_REN(alloc_virt_device)(struct xhci_hcd *xhci, int slot_id, struct usb_device *udev, gfp_t flags);
int F_REN(setup_addressable_virt_dev)(struct xhci_hcd *xhci, struct usb_device *udev);
unsigned int F_REN(get_endpoint_index)(struct usb_endpoint_descriptor *desc);
unsigned int F_REN(get_endpoint_flag)(struct usb_endpoint_descriptor *desc);
unsigned int F_REN(get_endpoint_flag_from_index)(unsigned int ep_index);
unsigned int F_REN(last_valid_endpoint)(u32 added_ctxs);
void F_REN(endpoint_zero)(struct xhci_hcd *xhci, struct xhci_virt_device *virt_dev, struct usb_host_endpoint *ep);
void F_REN(endpoint_copy)(struct xhci_hcd *xhci,
		struct xhci_container_ctx *in_ctx,
		struct xhci_container_ctx *out_ctx,
		unsigned int ep_index);
void F_REN(slot_copy)(struct xhci_hcd *xhci,
		struct xhci_container_ctx *in_ctx,
		struct xhci_container_ctx *out_ctx);
int F_REN(endpoint_init)(struct xhci_hcd *xhci, struct xhci_virt_device *virt_dev,
		struct usb_device *udev, struct usb_host_endpoint *ep,
		gfp_t mem_flags);
void F_REN(ring_free)(struct xhci_hcd *xhci, struct xhci_ring *ring);
struct xhci_command *F_REN(alloc_command)(struct xhci_hcd *xhci,
		bool allocate_completion, gfp_t mem_flags);
void F_REN(free_command)(struct xhci_hcd *xhci,
		struct xhci_command *command);

#ifdef __IS_AXI__
int xhciaxi_register_axi(void);
void xhciaxi_unregister_axi(void);
#else
#ifdef CONFIG_PCI
/* xHCI PCI glue */
int xhci_register_pci(void);
void xhci_unregister_pci(void);
#endif
#endif

/* xHCI host controller glue */
int F_REN(halt)(struct xhci_hcd *xhci);
int F_REN(reset)(struct xhci_hcd *xhci);
int F_REN(init)(struct usb_hcd *hcd);
int F_REN(run)(struct usb_hcd *hcd);
void F_REN(stop)(struct usb_hcd *hcd);
void F_REN(shutdown)(struct usb_hcd *hcd);
int F_REN(get_frame)(struct usb_hcd *hcd);
irqreturn_t F_REN(irq)(struct usb_hcd *hcd);
int F_REN(alloc_dev)(struct usb_hcd *hcd, struct usb_device *udev);
void F_REN(free_dev)(struct usb_hcd *hcd, struct usb_device *udev);
int F_REN(address_device)(struct usb_hcd *hcd, struct usb_device *udev);
int F_REN(update_hub_device)(struct usb_hcd *hcd, struct usb_device *hdev,
			struct usb_tt *tt, gfp_t mem_flags);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,32)
	int F_REN(urb_enqueue)(struct usb_hcd *hcd, struct usb_host_endpoint *ep, struct urb *urb, gfp_t mem_flags);
	int F_REN(urb_dequeue)(struct usb_hcd *hcd, struct urb *urb);
#else
	int F_REN(urb_enqueue)(struct usb_hcd *hcd, struct urb *urb, gfp_t mem_flags);
	int F_REN(urb_dequeue)(struct usb_hcd *hcd, struct urb *urb, int status);
#endif
int F_REN(add_endpoint)(struct usb_hcd *hcd, struct usb_device *udev, struct usb_host_endpoint *ep);
int F_REN(drop_endpoint)(struct usb_hcd *hcd, struct usb_device *udev, struct usb_host_endpoint *ep);
void F_REN(endpoint_reset)(struct usb_hcd *hcd, struct usb_host_endpoint *ep);
int F_REN(check_bandwidth)(struct usb_hcd *hcd, struct usb_device *udev);
void F_REN(reset_bandwidth)(struct usb_hcd *hcd, struct usb_device *udev);

/* xHCI ring, segment, TRB, and TD functions */
dma_addr_t F_REN(trb_virt_to_dma)(struct xhci_segment *seg, union xhci_trb *trb);
void F_REN(ring_cmd_db)(struct xhci_hcd *xhci);
void *F_REN(setup_one_noop)(struct xhci_hcd *xhci);
void F_REN(handle_event)(struct xhci_hcd *xhci);
void F_REN(set_hc_event_deq)(struct xhci_hcd *xhci);
int F_REN(queue_slot_control)(struct xhci_hcd *xhci, u32 trb_type, u32 slot_id);
int F_REN(queue_address_device)(struct xhci_hcd *xhci, dma_addr_t in_ctx_ptr,
		u32 slot_id);
int F_REN(queue_stop_endpoint)(struct xhci_hcd *xhci, int slot_id,
		unsigned int ep_index);
int F_REN(queue_ctrl_tx)(struct xhci_hcd *xhci, gfp_t mem_flags, struct urb *urb,
		int slot_id, unsigned int ep_index);
int F_REN(queue_bulk_tx)(struct xhci_hcd *xhci, gfp_t mem_flags, struct urb *urb,
		int slot_id, unsigned int ep_index);
int F_REN(queue_intr_tx)(struct xhci_hcd *xhci, gfp_t mem_flags, struct urb *urb,
		int slot_id, unsigned int ep_index);
int F_REN(queue_configure_endpoint)(struct xhci_hcd *xhci, dma_addr_t in_ctx_ptr,
		u32 slot_id, bool command_must_succeed);
int F_REN(queue_evaluate_context)(struct xhci_hcd *xhci, dma_addr_t in_ctx_ptr,
		u32 slot_id);
int F_REN(queue_reset_ep)(struct xhci_hcd *xhci, int slot_id,
		unsigned int ep_index);
void F_REN(find_new_dequeue_state)(struct xhci_hcd *xhci,
		unsigned int slot_id, unsigned int ep_index,
		struct xhci_td *cur_td, struct xhci_dequeue_state *state);
void F_REN(queue_new_dequeue_state)(struct xhci_hcd *xhci,
		unsigned int slot_id, unsigned int ep_index,
		struct xhci_dequeue_state *deq_state);
void F_REN(cleanup_stalled_ring)(struct xhci_hcd *xhci,
		struct usb_device *udev, unsigned int ep_index);
void F_REN(queue_config_ep_quirk)(struct xhci_hcd *xhci,
		unsigned int slot_id, unsigned int ep_index,
		struct xhci_dequeue_state *deq_state);

/* xHCI roothub code */
int F_REN(hub_control)(struct usb_hcd *hcd, u16 typeReq, u16 wValue, u16 wIndex,
		char *buf, u16 wLength);
int F_REN(hub_status_data)(struct usb_hcd *hcd, char *buf);

/* xHCI contexts */
struct xhci_input_control_ctx *F_REN(get_input_control_ctx)(struct xhci_hcd *xhci, struct xhci_container_ctx *ctx);
struct xhci_slot_ctx *F_REN(get_slot_ctx)(struct xhci_hcd *xhci, struct xhci_container_ctx *ctx);
struct xhci_ep_ctx *F_REN(get_ep_ctx)(struct xhci_hcd *xhci, struct xhci_container_ctx *ctx, unsigned int ep_index);

#endif /* __LINUX_XHCI_HCD_H */
