/*--------------------------------------------------------------------------*/
// COPYRIGHT(C) FUJITSU TEN LIMITED 2012,2013
/*--------------------------------------------------------------------------*/
/**
 * Display color adjust API
 */
#define LOG_TAG "color_adjust_api"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>
#include <linux/fb.h>
#include <tegra_dc_ext.h>

#include <display_color_drv.h>
#include <color_adjust/color_adjust_api.h>
#include "color_adjust_local.h"

#include <utils/Log.h>

struct COLOR_DATA    color_curr;

/* 2013.08.08 14A-DA FTEN [Camera Transparent Color] add start */
static int isCamera = COLOR_ADJ_MODE_OTHER;
/* 2013.08.08 14A-DA FTEN [Camera Transparent Color] add end */

/**
 * @brief  [同期] カラー調整ライブラリメイン
 * 
 * @param  struct COLOR_ADJ_PARAM *param : 各調整項目のユーザー調整値
 *         int                     mode  : カメラ中モード
 *                                           COLOR_ADJ_MODE_CAMERA
 *                                           COLOR_ADJ_MODE_OTHER
 * @retval int    write_adjust()返却値
 *
 * @note   パレットレジスタ/CSCレジスタ設定値の計算を行う
 *         カメラモード中は、カメラ透過色用の設定値となる
 */
int color_set_adjustment(struct COLOR_ADJ_PARAM *param, int mode)
{
    int    pal_val;
    int    ctr;
/* 2013.08.08 14A-DA FTEN [Camera Transparent Color] add start */
    int    col;
    int   *gamma;
    
    isCamera = mode;
/* 2013.08.08 14A-DA FTEN [Camera Transparent Color] add end */

/* 2013.11.21 14A-DA FTEN [Color/Tint change] add start */
	coloradajust_paramcheck(param);
/* 2013.11.21 14A-DA FTEN [Color/Tint change] add end */

    color_curr.csc.yof   =
        csc_color_shade_tbl [param->csc_color_shade ].yof +
        csc_tint_tbl        [param->csc_tint        ].yof +
        csc_cont_tbl        [param->csc_contrast    ].yof +
        csc_blacklevel_tbl  [param->csc_black_level ].yof;
    color_curr.csc.kyrgb =
        csc_color_shade_tbl [param->csc_color_shade ].kyrgb +
        csc_tint_tbl        [param->csc_tint        ].kyrgb +
        csc_cont_tbl        [param->csc_contrast    ].kyrgb +
        csc_blacklevel_tbl  [param->csc_black_level ].kyrgb;
    color_curr.csc.kur   =
        csc_color_shade_tbl [param->csc_color_shade ].kur *
        csc_tint_tbl        [param->csc_tint        ].kur *
        csc_cont_tbl        [param->csc_contrast    ].kur *
        csc_blacklevel_tbl  [param->csc_black_level ].kur;
    color_curr.csc.kvr   =
        csc_color_shade_tbl [param->csc_color_shade ].kvr *
        csc_tint_tbl        [param->csc_tint        ].kvr *
        csc_cont_tbl        [param->csc_contrast    ].kvr *
        csc_blacklevel_tbl  [param->csc_black_level ].kvr;
    color_curr.csc.kug   =
        csc_color_shade_tbl [param->csc_color_shade ].kug *
        csc_tint_tbl        [param->csc_tint        ].kug *
        csc_cont_tbl        [param->csc_contrast    ].kug *
        csc_blacklevel_tbl  [param->csc_black_level ].kug;
    color_curr.csc.kvg   =
        csc_color_shade_tbl [param->csc_color_shade ].kvg *
        csc_tint_tbl        [param->csc_tint        ].kvg *
        csc_cont_tbl        [param->csc_contrast    ].kvg *
        csc_blacklevel_tbl  [param->csc_black_level ].kvg;
    color_curr.csc.kub   =
        csc_color_shade_tbl [param->csc_color_shade ].kub *
        csc_tint_tbl        [param->csc_tint        ].kub *
        csc_cont_tbl        [param->csc_contrast    ].kub *
        csc_blacklevel_tbl  [param->csc_black_level ].kub;
    color_curr.csc.kvb   =
        csc_color_shade_tbl [param->csc_color_shade ].kvb *
        csc_tint_tbl        [param->csc_tint        ].kvb *
        csc_cont_tbl        [param->csc_contrast    ].kvb *
        csc_blacklevel_tbl  [param->csc_black_level ].kvb;

/* 2013.08.08 14A-DA FTEN [Camera Transparent Color] modify start */
    for(col = 0; col < PALETTE_REG_CNT; ++col) {
        if(col == PALETTE_R_REG_NO)
            gamma = gamma_r_tbl;
        else if(col == PALETTE_G_REG_NO)
            gamma = gamma_g_tbl;
        else if(col == PALETTE_B_REG_NO)
            gamma = gamma_b_tbl;

        for(ctr = 0; ctr < OFFSET_TABLE_MAX_CNT; ++ctr) {
            pal_val = ctr + cont_tbl      [param->pal_contrast]   [ctr] +
                            blacklevel_tbl[param->pal_black_level][ctr] +
                            gamma                                 [ctr];
            if(pal_val < PALETTE_REG_MIN_VAL) {
                pal_val = PALETTE_REG_MIN_VAL;
            }
            if(pal_val > PALETTE_REG_MAX_VAL) {
                pal_val = PALETTE_REG_MAX_VAL;
            }
            
            /* Camera Trasnparent Color Support. */
            if(isCamera) {
                if(ctr == PALETTE_REG_MIN_VAL) {
                    pal_val = PALETTE_REG_MIN_VAL;
                }
                if(ctr == PALETTE_REG_MAX_VAL) {
                    pal_val = PALETTE_REG_MAX_VAL;
                }
            }
            color_curr.palette[col][ctr] = pal_val;
        }
    }
    return write_adjust(&color_curr);
/* 2013.08.08 14A-DA FTEN [Camera Transparent Color] modify end */
}

