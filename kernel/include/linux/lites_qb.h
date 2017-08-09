/**
 * @file lites_qb.h
 *
 * @brief QuickBoot向け suspend/resume処理APIの定義
 *
 * Copyright (c) 2012 FUJITSU TEN LIMITED and
 * FUJITSU COMPUTER TECHNOLOGIES LIMITED. All rights reserved.
 */
#ifndef __LITES_QB_H
#define __LITES_QB_H

/**
 * @brief suspend処理関数
 *        snapshotdriverのハードレジスタ退避の最後で呼ばれる想定
 *
 * @retval 0  正常終了
 */
extern int lites_suspend(void);

/**
 * @brief resume処理関数
 *        snapshotdriverのハードレジスタ復元の最初で呼ばれる想定
 *
 * @retval 0  正常終了
 */
extern int lites_resume(void);

#endif  /** __LITES_QB_H */
