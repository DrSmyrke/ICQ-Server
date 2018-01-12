#include "icqserver.h"

ICQServer::ICQServer(QObject *parent) : QObject(parent)
{
	m_pServer = new QTcpServer(this);
	connect(m_pServer,&QTcpServer::newConnection,this,&ICQServer::slot_incommingConnection);
}

void ICQServer::init(unsigned int port)
{
	bool res=m_pServer->listen(QHostAddress::AnyIPv4,port);
	printf("ICQ SERVER %s at %u port\n",((res)?"STARTED":"ERROR [cannot bind port]"),port);
}

void ICQServer::deInit()
{
	if(m_pServer->isListening()) m_pServer->close();
}

void ICQServer::slot_incommingConnection()
{
	QTcpSocket* socket = m_pServer->nextPendingConnection();
	auto addr=socket->peerAddress();
	qDebug()<<"incomming connection from "<<addr;

	ICQClient* client = new ICQClient(socket);
	QThread* thread = new QThread();

	connect(thread,&QThread::started,client,&ICQClient::process);
	/* при запуске потока будет вызван метод process(), который будет работать в новом потоке */
	connect(client,&ICQClient::signal_finished,thread,&QThread::quit);
	/* … и при завершении работы построителя отчетов, обертка построителя передаст потоку сигнал finished() , вызвав срабатывание слота quit()	*/
	connect(this,&ICQServer::signal_stopAll,client,&ICQClient::stop);
	/* … и MainWindow может отправить сигнал о срочном завершении работы обертке построителя, а она уже остановит построитель и направит сигнал finished() потоку */
	connect(client,&ICQClient::signal_finished,client,&ICQClient::deleteLater);
	connect(client,&ICQClient::signal_finished,this,[this](ICQClient* client){
		for(auto iter = m_clients.begin(); iter != m_clients.end(); iter++){
			if((*iter) == client){
				m_clients.erase(iter);
				break;
			}
		}
	});
	/* … и обертка пометит себя для удаления */
	connect(thread,&QThread::finished,thread,&QThread::deleteLater);
	/* … и поток пометит себя для удаления. Удаление будет произведено только после полной остановки потока. */
	thread->start();

	m_clients.push_back(client);
}
