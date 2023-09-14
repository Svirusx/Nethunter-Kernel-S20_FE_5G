#if defined __ARM_ARCH && (__ARM_ARCH == 6 || __ARM_ARCH == 7)

// Enable IRQ interrupts
// Refer: 10.117 (__enable_irq intrinsic) on http://infocenter.arm.com
static __inline__ void __attribute__((__always_inline__, __nodebug__))
__enable_irq() {

  __asm__("cpsie i");
}

// Disable IRQ interrupts
// Refer: 10.113 (__disable_irq intrinsic) on http://infocenter.arm.com
static __inline__ uint32_t __attribute__((__always_inline__, __nodebug__))
__disable_irq() {

  __asm__("cpsid i");
}

// Refer: 10.110 __current_pc intrinsic on http://infocenter.arm.com
static __inline__ uint32_t __attribute__((__always_inline__, __nodebug__))
__current_pc() {
  return __builtin_arm_current_pc();
}

// Refer: 10.111 __current_sp intrinsic on http://infocenter.arm.com
static __inline__ uint32_t __attribute__((__always_inline__, __nodebug__))
__current_sp() {
  return __builtin_arm_current_sp();
}

#endif
