/////////////////////////////////////////////////////////////////
// Copyright (C), RenEvo Software & Designs, 2008
// FGPlugin Source File
//
// FlowBaseXmlNode.cpp
//
// Purpose: Flowgraph nodes to read/write Xml files
//
// History:
//	- 8/16/08 : File created - KAK
/////////////////////////////////////////////////////////////////

#ifndef _FLOWBASEXMLNODE_H_
#define _FLOWBASEXMLNODE_H_

#include "Nodes/G2FlowBaseNode.h"

#define ADD_BASE_INPUTS() \
	InputPortConfig_Void("Execute", _HELP("Execute Xml instruction"))

#define ADD_BASE_OUTPUTS() \
	OutputPortConfig<bool>("Success", _HELP("Called if Xml instruction is executed successfully")), \
	OutputPortConfig<bool>("Fail", _HELP("Called if Xml instruction fails")), \
	OutputPortConfig<bool>("Done", _HELP("Called when Xml instruction is carried out"))

////////////////////////////////////////////////////
class CFlowXmlNode_Base : public CFlowBaseNode
{
public:
	////////////////////////////////////////////////////
	CFlowXmlNode_Base(SActivationInfo *pActInfo);
	virtual ~CFlowXmlNode_Base(void);
	virtual void ProcessEvent(EFlowEvent event, SActivationInfo *pActInfo);

protected:
	enum EInputs
	{
		EIP_Execute,
		EIP_CustomStart,
	};

	enum EOutputs
	{
		EOP_Success,
		EOP_Fail,
		EOP_Done,
		EOP_CustomStart,
	};

	//! Overload to handle Xml execution
	virtual bool Execute(SActivationInfo *pActInfo) = 0;

private:
	SActivationInfo m_actInfo;
	bool m_initialized;
};

////////////////////////////////////////////////////
////////////////////////////////////////////////////

////////////////////////////////////////////////////
struct SXmlDocument
{
	XmlNodeRef root;
	XmlNodeRef active;
	UINT refCount;
};

class CGraphDocManager
{
private:
	CGraphDocManager();
	static CGraphDocManager *m_instance;
public:
	virtual ~CGraphDocManager();
	static void Create();
	static CGraphDocManager* Get();
	
	void MakeXmlDocument(IFlowGraph* pGraph);
	void DeleteXmlDocument(IFlowGraph* pGraph);
	bool GetXmlDocument(IFlowGraph* pGraph, SXmlDocument **document);

private:
	typedef std::map<IFlowGraph*, SXmlDocument> GraphDocMap;
	GraphDocMap m_GraphDocMap;
};
extern CGraphDocManager* GDM;

#endif //_FLOWBASEXMLNODE_H_
