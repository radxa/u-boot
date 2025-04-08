
obj-y += sunxi_drm_drv.o

obj-y += sunxi_drm_crtc.o  sunxi_drm_connector.o sunxi_drm_bridge.o sunxi_drm_helper_funcs.o \
	drm_fourcc.o drm_modes.o drm_plane.o drm_edid.o drm_displayid.o drm_framebuffer.o load_file.o

ccflags-y += -I$(srctree)/$(obj)/sunxi_device/hardware/
ccflags-y += -I$(srctree)/$(obj)/sunxi_device/hardware/lowlevel_de/
ccflags-y += -I$(srctree)/$(obj)/sunxi_device/hardware/lowlevel_lcd/
ccflags-y += -I$(srctree)/$(obj)/sunxi_device/hardware/lowlevel_tcon/
ccflags-y += -I$(srctree)/$(obj)/sunxi_device/hardware/lowlevel_edp/
ccflags-y += -I$(srctree)/$(obj)/sunxi_device/hardware/lowlevel_hdmi20/
ccflags-y += -I$(srctree)/$(obj)/


#tcon
include $(srctree)/$(obj)/sunxi_device/hardware/lowlevel_tcon/Makefile
obj-y += $(tcon_obj)

#de
include $(srctree)/$(obj)/sunxi_device/hardware/lowlevel_de/Makefile
obj-$(CONFIG_AW_DRM_DE) += $(de_obj)

obj-$(CONFIG_AW_DRM_RGB) += sunxi_drm_rgb.o

obj-$(CONFIG_AW_DRM_LVDS) += sunxi_drm_lvds.o

obj-$(CONFIG_AW_DRM_DSI) += sunxi_drm_dsi.o drm_mipi_dsi.o drm_dsc_helper.o
obj-$(CONFIG_AW_DRM_DSI) += sunxi_device/hardware/lowlevel_lcd/dsi_v1.o


obj-$(CONFIG_AW_DRM_TCON) += sunxi_device/sunxi_tcon.o
obj-$(CONFIG_AW_DRM_TCON) += sunxi_device/hardware/lowlevel_lcd/tcon_lcd.o

obj-$(CONFIG_AW_DRM_TCON_TOP) += sunxi_device/sunxi_tcon_top.o

obj-$(CONFIG_AW_DRM_HDMI_TX) += sunxi_drm_hdmi.o
obj-$(CONFIG_AW_DRM_HDMI_TX) += sunxi_device/sunxi_hdmi.o
include $(srctree)/$(obj)/sunxi_device/hardware/lowlevel_hdmi20/Makefile
obj-$(CONFIG_AW_DRM_HDMI_TX) += $(obj_low_hdmi)

obj-$(CONFIG_AW_DRM_EDP) += sunxi_drm_edp.o sunxi_device/sunxi_edp.o

obj-$(CONFIG_AW_DRM_INNO_EDP13) += sunxi_device/hardware/lowlevel_edp/inno_edp13/inno_edp13.o
obj-$(CONFIG_AW_DRM_TRILINEAR_EDP14) += sunxi_device/hardware/lowlevel_edp/trilinear_dp14/trilinear_dp14.o

obj-$(CONFIG_AW_DRM_EINK200) += sunxi_drm_eink.o
obj-$(CONFIG_AW_DRM_EINK200) += sunxi_device/sunxi_eink/sunxi_eink.o \
                   sunxi_device/sunxi_eink/waveform_mgr.o \
                   sunxi_device/sunxi_eink/sunxi_eink_helper.o \
                   sunxi_device/sunxi_eink/pipeline_mgr.o \
                   sunxi_device/sunxi_eink/buffer_mgr.o \
                   sunxi_device/sunxi_eink/timing_ctrl_mgr.o \

obj-$(CONFIG_AW_DRM_PANEL) += panels/

# phy
obj-$(CONFIG_AW_DRM_PHY) += phys/
