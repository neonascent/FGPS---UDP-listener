/////////////////////////////////////////////////////////////////
// Copyright (C), RenEvo Software & Designs, 2008
// FGPlugin Source File
//
// FlowXmlDataNodes.cpp
//
// Purpose: Flowgraph nodes to dealing with data in Xml elements
//
// History:
//	- 8/23/08 : File created - KAK
/////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "process.h"
#include "winsock2.h"
#include "Nodes/G2FlowBaseNode.h"

// super annoying: http://support.microsoft.com/kb/257460
// must include different header

/* Option to use with [gs]etsockopt at the IPPROTO_IP level */ 
// using ws2_32.lib and winsock2.h

#define	IP_OPTIONS		1 /* set/get IP options */ 
#define	IP_HDRINCL		2 /* header is included with data */ 
#define	IP_TOS			3 /* IP type of service and preced*/ 
#define	IP_TTL			4 /* IP time to live */ 
#define	IP_MULTICAST_IF		9 /* set/get IP multicast i/f  */ 
#define	IP_MULTICAST_TTL       10 /* set/get IP multicast ttl */ 
#define	IP_MULTICAST_LOOP      11 /*set/get IP multicast loopback */ 
#define	IP_ADD_MEMBERSHIP      12 /* add an IP group membership */ 
#define	IP_DROP_MEMBERSHIP     13/* drop an IP group membership */ 
#define IP_DONTFRAGMENT     14 /* don't fragment IP datagrams */ 

////////////////////////////////////////////////////
class CFlowUdpNode_MulticastSender : public CFlowBaseNode
{

	enum EInputPorts
	{
		EIP_Enabled, 
		EIP_Send,
		EIP_Address,
		EIP_Port,
		EIP_Message
	};

	enum EOutputs
	{
		EOP_Success = 0,
		EOP_Fail,
		EOP_Error
	};
	

	// UDP socket stuff
    //init
    int server_length;
    int port;
	string address;
    WSADATA wsaData;
    SOCKET mySocket;
    sockaddr_in myAddress;

	SActivationInfo m_actInfo;  // is this needed?  We already just use pActInfo

public:
	////////////////////////////////////////////////////
	CFlowUdpNode_MulticastSender(SActivationInfo *pActInfo)
	{
		pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
	}

	
	////////////////////////////////////////////////////
	void SendMessage(int port, string address, string message) {
		// init

		//create socket and send
		// inspired by : http://www.cplusplus.com/forum/general/50790/

		if( WSAStartup( MAKEWORD(2, 2), &wsaData ) != NO_ERROR )
		{
			string s = "WSAStartup failed with error";
			ActivateOutput(&m_actInfo, EOP_Error, s);
			WSACleanup();
		} else {

			
			//---------------------------------------------
			// Create a socket for sending data
			mySocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
			if (mySocket == INVALID_SOCKET)
			{
				string s = "socket failed";
				ActivateOutput(&m_actInfo, EOP_Error, s);
				WSACleanup();
			} else {

				
			    //---------------------------------------------
				// Set up the myAddress structure with the IP address of
				// the receiver and the specified port number.
				myAddress.sin_family = AF_INET;
				myAddress.sin_addr.s_addr = inet_addr( address );
				myAddress.sin_port = htons(port);

				//Multicast stuff at http://support.microsoft.com/kb/131978
				
				// bump up ttl a bit
				int ttl = 5; // Arbitrary TTL value.
				;
				if (setsockopt(mySocket, IPPROTO_IP, IP_MULTICAST_TTL, (char *)&ttl, sizeof(ttl)) == SOCKET_ERROR)
				{
					string s = "socket set ttl failed";
					ActivateOutput(&m_actInfo, EOP_Error, s);
					WSACleanup();
				} else {

					if(sendto(mySocket, message, message.length(), 0, (SOCKADDR *) &myAddress, sizeof(struct sockaddr_in)) == SOCKET_ERROR)
					{
						string s = "sending failed";
						ActivateOutput(&m_actInfo, EOP_Error, s);
						WSACleanup();
					} else {
						 //---------------------------------------------
						 // Now close socket
						if (closesocket(mySocket) == SOCKET_ERROR) {
							string s = "socket close failed";
							ActivateOutput(&m_actInfo, EOP_Error, s);
							WSACleanup();
						} else {
							WSACleanup();
							ActivateOutput(&m_actInfo, EOP_Success, true);
							return;
						}
					}
				}
			}
		}

		// failed, send Failed signal
		ActivateOutput(&m_actInfo, EOP_Fail, true); 
		return;
	}
	
	////////////////////////////////////////////////////
	virtual ~CFlowUdpNode_MulticastSender(void)
	{

	}

	////////////////////////////////////////////////////
	virtual void Serialize(SActivationInfo *pActInfo, TSerialize ser)
	{
		
	}

	////////////////////////////////////////////////////
	virtual void GetConfiguration(SFlowNodeConfig& config)
	{
		// Define input ports here, in same order as EInputPorts

		static const SInputPortConfig inputs[] =
		{
			InputPortConfig<bool>("Enabled", false, _HELP("Is sending enabled"), 0, 0),
			InputPortConfig_AnyType("Send", _HELP("Trigger Send of message")),
			InputPortConfig<string>("Address", "225.0.0.1", _HELP("Multicast Address to send to (between 225.0.0.1 and 239.255.255.255)"), 0, 0),
			InputPortConfig<int>("Port", 123, _HELP("Port number"), 0,0),
			InputPortConfig<string>("Message", "test", _HELP("Message to Send"), 0, 0),
			{0}
		};

		// Define output ports here, in same oreder as EOutputPorts
		static const SOutputPortConfig outputs[] =
		{
			OutputPortConfig<bool>("Success", _HELP("UDP socket successfully opened for listening")), 
			OutputPortConfig<bool>("Fail", _HELP("UDP socket failed to open")), 
			OutputPortConfig<string>("Error", _HELP("Any error messages")), 
			{0}
		};

		// Fill in configuration
		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP("Opens a UDP sender");
		//config.SetCategory(EFLN_ADVANCED);
	}


	////////////////////////////////////////////////////
	virtual void ProcessEvent(EFlowEvent event, SActivationInfo *pActInfo)
	{
		m_actInfo = *pActInfo;
		switch (event)
		{
			case eFE_Activate:
			{
				if (GetPortBool(pActInfo, EIP_Enabled)) {
					if (IsPortActive(pActInfo, EIP_Send)) {
						// try to open port socket
						port = GetPortInt(pActInfo, EIP_Port);
						address = GetPortString(pActInfo, EIP_Address);
						string message = GetPortString(pActInfo, EIP_Message);
						SendMessage(port, address, message);
					}
				}
			}
			break;
		}
	}

	////////////////////////////////////////////////////
	virtual IFlowNodePtr Clone(SActivationInfo *pActInfo)
	{
		return new CFlowUdpNode_MulticastSender(pActInfo);
	}

	////////////////////////////////////////////////////
	virtual void GetMemoryStatistics(ICrySizer *s)
	{
		s->Add(*this);
	}

};



////////////////////////////////////////////////////
////////////////////////////////////////////////////

REGISTER_FLOW_NODE("UDP:MulticastSender", CFlowUdpNode_MulticastSender);