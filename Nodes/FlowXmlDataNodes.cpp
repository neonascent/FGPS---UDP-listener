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
#include "FlowBaseXmlNode.h"
#include "Socket.h"
#include "process.h"
#include "sstream"

////////////////////////////////////////////////////
class CFlowXmlNode_GetValue : public CFlowXmlNode_Base
{
	
	enum EInputPorts
	{
		EIP_Port = EIP_CustomStart,  // "= EIP_CustomStart" essential to start additional inputs after inherited stuff
	};

	enum EOutputs
	{
		EOP_Value = EOP_CustomStart,
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

	bool socketWorking;
	std::string problem;

public:
	////////////////////////////////////////////////////
	CFlowXmlNode_GetValue(SActivationInfo *pActInfo) : CFlowXmlNode_Base(pActInfo)
	{
		socketWorking = false;
		problem = "nothing done";
	}

	void endSocket() {
		socketWorking = false;
		closesocket(mySocket);
		WSACleanup();
	}

	void startSocket(int port) {
		// init
		STRLEN = 256;
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

					socketWorking = true;
					problem = "no problem";
				}

			}
		}
	}

	////////////////////////////////////////////////////
	virtual ~CFlowXmlNode_GetValue(void)
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
			ADD_BASE_INPUTS(),
			InputPortConfig<int>("Port", 123, _HELP("Port number"), 0,0),
			{0}
		};

		// Define output ports here, in same oreder as EOutputPorts
		static const SOutputPortConfig outputs[] =
		{
			ADD_BASE_OUTPUTS(),
			OutputPortConfig_Void("Value", _HELP("Value")),
			{0}
		};

		// Fill in configuration
		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP("Get the value of the active element");
		config.SetCategory(EFLN_APPROVED);
	}

	////////////////////////////////////////////////////
	virtual IFlowNodePtr Clone(SActivationInfo *pActInfo)
	{
		return new CFlowXmlNode_GetValue(pActInfo);
	}

	////////////////////////////////////////////////////
	virtual void GetMemoryStatistics(ICrySizer *s)
	{
		s->Add(*this);
	}


	std::string ReceiveLine() {
	  std::string ret = "";
	  if (socketWorking) {
		server_length = sizeof(struct sockaddr_in);
		int size = recvfrom(mySocket, recMessage, STRLEN, 0, (SOCKADDR*) &myAddress, &server_length);
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
			ret += recMessage;
			return ret;
		} 
	  }
	  return "<failed>";
	}

	////////////////////////////////////////////////////
	virtual bool Execute(SActivationInfo *pActInfo)
	{
		
		// if port isn't working then try to start it (might be first time)
		if (!socketWorking) {
			port = GetPortInt(pActInfo, EIP_Port);
			startSocket(port);
		} else {
			// if port has changed then close old port and start new one
			if (port != GetPortInt(pActInfo, EIP_Port)) {
				endSocket();
				port = GetPortInt(pActInfo, EIP_Port);
				startSocket(port);
			}
		}
		
		bool bResult = false;

		if (socketWorking) {
			if (ReceiveLine() != "<failed>") {
				string value = ReceiveLine().c_str();
				ActivateOutput(pActInfo, EOP_Value, value);
				bResult = true;
			}
		}

		// return false if socket error, or no message

		return bResult;
	}
};


////////////////////////////////////////////////////
////////////////////////////////////////////////////

REGISTER_FLOW_NODE("Xml:GetValue", CFlowXmlNode_GetValue);

