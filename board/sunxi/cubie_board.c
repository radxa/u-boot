#include <env.h>
#include <errno.h>
#include <sunxi_gpio.h>
#include <asm-generic/gpio.h>
#include <linux/libfdt.h>
#include <sunxi_gpadc.h>
#include <dm/device.h>
#include "cubie_board.h"

unsigned int board_index = 0;
static const struct hw_info_def hw_info[] = {
	{
		.compatible = "allwinner,sun60i-a733",
		.fdtfile = "allwinner/sun60i-a733-cubie-a7z.dtb",
		.pcie_power_gpio = "PD20",
		.pcie_wake_gpio = "PD21",
		.pcie_reset_gpio = "PD22",
		.boot_led_gpio = SUNXI_GPM(2),
		.boot_led_on_value = 1,
		.hw_id_level = 1,
		.hw_id_lower_bound = 1750,
		.hw_id_upper_bound = 1850, //1800mv +/- 50mv
	},
	{
		.compatible = "allwinner,sun60i-a733",
		.fdtfile = "allwinner/sun60i-a733-cubie-a7a.dtb",
		.pcie_power_gpio = "PE11",
		.pcie_wake_gpio = "PE12",
		.pcie_reset_gpio = "PE13",
		.boot_led_gpio = SUNXI_GPJ(26),
		.boot_led_on_value = 0,
		.hw_id_level = 1,
		.hw_id_lower_bound = 1519,
		.hw_id_upper_bound = 1619, //1569mv +/- 50mv
	},
	{
		.compatible = "allwinner,sun60i-a733",
		.fdtfile = "allwinner/sun60i-a733-cubie-a7s.dtb",
		.pcie_power_gpio = "PD20",
		.pcie_wake_gpio = "PD21",
		.pcie_reset_gpio = "PD22",
		.boot_led_gpio = SUNXI_GPM(2),
		.boot_led_on_value = 1,
		.hw_id_level = 1,
		.hw_id_lower_bound = 1265,
		.hw_id_upper_bound = 1365, //1315mv +/- 50mv
	},
	{
		.compatible = "allwinner,sun60i-a733",
		.fdtfile = "allwinner/sun60i-a733-cm-a7-rpi-cm5-io.dtb",
		.pcie_power_gpio = "PD20",
		.pcie_wake_gpio = "PD21",
		.pcie_reset_gpio = "PD22",
		.boot_led_gpio = -1,
		.boot_led_on_value = 0,
		.hw_id_level = 1,
		.hw_id_lower_bound = 1000,
		.hw_id_upper_bound = 1100, //1050mv +/- 50mv
	}
};

int radxa_set_board_type(void)
{
	int i, ret, vol, level;

	sunxi_gpio_set_cfgpin(SUNXI_GPK(24), SUNXI_GPIO_INPUT);

	ret = gpio_request(HW_ID_GPIO, "hw_id_level");
	if (ret) {
		debug("Failed to request GPIO %d for hardware ID detection: %d\n",
		      HW_ID_GPIO, ret);
		return ret;
	}
	gpio_direction_input(HW_ID_GPIO);
	level = gpio_get_value(HW_ID_GPIO);
	vol = sunxi_get_gpadc_vol(HW_ID_ADC_CHANNEL);
	gpio_free(HW_ID_GPIO);

	debug("%s vol=%d, level=%d\n", __func__, vol, level);
	for (i = 0; i < countof(hw_info); i++) {
		if (of_machine_is_compatible(hw_info[i].compatible) &&
			vol >= hw_info[i].hw_id_lower_bound &&
			vol <= hw_info[i].hw_id_upper_bound &&
			level == hw_info[i].hw_id_level) {
			board_index = i;
			return 0;
		}
	}
	printf("No compatible board type found for vol=%d, level=%d\n", vol, level);
	return -ENODEV;
}

int radxa_fixup_pcie_gpios(void *fdt)
{
	const struct hw_info_def *info = &hw_info[board_index];
	const char *const names[] = {
		info->pcie_power_gpio, info->pcie_reset_gpio, info->pcie_wake_gpio,
	};
	static const char *const props[] = {
		"pwr-gpios", "rst-gpios", "wake-gpios",
	};
	fdt32_t cells[ARRAY_SIZE(props)][4];
	const fdt32_t *prop;
	unsigned long pin;
	char *end;
	int node, len, ret, i;

	node = fdt_node_offset_by_compatible(fdt, -1,
					     "allwinner,sunxi-pcie-v300-rc");
	if (node < 0)
		return node;

	for (i = 0; i < ARRAY_SIZE(props); i++) {
		const char *name = names[i];

		if (!name || name[0] != 'P' || name[1] < 'A' ||
		    name[1] > 'K' || !name[2])
			return -EINVAL;
		pin = dectoul(name + 2, &end);
		if (*end || pin >= 32)
			return -EINVAL;

		prop = fdt_getprop(fdt, node, props[i], &len);
		if (!prop)
			return len;
		if (len != sizeof(cells[i]))
			return -FDT_ERR_BADVALUE;
		memcpy(cells[i], prop, sizeof(cells[i]));
		/* DM GPIO numbers are dynamic; DT cells need the physical bank/pin. */
		cells[i][1] = cpu_to_fdt32(name[1] - 'A');
		cells[i][2] = cpu_to_fdt32(pin);
	}

	for (i = 0; i < ARRAY_SIZE(props); i++) {
		ret = fdt_setprop_inplace(fdt, node, props[i], cells[i],
					  sizeof(cells[i]));
		if (ret)
			return ret;
	}

	return 0;
}

void radxa_enable_boot_led(void)
{
	const struct hw_info_def *info = &hw_info[board_index];
	int ret;

	if (info->boot_led_gpio < 0)
		return;

	ret = gpio_request(info->boot_led_gpio, "boot_led");
	if (ret) {
		printf("Failed to request boot LED GPIO %d: %d\n",
		       info->boot_led_gpio, ret);
		return;
	}

	ret = gpio_direction_output(info->boot_led_gpio,
				    info->boot_led_on_value);
	if (ret)
		printf("Failed to enable boot LED GPIO %d: %d\n",
		       info->boot_led_gpio, ret);

	gpio_free(info->boot_led_gpio);
}

void radxa_set_compat_fdt(void)
{
	env_set("fdtfile", hw_info[board_index].fdtfile);
	printf("Override default fdtfile to %s\n", hw_info[board_index].fdtfile);
	return;
}

struct hw_info_def radxa_get_hw_info(void)
{
	return hw_info[board_index];
}
