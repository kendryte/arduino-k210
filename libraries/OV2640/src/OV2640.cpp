#include "OV2640.h"

#include "ov2640_regs.h"

#define SVGA_HSIZE     (800)
#define SVGA_VSIZE     (600)

#define UXGA_HSIZE     (1600)
#define UXGA_VSIZE     (1200)

static const uint8_t sensor_default_regs[][2] = {
    {0xff, 0x01},
    {0x12, 0x80},
    {0xff, 0x00},
    {0x2c, 0xff},
    {0x2e, 0xdf},
    {0xff, 0x01},
    {0x3c, 0x32},
    {0x11, 0x00},
    {0x09, 0x02},
    {0x04, 0x08}, //0xD8 //0x88
    {0x13, 0xe5},
    {0x14, 0x48},
    {0x2c, 0x0c},
    {0x33, 0x78},
    {0x3a, 0x33},
    {0x3b, 0xfb},
    {0x3e, 0x00},
    {0x43, 0x11},
    {0x16, 0x10},
    {0x39, 0x92},
    {0x35, 0xda},
    {0x22, 0x1a},
    {0x37, 0xc3},
    {0x23, 0x00},
    {0x34, 0xc0},
    {0x36, 0x1a},
    {0x06, 0x88},
    {0x07, 0xc0},
    {0x0d, 0x87},
    {0x0e, 0x41},
    {0x4c, 0x00},
    {0x48, 0x00},
    {0x5b, 0x00},
    {0x42, 0x03},
    {0x4a, 0x81},
    {0x21, 0x99},
    {0x24, 0x40},
    {0x25, 0x38},
    {0x26, 0x82},
    {0x5c, 0x00},
    {0x63, 0x00},
    {0x46, 0x22},
    {0x0c, 0x3c},
    {0x61, 0x70},
    {0x62, 0x80},
    {0x7c, 0x05},
    {0x20, 0x80},
    {0x28, 0x30},
    {0x6c, 0x00},
    {0x6d, 0x80},
    {0x6e, 0x00},
    {0x70, 0x02},
    {0x71, 0x94},
    {0x73, 0xc1},
    {0x3d, 0x34},
    {0x5a, 0x57},
    {0x12, 0x40},
    {0x17, 0x11},
    {0x18, 0x43},
    {0x19, 0x00},
    {0x1a, 0x4b},
    {0x32, 0x09},
    {0x37, 0xc0},
    {0x4f, 0xca},
    {0x50, 0xa8},
    {0x5a, 0x23},
    {0x6d, 0x00},
    {0x3d, 0x38},
    {0xff, 0x00},
    {0xe5, 0x7f},
    {0xf9, 0xc0},
    {0x41, 0x24},
    {0xe0, 0x14},
    {0x76, 0xff},
    {0x33, 0xa0},
    {0x42, 0x20},
    {0x43, 0x18},
    {0x4c, 0x00},
    {0x87, 0xd5},
    {0x88, 0x3f},
    {0xd7, 0x03},
    {0xd9, 0x10},
    {0xd3, 0x82},
    {0xc8, 0x08},
    {0xc9, 0x80},
    {0x7c, 0x00},
    {0x7d, 0x00},
    {0x7c, 0x03},
    {0x7d, 0x48},
    {0x7d, 0x48},
    {0x7c, 0x08},
    {0x7d, 0x20},
    {0x7d, 0x10},
    {0x7d, 0x0e},
    {0x90, 0x00},
    {0x91, 0x0e},
    {0x91, 0x1a},
    {0x91, 0x31},
    {0x91, 0x5a},
    {0x91, 0x69},
    {0x91, 0x75},
    {0x91, 0x7e},
    {0x91, 0x88},
    {0x91, 0x8f},
    {0x91, 0x96},
    {0x91, 0xa3},
    {0x91, 0xaf},
    {0x91, 0xc4},
    {0x91, 0xd7},
    {0x91, 0xe8},
    {0x91, 0x20},
    {0x92, 0x00},
    {0x93, 0x06},
    {0x93, 0xe3},
    {0x93, 0x05},
    {0x93, 0x05},
    {0x93, 0x00},
    {0x93, 0x04},
    {0x93, 0x00},
    {0x93, 0x00},
    {0x93, 0x00},
    {0x93, 0x00},
    {0x93, 0x00},
    {0x93, 0x00},
    {0x93, 0x00},
    {0x96, 0x00},
    {0x97, 0x08},
    {0x97, 0x19},
    {0x97, 0x02},
    {0x97, 0x0c},
    {0x97, 0x24},
    {0x97, 0x30},
    {0x97, 0x28},
    {0x97, 0x26},
    {0x97, 0x02},
    {0x97, 0x98},
    {0x97, 0x80},
    {0x97, 0x00},
    {0x97, 0x00},
    {0xc3, 0xed},
    {0xa4, 0x00},
    {0xa8, 0x00},
    {0xc5, 0x11},
    {0xc6, 0x51},
    {0xbf, 0x80},
    {0xc7, 0x10},
    {0xb6, 0x66},
    {0xb8, 0xa5},
    {0xb7, 0x64},
    {0xb9, 0x7c},
    {0xb3, 0xaf},
    {0xb4, 0x97},
    {0xb5, 0xff},
    {0xb0, 0xc5},
    {0xb1, 0x94},
    {0xb2, 0x0f},
    {0xc4, 0x5c},
    {0xc0, 0x64},
    {0xc1, 0x4b},
    {0x8c, 0x00},
    {0x86, 0x3d},
    {0x50, 0x00},
    {0x51, 0xc8},
    {0x52, 0x96},
    {0x53, 0x00},
    {0x54, 0x00},
    {0x55, 0x00},
    {0x5a, 0xc8},
    {0x5b, 0x96},
    {0x5c, 0x00},
    {0xd3, 0x02},
    {0xc3, 0xed},
    {0x7f, 0x00},
    {0xda, 0x08},
    {0xe5, 0x1f},
    {0xe1, 0x67},
    {0xe0, 0x00},
    {0xdd, 0x7f},
    {0x05, 0x00},
    {0xff, 0x00},
    {0xe0, 0x04},
    {0x5a, 0x50},
    {0x5b, 0x3c},
    {0x5c, 0x00},
    {0xe0, 0x00},
    {0x00, 0x00},
};

