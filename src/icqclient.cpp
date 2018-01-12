#include "icqclient.h"
//TODO: remove qDEbug
#include <QDebug>
#include <QHostAddress>

/*
	https://sites.google.com/site/imaderingcity/im-world/im-protocols/icq-protocol
*/

ICQClient::ICQClient(QTcpSocket *socket, QObject *parent)
	: QObject(parent)
	, m_pClient(socket)
{
	const char data[] = {0x00,0x00,0x00,0x01};
	m_connectFlag = QByteArray(data,sizeof(data));
	//snd(icq_channel_new_conn,m_connectFlag);

	m_responseFlag = mf::getRand(65000);

	connect(socket,&QTcpSocket::stateChanged,this,&ICQClient::slot_stateChanged);
	connect(socket,&QTcpSocket::readyRead,this,&ICQClient::slot_readyRead);
}

void ICQClient::stop()
{
	if(m_pClient->isOpen()) m_pClient->close();
	m_pClient->deleteLater();
	emit signal_finished(this);
}

void ICQClient::process()
{

}

void ICQClient::slot_readyRead()
{
//	QObject* object = QObject::sender();
//	if (!object) return;
//	QTcpSocket* socket = static_cast<QTcpSocket *>(object);

	QByteArray buff;
	while(m_pClient->bytesAvailable()){
		buff.append( m_pClient->read(1024) );
	}

	while(buff.size() > 0){
		auto pkt = parsPkt(buff);
		printPkt(pkt);

		if(pkt.chanelNum == icq_channel_new_conn){
			QByteArray cookie = findData(pkt.blocksData,pkt_block_data_cookie);
			if(app::chkCookie(cookie)){
				m_coockie.append(cookie);
				sendAvServices();
				m_auth = true;
			}else{
				//TODO: Реализовать отлуп в авторизации
			}
			continue;
		}
		//FIXME: Заумутить отлуп, если авторизация не пройдена

		switch (pkt.groupPktID) {
			case pkt_grp_auth:
				switch (pkt.PktID) {
					case pkt_login:
					{
						QByteArray login = findData(pkt.blocksData,pkt_block_data_login);
						if(app::isUser(login)){

							QByteArray passCode;
							passCode.append(mf::getRand(65000));
							passCode.append(mf::getRand(65000));
							passCode.append(mf::getRand(65000));
							passCode.append(mf::getRand(65000));
							passCode.append(mf::getRand(65000));
							app::setPassKey(login,passCode);
							sendPassKey(passCode);

						}else{ sendNoAuth(icq_auth_error_nologin,login); }
					}
					break;
					case pkt_login_pass:
						QByteArray login = findData(pkt.blocksData,pkt_block_data_login);
						QByteArray pass = findData(pkt.blocksData,pkt_block_data_pass);
						if(app::chkPass(login,pass)){
							sendYesAuth(login);
						}else{ sendNoAuth(icq_auth_error_loginPass,login); }
					break;
				}
			break;
			case pkt_grp_service:
				switch (pkt.PktID) {
					case pkt_client_service_available:
						//FIXME: Костыль переделать (замутить обработку пакета 113)
						sendAvServices2();
					break;
					case pkt_client_service_full_request: sendAvServices3(); break;
					case pkt_client_conn_info_request:
						app::setIP(m_coockie,m_pClient->peerAddress());
						sendConnInfo();
					break;
				}
			break;
			case pkt_grp_contacts:
				switch (pkt.PktID) {
					case pkt_rules_request: sendRules3(); break;
				}
			break;
			case pkt_grp_messages:
				switch (pkt.PktID) {
					case pkt_client_param_request: sendMessParams(); break;
				}
			break;
			case pkt_grp_BOS_info:
				switch (pkt.PktID) {
					case pkt_rules_request: sendBOSRules(); break;
				}
			break;
		}
	}

	if(buff.size() > 0) qDebug()<<">:"<<buff.toHex();
}

void ICQClient::slot_stateChanged(const QAbstractSocket::SocketState state)
{
//	QObject* object = QObject::sender();
//	if (!object) return;
//	QTcpSocket* socket = static_cast<QTcpSocket *>(object);

	if(state == QAbstractSocket::UnconnectedState){
		auto addr=m_pClient->peerAddress();
		qDebug()<<"connection from "<<addr<<" closed.";
		m_pClient->deleteLater();
		emit signal_finished(this);
	}
}

