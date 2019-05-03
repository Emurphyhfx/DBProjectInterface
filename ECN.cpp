//
//  ECN.cpp
//  DBMS_Playground
//
//  Created by Thomas Ciha on 2/16/18.
//  Copyright © 2018 Thomas Ciha. All rights reserved.
//

// ORDER MATCHING ALROGITHMS: PRO RATA & Time / Price Priority (FIFO)
//https://stackoverflow.com/questions/13112062/which-are-the-order-matching-algorithms-most-commonly-used-by-electronic-financi

// There are several order matching algorithms. We will implement the FIFO algorithm which is used by most nasdaq groups (source: https://docs.google.com/viewer?a=v&pid=sites&srcid=ZGVmYXVsdGRvbWFpbnxyYWplZXZyYW5qYW5zaW5naHxneDo2ZWE1YjRhYzQxZWIyYWQx)

#include <algorithm>
#include <iostream>
#include <tgmath.h>
#include <vector>
#include "ECN.hpp"
#include "Order.h"
#include "Offer.hpp"

using namespace std;

void Market::insert_front(Offer &o){
    (o.OfferType == Bid) ? current_bids.insert(current_bids.begin(), o) : current_asks.insert(current_asks.begin(), o);
}

void Market::remove_best_bid(){
    if(!current_bids.empty()) current_bids.erase(current_bids.begin());
}

void Market::remove_best_ask(){
    if(!current_asks.empty()) current_asks.erase(current_asks.begin());
}

// used for comparsion in NEW_insert_offer
bool compare_bids(Offer const b1, Offer const b2){
    if(b1.Price != b2.Price)
        return b1.Price > b2.Price;
    else
        return b1.timestamp < b2.timestamp;
}

// used for comparsion in NEW_insert_offer
bool compare_asks(Offer const a1, Offer const a2){
    if(a1.Price != a2.Price)
        return a1.Price < a2.Price;
    else
        return a1.timestamp < a2.timestamp;
}

//======== NEW_insert_offer() ================
//iteration_bounds.first = iterator of first offer that meets the condition w/respect to o.Price
//iteration_bounds.second = iterator of the first offer that does NOT meet the condition w/respect to o.Price
//==================== ====================

// want vector.front() to be the BEST BID and BEST ASK
void Market::NEW_insert_offer(Offer &o){
    pair<vector<Offer>::iterator, vector<Offer>::iterator> insertion_bounds;

    if(o.OfferType == Bid) {
        insertion_bounds = equal_range(current_bids.begin(), current_bids.end(), o, compare_bids);
        current_bids.insert(insertion_bounds.first, o);
    }

    else { // offer type = Ask
        insertion_bounds = equal_range(current_asks.begin(), current_asks.end(), o, compare_asks);
        current_asks.insert(insertion_bounds.first, o);
    }
}

void Market::Print_Offers(Type t){
    if(t == Bid){
        for(int i = 0; i < current_bids.size(); i++)
            current_bids[i].Print();
    }
    else {
        for(int i = 0; i < current_asks.size(); i++)
            current_asks[i].Print();
    }
    cout << "\n" <<  endl;
}


// ========================================= START OF ECN METHODS ========================================================================


//Status ECN::FOK_Transaction(Offer &o, Offer &best_ask){
//    bool can_be_filled = false; // if order cannot be completely filled, change order status to 'Aborted'
//
//    if(o.Price >= best_ask.Price){
//        can_be_filled = true;
//        int temp_qty = o.Qty, iter = 0;
//
//        //temp_qty = 500, o.Price = 50.05, asks(price, qty): (49.99, 300), (50.03, 1000)
//
//        // temp_qty = 200
//
//
//        while(temp_qty > 0 && iter < market.current_asks.size()){
//            if(market.current_asks[iter].Price <= o.Price)
//               temp_qty -= market.current_asks[iter].Qty;
//
//            else {
//                can_be_filled = false;
//                break;
//            }
//            iter++;
//        }
//        return Aborted;
//    }
//
//    else
//        return Aborted;
//}


//======== make_transaction() ================
//The make_transaction function is responsible for completing a transaction with a bid that has just been parsed by the ECN.
// this includes:
// - removing ask offers from current_asks if o.OfferType = Bid
// - removing bid offers from current_bids if o.OfferType = Ask
// - (maybe) recording the price at which the transaction occurred as well as the size of the transaction
//==================== ====================


//================ NEW MAKE TRANSACTION FUNCTION ================================