static const uint8_t svga_config[][2] = {
    {0xff, 0x01}, // bank sel
    {0x35, 0xda}, //[SVGA]:
    {0x22, 0x1a}, //[SVGA]:
    {0x37, 0xc3}, //[SVGA]:
    {0x34, 0xc0}, //[SVGA]:
    {0x06, 0x88}, //[SVGA]:
    {0x0d, 0x87}, //[SVGA]:
    {0x0e, 0x41}, //[SVGA]:
    {0x42, 0x03}, //[SVGA]:
    {0x3d, 0x34}, //[SVGA]:
    {0x12, 0x40}, //[SVGA]:  COM7,COM7_RES_SVGA  SVGA
    {0x03, 0x0f}, //[SVGA]:  COM1,0x0F
    {0x17, 0x11}, //[SVGA]:HSTART
    {0x18, 0x43}, //[SVGA]:HSTOP
    {0x19, 0x00}, //[SVGA]:VSTART
    {0x1a, 0x4b}, //[SVGA]:VSTOP
    {0x32, 0x09}, //[SVGA]:REG32

    {0xff, 0x00}, // bank sel
    {0xc0, 0x64}, //[SVGA]:HSIZE8 SVGA_HSIZE>>3
    {0xc1, 0x4b}, //[SVGA]:VSIZE8 SVGA_VSIZE>>3
    {0x8c, 0x00}, //[SVGA]:SIZEL
    {0x86, 0x3d}, //[SVGA]:
    {0x50, 0x00}, //[SVGA]:CTRLI
    {0x51, 0xc8}, //[SVGA]:HSIZE
    {0x52, 0x96}, //[SVGA]:VSIZE
    {0x53, 0x00}, //[SVGA]:XOFFL
    {0x54, 0x00}, //[SVGA]:YOFFL
    {0x55, 0x00}, //[SVGA]:VHYX
    {0xd3, 0x02}, //[SVGA]:R_DVP_SP
    {0, 0},
};