void ICQClient::snd(const char code, const QByteArray &data)
{
	QByteArray ba;
		ba[0] = 0x2a;
		ba.append(code);
		ba.append(mf::toBigEndianShort(m_pktCounter));
		ba.append(mf::toBigEndianShort(data.size()));
		ba.append(data);
	m_pClient->write(ba);
	m_pClient->waitForBytesWritten(100);
	qDebug()<<"<:"<<ba.toHex();
	m_pktCounter++;
}

void ICQClient::sndPkt(const ICQPkt &pkt)
{
	QByteArray ba;
		ba[0] = 0x2a;
		ba.append(pkt.chanelNum);
		ba.append(mf::toBigEndianShort(pkt.pktCounter));
		ba.append(mf::toBigEndianShort(pkt.dataLen));
		ba.append(pkt.data);
	m_pClient->write(ba);
	m_pClient->waitForBytesWritten(100);
	qDebug()<<"<:"<<ba.toHex();
}

ICQPkt ICQClient::parsPkt(QByteArray &data)
{
	ICQPkt pkt;

	if(data.size() < 6) return pkt;

	pkt.key = data[0];
	pkt.chanelNum = data[1];
	pkt.pktCounter = data[2]<<4;
	pkt.pktCounter += data[3];
	pkt.dataLen = data[4]<<4;
	pkt.dataLen += data[5];
	data.remove(0,6);

	if(pkt.dataLen > 0){
		pkt.data = data.left(pkt.dataLen);
		data.remove(0,pkt.dataLen);
	}

	// testing data
	QByteArray tmpData;
	tmpData.append(pkt.data);

	if(tmpData.left(4) != m_connectFlag){
		if(tmpData.size() >= 2){
			pkt.groupPktID = tmpData[0]<<4;
			pkt.groupPktID  += tmpData[1];
			tmpData.remove(0,2);
		}
		if(tmpData.size() >= 2){
			pkt.PktID = tmpData[0]<<4;
			pkt.PktID  += tmpData[1];
			tmpData.remove(0,2);
		}
		if(tmpData.size() >= 1){
			pkt.dofverF = tmpData[0];
			tmpData.remove(0,1);
		}
		if(tmpData.size() >= 1){
			pkt.dofdataF = tmpData[0];
			tmpData.remove(0,1);
		}
		if(tmpData.size() >= 4){
			pkt.respServF = tmpData[0]<<12;
			pkt.respServF += tmpData[1]<<8;
			pkt.respServF += tmpData[2]<<4;
			pkt.respServF += tmpData[3];
			tmpData.remove(0,4);
		}
	}else{ tmpData.remove(0,4); }

	while(tmpData.size() >= 8){
		unsigned short dataLen = 0;
		ICQPktData pktData;

			pktData.type = tmpData[0]<<4;
			pktData.type += tmpData[1];
			tmpData.remove(0,2);

			dataLen = tmpData[0]<<4;
			dataLen += tmpData[1];
			tmpData.remove(0,2);

		if(dataLen > 0){
			pktData.data = tmpData.left(dataLen);
			tmpData.remove(0,(dataLen));
			pkt.blocksData.push_back(pktData);
		}
	}

	return pkt;
}

void ICQClient::printPkt(const ICQPkt &pkt)
{
	printf(">: #%u %x %x [%u] %s\n",pkt.pktCounter,pkt.key,pkt.chanelNum,pkt.dataLen,pkt.data.toHex().data());
	// printing data
	switch (pkt.groupPktID) {
		case 0x17: printf("0x17 group auth packet\n"); break;
	}
	switch (pkt.PktID) {
		case 0x02: printf("0x02 login+pass packet\n"); break;
		case 0x06: printf("0x06 login packet\n"); break;
	}
	for(auto elem:pkt.blocksData){
		printf("[%x] %s\n",elem.type,elem.data.data());
	}
}

QByteArray ICQClient::findData(const std::vector<ICQPktData> &blocksData, const unsigned short idData)
{
	QByteArray str;
	for(auto elem:blocksData){
		if(elem.type == idData){
			str.append(elem.data);
			break;
		}
	}
	return str;
}

