/* LASI PS2 keyboard support code
 *
 * Copyright (C) 2019 Sven Schnelle <svens@stackframe.org>
 *
 * This file may be distributed under the terms of the GNU LGPLv2 license.
 */

#include "bregs.h"
#include "autoconf.h"
#include "types.h"
#include "output.h"
#include "hw/ps2port.h"
#include "util.h"
#include "string.h"
#include "lasips2.h"


int lasips2_kbd_in(char *c, int max)
{
    struct bregs regs;
    volatile int count = 0;

    // check if PS2 reported new keys, if so queue them up.
    while((gsc_readl(LASIPS2_KBD_STATUS) & LASIPS2_KBD_STATUS_RBNE)) {
        process_key(gsc_readb(LASIPS2_KBD_DATA));
    }

    while(count < max) {
        extern void VISIBLE16 handle_16(struct bregs *regs);
        // check if some key is queued up already
        regs.ah = 0x11;
        regs.flags = 0;
        handle_16(&regs);
        if (regs.flags & F_ZF)	// return if no key queued
            break;
        // read key from keyboard queue
        regs.ah = 0x10;
        handle_16(&regs);
        if (!regs.ah)
            break;
        *c++ = regs.ah;
        count++;
    }
    return count;
}


int ps2_kbd_command(int command, u8 *param)
{
    return 0;
}

int lasips2_command(u16 cmd)
{
    while(gsc_readl(LASIPS2_KBD_STATUS) & LASIPS2_KBD_STATUS_TBNE)
        udelay(10);
    writeb(LASIPS2_KBD_DATA, cmd & 0xff);

    while(!(gsc_readl(LASIPS2_KBD_STATUS) & LASIPS2_KBD_STATUS_RBNE))
        udelay(10);
    return gsc_readb(LASIPS2_KBD_DATA);
}

void ps2port_setup(void)
{
    writeb(LASIPS2_KBD_RESET, 0);
    udelay(1000);
    writeb(LASIPS2_KBD_CONTROL, LASIPS2_KBD_CONTROL_EN);
    lasips2_command(ATKBD_CMD_RESET_BAT);
    lasips2_command(ATKBD_CMD_RESET_DIS);
    lasips2_command(ATKBD_CMD_SSCANSET);
    lasips2_command(0x01);
    lasips2_command(ATKBD_CMD_ENABLE);
    kbd_init();
}