vector<order_info> ECN::make_transaction_ask_NEW(Offer &o){ // makes a transaction for an offer of type 'Ask'
    num_of_transactions++;
    Offer best_bid = market.get_best_bid();
    int order_qty = o.Qty; // the quantity of shares that have been filled (does not reflect it rn, but it will at the end of function call)
    float avg_price = 0; // avg price of transaction for offer o
    Status o_stat = Pending;
    vector<order_info> transaction_return_data; // holds the order_info for every bid and ask that was used to create the transaction
    bool market_order_flag = false; // flag = true if there is insufficient market liquity for market order to be filled and it should be rerouted


    if(o.OfferTerm == GTC || o.OfferTerm == Day){
        while(o.Price <= best_bid.Price && o.Qty > 0 && !market.current_bids.empty()){    //while bid can be filled by best ask
            if(o.Qty >= best_bid.Qty){ // can eliminate the best bid completely with the ask order
                avg_price += best_bid.Price * best_bid.Qty;

                if(best_bid.my_offer){ //if the best_bid was placed by my broker, create an order_info object and add it to the vector
                    transaction_return_data.push_back(order_info(best_bid.Price, best_bid.Qty, Filled, best_bid.OrderID, market_order_flag)); // see #1 in documentation
                }
                market.remove_best_bid(); //remove best bid
            }

            else{ // cannot eliminate the entire best_bid, but can fill the ask order we are processing
                avg_price += best_bid.Price * o.Qty; // multiplying by o.Qty because we are not eliminating the entire best_bid
                market.current_bids[0].Qty -= o.Qty; //reduce best bid qty

                if(best_bid.my_offer){ // see #2
                    transaction_return_data.push_back(order_info(best_bid.Price, o.Qty, Pending, best_bid.OrderID, market_order_flag));                     // Pending status with a positive quantity of shares filled signifies that the bid was partially filled
                }
            }

            o.Qty -= best_bid.Qty; // o.Qty = # shares left to be filled
            best_bid = market.get_best_bid(); // get next best bid (might be able to move this inside the first if statement above)
        }

        if(o.Qty > 0){ // see #3
            if(o.OrderType == Market)
                market_order_flag = true; // if there are still shares left to be filled, but insufficient liquidity, flag it!
            else
                market.NEW_insert_offer(o); //else insert the limit order
            order_qty -= o.Qty; // order_qty initialized to total num shares. shares filled = total num shares - shares left to be filled
        }
        else
            o_stat = Filled; // if there are no shares left to be filled, change the order status var for creating the order_info obj to 'Filled'

        avg_price /= order_qty;
        if(o.my_offer)
            transaction_return_data.push_back(order_info(avg_price, order_qty, o_stat, o.OrderID, market_order_flag)); // adding the order_info for the ask we are processing
        return transaction_return_data; // last element of transaction_return_data contains order_info for the order being processed
    }

    //    else { // o.OfferTerm == AON || IOC || FOK
    //
    //
    //    }
//    return order_info(0, 0, Aborted); // for fill or kill orders, ioc, etc.
    cout  << "ERROR ASK " << endl;
    return transaction_return_data;
}

/* make_transaction_ask_NEW(Offer &o) DOCUMENTATION

 1. has been omitted

 2. if the partially filled best bid was placed from my broker, we will need to get updated information on how many shares were filled and at what price and submit this information to the OMS. The OMS will then proceed to parse this information and update the pending order accordingly.
 3. if ask has any shares left to fill, but can't be filled at the moment, update share qty and add to current_asks, unless it's a market order - which we then flag this case.

 */