void ICQClient::sendNoAuth(const unsigned short code, const QByteArray &login)
{
	ICQPkt pkt;
		pkt.chanelNum = icq_channel_current_conn;
		pkt.pktCounter = m_pktCounter++;
		pkt.groupPktID = pkt_grp_auth;
		pkt.PktID = pkt_auth;

		setData(pkt.blocksData,pkt_block_data_login,login);
		setData(pkt.blocksData,pkt_block_data_auth_error_info,"Http://drsmyrk-home.pskoline.ru");
		setData(pkt.blocksData,pkt_block_data_auth_error_code,mf::toBigEndianShort(code));

	pkt.data = genPktData(pkt);
	pkt.dataLen = pkt.data.size();

	sndPkt(pkt);

	sendCloseConnect();
}

void ICQClient::sendYesAuth(const QByteArray &login)
{
	ICQPkt pkt;
		pkt.chanelNum = icq_channel_current_conn;
		pkt.pktCounter = m_pktCounter++;
		pkt.groupPktID = pkt_grp_auth;
		pkt.PktID = pkt_auth;

		setData(pkt.blocksData,pkt_block_data_login,login);
		setData(pkt.blocksData,pkt_block_data_bos_addr,"127.0.0.1:5190");

		ICQPktData cookieData;
			cookieData.type = pkt_block_data_cookie;
			QByteArray cookie;
			for(char i = 0; i < 30; i++) cookie.append(mf::getRand(65000));
			cookieData.data.append(cookie);
		pkt.blocksData.push_back(cookieData);

		app::setCookie(login,cookie);

	pkt.data = genPktData(pkt);
	pkt.dataLen = pkt.data.size();

	sndPkt(pkt);

	sendCloseConnect();
}

QByteArray ICQClient::genPktData(const ICQPkt &pkt)
{
	QByteArray ba;
		ba.append(mf::toBigEndianShort(pkt.groupPktID));
		ba.append(mf::toBigEndianShort(pkt.PktID));
		ba.append(pkt.dofverF);
		ba.append(pkt.dofdataF);
		ba.append(mf::toBigEndianInt(pkt.respServF));
		for(auto elem:pkt.blocksData){
			ba.append(mf::toBigEndianShort(elem.type));
			ba.append(mf::toBigEndianShort(elem.data.size()));
			ba.append(elem.data);
		}
	return ba;
}

void ICQClient::sendPassKey(const QByteArray &passKey)
{
	ICQPkt pkt;
		pkt.chanelNum = icq_channel_current_conn;
		pkt.pktCounter = m_pktCounter++;
		pkt.groupPktID = pkt_grp_auth;
		pkt.PktID = pkt_key;

	pkt.data = genPktData(pkt);
	pkt.data.append(mf::toBigEndianShort(passKey.size()));
	pkt.data.append(passKey);

	pkt.dataLen = pkt.data.size();

	sndPkt(pkt);
}

void ICQClient::sendCloseConnect()
{
	const char data[] = {0x00,0x00};
	snd(icq_channel_close_conn,QByteArray(data,sizeof(data)));
}

void ICQClient::sendAvServices()
{
	ICQPkt pkt;
		pkt.chanelNum = icq_channel_current_conn;
		pkt.pktCounter = m_pktCounter++;
		pkt.groupPktID = pkt_grp_service;
		pkt.PktID = pkt_server_service_available;
		pkt.respServF = m_responseFlag;

	pkt.data = genPktData(pkt);
	// razdel 111
	QByteArray ba;
	//0001 // Поддержка группы пакетов с сервисной информацией
	ba[0] = 0x00;ba[1] = 0x01;
	pkt.data.append(ba);
	//0003 //Поддержка группы пакетов для операций со списком контактов
	ba[0] = 0x00;ba[1] = 0x03;
	pkt.data.append(ba);
	//0004 //Поддержка группы пакетов для отправки и получения сообщений
	ba[0] = 0x00;ba[1] = 0x04;
	pkt.data.append(ba);
	//000a //Поддержка группы пакетов с информацией о пользователе
	ba[0] = 0x00;ba[1] = 0x0a;
	pkt.data.append(ba);

	pkt.dataLen = pkt.data.size();

	sndPkt(pkt);
}