static const uint8_t uxga_regs[][2] = {
    {BANK_SEL, BANK_SEL_SENSOR},
    /* DSP input image resoultion and window size control */
    {COM7, COM7_RES_UXGA},
    {COM1, 0x0F},  /* UXGA=0x0F, SVGA=0x0A, CIF=0x06 */
    {REG32, 0x36}, /* UXGA=0x36, SVGA/CIF=0x09 */

    {HSTART, 0x11}, /* UXGA=0x11, SVGA/CIF=0x11 */
    {HSTOP, 0x75},  /* UXGA=0x75, SVGA/CIF=0x43 */

    {VSTART, 0x01}, /* UXGA=0x01, SVGA/CIF=0x00 */
    {VSTOP, 0x97},  /* UXGA=0x97, SVGA/CIF=0x4b */
    {0x3d, 0x34},   /* UXGA=0x34, SVGA/CIF=0x38 */

    {0x35, 0x88},
    {0x22, 0x0a},
    {0x37, 0x40},
    {0x34, 0xa0},
    {0x06, 0x02},
    {0x0d, 0xb7},
    {0x0e, 0x01},
    {0x42, 0x83},

    /* Set DSP input image size and offset.
       The sensor output image can be scaled with OUTW/OUTH */
    {BANK_SEL, BANK_SEL_DSP},
    {R_BYPASS, R_BYPASS_DSP_BYPAS},

    {RESET, RESET_DVP},
    {HSIZE8, (UXGA_HSIZE >> 3)}, /* Image Horizontal Size HSIZE[10:3] */
    {VSIZE8, (UXGA_VSIZE >> 3)}, /* Image Vertiacl Size VSIZE[10:3] */

    /* {HSIZE[11], HSIZE[2:0], VSIZE[2:0]} */
    {SIZEL, ((UXGA_HSIZE >> 6) & 0x40) | ((UXGA_HSIZE & 0x7) << 3) | (UXGA_VSIZE & 0x7)},

    {XOFFL, 0x00},                       /* OFFSET_X[7:0] */
    {YOFFL, 0x00},                       /* OFFSET_Y[7:0] */
    {HSIZE, ((UXGA_HSIZE >> 2) & 0xFF)}, /* H_SIZE[7:0] real/4 */
    {VSIZE, ((UXGA_VSIZE >> 2) & 0xFF)}, /* V_SIZE[7:0] real/4 */

    /* V_SIZE[8]/OFFSET_Y[10:8]/H_SIZE[8]/OFFSET_X[10:8] */
    {VHYX, ((UXGA_VSIZE >> 3) & 0x80) | ((UXGA_HSIZE >> 7) & 0x08)},
    {TEST, (UXGA_HSIZE >> 4) & 0x80}, /* H_SIZE[9] */

    {CTRL2, CTRL2_DCW_EN | CTRL2_SDE_EN | CTRL2_UV_AVG_EN | CTRL2_CMX_EN | CTRL2_UV_ADJ_EN},

    /* H_DIVIDER/V_DIVIDER */
    {CTRLI, CTRLI_LP_DP | 0x00},
    /* DVP prescalar */
    {R_DVP_SP, R_DVP_SP_AUTO_MODE | 0x04},

    {R_BYPASS, R_BYPASS_DSP_EN},
    {RESET, 0x00},
    {0, 0},
};

#define NUM_BRIGHTNESS_LEVELS (5)
static const uint8_t brightness_regs[NUM_BRIGHTNESS_LEVELS + 1][5] = {
    {BPADDR, BPDATA, BPADDR, BPDATA, BPDATA},
    {0x00, 0x04, 0x09, 0x00, 0x00}, /* -2 */
    {0x00, 0x04, 0x09, 0x10, 0x00}, /* -1 */
    {0x00, 0x04, 0x09, 0x20, 0x00}, /*  0 */
    {0x00, 0x04, 0x09, 0x30, 0x00}, /* +1 */
    {0x00, 0x04, 0x09, 0x40, 0x00}, /* +2 */
};

#define NUM_CONTRAST_LEVELS (5)
static const uint8_t contrast_regs[NUM_CONTRAST_LEVELS + 1][7] = {
    {BPADDR, BPDATA, BPADDR, BPDATA, BPDATA, BPDATA, BPDATA},
    {0x00, 0x04, 0x07, 0x20, 0x18, 0x34, 0x06}, /* -2 */
    {0x00, 0x04, 0x07, 0x20, 0x1c, 0x2a, 0x06}, /* -1 */
    {0x00, 0x04, 0x07, 0x20, 0x20, 0x20, 0x06}, /*  0 */
    {0x00, 0x04, 0x07, 0x20, 0x24, 0x16, 0x06}, /* +1 */
    {0x00, 0x04, 0x07, 0x20, 0x28, 0x0c, 0x06}, /* +2 */
};