/**
 * @brief  [同期] 浮動小数点数変換(符号有 整数2bit小数8bit)
 * 
 * @param  float fdata     : 調整値(標準値と補正値による調整式後の値)
 * @retval unsigned short  : 変換後の値
 *
 * @note   KUR/KVR/KUB/KVBレジスタ設定用浮動小数点変換
 */
static unsigned short f_fmt_s28(float fdata)
{
    unsigned long    *uldata;
    unsigned short    intval;
    int               shr;
    int               sign;
    int               int_val;

    uldata  = (unsigned long *)&fdata;
    int_val = 0 + fdata;
    intval  = *uldata;
    shr     = 127 - (( *uldata >> 23 ) & 0x000000FF);
    sign    = (*uldata >> 31) & 0x00000001;
    if( shr < 0 ) {
/* FUJITSU TEN:2012-12-21 Color Adjust modified. start */
        intval = ((((*uldata & 0x7FFFFF) | 0x00800000) << (0-shr)) >> 15) & 0x000000FF;
    }
    else {
        intval = ((((*uldata & 0x7FFFFF) | 0x00800000) >> shr) >> 15) & 0x000000FF;
/* FUJITSU TEN:2012-12-21 Color Adjust modified. end */
    }
    if( sign ) {
        intval ^= 0x000000FF;
/* FUJITSU TEN:2012-12-25 Color Adjust modified. start */
        int_val = (int_val - 1) & 0x0003;
/* FUJITSU TEN:2012-12-25 Color Adjust modified. end */
    }
    intval = intval | ((int_val << 8) & 0x00000300);
    intval = intval | (sign << 10);
    return intval;
}

/**
 * @brief  [同期] 浮動小数点数変換(符号無 整数2bit小数8bit)
 * 
 * @param  float fdata     : 調整値(標準値と補正値による調整式後の値)
 * @retval unsigned short  : 変換後の値
 *
 * @note   KYRGBレジスタ設定用浮動小数点変換
 */
static unsigned short f_fmt_28(float fdata)
{
    unsigned long    *uldata;
    unsigned short    intval;
    int               shr;
    unsigned short    int_val;

    uldata  = (unsigned long *)&fdata;
    int_val = 0 + fdata;
    intval  = *uldata;
    shr     = 127 - (( *uldata >> 23 ) & 0x000000FF);
    if( shr < 0 ) {
/* FUJITSU TEN:2012-12-21 Color Adjust modified. start */
        intval = ((((*uldata & 0x7FFFFF) | 0x00800000) << (0-shr)) >> 15) & 0x000000FF;
    }
    else {
        intval = ((((*uldata & 0x7FFFFF) | 0x00800000) >> shr) >> 15) & 0x000000FF;
/* FUJITSU TEN:2012-12-21 Color Adjust modified. end */
    }
    intval = intval | ((int_val << 8) & 0x00000300);
    return intval;
}

/**
 * @brief  [同期] 浮動小数点数変換(符号有 整数1bit小数8bit)
 * 
 * @param  float fdata     : 調整値(標準値と補正値による調整式後の値)
 * @retval unsigned short  : 変換後の値
 *
 * @note   KUG/KVGレジスタ設定用浮動小数点変換
 */
