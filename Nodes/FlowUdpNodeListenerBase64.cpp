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
#include "base64.h"
#include <iostream>
#include "process.h"
#include "winsock2.h"
#include "Nodes/G2FlowBaseNode.h"

////////////////////////////////////////////////////
class CFlowUdpNode_ListenerBase64 : public CFlowBaseNode
{
	
	enum EInputPorts
	{
		EIP_Enable,
		EIP_Disable,
		EIP_Port, 
	};

	enum EOutputs
	{
		EOP_Success = 0,
		EOP_Fail,
		EOP_Received,
		EOP_Value,
	};
	

	// UDP socket stuff
    //init
    int server_length;
    int port;
	int STRLEN;
    char recMessage[1024];
	std::string base64Message;
    WSADATA wsaData;
    SOCKET mySocket;
    sockaddr_in myAddress;

	bool socketWorking;
	std::string problem;


	bool m_bEnabled;
	SActivationInfo m_actInfo;  // is this needed?  We already just use pActInfo
	CTimeValue m_lastTime;

public:
	////////////////////////////////////////////////////
	CFlowUdpNode_ListenerBase64(SActivationInfo *pActInfo)
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
	void startSocket(int port) {
		// init
		STRLEN = 1024;
		socketWorking = false;

		//create socket
		// a bit messy, from : http://www.cplusplus.com/forum/windows/24301/
		// and non-blocking from: http://www.adp-gmbh.ch/win/misc/sockets.html Socket.cpp

		if( WSAStartup( MAKEWORD(2, 2), &wsaData ) != NO_ERROR )
		{
			problem = "Socket Initialization: Error with WSAStartup\n";
			WSACleanup();

		} else {


			mySocket = socket(AF_INET, SOCK_DGRAM, 0);
			if (mySocket == INVALID_SOCKET)
			{
				problem = "Socket Initialization: Error creating socket";
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
					problem = "ServerSocket: Failed to connect\n";
					WSACleanup();
				} else {
					// all went well, send Success signal, and set details
					socketWorking = true;
					problem = "no problem";
					ActivateOutput(&m_actInfo, EOP_Success, true);
					return;
				}

			}
		}

		// failed, send Failed signal
		ActivateOutput(&m_actInfo, EOP_Fail, true); 
		return;
	}

	////////////////////////////////////////////////////
	virtual ~CFlowUdpNode_ListenerBase64(void)
	{
		// destructor
		if (socketWorking) {
			endSocket();
		}
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
			InputPortConfig<int>("Port", 1234, _HELP("Port number"), 0,0),
			{0}
		};

		// Define output ports here, in same oreder as EOutputPorts
		static const SOutputPortConfig outputs[] =
		{
			OutputPortConfig<bool>("Success", _HELP("UDP socket successfully opened for listening")), 
			OutputPortConfig<bool>("Fail", _HELP("UDP socket failed to open")), 
			OutputPortConfig<bool>("Received", _HELP("New data")), 
			OutputPortConfig_Void("Value", _HELP("Value")),
			{0}
		};

		// Fill in configuration
		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP("Opens a UDP listener");
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
					startSocket(port);

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
		return new CFlowUdpNode_ListenerBase64(pActInfo);
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
			// convert string to base64
			  /*const std::string s = "ADP GmbH\nAnalyse Design & Programmierung\nGesellschaft mit beschränkter Haftung" ;

			  std::string encoded = base64_encode(reinterpret_cast<const unsigned char*>(s.c_str()), s.length());
			  std::string decoded = base64_decode(encoded);

			  std::cout << "encoded: " << encoded << std::endl;
			  std::cout << "decoded: " << decoded << std::endl;
			  */


			base64Message = base64_encode(reinterpret_cast<const unsigned char*>(recMessage), size);

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
				string value = base64Message.c_str();
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

REGISTER_FLOW_NODE("UDP:ListenerBase64", CFlowUdpNode_ListenerBase64);

