#ifndef RESOURCE_H
#define RESOURCE_H

// Icons
#define IDI_APP_ICON               1

// Embedded language.json for i18n (RCDATA)
#define IDR_LANGUAGE_JSON          2

// Dialogs
#define IDD_CTRL_DIALOG          100
#define IDD_CONFIG_DIALOG       101
#define IDD_REMINDER_DIALOG     102

// Control dialog controls
#define IDC_RADIO_CONFIG        1001
#define IDC_RADIO_SUSPEND       1002
#define IDC_RADIO_EXIT          1003
#define IDC_EDIT_Y              1004
#define IDC_EDIT_PASSWORD       1005
#define IDC_CTRL_PWD_LABEL      1006
#define IDC_STATIC_PWD_ERROR    1007

// Config dialog controls
#define IDC_EDIT_START_TIME     2001
#define IDC_EDIT_END_TIME       2002
#define IDC_EDIT_MAX_CONT       2003
#define IDC_EDIT_WARN_MIN       2004
#define IDC_EDIT_REST_MIN       2005
#define IDC_EDIT_PARENT_PIN     2006
#define IDC_COMBO_LANGUAGE      2007
#define IDC_CHECK_AUTOSTART     2008
#define IDC_STATIC_USABLE_TIME  2009
#define IDC_STATIC_TILDE        2010
#define IDC_STATIC_MAX_CONT     2011
#define IDC_STATIC_WARN         2012
#define IDC_STATIC_REST         2013
#define IDC_STATIC_PARENT_PIN   2014
#define IDC_STATIC_AUTOSTART    2015
#define IDC_STATIC_LANGUAGE     2016
#define IDC_BTN_AVAILABLE_TIME_HELP 2017

// Reminder dialog (countdown warning)
#define IDC_REMINDER_MESSAGE   3001
#define IDC_REMINDER_COUNTDOWN 3002

// Hotkey failure dialog (App layer)
#define IDD_HOTKEY_FAILURE_DIALOG 103
#define IDC_HOTKEY_FAILURE_MSG   3003

// Unavailable-time lock (full-screen) controls on primary monitor
#define IDC_UNAVAIL_STATIC_LABEL 4000
#define IDC_UNAVAIL_EDIT_PIN    4001
#define IDC_UNAVAIL_BTN_OK      4002
#define IDC_UNAVAIL_STATIC_ERR  4003

#ifndef IDC_STATIC
#define IDC_STATIC             -1
#endif

#endif // RESOURCE_H
