#ifndef _RESUMEFLAGS_H
#define _RESUMEFLAGS_H

//TODO disable this definition if CONFIG_ macro is undefined.

#include <linux/types.h>

/**
 * resume_notifierドライバが保持している、resume通知フラグの
 * 状態を取得する.
 * 
 * @param flag_id 取得するフラグの番号.
 * @return フラグ状態をbool値で取得する.
 * @version 1.0
 */
extern bool get_resume_notifier_flag(unsigned int flag_id);

/**
 * #get_resume_notifier_flag の引数に使うフラグの定義.
 */
enum resume_notifier_flag_id
{
	/** BU-DET,ACC-OFFの割り込みが有効か. */
	RNF_BUDET_ACCOFF_INTERRUPT_MASK
};

#endif
