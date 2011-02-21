/////////////////////////////////////////////////////////////////////
//                                                                 //
// file: acpXmlTag_PATCHBAY.h 		 		           //
//                                                                 //
/////////////////////////////////////////////////////////////////////
//                                                                 //
// description: Definition of XML tag class.                       //
//                                                                 //
/////////////////////////////////////////////////////////////////////
//                                                                 //
// Copyright 1994-2008. Acroname Inc.                              //
//                                                                 //
// This software is the property of Acroname Inc.  Any             //
// distribution, sale, transmission, or re-use of this code is     //
// strictly forbidden except with permission from Acroname Inc.    //
//                                                                 //
// To the full extent allowed by law, Acroname Inc. also excludes  //
// for itself and its suppliers any liability, wheither based in   //
// contract or tort (including negligence), for direct,            //
// incidental, consequential, indirect, special, or punitive       //
// damages of any kind, or for loss of revenue or profits, loss of //
// business, loss of information or data, or other financial loss  //
// arising out of or in connection with this software, even if     //
// Acroname Inc. has been advised of the possibility of such       //
// damages.                                                        //
//                                                                 //
// Acroname Inc.                                                   //
// www.acroname.com                                                //
// 720-564-0373                                                    //
//                                                                 //
/////////////////////////////////////////////////////////////////////

#ifndef _acpXmlTag_PATCHBAY_H_
#define _acpXmlTag_PATCHBAY_H_

#include "acpPkgXML.h"

#include "acpXmlTag_CONNECTION.h"


class PATCHBAY :
  public acpPkgXML
{
  public:
  			PATCHBAY(
  			  acpPkgImport* pImporter,
  			  acpPkgXML* pParent = NULL) :
  			  acpPkgXML(pImporter, pParent)
  			  {}
  			~PATCHBAY();

    acpPkgXML*		clone(
    			  acpPkgXML* pParent = NULL)
    			  { return new PATCHBAY(m_pImporter, 
    			  		        pParent); }

    bool		addTag(
    			  acpPkgXML* pTag);

    const char*  	tagName() const
  			  { return "PATCHBAY"; }

    void		traverse();

  private:
    acpList<CONNECTION>	m_connections;
};

#endif // _acpXmlTag_PATCHBAY_H_