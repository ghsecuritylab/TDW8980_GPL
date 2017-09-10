/******************************************************************************

                               Copyright (c) 2013
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/
/* $Id$ */

#if !defined(SAFE_PLACE_TO_INCLUDE_MTLK_PCIG3_CCR_DEFS)
#error "You shouldn't include this file directly!"
#endif /* SAFE_PLACE_TO_INCLUDE_MTLK_PCIG3_CCR_DEFS */
#undef SAFE_PLACE_TO_INCLUDE_MTLK_PCIG3_CCR_DEFS

static __INLINE int
_mtlk_pcig3_ccr_init(_mtlk_pcig3_ccr_t *pcig3_mem, mtlk_hw_bus_t *bus,
                     void* bar0, void* bar1) 
{
  MTLK_ASSERT(NULL != pcig3_mem);

  pcig3_mem->hrc = (struct pci_hrc_regs *)  bar0;
  pcig3_mem->pas = (struct g3_pas_map *)    bar1;
  pcig3_mem->bus = bus;

  return ((NULL != bar0) && (NULL != bar1)) 
    ? MTLK_ERR_OK : MTLK_ERR_PARAMS;
}

static __INLINE  void
_mtlk_pcig3_ccr_cleanup(_mtlk_pcig3_ccr_t *pcig3_mem)
{
  MTLK_ASSERT(NULL != pcig3_mem);
  MTLK_UNREFERENCED_PARAM(pcig3_mem);
}

#ifdef MTCFG_TSF_TIMER_TIMESTAMPS_IN_DEBUG_PRINTOUTS
static __INLINE void
_mtlk_pcig3_read_hw_timestamp(_mtlk_pcig3_ccr_t *pcig3_mem, uint32 *low, uint32 *high)
{
  MTLK_ASSERT(NULL != pcig3_mem);

  __ccr_readl(pcig3_mem->bus, &pcig3_mem->pas->tsf_timer_low, *low);
  __ccr_readl(pcig3_mem->bus, &pcig3_mem->pas->tsf_timer_high, *high);
}
#endif /* MTCFG_TSF_TIMER_TIMESTAMPS_IN_DEBUG_PRINTOUTS */

static __INLINE  void
_mtlk_pcig3_enable_interrupts(_mtlk_pcig3_ccr_t *pcig3_mem)
{
  MTLK_ASSERT(NULL != pcig3_mem);
  __ccr_writel(pcig3_mem->bus, 
               (HWI_MASK_local_hrt_to_host << HWI_OFFSET_local_hrt_to_host) |
               (HWI_MASK_global << HWI_OFFSET_global),
               &pcig3_mem->hrc->HWI_ADDR_host_interrupt_enable);
}

static __INLINE  void
_mtlk_pcig3_disable_interrupts(_mtlk_pcig3_ccr_t *pcig3_mem)
{
  MTLK_ASSERT(NULL != pcig3_mem);
  __ccr_writel(pcig3_mem->bus, 0, 
               &pcig3_mem->hrc->HWI_ADDR_host_interrupt_enable);
}

static __INLINE BOOL
_mtlk_pcig3_clear_interrupts_if_pending(_mtlk_pcig3_ccr_t *pcig3_mem)
{
  uint32 v;
  MTLK_ASSERT(NULL != pcig3_mem);
  
  __ccr_readl(pcig3_mem->bus, &pcig3_mem->hrc->HWI_ADDR_host_interrupt_active, v);
  v &= HWI_MASK_local_hrt_to_host << HWI_OFFSET_local_hrt_to_host;
  __ccr_writel(pcig3_mem->bus, v, &pcig3_mem->hrc->HWI_ADDR_host_interrupt_status);

  return v != 0;
}

static __INLINE BOOL
_mtlk_pcig3_disable_interrupts_if_pending(_mtlk_pcig3_ccr_t *pcig3_mem)
{
  uint32 v;
  MTLK_ASSERT(NULL != pcig3_mem);


  __ccr_readl(pcig3_mem->bus, &pcig3_mem->hrc->HWI_ADDR_host_interrupt_active, v);
  if ((v & (HWI_MASK_global << HWI_OFFSET_global)) == 0)
    return FALSE;

  __ccr_writel(pcig3_mem->bus, 
               v & ~(HWI_MASK_global << HWI_OFFSET_global), 
               &pcig3_mem->hrc->HWI_ADDR_host_interrupt_enable);
  return TRUE;
}

