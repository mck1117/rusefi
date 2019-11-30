/*
 * @file chconf_common.h
 *
 * @date Apr 20, 2019
 * @author Andrey Belomutskiy, (c) 2012-2019
 */

#ifndef CONFIG_CHCONF_COMMON_H_
#define CONFIG_CHCONF_COMMON_H_

#if !defined(_FROM_ASM_)
#include "obd_error_codes.h"
#endif /* _FROM_ASM_ */


#ifndef ENABLE_PERF_TRACE
// looks like this value could not be defined in efifeatures.h - please define either externally or just change the value here
 #define ENABLE_PERF_TRACE FALSE
#endif /* ENABLE_PERF_TRACE */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
#ifndef __ASSEMBLER__
  void firmwareError(obd_code_e code, const char *fmt, ...);
  #if ENABLE_PERF_TRACE
    void irqEnterHook(void);
    void irqExitHook(void);
    void contextSwitchHook(void);
    void onLockHook(void);
    void onUnlockHook(void);
  #else /* ENABLE_PERF_TRACE */
    #define irqEnterHook() {}
    #define irqExitHook() {}
    #define contextSwitchHook() {}
    #define onLockHook() {}
    #define onUnlockHook() {}
  #endif /* ENABLE_PERF_TRACE */
#endif /* __ASSEMBLER__ */
#ifdef __cplusplus
}
#endif /* __cplusplus */

#define ON_LOCK_HOOK {onLockHook();}
#define ON_UNLOCK_HOOK {onUnlockHook();}

#if !defined(_FROM_ASM_)
#ifdef __cplusplus
extern "C" {
#endif
  void boardInit(void);
  void setBoardConfigurationOverrides(void);
  void setPinConfigurationOverrides(void);
  void setSerialConfigurationOverrides(void);
  void setSdCardConfigurationOverrides(void);
  void setAdcChannelOverrides(void);
#ifdef __cplusplus
}
#endif
#endif /* _FROM_ASM_ */


/**
 * @brief   Context switch hook.
 * @details This hook is invoked just before switching between threads.
 */
#define CH_CFG_CONTEXT_SWITCH_HOOK(ntp, otp) {                              \
  contextSwitchHook();                                            \
}

/**
 * @brief   ISR enter hook.
 */
#define CH_CFG_IRQ_PROLOGUE_HOOK() {                                        \
  /* IRQ prologue code here.*/                                              \
  irqEnterHook();                                                           \
}

/**
 * @brief   ISR exit hook.
 */
#define CH_CFG_IRQ_EPILOGUE_HOOK() {                                        \
  /* IRQ epilogue code here.*/                                              \
  irqExitHook();                                                            \
}

#endif /* CONFIG_CHCONF_COMMON_H_ */