#define NUM_SATURATION_LEVELS (5)
static const uint8_t saturation_regs[NUM_SATURATION_LEVELS + 1][5] = {
    {BPADDR, BPDATA, BPADDR, BPDATA, BPDATA},
    {0x00, 0x02, 0x03, 0x28, 0x28}, /* -2 */
    {0x00, 0x02, 0x03, 0x38, 0x38}, /* -1 */
    {0x00, 0x02, 0x03, 0x48, 0x48}, /*  0 */
    {0x00, 0x02, 0x03, 0x58, 0x58}, /* +1 */
    {0x00, 0x02, 0x03, 0x58, 0x58}, /* +2 */
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OV2640::OV2640(int8_t sda, int8_t scl, int i2cNum)
:Camera(i2cNum, uint16_t(0x30))
,_sdaPin(sda)
,_sclPin(scl)
{

}

OV2640::~OV2640(void)
{

}

int OV2640::reset(framesize_t framesize, camera_buffers_t *buff)
{
    int w = K210::_camera_resolution[framesize][0];
    int h = K210::_camera_resolution[framesize][1];

    if ((0x00 == w) || (0x00 == h))
    {
        return -1;
    }

    if ((0 > _sdaPin) || (0 > _sclPin))
    {
        return -1;
    }

    if((NULL == _wire) || (false == _wire->begin(_sclPin, _sdaPin, 1000 * 400)))
    {
		rt_kprintf("begin wire failed\n");
        return -1;
    }

    if(0 != begin(w, h, buff))
    {
        rt_kprintf("dvp begin failed\n");
        return -1;
    }
    delay(10);

    for(size_t i = 0; (0x00 != (sensor_default_regs[i][0] + sensor_default_regs[i][1])); i++)
    {
		if(0x00 == sensor_default_regs[i][0])
		{
			delay(30);
		}
        write_reg(sensor_default_regs[i][0], sensor_default_regs[i][1]);
        delay(1);
    }

    if(0x00 != set_framesize(framesize))
    {
        rt_kprintf("set output size failed\n");
        return -1;
    }

    _imgWidth = w;
    _imgHeight = h;

    _sensorVflip = false;
    _sensorHmirror = false;

    return 0;
}

int OV2640::read_id(void)
{
    int t, id = 0;

    if(0x00 != write_reg(0xFF, 0x01))
    {
        return -1;
    }

    read_reg(0x0A, &t);
    id = t << 8;
    read_reg(0x0B, &t);
    id |= t;

    return id;
}

int OV2640::set_framesize(framesize_t framesize)
{
    int ret = 0, clkrc = 0;
    int w = K210::_camera_resolution[framesize][0];
    int h = K210::_camera_resolution[framesize][1];

    const uint8_t(*regs)[2];

    if ((w <= 800) && (h <= 600))
    {
        clkrc = 0x80;
        regs = svga_config;
    }
    else
    {
        clkrc = 0x81;
        regs = uxga_regs;
    }

    /* Disable DSP */
    ret += write_reg(BANK_SEL, BANK_SEL_DSP);
    ret += write_reg(R_BYPASS, R_BYPASS_DSP_BYPAS);

    /* Set CLKRC */
    if (clkrc == 0x81)
    {
        ret |= write_reg(BANK_SEL, BANK_SEL_SENSOR);
        ret |= write_reg(CLKRC, clkrc);
    }

    /* Write DSP input regsiters */
    int index = 0;
    while (regs[index][0])
    {
        write_reg(regs[index][0], regs[index][1]);
        index++;
    }

    /* Write output width */
    ret += write_reg(0xe0, 0x04);                                   /* OUTH[8]/OUTW[9:8] */
    ret += write_reg(ZMOW, (w >> 2) & 0xFF);                        /* OUTW[7:0] (real/4) */
    ret += write_reg(ZMOH, (h >> 2) & 0xFF);                        /* OUTH[7:0] (real/4) */
    ret += write_reg(ZMHH, ((h >> 8) & 0x04) | ((w >> 10) & 0x03)); /* OUTH[8]/OUTW[9:8] */
    ret += write_reg(0xe0, 0x00);                                   /* OUTH[8]/OUTW[9:8] */

    /* Enable DSP */
    ret += write_reg(BANK_SEL, BANK_SEL_DSP);
    ret += write_reg(R_BYPASS, R_BYPASS_DSP_EN);

    return ret;
}

int OV2640::set_hmirror(int enable)
{
    int ret = 0, reg = 0;

    ret += read_reg(BANK_SEL, &reg);
    ret += write_reg(BANK_SEL, reg | BANK_SEL_SENSOR);
    ret += read_reg(REG04, &reg);

    if (enable) {
        _sensorHmirror = true;

        reg |= REG04_HFLIP_IMG;
    } else {
        _sensorHmirror = false;

        reg &= ~REG04_HFLIP_IMG;
    }

    ret += write_reg(REG04, reg);

    return ret;
}

int OV2640::set_vflip(int enable)
{
    int ret = 0, reg = 0;

    ret += read_reg(BANK_SEL, &reg);
    ret += write_reg(BANK_SEL, reg | BANK_SEL_SENSOR);
    ret += read_reg(REG04, &reg);

    if (enable) {
        _sensorVflip = true;

        reg |= REG04_VFLIP_IMG;
        reg |= REG04_VREF_EN;
    } else {
        _sensorVflip = false;

        reg &= ~REG04_VFLIP_IMG;
        reg &= ~REG04_VREF_EN;
    }

    ret += write_reg(REG04, reg);

    return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int OV2640::set_windowing(framesize_t framesize, int x, int y, int w, int h)
{
    return -1;
}

int OV2640::set_contrast(int level)
{
    int ret = 0;

    level += (NUM_CONTRAST_LEVELS / 2 + 1);
    if (level < 0 || level > NUM_CONTRAST_LEVELS)
    {
        return -1;
    }

    /* Switch to DSP register bank */
    ret += write_reg(BANK_SEL, BANK_SEL_DSP);

    /* Write contrast registers */
    for (int i = 0; i < sizeof(contrast_regs[0]) / sizeof(contrast_regs[0][0]); i++)
    {
        ret += write_reg(contrast_regs[0][i], contrast_regs[level][i]);
    }

    return ret;
}

int OV2640::set_brightness(int level)
{
    int ret = 0;

    level += (NUM_BRIGHTNESS_LEVELS / 2 + 1);
    if (level < 0 || level > NUM_BRIGHTNESS_LEVELS)
    {
        return -1;
    }

    /* Switch to DSP register bank */
    ret += write_reg(BANK_SEL, BANK_SEL_DSP);

    /* Write brightness registers */
    for (int i = 0; i < sizeof(brightness_regs[0]) / sizeof(brightness_regs[0][0]); i++)
    {
        ret += write_reg(brightness_regs[0][i], brightness_regs[level][i]);
    }

    return ret;
}

int OV2640::set_saturation(int level)
{
    int ret = 0;

    level += (NUM_SATURATION_LEVELS / 2 + 1);
    if (level < 0 || level > NUM_SATURATION_LEVELS)
    {
        return -1;
    }

    /* Switch to DSP register bank */
    ret += write_reg(BANK_SEL, BANK_SEL_DSP);

    /* Write contrast registers */
    for (int i = 0; i < sizeof(saturation_regs[0]) / sizeof(saturation_regs[0][0]); i++)
    {
        ret += write_reg(saturation_regs[0][i], saturation_regs[level][i]);
    }

    return ret;
}

int OV2640::set_gainceiling(gainceiling_t gainceiling)
{
    int ret = 0;

    /* Switch to SENSOR register bank */
    ret += write_reg(BANK_SEL, BANK_SEL_SENSOR);

    /* Write gain ceiling register */
    ret += write_reg(COM9, COM9_AGC_SET(gainceiling));

    return ret;
}

int OV2640::set_colorbar(int enable)
{
    int ret = 0, reg = 0;

    ret += write_reg(BANK_SEL, BANK_SEL_SENSOR);
    ret += read_reg(COM7, &reg);

    if (enable)
    {
        reg |= COM7_COLOR_BAR;
    }
    else
    {
        reg &= ~COM7_COLOR_BAR;
    }

    ret += write_reg(COM7, reg);

    return ret;
}

int OV2640::set_auto_gain(int enable, float gain_db, float gain_db_ceiling)
{
    return -1;
}

int OV2640::get_gain_db(float *gain_db)
{
    return -1;
}

int OV2640::set_auto_exposure(int enable, int exposure_us)
{
    return -1;
}

int OV2640::get_exposure_us(int *exposure_us)
{
    return -1;
}

int OV2640::set_auto_whitebal(int enable, uint8_t r_gain_db, uint8_t g_gain_db, uint8_t b_gain_db)
{
    int ret = 0, reg = 0;

    ret += read_reg(BANK_SEL, &reg);
    ret += write_reg(BANK_SEL, reg & (~BANK_SEL_SENSOR));
    ret += read_reg(CTRL1, &reg);
    ret += write_reg(CTRL1, (reg & (~CTRL1_AWB)) | ((enable != 0) ? CTRL1_AWB : 0));

    return ret;
}

int OV2640::get_rgb_gain_db(uint8_t *r_gain_db, uint8_t *g_gain_db, uint8_t *b_gain_db)
{
    int ret = 0, reg = 0;

    ret += read_reg(BANK_SEL, &reg);
    ret += write_reg(BANK_SEL, reg & (~BANK_SEL_SENSOR));
    ret += read_reg(CTRL1, &reg);

    return ret;
}