static __INLINE int
_mtlk_pcig3_handle_interrupt(_mtlk_pcig3_ccr_t *pcig3_mem)
{
  MTLK_ASSERT(NULL != pcig3_mem);

  return mtlk_pci_handle_interrupt(pcig3_mem->bus);
}

static __INLINE  void
_mtlk_pcig3_initiate_doorbell_inerrupt(_mtlk_pcig3_ccr_t *pcig3_mem)
{
  MTLK_ASSERT(NULL != pcig3_mem);
  __ccr_writel(pcig3_mem->bus, 1, &pcig3_mem->hrc->HWI_ADDR_host_to_local_doorbell_interrupt);
}

static __INLINE  void
_mtlk_pcig3_enable_xo(_mtlk_pcig3_ccr_t *pcig3_mem)
{
  MTLK_ASSERT(NULL != pcig3_mem);
  /* No enable XO is needed on PCI Gen3 */
  MTLK_UNREFERENCED_PARAM(pcig3_mem);
}

static __INLINE  void
_mtlk_pcig3_disable_xo(_mtlk_pcig3_ccr_t *pcig3_mem)
{
  MTLK_ASSERT(NULL != pcig3_mem);
  /* No disable XO is needed on PCI Gen3 */
  MTLK_UNREFERENCED_PARAM(pcig3_mem);
}

static __INLINE  void
_mtlk_pcig3_boot_from_bus(_mtlk_pcig3_ccr_t *pcig3_mem)
{
  MTLK_ASSERT(NULL != pcig3_mem);

  __ccr_setl(pcig3_mem->bus,
             &pcig3_mem->hrc->HWI_ADDR_general_purpose_control,
             HWI_MASK_cpu_control_pci_cpu_mode);
}

static __INLINE  void
_mtlk_pcig3_clear_boot_from_bus(_mtlk_pcig3_ccr_t *pcig3_mem)
{
  MTLK_ASSERT(NULL != pcig3_mem);

  __ccr_resetl(pcig3_mem->bus,
               &pcig3_mem->hrc->HWI_ADDR_general_purpose_control,
               HWI_MASK_cpu_control_pci_cpu_mode);
}

static __INLINE  void
_mtlk_pcig3_switch_to_iram_boot(_mtlk_pcig3_ccr_t *pcig3_mem)
{
  MTLK_ASSERT(NULL != pcig3_mem);

  /* According to spec this operation requires system to be in PCI boot mode */
#ifdef MTLK_DEBUG
  {
    int res;
    __ccr_issetl(pcig3_mem->bus,
                 &pcig3_mem->hrc->HWI_ADDR_general_purpose_control,
                 HWI_MASK_cpu_control_pci_cpu_mode, res);
    MTLK_ASSERT(res);

  }
#endif

  __ccr_writel(pcig3_mem->bus, 0, &pcig3_mem->hrc->HWI_ADDR_cpu_control);
}

static __INLINE  void
_mtlk_pcig3_exit_debug_mode(_mtlk_pcig3_ccr_t *pcig3_mem)
{
  MTLK_ASSERT(NULL != pcig3_mem);

  /* According to spec this operation requires system to be in PCI boot mode */
#ifdef MTLK_DEBUG
  {
    int res;
    __ccr_issetl(pcig3_mem->bus,
                 &pcig3_mem->hrc->HWI_ADDR_general_purpose_control,
                 HWI_MASK_cpu_control_pci_cpu_mode, res);
    MTLK_ASSERT(res);

  }
#endif

  __ccr_writel(pcig3_mem->bus, 0, &pcig3_mem->hrc->HWI_ADDR_cpu_control);
}

static __INLINE  void
_mtlk_pcig3_switch_clock(_mtlk_pcig3_ccr_t *pcig3_mem)
{
  MTLK_ASSERT(NULL != pcig3_mem);

  /* Not needed on PCI Gen3 */
  MTLK_UNREFERENCED_PARAM(pcig3_mem);
}

static __INLINE  void
_mtlk_pcig3_unswitch_clock(_mtlk_pcig3_ccr_t *pcig3_mem)
{
  MTLK_ASSERT(NULL != pcig3_mem);

  /* Not needed on PCI Gen3 */
  MTLK_UNREFERENCED_PARAM(pcig3_mem);
}

