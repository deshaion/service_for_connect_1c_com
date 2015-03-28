// DialogIT.cpp: файл реализации
//

#include "stdafx.h"
#include "DialogIT.h"


// диалоговое окно DialogIT

IMPLEMENT_DYNAMIC(DialogIT, CDialog)

DialogIT::DialogIT(CWnd* pParent /*=NULL*/)
	: CDialog(DialogIT::IDD, pParent)
{

}

DialogIT::~DialogIT()
{
}

void DialogIT::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT2, status_d);
}


BEGIN_MESSAGE_MAP(DialogIT, CDialog)
END_MESSAGE_MAP()


// обработчики сообщений DialogIT
