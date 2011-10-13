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

#include "StdAfx.h"
#include "FlowBaseXmlNode.h"

CGraphDocManager* CGraphDocManager::m_instance = NULL;
CGraphDocManager* GDM = NULL;

////////////////////////////////////////////////////
CFlowXmlNode_Base::CFlowXmlNode_Base(SActivationInfo *pActInfo) : m_initialized(false)
{
	
}

////////////////////////////////////////////////////
CFlowXmlNode_Base::~CFlowXmlNode_Base(void)
{
	GDM->DeleteXmlDocument(m_actInfo.pGraph);
}

////////////////////////////////////////////////////
void CFlowXmlNode_Base::ProcessEvent(EFlowEvent event, SActivationInfo *pActInfo)
{
	m_actInfo = *pActInfo;
	if (eFE_Initialize == event && !m_initialized)
	{
		m_initialized = true;
		GDM->MakeXmlDocument(pActInfo->pGraph);
	}
	else if (eFE_Activate == event && IsPortActive(pActInfo, EIP_Execute))
	{
		const bool bResult = Execute(pActInfo);
		if (bResult) ActivateOutput(pActInfo, EOP_Success, true);
		else ActivateOutput(pActInfo, EOP_Fail, true);
		ActivateOutput(pActInfo, EOP_Done, bResult);
	}
}

////////////////////////////////////////////////////
////////////////////////////////////////////////////

////////////////////////////////////////////////////
CGraphDocManager::CGraphDocManager()
{

}

////////////////////////////////////////////////////
CGraphDocManager::~CGraphDocManager()
{

}

////////////////////////////////////////////////////
void CGraphDocManager::Create()
{
	if (!m_instance)
	{
		m_instance = new CGraphDocManager;
		GDM = m_instance;
	}
}

////////////////////////////////////////////////////
CGraphDocManager* CGraphDocManager::Get()
{
	return m_instance;
}

////////////////////////////////////////////////////
void CGraphDocManager::MakeXmlDocument(IFlowGraph* pGraph)
{
	GraphDocMap::iterator graph = m_GraphDocMap.find(pGraph);
	if (graph == m_GraphDocMap.end())
	{
		SXmlDocument doc;
		doc.root = doc.active = NULL;
		doc.refCount = 1;
		m_GraphDocMap[pGraph] = doc;
	}
	else
	{
		++graph->second.refCount;
	}
}

////////////////////////////////////////////////////
void CGraphDocManager::DeleteXmlDocument(IFlowGraph* pGraph)
{
	GraphDocMap::iterator graph = m_GraphDocMap.find(pGraph);
	if (graph != m_GraphDocMap.end() && --graph->second.refCount <= 0)
	{
		m_GraphDocMap.erase(graph);
	}
}

////////////////////////////////////////////////////
bool CGraphDocManager::GetXmlDocument(IFlowGraph* pGraph, SXmlDocument **document)
{
	bool bResult = false;

	GraphDocMap::iterator graph = m_GraphDocMap.find(pGraph);
	if (document && graph != m_GraphDocMap.end())
	{
		*document = &(graph->second);
		bResult = true;
	}

	return bResult;
}
