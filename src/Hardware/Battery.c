/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Battery.h"

#ifdef HAVE_BATTERY

/** Battery percentage; negative means unknown */
int PDABatteryPercent = -1;
/** Battery temperature (default = 0�C (?)) */
int PDABatteryTemperature = 0;

bool PDABatteryAC = false;

/** Warning time before battery is empty */
DWORD BatteryWarningTime = 0;

void
UpdateBatteryInfo(void)
{
  SYSTEM_POWER_STATUS_EX2 sps;

  // request the power status
  DWORD result = GetSystemPowerStatusEx2(&sps, sizeof(sps), TRUE);
  if (result >= sizeof(sps)) {
    if (sps.BatteryLifePercent != BATTERY_PERCENTAGE_UNKNOWN)
      PDABatteryPercent = sps.BatteryLifePercent;
    else
      PDABatteryPercent = -1;
    PDABatteryTemperature = sps.BatteryTemperature;
    PDABatteryAC = sps.ACLineStatus != AC_LINE_OFFLINE;
  } else {
    PDABatteryPercent = -1;
    PDABatteryTemperature = 0;
    PDABatteryAC = false;
  }
}

#endif
