#pragma once


// ���������� ���� DialogIT

class DialogIT : public CDialog
{
	DECLARE_DYNAMIC(DialogIT)

public:
	DialogIT(CWnd* pParent = NULL);   // ����������� �����������
	virtual ~DialogIT();

// ������ ����������� ����
	enum { IDD = IDD_DIALOG1 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // ��������� DDX/DDV

	DECLARE_MESSAGE_MAP()
public:
	CEdit status_d;
};