void ICQClient::sendAvServices2()
{
	ICQPkt pkt;
		pkt.chanelNum = icq_channel_current_conn;
		pkt.pktCounter = m_pktCounter++;
		pkt.groupPktID = pkt_grp_service;
		pkt.PktID = pkt_server_service_available_VER;
		pkt.respServF = m_responseFlag;

	pkt.data = genPktData(pkt);
	// razdel 111
	QByteArray ba;
	//0001 // Поддержка группы пакетов с сервисной информацией
	ba[0] = 0x00;ba[1] = 0x01;ba[2] = 0x00;ba[3] = 0x04;
	pkt.data.append(ba);
	//0003 //Поддержка группы пакетов для операций со списком контактов
	ba[0] = 0x00;ba[1] = 0x03;ba[2] = 0x00;ba[3] = 0x01;
	pkt.data.append(ba);
	//0004 //Поддержка группы пакетов для отправки и получения сообщений
	ba[0] = 0x00;ba[1] = 0x04;ba[2] = 0x00;ba[3] = 0x01;
	pkt.data.append(ba);
	//000a //Поддержка группы пакетов с информацией о пользователе
	ba[0] = 0x00;ba[1] = 0x0a;ba[2] = 0x00;ba[3] = 0x01;
	pkt.data.append(ba);

	pkt.dataLen = pkt.data.size();

	sndPkt(pkt);
}

void ICQClient::sendAvServices3()
{
	ICQPkt pkt;
		pkt.chanelNum = icq_channel_current_conn;
		pkt.pktCounter = m_pktCounter++;
		pkt.groupPktID = pkt_grp_service;
		pkt.PktID = pkt_server_service_available_full;
		pkt.respServF = m_responseFlag;

	pkt.data = genPktData(pkt);
	// razdel 117
	QByteArray ba;
	// Количество классов на которые разбита информация о сервисах
	ba[0] = 0x00;ba[1] = 0x01;
	pkt.data.append(ba);
	// Идентификатор класса 1
	pkt.data.append(ba);
	// Размер окна (значение: 80)
	ba[0] = 0x00;ba[1] = 0x00;ba[2] = 0x00;ba[3] = 0x50;
	pkt.data.append(ba);
	// Чистый уровень (значение: 2500)
	ba[0] = 0x00;ba[1] = 0x00;ba[2] = 0x09;ba[3] = 0xc4;
	pkt.data.append(ba);
	// Уровень предупреждений (значение: 2000)
	ba[0] = 0x00;ba[1] = 0x00;ba[2] = 0x07;ba[3] = 0xd0;
	pkt.data.append(ba);
	// Лимит (значение: 1500)
	ba[0] = 0x00;ba[1] = 0x00;ba[2] = 0x05;ba[3] = 0xdc;
	pkt.data.append(ba);
	// Уровень отключения (значение: 800)
	ba[0] = 0x00;ba[1] = 0x00;ba[2] = 0x03;ba[3] = 0x20;
	pkt.data.append(ba);
	// Текущий уровень (значение: 5866)
	ba[0] = 0x00;ba[1] = 0x00;ba[2] = 0x16;ba[3] = 0xea;
	pkt.data.append(ba);
	// Максимальный уровень (значение: 6000)
	ba[0] = 0x00;ba[1] = 0x00;ba[2] = 0x17;ba[3] = 0x70;
	pkt.data.append(ba);
	// Текущее время (значение: 0)
	ba[0] = 0x00;ba[1] = 0x00;ba[2] = 0x00;ba[3] = 0x00;
	pkt.data.append(ba);
	// Неизвестные данные
	ba[0] = 0x00;ba.resize(1);
	pkt.data.append(ba);

	pkt.dataLen = pkt.data.size();

	sndPkt(pkt);
}