static unsigned short f_fmt_s18(float fdata)
{
    unsigned long    *uldata;
    unsigned short    intval;
    int               shr;
    int               sign;
    short             int_val;

    uldata  = (unsigned long *)&fdata;
    int_val = 0 + fdata;
    intval  = *uldata;
    shr     = 127 - (( *uldata >> 23 ) & 0x000000FF);
    sign    = (*uldata >> 31) & 0x00000001;
    if( shr < 0 ) {
/* FUJITSU TEN:2012-12-21 Color Adjust modified. start */
        intval = ((((*uldata & 0x7FFFFF) | 0x00800000) << (0-shr)) >> 15) & 0x000000FF;
    }
    else {
        intval = ((((*uldata & 0x7FFFFF) | 0x00800000) >> shr) >> 15) & 0x000000FF;
/* FUJITSU TEN:2012-12-21 Color Adjust modified. end */
    }
    if( sign ) {
        intval ^= 0x000000FF;
/* FUJITSU TEN:2012-12-21 Color Adjust modified. start */
        int_val = (int_val - 1) & 0x0001;
/* FUJITSU TEN:2012-12-21 Color Adjust modified. end */
    }
    intval = intval | ((int_val << 8) & 0x00000100);
    intval = intval | (sign << 9);
    return intval;
}

/**
 * @brief  [同期] (画質調整TP用)レジスタ値初期化関数
 * 
 * @param  struct COLOR_DATA *color_data  : 色調整データ(レジスタ値)
 * @retval -(void)
 *
 * @note   各レジスタ値に基準値を設定
 */
static void set_default(struct COLOR_DATA *color_data)
{
    int    ctr;
/* 2013.08.08 14A-DA FTEN [Camera Transparent Color] add start */
    int    col;
/* 2013.08.08 14A-DA FTEN [Camera Transparent Color] add end */

    color_data->csc.yof   = 0;
    color_data->csc.kyrgb = 0.0;
    color_data->csc.kur   = 1.0;
    color_data->csc.kvr   = 1.0;
    color_data->csc.kug   = 1.0;
    color_data->csc.kvg   = 1.0;
    color_data->csc.kub   = 1.0;
    color_data->csc.kvb   = 1.0;
/* 2013.08.08 14A-DA FTEN [Camera Transparent Color] modify start */
    for(ctr = 0; ctr < PALETTE_REG_TABLE_MAX_CNT; ++ctr) {
        color_data->palette[PALETTE_R_REG_NO][ctr] = ctr;
        color_data->palette[PALETTE_G_REG_NO][ctr] = ctr;
        color_data->palette[PALETTE_B_REG_NO][ctr] = ctr;
    }
/* 2013.08.08 14A-DA FTEN [Camera Transparent Color] modify end */
}

/* 2013.08.08 14A-DA FTEN [Camera Transparent Color] modify start */
__u16 r_tmppal[PALETTE_REG_TABLE_MAX_CNT];
__u16 g_tmppal[PALETTE_REG_TABLE_MAX_CNT];
__u16 b_tmppal[PALETTE_REG_TABLE_MAX_CNT];
/* 2013.08.08 14A-DA FTEN [Camera Transparent Color] modify end */

/**
 * @brief  [同期] デバイスファイルへ書き込み
 * 
 * @param  struct COLOR_DATA *color_data  : 各レジスタ設定用の調整値
 * @retval int(enum COLOR_ADJ_RTN)        : 処理結果
 *                                            COLOR_ADJ_RTN_SUCCESS
 *                                            COLOR_ADJ_RTN_FAIL
 * @note   各調整値の丸め込みを行い、デバイスファイルへ書き込みを行う
 */
