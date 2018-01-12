#ifndef ICQCLIENT_H
#define ICQCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include "global.h"

typedef enum{
	pkt_grp_service = 0x01,
	pkt_grp_contacts = 0x03,
	pkt_grp_messages = 0x04,
	pkt_grp_BOS_info = 0x09,
	pkt_grp_auth = 0x17,
}icq_pkt_grp_id;

typedef enum{
	pkt_server_service_available = 0x03,
	pkt_server_service_available_VER = 0x18,
	pkt_server_service_available_full = 0x07,
	pkt_client_service_available = 0x17,
	pkt_client_service_full_request = 0x06,
	pkt_client_conn_info_request = 0x0f,
	pkt_client_param_request = 0x04,
	pkt_client_param = 0x05,
	pkt_login_pass = 0x02,
	pkt_rules_request = 0x02,
	pkt_rules = 0x03,
	pkt_auth = 0x03,
	pkt_login = 0x06,
	pkt_key = 0x07,
}icq_pkt_id;

typedef enum{
	pkt_block_data_login = 0x01,
	pkt_block_data_contacts_count = 0x01,
	pkt_block_data_fantoms_count = 0x02,
	pkt_block_data_online_count = 0x03,
	pkt_block_data_auth_error_info = 0x04,
	pkt_block_data_bos_addr = 0x05,
	pkt_block_data_cookie = 0x06,
	pkt_block_data_auth_error_code = 0x08,
	pkt_block_data_ip = 0x0a,
	pkt_block_data_email = 0x11,
	pkt_block_data_nikname = 0x18,
	pkt_block_data_podpiski = 0x1e,
	pkt_block_data_pass = 0x25,
}icq_pkt_block_data_types;

typedef enum{
	icq_auth_error_nologin = 0x01,
	icq_auth_error = 0x03,
	icq_auth_error_loginPass = 0x05,
}icq_auth_error_code;

typedef enum{
	icq_channel_new_conn = 0x01,
	icq_channel_current_conn = 0x02,
	icq_channel_error = 0x03,
	icq_channel_close_conn = 0x04,
	icq_channel_avail_conn = 0x05,
}icq_channel_code;

struct ICQPktData{
	unsigned short type;
	QByteArray data;
};

struct ICQPkt{
	unsigned char key = 0;
	unsigned char chanelNum = 0;
	unsigned short pktCounter = 0;
	unsigned short dataLen = 0;
	QByteArray data;
	unsigned short groupPktID = 0;
	unsigned short PktID = 0;
	unsigned char dofverF = 0;
	unsigned char dofdataF = 0;
	unsigned int respServF = 0;
	std::vector<ICQPktData> blocksData;
};

class ICQClient : public QObject
{
	Q_OBJECT
public:
	explicit ICQClient(QTcpSocket* socket, QObject *parent = 0);
public slots:
	void stop();
	void process();
signals:
	void signal_finished(ICQClient* client);
private slots:
	void slot_readyRead();
	void slot_stateChanged(const QAbstractSocket::SocketState state);
private:
	QTcpSocket* m_pClient;
	unsigned short m_pktCounter = 1;
	QByteArray m_connectFlag;
	unsigned int m_responseFlag;
	QByteArray m_coockie;
	bool m_auth = false;

	void snd(const char code, const QByteArray &data);
	void sndPkt(const ICQPkt &pkt);
	ICQPkt parsPkt(QByteArray &data);
	/**
	 * @brief printPkt
	 * Выводит на экран содержимое пакета
	 */
	void printPkt(const ICQPkt &pkt);
	QByteArray findData(const std::vector<ICQPktData> &blocksData, const unsigned short idData);
	void sendNoAuth(const unsigned short code, const QByteArray &login);
	void sendYesAuth(const QByteArray &login);
	QByteArray genPktData(const ICQPkt &pkt);
	void sendPassKey(const QByteArray &passKey);
	void sendCloseConnect();
	void sendAvServices();
	void sendAvServices2();
	void sendAvServices3();
	void sendConnInfo();
	void sendRules3();
	void sendMessParams();
	void sendBOSRules();
	void setData(std::vector<ICQPktData> &blocksData, const unsigned short type, const QByteArray &data);
};

#endif // ICQCLIENT_H
