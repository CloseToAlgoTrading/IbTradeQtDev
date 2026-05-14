#ifndef WORKER_H
#define WORKER_H

#include <QObject>
#include "./IBComm/cbrokerdataprovider.h"

namespace IBWorker
{
	enum WorkerCommand
	{
		IDLE = 0,
		CONNECT,
		DISCONNECT,
		PROCESSING,
        EXIT
	};

	class Worker : public QObject
	{
		Q_OBJECT

	public:
        Worker(QObject *parent, CBrokerDataProvider & _refClient);
		~Worker();

		void setCommand(WorkerCommand cmd);
		//IBComClient& mClient;


		public slots:
		void process();

    signals:
        void connectionAttemptFinished(bool connected);

	private:
        CBrokerDataProvider & m_Client;

		WorkerCommand m_workerCmd;

	};
}
#endif // WORKER_H
