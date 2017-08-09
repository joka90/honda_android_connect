/*--------------------------------------------------------------------------*/
// COPYRIGHT(C) FUJITSU TEN LIMITED 2012,2013
/*--------------------------------------------------------------------------*/
/**
 *	Display color library API definition
 */

#ifdef __cplusplus
extern "C" {
#endif

/* 2013.08.08 14A-DA FTEN [Camera Transparent Color] add start */
#define COLOR_ADJ_MODE_OTHER  0 /* Non-camera display state */
#define COLOR_ADJ_MODE_CAMERA 1 /* Camera display state */
/* 2013.08.08 14A-DA FTEN [Camera Transparent Color] add start */

enum COLOR_ADJ_RTN {
	COLOR_ADJ_RTN_SUCCESS,
	COLOR_ADJ_RTN_FAIL
};

struct COLOR_ADJ_PARAM {
	int		pal_black_level;
	int		pal_contrast;
	int		csc_color_shade;
	int		csc_tint;
	int		csc_black_level;
	int		csc_contrast;
};

/* 2013.08.08 14A-DA FTEN [Camera Transparent Color] modify start */
extern int color_set_adjustment(struct COLOR_ADJ_PARAM *param, int mode);
/* 2013.08.08 14A-DA FTEN [Camera Transparent Color] modify end */

#ifdef __cplusplus
}
#endif
