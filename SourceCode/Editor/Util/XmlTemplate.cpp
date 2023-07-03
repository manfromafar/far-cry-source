////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001.
// -------------------------------------------------------------------------
//  File name:   XmlTemplate.cpp
//  Version:     v1.00
//  Created:     28/11/2001 by Timur.
//  Compilers:   Visual C++ 6.0
//  Description: CXmlTemplate implementation.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include <StdAfx.h>
#include "XmlTemplate.h"
#include "FileEnum.h"

#include <io.h>
//////////////////////////////////////////////////////////////////////////
// CXmlTemplate implementation
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
void CXmlTemplate::GetValues( XmlNodeRef &node, const XmlNodeRef &fromNode )
{
	assert( node != 0 && fromNode != 0 );

	for (int i = 0; i < node->getChildCount(); i++)
	{
		XmlNodeRef prop = node->getChild(i);

		if (prop->getChildCount() == 0)
		{
			CString value;
			if (fromNode->getAttr( prop->getTag(),value ))
			{
				prop->setAttr( "Value",value );
			}
		}
		else
		{
			// Have childs.
			XmlNodeRef fromNodeChild = fromNode->findChild(prop->getTag());
			if (fromNodeChild)
			{
				CXmlTemplate::GetValues( prop,fromNodeChild );
			}
		}
	}
}
	
//////////////////////////////////////////////////////////////////////////
void CXmlTemplate::SetValues( const XmlNodeRef &node,XmlNodeRef &toNode )
{
	assert( node != 0 && toNode != 0 );

	toNode->removeAllAttributes();
	toNode->removeAllChilds();
	for (int i = 0; i < node->getChildCount(); i++)
	{
		XmlNodeRef prop = node->getChild(i);
		if (prop)
		{
			if (prop->getChildCount() > 0)
			{
				XmlNodeRef childToNode = toNode->newChild(prop->getTag());
				if (childToNode)
					CXmlTemplate::SetValues( prop,childToNode );
			}
			else
			{
				CString value;
				prop->getAttr( "Value",value );
				toNode->setAttr( prop->getTag(),value );
			}
		}else
			TRACE("NULL returned from node->GetChild()");
	}
}

//////////////////////////////////////////////////////////////////////////
bool CXmlTemplate::SetValues( const XmlNodeRef &node,XmlNodeRef &toNode,const XmlNodeRef &modifiedNode )
{
	assert( node != 0 && toNode != 0 && modifiedNode != 0 );

	for (int i = 0; i < node->getChildCount(); i++)
	{
		XmlNodeRef prop = node->getChild(i);
		if (prop)
		{
			if (prop->getChildCount() > 0)
			{
				XmlNodeRef childToNode = toNode->findChild(prop->getTag());
				if (childToNode)
				{
					if (CXmlTemplate::SetValues( prop,childToNode,modifiedNode ))
						return true;
				}
			}
			else if (prop == modifiedNode)
			{
				CString value;
				prop->getAttr( "Value",value );
				toNode->setAttr( prop->getTag(),value );
				return true;
			}
		}else
			TRACE("NULL returned from node->GetChild()");
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
void CXmlTemplate::AddParam( XmlNodeRef &templ,const char *sName,bool value )
{
	XmlNodeRef param = templ->newChild(sName);
	param->setAttr( "type","Bool" );
	param->setAttr( "value",value );
}

//////////////////////////////////////////////////////////////////////////
void CXmlTemplate::AddParam( XmlNodeRef &templ,const char *sName,int value,int min,int max )
{
	XmlNodeRef param = templ->newChild(sName);
	param->setAttr( "type","Int" );
	param->setAttr( "value",value );
	param->setAttr( "min",min );
	param->setAttr( "max",max );
}

//////////////////////////////////////////////////////////////////////////
void CXmlTemplate::AddParam( XmlNodeRef &templ,const char *sName,float value,float min,float max )
{
	XmlNodeRef param = templ->newChild(sName);
	param->setAttr( "type","Float" );
	param->setAttr( "value",value );
	param->setAttr( "min",min );
	param->setAttr( "max",max );
}

//////////////////////////////////////////////////////////////////////////
void CXmlTemplate::AddParam( XmlNodeRef &templ,const char *sName,const char *sValue )
{
	XmlNodeRef param = templ->newChild(sName);
	param->setAttr( "type","String" );
	param->setAttr( "value",sValue );
}


//////////////////////////////////////////////////////////////////////////
//
// CXmlTemplateRegistry implementation
//
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
CXmlTemplateRegistry::CXmlTemplateRegistry()
{}

//////////////////////////////////////////////////////////////////////////
void CXmlTemplateRegistry::LoadTemplates( const CString &path )
{
	XmlParser parser;

	CString dir = Path::AddBackslash(path);

	std::vector<CFileUtil::FileDesc> files;
	CFileUtil::ScanDirectory( dir,"*.xml",files,false );

	for (int k = 0; k < files.size(); k++)
	{
		XmlNodeRef child;
		// Construct the full filepath of the current file
		XmlNodeRef node = parser.parse( dir + files[k].filename );
		if (node != 0 && node->isTag("Templates"))
		{
			CString name;
			for (int i = 0; i < node->getChildCount(); i++)
			{
				child = node->getChild(i);
				AddTemplate( child->getTag(),child );
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CXmlTemplateRegistry::AddTemplate( const CString &name,XmlNodeRef &tmpl )
{
	m_templates[name] = tmpl;
}


//////////////////////////////////////////////////////////////////////////
XmlNodeRef CXmlTemplateRegistry::FindTemplate( const CString &name )
{
	XmlNodeRef node;
	if (m_templates.Find( name,node )) {
		return node;
	}
	return 0;
}