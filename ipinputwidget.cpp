#include "ipinputwidget.h"

IPInputWidget::IPInputWidget(QWidget *parent)
    : QWidget(parent)
{
    initUI();
}

void IPInputWidget::initUI()
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setSpacing(2);
    layout->setContentsMargins(0,0,0,0);

    QRegularExpression reg("^(0|([1-9]\\d{0,2}|1\\d{2}|2[0-4]\\d|25[0-5]))$");
    QRegularExpressionValidator* val = new QRegularExpressionValidator(reg, this);

    // 透明样式：背景透明，保留边框方便看清输入区
    QString editStyle = R"(
        QLineEdit{
            background-color: transparent;
            border:1px solid #999999;
            color:#333333;
        }
        QLineEdit:focus{
            border:1px solid #2277dd;
        }
    )";

    for(int i=0;i<4;i++)
    {
        m_edit[i] = new QLineEdit;
        m_edit[i]->setFixedSize(45,26);
        m_edit[i]->setAlignment(Qt::AlignCenter);
        m_edit[i]->setValidator(val);

        // 设置透明
        m_edit[i]->setStyleSheet(editStyle);

        layout->addWidget(m_edit[i]);

        if(i<3)
        {
            QLabel* dot = new QLabel(".");
            dot->setFont(QFont("Microsoft YaHei",11,QFont::Bold));
            dot->setStyleSheet("background:transparent;color:#333333;");
            layout->addWidget(dot);
        }
        connect(m_edit[i],&QLineEdit::textEdited,this,&IPInputWidget::onTextEdited);
    }

    // 整体控件背景也透明
    this->setStyleSheet("background-color:transparent;");
}

// 修复：自动跳转逻辑（输满 3 位 或 合法数字自动跳）
void IPInputWidget::onTextEdited(const QString &text)
{
    QLineEdit* cur = qobject_cast<QLineEdit*>(sender());
    if(!cur) return;

    // 只有 输入 3 个字符 才自动跳转 ← 严格满足你的要求
    if (text.length() == 3)
    {
        for(int i=0;i<3;i++)
        {
            if(m_edit[i]==cur)
            {
                m_edit[i+1]->setFocus();
                m_edit[i+1]->selectAll();
                return;
            }
        }
    }
}

// 修复：退格回退逻辑
void IPInputWidget::keyPressEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Backspace)
    {
        for(int i=1;i<4;i++)
        {
            if(m_edit[i]->hasFocus() && m_edit[i]->text().isEmpty())
            {
                m_edit[i-1]->setFocus();
                m_edit[i-1]->setCursorPosition(m_edit[i-1]->text().length());
                return;
            }
        }
    }
    QWidget::keyPressEvent(event);
}

QString IPInputWidget::getIP() const
{
    return QString("%1.%2.%3.%4")
            .arg(m_edit[0]->text())
            .arg(m_edit[1]->text())
            .arg(m_edit[2]->text())
            .arg(m_edit[3]->text());
}

void IPInputWidget::setIP(const QString &ip)
{
    QStringList parts = ip.split(".");
    if(parts.size()!=4) return;
    for(int i=0;i<4;i++){
        m_edit[i]->setText(parts[i]);
    }
}

void IPInputWidget::clearIP()
{
    for(int i=0;i<4;i++) m_edit[i]->clear();
    m_edit[0]->setFocus();
}
