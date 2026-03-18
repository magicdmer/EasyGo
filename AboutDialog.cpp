#include "AboutDialog.h"
#include "ui_AboutDialog.h"
#include "Settings.h"

#if defined(_MSC_VER) && (_MSC_VER >= 1600)
# pragma execution_character_set("utf-8")
#endif

AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags()&~Qt::WindowContextHelpButtonHint);

    setWindowTitle(tr("关于 - %1").arg(EASYGO_VERSION));
}

AboutDialog::~AboutDialog()
{
    delete ui;
}
