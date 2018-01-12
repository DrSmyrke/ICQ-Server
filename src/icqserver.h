#ifndef ICQSERVER_H
#define ICQSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QThread>
#include "global.h"
#include "icqclient.h"

class ICQServer : public QObject
{
	Q_OBJECT
public:
	explicit ICQServer(QObject *parent = 0);
	void init(unsigned int port = 5190);
	void deInit();
signals:
	void signal_stopAll();
private slots:
	void slot_incommingConnection();
private:
	QTcpServer* m_pServer;
	std::vector<ICQClient*> m_clients;
};

#endif // ICQSERVER_H
