//
//  OrderManagementSystem.hpp
//  DBMS_Playground
//
//  Created by Thomas Ciha on 2/16/18.
//  Copyright © 2018 Thomas Ciha. All rights reserved.
//

#ifndef OrderManagementSystem_hpp
#define OrderManagementSystem_hpp

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <queue>
#include <unordered_map>
#include <vector>
#include "Order.h"
#include "ECN.hpp"
#include "C:\Users\Ethan\Oracle\SQLAPI\include\SQLAPI.h"

using namespace std;

// ==================== SOURCES ========================
// EMS (Execution Management Systems)http://www.wallstreetandtech.com/trading-technology/execution-management-systems-from-the-street-and-on-the-block-/d/d-id/1258066?
//      - An EMS is a software application that provides market access to a user with real time data, numerous algorithms and analytical tools.

// TCA - Transaction Cost Analysis - is a tool utilized by instutitional investors to analyze the cost of a trade route to determine the optimal route for execution.

// source: https://en.wikipedia.org/wiki/Transaction_cost_analysis

// Here is some additional info on how brokers process orders: https://www.sec.gov/reportspubs/investor-publications/investorpubstradexechtm.html
// ======================================================


// What does an OMS do?
//   - The order management system (OMS) places orders in a queue for processing, rebalances portfolios, documents orders and transaction history.

//It needs to take into consideration not only the price and quantity of shares, but also the type of order. It also needs to take into consideration the possibility and likelyhood of price improvement.

// It takes an order from one of the broker's clients as input and then makes a decision as to whether the order should be internalized, sent to an exchange or sent to an ECN.

// ======================================================
// Functions to add:
//   - recording transactions / order history in DBMS

// ======================================================

struct stock { // stock inventory structure used by OMS, this is used to store data from StockInventory when analyzing the opportunity to internalize
    string ticker;
    float price;
    int qty;
    stock(string t, float p, int q){
        ticker = t;
        price = p;
        qty = q;
    }
};



class OMS{
    private:
    int commission_rate = 10;           // charge per trade
    float internalization_profit = 0;   // profit resulting from internalization

    public:

    queue<Order> end_of_day; // this queue holds all orders that are meant to be executed upon market close
    queue<Order> begin_of_day;  // this queue holds all orders to be submitted upon market open

    // question: why do we need class here? I #included "ECN.hpp"??
    vector<class ECN *> available_ECNs; // Available ECNs, ECNS that the OMS is connected to

    // need a pending orders data structure to hold orders, but will also need to add a communication framework between the OMS and the ECN to notify when an order has been processed. Want very quick lookup time and a method of retrieving the order's data so we can add it to the OrderTable once it has been filled or cancelled.


    pair<bool, string> eligible_order(Order &o, SAConnection &con, long AccNum);
    vector<order_info> ProcessOrder(Order &o, SAConnection &con, long AccNum);
    pair<float, int> get_best_mkt_price(Type &o);
    // Type is the type for OfferType


    // could store all the orders in a hash table for faster lookup, but then we need to write a custom hash funciton. As of now, we have tasks that take precedent over this change.

    vector<Order> pending_orders; // this will keep track of every order that is created and filled by the ECN. We could add orders to this to be processed at a timestamp of XYZ and then would be submitted when iterating through the while loop

    void update_orders(vector<order_info> &vec, SAConnection &con, long AccNum);
    vector<Order>::iterator find_pending_order(int id);
    void Update_OrderTable(Order &o, SAConnection &con, long AccNum);
    void DisplayPendingOrders();

    order_info internalize_order(Offer &o, pair<float,int> internal_inventory, float i_price, string tick, SAConnection &con, long AccNum);
    bool internalization_profitable(Order const &o, float const i_price, const float m_price); // returns true if we can profit off internalization
    pair<float,int> get_internalization_price_and_qty(string tick, SAConnection &con, long AccNum);


    OMS(vector<class ECN *> ecns){
        for(vector<class ECN *>::iterator it = ecns.begin(); it != ecns.end(); it++)
            available_ECNs.push_back(*it);
    }

    float get_internalization_profit(){
        return internalization_profit;
    }

    //this function is just a sample function that adds an order to the DBMS -- it works!
    bool sample_SAConnectionFunction(SAConnection &con, Order &o, long AccNum, string userID){
        string order_types[] = {"L", "M", "MOC", "SOQ", "LOC", "HS", "TSD", "TSP"};
        string order_terms[] = {"AON", "GTC", "IOC", "FOK", "DAY"};
        string order_actions[] = {"Buy", "Sell", "Short", "Cover"};
        string order_statuses[] = {"Filled", "Pending", "Canceled", "Aborted"};
        string type = "Stock", order_type = order_types[o.OrderType], order_term = order_terms[o.OrderTerm];
        string order_action = order_actions[o.OrderAction], order_status = order_statuses[o.OrderStatus], time = (string) to_string(o.timestamp.count());
        SACommand update_order_table(&con,
        "INSERT INTO StockOrder VALUES(:1, :2, :3, :4, :5, :6, :7, :8,:9,:10,:11, :12) "
        );
        cout << "order info: " << endl;
        o.Print();
        cout << "Asset class:" << type << endl;
//        update_order_table << (long) o.OrderNumber << type.c_str() << o.Ticker.c_str()  << AccNum << userID.c_str() << order_type << order_term << order_action << (long) o.timestamp.count() << (long) o.Size << order_status << (double) o.fill_price;
        update_order_table.Param(1).setAsLong() = o.OrderNumber;
        update_order_table.Param(2).setAsString() = type.c_str();
        update_order_table.Param(3).setAsString() = o.Ticker.c_str();
        update_order_table.Param(4).setAsLong() = AccNum;
        update_order_table.Param(5).setAsString() = userID.c_str();
        update_order_table.Param(6).setAsString() = order_type.c_str();
        update_order_table.Param(7).setAsString() = order_term.c_str();
        update_order_table.Param(8).setAsString() = order_action.c_str();
        update_order_table.Param(9).setAsString() = time.c_str();
        update_order_table.Param(10).setAsLong() = o.Size;
        update_order_table.Param(11).setAsString() = order_status.c_str();
        update_order_table.Param(12).setAsDouble() = o.Price;
        update_order_table.Execute();
        return true;
    }



};



#endif /* OrderManagementSystem_hpp */
