dtb-$(CONFIG_DTB_BAIKAL)		+= baikal.dtb
dtb-$(CONFIG_DTB_BAIKAL_BFK)		+= baikal_bfk.dtb
dtb-$(CONFIG_DTB_BAIKAL_BFK2)		+= baikal_bfk2.dtb
dtb-$(CONFIG_DTB_TP_MITX2)		+= tplatforms_mitx2.dtb
dtb-$(CONFIG_DTB_TP_PM_NIIR)		+= tplatforms_pm_niir.dtb
dtb-$(CONFIG_DTB_TP_MSBT2)		+= tplatforms_msbt2.dtb
dtb-$(CONFIG_DTB_BAIKAL_FPGA)		+= baikal_fpga.dtb
dtb-$(CONFIG_DTB_BAIKAL_QEMU)		+= baikal_qemu.dtb
dtb-$(CONFIG_DTB_BAIKAL_MODULE)		+= baikal_module.dtb
dtb-$(CONFIG_BCM3384)			+= bcm93384wvg.dtb
dtb-$(CONFIG_CAVIUM_OCTEON_SOC)		+= octeon_3xxx.dtb octeon_68xx.dtb
dtb-$(CONFIG_DT_EASY50712)		+= easy50712.dtb
dtb-$(CONFIG_DT_XLP_EVP)		+= xlp_evp.dtb
dtb-$(CONFIG_DT_XLP_SVP)		+= xlp_svp.dtb
dtb-$(CONFIG_DT_XLP_FVP)		+= xlp_fvp.dtb
dtb-$(CONFIG_DT_XLP_GVP)		+= xlp_gvp.dtb
dtb-$(CONFIG_DTB_RT2880_EVAL)		+= rt2880_eval.dtb
dtb-$(CONFIG_DTB_RT305X_EVAL)		+= rt3052_eval.dtb
dtb-$(CONFIG_DTB_RT3883_EVAL)		+= rt3883_eval.dtb
dtb-$(CONFIG_DTB_MT7620A_EVAL)		+= mt7620a_eval.dtb
dtb-$(CONFIG_MIPS_SEAD3)		+= sead3.dtb

obj-y		+= $(patsubst %.dtb, %.dtb.o, $(dtb-y))

targets		+= dtbs
targets		+= $(dtb-y)

dtbs: $(addprefix $(obj)/, $(dtb-y))

clean-files	+= *.dtb *.dtb.S
