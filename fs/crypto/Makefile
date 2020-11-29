obj-$(CONFIG_FS_ENCRYPTION)	+= fscrypto.o

ccflags-y += -Ifs/ext4
ccflags-y += -Ifs/f2fs

fscrypto-y := crypto.o fname.o hooks.o keyinfo.o policy.o
fscrypto-$(CONFIG_BLOCK) += bio.o
fscrypto-$(CONFIG_DDAR) += fscrypt_ddar.o
fscrypto-$(CONFIG_FSCRYPT_SDP) += sdp/sdp_cache.o sdp/sdp_dek.o sdp/sdp_ioctl.o sdp/sdp_name.o sdp/sdp_xattr.o sdp/sdp_crypto.o
fscrypto-$(CONFIG_PFK) += fscrypt_ice.o
