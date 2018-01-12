#ifndef GLOBAL_H
#define GLOBAL_H

#include <QByteArray>
#include <QString>
#include <QHostAddress>

typedef enum{
	icq_user_notReg = 0x01,
	icq_user_admin = 0x02,
	icq_user_AOL = 0x04,
	icq_user_premium = 0x08,
	icq_user_AIM = 0x10,
	icq_user_sleep = 0x20,
	icq_user_ICQ = 0x40,
	icq_user_mobile = 0x80,
}icq_user_types;

struct User{
	QByteArray login;
	QByteArray nikName;
	QByteArray pass;
	QByteArray passKey;
	QByteArray cookie;
	unsigned int type;
	QHostAddress ip;
};

struct ServerSettings{
	unsigned short maxContactListSize = 100;
	unsigned short maxOnlineContactListSize = 50;
	unsigned short maxFantomListSize = 10;
};

namespace app {
	extern std::vector<User> users;
	extern ServerSettings settings;

	/**
	 * @brief isUser
	 * Проверяет наличие пользователя в базе
	 * @return
	 * true, если пользователь присутствует; иначе false
	 */
	bool isUser(const QByteArray &login);

	/**
	 * @brief setPassKey
	 * @param login
	 * @param passKey
	 */
	void setPassKey(const QByteArray &login, const QByteArray &passKey);
	/**
	 * @brief setCookie
	 * Устанавливает куки для пользователя
	 * @param login
	 * @param cookie
	 */
	void setIP(const QByteArray &cookie, const QHostAddress &addr);
	void setCookie(const QByteArray &login, const QByteArray &cookie);
	/**
	 * @brief chkPass
	 * Проверяет соответствие пароля пользователя в базе с тем, что пришел
	 * @return
	 * true, если пароль верен; иначе false
	 */
	bool chkPass(const QByteArray &login, const QByteArray &pass);
	bool chkCookie(const QByteArray &cookie);
	QByteArray getLogin(const QByteArray &cookie);
	QByteArray getIPhex(const QByteArray &login);
	QByteArray getNikname(const QByteArray &login);
	unsigned int getUserType(const QByteArray &login);
}

namespace mf {
	QByteArray toBigEndianInt(const int val);
	QByteArray toBigEndianShort(const short val);
	unsigned int getRand(const int max);
	QByteArray md5(const QByteArray &string);
}

#endif // GLOBAL_H
