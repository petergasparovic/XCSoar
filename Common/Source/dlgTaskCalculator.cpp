/*
Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2005

  	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@bigfoot.com>

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

#include "stdafx.h"

#include "statistics.h"

#include "externs.h"
#include "units.h"
#include "McReady.h"
#include "device.h"

#include "WindowControls.h"
#include "dlgTools.h"
#include "Port.h"
#include "Calculations2.h"
#include "Dialogs.h"

extern HWND   hWndMainWindow;
static WndForm *wf=NULL;

static double emc= 0.0;

static void OnCancelClicked(WindowControl * Sender){
	(void)Sender;
  wf->SetModalResult(mrCancle);
}

static void OnOKClicked(WindowControl * Sender){
	(void)Sender;
  wf->SetModalResult(mrOK);
}

static double Range = 0;

static void RefreshCalculator(void) {
  WndProperty* wp;

  RefreshTask();
  RefreshTaskStatistics();

  // update outputs
  wp = (WndProperty*)wf->FindByName(TEXT("prpAATEst"));
  if (wp) {
    double dd = CALCULATED_INFO.TaskTimeToGo;
    if ((CALCULATED_INFO.TaskStartTime>0.0)&&(CALCULATED_INFO.Flying)) {
      dd += GPS_INFO.Time-CALCULATED_INFO.TaskStartTime;
    }
    dd= min(24.0*60.0,dd/60.0);
    wp->GetDataField()->SetAsFloat(dd);
    wp->RefreshDisplay();
  }

  // update outputs
  wp = (WndProperty*)wf->FindByName(TEXT("prpAATTime"));
  if (wp) {
    if (!AATEnabled) {
      wp->SetVisible(false);
    } else {
      wp->GetDataField()->SetAsFloat(AATTaskLength);
    }
      wp->RefreshDisplay();
  }

  double d1 = (CALCULATED_INFO.TaskDistanceToGo
	       +CALCULATED_INFO.TaskDistanceCovered);
  if (AATEnabled && (d1==0.0)) {
    d1 = CALCULATED_INFO.AATTargetDistance;
  }
  wp = (WndProperty*)wf->FindByName(TEXT("prpDistance"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(d1*DISTANCEMODIFY);
    wp->GetDataField()->SetUnits(Units::GetDistanceName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpMacCready"));
  if (wp) {
    wp->GetDataField()->SetUnits(Units::GetVerticalSpeedName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpEffectiveMacCready"));
  if (wp) {
    wp->GetDataField()->SetUnits(Units::GetVerticalSpeedName());
    wp->GetDataField()->SetAsFloat(emc*LIFTMODIFY);
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpRange"));
  if (wp) {
    wp->RefreshDisplay();
    if (!AATEnabled) {
      wp->SetVisible(false);
    }
    wp->GetDataField()->SetAsFloat(Range*100.0);
    wp->RefreshDisplay();
  }

  double v1;
  if (CALCULATED_INFO.TaskTimeToGo>0) {
    v1 = CALCULATED_INFO.TaskDistanceToGo/
      CALCULATED_INFO.TaskTimeToGo;
  } else {
    v1 = 0;
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpSpeedRemaining"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(v1*TASKSPEEDMODIFY);
    wp->GetDataField()->SetUnits(Units::GetTaskSpeedName());
    wp->RefreshDisplay();
  }

  wp = (WndProperty*)wf->FindByName(TEXT("prpSpeedAchieved"));
  if (wp) {
    wp->GetDataField()->SetAsFloat(CALCULATED_INFO.TaskSpeed*TASKSPEEDMODIFY);
    wp->GetDataField()->SetUnits(Units::GetTaskSpeedName());
    wp->RefreshDisplay();
  }

}

static void OnTargetClicked(WindowControl * Sender){
  (void)Sender;
  wf->SetVisible(false);
  dlgTarget();
  // find start value for range (it may have changed)
  Range = AdjustAATTargets(2.0);
  RefreshCalculator();
  wf->SetVisible(true);
}


static void OnMacCreadyData(DataField *Sender,
			    DataField::DataAccessKind_t Mode){
  switch(Mode){
    case DataField::daGet:
      Sender->Set(MACCREADY*LIFTMODIFY);
    break;
    case DataField::daPut:
    case DataField::daChange:
      MACCREADY = Sender->GetAsFloat()/LIFTMODIFY;
      RefreshCalculator();
    break;
  }
}


static void OnRangeData(DataField *Sender, DataField::DataAccessKind_t Mode){
  double rthis;
  switch(Mode){
    case DataField::daGet:
      //      Sender->Set(Range*100.0);
    break;
    case DataField::daPut:
    case DataField::daChange:
      rthis = Sender->GetAsFloat()/100.0;
      if (fabs(Range-rthis)>0.01) {
        Range = rthis;
        AdjustAATTargets(Range);
        RefreshCalculator();
      }
    break;
  }
}

extern bool TargetDialogOpen;

static void OnOptimiseClicked(WindowControl * Sender){
  bool first = true;
  double myrange= Range;
  if (!AATEnabled) return;

  TargetDialogOpen = true;

  do {
    myrange = Range;
    AdjustAATTargets(Range);
    RefreshCalculator();
    double dd = CALCULATED_INFO.TaskTimeToGo;
    if ((CALCULATED_INFO.TaskStartTime>0.0)&&(CALCULATED_INFO.Flying)) {
      dd += GPS_INFO.Time-CALCULATED_INFO.TaskStartTime;
    }
    dd= min(24.0*60.0,dd/60.0);
    if (dd<= AATTaskLength+5) {
      if (first) {
        Range += 0.05;
      } else {
        break;
      }
    } else {
      first = false;
      Range -= 0.05;
    }
  } while (fabs(Range)<1.0);
  Range = myrange;
  AdjustAATTargets(Range);
  RefreshCalculator();

  TargetDialogOpen = false;
}


static CallBackTableEntry_t CallBackTable[]={
  DeclearCallBackEntry(OnMacCreadyData),
  DeclearCallBackEntry(OnRangeData),
  DeclearCallBackEntry(OnOKClicked),
  DeclearCallBackEntry(OnCancelClicked),
  DeclearCallBackEntry(OnOptimiseClicked),
  DeclearCallBackEntry(OnTargetClicked),
  DeclearCallBackEntry(NULL)
};


void dlgTaskCalculatorShowModal(void){

  char filename[MAX_PATH];
  LocalPathS(filename, TEXT("dlgTaskCalculator.xml"));
  wf = dlgLoadFromXML(CallBackTable,
                      filename,
		      hWndMainWindow,
		      TEXT("IDR_XML_TASKCALCULATOR"));

  if (!wf) return;

  emc = EffectiveMacCready(&GPS_INFO, &CALCULATED_INFO);

  // find start value for range
  Range = AdjustAATTargets(2.0);

  RefreshCalculator();

  double MACCREADYenter = MACCREADY;

  if (!AATEnabled) {
    ((WndButton *)wf->FindByName(TEXT("Optimise")))->SetVisible(false);
  }
  if (!ValidTaskPoint(ActiveWayPoint)) {
    ((WndButton *)wf->FindByName(TEXT("Target")))->SetVisible(false);
  }

  if (wf->ShowModal() == mrCancle) {
    // todo: restore task settings.
    MACCREADY = MACCREADYenter;
  }
  delete wf;
  wf = NULL;

}
