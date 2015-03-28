#pragma once


// диалоговое окно DialogIT

class DialogIT : public CDialog
{
	DECLARE_DYNAMIC(DialogIT)

public:
	DialogIT(CWnd* pParent = NULL);   // стандартный конструктор
	virtual ~DialogIT();

// Данные диалогового окна
	enum { IDD = IDD_DIALOG1 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // поддержка DDX/DDV

	DECLARE_MESSAGE_MAP()
public:
	CEdit status_d;
};
