/* Copyright (C) 2013 Webyog Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307, USA

*/

#include "Include.h"
#include "Global.h"

//#define CONERROR L"An error occured when trying to connect to the server.\r\nMake sure your internet connection is active."
#define CONERROR _(L"Unable to find update information")

#define UPGRADEYES	  _(L"Updates are available for SQLyog.\r\nPlease download and install the latest version")
#define UPGRADEYESPRO _(L"Updates are available for SQLyog Professional.\r\nPlease download and install the latest version")
#define UPGRADEYESENT _(L"Updates are available for SQLyog Enterprise.\r\nPlease download and install the latest version")
#define UPGRADEYESULT _(L"Updates are available for SQLyog Ultimate.\r\nPlease download and install the latest version")

#define UPGRADENO	 _(L"You are using the latest version of SQLyog")
#define UPGRADENOPRO _(L"You are using the latest version of SQLyog Professional")
#define UPGRADENOENT _(L"You are using the latest version of SQLyog Enterprise")
#define UPGRADENOULT _(L"You are using the latest version of SQLyog Ultimate")

#define UPGRDCHKING	_(L"Check for updates...")

class UpgradeCheck;
//LPVOID param strct passed from thread
typedef struct upgrade
{
	//HANDLE			m_event;
	HWND			m_hwndmain;
	UpgradeCheck	*m_upchk;
	wyBool			m_isexplicitcheck;

}UPGRD;

class UpgradeCheck
{
public:	
	//Constructor and destructor
	UpgradeCheck();
	~UpgradeCheck();

    //On exit the upgrade check thread
	/**
	@return void
	*/
	VOID		ExitUpgradeCheckThread();

	///Handkes the upgrade check
	/**
	@return void
	*/
	void		HandleUpgradeCheck(wyBool isexplict = wyFalse);

	
	//Sets the struct variables
	wyBool		SetThreadParam(UPGRD *evt, wyBool isexplict);
	
	///Thread function handles the upgrade check
	/**
	@param : IN argent passed as struct
	*/
	static	unsigned __stdcall	HandleHttpRequest(LPVOID param);
	
	///Handles the upgrade check dialog
	/**
	@return void
	*/
	void	HandleDialog();

	///Dialog Procedure
	 /**
    @param hwnd                 : IN dialog Handler.
    @param message              : IN Messages.
    @param wparam               : IN Unsigned message parameter.
    @param lparam               : IN LPARAM parameter.
    */
	static INT_PTR	CALLBACK	DlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	
	///Initialise the dialogbox
	/**
	@param hwnd : IN handle to dialog
	@return void
	*/
	void	InitDialog(HWND hwnd);

	//Upgrade check on when "Help->Check for updates"
    /**
	@return void
	*/
	void	HandleExplicitUpgradeCheck();
	
	///Handle sets the color and fonts of dialog texts
	/**
	@param hwnd					: IN dialog handle
	@param wparam               : IN Unsigned message parameter.
    @param lparam               : IN LPARAM parameter.
	@return value
	*/
	wyInt32		OnDlgProcColorStatic(HWND hwnd, WPARAM wParam, LPARAM lParam);	

	///Change the dialog caption during explicit upgrade check
	/**
	@return vouid
	*/
	void		ChangeTextOnExplicitCheck(HWND hwnd);

	/// Handle WM_COMMAND
	/**
	@param hwnd					: IN dialog handle
	@param wparam               : IN Unsigned message parameter.
	@return value
	*/
	wyInt32		OnWMCommand(HWND hwnd, WPARAM wParam);

	///Link static text Procedure
	 /**
    @param hwnd                 : IN dialog Handler.
    @param message              : IN Messages.
    @param wparam               : IN Unsigned message parameter.
    @param lparam               : IN LPARAM parameter.
    */
	static LRESULT	CALLBACK	LinkDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	///Sets the dtae format(DDYYYY)
	/**
	@param pday		: IN Date
	@param pmonth   : IN month
	@param pyear    : IN Year
	@return in new format
	*/
	wyUInt32			HandleSetDayFormat(wyInt32 pday, wyInt32 pmonth, wyInt32 pyear);
	
	// Thread handle
	HANDLE		m_thrid;

	///Upgrade dialog handle
	HWND		m_hwnddlg;

	//frame window handle
	HWND		m_hwndmain;

	//Flag sets wyTrue if check for upgrade explicitly
	wyBool		m_isupgradeexplicit;

	//sets wyTrue if upgrade available, else sets wyFalse
	wyBool		m_isupgrade;

	//Flag tells whether upgrade check dialog tobe popped or not
	wyBool		m_ispopupdlg;

	//Errorn on upgrade check
	wyBool		m_iserror;

	//During explicit check flag sets wyTrue once the checking is done, sets wyFalse during 'checking'
	wyBool		m_ischecked;

	///window proc for IDC_LINK(static text used for link)
	WNDPROC		m_linkdlgproc;

	//Static text font
	HFONT		m_hfont;

	//Keeps current date
	wyString	m_currentdate;

	wyString	m_url;
	wyString	m_appmajorversion;
	wyString	m_appminorversion;
	wyString	m_extrainfo;
	wyString	m_productid;
    wyChar      m_availableversion[10];
    wyChar      m_versiontobeignored[10];
};

