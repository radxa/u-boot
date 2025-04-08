/*
 * Copyright (C) 2019 Allwinner.
 * weidonghui <weidonghui@allwinnertech.com>
 *
 * SUNXI PMU_GENERAL Driver
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <sunxi_power/pmu_general.h>

/* private function */
char *pmu_general_name[NR_PMU_GENERAL_TYPE_MAX - 2];
int pmu_general_index[NR_PMU_GENERAL_TYPE_MAX - 1];

static struct sunxi_pmu_general_dev_t *pmu_general_get_dev_t(int pmu_type)
{
	struct sunxi_pmu_general_dev_t *sunxi_pmu_general_dev_temp = NULL;
	struct sunxi_pmu_general_dev_t *sunxi_pmu_general_dev_start =
		ll_entry_start(struct sunxi_pmu_general_dev_t, pmu_general);
	int max = ll_entry_count(struct sunxi_pmu_general_dev_t, pmu_general);
	int index;

	if ((pmu_type - 2) >= max)
		return NULL;

	index = pmu_general_index[pmu_type - 1];
	sunxi_pmu_general_dev_temp = sunxi_pmu_general_dev_start + index;

	return sunxi_pmu_general_dev_temp;
}

int _pmu_general_set_voltage(int pmu_type, char *name, uint vol_value, uint onoff)
{
	struct sunxi_pmu_general_dev_t *sunxi_pmu_general_dev_temp = pmu_general_get_dev_t(pmu_type);

	if ((sunxi_pmu_general_dev_temp) && (sunxi_pmu_general_dev_temp->set_voltage))
		return sunxi_pmu_general_dev_temp->set_voltage(name, vol_value, onoff);
	pmu_err("not imple:%s\n", __func__);

	return -1;
}

int _pmu_general_get_voltage(int pmu_type, char *name)
{
	struct sunxi_pmu_general_dev_t *sunxi_pmu_general_dev_temp = pmu_general_get_dev_t(pmu_type);

	if ((sunxi_pmu_general_dev_temp) && (sunxi_pmu_general_dev_temp->get_voltage))
		return sunxi_pmu_general_dev_temp->get_voltage(name);
	pmu_err("not imple:%s\n", __func__);

	return -1;
}

static int pmu_general_get_dev_index(void)
{
	struct sunxi_pmu_general_dev_t *sunxi_pmu_general_dev_temp;
	struct sunxi_pmu_general_dev_t *sunxi_pmu_general_dev_start =
		ll_entry_start(struct sunxi_pmu_general_dev_t, pmu_general);
	int max = ll_entry_count(struct sunxi_pmu_general_dev_t, pmu_general);
	int index = 0, index_exist = 0;

	for (sunxi_pmu_general_dev_temp = sunxi_pmu_general_dev_start;
	     sunxi_pmu_general_dev_temp != sunxi_pmu_general_dev_start + max;
	     sunxi_pmu_general_dev_temp++) {
		if (!strncmp("pmu", sunxi_pmu_general_dev_temp->pmu_general_name, 3)) {
			if (!sunxi_pmu_general_dev_temp->probe()) {
				pr_info("PMU_GENERAL: %s found\n",
				       sunxi_pmu_general_dev_temp->pmu_general_name);
				pmu_general_name[index_exist] = (char *)sunxi_pmu_general_dev_temp->pmu_general_name;
				pmu_general_index[index_exist + 1] = index;
				index_exist++;
			}
		}
		index++;
	}

	pmu_general_index[0] = index_exist;

	if (!index_exist)
		pr_info("PMU_GENERAL: no found\n");

	return index_exist;
}

unsigned int _pmic_general_set_i2c(unsigned int i2c_bus)
{
	int twi_bus_num;
	twi_bus_num = i2c_get_bus_num();
	i2c_set_bus_num(i2c_bus);

	return twi_bus_num;
}

int pmic_general_bus_init(u16 device_addr, u16 runtime_addr)
{
	return 0;
}

int pmic_general_bus_read(unsigned int i2c_bus, u16 runtime_addr, u8 reg, u8 *data)
{
	int old_twi_bus_num, ret = 0;

	old_twi_bus_num = _pmic_general_set_i2c(i2c_bus);
	ret = pmic_bus_read(runtime_addr, reg, data);
	i2c_set_bus_num(old_twi_bus_num);

	return ret;
}

int pmic_general_bus_write(unsigned int i2c_bus, u16 runtime_addr, u8 reg, u8 data)
{
	int old_twi_bus_num, ret = 0;

	old_twi_bus_num = _pmic_general_set_i2c(i2c_bus);
	ret = pmic_bus_write(runtime_addr, reg, data);
	i2c_set_bus_num(old_twi_bus_num);

	return ret;
}

int pmic_general_bus_setbits(unsigned int i2c_bus, u16 runtime_addr, u8 reg, u8 bits)
{
	int old_twi_bus_num, ret = 0;

	old_twi_bus_num = _pmic_general_set_i2c(i2c_bus);
	ret = pmic_bus_setbits(runtime_addr, reg, bits);
	i2c_set_bus_num(old_twi_bus_num);

	return ret;
}


