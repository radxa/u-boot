// SPDX-License-Identifier: GPL-2.0+
/*
 * U-Boot MPQ8655 regulator driver.
 *
 * One UCLASS_REGULATOR device represents the MPQ8655 I2C node. During bind,
 * its OF node is switched to the nested regulator node so the regulator
 * uclass parses its constraints and runs the normal regulator_autoset() path.
 */

#include <dm.h>
#include <linux/compiler.h>
#include <dm/device.h>
#include <dm/device_compat.h>
#include <errno.h>
#include <i2c.h>
#include <log.h>
#include <power/regulator.h>

DECLARE_GLOBAL_DATA_PTR;

static int dbg_enable;
#define mpq8655_dbg(dev, fmt, ...)					\
	do {								\
		if (dbg_enable)						\
			dev_dbg(dev, "MPQ8655: " fmt, ##__VA_ARGS__);	\
	} while (0)

#define MPQ8655_OPERATION		0x01
#define MPQ8655_VOUT_COMMAND		0x21
#define MPQ8655_VOUT_MAX		0x24
#define MPQ8655_VOUT_SCALE_LOOP		0x29
#define MPQ8655_OT_FAULT_LIMIT		0x4f
#define MPQ8655_MFR_ID			0x99
#define MPQ8655_MFR_CTRL_VOUT		0xd1
#define MPQ8655_MFR_CTRL_OPS		0xd2
#define MPQ8655_MFR_VOUT_STEP		0xda
#define MPQ8655_MFR_CTRL		0xea
#define MPQ8655_MODE_NORMAL		1
#define MPQ8655_MODE_FAST		2
#define MPQ8655_VOUT_RANGE_MASK		GENMASK(1, 0)
#define MPQ8655_FREQ_MASK		GENMASK(2, 1)
#define MPQ8655_PS_MODE			BIT(0)
#define MPQ8655_PHASE_MULTI		BIT(3)
#define MPQ8655_OP_ON			0x80
#define MPQ8655_VOUT_CMD_MAX		0x0fff
#define MPQ8655_MFR_ID_LEN		3
#define MPQ8655_MFR_ID_RAW_LEN		(MPQ8655_MFR_ID_LEN + 1)
#define MPQ8655_IDENTIFY_ATTEMPTS	2
/* Bounded depth for the *-supply audit walk; DT nesting is shallow. */
#define MPQ8655_SUPPLY_SCAN_MAX_DEPTH	32
/* OT_FAULT_LIMIT window shared by DT parsing and the setter. */
#define MPQ8655_OT_LIMIT_MIN_C		100U
#define MPQ8655_OT_LIMIT_MAX_C		160U

enum mpq8655_chip_type {
	MPQ8655_CHIP_MPQ8655,
	MPQ8655_CHIP_SC812C0,
	MPQ8655_CHIP_IS6608,
	MPQ8655_CHIP_ES5330,
};

static const char *mpq8655_chip_name(enum mpq8655_chip_type chip)
{
	switch (chip) {
	case MPQ8655_CHIP_MPQ8655:
		return "MPQ8655";
	case MPQ8655_CHIP_SC812C0:
		return "SC812C0";
	case MPQ8655_CHIP_IS6608:
		return "IS6608";
	case MPQ8655_CHIP_ES5330:
		return "ES5330";
	default:
		return "unknown";
	}
}

struct mpq8655_dt_config {
	u32 rfb1_ohms;
	u32 rfb2_ohms;
	u32 kdiv_x1000;
	u32 switching_freq;
	u32 ot_fault_limit;
	bool use_external_div;
	bool switching_freq_set;
	bool ot_fault_limit_set;
	bool phase_set;
	bool multi_phase;
};

struct mpq8655_regulator_plat {
	ofnode chip_node;
};

struct mpq8655_regulator_priv {
	u32 kdiv_x1000;
	u32 vout_scale_loop;
	u32 min_supported_uv;
	u32 max_supported_uv;
	u16 vout_max_cmd;
	enum mpq8655_chip_type chip;
	struct mpq8655_dt_config config;
	bool range_warned;
	bool vout_max_warned;
};

static struct dm_regulator_mode mpq8655_modes[] = {
	{ .id = MPQ8655_MODE_NORMAL, .register_value = 0, .name = "NORMAL" },
	{ .id = MPQ8655_MODE_FAST, .register_value = 0, .name = "FAST" },
};

static const u32 mpq8655_voltage_ranges[][2] = {
	{ 400000, 672000 },
	{ 400000, 1344000 },
	{ 700000, 2688000 },
	{ 1300000, 5376000 },
};

static const u16 mpq8655_vout_scale_ranges[] = {
	1000, 500, 250, 125,
};

struct mpq8655_register {
	const char *name;
	u8 address;
	u8 size;
};

static const struct mpq8655_register mpq8655_registers[] = {
	{ "OPERATION", 0x01, 1 },
	{ "ON_OFF_CONFIG", 0x02, 1 },
	{ "VOUT_COMMAND", 0x21, 2 },
	{ "VOUT_MAX", MPQ8655_VOUT_MAX, 2 },
	{ "VOUT_MARGIN_HIGH", 0x25, 2 },
	{ "VOUT_MARGIN_LOW", 0x26, 2 },
	{ "VOUT_SCALE_LOOP", 0x29, 2 },
	{ "VOUT_MIN", 0x2b, 2 },
	{ "IOUT_OC_FAULT_LIMIT", 0x46, 2 },
	{ "IOUT_OC_WARN_LIMIT", 0x4a, 2 },
	{ "OT_FAULT_LIMIT", 0x4f, 2 },
	{ "OT_WARN_LIMIT", 0x51, 2 },
	{ "STATUS_BYTE", 0x78, 1 },
	{ "STATUS_WORD", 0x79, 2 },
	{ "READ_VOUT", 0x8b, 2 },
	{ "READ_IOUT", 0x8c, 2 },
	{ "READ_TEMPERATURE", 0x8d, 2 },
	{ "MFR_CTRL_VOUT", MPQ8655_MFR_CTRL_VOUT, 1 },
	{ "MFR_CTRL_OPS", MPQ8655_MFR_CTRL_OPS, 1 },
	{ "MFR_OC_PHASE_SET", 0xd7, 1 },
	{ "MFR_VOUT_STEP", MPQ8655_MFR_VOUT_STEP, 1 },
	{ "MFR_CTRL", MPQ8655_MFR_CTRL, 2 },
};

static int mpq8655_read8(struct udevice *dev, u8 reg, u8 *value)
{
	int ret = dm_i2c_read(dev, reg, value, 1);

	if (ret)
		dev_dbg(dev, "8-bit read of register 0x%02x failed: %d\n", reg, ret);

	return ret;
}

static int mpq8655_write8(struct udevice *dev, u8 reg, u8 value)
{
	int ret = dm_i2c_write(dev, reg, &value, 1);

	if (ret)
		dev_dbg(dev, "8-bit write of register 0x%02x failed: %d\n", reg, ret);

	return ret;
}

static int mpq8655_read16(struct udevice *dev, u8 reg, u16 *value)
{
	u8 data[2];
	int ret;

	ret = dm_i2c_read(dev, reg, data, sizeof(data));
	if (ret) {
		dev_dbg(dev, "16-bit read of register 0x%02x failed: %d\n", reg, ret);
		return ret;
	}
	/* PMBus words use SMBus word-data order: least-significant byte first. */
	*value = data[0] | ((u16)data[1] << 8);

	return 0;
}

static void mpq8655_dump_registers(struct udevice *dev)
{
	const struct mpq8655_register *reg;
	int failed = 0;
	u8 data[2];
	int i, ret;

	dev_dbg(dev, "MPQ8655 register dump (MFR_ID excluded):\n");
	for (i = 0; i < ARRAY_SIZE(mpq8655_registers); i++) {
		reg = &mpq8655_registers[i];

		switch (reg->size) {
		case 1:
			ret = dm_i2c_read(dev, reg->address, data, 1);
			if (!ret)
				dev_dbg(dev, "  %-26s [0x%02x] = 0x%02x\n",
					reg->name, reg->address, data[0]);
			break;
		case 2:
			ret = dm_i2c_read(dev, reg->address, data, sizeof(data));
			if (!ret)
				dev_dbg(dev, "  %-26s [0x%02x] = 0x%04x\n",
					reg->name, reg->address,
					data[0] | ((u16)data[1] << 8));
			break;
		default:
			ret = -EINVAL;
			break;
		}

		if (ret)
			failed++;
	}

	if (failed)
		dev_dbg(dev, "MPQ8655 register dump: %d register reads failed\n", failed);
}

static int mpq8655_write16(struct udevice *dev, u8 reg, u16 value)
{
	/* Keep the SMBus word-data byte order used by PMBus. */
	u8 data[2] = { value & 0xff, value >> 8 };
	int ret;

	ret = dm_i2c_write(dev, reg, data, sizeof(data));
	if (ret)
		dev_dbg(dev, "16-bit write of register 0x%02x failed: %d\n", reg, ret);

	return ret;
}

/*
 * Identify the chip by reading MFR_ID (0x99).
 *
 * The MPQ8655 and its pin-compatible variants (SC812C0, IS6608, ES5330) all
 * report a different manufacturer ID via the PMBus MFR_ID register.  Reading
 * it at probe time allows the same driver binary and the same device tree to
 * work across all of them — swapping the chip on the board requires no
 * software changes.
 *
 * MFR_ID is a PMBus block response (raw[0] = byte count, payload follows).
 * The three ID bytes are stored in little-endian order in the payload:
 *   MPQ8655 : raw[3]='M', raw[2]='P', raw[1]='S'  (MPS)
 *   SC812C0: raw[1]='S', raw[2]='C', raw[3]=0x00  (SC\0)
 *   IS6608 : raw[1]='I', raw[2]='S', raw[3]=0x00  (IS\0)
 *   ES5330 : raw[0]=0x00                       (single-byte value of 0)
 *
 * The first read attempt reads one byte to distinguish ES5330 (value 0) from
 * the PMBus parts (byte count = 3).  For PMBus parts a second read fetches the
 * full count+payload sequence and matches it against the known IDs above.
 *
 * The chip type is decided by hardware ID alone, never by the device tree:
 * all four compatibles are interchangeable at runtime, so a board where the
 * MPQ8655 is replaced by SC812C0, IS6608 or ES5330 keeps working without any
 * DT change.  A PMBus part caught mid power-up can answer 0x00 once, so a
 * zero is confirmed by a second read before ES5330 is assumed: the real part
 * reports a stable single zero byte, while a transient zero either yields a
 * plausible count on the re-read or fallthrough to the retry below.  The
 * residual risk is contained on purpose: priv->chip only labels the probe
 * log, while every register op in this driver is common to all four parts,
 * so voltage, enable, mode and ramp control keep working even if the label
 * is wrong.
 *
 * The whole identification sequence is tried MPQ8655_IDENTIFY_ATTEMPTS times
 * to tolerate transient I2C NACKs during power-up ramp.
 */
static int mpq8655_identify(struct udevice *dev, enum mpq8655_chip_type *chip)
{
	u8 raw[MPQ8655_MFR_ID_RAW_LEN];
	int ret = 0, attempt;
	u8 confirm;
	u8 first;

	/*
	 * PMBus MFR_ID is a block response: read its count byte first, then
	 * the count and three ID bytes.  ES5330 instead reports a single
	 * zero byte.
	 *
	 * The whole identification sequence (first-byte read + full block
	 * read + validation) is retried MPQ8655_IDENTIFY_ATTEMPTS times to
	 * tolerate transient I2C NACKs during power-up ramp.  Both the
	 * initial 1-byte read and the subsequent 4-byte block read can
	 * fail when the chip is still powering up, so we wrap both in the
	 * same retry loop.
	 */
	for (attempt = 0; attempt < MPQ8655_IDENTIFY_ATTEMPTS; attempt++) {
		ret = mpq8655_read8(dev, MPQ8655_MFR_ID, &first);
		if (ret)
			goto retry;

		if (!first) {
			/*
			 * ES5330 answers MFR_ID as a single 0x00 byte; a PMBus
			 * part still powering up can read 0x00 once too, so
			 * confirm it with a second read before classifying.
			 * The decision still comes from silicon alone; the type
			 * must never depend on the board file.
			 */
			ret = mpq8655_read8(dev, MPQ8655_MFR_ID, &confirm);
			if (ret)
				goto retry;
			if (!confirm) {
				mpq8655_dbg(dev, "MFR_ID reads 0x00 twice; assuming ES5330\n");
				*chip = MPQ8655_CHIP_ES5330;
				return 0;
			}
			mpq8655_dbg(dev,
				    "MFR_ID count byte 0x%02x was transient (re-read 0x%02x); continuing\n",
				    first, confirm);
			first = confirm;
		}
		if (first != MPQ8655_MFR_ID_LEN) {
			mpq8655_dbg(dev, "Unexpected MFR_ID byte count: 0x%02x; retrying\n",
				    first);
			ret = -EIO;
			goto retry;
		}

		ret = dm_i2c_read(dev, MPQ8655_MFR_ID, raw, sizeof(raw));
		if (ret)
			goto retry;
		if (raw[0] != MPQ8655_MFR_ID_LEN) {
			mpq8655_dbg(dev,
				    "MFR_ID block byte count mismatch (got 0x%02x); retrying\n",
				    raw[0]);
			ret = -EIO;
			goto retry;
		}

		/* Valid block response; match the ID payload. */
		if (raw[3] == 'M' && raw[2] == 'P' && raw[1] == 'S') {
			*chip = MPQ8655_CHIP_MPQ8655;
		} else if (raw[1] == 'S' && raw[2] == 'C' &&
			   raw[3] == 0x00) {
			*chip = MPQ8655_CHIP_SC812C0;
		} else if (raw[1] == 'I' && raw[2] == 'S' &&
			   raw[3] == 0x00) {
			*chip = MPQ8655_CHIP_IS6608;
		} else {
			dev_err(dev,
				"Unknown MFR_ID: 0x%02x%02x%02x\n",
				raw[1], raw[2], raw[3]);
			return -ENODEV;
		}
		return 0;

retry:
		if (attempt < MPQ8655_IDENTIFY_ATTEMPTS - 1)
			mpq8655_dbg(dev,
				    "MFR_ID identify attempt %d/%d failed (%d); retrying\n",
				    attempt + 1, MPQ8655_IDENTIFY_ATTEMPTS, ret);
	}

	dev_err(dev,
		"Failed to read MFR_ID after %d attempts: %d\n",
		MPQ8655_IDENTIFY_ATTEMPTS, ret);

	return ret;
}

/*
 * Datasheet Rev. 1.1, p. 23 (Table 1 and external-divider example):
 * VOUT_COMMAND has a 2mV LSB, while VOUT_SCALE_LOOP and MFR_CTRL_VOUT
 * select the feedback scaling:
 * VOUT[uV] = VOUT_COMMAND * 2000 * VOUT_SCALE_LOOP / Kdiv_x1000.
 * Kdiv_x1000 is 1000 for the internal divider and the external feedback
 * ratio (RFB2 / (RFB1 + RFB2)) scaled by 1000 otherwise.
 */
static int mpq8655_get_voltage(struct udevice *dev)
{
	struct mpq8655_regulator_priv *priv = dev_get_priv(dev);
	u16 command, scale;
	u64 voltage;
	int ret;

	ret = mpq8655_read16(dev, MPQ8655_VOUT_COMMAND, &command);
	if (ret)
		return ret;

	scale = priv->vout_scale_loop;
	if (!scale || !priv->kdiv_x1000 || !priv->min_supported_uv || !priv->max_supported_uv)
		return -EINVAL;
	if (command > MPQ8655_VOUT_CMD_MAX)
		return -ERANGE;

	voltage = (u64)command * 2000ULL * scale / priv->kdiv_x1000;
	if (voltage > 0x7fffffff) {
		dev_err(dev, "VOUT_COMMAND reports overflow voltage %llu uV\n",
			(unsigned long long)voltage);
		return -ERANGE;
	}
	if (voltage < priv->min_supported_uv || voltage > priv->max_supported_uv) {
		/*
		 * VOUT_COMMAND is outside the expected software range but
		 * still represents the chip's actual output setting.  This
		 * can happen when another firmware stage or OTP/MTP has
		 * programmed a value outside what DT advertises.  Warn once
		 * and return the computed value so that regulator_show()
		 * and upper layers can still display the real state,
		 * rather than propagating -ERANGE up the whole call chain.
		 *
		 * The console warning is rate-limited to one occurrence; the
		 * debug channel below is intentionally per-call so that a
		 * debug build traces every get_voltage during autoset or
		 * regulator_show.
		 */
		mpq8655_dbg(dev,
			    "VOUT_COMMAND %llu uV outside DT range %u-%u uV\n",
			    (unsigned long long)voltage, priv->min_supported_uv,
			    priv->max_supported_uv);
		if (!priv->range_warned) {
			priv->range_warned = true;
			dev_warn(dev,
				 "VOUT_COMMAND %llu uV is outside DT range %u-%u uV (suppressing further warnings)\n",
				 (unsigned long long)voltage, priv->min_supported_uv,
				 priv->max_supported_uv);
		}
	}

	return voltage;
}

static int mpq8655_set_voltage(struct udevice *dev, int uV)
{
	struct mpq8655_regulator_priv *priv = dev_get_priv(dev);
	u64 command, denominator, numerator;
	u32 requested_uV;
	int ret;

	if (uV <= 0 || !priv->vout_scale_loop || !priv->kdiv_x1000)
		return -EINVAL;

	requested_uV = uV;
	if ((priv->min_supported_uv && requested_uV < priv->min_supported_uv) ||
	    (priv->max_supported_uv && requested_uV > priv->max_supported_uv))
		return -ERANGE;

	numerator = (u64)requested_uV * priv->kdiv_x1000;
	denominator = (u64)priv->vout_scale_loop * 2000ULL;
	command = (numerator + denominator / 2) / denominator;
	if (!command || command > MPQ8655_VOUT_CMD_MAX) {
		dev_err(dev, "Requested voltage %d uV is out of range\n", uV);
		return -ERANGE;
	}

	/*
	 * Software mirror of the chip's VOUT_MAX clamp (0xFFFF when the
	 * register is unprogrammed or unreadable, which never triggers).
	 * The part would clamp internally anyway; clamping here keeps the
	 * programmed VOUT_COMMAND consistent with what the driver believes
	 * the output is, and the mismatch is surfaced once instead of being
	 * silently absorbed by the hardware.
	 */
	if (command > priv->vout_max_cmd) {
		if (!priv->vout_max_warned) {
			priv->vout_max_warned = true;
			dev_warn(dev,
				 "Requested %d uV (command 0x%03llx) exceeds VOUT_MAX 0x%03x; clamping\n",
				 uV, (unsigned long long)command, priv->vout_max_cmd);
		}
		command = priv->vout_max_cmd;
	}

	mpq8655_dbg(dev, "set_voltage: target=%d uV, Kdiv=%u, VOUT_SCALE_LOOP=%u, "
		    "VOUT_COMMAND=0x%03llx (%llu), effective=%llu uV\n",
		    uV, priv->kdiv_x1000, priv->vout_scale_loop,
		    (unsigned long long)command, (unsigned long long)command,
		    (unsigned long long)(command * 2000ULL * priv->vout_scale_loop /
					 priv->kdiv_x1000));
	ret = mpq8655_write16(dev, MPQ8655_VOUT_COMMAND, command);
	if (!ret)
		mpq8655_dbg(dev,
			    "wrote VOUT_COMMAND=0x%04x for %d uV\n",
			    (u32)command, uV);

	return ret;
}

static int mpq8655_get_mode(struct udevice *dev)
{
	u8 value;
	int ret;

	ret = mpq8655_read8(dev, MPQ8655_MFR_CTRL_OPS, &value);
	if (ret)
		return ret;

	return value & MPQ8655_PS_MODE ? MPQ8655_MODE_FAST : MPQ8655_MODE_NORMAL;
}

static int mpq8655_set_mode(struct udevice *dev, int mode_id)
{
	u8 value;
	int ret;

	if (mode_id != MPQ8655_MODE_FAST && mode_id != MPQ8655_MODE_NORMAL)
		return -EINVAL;

	ret = mpq8655_read8(dev, MPQ8655_MFR_CTRL_OPS, &value);
	if (ret)
		return ret;

	if (mode_id == MPQ8655_MODE_FAST)
		value |= MPQ8655_PS_MODE;
	else
		value &= ~MPQ8655_PS_MODE;

	return mpq8655_write8(dev, MPQ8655_MFR_CTRL_OPS, value);
}

static int mpq8655_get_enable(struct udevice *dev)
{
	u8 operation;
	int ret;

	ret = mpq8655_read8(dev, MPQ8655_OPERATION, &operation);
	if (ret)
		return ret;

	return !!(operation & MPQ8655_OP_ON);
}

static int mpq8655_set_enable(struct udevice *dev, bool enable)
{
	u8 operation;
	int ret;

	ret = mpq8655_read8(dev, MPQ8655_OPERATION, &operation);
	if (ret)
		return ret;

	if (enable)
		operation |= MPQ8655_OP_ON;
	else
		operation &= ~MPQ8655_OP_ON;

	return mpq8655_write8(dev, MPQ8655_OPERATION, operation);
}

/*
 * Program OT_FAULT_LIMIT.
 *
 * The register field is an unsigned byte (1 LSB = 1 degree C, upper byte
 * unused), but the accepted window is the same [100, 160] C the DT parser
 * enforces, so this setter cannot be driven outside that range by a future
 * caller.
 */
static int mpq8655_set_ot_limit(struct udevice *dev, u32 limit)
{
	if (limit < MPQ8655_OT_LIMIT_MIN_C || limit > MPQ8655_OT_LIMIT_MAX_C)
		return -EINVAL;

	return mpq8655_write16(dev, MPQ8655_OT_FAULT_LIMIT, (u16)limit);
}

/*
 * Calculate the MFR_VOUT_STEP register value for a given ramp_delay.
 *
 * Datasheet Rev. 1.1, p. 47: MFR_VOUT_STEP encodes 10 + 2.5 * step us/mV.
 *
 * Conversion chain:
 *   ramp_delay (uV/us)  →  us_per_mv = round(1000 / ramp_delay)
 *   us_per_mv           →  step = round((us_per_mv - 10) / 2.5)
 *
 * Both conversions use round-to-nearest to avoid systematic bias.
 *
 * @strict:
 *   - false (lenient):  clamp step to 0..15, never fail (for hw_init /
 *     DT parsing, where a misconfigured value should not prevent probe).
 *   - true  (strict):  return -EINVAL if the request is slower than the
 *     hardware slowest setting (step > 0xf).  For runtime uclass ops,
 *     where callers expect exact behaviour.
 *
 * Faster-than-hardware requests (us_per_mv <= 10) always clamp to step 0
 * (fastest).  This matches the common "go as fast as possible" idiom.
 */
static int mpq8655_calc_ramp_step(u32 ramp_delay, u32 *step, bool strict)
{
	u32 us_per_mv;

	if (!ramp_delay)
		return -EINVAL;

	us_per_mv = DIV_ROUND_CLOSEST(1000U, ramp_delay);
	if (us_per_mv <= 10) {
		*step = 0;
	} else {
		/* step = round((us_per_mv - 10) / 2.5) */
		*step = DIV_ROUND_CLOSEST((us_per_mv - 10) * 2, 5);
		if (*step > 0xf) {
			if (strict)
				return -EINVAL;
			*step = 0xf;
		}
	}

	return 0;
}

/*
 * uclass set_ramp_delay operation.
 *
 * Runtime calls are strict: out-of-range (too slow) values return -EINVAL
 * so the caller knows the request could not be satisfied exactly.  This
 * matches the convention used by other regulator drivers (e.g. rk860x)
 * and prevents silent behavioural surprises.
 */
static int mpq8655_set_ramp_delay(struct udevice *dev, u32 ramp_delay)
{
	u32 step;
	int ret;

	ret = mpq8655_calc_ramp_step(ramp_delay, &step, true);
	if (ret)
		return ret;

	return mpq8655_write8(dev, MPQ8655_MFR_VOUT_STEP, step);
}

static int mpq8655_frequency_to_code(u32 frequency)
{
	switch (frequency) {
	case 400000:
		return 0;
	case 600000:
		return 1;
	case 800000:
		return 2;
	case 1000000:
		return 3;
	default:
		return -EINVAL;
	}
}

static int mpq8655_set_frequency(struct udevice *dev, u32 frequency)
{
	u8 value, freq;
	int ret;

	ret = mpq8655_frequency_to_code(frequency);
	if (ret < 0) {
		dev_err(dev, "Unsupported switching frequency: %u Hz\n",
			frequency);
		return ret;
	}
	freq = ret;

	/*
	 * Defensive guard: frequency_to_code must return a value that fits
	 * in FREQ_MASK (bits[2:1], 4 possible codes).  If someone extends
	 * frequency_to_code without updating the bit layout, this catches
	 * the overflow instead of silently corrupting neighbouring bits.
	 */
	if (freq > (MPQ8655_FREQ_MASK >> 1)) {
		dev_err(dev,
			"Frequency code %u overflows FREQ_MASK (0x%02x)\n",
			freq, (unsigned int)MPQ8655_FREQ_MASK);
		return -EINVAL;
	}

	ret = mpq8655_read8(dev, MPQ8655_MFR_CTRL_OPS, &value);
	if (ret)
		return ret;

	value &= ~MPQ8655_FREQ_MASK;
	value |= (freq << 1) & MPQ8655_FREQ_MASK;

	return mpq8655_write8(dev, MPQ8655_MFR_CTRL_OPS, value);
}

static int mpq8655_set_phase(struct udevice *dev, bool multi_phase)
{
	u16 value;
	int ret;

	ret = mpq8655_read16(dev, MPQ8655_MFR_CTRL, &value);
	if (ret)
		return ret;
	if (multi_phase)
		value |= MPQ8655_PHASE_MULTI;
	else
		value &= ~MPQ8655_PHASE_MULTI;

	return mpq8655_write16(dev, MPQ8655_MFR_CTRL, value);
}

static ofnode mpq8655_chip_node(struct udevice *dev)
{
	struct mpq8655_regulator_plat *plat = dev_get_plat(dev);

	return plat ? plat->chip_node : ofnode_null();
}

static bool mpq8655_read_optional_u32(struct udevice *dev, ofnode node,
				      const char *propname, u32 *value)
{
	int ret;

	if (!ofnode_get_property(node, propname, NULL))
		return false;

	ret = ofnode_read_u32(node, propname, value);
	if (ret) {
		dev_warn(dev, "Ignoring invalid optional property %s: %d\n",
			 propname, ret);
		return false;
	}

	return true;
}

static int mpq8655_parse_dt(struct udevice *dev, struct mpq8655_dt_config *config)
{
	ofnode chip_node = mpq8655_chip_node(dev);
	u64 divider_sum, kdiv_x1000;
	int ret;

	if (!ofnode_valid(chip_node))
		return -EINVAL;

	memset(config, 0, sizeof(*config));
	config->kdiv_x1000 = 1000;
	config->use_external_div = ofnode_read_bool(chip_node,
						    "mps,external-voltage-divider");

	config->switching_freq_set = mpq8655_read_optional_u32(dev, chip_node,
							       "mps,switching-frequency-hz",
							       &config->switching_freq);

	if (config->switching_freq_set) {
		if (mpq8655_frequency_to_code(config->switching_freq) < 0) {
			dev_warn(dev,
				 "Unsupported switching frequency %u Hz; preserving hardware setting\n",
				 config->switching_freq);
			config->switching_freq_set = false;
		}
	}
	if (!config->switching_freq_set)
		mpq8655_dbg(dev,
			    "No switching frequency configured; preserving hardware setting\n");

	config->ot_fault_limit_set = mpq8655_read_optional_u32(dev, chip_node,
							       "mps,overtemperature-limit-celsius",
							       &config->ot_fault_limit);
	if (!config->ot_fault_limit_set)
		mpq8655_dbg(dev,
			    "No overtemperature limit configured; preserving hardware setting\n");
	if (config->ot_fault_limit_set &&
	    (config->ot_fault_limit < MPQ8655_OT_LIMIT_MIN_C ||
	     config->ot_fault_limit > MPQ8655_OT_LIMIT_MAX_C)) {
		dev_warn(dev,
			 "Overtemperature limit %u C out of range [%u, %u]; clamping\n",
			 config->ot_fault_limit, MPQ8655_OT_LIMIT_MIN_C,
			 MPQ8655_OT_LIMIT_MAX_C);
		config->ot_fault_limit = clamp(config->ot_fault_limit,
					       MPQ8655_OT_LIMIT_MIN_C,
					       MPQ8655_OT_LIMIT_MAX_C);
	}
	config->multi_phase = ofnode_read_bool(chip_node, "mps,multi-phase");
	config->phase_set = config->multi_phase ||
		ofnode_read_bool(chip_node, "mps,single-phase");
	if (config->multi_phase &&
	    ofnode_read_bool(chip_node, "mps,single-phase"))
		dev_warn(dev, "Both phase properties are set; using multi-phase\n");

	if (!config->use_external_div)
		return 0;

	ret = ofnode_read_u32(chip_node, "mps,rfp1-ohms", &config->rfb1_ohms);
	if (ret)
		goto missing_divider;
	ret = ofnode_read_u32(chip_node, "mps,rfp2-ohms", &config->rfb2_ohms);
	if (ret)
		goto missing_divider;
	if (!config->rfb1_ohms || !config->rfb2_ohms) {
		dev_err(dev, "External divider resistor values must be non-zero\n");
		return -EINVAL;
	}

	divider_sum = (u64)config->rfb1_ohms + config->rfb2_ohms;
	kdiv_x1000 = (u64)config->rfb2_ohms * 1000 / divider_sum;
	if (!kdiv_x1000 || kdiv_x1000 > 0xffff) {
		dev_err(dev, "External divider ratio is not representable\n");
		return -ERANGE;
	}
	config->kdiv_x1000 = kdiv_x1000;
	return 0;

missing_divider:
	dev_err(dev, "External divider requires both resistor values\n");

	return ret;
}

static int mpq8655_hw_init(struct udevice *dev,
			   struct mpq8655_regulator_priv *priv,
			   struct dm_regulator_uclass_plat *uc_pdata)
{
	u32 initial_uV, range_min, range_max;
	u32 step_code, ramp_us_per_mv;
	u16 ext_scale_rb = 0;
	u16 scale_readback;
	u64 min64, max64;
	int range, ret;
	u8 d1;

	ret = mpq8655_parse_dt(dev, &priv->config);
	if (ret)
		return ret;

	/*
	 * Pre-flight checks: validate everything we can before touching any
	 * chip registers, so that a configuration error does not leave the
	 * part in a partially-programmed state.
	 *
	 * - External-divider mode requires regulator-init-microvolt.  Per
	 *   the datasheet (Rev.1.1, p.24), VOUT_SCALE_LOOP changes only
	 *   take effect on the next VOUT_COMMAND write, so we need an
	 *   explicit init value to latch the new scale loop into place.
	 * - init_uV must be positive if present.
	 */
	if (priv->config.use_external_div && uc_pdata->init_uV == -ENODATA) {
		dev_err(dev,
			"External divider requires regulator-init-microvolt to latch VOUT_SCALE_LOOP\n");
		return -EINVAL;
	}
	if (uc_pdata->init_uV != -ENODATA && uc_pdata->init_uV <= 0) {
		dev_err(dev, "Invalid initial voltage %d uV\n", uc_pdata->init_uV);
		return -EINVAL;
	}

	/*
	 * Step 1: Apply an explicitly configured OTP threshold. If absent,
	 * preserve the value already programmed by hardware/MTP.
	 */
	if (priv->config.ot_fault_limit_set) {
		ret = mpq8655_set_ot_limit(dev, priv->config.ot_fault_limit);
		if (ret)
			return ret;
	}

	/*
	 * Step 2: Apply a configured switching frequency. Otherwise preserve
	 * the frequency selected by hardware/MTP.
	 */
	if (priv->config.switching_freq_set) {
		ret = mpq8655_set_frequency(dev, priv->config.switching_freq);
		if (ret)
			return ret;
	}

	/*
	 * Step 3: Apply the requested phase mode before setting VOUT_COMMAND;
	 * otherwise leave the phase configuration established by hardware.
	 */
	if (priv->config.phase_set) {
		ret = mpq8655_set_phase(dev, priv->config.multi_phase);
		if (ret)
			return ret;
	}

	/*
	 * Step 4: Configure the slew rate only when requested. Otherwise keep
	 * the rate already programmed by hardware/OTP.
	 */
	if (uc_pdata->ramp_delay) {
		/*
		 * During probe we clamp out-of-range ramp values so a
		 * misconfigured DT does not prevent the regulator from
		 * coming up at all.  The runtime ops entry point is strict
		 * and returns -EINVAL for out-of-range requests.
		 */
		ret = mpq8655_calc_ramp_step(uc_pdata->ramp_delay, &step_code, false);
		if (ret)
			return ret;
		ramp_us_per_mv = DIV_ROUND_CLOSEST(1000U, uc_pdata->ramp_delay);
		if (ramp_us_per_mv > 47) {
			dev_warn(dev,
				 "ramp_delay %u uV/us (%u us/mV) exceeds HW slowest setting; clamping to 47.5 us/mV\n",
				 uc_pdata->ramp_delay, ramp_us_per_mv);
		}
		ret = mpq8655_write8(dev, MPQ8655_MFR_VOUT_STEP, step_code);
		if (ret)
			return ret;
	} else {
		mpq8655_dbg(dev,
			    "No ramp delay configured; preserving hardware setting\n");
	}

	/*
	 * Step 5: Read the hardware voltage range. External-divider mode uses
	 * range 0; internal-divider mode keeps the range selected by hardware.
	 */
	ret = mpq8655_read8(dev, MPQ8655_MFR_CTRL_VOUT, &d1);
	if (ret)
		return ret;
	range = d1 & MPQ8655_VOUT_RANGE_MASK;
	if (priv->config.use_external_div)
		range = 0;
	range_min = mpq8655_voltage_ranges[range][0];
	range_max = mpq8655_voltage_ranges[range][1];

	/*
	 * Step 6: For an external divider, select that mode and program
	 * VOUT_SCALE_LOOP to RFB2 / (RFB1 + RFB2), in 0.001 units.
	 */
	if (priv->config.use_external_div) {
		priv->kdiv_x1000 = priv->config.kdiv_x1000;
		priv->vout_scale_loop = priv->kdiv_x1000;
		d1 &= ~MPQ8655_VOUT_RANGE_MASK;
		ret = mpq8655_write8(dev, MPQ8655_MFR_CTRL_VOUT, d1);
		if (ret)
			return ret;
		ret = mpq8655_write16(dev, MPQ8655_VOUT_SCALE_LOOP,
				      priv->vout_scale_loop);
		if (ret)
			return ret;
	} else {
		priv->kdiv_x1000 = 1000;
		/*
		 * Read back VOUT_SCALE_LOOP rather than assuming it matches
		 * the table value for the current range.  OTP/MTP defaults or
		 * a previous firmware stage may have programmed a non-standard
		 * scale, and using the wrong value would make get_voltage()
		 * report incorrect results.  Fall back to the table value
		 * only if the register read fails.
		 */
		ret = mpq8655_read16(dev, MPQ8655_VOUT_SCALE_LOOP, &scale_readback);
		if (ret) {
			dev_warn(dev,
				 "Failed to read VOUT_SCALE_LOOP; using table default for range %d\n",
				 range);
			priv->vout_scale_loop = mpq8655_vout_scale_ranges[range];
		} else {
			priv->vout_scale_loop = scale_readback;
			if (scale_readback != mpq8655_vout_scale_ranges[range])
				dev_dbg(dev,
					"VOUT_SCALE_LOOP=0x%04x differs from table default 0x%04x for range %d\n",
					scale_readback, mpq8655_vout_scale_ranges[range],
					range);
		}
	}

	/* Convert the selected hardware range to the regulator's output range. */
	if (priv->config.use_external_div) {
		/*
		 * For an external divider, the output range is the internal
		 * range divided by the feedback ratio (kdiv_x1000 / 1000).
		 * Both resistors are validated non-zero, so rfb2 < rfb1 +
		 * rfb2 and kdiv_x1000 ends up in 1..999.  External-divider
		 * mode pins the internal range to
		 * mpq8655_voltage_ranges[0] (400000..672000 uV), so even
		 * the extreme kdiv_x1000 = 1 stays inside u32
		 * (672,000,000 uV).  Keep the check below as a defensive
		 * guard against future changes to the range table.
		 */
		min64 = (u64)range_min * 1000 / priv->kdiv_x1000;
		max64 = (u64)range_max * 1000 / priv->kdiv_x1000;
		if (max64 > 0xffffffffULL) {
			dev_err(dev,
				"External divider output range %llu uV exceeds 32-bit limit\n",
				(unsigned long long)max64);
			return -ERANGE;
		}
		priv->min_supported_uv = (u32)min64;
		priv->max_supported_uv = (u32)max64;
	} else {
		priv->min_supported_uv = range_min;
		priv->max_supported_uv = range_max;
	}

	/*
	 * Step 7: Apply VOUT_COMMAND to latch the VOUT_SCALE_LOOP and
	 * MFR_CTRL_VOUT (D1h) changes.
	 *
	 * Per the MPQ8655 datasheet (Rev.1.1, p.24), VOUT_COMMAND must be
	 * written *after* VOUT_SCALE_LOOP and D1h, otherwise the scale
	 * loop and range-bit changes do not take effect.
	 *
	 * The pre-flight block above (run before any register write)
	 * rejected a non-positive init_uV and required init_uV for
	 * external-divider mode.  So init_uV is either a positive value or
	 * -ENODATA in internal-divider mode; only the positive case falls
	 * through to the write below, the -ENODATA case returns just after.
	 *
	 * For internal-divider mode with no init_uV, leave VOUT_COMMAND
	 * alone — the power-on default scale loop matches the current
	 * range, so no latch is needed.  This matches the "keep current
	 * voltage" behaviour described in the binding doc.
	 */
	if (uc_pdata->init_uV == -ENODATA) {
		dev_dbg(dev,
			"regulator-init-microvolt not set; keeping current voltage\n");
		return 0;
	}

	initial_uV = uc_pdata->init_uV;
	if (initial_uV < priv->min_supported_uv || initial_uV > priv->max_supported_uv) {
		dev_err(dev,
			"Initial voltage %u uV is outside hardware range %u-%u uV\n",
			initial_uV, priv->min_supported_uv, priv->max_supported_uv);
		return -ERANGE;
	}

	/*
	 * Cross-check init_uV against the DT regulator-min-microvolt /
	 * regulator-max-microvolt constraints (when present).  The board-
	 * level limits may be tighter than the chip's hardware range.
	 *
	 * Both values are validated first: -ENODATA means "not set", and
	 * any other non-positive value is treated as corrupt DT data and
	 * is rejected rather than silently wrapping to a huge u32.
	 */
	if (uc_pdata->min_uV != -ENODATA) {
		if (uc_pdata->min_uV <= 0) {
			dev_err(dev, "Invalid DT regulator-min-microvolt: %d uV\n",
				uc_pdata->min_uV);
			return -EINVAL;
		}
		if (initial_uV < (u32)uc_pdata->min_uV) {
			dev_err(dev,
				"Initial voltage %u uV is below DT regulator-min-microvolt (%d uV)\n",
				initial_uV, uc_pdata->min_uV);
			return -ERANGE;
		}
	}
	if (uc_pdata->max_uV != -ENODATA) {
		if (uc_pdata->max_uV <= 0) {
			dev_err(dev, "Invalid DT regulator-max-microvolt: %d uV\n",
				uc_pdata->max_uV);
			return -EINVAL;
		}
		if (initial_uV > (u32)uc_pdata->max_uV) {
			dev_err(dev,
				"Initial voltage %u uV is above DT regulator-max-microvolt (%d uV)\n",
				initial_uV, uc_pdata->max_uV);
			return -ERANGE;
		}
	}

	ret = mpq8655_set_voltage(dev, initial_uV);
	if (ret)
		return ret;

	if (priv->config.use_external_div) {
		/*
		 * External-divider mode programs VOUT_SCALE_LOOP in Step 6
		 * without a readback.  Verify it here, after VOUT_COMMAND has
		 * latched the new scale, so debug logs reflect the effective
		 * register value while production builds pay no extra I2C
		 * traffic.
		 */
		if (!mpq8655_read16(dev, MPQ8655_VOUT_SCALE_LOOP, &ext_scale_rb))
			mpq8655_dbg(dev,
				    "VOUT_SCALE_LOOP readback 0x%04x (programmed %u)\n",
				    ext_scale_rb, priv->vout_scale_loop);
	}

	mpq8655_dbg(dev,
		    "init_uV=%u uV applied (range %u..%u uV)\n",
		    initial_uV, priv->min_supported_uv, priv->max_supported_uv);

	return 0;
}

/*
 * Format a human-readable identifier for a DT node: the full path when
 * ofnode_get_path() succeeds, the node name otherwise.
 *
 * ofnode_get_path() returns 0 on success and a negative errno when the node
 * is invalid or the path does not fit in the buffer.  Testing its return
 * value as if it were a length, as an earlier revision of this driver did,
 * would discard every successful path and always fall back to the name.
 */
static void mpq8655_node_label(ofnode node, char *buf, size_t buflen)
{
	const char *name;

	if (ofnode_valid(node) && ofnode_get_path(node, buf, buflen) == 0)
		return;

	name = ofnode_valid(node) ? ofnode_get_name(node) : NULL;
	if (!name || !*name)
		name = "<unknown>";

	snprintf(buf, buflen, "%s", name);
}

static void mpq8655_bind_error(struct udevice *dev, ofnode chip_node,
			       const char *message)
{
	char path[128];

	mpq8655_node_label(chip_node, path, sizeof(path));
	dev_err(dev, "%s: %s\n", path, message);
}

/*
 * Report *-supply properties whose phandle resolves to the chip node.
 * The driver's bind switches its device OF node to the nested regulator
 * subnode, so supply references must target that subnode; anything still
 * pointing at the chip node would silently fail to resolve later.
 *
 * @depth: recursion level of @node, 0 for the tree root.  A hand-edited or
 * corrupt device tree could nest pathologically deep, so the walk stops at
 * MPQ8655_SUPPLY_SCAN_MAX_DEPTH instead of risking a stack overflow.  Real
 * trees nest far below that limit; only the depth is bounded, so sibling
 * fan-out stays unlimited.
 * @pathbuf: scratch buffer for the node path in the diagnostic, owned by
 * the caller so that each recursion level costs a pointer, not a 128-byte
 * frame.
 */
static void mpq8655_scan_supply_refs(struct udevice *dev, ofnode node,
				     ofnode chip_node, int depth,
				     char *pathbuf, size_t pathbuflen)
{
	struct ofprop prop;
	ofnode child;

	if (depth > MPQ8655_SUPPLY_SCAN_MAX_DEPTH) {
		dev_dbg(dev, "supply scan truncated at depth %d\n", depth);
		return;
	}

	ofnode_for_each_prop(prop, node) {
		const __be32 *phandles;
		const char *name;
		ofnode target;
		int len;

		phandles = ofprop_get_property(&prop, &name, &len);
		if (!phandles || len < 4 || !name ||
		    !strstr(name, "-supply"))
			continue;

		target = ofnode_get_by_phandle(fdt32_to_cpu(phandles[0]));
		if (ofnode_valid(target) && ofnode_equal(target, chip_node)) {
			mpq8655_node_label(node, pathbuf, pathbuflen);
			dev_err(dev,
				"%s: property \"%s\" supplies from the chip node; point it at the regulator subnode instead\n",
				pathbuf, name);
		}
	}

	ofnode_for_each_subnode(child, node)
		mpq8655_scan_supply_refs(dev, child, chip_node, depth + 1,
					 pathbuf, pathbuflen);
}

static int mpq8655_bind(struct udevice *dev)
{
	struct mpq8655_regulator_plat *plat = dev_get_plat(dev);
	ofnode chip_node = dev_ofnode(dev);
	ofnode regulators_node, node, regulator_node = ofnode_null();
	bool supported_chip;
	char pathbuf[128];

	if (!plat) {
		mpq8655_bind_error(dev, chip_node,
				   "regulator platform data is unavailable");
		return -ENOMEM;
	}

	supported_chip = ofnode_device_is_compatible(chip_node, "mps,mpq8655") ||
		ofnode_device_is_compatible(chip_node, "sc,sc812c0") ||
		ofnode_device_is_compatible(chip_node, "ifs,is6608") ||
		ofnode_device_is_compatible(chip_node, "ein,es5330");
	if (!supported_chip) {
		mpq8655_bind_error(dev, chip_node, "unsupported regulator chip compatible");
		return -ENODEV;
	}

	plat->chip_node = chip_node;

	regulators_node = ofnode_find_subnode(chip_node, "regulators");
	if (!ofnode_valid(regulators_node)) {
		mpq8655_bind_error(dev, chip_node, "regulators subnode not found");
		return -ENXIO;
	}

	ofnode_for_each_subnode(node, regulators_node) {
		if (!ofnode_is_enabled(node))
			continue;
		if (ofnode_valid(regulator_node)) {
			mpq8655_bind_error(dev, chip_node,
					   "only one enabled regulator subnode is supported");
			return -EINVAL;
		}
		regulator_node = node;
	}

	if (!ofnode_valid(regulator_node)) {
		mpq8655_bind_error(dev, chip_node, "no enabled regulator subnode found");
		return -ENODEV;
	}

	/*
	 * The I2C child-post-bind hook has already parsed the chip address from
	 * the original pmic@30 node. Switch this regulator device's OF node
	 * before regulator_post_bind() runs, so the regulator uclass reads the
	 * nested constraints itself and later executes normal autoset handling.
	 * Supply phandles must therefore reference the regulator subnode, not
	 * the parent I2C chip node.  Because that mistake would otherwise stay
	 * silent (the consumer just fails to find its regulator), walk the tree
	 * once for *-supply properties still pointing here and report each hit
	 * with the offending node path.  The walk is depth-bounded.
	 *
	 * Bind is deliberately not aborted on a hit: a stale reference that no
	 * consumer actually uses must not stop the regulator from registering
	 * and take the whole rail down with it.
	 */
	mpq8655_scan_supply_refs(dev, ofnode_root(), chip_node, 0, pathbuf, sizeof(pathbuf));

	dev_set_ofnode(dev, regulator_node);

	return 0;
}

static int mpq8655_probe(struct udevice *dev)
{
	struct dm_regulator_uclass_plat *uc_pdata = dev_get_uclass_plat(dev);
	struct dm_i2c_chip *i2c_chip = dev_get_parent_plat(dev);
	struct mpq8655_regulator_priv *priv = dev_get_priv(dev);
	u64 clamp_uV = 0;
	bool vout_max_ok;
	u16 vout_max = 0;
	int ret;

	if (!dev->parent || device_get_uclass_id(dev->parent) != UCLASS_I2C ||
	    !i2c_chip || !i2c_chip->chip_addr || i2c_chip->chip_addr > 0x7f) {
		dev_err(dev, "Invalid I2C parent or 7-bit chip address\n");
		return -ENODEV;
	}

	/* Fail-open defaults for the VOUT_MAX mirror until the read below. */
	priv->vout_max_cmd = 0xffff;
	priv->vout_max_warned = false;

	uc_pdata->type = REGULATOR_TYPE_BUCK;
	uc_pdata->mode = mpq8655_modes;
	uc_pdata->mode_count = ARRAY_SIZE(mpq8655_modes);

	ret = mpq8655_identify(dev, &priv->chip);
	if (ret)
		return ret;

	/*
	 * Capture the hardware clamp before hw_init() programs VOUT_COMMAND.
	 * The software mirror starts fail-open at 0xFFFF, so reading VOUT_MAX
	 * only afterwards would let the very first voltage write bypass the
	 * mirror and leave it inconsistent with what was actually programmed.
	 */
	vout_max_ok = !mpq8655_read16(dev, MPQ8655_VOUT_MAX, &vout_max);
	if (vout_max_ok) {
		if (vout_max && vout_max < 0xffff)
			priv->vout_max_cmd = vout_max;
	} else {
		mpq8655_dbg(dev, "VOUT_MAX read failed; hardware clamp unknown\n");
	}

	ret = mpq8655_hw_init(dev, priv, uc_pdata);
	if (ret)
		return ret;

	/*
	 * Audit trail for the clamp: the part enforces VOUT_MAX (0x24),
	 * normally programmed via OTP.  Print the raw value and its uV
	 * equivalent once per probe.  Converting to uV needs the scale loop
	 * and divider that only hw_init() establishes, which is why this log
	 * line stays here even though the clamp was captured above.  A failed
	 * read (some compatibles may omit the register) is not fatal and only
	 * skips the log line.
	 */
	if (vout_max_ok) {
		clamp_uV = (priv->vout_scale_loop && priv->kdiv_x1000) ?
			(u64)vout_max * 2000ULL * priv->vout_scale_loop /
				priv->kdiv_x1000 : 0;

		mpq8655_dbg(dev,
			    "VOUT_MAX=0x%04x (%llu uV) is the hardware clamp for VOUT_COMMAND\n",
			    vout_max, (unsigned long long)clamp_uV);

		if (priv->vout_max_cmd == 0xffff)
			mpq8655_dbg(dev,
				    "VOUT_MAX unprogrammed; software mirror disabled\n");
	}

	mpq8655_dump_registers(dev);

	dev_info(dev, "%s regulator initialized%s\n",
		 mpq8655_chip_name(priv->chip),
		 priv->config.use_external_div ? " with external divider" :
		 " with internal divider");

	return 0;
}

static const struct dm_regulator_ops mpq8655_regulator_ops = {
	.get_value = mpq8655_get_voltage,
	.set_value = mpq8655_set_voltage,
	.get_enable = mpq8655_get_enable,
	.set_enable = mpq8655_set_enable,
	.get_mode = mpq8655_get_mode,
	.set_mode = mpq8655_set_mode,
	.set_ramp_delay = mpq8655_set_ramp_delay,
};

static const struct udevice_id mpq8655_regulator_ids[] = {
	{ .compatible = "mps,mpq8655" },
	{ .compatible = "sc,sc812c0" },
	{ .compatible = "ifs,is6608" },
	{ .compatible = "ein,es5330" },
	{ }
};

U_BOOT_DRIVER(mpq8655_regulator) = {
	.name = "mpq8655_regulator",
	.id = UCLASS_REGULATOR,
	.of_match = mpq8655_regulator_ids,
	.bind = mpq8655_bind,
	.probe = mpq8655_probe,
	.ops = &mpq8655_regulator_ops,
	.plat_auto = sizeof(struct mpq8655_regulator_plat),
	.priv_auto = sizeof(struct mpq8655_regulator_priv),
};