static int write_adjust(struct COLOR_DATA *color_data)
{
    struct fb_cmap              cmap;
    struct DISP_COLOR_CSC_PARAM cparam;
    int                         fb_fd;
    int                         color_fd;
    int                         ctr;
    int                         val;
    long                        sts;
    float                       cal;

    if((color_fd = open("/dev/color", O_RDONLY)) < 0) {
        LOGE("open error[/dev/color]\n");
        return COLOR_ADJ_RTN_FAIL;
    }

    cparam.yof = color_data->csc.yof + -16;

/* FUJITSU TEN:2012-12-21 Color Adjust modified. start */
    cal = color_data->csc.kyrgb + 1.1640625;
    if(cal > 3.996) {
        cal = 0x3FF;
    } else {
        cparam.kyrgb = f_fmt_28(cal);
    }

    cparam.kur = f_fmt_s28(color_data->csc.kur * 0.0);

    cal = color_data->csc.kvr * 1.59375;
    if(cal > 3.996) {
        cparam.kvr = 0x3FF;
    } else if(cal < -3.996) {
        cparam.kvr = 0x7FF;
    } else {
        cparam.kvr = f_fmt_s28(cal);
    }

    cal = color_data->csc.kug * -0.390625;
    if(cal > 1.996) {
        cparam.kug = 0x1FF;
    } else if(cal < -1.996) {
        cparam.kug = 0x3FF;
    } else {
        cparam.kug = f_fmt_s18(cal);
    }

    cal = color_data->csc.kvg * -0.8125;
    if(cal > 1.996) {
        cparam.kvg = 0x1FF;
    } else if(cal < -1.996) {
        cparam.kvg = 0x3FF;
    } else {
        cparam.kvg = f_fmt_s18(cal);
    }

    cal = color_data->csc.kub * 2.015625;
    if(cal > 3.996) {
        cparam.kub = 0x3FF;
    } else if(cal < -3.996) {
        cparam.kub = 0x7FF;
    } else {
        cparam.kub = f_fmt_s28(cal);
    }
/* FUJITSU TEN:2012-12-21 Color Adjust modified. end */
    cparam.kvb = f_fmt_s28(color_data->csc.kvb * 0.0);

    if((sts = ioctl(color_fd, DISP_COLOR_REQ_SET_CSC, &cparam)) != 0 ) {
        LOGE("ioctl error[DISP_COLOR_REQ_SET_CSC]\n");
        close(color_fd);
        return COLOR_ADJ_RTN_FAIL;
    }

    close(color_fd);


    if ((fb_fd = open("/dev/graphics/fb0", O_RDWR)) < 0) {
        LOGE("open error[/dev/graphics/fb0]\n");
        return COLOR_ADJ_RTN_FAIL;
    }

    for(ctr = 0; ctr < PALETTE_REG_TABLE_MAX_CNT; ++ctr) {
/* 2013.08.08 14A-DA FTEN [Camera Transparent Color] modify start */
        r_tmppal[ctr] = color_data->palette[PALETTE_R_REG_NO][ctr] << 8;
        g_tmppal[ctr] = color_data->palette[PALETTE_G_REG_NO][ctr] << 8;
        b_tmppal[ctr] = color_data->palette[PALETTE_B_REG_NO][ctr] << 8;
/* 2013.08.08 14A-DA FTEN [Camera Transparent Color] modify end */
    }

    cmap.start  = 0;
/* 2013.08.08 14A-DA FTEN [Camera Transparent Color] modify start */
    cmap.len    = PALETTE_REG_TABLE_MAX_CNT;
/* 2013.08.08 14A-DA FTEN [Camera Transparent Color] modify end */
    cmap.red    = r_tmppal;
    cmap.green  = g_tmppal;
    cmap.blue   = b_tmppal;
    cmap.transp = NULL;

    if((sts = ioctl(fb_fd, FBIOPUTCMAP, &cmap)) != 0 ) {
        LOGE("ioctl error[FBIOPUTCMAP]\n");
        close(fb_fd);
        return COLOR_ADJ_RTN_FAIL;
    }

    close(fb_fd);

    return COLOR_ADJ_RTN_SUCCESS;
}

/* 2013.11.21 14A-DA FTEN [Color/Tint change] add start */
/**
 * @brief  [同期] 調整値チェック
 * 
 * @param  struct COLOR_DATA *checkparam[I/O]  : 各レジスタ設定用の調整値
 * @retval -
 * @note   各調整値の有効値判定し、無効値はMINかMAXに補正する
 */
static void coloradajust_paramcheck(struct COLOR_ADJ_PARAM *checkparam)
{
    checkparam->csc_color_shade = coloradajust_paramoffset(checkparam->csc_color_shade);
    checkparam->csc_tint        = coloradajust_paramoffset(checkparam->csc_tint);
    checkparam->csc_contrast    = coloradajust_paramoffset(checkparam->csc_contrast);
    checkparam->csc_black_level = coloradajust_paramoffset(checkparam->csc_black_level);
    checkparam->pal_contrast    = coloradajust_paramoffset(checkparam->pal_contrast);
    checkparam->pal_black_level = coloradajust_paramoffset(checkparam->pal_black_level);

}

/**
 * @brief  [同期] 調整値丸め込み
 * 
 * @param  int checkdata[I/O]  : 範囲チェック対象調整値
 * @retval -
 * @note   各調整値の有効値判定し、無効値はMINかMAXに補正する
 */
static int coloradajust_paramoffset(int checkdata)
{
    int retval;

    if(checkdata > ADJUST_MAX_VAL) {
        retval = ADJUST_MAX_VAL;
    } else if(checkdata < ADJUST_MIN_VAL) {
        retval = ADJUST_MIN_VAL;
    } else {
        retval = checkdata;
    }

    return retval;

}
/* 2013.11.21 14A-DA FTEN [Color/Tint change] add end */


