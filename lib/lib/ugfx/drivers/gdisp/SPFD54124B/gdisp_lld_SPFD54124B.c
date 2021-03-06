/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.io/license.html
 */

/*
 *	Driver for display from Nokia 1616 phone
 *	based on the work of axillent@gmail.com (http://radiokot.ru/articles/53/)
 *	adapted by shilow@ukr.net with the help of Federico Reds <fede.677387@hotmail.it>
 */

#include "gfx.h"

#if GFX_USE_GDISP

#if defined(GDISP_SCREEN_HEIGHT) || defined(GDISP_SCREEN_HEIGHT)
	#if GFX_COMPILER_WARNING_TYPE == GFX_COMPILER_WARNING_DIRECT
		#warning "GDISP: This low level driver does not support setting a screen size. It is being ignored."
	#elif GFX_COMPILER_WARNING_TYPE == GFX_COMPILER_WARNING_MACRO
		COMPILER_WARNING("GDISP: This low level driver does not support setting a screen size. It is being ignored.")
	#endif
	#undef GDISP_SCREEN_WIDTH
	#undef GDISP_SCREEN_HEIGHT
#endif

#include "gdisp_lld_config.h"
#include "../../../src/gdisp/gdisp_driver.h"

#include "board_SPFD54124B.h"

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

// Set parameters if they are not already set
#ifndef GDISP_SCREEN_HEIGHT
#define GDISP_SCREEN_HEIGHT		161
#endif
#ifndef GDISP_SCREEN_WIDTH
#define GDISP_SCREEN_WIDTH		129
#endif
/* Offset in RAM of visible area */
#ifndef GDISP_RAM_X_OFFSET
#define GDISP_RAM_X_OFFSET		2
#endif
/* Offset in RAM of visible area */
#ifndef GDISP_RAM_Y_OFFSET
#define GDISP_RAM_Y_OFFSET		1
#endif
#ifndef GDISP_SLEEP_POS
#define GDISP_SLEEP_POS			((GDISP_SCAN_LINES-GDISP_SLEEP_SIZE)/2 & ~3)
#endif
#ifndef GDISP_INITIAL_CONTRAST
#define GDISP_INITIAL_CONTRAST	50
#endif
#ifndef GDISP_INITIAL_BACKLIGHT
#define GDISP_INITIAL_BACKLIGHT	100
#endif

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables.                                                   */
/*===========================================================================*/

// Use the priv pointer itself to save our color. This save allocating ram for it
//	and works provided sizeof(gU16) <= sizeof(void *)
#define savecolor(g)				(*(gU16 *)&g->priv)

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

// Some macros just to make reading the code easier
#define delayms(ms)			gfxSleepMilliseconds(ms)
#define write_data2(g, d1, d2)		{ write_data(g, d1); write_data(g, d2); }
#define write_data3(g, d1, d2, d3)	{ write_data(g, d1); write_data(g, d2); write_data(g, d3); }
#define write_reg(g, cmd, d1)		{ write_index(g, cmd); write_data(g, d1); }
#define write_reg2(g, cmd, d1, d2)	{ write_index(g, cmd); write_data2(g, d1, d2); }
#define write_reg3(g, cmd, d1, d2, d3)	{ write_index(g, cmd); write_data3(g, d1, d2, d3); }

