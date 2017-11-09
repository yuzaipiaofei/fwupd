pkidir = $(sysconfdir)/pki/fwupd
dist_pki_DATA =

if ENABLE_GPG
dist_pki_DATA +=							\
	GPG-KEY-Hughski-Limited						\
	GPG-KEY-Linux-Vendor-Firmware-Service
endif

if ENABLE_PKCS7
dist_pki_DATA +=							\
	LVFS-CA.pem
endif

pkimetadatadir = $(sysconfdir)/pki/fwupd-metadata
dist_pkimetadata_DATA =

if ENABLE_GPG
dist_pkimetadata_DATA +=						\
	GPG-KEY-Linux-Vendor-Firmware-Service
endif

if ENABLE_PKCS7
dist_pkimetadata_DATA +=						\
	LVFS-CA.pem
endif

-include $(top_srcdir)/git.mk
