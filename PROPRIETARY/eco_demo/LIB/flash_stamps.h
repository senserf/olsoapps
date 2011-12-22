#ifndef __flash_stamps_h
#define __flash_stamps_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2011                           */
/* All rights reserved.                                                 */
/* ==================================================================== */

/*
   At the beginning of 2nd page of iflash we store special application
   (i.e. applicable to all involved praxes) marks.
   To start with, we need virginity test to force eeprom erase in versions
   where leftovers are dangerous - EE_ERASE_ON_NEW_FLASH in options.sys
   decides if it is so.
   If this functionality grows, it may be optimized in many ways.
*/

#if EE_ERASE_ON_NEW_FLASH

#define NEW_FLASH_MARK	64
#define is_flash_new	(if_read (NEW_FLASH_MARK) == 0xFFFF)
#define break_flash	(if_write (NEW_FLASH_MARK, 0xFFFE))

#else

#define is_flash_new	0
#define break_flash	CNOP

#endif

#endif
