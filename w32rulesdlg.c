const char w32rulesdlg_rcs[] = "$Id: w32rulesdlg.c,v 1.1.1.1 2001/05/15 13:59:08 oes Exp $";
/*********************************************************************
 *
 * File        :  $Source: /cvsroot/ijbswa/current/w32rulesdlg.c,v $
 *
 * Purpose     :  A dialog to allow GUI editing of the rules.
 *                Unfinished.
 *
 * Copyright   :  Written by and Copyright (C) 2001 the SourceForge
 *                IJBSWA team.  http://ijbswa.sourceforge.net
 *
 *                Written by and Copyright (C) 1999 Adam Lock
 *                <locka@iol.ie>
 *
 *                This program is free software; you can redistribute it 
 *                and/or modify it under the terms of the GNU General
 *                Public License as published by the Free Software
 *                Foundation; either version 2 of the License, or (at
 *                your option) any later version.
 *
 *                This program is distributed in the hope that it will
 *                be useful, but WITHOUT ANY WARRANTY; without even the
 *                implied warranty of MERCHANTABILITY or FITNESS FOR A
 *                PARTICULAR PURPOSE.  See the GNU General Public
 *                License for more details.
 *
 *                The GNU General Public License should be included with
 *                this file.  If not, you can view it at
 *                http://www.gnu.org/copyleft/gpl.html
 *                or write to the Free Software Foundation, Inc., 59
 *                Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * Revisions   :
 *    $Log: w32rulesdlg.c,v $
 *    Revision 1.1.1.1  2001/05/15 13:59:08  oes
 *    Initial import of version 2.9.3 source tree
 *
 *
 *********************************************************************/

#include "config.h"

#include <stdio.h>

#include <windows.h>
#include <commctrl.h>

#include "w32res.h"
#include "w32rulesdlg.h"
#include "win32.h"

#ifdef __MINGW32__
#include "cygwin.h"
#endif

const char w32rulesdlg_h_rcs[] = W32RULESDLG_H_VERSION;

#ifndef _WIN_CONSOLE /* entire file */

const int nSmallIconWidth = 16;
const int nSmallIconHeight = 16;

static BOOL CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

static HIMAGELIST g_hImageList = NULL;
static char *g_pszDefaultRule;
static BOOL g_bDirty = FALSE;



/*********************************************************************
 *
 * Function    :  ShowRulesDialog
 *
 * Description :  (Please fill me in!)
 *
 * Parameters  :
 *          1  :  hwndParent = (what?)
 *
 * Returns     :  (Please fill me in!)
 *
 *********************************************************************/
int ShowRulesDialog(HWND hwndParent)
{
   DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_RULES), hwndParent, DialogProc);
   return TRUE;

}


/*********************************************************************
 *
 * Function    :  SetDefaultRule
 *
 * Description :  (Please fill me in!)
 *
 * Parameters  :
 *          1  :  pszRule = (what?)
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void SetDefaultRule(const char *pszRule)
{
   if (pszRule == NULL)
   {
      if (g_pszDefaultRule)
      {
         free(g_pszDefaultRule);
         g_pszDefaultRule = NULL;
      }
   }
   else
   {
      g_pszDefaultRule = strdup(pszRule);
   }

}


#define IMAGE_ALLOW 0
#define IMAGE_DENY  1

/*********************************************************************
 *
 * Function    :  InsertRule
 *
 * Description :  (Please fill me in!)
 *
 * Parameters  :
 *          1  :  hwndListView = (what?)
 *          2  :  pszRule = (what?)
 *          3  :  bAllow = (what?)
 *
 * Returns     :  N/A
 *
 *********************************************************************/
static void InsertRule(HWND hwndListView, const char *pszRule, BOOL bAllow)
{
   LVITEM item;
   item.mask = LVIF_TEXT | LVIF_IMAGE;
   item.pszText = (char *)pszRule;
   item.iItem = ListView_GetItemCount(hwndListView) + 1;
   item.iSubItem = 0;
   item.iImage = bAllow ? IMAGE_ALLOW : IMAGE_DENY;
   ListView_InsertItem(hwndListView, &item);
   /* TODO add subitem for whether the rule is always or never */

}


/*********************************************************************
 *
 * Function    :  SetDirty
 *
 * Description :  (Please fill me in!)
 *
 * Parameters  :
 *          1  :  bDirty = (what?)
 *
 * Returns     :  N/A
 *
 *********************************************************************/
static void SetDirty(BOOL bDirty)
{
   g_bDirty = bDirty;
   /* TODO Change some values */

}


/*********************************************************************
 *
 * Function    :  OnInitDialog
 *
 * Description :  (Please fill me in!)
 *
 * Parameters  :
 *          1  :  hwndDlg = (what?)
 *
 * Returns     :  N/A
 *
 *********************************************************************/
