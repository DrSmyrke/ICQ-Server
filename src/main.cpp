#include <QCoreApplication>
#include <QTime>
#include "icqserver.h"
#include "global.h"

int main(int argc, char *argv[])
{
	QTime midnight(0,0,0);
	qsrand(midnight.secsTo(QTime::currentTime()));

	User user;
		user.login = "DrSmyrke";
		user.nikName = user.login;
		user.pass = "123456";
		user.type = icq_user_ICQ;
	app::users.push_back(user);

	QCoreApplication a(argc, argv);

	ICQServer server;

	server.init();

	return a.exec();
}

