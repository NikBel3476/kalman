#include "authenticationpage.h"

const static int MIN_FORM_WIDTH = 150;
const static int MAX_FORM_WIDTH = 250;

AuthenticationPage::AuthenticationPage(QWidget *parent)
		: QWidget{parent}, m_layout(new QVBoxLayout(this)),
			m_username_input(new QLineEdit()), m_password_input(new QLineEdit()),
			m_login_button(new QPushButton()) {
	m_layout->setAlignment(Qt::AlignCenter);

	m_layout->addWidget(m_username_input);
	m_layout->addWidget(m_password_input);
	m_layout->addWidget(m_login_button);

	m_username_input->setPlaceholderText(tr("Username"));
	m_username_input->setSizePolicy(
			QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
	m_username_input->setMinimumWidth(MIN_FORM_WIDTH);
	m_username_input->setMaximumWidth(MAX_FORM_WIDTH);

	m_password_input->setEchoMode(QLineEdit::Password);
	m_password_input->setPlaceholderText(tr("Password"));
	m_password_input->setSizePolicy(
			QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
	m_password_input->setMinimumWidth(MIN_FORM_WIDTH);
	m_password_input->setMaximumWidth(MAX_FORM_WIDTH);

	m_login_button->setText(tr("Login"));
	m_login_button->setSizePolicy(
			QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
	m_login_button->setMinimumWidth(MIN_FORM_WIDTH);
	m_login_button->setMaximumWidth(MAX_FORM_WIDTH);

	connect(m_login_button, &QPushButton::pressed, this,
					&AuthenticationPage::handleLoginButtonPress);
	connect(m_username_input, &QLineEdit::textChanged, this,
					&AuthenticationPage::handleUsernameChange);
	connect(m_password_input, &QLineEdit::textChanged, this,
					&AuthenticationPage::handlePasswordChange);
}

void AuthenticationPage::handleUsernameChange(const QString &changedUsername) {
	m_username = changedUsername;
}

void AuthenticationPage::handlePasswordChange(const QString &changedPassword) {
	m_password = changedPassword;
}

void AuthenticationPage::handleLoginButtonPress() {
	emit login(m_username, m_password);
}