static void OnInitDialog(HWND hwndDlg)
{
   LVCOLUMN aCols[2];
   HWND hwndListView;
   RECT rcListView;
   int cx;

   if (g_hImageList == NULL)
   {
      /* Create image list and add icons */
      HICON hIconDeny = LoadImage(g_hInstance, MAKEINTRESOURCE(IDI_DENYRULE), IMAGE_ICON, nSmallIconWidth, nSmallIconHeight, 0);
      HICON hIconAllow = LoadImage(g_hInstance, MAKEINTRESOURCE(IDI_ALLOWRULE), IMAGE_ICON, nSmallIconWidth, nSmallIconHeight, 0);
      g_hImageList = ImageList_Create(nSmallIconWidth, nSmallIconHeight, ILC_COLOR | ILC_MASK, 0, 10);
      ImageList_AddIcon(g_hImageList, hIconAllow);
      ImageList_AddIcon(g_hImageList, hIconDeny);
   }

   /* Set the default rule value if there is one */
   if (g_pszDefaultRule)
   {
      SetDlgItemText(hwndDlg, IDC_NEW, g_pszDefaultRule);
      SetDefaultRule(NULL);
   }

   /* Initialise the list view */
   hwndListView = GetDlgItem(hwndDlg, IDC_RULES);
   ListView_SetImageList(hwndListView, g_hImageList, LVSIL_SMALL);
   GetClientRect(hwndListView, &rcListView);
   cx = rcListView.right - rcListView.left;
   aCols[0].mask = LVCF_TEXT | LVCF_WIDTH;
   aCols[0].pszText = "Rule";
   aCols[0].cx = (70 * cx) / 100;
   aCols[1].mask = LVCF_TEXT | LVCF_WIDTH;
   aCols[1].pszText = "Applies when";
   aCols[1].cx = cx - aCols[0].cx;
   ListView_InsertColumn(hwndListView, 0, &aCols[0]);
   ListView_InsertColumn(hwndListView, 1, &aCols[1]);

   /* Read and add rules to the list */
   /* TODO */
   InsertRule(hwndListView, "Test rule 1", TRUE);
   InsertRule(hwndListView, "Test rule 2", TRUE);
   InsertRule(hwndListView, "Test rule 3", FALSE);
   InsertRule(hwndListView, "Test rule 4", FALSE);

}


/*********************************************************************
 *
 * Function    :  GetFirstSelectedItem
 *
 * Description :  (Please fill me in!)
 *
 * Parameters  :
 *          1  :  hwndDlg = (what?)
 *
 * Returns     :  (Please fill me in!)
 *
 *********************************************************************/
static int GetFirstSelectedItem(HWND hwndDlg)
{
   /* Check for selected items */
   HWND hwndListView = GetDlgItem(hwndDlg, IDC_RULES);
   int nItem = -1;
   do
   {
      nItem = ListView_GetNextItem(hwndListView, nItem, LVNI_SELECTED);
      if (nItem >= 0)
      {
         return nItem;
      }
   } while (nItem >= 0);
   return -1;

}


/*********************************************************************
 *
 * Function    :  OnRulesItemChanged
 *
 * Description :  (Please fill me in!)
 *
 * Parameters  :
 *          1  :  hwndDlg = (what?)
 *
 * Returns     :  N/A
 *
 *********************************************************************/
static void OnRulesItemChanged(HWND hwndDlg)
{
   int nItem = GetFirstSelectedItem(hwndDlg);
   HWND hwndListView = GetDlgItem(hwndDlg, IDC_RULES);
   int nItems = ListView_GetItemCount(hwndListView);
   BOOL bHaveSelection = (nItem >= 0) ? TRUE : FALSE;
   BOOL bMoveUp = (bHaveSelection && nItem > 0) ? TRUE : FALSE;
   BOOL bMoveDown = (bHaveSelection && nItem < nItems - 1) ? TRUE : FALSE;

   /* Enable/disable buttons */
   EnableWindow(GetDlgItem(hwndDlg, IDC_DELETE), bHaveSelection);
   EnableWindow(GetDlgItem(hwndDlg, IDC_MOVEUP), bMoveUp);
   EnableWindow(GetDlgItem(hwndDlg, IDC_MOVEDOWN), bMoveDown);

}


/*********************************************************************
 *
 * Function    :  MoveRules
 *
 * Description :  (Please fill me in!)
 *
 * Parameters  :
 *          1  :  hwndDlg = (what?)
 *          2  :  bMoveUp = (what?)
 *
 * Returns     :  N/A
 *
 *********************************************************************/
static void MoveRules(HWND hwndDlg, BOOL bMoveUp)
{
}


/*********************************************************************
 *
 * Function    :  OnMoveRuleUpClick
 *
 * Description :  (Please fill me in!)
 *
 * Parameters  :
 *          1  :  hwndDlg = (what?)
 *
 * Returns     :  N/A
 *
 *********************************************************************/
static void OnMoveRuleUpClick(HWND hwndDlg)
{
}


