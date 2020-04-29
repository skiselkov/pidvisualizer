/*
 * CONFIDENTIAL
 *
 * Copyright 2020 Saso Kiselkov. All rights reserved.
 *
 * NOTICE:  All information contained herein is, and remains the property
 * of Saso Kiselkov. The intellectual and technical concepts contained
 * herein are proprietary to Saso Kiselkov and may be covered by U.S. and
 * Foreign Patents, patents in process, and are protected by trade secret
 * or copyright law. Dissemination of this information or reproduction of
 * this material is strictly forbidden unless prior written permission is
 * obtained from Saso Kiselkov.
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
