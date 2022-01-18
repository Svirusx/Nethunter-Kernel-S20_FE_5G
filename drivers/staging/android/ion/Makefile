# SPDX-License-Identifier: GPL-2.0
subdir-ccflags-y := -Wformat
obj-$(CONFIG_ION) +=	ion.o ion-ioctl.o ion_heap.o \
			ion_page_pool.o ion_system_heap.o \
			ion_carveout_heap.o ion_chunk_heap.o \
			ion_system_secure_heap.o ion_cma_heap.o \
			ion_secure_util.o ion_cma_secure_heap.o msm/
obj-$(CONFIG_ION_RBIN_HEAP) +=	ion_rbin_heap.o
