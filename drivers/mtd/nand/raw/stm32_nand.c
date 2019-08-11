
#define pr_fmt(fmt) "[stm32_nand] %s [%d]" fmt, __func__, __LINE__

#define DEBUG
#include <linux/mtd/rawnand.h>
#include <asm/io.h>
#include <linux/sizes.h>
#include <linux/mtd/nand_ecc.h>

#define FMC_PCR		0x80
#define FMC_SR		0x84
#define FMC_PMEM	0x88
#define FMC_PATT	0x8C
#define FMC_ECCR	0x94

#define STM32F7XX_FMC_REGBASE	0xA0000000
#define STM32F7XX_FMC_REG(x)	\
	(*(volatile unsigned int *)(STM32F7XX_FMC_REGBASE + FMC_##x))

static void stm32xx_nand_cmd_ctrl(struct mtd_info *mtd, int cmd, unsigned int ctrl)
{
	if (cmd == NAND_CMD_NONE)
		return;
	if (*(char *)0xc0500000 == 0x12)
	debug("ctrl: 0x%08x, cmd: 0x%08x\n", ctrl, cmd);

	if (ctrl & NAND_CLE)
		writeb(cmd & 0xFF, 0x80010000);
	else if (ctrl & NAND_ALE)
		writeb(cmd & 0xFF, 0x80020000);
}

static uint8_t stm32xx_read_byte(struct mtd_info *mtd)
{
	return readb(0x80000000);
}

static void stm32xx_write_byte(struct mtd_info *mtd, uint8_t byte)
{
	writeb(byte, 0x80000000);
}

static void stm32xx_read_buf(struct mtd_info *mtd, uint8_t *buf, int len)
{
	while (len-- > 0) {
		*buf++ = readb(0x80000000);
	}
	udelay(10);
}

static void stm32xx_write_buf(struct mtd_info *mtd, const uint8_t *buf, int len)
{
	while (len-- > 0)
		writeb(*buf++, 0x80000000);
	udelay(10);
}

static void stm32xx_nand_init(void)
{
	unsigned int reg_value = 0;

	reg_value = STM32F7XX_FMC_REG(PCR);
	reg_value &= (~(3 << 4));
	reg_value &= ~1;
	reg_value |= 1 << 3;
	/* 开启 ECC 使能 */
	reg_value |= 1 << 6;
	/* 2KB page size 纠错 */
	reg_value |= 3 << 17;
	
	reg_value |= 1 << 2;
	reg_value |= 10 << 9;
	reg_value |= 10 << 13;

	STM32F7XX_FMC_REG(PCR) = reg_value;
	STM32F7XX_FMC_REG(PMEM) = (10 | 10 << 8 | 10 << 16 | 10 << 24);
	STM32F7XX_FMC_REG(PATT) = (10 | 10 << 8 | 10 << 16 | 10 << 24);
	
	mdelay(2);
}

#define COUNT_2K_ECC_SIZE	(sizeof(int) / sizeof(char))

#define COUNT_ECC_SIZE		COUNT_2K_ECC_SIZE

static int stm32xx_ecc_calculate(struct mtd_info *mtd, const uint8_t *dat, uint8_t *ecc_code)
{
	int i;
	unsigned int ecc_value = STM32F7XX_FMC_REG(ECCR);

	for (i = 0; i < COUNT_ECC_SIZE; i++ )
	{
		*(ecc_code + i) = (uint8_t)(ecc_value & 0xff); ecc_value >>= 8;
	}

	return 0;
}

static int stm32xx_correct_data(struct mtd_info *mtd, uint8_t *dat, uint8_t *read_ecc, uint8_t *calc_ecc)
{
	int i = 0, ret = 0;

	for (; i < COUNT_ECC_SIZE; i++)
	{
		if (*(i + read_ecc) != *(i + calc_ecc))
		{
			ret = nand_correct_data(mtd, dat, read_ecc, calc_ecc);
			//ret = -EFAULT;
			break;
		}
		else
		{
			continue;
		}
	}

	return ret;
}

static void stm32xx_hwecc_enable(struct mtd_info *mtd, int mode)
{
	STM32F7XX_FMC_REG(PCR) &= (~ (1 << 6));
	isb();
	/* 标记 ECC 使能 */
	STM32F7XX_FMC_REG(PCR) |= (1 << 6);
}

static int stm32xx_read_page_hwecc(struct mtd_info *mtd, struct nand_chip *chip, uint8_t *buf, int oob_required, int page)
{
	int i;
	int stat;
	uint8_t *p = buf;
	uint8_t *ecc_calc = chip->buffers->ecccalc;
	uint8_t *ecc_code = chip->buffers->ecccode;
	uint32_t *eccpos = chip->ecc.layout->eccpos;
	unsigned int max_bitflips = 0;

	stm32xx_hwecc_enable(mtd, NAND_ECC_READ);
	stm32xx_read_buf(mtd, p, chip->ecc.size * chip->ecc.steps);
	stm32xx_ecc_calculate(mtd, p, &ecc_calc[0]);
	
	stm32xx_read_buf(mtd, chip->oob_poi, mtd->oobsize);

	for (i = 0; i < chip->ecc.total; i++)
		ecc_code[i] = chip->oob_poi[eccpos[i]];

	stat = chip->ecc.correct(mtd, p, &ecc_code[0], &ecc_calc[0]);
	if (stat < 0)
		mtd->ecc_stats.failed++;
	else {
		mtd->ecc_stats.corrected += stat;
		max_bitflips = max_t(unsigned int, max_bitflips, stat);
	}

	return max_bitflips;
}

static int stm32xx_write_page_hwecc(struct mtd_info *mtd, struct nand_chip *chip, const uint8_t *buf, int oob_required, int page)
{
	int i;
	uint8_t *ecc_calc = chip->buffers->ecccalc;
	const uint8_t *p = buf;
	uint32_t *eccpos = chip->ecc.layout->eccpos;
	
	stm32xx_hwecc_enable(mtd, NAND_ECC_WRITE);
	stm32xx_write_buf(mtd, p, chip->ecc.size * chip->ecc.steps);
	stm32xx_ecc_calculate(mtd, p, &ecc_calc[0]);

	for (i = 0; i < chip->ecc.total; i++)
	{
		chip->oob_poi[eccpos[i]] = ecc_calc[i];
	}
	stm32xx_write_buf(mtd, chip->oob_poi, mtd->oobsize);

	return 0;
}

static int stm32xx_waitfunc(struct mtd_info *mtd, struct nand_chip *this)
{
	uint8_t status_test;
#if 1
	do {
	    stm32xx_nand_cmd_ctrl(mtd, 0x70, NAND_CLE);
	    status_test = stm32xx_read_byte(mtd);
	} while(!(status_test & 1 << 6));
#else
	udelay(20);
#endif
	return 0;
}

int board_nand_init(struct nand_chip *nand)
{
	nand->cmd_ctrl  = stm32xx_nand_cmd_ctrl;
	
	/*
	 * The implementation of these functions is quite common, but
	 * they MUST be defined, because access to data register
	 * is strictly 32-bit aligned.
	 */
	nand->read_byte  = stm32xx_read_byte;
	nand->write_byte = stm32xx_write_byte;

	nand->ecc.mode = NAND_ECC_HW_SYNDROME;//NAND_ECC_NONE;
	/*
	 * The implementation of these functions is quite common, but
	 * they MUST be defined, because access to data register
	 * is strictly 32-bit aligned.
	 */
	nand->read_buf   = stm32xx_read_buf;
	nand->write_buf  = stm32xx_write_buf;
	/* 近似 20us 的等待 ready 时间 */
	nand->chip_delay = 20;
	nand->waitfunc = stm32xx_waitfunc;
	/* ECC 函数读写初始化 */
	nand->ecc.size     = SZ_2K;
	nand->ecc.bytes    = COUNT_2K_ECC_SIZE;
	nand->ecc.strength = 1;
	nand->ecc.calculate	= stm32xx_ecc_calculate;
	nand->ecc.correct	= stm32xx_correct_data;
	nand->ecc.hwctl		= stm32xx_hwecc_enable;
	nand->ecc.read_page	= stm32xx_read_page_hwecc;
	nand->ecc.write_page	= stm32xx_write_page_hwecc;
	//nand->options |=  NAND_ROW_ADDR_3;
	/* Initialize NAND interface */
	stm32xx_nand_init();

	return 0;
}
