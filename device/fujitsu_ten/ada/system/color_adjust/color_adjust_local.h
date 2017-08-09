/*--------------------------------------------------------------------------*/
// COPYRIGHT(C) FUJITSU TEN LIMITED 2012,2013
/*--------------------------------------------------------------------------*/

/* 2013.08.08 14A-DA FTEN [Camera Transparent Color] add start */
/**
 * Display color driver local definition
 */

#define PALETTE_REG_TABLE_MAX_CNT   256
#define PALETTE_REG_CNT             3

#define PALETTE_R_REG_NO            0
#define PALETTE_G_REG_NO            1
#define PALETTE_B_REG_NO            2

#define OFFSET_TABLE_MAX_CNT        256

#define PALETTE_REG_MAX_VAL         255
#define PALETTE_REG_MIN_VAL         0
/* 2013.08.08 14A-DA FTEN [Camera Transparent Color] add end */
/* 2013.11.21 14A-DA FTEN [Color/Tint change] add start */
#define ADJUST_MIN_VAL              0
#define ADJUST_MAX_VAL              11
/* 2013.11.21 14A-DA FTEN [Color/Tint change] add end */


struct CSC_COEFFICIENT {
    int      yof;
    float    kyrgb;
    float    kur;
    float    kvr;
    float    kug;
    float    kvg;
    float    kub;
    float    kvb;
};

struct COLOR_DATA {
    struct CSC_COEFFICIENT    csc;
/* 2013.08.08 14A-DA FTEN [Camera Transparent Color] modify start */
    unsigned char             palette[PALETTE_REG_CNT][PALETTE_REG_TABLE_MAX_CNT];
/* 2013.08.08 14A-DA FTEN [Camera Transparent Color] modify end */
};

static unsigned short f_fmt_s28(float);
static unsigned short f_fmt_28 (float);
static unsigned short f_fmt_s18(float);
static int write_adjust(struct COLOR_DATA *);
/* 2013.11.21 14A-DA FTEN [Color/Tint change] add start */
static void coloradajust_paramcheck(struct COLOR_ADJ_PARAM *checkparam);
static int  coloradajust_paramoffset(int checkdata);
/* 2013.11.21 14A-DA FTEN [Color/Tint change] add end */

/* 2013.08.08 14A-DA FTEN [Camera Transparent Color] modify start */
extern int cont_tbl[][OFFSET_TABLE_MAX_CNT];
extern int blacklevel_tbl[][OFFSET_TABLE_MAX_CNT];
extern int gamma_r_tbl[];
extern int gamma_g_tbl[];
extern int gamma_b_tbl[];
/* 2013.08.08 14A-DA FTEN [Camera Transparent Color] modify end */

extern struct CSC_COEFFICIENT csc_color_shade_tbl[];
extern struct CSC_COEFFICIENT csc_tint_tbl[];
extern struct CSC_COEFFICIENT csc_cont_tbl[];
extern struct CSC_COEFFICIENT csc_blacklevel_tbl[];