static GFXINLINE void set_viewport(GDisplay* g)
{
	write_reg2(g, SPFD54124B_CMD_CASET, GDISP_RAM_X_OFFSET+g->p.x, GDISP_RAM_X_OFFSET+g->p.x+g->p.cx-1);	// Column address set
	write_reg2(g, SPFD54124B_CMD_RASET, GDISP_RAM_Y_OFFSET+g->p.y, GDISP_RAM_Y_OFFSET+g->p.y+g->p.cy-1);	// Page address set
	write_index(g, SPFD54124B_CMD_RAMWR);
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

const gU16 lcd_init_list[] = {
	SPFD54124B_CMD_SLPOUT,
	SPFD54124B_CMD_COLMOD, SPFD54124B_SEND_DATA | SPFD54124B_CMD_COLMOD_MCU16bit,
	SPFD54124B_CMD_DISPON,
	SPFD54124B_CMD_INVOFF,
	SPFD54124B_CMD_NORON
};

LLDSPEC gBool gdisp_lld_init(GDisplay *g)
{
	// No private area for this controller
	g->priv = 0;

	// Initialise the board interface
	init_board(g);

	// Hardware reset
	setpin_reset(g, gTrue);
	delayms(100);
	setpin_reset(g, gFalse);
	delayms(100);

	acquire_bus(g);

	const gU16 *list = &lcd_init_list[0];
	gU8 size = sizeof(lcd_init_list) / sizeof(lcd_init_list[0]);

	while(size--) {
		write_index(g, *list++);
	}

	// Finish Init
	post_init_board(g);

	release_bus(g);

	// Turn on the back-light
	set_backlight(g, GDISP_INITIAL_BACKLIGHT);

	// Initialise the GDISP structure to match 
	g->g.Width = GDISP_SCREEN_WIDTH;
	g->g.Height = GDISP_SCREEN_HEIGHT;
	g->g.Orientation = gOrientation0;
	g->g.Powermode = gPowerOn;
	g->g.Backlight = GDISP_INITIAL_BACKLIGHT;
	g->g.Contrast = GDISP_INITIAL_CONTRAST;

	return gTrue;
}

#if GDISP_HARDWARE_STREAM_WRITE
LLDSPEC	void gdisp_lld_write_start(GDisplay *g)
{
	acquire_bus(g);
	set_viewport(g);
}

LLDSPEC	void gdisp_lld_write_color(GDisplay *g)
{
	write_data(g, gdispColor2Native(g->p.color));
}

LLDSPEC	void gdisp_lld_write_stop(GDisplay *g)
{
	release_bus(g);
}
#endif


#if GDISP_NEED_CONTROL && GDISP_HARDWARE_CONTROL
LLDSPEC void gdisp_lld_control(GDisplay *g)
{
	/* The hardware is capable of supporting...
	 * 	GDISP_CONTROL_POWER		- supported
	 * 	GDISP_CONTROL_ORIENTATION	- supported
	 * 	GDISP_CONTROL_BACKLIGHT		- unsupported
	 * 	GDISP_CONTROL_CONTRAST		- unsupported
	 */

	switch(g->p.x) {
		case GDISP_CONTROL_POWER:
			if (g->g.Powermode == (gPowermode)g->p.ptr)
				return;

			switch((gPowermode)g->p.ptr) {
				case gPowerOff:
					acquire_bus(g);
					write_index(g, SPFD54124B_CMD_SLPIN);
					release_bus(g);
					break;

				case gPowerOn:
					acquire_bus(g);
					write_index(g, SPFD54124B_CMD_SLPOUT);
					delayms(20);
					write_index(g, SPFD54124B_CMD_NORON);
					release_bus(g);
					break;

				case gPowerSleep:
					acquire_bus(g);
					write_index(g, SPFD54124B_CMD_SLPOUT);
					delayms(20);
					release_bus(g);
   					break;

				default:
					return;
			}

			g->g.Powermode = (gPowermode)g->p.ptr;
			return;

		case GDISP_CONTROL_ORIENTATION:
			if (g->g.Orientation == (gOrientation)g->p.ptr)
				return;

			switch((gOrientation)g->p.ptr) {
				case gOrientation0:
					acquire_bus(g);
					write_index(g, SPFD54124B_CMD_MADCTR);
					write_index(g, 0x0100);
					release_bus(g);
					g->g.Height = GDISP_SCREEN_HEIGHT;
					g->g.Width = GDISP_SCREEN_WIDTH;
					break;

				case gOrientation90:
					acquire_bus(g);
					write_index(g, SPFD54124B_CMD_MADCTR);
					write_index(g, 0x01A0);
					release_bus(g);
					g->g.Height = GDISP_SCREEN_WIDTH;
					g->g.Width = GDISP_SCREEN_HEIGHT;
					break;

				case gOrientation180:
					acquire_bus(g);
					write_index(g, SPFD54124B_CMD_MADCTR);
					write_index(g, 0x01C0);
					release_bus(g);
					g->g.Height = GDISP_SCREEN_HEIGHT;
					g->g.Width = GDISP_SCREEN_WIDTH;
					break;

				case gOrientation270:
					acquire_bus(g);
					write_index(g, SPFD54124B_CMD_MADCTR);
					write_index(g, 0x0160);
					release_bus(g);
					g->g.Height = GDISP_SCREEN_WIDTH;
					g->g.Width = GDISP_SCREEN_HEIGHT;
					break;

				default:
					// gOrientationPortrait and gOrientationLandscape are handled by the higher level code
					return;
			}

			g->g.Orientation = (gOrientation)g->p.ptr;

			return;
	}
}
#endif

#endif /* GFX_USE_GDISP */
