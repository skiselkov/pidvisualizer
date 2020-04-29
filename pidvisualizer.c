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

#include <stddef.h>

#include <XPLMDisplay.h>

#include <acfutils/assert.h>
#include <acfutils/list.h>
#include <acfutils/mt_cairo_render.h>

#include "pidvisualizer.h"

#define	RENDER_FPS		30
#define	MAX_SAMPLES		8192

typedef struct {
	pid_ctl_t		data;
	list_node_t		node;
} sample_t;

struct pidvis_s {
	list_t			samples;
	pid_ctl_t		*pid;
	int			mtcr_w;
	int			mtcr_h;
	mt_cairo_render_t	*mtcr;
	mt_cairo_uploader_t	*mtul;
	XPLMWindowID		win;
};

static void
sample_pid(pidvis_t *vis)
{
	sample_t *sample = safe_calloc(1, sizeof (*sample));

	ASSERT(vis != NULL);
	ASSERT(vis->pid != NULL);
	memcpy(&sample->data, vis->pid, sizeof (sample->data));
	list_insert_tail(&vis->samples, sample);

	while (list_count(&vis->samples) > MAX_SAMPLES) {
		sample = list_remove_head(&vis->samples);
		free(sample);
	}
}

static void
render_cb(cairo_t *cr, unsigned w, unsigned h, void *userinfo)
{
	pidvis_t *vis;

	ASSERT(cr != NULL);
	UNUSED(w);
	UNUSED(h);
	ASSERT(userinfo != NULL);
	vis = userinfo;

	sample_pid(vis);

	cairo_set_source_rgb(cr, 1, 1, 1);
	cairo_paint(cr);
}

static void
draw_cb(XPLMWindowID win, void *refcon)
{
	pidvis_t *vis;
	int left, top, right, bottom, w, h;

	ASSERT(win != NULL);
	ASSERT(refcon != NULL);
	vis = refcon;
	XPLMGetWindowGeometry(win, &left, &top, &right, &bottom);
	w = right - left;
	h = top - bottom;

	if (vis->mtcr == NULL ||
	    (int)mt_cairo_render_get_width(vis->mtcr) != w ||
	    (int)mt_cairo_render_get_height(vis->mtcr) != h) {
		if (vis->mtcr != NULL)
			mt_cairo_render_fini(vis->mtcr);
		vis->mtcr = mt_cairo_render_init(w, h, RENDER_FPS, NULL,
		    render_cb, NULL, vis);
		if (vis->mtul != NULL)
			mt_cairo_render_set_uploader(vis->mtcr, vis->mtul);
	}
	mt_cairo_render_draw(vis->mtcr, VECT2(left, bottom), VECT2(w, h));
}

pidvis_t *
pidvis_new(const char *title, pid_ctl_t *pid, mt_cairo_uploader_t *mtul)
{
	pidvis_t *vis = safe_calloc(1, sizeof (*vis));
	XPLMCreateWindow_t cr = {
	    .structSize = sizeof (cr),
	    .left = 100,
	    .top = 400,
	    .right = 400,
	    .bottom = 100,
	    .drawWindowFunc = draw_cb,
	    .decorateAsFloatingWindow = xplm_WindowDecorationRoundRectangle,
	    .layer = xplm_WindowLayerFloatingWindows,
	    .refcon = vis
	};

	ASSERT(title != NULL);
	ASSERT(pid != NULL);

	vis->pid = pid;
	vis->mtul = mtul;
	vis->win = XPLMCreateWindowEx(&cr);
	XPLMSetWindowResizingLimits(vis->win, 200, 200, 1000000, 1000000);
	XPLMSetWindowTitle(vis->win, title);
	list_create(&vis->samples, sizeof (sample_t), offsetof(sample_t, node));
	ASSERT(vis->win != NULL);

	pidvis_open(vis);

	return (vis);
}

void
pidvis_destroy(pidvis_t *vis)
{
	sample_t *sample;

	if (vis == NULL)
		return;
	if (vis->win != NULL)
		XPLMDestroyWindow(vis->win);
	if (vis->mtcr != NULL)
		mt_cairo_render_fini(vis->mtcr);
	while ((sample = list_remove_head(&vis->samples)) != NULL)
		free(sample);
	list_destroy(&vis->samples);
	free(vis);
}

void
pidvis_open(pidvis_t *vis)
{
	ASSERT(vis != NULL);
	ASSERT(vis->win != NULL);
	XPLMSetWindowIsVisible(vis->win, true);
	XPLMBringWindowToFront(vis->win);
}

bool
pidvis_is_open(const pidvis_t *vis)
{
	ASSERT(vis != NULL);
	ASSERT(vis->win != NULL);
	return (XPLMGetWindowIsVisible(vis->win));
}
