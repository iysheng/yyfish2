//#define DEBUG
#include <linux/mtd/rawnand.h>
#include <asm/io.h>

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
	
	debug("ctrl: 0x%08x, cmd: 0x%08x\n", ctrl, cmd);

	if (cmd == NAND_CMD_NONE)
		return;

	if (ctrl & NAND_CLE)
		writeb(cmd & 0xFF, 0x80010000);
	else if (ctrl & NAND_ALE)
		writeb(cmd & 0xFF, 0x80020000);
}

#if 0
static int stm32xx_nand_dev_ready(struct mtd_info *mtd)
{
	return  & STAT_NAND_READY;
}
#endif

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
	while (len-- > 0)
		*buf++ = readb(0x80000000);
}

static void stm32xx_write_buf(struct mtd_info *mtd, const uint8_t *buf, int len)
{
	while (len-- > 0)
		writeb(*buf++, 0x80000000);
}

static void stm32xx_nand_init(void)
{
	unsigned int reg_value = 0;

	reg_value = STM32F7XX_FMC_REG(PCR);
	reg_value &= (~(3 << 4));
	reg_value &= (~(1 | 1 << 6));
	reg_value |= 1 << 3;
	reg_value |= 1 << 2;
	reg_value |= 10 << 9;
	reg_value |= 10 << 13;

	STM32F7XX_FMC_REG(PCR) = reg_value;
	STM32F7XX_FMC_REG(PMEM) = (10 | 10 << 8 | 10 << 16 | 10 << 24);
	STM32F7XX_FMC_REG(PATT) = (10 | 10 << 8 | 10 << 16 | 10 << 24);
	
	mdelay(2);
}

int board_nand_init(struct nand_chip *nand)
{
	nand->cmd_ctrl  = stm32xx_nand_cmd_ctrl;
	//stm32xx_chip->dev_ready = stm32xx_nand_dev_ready;
	
	/*
	 * The implementation of these functions is quite common, but
	 * they MUST be defined, because access to data register
	 * is strictly 32-bit aligned.
	 */
	nand->read_byte  = stm32xx_read_byte;
	nand->write_byte = stm32xx_write_byte;

	nand->ecc.mode = NAND_ECC_NONE;
	/*
	 * The implementation of these functions is quite common, but
	 * they MUST be defined, because access to data register
	 * is strictly 32-bit aligned.
	 */
	nand->read_buf   = stm32xx_read_buf;
	nand->write_buf  = stm32xx_write_buf;
	
	/* Initialize NAND interface */
	stm32xx_nand_init();

	return 0;
}