void ICQClient::sendConnInfo()
{
	ICQPkt pkt;
		pkt.chanelNum = icq_channel_current_conn;
		pkt.pktCounter = m_pktCounter++;
		pkt.groupPktID = pkt_grp_service;
		pkt.PktID = pkt_client_conn_info_request;
		pkt.respServF = m_responseFlag;

		auto login = app::getLogin(m_coockie);
		auto userType = app::getUserType(login);

		setData(pkt.blocksData,pkt_block_data_login,mf::toBigEndianShort(userType+10+1));
		setData(pkt.blocksData,pkt_block_data_ip,app::getIPhex(login));
		setData(pkt.blocksData,pkt_block_data_nikname,app::getNikname(login));
		setData(pkt.blocksData,pkt_block_data_podpiski,mf::toBigEndianInt(0));
		//Таблица параметров родительского контроля

	pkt.data = genPktData(pkt);

	// razdel 127
	// Количество вложенных блоков данных в пакете
	pkt.data.prepend(mf::toBigEndianInt(pkt.blocksData.size()));
	//Уровень предупреждений (нарушений лимитов)
	pkt.data.prepend(mf::toBigEndianInt(0));
	QByteArray ba;
	ba[0] = login.size();
	pkt.data.prepend(login);
	pkt.data.prepend(ba);
	// TODO: Версия 2:


	pkt.dataLen = pkt.data.size();

	sndPkt(pkt);
}

void ICQClient::sendRules3()
{
	ICQPkt pkt;
		pkt.chanelNum = icq_channel_current_conn;
		pkt.pktCounter = m_pktCounter++;
		pkt.groupPktID = pkt_grp_contacts;
		pkt.PktID = pkt_rules;
		pkt.respServF = m_responseFlag;

		// Максимальное количество контактов в списке
		setData(pkt.blocksData,pkt_block_data_contacts_count,mf::toBigEndianShort(app::settings.maxContactListSize));
		//Максимальное количество "наблюдателей"
		setData(pkt.blocksData,pkt_block_data_fantoms_count,mf::toBigEndianShort(app::settings.maxFantomListSize));
		//Максимальное количество онлайн контактов в списке
		setData(pkt.blocksData,pkt_block_data_online_count,mf::toBigEndianShort(app::settings.maxOnlineContactListSize));

	pkt.data = genPktData(pkt);
	// razdel 129
	pkt.dataLen = pkt.data.size();

	sndPkt(pkt);
}

void ICQClient::sendMessParams()
{
	ICQPkt pkt;
		pkt.chanelNum = icq_channel_current_conn;
		pkt.pktCounter = m_pktCounter++;
		pkt.groupPktID = pkt_grp_messages;
		pkt.PktID = pkt_client_param;
		pkt.respServF = m_responseFlag;

	pkt.data = genPktData(pkt);
	// razdel 130
	// Канал сообщений
	pkt.data.append(mf::toBigEndianShort(1));
	// Флаг сообщений
	pkt.data.append(mf::toBigEndianInt(3));
	// Максимальный размер пакета с сообщением
	pkt.data.append(mf::toBigEndianShort(512));
	// Максимальный уровень предупреждений для отправляющего пользователя
	pkt.data.append(mf::toBigEndianShort(900));
	// Максимальный уровень предупреждений для получающего пользователя
	pkt.data.append(mf::toBigEndianShort(999));
	// Минимальный интервал между отправляемыми сообщениями (значение: 1 секунда)
	pkt.data.append(mf::toBigEndianInt(1000));

	pkt.dataLen = pkt.data.size();

	sndPkt(pkt);
}

void ICQClient::sendBOSRules()
{
	ICQPkt pkt;
		pkt.chanelNum = icq_channel_current_conn;
		pkt.pktCounter = m_pktCounter++;
		pkt.groupPktID = pkt_grp_BOS_info;
		pkt.PktID = pkt_rules;
		pkt.respServF = m_responseFlag;

		// FIXME: Костыль
		// Метка блока данных с информацией о размере списка "невидящих" вас пользователей
		setData(pkt.blocksData,0x02,mf::toBigEndianShort(1000));
		// Метка блока данных с информацией о размере списка "видящих" вас пользователей
		setData(pkt.blocksData,0x01,mf::toBigEndianShort(1000));

	pkt.data = genPktData(pkt);
	// razdel 131

	pkt.dataLen = pkt.data.size();

	sndPkt(pkt);
}

void ICQClient::setData(std::vector<ICQPktData> &blocksData, const unsigned short type, const QByteArray &data)
{
	if(!type or data.size() == 0) return;
	ICQPktData blockData;
		blockData.type = type;
		blockData.data.append(data);
	blocksData.push_back(blockData);
}