/*
* HWI_ADDR_cpu_control:
* Bit [0] = CPU power on reset (default = 0x1). If boot mode = PCI, this
* bit is used to override the boot mode and change it to ?Boot from IRAM?.
* Bit [1] = Upper CPU reset (default = 0x0). When this bit is set, and
* boot mode is PCI (regardless of the state of bit[0] in this register),
* It will release the Upper CPU reset. Default value = 0x0 = CPU in reset.
* Bit [2] = Lower CPU reset (default = 0x0). Operation mode of this bit is
* the same as the previous one, with one change ? it controls the Lower
* CPU.
*/

static __INLINE  void
_mtlk_pcig3_put_cpus_to_reset(_mtlk_pcig3_ccr_t *pcig3_mem)
{
  MTLK_ASSERT(NULL != pcig3_mem);

  __ccr_writel(pcig3_mem->bus, 0, &pcig3_mem->hrc->HWI_ADDR_cpu_control);

  /* CPU requires time to go to  reset, so we       */
  /* MUST wait here before writing something else   */
  /* to CPU control register. In other case this    */
  /* may lead to unpredictable results.             */
  mtlk_osal_msleep(20);
}

static __INLINE  void
_mtlk_pcig3_power_on_cpus(_mtlk_pcig3_ccr_t *pcig3_mem)
{
  MTLK_ASSERT(NULL != pcig3_mem);

  __ccr_writel(pcig3_mem->bus, HWI_MASK_cpu_power_on_reset << HWI_OFFSET_cpu_power_on_reset,
               &pcig3_mem->hrc->HWI_ADDR_cpu_control);
}

static __INLINE  void
_mtlk_pcig3_release_ucpu_reset(_mtlk_pcig3_ccr_t *pcig3_mem)
{
  MTLK_ASSERT(NULL != pcig3_mem);

  __ccr_setl(pcig3_mem->bus, &pcig3_mem->hrc->HWI_ADDR_cpu_control, 
             HWI_MASK_upper_cpu_reset << HWI_OFFSET_upper_cpu_reset);

  /* CPU requires time to exit from reset, so we    */
  /* MUST wait here before writing something else   */
  /* to CPU control register. In other case this    */
  /* may lead to unpredictable results.             */
  mtlk_osal_msleep(10);
}

static __INLINE  void
_mtlk_pcig3_release_lcpu_reset(_mtlk_pcig3_ccr_t *pcig3_mem)
{
  MTLK_ASSERT(NULL != pcig3_mem);

  __ccr_setl(pcig3_mem->bus, &pcig3_mem->hrc->HWI_ADDR_cpu_control, 
            HWI_MASK_lower_cpu_reset << HWI_OFFSET_lower_cpu_reset);

  /* CPU requires time to exit from reset, so we    */
  /* MUST wait here before writing something else   */
  /* to CPU control register. In other case this    */
  /* may lead to unpredictable results.             */
  mtlk_osal_msleep(10);
}

static __INLINE  void
_mtlk_pcig3_release_cpus_reset(_mtlk_pcig3_ccr_t *pcig3_mem)
{
  _mtlk_pcig3_release_ucpu_reset(pcig3_mem);
  _mtlk_pcig3_release_lcpu_reset(pcig3_mem);
}

static __INLINE BOOL
_mtlk_pcig3_check_bist(_mtlk_pcig3_ccr_t *pcig3_mem, uint32 *bist_result)
{
  MTLK_ASSERT(NULL != pcig3_mem);

  __ccr_writel(pcig3_mem->bus, G3_VAL_START_BIST, &pcig3_mem->pas->HTEXT.start_bist);
  mtlk_osal_msleep(20);

  __ccr_readl(pcig3_mem->bus, &pcig3_mem->hrc->HWI_ADDR_general_purpose_status, *bist_result);

  return ((*bist_result & G3_BIST_SUCCESS_MASK) == G3_BIST_SUCCESS_MASK);
}

static __INLINE void
_mtlk_pcig3_release_ctl_from_reset(_mtlk_pcig3_ccr_t *pcig3_mem)
{
  MTLK_ASSERT(NULL != pcig3_mem);

  __mtlk_g3_release_ctl_from_reset_phase1(pcig3_mem->bus, pcig3_mem->pas);
  __mtlk_g3_release_ctl_from_reset_phase2(pcig3_mem->bus, pcig3_mem->pas);
}
