// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//

#ifndef MOCK_KEY_VALUE_STORE_SHOPPING_CART_H
#define MOCK_KEY_VALUE_STORE_SHOPPING_CART_H

#include "user.h"
#include "counter.h"
#include "../utils.h"
#include "../../kv_store/include/kv_store.h"

#include <vector>
#include <cpprest/json.h>
#include <thread>

class integer_counter {
public:
    integer_counter(user u, mockdb::kv_store<std::string, web::json::value> *store,
                    consistency consistency_level, int init_val);
    void inc_counter(int val, long session_id = 1);
    void dec_counter(int val, long session_id = 1);
    int get_value(long session_id = 1);
    void set_value(int new_value, long session_id = 1);
    
    void tx_start();
    void tx_end();
    
private:
    int user_id;
    mockdb::kv_store<std::string, web::json::value> *store;
    std::mutex mtx;
    consistency consistency_level;
    

};

// constructor
integer_counter::integer_counter(user u, mockdb::kv_store<std::string, web::json::value> *store,
                                 consistency consistency_level, int init_val) {
    this->user_id = u.id;
    this->store = store;
    this->consistency_level = consistency_level;
}

void integer_counter::tx_start() {
    std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 4));
    mtx.lock();
}

void integer_counter::tx_end() {
    mtx.unlock();
}

void integer_counter::inc_counter(int inc_val, long session_id) {
    int old_val = get_value (session_id);
    set_value(old_val + inc_val, session_id);
}
void integer_counter::dec_counter(int dec_val, long session_id) {
    int old_val = get_value (session_id);
    set_value(old_val - dec_val, session_id);
}


int integer_counter::get_value(long session_id) {
    int value = 0;
    web::json::value jval;
    try {
        jval = this->store->get("counter:" + std::to_string(this->user_id), session_id);
        value = jval["value"].as_integer();
    } catch (std::exception &e) {
        // Pass
    }
    return value;
}


void integer_counter::set_value(int new_value, long session_id) {
    web::json::value jval;
    jval["value"] = web::json::value(new_value);
    this->store->put("counter:" + std::to_string(this->user_id), jval, session_id);
}



#endif //MOCK_KEY_VALUE_STORE_SHOPPING_CART_H
