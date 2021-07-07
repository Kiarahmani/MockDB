// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//

#include <thread>
#include "../utils.h"
#include "../../kv_store/include/read_response_selector.h"
#include "integer_counter.h"

#define NUM_SESSIONS 2
#define NUM_OPS 3

app_config *config = nullptr;
mockdb::kv_store<std::string, web::json::value> *store;
user u("dip", 1);

void add_user_to_store(user u) {
    store->put("user:" + std::to_string(u.id) + ":name", web::json::value(utility::conversions::to_string_t(u.name)));
}


/*
 void do_operations(shopping_cart *cart, int t_id) {
 if (t_id == 1) {
 cart->tx_start();
 cart->add_item(shoes, 2);
 cart->add_item(ball);
 cart->tx_end();
 
 cart->tx_start();
 cart->change_quantity(shoes, 3);
 cart->remove_item(shoes);
 int qt_x = cart->get_quantity(shoes);
 std::vector<std::pair<item, int>> cart_list = cart->get_cart_list();
 cart->tx_end();
 
 std::cout << "cart_list " << cart_list.size() << std::endl;
 for (auto i : cart_list)
 std::cout << i.first.name << " " << i.second << std::endl;
 
 assert_count(qt_x == 0, 0);
 assert_count(cart_list.size() == 1 && cart_list[0].first == ball &&
 cart_list[0].second == 1 || cart_list[0].second == 2, 1);
 }
 else if (t_id == 2) {
 cart->tx_start();
 cart->add_item(ball);
 int qt_x = cart->get_quantity(shoes);
 int qt_y = cart->get_quantity(ball);
 cart->tx_end();
 
 cart->tx_start();
 cart->change_quantity(shoes, 4);
 int new_qt_x = cart->get_quantity(shoes);
 cart->tx_end();
 
 std::cout << "qt x y" << qt_x << " " << qt_y << std::endl;
 
 assert_count(qt_x == 0 || qt_x == 2, 2);
 assert_count(qt_y == 1 || qt_y == 2, 3);
 if (qt_x == 2)
 assert_count(qt_y == 2, 4);
 
 assert_count(new_qt_x == 0 || new_qt_x == 4, 5);
 }
 }
 */



void do_op(integer_counter *counter, int t_id) {
    int dec_amount = 30;
    
    if (t_id == 1) {
        counter->tx_start();
        int old_val = counter->get_value (t_id);
        if (old_val > dec_amount)
            counter->dec_counter(dec_amount, t_id);
        counter->tx_end();
    }
    else if (t_id == 2) {
        counter->tx_start();
        int old_val = counter->get_value (t_id);
        if (old_val > dec_amount)
            counter->dec_counter(dec_amount, t_id);
        counter->tx_end();
    }
}





int run_iteration() {
    mockdb::read_response_selector<std::string, web::json::value> *get_next_tx;
    
    if (config->consistency_level == consistency::causal)
        get_next_tx = new mockdb::causal_read_response_selector<std::string, web::json::value>();
    else if (config->consistency_level == consistency::linear)
        get_next_tx = new mockdb::linearizable_read_response_selector<std::string, web::json::value>();
    else if (config->consistency_level == consistency::k_causal)
        get_next_tx = new mockdb::k_causal_read_response_selector<std::string, web::json::value>(2, 12);
    
    store = new mockdb::kv_store<std::string, web::json::value>(get_next_tx);
    get_next_tx->init_consistency_checker(store);
    
    integer_counter *counter = new integer_counter(u, store, config->consistency_level, 100);
    
    std::vector<std::thread> threads;
    
    
    // initialize the counter
    counter->tx_start();
    counter->set_value(50, 0);
    counter->tx_end();
    
    
    // run concurrent sessions
    for (int i = 1; i <= NUM_SESSIONS; i++) {
        threads.push_back(std::thread(do_op, counter, i));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    
    // wait for all sessions
    for (auto &t : threads){
        t.join();
    }
    
    // read the final value
    auto value = counter->get_value(0);
    std::cout << "value: ";
    std::cout << value;
    std::cout << "\n";
    
    
    delete counter;
    delete store;
    delete get_next_tx;
    
    if (value < 0)
        return true;
    else
        return false;
}


/*
 * Args:
 * num of iterations
 * consistency-level: linear, causal, k-causal
 */
int main(int argc, char **argv) {
    config = parse_command_line(argc, argv);
    int number_of_failed_cases = 0;
    for (int j = 0; j < config->iterations; j++) {
        if (config->debug)
            std::cout << "[MOCKDB::app] Iteration " << j << " start" << std::endl;
        
        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        bool res = run_iteration();
        if (res)
            number_of_failed_cases++;
        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    }
    std::cout << "number of failed cases: ";
    std::cout << number_of_failed_cases;
    std::cout << "\nEnd\n";
    delete config;
    return 0;
}
