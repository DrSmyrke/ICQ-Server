#include "global.h"
#include <QIODevice>
#include <QDataStream>
#include <QCryptographicHash>
//TODO: remove qDEbug
#include <QDebug>

namespace app {
	std::vector<User> users;
	ServerSettings settings;

	bool isUser(const QByteArray &login)
	{
		for(auto elem:app::users){
			if(elem.login == login){
				return true;
				break;
			}
		}
		return false;
	}

	void setPassKey(const QByteArray &login, const QByteArray &passKey)
	{
		for(auto &elem:app::users){
			if(elem.login == login){
				elem.passKey = passKey;
				break;
			}
		}
	}

	bool chkPass(const QByteArray &login, const QByteArray &pass)
	{
		for(auto elem:app::users){
			if(elem.login == login){
				auto myPass = mf::md5(elem.passKey+elem.pass+"AOL Instant Messenger (SM)");
				auto myPass2 = mf::md5(elem.passKey+mf::md5(elem.pass)+"AOL Instant Messenger (SM)");
				if( myPass == pass or myPass2 == pass ) return true;
				break;
			}
		}
		return false;
	}

	void setCookie(const QByteArray &login, const QByteArray &cookie)
	{
		for(auto &elem:app::users){
			if(elem.login == login){
				elem.cookie = cookie;
				break;
			}
		}
	}

	bool chkCookie(const QByteArray &cookie)
	{
		for(auto &elem:app::users){
			if(elem.cookie == cookie){
				return true;
				break;
			}
		}
		return false;
	}

	QByteArray getLogin(const QByteArray &cookie)
	{
		QByteArray ba;
		for(auto elem:app::users){
			if(elem.cookie == cookie){
				ba.append(elem.login);
				break;
			}
		}
		return ba;
	}

	unsigned int getUserType(const QByteArray &login)
	{
		unsigned int type;
		for(auto elem:app::users){
			if(elem.login == login){
				type = elem.type;
				break;
			}
		}
		return type;
	}

	void setIP(const QByteArray &cookie, const QHostAddress &addr)
	{
		for(auto &elem:app::users){
			if(elem.cookie == cookie){
				elem.ip = addr;
				break;
			}
		}
	}

	QByteArray getIPhex(const QByteArray &login)
	{
		QByteArray ba;
		for(auto elem:app::users){
			if(elem.login == login){
				QByteArray tmp;
				for(auto byte:elem.ip.toString().split(".")){
					tmp[0] = byte.toUInt();
					ba.append(tmp);
				}
				break;
			}
		}
		return ba;
	}

	QByteArray getNikname(const QByteArray &login)
	{
		QByteArray ba;
		for(auto elem:app::users){
			if(elem.login == login){
				ba.append(elem.nikName);
				break;
			}
		}
		return ba;
	}

}

namespace mf {

	QByteArray toBigEndianInt(const int val)
	{
		QByteArray bytes(4, 0x00);
		QDataStream stream(&bytes, QIODevice::WriteOnly);
		stream.setByteOrder(QDataStream::BigEndian);
		stream << val;
		bytes.resize(4);
		return bytes;
	}

	QByteArray toBigEndianShort(const short val)
	{
		QByteArray bytes(2, 0x00);
		QDataStream stream(&bytes, QIODevice::WriteOnly);
		stream.setByteOrder(QDataStream::BigEndian);
		stream << val;
		bytes.resize(2);
		return bytes;
	}

	unsigned int getRand(const int max)
	{
		return qrand() % max;
	}

	QByteArray md5(const QByteArray &string)
	{
		return QCryptographicHash::hash(string,QCryptographicHash::Md5);
	}
}
