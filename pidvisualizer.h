/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license in the file COPYING
 * or http://www.opensource.org/licenses/CDDL-1.0.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file COPYING.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2020 Saso Kiselkov. All rights reserved.
 */

#ifndef	_PIDVISUALIZER_H_
#define	_PIDVISUALIZER_H_

#include <stdbool.h>

#include <acfutils/mt_cairo_render.h>
#include <acfutils/pid_ctl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pidvis_s pidvis_t;

pidvis_t *pidvis_new(const char *title, pid_ctl_t *pid,
    mt_cairo_uploader_t *mtul);
void pidvis_destroy(pidvis_t *vis);

void pidvis_open(pidvis_t *vis);
bool pidvis_is_open(const pidvis_t *vis);

#ifdef __cplusplus
}
#endif

#endif	/* _PIDVISUALIZER_H_ */