/*********************************************************************
 *
 * Function    :  OnMoveRuleDownClick
 *
 * Description :  (Please fill me in!)
 *
 * Parameters  :
 *          1  :  hwndDlg = (what?)
 *
 * Returns     :  N/A
 *
 *********************************************************************/
static void OnMoveRuleDownClick(HWND hwndDlg)
{
}


/*********************************************************************
 *
 * Function    :  OnCreateRuleClick
 *
 * Description :  (Please fill me in!)
 *
 * Parameters  :
 *          1  :  hwndDlg = (what?)
 *
 * Returns     :  N/A
 *
 *********************************************************************/
static void OnCreateRuleClick(HWND hwndDlg)
{
}


/*********************************************************************
 *
 * Function    :  OnDeleteRuleClick
 *
 * Description :  (Please fill me in!)
 *
 * Parameters  :
 *          1  :  hwndDlg = (what?)
 *
 * Returns     :  N/A
 *
 *********************************************************************/
static void OnDeleteRuleClick(HWND hwndDlg)
{
   /* Get selection and remove it */
   int nItem = GetFirstSelectedItem(hwndDlg);
   if (nItem >= 0)
   {
      LVITEM item;
      HWND hwndListView = GetDlgItem(hwndDlg, IDC_RULES);
      item.mask = LVIF_PARAM;
      item.iItem = nItem;
      item.iSubItem = 0;
      ListView_GetItem(hwndListView, &item);
      /* TODO erase data stored with item */
      ListView_DeleteItem(hwndListView, nItem);
   }

}


/*********************************************************************
 *
 * Function    :  OnCommand
 *
 * Description :  (Please fill me in!)
 *
 * Parameters  :
 *          1  :  hwndDlg = (what?)
 *          2  :  nCommand = (what?)
 *          3  :  nNotifyCode = (what?)
 *          4  :  hwndItem = (what?)
 *
 * Returns     :  N/A
 *
 *********************************************************************/
static void OnCommand(HWND hwndDlg, int nCommand, int nNotifyCode, HWND hwndItem)
{
   switch (nCommand)
   {
      case IDCANCEL:
      case IDC_SAVE:
         EndDialog(hwndDlg, IDOK);
         break;
      case IDC_CREATE:
         if (nNotifyCode == BN_CLICKED)
         {
            OnCreateRuleClick(hwndDlg);
         }
         break;
      case IDC_DELETE:
         if (nNotifyCode == BN_CLICKED)
         {
            OnDeleteRuleClick(hwndDlg);
         }
         break;
      case IDC_MOVEUP:
         if (nNotifyCode == BN_CLICKED)
         {
            OnMoveRuleUpClick(hwndDlg);
         }
         break;
      case IDC_MOVEDOWN:
         if (nNotifyCode == BN_CLICKED)
         {
            OnMoveRuleDownClick(hwndDlg);
         }
         break;
   }

}


/*********************************************************************
 *
 * Function    :  OnNotify
 *
 * Description :  (Please fill me in!)
 *
 * Parameters  :
 *          1  :  hwndDlg = (what?)
 *          2  :  nIdCtrl = (what?)
 *          3  :  pnmh = (what?)
 *
 * Returns     :  N/A
 *
 *********************************************************************/
static void OnNotify(HWND hwndDlg, int nIdCtrl, LPNMHDR pnmh)
{
   switch (nIdCtrl)
   {
      case IDC_RULES:
         switch (pnmh->code)
         {
            case LVN_ITEMCHANGED:
               OnRulesItemChanged(hwndDlg);
               break;
         }
         break;
   }

}


/*********************************************************************
 *
 * Function    :  OnDestroy
 *
 * Description :  (Please fill me in!)
 *
 * Parameters  :
 *          1  :  hwndDlg = (what?)
 *
 * Returns     :  N/A
 *
 *********************************************************************/
static void OnDestroy(HWND hwndDlg)
{
   /* TODO any destruction cleanup */

}


/*********************************************************************
 *
 * Function    :  DialogProc
 *
 * Description :  (Please fill me in!)
 *
 * Parameters  :
 *          1  :  hwndDlg = (what?)
 *          2  :  uMsg = (what?)
 *          3  :  wParam = (what?)
 *          4  :  lParam = (what?)
 *
 * Returns     :  (Please fill me in!)
 *
 *********************************************************************/
static BOOL CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   switch (uMsg)
   {
      case WM_INITDIALOG:
         OnInitDialog(hwndDlg);
         return TRUE;

      case WM_DESTROY:
         OnDestroy(hwndDlg);
         return TRUE;

      case WM_COMMAND:
         OnCommand(hwndDlg, LOWORD(wParam), HIWORD(wParam), (HWND) lParam);
         break;

      case WM_NOTIFY:
         OnNotify(hwndDlg, (int) wParam, (LPNMHDR) lParam);
         break;
   }
   return FALSE;

}

#endif /* ndef _WIN_CONSOLE - entire file */

/*
  Local Variables:
  tab-width: 3
  end:
*/
