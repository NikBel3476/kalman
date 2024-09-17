#ifndef AUTHENTICATIONFORM_H
#define AUTHENTICATIONFORM_H

#include <QLineEdit>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

class AuthenticationForm : public QWidget {
	Q_OBJECT
public:
	explicit AuthenticationForm(QWidget *parent = nullptr);

signals:
	void login(const QString &username, const QString &password);

private slots:
	void handleUsernameChange(const QString &changedUsername);
	void handlePasswordChange(const QString &changedPassword);
	void handleLoginButtonPress();

private:
	QVBoxLayout *m_layout = nullptr;
	QLineEdit *m_username_input = nullptr;
	QLineEdit *m_password_input = nullptr;
	QPushButton *m_login_button = nullptr;

	QString m_username;
	QString m_password;
};

#endif // AUTHENTICATIONFORM_H
