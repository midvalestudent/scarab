/////////////////////////////////////////////////////////////////////
//                                                                 //
// file: acpGarciaXMLScript.h                                      //
//                                                                 //
/////////////////////////////////////////////////////////////////////
//                                                                 //
// description: Definition of the Garcia API script primitive.     //
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


#ifndef _acpGarciaXMLScript_H_
#define _acpGarciaXMLScript_H_

#include "acpGarciaPrimitive.h"

class acpGarciaXMLScript :
  public acpGarciaPrimitive
{
  public:
	  		acpGarciaXMLScript(acpGarciaInternal* pGarcia);
  			~acpGarciaXMLScript() {}

    void		execute(acpBehavior* pBehavior);
    
    const char*		getParamHTML();

    acpBehavior*	factoryBehavior(
    			  const char* pBehaviorName,
    			  acpGarciaPrimitive* pPrimitive,
    			  const int nID,
    			  acpGarciaInternal* pGarciaInternal,
    			  acpBehaviorList* pParent = NULL);

//    acpBehavior*	factoryBehavior(acpHTMLPage& page);

  private:
    int			m_nFilenamePropIndex;
};

/////////////////////////////////////////////////////////////////////

class acpScriptBehaviorFilenameProperty : public acpBehaviorProperty
{
  public:
  				acpScriptBehaviorFilenameProperty(
  				  acpGarciaPrimitive* pPrimitive);
};

#endif // _acpGarciaXMLScript_H_