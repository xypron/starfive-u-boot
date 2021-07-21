// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2021, Heinrich Schuchardt <xypron.glpk@gmx.de>
 */

#include <common.h>
#include <dm.h>
#include <errno.h>
#include <efi_loader.h>
#include <log.h>
#include <sysreset.h>
#include <asm/sbi.h>

static long __efi_runtime_data have_reset;

static int sbi_sysreset_request(struct udevice *dev, enum sysreset_t type)
{
	enum sbi_srst_reset_type reset_type;

	switch (type) {
	case SYSRESET_WARM:
		reset_type = SBI_SRST_RESET_TYPE_WARM_REBOOT;
		break;
	case SYSRESET_COLD:
		reset_type = SBI_SRST_RESET_TYPE_COLD_REBOOT;
		break;
	case SYSRESET_POWER_OFF:
		reset_type = SBI_SRST_RESET_TYPE_SHUTDOWN;
		break;
	default:
		log_err("SBI has no system reset extension\n");
		return -ENOSYS;
	}

	sbi_srst_reset(reset_type, SBI_SRST_RESET_REASON_NONE);

	return -EINPROGRESS;
}

efi_status_t efi_reset_system_init(void)
{
	return EFI_SUCCESS;
}

void __efi_runtime EFIAPI efi_reset_system(enum efi_reset_type type,
					   efi_status_t reset_status,
					   unsigned long data_size,
					   void *reset_data)
{
	enum sbi_srst_reset_type reset_type;
	enum sbi_srst_reset_reason reset_reason;

	if (have_reset)
		switch (type) {
		case SYSRESET_COLD:
			reset_type = SBI_SRST_RESET_TYPE_COLD_REBOOT;
			break;
		case SYSRESET_POWER_OFF:
			reset_type = SBI_SRST_RESET_TYPE_SHUTDOWN;
			break;
		default:
			reset_type = SBI_SRST_RESET_TYPE_WARM_REBOOT;
			break;
	}

	if (reset_status == EFI_SUCCESS)
		reset_reason = SBI_SRST_RESET_REASON_NONE;
	else
		reset_reason = SBI_SRST_RESET_REASON_SYS_FAILURE;

	sbi_srst_reset(reset_type, reset_reason);

	while (1)
		;
}

static int sbi_sysreset_probe(struct udevice *dev)
{
	have_reset = sbi_probe_extension(SBI_EXT_SRST);
	if (have_reset)
		return 0;

	log_warning("SBI has no system reset extension\n");
	return -ENOENT;
}

static struct sysreset_ops sbi_sysreset_ops = {
	.request = sbi_sysreset_request,
};

U_BOOT_DRIVER(sbi_sysreset) = {
	.name = "sbi-sysreset",
	.id = UCLASS_SYSRESET,
	.ops = &sbi_sysreset_ops,
	.probe = sbi_sysreset_probe,
};
