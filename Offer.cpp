//
//  Offer.cpp
//  DBMS_Playground
//
//  Created by Thomas Ciha on 3/18/18.
//  Copyright © 2018 Thomas Ciha. All rights reserved.
//

#include "Offer.hpp"

#include <iostream>
#include <iomanip>


using namespace std;

//-------------  OFFER    -------------


void Offer::Print(){
    cout << "-------------------------------" << endl;
    cout << "| " << Price << setw(7) << Qty << setw(15) << timestamp.count() << "|" << endl;
    cout << "-------------------------------" << endl;
}

