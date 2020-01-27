#include "IBworker.h"
#include "GlobalDef.h"
#include "NHelper.h"

namespace IBWorker
{
    Worker::Worker(QObject *parent, CBrokerDataProvider & _refClient)
		: QObject(parent)
        , m_Client(_refClient)
		, m_workerCmd(IDLE)
	{

	}

	Worker::~Worker()
	{

	}

    void Worker::process()
    {
        while (true)
        {

            if (nullptr != m_Client.getClien())
            {
                switch (m_workerCmd)
                {
                case CONNECT:
                    m_workerCmd = IDLE;
                    if (!m_Client.getClien()->isConnectedAPI())
                    {
                        m_Client.getClien()->connectAPI(CONNECTIONS_SERVER_IP, NHelper::getServerPort(), CONNECTIONS_CLIENT_ID);
                    }
                    break;
                case DISCONNECT:
                    m_workerCmd = IDLE;
                    if (m_Client.getClien()->isConnectedAPI())
                    {
                        m_Client.getClien()->disconnectAPI();
                    }
                    break;
                case PROCESSING:
                    if (m_Client.getClien()->isConnectedAPI())
                    {
                        m_Client.getClien()->processMessagesAPI();
                    }
                    else
                    {
                        m_workerCmd = IDLE;
                    }
                    break;
                case IDLE:
                default:
                    if (m_Client.getClien()->isConnectedAPI())
                    {
                        m_workerCmd = PROCESSING;
                    }
                    break;

                case EXIT:
                    if (m_Client.getClien()->isConnectedAPI())
                    {
                        m_Client.getClien()->disconnectAPI();
                    }
                    return;
                    break;
                }
            }
        }
    }

	void Worker::setCommand(IBWorker::WorkerCommand cmd)
	{
		m_workerCmd = cmd;
	}
}