int pmic_general_bus_clrbits(unsigned int i2c_bus, u16 runtime_addr, u8 reg, u8 bits)
{
	int old_twi_bus_num, ret = 0;

	old_twi_bus_num = _pmic_general_set_i2c(i2c_bus);
	ret = pmic_bus_clrbits(runtime_addr, reg, bits);
	i2c_set_bus_num(old_twi_bus_num);

	return ret;
}

/* public function */
/* matches chipid*/
int pmu_general_probe(void)
{
	int index_exist;

	index_exist = pmu_general_get_dev_index();

	if (!index_exist)
		return -1;

	return 0;
}

/* get pmu_general exist */
bool pmu_general_get_exist(void)
{
	if (pmu_general_index[0])
		return true;

	return false;
}

/* get pmu_general type */
int pmu_general_get_type_by_name(char *name)
{
	char *pmu_name, *dash;
	int index;

	pmu_name = (char *)pmu_get_name();
	if (pmu_name) {
		dash = strchr(pmu_name, '_');
		if (dash == NULL)
			return -1;
		dash++;
		if (!strncmp(dash, name, strlen(dash)))
			return PMU_NORMAL;
	}

#ifdef CONFIG_SUNXI_PMU_EXT
	pmu_name = (char *)pmu_ext_get_name();
	if (pmu_name) {
		dash = strchr(pmu_name, '_');
		if (dash == NULL)
			return -1;
		dash++;
		if (!strncmp(dash, name, strlen(dash)))
			return PMU_EXT;
	}
#endif

	if (!pmu_general_get_exist())
		return -1;

	for (index = 0; index < (NR_PMU_GENERAL_TYPE_MAX - 2); index++) {
		dash = strchr(pmu_general_name[index + 1], '_');
		if (!strncmp(dash, name, strlen(dash)))
			return index + 2;
	}

	return -1;
}

char *pmu_general_get_type_by_full_name(char *name, int *pmu_type)
{
	char *result;

	result = strchr(name, '-');
	if (result == NULL) {
		*pmu_type = -1;
		return NULL;
	}

	result++;

	*pmu_type = pmu_general_get_type_by_name(name);

	return result;
}

char *pmu_general_get_type_by_phandle(const void *fdt, uint32_t phandle, int *pmu_type)
{
	int offset, len = -1;
	const char *property = "regulator-name";
	char *result;
	char *name = NULL;

	offset = fdt_node_offset_by_phandle(fdt, phandle);
	if (offset <= 0) {
		tick_printf("invalid power phandle, ret=%d\n", offset);
		return NULL;
	}
	result = (char *)fdt_get_name(fdt, offset, NULL);
	if (!result) {
		tick_printf("invalid power phandle, name not found\n");
		return NULL;
	}
	name = (char *)fdt_getprop(fdt, offset, property, &len);
	if (!name) {
		tick_printf("invalid power phandle, regulator-name not found\n");
		return NULL;
	}

	*pmu_type = pmu_general_get_type_by_name(name);

	return result;
}

int pmu_general_set_voltage(char *name, int pmu_type, uint vol_value, uint onoff)
{
	if (pmu_type < 0)
		return -1;

	switch (pmu_type) {
	case PMU_NORMAL:
		return pmu_set_voltage(name, vol_value, onoff);
		break;
#ifdef CONFIG_SUNXI_PMU_EXT
	case PMU_EXT:
		return pmu_ext_set_voltage(name, vol_value, onoff);
		break;
#endif
	default:
		return _pmu_general_set_voltage(pmu_type, name, vol_value, onoff);
		break;
	}

	return -1;
}

/*Set a certain power, voltage value. */
int pmu_general_set_voltage_by_phandle(const void *fdt, uint32_t phandle, uint vol_value, uint onoff)
{
	int pmu_type;
	char *result;

	result = pmu_general_get_type_by_phandle(working_fdt, phandle, &pmu_type);

	return pmu_general_set_voltage(result, pmu_type, vol_value, onoff);
}

int pmu_general_set_voltage_by_full_name(char *name, uint vol_value, uint onoff)
{
	int pmu_type;
	char *result;

	result = pmu_general_get_type_by_full_name(name, &pmu_type);

	return pmu_general_set_voltage(result, pmu_type, vol_value, onoff);
}

/*Read a certain power, voltage value */

int pmu_general_get_voltage(char *name, int pmu_type)
{
	if (pmu_type < 0)
		return -1;

	switch (pmu_type) {
	case PMU_NORMAL:
		return pmu_get_voltage(name);
		break;
#ifdef CONFIG_SUNXI_PMU_EXT
	case PMU_EXT:
		return pmu_ext_get_voltage(name);
		break;
#endif
	default:
		return _pmu_general_get_voltage(pmu_type, name);
		break;
	}

	return -1;
}

/*get a certain power, voltage value. */
int pmu_general_get_voltage_by_phandle(const void *fdt, uint32_t phandle)
{
	int pmu_type;
	char *result;

	result = pmu_general_get_type_by_phandle(working_fdt, phandle, &pmu_type);

	return pmu_general_get_voltage(result, pmu_type);
}

int pmu_general_get_voltage_by_full_name(char *name)
{
	int pmu_type;
	char *result;

	result = pmu_general_get_type_by_full_name(name, &pmu_type);

	return pmu_general_get_voltage(result, pmu_type);
}
