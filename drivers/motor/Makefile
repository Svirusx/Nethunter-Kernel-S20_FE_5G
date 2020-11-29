#
#  makefile for motor driver kernel module
#

subdir-ccflags-y := -Wformat
obj-$(CONFIG_MOTOR_DRV_MAX77705)	+= max77705_haptic.o
obj-$(CONFIG_SS_VIBRATOR)		+= ss_vibrator.o
obj-$(CONFIG_ANDROID_TIMED_OUTPUT)	+= timed_output.o
obj-$(CONFIG_MOTOR_DRV_CS40L2X)		+= cs40l2x.o cs40l2x-tables.o
obj-$(CONFIG_MSM_VIBRATOR) 		+= msm_vibrator.o
obj-$(CONFIG_VIB_NOTIFIER)		+= vib_notifier.o
