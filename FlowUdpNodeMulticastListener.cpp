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
class CFlowUdpNode_MulticastListener : public CFlowBaseNode
{
	
	struct ip_mreq {
       struct in_addr imr_multiaddr;   /* multicast group to join */ 
       struct in_addr imr_interface;   /* interface to join on    */ 
   };

	enum EInputPorts
	{
		EIP_Enable,
		EIP_Disable,
		EIP_Port, 
		EIP_Multicast,
	};

	enum EOutputs
	{
		EOP_Success = 0,
		EOP_Fail,
		EOP_Received,
		EOP_Value,
		EOP_Error,
	};
	

	// UDP socket stuff
    //init
    int server_length;
    int port;
	int STRLEN;
    char recMessage[256];
    WSADATA wsaData;
    SOCKET mySocket;
    sockaddr_in myAddress;
	ip_mreq mreq;  // multicast address
	string multicast;

	bool socketWorking;
	std::string problem;


	bool m_bEnabled;
	SActivationInfo m_actInfo;  // is this needed?  We already just use pActInfo
	CTimeValue m_lastTime;

public:
	////////////////////////////////////////////////////
	CFlowUdpNode_MulticastListener(SActivationInfo *pActInfo)
	{
		// constructor
		socketWorking = false;
		m_bEnabled = false;
		problem = "nothing done";
	}

	////////////////////////////////////////////////////
	void endSocket() {
		socketWorking = false;
		closesocket(mySocket);
		WSACleanup();
	}

	////////////////////////////////////////////////////
	void startSocket(int port, string multicast) {
		// init
		STRLEN = 256;
		socketWorking = false;

		//create socket
		// a bit messy, from : http://www.cplusplus.com/forum/windows/24301/
		// and non-blocking from: http://www.adp-gmbh.ch/win/misc/sockets.html Socket.cpp
		// also, notes on linking to the right Winsock library for winsock2.h or winsock.h: http://www.codeguru.com/forum/archive/index.php/t-309487.html

		if( WSAStartup( MAKEWORD(2, 2), &wsaData ) != NO_ERROR )
		{
			ActivateOutput(&m_actInfo, EOP_Error, (string) "Socket Initialization: Error with WSAStartup\n"); 
			WSACleanup();

		} else {


			mySocket = socket(AF_INET, SOCK_DGRAM, 0);
			if (mySocket == INVALID_SOCKET)
			{
				ActivateOutput(&m_actInfo, EOP_Error, (string) "Socket Initialization: Error creating socket"); 
				WSACleanup();
			} else {

				//bind
				myAddress.sin_family = AF_INET;
				myAddress.sin_addr.s_addr = inet_addr( "0.0.0.0" );
				myAddress.sin_port = htons(port);

				// set up as non-blocking
				u_long arg = 1;
				ioctlsocket(mySocket, FIONBIO, &arg);
  
				if(bind(mySocket, (SOCKADDR*) &myAddress, sizeof(myAddress)) == SOCKET_ERROR)
				{
					ActivateOutput(&m_actInfo, EOP_Error, (string) "ServerSocket: Failed to connect\n"); 
					WSACleanup();
				} else {
					
					// try to join multicast
					mreq.imr_multiaddr.s_addr = inet_addr(multicast);
					mreq.imr_interface.s_addr = INADDR_ANY;
					if (setsockopt(mySocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof(mreq)) == SOCKET_ERROR) {
						string s = "ServerSocket: Can't join multicast:";
						switch (WSAGetLastError()) {
						case WSANOTINITIALISED:
							s = "WSANOTINITIALISED";
							break;
						case WSAENETDOWN:
							s = "WSAENETDOWN";
							break;
						case WSAEFAULT:
							s = "WSAEFAULT";
							break;
						case WSAEINPROGRESS:
							s = "WSAEINPROGRESS";
							break;
						case WSAEINVAL:
							s = "WSAEINVAL";
							break;
						case WSAENETRESET:
							s = "WSAENETRESET";
							break;
						case WSAENOPROTOOPT:
							s = "WSAENOPROTOOPT";
							break;
						case WSAENOTCONN:
							s = "WSAENOTCONN";
							break;
						case WSAENOTSOCK:
							s = "WSAENOTSOCK";
							break;
						}
			
						ActivateOutput(&m_actInfo, EOP_Error, s ); 
						closesocket(mySocket);
						WSACleanup();
					} else {
						// all went well, send Success signal, and set details
						socketWorking = true;
						ActivateOutput(&m_actInfo, EOP_Error, (string) "no error\n"); 
						ActivateOutput(&m_actInfo, EOP_Success, true);
						return;
					}
				}

			}
		}

		// failed, send Failed signal
		ActivateOutput(&m_actInfo, EOP_Fail, true); 
		return;
	}

