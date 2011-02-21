/////////////////////////////////////////////////////////////////////
//                                                                 //
// file: acpGarciaDock.h                                           //
//                                                                 //
/////////////////////////////////////////////////////////////////////
//                                                                 //
// description: Definition of the Garcia API move primitive.       //
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


#ifndef _acpGarciaDock_H_
#define _acpGarciaDock_H_

#include "acpGarciaPrimitive.h"

class acpGarciaDock :
  public acpGarciaPrimitive
{
  public:
	  		acpGarciaDock(acpGarciaInternal* pGarcia);
  			~acpGarciaDock() {}

    void		execute(acpBehavior* pBehavior);
    
    const char*		getParamHTML();

  private:
    int			m_nRangePropIndex;
};


/////////////////////////////////////////////////////////////////////

class acpDockBehaviorRangeProperty : public acpBehaviorProperty
{
  public:
  				acpDockBehaviorRangeProperty(
  				  acpGarciaPrimitive* pPrimitive);
};

#endif // _acpGarciaDock_H_