vector<order_info> ECN::make_transaction_bid_NEW(Offer &o){
    num_of_transactions++;
    Offer best_ask = market.get_best_ask();
    int order_qty = o.Qty; // the quantity of shares that have been filled
    float avg_price = 0; // avg price of transaction for offer o
    Status o_stat = Pending;
    vector<order_info> transaction_return_data; // holds the order_info for every bid and ask that was used to create the transaction
    bool market_order_flag = false; // flag = true if there is insufficient market liquity for market order to be filled and it should be rerouted


    if(o.OfferTerm == GTC || o.OfferTerm == Day){
        while(o.Price >= best_ask.Price && o.Qty > 0 && !market.current_asks.empty()){    //while bid can be filled by best ask
            if(o.Qty >= best_ask.Qty){ // can eliminate the best ask completely with the bid order
                avg_price += best_ask.Price * best_ask.Qty;

                if(best_ask.my_offer){ //if the best_ask was placed by my broker, create an order_info object and add it to the vector
                    order_info temp(best_ask.Price, -1, Filled, best_ask.OrderID, market_order_flag); // creating order_info for best_ask being filled
                    transaction_return_data.push_back(temp); //add order info to vector
                }
                market.remove_best_ask(); //remove best ask
            }

            else{ // cannot eliminate the entire best_ask, but can complete the bid order we are processing
                avg_price += best_ask.Price * o.Qty;
                market.current_asks[0].Qty -= o.Qty; //reduce best ask qty

                if(best_ask.my_offer){ //if the best_ask originated from my broker OMS, then create order_info obj
                    order_info temp(best_ask.Price, o.Qty, Pending, best_ask.OrderID, market_order_flag); // signifies that the bid was partially filled
                    transaction_return_data.push_back(temp);
                }
            }

            o.Qty -= best_ask.Qty; // o.Qty = # shares left to be filled
            best_ask = market.get_best_ask();
        }


        if(o.Qty > 0){ //if ask has any shares left to fill, but can't be filled at the moment, update share qty and add to current_asks
            if(o.OrderType == Market)
                market_order_flag = true; // if there are still shares left to be filled, but insufficient liquidity, flag it!
            else
                market.NEW_insert_offer(o); //else insert the limit order
            order_qty -= o.Qty; // order_qty initialized to total num shares. shares filled = total num shares - shares left to be filled
        }
        else
            o_stat = Filled;

        avg_price /= order_qty;
        if(o.my_offer)
            transaction_return_data.push_back(order_info(avg_price, order_qty, o_stat, o.OrderID, market_order_flag)); // adding the order_info for the ask we are processing
        return transaction_return_data; // last element of transaction_return_data contains order_info for the order being processed
    }

    // else {process FOK, IOC, etc.}
    cout << "ERROR\n\n" << endl;
    return transaction_return_data;
}

vector<order_info> ECN::ParseOffer_TEST(Offer &o){
    float threshold = 0;    // for now, the threshold = 0, this is the spread or "cut" the ECN will take for each transaction
    vector<order_info> order_info_vec;
    if(o.my_offer)
        cout << "ECN: Processing my order at " << ECN_Name << "..." << endl;

    if(o.OfferType == Ask){
        if(o.OrderType == Market){ // we need to ensure that there is sufficient market liquidity
            if(!market.current_bids.empty())
                return make_transaction_ask_NEW(o);
        }

        else if(o.OrderType == Limit){

            if(o.Price - threshold <= this->market.get_best_bid().Price && !market.current_bids.empty()) // if offer o can make a transaction happen,
                return make_transaction_ask_NEW(o);

            else
                this->market.NEW_insert_offer(o);
        }

        //if we get to this point, the order was not immediately used to create a transaction, so we return a pending order_info struct
        // the price of -1 and shares filled of 0 indicate that the order has not been filled at all yet
        if(o.my_offer)
            order_info_vec.push_back(order_info(-1, 0, Pending, o.OrderID, false));
        return order_info_vec;
    }

    else { // process Bid
        if(o.OrderType == Market){ // we need to ensure that there is sufficient market liquidity
            if(!market.current_asks.empty()) {
                return make_transaction_bid_NEW(o);
            }
        }

        else if(o.OrderType == Limit){

            if(o.Price + threshold >= this->market.get_best_ask().Price && !market.current_asks.empty()) // if offer o can make a transaction happen,
                return make_transaction_bid_NEW(o);

            else
                this->market.NEW_insert_offer(o);
        }

        //if we get to this point, the order was not immediately used to create a transaction, so we return a pending order_info struct
        // the price of -1 and shares filled of 0 indicate that the order has not been filled at all yet
        if(o.my_offer)
            order_info_vec.push_back(order_info(-1, 0, Pending, o.OrderID, false));
        return order_info_vec;
    }
}




/* ================= PARSING DATA ==================
 - Due to the inconsistent formatting of the TAQ (Trade and Quote) data we have obtained, we will initally be processing the data as follows:

 1) Event types "QUOTE BID NB" and "QUOTE BID" will be treated the same. As will "QUOTE ASK NB" and "QUOTE ASK".

 2) We will ignore "TRADE" and "TRADE NB" event types because there is no information pertaining to which offers are being cleared when the trade takes place. In addition, the "TRADE" and "TRADE NB" rows are trades at a specific exchange. Since we are initially treating the entire file as one exchange, it doesn't make sense to process these offers. NOTE: it would be interesting to record all the trade informaiton as the data is being processed by the ECN and compare the trade results from our ECN to the trade tuples in the file.

 3) We will commence the ECN's functionality by simply using the insert_offer function to handle all offers in the file.


 100.1      200     9:06

 100.1      100     9:00 //BB
 100.1      299     9:05
 100.1      500     9:08
 100.05     250     8:57
 100.03     500     8:34
*/