	////////////////////////////////////////////////////
	virtual ~CFlowUdpNode_MulticastListener(void)
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
			InputPortConfig_Void("Enable", _HELP("Enable receiving signals")),
			InputPortConfig_Void("Disable", _HELP("Disable receiving signals")),
			InputPortConfig<int>("Port", 123, _HELP("Port number"), 0,0),
			InputPortConfig<string>("Multicast", "225.0.0.1", _HELP("UDP Multicast address (between 225.0.0.1 and 239.255.255.255)"), 0,0),
			{0}
		};

		// Define output ports here, in same oreder as EOutputPorts
		static const SOutputPortConfig outputs[] =
		{
			OutputPortConfig<bool>("Success", _HELP("UDP socket successfully opened for listening")), 
			OutputPortConfig<bool>("Fail", _HELP("UDP socket failed to open")), 
			OutputPortConfig<bool>("Received", _HELP("New data")), 
			OutputPortConfig_Void("Value", _HELP("Value")),
			OutputPortConfig_Void("Error", _HELP("Error generated by UDP Listener")),
			{0}
		};

		// Fill in configuration
		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP("Opens a Multicast UDP listener");
		//config.SetCategory(EFLN_ADVANCED);
	}





	////////////////////////////////////////////////////
	virtual void ProcessEvent(EFlowEvent event, SActivationInfo *pActInfo)
	{
		switch (event)
		{
			case eFE_Initialize:
			{
				m_actInfo = *pActInfo;
			}
			break;

			case eFE_Activate:
			{
				if (IsPortActive(pActInfo, EIP_Enable)) {
					
					if (socketWorking) {
						endSocket();
					}
					
					m_bEnabled = true;
					// try to open port socket
					port = GetPortInt(pActInfo, EIP_Port);
					multicast = GetPortString(pActInfo, EIP_Multicast);
					startSocket(port, multicast);

					Execute(pActInfo);
					pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
				}
				if (IsPortActive(pActInfo, EIP_Disable)) {
					m_bEnabled = false;
					endSocket();
					pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
				}
			}
			break;

			case eFE_Update:
			{
				CTimeValue currTime(gEnv->pTimer->GetCurrTime());
				float delay = 0;  // processing delay
				delay -= (currTime-m_lastTime).GetSeconds();
				m_lastTime = currTime;

				// Execute?
				if (delay <= 0.0f)
				{
					Execute(pActInfo);
				}
			}
			break;
		}
	}

	////////////////////////////////////////////////////
	virtual IFlowNodePtr Clone(SActivationInfo *pActInfo)
	{
		return new CFlowUdpNode_MulticastListener(pActInfo);
	}

	////////////////////////////////////////////////////
	virtual void GetMemoryStatistics(ICrySizer *s)
	{
		s->Add(*this);
	}


	int ReceiveLine() {
	  int size = -1;
		if (socketWorking) {
		server_length = sizeof(struct sockaddr_in);
		size = recvfrom(mySocket, recMessage, STRLEN, 0, (SOCKADDR*) &myAddress, &server_length);
		/*if (size == SOCKET_ERROR) {
			  // get last error
			switch(WSAGetLastError()) {
			  case WSANOTINITIALISED:
					return "WSANOTINITIALISED";
			  case WSAENETDOWN:
					return "WSAENETDOWN";
			  case WSAEFAULT:
				  return "WSAEFAULT";
			  case WSAENOTCONN:
				  return "WSAENOTCONN";
			  case WSAEINTR:
				  return "WSAEINTR";
			  case WSAEINPROGRESS:
				  return "WSAEINPROGRESS";
			  case WSAENETRESET:
				  return "WSAENETRESET";
			  case WSAENOTSOCK:
				  return "WSAENOTSOCK";
			  case WSAEOPNOTSUPP:
				  return "WSAEOPNOTSUPP";
			  case WSAESHUTDOWN:
				  return "WSAESHUTDOWN";
			  case WSAEWOULDBLOCK:
				  return "WSAEWOULDBLOCK";
			  case WSAEMSGSIZE:
				  return "WSAEMSGSIZE";
			  case WSAEINVAL:
				  return "WSAEINVAL";
			  case WSAECONNABORTED:
				  return "WSAECONNABORTED";
			  case WSAETIMEDOUT:
				  return "WSAETIMEDOUT";
			  case WSAECONNRESET:
				  return "WSAECONNRESET";
			  default:
				  return "UNKNOWN SOCKET ERROR";
			}
		}*/
		if (size != SOCKET_ERROR) {
			recMessage[size] = '\0';
		} 
	  }
	  return size;
	}

	////////////////////////////////////////////////////
	virtual void Execute(SActivationInfo *pActInfo)
	{
		bool bResult = false;

		// did the socket connect okay?
		if (socketWorking) {
			if (ReceiveLine() != -1) {
				std::string r = recMessage;
				string value = r.c_str();
				ActivateOutput(pActInfo, EOP_Value, value);
				bResult = true;
			}
		}
	
		// return false if socket error, or no message
		
		if (bResult) ActivateOutput(pActInfo, EOP_Received, true);
		return;
	}

};


////////////////////////////////////////////////////
////////////////////////////////////////////////////

REGISTER_FLOW_NODE("UDP:MulticastListener", CFlowUdpNode_MulticastListener);

