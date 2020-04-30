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
#define	PX_PER_STEP		2

typedef struct {
	pid_ctl_t		data;
	list_node_t		node;
} sample_t;

typedef enum {
	DATA_SEL_ERR_P =	1 << 0,
	DATA_SEL_ERR_I =	1 << 1,
	DATA_SEL_ERR_D =	1 << 2,
	DATA_SEL_K_P =		1 << 3,
	DATA_SEL_K_I =		1 << 4,
	DATA_SEL_LIM_I =	1 << 5,
	DATA_SEL_K_D =		1 << 6,
	DATA_SEL_R_D =		1 << 7
#define	DATA_SEL_MAX_BIT	8
} data_sel_t;

struct pidvis_s {
	data_sel_t		data_sel;
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

static inline double
data_get(const sample_t *sample, int bit_nr)
{
	const pid_ctl_t *pid;

	ASSERT(sample != NULL);
	pid = &sample->data;

	switch (1 << bit_nr) {
	case DATA_SEL_ERR_P:
		return (pid->k_p * pid->e_prev);
	case DATA_SEL_ERR_I:
		return (pid->k_i * pid->e_integ);
	case DATA_SEL_ERR_D:
		return (pid->k_d * pid->e_deriv);
	case DATA_SEL_K_P:
		return (pid->k_p);
	case DATA_SEL_K_I:
		return (pid->k_i);
	case DATA_SEL_LIM_I:
		return (pid->lim_i);
	case DATA_SEL_K_D:
		return (pid->k_d);
	case DATA_SEL_R_D:
		return (pid->r_d);
	default:
		VERIFY(0);
	}
}

#define	MAKE_Y(val) \
	(MARGIN + (h - 2 * MARGIN) * \
	    (1 - iter_fract((val), minval, maxval, true)))

static void
render_cb(cairo_t *cr, unsigned w, unsigned h, void *userinfo)
{
	enum { MARGIN = 25, LINE_HEIGHT = 18 };
	const unsigned num_samples =
	    ceil((w - 2 * MARGIN) / (double)PX_PER_STEP);
	pidvis_t *vis;
	double minval = 0, maxval = 0;
	const double colors[DATA_SEL_MAX_BIT][3] = {
	    {1, 0, 0},
	    {0, 0.8, 0},
	    {0, 0, 1},
	    {1, 1, 0},
	    {1, 0, 1},
	    {0, 1, 1},
	    {0.8, 0.5, 0},
	    {0.8, 0, 0.5}
	};
	const char *data_sel_names[DATA_SEL_MAX_BIT] = {
	    "P",
	    "I",
	    "D",
	    "Kp",
	    "Ki",
	    "Li",
	    "Kd",
	    "Rd"
	};

	ASSERT(cr != NULL);
	UNUSED(h);
	ASSERT(userinfo != NULL);
	vis = userinfo;
	/*
	 * Draws the white background.
	 */
	cairo_set_source_rgb(cr, 1, 1, 1);
	cairo_paint(cr);
	/*
	 * Grab a fresh sample every draw cycle.
	 */
	sample_pid(vis);
	/*
	 * Determine the min/max values to figure out the scale we
	 * need to apply.
	 */
	for (int data_sel_bit = 0; data_sel_bit < DATA_SEL_MAX_BIT;
	    data_sel_bit++) {
		const sample_t *sample;
		unsigned j;

		if (!(vis->data_sel & (1 << data_sel_bit)))
			continue;
		for (j = 0, sample = list_tail(&vis->samples);
		    j < num_samples && sample != NULL;
		    j++, sample = list_prev(&vis->samples, sample)) {
			double data_val = data_get(sample, data_sel_bit);
			if (isnan(data_val))
				break;
			minval = MIN(minval, data_val);
			maxval = MAX(maxval, data_val);
		}
	}
	if (minval == maxval)
		return;
	/*
	 * Render some axes - primarily the zero line.
	 */
	cairo_set_line_width(cr, 1);
	cairo_set_source_rgb(cr, 0, 0, 0);

	cairo_move_to(cr, MARGIN, MARGIN);
	cairo_line_to(cr, MARGIN, h - MARGIN);
	cairo_move_to(cr, MARGIN, MAKE_Y(0));
	cairo_line_to(cr, w - MARGIN, MAKE_Y(0));
	cairo_stroke(cr);
	cairo_set_font_size(cr, 15);
	/*
	 * Now render the actual data samples.
	 */
	for (int data_sel_bit = 0; data_sel_bit < DATA_SEL_MAX_BIT;
	    data_sel_bit++) {
		sample_t *sample;
		unsigned j;
		double data_val;

		if (!(vis->data_sel & (1 << data_sel_bit)))
			continue;

		sample = list_tail(&vis->samples);
		if (sample == NULL)
			continue;

		cairo_new_path(cr);
		cairo_set_source_rgb(cr, colors[data_sel_bit][0],
		    colors[data_sel_bit][1], colors[data_sel_bit][2]);

		data_val = data_get(sample, data_sel_bit);

		cairo_move_to(cr, w - MARGIN, MAKE_Y(data_val));
		for (j = 0; j < num_samples && sample != NULL;
		    j++, sample = list_prev(&vis->samples, sample)) {
			data_val = data_get(sample, data_sel_bit);
			if (isnan(data_val))
				break;
			cairo_line_to(cr, w - MARGIN - j * PX_PER_STEP,
			    MAKE_Y(data_val));
		}
		cairo_stroke(cr);
	}
	/*
	 * Draws the data labels' white background to make sure they
	 * remain readable even if the data lines are below the labels.
	 */
	cairo_set_source_rgb(cr, 1, 1, 1);
	cairo_rectangle(cr, MARGIN, MARGIN, 125,
	    DATA_SEL_MAX_BIT * LINE_HEIGHT);
	cairo_fill(cr);
	/*
	 * And finally draw the data labels.
	 */
	for (int data_sel_bit = 0; data_sel_bit < DATA_SEL_MAX_BIT;
	    data_sel_bit++) {
		sample_t *sample;
		double data_val;
		char buf[64];

		sample = list_tail(&vis->samples);
		if (sample == NULL)
			continue;

		if (vis->data_sel & (1 << data_sel_bit)) {
			cairo_set_source_rgb(cr, colors[data_sel_bit][0],
			    colors[data_sel_bit][1], colors[data_sel_bit][2]);
		} else {
			cairo_set_source_rgb(cr, 0.67, 0.67, 0.67);
		}
		data_val = data_get(sample, data_sel_bit);

		snprintf(buf, sizeof (buf), "%s: %f",
		    data_sel_names[data_sel_bit], data_val);
		cairo_move_to(cr, 1.5 * MARGIN,
		    1.5 * MARGIN + data_sel_bit * LINE_HEIGHT);
		cairo_show_text(cr, buf);
	}
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

	vis->data_sel = DATA_SEL_ERR_P | DATA_SEL_ERR_I  | DATA_SEL_ERR_D;
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
