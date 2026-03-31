#ifndef IPINPUTWIDGET_H
#define IPINPUTWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QLabel>
#include <QStringList>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QKeyEvent>

// 必须加：Qt Designer可识别
class IPInputWidget : public QWidget
{
    Q_OBJECT
    // 属性：Designer里可右键/属性栏改IP
    Q_PROPERTY(QString ip READ getIP WRITE setIP)

public:
    explicit IPInputWidget(QWidget *parent = nullptr);

    QString getIP() const;
    void setIP(const QString &ip);
    void clearIP();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onTextEdited(const QString &text);

private:
    void initUI();
    QLineEdit* m_edit[4];
};

#endif // IPINPUTWIDGET_H
