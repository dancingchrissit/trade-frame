/************************************************************************
 * Copyright(c) 2009, One Unified. All rights reserved.                 *
 *                                                                      *
 * This file is provided as is WITHOUT ANY WARRANTY                     *
 *  without even the implied warranty of                                *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                *
 *                                                                      *
 * This software may not be used nor distributed without proper license *
 * agreement.                                                           *
 *                                                                      *
 * See the file LICENSE.txt for redistribution information.             *
 ************************************************************************/

#pragma once

// 2010/09/12
// At some point, make order manager responsible for constructing Order

#include <map>
#include <vector>
#include <stdexcept>

#include <OUCommon/Delegate.h>
#include <OUCommon/ManagerBase.h>

#include "KeyTypes.h"

#include "ProviderInterface.h"
#include "TradingEnumerations.h"
#include "Order.h"
#include "Execution.h"

namespace ou { // One Unified
namespace tf { // TradeFrame

//
// OrderManager
//

class ProviderInterfaceBase;

// this is a singleton so use the Instance() call from all users
class OrderManager: public ou::db::ManagerBase<OrderManager> {
public:

  typedef keytypes::idInstrument_t idInstrument_t;
  typedef Instrument::pInstrument_t pInstrument_t;

  typedef keytypes::idPosition_t idPosition_t;

  typedef keytypes::idOrder_t idOrder_t;
  typedef Order::pOrder_t pOrder_t;

  typedef keytypes::idExecution_t idExecution_t;
  typedef Execution::pExecution_t pExecution_t;

  OrderManager(void);
  ~OrderManager(void);
  pOrder_t ConstructOrder( // market order
    Instrument::pInstrument_cref instrument, 
    OrderType::enumOrderType eOrderType, OrderSide::enumOrderSide eOrderSide, 
    boost::uint32_t nOrderQuantity, 
    idPosition_t idPosition = 0
    );
  pOrder_t ConstructOrder( // limit or stop
    Instrument::pInstrument_cref instrument, 
    OrderType::enumOrderType eOrderType, OrderSide::enumOrderSide eOrderSide, 
    boost::uint32_t nOrderQuantity, double dblPrice1,  
    idPosition_t idPosition = 0
    );
  pOrder_t ConstructOrder( // limit and stop
    Instrument::pInstrument_cref instrument, 
    OrderType::enumOrderType eOrderType, OrderSide::enumOrderSide eOrderSide, 
    boost::uint32_t nOrderQuantity, double dblPrice1, double dblPrice2,
    idPosition_t idPosition = 0
    );
  void PlaceOrder( ProviderInterfaceBase* pProvider, Order::pOrder_t pOrder );
  void CancelOrder( idOrder_t nOrderId );
  void ReportCancellation( idOrder_t nOrderId );  // feedback from provider
  void ReportExecution( idOrder_t orderId, const Execution& exec );  // feedback from provider
  void ReportCommission( idOrder_t nOrderId, double dblCommission );  // feedback from provider
  void ReportErrors( idOrder_t nOrderId, OrderErrors::enumOrderErrors eError );  // feedback from provider
//  ou::Delegate<const Order &> OnOrderCompleted;

  idOrder_t CheckOrderId( idOrder_t );  // used by ibtws to sync order ids

  // need a query to find pending orders like GTC, etc

  typedef FastDelegate2<idInstrument_t,pInstrument_t&> OnOrderNeedsDetailsHandler;
  void SetOnOrderNeedsDetails( OnOrderNeedsDetailsHandler function ) {
    OnOrderNeedsDetails = function;
  }

  void AttachToSession( ou::db::Session* pSession );
  void DetachFromSession( ou::db::Session* pSession );

protected:

  typedef std::pair<idExecution_t, pExecution_t> pairExecution_t;
  typedef std::map<idExecution_t, pExecution_t> mapExecutions_t;
  typedef mapExecutions_t::iterator iterExecutions_t;
  typedef boost::shared_ptr<mapExecutions_t> pmapExecutions_t;

  struct structOrderState {
    pOrder_t pOrder;
    ProviderInterfaceBase* pProvider;
    pmapExecutions_t pmapExecutions;
    structOrderState( pOrder_t& pOrder_ )
      : pOrder( pOrder_ ), pProvider( 0 ), pmapExecutions( new mapExecutions_t ) {};
    structOrderState( pOrder_t& pOrder_, ProviderInterfaceBase* pProvider_ )
      : pOrder( pOrder_ ), pProvider( pProvider_ ), pmapExecutions( new mapExecutions_t ) {};
    ~structOrderState( void ) {
//      delete pmapExecutions;  // check that executions have been committed to db?
      // check that orders have been committed to db?
    }
  };

  typedef std::pair<idOrder_t, structOrderState> pairOrderState_t;
  typedef std::map<idOrder_t, structOrderState> mapOrders_t;  // used for active orders
  typedef mapOrders_t::iterator iterOrders_t;

private:

  //CAutoIncKey m_orderIds;  // may need to worry about multi-threading at some point in time
  // ToDo:  migrate away from this later on, may need to deal with multiple programs interacting with same 
  //  database table, and will need to auto-key from the order table instead.

  struct AutoIncKey {
    int i;
    AutoIncKey( void ) : i( 1 ) {};
    int GetNextId( void ) { return ++i; };
    void SetNextId( int i_ ) { assert( 0 < i_ ); i = i_; };
    int GetCurrentId( void ) { return i; };
  } m_orderIds;

  mapOrders_t m_mapOrders; // all orders for when checking for consistency

//  iterOrders_t LocateOrder( idOrder_t nOrderId );  // in memory or from disk
  bool LocateOrder( idOrder_t nOrderId, iterOrders_t& );  // in memory or from disk, return true if order found

  OnOrderNeedsDetailsHandler OnOrderNeedsDetails;

  void ConstructOrder( pOrder_t& pOrder );

  void HandleRegisterTables( ou::db::Session& session );
  void HandleRegisterRows( ou::db::Session& session );
  void HandlePopulateTables( ou::db::Session& session );

};

} // namespace tf
} // namespace ou
